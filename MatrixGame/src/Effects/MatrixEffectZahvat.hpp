// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

#define CAPTURE_CIRCLE_SIZE         6
#define CAPTURE_CIRCLE_GRAY_1       0x00ffffff
#define CAPTURE_CIRCLE_GRAY_2       0xf0808080
#define CAPTURE_CIRCLE_FLASH_PERIOD 3000

class CMatrixEffectCaptureCircles : public CMatrixEffect
{
    int m_Count = 0;
    CMatrixEffectBillboard* m_Sprites = nullptr;

    CMatrixEffectCaptureCircles(const D3DXVECTOR3& pos, float radius, float angle, int cnt);
	virtual ~CMatrixEffectCaptureCircles();

public:
    friend class CMatrixEffect;

    void UpdateData(dword color, int count);

    virtual void BeforeDraw();
    virtual void Draw();
    virtual void Tact(float step);
    virtual void Release();

    virtual int  Priority() { return MAX_EFFECT_PRIORITY; };
};