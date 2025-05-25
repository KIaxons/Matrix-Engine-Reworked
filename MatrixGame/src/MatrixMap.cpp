// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "stdafx.h"
#include <windows.h>
#include "MatrixMap.hpp"
#include "MatrixMapStatic.hpp"
#include "MatrixObject.hpp"
#include "MatrixObjectBuilding.hpp"
#include "MatrixObjectCannon.hpp"
#include "MatrixRobot.hpp"
#include "MatrixRenderPipeline.hpp"
#include "MatrixFlyer.hpp"
#include "MatrixProgressBar.hpp"
#include "ShadowStencil.hpp"
#include "MatrixLoadProgress.hpp"
#include "MatrixTerSurface.hpp"
#include "MatrixSkinManager.hpp"
#include "MatrixSoundManager.hpp"
#include "Interface/MatrixHint.hpp"
#include "MatrixInstantDraw.hpp"

#include "Interface/CInterface.h"
#include "Logic/MatrixTactics.h"
#include "MatrixGameDll.hpp"
#include "MatrixSampleStateManager.hpp"


#define RENDER_PROJ_SHADOWS_IN_STENCIL_PASS

//Globals
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CMatrixMap::CMatrixMap() : CMain(), m_Console(), m_Camera(), m_AllObjects(Base::g_MatrixHeap, 1024), m_RoadNetwork(Base::g_MatrixHeap), m_DialogModeHints(Base::g_MatrixHeap)
{
    //m_LightMain = { 250.0f, -50.0f, -250.0f };
    D3DXVec3Normalize(&m_LightMain, &m_LightMain);

    //zakker
    D3DXMatrixIdentity(&m_Identity);

    //m_Reflection->Tex()->SetPriority(0xFFFFFFFF);

    CSound::Init();

    //����� ������������ ��������� ����
    CBlockPar* repl = g_MatrixData->BlockGetNE(PAR_REPLACE);
    if(repl)
    {
        CWStr t1(repl->ParGetNE(PAR_REPLACE_DIFFICULTY), Base::g_MatrixHeap);

        if(!t1.IsEmpty())
        {
            CBlockPar* dif = g_MatrixData->BlockGet(BLOCK_PATH_MAIN_CONFIG)->BlockGetNE(PAR_SOURCE_DIFFICULTY);
            if(dif)
            {
                CWStr t2(Base::g_MatrixHeap);
                t2 = dif->ParGetNE(t1);
                if(!t2.IsEmpty())
                {
                    m_Difficulty.coef_enemy_damage_to_player_side = max(1.0f + t2.GetFloatPar(0, L",") / 100.0f, 0.01f);
                    m_Difficulty.coef_time_before_reinforcements = max(1.0f + t2.GetFloatPar(1, L",") / 100.0f, 0.01f);
                    m_Difficulty.coef_friendly_fire = max(1.0f + t2.GetFloatPar(2, L",") / 100.0f, 0.01f);
                }
            }
        }
    }
}

CMatrixMap::~CMatrixMap()
{
	//Clear(); do not call Clear method from this destructor!!!!!! it will be called from CMatrixMapLogic Clear
}


void CMatrixMap::Clear()
{
    m_RoadNetwork.Clear();

    m_Minimap.Clear();
	StaticClear();

    // clear effects

    while(m_EffectsFirst)
    {
        SubEffect(m_EffectsFirst);
    }

    if(m_EffectSpawners)
    {
        for(int i = 0; i < m_EffectSpawnersCnt; ++i)
        {
            m_EffectSpawners[i].~CEffectSpawner();
        }
        HFree(m_EffectSpawners, Base::g_MatrixHeap);
    }

    m_EffectSpawnersCnt = 0;
    m_EffectSpawners = nullptr;
    CMatrixEffect::ClearEffects();


	MacrotextureClear();
	WaterClear();
	GroupClear();

    ModuleClear();
	IdsClear();
	ClearSide();

    CTerSurface::ClearSurfaces();
    CBottomTextureUnion::Clear();
    CSkinManager::Clear();
    CSound::Clear();

    if(IS_VB(m_ShadowVB)) DESTROY_VB(m_ShadowVB);

    m_Cursor.Clear();
    m_DI.Clear();

    if(m_DeviceState != nullptr)
    {
        HDelete(CDeviceState, m_DeviceState, Base::g_MatrixHeap);
        m_DeviceState = nullptr;
    }
}

void CMatrixMap::IdsClear()
{
	if(m_Ids)
    {
		for(int i = 0; i < m_IdsCnt; ++i)
        {
            m_Ids[i].~CWStr();
		}

        HFree(m_Ids, Base::g_MatrixHeap);
        m_Ids = nullptr;
	}

	m_IdsCnt = 0;
}

//��������� ������ � �������� ������� �� ����� �� ����, ���������� ��� ��������� � ����������� �������� ���������
void CMatrixMap::RobotPreload(void)
{
    g_LoadProgress->SetCurLP(LP_PRELOADROBOTS);
    g_LoadProgress->InitCurLP(ROBOT_HULLS_COUNT + ROBOT_CHASSIS_COUNT + ROBOT_HEADS_COUNT + ROBOT_WEAPONS_COUNT);
    int curlp = 0;

    CWStr tstr(Base::g_MatrixHeap), tstr2(Base::g_MatrixHeap), part(Base::g_MatrixHeap);

    for(int i = 1; i <= ROBOT_HULLS_COUNT; ++i)
    {
        part.Set(g_Config.m_RobotHullsConsts[i].model_path);
        CVectorObject* vo = (CVectorObject*)g_Cache->Get(cc_VO, (g_CacheData->ParPathGet(part + L".Model") + L".vo").Get());
        vo->PrepareSpecial(OLF_MULTIMATERIAL_ONLY, CSkinManager::GetSkin, GSP_SIDE);
        g_LoadProgress->SetCurLPPos(curlp++);
    }

    for(int i = 1; i <= ROBOT_CHASSIS_COUNT; ++i)
    {
        part.Set(g_Config.m_RobotChassisConsts[i].model_path);
        CVectorObject* vo = (CVectorObject*)g_Cache->Get(cc_VO, (g_CacheData->ParPathGet(part + L".Model") + L".vo").Get());
        vo->PrepareSpecial(OLF_MULTIMATERIAL_ONLY, CSkinManager::GetSkin, GSP_SIDE);

        if(g_Config.m_RobotChassisConsts[i].is_walking) CMatrixRobot::BuildWalkingChassisData(vo, i);

        g_LoadProgress->SetCurLPPos(curlp++);
    }

    for(int i = 1; i <= ROBOT_HEADS_COUNT; ++i)
    {
        part.Set(g_Config.m_RobotHeadsConsts[i].model_path);
        CVectorObject* vo = (CVectorObject*)g_Cache->Get(cc_VO, (g_CacheData->ParPathGet(part + L".Model") + L".vo").Get());

        vo->PrepareSpecial(OLF_MULTIMATERIAL_ONLY, CSkinManager::GetSkin, GSP_SIDE);

        g_LoadProgress->SetCurLPPos(curlp++);
    }

    for(int i = 1; i <= ROBOT_WEAPONS_COUNT; ++i)
    {
        part.Set(g_Config.m_RobotWeaponsConsts[i].model_path);
        CVectorObject* vo = (CVectorObject*)g_Cache->Get(cc_VO, (g_CacheData->ParPathGet(part + L".Model") + L".vo").Get());
        vo->PrepareSpecial(OLF_MULTIMATERIAL_ONLY, CSkinManager::GetSkin, GSP_SIDE);

        g_LoadProgress->SetCurLPPos(curlp++);
    }
}

void CMatrixMap::ModuleClear(void)
{
    if(m_Module != nullptr) { HFree(m_Module, Base::g_MatrixHeap); m_Module = nullptr; }
    if(m_Point != nullptr) { HFree(m_Point, Base::g_MatrixHeap); m_Point = nullptr; }
    if(m_Move != nullptr) { HFree(m_Move, Base::g_MatrixHeap); m_Move = nullptr; }
}

void CMatrixMap::UnitInit(int sx, int sy)
{
	ModuleClear();

    m_Size.x = sx;
    m_Size.y = sy;
    m_SizeMove.x = sx * MOVE_CNT;
    m_SizeMove.y = sy * MOVE_CNT;
    int cnt = sx * sy;

    m_Module = (SMatrixMapUnit*)HAllocClear(sizeof(SMatrixMapUnit) * cnt, Base::g_MatrixHeap);

    m_Point = (SMatrixMapPoint*)HAllocClear(sizeof(SMatrixMapPoint) * (sx + 1) * (sy + 1), Base::g_MatrixHeap);

    m_Move = (SMatrixMapMove*)HAllocClear(sizeof(SMatrixMapMove) * m_SizeMove.x * m_SizeMove.y, Base::g_MatrixHeap);

    /*
	SMatrixMapUnit* un = m_Module;
	while(cnt > 0)
    {
		un->m_TextureTop = -1;
		++un;
		--cnt;
	}
    */
}


dword CMatrixMap::GetColor(float wx, float wy)
{
    //static int ccc = 0;
    //CDText::T("COLOR", CStr(ccc++));

    float scaledx = wx * INVERT(GLOBAL_SCALE);
    float scaledy = wy * INVERT(GLOBAL_SCALE);

    int x = TruncFloat(scaledx);
    int y = TruncFloat(scaledy);

    SMatrixMapUnit* un = UnitGetTest(x, y);
    if((un == nullptr) || (un->IsWater())) return m_AmbientColorObj;

    float kx = scaledx - float(x);
    float ky = scaledy - float(y);
	
    SMatrixMapPoint* mp = PointGet(x, y);
    int temp0r, temp0g, temp0b;
    int temp1r, temp1g, temp1b;
    int temp2r, temp2g, temp2b;
    int temp3r, temp3g, temp3b;

    // color 0;
    temp0r = ((mp->color >> 16) & 255) + mp->lum_r; 
    temp0g = ((mp->color >> 8) & 255) + mp->lum_g; 
    temp0b = ((mp->color >> 0) & 255) + mp->lum_b; 

    // color 1;
    SMatrixMapPoint* mp0 = mp + 1;
    temp1r = ((mp0->color >> 16) & 255) + mp0->lum_r; 
    temp1g = ((mp0->color >> 8) & 255) + mp0->lum_g; 
    temp1b = ((mp0->color >> 0) & 255) + mp0->lum_b; 

    // color 2;
    mp0 = (mp + m_Size.x + 1);
    temp2r = ((mp0->color >> 16) & 255) + mp0->lum_r;
    temp2g = ((mp0->color >> 8) & 255) + mp0->lum_g;
    temp2b = ((mp0->color >> 0) & 255) + mp0->lum_b;

    // color 2;
    mp0 = (mp + m_Size.x + 2);
    temp3r = ((mp0->color >> 16) & 255) + mp0->lum_r;
    temp3g = ((mp0->color >> 8) & 255) + mp0->lum_g;
    temp3b = ((mp0->color >> 0) & 255) + mp0->lum_b;

    int r = Float2Int(LERPFLOAT(ky, LERPFLOAT(kx, temp0r, temp1r), LERPFLOAT(kx, temp2r, temp3r)));
    int g = Float2Int(LERPFLOAT(ky, LERPFLOAT(kx, temp0g, temp1g), LERPFLOAT(kx, temp2g, temp3g)));
    int b = Float2Int(LERPFLOAT(ky, LERPFLOAT(kx, temp0b, temp1b), LERPFLOAT(kx, temp2b, temp3b)));
    if(r > 255) r = 255;
    if(g > 255) g = 255;
    if(b > 255) b = 255;

    return (r << 16) | (g << 8) | (b);
}
float CMatrixMap::GetZInterpolatedLand(float wx,float wy)
{
    int x = Float2Int((float)floor(wx * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE)));
    int y = Float2Int((float)floor(wy * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE)));

    D3DXVECTOR3 xx[4], yy[4], ox, oy;
    float fx(x * (GLOBAL_SCALE * MAP_GROUP_SIZE)), fy(y * (GLOBAL_SCALE * MAP_GROUP_SIZE)), z;

    float tx = (wx - fx) * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE);
    float ty = (wy - fy) * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE);
    if(tx < 0) tx = 1.0f + tx;
    if(ty < 0) ty = 1.0f + ty;

    for(int j = 0; j <= 3; ++j)
    {
        int cy = y + j - 1;
        float prex_z = GetGroupMaxZLand(x - 2, cy);
        if(prex_z < m_GroundZBaseMiddle) prex_z = m_GroundZBaseMiddle;
        else if(prex_z > m_GroundZBaseMax) prex_z = m_GroundZBaseMax;
        for(int i = 0; i <= 3; ++i)
        {
            int cx = x + i - 1;

            // along x
            xx[i].x = cx * (GLOBAL_SCALE * MAP_GROUP_SIZE);
            xx[i].y = cy * (GLOBAL_SCALE * MAP_GROUP_SIZE);
            z = GetGroupMaxZLand(cx,cy);
            if(z < m_GroundZBaseMiddle) z = m_GroundZBaseMiddle;
            else if(z > m_GroundZBaseMax) z = m_GroundZBaseMax;
            xx[i].z = max(prex_z, z); //(prex_z + z) * 0.5f;
            prex_z = z;
        }

        SBSplineKoefs kx;
        CalcBSplineKoefs2(kx, xx[0], xx[1], xx[2], xx[3]);
        CalcBSplinePoint(kx, yy[j], tx);
    }

    SBSplineKoefs ky;
    CalcBSplineKoefs2(ky, yy[0], yy[1], yy[2], yy[3]);
    CalcBSplinePoint(ky, oy, ty);
    return oy.z;
}

float CMatrixMap::GetZInterpolatedObj(float wx, float wy)
{
    int x = Float2Int((float)floor(wx * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE)));
    int y = Float2Int((float)floor(wy * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE)));

    D3DXVECTOR3 xx[4], yy[4], ox, oy;
    float fx(x * (GLOBAL_SCALE * MAP_GROUP_SIZE)), fy(y * (GLOBAL_SCALE * MAP_GROUP_SIZE)), z;
    
    float tx = (wx - fx) * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE);
    float ty = (wy - fy) * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE);
    if(tx < 0) tx = 1.0f + tx;
    if(ty < 0) ty = 1.0f + ty;

    for(int j = 0; j <= 3; ++j)
    {
        int cy = y + j - 1;
        float prex_z = GetGroupMaxZObj(x-2,cy);
        for (int i=0; i<=3; ++i)
        {
            int cx = x + i - 1;

            // along x
            xx[i].x = cx * (GLOBAL_SCALE * MAP_GROUP_SIZE);
            xx[i].y = cy * (GLOBAL_SCALE * MAP_GROUP_SIZE);
            z = GetGroupMaxZObj(cx,cy);
            xx[i].z = max(prex_z, z); //(prex_z + z) * 0.5f;
            prex_z = z;
        }
        SBSplineKoefs   kx;
        CalcBSplineKoefs2(kx, xx[0], xx[1], xx[2], xx[3]);
        CalcBSplinePoint(kx, yy[j], tx);
    }

    SBSplineKoefs   ky;
    CalcBSplineKoefs2(ky, yy[0], yy[1], yy[2], yy[3]);
    CalcBSplinePoint(ky, oy, ty);
    return oy.z;
}

float CMatrixMap::GetZInterpolatedObjRobots(float wx, float wy)
{
    int x = Float2Int((float)floor(wx * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE)));
    int y = Float2Int((float)floor(wy * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE)));

    D3DXVECTOR3 xx[4], yy[4], ox, oy;
    float fx(x * (GLOBAL_SCALE * MAP_GROUP_SIZE)), fy(y * (GLOBAL_SCALE * MAP_GROUP_SIZE)), z;

    float tx = (wx - fx) * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE);
    float ty = (wy - fy) * INVERT(GLOBAL_SCALE * MAP_GROUP_SIZE);
    if(tx < 0) tx = 1.0f + tx;
    if(ty < 0) ty = 1.0f + ty;

    for(int j = 0; j <= 3; ++j)
    {
        int cy = y + j - 1;
        float prex_z = GetGroupMaxZObjRobots(x - 2, cy);
        for(int i = 0; i <= 3; ++i)
        {
            int cx = x + i - 1;

            // along x
            xx[i].x = cx * (GLOBAL_SCALE * MAP_GROUP_SIZE);
            xx[i].y = cy * (GLOBAL_SCALE * MAP_GROUP_SIZE);
            z = GetGroupMaxZObjRobots(cx, cy);
            xx[i].z = max(prex_z, z); //(prex_z + z) * 0.5f;
            prex_z = z;
        }

        SBSplineKoefs kx;
        CalcBSplineKoefs2(kx, xx[0], xx[1], xx[2], xx[3]);
        CalcBSplinePoint(kx, yy[j], tx);
    }

    SBSplineKoefs ky;
    CalcBSplineKoefs2(ky, yy[0], yy[1], yy[2], yy[3]);
    CalcBSplinePoint(ky, oy, ty);
    return oy.z;

}

