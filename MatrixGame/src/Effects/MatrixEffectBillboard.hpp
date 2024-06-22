// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once


// billboard

#define BBOARD_FVF (D3DFVF_XYZ | D3DFVF_TEX1 )
struct SBBoardVertex
{
    D3DXVECTOR3 p = { 0.0f, 0.0f, 0.0f }; // Vertex position
    float       tu = 0.0f;
    float       tv = 0.0f; // Vertex texture coordinates
};

class CMatrixEffectBillboard : public CMatrixEffect
{
protected:
    CTextureManaged* m_Tex = nullptr;
    dword            m_Color = 0;
    dword            m_Color1 = 0;
    dword            m_Color2 = 0;
    float            m_TTL = 0.0f;
    float            m_TTL2 = 0.0f;
    float            m_Delay = 0.0f;
    float           _m_TTL = 0.0f;
    float            m_Radius1 = 0.0f;
    float            m_Radius2 = 0.0f;

    D3DXMATRIX       m_Mat;
    D3DXVECTOR3      m_Dir = { 0.0f, 0.0f, 0.0f };

    ADD_TAKT         m_AddTactHandler = nullptr;

    bool m_Intense = false;

    static D3D_VB   m_VB;
    static int      m_VB_ref;

    CMatrixEffectBillboard() = default; // does nothing
    CMatrixEffectBillboard(const D3DXVECTOR3& pos, float radius1, float radius2, dword c1, dword c2, float ttl, float delay, const wchar* tex, const D3DXVECTOR3& dir, ADD_TAKT add_tact = nullptr);
	virtual ~CMatrixEffectBillboard();

    void UpdateData();

    void SetColor(dword color1, dword color2) { m_Color1 = color1; m_Color2 = color2; UpdateData(); }
    void Tact2(float step);

    static void CreateGeometry();
    static void ReleaseGeometry();

    static bool PrepareDX();
    static void StaticInit()
    {
        m_VB = nullptr;
        m_VB_ref = 0;
    }

    static void MarkAllBuffersNoNeed()
    {
	    if(m_VB) DESTROY_VB(m_VB);
    }

public:
    static void Draw(const D3DXMATRIX& m, dword color, CTextureManaged* tex, bool intense);

    void SetColor(dword color) { m_Color = color; }

    void SetPos(const D3DXVECTOR3 &pos)
    {
        m_Mat._41 = pos.x;
        m_Mat._42 = pos.y;
        m_Mat._43 = pos.z;
    }

    friend class CMatrixEffect;
    friend class CMatrixEffectCaptureCircles;
    friend class CMatrixEffectDust;
    friend class CMatrixEffectMoveto;

    virtual void BeforeDraw() { DTRACE(); PrepareDX(); m_Tex->Preload(); };
    virtual void Draw();
    virtual void Tact(float step);
    virtual void Release();
    
    virtual int  Priority();
};

class CMatrixEffectSpritesLine : public CMatrixEffect
{
protected:
    dword       m_Color1 = 0xFFFFFFFF;
    dword       m_Color2 = 0xFFFFFFFF;
    float       m_TTL = 0.0f;
    float      _m_TTL = 0.0f;

    CSpriteSequence m_Bl;

    CMatrixEffectSpritesLine(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, float width, dword color1, dword color2, float ttl, CTextureManaged* tex) :
        CMatrixEffect(), m_Color1(color1), m_Color2(color2), m_TTL(ttl), _m_TTL(INVERT(ttl)), m_Bl(TRACE_PARAM_CALL pos0, pos1, width, color1, tex)
    {
        m_EffectType = EFFECT_SPRITES_LINE;
    }

    virtual ~CMatrixEffectSpritesLine()
    {
        m_Bl.Release();
    }

public:
    friend class CMatrixEffect;

    virtual void BeforeDraw() {};
    virtual void Draw();
    virtual void Tact(float step);
    virtual void Release();

    virtual int  Priority();    
};

#define BBS_TTL     2000
#define BBS_FADE    0.7f
#define BBS_SPEED   0.02f
#define BBS_DX      6
#define BBS_ICON_DX 12

class CMatrixEffectBillboardScore : public CMatrixEffect
{
    struct SBB
    {
        CSprite bb;
        float   disp = 0.0f;
    };

    SBB* m_bbs = nullptr;
    int  m_bbs_count = 0;

    D3DXVECTOR3 m_Pos = { 0.0f,0.0f,0.0f };
    float       m_TTL = 0.0f;
    dword       m_Color = 0;

    CMatrixEffectBillboardScore(const wchar* n, const D3DXVECTOR3& pos, dword color);

public:
    ~CMatrixEffectBillboardScore();

    friend class CMatrixEffect;

    virtual void BeforeDraw() {};
    virtual void Draw();
    virtual void Tact(float step);
    virtual void Release();

    virtual int  Priority() { return MAX_EFFECT_PRIORITY; };
};