// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once


// selection
#define SEL_COLOR_CHANGE_TIME 200
#define SEL_SPEED       0.1f
#define SEL_BLUR_CNT    1
#define SEL_SIZE        1.5f
#define SEL_PP_DIST     5.5f

//___________________________________________________________________________________________________
/////////////////////////////////////////////////////////////////////////////////////////////////////

struct SSelPoint
{
    CSprite     m_Blur[SEL_BLUR_CNT];
    D3DXVECTOR3 m_Pos[SEL_BLUR_CNT];
    //D3DXVECTOR3 m_Pos = { 0.0f, 0.0f, 0.0f };

    ~SSelPoint()
    {
        for(int i = 0; i < SEL_BLUR_CNT; ++i) m_Blur[i].Release();
    }
};

class CMatrixEffectSelection : public CMatrixEffect
{
    D3DXVECTOR3 m_Pos = { 0.0f, 0.0f, 0.0f };
    float       m_Radius = 0.0f;
    float       m_InitRadius = 0.0f;

    dword       m_Color_current = 0;
    dword       m_ColorTo = 0;
    dword       m_ColorFrom = 0;
    float       m_ColorTime = 0.0f;

    SSelPoint*  m_Points = nullptr;
    int         m_SelCnt = 0;

    CMatrixEffectSelection(const D3DXVECTOR3 &pos, float r, dword color);
	virtual ~CMatrixEffectSelection();

    void UpdatePositions();

public:
    friend class CMatrixEffect;

    virtual void BeforeDraw();
    virtual void Draw();
    virtual void Tact(float step);
    virtual void Release();

    virtual int  Priority() { return MAX_EFFECT_PRIORITY; };

    void SetPos(const D3DXVECTOR3& pos) { m_Pos = pos; UpdatePositions(); }
    void SetRadius(float r) { m_Radius = r; }

    void Kill() { SETFLAG(m_Flags, SELF_KIP); }
    void SetColor(dword color) { m_ColorFrom = m_Color_current; m_ColorTo = color; m_ColorTime = SEL_COLOR_CHANGE_TIME; }
    void SetAlpha(byte a) { m_ColorFrom = m_Color_current; m_ColorTo = (m_Color_current & 0x00FFFFFF) | (a << 24); m_ColorTime = SEL_COLOR_CHANGE_TIME; }
};