float CMatrixMap::GetZLand(double wx, double wy)
{
    int x = TruncDouble(wx * INVERT(GLOBAL_SCALE));
    int y = TruncDouble(wy * INVERT(GLOBAL_SCALE));

    SMatrixMapUnit* un = UnitGetTest(x, y);
    if(un == nullptr || !un->IsLand()) return 0;

    wx -= x * GLOBAL_SCALE;
    wy -= y * GLOBAL_SCALE;

    SMatrixMapPoint* mp = PointGet(x, y);

    D3DXVECTOR3 p0, p2;
    p0.x = 0;
    p0.y = 0;
    p0.z = mp->z;
    p2.x = GLOBAL_SCALE;
    p2.y = GLOBAL_SCALE;
    p2.z = (mp + m_Size.x + 2)->z;

    if(wy < wx)
    {
        D3DXVECTOR3 p1;
        D3DXPLANE pl;

        p1.x = GLOBAL_SCALE;
        p1.y = 0;
        p1.z = (mp + 1)->z;

        D3DXPlaneFromPoints(&pl, &p0, &p1, &p2);
        float cc = -1.0f / pl.c;
        float a1 = pl.a * cc;
        float b1 = pl.b * cc;
        float c1 = pl.d * cc;

        return float(a1 * wx + b1 * wy + c1);
    }
    else
    {
        D3DXVECTOR3 p3;
        D3DXPLANE pl;
        p3.x = 0;
        p3.y = GLOBAL_SCALE;
        p3.z = (mp + m_Size.x + 1)->z;

        D3DXPlaneFromPoints(&pl, &p0, &p2, &p3);
        float cc = -1.0f / pl.c;
        float a2 = pl.a * cc;
        float b2 = pl.b * cc;
        float c2 = pl.d * cc;

        return float(a2 * wx + b2 * wy + c2);
    }
}

float CMatrixMap::GetZ(float wx, float wy)
{
    int x = TruncFloat(wx * INVERT(GLOBAL_SCALE));
    int y = TruncFloat(wy * INVERT(GLOBAL_SCALE));

    SMatrixMapUnit* un = UnitGetTest(x, y);
    if(un == nullptr) return -1000.0f;
    if(!un->IsBridge())
    {
        if(un->IsWater()) return -1000.0f;
    }
    if(un->IsFlat()) return un->a1;

    wx -= x * GLOBAL_SCALE;
    wy -= y * GLOBAL_SCALE;

    if(wy < wx) return un->a1 * wx + un->b1 * wy + un->c1;
    else return un->a2 * wx + un->b2 * wy + un->c2;
}

void CMatrixMap::GetNormal(D3DXVECTOR3* out, float wx, float wy, bool check_water)
{
    float scaledx = wx / GLOBAL_SCALE;
    float scaledy = wy / GLOBAL_SCALE;

    int x = TruncFloat(scaledx);
    int y = TruncFloat(scaledy);

    SMatrixMapUnit* un = UnitGetTest(x, y);

    if((un == nullptr) || un->IsFlat())
    {
    water:
        out->x = 0;
        out->y = 0;
        out->z = 1;
        return;
    }

    if(un->IsBridge())
    {
        D3DXVECTOR3 norm(GetZ(wx - (GLOBAL_SCALE * 0.5f), wy) - GetZ(wx + (GLOBAL_SCALE * 0.5f), wy),
            GetZ(wx, wy - (GLOBAL_SCALE * 0.5f)) - GetZ(wx, wy + (GLOBAL_SCALE * 0.5f)), GLOBAL_SCALE);
        D3DXVec3Normalize(out, &norm);
        return;
    }
    else if(un->IsWater())
    {
        goto water;
    }

    float kx = scaledx - float(x);
    float ky = scaledy - float(y);

    SMatrixMapPoint* mp0 = PointGet(x, y);
    SMatrixMapPoint* mp1 = mp0 + 1;
    SMatrixMapPoint* mp2 = (mp0 + m_Size.x + 1);
    SMatrixMapPoint* mp3 = (mp0 + m_Size.x + 2);

    if(check_water && ((mp0->z < 0) || (mp1->z < 0) || (mp2->z < 0) || (mp3->z < 0))) goto water;

    D3DXVECTOR3 v1 = LERPVECTOR(kx, mp0->n, mp1->n);
    D3DXVECTOR3 v2 = LERPVECTOR(kx, mp2->n, mp3->n);

    D3DXVECTOR3 temp = LERPVECTOR(ky, v1, v2);
    D3DXVec3Normalize(out, &temp);
}

bool CMatrixMap::UnitPick(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, const CRect& ar, int* ox, int* oy, float* ot)
{
    int x, y;
    D3DXVECTOR3 p0, p1, p2, p3;
    SMatrixMapUnit* un;
    float t, m_Module, v;

    float mt = 1e30f;
    bool rv = false;

    for(y = max(0, ar.top); y < min(m_Size.y, ar.bottom); y++)
    {
        for(x = max(0, ar.left); x < min(m_Size.x, ar.right); x++)
        {
            un = UnitGet(x, y);
            if(!un->IsLand()) continue;

            p0.x = GLOBAL_SCALE * (x);
            p0.y = GLOBAL_SCALE * (y);
            p0.z = PointGet(x, y)->z;
            p1.x = GLOBAL_SCALE * (x + 1);
            p1.y = GLOBAL_SCALE * (y);
            p1.z = PointGet(x + 1, y)->z;
            p2.x = GLOBAL_SCALE * (x + 1);
            p2.y = GLOBAL_SCALE * (y + 1);
            p2.z = PointGet(x + 1, y + 1)->z;
            p3.x = GLOBAL_SCALE * (x);
            p3.y = GLOBAL_SCALE * (y + 1);
            p3.z = PointGet(x, y + 1)->z;

            if(IntersectTriangle(orig, dir, p0, p1, p2, t, m_Module, v) || IntersectTriangle(orig, dir, p0, p2, p3, t, m_Module, v))
            {
                if(t < mt)
                {
                    mt = t;
                    if(ox != nullptr) *ox = x;
                    if(oy != nullptr) *oy = y;
                    if(ot != nullptr) *ot = mt;
                    rv = true;
                }
            }
        }
    }
    return rv;
}

bool CMatrixMap::PointPick(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, const CRect& ar, int* ox, int* oy)
{
    int x, y;
    if(!UnitPick(orig, dir, ar, &x, &y)) return false;

    D3DXVECTOR3 p;

    p.x = GLOBAL_SCALE * (x);
    p.y = GLOBAL_SCALE * (y);
    p.z = PointGet(x, y)->z;
    float d0 = DistLinePoint(orig, orig + dir, p);

    p.x = GLOBAL_SCALE * (x + 1);
    p.y = GLOBAL_SCALE * (y);
    p.z = PointGet(x + 1, y)->z;
    float d1 = DistLinePoint(orig, orig + dir, p);

    p.x = GLOBAL_SCALE * (x + 1);
    p.y = GLOBAL_SCALE * (y + 1);
    p.z = PointGet(x + 1, y + 1)->z;
    float d2 = DistLinePoint(orig, orig + dir, p);

    p.x = GLOBAL_SCALE * (x);
    p.y = GLOBAL_SCALE * (y + 1);
    p.z = PointGet(x, y + 1)->z;
    float d3 = DistLinePoint(orig, orig + dir, p);

    if(d1 <= d0 && d1 <= d2 && d1 <= d3) x++;
    else if(d2 <= d0 && d2 <= d1 && d2 <= d3)
    {
        x++;
        y++;
    }
    else if(d3 <= d0 && d3 <= d1 && d3 <= d2) y++;

    if(ox != nullptr) *ox = x;
    if(oy != nullptr) *oy = y;

    return true;
}

bool CMatrixMap::UnitPickGrid(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, int* ox, int* oy)
{
    D3DXPLANE pl;
    D3DXVECTOR3 temp1 = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3 temp2 = { 1.0f, 0.0f, 0.0f };
    D3DXVECTOR3 temp3 = { 0.0f, 1.0f, 0.0f };
    D3DXPlaneFromPoints(&pl, &temp1, &temp2, &temp3);
    //D3DXPlaneNormalize(&pl, &pl);

    D3DXVECTOR3 v;
    temp1 = orig + dir * 1000000.0f;
    if(D3DXPlaneIntersectLine(&v, &pl, &orig, &temp1) == nullptr) return false;

    if(ox != nullptr) *ox = TruncFloat(v.x / GLOBAL_SCALE);
    if(oy != nullptr) *oy = TruncFloat(v.y / GLOBAL_SCALE);

    return true;
}

bool CMatrixMap::UnitPickWorld(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* ox, float* oy)
{
    D3DXPLANE pl;
    D3DXVECTOR3 temp1 = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3 temp2 = { 1.0f, 0.0f, 0.0f };
    D3DXVECTOR3 temp3 = { 0.0f, 1.0f, 0.0f };
    D3DXPlaneFromPoints(&pl, &temp1, &temp2, &temp3);
    //D3DXPlaneNormalize(&pl, &pl);

    D3DXVECTOR3 v;
    temp1 = orig + dir * 1000000.0f;
    if(D3DXPlaneIntersectLine(&v, &pl, &orig, &temp1) == nullptr) return false;

    if(ox != nullptr) *ox = v.x;
    if(oy != nullptr) *oy = v.y;

    return true;
}

void CMatrixMap::StaticClear()
{
    while(m_AllObjects.Len() > 0)
    {
        StaticDelete(*m_AllObjects.Buff<CMatrixMapStatic*>());
    }

    CMatrixMapObject::ClearTextures();
}

void CMatrixMap::StaticDelete(CMatrixMapStatic* ms)
{
    if(ms->IsDIP()) return; // already deleted
    ms->SetDIP();

    if(m_NextLogicObject == ms)
    {
        m_NextLogicObject = ms->GetNextLogic();
    }

    CMatrixMapStatic::RemoveFromSorted(ms);

    CMatrixMapStatic** sb = m_AllObjects.Buff<CMatrixMapStatic*>();
    CMatrixMapStatic** se = m_AllObjects.BuffEnd<CMatrixMapStatic*>();
    CMatrixMapStatic** ses = se - 1;

    if(ms->InLT())
    {
        ms->DelLT();
        //remove this object from end of array
        while(sb < se)
        {
            --se;
            if(*se == ms && ses != se)
            {
                *se = *ses;
            }
        }
    }
    else
    {
        // remove this object from begin of array
        while(sb < se)
        {
            if(*sb == ms && ses != sb)
            {
                *sb = *ses;
            }
            ++sb;
        }
    }

    m_AllObjects.SetLenNoShrink(m_AllObjects.Len() - sizeof(CMatrixMapStatic*));

	if(ms->GetObjectType() == OBJECT_TYPE_MAPOBJECT)
    {
        HDelete(CMatrixMapObject, (CMatrixMapObject*)ms, Base::g_MatrixHeap);
        //delete (CMatrixMapObject*)ms;
    }
    else if(ms->IsRobot())
    {
        HDelete(CMatrixRobotAI, (CMatrixRobotAI*)ms, Base::g_MatrixHeap);
        //delete (CMatrixRobotAI*)ms;
    }
    else if(ms->IsTurret())
    {
        HDelete(CMatrixTurret, (CMatrixTurret*)ms, Base::g_MatrixHeap);
        //delete (CMatrixTurret*)ms;
    }
    else if(ms->IsFlyer())
    {
        HDelete(CMatrixFlyer, (CMatrixFlyer*)ms, Base::g_MatrixHeap);
        //delete (CMatrixFlyer*)ms;
    }
    else if(ms->IsBuilding())
    {
        HDelete(CMatrixBuilding, (CMatrixBuilding*)ms, Base::g_MatrixHeap);
        //delete (CMatrixBuilding*)ms;
	}
    else ERROR_S(L"CMatrixMap::StaticDelete Error!");
}

//CMatrixMapStatic* CMatrixMap::StaticAdd(EObjectType type, bool add_to_logic)
//{
//DTRACE();
//
//	CMatrixMapStatic* ms;
//	if (type==OBJECT_TYPE_MAPOBJECT) ms=HNew(g_MatrixHeap) CMatrixMapObject();
//	else if(type==OBJECT_TYPE_ROBOT_AI) ms=HNew(g_MatrixHeap) CMatrixRobotAI();
//	else if(type==OBJECT_TYPE_BUILDING) ms=HNew(g_MatrixHeap) CMatrixBuilding();
//    else if(type==OBJECT_TYPE_TURRET) ms=HNew(g_MatrixHeap) CMatrixTurret();
//    else if(type==OBJECT_TYPE_FLYER) ms=HNew(g_MatrixHeap) CMatrixFlyer();
//	else ERROR_E;
//
////#ifdef _DEBUG
////    CWStr c(L"New obj ");
////    if (type==OBJECT_TYPE_MAPOBJECT) c+=L" mapobj: ";
////	else if(type==OBJECT_TYPE_ROBOT_AI) c+=L" robot: ";
////	else if(type==OBJECT_TYPE_BUILDING) c+=L" build: ";
////    else if(type==OBJECT_TYPE_TURRET) c+=L" cannon: ";
////    else if(type==OBJECT_TYPE_FLYER) c+=L" flyer: ";
////    c.AddHex(ms);
////    c += L"\n";
////    SLOG("objlog.txt", c.Get());
////#endif
//    
//    m_AllObjects.Expand(sizeof(CMatrixMapStatic*));
//    CMatrixMapStatic** e = m_AllObjects.BuffEnd<CMatrixMapStatic*>();
//    *(e-1) = ms;
//
//
//	if(add_to_logic && type != OBJECT_TYPE_MAPOBJECT)
//    {
//		ms->AddLT();
//	}
//
//	return ms;
//}


void CMatrixMap::MacrotextureClear()
{
	if(m_Macrotexture) m_Macrotexture = nullptr;
}

void CMatrixMap::MacrotextureInit(const CWStr& path)
{
    MacrotextureClear();

    m_Macrotexture = (CTextureManaged*)g_Cache->Get(cc_TextureManaged, path.Get());

    int cnt = path.GetCountPar(L"?");
    for(int i = 1; i < cnt; ++i)
    {
        CWStr op(path.GetStrPar(i, L"?").Trim(), g_CacheHeap);
        if(op.CompareFirst(L"SIM")) m_MacrotextureSize = op.GetInt();
    }
}


void CMatrixMap::ClearSide()
{
    if(m_Side)
    {
        for(int i = 0; i < m_SidesCount; i++)
        {
            m_Side[i].CMatrixSideUnit::~CMatrixSideUnit();
        }

        HFree(m_Side, Base::g_MatrixHeap);
        m_Side = nullptr;
    }

    m_PlayerSide = nullptr;
    if(m_NeutralSideColorTexture)
    {
        CCache::Destroy(m_NeutralSideColorTexture);
        m_NeutralSideColorTexture = nullptr;
    }
    m_SidesCount = 0;
}

/*
void CMatrixMap::LoadTactics(CBlockPar& bp)
{
DTRACE();

    //bp.SaveInTextFile(L"#tactics.txt");
    for(int sides = 1; sides <= 3; ++sides)
    {
        CMatrixSideUnit* side = GetSideById(sides);
        side->m_TacticsPar = bp.BlockPathGet(L"Tactics");
    }
}
*/

