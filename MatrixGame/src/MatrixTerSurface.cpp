// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "stdafx.h"
#include "MatrixTerSurface.hpp"
#include "MatrixMap.hpp"
#include "MatrixRenderPipeline.hpp"

CBigIB*                               CTerSurface::m_BigIB;
CBigVB<CTerSurface::STerSurfVertex>*  CTerSurface::m_BigVB;
CBigVB<CTerSurface::STerSurfVertexM>* CTerSurface::m_BigVBM;
CTerSurface*                          CTerSurface::m_Surfaces;
int                                   CTerSurface::m_SurfacesCnt;

CTerSurface**                         CTerSurface::m_SurfacesDraw;
int                                   CTerSurface::m_SurfaceLeft;
int                                   CTerSurface::m_SurfaceRite;


CTerSurface::~CTerSurface()
{
    if(m_BigVB)
    {
        if(m_BigVB->DelSource(&m_VertsSource))
        {
            CBigVB<STerSurfVertex>::DestroyBigVB(m_BigVB);
            m_BigVB = nullptr;
        }

        HFree(m_VertsSource.verts, g_MatrixHeap);
    }

    if(m_BigVBM)
    {
        if(m_BigVBM->DelSource(&m_VertsSourceM))
        {
            CBigVB<STerSurfVertexM>::DestroyBigVB(m_BigVBM);
            m_BigVBM = nullptr;
        }

        HFree(m_VertsSourceM.verts, g_MatrixHeap);
    }

    if(m_BigIB)
    {
        if(m_BigIB->DelSource(&m_IdxsSource))
        {
            CBigIB::DestroyBigIB(m_BigIB);
            m_BigIB = nullptr;
        }

        HFree(m_IdxsSource.inds, g_MatrixHeap);
    }
}

void CTerSurface::AllocSurfaces(int n)
{
    m_SurfacesCnt = n;
    if(n)
    {
        m_Surfaces = (CTerSurface*)HAlloc(sizeof(CTerSurface) * n, g_MatrixHeap);
        m_SurfacesDraw = (CTerSurface**)HAlloc(sizeof(CTerSurface*) * n, g_MatrixHeap);
    }
    else
    {
        m_Surfaces = nullptr;
        m_SurfacesDraw = nullptr;
        return;
    }

    for(int i = 0; i < n; ++i) m_Surfaces[i].CTerSurface::CTerSurface();
}

void CTerSurface::ClearSurfaces()
{
    if(m_Surfaces)
    {
        for(int i = 0; i < m_SurfacesCnt; i++) m_Surfaces[i].~CTerSurface();
        HFree(m_Surfaces, g_MatrixHeap);
    }

    if(m_SurfacesDraw) HFree(m_SurfacesDraw, g_MatrixHeap);
}

//inline bool InSurf(const D3DXVECTOR2& p, const D3DXVECTOR2* points)
//{
//    return PointLineCatch(points[0], points[1], p) &&
//           PointLineCatch(points[1], points[2], p) &&
//           PointLineCatch(points[2], points[3], p) &&
//           PointLineCatch(points[3], points[0], p);
//}

