// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "stdafx.h"
#include "CBillboard.hpp"

CSprite* CSprite::m_FirstIntense;
CSprite* CSprite::sprites[MAX_SPRITES];
int      CSprite::sprites_left;
int      CSprite::sprites_right;

CTextureManaged* CSprite::m_SortableTex;
//CTextureManaged* CSprite::m_SingleTex;

D3D_VB CSprite::m_VB;
D3D_IB CSprite::m_IB;

//Это всё какой-то говнокод, разобраться не смогли
void CSprite::SortEndDraw(const D3DXMATRIX& i_view, const D3DXVECTOR3& cam_pos)
{
    if(m_VB == nullptr || m_IB == nullptr) Init();

    int ns = 0;
    int ni = 0;
    SSpriteVertex* vb;

    if(sprites_left < sprites_right)
    {
        ns = (sprites_right - sprites_left);

        LOCK_VB_DYNAMIC(m_VB, &vb);

        for(int i = sprites_left; i < sprites_right; ++i)
        {
            sprites[i]->UpdateVBSlot(vb, i_view);
            vb += 4;
        }
    }

    struct SDrawSpriteGroup
    {
        int cntv = 0;    // vertices
        int cntt = 0;    // triangles
        //int vbase = 0; // no need, always 0
        int ibase = 0;
        int min_idx = 0;
        CTextureManaged* tex = nullptr;
    };

    SDrawSpriteGroup groups[MAX_SPRITE_TEX_GROUPS];
    int groupscnt = 0;

    if(m_FirstIntense || CSpriteSequence::m_First)
    {
        int ost = (MAX_SPRITES * 2) - ns;

        if(!ns)
        {
            // lock VB/IB if not locked yet
            LOCK_VB_DYNAMIC(m_VB, &vb);
        }

        CSprite* cur = m_FirstIntense;

        int n = 0, minvi = ns * 4;

        groups[0].tex = nullptr;

        int ibt_ = ns * 6;
        while(cur)
        {
            cur->UpdateVBSlot(vb, i_view);
            vb += 4;

            if(groups[groupscnt].tex == nullptr)
            {
                // new group
                groups[groupscnt].tex = cur->m_TexSingle;
                groups[groupscnt].min_idx = minvi;

                //groups[groupscnt].vbase = base;
                groups[groupscnt].ibase = ibt_;
            }

            ++n;
            ++ni;
            minvi += 4;
            ibt_ += 6;
            --ost;

            if(cur->m_Next == cur->m_NextTex)
            {
                // end of group
                groups[groupscnt].cntv = n * 4;
                groups[groupscnt].cntt = n * 2;
                if(++groupscnt >= MAX_SPRITE_TEX_GROUPS) break;
                groups[groupscnt].tex = nullptr;

                n = 0;
            }

            if(ost <= 0) break;

            cur = cur->m_Next;
        };

        if(groupscnt < MAX_SPRITE_TEX_GROUPS && ost > 0)
        {
            ASSERT(n == 0);

            CSpriteSequence* bl = CSpriteSequence::m_First;
            while(bl != nullptr)
            {
                bl->UpdateVBSlot(vb, cam_pos);
                vb += 4;

                if(groups[groupscnt].tex == nullptr)
                {
                    // new group
                    groups[groupscnt].tex = bl->m_Tex;
                    groups[groupscnt].min_idx = minvi;

                    //groups[groupscnt].vbase = base;
                    groups[groupscnt].ibase = ibt_;
                }

                ++n;
                ++ni;
                minvi += 4;
                ibt_ += 6;
                --ost;

                if(bl->m_Next == bl->m_NextTex)
                {
                    // end of group
                    groups[groupscnt].cntv = n * 4;
                    groups[groupscnt].cntt = n * 2;
                    if(++groupscnt >= MAX_SPRITE_TEX_GROUPS) break;
                    groups[groupscnt].tex = nullptr;

                    n = 0;
                }

                if(ost <= 0) break;

                bl = bl->m_Next;
            }

            ASSERT((ost > 0 && n == 0) || ost <= 0);
        }

        CSpriteSequence::m_First = nullptr;
        UNLOCK_VB(m_VB);
    }
    else
    {
        if(ns > 0)
        {
            UNLOCK_VB(m_VB);
        }
    }

    //draw sorted
    if(ns > 0)
    {
        ASSERT_DX(g_D3DD->SetFVF(SPRITE_FVF));

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF, 0));

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_LIGHTING, FALSE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));

        ASSERT_DX(g_D3DD->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE));
        ASSERT_DX(g_D3DD->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE));
        ASSERT_DX(g_D3DD->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0));
        ASSERT_DX(g_D3DD->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1));

        ASSERT_DX(g_D3DD->SetTextureStageState(0, D3DTSS_RESULTARG, D3DTA_CURRENT));
        ASSERT_DX(g_D3DD->SetTextureStageState(1, D3DTSS_RESULTARG, D3DTA_CURRENT));

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));

        SetColorOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
        SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
        SetColorOpDisable(1);

        ASSERT_DX(g_D3DD->SetStreamSource(0, GET_VB(m_VB), 0, sizeof(SSpriteVertex)));
        ASSERT_DX(g_D3DD->SetIndices(GET_IB(m_IB)));

        ASSERT_DX(g_D3DD->SetTexture(0, m_SortableTex->Tex()));

        ASSERT_DX(g_D3DD->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, ns * 4, 0, ns * 2));
    }

    if(ni > 0)
    {
        ASSERT_DX(g_D3DD->SetFVF(SPRITE_FVF));

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF, 0));

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));

        SetColorOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
        SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
        SetColorOpDisable(1);

        g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

        g_D3DD->SetStreamSource(0, GET_VB(m_VB), 0, sizeof(SSpriteVertex));
        ASSERT_DX(g_D3DD->SetIndices(GET_IB(m_IB)));

        for(int i = 0; i < groupscnt; ++i)
        {
            groups[i].tex->Preload();
            ASSERT_DX(g_D3DD->SetTexture(0, groups[i].tex->Tex()));
            ASSERT_DX(g_D3DD->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, groups[i].min_idx, groups[i].cntv, groups[i].ibase, groups[i].cntt));
        }
    }

    g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE));
    g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF, 0x08));

    //m_First = nullptr;
    sprites_left = MAX_SPRITES >> 1;
    sprites_right = MAX_SPRITES >> 1;
    //m_Root = nullptr;
    m_FirstIntense = nullptr;
}