void CMatrixMap::LoadSide(CBlockPar& bp)
{
	ClearSide();

	int cnt = bp.ParCount();
    ASSERT(cnt == TOTAL_SIDES); // there are 4 sides + one neutral. please updated data.txt

	m_SidesCount = cnt - 1;
    m_Side = (CMatrixSideUnit*)HAllocClear(m_SidesCount * sizeof(CMatrixSideUnit), Base::g_MatrixHeap);
    for(int i = 0; i < m_SidesCount; ++i) new(m_Side + i) CMatrixSideUnit();

    int idx = 0;
    for(int i = 0; i < cnt; ++i)
    {
        int id = bp.ParGetName(i).GetInt();
        const CWStr* name = &bp.ParGet(i);
        dword color = (dword(name->GetIntPar(1, L",") & 255) << 16) | (dword(name->GetIntPar(2, L",") & 255) << 8) | dword(name->GetIntPar(3, L",") & 255);
        dword colorMM = (dword(name->GetIntPar(5, L",") & 255) << 16) | (dword(name->GetIntPar(6, L",") & 255) << 8) | dword(name->GetIntPar(7, L",") & 255);

        if(!id)
        {
            m_NeutralSideColor = color;
            m_NeutralSideColorMM = colorMM;
            continue;
        }

        m_Side[idx].m_Id = id;

        m_Side[idx].m_Constructor->SetSide(id);
        m_Side[idx].m_Color = color;
        m_Side[idx].m_ColorMM = colorMM;
        m_Side[idx].m_ColorTexture = nullptr;
        m_Side[idx].m_Name = name->GetStrPar(0, L",");
        ++idx;

        if(id == PLAYER_SIDE)
        {
            m_PlayerSide = m_Side;
            m_Side->InitPlayerSide();
        }
    }

    //m_PlayerSide = GetSideById(PLAYER_SIDE);
}

void CMatrixMap::WaterClear()
{
DTRACE();

    if(m_Water) 
    {
        HDelete(CMatrixWater, m_Water, Base::g_MatrixHeap);
        m_Water = nullptr;
    }

    if(m_VisWater)
    {
        HDelete(CBuf, m_VisWater, Base::g_MatrixHeap);
        m_VisWater = nullptr;
    }

    SInshorewave::MarkAllBuffersNoNeed();
}

void CMatrixMap::WaterInit()
{
DTRACE();

    if(!m_Water) m_Water = HNew(Base::g_MatrixHeap) CMatrixWater;
    else m_Water->Clear();

    if(!m_VisWater) m_VisWater = HNew(Base::g_MatrixHeap) CBuf(Base::g_MatrixHeap);
    else m_VisWater->Clear();

    m_Water->Init();

}

void CMatrixMap::BeforeDraw()
{
    ++m_CurFrame; //uniq number per ~500 days

    m_Camera.BeforeDraw();

	CPoint p(m_Cursor.GetPos());
	//ScreenToClient(g_Wnd, &p);
	m_Camera.CalcPickVector(p, m_MouseDir);

    /*
    {
        static bool start = false;
        static bool end = false;
        static D3DXVECTOR3 s, e;

        if(m_KeyDown && m_KeyScan == KEY_F5)
        {
            start = true;
            s = m_Camera.GetFrustumCenter();
            m_KeyDown = false;

            //s = D3DXVECTOR3(2946.17f, 1301.01f, 142.9f);
        }

        if(m_KeyDown && m_KeyScan == KEY_F6)
        {
            end = true;
            e = s + m_MouseDir * 1000;
            m_KeyDown = false;
            //e = D3DXVECTOR3(3827.65f, 1731.25f, -51.7776f);
        }

        if(start && end)
        {
            CHelper::Create(1, 0)->Line(s, e);

            D3DXVECTOR3 hit = e;
            CMatrixMapStatic* ms = Trace(&hit, s, e, TRACE_ALL, nullptr);

            if(IS_TRACE_STOP_OBJECT(ms)) ms->SetTerainColor(0xFF00FF00);
            
            CHelper::Create(1,0)->Line(hit-D3DXVECTOR3(0, 0, 100), hit + D3DXVECTOR3(0, 0, 100));
            
            //CMatrixTurret* ca = (CMatrixTurret*)0x04d58d60;
            //CVectorObjectAnim* o = ca->m_Module[1].m_Graph;
            //
            //SVOGeometrySimple* gs = o->VO()->GetGS();

            //D3DXVECTOR3 gsc = gs->m_AsIs.m_Center;
            //D3DXVec3TransformCoord(&gsc, &gsc, &ca->m_Module[1].m_Matrix);

            //CHelper::Create(1, 0)->Line(gsc-D3DXVECTOR3(0, 0, 100), gsc + D3DXVECTOR3(0, 0, 100));
            //CHelper::Create(1, 0)->Line(gsc-D3DXVECTOR3(0, 100, 0), gsc + D3DXVECTOR3(0, 100, 0));
            //CHelper::Create(1, 0)->Line(gsc-D3DXVECTOR3(100, 0, 0), gsc + D3DXVECTOR3(100, 0, 0));
        }
    }
    */

    //TAKT_BEGIN();
    //����� ��������� ��������� m_TraceStopPos � m_TraceStopObj
    m_TraceStopObj = Trace(&m_TraceStopPos, m_Camera.GetFrustumCenter(), m_Camera.GetFrustumCenter() + (m_MouseDir * 10000.0f), TRACE_ALL, g_MatrixMap->GetPlayerSide()->GetUnitUnderManualControl());

    if(IS_TRACE_STOP_OBJECT(m_TraceStopObj))
    {
        if(m_TraceStopObj->GetObjectType() == OBJECT_TYPE_BUILDING)
        {
            ((CMatrixBuilding*)m_TraceStopObj)->ShowHitpoints();
        }
        else if(m_TraceStopObj->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
        {
            ((CMatrixRobotAI*)m_TraceStopObj)->ShowHitpoints();
        }
        else if(m_TraceStopObj->GetObjectType() == OBJECT_TYPE_TURRET)
        {
            ((CMatrixTurret*)m_TraceStopObj)->ShowHitpoints();

            /*
            for(int i = 0; i < m_TraceStopObj->GetGroupCnt(); ++i)
            {
                CMatrixMapGroup* g =  m_TraceStopObj->GetGroup(i);
                g->DrawBBox();

                m_DI.ShowScreenText(L"Group" + CWStr(i), CWStr(g->GetPos0().x) + L", " + CWStr(g->GetPos0().y));
            }
            */
        }
        else if(m_TraceStopObj->GetObjectType() == OBJECT_TYPE_FLYER)
        {
            ((CMatrixFlyer*)m_TraceStopObj)->ShowHitpoints();
        }

#ifdef _DEBUG
        if(m_TraceStopObj->GetObjectType() == OBJECT_TYPE_MAPOBJECT) m_DI.ShowScreenText(L"Under cursor", L"Mesh", 1000);
        else if(m_TraceStopObj->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
        {
            m_DI.ShowScreenText(L"Under cursor", CWStr().Format(L"Robot <b=16><u>   S<b=10><i> T<i> G<i>", dword(m_TraceStopObj), m_TraceStopObj->GetSide(), ((CMatrixRobotAI*)m_TraceStopObj)->GetTeam(), ((CMatrixRobotAI*)m_TraceStopObj)->GetGroupLogic()).Get(), 1000);
        }
        else if(m_TraceStopObj->GetObjectType() == OBJECT_TYPE_TURRET) m_DI.ShowScreenText(L"Under cursor", L"Cannon", 1000);
        else if(m_TraceStopObj->GetObjectType() == OBJECT_TYPE_BUILDING)
        {

            m_DI.ShowScreenText(L"Under cursor", CWStr(L"Building: 0x", g_CacheHeap).AddHex((void*)m_TraceStopObj).Get(), 1000);
        }
        else if(m_TraceStopObj->GetObjectType() == OBJECT_TYPE_FLYER) m_DI.ShowScreenText(L"Under cursor", L"Flyer", 1000);
#endif
    }

    //flyer
    CMatrixSideUnit* player_side = GetPlayerSide();
    //CDText::T("sel", CStr(player_side->m_CurrSel));
    if(player_side->IsManualControlMode() && player_side->GetUnitUnderManualControl()->GetObjectType() == OBJECT_TYPE_FLYER && (GetAsyncKeyState(g_Config.m_KeyActions[KA_AUTO]) & 0x8000) == 0x8000 && g_IFaceList->m_InFocus != INTERFACE)
    {
        CMatrixFlyer* fl = (CMatrixFlyer*)player_side->GetUnitUnderManualControl();

        SPlane hp;
        hp.BuildFromPointNormal(hp, fl->GetPos(), D3DXVECTOR3(0, 0, 1));

        float t = 0;
        bool hit = hp.FindIntersect(m_Camera.GetFrustumCenter(), m_MouseDir, t);
        if(hit)
        {
            fl->SetTarget(D3DXVECTOR2(m_Camera.GetFrustumCenter().x + m_MouseDir.x * t, m_Camera.GetFrustumCenter().y + m_MouseDir.y * t));

            //if(player_side->HasFlyer()) player_side->Select(HELICOPTER, nullptr);

            //D3DXVECTOR3 p = m_Camera.GetFrustumCenter() + m_MouseDir * t;
            //CHelper::Create(1, 0)->Line(p, p + D3DXVECTOR3(0, 0, -100));
        }

        //player_side->GetFlyer()->SetTarget(D3DXVECTOR2(m_TraceStopPos.x, m_TraceStopPos.y));
    }

#ifdef _DEBUG

    if(player_side->m_ActiveObject != m_TraceStopObj && player_side->m_ActiveObject && player_side->m_ActiveObject->GetObjectType() == OBJECT_TYPE_FLYER && (GetAsyncKeyState(g_Config.m_KeyActions[KA_AUTO]) & 0x8000) == 0x8000 && g_IFaceList->m_InFocus != INTERFACE)
    {
        CMatrixFlyer* fl = (CMatrixFlyer*)player_side->m_ActiveObject;

        SPlane hp;
        hp.BuildFromPointNormal(hp, fl->GetPos(), D3DXVECTOR3(0, 0, 1));

        float t = 0;
        bool hit = hp.FindIntersect(m_Camera.GetFrustumCenter(), m_MouseDir, t);
        if(hit)
        {
            fl->SetTarget(D3DXVECTOR2(m_Camera.GetFrustumCenter().x + m_MouseDir.x * t, m_Camera.GetFrustumCenter().y + m_MouseDir.y * t));

            CMatrixSideUnit* player_side = GetPlayerSide();
            //if(player_side->HasFlyer()) player_side->Select(HELICOPTER, nullptr);

            //D3DXVECTOR3 p = m_Camera.GetFrustumCenter() + m_MouseDir * t;
            //CHelper::Create(1, 0)->Line(p, p + D3DXVECTOR3(0, 0, -100));
        }

        //player_side->GetFlyer()->SetTarget(D3DXVECTOR2(m_TraceStopPos.x, m_TraceStopPos.y));
    }

#endif

    //CDText::T("t", CStr(int(m_TraceStopObj)));

    //if(m_KeyDown && m_KeyScan == KEY_PGDN) { m_KeyDown = false; m_Minimap.SetOutParams(m_Minimap.GetScale() * 0.8f); }
    //if(m_KeyDown && m_KeyScan == KEY_PGUP) { m_KeyDown = false; m_Minimap.SetOutParams(m_Minimap.GetScale() * 1.25f); }

    //static float angle = 0;
    //if(m_KeyDown && m_KeyScan == KEY_PGDN) { m_KeyDown = false; angle += 0.01f; }
    //if(m_KeyDown && m_KeyScan == KEY_PGUP) { m_KeyDown = false; angle -= 0.01f; }

    //m_Minimap.SetAngle(angle);

    m_Minimap.SetOutParams(D3DXVECTOR2(m_Camera.GetXYStrategy().x, m_Camera.GetXYStrategy().y));
//#ifdef MINIMAP_SUPPORT_ROTATION
//    m_Minimap.SetAngle(-m_Camera.GetAngleZ());
//#endif

    //D3DXVECTOR3 out;
    //if(TraceLand(&out, m_Camera.GetFrustumCenter() + D3DXVECTOR3(0,100,0), m_MouseDir))
    //{
    //    CHelper::Create(1,0)->Line(m_Camera.GetFrustumCenter() + D3DXVECTOR3(0, 100, 0),out);
    //    CHelper::Create(1,0)->Line(out - D3DXVECTOR3(0, 0, 100), out + D3DXVECTOR3(0, 0, 100));
    //}

    //D3DXVECTOR3 p0, p1;
    //p0 = m_TraceStopPos;
    //p0.z = GetZ(p0.x, p0.y);
    //GetNormal(&p1, p0.x, p0.y);
    ////p1 = p0 + p1 * 50;
    //p1 = p0 + p1 * 100;

    //D3DXVECTOR3 p0 = m_TraceStopPos;
    //CHelper::Create(1, 0)->Line(p0 - D3DXVECTOR3(0, 0, 100), p0 + D3DXVECTOR3(0, 0, 100));
    //CHelper::Create(1, 0)->Line(p0 - D3DXVECTOR3(0, 100, 0), p0 + D3DXVECTOR3(0, 100, 0));
    //CHelper::Create(1, 0)->Line(p0 - D3DXVECTOR3(100, 0, 0), p0 + D3DXVECTOR3(100, 0, 0));


    //static CMatrixEffectPointLight *pl = nullptr;
    //if(pl == nullptr)
    //{
    //    pl = (CMatrixEffectPointLight *)CMatrixEffect::CreatePointLight(p1, 100, 0xFFFFFF00, true);
    //}
    //else
    //{
    //    pl->SetPos(p1);
    //}


    //int ux = int(p0.x / GLOBAL_SCALE);
    //int uy = int(p0.y / GLOBAL_SCALE);
    //SMatrixMapUnit* mu = UnitGetTest(ux, uy);

    //CHelper::Create(1, 0)->BoundBox(D3DXVECTOR3(ux * GLOBAL_SCALE, uy * GLOBAL_SCALE, 0), D3DXVECTOR3((ux + 1) * GLOBAL_SCALE, (uy + 1) * GLOBAL_SCALE, 10));

    //if(mu)
    //{
    //    CWStr str;
    //    if(mu->IsBridge()) str += L"Bridge,";
    //    if(mu->IsFlat()) str += L"Flat,";
    //    if(mu->IsInshore()) str += L"Inshore,";
    //    if(mu->IsWater()) str += L"Water,";
    //    if(mu->IsLand()) str += L"Land,";
    //    str += L"ok";
    //    m_DI.ShowScreenText(L"Unit type", str.Get());
    //}

    //CDText::T("tracez",CStr(p0.z));
    
    //TAKT_END("Trace");

    //int gx = int(m_TraceStopPos.x / (GLOBAL_SCALE*MATRIX_MAP_GROUP_SIZE));
    //int gy = int(m_TraceStopPos.y / (GLOBAL_SCALE*MATRIX_MAP_GROUP_SIZE));

    //m_DI.ShowScreenText(L"group", CWStr(gx) + L"," + CWStr(gy));
    //GetGroupByIndex(gx,gy)->DrawBBox();

    /*
    D3DXVECTOR3 p0 = GetPlayerSide()->m_HelicopterPos;
    D3DXVECTOR3 p1;
    Trace(&p1, p0, p0 + ((m_TraceStopPos-p0)*5), TRACE_ALL, TRACE_STOP_FLYER);

    CHelper::Create(1,0)->Line(p0, p1);
    CHelper::Create(1,0)->Line(p1, p1 + D3DXVECTOR3(0,0,100));
    */

    CalcMapGroupVisibility();

    BeforeDrawLandscape();
    if(CTerSurface::IsSurfacesPresent()) BeforeDrawLandscapeSurfaces();

    //build objects sort array

    CMatrixMapStatic::SortBegin();
    //CMatrixMapStatic::OnEndOfDraw(); // this will call OnOutScreen for all

	int cnt = m_VisibleGroupsCount;
	CMatrixMapGroup** md = m_VisibleGroups;
	while(cnt-- > 0)
    {
        if(*(md) != nullptr)
        {
            (*(md))->SortObjects(m_Camera.GetViewMatrix());
            (*(md))->BeforeDrawSurfaces();
        }
        ++md;
	}

    for(int od = 0; od < m_AD_Obj_cnt; ++od)
    {
        m_AD_Obj[od]->Sort(m_Camera.GetViewMatrix());
        if(m_AD_Obj[od]->GetObjectType() == OBJECT_TYPE_FLYER)
        {
            if(((CMatrixFlyer*)m_AD_Obj[od])->IsCarryingRobot())
            {
                ((CMatrixFlyer*)m_AD_Obj[od])->GetCarryingRobot()->Sort(m_Camera.GetViewMatrix());
            }
        }
    }

    if(FLAG(m_Flags, MMFLAG_NEEDRECALCTER))
    {
        CMatrixMapStatic::SortEndRecalcTerainColor();
        RESETFLAG(m_Flags, MMFLAG_NEEDRECALCTER);
    }

    if(FLAG(g_Config.m_DIFlags, DI_VIS_OBJ)) m_DI.ShowScreenText(L"Visible objects", CWStr(CMatrixMapStatic::GetVisObjCnt(), Base::g_MatrixHeap));

    CMatrixMapStatic::SortEndBeforeDraw();

    //cannon for build before draw
    //player_side = GetPlayerSide();
    if(player_side->m_CannonForBuild.m_Cannon)
    {
        if(g_IFaceList->m_InFocus == UNKNOWN)
        {
            player_side->m_CannonForBuild.m_Cannon->SetVisible(true);
            player_side->m_CannonForBuild.m_Cannon->BeforeDraw();
        }
        else
        {
            player_side->m_CannonForBuild.m_Cannon->SetVisible(false);
        }
    }

    //CVectorObject::m_VB->PrepareAll();

    m_Minimap.BeforeDraw();

    CSprite::BeforeDraw();
    for(CMatrixEffect* e = m_EffectsFirst; e != nullptr; e = e->m_Next) e->BeforeDraw();
    CMatrixProgressBar::BeforeDrawAll();
    m_Water->BeforeDraw();

    g_IFaceList->BeforeRender();
}

bool CMatrixMap::CalcVectorToLandscape(const D3DXVECTOR2& pos, D3DXVECTOR2& dir)
{
    const float maxrad = GLOBAL_SCALE * 5;
    const float minrad = GLOBAL_SCALE * 5;
    const float radstep = GLOBAL_SCALE;

    dir.x = 0;
    dir.y = 0;

    const float up_level = 0.0f;
    const float down_level = -20.1f;


    float zz;

    for (float r = minrad; r<=maxrad; r+= radstep)
    {
        float astep = float(INVERT(r * GLOBAL_SCALE));
        for (float a = 0; a < M_PI_MUL(2); a += astep)
        {
            float ca = TableCos(a);
            float sa = TableSin(a);

            float x = pos.x + r * ca;
            float y = pos.y + r * sa;

            float z = GetZ(x,y);

            if (z < down_level) zz = 0;
            else
                if (z > up_level) zz = 1;
                else
                    zz = ((z-down_level)/(up_level-down_level));

            dir += zz * D3DXVECTOR2(ca, sa);
        }
    }

    if (dir.x == 0 && dir.y == 0)
    {
        return false;
    }

    D3DXVec2Normalize(&dir, &dir);
    return true;
}

//#include <stdio.h>

void CMatrixMap::BeforeDrawLandscape(bool all)
{
    DTRACE();

    CMatrixMapGroup::BeforeDrawAll();
    if (m_Macrotexture) m_Macrotexture->Prepare();

	int cnt;
	CMatrixMapGroup * * md;

    if (all)
    {
	    cnt = m_GroupSize.x*m_GroupSize.y;
	    md = m_Group;
    } else
    {
	    cnt = m_VisibleGroupsCount;
	    md = m_VisibleGroups;
    }

	while(cnt>0)
    {
        if (*md) (*md)->BeforeDraw();
        md++;
		cnt--;
    }
}

void CMatrixMap::BeforeDrawLandscapeSurfaces(bool all)
{
DTRACE();

    CTerSurface::BeforeDrawAll();
    if(m_Macrotexture) m_Macrotexture->Prepare();

	int cnt;
	CMatrixMapGroup** md;

    if(all)
    {
	    cnt = m_GroupSize.x * m_GroupSize.y;
	    md = m_Group;
    }
    else
    {
	    cnt = m_VisibleGroupsCount;
	    md = m_VisibleGroups;
    }

	while(cnt > 0)
    {
        if((*md)) (*md)->BeforeDrawSurfaces();
        ++md;
		--cnt;
    }
}

void CMatrixMap::DrawLandscapeSurfaces(bool all)
{
DTRACE();

    for(int i = 0; i < 8; i++)
    {
        ASSERT_DX(g_D3DD->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD) (&m_BiasTer))));
	    ASSERT_DX(g_Sampler.SetState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	    ASSERT_DX(g_Sampler.SetState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
        ASSERT_DX(g_Sampler.SetState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));
    }

    /*
    ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD, &GetIdentityMatrix()));

    int cnt;
    CMatrixMapGroup** md;

    if(all)
    {
        cnt = m_GroupSize.x * m_GroupSize.y;
        md = m_Group;
    }
    else
    {
        cnt = m_VisibleGroupsCount;
        md = m_VisibleGroups;
    }
    while(cnt > 0)
    {
        if(*md) (*md)->DrawSurfaces();
        ++md;
        --cnt;
    }
    */

    CTerSurface::DrawAll();
}

