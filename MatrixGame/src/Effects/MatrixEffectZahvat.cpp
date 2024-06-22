// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "../stdafx.h"
#include "MatrixEffect.hpp"
#include "../MatrixMap.hpp"
#include <math.h>

CMatrixEffectCaptureCircles::CMatrixEffectCaptureCircles(const D3DXVECTOR3& pos, float radius, float angle, int cnt) : m_Count(cnt)
{
    m_EffectType = EFFECT_CAPTURE_CIRCLES;

    m_Sprites = (CMatrixEffectBillboard*)HAlloc(sizeof(CMatrixEffectBillboard) * cnt, m_Heap);

    float da = float(2 * M_PI / double(cnt));
    float s, c;

    CWStr tex_path = g_CacheData->ParPathGet(CAPTURE_CIRCLE_TEXTURE_PATH);
    for(int i = 0; i < m_Count; ++i)
    {
        SinCos(angle, &s, &c);
        m_Sprites[i].CMatrixEffectBillboard::CMatrixEffectBillboard(pos + D3DXVECTOR3(c * radius, s * radius, 0), CAPTURE_CIRCLE_SIZE, CAPTURE_CIRCLE_SIZE, CAPTURE_CIRCLE_GRAY_1, CAPTURE_CIRCLE_GRAY_2, CAPTURE_CIRCLE_FLASH_PERIOD, 0, tex_path, D3DXVECTOR3(1.0f, 0.0f, 0.0f));
        m_Sprites[i].m_Intense = false;
        angle += da;
    }

    UpdateData(0, 0);
}

CMatrixEffectCaptureCircles::~CMatrixEffectCaptureCircles()
{
    for(int i = 0; i < m_Count; ++i) m_Sprites[i].~CMatrixEffectBillboard();
    HFree(m_Sprites, m_Heap);
}

void CMatrixEffectCaptureCircles::UpdateData(dword color, int count)
{
    ASSERT(count <= m_Count);
    int count1 = m_Count - count;
    int i = 0;

    while(count-- > 0)
    {
        m_Sprites[i++].SetColor(color, CAPTURE_CIRCLE_GRAY_2);
    }
    while(count1-- > 0)
    {
        m_Sprites[i++].SetColor(CAPTURE_CIRCLE_GRAY_1, CAPTURE_CIRCLE_GRAY_2);
    }
}

void CMatrixEffectCaptureCircles::BeforeDraw()
{
    if(m_Count > 0) m_Sprites[0].BeforeDraw();
}

void CMatrixEffectCaptureCircles::Draw()
{
    for(int i = 0; i < m_Count; ++i) m_Sprites[i].Draw();
}

void CMatrixEffectCaptureCircles::Tact(float step)
{
    //for(int i = 0; i < m_Count; ++i) m_Sprites[i].Tact2(step);
}

void CMatrixEffectCaptureCircles::Release()
{
    SetDIP();
    HDelete(CMatrixEffectCaptureCircles, this, m_Heap);
}