// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#ifndef SHADOW_PROJ_INCLUDE
#define SHADOW_PROJ_INCLUDE

#include "Cache.hpp"
#include "D3DControl.hpp"
#include "BigVB.hpp"
#include "BigIB.hpp"

struct SVOShadowProjVertex
{
    D3DXVECTOR3 v = { 0.0f, 0.0f, 0.0f };
    float       tu = 0.0f;
    float       tv = 0.0f;

    static const dword FVF = D3DFVF_XYZ | D3DFVF_TEX1;
};

class CVOShadowCliper : public CMain
{
public:
    CVOShadowCliper() {};
    ~CVOShadowCliper() {};

    virtual void BeforeRender() = 0;
    virtual void Render() = 0;
};

class CVOShadowProj : public CMain
{
    CBaseTexture* m_Tex = nullptr; // it can be CTexture or CTextureManaged

protected:
    static CBigVB<SVOShadowProjVertex>* m_VB;
	static CBigIB*                      m_IB;
    CHeap*                              m_Heap = nullptr;
    SBigVBSource<SVOShadowProjVertex>   m_preVB;
    SBigIBSource                        m_preIB;
    float m_DX = 0.0f;
    float m_DY = 0.0f;
    int   m_TriCnt = 0;
    int   m_VertCnt = 0;

public:

    static void StaticInit()
    {
        m_IB = nullptr;
        m_VB = nullptr;
    }

    static void MarkAllBuffersNoNeed() { if(m_VB) m_VB->ReleaseBuffers(); if(m_IB) m_IB->ReleaseBuffers(); }

    CVOShadowProj(CHeap* heap);
    ~CVOShadowProj();

    static void BeforeRenderAll()
    {
        if(m_IB) m_IB->BeforeDraw();
        if(m_VB) m_VB->BeforeDraw();
        ASSERT_DX(g_D3DD->SetFVF(SVOShadowProjVertex::FVF));
    }

    CBaseTexture* GetTexture() { return m_Tex; }
    void        SetTexture(CBaseTexture* tex) { m_Tex = tex; }
    bool        IsProjected() const { return m_preVB.verts != nullptr; }
    void        DX_Prepare() { m_preVB.Prepare(m_VB); m_preIB.Prepare(m_IB); };
    void        DX_Free() { m_preVB.MarkNoNeed(m_VB); m_preIB.MarkNoNeed(m_IB); }

    float       GetDX() const { return m_DX; }
    float       GetDY() const { return m_DY; }

    void DestroyTexture();
    
    void Prepare(float dx, float dy, SVOShadowProjVertex* verts, int vertscnt, word* idxs, int idxscnt);
    void BeforeRender()
    {
        if(m_Tex)
        {
            if(m_Tex->IsTextureManaged()) 
            {
                ((CTextureManaged*)m_Tex)->Preload();
            }
            else
            {
                ((CTexture*)m_Tex)->Preload();
            }
        }

        DX_Prepare();
    }

    void RenderMin();
    void Render()
    {
        if(!m_Tex) ERROR_S(L"Oops, texture is nullptr in shadow rendering.");
        
        if(m_Tex->IsTextureManaged())
        {
            g_D3DD->SetTexture(0, ((CTextureManaged*)m_Tex)->Tex());
        }
        else
        {
            g_D3DD->SetTexture(0, ((CTexture*)m_Tex)->Tex());
        }

        RenderMin();
    }

    void RenderCustom(IDirect3DTexture9* tex)
    {
	    g_D3DD->SetTexture(0, tex);
        RenderMin();
    }
};

struct SProjData
{
    D3DXVECTOR3 vpos, vx, vy, vz;
};

#endif