void CMatrixMap::DrawLandscape(bool all)
{
DTRACE();

#ifdef _DEBUG
    CDText::T("land_dp_calls", CStr(CMatrixMapGroup::m_DPCalls));
    CMatrixMapGroup::m_DPCalls = 0;
#endif

    ASSERT_DX(g_D3DD->SetSamplerState(0,D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)(&m_BiasTer))));
    ASSERT_DX(g_Sampler.SetState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR));
    ASSERT_DX(g_Sampler.SetState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT));
    ASSERT_DX(g_Sampler.SetState(0,D3DSAMP_MIPFILTER,D3DTEXF_LINEAR));

	//ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD, &GetIdentityMatrix()));

	ASSERT_DX(g_D3DD->SetFVF(MATRIX_MAP_BOTTOM_VERTEX_FORMAT));
    ASSERT_DX(g_D3DD->SetRenderState( D3DRS_LIGHTING, FALSE));

	int cnt;
	CMatrixMapGroup** md;

    if(all)
    {
	    cnt = m_GroupSize.x*m_GroupSize.y;
	    md = m_Group;
	    while(cnt > 0)
        {
            if ((*md)) (*md)->Draw();
            ++md;
		    --cnt;
        }
    }
    else
    {
	    cnt = m_VisibleGroupsCount;
	    md = m_VisibleGroups;
	    while(cnt > 0)
        {
            (*md)->Draw();

#ifdef _DEBUG
    #if DRAW_LANDSCAPE_SETKA
        CHelper::Create(1, 0)->Line(D3DXVECTOR3((*md)->GetPos0().x, (*md)->GetPos0().y, 10.0f),
            D3DXVECTOR3((*md)->GetPos0().x, (*md)->GetPos1().y, 10.0f),
            0xFFFF0000, 0xFFFF0000);

        CHelper::Create(1, 0)->Line(D3DXVECTOR3((*md)->GetPos0().x, (*md)->GetPos1().y, 10.0f),
            D3DXVECTOR3((*md)->GetPos1().x, (*md)->GetPos1().y, 10.0f),
            0xFFFF0000, 0xFFFF0000);

        CHelper::Create(1, 0)->Line(D3DXVECTOR3((*md)->GetPos1().x, (*md)->GetPos1().y, 10.0f),
            D3DXVECTOR3((*md)->GetPos1().x, (*md)->GetPos0().y, 10.0f),
            0xFFFF0000, 0xFFFF0000);

        CHelper::Create(1, 0)->Line(D3DXVECTOR3((*md)->GetPos1().x, (*md)->GetPos0().y, 10.0f),
            D3DXVECTOR3((*md)->GetPos0().x, (*md)->GetPos0().y, 10.0f),
            0xFFFF0000, 0xFFFF0000);
    #endif
#endif

            ++md;
		    --cnt;
	    }
    }

    /*
    for(int x = 0; x < m_Size.x; ++x)
    {
        for(int y = 0; y < m_Size.y; ++y)
        {
            D3DXVECTOR3 p0(x * GLOBAL_SCALE, y * GLOBAL_SCALE, 30);
            D3DXVECTOR3 p1((x + 1) * GLOBAL_SCALE, y * GLOBAL_SCALE, 30);
            D3DXVECTOR3 p2((x + 1) * GLOBAL_SCALE, (y + 1) * GLOBAL_SCALE, 30);
            SMatrixMapUnit* mu = UnitGet(x, y);
            if(mu->IsBridge()) CHelper::Create(1, 0)->Triangle(p0, p1, p2, 0xFFFF0000);
            //else CHelper::Create(1, 0)->Triangle(p0, p1, p2, 0xFF0000FF);
        }
    }
    */
}

void CMatrixMap::DrawObjects(void)
{
	//ASSERT_DX(g_D3DD->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0));
	//ASSERT_DX(g_D3DD->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE));

    CVectorObject::DrawBegin();

    //if(m_KeyScan != KEY_L || m_KeyDown == false)

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_LIGHTING, TRUE));

    ASSERT_DX(g_D3DD->SetRenderState( D3DRS_AMBIENT, m_AmbientColorObj ));
	ASSERT_DX(g_D3DD->SetRenderState( D3DRS_TEXTUREFACTOR, 0xFFFFFFFF));

    //ASSERT_DX(g_D3DD->SetRenderState( D3DRS_TEXTUREFACTOR, m_AmbientColorObj ));
	//ASSERT_DX(g_D3DD->SetRenderState( D3DRS_AMBIENT, 0xFFFFFFFF));

    for(int i = 0; i < 8; ++i)
    {
        ASSERT_DX(g_D3DD->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD) (&m_BiasRobots))));
	    ASSERT_DX(g_Sampler.SetState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	    ASSERT_DX(g_Sampler.SetState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
        ASSERT_DX(g_Sampler.SetState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));
    }

    //draw all objects
    CMatrixMapStatic::SortEndDraw();

    //draw cannon for build
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_LIGHTING, FALSE));

    CMatrixSideUnit* player_side = GetPlayerSide();
    if(player_side->m_CannonForBuild.m_Cannon)
    {
        if(player_side->m_CannonForBuild.m_Cannon->IsVisible()) player_side->m_CannonForBuild.m_Cannon->Draw();
    }

    CVectorObject::DrawEnd();
}

void CMatrixMap::DrawWater(void)
{
DTRACE();

    if(!m_Water->IsReadyForDraw()) return;

    //g_D3DD->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

	//g_D3DD->SetMaterial(&mtrl);
	//g_D3DD->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
    //g_D3DD->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR1);
    
    ASSERT_DX(g_Sampler.SetState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
    ASSERT_DX(g_Sampler.SetState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
    ASSERT_DX(g_Sampler.SetState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));

    for(int i = 0; i < 4; i++)
    {
        ASSERT_DX(g_D3DD->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD) (&m_BiasWater))));

        ASSERT_DX(g_D3DD->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
        ASSERT_DX(g_D3DD->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));
    }

    D3DXMATRIX m = this->GetIdentityMatrix();
   
    m._11 *= GLOBAL_SCALE * (float)(MAP_GROUP_SIZE) / (float)(WATER_SIZE);
    m._22 *= GLOBAL_SCALE * (float)(MAP_GROUP_SIZE) / (float)(WATER_SIZE);
    m._33 *= GLOBAL_SCALE * (float)(MAP_GROUP_SIZE) / (float)(WATER_SIZE);
    m._43 = WATER_LEVEL;

    int curpass;
    for(curpass = 0; curpass < g_Render->m_WaterPassAlpha; ++curpass)
    {
        g_Render->m_WaterAlpha(m_Water->m_WaterTex1, m_Water->m_WaterTex2, curpass);

	    int cnt = m_VisibleGroupsCount;
        CMatrixMapGroup** md = m_VisibleGroups;

	    while((cnt--) > 0)
        {
            if(!(*(md))->HasWater())
            {
                ++md;
                continue;
            }

            if(curpass == 0) ASSERT_DX(g_D3DD->SetTexture(0,(*md)->GetWaterAlpha()->Tex()));

            const D3DXVECTOR2 &p = (*(md))->GetPos0();
            m._41 = p.x;
            m._42 = p.y;
            m_Water->Draw(m);
            ++md;
	    }
    }

    g_Render->m_WaterClearAlpha();


    for(curpass = 0; curpass < g_Render->m_WaterPassSolid; ++curpass)
    {
        g_Render->m_WaterSolid(m_Water->m_WaterTex1, m_Water->m_WaterTex2, curpass);

        D3DXVECTOR2* bp = m_VisWater->Buff<D3DXVECTOR2>();
        D3DXVECTOR2* ep = m_VisWater->BuffEnd<D3DXVECTOR2>();

        while(bp < ep)
        {
		    //m_VisWater->BufGet(&p,sizeof(D3DXVECTOR2));
            m._41 = bp->x;
            m._42 = bp->y;
            m_Water->Draw(m);

            ++bp;
        }
    }

    g_Render->m_WaterClearSolid();

    //g_D3DD->SetRenderState( D3DRS_NORMALIZENORMALS,  FALSE );

    int cnt;
    CMatrixMapGroup** md;
    if (SInshorewave::m_Tex)
    {
        SInshorewave::DrawBegin();

	    cnt = m_VisibleGroupsCount;
	    md = m_VisibleGroups;
	    while((cnt--) > 0)
        {
            if (!(*(md))->HasWater())
            {
                ++md;
                continue;
            }

            (*md)->DrawInshorewaves();
            ++md;
	    }
        SInshorewave::DrawEnd();
    }
}

void CMatrixMap::DrawShadowsProjFast(void)
{
    DTRACE();

    CVOShadowProj::BeforeRenderAll();

	//g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE,		TRUE);
	g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE,		TRUE);

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA));

	g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR,			m_ShadowColor);

    SetColorOpSelect(0, D3DTA_TFACTOR);
    SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_TFACTOR);
    SetColorOpDisable(1);
    

	ASSERT_DX(g_Sampler.SetState(0, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR));
	ASSERT_DX(g_Sampler.SetState(0, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR));
    ASSERT_DX(g_Sampler.SetState(0, D3DSAMP_MIPFILTER,		D3DTEXF_NONE));

	g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);

	ASSERT_DX(g_D3DD->SetSamplerState(0, D3DSAMP_ADDRESSU,			D3DTADDRESS_CLAMP));
	ASSERT_DX(g_D3DD->SetSamplerState(0, D3DSAMP_ADDRESSV,			D3DTADDRESS_CLAMP));

    CMatrixMapStatic::SortEndDrawShadowProj();
	int cnt = m_VisibleGroupsCount;
	CMatrixMapGroup **md = m_VisibleGroups;
	while(cnt>0)
    {
        (*md)->DrawShadowProj();
        md++;
		cnt--;
	}

	//g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE,		FALSE);
	g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE,		FALSE);

	g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);

	ASSERT_DX(g_D3DD->SetSamplerState(0,D3DSAMP_ADDRESSU,			D3DTADDRESS_WRAP));
	ASSERT_DX(g_D3DD->SetSamplerState(0,D3DSAMP_ADDRESSV,			D3DTADDRESS_WRAP));
}

