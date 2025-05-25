// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

//Ёффект отрисовки конуса из спрайтов

#define CONE_FVF (D3DFVF_XYZ | D3DFVF_TEX1)

struct SConeVertex
{
    D3DXVECTOR3 p = { 0.0f, 0.0f, 0.0f }; // Vertex position
    float       tu = 0.0f, tv = 0.0f;     // Vertex texture coordinates
};

#define CONE_NUM_SECTORS  7


class CMatrixEffectCone : public CMatrixEffect
{
protected:
    CTextureManaged* m_Tex = nullptr;
    float            m_TTL = 0.0f;
    float            m_Radius = 0.0f;
    float            m_Height = 0.0f;
    float            m_Angle = 0.0f;
    dword            m_Color = 0xFFFFFFFF; // diffuse color
    bool             m_Intense = false;

    float           _m_TTL = 0.0f;

    D3DXVECTOR3     m_Pos = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3     m_Dir = { 0.0f, 0.0f, 0.0f };
    D3DXMATRIX      m_Mat;

    static D3D_VB   m_VB;
    static int      m_VB_ref;

    CMatrixEffectCone(const D3DXVECTOR3& start, const D3DXVECTOR3& dir, float radius, float height, float angle, float ttl, dword color, bool intense, CTextureManaged* tex);
    virtual ~CMatrixEffectCone();

    void UpdateMatrix();

    static bool PrepareDX();
    static void StaticInit()
    {
        m_VB = nullptr;
        m_VB_ref = 0;
    }

    static void MarkAllBuffersNoNeed()
    {
	    if(IS_VB(m_VB)) 
        {
            DESTROY_VB(m_VB);
        }
    }

public:
    friend class CMatrixEffect;
    friend class CMachinegun;

    virtual void BeforeDraw() { PrepareDX(); m_Tex->Preload(); };
    virtual void Draw();
    virtual void Tact(float step);
    virtual void Release();

    virtual int  Priority() { return 10; };

    void Modify(const D3DXVECTOR3& pos, const D3DXVECTOR3& dir)
    {
        m_Pos = pos;
        m_Dir = dir;
        UpdateMatrix();
    }

    void Modify(const D3DXVECTOR3 &pos, const D3DXVECTOR3 &dir, float radius, float height, float angle)
    {
        m_Pos = pos;
        m_Dir = dir;
        m_Radius = radius;
        m_Height = height;
        m_Angle = angle;
        UpdateMatrix();
    }
};


class CMatrixEffectConeSplash : public CMatrixEffectCone
{
    float m_HeightInit = 0.0f;
    float m_RadiusInit = 0.0f;
    D3DXVECTOR3 m_PosInit = { 0.0f, 0.0f, 0.0f };

public:
    CMatrixEffectConeSplash(const D3DXVECTOR3& start, const D3DXVECTOR3& dir, float radius, float height, float angle, float ttl, dword color, bool is_intense, CTextureManaged* tex) :
        CMatrixEffectCone(start, dir, radius, height, angle, ttl, color, is_intense, tex), m_HeightInit(height), m_RadiusInit(radius), m_PosInit(start)
    {
        m_EffectType = EFFECT_SPLASH;
        CMatrixEffectConeSplash::Tact(0);
    }

    virtual void Tact(float step);
    virtual int  Priority() { return 0; };
};