void CTerSurface::Load(byte* raw)
{
    if(m_BigVB == nullptr) m_BigVB = CBigVB<STerSurfVertex>::NewBigVB(g_MatrixHeap);

    int ids = *(int*)raw; raw += sizeof(int);

    m_Index = *(int*)raw; raw += sizeof(int);
    m_Color = *(dword*)raw; raw += sizeof(dword);

    CWStr name(g_CacheHeap);
    name = g_MatrixMap->IdsGet(ids).GetStrPar(0, L"?");

    m_Tex = (CTextureManaged*)g_Cache->Get(cc_TextureManaged, name.Get());

    int par = g_MatrixMap->IdsGet(ids).GetCountPar(L"?");
    if(par > 1)
    {
        CWStr parn(g_CacheHeap), park(g_CacheHeap), parv(g_CacheHeap);
        for(int i = 1; i < par; ++i)
        {
            parn = g_MatrixMap->IdsGet(ids).GetStrPar(i, L"?");
            park = parn.GetStrPar(0, L"=");
            parv = parn.GetStrPar(1, L"=");

            if(g_Config.m_LandTexturesGloss && (PAR_TOP_TEX_GLOSS == park))
            {
                CWStr gloss_name(g_CacheHeap);
                CacheReplaceFileNameAndExt(gloss_name, name.Get(), parv.Get());
                if(!parv.IsEmpty() && CFile::FileExist(name, gloss_name.Get(), L"png~dds"))
                {
                    m_TexGloss = (CTextureManaged*)g_Cache->Get(cc_TextureManaged, gloss_name);
                    SETFLAG(m_Flags, SURFF_GLOSS);
                }
            }
        }
    }

    if(m_Color == 0xFFFFFFFF) SETFLAG(m_Flags, SURFF_WHITE);

    dword vcnt = *(dword *)raw; raw += sizeof(dword);
    dword idxsz = *(dword *)raw; raw += sizeof(dword);
    dword grpsc = *(dword *)raw; raw += sizeof(dword);

    m_DispX = *(float *)raw; raw += sizeof(float);
    m_DispY = *(float *)raw; raw += sizeof(float);

    m_VertsSource.size = sizeof(STerSurfVertex) * vcnt;
    m_VertsSource.verts = (STerSurfVertex *)HAlloc(m_VertsSource.size, g_MatrixHeap);
    for (dword i=0; i<vcnt; ++i)
    {
        m_VertsSource.verts[i].p.x = *(float *)raw; raw += sizeof(float);
        m_VertsSource.verts[i].p.y = *(float *)raw; raw += sizeof(float);
        m_VertsSource.verts[i].p.z = *(float *)raw; raw += sizeof(float);

        g_MatrixMap->GetNormal( &m_VertsSource.verts[i].n, m_VertsSource.verts[i].p.x, m_VertsSource.verts[i].p.y);

        m_VertsSource.verts[i].color = *(dword *)raw; raw += sizeof(dword);
        m_VertsSource.verts[i].tu = *(float *)raw; raw += sizeof(float);
        m_VertsSource.verts[i].tv = *(float *)raw; raw += sizeof(float);

        
        if (!FLAG(m_Flags, SURFF_WRAPY) && (m_VertsSource.verts[i].tv < 0 || m_VertsSource.verts[i].tv > 1)) SETFLAG(m_Flags, SURFF_WRAPY);
    }

    m_BigVB->AddSource(&m_VertsSource);

    m_IdxsSource.size = idxsz;
    m_IdxsSource.inds = (word *)HAlloc(idxsz, g_MatrixHeap);
    memcpy(m_IdxsSource.inds, raw, idxsz);
    raw += idxsz;

    if (m_BigIB == nullptr) m_BigIB = CBigIB::NewBigIB(g_MatrixHeap);
    m_BigIB->AddSource(&m_IdxsSource);


    for (dword i=0; i<grpsc; ++i)
    {
        dword idx = *(dword *)raw; raw += sizeof(dword);
        CMatrixMapGroup *g = g_MatrixMap->GetGroupByIndex(idx);
        ASSERT(g != nullptr);
        g->AddSurface(this);       
    }
}
void CTerSurface::LoadM(byte *raw)
{
    if (m_BigVBM == nullptr) m_BigVBM = CBigVB<STerSurfVertexM>::NewBigVB(g_MatrixHeap);

    SETFLAG(m_Flags, SURFF_MACRO);

    int ids = *(int *)raw; raw += sizeof(int);

    m_Index = *(int *)raw; raw += sizeof(int);
    m_Color = *(dword *)raw; raw += sizeof(dword);

    CWStr name(g_CacheHeap);
    name = g_MatrixMap->IdsGet(ids).GetStrPar(0, L"?");

    m_Tex = (CTextureManaged *)g_Cache->Get(cc_TextureManaged, name.Get());

    int par = g_MatrixMap->IdsGet(ids).GetCountPar(L"?");
    if(par > 1)
    {
        CWStr parn(g_CacheHeap), park(g_CacheHeap), parv(g_CacheHeap);
        for(int i = 1; i < par; ++i)
        {
            parn = g_MatrixMap->IdsGet(ids).GetStrPar(i,L"?");
            park = parn.GetStrPar(0,L"=");
            parv = parn.GetStrPar(1,L"=");

            if(g_Config.m_LandTexturesGloss && (PAR_TOP_TEX_GLOSS == park))
            {
                CWStr gloss_name(g_CacheHeap);
                CacheReplaceFileNameAndExt(gloss_name,name.Get(), parv.Get());
                if(CFile::FileExist(name,gloss_name.Get(),L"png~dds"))
                {
                    m_TexGloss = (CTextureManaged *)g_Cache->Get(cc_TextureManaged, gloss_name);
                    SETFLAG(m_Flags, SURFF_GLOSS);
                }

            }

        }

    }


    if (m_Color == 0xFFFFFFFF) SETFLAG(m_Flags, SURFF_WHITE);

    dword vcnt = *(dword *)raw; raw += sizeof(dword);
    dword idxsz = *(dword *)raw; raw += sizeof(dword);
    dword grpsc = *(dword *)raw; raw += sizeof(dword);

    m_DispX = *(float *)raw; raw += sizeof(float);
    m_DispY = *(float *)raw; raw += sizeof(float);

    m_VertsSourceM.size = sizeof(STerSurfVertexM) * vcnt;
    m_VertsSourceM.verts = (STerSurfVertexM *)HAlloc(m_VertsSourceM.size, g_MatrixHeap);
    for (dword i=0; i<vcnt; ++i)
    {
        m_VertsSourceM.verts[i].p.x = *(float *)raw; raw += sizeof(float);
        m_VertsSourceM.verts[i].p.y = *(float *)raw; raw += sizeof(float);
        m_VertsSourceM.verts[i].p.z = *(float *)raw; raw += sizeof(float);
        g_MatrixMap->GetNormal( &m_VertsSourceM.verts[i].n, m_VertsSourceM.verts[i].p.x, m_VertsSourceM.verts[i].p.y);
        m_VertsSourceM.verts[i].color = *(dword *)raw; raw += sizeof(dword);
        m_VertsSourceM.verts[i].tu = *(float *)raw; raw += sizeof(float);
        m_VertsSourceM.verts[i].tv = *(float *)raw; raw += sizeof(float);
        m_VertsSourceM.verts[i].tum = *(float *)raw; raw += sizeof(float);
        m_VertsSourceM.verts[i].tvm = *(float *)raw; raw += sizeof(float);

        if (!FLAG(m_Flags, SURFF_WRAPY) && (m_VertsSourceM.verts[i].tv < 0 || m_VertsSourceM.verts[i].tv > 1)) SETFLAG(m_Flags, SURFF_WRAPY);
    }

    m_BigVBM->AddSource(&m_VertsSourceM);

    m_IdxsSource.size = idxsz;
    m_IdxsSource.inds = (word *)HAlloc(idxsz, g_MatrixHeap);
    memcpy(m_IdxsSource.inds, raw, idxsz);
    raw += idxsz;

    if (m_BigIB == nullptr) m_BigIB = CBigIB::NewBigIB(g_MatrixHeap);
    m_BigIB->AddSource(&m_IdxsSource);

    for (dword i=0; i<grpsc; ++i)
    {
        dword idx = *(dword *)raw; raw += sizeof(dword);
        CMatrixMapGroup *g = g_MatrixMap->GetGroupByIndex(idx);
        ASSERT(g != nullptr);
        g->AddSurface(this);       
    }

}