void CMatrixMap::DrawShadows()
{
    CVOShadowStencil::BeforeRenderAll();

	ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD,&GetIdentityMatrix()));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,	FALSE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILENABLE,	TRUE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SHADEMODE,		D3DSHADE_FLAT));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILFUNC,		D3DCMP_ALWAYS ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILZFAIL,	D3DSTENCILOP_KEEP ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILFAIL,		D3DSTENCILOP_KEEP ));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILREF,		0x1));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILMASK,		0xffffffff));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILWRITEMASK,0xffffffff));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILPASS,		D3DSTENCILOP_INCR));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE,	TRUE ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_ZERO ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_ONE ));

	if(g_D3DDCaps.StencilCaps & D3DSTENCILCAPS_TWOSIDED)
    {
		ASSERT_DX(g_D3DD->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE,	TRUE));
		ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CCW_STENCILFUNC,		D3DCMP_ALWAYS ));
		ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CCW_STENCILZFAIL,	D3DSTENCILOP_KEEP ));
		ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CCW_STENCILFAIL,		D3DSTENCILOP_KEEP ));
		ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CCW_STENCILPASS,		D3DSTENCILOP_DECR ));

		ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE,			D3DCULL_NONE));

        CMatrixMapStatic::SortEndDrawShadowStencil();

		ASSERT_DX(g_D3DD->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE,	FALSE));
	} else {


        CMatrixMapStatic::SortEndDrawShadowStencil();

		ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE,			D3DCULL_CW ));
		ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILPASS,			D3DSTENCILOP_DECR ));

        CMatrixMapStatic::SortEndDrawShadowStencil();
        
	}
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE,			D3DCULL_CCW));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILPASS,			D3DSTENCILOP_INCR));

#ifdef RENDER_PROJ_SHADOWS_IN_STENCIL_PASS
    // Shadow proj

    CVOShadowProj::BeforeRenderAll();

	ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD,&GetIdentityMatrix()));

	ASSERT_DX(g_D3DD->SetSamplerState(0,D3DSAMP_ADDRESSU,			D3DTADDRESS_CLAMP));
	ASSERT_DX(g_D3DD->SetSamplerState(0,D3DSAMP_ADDRESSV,			D3DTADDRESS_CLAMP));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE,			TRUE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAFUNC, 			    D3DCMP_GREATEREQUAL));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF,			    0x8 ));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILFUNC,		D3DCMP_ALWAYS ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILZFAIL,	D3DSTENCILOP_KEEP ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILFAIL,		D3DSTENCILOP_KEEP ));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILREF,		0x1));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILMASK,		0xffffffff));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILWRITEMASK,0xffffffff));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILPASS,		D3DSTENCILOP_REPLACE));


    SetColorOpSelect(0, D3DTA_TEXTURE);
    SetAlphaOpSelect(0, D3DTA_TEXTURE);
    SetColorOpDisable(1);

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0));

    CMatrixMapStatic::SortEndDrawShadowProj();
	int cnt = m_VisibleGroupsCount;
	CMatrixMapGroup **md = m_VisibleGroups;
	while(cnt>0)
    {
        (*md)->DrawShadowProj();
        md++;
		cnt--;
	}


    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF));

	ASSERT_DX(g_D3DD->SetSamplerState(0,D3DSAMP_ADDRESSU,		D3DTADDRESS_WRAP));
	ASSERT_DX(g_D3DD->SetSamplerState(0,D3DSAMP_ADDRESSV,		D3DTADDRESS_WRAP));

	ASSERT_DX(g_D3DD->SetTexture(0,nullptr));
#endif

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILPASS,			D3DSTENCILOP_DECR ));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SHADEMODE,			D3DSHADE_GOURAUD));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILENABLE,		FALSE));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE,				D3DZB_FALSE ));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,		FALSE));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILENABLE,		TRUE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR,		0xC0C0C0C0/* 0xffffffff*/));

    SetColorOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_DIFFUSE, D3DTA_TFACTOR);
    SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_DIFFUSE, D3DTA_TFACTOR);
    SetColorOpDisable(1);

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILREF,			0x1));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILFUNC,			D3DCMP_LESSEQUAL));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILPASS,			D3DSTENCILOP_KEEP));

	ASSERT_DX(g_D3DD->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE));
    ASSERT_DX(g_D3DD->SetStreamSource( 0, GET_VB(m_ShadowVB), 0, sizeof(SShadowRectVertex) ));
    ASSERT_DX(g_D3DD->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2));

	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE,				D3DZB_TRUE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,		TRUE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_STENCILENABLE,		FALSE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE,	FALSE));
	ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE,		FALSE));
}

void CMatrixMap::DrawEffects()
{
    CMatrixEffect::DrawBegin();

    //CSortable::SortBegin();

    for(CMatrixEffect* e = m_EffectsFirst; e != nullptr; e = e->m_Next)
    {
        e->Draw();
    } 

    ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD, &GetIdentityMatrix()));
    CSprite::SortEndDraw(m_Camera.GetViewInversedMatrix(), m_Camera.GetFrustumCenter());
    CMatrixEffect::DrawEnd();
}

void CMatrixMap::DrawSky()
{
    ASSERT_DX(g_Sampler.SetState(0,D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	ASSERT_DX(g_Sampler.SetState(0,D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
	ASSERT_DX(g_Sampler.SetState(0,D3DSAMP_MIPFILTER, D3DTEXF_NONE));

    g_D3DD->SetRenderState(D3DRS_FOGENABLE, FALSE);
    g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

    //���� �������� ������� � ���������� � �������� ������� ���������
    if(g_Config.m_SkyBox != 0 && m_SkyTex[0].tex)
    {
        ASSERT_DX(g_D3DD->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP));
        ASSERT_DX(g_D3DD->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP));

        SetColorOpSelect(0, D3DTA_TEXTURE);
        SetColorOpDisable(1);
        SetAlphaOpDisable(0);

        g_D3DD->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

        D3DXMATRIX wo;
        D3DXMatrixPerspectiveFovLH(&wo, CAM_HFOV, float(g_ScreenX) * m_Camera.GetResYInversed(), 0.01f, 3);
        ASSERT_DX(g_D3DD->SetTransform(D3DTS_PROJECTION, &wo));

        ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD, &GetIdentityMatrix()));

        m_Camera.CalcSkyMatrix(wo);
        ASSERT_DX(g_D3DD->SetTransform(D3DTS_VIEW, &wo));
        //ASSERT_DX(g_D3DD->SetTransform(D3DTS_VIEW, &GetIdentityMatrix()));

        SVert_V3_C_UV verts[4];
        verts[0].color = 0xFFFFFFFF;
        verts[1].color = 0xFFFFFFFF;
        verts[2].color = 0xFFFFFFFF;
        verts[3].color = 0xFFFFFFFF;

        float cut_dn = (200 + m_Camera.GetFrustumCenter().z) * 0.5f * INVERT(g_MaxViewDistance) + 0.5f;
        float cut_up = 0.0f;

        float geo_dn = 2 * (1.0f - cut_dn) - 1;

        CInstDraw::BeginDraw(IDFVF_V3_C_UV);

        
        if(true) //������ ����������
        {
            int tex = SKY_FRONT;
            float v1 = (m_SkyTex[tex].v1 - m_SkyTex[tex].v0) * cut_dn + m_SkyTex[tex].v0;
            verts[0].p = D3DXVECTOR3(-1, 1, 1);       verts[0].tu = m_SkyTex[tex].u0; verts[0].tv = m_SkyTex[tex].v0;
            verts[1].p = D3DXVECTOR3(-1, geo_dn, 1);  verts[1].tu = m_SkyTex[tex].u0; verts[1].tv = v1;
            verts[2].p = D3DXVECTOR3(1, 1, 1);        verts[2].tu = m_SkyTex[tex].u1; verts[2].tv = m_SkyTex[tex].v0;
            verts[3].p = D3DXVECTOR3(1, geo_dn, 1);   verts[3].tu = m_SkyTex[tex].u1; verts[3].tv = v1;
            CInstDraw::AddVerts(verts, m_SkyTex[tex].tex);

            tex = SKY_RIGHT;
            v1 = (m_SkyTex[tex].v1 - m_SkyTex[tex].v0) * cut_dn + m_SkyTex[tex].v0;
            verts[0].p = D3DXVECTOR3(1, 1, 1);        verts[0].tu = m_SkyTex[tex].u0; verts[0].tv = m_SkyTex[tex].v0;
            verts[1].p = D3DXVECTOR3(1, geo_dn, 1);   verts[1].tu = m_SkyTex[tex].u0; verts[1].tv = v1;
            verts[2].p = D3DXVECTOR3(1, 1, -1);       verts[2].tu = m_SkyTex[tex].u1; verts[2].tv = m_SkyTex[tex].v0;
            verts[3].p = D3DXVECTOR3(1, geo_dn, -1);  verts[3].tu = m_SkyTex[tex].u1; verts[3].tv = v1;
            CInstDraw::AddVerts(verts, m_SkyTex[tex].tex);

            tex = SKY_LEFT;
            v1 = (m_SkyTex[tex].v1 - m_SkyTex[tex].v0) * cut_dn + m_SkyTex[tex].v0;
            verts[0].p = D3DXVECTOR3(-1, 1, -1);      verts[0].tu = m_SkyTex[tex].u0; verts[0].tv = m_SkyTex[tex].v0;
            verts[1].p = D3DXVECTOR3(-1, geo_dn, -1); verts[1].tu = m_SkyTex[tex].u0; verts[1].tv = v1;
            verts[2].p = D3DXVECTOR3(-1, 1, 1);       verts[2].tu = m_SkyTex[tex].u1; verts[2].tv = m_SkyTex[tex].v0;
            verts[3].p = D3DXVECTOR3(-1, geo_dn, 1);  verts[3].tu = m_SkyTex[tex].u1; verts[3].tv = v1;
            CInstDraw::AddVerts(verts, m_SkyTex[tex].tex);

            tex = SKY_BACK;
            v1 = (m_SkyTex[tex].v1 - m_SkyTex[tex].v0) * cut_dn + m_SkyTex[tex].v0;
            verts[0].p = D3DXVECTOR3(1, 1, -1);       verts[0].tu = m_SkyTex[tex].u0; verts[0].tv = m_SkyTex[tex].v0;
            verts[1].p = D3DXVECTOR3(1, geo_dn, -1);  verts[1].tu = m_SkyTex[tex].u0; verts[1].tv = v1;
            verts[2].p = D3DXVECTOR3(-1, 1, -1);      verts[2].tu = m_SkyTex[tex].u1; verts[2].tv = m_SkyTex[tex].v0;
            verts[3].p = D3DXVECTOR3(-1, geo_dn, -1); verts[3].tu = m_SkyTex[tex].u1; verts[3].tv = v1;
            CInstDraw::AddVerts(verts, m_SkyTex[tex].tex);
        }
        /*
        else //������ ���������� ��������
        {
        
            float W = 1.0f, H = 1.0f, L = 1.0f; //Width, Height, Length

            int tex = SKY_FRONT;
            verts[0].p = D3DXVECTOR3(-W, -H, -L);    verts[0].tu = m_SkyTex[tex].u0; verts[0].tv = m_SkyTex[tex].v0;
            verts[1].p = D3DXVECTOR3(-W, H, -L);     verts[1].tu = m_SkyTex[tex].u0; verts[1].tv = m_SkyTex[tex].v1;
            verts[2].p = D3DXVECTOR3(W, -H, -L);     verts[2].tu = m_SkyTex[tex].u1; verts[2].tv = m_SkyTex[tex].v0;
            verts[3].p = D3DXVECTOR3(W, H, -L);      verts[3].tu = m_SkyTex[tex].u1; verts[3].tv = m_SkyTex[tex].v1;
            CInstDraw::AddVerts(verts, m_SkyTex[tex].tex);

            tex = SKY_BACK;
            verts[0].p = D3DXVECTOR3(-W, -H, L);     verts[0].tu = m_SkyTex[tex].u0; verts[0].tv = m_SkyTex[tex].v0;
            verts[1].p = D3DXVECTOR3(W, -H, L);      verts[1].tu = m_SkyTex[tex].u1; verts[1].tv = m_SkyTex[tex].v0;
            verts[2].p = D3DXVECTOR3(-W, H, L);      verts[2].tu = m_SkyTex[tex].u0; verts[2].tv = m_SkyTex[tex].v1;
            verts[3].p = D3DXVECTOR3(W, H, L);       verts[3].tu = m_SkyTex[tex].u1; verts[3].tv = m_SkyTex[tex].v1;
            CInstDraw::AddVerts(verts, m_SkyTex[tex].tex);

            tex = SKY_TOP;
            verts[0].p = D3DXVECTOR3(-W, H, -L);     verts[0].tu = m_SkyTex[tex].u0; verts[0].tv = m_SkyTex[tex].v0;
            verts[1].p = D3DXVECTOR3(-W, H, L);      verts[1].tu = m_SkyTex[tex].u0; verts[1].tv = m_SkyTex[tex].v1;
            verts[2].p = D3DXVECTOR3(W, H, -L);      verts[2].tu = m_SkyTex[tex].u1; verts[2].tv = m_SkyTex[tex].v0;
            verts[3].p = D3DXVECTOR3(W, H, L);       verts[3].tu = m_SkyTex[tex].u1; verts[3].tv = m_SkyTex[tex].v1;
            CInstDraw::AddVerts(verts, m_SkyTex[tex].tex);

            tex = SKY_LEFT;
            verts[0].p = D3DXVECTOR3(-W, -H, L);     verts[0].tu = m_SkyTex[tex].u0; verts[0].tv = m_SkyTex[tex].v0;
            verts[1].p = D3DXVECTOR3(-W, H, L);      verts[1].tu = m_SkyTex[tex].u0; verts[1].tv = m_SkyTex[tex].v1;
            verts[2].p = D3DXVECTOR3(-W, -H, -L);    verts[2].tu = m_SkyTex[tex].u1; verts[2].tv = m_SkyTex[tex].v0;
            verts[3].p = D3DXVECTOR3(-W, H, -L);     verts[3].tu = m_SkyTex[tex].u1; verts[3].tv = m_SkyTex[tex].v1;
            CInstDraw::AddVerts(verts, m_SkyTex[tex].tex);

            tex = SKY_RIGHT;
            verts[0].p = D3DXVECTOR3(W, -H, -L);     verts[0].tu = m_SkyTex[tex].u0; verts[0].tv = m_SkyTex[tex].v0;
            verts[1].p = D3DXVECTOR3(W, H, -L);      verts[1].tu = m_SkyTex[tex].u0; verts[1].tv = m_SkyTex[tex].v1;
            verts[2].p = D3DXVECTOR3(W, -H, L);      verts[2].tu = m_SkyTex[tex].u1; verts[2].tv = m_SkyTex[tex].v0;
            verts[3].p = D3DXVECTOR3(W, H, L);       verts[3].tu = m_SkyTex[tex].u1; verts[3].tv = m_SkyTex[tex].v1;
            CInstDraw::AddVerts(verts, m_SkyTex[tex].tex);
        }
        */

        CInstDraw::ActualDraw();

        g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

        float SH1 = float(g_ScreenY * 0.270416666666667);
        float SH2 = float(g_ScreenY * 0.07);

        g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

        dword m_SkyColorUp = m_SkyColor & 0x00FFFFFF;
        SVert_V4_C v[6];
        v[0].p = D3DXVECTOR4(0, m_SkyHeight - SH1, 0.0f, 1.0f); v[0].col = m_SkyColorUp;
        v[1].p = D3DXVECTOR4(float(g_ScreenX), m_SkyHeight - SH1, 0.0f, 1.0f); v[1].col = m_SkyColorUp;
        v[2].p = D3DXVECTOR4(0, m_SkyHeight - SH2, 0.0f, 1.0f); v[2].col = m_SkyColor;
        v[3].p = D3DXVECTOR4(float(g_ScreenX), m_SkyHeight - SH2, 0.0f, 1.0f); v[3].col = m_SkyColor;
        v[4].p = D3DXVECTOR4(0, m_SkyHeight, 0.0f, 1.0f); v[4].col = m_SkyColor;
        v[5].p = D3DXVECTOR4(float(g_ScreenX), m_SkyHeight, 0.0f, 1.0f); v[5].col = m_SkyColor;

        SetAlphaOpSelect(0, D3DTA_DIFFUSE);
        
        SetColorOpSelect(0, D3DTA_DIFFUSE);
        SetColorOpDisable(1);
            
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE, FALSE));

        CInstDraw::BeginDraw(IDFVF_V4_C);
        CInstDraw::AddVerts(v, nullptr);
        CInstDraw::AddVerts(v + 2, nullptr);
        CInstDraw::ActualDraw();
    }
    else //���� �������� ��������, ���� �������� �� ���� ��������� ���������
    {
        // do not draw skybox
        float SH1 = float(g_ScreenY * 0.270416666666667);
        float SH2 = float(g_ScreenY * 0.07);

        dword m_SkyColorUp = 0x00000000;
        SVert_V4_C v[6];
        v[0].p = D3DXVECTOR4(0, m_SkyHeight-SH1, 0.0f, 1.0f); v[0].col = m_SkyColorUp;
        v[1].p = D3DXVECTOR4(float(g_ScreenX), m_SkyHeight-SH1, 0.0f, 1.0f); v[1].col = m_SkyColorUp;

        v[2].p = D3DXVECTOR4(0, m_SkyHeight-SH2, 0.0f, 1.0f); v[2].col = m_SkyColor;
        v[3].p = D3DXVECTOR4(float(g_ScreenX), m_SkyHeight-SH2, 0.0f, 1.0f); v[3].col = m_SkyColor;

        v[4].p = D3DXVECTOR4(0, m_SkyHeight, 0.0f, 1.0f); v[4].col = m_SkyColor;
        v[5].p = D3DXVECTOR4(float(g_ScreenX), m_SkyHeight, 0.0f, 1.0f); v[5].col = m_SkyColor;

        if(v[0].p.y > 0)
        {
            v[0].p.y = 0;
            v[1].p.y = 0;
        }

        SetAlphaOpDisable(0);
        
        SetColorOpSelect(0, D3DTA_DIFFUSE);
        SetColorOpDisable(1);

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE,	FALSE));

        CInstDraw::BeginDraw(IDFVF_V4_C);
        CInstDraw::AddVerts(v, nullptr);
        CInstDraw::AddVerts(v+2, nullptr);
        CInstDraw::ActualDraw();

        //D3DRECT r;
        //r.x1 = 0;
        //r.y1 = Float2Int(g_MatrixMap->m_SkyHeight);
        //r.x2 = g_ScreenX;
        //r.y2 = r.y1 + 20;
        //ASSERT_DX(g_D3DD->Clear( 1, &r, D3DCLEAR_TARGET, g_MatrixMap->m_SkyColor, 1.0f, 0 ));
    }

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE,	TRUE));
    g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

}