void CSprite::SortIntense(void)
{
    CSprite* f = m_FirstIntense;

    while(f)
    {
        if(f->m_TexSingle == this->m_TexSingle)
        {
            m_Next = f->m_Next;
            m_NextTex = f->m_NextTex;
            f->m_Next = this;
            return;
        }
        f = f->m_NextTex;
    }

    // no with such tex
    m_Next = m_FirstIntense;
    m_NextTex = m_FirstIntense;
    m_FirstIntense = this;
}

void CSprite::Sort(const D3DXMATRIX& sort)
{
    if(IsSingleBrightTexture())
    {
        SortIntense();
        return;
    }

    bool no_left = (sprites_left <= 0);
    bool no_right = (sprites_right >= MAX_SPRITES - 1);
    if(no_left && no_right) return;

    m_Z = int((sort._13 * m_Pos.x + sort._23 * m_Pos.y + sort._33 * m_Pos.z + sort._43) * 256.0);

    if(sprites_left == sprites_right)
    {
        sprites[sprites_left] = this;
        ++sprites_right;
        return;
    }

    //seek index
    int idx;
    int idx0 = sprites_left;
    int idx1 = sprites_right;
    while(true)
    {
        idx = ((idx1 - idx0) >> 1) + idx0;

        if(sprites[idx]->m_Z < m_Z)
        {
            //left
            if(idx == idx0) break;
            idx1 = idx;
        }
        else
        {
            //right
            ++idx;
            if(idx == idx1) break;
            idx0 = idx;
        }
    }

    if(!no_right && (idx == sprites_right))
    {
        ++sprites_right;
        sprites[idx] = this;
    }
    else  if(!no_left && (idx == sprites_left))
    {
        --sprites_left;
        sprites[idx - 1] = this;
    }
    else
    {
        int lc = (idx - sprites_left);
        int rc = (sprites_right - idx);
        bool expand_left = no_right || ((lc <= rc) && !no_left);

        if(expand_left)
        {
            memcpy(&sprites[sprites_left - 1], &sprites[sprites_left], sizeof(CSprite*) * lc);
            --sprites_left;
            sprites[idx - 1] = this;
        }
        else
        {
            memcopy_back_dword(&sprites[idx + 1], &sprites[idx], rc);
            ++sprites_right;
            sprites[idx] = this;
        }
    }
}

void CSprite::Init()
{
    if(m_VB == nullptr)
    {
        CREATE_VB_DYNAMIC(MAX_SPRITES * 2 * 4 * sizeof(SSpriteVertex), SPRITE_FVF, m_VB);
    }

    if(m_IB == nullptr)
    {
        CREATE_IB16(MAX_SPRITES * 6, m_IB);

        if(m_IB != nullptr)
        {
            word* p;
            LOCK_IB(m_IB, &p);
            for(int i = 0; i < MAX_SPRITES; ++i)
            {
                dword vol = i * 4;
                *(dword*)p = vol | ((vol + 1) << 16);
                *(dword*)(p + 2) = (vol + 2) | ((vol + 1) << 16);
                *(dword*)(p + 4) = (vol + 3) | ((vol + 2) << 16);

                p += 6;
            }
            UNLOCK_IB(m_IB);
        }
    }
}