void CTerSurface::SetFVF(void)
{
    g_D3DD->SetFVF(STerSurfVertex::FVF);
}

void CTerSurface::SetFVFM(void)
{
    g_D3DD->SetFVF(STerSurfVertexM::FVF);
}


void CTerSurface::BeforeDraw(void)
{
    if (m_DrawFrameMarker == g_MatrixMap->GetCurrentFrame()) return;
    m_DrawFrameMarker = g_MatrixMap->GetCurrentFrame();

    if (m_BigVB) m_VertsSource.Prepare(m_BigVB);
    if (m_BigVBM) m_VertsSourceM.Prepare(m_BigVBM);
    
    m_IdxsSource.Prepare(m_BigIB);

    m_Tex->Preload();
    if (m_TexGloss) m_TexGloss->Preload();



    bool no_left = (m_SurfaceLeft <= 0);
    bool no_right = (m_SurfaceRite >= m_SurfacesCnt);

    if (m_SurfaceLeft == m_SurfaceRite)
    {
        m_SurfacesDraw[m_SurfaceLeft] = this;
        ++m_SurfaceRite;
        return;
    }
    //seek index
    int idx;
    int idx0 = m_SurfaceLeft;
    int idx1 = m_SurfaceRite;
    while (true)
    {
        idx = ((idx1-idx0) >> 1) + idx0;
        
        if (m_Index < m_SurfacesDraw[idx]->m_Index)
        {
            //left
            if (idx == idx0) break;
            idx1 = idx;
        }
        else
        {
            //rite
            ++idx;
            if (idx == idx1) break;
            idx0 = idx;
        }
    }

    if (no_left && no_right)
    {
        if (idx == m_SurfaceRite) return;    // far object
        --m_SurfaceRite;
        no_right = false;
        goto insert;
    }

    if (!no_right && (idx == m_SurfaceRite))
    {
        ++m_SurfaceRite;
        m_SurfacesDraw[idx] = this;
    }
    else  if (!no_left && (idx == m_SurfaceLeft))
    {
        --m_SurfaceLeft;
        m_SurfacesDraw[idx-1] = this;
    }
    else
    {
insert:
        int lc = (idx-m_SurfaceLeft);
        int rc = (m_SurfaceRite-idx);
        bool expand_left = no_right || ((lc <= rc) && !no_left);

        if(expand_left)
        {
            memcpy(&m_SurfacesDraw[m_SurfaceLeft-1], &m_SurfacesDraw[m_SurfaceLeft], sizeof(CTerSurface *) * lc);
            --m_SurfaceLeft;
            m_SurfacesDraw[idx-1] = this;
        }
        else
        {
            memcopy_back_dword(&m_SurfacesDraw[idx+1], &m_SurfacesDraw[idx], rc);
            ++m_SurfaceRite;
            m_SurfacesDraw[idx] = this;
        }
    }



}

