// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "../stdafx.h"
#include "MatrixEffect.hpp"
#include "../MatrixMap.hpp"
#include <math.h>


D3D_VB CMatrixEffectCone::m_VB;
int    CMatrixEffectCone::m_VB_ref;

CMatrixEffectCone::CMatrixEffectCone(const D3DXVECTOR3 &start, const D3DXVECTOR3& dir, float radius, float height, float angle, float ttl, dword color, bool intense, CTextureManaged* tex) :
    CMatrixEffect(), m_Pos(start), m_Dir(dir), m_Radius(radius), m_Height(height), m_TTL(ttl), m_Intense(intense), m_Tex(tex), m_Angle(angle), _m_TTL(INVERT(ttl)), m_Color(color)
{
    m_EffectType = EFFECT_CONE;
    //D3DXVec3Normalize(&m_Dir, &(m_End - m_Pos));
    //m_Prevdist = D3DXVec3Length(&(m_End - m_Pos));

    ++m_VB_ref;

    if(!m_Tex)
    {
        m_Tex = m_SpriteTextures[SPR_GUN_FIRE].tex;
    }

    //m_Tex->Prepare();
    m_Tex->RefInc();

    UpdateMatrix();
}

bool CMatrixEffectCone::PrepareDX()
{
    if(IS_VB(m_VB)) return true;

    int sz = (2 + CONE_NUM_SECTORS) * sizeof(SConeVertex);
    SConeVertex* v;
    CREATE_VB(sz, CONE_FVF, m_VB);

    if(!IS_VB(m_VB)) return false;

    LOCK_VB(m_VB, &v);

    v[0].p = D3DXVECTOR3(0, 0, 0); v[0].tu = 0.5f; v[0].tv = 0.5f;
    v[1].p = D3DXVECTOR3(1, 1, 0); v[1].tu = 1; v[1].tv = 0.5f;
    v[CONE_NUM_SECTORS + 1].p = D3DXVECTOR3(1, 1, 0); v[CONE_NUM_SECTORS + 1].tu = 1; v[CONE_NUM_SECTORS + 1].tv = 0.5f;

    for(int i = 2; i <= CONE_NUM_SECTORS; ++i)
    {
        float angle = M_PI_MUL((2.0 / CONE_NUM_SECTORS) * (i - 1));
        float ss = TableSin(angle);
        float cc = TableCos(angle);

        v[i].p = D3DXVECTOR3(1, cc, ss); v[i].tu = (cc + 1.0f) * 0.5f; v[i].tv = (ss + 1.0f) * 0.5f;
    }

    UNLOCK_VB(m_VB);

    return true;
}

CMatrixEffectCone::~CMatrixEffectCone()
{
    if((--m_VB_ref) <= 0)
    {
        m_VB_ref = 0;
        if(IS_VB(m_VB))
        {
            DESTROY_VB(m_VB);
        }
    }

    m_Tex->RefDecUnload();
}

void CMatrixEffectCone::Release()
{
    SetDIP();
    HDelete(CMatrixEffectCone, this, m_Heap);
}

void CMatrixEffectCone::Draw()
{
    if(!IS_VB(m_VB)) return;

    ASSERT_DX(g_D3DD->SetFVF(CONE_FVF));

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));

    if(m_Intense)
    {
        g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    }

    ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD, &m_Mat));
    ASSERT_DX(g_D3DD->SetTexture(0, m_Tex->Tex()));

    g_D3DD->SetStreamSource(0, GET_VB(m_VB), 0, sizeof(SConeVertex));
    g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR, m_Color);

    SetColorOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_TFACTOR);
    SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_TFACTOR);

    SetColorOpDisable(1);

    g_D3DD->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, CONE_NUM_SECTORS);
    //g_D3DD->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

    if(m_Intense)
    {
        g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    }
}

void CMatrixEffectCone::Tact(float step)
{
    m_TTL -= step;

    if(m_TTL < 0)
    {
        g_MatrixMap->SubEffect(this);
        return;
    }

    m_Color = (m_Color & 0x00FFFFFF) | (Float2Int(255 * (m_TTL * _m_TTL)) << 24);
}


void CMatrixEffectCone::UpdateMatrix()
{
    float ss = TableSin(m_Angle);
    float cc = TableCos(m_Angle);

    m_Mat._11 = m_Height;  m_Mat._12 = 0.0f;            m_Mat._13 = 0.0f;           m_Mat._14 = 0.0f;
    m_Mat._21 = 0.0f;      m_Mat._22 = cc * m_Radius;   m_Mat._23 = ss * m_Radius;  m_Mat._24 = 0.0f;
    m_Mat._31 = 0.0f;      m_Mat._32 = -ss * m_Radius;  m_Mat._33 = cc * m_Radius;  m_Mat._34 = 0.0f;
    m_Mat._41 = 0.0f;      m_Mat._42 = 0.0f;            m_Mat._43 = 0.0f;           m_Mat._44 = 1.0f;

    D3DXMATRIX m;
    VecToMatrixX(m, m_Pos, m_Dir);

    m_Mat = m_Mat * m;
}


void CMatrixEffectConeSplash::Tact(float step)
{
    m_TTL -= step;
    if(m_TTL < 0)
    {
        g_MatrixMap->SubEffect(this);
        return;
    }

    float t = (m_TTL * _m_TTL);

    m_Height = m_HeightInit * TableSin(M_PI_MUL((1.0 - t) * 1.5));
    m_Radius = m_RadiusInit * TableSin(M_PI_MUL((1.0 - t) * 0.5));

    m_Pos = m_PosInit;
    m_Pos.z += 0.5f * m_HeightInit * TableSin(M_PI_MUL(1.0 - t));
    m_Color = (m_Color & 0x00FFFFFF) | (Float2Int(255 * t) << 24);

    UpdateMatrix();
}