// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "../stdafx.h"
#include "MatrixEffect.hpp"
#include "../MatrixMap.hpp"
#include <math.h>

#define REPAIR_CHANGE_TIME        500
#define REPAIR_SEEK_TARGET_PERIOD 100
#define REPAIR_SPAWN_PERIOD       13

CMatrixEffectRepair::CMatrixEffectRepair(const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, float seek_radius, CMatrixMapStatic* skip, ESpriteTextureSort sprite_spot) :
    CMatrixEffect(), m_SeekRadius(seek_radius), m_Skip(skip), m_Pos(pos), m_Dir(dir), m_KordOnTarget(m_Heap), m_Kord(m_Heap), m_SpriteSpot(sprite_spot)
{
    m_EffectType = EFFECT_REPAIR;

    m_Flags = ERF_OFF_TARGET;

    D3DXVECTOR3 p[4];
    p[0] = pos;
    p[1] = pos + dir * (seek_radius * INVERT(3.0f));
    p[2] = pos + dir * (seek_radius * 2 * INVERT(3.0f));
    p[3] = pos + dir * seek_radius;

    m_Kord.Init1(p, 4);

    UpdateData(pos, dir);
}

CMatrixEffectRepair::~CMatrixEffectRepair()
{
    int i;
    for(i = 0; i <m_BBCnt; ++i)
    {
        m_BBoards[i].bb.Release();
    }

    if(m_Target) m_Target->Release();
}

void CMatrixEffectRepair::BeforeDraw()
{

}

void CMatrixEffectRepair::Draw()
{
    for(int i = 0; i < m_BBCnt; ++i) m_BBoards[i].bb.AddToDrawQueue();
}

struct SFindPatientData
{
    CMatrixMapStatic* tgt = nullptr;
    float             dist = 0.0f;
    D3DXVECTOR3*      wpos = nullptr;
    float             wdist2 = 0.0f;
    int               side_of_owner = 0;
};

static bool FindPatient(const D3DXVECTOR3& fpos, CMatrixMapStatic* ms, dword user)
{
    if(!ms->NeedRepair()) return true;
    SFindPatientData * data = (SFindPatientData*)user;
    if(ms->GetSide() != data->side_of_owner) return true;

    D3DXVECTOR3 temp = *data->wpos - ms->GetGeoCenter();
    float dist = D3DXVec3LengthSq(&temp);
    if(dist > data->wdist2) return true;
    
    temp = fpos - ms->GetGeoCenter();
    dist = D3DXVec3LengthSq(&temp);
    if(dist < data->dist)
    {
        if(ms->IsRobot())
        {
            data->tgt = ms;
            data->dist = dist;
        }
        else if(data->tgt)
        {
            if(!data->tgt->IsRobot())
            {
                data->tgt = ms;
                data->dist = dist;
            } 
        }
        else
        {
            data->tgt = ms;
            data->dist = dist;
        }
    }

    return true;
}