void CSprite::Deinit()
{
    if(m_VB != nullptr) DESTROY_VB(m_VB);
    if(m_IB != nullptr) DESTROY_IB(m_IB);
}

CSprite::CSprite(TRACE_PARAM_DEF const D3DXVECTOR3& pos, float scale, float angle, dword color, const SSpriteTexture* tex) : //For merged tex
    m_Pos(pos), m_Scale(scale), m_Color(color), m_Tex(tex)
{
    SetSingleBrightTexture(false);
    SetAngle(angle, 0.0f, 0.0f);
}

CSprite::CSprite(TRACE_PARAM_DEF const D3DXVECTOR3& pos, float scale, float angle, dword color, CTextureManaged* tex) : //For single bright tex
    m_Pos(pos), m_Scale(scale), m_Color(color), m_TexSingle(tex)
{
    SetSingleBrightTexture(true);
    SetAngle(angle, 0.0f, 0.0f);
}

void CSprite::UpdateVBSlot(SSpriteVertex* vb, const D3DXMATRIX& i_view)
{
    static const D3DXVECTOR3 base[4] =
    {
        D3DXVECTOR3(-1.0f, -1.0f, 0.0f),
        D3DXVECTOR3(-1.0f,  1.0f, 0.0f),
        D3DXVECTOR3( 1.0f, -1.0f, 0.0f),
        D3DXVECTOR3( 1.0f,  1.0f, 0.0f),
    };

    float r11 = m_Rot._11 * m_Scale;
    float r12 = m_Rot._12 * m_Scale;
    float r21 = m_Rot._21 * m_Scale;
    float r22 = m_Rot._22 * m_Scale;

    D3DXMATRIX m(

        (r11 * i_view._11 + r12 * i_view._21),
        (r11 * i_view._12 + r12 * i_view._22),
        (r11 * i_view._13 + r12 * i_view._23),
        0,

        (r21 * i_view._11 + r22 * i_view._21),
        (r21 * i_view._12 + r22 * i_view._22),
        (r21 * i_view._13 + r22 * i_view._23),
        0,

        (i_view._31) * m_Scale,
        (i_view._32) * m_Scale,
        (i_view._33) * m_Scale,
        0,

        m_Rot._41 * i_view._11 + m_Rot._42 * i_view._21 + m_Pos.x,
        m_Rot._41 * i_view._12 + m_Rot._42 * i_view._22 + m_Pos.y,
        m_Rot._41 * i_view._13 + m_Rot._42 * i_view._23 + m_Pos.z,
        1);

    D3DXVec3TransformCoordArray(&vb->p, sizeof(SSpriteVertex), base, sizeof(D3DXVECTOR3), &m, 4);

    if(IsSingleBrightTexture())
    {
        vb->color = m_Color;
        vb->tu = 0;
        vb->tv = 1;

        ++vb;

        vb->color = m_Color;
        vb->tu = 0;
        vb->tv = 0;

        ++vb;

        vb->color = m_Color;
        vb->tu = 1;
        vb->tv = 1;

        ++vb;

        vb->color = m_Color;
        vb->tu = 1;
        vb->tv = 0;
    }
    else
    {
        vb->color = m_Color;
        vb->tu = m_Tex->tu0;
        vb->tv = m_Tex->tv1;

        ++vb;

        vb->color = m_Color;
        vb->tu = m_Tex->tu0;
        vb->tv = m_Tex->tv0;

        ++vb;

        vb->color = m_Color;
        vb->tu = m_Tex->tu1;
        vb->tv = m_Tex->tv1;

        ++vb;

        vb->color = m_Color;
        vb->tu = m_Tex->tu1;
        vb->tv = m_Tex->tv0;
    }
}

void CSprite::DrawNow(const D3DXMATRIX& i_view)
{
    if(!IsSingleBrightTexture()) return;

    ASSERT_DX(g_D3DD->SetFVF(SPRITE_FVF));

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF, 0));

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_LIGHTING, FALSE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));

    ASSERT_DX(g_D3DD->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE));
    ASSERT_DX(g_D3DD->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE));
    ASSERT_DX(g_D3DD->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0));
    ASSERT_DX(g_D3DD->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1));

    ASSERT_DX(g_D3DD->SetTextureStageState(0, D3DTSS_RESULTARG, D3DTA_CURRENT));
    ASSERT_DX(g_D3DD->SetTextureStageState(1, D3DTSS_RESULTARG, D3DTA_CURRENT));

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));

    g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    SetColorOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
    SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
    SetColorOpDisable(1);

    SSpriteVertex vb[4];

    UpdateVBSlot(vb, i_view);

    m_TexSingle->Preload();
    g_D3DD->SetTexture(0, m_TexSingle->Tex());

    D3DXMATRIX wrld;
    D3DXMatrixIdentity(&wrld);
    g_D3DD->SetTransform(D3DTS_WORLD, &wrld);

    g_D3DD->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &vb, sizeof(SSpriteVertex));

    g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE));
    g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