void CMatrixMap::Draw()
{
    float fBias = -1.0f;

    if(FLAG(g_MatrixMap->m_Flags, MMFLAG_SKY_ON))
    {
        DrawSky();
    }

    ASSERT_DX(g_D3DD->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP));
    ASSERT_DX(g_D3DD->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP));

    // enable fogging

    if(FLAG(g_MatrixMap->m_Flags, MMFLAG_FOG_ON))
    {

        float Start   = float(g_MaxViewDistance * g_ThinFogDrawDistance);    // For linear mode
        float End     = float(g_MaxViewDistance * g_DenseFogDrawDistance);
        if(g_D3DDCaps.RasterCaps & D3DPRASTERCAPS_WFOG)
        {}
        else
        {
            Start /= g_MaxViewDistance;
            End /= g_MaxViewDistance;
        }
        // Enable fog blending.
        g_D3DD->SetRenderState(D3DRS_FOGENABLE, TRUE);
     
        // Set the fog color.
        g_D3DD->SetRenderState(D3DRS_FOGCOLOR, m_SkyColor);
        
        g_D3DD->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_NONE );
        g_D3DD->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
        g_D3DD->SetRenderState(D3DRS_FOGSTART, *(dword *)(&Start));
        g_D3DD->SetRenderState(D3DRS_FOGEND, *(dword *)(&End));
    }
    else
    {
        g_D3DD->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }

    // one per frame
    ASSERT_DX(g_D3DD->SetTransform(D3DTS_VIEW, &m_Camera.GetViewMatrix()));
    ASSERT_DX(g_D3DD->SetTransform(D3DTS_PROJECTION, &m_Camera.GetProjMatrix()));

    //DrawObjects();
    //DrawLandscape();



    RESETFLAG(m_Flags, MMFLAG_OBJECTS_DRAWN);
    if(FLAG(g_Flags, GFLAG_STENCILAVAILABLE) && g_Config.m_IzvratMS)
    {
        bool domask = CMultiSelection::DrawAllPass1Begin();
        DrawObjects();
        DrawLandscape();
        if(CTerSurface::IsSurfacesPresent()) DrawLandscapeSurfaces();
        if(domask)
        {
            CMultiSelection::DrawAllPass2Begin();
            SETFLAG(m_Flags, MMFLAG_OBJECTS_DRAWN);
            DrawObjects();

            //g_D3DD->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            //CMatrixMapGroup::BeforeDrawAll();
            //DrawLandscape();
            CMultiSelection::DrawAllPassEnd();
        }
    }
    else
    {
        DrawObjects();
        DrawLandscape();
        if (CTerSurface::IsSurfacesPresent()) DrawLandscapeSurfaces();
    }

    for(int i = 0; i < 4; ++i)
    {
        ASSERT_DX(g_D3DD->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD) (&(fBias)))));
    }

    CMatrixEffectLandscapeSpot::DrawAll(); // we should draw landscape spots immediately after draw landscape

    DrawWater();

    fBias = 0.0f;
    for(int i = 0; i < 4; ++i)
    {
        ASSERT_DX(g_D3DD->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD) (&(fBias)))));
    }
#ifndef RENDER_PROJ_SHADOWS_IN_STENCIL_PASS
    if(g_Config.m_ShowProjShadows) DrawShadowsProjFast();
#endif
    if(g_Config.m_ShowStencilShadows) DrawShadows();

    DrawEffects();

    for(int od = 0; od < m_AD_Obj_cnt; ++od)
    {
        if(m_AD_Obj[od]->GetObjectType() == OBJECT_TYPE_FLYER)
        {
            ((CMatrixFlyer *)m_AD_Obj[od])->DrawPropeller();
        }
    }

    //CDText::T("COL", CStr((int)GetColor(m_TraceStopPos.x, m_TraceStopPos.y)));


/*
    CHelper::Create(1,0)->Line(D3DXVECTOR3(m_TraceStopPos.x, m_TraceStopPos.y, -100),
                            D3DXVECTOR3(m_TraceStopPos.x, m_TraceStopPos.y, 100));

    D3DXVECTOR2 dir;
    if (CalcVectorToLandscape(D3DXVECTOR2(m_TraceStopPos.x, m_TraceStopPos.y), dir))

    {

        CHelper::Create(1,0)->Line(D3DXVECTOR3(m_TraceStopPos.x, m_TraceStopPos.y, 10),
                                D3DXVECTOR3(m_TraceStopPos.x, m_TraceStopPos.y, 10) + 100 * D3DXVECTOR3(dir.x, dir.y, 0));
    }
    */


    //m_Minimap.Draw();

    CMatrixProgressBar::DrawAll();
    CMultiSelection::DrawAll();
    
//#ifdef _DEBUG
//
//    {
//        static CTextureManaged *test = nullptr;
//        
//        if (m_KeyDown && m_KeyScan == KEY_R)
//        {
//            if (IS_TRACE_STOP_OBJECT(m_TraceStopObj))
//            {
//                
//                if (test)
//                {
//                    test->FreeReminder();
//                    g_Cache->Destroy(test);
//                }
//
//                test = m_TraceStopObj->RenderToTexture(TEXSIZE_128);
//            }
//        }
//
//#define SME  256
//#define SZ  128
//
//        if (test)
//        {
//            struct { D3DXVECTOR4 v; float tu, tv; } v[4] = 
//            {
//                { D3DXVECTOR4(0+SME,  SZ+SME, 0.0f, 1.0f), 0,1},
//                { D3DXVECTOR4(0.0f+SME, 0.0f+SME, 0.0f, 1.0f), 0,0},
//                { D3DXVECTOR4(SZ+SME,  SZ+SME, 0.0f, 1.0f), 1,1},
//                { D3DXVECTOR4(SZ+SME, 0+SME, 0.0f, 1.0f), 1,0}
//            };
//
//            g_D3DD->SetTexture(0,test->Tex());
//
//            SetAlphaOpSelect(0, D3DTA_TEXTURE);
//            SetColorOpSelect(0, D3DTA_TEXTURE);
//            SetColorOpDisable(1);
//            
//            ASSERT_DX(g_D3DD->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1));
//            ASSERT_DX(g_D3DD->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &v, sizeof(D3DXVECTOR4)+8 ));
//
//        }
//
//    }
//
//#endif


    // shader
    //if (m_ShadeOn)
    //{
    //    float k = (1.0f-(float(m_ShadeTime)*INVERT(SHADER_TIME)));
    //    float y = k * float(g_ScreenY) * (SHADER_PERC / 100.0f);

    //    D3DXVECTOR4 v[8] = 
    //    {
    //        D3DXVECTOR4(0.0f, y, 0.0f, 1.0f),
    //        D3DXVECTOR4(0,  0, 0.0f, 1.0f),
    //        D3DXVECTOR4(float(g_ScreenX), y, 0.0f, 1.0f),
    //        D3DXVECTOR4(float(g_ScreenX),  0, 0.0f, 1.0f),

    //        D3DXVECTOR4(0.0f, float(g_ScreenY), 0.0f, 1.0f),
    //        D3DXVECTOR4(0,  float(g_ScreenY)-y, 0.0f, 1.0f),
    //        D3DXVECTOR4(float(g_ScreenX), float(g_ScreenY), 0.0f, 1.0f),
    //        D3DXVECTOR4(float(g_ScreenX),  float(g_ScreenY)-y, 0.0f, 1.0f)

    //    };
    //    

	   // ASSERT_DX(g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR,		0x0));

    //    SetAlphaOpDisable(0);
    //    SetColorOpSelect(0, D3DTA_TFACTOR);
    //    SetColorOpDisable(1);
    //    
    //    ASSERT_DX(g_D3DD->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE));
    //    ASSERT_DX(g_D3DD->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &v, sizeof(D3DXVECTOR4) ));
    //    ASSERT_DX(g_D3DD->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &v[4], sizeof(D3DXVECTOR4) ));


    //    if (m_ShadeTimeCurrent > 3000)
    //    {
    //        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE,		TRUE));
    //        float k = (float(m_ShadeTimeCurrent-3000) * INVERT(2000.0f));
    //        if (k < 0) k = 0;
    //        if (k > 1) k = 1;
    //        byte a = byte(k*255.0);
    //        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR, (a << 24)));

    //        D3DXVECTOR4 v[4] = 
    //        {
    //            D3DXVECTOR4(0,  float(g_ScreenY)-y, 0.0f, 1.0f),
    //            D3DXVECTOR4(0.0f, y, 0.0f, 1.0f),
    //            D3DXVECTOR4(float(g_ScreenX),  float(g_ScreenY)-y, 0.0f, 1.0f),
    //            D3DXVECTOR4(float(g_ScreenX), y, 0.0f, 1.0f),
    //        };

    //        SetAlphaOpSelect(0, D3DTA_TFACTOR);

    //        ASSERT_DX(g_D3DD->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &v, sizeof(D3DXVECTOR4) ));
    //        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE,		FALSE));

    //    }

    //    m_Camera.SetTarget(m_ShadePosFrom + (m_ShadePosTo-m_ShadePosFrom) * k);

    //}
    
    if(IsPaused())
    {
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE,	TRUE ));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE,				D3DZB_FALSE ));
	    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,		FALSE));
	    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA));
	    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA));

	    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR,		0x14000000/* 0xffffffff*/));

        SetColorOpSelect(0, D3DTA_TFACTOR);
        SetAlphaOpSelect(0, D3DTA_TFACTOR);
        SetColorOpDisable(1);

	    ASSERT_DX(g_D3DD->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE));
        ASSERT_DX(g_D3DD->SetStreamSource( 0, GET_VB(m_ShadowVB), 0, sizeof(SShadowRectVertex) ));
        ASSERT_DX(g_D3DD->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 ));

	    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZENABLE,				D3DZB_TRUE));
	    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE,		TRUE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE,	FALSE ));
    }

#if (defined _DEBUG) &&  !(defined _RELDEBUG)
    CHelper::DrawAll();
#endif


    m_DI.Draw();        // debug info
    

    if(m_DialogModeName)
    {
        if(wcscmp(m_DialogModeName, TEMPLATE_DIALOG_MENU) == 0)
        {
            m_DialogModeHints.Buff<CMatrixHint *>()[0]->DrawNow();
        }
    }

    g_IFaceList->Render();
    CMatrixProgressBar::DrawAllClones();
    CMatrixHint::DrawAll();

    //_________________________________________________________________________________________________

    if(FLAG(m_Flags, MMFLAG_TRANSITION)) m_Transition.Render();

    //_________________________________________________________________________________________________

    m_Cursor.Draw();
}

void CMatrixMap::Tact(int step)
{
    float fstep = float(step);
    m_SkyAngle += m_SkyDeltaAngle* step;
    int i;

    CMatrixMapStatic::SortEndGraphicTact(step);

    int cnt = m_VisibleGroupsCount;
    CMatrixMapGroup** md = m_VisibleGroups;
    while((cnt--) > 0)
    {
        (*(md++))->GraphicTact(step);
    }

    CSkinManager::Tact(fstep);
    m_Water->Tact(step);
    m_Cursor.Tact(step);
    m_DI.Tact(step);

    for(i = 0; i < m_EffectSpawnersCnt; ++i) m_EffectSpawners[i].Tact(fstep);

    RemoveEffectSpawnerByTime();

    //SETFLAG(m_Flags, MMFLAG_EFF_TAKT);
    CMatrixEffect* e = m_EffectsFirst;
    while(e != nullptr)
    {
#ifdef DEAD_PTR_SPY_ENABLE
        CMatrixEffect* deade = (CMatrixEffect*)DeadPtr::get_dead_mem(e);
        if(deade)
        {
            _asm int 3
        }
#endif
#ifdef DEAD_CLASS_SPY_ENABLE
        CMatrixEffectLandscapeSpot* spot = (CMatrixEffectLandscapeSpot*)e->DCS_GetDeadBody();
        if(spot)
        {
            _asm int 3
        }
#endif

        m_EffectsNextTact = e->m_Next;
        e->Tact(fstep);
        e = m_EffectsNextTact;
    }

    //RESETFLAG(m_Flags,MMFLAG_EFF_TAKT);

    //// some effects must be removed
    //while(m_Effects2RemoveFirst)
    //{
    //    CMatrixEffect* e = m_Effects2RemoveFirst;
    //   
    //    // delete from base effect list
    //    LIST_DEL(e, m_EffectsFirst, m_EffectsLast, m_Prev, m_Next);

    //    //remove from death list
    //    LIST_DEL(e, m_Effects2RemoveFirst, m_Effects2RemoveLast, m_PrevDel, m_NextDel);

    //    e->Release();
    //}

    m_Minimap.Tact(fstep);
    CSound::Tact();

    //{
    //    typedef CSprite* CSprite*;
    //    static bool inited = false;
    //    static CSprite* ar[8192];
    //    static int bn = 3024;
    //    if(!inited)
    //    {
    //        D3DXVECTOR3 pos(300, 100, 30);
    //        float a = 0;
    //        float r = 30;
    //        for(int i = 0; i < bn; ++i)
    //        {
    //            float sn, cs;
    //            SinCos(a, &sn, &cs);

    //            ar[i] = HNew(g_CacheHeap) CSprite(TRACE_PARAM_CALL pos + D3DXVECTOR3(sn * r, cs * r, 0), 3, a, 0xFF400401, false, CMatrixEffect::GetSpriteTex(SPR_SMOKE_PART));

    //            a += GRAD2RAD(1);
    //            r += 0.05f;
    //            pos.z += 0.05f;
    //        }

    //        inited = true;
    //    }
    //
    //    for(int i = 0; i < bn; ++i) ar[i]->Sort(m_Camera.GetViewMatrix());
    //}
}