void CMatrixEffectRepair::Tact(float t)
{
    m_Time += t;

    if(FLAG(m_Flags, ERF_FOUND_TARGET))
    {
        m_ChangeTime += t;
        if(m_ChangeTime > REPAIR_CHANGE_TIME)
        {
            m_ChangeTime = REPAIR_CHANGE_TIME;

            RESETFLAG(m_Flags, ERF_FOUND_TARGET);
            SETFLAG(m_Flags, ERF_WITH_TARGET);
        }
    }

    if(FLAG(m_Flags, ERF_LOST_TARGET))
    {
        m_ChangeTime -= t;
        if(m_ChangeTime < 0)
        {
            m_ChangeTime = 0;

            RESETFLAG(m_Flags, ERF_LOST_TARGET);
            SETFLAG(m_Flags, ERF_OFF_TARGET);
        }
    }

    //CDText::T("ct", CStr(m_ChangeTime));

    if(!FLAG(m_Flags,ERF_MAX_AMP))
    {
        m_OffTargetAmp += 0.001f * t;
        float lim = m_SeekRadius*0.2f;
        if(m_OffTargetAmp > lim)
        {
            m_OffTargetAmp = lim;
            SETFLAG(m_Flags,ERF_MAX_AMP);
        }
    }

    // seek target
    if(m_Time > m_NextSeekTime)
    {
        m_NextSeekTime = m_Time + REPAIR_SEEK_TARGET_PERIOD;

        SFindPatientData data;
        data.tgt = nullptr;
        data.dist = 1E30f;
        data.wpos = &m_Pos;
        data.wdist2 = m_SeekRadius * m_SeekRadius;
        data.side_of_owner = 0;
        if(m_Skip) data.side_of_owner = m_Skip->GetSide();
    
        //g_MatrixMap->m_DI.ShowScreenText(L"ishem...", L"", 1000);
        g_MatrixMap->FindObjects(m_Pos + m_Dir * m_SeekRadius * 0.5f, m_SeekRadius * 0.5f, 1.0f, TRACE_ROBOT | TRACE_FLYER | TRACE_BUILDING | TRACE_TURRET, m_Skip, FindPatient, (dword)&data);

        if(data.tgt)
        {
            // ok

            //g_MatrixMap->m_DI.ShowScreenText(L"i nahodim...", L"", 1000);
            
            SObjectCore *core = data.tgt->GetCore(DEBUG_CALL_INFO);
            
            if(m_Target == core) core->Release();
            else
            {
                if(m_Target)
                {
                    m_Target->Release();
                    m_Target = nullptr;
                    RESETFLAG(m_Flags, ERF_OFF_TARGET);
                    RESETFLAG(m_Flags, ERF_WITH_TARGET);
                    SETFLAG(m_Flags, ERF_LOST_TARGET);
                    RESETFLAG(m_Flags, ERF_FOUND_TARGET);

                    core->Release();
                }
                else
                {
                    m_Target = core;

                    RESETFLAG(m_Flags, ERF_OFF_TARGET);
                    RESETFLAG(m_Flags, ERF_WITH_TARGET);
                    RESETFLAG(m_Flags, ERF_LOST_TARGET);
                    SETFLAG(m_Flags, ERF_FOUND_TARGET);
                }
            }
        }
        else
        {
            RESETFLAG(m_Flags, ERF_WITH_TARGET);
            RESETFLAG(m_Flags, ERF_FOUND_TARGET);
            if(m_Target)
            {
                m_Target->Release();
                m_Target = nullptr;
                RESETFLAG(m_Flags, ERF_OFF_TARGET);
                SETFLAG(m_Flags, ERF_LOST_TARGET);
            }
            //else {}
        }
    }

    if(m_Target)
    {
        if(!m_Target->m_Object)
        {
            RESETFLAG(m_Flags, ERF_WITH_TARGET);
            RESETFLAG(m_Flags, ERF_FOUND_TARGET);
            SETFLAG(m_Flags, ERF_LOST_TARGET);
            m_Target->Release();
            m_Target = nullptr;
        }
    }

    float sins[10] = 
    {
        (float)sin((double)m_Time * 0.005f + 0.9f),
        (float)sin((double)m_Time * 0.007f + 0.2f),
        (float)sin((double)m_Time * 0.0073f + 0.3f),
        (float)sin((double)m_Time * 0.0013f),
        (float)sin((double)m_Time * 0.0022f + 0.2f),
        (float)sin((double)m_Time * 0.0017f + 0.8f),
        (float)sin((double)m_Time * 0.0028f + 0.3f),
        (float)sin((double)m_Time * 0.0051f + 0.9f),
        (float)sin((double)m_Time * 0.003f + 0.01f),
        (float)sin((double)m_Time * 0.001f + 0.93f)
    };

    D3DXVECTOR3 p4[4];

    if(FLAG(m_Flags, ERF_OFF_TARGET | ERF_FOUND_TARGET | ERF_LOST_TARGET) && !FLAG(m_Flags, ERF_WITH_TARGET))
    {
        // build p4;
        p4[0] = m_Pos;
        p4[1] = m_Pos + m_Dir * (m_SeekRadius * INVERT(3.0f));

        float dx2 = 0.4f * m_OffTargetAmp * sins[3];
        float dy2 = 0.4f * m_OffTargetAmp * sins[7];
        float dz2 = 0.4f * m_OffTargetAmp * sins[9];

        p4[2] = m_Pos + m_Dir * (m_SeekRadius * 2 * INVERT(3.0f)) + D3DXVECTOR3(dx2, dy2, dz2);

        float dx1 = m_OffTargetAmp * sins[1];
        float dy1 = m_OffTargetAmp * sins[4];
        float dz1 = m_OffTargetAmp * sins[8];

        p4[3] = m_Pos + m_Dir * m_SeekRadius + D3DXVECTOR3(dx1, dy1, dz1);
    }

    D3DXVECTOR3 p9[9];
    if(FLAG(m_Flags, ERF_WITH_TARGET | ERF_FOUND_TARGET) && !FLAG(m_Flags, ERF_OFF_TARGET))
    {
        // build p9
        ASSERT(m_Target && m_Target->m_Object);

        D3DXVECTOR3 temp = m_Pos - m_Target->m_GeoCenter;
        float len = D3DXVec3Length(&temp);

        float disp = len * 0.01f;

        p9[0] = m_Pos;
        p9[1] = m_Pos + m_Dir * (len * INVERT(3.0f));

        D3DXVECTOR3 td(p9[1] - m_Target->m_GeoCenter);

        D3DXVec3Normalize(&td, &td);
        D3DXVECTOR3 side;
        D3DXVec3Cross(&side, &td, (D3DXVECTOR3*)&m_Target->m_Matrix._31);
        D3DXVec3Normalize(&side, &side);

        float dx1 = disp * sins[0];
        float dy1 = disp * sins[1];
        float dz1 = disp * sins[2];

        float dx2 = disp * sins[2];
        float dy2 = disp * sins[3];
        float dz2 = disp * sins[4];

        float dx3 = disp * sins[4];
        float dy3 = disp * sins[5];
        float dz3 = disp * sins[6];
                           
        float dx4 = disp * sins[6];
        float dy4 = disp * sins[7];
        float dz4 = disp * sins[8];

        float dx5 = disp * sins[3];
        float dy5 = disp * sins[6];
        float dz5 = disp * sins[9];

        p9[2] = D3DXVECTOR3(dx1, dy1, dz1) + m_Target->m_GeoCenter + td * m_Target->m_Radius * 1.4f;
        p9[3] = D3DXVECTOR3(dx2, dy2, dz2) + m_Target->m_GeoCenter + side * m_Target->m_Radius;
        p9[4] = D3DXVECTOR3(dx3, dy3, dz3) + m_Target->m_GeoCenter - td * m_Target->m_Radius;
        p9[5] = D3DXVECTOR3(dx4, dy4, dz4) + m_Target->m_GeoCenter - side * m_Target->m_Radius;
        p9[6] = D3DXVECTOR3(dx5, dy5, dz5) + m_Target->m_GeoCenter + td * m_Target->m_Radius * 1.4f;
        p9[7] = p9[1];
        p9[8] = p9[0];

        m_KordOnTarget.Init2(p9, 9);
    }

    if(FLAG(m_Flags, ERF_FOUND_TARGET | ERF_LOST_TARGET))
    {
        float k = m_ChangeTime * INVERT(REPAIR_CHANGE_TIME);

        CTrajectory tr(m_Heap, p4, 4);

        D3DXVECTOR3 p0;
        D3DXVECTOR3 p1;

        D3DXVECTOR3 newkord[9];

        for(int i = 0; i < 9; ++i)
        {
            float t = i * INVERT(8.0f);

            tr.CalcPoint(p0, t);

            m_KordOnTarget.CalcPoint(p1, t * (k * 0.5f + 0.5f));

            newkord[i] = LERPVECTOR(k, p0, p1);
        }

        m_Kord.Init1(newkord, 9);
    }
    else
    {
        if(FLAG(m_Flags, ERF_WITH_TARGET)) m_Kord.Init1(p9, 9);
        else m_Kord.Init1(p4, 4);
    }

    float dt = 0.06f;
    if(!m_Target) dt = 0.1f;
    int i = 0;
    while(i < m_BBCnt)
    {
        m_BBoards[i].t += m_BBoards[i].dt;
        D3DXVECTOR3 newpos0, newpos1;
        m_Kord.CalcPoint(newpos0, m_BBoards[i].t - dt);
        m_Kord.CalcPoint(newpos1, m_BBoards[i].t);
        m_BBoards[i].bb.SetPos(newpos0, newpos1);

        if(m_BBoards[i].t > 1)
        {
            //m_Activated = true;
            m_BBoards[i].bb.Release();
            m_BBoards[i] = m_BBoards[--m_BBCnt];
            continue;
        }
        else
        {
            byte a = byte(KSCALE(m_BBoards[i].t, 0, 0.2f) * 255.0);
            m_BBoards[i].bb.SetAlpha(a);
        }
        ++i;
    }

    while(m_NextSpawnTime < m_Time)
    {
        m_NextSpawnTime += REPAIR_SPAWN_PERIOD;

        // spawn bill
        if(m_BBCnt < REPAIR_BB_CNT)
        {
            D3DXVECTOR3 p;
            m_Kord.CalcPoint(p, 0);

            m_BBoards[m_BBCnt].bb = CSpriteSequence(TRACE_PARAM_CALL p, p, 4, 0x00FFFFFF, GetSingleBrightSpriteTex(m_SpriteSpot));
            m_BBoards[m_BBCnt].dt = FRND(0.03f) + 0.006f;
            m_BBoards[m_BBCnt].t = 0;
            ++m_BBCnt;
        }
    }

    ////D3DXVec3TransformCoordArray(&m_BBoards[0].bb.m_Pos, sizeof(SElevatorFieldBBoard), &m_BBoards[0].bb.m_Pos, sizeof(SElevatorFieldBBoard), &m_Rot, m_AllBBCnt);
}
void CMatrixEffectRepair::Release(void)
{
DTRACE();

    SetDIP();
    HDelete(CMatrixEffectRepair, this, m_Heap);
}