CSpriteSequence* CSpriteSequence::m_First;

void CSpriteSequence::UpdateVBSlot(SSpriteVertex* vb, const D3DXVECTOR3& cam_pos)
{
    static const D3DXVECTOR3 base[4] = 
    {
        D3DXVECTOR3(0.0f,  0.5f, 0.0f),
        D3DXVECTOR3(0.0f, -0.5f, 0.0f),
        D3DXVECTOR3(1.0f,  0.5f, 0.0f),
        D3DXVECTOR3(1.0f, -0.5f, 0.0f),
    };

    D3DXMATRIX m_Mat;

    D3DXVECTOR3 dir(m_Pos1 - m_Pos0);
    D3DXVECTOR3 view_dir;
    if(m_FaceToCamLink) //Обновляем ориентацию спрайта относительно положения камеры
    {
        view_dir = { cam_pos.x - (m_Pos0.x + m_Pos1.x) * 0.5f,
                     cam_pos.y - (m_Pos0.y + m_Pos1.y) * 0.5f,
                     cam_pos.z - (m_Pos0.z + m_Pos1.z) * 0.5f };
    }
    else //Не обновляем ориентацию спрайта относительно положения камеры
    {
        view_dir = { (m_Pos0.x + m_Pos1.x) * 0.5f,
                     (m_Pos0.y + m_Pos1.y) * 0.5f,
                     (m_Pos0.z + m_Pos1.z) * 0.5f };
    }

    m_Mat._11 = dir.x; //x1
    m_Mat._12 = dir.y; //y1
    m_Mat._13 = dir.z; //z1
    m_Mat._14 = 0.0f;

    m_Mat._31 = view_dir.x; //x2
    m_Mat._32 = view_dir.y; //y2 
    m_Mat._33 = view_dir.z; //z2 
    m_Mat._34 = 0.0f;

    D3DXVECTOR3 left( (m_Mat._12 * m_Mat._33 - m_Mat._13 * m_Mat._32),
                      (m_Mat._13 * m_Mat._31 - m_Mat._11 * m_Mat._33),
                      (m_Mat._11 * m_Mat._32 - m_Mat._12 * m_Mat._31) );

    D3DXVec3Normalize(&left, &left);

    m_Mat._21 = left.x * m_Width;
    m_Mat._22 = left.y * m_Width;
    m_Mat._23 = left.z * m_Width;
    m_Mat._24 = 0;

    m_Mat._41 = m_Pos0.x;
    m_Mat._42 = m_Pos0.y;
    m_Mat._43 = m_Pos0.z;
    m_Mat._44 = 1;

    D3DXVec3TransformCoordArray(&vb->p, sizeof(SSpriteVertex), base, sizeof(D3DXVECTOR3), &m_Mat, 4);

    vb->color = m_Color;
    vb->tu = 0;
    vb->tv = 1;

    ++vb;

    vb->color = m_Color;
    vb->tu = 0;
    vb->tv = 0;

    ++vb;

    vb->color = m_Color;
    vb->tu = 1;
    vb->tv = 1;

    ++vb;

    vb->color = m_Color;
    vb->tu = 1;
    vb->tv = 0;
}

void CSpriteSequence::DrawNow(const D3DXVECTOR3& cam_pos)
{
    ASSERT_DX(g_D3DD->SetFVF(SPRITE_FVF));

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));

    g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    SetColorOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
    SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
    SetColorOpDisable(1);

    SSpriteVertex vb[4];

    UpdateVBSlot(vb, cam_pos);

    m_Tex->Preload();
    g_D3DD->SetTexture(0, m_Tex->Tex());

    D3DXMATRIX wrld;
    D3DXMatrixIdentity(&wrld);
    g_D3DD->SetTransform(D3DTS_WORLD, &wrld);

    g_D3DD->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &vb, sizeof(SSpriteVertex));

    g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE));
    g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));
}

void CSpriteSequence::AddToDrawQueue()
{
    CSpriteSequence* f = m_First;

    while(f)
    {
        if(f->m_Tex == this->m_Tex)
        {
            m_Next = f->m_Next;
            m_NextTex = f->m_NextTex;
            f->m_Next = this;
            return;
        }
        f = f->m_NextTex;
    }

    // no with such tex
    m_Next = m_First;
    m_NextTex = m_First;
    m_First = this;
}