// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "../stdafx.h"
#include "MatrixEffect.hpp"
#include "../MatrixMap.hpp"
#include <math.h>


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CMatrixEffectSelection::CMatrixEffectSelection(const D3DXVECTOR3 &pos, float r, dword color):
CMatrixEffect(), m_Pos(pos), m_Radius(r), m_InitRadius(r)
{
DTRACE();
    
    m_EffectType = EFFECT_SELECTION;

    m_Color_current = 0;

    m_SelCnt = Float2Int(M_PI_MUL(2 * r) / SEL_PP_DIST);
    if (m_SelCnt < 10) m_SelCnt = 10;
    m_Points = (SSelPoint*)HAlloc(sizeof(SSelPoint) * m_SelCnt, m_Heap);

    float da = M_PI_MUL(2.0 / float(m_SelCnt));

    for(int i = 0; i < m_SelCnt; ++i)
    {
        float s, c;
        D3DXVECTOR3 cpos;
        SinCos(float(i) * da, &s, &c);
        cpos.x = m_Pos.x + s * r;
        cpos.y = m_Pos.y + c * r;
        cpos.z = m_Pos.z;

        float sz = 1;
        for(int j = 0; j < SEL_BLUR_CNT; ++j)
        {
            if(m_SpriteTextures[SPR_SELECTION_PART].IsSingleBrightTexture()) m_Points[i].m_Blur[j].CSprite::CSprite(TRACE_PARAM_CALL cpos, SEL_SIZE * sz, 0, color, m_SpriteTextures[SPR_SELECTION_PART].tex);
            else m_Points[i].m_Blur[j].CSprite::CSprite(TRACE_PARAM_CALL cpos, SEL_SIZE * sz, 0, color, &m_SpriteTextures[SPR_SELECTION_PART].spr_tex);

            //m_Points[i].m_Blur[j].SetSingleBrightTexture(true);
            sz *= 0.96f;

            SinCos(float(i) * da - (j * M_PI_MUL(2.0 / (float(m_SelCnt) * SEL_BLUR_CNT))), &s, &c);

            m_Points[i].m_Pos[j] = D3DXVECTOR3(s * r, c * r, 0);
        }
    }

    SetColor(color);

}
CMatrixEffectSelection::~CMatrixEffectSelection()
{
    for(int i = 0; i<m_SelCnt; ++i)
    {
        m_Points[i].~SSelPoint();
    }
    HFree(m_Points, m_Heap);
}


void CMatrixEffectSelection::UpdatePositions(void)
{
    for(int i = 0; i < m_SelCnt; ++i)
    {
        for(int j = 0; j < SEL_BLUR_CNT; ++j) m_Points[i].m_Blur[j].SetPos(m_Pos + m_Points[i].m_Pos[j]);
    }
}
void CMatrixEffectSelection::BeforeDraw(void)
{
}

void CMatrixEffectSelection::Draw(void)
{
DTRACE();

    for(int i = 0; i < m_SelCnt; ++i)
    {
        for(int j = 0; j < SEL_BLUR_CNT; ++j) m_Points[i].m_Blur[j].Sort(g_MatrixMap->m_Camera.GetViewMatrix());
    }
}
void CMatrixEffectSelection::Tact(float step)
{
DTRACE();

    float dtime = (0.1f * step);
    //if (dtime > 1) dtime = 1;

    if (m_ColorTime >= 0)
    {
        m_Color_current = LIC(m_ColorTo, m_ColorFrom, m_ColorTime * INVERT(SEL_COLOR_CHANGE_TIME));
        m_ColorTime -= step;
        if (m_ColorTime < 0)
        {
            m_Color_current = m_ColorTo;
        }
    }

    for(int i = 0; i < m_SelCnt; ++i)
    {
        D3DXVECTOR3 dir = { 0.0f, 0.0f, 0.0f };
        for(int j = 0; j < m_SelCnt; ++j)
        {
            if(i == j) continue;
            D3DXVECTOR3 dir0;
            D3DXVECTOR3 temp = m_Points[i].m_Pos[0] - m_Points[j].m_Pos[0];
            D3DXVec3Normalize(&dir0, &temp);
            dir += dir0;
        }

        D3DXVec3Normalize(&dir, &dir);

        D3DXVECTOR3 move;
        D3DXVECTOR3 temp = { 0.0f, 0.0f, 1.0f };
        D3DXVec3Cross(&move, &m_Points[i].m_Pos[0], &temp);
        D3DXVec3Normalize(&move, &move);

        //if(i & 1)
        dir += move * SEL_SPEED;
        //else dir -= move * SEL_SPEED;

        m_Points[i].m_Pos[0] += dir * dtime;
       
        D3DXVECTOR3 newpos(m_Points[i].m_Pos[0] + m_Pos);
        float z = g_MatrixMap->GetZ(newpos.x, newpos.y);

        m_Points[i].m_Pos[0].z = z + SEL_SIZE - m_Pos.z;
        if(m_Points[i].m_Pos[0].z < 0) m_Points[i].m_Pos[0].z = 0;
        
        float r = D3DXVec3Length(&m_Points[i].m_Pos[0]);
        if(r > m_Radius) m_Points[i].m_Pos[0] *= (m_Radius / r);
    }

    //�����-�� ����� ������ - �������� � ����, ��� ��� ����� ���� ��������� � ������ �������
    /*
    for(int i = 0; i < m_SelCnt; ++i)
    {
        for(int j = 0; j < SEL_BLUR_CNT; ++j)
        {
            D3DXVECTOR3 delta((m_Points[i].m_Pos[j - 1] - m_Points[i].m_Pos[j]));
            float x = D3DXVec3Length(&delta);
            if(x > 1) m_Points[i].m_Pos[j] += delta * (x - 1) / x;
        }
    }
    */

    UpdatePositions();

    if(FLAG(m_Flags, SELF_KIP))
    {
#ifdef _DEBUG
        g_MatrixMap->SubEffect(DEBUG_CALL_INFO, this);
#else
        g_MatrixMap->SubEffect(this);
#endif
        return;

        m_Radius -= dtime;
        if(m_Radius < 0 )
        {
#ifdef _DEBUG
        g_MatrixMap->SubEffect(DEBUG_CALL_INFO, this);
#else
        g_MatrixMap->SubEffect(this);
#endif
            return;
        }

        for(int i = 0; i < m_SelCnt; ++i)
        {
            for(int j = 0; j < SEL_BLUR_CNT; ++j)
            {
                dword c = (byte((m_Radius / m_InitRadius) * float(m_Color_current >> 24)) << 24) | (m_Color_current & 0x00FFFFFF);

                m_Points[i].m_Blur[j].SetColor(c);
                //m_Points[i].m_Blur[j].SetAlpha();
            }
        }
    }
    else
    {
        for(int i = 0; i < m_SelCnt; ++i)
        {
            for(int j = 0; j < SEL_BLUR_CNT; ++j) m_Points[i].m_Blur[j].SetColor(m_Color_current);
        }
    }
}

void CMatrixEffectSelection::Release(void)
{
DTRACE();

    SetDIP();
    HDelete(CMatrixEffectSelection, this, m_Heap);
}