bool CMatrixMap::FindObjects(
    const D3DXVECTOR2& pos,
    float radius,
    float oscale,
    dword mask,
    CMatrixMapStatic* skip,
    ENUM_OBJECTS2D callback,
    dword user //������ ��� ����� �� ����� ���
)
{
    bool hit = false;

    D3DXVECTOR2 vmin(pos.x - radius, pos.y - radius), vmax(pos.x + radius, pos.y + radius);

	int minx1, miny1, maxx1, maxy1;

    const float groupsize = GLOBAL_SCALE * MAP_GROUP_SIZE;
    const float obr_groupsize = 1.0f / groupsize;

    minx1 = TruncFloat(vmin.x * obr_groupsize);
    miny1 = TruncFloat(vmin.y * obr_groupsize);

    maxx1 = TruncFloat(vmax.x * obr_groupsize);
    maxy1 = TruncFloat(vmax.y * obr_groupsize);

    if(maxx1 < minx1)
    {
        maxx1 ^= minx1;
        minx1 ^= maxx1;
        maxx1 ^= minx1;
    }

    if(maxy1 < miny1)
    {
        maxy1 ^= miny1;
        miny1 ^= maxy1;
        maxy1 ^= miny1;
    }

    ++maxx1;
    ++maxy1;

    ++m_IntersectFlagFindObjects;

    if(minx1 < 0)
    {
        minx1 = 0;
        if(0 > maxx1) goto skip;
    }

    if(maxx1 >= m_GroupSize.x)
    {
        maxx1 = m_GroupSize.x - 1;
        if(maxx1 < minx1) goto skip;
    }

    if(miny1 < 0)
    {
        miny1 = 0;
        if(0 > maxy1) goto skip;
    }

    if(maxy1 >= m_GroupSize.y)
    {
        maxy1 = m_GroupSize.y - 1;
        if(maxy1 < miny1) goto skip;
    }

    for(int x = minx1; x <= maxx1; ++x)
    {
        for(int y = miny1; y <= maxy1; ++y)
        {
            CMatrixMapGroup* g = GetGroupByIndex(x, y);
            if(g == nullptr) continue;
            int i = 0;
            CMatrixMapStatic* ms;
            CMatrixMapStatic* ms2 = nullptr;
            while(true)
            {
                if(ms2 == nullptr) ms = g->FindObjectAny(mask, pos, radius, oscale, i);
                else
                {
                    ms = ms2;
                    ms2 = nullptr;
                }
                if(ms == nullptr) break;

                if(ms->m_IntersectFlagFindObjects == m_IntersectFlagFindObjects) continue;
                ms->m_IntersectFlagFindObjects = m_IntersectFlagFindObjects;

                if(ms == skip) continue;
                if(ms->IsFlyer())
                {
                    ms2 = ((CMatrixFlyer*)ms)->GetCarryingRobot();
                    if(ms2 != nullptr)
                    {
                        D3DXVECTOR2 temp = *(D3DXVECTOR2*)&ms2->GetGeoCenter() - pos;
                        float dist = D3DXVec2Length(&temp) - ms2->GetRadius() * oscale;
                        if(dist >= radius) ms2 = nullptr;
                    }
                }
                
                hit = true;
                if(callback)
                {
                    if(!callback(pos, ms, user)) return hit;
                }
                else return true;
            }
        }
    }

skip:;
    for(int od = 0; od < m_AD_Obj_cnt; ++od)
    {
        if(m_AD_Obj[od]->m_IntersectFlagFindObjects != m_IntersectFlagFindObjects)
        {
            m_AD_Obj[od]->m_IntersectFlagFindObjects = m_IntersectFlagFindObjects;

            CMatrixMapStatic* msa[2];
            int mscnt = 1;
            msa[0] = m_AD_Obj[od];

            while(true)
            {
                if(!mscnt) break;
                CMatrixMapStatic *ms = msa[0];
                if(mscnt == 2) msa[0] = msa[1];
                --mscnt;

                if(ms->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
                {
                    if(((CMatrixRobot *)ms)->m_CurrentState == ROBOT_DIP) continue;
                    if(!(mask & TRACE_ROBOT)) continue;
                }
                else if(ms->GetObjectType() == OBJECT_TYPE_FLYER)
                {
                    D3DXVECTOR2 temp;

                    msa[mscnt] = ((CMatrixFlyer*)ms)->GetCarryingRobot();
                    if(msa[mscnt] != nullptr)
                    {
                        temp = *(D3DXVECTOR2*)&msa[mscnt]->GetGeoCenter() - pos;
                        float dist = D3DXVec2Length(&temp) - msa[mscnt]->GetRadius() * oscale;
                        if(dist < radius) ++mscnt;
                    }
                    if(!(mask & TRACE_FLYER)) continue;

                    temp = *(D3DXVECTOR2*)&ms->GetGeoCenter() - pos;
                    float dist = D3DXVec2Length(&temp) - ms->GetRadius() * oscale;

                    if(dist >= radius) continue;
                }

                if(ms == skip) continue;

                hit = true;
                if(callback)
                {
                    if(!callback(pos, ms, user)) return hit;
                }
                else return true;
            }
        }
    }

    return hit;
}

bool CMatrixMap::FindObjects(
    const D3DXVECTOR3& pos,
    float radius,
    float oscale,
    dword mask,
    CMatrixMapStatic* skip,
    ENUM_OBJECTS callback,
    dword user //������ ��� ����� �� ����� ���
)
{
    bool hit = false;

    D3DXVECTOR2 vmin(pos.x - radius, pos.y - radius), vmax(pos.x + radius, pos.y + radius);

	int minx1, miny1, maxx1, maxy1;

    const float groupsize = GLOBAL_SCALE * MAP_GROUP_SIZE;
    const float obr_groupsize = 1.0f / groupsize;

	minx1 = TruncFloat(vmin.x * obr_groupsize);
	miny1 = TruncFloat(vmin.y * obr_groupsize);

	maxx1 = TruncFloat(vmax.x * obr_groupsize);
	maxy1 = TruncFloat(vmax.y * obr_groupsize);

    if(maxx1 < minx1)
    {
        maxx1 ^= minx1;
        minx1 ^= maxx1;
        maxx1 ^= minx1;
    }

    if(maxy1 < miny1)
    {
        maxy1 ^= miny1;
        miny1 ^= maxy1;
        maxy1 ^= miny1;
    }

    ++maxx1;
    ++maxy1;

    ++m_IntersectFlagFindObjects;

    if(minx1 < 0)
    {
        minx1 = 0;
        if(0 > maxx1) goto skip_search;
    }

    if(maxx1 >= m_GroupSize.x)
    {
        maxx1 = m_GroupSize.x - 1;
        if(maxx1 < minx1) goto skip_search;
    }

    if(miny1 < 0)
    {
        miny1 = 0;
        if(0 > maxy1) goto skip_search;
    }

    if(maxy1 >= m_GroupSize.y)
    {
        maxy1 = m_GroupSize.y - 1;
        if(maxy1 < miny1) goto skip_search;
    }

    for(int x = minx1; x <= maxx1; ++x)
    {
        for(int y = miny1; y <= maxy1; ++y)
        {
            CMatrixMapGroup* g = GetGroupByIndex(x, y);
            if(g == nullptr) continue;
            int i = 0;
            CMatrixMapStatic* ms;
            CMatrixMapStatic* ms2 = nullptr;
            while(true)
            {
                if(ms2 == nullptr) ms = g->FindObjectAny(mask, pos, radius, oscale, i);
                else
                {
                    ms = ms2;
                    ms2 = nullptr;
                }
                if(ms == nullptr) break;

                if(ms->m_IntersectFlagFindObjects == m_IntersectFlagFindObjects) continue;
                ms->m_IntersectFlagFindObjects = m_IntersectFlagFindObjects;

                if(ms == skip) continue;
                if(ms->GetObjectType() == OBJECT_TYPE_FLYER)
                {
                    ms2 = ms->AsFlyer()->GetCarryingRobot();
                    if(ms2 != nullptr)
                    {
                        D3DXVECTOR3 temp = ms2->GetGeoCenter() - pos;
                        float dist = D3DXVec3Length(&temp) - ms2->GetRadius() * oscale;
                        if(dist >= radius) ms2 = nullptr;
                    }
                }
                
                hit = true;
                if(callback)
                {
                    if(!callback(pos, ms, user)) return hit;
                }
                else return true;
            }
        }
    }

skip_search:;
    for(int od = 0; od < m_AD_Obj_cnt; ++od)
    {
        if(m_AD_Obj[od]->m_IntersectFlagFindObjects != m_IntersectFlagFindObjects)
        {
            m_AD_Obj[od]->m_IntersectFlagFindObjects = m_IntersectFlagFindObjects;

            CMatrixMapStatic* msa[2];
            int ms_cnt = 1;
            msa[0] = m_AD_Obj[od];

            while(true)
            {
                if(!ms_cnt) break;
                CMatrixMapStatic* ms = msa[0];
                if(ms_cnt == 2) msa[0] = msa[1];
                --ms_cnt;

                if(ms->IsRobot())
                {
                    if(ms->AsRobot()->m_CurrentState == ROBOT_DIP) continue;
                    if(!(mask & TRACE_ROBOT)) continue;
                }
                else if(ms->IsFlyer())
                {
                    msa[ms_cnt] = ms->AsFlyer()->GetCarryingRobot();
                    if(msa[ms_cnt] != nullptr)
                    {
                        D3DXVECTOR3 temp = msa[ms_cnt]->GetGeoCenter() - pos;
                        float dist = D3DXVec3Length(&temp) - msa[ms_cnt]->GetRadius() * oscale;
                        if(dist < radius) ++ms_cnt;
                    }

                    if(!(mask & TRACE_FLYER)) continue;

                    D3DXVECTOR3 temp = ms->GetGeoCenter() - pos;
                    float dist = D3DXVec3Length(&temp) - ms->GetRadius() * oscale;
                    if(dist >= radius) continue;
                }

                if(ms == skip) continue;

                hit = true;
                if(callback)
                {
                    if(!callback(pos, ms, user)) return hit;
                }
                else return true;
            }
        }
    }

    return hit;
}

void CMatrixMap::SubEffect(CMatrixEffect* e)
{
    if(e->IsDIP()) return; // already under destruction

    if(e->m_Next == nullptr && e->m_Prev == nullptr && m_EffectsFirst != e)
    {
        // effect not in list
        e->Release();
    }
    else
    {
        /*
        if(FLAG(m_Flags, MMFLAG_EFF_TAKT))
        {
              if(!(e->m_PrevDel == nullptr && e->m_NextDel == nullptr && e != m_Effects2RemoveFirst)) return; // already dead
              // effect tact in progress. just put effect to dead list
              LIST_ADD(e, m_Effects2RemoveFirst, m_Effects2RemoveLast, m_PrevDel, m_NextDel);
              e->Unconnect();
        }
        else
        */
        {
            if(m_EffectsNextTact == e)
            {
                m_EffectsNextTact = e->m_Next;
            }

            LIST_DEL(e, m_EffectsFirst, m_EffectsLast, m_Prev, m_Next);
            e->Release();
        }

        --m_EffectsCnt;
    }
}

/*
bool CMatrixMap::IsEffect(CMatrixEffect* e)
{
    //for(int i = 0; i < m_EffectsCnt; ++i)
    //{
    //    if (m_Effects[i] == e)
    //    {
    //        return true;
    //    }
    //}
    //return false;
    return m_Effects.Get(PTR2KEY(e), nullptr);
}
*/

void CMatrixMap::RemoveEffectSpawnerByObject(CMatrixMapStatic* ms)
{
    int i = 0;
    while(i < m_EffectSpawnersCnt)
    {
        if(m_EffectSpawners[i].GetUnder() == ms)
        {
            m_EffectSpawners[i].~CEffectSpawner();
            --m_EffectSpawnersCnt;
            if(i == m_EffectSpawnersCnt) return;

            m_EffectSpawners[i] = m_EffectSpawners[m_EffectSpawnersCnt];
            continue;
        }
        ++i;
    }
}

void CMatrixMap::RemoveEffectSpawnerByTime()
{
    int i = 0;
    while(i < m_EffectSpawnersCnt)
    {
        if(m_EffectSpawners[i].OutOfTime()) break; 
        ++i;
    }
    if(i >= m_EffectSpawnersCnt) return;

    m_EffectSpawners[i].~CEffectSpawner();
    --m_EffectSpawnersCnt;
    if(i == m_EffectSpawnersCnt) return;

    m_EffectSpawners[i] = m_EffectSpawners[m_EffectSpawnersCnt];
}

void CMatrixMap::AddEffectSpawner(float x, float y, float z, int ttl, const CWStr& par)
{
    SpawnEffectSmoke smoke;
    SpawnEffectFire fire;
    SpawnEffectSound sound;
    SpawnEffectLightening lightening;

    SpawnEffectProps* props = &smoke;

    int parcnt = par.GetCountPar(L",");
    if(parcnt < 3) return;
    parcnt -= 3;

    int minper = par.GetIntPar(1, L",");
    int maxper = par.GetIntPar(2, L",");

    int idx = 3;

    m_EffectSpawners = (CEffectSpawner*)HAllocEx(m_EffectSpawners, sizeof(CEffectSpawner) * (m_EffectSpawnersCnt + 1), Base::g_MatrixHeap);

    EEffectSpawnerType type = (EEffectSpawnerType)par.GetIntPar(0, L",");

    bool light_need_action = false;

    switch(type)
    {
        case EST_SMOKE:
        {
            smoke.m_Type = EFFECT_SMOKE;

            smoke.m_ttl = par.GetFloatPar(idx++, L",");
            smoke.m_puffttl = par.GetFloatPar(idx++, L",");
            smoke.m_spawntime = par.GetFloatPar(idx++, L",");
            smoke.m_IsBright = par.GetTrueFalsePar(idx++, L",");
            smoke.m_speed = par.GetFloatPar(idx++, L",");
            smoke.m_color = par.GetStrPar(idx++, L",").GetHexUnsigned();

            break;
        }
        case EST_FIRE:
        {
            props = &fire;

            fire.m_Type = EFFECT_FIRE;

            fire.m_ttl = par.GetFloatPar(idx++, L",");
            fire.m_puffttl = par.GetFloatPar(idx++, L",");
            fire.m_spawntime = par.GetFloatPar(idx++, L",");
            fire.m_IsBright = par.GetTrueFalsePar(idx++, L",");
            fire.m_speed = par.GetFloatPar(idx++, L",");
            fire.m_dispfactor = par.GetFloatPar(idx++, L",");

            break;
        }
        case EST_SOUND:
        {
            props = &sound;

            sound.m_Type = EFFECT_UNDEFINED;

            sound.m_vol0 = par.GetFloatPar(idx++, L",");
            sound.m_vol1 = par.GetFloatPar(idx++, L",");
            sound.m_pan0 = par.GetFloatPar(idx++, L",");
            sound.m_pan1 = par.GetFloatPar(idx++, L",");
            sound.m_attn = par.GetFloatPar(idx++, L",");
            //sound.m_looped = par.GetTrueFalsePar(idx++, L",");

            CWStr nam(par.GetStrPar(idx++, L","), g_CacheHeap);
            if(nam.GetLen() > (sizeof(sound.m_name) / sizeof(sound.m_name[0])))
            {
                ERROR_S(L"Effect spawner: sound name too long!");
            }

            memcpy(sound.m_name, nam.Get(), (nam.GetLen() + 1) * sizeof(wchar));

            break;
        }
        case EST_LIGHTENING:
        {
            props = &lightening;
            lightening.m_Type = EFFECT_LIGHTENING;

            lightening.m_Tag = HNew(Base::g_MatrixHeap) CWStr(Base::g_MatrixHeap);

            *lightening.m_Tag = par.GetStrPar(idx++, L",");

            lightening.m_ttl = par.GetFloatPar(idx++, L",");
            lightening.m_Color = par.GetStrPar(idx++, L",").GetHexUnsigned();
            lightening.m_Width = -par.GetFloatPar(idx++, L","); // set negative width. it means that m_Tag used instead of m_Pair;
            lightening.m_Dispers = par.GetFloatPar(idx++, L",");

            // seek tag
            int bla = m_EffectSpawnersCnt - 1;
            while(bla >= 0)
            {
                if(m_EffectSpawners[bla].Props()->m_Type == EFFECT_LIGHTENING)
                {
                    SpawnEffectLightening* l = (SpawnEffectLightening*)m_EffectSpawners[bla].Props();
                    if(l->m_Width < 0)
                    {
                        if(*l->m_Tag == *lightening.m_Tag)
                        {
                            NEG_FLOAT(l->m_Width);
                            NEG_FLOAT(lightening.m_Width);

                            HDelete(CWStr, l->m_Tag, Base::g_MatrixHeap);
                            HDelete(CWStr, lightening.m_Tag, Base::g_MatrixHeap);
                            lightening.m_Pair = l;

                            lightening.m_Dispers = -1; // me - non host

                            light_need_action = true;
                        }

                    }
                }
                --bla;
            }

            break;
        }

        default: ERROR_S(L"CMatrixMap::AddEffectSpawner Error!");
    }

    props->m_Pos.x = x;
    props->m_Pos.y = y;
    props->m_Pos.z = z;

    //��������� ������� �������� (��������� ������ �� ������� ��������� ���������� ����)
    new(&m_EffectSpawners[m_EffectSpawnersCnt]) CEffectSpawner(minper, maxper, ttl, props);
    //m_EffectSpawners[m_EffectSpawnersCnt].CEffectSpawner::CEffectSpawner(minper, maxper, ttl, props);

    if(light_need_action)
    {
        SpawnEffectLightening* l = (SpawnEffectLightening*)m_EffectSpawners[m_EffectSpawnersCnt].Props();
        l->m_Pair->m_Pair = l;
    }

    ++m_EffectSpawnersCnt;
}

//������� ����� �� ����������� ���� ��� ���������� �������� (��������, ���� �� ������������� �������)
void CMatrixMap::OkHandler()
{
    g_MatrixMap->LeaveDialogMode();
}
//����� �� ����������� ���� � �������� ���������� ��� (��� ���������, ���� ������)
void CMatrixMap::OkExitHandler()
{
    g_MatrixMap->LeaveDialogMode();
    g_ExitState = ES_EXIT_TO_MAIN_MENU;

    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_STAT_DIALOG);
    //SETFLAG(g_Flags, GFLAG_EXITLOOP);
}
//����� �� ��������� ����������� � ����� ���� �� ����������� ���
void CMatrixMap::OkExitWinHandler()
{
    g_MatrixMap->LeaveDialogMode();
    g_ExitState = ES_EXIT_AFTER_WIN;

    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_STAT_DIALOG);
    //SETFLAG(g_Flags, GFLAG_EXITLOOP);
}
//����� �� �������������� ����������� � ����� ���� �� ����������� ���
void CMatrixMap::OkExitLoseHandler()
{
    g_MatrixMap->LeaveDialogMode();
    g_ExitState = ES_EXIT_AFTER_LOSE;

    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_STAT_DIALOG);
    //SETFLAG(g_Flags, GFLAG_EXITLOOP);
}
//����� �� ����������� ���� � ���������� ����������
void CMatrixMap::OkJustExitHandler()
{
    g_MatrixMap->LeaveDialogMode();
    SETFLAG(g_Flags, GFLAG_EXITLOOP);
}
void CMatrixMap::OkSurrenderHandler()
{
    g_MatrixMap->LeaveDialogMode();
    g_ExitState = ES_EXIT_AFTER_SURRENDER;
    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_STAT_DIALOG);

    for(int i = 0; i < g_MatrixMap->m_SidesCount; ++i)
    {
        CMatrixSideUnit* su = g_MatrixMap->m_Side + i;
        if(su->GetStatus() == SS_ACTIVE && su != g_MatrixMap->GetPlayerSide())
        {
            su->SetStatValue(STAT_TIME, -su->GetStatValue(STAT_TIME));
        }
    }
    //SETFLAG(g_Flags, GFLAG_EXITLOOP);
}
void CMatrixMap::OkResetHandler()
{
    g_MatrixMap->LeaveDialogMode();
    g_MatrixMap->Restart();
    //g_ExitState = ES_EXIT_AFTER_SURRENDER;
    //SETFLAG(g_Flags, GFLAG_EXITLOOP);
}
void ConfirmCancelHandler()
{
    CMatrixHint* h = (CMatrixHint*)g_MatrixMap->m_DialogModeHints.Buff<dword>()[1];
    g_MatrixMap->m_DialogModeHints.SetLenNoShrink(g_MatrixMap->m_DialogModeHints.Len() - sizeof(dword));
    h->Release();

    g_IFaceList->EnableMainMenuButton(HINT_CANCEL_MENU);
    g_IFaceList->EnableMainMenuButton(HINT_CONTINUE);
    g_IFaceList->EnableMainMenuButton(HINT_SURRENDER);
    g_IFaceList->EnableMainMenuButton(HINT_RESET);
    g_IFaceList->EnableMainMenuButton(HINT_EXIT);

    g_IFaceList->HideHintButton(HINT_OK);
    g_IFaceList->HideHintButton(HINT_CANCEL);
}

