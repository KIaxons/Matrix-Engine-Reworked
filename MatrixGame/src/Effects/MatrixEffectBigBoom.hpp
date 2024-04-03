// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once


// big boom


#define BB_FVF (D3DFVF_XYZ  | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0) )
struct SBBVertex
{
    D3DXVECTOR3 p;      // Vertex position
    //float       tu, tv; // Vertex texture coordinates
    float x,y,z;
};


//#define BB_PASS_CNT 2
//#define BB_TRI_CNT  216
//#define BB_PTS_CNT  173

#define BB_PASS_CNT 3
#define BB_TRI_CNT  1296
#define BB_PTS_CNT  1037


class CMatrixEffectBigBoom : public CMatrixEffect
{
protected:
    dword            m_Color;  // diffuse color
    float            m_TTL;
    float            m_Radius;

    float            m_k1, m_k2, m_k3;

    float           _m_TTL;

    dword   m_User = 0;
    FIRE_END_HANDLER m_Handler;
    dword   m_hitmask;
    CMatrixMapStatic * m_skip;

    SEffectHandler  m_Light;
    

    D3DXMATRIX      m_Mat;
    D3DXMATRIX      m_TMat;
   
    static D3D_VB           m_VB;
    static D3D_IB           m_IB;
    static CTextureManaged* m_Tex;
    static int              m_VB_ref;

    CMatrixEffectBigBoom(const D3DXVECTOR3 &pos, float radius, float ttl, dword hitmask, CMatrixMapStatic* skip, dword user, FIRE_END_HANDLER handler, dword light);
	virtual ~CMatrixEffectBigBoom();

    static bool PrepareDX();
    static void StaticInit()
    {
        m_VB = nullptr;
        m_IB = nullptr;
        m_Tex = nullptr;
        m_VB_ref = 0;
    }

    static void MarkAllBuffersNoNeed()
    {
	    if(m_VB) DESTROY_VB(m_VB);
        if(m_IB) DESTROY_IB(m_IB);
    }

public:
    friend class CMatrixEffect;
    friend bool BoomEnum(const D3DXVECTOR3& center, CMatrixMapStatic* ms, dword user);
    friend bool BoomEnumNaklon(const D3DXVECTOR3& center, CMatrixMapStatic* ms, dword user);

    const D3DXVECTOR3 & GetPos() const {return *(D3DXVECTOR3*)&m_Mat._41;}

    virtual void BeforeDraw();
    virtual void Draw();
    virtual void Tact(float step);
    virtual void Release();

    virtual int  Priority() {return 1000;}
};