void CTerSurface::DrawAll(void)
{
    //g_D3DD->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

    g_D3DD->SetRenderState( D3DRS_ALPHABLENDENABLE,   TRUE );
    g_D3DD->SetRenderState( D3DRS_ZWRITEENABLE, FALSE);

    

    for(int i=m_SurfaceLeft; i<m_SurfaceRite; ++i)
    {
#ifdef _DEBUG
        if(g_MatrixMap->m_KeyDown && g_MatrixMap->m_KeyScan == KEY_0) {} else
#endif

        m_SurfacesDraw[i]->Draw();
    }

    g_D3DD->SetRenderState( D3DRS_ALPHABLENDENABLE,   FALSE );
    g_D3DD->SetRenderState( D3DRS_ZWRITEENABLE, TRUE);


    g_D3DD->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
}



void CTerSurface::Draw(void)
{
    int vbase = -1;
    int vcnt;
    if(FLAG(m_Flags, SURFF_MACRO))
    {
        if(m_BigVBM)
        {
            vbase = m_VertsSourceM.Select(m_BigVBM);
            vcnt = m_VertsSourceM.size / sizeof(STerSurfVertexM);
            SetFVFM();
        }
    }
    else
    {
        if(m_BigVB)
        {
            vbase = m_VertsSource.Select(m_BigVB);
            vcnt = m_VertsSource.size / sizeof(STerSurfVertex);
            SetFVF();
        }
    }

    if(vbase < 0) return;
    
    int ibase = m_IdxsSource.Select(m_BigIB);
    if(ibase < 0) return;

    D3DMATRIX m;
    m = g_MatrixMap->GetIdentityMatrix();
    m._41 = m_DispX;
    m._42 = m_DispY;

    ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD,&m));

    g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR, m_Color);

    int type = m_Flags & SURF_FLAG_MASK;

    //static int iii = 0;
    //if(g_MatrixMap->m_KeyDown && g_MatrixMap->m_KeyScan == KEY_F7) { --iii; g_MatrixMap->m_KeyDown = false; }
    //if(g_MatrixMap->m_KeyDown && g_MatrixMap->m_KeyScan == KEY_F8) { ++iii; g_MatrixMap->m_KeyDown = false; }


    for (int pass = 0; pass < g_Render->m_TerSurfPass[type]; ++pass)
    {
        g_Render->m_TerSurfTex[type](m_Tex,m_TexGloss, pass);
        g_Render->m_TerSurf[type](pass, FLAG(m_Flags, SURFF_WRAPY));

        //if (iii == m_Index) 
        {
            //ASSERT_DX(g_D3DD->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,vbase,0,vcnt, ibase, m_IdxsSource.size / (3 * sizeof(word))));
            ASSERT_DX(g_D3DD->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP,vbase,0,vcnt, ibase, m_IdxsSource.size / sizeof(word) - 2));
        }
    }
    g_Render->m_TerSurfClear[type]();
}