static void CreateConfirmation(const wchar* hint, DialogButtonHandler handler)
{
    g_IFaceList->DisableMainMenuButton(HINT_CANCEL_MENU);
    g_IFaceList->DisableMainMenuButton(HINT_CONTINUE);
    g_IFaceList->DisableMainMenuButton(HINT_SURRENDER);
    g_IFaceList->DisableMainMenuButton(HINT_RESET);
    g_IFaceList->DisableMainMenuButton(HINT_EXIT);

    g_MatrixMap->m_DialogModeHints.Pointer(g_MatrixMap->m_DialogModeHints.Len());

    CMatrixHint* h = CMatrixHint::Build(CWStr(hint, g_CacheHeap), hint);
    int ww = (g_ScreenX - h->m_Width) / 2;
    int hh = (g_ScreenY - h->m_Height) / 2 - Float2Int(float(g_ScreenY) * 0.09f);
    h->Show(ww, hh);

    if(h->GetCopyPosCnt() > 0)
    {
        int x = h->m_PosX + h->GetCopyPos(0).x;
        int y = h->m_PosY + h->GetCopyPos(0).y;
        g_IFaceList->CreateHintButton(x, y, HINT_OK, handler);
    }

    if(h->GetCopyPosCnt() > 1)
    {
        int x = h->m_PosX + h->GetCopyPos(1).x;
        int y = h->m_PosY + h->GetCopyPos(1).y;
        g_IFaceList->CreateHintButton(x, y, HINT_CANCEL, ConfirmCancelHandler);
    }

    g_MatrixMap->m_DialogModeHints.Dword((dword)h);
}

void ExitRequestHandler()
{
    CreateConfirmation(TEMPLATE_DIALOG_EXIT, g_MatrixMap->OkExitHandler);
}

void ResetRequestHandler()
{
    CreateConfirmation(TEMPLATE_DIALOG_RESET, g_MatrixMap->OkResetHandler);
}

void HelpRequestHandler()
{
    g_MatrixMap->LeaveDialogMode();
    CWStr fn(g_CacheHeap);
    if(CFile::FileExist(fn, L"Manual.exe"))
    {
        PROCESS_INFORMATION pi0;
        STARTUPINFOA        si0;

        ZeroMemory(&si0, sizeof(si0));
        si0.cb = sizeof(si0);
        ZeroMemory(&pi0, sizeof(pi0));

        CreateProcess("Manual.exe", nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si0, &pi0);

        ShowWindow(g_Wnd, SW_MINIMIZE);

        CloseHandle(pi0.hProcess);
        CloseHandle(pi0.hThread);
    }
}

void SurrenderRequestHandler()
{
    CreateConfirmation(TEMPLATE_DIALOG_SURRENDER, g_MatrixMap->OkSurrenderHandler);
}

//��������� ���� � ������������� � ��������/������������� �������, ���� ������������� 
void CMatrixMap::EnterDialogMode(const wchar* hint_i)
{
    if(m_PauseHint)
    {
        m_PauseHint->Release();
        m_PauseHint = nullptr;
    }

    if(m_GameSpeedHint)
    {
        m_GameSpeedHint->Release();
        m_GameSpeedHint = nullptr;
    }

    LeaveDialogMode();
    Pause(true);
    SETFLAG(m_Flags, MMFLAG_DIALOG_MODE);
    m_DialogModeName = hint_i;

    if(wcscmp(hint_i, TEMPLATE_DIALOG_BEGIN))
    {
        g_MatrixMap->GetPlayerSide()->PLDropAllActions();
    }

    m_DialogModeHints.Pointer(m_DialogModeHints.Len());

    CBlockPar* bp = g_MatrixData->BlockGet(PAR_TEMPLATES);

    int cnt = bp->ParCount();

    int ww = 20;
    int hh = 62;

    const wchar* hint = hint_i;
    if(!wcscmp(hint_i, TEMPLATE_DIALOG_STATISTICS_D))
    {
        hint = TEMPLATE_DIALOG_STATISTICS;
    }

    for(int i = 0; i < cnt; ++i)
    {
        if(bp->ParGetName(i) == hint)
        {
            CWStr templ(bp->ParGet(i), g_CacheHeap);
            CWStr templ2(g_CacheHeap);
            if (templ[0] == '|') continue;

            int ii = i + 1;
            for(; ii < cnt; ++ii)
            {
                if(bp->ParGetName(ii) == hint)
                {
                    templ2 = bp->ParGet(ii);
                    if(templ2[0] == '|') templ += templ2;
                }
            }

            CMatrixHint* h = CMatrixHint::Build(templ, g_MatrixData->BlockGet(PAR_REPLACE), hint);

            if(!wcscmp(hint, TEMPLATE_DIALOG_MENU))
            {
                h->m_PosX = (g_ScreenX - h->m_Width) / 2;
                h->m_PosY = (g_ScreenY - h->m_Height) / 2 - Float2Int(float(g_ScreenY) * 0.09f);
                h->SetVisible(false);
                h->SoundIn();
            }
            else if(!wcscmp(hint, TEMPLATE_DIALOG_STATISTICS))
            {
                h->m_PosX = (g_ScreenX - h->m_Width) / 2;
                h->m_PosY = (g_ScreenY - h->m_Height) / 2 - Float2Int(float(g_ScreenY) * 0.09f);
                h->SetVisible(true);
            }
            else h->Show(ww, hh);

            if(h->GetCopyPosCnt() > 0)
            {
                int x = h->m_PosX + h->GetCopyPos(0).x;
                int y = h->m_PosY + h->GetCopyPos(0).y;

                //����� �������� ������� "��" � �������� ����
                if(!wcscmp(hint, TEMPLATE_DIALOG_MENU))
                {
                    g_IFaceList->CreateHintButton(x, y, HINT_CONTINUE, OkHandler);
                }
                else if(!wcscmp(hint, TEMPLATE_DIALOG_WIN))
                {
                    g_IFaceList->CreateHintButton(x, y, HINT_OK, OkExitWinHandler);
                }
                else if(!wcscmp(hint, TEMPLATE_DIALOG_LOSE))
                {
                    g_IFaceList->CreateHintButton(x, y, HINT_OK, OkExitLoseHandler);
                }
                else if(!wcscmp(hint, TEMPLATE_DIALOG_STATISTICS))
                {
                    if(!wcscmp(hint_i, TEMPLATE_DIALOG_STATISTICS_D))
                    {
                        g_IFaceList->CreateHintButton(x, y, HINT_OK, OkHandler);
                    }
                    else
                    {
                        g_IFaceList->CreateHintButton(x, y, HINT_OK, OkJustExitHandler);
                    }
                }
                else
                {
                    g_IFaceList->CreateHintButton(x, y, HINT_OK, OkHandler);
                }
            }

            if(h->GetCopyPosCnt() > 1)
            {
                int x = h->m_PosX + h->GetCopyPos(1).x;
                int y = h->m_PosY + h->GetCopyPos(1).y;
                g_IFaceList->CreateHintButton(x, y, HINT_RESET, ResetRequestHandler);
            }
            if(h->GetCopyPosCnt() > 2)
            {
                int x = h->m_PosX + h->GetCopyPos(2).x;
                int y = h->m_PosY + h->GetCopyPos(2).y;
                g_IFaceList->CreateHintButton(x, y, HINT_HELP, HelpRequestHandler);
            }
            if(h->GetCopyPosCnt() > 3)
            {
                int x = h->m_PosX + h->GetCopyPos(3).x;
                int y = h->m_PosY + h->GetCopyPos(3).y;
                g_IFaceList->CreateHintButton(x, y, HINT_SURRENDER, SurrenderRequestHandler);
            }
            if(h->GetCopyPosCnt() > 4)
            {
                int x = h->m_PosX + h->GetCopyPos(4).x;
                int y = h->m_PosY + h->GetCopyPos(4).y;
                g_IFaceList->CreateHintButton(x, y, HINT_EXIT, ExitRequestHandler);
            }
            if(h->GetCopyPosCnt() > 5)
            {
                int x = h->m_PosX + h->GetCopyPos(5).x;
                int y = h->m_PosY + h->GetCopyPos(5).y;
                g_IFaceList->CreateHintButton(x, y, HINT_CANCEL_MENU, OkHandler);
            }

            ww += h->m_Width + 20;
            m_DialogModeHints.Dword((dword)h);
        }
    }
}

void CMatrixMap::LeaveDialogMode()
{
    if(m_DialogModeName == nullptr) return;

    if(!wcscmp(m_DialogModeName, TEMPLATE_DIALOG_MENU))
    {
        m_DialogModeHints.Buff<CMatrixHint*>()[0]->SoundOut();
    }

    RESETFLAG(m_Flags, MMFLAG_DIALOG_MODE);
    Pause(false);
    dword* a = m_DialogModeHints.Buff<dword>();
    dword* b = m_DialogModeHints.BuffEnd<dword>();
    for(; a < b; ++a)
    {
        ((CMatrixHint*)(*a))->Release();
    }

    m_DialogModeHints.Clear();
    g_IFaceList->HideHintButtons();
    m_DialogModeName = nullptr;
}

void CMatrixMap::RestoreMusicVolume()
{
    if (!g_RangersInterface) return;
    SETFLAG(m_Flags, MMFLAG_MUSIC_VOL_CHANGING);
    m_TargetMusicVolume = m_StoreCurrentMusicVolume;
}

void CMatrixMap::SetMusicVolume(float vol)
{
    if(!g_RangersInterface) return;
    
    if(FLAG(m_Flags, MMFLAG_MUSIC_VOL_CHANGING))
    {
        m_StoreCurrentMusicVolume = m_TargetMusicVolume;
    }
    else
    {
        SETFLAG(m_Flags, MMFLAG_MUSIC_VOL_CHANGING);
        m_StoreCurrentMusicVolume = g_RangersInterface->m_MusicVolumeGet();
    }

    m_TargetMusicVolume = vol;
}

void CMatrixMap::ShowPortraits()
{
    int n = 0;
    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_SHOWPORTRETS);

    CMatrixMapStatic** sb = m_AllObjects.Buff<CMatrixMapStatic*>();
    CMatrixMapStatic** se = m_AllObjects.BuffEnd<CMatrixMapStatic*>();
    for(; sb < se; ++sb)
    {
        if((*sb)->GetObjectType() == OBJECT_TYPE_MAPOBJECT && ((CMatrixMapObject*)(*sb))->m_BehaviorFlag == BEHF_PORTRET)
        {
            ((CMatrixMapObject*)(*sb))->m_PrevStateRobotsInRadius = ++n;
            ((CMatrixMapObject*)(*sb))->RChange(MR_Graph);
        }
    }
}

//void CMatrixMap::BeginDieSequence()
//{
//    m_ShadeOn = true;
//    m_ShadeTime = SHADER_TIME;
//    m_ShadeTimeCurrent = 0;
//
//    m_ShadePosFrom = m_Camera.GetTarget();
//
//    m_ShadePosTo  = m_ShadePosFrom ; //GetPlayerSide()->GetFlyer()->GetPos();
//    m_ShadeInterfaceColor = m_Minimap.GetColor();
//}

bool CMatrixMap::IsTraceNonPlayerObj()
{   
    if(
        IS_TRACE_STOP_OBJECT(g_MatrixMap->m_TraceStopObj) &&
        (g_MatrixMap->m_TraceStopObj->IsRobot() || 
        g_MatrixMap->m_TraceStopObj->IsBuilding() ||
        g_MatrixMap->m_TraceStopObj->IsTurret() ||
        g_MatrixMap->m_TraceStopObj->GetObjectType() == OBJECT_TYPE_FLYER ||
        g_MatrixMap->m_TraceStopObj->IsSpecial()) && 
        (g_MatrixMap->m_TraceStopObj->GetSide() != PLAYER_SIDE)
        ) return true;

    return false;
}