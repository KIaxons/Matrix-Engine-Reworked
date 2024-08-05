// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "stdafx.h"
#include "MatrixSide.hpp"
#include "MatrixMap.hpp"
#include "MatrixRobot.hpp"
#include "MatrixObjectBuilding.hpp"
#include "MatrixFormGame.hpp"
#include "MatrixMapStatic.hpp"
#include "Effects/MatrixEffectSelection.hpp"
#include "Interface/CConstructor.h"
#include "Interface/CIFaceElement.h"
#include "Logic/MatrixTactics.h"
#include "Logic/MatrixAIGroup.h"
#include "Interface/CConstructor.h"
#include "Interface/CInterface.h"
#include "Logic/MatrixTactics.h"
#include "Logic/MatrixAIGroup.h"
#include "MatrixObjectCannon.hpp"
#include "MatrixFlyer.hpp"
#include "Interface/CCounter.h"
#include <time.h>
#include <math.h>

// robot->GetEnv()->m_Place -   место, куда робот идет или где стоит. 
//                              <0 - роботу нужно назначить приказ
//                              >=0 - должен быть низкоуровневый приказ роботу. есть исключение: роботу не был отдан приказ так как предыдущий нельзя было прервать CanBreakOrder==false. в этом случае когда CanBreakOrder станет true выполнится код: robot->GetEnv()->m_Place=-1

inline bool PrepareBreakOrder(CMatrixMapStatic* robot);
inline CPoint GetMapPos(CMatrixMapStatic* obj);
inline D3DXVECTOR2 GetWorldPos(CMatrixMapStatic* obj);
inline bool IsToPlace(CMatrixRobotAI* robot, int place);    //Движется ли робот к назначенному месту
inline bool IsInPlace(CMatrixRobotAI* robot, int place);    //Если робот стоит на месте
inline bool IsInPlace(CMatrixRobotAI* robot);               //Если робот стоит на месте
inline int RobotPlace(CMatrixRobotAI* robot);
inline int TurretPlace(CMatrixTurret* turret);
inline int FlyerPlace(CMatrixFlyer* flyer);
inline int ObjPlace(CMatrixMapStatic* obj);
inline SMatrixPlace* ObjPlacePtr(CMatrixMapStatic* obj);
inline dword ObjPlaceData(CMatrixMapStatic* obj);
inline void ObjPlaceData(CMatrixMapStatic* obj, dword data);
inline SMatrixPlace* GetPlacePtr(int no);
inline bool CanMove(byte move_type, CMatrixRobotAI* robot);
inline int GetDesRegion(CMatrixRobotAI* robot);
inline int GetRegion(const CPoint& tp);
inline int GetRegion(int x, int y);
inline int GetRegion(CMatrixMapStatic* obj);
inline D3DXVECTOR3 SetPointOfAim(CMatrixMapStatic* obj);
inline int GetObjTeam(CMatrixMapStatic* robot) { return ((CMatrixRobotAI*)(robot))->GetTeam(); }
inline CInfo* GetEnv(CMatrixMapStatic* robot) { return ((CMatrixRobotAI*)(robot))->GetEnv(); }
inline int GetGroupLogic(CMatrixMapStatic* robot) { return ((CMatrixRobotAI*)(robot))->GetGroupLogic(); }
inline float Dist2(D3DXVECTOR2 p1, D3DXVECTOR2 p2) { return POW2(p1.x - p2.x) + POW2(p1.y - p2.y); }
inline bool CanChangePlace(CMatrixRobotAI* robot) { return (g_MatrixMap->GetTime() - robot->GetEnv()->m_PlaceNotFound) > 2000; }

inline bool PLIsToPlace(CMatrixRobotAI* robot);
inline CPoint PLPlacePos(CMatrixRobotAI* robot);

void CMatrixSideUnit::ClearLogicGroupsOrderLists(void)
{
    //Здесь должна была быть зачистка всех логических указателей для функции Restart()
    //Однако мне не удалось понять, что именно нужно чистить, чтобы логика доминаторов не сохранялась после рестарта карты
    /*
    for(int i = 0; i < m_TeamCnt; ++i)
    {
        m_Team[i].m_Action.m_Type = mlat_None;
    }
    */

    /*
    //Очищаем приказы во всех логических группах
    for(int i = 0; i < MAX_LOGIC_GROUP; ++i)
    {
        if(m_LogicGroup[i].RobotsCnt() <= 0) continue;

        m_LogicGroup[i].Order(mpo_Capture);
        m_LogicGroup[i].m_To = GetMapPos(building);
        m_LogicGroup[i].m_Obj = building;
        m_LogicGroup[i].SetShowPlace(false);
        m_LogicGroup[i].m_RoadPath->Clear();
    }
    */
}

//Вызывается из тактовой логики, используется для нормальной работы следующей функции
void CMatrixSideUnit::InterpolateManualControlledRobotHullPos(int ms)
{
    //return;
    if(!m_UnitUnderManualControl_available) return;

    D3DXVECTOR3 dir(m_UnitUnderManualControl_prevrp - m_UnitUnderManualControl_cur);

    float mul1 = (float)(1.0 - pow(0.996, double(ms)));
    float mul2 = (float)(1.0 - pow(0.994, double(ms)));
    //float mul = 1;

    m_UnitUnderManualControl_cur += dir * mul1;

    dir = (m_UnitUnderManualControl_ppos1 - m_UnitUnderManualControl_ppos0);
    m_UnitUnderManualControl_ppos0 += dir * mul2;

    m_UnitUnderManualControl_k += float(ms) * INVERT(2000);
    if(m_UnitUnderManualControl_k > 1.0f) m_UnitUnderManualControl_k = 1.0f;
}

//Рассинхронизирует положение корпуса и шагающего шасси робота, чтобы избавить корпус от неестественной тряски во время движения
//(в данный момент применяется только к роботам под ручным контролем игрока)
D3DXVECTOR3 CMatrixSideUnit::CorrectWalkingManualControlledRobotHullPos(D3DXVECTOR3& p, CMatrixRobot* robot)
{
    SObjectCore* core = robot->GetCore(DEBUG_CALL_INFO);
    //D3DXVECTOR3 p1(robot->m_PosX, robot->m_PosY, core->m_Matrix._43);

    //static int  micnt = 0;
    //static float mid = 0;
    //static float prevdelta;

    D3DXVECTOR3 pp, ppos(robot->m_PosX, robot->m_PosY, core->m_Matrix._43);
    //D3DXVec3TransformNormal(&m_UnitUnderManualControl_ppos1, &ppos, &core->m_IMatrix);
    m_UnitUnderManualControl_ppos1 = ppos;

    float speed = robot->m_Speed / ((CMatrixRobotAI*)robot)->GetMaxSpeed();

    static D3DXVECTOR3 pf;

    if(m_UnitUnderManualControl_available && speed > 0)
    {
        //D3DXVec3TransformNormal(&ppos, &m_UnitUnderManualControl_ppos0, &core->m_Matrix);
        ppos = m_UnitUnderManualControl_ppos0;

        //CHelper::Create(1, 0)->Line(ppos + D3DXVECTOR3(0, 0, 50), ppos + D3DXVECTOR3(0, 0, 100));

        D3DXVec3TransformCoord(&pp, &p, &core->m_Matrix);

        m_UnitUnderManualControl_prevrp = pp - ppos;

        pp = m_UnitUnderManualControl_cur + ppos;
        D3DXVec3TransformCoord(&pp, &pp, &core->m_IMatrix);

        core->Release();
        D3DXVECTOR3 rp = LERPVECTOR(m_UnitUnderManualControl_k, p, pp);

        if(pf.x != robot->m_ChassisForward.x)
        {
            m_UnitUnderManualControl_cur = m_UnitUnderManualControl_prevrp;
            m_UnitUnderManualControl_ppos0 = m_UnitUnderManualControl_ppos1;
            m_UnitUnderManualControl_k = 0;

            rp = p;
        }

        pf = robot->m_ChassisForward;

        return rp;
    }
    else
    {
        m_UnitUnderManualControl_available = 1;

        D3DXVec3TransformCoord(&pp, &p, &core->m_Matrix);

        m_UnitUnderManualControl_prevrp = pp - ppos;
        m_UnitUnderManualControl_cur = m_UnitUnderManualControl_prevrp;
        m_UnitUnderManualControl_ppos0 = m_UnitUnderManualControl_ppos1;

        m_UnitUnderManualControl_k = 0;

        pf = robot->m_ChassisForward;

        core->Release();
        return p;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CMatrixSideUnit::CMatrixSideUnit() : CMain()
{
    m_Constructor = HNew(Base::g_MatrixHeap) CConstructor;
    //m_GroupsList = HNew(g_MatrixHeap) CMatrixGroupList;

    for(int i = 0; i < MAX_TEAM_CNT; ++i)
    {
        m_Team[i].m_RoadPath = HNew(Base::g_MatrixHeap) CMatrixRoadRoute(&(g_MatrixMap->m_RoadNetwork));
    }

    for(int i = 0; i < MAX_LOGIC_GROUP; ++i)
    {
        m_PlayerGroup[i].m_RoadPath = HNew(Base::g_MatrixHeap) CMatrixRoadRoute(&(g_MatrixMap->m_RoadNetwork));
    }

    for(int i = 0; i < MAX_RESOURCES; ++i)
    {
        m_Resources[i] = 300;
    }
}
CMatrixSideUnit::~CMatrixSideUnit()
{
    if(m_Region != nullptr)
    {
        HFree(m_Region, Base::g_MatrixHeap);
        m_Region = nullptr;
    }

    if(m_RegionIndex != nullptr)
    {
        HFree(m_RegionIndex, Base::g_MatrixHeap);
        m_RegionIndex = nullptr;
    }

    if(m_PlaceList != nullptr)
    {
        HFree(m_PlaceList, Base::g_MatrixHeap);
        m_PlaceList = nullptr;
    }

    if(m_Constructor != nullptr) HDelete(CConstructor, m_Constructor, Base::g_MatrixHeap);

    if(m_ColorTexture) CCache::Destroy(m_ColorTexture);

    /*
    if(m_GroupsList)
    {
        HDelete(CMatrixGroupList, m_GroupsList, g_MatrixHeap);
        m_GroupsList = nullptr;
    }
    */

    if(m_CurSelGroup)
    {
        HDelete(CMatrixGroup, m_CurSelGroup, Base::g_MatrixHeap);
        m_CurSelGroup = nullptr;
    }

    //m_CannonForBuild.Delete();
    if(m_CannonForBuild.m_Cannon)
    {
        HDelete(CMatrixTurret, m_CannonForBuild.m_Cannon, Base::g_MatrixHeap);
        m_CannonForBuild.m_Cannon = nullptr;
    }

    if(m_ConstructPanel) HDelete(CConstructorPanel, m_ConstructPanel, Base::g_MatrixHeap);

    for(int i = 0; i < MAX_TEAM_CNT; ++i)
    {
        if(m_Team[i].m_RoadPath)
        {
            HDelete(CMatrixRoadRoute, m_Team[i].m_RoadPath, Base::g_MatrixHeap);
            m_Team[i].m_RoadPath = nullptr;
        }
    }
    for(int i = 0; i < MAX_LOGIC_GROUP; ++i)
    {
        if(m_PlayerGroup[i].m_RoadPath)
        {
            HDelete(CMatrixRoadRoute, m_PlayerGroup[i].m_RoadPath, Base::g_MatrixHeap);
            m_PlayerGroup[i].m_RoadPath = nullptr;
        }
    }

    CMatrixGroup* grps = m_FirstGroup;

    while(grps)
    {
        if(grps->m_NextGroup)
        {
            grps = grps->m_NextGroup;
        }
        else
        {
            HDelete(CMatrixGroup, grps, Base::g_MatrixHeap);
            grps = nullptr;
            m_FirstGroup = nullptr;
            m_LastGroup = nullptr;
        }

        if(grps) HDelete(CMatrixGroup, grps->m_PrevGroup, Base::g_MatrixHeap);
    }

    m_CurrentGroup = nullptr; //SetCurGroup(nullptr);

    /*
    if(m_CurGroup)
    {
        HDelete(CMatrixGroup, m_CurGroup, g_MatrixHeap);
        m_CurGroup = nullptr;
    }
    */
}


void CMatrixSideUnit::GetResourceIncome(int& base_i, int& fa_i, ERes resource_type)
{
    int base_cnt = 0;
    int fa_cnt = 0;

    EBuildingType type = BUILDING_BASE;
    switch(resource_type)
    {
        case TITAN:       type = BUILDING_TITAN; break;
        case ELECTRONICS: type = BUILDING_ELECTRONIC; break;
        case ENERGY:      type = BUILDING_ENERGY; break;
        case PLASMA:      type = BUILDING_PLASMA; break;
    }

    CMatrixMapStatic* ms = CMatrixMapStatic::GetFirstLogic();
    for(; ms; ms = ms->GetNextLogic())
    {
        if(ms->IsBuildingAlive() && ms->GetSide() == m_Id)
        {
            if(ms->IsBase()) ++base_cnt;
            else if(ms->AsBuilding()->m_Kind == type) ++fa_cnt;
        }
    }

    int fu = GetResourceForceUp();
    fa_i = (fa_cnt * RESOURCES_INCOME_FROM_FACTORY)/* * 60000 / g_Config.m_Timings[tim]*/;
    base_i = (base_cnt * RESOURCES_INCOME_FROM_BASE * fu / 100)/* * 15000 / g_Config.m_Timings[RESOURCE_BASE]*/;
}

int CMatrixSideUnit::GetIncomePerTime(int building, int ms)
{
    ETimings tim;
    switch(building)
    {
        case BUILDING_TITAN: tim = RESOURCE_TITAN; break;
        case BUILDING_ELECTRONIC: tim = RESOURCE_ELECTRONICS; break;
        case BUILDING_PLASMA: tim = RESOURCE_PLASMA; break;
        case BUILDING_ENERGY: tim = RESOURCE_ENERGY; break;
        default: tim = RESOURCE_TITAN; break;
    }

    int fu = GetResourceForceUp();

    if(building != int(BUILDING_BASE)) return (RESOURCES_INCOME_FROM_FACTORY)/* * ms / g_Config.m_Timings[tim]*/;

    return (RESOURCES_INCOME_FROM_BASE * fu / 100)/* * (ms/4) / g_Config.m_Timings[RESOURCE_BASE]*/;
}

void CMatrixSideUnit::BufPrepare()
{
    if(!m_PlaceList) m_PlaceList = (int*)HAllocClear(sizeof(int) * g_MatrixMap->m_RoadNetwork.m_PlaceCnt, Base::g_MatrixHeap);
}

//Здесь спавнятся и получают приказ вертолёты, доставляющие игроку подкрепление
void CMatrixSideUnit::SpawnDeliveryFlyer(
    const D3DXVECTOR2& to,
    EFlyerOrder order,
    float ang,
    int place,
    const CPoint& b_pos,
    int robot_template,
    EFlyerKind flyer_type
)
{
    if(g_MatrixMap->m_AD_Obj_cnt >= MAX_ALWAYS_DRAW_OBJ) return;

    //CMatrixFlyer* flyer = g_MatrixMap->StaticAdd<CMatrixFlyer>(true);
    //flyer->m_FlyerKind = flyer_type;

    CMatrixFlyer* flyer = HNew(Base::g_MatrixHeap) CMatrixFlyer(flyer_type);
    g_MatrixMap->AddObject(flyer, true);

    g_MatrixMap->m_AD_Obj[g_MatrixMap->m_AD_Obj_cnt] = flyer;
    ++g_MatrixMap->m_AD_Obj_cnt;

    flyer->ApplyOrder(to, m_Id, order, ang, place, b_pos, robot_template);
}

void CMatrixSideUnit::BuildCrazyBot()
{
    SRobotTemplate bot;

    //Собираем робота из случайного набора доступных деталей
    bot.m_Chassis.m_nKind = (ERobotModuleKind)g_MatrixMap->Rnd(1, ROBOT_CHASSIS_COUNT);
    bot.m_Hull.m_Module.m_nKind = (ERobotModuleKind)(ROBOT_HULLS_COUNT ? g_MatrixMap->Rnd(1, ROBOT_HULLS_COUNT) : RUK_EMPTY);
    bot.m_Head.m_nKind = (ERobotModuleKind)(ROBOT_HEADS_COUNT ? g_MatrixMap->Rnd(1, ROBOT_HEADS_COUNT) : RUK_EMPTY);

    int hull_kind = bot.m_Hull.m_Module.m_nKind;
    for(int i = 0; i < (int)g_Config.m_RobotHullsConsts[hull_kind].weapon_pylon_data.size(); ++i)
    {
        int weapon_kind = RUK_EMPTY;

        //Если в этой игре в принципе подключены пушки и хотя бы одну из них можно установить в данный слот, выбираем случайную подходящую
        if(ROBOT_WEAPONS_COUNT && g_Config.m_RobotHullsConsts[hull_kind].weapon_pylon_data[i].fit_weapon.count())
        {
            do weapon_kind = g_MatrixMap->Rnd(1, ROBOT_WEAPONS_COUNT);
            while(!g_Config.m_RobotHullsConsts[hull_kind].weapon_pylon_data[i].fit_weapon.test(weapon_kind));
        }

        bot.m_Weapon[g_Config.m_RobotHullsConsts[hull_kind].weapon_pylon_data[i].constructor_slot_num].m_Module.m_nKind = (ERobotModuleKind)weapon_kind;
    }

    bot.m_Team = -1;

    m_Constructor->BuildRobotByTemplate(bot);
}

void CMatrixSideUnit::LogicTact(int ms)
{
    if(GetStatus() != SS_NONE && FLAG(g_Config.m_DIFlags, DI_SIDE_INFO))
    {
        switch(m_Id)
        {
            case PLAYER_SIDE: g_MatrixMap->m_DI.ShowScreenText(CWStr(L"Player Side:"), CWStr().Format(L"Titan <i>, Electronics <i>, Energy <i>, Plasma <i>", m_Resources[TITAN], m_Resources[ELECTRONICS], m_Resources[ENERGY], m_Resources[PLASMA]).Get()); break;
            case BLAZER_SIDE: g_MatrixMap->m_DI.ShowScreenText(CWStr(L"Blazer Side:"), CWStr().Format(L"Titan <i>, Electronics <i>, Energy <i>, Plasma <i>", m_Resources[TITAN], m_Resources[ELECTRONICS], m_Resources[ENERGY], m_Resources[PLASMA]).Get()); break;
            case KELLER_SIDE: g_MatrixMap->m_DI.ShowScreenText(CWStr(L"Keller Side:"), CWStr().Format(L"Titan <i>, Electronics <i>, Energy <i>, Plasma <i>", m_Resources[TITAN], m_Resources[ELECTRONICS], m_Resources[ENERGY], m_Resources[PLASMA]).Get()); break;
            case TERRON_SIDE: g_MatrixMap->m_DI.ShowScreenText(CWStr(L"Terron Side:"), CWStr().Format(L"Titan <i>, Electronics <i>, Energy <i>, Plasma <i>", m_Resources[TITAN], m_Resources[ELECTRONICS], m_Resources[ENERGY], m_Resources[PLASMA]).Get()); break;
            default: g_MatrixMap->m_DI.ShowScreenText(CWStr(L"Side ") + CWStr(m_Id) + CWStr(L":"), CWStr().Format(L"Titan <i>, Electronics <i>, Energy <i>, Plasma <i>", m_Resources[TITAN], m_Resources[ELECTRONICS], m_Resources[ENERGY], m_Resources[PLASMA]).Get()); break;
        }
    }

    if(GetStatus() != SS_NONE)
    {
        dword ctime = timeGetTime();
        dword v = (g_MatrixMap->GetStartTime() <= ctime) ? (ctime - g_MatrixMap->GetStartTime()) : (0xFFFFFFFF - g_MatrixMap->GetStartTime() + ctime);
        SetStatValue(STAT_TIME, v);
    }

    if(g_MatrixMap->GetPlayerSide() != this || FLAG(g_MatrixMap->m_Flags, MMFLAG_AUTOMATIC_MODE))
    {
        if(m_Id == PLAYER_SIDE)
        {
            if(!g_MatrixMap->ReinforcementsDisabled())
            {
                if(!g_MatrixMap->BeforeReinforcementsTime() && (FRND(1.0f) < 0.05f))
                {
                    CMatrixMapStatic* b = nullptr;
                    CMatrixMapStatic* ms = CMatrixMapStatic::GetFirstLogic();
                    for(; ms; ms = ms->GetNextLogic())
                    {
                        if(ms->IsBuildingAlive() && ms->GetSide() == m_Id)
                        {
                            b = ms;
                            if(FRND(1.0f) < 0.05f) break;
                        }
                    }

                    if(b) b->AsBuilding()->Reinforcements();
                }
            }
        }

        TactSideLogic();
        TactTL();
    }
    else
    {
        if(CMultiSelection::m_GameSelection)
        {
            SCallback cbs;
            cbs.mp = g_MatrixMap->m_Cursor.GetPos();
            cbs.calls = 0;

            dword mask = 0;
            if(g_SelectableTurrets) mask = TRACE_ROBOT | TRACE_FLYER | TRACE_TURRET | TRACE_BUILDING;
            else mask = TRACE_ROBOT | TRACE_FLYER | TRACE_BUILDING;
            CMultiSelection::m_GameSelection->Update(g_MatrixMap->m_Cursor.GetPos(), mask, SideSelectionCallBack, (dword)&cbs);
        }

        PumpGroups();
        if((!GetCurGroup() || !GetCurGroup()->GetObjectsCnt()) && (m_CurrSel == GROUP_SELECTED || m_CurrSel == ROBOT_SELECTED || m_CurrSel == FLYER_SELECTED))
        {
            Select(NOTHING);
        }

        TactPL();
    }

    CalcMaxSpeed();

    CMatrixMapStatic* object = CMatrixMapStatic::GetFirstLogic();
    m_RobotsCnt = 0;
    for(int i = 0; i < m_TeamCnt; ++i)
    {
        m_Team[i].m_RobotCnt = 0;
    }

    if(this == g_MatrixMap->GetPlayerSide())
    {
        if(m_CurrSel == BUILDING_SELECTED && m_ActiveObject)
        {
            if(((CMatrixBuilding*)m_ActiveObject)->m_Side != m_Id)
            {
                //Select(FLYER, nullptr);
                m_CurrentAction = NOTHING_SPECIAL;
                m_CannonForBuild.m_CanBuildFlag = 0;
                m_CannonForBuild.m_Cannon = nullptr;
                m_CannonForBuild.m_ParentBuilding = nullptr;
            }
        }

        switch(m_CurrentAction)
        {
            case NOTHING_SPECIAL:
            {
                g_MatrixMap->m_Cursor.SetVisible(true);
                break;
            }
            case BUILDING_TURRET:
            {
                //Выбранная игроком для выбора места постройки турель обновляет свои координаты в соответствии с положением курсора
                if(m_CannonForBuild.m_Cannon)
                {
                    bool build_flag = (m_CannonForBuild.m_ParentBuilding->m_TurretsHave < m_CannonForBuild.m_ParentBuilding->m_TurretsLimit);

                    if(g_IFaceList->m_InFocus == UNKNOWN)
                    {
                        //g_MatrixMap->m_Cursor.Select(CURSOR_EMPTY);
                        g_MatrixMap->m_Cursor.SetVisible(false);
                    }
                    else if(g_IFaceList->m_InFocus == INTERFACE)
                    {
                        g_MatrixMap->m_Cursor.SetVisible(true);
                        g_MatrixMap->m_Cursor.Select(CURSOR_ARROW);
                    }

                    CPoint places[MAX_PLACES];
                    int places_cnt = m_CannonForBuild.m_ParentBuilding->GetPlacesForTurrets(places);
                    //places->x, places + 1->y

                    D3DXVECTOR3 pos = g_MatrixMap->m_TraceStopPos;
                    g_MatrixMap->Trace(&pos, g_MatrixMap->m_Camera.GetFrustumCenter(), g_MatrixMap->m_Camera.GetFrustumCenter() + (g_MatrixMap->m_MouseDir * 10000.0f), TRACE_LANDSCAPE | TRACE_WATER);

                    int can_build_flag = IsInPlaces(places, places_cnt, Float2Int(pos.x * INVERT(GLOBAL_SCALE_MOVE)), Float2Int(pos.y * INVERT(GLOBAL_SCALE_MOVE)));

                    if(build_flag && can_build_flag >= 0)
                    {
                        m_CannonForBuild.m_Cannon->SetTerainColor(TURRET_UNDER_CONSTRUCTION_COLOR);
                        m_CannonForBuild.m_CanBuildFlag = 1;
                    }
                    else
                    {
                        m_CannonForBuild.m_Cannon->SetTerainColor(TURRET_CANT_BE_CONSTRUCTED_COLOR);
                        m_CannonForBuild.m_CanBuildFlag = 0;
                    }

                    //Турель навели на подходящее для строительства места, "линкуем" её туда
                    if(can_build_flag >= 0)
                    {
                        m_CannonForBuild.m_Cannon->m_Pos.x = places[can_build_flag].x * GLOBAL_SCALE_MOVE;
                        m_CannonForBuild.m_Cannon->m_Pos.y = places[can_build_flag].y * GLOBAL_SCALE_MOVE;
                        m_CannonForBuild.m_Cannon->m_Place = g_MatrixMap->m_RoadNetwork.FindInPL(places[can_build_flag]);
                    }
                    //Турель ещё не наведена на подходящее для строительства место, её положение обновляется по позиции курсора
                    else
                    {
                        m_CannonForBuild.m_Cannon->m_Pos.x = pos.x;
                        m_CannonForBuild.m_Cannon->m_Pos.y = pos.y;
                        m_CannonForBuild.m_Cannon->m_Place = -1;
                    }

                    if(can_build_flag >= 0)
                    {
                        for(int i = 0; i < m_CannonForBuild.m_ParentBuilding->m_TurretsLimit; ++i)
                        {
                            if(m_CannonForBuild.m_ParentBuilding->m_TurretsPlaces[i].m_Coord == places[can_build_flag])
                            {
                                float dang = (float)AngleDist(double(m_CannonForBuild.m_Cannon->m_Angle), float(m_CannonForBuild.m_ParentBuilding->m_TurretsPlaces[i].m_Angle));

                                m_CannonForBuild.m_Cannon->m_Angle += float(dang * (1.0 - pow(0.99, double(ms))));
                                m_CannonForBuild.m_Cannon->SetMustBeAngle(m_CannonForBuild.m_ParentBuilding->m_TurretsPlaces[i].m_Angle);
                                break;
                            }
                        }
                    }

                    m_CannonForBuild.m_Cannon->RChange(MR_Matrix);
                }

                break;
            }
        }
    }

    while(object)
    {
        if(object->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
        {
            CMatrixRobotAI* robot = object->AsRobot();

            if(robot->m_Side == m_Id && robot->m_CurrentState != ROBOT_DIP)
            {
                ++m_RobotsCnt;
                if(robot->GetTeam() >= 0)
                {
                    ASSERT(robot->GetTeam() >= 0 && robot->GetTeam() < m_TeamCnt);
                    ++m_Team[robot->GetTeam()].m_RobotCnt;
                }
            }
        }

        object = object->GetNextLogic();
    }

    //Нахуй надо? Там же отдельная полоска в интерфейсе есть.
    /*
    if(IsUnitUnderManualControlRobot())
    {
        m_UnitUnderManualControl->AsRobot()->ShowHitpoints();
    }
    */
}

//Игрок водит мышкой по экрану, но не по элементам интерфейса
void CMatrixSideUnit::OnMouseMove()
{
    //Если включён режим ручного управления и игрок не навёлся на интерфейс, передаём команду роботу
    /*
    if(m_Id == PLAYER_SIDE && IsUnitUnderManualControlRobot() && g_IFaceList->m_InFocus != INTERFACE)
    {
        CMatrixRobotAI* robot = GetUnitUnderManualControl()->AsRobot();
    }
    */
}

//Игрок кликнул левой кнопкой мыши не по интерфейсу
void CMatrixSideUnit::OnLButtonDown(const CPoint& mouse_pos)
{
    if(IsManualControlMode()) return;

    CMatrixMapStatic* pObject = MouseToLand();

    if(pObject == TRACE_STOP_NONE) return;

    //Если ранее игрок выбрал турель для постройки (клик выберет конкретное место)
    if(m_CurrentAction == BUILDING_TURRET && m_CannonForBuild.m_Cannon)
    {
        //Игрок запустил постройку турели, кликнув выбранной турелью по подходящему месту
        if(m_CannonForBuild.m_CanBuildFlag/* && (m_CannonForBuild.m_ParentBuilding->m_TurretsHave < m_CannonForBuild.m_ParentBuilding->m_TurretsLimit)*/)
        {
            if(g_MatrixMap->IsPaused()) return;
            CMatrixTurret* turret = g_MatrixMap->StaticAdd<CMatrixTurret>(true);
            turret->m_CurrentState = TURRET_UNDER_CONSTRUCTION;

            turret->SetInvulnerability();
            turret->m_Pos.x = m_CannonForBuild.m_Cannon->m_Pos.x; //g_MatrixMap->m_TraceStopPos.x;
            turret->m_Pos.y = m_CannonForBuild.m_Cannon->m_Pos.y; //g_MatrixMap->m_TraceStopPos.y;
            turret->m_Place = m_CannonForBuild.m_Cannon->m_Place;
            turret->SetSide(m_Id);
            turret->ModelInit(m_CannonForBuild.m_Cannon->m_TurretKind);

            turret->m_Angle = m_CannonForBuild.m_Cannon->GetMustBeAngle();
            turret->m_AddH = 0;

            turret->GetResources(MR_Matrix | MR_Graph);
            turret->m_ParentBuilding = (CMatrixBuilding*)m_ActiveObject;
            turret->JoinToGroup();

            ++m_CannonForBuild.m_ParentBuilding->m_TurretsHave;
            turret->SetHitpoints(0);
            ((CMatrixBuilding*)m_ActiveObject)->m_BuildingQueue.AddItem(turret);

            int kind = m_CannonForBuild.m_Cannon->m_TurretKind;
            STurretsConsts* turret_info = &g_Config.m_TurretsConsts[kind];
            AddResourceAmount(TITAN,       -turret_info->cost_titan);
            AddResourceAmount(ELECTRONICS, -turret_info->cost_electronics);
            AddResourceAmount(ENERGY,      -turret_info->cost_energy);
            AddResourceAmount(PLASMA,      -turret_info->cost_plasma);

            m_CurrentAction = NOTHING_SPECIAL;
            m_CannonForBuild.Delete();
            CSound::Play(S_TURRET_BUILD_START, SL_ALL);
            g_IFaceList->ResetBuildCaMode();

            return;
        }
        //Игрок кликнул выбранной турелью в неподходящую точку на карте
        else return;
    }

    int mx = Float2Int(g_MatrixMap->m_TraceStopPos.x / GLOBAL_SCALE_MOVE);
    int my = Float2Int(g_MatrixMap->m_TraceStopPos.y / GLOBAL_SCALE_MOVE);
    D3DXVECTOR3 tpos = g_MatrixMap->m_TraceStopPos;

    //Игрок кликнул по миникарте
    if(g_IFaceList->m_InFocus == INTERFACE && g_IFaceList->m_FocusedInterface->m_strName == IF_MINI_MAP)
    {
        D3DXVECTOR2 tgt;
        if(g_MatrixMap->m_Minimap.CalcMinimap2World(tgt))
        {
            pObject = TRACE_STOP_LANDSCAPE;
            mx = Float2Int(tgt.x / GLOBAL_SCALE_MOVE);
            my = Float2Int(tgt.y / GLOBAL_SCALE_MOVE);
            tpos = D3DXVECTOR3(tgt.x, tgt.y, tpos.z);
        }
    }

    //Если игрок задаёт роботу команду из правого меню (выбрал мышкой на интерфейсе действие и готовится отдать соответствующий приказ)
    if(IS_PREORDERING)
    {
        if(FLAG(g_IFaceList->m_IfListFlags, PREORDER_MOVE))
        {
            //Move 
            RESETFLAG(g_IFaceList->m_IfListFlags, PREORDER_MOVE | ORDERING_MODE);

            PGOrderMoveTo(SelGroupToLogicGroup(), CPoint(mx - ROBOT_MOVECELLS_PER_SIZE / 2, my - ROBOT_MOVECELLS_PER_SIZE / 2));

            //Отдельный обработчик приказа движения для управляемых вертолётов в выбранной группе
            CMatrixGroup* group = GetCurGroup();
            CMatrixGroupObject* objs = group->m_FirstObject;
            while(objs)
            {
                if(objs->ReturnObject() && objs->ReturnObject()->GetObjectType() == OBJECT_TYPE_FLYER)
                {
                    int param = (group->GetObjectsCnt() - group->GetRobotsCnt()) - 1;
                    if(param > 4) param = 4;
                    float x = g_MatrixMap->RndFloat(g_MatrixMap->m_TraceStopPos.x - param * GLOBAL_SCALE_MOVE, g_MatrixMap->m_TraceStopPos.x + param * GLOBAL_SCALE_MOVE);
                    float y = g_MatrixMap->RndFloat(g_MatrixMap->m_TraceStopPos.y - param * GLOBAL_SCALE_MOVE, g_MatrixMap->m_TraceStopPos.y + param * GLOBAL_SCALE_MOVE);
                    ((CMatrixFlyer*)objs->ReturnObject())->SetTarget(D3DXVECTOR2(x, y));
                }
                objs = objs->m_NextObject;
            }
        }
        else if(FLAG(g_IFaceList->m_IfListFlags, PREORDER_FIRE))
        {
            //Игрок указал курсором с прицелом на какой-то объект
            if(IS_TRACE_STOP_OBJECT(pObject) && (pObject->IsAlive() || pObject->IsSpecial()))
            {
                //Для турели
                if(m_ActiveObject && m_ActiveObject->GetObjectType() == OBJECT_TYPE_TURRET)
                {
                    RESETFLAG(g_IFaceList->m_IfListFlags, PREORDER_FIRE | ORDERING_MODE);
                    if(pObject != m_ActiveObject->AsTurret()) m_ActiveObject->AsTurret()->SetTargetOverride(pObject);
                }
                else //Для роботов и вертолётов
                {
                    RESETFLAG(g_IFaceList->m_IfListFlags, PREORDER_FIRE | ORDERING_MODE);
                    PGOrderAttack(SelGroupToLogicGroup(), GetMapPos(pObject), pObject);
                }
            }
            //Игрок указал курсором с прицелом на пустую точку
            else
            {
                if(m_ActiveObject && m_ActiveObject->GetObjectType() == OBJECT_TYPE_TURRET)
                {
                    RESETFLAG(g_IFaceList->m_IfListFlags, PREORDER_FIRE | ORDERING_MODE);
                }
                else //Для роботов и вертолётов
                {
                    RESETFLAG(g_IFaceList->m_IfListFlags, PREORDER_FIRE | ORDERING_MODE);
                    PGOrderAttack(SelGroupToLogicGroup(), CPoint(mx - ROBOT_MOVECELLS_PER_SIZE / 2, my - ROBOT_MOVECELLS_PER_SIZE / 2), nullptr);
                }
            }
        }
        else if(FLAG(g_IFaceList->m_IfListFlags, PREORDER_CAPTURE))
        {
            //Capture
            if(IS_TRACE_STOP_OBJECT(pObject) && pObject->IsBuildingAlive() && pObject->GetSide() != PLAYER_SIDE)
            {
                RESETFLAG(g_IFaceList->m_IfListFlags, PREORDER_CAPTURE | ORDERING_MODE);

                PGOrderCapture(SelGroupToLogicGroup(), (CMatrixBuilding*)pObject);
            }
        }
        else if(FLAG(g_IFaceList->m_IfListFlags, PREORDER_PATROL))
        {
            //Patrol
            RESETFLAG(g_IFaceList->m_IfListFlags, PREORDER_PATROL | ORDERING_MODE);
            PGOrderPatrol(SelGroupToLogicGroup(), CPoint(mx - ROBOT_MOVECELLS_PER_SIZE / 2, my - ROBOT_MOVECELLS_PER_SIZE / 2));
        }
        else if(FLAG(g_IFaceList->m_IfListFlags, PREORDER_BOMB))
        {
            //Nuclear BOMB!!! spasaisya kto mozhet!!! dab shas rvanet bombu!!!!
            if(IS_TRACE_STOP_OBJECT(pObject) && (pObject->IsAlive() || pObject->IsSpecial()))
            {
                RESETFLAG(g_IFaceList->m_IfListFlags, PREORDER_BOMB | ORDERING_MODE);

                PGOrderBomb(SelGroupToLogicGroup(), GetMapPos(pObject), pObject);
            }
            else
            {
                RESETFLAG(g_IFaceList->m_IfListFlags, PREORDER_BOMB | ORDERING_MODE);

                PGOrderBomb(SelGroupToLogicGroup(), CPoint(mx - ROBOT_MOVECELLS_PER_SIZE / 2, my - ROBOT_MOVECELLS_PER_SIZE / 2), nullptr);
            }
        }
        else if(FLAG(g_IFaceList->m_IfListFlags, PREORDER_REPAIR))
        {
            //Repair our robots please
            if(IS_TRACE_STOP_OBJECT(pObject) && pObject->IsAlive() && pObject->GetSide() == PLAYER_SIDE)
            {
                RESETFLAG(g_IFaceList->m_IfListFlags, PREORDER_REPAIR | ORDERING_MODE);

                PGOrderRepair(SelGroupToLogicGroup(), (CMatrixBuilding*)pObject);
            }
        }
    }
}

//Игрок кликнул (отпустил) левую кнопку мыши после клика не по интерфейсу
void CMatrixSideUnit::OnLButtonUp(const CPoint& mouse_pos)
{
    if(IsManualControlMode()) return;

    CMatrixMapStatic* pObject = MouseToLand();

    if(pObject == TRACE_STOP_NONE) return;

    if(IS_TRACE_STOP_OBJECT(pObject)) {}
}

//Двойной клик левой кнопкой мыши не по интерфейсу
void CMatrixSideUnit::OnLButtonDouble(const CPoint& mouse_pos)
{
    if(IsManualControlMode()) return;

    CMatrixMapStatic* pObject = MouseToLand();

    //Если двойной клик пришелся на пустое место, либо не по роботу игрока, то прерываем исполнение
    if(
        pObject == TRACE_STOP_NONE ||
        !(IS_TRACE_STOP_OBJECT(pObject) && pObject->IsRobotAlive() && pObject->GetSide() == PLAYER_SIDE)
      ) return;

    //Выделение всех роботов на экране по двойному левому клику мыши на одном из них
    if(IS_TRACE_STOP_OBJECT(pObject) && pObject->IsRobotAlive() && pObject->GetSide() == PLAYER_SIDE)
    {
        D3DXVECTOR3 o_pos = pObject->GetGeoCenter();
        CMatrixMapStatic* st = CMatrixMapStatic::GetFirstLogic();

        if(GetCurGroup())
        {
            SelectedGroupUnselect();
            GetCurSelGroup()->RemoveAll();
        }

        while(st)
        {
            if(st->GetSide() == PLAYER_SIDE && st->IsRobotAlive() && !st->AsRobot()->IsCrazy())
            {
                D3DXVECTOR3 temp = o_pos - st->GetGeoCenter();
                if(D3DXVec3LengthSq(&temp) <= FRIENDLY_SEARCH_RADIUS * FRIENDLY_SEARCH_RADIUS)
                {
                    GetCurSelGroup()->AddObject(st, -4);
                }
            }

            st = st->GetNextLogic();
        }
    }
    CreateGroupFromCurrent();

    if(GetCurGroup() && GetCurGroup()->GetObjectsCnt() == 1)
    {
        Select(ROBOT, nullptr);
    }
    else if(GetCurGroup() && GetCurGroup()->GetObjectsCnt() > 1)
    {
        Select(GROUP, nullptr);
    }
}

//Игрок кликнул (нажал) правой кнопкой мыши не по интерфейсу
void CMatrixSideUnit::OnRButtonDown(const CPoint& mouse_pos)
{
    if(IsManualControlMode()) return;

    //Если ранее игрок выбрал для строительства определённую турель, правый клик сбросит её установку
    if(IS_PREORDERING && m_CurrentAction == BUILDING_TURRET)
    {
        g_IFaceList->ResetOrderingMode();
        m_CannonForBuild.Delete();
        m_CurrentAction = NOTHING_SPECIAL;
        return;
    }

    //Определяется возможный объект клика (если игрок кликнул на конкретный объект)
    CMatrixMapStatic* pObject = MouseToLand();

    //Определяются координаты места клика
    int mx = Float2Int(g_MatrixMap->m_TraceStopPos.x / GLOBAL_SCALE_MOVE);
    int my = Float2Int(g_MatrixMap->m_TraceStopPos.y / GLOBAL_SCALE_MOVE);
    D3DXVECTOR3 tpos = g_MatrixMap->m_TraceStopPos;

    //Если игрок кликнул по миникарте, перед этим выделив группу юнитов
    if(!IS_PREORDERING && GetCurGroup() && g_IFaceList->m_InFocus == INTERFACE && g_IFaceList->m_FocusedInterface->m_strName == IF_MINI_MAP)
    {
        D3DXVECTOR2 tgt;
        if(g_MatrixMap->m_Minimap.CalcMinimap2World(tgt))
        {
            pObject = TRACE_STOP_LANDSCAPE;
            mx = Float2Int(tgt.x / GLOBAL_SCALE_MOVE);
            my = Float2Int(tgt.y / GLOBAL_SCALE_MOVE);
            tpos = D3DXVECTOR3(tgt.x, tgt.y, tpos.z);
            g_MatrixMap->m_Minimap.AddEvent(tpos.x, tpos.y, 0xffff0000, 0xffff0000);
        }
    }

    if(pObject == TRACE_STOP_NONE) return;

    //Если игрок не выбирал приказы с правой панели юнита
    if(!IS_PREORDERING)
    {
        //Если в данный момент выделен какой-либо робот, вертолёт или группа
        if(m_CurrSel == GROUP_SELECTED || m_CurrSel == ROBOT_SELECTED || m_CurrSel == FLYER_SELECTED)
        {
            //Игрок кликнул по зданию противника (либо нейтральному): приказ захвата
            if(IS_TRACE_STOP_OBJECT(pObject) && pObject->IsBuildingAlive() && pObject->GetSide() != m_Id)
            {
                //Capture - приказ захвата всей выделенной группе
                PGOrderCapture(SelGroupToLogicGroup(), (CMatrixBuilding*)pObject);
            }
            //Игрок кликнул по противнику: приказ атаки
            else if(IS_TRACE_STOP_OBJECT(pObject) && ((pObject->IsUnitAlive() && pObject->GetSide() != m_Id) || pObject->IsSpecial()))
            {
                //Attack - приказ атаки всей выделенной группе
                PGOrderAttack(SelGroupToLogicGroup(), GetMapPos(pObject), pObject);
            }
            //Игрок кликнул по свободному пространству на карте: приказ движения
            else if(pObject == TRACE_STOP_LANDSCAPE || pObject == TRACE_STOP_WATER || (IS_TRACE_STOP_OBJECT(pObject)))
            {
                //MoveTo - приказ движения всей выделенной группе
                PGOrderMoveTo(SelGroupToLogicGroup(), CPoint(mx - ROBOT_MOVECELLS_PER_SIZE / 2, my - ROBOT_MOVECELLS_PER_SIZE / 2));

                //Подробный расчёт точки движения на случай, если в выделенной группе имелись вертолёты
                CMatrixGroupObject *objs = GetCurGroup()->m_FirstObject;
                while(objs)
                {
                    if(objs->ReturnObject() && objs->ReturnObject()->GetObjectType() == OBJECT_TYPE_FLYER)
                    {
                        int param = (GetCurGroup()->GetObjectsCnt() - GetCurGroup()->GetRobotsCnt()) - 1;
                        if(param > 4) param = 4;
                        float x = g_MatrixMap->RndFloat(g_MatrixMap->m_TraceStopPos.x - param * GLOBAL_SCALE_MOVE, g_MatrixMap->m_TraceStopPos.x + param * GLOBAL_SCALE_MOVE);
                        float y = g_MatrixMap->RndFloat(g_MatrixMap->m_TraceStopPos.y - param * GLOBAL_SCALE_MOVE, g_MatrixMap->m_TraceStopPos.y + param * GLOBAL_SCALE_MOVE);
                        ((CMatrixFlyer*)objs->ReturnObject())->SetTarget(D3DXVECTOR2(x, y));
                    }
                    objs = objs->m_NextObject;
                }
            }
        }
        //Если в данный момент выделена турель
        if(m_CurrSel == TURRET_SELECTED)
        {
            if(!IS_PREORDERING)
            {
                if(IS_TRACE_STOP_OBJECT(pObject) && ((pObject->IsUnitAlive() && pObject->GetSide() != m_Id) || pObject->IsSpecial()))
                {
                    m_ActiveObject->AsTurret()->SetTargetOverride(pObject);
                }
            }
        }
        //Если в данный момент выбрана база, то устанавливаем точку сбора
        else if(m_CurrSel == BASE_SELECTED && !g_MatrixMap->IsPaused())
        {
            GetCurSelObject()->AsBuilding()->SetGatheringPoint(Float2Int(g_MatrixMap->m_TraceStopPos.x), Float2Int(g_MatrixMap->m_TraceStopPos.y));
            //Первоначальная отрисовка точки сбора (перенесена в функцию общей постоянной отрисовки)
            //CMatrixEffect::CreateMoveToAnim(2);
        }
    }
}

//Игрок кликнул (отпустил) правую кнопку мыши не по интерфейсу
void CMatrixSideUnit::OnRButtonUp(const CPoint& mouse_pos)
{
    //В режиме ручного управления роботом используем этот обработчик, чтобы ловить нажатия ПКМ для активации/деактивации режима ручного наведения ракет
    if(IsManualControlMode())
    {
        if(GetUnitUnderManualControl()->IsRobotAlive())
        {
            CMatrixRobotAI* robot = GetUnitUnderManualControl()->AsRobot();
            if(robot->IsHomingMissilesEquipped())
            {
                if(!g_Config.IsManualMissileControl()) g_Config.m_IsManualMissileControl = true;
                else g_Config.m_IsManualMissileControl = false;
                CSound::AddSound(S_GUIDANCE_SWITCH, robot->GetGeoCenter(), SL_ALL, SEF_SKIP);
            }
        }
    }
}

//Двойной клик правой клавишей мыши не по интерфейсу
void CMatrixSideUnit::OnRButtonDouble(const CPoint& mouse_pos)
{
    //В данный момент функционала не имеет
    return;

    if(IsManualControlMode()) return;

    /*
    if(m_CurrentAction == BUILDING_TURRET) return;

    CMatrixMapStatic* pObject = MouseToLand();
    if(pObject == TRACE_STOP_NONE) return;

    if(pObject == TRACE_STOP_LANDSCAPE)
    {
        if(m_CurrentAction == CAPTURING_ROBOT || m_CurrentAction == GETTING_IN_ROBOT)
        {
            m_CurrentAction = NOTHING_SPECIAL;
        }
        if(m_CurrSel == BUILDING_SELECTED || m_CurrSel == BASE_SELECTED)
        {
            Select(FLYER, nullptr);
            return;
        }
    }
    else if(pObject == TRACE_STOP_WATER)
    {
        if(m_CurrentAction == CAPTURING_ROBOT || m_CurrentAction == GETTING_IN_ROBOT)
        {
            m_CurrentAction = NOTHING_SPECIAL;
        }

        if(!(m_CurrSel == ROBOT_SELECTED)) Select(FLYER, nullptr);
        return;
    }
    */
}

void CMatrixSideUnit::OnForward(bool is_down)
{
    if(!IsUnitUnderManualControlRobot() || !m_UnitUnderManualControl) return;

    CMatrixRobotAI* robot = m_UnitUnderManualControl->AsRobot();
    bool is_ord = robot->FindOrderLikeThat(ROT_MOVE_TO);
    if(is_down && !is_ord)
    {
        D3DXVECTOR3 vel = robot->m_ChassisForward * robot->GetMaxSpeed();
        float x = robot->m_PosX + vel.x;
        float y = robot->m_PosY + vel.y;
        robot->MoveTo(Float2Int(x / GLOBAL_SCALE_MOVE), Float2Int(y / GLOBAL_SCALE_MOVE));
    }
    else if(!is_down && is_ord) robot->StopMoving();
}

void CMatrixSideUnit::OnBackward(bool is_down)
{
    if(!IsUnitUnderManualControlRobot() || !m_UnitUnderManualControl) return;

    CMatrixRobotAI* robot = m_UnitUnderManualControl->AsRobot();
    bool is_ord = robot->FindOrderLikeThat(ROT_MOVE_TO_BACK);
    if(is_down && !is_ord)
    {
        D3DXVECTOR3 vel = robot->m_ChassisForward * robot->GetMaxSpeed();
        float x = robot->m_PosX - vel.x;
        float y = robot->m_PosY - vel.y;
        robot->MoveToBack(Float2Int(x / GLOBAL_SCALE_MOVE), Float2Int(y / GLOBAL_SCALE_MOVE));
    }
    else if(!is_down && is_ord) robot->StopMoving();
}

/*
void CMatrixSideUnit::OnLeft(bool is_down)
{
    if(!IsUnitUnderManualControlRobot() || !m_UnitUnderManualControl) return;
}

void CMatrixSideUnit::OnRight(bool is_down)
{
    if(!IsUnitUnderManualControlRobot() || !m_UnitUnderManualControl) return;
}
*/

void CMatrixSideUnit::Select(ESelType type, CMatrixMapStatic* pObject)
{
    if(m_CurrSel == TURRET_SELECTED)
    {
        g_IFaceList->DeletePersonalImage();
        g_IFaceList->ResetOrderingMode();

        m_ActiveObject->AsTurret()->UnSelect();
    }
    else if(m_CurrSel == BUILDING_SELECTED || m_CurrSel == BASE_SELECTED)
    {
        g_IFaceList->DeletePersonalImage();
        g_IFaceList->DeleteDynamicTurrets();
        //RESETFLAG(g_IFaceList->m_IfListFlags, TURRET_BUILD_MODE | FLYER_BUILD_MODE);
        g_IFaceList->ResetOrderingMode();

        m_ActiveObject->AsBuilding()->UnSelect();
    }

    //m_CurrentAction = NOTHING_SPECIAL;
    m_ActiveObject = pObject;

    if((m_CurrSel == ROBOT_SELECTED || m_CurrSel == FLYER_SELECTED || m_CurrSel == ARCADE_SELECTED) && type != ARCADE)
    {
        RESETFLAG(g_IFaceList->m_IfListFlags, SINGLE_MODE);
        g_IFaceList->DeleteWeaponDynamicStatics();
        g_IFaceList->DeletePersonalImage();
        g_IFaceList->ResetOrderingMode();
    }

    if(m_CurrSel == GROUP_SELECTED)
    {
        g_IFaceList->DeleteProgressBars(nullptr);
        g_IFaceList->DeleteGroupIcons();
        g_IFaceList->DeletePersonalImage();
        g_IFaceList->ResetOrderingMode();
    }

    //Играется случайный звук при выборе робота/группы роботов
    if(type == GROUP || type == FLYER || type == ROBOT)
    {
        //Эта проверка нужна, чтобы робот не поигрывал звук выделения при выходе из режима ручного управления
        bool is_after_manual = false;
        if(pObject && pObject->IsRobot())
        {
            if(pObject->AsRobot()->IsAfterManual())
            {
                //Снимаем с робота маркер отключения ручного контроля
                pObject->AsRobot()->SetAfterManual(false);
                is_after_manual = true;
            }
        }

        if(m_Id == PLAYER_SIDE && !is_after_manual)
        {
            switch(g_MatrixMap->Rnd(1, 7))
            {
                case 1: CSound::Play(S_SELECTION_1, SL_SELECTION); break;
                case 2: CSound::Play(S_SELECTION_2, SL_SELECTION); break;
                case 3: CSound::Play(S_SELECTION_3, SL_SELECTION); break;
                case 4: CSound::Play(S_SELECTION_4, SL_SELECTION); break;
                case 5: CSound::Play(S_SELECTION_5, SL_SELECTION); break;
                case 6: CSound::Play(S_SELECTION_6, SL_SELECTION); break;
                case 7: CSound::Play(S_SELECTION_7, SL_SELECTION); break;
            }
        }
    }

    if(type == TURRET && pObject)
    {
        m_nCurrRobotPos = -1;
        m_CurrSel = TURRET_SELECTED;
        pObject->AsTurret()->CreateSelection();
        g_IFaceList->CreatePersonalImage(pObject);

        CSound::Play(S_TURRET_SEL, SL_SELECTION);
    }
    else if(type == BUILDING && pObject)
    {
        CMatrixBuilding* building = pObject->AsBuilding();

        m_nCurrRobotPos = -1;
        m_CurrSel = BUILDING_SELECTED;

        building->CreateSelection();
        g_IFaceList->CreateDynamicTurrets(building);

        if(pObject->IsBase())
        {
            m_CurrSel = BASE_SELECTED;
            m_Constructor->SetBase(building);

            if(FLAG(g_MatrixMap->m_Flags, MMFLAG_SOUND_BASE_SEL_ENABLED)) CSound::Play(S_BASE_SEL, SL_SELECTION);
            SETFLAG(g_MatrixMap->m_Flags, MMFLAG_SOUND_BASE_SEL_ENABLED);
        }
        else CSound::Play(S_BUILDING_SEL, SL_SELECTION);
    }
    else if(type == GROUP)
    {
        m_CurrSel = GROUP_SELECTED;

        SetCurSelNum(0);
        g_IFaceList->CreateGroupIcons();

        ShowOrderState();
    }
    else if(type == ROBOT || type == FLYER)
    {
        SETFLAG(g_IFaceList->m_IfListFlags, SINGLE_MODE);
        if(type == ROBOT) m_CurrSel = ROBOT_SELECTED;
        else /*if(type == FLYER)*/ m_CurrSel = FLYER_SELECTED;
        SetCurSelNum(0);
        g_IFaceList->CreateWeaponDynamicStatics();

        ShowOrderState();
    }
    else if(type == ARCADE) {}
    else
    {
        m_nCurrRobotPos = -1;
        m_CurrSel = NOTHING_SELECTED;
        SelectedGroupUnselect();
    }
}

void CMatrixSideUnit::Reselect()
{
    if(!GetCurGroup()) return;

    CMatrixMapStatic* objs = CMatrixMapStatic::GetFirstLogic();
    while(objs)
    {
        if(objs->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
        {
            if(!GetCurGroup()->FindObject(objs)) objs->AsRobot()->UnSelect();
            else objs->AsRobot()->SelectByGroup();
        }
        else if(objs->GetObjectType() == OBJECT_TYPE_FLYER)
        {
            if(!GetCurGroup()->FindObject(objs)) objs->AsFlyer()->UnSelect();
            else objs->AsFlyer()->SelectByGroup();
        }

        objs = objs->GetNextLogic();
    }
}

void CMatrixSideUnit::ShowOrderState()
{
    /*
    bool order_stop = false;
    bool order_move = false;
    bool order_capture = false;
    bool order_attack = false;
    bool order_patrol = false;
    */

    bool auto_capture = false;
    bool auto_attack = false;
    bool auto_defence = false;

    CMatrixGroupObject* objs = GetCurGroup()->m_FirstObject;
    while(objs)
    {
        if(objs->ReturnObject() && objs->ReturnObject()->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)(objs->ReturnObject());
            if(robot->IsRobotAlive())
            {
                if(robot->GetGroupLogic() >= 0)
                {
                    int cur_order = g_MatrixMap->GetSideById(robot->GetSide())->m_PlayerGroup[robot->GetGroupLogic()].Order();
                    
                    switch(cur_order)
                    {
                    /*case(mpo_MoveTo):
                        order_move = true;
                        break;
                    case(mpo_Capture):
                        order_capture = true;
                        break;
                    case(mpo_Attack):
                        order_attack = true;
                        break;
                    case(mpo_Patrol):
                        order_patrol = true;
                        break;*/
                    case(mpo_AutoAttack):
                        auto_attack = true;
                        break;
                    case(mpo_AutoCapture):
                        auto_capture = true;
                        break;
                    case(mpo_AutoDefence):
                        auto_defence = true;
                        break;
                    //default:
                        //order_stop = true;
                    }

                    /*if (cur_order == mpo_MoveTo)
                    {
                        order_move = true;
                    }
                    else if(cur_order == mpo_Capture)
                    {
                        order_capture = true;
                    }
                    else if(cur_order == mpo_Attack)
                    {
                        order_attack = true;
                    }
                    else if(cur_order == mpo_Patrol)
                    {
                        order_patrol = true;
                    }
                    else if(cur_order == mpo_AutoAttack)
                    {
                        auto_attack = true;
                    }
                    else if(cur_order == mpo_AutoCapture)
                    {
                        auto_capture = true;
                    }
                    else if(cur_order == mpo_AutoDefence)
                    {
                        auto_defence = true;
                    }*/
                }
            }
        }

        objs = objs->m_NextObject;
    }
    
    if(auto_attack) SETFLAG(g_IFaceList->m_IfListFlags, AUTO_ATTACK_ON);
    else RESETFLAG(g_IFaceList->m_IfListFlags, AUTO_ATTACK_ON);
    
    if(auto_capture) SETFLAG(g_IFaceList->m_IfListFlags, AUTO_CAPTURE_ON);
    else RESETFLAG(g_IFaceList->m_IfListFlags, AUTO_CAPTURE_ON);

    if(auto_defence) SETFLAG(g_IFaceList->m_IfListFlags, AUTO_PROTECT_ON);
    else RESETFLAG(g_IFaceList->m_IfListFlags, AUTO_PROTECT_ON);
}

bool CMatrixSideUnit::MouseToLand(const CPoint& mouse_pos, float* pWorldX, float* pWorldY, int* pMapX, int* pMapY)
{
    if(g_MatrixMap->m_TraceStopObj)
    {
        *pMapX = int(g_MatrixMap->m_TraceStopPos.x / GLOBAL_SCALE);
        *pMapY = int(g_MatrixMap->m_TraceStopPos.y / GLOBAL_SCALE);
        *pWorldX = (float)*pMapX * GLOBAL_SCALE;
        *pWorldY = (float)*pMapY * GLOBAL_SCALE;

        return true;
    }

    return false;
}

CMatrixMapStatic* CMatrixSideUnit::MouseToLand()
{
    return g_MatrixMap->m_TraceStopObj;
}

void CMatrixSideUnit::RobotStop(void*)
{
    if(IsUnitUnderManualControlRobot())
    {
        m_UnitUnderManualControl->AsRobot()->BreakAllOrders();
    }
}

void __stdcall CMatrixSideUnit::PlayerAction(void* object)
{
    CIFaceElement* element = (CIFaceElement*)object;

    if(element->m_strName == IF_BUILD_ROBOT)
    {
        g_IFaceList->m_RCountControl->Reset();
        g_IFaceList->m_RCountControl->CheckUp();
        m_ConstructPanel->ActivateAndSelect();
    }
}

//void CMatrixSideUnit::GiveRandomOrder()
//{
//    //return;
//    //Attack
//    CMatrixGroup* groups = m_GroupsList->m_FirstGroup;
//    while(groups){
//
//        if(!groups->GetTactics()){
//            CMatrixGroupObject* gr_objects = groups->m_FirstObject;
//            while(gr_objects){
//                if(gr_objects->ReturnObject()->GetObjectType() == OBJECT_TYPE_ROBOT_AI){
//                    if(((CMatrixRobotAI*)gr_objects->ReturnObject())->GetEnv()->GetEnemyCnt() > 0){
//                        groups->InstallTactics(ATTACK_TACTICS, m_TacticsPar);
//                        if(groups->GetTactics()){
//                            groups->GetTactics()->InitialiseTactics(groups, nullptr, -1);
//                        }
//                    }
//                }
//                gr_objects = gr_objects->m_NextObject;
//            }
//        }
//
//        if(groups->GetTactics() != nullptr && (groups->GetTactics()->GetType() == ATTACK_TACTICS || groups->GetTactics()->GetType() == JOIN_TACTICS)){
//            groups = groups->m_NextGroup;
//            continue;
//        }
//
//        if(groups->GetTactics() != nullptr && groups->GetTactics()->GetType() == CAPTURE_TACTICS && groups->GetTactics()->GetTarget() && ((CMatrixBuilding*)groups->GetTactics()->GetTarget())->m_Side != m_Id){
//            groups = groups->m_NextGroup;
//            continue;
//        }
//
//
//        
//        if(groups->GetGroupId() == 1){
//            //Capture
//            CMatrixMapStatic*   ms = CMatrixMapStatic::GetFirstLogic();
//            CMatrixMapStatic*   nearest = nullptr;
//            float               nearest_l = 0;
//
//            while(ms) {
//                if(ms->GetObjectType()==OBJECT_TYPE_BUILDING && !((CMatrixBuilding*)ms)->IsBase() && ((CMatrixBuilding *)ms)->m_Side != m_Id) {
//                    if(!((CMatrixBuilding *)ms)->m_BusyFlag.IsBusy()){
//                        D3DXVECTOR2 f_pos = ((CMatrixBuilding *)ms)->m_Pos;
//                        D3DXVECTOR2 my_pos = D3DXVECTOR2(groups->GetGroupPos().x, groups->GetGroupPos().y);
//                        float a = D3DXVec2LengthSq(&(f_pos - my_pos));
//                        if(a < nearest_l || nearest == nullptr){
//                            nearest_l   = a;
//                            nearest     = ms;
//                        }
//                    }
//                }
//                ms = ms->GetNextLogic();
//            }
//            if(!nearest){
//                ms = CMatrixMapStatic::GetFirstLogic();
//                nearest_l = 0;
//                nearest = nullptr;
//                while(ms) {
//		            if(ms->GetObjectType()==OBJECT_TYPE_BUILDING && ((CMatrixBuilding *)ms)->m_Side != m_Id && ((CMatrixBuilding*)ms)->IsBase()) {
//                        if(!((CMatrixBuilding*)ms)->m_BusyFlag.IsBusy() /*|| ((((CMatrixBuilding*)ms)->m_BusyFlag.IsBusy() && ((CMatrixBuilding*)ms)->m_BusyFlag.GetBusyBy() && ((CMatrixBuilding*)ms)->m_BusyFlag.GetBusyBy()->m_Side != m_Id)) */){
//                            D3DXVECTOR2 f_pos = ((CMatrixBuilding *)ms)->m_Pos;
//                            D3DXVECTOR2 my_pos = D3DXVECTOR2(groups->GetGroupPos().x, groups->GetGroupPos().y);
//                            float a = D3DXVec2LengthSq(&(f_pos - my_pos));
//                            if(a < nearest_l || nearest == nullptr){
//                                nearest_l   = a;
//                                nearest     = ms;
//                            }
//                        }
//                    }
//                    ms = ms->GetNextLogic();
//                }
//            }
//            if(nearest){
//                groups->InstallTactics(CAPTURE_TACTICS, m_TacticsPar);
//                groups->GetTactics()->InitialiseTactics(groups, nearest, -1);
//            }
//        }else if(groups->GetGroupId() > 1){
//            //Join to the main group
//            
//            CMatrixMapStatic* target = nullptr;
//            CMatrixGroup* main_grp = GetGroup(1, groups->GetTeam());
//            if(main_grp && main_grp->GetTactics())
//                target = main_grp->GetTactics()->GetTarget();
//            
//            if(target && ((!((CMatrixBuilding*)target)->m_BusyFlag.IsBusy()) || (((CMatrixBuilding*)target)->m_BusyFlag.IsBusy() && ((CMatrixBuilding*)target)->m_BusyFlag.GetBusyBy() && ((CMatrixBuilding*)target)->m_BusyFlag.GetBusyBy()->m_Side != m_Id))){
//                groups->InstallTactics(CAPTURE_TACTICS, m_TacticsPar);
//                groups->GetTactics()->InitialiseTactics(groups, target, -1);
//                
//            }else{
//                groups->InstallTactics(JOIN_TACTICS, m_TacticsPar);
//                if(groups->GetTactics() != nullptr){
//                    groups->GetTactics()->InitialiseTactics(groups, nullptr, -1);
//                }
//            }
//
//        }
//        groups = groups->m_NextGroup;
//    }
//}

//CMatrixGroup* CMatrixSideUnit::GetGroup(int id, int t)
//{
//    CMatrixGroup* grps = m_GroupsList->m_FirstGroup, *main_grp = nullptr;
//    while(grps)
//    {
//        if(grps->GetGroupId() == id && grps->GetTeam() == t)
//        {
//            main_grp = grps;
//            break;
//        }
//
//        grps = grps->m_NextGroup;
//    }
//
//    return main_grp;
//}

void SCannonForBuild::Delete()
{
    if(m_Cannon)
    {
        HDelete(CMatrixTurret, m_Cannon, Base::g_MatrixHeap);
        m_Cannon = nullptr;
        m_ParentBuilding = nullptr;
        m_ParentSpot.Release();
    }
}

bool CMatrixSideUnit::IsUnitUnderManualControlRobot()
{
    return (m_UnitUnderManualControl && m_UnitUnderManualControl->GetObjectType() == OBJECT_TYPE_ROBOT_AI);
}

bool CMatrixSideUnit::IsUnitUnderManualControlFlyer()
{
    return (m_UnitUnderManualControl && m_UnitUnderManualControl->GetObjectType() == OBJECT_TYPE_FLYER);
}

//Функция перевода и вывода юнита в режим ручного управления
void CMatrixSideUnit::SetManualControledUnit(CMatrixMapStatic* cur_unit)
{
    if(cur_unit == nullptr)
    {
        m_UnitUnderManualControl_available = 0;
    }

    if(cur_unit && cur_unit->GetSide() != PLAYER_SIDE)
    {
        return;
    }

    if(IsUnitUnderManualControlRobot())
    {
        CMatrixRobotAI* robot = m_UnitUnderManualControl->AsRobot();
        if(robot->m_SoundChassis)
        {
            CSound::StopPlay(robot->m_SoundChassis);
            robot->m_SoundChassis = SOUND_ID_EMPTY;
        }
    }

    //Выдаём роботу приказ "Stop" при выходе из аркадного режима
    if(IsUnitUnderManualControlRobot() && !m_UnitUnderManualControl->IsDIP())
    {
        CMatrixRobotAI* robot = m_UnitUnderManualControl->AsRobot();
        //SETFLAG(g_MatrixMap->m_Flags, MMFLAG_SOUND_ORDER_ATTACK_DISABLE);
        robot->MapPosCalc();
        //g_MatrixMap->GetSideById(robot->GetSide())->PGOrderAttack(g_MatrixMap->GetSideById(robot->GetSide())->RobotToLogicGroup(robot), CPoint(robot->GetMapPosX(), robot->GetMapPosY()), nullptr);
        g_MatrixMap->GetSideById(robot->GetSide())->PGOrderStop(g_MatrixMap->GetSideById(robot->GetSide())->RobotToLogicGroup(robot));
        //RESETFLAG(g_MatrixMap->m_Flags, MMFLAG_SOUND_ORDER_ATTACK_DISABLE);
    }
    //else if(IsUnitUnderManualControlFlyer() && !m_UnitUnderManualControl->IsDIP()) {}

    //Отключение аркадного режима, очистка аркадной переменной и снятие аркадных бонусов с юнита
    if(IsUnitUnderManualControlRobot() && (cur_unit == nullptr || cur_unit == m_UnitUnderManualControl))
    {
        CMatrixRobotAI* robot = m_UnitUnderManualControl->AsRobot();
        robot->SetMaxSpeed(robot->GetMaxSpeed() / g_UnitSpeedArcadeCoef);
        robot->SetMaxStrafeSpeed(robot->GetMaxStrafeSpeed() / g_UnitSpeedArcadeCoef);
        cur_unit = nullptr;
        robot->UnSelect();
        if(g_IFaceList) g_IFaceList->DeleteWeaponDynamicStatics();
        Select(NOTHING);
    }
    else if(IsUnitUnderManualControlRobot() && cur_unit->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
    {
        CMatrixRobotAI* robot = m_UnitUnderManualControl->AsRobot();
        if(g_IFaceList) g_IFaceList->DeleteWeaponDynamicStatics();
        robot->UnSelect();
        robot->SetMaxSpeed(robot->GetMaxSpeed() / g_UnitSpeedArcadeCoef);
        robot->SetMaxStrafeSpeed(robot->GetMaxStrafeSpeed() / g_UnitSpeedArcadeCoef);
        robot->SetWeaponToDefaultCoeff();
    }
    else if(IsUnitUnderManualControlRobot() && cur_unit->GetObjectType() == OBJECT_TYPE_FLYER)
    {
        CMatrixRobotAI* robot = m_UnitUnderManualControl->AsRobot();
        if(g_IFaceList) g_IFaceList->DeleteWeaponDynamicStatics();
        robot->UnSelect();
        robot->SetMaxSpeed(robot->GetMaxSpeed() / g_UnitSpeedArcadeCoef);
        robot->SetMaxStrafeSpeed(robot->GetMaxStrafeSpeed() / g_UnitSpeedArcadeCoef);
    }

    m_UnitUnderManualControl = cur_unit;
    if(IsUnitUnderManualControlRobot())
    {
        CMatrixRobotAI* robot = cur_unit->AsRobot();
        robot->m_SoundChassis = CSound::Play(robot->m_SoundChassis, g_Config.m_RobotChassisConsts[robot->m_Module[0].m_Kind].arcade_enter_sound_num, robot->GetGeoCenter(), SL_CHASSIS);

        robot->SetMaxSpeed(robot->GetMaxSpeed() * g_UnitSpeedArcadeCoef);
        robot->SetMaxStrafeSpeed(robot->GetMaxStrafeSpeed() * g_UnitSpeedArcadeCoef);
        robot->SetWeaponToArcadedCoeff();
        Select(ARCADE, cur_unit);
        robot->SelectArcade();

        if(g_IFaceList) g_IFaceList->CreateWeaponDynamicStatics();
        robot->BreakAllOrders();
    }
}

void CMatrixSideUnit::ResetSelection()
{
    if(GetCurGroup())
    {
        GetCurGroup()->RemoveAll();
        SetCurGroup(nullptr);
    }
}

void CMatrixSideUnit::ResetSystemSelection()
{
    if(m_CurSelGroup)
    {
        m_CurSelGroup->RemoveAll();
    }
}

void CMatrixSideUnit::SelectedGroupUnselect()
{
    if(!GetCurGroup()) return;

    CMatrixGroupObject* objs = GetCurGroup()->m_FirstObject;
    while(objs)
    {
        if(objs->ReturnObject()->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
        {
            ((CMatrixRobotAI*)objs->ReturnObject())->UnSelect();
        }
        else if(objs->ReturnObject()->GetObjectType() == OBJECT_TYPE_FLYER)
        {
            ((CMatrixFlyer*)objs->ReturnObject())->UnSelect();
        }

        objs = objs->m_NextObject;
    }

    SetCurGroup(nullptr);
}

void CMatrixSideUnit::GroupsUnselectSoft()
{
    CMatrixGroup* grps = m_FirstGroup;
    while(grps)
    {
        CMatrixGroupObject* objs = grps->m_FirstObject;
        while(objs)
        {
            CMatrixMapStatic* unit = objs->ReturnObject();

            if(unit->GetObjectType() == OBJECT_TYPE_ROBOT_AI) unit->AsRobot()->UnSelect();
            else if(unit->GetObjectType() == OBJECT_TYPE_FLYER) unit->AsFlyer()->UnSelect();

            objs = objs->m_NextObject;
        }

        grps = grps->m_NextGroup;
    }
}

//Отменяем вообще все приказы
void CMatrixSideUnit::SelectedGroupBreakAllOrders()
{
    CMatrixGroupObject* objs = GetCurGroup()->m_FirstObject;
    while(objs)
    {
        if(objs->ReturnObject()->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
        {
            objs->ReturnObject()->AsRobot()->BreakAllOrders();
        }
        //else if(objs->ReturnObject()->GetObjectType() == OBJECT_TYPE_FLYER) {}

        objs = objs->m_NextObject;
    }
}

//Отменяем все приказы, связанные с движением, не затрагивая приказы на стрельбу
void CMatrixSideUnit::SelectedGroupBreakMoveOrders()
{
    CMatrixGroupObject* objs = GetCurGroup()->m_FirstObject;
    while(objs)
    {
        if(objs->ReturnObject()->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
        {
            objs->ReturnObject()->AsRobot()->BreakAllMoveOrders();
        }
        //else if(objs->ReturnObject()->GetObjectType() == OBJECT_TYPE_FLYER) {}

        objs = objs->m_NextObject;
    }
}

void CMatrixSideUnit::SetCurSelNum(int i)
{
    g_IFaceList->DeletePersonalImage();

    if(i >= 0) m_CurSelNum = i;
    else
    {
        if(GetCurGroup() && m_CurSelNum >= GetCurGroup()->GetObjectsCnt())
        {
            --m_CurSelNum;
        }
    }

    g_IFaceList->CreatePersonalImage();
}

CMatrixMapStatic* CMatrixSideUnit::GetCurSelObject()
{
    if(!GetCurGroup())
    {
        if(m_CurrSel == BASE_SELECTED || m_CurrSel == BUILDING_SELECTED) return m_ActiveObject;
        else return nullptr;
    }

    CMatrixGroupObject* go = GetCurGroup()->m_FirstObject;
    for(int i = 0; i < m_CurSelNum && go; ++i) go = go->m_NextObject;

    if(go) return go->ReturnObject();
    return nullptr;
}

CMatrixGroup* CMatrixSideUnit::CreateGroupFromCurrent()
{
    CMatrixGroup* new_group = HNew(Base::g_MatrixHeap) CMatrixGroup;

    CMatrixGroupObject* go = m_CurSelGroup->m_FirstObject;
    while(go)
    {
        CMatrixMapStatic* obj = go->ReturnObject();

        new_group->AddObject(obj, -4);
        CMatrixGroup* grps = m_FirstGroup;
        while(grps)
        {
            grps->RemoveObject(obj);
            //if(grps->m_Tactics) grps->m_Tactics->RemoveObjectFromT(obj);
            grps = grps->m_NextGroup;
        }

        go = go->m_NextObject;
    }

    LIST_ADD(new_group, m_FirstGroup, m_LastGroup, m_PrevGroup, m_NextGroup);
    m_CurSelGroup->RemoveAll();

    SetCurGroup(new_group);
    Reselect();

    return new_group;
}

void CMatrixSideUnit::CreateGroupFromCurrent(CMatrixMapStatic* obj)
{
    CMatrixGroup* new_group = HNew(Base::g_MatrixHeap) CMatrixGroup;

    new_group->AddObject(obj, -4);
    CMatrixGroup* grps = m_FirstGroup;
    while(grps)
    {
        grps->RemoveObject(obj);
        /*
        if(grps->m_Tactics)
        {
            grps->m_Tactics->RemoveObjectFromT(obj);
        }
        */
        grps = grps->m_NextGroup;
    }

    LIST_ADD(new_group, m_FirstGroup, m_LastGroup, m_PrevGroup, m_NextGroup);
    m_CurSelGroup->RemoveAll();
    SetCurGroup(new_group);
    Reselect();
}

void CMatrixSideUnit::AddToCurrentGroup()
{
    if(GetCurGroup())
    {
        CMatrixGroupObject* go = m_CurSelGroup->m_FirstObject;
        while(go)
        {
            if(GetCurGroup()->FindObject(go->ReturnObject()))
            {
                //???
            }
            else
            {
                if(GetCurGroup()->GetObjectsCnt() < 9) GetCurGroup()->AddObject(go->ReturnObject(), -4);
            }
            go = go->m_NextObject;
        }

        m_CurSelGroup->RemoveAll();
        Reselect();
    }
}
void CMatrixSideUnit::PumpGroups()
{
    CMatrixGroup* grps = m_FirstGroup;

    while(grps)
    {
        if(grps != GetCurGroup())
        {
            CMatrixGroup* nxtgrp = grps->m_NextGroup;
            LIST_DEL(grps, m_FirstGroup, m_LastGroup, m_PrevGroup, m_NextGroup);
            HDelete(CMatrixGroup, grps, Base::g_MatrixHeap);
            grps = nxtgrp;
            continue;

            /*
            if(grps->m_Tactics == nullptr)
            {
                CMatrixGroupObject *gos = grps->m_FirstObject;
                bool canbefree = true;

                while(gos)
                {
                    if(gos->ReturnObject() && gos->ReturnObject()->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
                    {
                        if(((CMatrixRobotAI*)gos->ReturnObject())->GetOrdersInPool())
                        {
                            canbefree = false;
                            break;
                        }
                    }
                    else if(gos->ReturnObject() && gos->ReturnObject()->GetObjectType() == OBJECT_TYPE_FLYER)
                    {}

                    gos = gos->m_NextObject;
                }
                if(canbefree)
                {
                    CMatrixGroup *nxtgrp = grps->m_NextGroup;
                    LIST_DEL(grps, m_FirstGroup, m_LastGroup, m_PrevGroup, m_NextGroup);
                    HDelete(CMatrixGroup, grps, g_MatrixHeap);
                    grps = nxtgrp;
                    continue;
                }
            }
            */
        }
        grps = grps->m_NextGroup;
    }
}

void CMatrixSideUnit::RemoveObjectFromSelectedGroup(CMatrixMapStatic* o)
{
    if(o && GetCurGroup())
    {
        if(o->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
        {
            ((CMatrixRobotAI*)o)->DeleteHealthBarClone(PBC_CLONE1);
            ((CMatrixRobotAI*)o)->DeleteHealthBarClone(PBC_CLONE2);
            ((CMatrixRobotAI*)o)->UnSelect();
        }
        else if(o->GetObjectType() == OBJECT_TYPE_FLYER)
        {
            ((CMatrixFlyer*)o)->DeleteHealthBarClone(PBC_CLONE1);
            ((CMatrixFlyer*)o)->DeleteHealthBarClone(PBC_CLONE2);
            ((CMatrixFlyer*)o)->UnSelect();
        }

        GetCurGroup()->RemoveObject(o);
        m_CurSelGroup->RemoveObject(o);

        if(g_IFaceList)
        {
            g_IFaceList->DeleteProgressBars(o);
            g_IFaceList->CreateGroupIcons();
            SetCurSelNum(-1);
        }

        Reselect();
    }
}

void CMatrixSideUnit::RemoveFromSelection(CMatrixMapStatic* o)
{
    if(CMultiSelection::m_GameSelection)
    {
        CMultiSelection::m_GameSelection->Remove(o);
    }
}

bool CMatrixSideUnit::FindObjectInSelection(CMatrixMapStatic* o)
{
    if(GetCurGroup() && GetCurGroup()->FindObject(o))
    {
        return true;
    }

    if(CMultiSelection::m_GameSelection && CMultiSelection::m_GameSelection->FindItem(o))
    {
        return true;
    }

    return false;
}

void CMatrixSideUnit::InitPlayerSide()
{
    m_ConstructPanel = HNew(Base::g_MatrixHeap) CConstructorPanel;
    m_CurSelGroup = HNew(Base::g_MatrixHeap) CMatrixGroup;
}

int CMatrixSideUnit::IsInPlaces(
    const CPoint* places,
    int placescnt,
    int x,
    int y
)
{
    const CPoint* p_tmp = places;
    for(int i = 0; i < placescnt; ++i, ++p_tmp)
    {
        int dx = (x - p_tmp->x);
        int dy = (y - p_tmp->y);
        int rr = dx * dx + dy * dy;
        if(rr < 4)
        {
            return i;
        }
    }

    return -1;
}

int CMatrixSideUnit::GetMaxSideRobots()
{
    CMatrixMapStatic* os = CMatrixMapStatic::GetFirstLogic();
    int bases = 0, factories = 0;
    while(os)
    {
        if(os->GetSide() == m_Id && os->GetObjectType() == OBJECT_TYPE_BUILDING)
        {
            if(((CMatrixBuilding*)os)->IsBase()) ++bases;
            else ++factories;
        }
        os = os->GetNextLogic();
    }

    return (bases * ROBOTS_BY_BASE) + (bases == 0 ? 0 : ROBOTS_BY_MAIN) + factories/* *ROBOT_BY_FACTORY */;
}

int CMatrixSideUnit::GetRobotsInQueue()
{
    CMatrixMapStatic* os = CMatrixMapStatic::GetFirstLogic();
    int robots = 0;
    while(os)
    {
        if(os->GetSide() == m_Id && os->GetObjectType() == OBJECT_TYPE_BUILDING && ((CMatrixBuilding*)os)->IsBase())
        {
            robots += ((CMatrixBuilding*)os)->GetRobotsBuildingQueueCount();
        }
        os = os->GetNextLogic();
    }

    return robots;
}

void CMatrixSideUnit::PLDropAllActions()
{
    g_IFaceList->ExitManualControlMode();
    m_CurrentAction = NOTHING_SPECIAL;
    m_CannonForBuild.Delete();
    g_IFaceList->ResetOrderingMode();
    if(m_ActiveObject && m_ActiveObject->IsBuilding())
    {
        m_ActiveObject->AsBuilding()->m_BuildingQueue.KillBar();
    }

    Select(NOTHING);
    RESETFLAG(g_IFaceList->m_IfListFlags, POPUP_MENU_ACTIVE);
    m_ConstructPanel->ResetGroupNClose();
    g_MatrixMap->m_Cursor.SetVisible(true);
}

void CMatrixSideUnit::SetCurGroup(CMatrixGroup* group)
{
    m_CurrentGroup = group;
    if(group == nullptr)
    {
        //_asm int 3
        g_IFaceList->ResetOrderingMode();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SideSelectionCallBack(CMatrixMapStatic* ms, dword param)
{
    if(!ms || ms->GetSide() != PLAYER_SIDE) return;
    if(ms->GetObjectType() != OBJECT_TYPE_ROBOT_AI && ms->GetObjectType() != OBJECT_TYPE_FLYER && ms->GetObjectType() != OBJECT_TYPE_BUILDING)
    {
        //Турели отсортировываются отдельной опцией g_SelectableTurrets
        if(ms->GetObjectType() == OBJECT_TYPE_TURRET && !g_SelectableTurrets) return;
    }

    if(ms->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
    {
        if(ms->AsRobot()->m_CurrentState == ROBOT_DIP) return;
    }
    else if(ms->GetObjectType() == OBJECT_TYPE_BUILDING)
    {
        if(ms->AsBuilding()->State() == BUILDING_DIP || ms->AsBuilding()->State() == BUILDING_DIP_EXPLODED) return;
    }
    else if(ms->GetObjectType() == OBJECT_TYPE_TURRET)
    {
        if(ms->AsTurret()->m_CurrentState == TURRET_UNDER_CONSTRUCTION || ms->AsTurret()->m_CurrentState == TURRET_UNDER_DECONSTRUCTION || ms->AsTurret()->m_CurrentState == TURRET_DIP) return;
    }

    CMatrixSideUnit* my_side = g_MatrixMap->GetPlayerSide();
    CMatrixGroup* cursel = my_side->GetCurSelGroup();

    static CPoint prev_mp(0, 0);

    SCallback* cbs = (SCallback*)param;

    CPoint mp = cbs->mp;
    ++cbs->calls;

    if(mp.x == -1 && mp.y == -1)
    {
        //end
        if(prev_mp.x != mp.x || prev_mp.y != mp.y)
        {
            cursel->RemoveAll();
        }
        if((cursel->GetRobotsCnt() + cursel->GetFlyersCnt()) == 9) return;

        cursel->AddObject(ms, -4);
        prev_mp = mp;
    }
    else
    {
        //Update
        /*
        if(ms->GetObjectType() == OBJECT_TYPE_ROBOT_AI && !((CMatrixRobotAI*)ms)->IsSelected())
        {
            ((CMatrixRobotAI*)ms)->SelectByGroup();
            ((CMatrixRobotAI*)ms)->GetSelection()->SetColor(SEL_COLOR_TMP);
        }
        else if(ms->GetObjectType() == OBJECT_TYPE_FLYER && !((CMatrixFlyer*)ms)->IsSelected())
        {
            ((CMatrixFlyer*)ms)->SelectByGroup();
            ((CMatrixFlyer*)ms)->GetSelection()->SetColor(SEL_COLOR_TMP);
        }
        */
        cursel->AddObject(ms, -4);
        /*
        if(tmp1 && tmp2 && 0)
        {
            if(prev_mp.x != mp.x || prev_mp.y != mp.y)
            {
                static int calles3;
                ++calles3;
                CDText::T("ccc", CStr(calles3));

                CMatrixGroupObject *o2 = tmp2->m_FirstObject;
                while(o2)
                {
                    if(!tmp1->FindObject(o2->ReturnObject()) && !cursel->FindObject(o2->ReturnObject()) && ((!my_side->m_CurGroup) || (my_side->m_CurGroup && !my_side->m_CurGroup->FindObject(o2->ReturnObject()))))
                    {
                        if(o2->ReturnObject() && o2->ReturnObject()->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
                        {
                            static int calles2;
                            ++calles2;
                            CDText::T("bbb", CStr(calles2));

                            ((CMatrixRobotAI*)o2->ReturnObject())->UnSelect();
                        }
                        else if(ms->GetObjectType() == OBJECT_TYPE_FLYER)
                        {
                            ((CMatrixFlyer*)o2->ReturnObject())->UnSelect();
                        }
                    }
                    o2 = o2->m_NextObject;
                }
                tmp2->RemoveAll();
                CMatrixGroupObject *o1 = tmp1->m_FirstObject;
                while(o1)
                {
                    tmp2->AddObject(o1->ReturnObject(), -4);
                    if(o1->ReturnObject() && o1->ReturnObject()->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
                    {
                        if(!((CMatrixRobotAI*)o1->ReturnObject())->IsSelected())
                        {
                            static int calles;
                            ++calles;
                            CDText::T("aaa", CStr(calles));
                            ((CMatrixRobotAI*)o1->ReturnObject())->SelectByGroup();
                            ((CMatrixRobotAI*)o1->ReturnObject())->GetSelection()->SetColor(SEL_COLOR_TMP);

                        }
                    }
                    else if(ms->GetObjectType() == OBJECT_TYPE_FLYER)
                    {
                        if(!((CMatrixFlyer*)o1->ReturnObject())->IsSelected())
                        {
                            ((CMatrixFlyer*)o1->ReturnObject())->SelectByGroup();
                            ((CMatrixFlyer*)o1->ReturnObject())->m_Selection->SetColor(SEL_COLOR_TMP);
                        }
                    }

                    o1 = o1->m_NextObject;
                }
                tmp1->RemoveAll();
            }
            tmp1->AddObject(ms, -4);
        }
        */
        prev_mp = mp;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CMatrixSideUnit::CalcStrength()
{
    int c_base = 0;
    int c_building = 0;
    float s_cannon = 0;
    float s_robot = 0;

    CMatrixMapStatic* obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->GetSide() == m_Id)
        {
            if(obj->IsBuildingAlive())
            {
                if(obj->AsBuilding()->IsBase()) ++c_base;
                else ++c_building;
            }
            else if(obj->IsActiveTurretAlive()) s_cannon += obj->AsTurret()->GetStrength();
            else if(obj->IsRobotAlive()) s_robot += obj->AsRobot()->GetStrength();
        }
        obj = obj->GetNextLogic();
    }

    int res = 0;
    for(int r = 0; r < MAX_RESOURCES; ++r) res += min(1000, m_Resources[r]);
    res /= MAX_RESOURCES;

    m_Strength = 5.0f * c_base
        + 1.0f * c_building
        + s_cannon / 2000.0f
        + s_robot / 1000.0f
        + float(res) / 100.0f;

    m_Strength *= m_StrengthMultiplier;
}

void CMatrixSideUnit::Regroup()
{
    CMatrixMapStatic* obj;
    int i, u, t, k;
    D3DXVECTOR2 v1, v2;

    CMatrixRobotAI* rl[MAX_ROBOTS_ON_MAP]; // Список роботов на карте
    int subl[MAX_ROBOTS_ON_MAP];           // В какую подгруппу входит робот
    int ally_robots_cnt = 0;                  // Кол-во роботов на карте
    int subcnt = 0;                 // Кол-во подгрупп

    int tl[MAX_ROBOTS_ON_MAP];
    int tlcnt = 0, tlsme = 0;

    for(i = 0; i < MAX_ROBOTS_ON_MAP; ++i) subl[i] = -1;
    for(i = 0; i < MAX_LOGIC_GROUP; ++i) m_LogicGroup[i].RobotsCnt(0);

    // Запоминаем всех роботов
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->GetObjectType() == OBJECT_TYPE_ROBOT_AI && obj->GetSide() == m_Id)
        {
            rl[ally_robots_cnt] = (CMatrixRobotAI*)obj;
            if(rl[ally_robots_cnt]->GetGroupLogic() >= 0 && rl[ally_robots_cnt]->GetGroupLogic() < MAX_LOGIC_GROUP)
            {
                m_LogicGroup[rl[ally_robots_cnt]->GetGroupLogic()].IncRobotsCnt();
                m_LogicGroup[rl[ally_robots_cnt]->GetGroupLogic()].m_Team = rl[ally_robots_cnt]->GetTeam();
            }

            if(rl[ally_robots_cnt]->GetTeam() >= 0) ++ally_robots_cnt;
            ASSERT(ally_robots_cnt <= MAX_ROBOTS_ON_MAP);
        }
        obj = obj->GetNextLogic();
    }

    //Делим на подгруппы для разделения. Растояние между роботами должно быть не более 400
    for(u = 0; u < ally_robots_cnt; ++u)
    {
        if(subl[u] >= 0) continue;

        tl[0] = u;
        tlcnt = 1;
        tlsme = 0;
        ++subcnt;
        subl[u] = subcnt - 1;

        while(tlsme < tlcnt)
        {
            v1.x = rl[tl[tlsme]]->m_PosX;
            v1.y = rl[tl[tlsme]]->m_PosY;

            for(i = u + 1; i < ally_robots_cnt; ++i)
            {
                if(subl[i] >= 0) continue;
                if(tl[tlsme] == i) continue;
                if(rl[tl[tlsme]]->GetTeam() != rl[i]->GetTeam()) continue;
                if(rl[tl[tlsme]]->GetGroupLogic() != rl[i]->GetGroupLogic()) continue;
                v2.x = rl[i]->m_PosX; v2.y = rl[i]->m_PosY;
                if(((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y)) > POW2(400.0f)) continue;

                tl[tlcnt] = i;
                ++tlcnt;
                subl[i] = subcnt - 1;
            }
            ++tlsme;
        }
    }

    //Разделяем
    for(u = 0; u < MAX_LOGIC_GROUP; ++u)
    {
        if(m_LogicGroup[u].RobotsCnt() < 0) continue;

        tlcnt = 0;
        for(i = 0; i < ally_robots_cnt; ++i)
        {
            if(rl[i]->GetGroupLogic() != u) continue;
            for(t = 0; t < tlcnt; ++t)
            {
                if(tl[t] == subl[i]) break;
            }

            if(t >= tlcnt)
            {
                tl[tlcnt] = subl[i];
                ++tlcnt;
            }
        }

        for(i = 1; i < tlcnt; ++i)
        {
            // Ищем пустую группу
            for(k = 0; k < MAX_LOGIC_GROUP; ++k)
            {
                if(m_LogicGroup[k].RobotsCnt() <= 0) break;
            }

            if(k >= MAX_LOGIC_GROUP) break;

            //Копируем группу
            for(t = 0; t < ally_robots_cnt; ++t)
            {
                if(rl[t]->GetGroupLogic() != u) continue;
                if(subl[t] == tl[0])
                {
                    m_LogicGroup[k] = m_LogicGroup[rl[t]->GetGroupLogic()];
                    m_LogicGroup[k].RobotsCnt(0);
                    break;
                }
            }

            if(t >= ally_robots_cnt) break;

            //Переносим роботов в новую группу
            for(t = 0; t < ally_robots_cnt; ++t)
            {
                if(rl[t]->GetGroupLogic() != u) continue;
                if(subl[t] != tl[i]) continue;
                m_LogicGroup[rl[t]->GetGroupLogic()].IncRobotsCnt();
                rl[t]->SetGroupLogic(k);
                m_LogicGroup[k].IncRobotsCnt();
            }
        }

        if(i < tlcnt) break;
    }

    //Если робот не в группе, то создаем для него новую группу
    for(t = 0; t < ally_robots_cnt; ++t)
    {
        if(rl[t]->GetGroupLogic() >= 0 && rl[t]->GetGroupLogic() < MAX_LOGIC_GROUP) continue;

        for(i = 0; i < MAX_LOGIC_GROUP; ++i)
        {
            if(m_LogicGroup[i].RobotsCnt() <= 0) break;
        }

        if(i >= MAX_LOGIC_GROUP) break;

        ZeroMemory(m_LogicGroup + i, sizeof(SMatrixLogicGroup));
        m_LogicGroup[i].m_Team = rl[t]->GetTeam();
        m_LogicGroup[i].RobotsCnt(1);
        m_LogicGroup[i].m_Action.m_Type = mlat_None;
        rl[t]->SetGroupLogic(i);

    }

    //Объеденяем группы если растояние между ними меньше 300
    for(u = 0; u < MAX_LOGIC_GROUP; ++u)
    {
        if(m_LogicGroup[u].RobotsCnt() <= 0) continue;
        if(m_LogicGroup[u].m_Team < 0) continue;

        for(t = u + 1; t < MAX_LOGIC_GROUP; ++t)
        {
            if(m_LogicGroup[t].RobotsCnt() <= 0) continue;
            if(m_LogicGroup[t].m_Team < 0) continue;

            if(m_LogicGroup[u].m_Team != m_LogicGroup[t].m_Team) continue;

            //Проверяем
            for(i = 0; i < ally_robots_cnt; ++i)
            {
                if(rl[i]->GetGroupLogic() != u) continue;

                for(k = 0; k < ally_robots_cnt; ++k)
                {
                    if(rl[k]->GetGroupLogic() != t) continue;

                    float d = (rl[i]->m_PosX - rl[k]->m_PosX) * (rl[i]->m_PosX - rl[k]->m_PosX) + (rl[i]->m_PosY - rl[k]->m_PosY) * (rl[i]->m_PosY - rl[k]->m_PosY);
                    if(d < POW2(300.0f)) break;
                }

                if(k < ally_robots_cnt) break;
            }

            //Объеденяем
            if(i < ally_robots_cnt)
            {
                if(m_LogicGroup[u].m_Action.m_Type == mlat_None) // Выбираем лучшую для которой приказ останется
                {
                    SMatrixLogicGroup temp = m_LogicGroup[u]; m_LogicGroup[u] = m_LogicGroup[t]; m_LogicGroup[t] = temp;
                }

                m_LogicGroup[u].IncRobotsCnt(m_LogicGroup[t].RobotsCnt());

                for(k = 0; k < ally_robots_cnt; ++k)
                {
                    if(rl[k]->GetGroupLogic() != t) continue;
                    rl[k]->SetGroupLogic(u);
                }
            }
        }
    }

#if(defined _DEBUG) && !(defined _RELDEBUG) && !(defined _DISABLE_AI_HELPERS)
    // Отображаем хелперы
    dword colors[10] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffff00, 0xffff00ff, 0xff00ffff, 0xff800000, 0xff008000, 0xff000080, 0xff808000 };
    //    CHelper::DestroyByGroup(101);
    //if(m_Id==PLAYER_SIDE)
    for(i = 0; i < ally_robots_cnt; ++i)
    {
        float z = g_MatrixMap->GetZ(rl[i]->m_PosX, rl[i]->m_PosY);
        /*team*/        CHelper::Create(1)->Cone(D3DXVECTOR3(rl[i]->m_PosX, rl[i]->m_PosY, z + 10.0f), D3DXVECTOR3(rl[i]->m_PosX, rl[i]->m_PosY, z + 80.0f), 1.0f, 1.0f, colors[rl[i]->GetTeam() % 10], colors[rl[i]->GetTeam() % 10], 3);
        /*group*/       CHelper::Create(1)->Cone(D3DXVECTOR3(rl[i]->m_PosX, rl[i]->m_PosY, z + 80.0f), D3DXVECTOR3(rl[i]->m_PosX, rl[i]->m_PosY, z + 100.0f), 1.0f, 1.0f, colors[rl[i]->GetGroupLogic() % 10], colors[rl[i]->GetGroupLogic() % 10], 3);
    }
#endif
}

void CMatrixSideUnit::ClearTeam(int team)
{
DTRACE();

    m_Team[team].m_TargetRegion = -1;
    m_Team[team].m_Action.m_Type = mlat_None;
    m_Team[team].m_Action.m_RegionPathCnt = 0;
    m_Team[team].SetWar(false);
    m_Team[team].m_Brave = 0;
    m_Team[team].m_BraveStrengthCancel = 0;
    m_Team[team].SetRegroupOnlyAfterWar(false);
    m_Team[team].SetWaitUnion(false);
    m_Team[team].m_WaitUnionLast = 0;
}

int CMatrixSideUnit::ClacSpawnTeam(int region, int nsh)
{
DTRACE();

    for (int i = 0; i < m_TeamCnt; ++i)
    {
        if (m_Team[i].m_RobotCnt <= 0)
        {
            ClearTeam(i);
            return i;
        }
    }

    if (!m_Region) return 0;

    for (int ct = 0; ct <= 2; ++ct)
    {
        int u;
        int cnt = 0; int sme = 0;

        for (int i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i) m_Region[i].m_Data = 0;

        m_RegionIndex[cnt] = region;
        ++cnt;
        m_Region[region].m_Data = 1;

        int teamfind = -1;

        while (sme < cnt)
        {
            for (int i = 0; i < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++i)
            {
                u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[i];
                if (m_Region[u].m_Data) continue;

                if (g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearMove[i] & (1 << nsh)) continue;

                if (ct == 0 && m_Region[u].m_EnemyRobotCnt > 0) continue;

                if (m_Region[u].m_OurRobotCnt > 0)
                {
                    CMatrixMapStatic* ms = CMatrixMapStatic::GetFirstLogic();
                    while (ms)
                    {
                        if (ms->IsRobotAlive() && ms->GetSide() == m_Id && ms->AsRobot()->GetTeam() >= 0)
                        {
                            if (ct == 2 || ms->AsRobot()->GetEnv()->GetEnemyCnt() <= 0)
                            {
                                if (teamfind < 0) teamfind = ms->AsRobot()->GetTeam();
                                else
                                {
                                    if (m_Team[ms->AsRobot()->GetTeam()].m_RobotCnt < m_Team[teamfind].m_RobotCnt)
                                    {
                                        teamfind = ms->AsRobot()->GetTeam();
                                    }
                                }
                                //return ms->AsRobot()->GetTeam();
                            }
                        }
                        ms = ms->GetNextLogic();
                    }
                }

                m_RegionIndex[cnt] = u;
                ++cnt;
                m_Region[u].m_Data = 1;
            }
            ++sme;
        }

        if(teamfind >= 0) return teamfind;
    }
    return 0;
}

//Вынуждает робота бежать от враждебного робота
void CMatrixSideUnit::EscapeFromBomb()
{
    CMatrixRobotAI* rb[MAX_ROBOTS_ON_MAP * 3];
    float min_dist_enemy[MAX_ROBOTS_ON_MAP * 3];
    int rbcnt = 0;
    float escape_radius = 250.0f * 1.2f; //С запасом
    int escape_dist = Float2Int(escape_radius / GLOBAL_SCALE_MOVE) + 4;
    CPoint tp, tp2;
    int i;
    CEnemy* enemy;

    CMatrixMapStatic* ms = CMatrixMapStatic::GetFirstLogic();
    for(; ms; ms = ms->GetNextLogic())
    {
        if(!ms->IsRobotAlive()) continue;
        if(!ms->AsRobot()->HaveBomb()) continue;

        if(ms->AsRobot()->GetEnv()->GetEnemyCnt() <= 0) continue;

        ASSERT(rbcnt < MAX_ROBOTS_ON_MAP * 3);
        min_dist_enemy[rbcnt] = 1e20f;

        enemy = ms->AsRobot()->GetEnv()->m_FirstEnemy;
        for(; enemy; enemy = enemy->m_NextEnemy)
        {
            D3DXVECTOR2 temp = GetWorldPos(enemy->m_Enemy) - GetWorldPos(ms);
            min_dist_enemy[rbcnt] = min(min_dist_enemy[rbcnt], D3DXVec2LengthSq(&temp));
        }

        rb[rbcnt] = (CMatrixRobotAI*)ms;
        ++rbcnt;
    }

    if(rbcnt <= 0) return;

    CMatrixRobotAI* skip_normal = nullptr;
    CMatrixRobotAI* skip_withbomb = nullptr;

    float skip_normal_dist = 0.0f;
    float skip_withbomb_dist = 0.0f;

    ms = CMatrixMapStatic::GetFirstLogic();
    for(; ms; ms = ms->GetNextLogic())
    {
        if(!ms->IsRobotAlive()) continue;
        if(ms->GetSide() != m_Id) continue;
        CMatrixRobotAI* robot = ms->AsRobot();
        if(m_Id == PLAYER_SIDE && robot->GetGroupLogic() >= 0 && m_PlayerGroup[robot->GetGroupLogic()].Order() < mpo_AutoCapture) continue;

        float mdist = 1e20f;
        for(i = 0; i < rbcnt; ++i)
        {
            if(robot == rb[i]) continue;
            if(rb[i]->GetSide() == m_Id) continue;

            float rpx = robot->m_PosX;
            float rpy = robot->m_PosY;

            float dist = POW2(rpx - rb[i]->m_PosX) + POW2(rpy - rb[i]->m_PosY);
            if(dist < POW2(escape_radius))
            {
                if(dist < mdist) mdist = dist;
            }
        }

        if(mdist > 1e19f) continue;

        if(!robot->HaveBomb())
        {
            if(!skip_normal || mdist < skip_normal_dist)
            {
                skip_normal = robot;
                skip_normal_dist = mdist;
            }
        }
        else
        {
            if(!skip_withbomb || mdist < skip_withbomb_dist)
            {
                skip_withbomb = robot;
                skip_withbomb_dist = mdist;
            }
        }
    }

    if(skip_withbomb) skip_normal = nullptr;

    g_MatrixMap->PrepareBuf();
    int* ind = g_MatrixMap->m_ZoneIndex;
    dword* data = g_MatrixMap->m_ZoneDataZero;

    ms = CMatrixMapStatic::GetFirstLogic();
    for(; ms; ms = ms->GetNextLogic())
    {
        if(!ms->IsRobotAlive()) continue;
        if(ms->GetSide() != m_Id) continue;
        if(ms == skip_normal || ms == skip_withbomb) continue;
        CMatrixRobotAI* robot = ms->AsRobot();
        if(m_Id == PLAYER_SIDE && robot->GetGroupLogic() >= 0 && m_PlayerGroup[robot->GetGroupLogic()].Order() < mpo_AutoCapture) continue;

        if(robot->GetReturnCoords(tp)) continue;
        float rpx = robot->m_PosX;
        float rpy = robot->m_PosY;

        float mde = 1e20f;
        enemy = ms->AsRobot()->GetEnv()->m_FirstEnemy;
        for(; enemy; enemy = enemy->m_NextEnemy)
        {
            D3DXVECTOR2 temp = GetWorldPos(enemy->m_Enemy) - GetWorldPos(ms);
            mde = min(mde, D3DXVec2LengthSq(&temp));
        }

        for(i = 0; i < rbcnt; ++i)
        {
            if(robot == rb[i]) continue;
            if(rb[i]->GetSide() == m_Id && mde < min_dist_enemy[i]) continue; // Роботы впереди бомбы не отступают. Они прикрывают пока робот с бомбой дойдет до врага.
            float dist = POW2(rpx - rb[i]->m_PosX) + POW2(rpy - rb[i]->m_PosY);
            if(dist < POW2(escape_radius)) break;
        }

        if(i >= rbcnt) continue;

        int rp = g_MatrixMap->FindNearPlace(1 << (robot->m_Module[0].m_Kind - 1), CPoint(robot->GetMapPosX(), robot->GetMapPosY()));
        if(rp < 0) continue;

        int sme = 0;
        int cnt = 0;

        CMatrixMapStatic* ms2 = CMatrixMapStatic::GetFirstLogic();
        for(; ms2; ms2 = ms2->GetNextLogic())
        {
            if(robot != ms2 && ms2->IsRobotAlive())
            {
                if(ms2->AsRobot()->GetMoveToCoords(tp))
                {
                    int np = g_MatrixMap->FindPlace(tp);
                    if(np >= 0)
                    {
                        ind[cnt] = np;
                        data[np] = 2;
                        ++cnt;
                    }
                }

                if(ms2->AsRobot()->GetEnv()->m_Place >= 0)
                {
                    ind[cnt] = ms2->AsRobot()->GetEnv()->m_Place;
                    data[ind[cnt]] = 2;
                    ++cnt;
                }
            }
            else if(ms2->IsTurret())
            {
                ind[cnt] = ms2->AsTurret()->m_Place;
                data[ind[cnt]] = 2;
                ++cnt;
            }
        }

        sme = cnt;

        ind[cnt] = rp;
        data[rp] |= 1;
        ++cnt;

        while(sme < cnt)
        {
            rp = ind[sme];
            tp = g_MatrixMap->m_RoadNetwork.m_Place[rp].m_Pos;
            if(!(data[rp] & 2))
            {
                for(i = 0; i < rbcnt; ++i)
                {
                    if(robot == rb[i]) continue;
                    int dist = tp.Dist2(CPoint(rb[i]->GetMapPosX(), rb[i]->GetMapPosY()));
                    if(dist < POW2(escape_dist)) break;
                }
                if(i >= rbcnt)
                {
                    if(robot->GetMoveToCoords(tp2)) robot->MoveReturn(tp2.x, tp2.y);
                    else robot->MoveReturn(robot->GetMapPosX(), robot->GetMapPosY());
                    robot->MoveTo(tp.x, tp.y);
                    break;
                }
            }
            for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_Place[rp].m_NearCnt; ++i)
            {
                int np = g_MatrixMap->m_RoadNetwork.m_Place[rp].m_Near[i];
                if(data[np] & 1) continue;
                if(g_MatrixMap->m_RoadNetwork.m_Place[rp].m_NearMove[i] & (1 << (robot->m_Module[0].m_Kind - 1))) continue;

                ind[cnt] = np;
                data[np] |= 1;
                ++cnt;
            }

            ++sme;
        }

        for(i = 0; i < cnt; ++i) data[ind[i]] = 0;
    }
}

void CMatrixSideUnit::GroupNoTeamRobot()
{
    if(m_Id == PLAYER_SIDE) return;

    CMatrixRobotAI* rl[MAX_ROBOTS_ON_MAP];
    int il[MAX_ROBOTS_ON_MAP];
    int ally_robots_cnt = 0;
    CMatrixMapStatic* ms;
    int g, i, u, cnt, sme;
    float cx, cy;

    for(i = 0; i < MAX_LOGIC_GROUP; ++i) m_LogicGroup[i].RobotsCnt(0);

    ms = CMatrixMapStatic::GetFirstLogic();
    while(ms)
    {
        if(ms->IsRobot() && ms->GetSide() == m_Id)
        {
            CMatrixRobotAI* robot = ms->AsRobot();
            if(robot->GetGroupLogic() >= 0 && robot->GetGroupLogic() < MAX_LOGIC_GROUP)
            {
                m_LogicGroup[robot->GetGroupLogic()].IncRobotsCnt();
                m_LogicGroup[robot->GetGroupLogic()].m_Team = robot->GetTeam();
            }
        }
        ms = ms->GetNextLogic();
    }

    ms = CMatrixMapStatic::GetFirstLogic();
    for(; ms; ms = ms->GetNextLogic())
    {
        if(!ms->IsRobot()) continue;
        if(ms->GetSide() != m_Id) continue;
        if(ms->AsRobot()->GetTeam() >= 0) continue;

        ms->AsRobot()->SetGroupLogic(-1);

        ASSERT(ally_robots_cnt < MAX_ROBOTS_ON_MAP);
        rl[ally_robots_cnt] = (CMatrixRobotAI*)ms;
        ++ally_robots_cnt;
    }

    for(i = 0; i < ally_robots_cnt; ++i)
    {
        if(rl[i]->GetGroupLogic() >= 0) continue;

        for(g = 0; g < MAX_LOGIC_GROUP; ++g)
        {
            if(m_LogicGroup[g].RobotsCnt() <= 0) break;
        }
        ASSERT(g < MAX_LOGIC_GROUP);

        m_LogicGroup[g].m_Team = -1;
        m_LogicGroup[g].RobotsCnt(1);
        m_LogicGroup[g].m_Action.m_Type = mlat_Defence;
        m_LogicGroup[g].m_Action.m_RegionPathCnt = 0;
        m_LogicGroup[g].SetWar(false);

        rl[i]->SetGroupLogic(g);
        cx = rl[i]->m_PosX;
        cy = rl[i]->m_PosY;

        cnt = 0;
        sme = 0;
        il[cnt] = i;
        ++cnt;

        while(sme < cnt)
        {
            for(u = i + 1; u < ally_robots_cnt; ++u)
            {
                if(rl[i]->GetGroupLogic() >= 0) continue;
                if((POW2(rl[il[sme]]->m_PosX - rl[u]->m_PosX) + POW2(rl[il[sme]]->m_PosY - rl[u]->m_PosY)) < POW2(200))
                {
                    il[cnt] = u;
                    ++cnt;

                    m_LogicGroup[g].IncRobotsCnt();
                    rl[u]->SetGroupLogic(g);
                    cx += rl[u]->m_PosX;
                    cy += rl[u]->m_PosY;
                }
            }
            ++sme;
        }

        cx /= m_LogicGroup[g].RobotsCnt();
        cy /= m_LogicGroup[g].RobotsCnt();

        m_LogicGroup[g].m_Action.m_Region = g_MatrixMap->GetRegion(Float2Int(cx / GLOBAL_SCALE_MOVE), Float2Int(cy / GLOBAL_SCALE_MOVE));
        if(m_LogicGroup[g].m_Action.m_Region < 0) m_LogicGroup[g].m_Action.m_Region = g_MatrixMap->GetRegion(Float2Int(rl[i]->m_PosX / GLOBAL_SCALE_MOVE), Float2Int(rl[i]->m_PosY / GLOBAL_SCALE_MOVE));
        if(m_LogicGroup[g].m_Action.m_Region < 0) ERROR_S(L"Robot stands in prohibited area");
    }
}

void CMatrixSideUnit::CalcMaxSpeed()
{
    CMatrixRobotAI* rl[MAX_ROBOTS_ON_MAP];
    float pr[MAX_ROBOTS_ON_MAP];
    int ally_robots_cnt;
    int i, u;
    CPoint tp;
    CMatrixMapStatic* ms;
    float d;

    ms = CMatrixMapStatic::GetFirstLogic();
    for(; ms; ms = ms->GetNextLogic())
    {
        if(!ms->IsRobotAlive()) continue;
        if(ms->GetSide() != m_Id) continue;

        ms->AsRobot()->m_GroupSpeed = ms->AsRobot()->GetMaxSpeed();
    }

    for(i = 0; i < MAX_LOGIC_GROUP; ++i)
    {
        if(m_Id == PLAYER_SIDE && m_PlayerGroup[i].m_RobotCnt <= 0) continue;
        else if(m_Id != PLAYER_SIDE && m_LogicGroup[i].RobotsCnt() <= 0) continue;

        float cx = 0.0f;
        float cy = 0.0f;
        float dx = 0.0f;
        float dy = 0.0f;

        ally_robots_cnt = 0;

        ms = CMatrixMapStatic::GetFirstLogic();
        for(; ms; ms = ms->GetNextLogic())
        {
            if(!ms->IsRobotAlive()) continue;
            if(ms->GetSide() != m_Id) continue;
            if(ms->AsRobot()->GetGroupLogic() != i) continue;

            rl[ally_robots_cnt] = ms->AsRobot();
            cx += rl[ally_robots_cnt]->m_PosX;
            cy += rl[ally_robots_cnt]->m_PosY;

            if(rl[ally_robots_cnt]->GetReturnCoords(tp))
            {
                dx += GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                dy += GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
            }
            else if(rl[ally_robots_cnt]->GetMoveToCoords(tp))
            {
                dx += GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                dy += GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
            }
            else
            {
                dx += rl[ally_robots_cnt]->m_PosX;
                dy += rl[ally_robots_cnt]->m_PosY;
            }

            ++ally_robots_cnt;
        }

        if(ally_robots_cnt <= 1) continue;

        d = 1.0f / ally_robots_cnt;
        cx *= d;
        cy *= d;
        dx = dx * d - cx;
        dy = dy * d - cy;
        d = sqrt(dx * dx + dy * dy);
        if(d < 100.0f) continue;
        dx /= d;
        dy /= d;

        float minpr = 1e20f;
        float maxpr = -1e20f;

        for(u = 0; u < ally_robots_cnt; ++u)
        {
            float rx = rl[u]->m_PosX;
            float ry = rl[u]->m_PosY;

            pr[u] = dx * (rx - cx) + dy * (ry - cy);
            minpr = min(pr[u], minpr);
            maxpr = max(pr[u], maxpr);
        }

        for(u = 0; u < ally_robots_cnt - 1; ++u)
        {
            for(int t = u + 1; t < ally_robots_cnt; ++t)
            {
                if(pr[t] < pr[u])
                {
                    d = pr[t];
                    pr[t] = pr[u];
                    pr[u] = d;
                    CMatrixRobotAI* robot = rl[t];
                    rl[t] = rl[u];
                    rl[u] = robot;
                }
            }
        }

        for(u = 0; u < ally_robots_cnt - 1; ++u)
        {
            if((pr[u + 1] - pr[u]) > COLLIDE_BOT_R * 7.0f) break;
        }
        minpr = pr[u];

        maxpr -= minpr;

        for(u = 0; u < ally_robots_cnt; ++u)
        {
            if(rl[u]->GetReturnCoords(tp)) continue;
            if(!rl[u]->GetMoveToCoords(tp)) continue;
            if(rl[u]->GetEnv()->GetEnemyCnt()) continue;

            pr[u] -= minpr;

            if(pr[u] < COLLIDE_BOT_R * 10.0f) continue; // Отстающие, скорость не уменьшают

            rl[u]->m_GroupSpeed *= 0.6f + 0.4f * ((maxpr - pr[u])) / (maxpr - COLLIDE_BOT_R * 10.0f);
        }
    }
}

//Главный обработчик логики, задающий текущее поведение каждой из сторон доминаторов
void CMatrixSideUnit::TactSideLogic()
{
    //dword it1 = timeGetTime();
    //DM(CWStr().Format(L"Res1 Side=<i>", m_Id).Get(), CWStr().Format(L"<i> <i> <i> <i>", m_Resources[0], m_Resources[1], m_Resources[2], m_Resources[3]).Get());

    /*
    if(g_MatrixMap->GetPlayerSide()->GetUnitUnderManualControl())
    {
        if(g_MatrixMap->GetPlayerSide()->GetUnitUnderManualControl()->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
        {
            if(((CMatrixRobotAI*)g_MatrixMap->GetPlayerSide()->GetUnitUnderManualControl())->GetOrdersInPool() > 0)
            {
                ((CMatrixRobotAI*)g_MatrixMap->GetPlayerSide()->GetUnitUnderManualControl())->BreakAllOrders();
            }
        }
     }
     */

    if(g_MatrixMap->m_RoadNetwork.m_RegionCnt <= 0) return;

    Regroup();

    int i, u, t, k, p, team, cnt, sme, level, next, dist;
    //int skipregion[MAX_ROBOTS_ON_MAP];
    //int skipregioncnt;
    SMatrixLogicAction* ac;
    CPoint tp;
    CMatrixMapStatic* ms;
    //SMatrixRegion *mr;
    SMatrixPlace* place;
    CMatrixRobotAI* rl[MAX_ROBOTS_ON_MAP];    //Список всех роботов на карте
    int ally_robots_cnt;                         //Количество всех роботов на карте

    if(m_Region == nullptr) m_Region = (SMatrixLogicRegion*)HAllocClear(sizeof(SMatrixLogicRegion) * g_MatrixMap->m_RoadNetwork.m_RegionCnt, Base::g_MatrixHeap);
    if(m_RegionIndex == nullptr) m_RegionIndex = (int*)HAllocClear(sizeof(int) * g_MatrixMap->m_RoadNetwork.m_RegionCnt, Base::g_MatrixHeap);

    //Запускаем высшую логику раз в 100 тактов
    if(m_LastTactHL && (g_MatrixMap->GetTime() - m_LastTactHL) < 100) return;
    m_LastTactHL = g_MatrixMap->GetTime();

    CalcStrength();

    EscapeFromBomb();

    /*
    static int lastsave = 0;
    if(timeGetTime() - lastsave > 1000)
    {
        lastsave = timeGetTime();
        HListPrint(L"#save_mem.xls");
    }
    */

    //Определяем текущего врага из всех враждующих сторон на карте, если уже подошло время пересчитать приоритеты
    if(m_NextWarSideCalcTime < g_MatrixMap->GetTime())
    {
        if(m_WarSide < 0 || g_MatrixMap->Rnd(0, 2))
        {
            //Сначала ищем самого слабого, чтобы побыстрее забить его и нарастить силы
            m_WarSide = -1;
            float mst = 1e20f; //100000000000000000000
            if(m_Strength > 0)
            {
                //Перебираем всех активных участников сражения
                for(i = 0; i < g_MatrixMap->m_SidesCount; ++i)
                {
                    //Пропускаем сторону, для которой сейчас считается логика
                    if(g_MatrixMap->m_Side[i].m_Id == m_Id) continue;
                    //Пропускаем отсутствующую, либо уже уничтоженную сторону
                    if(g_MatrixMap->m_Side[i].GetStatus() == SS_NONE) continue;
                    float cur_str = g_MatrixMap->m_Side[i].m_Strength;
                    if(cur_str <= 0) continue;
                    //Если нашли хотя бы кого-то слабее заданного значения, запоминаем Id его стороны (проверяем всех, чтобы найти самого слабого)
                    if((cur_str < mst) && (cur_str < m_Strength * 0.5f))
                    {
                        mst = cur_str;
                        m_WarSide = g_MatrixMap->m_Side[i].m_Id;
                    }
                }
            }
            //Если слабого нет то бьём самого сильного, чтобы он не стал ещё сильней
            if(m_WarSide < 0)
            {
                mst = -1e20f; //-100000000000000000000
                //Перебираем всех активных участников сражения
                for(i = 0; i < g_MatrixMap->m_SidesCount; ++i)
                {
                    //Пропускаем сторону, для которой сейчас считается логика
                    if(g_MatrixMap->m_Side[i].m_Id == m_Id) continue;
                    //Пропускаем отсутствующую, либо уже уничтоженную сторону
                    if(g_MatrixMap->m_Side[i].GetStatus() == SS_NONE) continue;
                    float cur_str = g_MatrixMap->m_Side[i].m_Strength;
                    //"Самый сильный" враг здесь будет подобран в любом случае
                    if(cur_str > mst)
                    {
                        mst = cur_str;
                        m_WarSide = g_MatrixMap->m_Side[i].m_Id;
                    }
                }
            }
        }
        else
        {
            int tries = 10;
            int idx = 0;
            do
            {
                idx = g_MatrixMap->Rnd(0, g_MatrixMap->m_SidesCount - 1);
                if(m_Id != g_MatrixMap->m_Side[idx].m_Id && g_MatrixMap->m_Side[idx].GetStatus() != SS_NONE) break;
            } while(--tries > 0);

            if(tries < 1)
            {
                m_WarSide = -1;
            }
            else m_WarSide = g_MatrixMap->m_Side[idx].m_Id;
        }
        m_NextWarSideCalcTime = g_MatrixMap->GetTime() + MAIN_ENEMY_RECALC_PERIOD;
    }

    //Собираем общую статистику силы сторон на поле боя (определяем картину боя с позиции стороны, для которой сейчас исполняется код)
    m_RobotsCnt = 0;

    for(i = 0; i < MAX_LOGIC_GROUP; ++i)
    {
        m_LogicGroup[i].m_Strength = 0;
    }

    for(i = 0; i < m_TeamCnt; ++i)
    {
        m_Team[i].m_RobotCnt = 0;
        m_Team[i].m_Strength = 0;
        m_Team[i].m_GroupCnt = 0;
        //m_Team[i].m_WaitUnion = false;
        m_Team[i].SetStay(true);
        m_Team[i].SetWar(false);
        m_Team[i].m_CenterMass.x = 0;
        m_Team[i].m_CenterMass.y = 0;
        m_Team[i].m_RadiusMass = 0;
        m_Team[i].m_Rect = CRect(1000000000, 1000000000, 0, 0);
        m_Team[i].m_Center.x = 0;
        m_Team[i].m_Center.y = 0;
        m_Team[i].m_Radius = 0;
        m_Team[i].m_ActionCnt = 0;
        //m_Team[i].m_RegionMass = -1;
        m_Team[i].m_RegionNearDanger = -1;
        m_Team[i].m_RegionFarDanger = -1;
        m_Team[i].m_RegionNearEnemy = -1;
        m_Team[i].m_RegionNearRetreat = -1;
        m_Team[i].m_RegionNearForward = -1;
        m_Team[i].m_RegionNerestBase = -1;
        m_Team[i].m_ActionPrev = m_Team[i].m_Action;
        m_Team[i].m_RegionNext = -1;
        m_Team[i].SetRobotInDesRegion(false);

        m_Team[i].Move(0);

        m_Team[i].m_RegionListCnt = 0;
    }

    for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i)
    {
        m_Region[i].m_WarEnemyRobotCnt = 0;
        m_Region[i].m_WarEnemyCannonCnt = 0;
        m_Region[i].m_WarEnemyBuildingCnt = 0;
        m_Region[i].m_WarEnemyBaseCnt = 0;
        m_Region[i].m_EnemyRobotCnt = 0;
        m_Region[i].m_EnemyCannonCnt = 0;
        m_Region[i].m_EnemyBuildingCnt = 0;
        m_Region[i].m_EnemyBaseCnt = 0;
        m_Region[i].m_NeutralCannonCnt = 0;
        m_Region[i].m_NeutralBuildingCnt = 0;
        m_Region[i].m_NeutralBaseCnt = 0;
        m_Region[i].m_OurRobotCnt = 0;
        m_Region[i].m_OurCannonCnt = 0;
        m_Region[i].m_OurBuildingCnt = 0;
        m_Region[i].m_OurBaseCnt = 0;
        m_Region[i].m_EnemyRobotDist = -1;
        m_Region[i].m_EnemyBuildingDist = -1;
        m_Region[i].m_OurBaseDist = -1;

        m_Region[i].m_Danger = 0.0f;
        m_Region[i].m_DangerAdd = 0.0f;

        m_Region[i].m_EnemyForces = 0.0f;
        m_Region[i].m_OurForces = 0.0f;
        m_Region[i].m_EnemyForcesAdd = 0.0f;
        m_Region[i].m_OurForcesAdd = 0.0f;
    }

    ms = CMatrixMapStatic::GetFirstLogic();
    while(ms)
    {
        switch(ms->GetObjectType())
        {
            //Статистика по базам/заводам
            case OBJECT_TYPE_BUILDING:
            {
                CMatrixBuilding* cur_building = ms->AsBuilding();

                //Определяем позицию здания на карте
                i = GetRegion(
                    int(cur_building->m_Pos.x / GLOBAL_SCALE_MOVE),
                    int(cur_building->m_Pos.y / GLOBAL_SCALE_MOVE)
                );

                if(i >= 0)
                {
                    if(cur_building->m_Side == NEUTRAL_SIDE)
                    {
                        ++m_Region[i].m_NeutralBuildingCnt;
                    }
                    else if(cur_building->m_Side != m_Id)
                    {
                        ++m_Region[i].m_EnemyBuildingCnt;
                        if(cur_building->m_Side == m_WarSide)
                        {
                            ++m_Region[i].m_WarEnemyBuildingCnt;
                        }
                    }
                    else
                    {
                        ++m_Region[i].m_OurBuildingCnt;
                    }

                    if(cur_building->m_Kind == BUILDING_BASE)
                    {
                        if(cur_building->m_Side == NEUTRAL_SIDE)
                        {
                            ++m_Region[i].m_NeutralBaseCnt;
                        }
                        else if(cur_building->m_Side != m_Id)
                        {
                            ++m_Region[i].m_EnemyBaseCnt;
                            if(cur_building->m_Side == m_WarSide)
                            {
                                ++m_Region[i].m_WarEnemyBaseCnt;
                            }
                        }
                        else
                        {
                            ++m_Region[i].m_OurBaseCnt;
                        }
                    }
                }

                break;
            }
            //Статистика по роботам
            case OBJECT_TYPE_ROBOT_AI:
            {
                CMatrixRobotAI* cur_robot = ms->AsRobot();

                //Если данный робот ещё не помирает
                if(cur_robot->m_CurrentState != ROBOT_DIP)
                {
                    //Определяем позицию робота на карте
                    tp.x = int(cur_robot->m_PosX / GLOBAL_SCALE_MOVE);
                    tp.y = int(cur_robot->m_PosY / GLOBAL_SCALE_MOVE);
                    i = GetRegion(tp);

                    if(i >= 0)
                    {
                        if(cur_robot->m_Side == NEUTRAL_SIDE) {} //Нейтральных роботов на карте в настоящее время не существует
                        else if(cur_robot->m_Side != m_Id)
                        {
                            ++m_Region[i].m_EnemyRobotCnt;
                            if(cur_robot->m_Side == m_WarSide)
                            {
                                ++m_Region[i].m_WarEnemyRobotCnt;
                            }

                            float strength = cur_robot->GetStrength();
                            //if(ms->GetSide() != PLAYER_SIDE) strength *= 1.5; //Завышаем опасность между компьютерами, чтобы они воевали не между собой, а против игрока
                            m_Region[i].m_EnemyForces += strength;
                            m_Region[i].m_Danger += strength * m_DangerMultiplier;
                        }
                        else
                        {
                            ++m_Region[i].m_OurRobotCnt;
                            m_Region[i].m_OurForces += cur_robot->GetStrength();
                        }
                    }

                    if(cur_robot->m_Side == m_Id)
                    {
                        ++m_RobotsCnt;
                        team = cur_robot->GetTeam();
                        if(team >= 0 && team < m_TeamCnt)
                        {
                            ++m_Team[team].m_RobotCnt;
                            m_Team[team].m_Strength += cur_robot->GetStrength();
                            m_Team[team].m_CenterMass += tp;
                            m_Team[team].m_Rect.left = min(m_Team[team].m_Rect.left, tp.x);
                            m_Team[team].m_Rect.top = min(m_Team[team].m_Rect.top, tp.y);
                            m_Team[team].m_Rect.right = max(m_Team[team].m_Rect.right, tp.x);
                            m_Team[team].m_Rect.bottom = max(m_Team[team].m_Rect.bottom, tp.y);

                            m_Team[team].OrMove(1 << (cur_robot->m_Module[0].m_Kind - 1));

                            if(i == m_Team[team].m_Action.m_Region) m_Team[team].SetRobotInDesRegion(true);

                            for(t = 0; t < m_Team[team].m_RegionListCnt; ++t)
                            {
                                if(m_Team[team].m_RegionList[t] == i) break;
                            }

                            if(t >= m_Team[team].m_RegionListCnt)
                            {
                                ++m_Team[team].m_RegionListCnt;
                                m_Team[team].m_RegionList[t] = i;
                                m_Team[team].m_RegionListRobots[t] = 1;
                            }
                            else
                            {
                                ++m_Team[team].m_RegionListRobots[t];
                            }

                            if(!m_Team[team].IsWar()) m_Team[team].SetWar(cur_robot->GetEnv()->GetEnemyCnt() > 0);
                        }

                        if(cur_robot->FindOrderLikeThat(ROT_MOVE_TO) || cur_robot->FindOrderLikeThat(ROT_MOVE_RETURN)) m_Team[team].SetStay(false);

                        ASSERT(robot->GetGroupLogic() >= 0 && cur_robot->GetGroupLogic() < MAX_LOGIC_GROUP);
                        m_LogicGroup[cur_robot->GetGroupLogic()].m_Strength += cur_robot->GetStrength();
                    }
                }

                break;
            }
            // ;(
            /*
            case OBJECT_TYPE_FLYER:
            {
                CMatrixFlyer* cur_flyer = ms->AsFlyer();
            }
            */
            //Статистика по турелям
            case OBJECT_TYPE_TURRET:
            {
                CMatrixTurret* cur_turret = ms->AsTurret();

                //Определяем позицию пушки на карте
                tp.x = TruncFloat(cur_turret->m_Pos.x * INVERT(GLOBAL_SCALE_MOVE));
                tp.y = TruncFloat(cur_turret->m_Pos.y * INVERT(GLOBAL_SCALE_MOVE));
                i = GetRegion(tp);

                if(i >= 0)
                {
                    if(cur_turret->GetSide() == NEUTRAL_SIDE)
                    {
                        ++m_Region[i].m_NeutralCannonCnt;
                        m_Region[i].m_DangerAdd += cur_turret->GetStrength() * m_DangerMultiplier;
                    }
                    else if(cur_turret->GetSide() != m_Id)
                    {
                        ++m_Region[i].m_EnemyCannonCnt;
                        if(cur_turret->GetSide() == m_WarSide)
                        {
                            ++m_Region[i].m_WarEnemyCannonCnt;
                        }

                        float strength = cur_turret->GetStrength();
                        //if(cur_turret->GetSide() != PLAYER_SIDE) strength *= 1.5; // Завышаем опасность между компьютерами, чтобы они воевали не между собой, а против игрока
                        m_Region[i].m_DangerAdd += strength * m_DangerMultiplier;
                        //m_Region[i].m_EnemyForces += strength;
                        m_Region[i].m_EnemyForcesAdd += strength;
                    }
                    else
                    {
                        ++m_Region[i].m_OurCannonCnt;
                        //m_Region[i].m_OurForces += cur_turret->GetStrength();
                        m_Region[i].m_OurForcesAdd += cur_turret->GetStrength();
                    }
                }

                break;
            }
        }

        ms = ms->GetNextLogic();
    }

    //Повышаем опасность
    for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i)
    {
        if(m_Region[i].m_Danger <= 0.0f) continue;

        SMatrixLogicRegion* lr = m_Region;
        for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u, ++lr)
        {
            lr->m_Data = 0;
        }

        for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_Region[i].m_NearCnt; ++u)
        {
            t = g_MatrixMap->m_RoadNetwork.m_Region[i].m_Near[u];
            if(m_Region[t].m_Data > 0) continue;

            m_Region[t].m_DangerAdd += m_Region[i].m_Danger;
            m_Region[t].m_EnemyForcesAdd += m_Region[i].m_EnemyForces;
            m_Region[t].m_OurForcesAdd += m_Region[i].m_OurForces;
            m_Region[t].m_Data = 1;
        }

        /*
        cnt = 0; sme = 0; dist = 0;
        m_RegionIndex[cnt] = i;
        m_Region[i].m_Data = 3;
        ++cnt;

        while(sme < cnt)
        {
            for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++u)
            {
                t = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[u];
                if(m_Region[t].m_Data > 0) continue;

                m_Region[t].m_DangerAdd += m_Region[i].m_Danger;

                if(m_Region[t].m_Danger > 0)
                {
                    m_RegionIndex[cnt] = t;
                    m_Region[t].m_Data = 3;
                    ++cnt;
                }
                else
                {
                    m_Region[t].m_Data = m_Region[m_RegionIndex[sme]].m_Data - 1;
                    if(m_Region[t].m_Data >= 2)
                    {
                        m_RegionIndex[cnt] = t;
                        ++cnt;
                    }
                }
            }

            ++sme;
        }
        */
    }

    for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i)
    {
        m_Region[i].m_Danger += m_Region[i].m_DangerAdd;
        m_Region[i].m_EnemyForces += m_Region[i].m_EnemyForcesAdd;
        m_Region[i].m_OurForces += m_Region[i].m_OurForcesAdd;
    }

    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].m_RobotCnt <= 0)
        {
            m_Team[i].m_RegionMass = -1;
            m_Team[i].m_RegionMassPrev = -1;
            m_Team[i].m_Brave = 0;
            m_Team[i].m_BraveStrengthCancel = 0;
            m_Team[i].SetWar(false);
            continue;
        }

        if(m_Team[i].IsWar()) m_Team[i].SetRegroupOnlyAfterWar(false);

        //if(!m_Team[i].m_WarCalc) m_Team[i].m_War = false;

        //Сортируем список по количеству роботов
        for(t = 0; t < m_Team[i].m_RegionListCnt - 1; ++t)
        {
            for(u = t + 1; u < m_Team[i].m_RegionListCnt; ++u)
            {
                if(m_Team[i].m_RegionListRobots[u] > m_Team[i].m_RegionListRobots[t])
                {
                    k = m_Team[i].m_RegionList[u]; m_Team[i].m_RegionList[u] = m_Team[i].m_RegionList[t]; m_Team[i].m_RegionList[t] = k;
                    k = m_Team[i].m_RegionListRobots[u]; m_Team[i].m_RegionListRobots[u] = m_Team[i].m_RegionListRobots[t]; m_Team[i].m_RegionListRobots[t] = k;
                }
            }
        }

        m_Team[i].m_CenterMass.x = m_Team[i].m_CenterMass.x / m_Team[i].m_RobotCnt;
        m_Team[i].m_CenterMass.y = m_Team[i].m_CenterMass.y / m_Team[i].m_RobotCnt;
        m_Team[i].m_Center.x = (m_Team[i].m_Rect.left + m_Team[i].m_Rect.right) / 2;
        m_Team[i].m_Center.y = (m_Team[i].m_Rect.top + m_Team[i].m_Rect.bottom) / 2;

        ASSERT(m_Team[i].m_RegionListCnt);

        int cr = -1;
        k = m_Team[i].m_RegionListRobots[0];
        if(m_Team[i].m_Action.m_Type != mlat_None)
        {
            for(t = 0; t < m_Team[i].m_RegionListCnt && k == m_Team[i].m_RegionListRobots[t]; ++t)
            {
                if(m_Team[i].m_Action.m_Region == m_Team[i].m_RegionList[t])
                {
                    cr = m_Team[i].m_Action.m_Region;
                    break;
                }
            }
        }

        if(cr < 0) cr = m_Team[i].m_RegionList[0];

        //int cr = GetRegion(m_Team[i].m_CenterMass);
        //Если регион назначения занят и текущий регион возле региона назначения, то считаем что текущий регион равен региону назначения
        if(m_Team[i].m_Action.m_Type != mlat_None && m_Team[i].m_Action.m_Region >= 0 && m_Team[i].m_Action.m_Region != cr && g_MatrixMap->m_RoadNetwork.IsNearestRegion(cr, m_Team[i].m_Action.m_Region))
        {
            SMatrixRegion* region = g_MatrixMap->m_RoadNetwork.GetRegion(m_Team[i].m_Action.m_Region);
            for(u = 0; u < region->m_PlaceCnt; ++u) GetPlacePtr(region->m_Place[u])->m_Data = 0;

            //Находим роботов, текущей команды, места которые не в регионе назначения
            // + помечаем занятые места
            ally_robots_cnt = 0;
            ms = CMatrixMapStatic::GetFirstLogic();
            while(ms)
            {
                if(ms->IsRobotAlive() && ms->GetSide() == m_Id && ms->AsRobot()->GetTeam() == i)
                {
                    place = ObjPlacePtr(ms);
                    if(place == nullptr || place->m_Region != m_Team[i].m_Action.m_Region)
                    {
                        rl[ally_robots_cnt] = (CMatrixRobotAI*)ms;
                        ++ally_robots_cnt;
                    }
                }

                if(ms->IsUnitAlive()) ObjPlaceData(ms, 1);
                ms = ms->GetNextLogic();
            }

            //Проверяем, можем ли разместить хоть одного работа в регионе назначения
            if(ally_robots_cnt > 0)
            {
                for(t = 0; t < ally_robots_cnt; ++t)
                {
                    for(u = 0; u < region->m_PlaceCnt; ++u)
                    {
                        place = GetPlacePtr(region->m_Place[u]);
                        if(place->m_Data) continue;
                        if(!CanMove(place->m_Move, rl[t])) continue;

                        break;
                    }

                    if(u < region->m_PlaceCnt) break;
                }

                if(t >= ally_robots_cnt) cr = m_Team[i].m_Action.m_Region;
            }
        }

        if(m_Team[i].m_RegionMass != cr)
        {
            m_Team[i].m_RegionMassPrev = m_Team[i].m_RegionMass;
            m_Team[i].m_RegionMass = cr;
        }

        //Находим следующий регион в пути
        if(m_Team[i].m_Action.m_Type != mlat_None)
        {
            for(u = 0; u < m_Team[i].m_Action.m_RegionPathCnt; ++u)
            {
                if(m_Team[i].m_Action.m_RegionPath[u] == m_Team[i].m_RegionMass)
                {
                    if((u + 1) < m_Team[i].m_Action.m_RegionPathCnt) m_Team[i].m_RegionNext = m_Team[i].m_Action.m_RegionPath[u + 1];
                    break;
                }
            }

            //Если текущий регион не в пути, то ищем ближайший к пути
            if(m_Team[i].m_Action.m_RegionPathCnt >= 2 && u >= m_Team[i].m_Action.m_RegionPathCnt)
            {
                for(u = 0;u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u)
                {
                    m_Region[u].m_Data = 0;
                }

                for(u = 0;u < m_Team[i].m_Action.m_RegionPathCnt; ++u)
                {
                    m_Region[m_Team[i].m_Action.m_RegionPath[u]].m_Data = dword(-(u + 1));
                }

                sme = 0; cnt = 0; k = -1; level = 1;
                m_RegionIndex[cnt] = m_Team[i].m_RegionMass;
                m_Region[m_RegionIndex[cnt]].m_Data = level;
                ++cnt;
                ++level;

                next = cnt;
                while(sme < next)
                {
                    for(t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++t)
                    {
                        u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[t];
                        if(int(m_Region[u].m_Data) > 0) continue;

                        if(k >= 0)
                        {
                            if(int(m_Region[k].m_Data) < int(m_Region[u].m_Data)) k = u;
                        }
                        else
                        {
                            if(m_Region[u].m_Data == 0)
                            {
                                m_RegionIndex[cnt] = u;
                                ++cnt;
                                m_Region[u].m_Data = level;
                            }
                            else
                            {
                                k = u;
                            }
                        }
                    }

                    ++sme;

                    if(sme >= next)
                    {
                        next = cnt;
                        if(k) break;
                        ++level;
                    }
                }

                if(k >= 0)
                {
                    while(true)
                    {
                        p = -1;
                        for(t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[k].m_NearCnt; ++t)
                        {
                            u = g_MatrixMap->m_RoadNetwork.m_Region[k].m_Near[t];
                            if(int(m_Region[u].m_Data) <= 0) continue;

                            if(int(m_Region[u].m_Data) < level)
                            {
                                p = u;
                                break;
                            }
                        }

                        if(p < 0) ERROR_S(L"CMatrixSideUnit::TactSideLogic Error!");
                        if(m_Region[p].m_Data <= 1)
                        {
                            m_Team[i].m_RegionNext = k;
                            break;
                        }
                        else
                        {
                            k = p;
                            level = m_Region[k].m_Data;
                        }
                    }
                }
            }

            //g_MatrixMap->m_RoadNetwork.FindPathInRegionInit();
            //m_Team[i].m_RegionPathCnt = g_MatrixMap->m_RoadNetwork.FindPathInRegionRun(m_Team[i].m_RegionMass, m_Team[i].m_Action.m_Region, m_Team[i].m_RegionPath, 128);
            //if(m_Team[i].m_RegionPathCnt >= 2) m_Team[i].m_RegionNext = m_Team[i].m_RegionPath[1];
        }

        if(m_Team[i].m_RegionMass >= 0)
        {
            if(m_Region[m_Team[i].m_RegionMass].m_EnemyRobotCnt > 0)
            {
                m_Team[i].m_RegionNearEnemy = m_Team[i].m_RegionMass;
            }
            else
            {
                for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_NearCnt; ++u)
                {
                    t = g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_Near[u];
                    if(m_Region[t].m_EnemyRobotCnt > 0)
                    {
                        m_Team[i].m_RegionNearEnemy = t;
                        break;
                    }
                }
            }

            float md = 0, md2 = 0;
            if(m_Region[m_Team[i].m_RegionMass].m_Danger > 0)
            {
                m_Team[i].m_RegionFarDanger = m_Team[i].m_RegionNearDanger = m_Team[i].m_RegionMass;
                md2 = md = m_Region[m_Team[i].m_RegionMass].m_Danger;
            }

            for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_NearCnt; ++u)
            {
                t = g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_Near[u];

                if(m_Region[t].m_Danger > md)
                {
                    md = m_Region[t].m_Danger;
                    m_Team[i].m_RegionNearDanger = t;
                }
                if(m_Region[t].m_Danger > md2)
                {
                    md2 = m_Region[t].m_Danger;
                    m_Team[i].m_RegionFarDanger = t;
                }

                for(k = 0; k < g_MatrixMap->m_RoadNetwork.m_Region[t].m_NearCnt; ++k)
                {
                    p = g_MatrixMap->m_RoadNetwork.m_Region[t].m_Near[k];

                    if(m_Region[p].m_Danger > md2)
                    {
                        md2 = m_Region[p].m_Danger;
                        m_Team[i].m_RegionFarDanger = p;
                    }
                }
            }

            /*
            for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_NearCnt; ++u)
            {
                t = g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_Near[u];
                if(m_Region[t].m_OurRobotCnt > 0 && m_Region[t].m_EnemyRobotCnt <= 0)
                {
                    m_Team[i].m_RegionNearRetreat = t;
                }
            }
            if(m_Team[i].m_RegionNearRetreat < 0)
            {
                for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_NearCnt; ++u)
                {
                    t = g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_Near[u];
                    if(m_Region[t].m_EnemyRobotCnt <= 0) m_Team[i].m_RegionNearRetreat = t;
                }
            }
            */
        }
    }

    ms = CMatrixMapStatic::GetFirstLogic();
    while(ms)
    {
        if(ms->IsRobotAlive() && ms->AsRobot()->m_Side == m_Id)
        {
            tp.x = int(ms->AsRobot()->m_PosX / GLOBAL_SCALE_MOVE);
            tp.y = int(ms->AsRobot()->m_PosY / GLOBAL_SCALE_MOVE);

            team = ((CMatrixRobotAI*)ms)->GetTeam();
            if(team >= 0 && team < m_TeamCnt)
            {
                m_Team[team].m_RadiusMass = max(m_Team[team].m_RadiusMass, m_Team[team].m_CenterMass.Dist2(tp));
                m_Team[team].m_Radius = max(m_Team[team].m_Radius, m_Team[team].m_Center.Dist2(tp));
            }
        }

        ms = ms->GetNextLogic();
    }

    for(i = 0; i < m_TeamCnt; ++i)
    {
        m_Team[i].m_RadiusMass = int(sqrt(double(m_Team[i].m_RadiusMass)));
        m_Team[i].m_Radius = int(sqrt(double(m_Team[i].m_Radius)));
    }

    for(i = 0; i < m_TeamCnt; ++i)
    {
        m_Team[i].m_GroupCnt = 0;
        for(u = 0; u < MAX_LOGIC_GROUP; ++u)
        {
            if(m_LogicGroup[u].RobotsCnt() <= 0) continue;
            if(m_LogicGroup[u].m_Team != i) continue;
            ++m_Team[i].m_GroupCnt;
        }
    }

    //Стоит ли ждать пока команда соберется
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].m_GroupCnt <= 1)
        {
            m_Team[i].SetWaitUnion(false);
            continue;
        }

        if(m_Team[i].m_RobotCnt <= 1)
        {
            m_Team[i].SetWaitUnion(false);
            continue;
        }

        int groupms = -1;
        for(u = 0; u < MAX_LOGIC_GROUP; ++u)
        {
            if(m_LogicGroup[u].RobotsCnt() <= 0) continue;
            if(m_LogicGroup[u].m_Team != i) continue;

            if(groupms < 0) groupms = u;
            else
            {
                if (m_LogicGroup[u].m_Strength > m_LogicGroup[groupms].m_Strength) groupms = u;
            }
        }

        if(groupms < 0)
        {
            m_Team[i].SetWaitUnion(false);
            continue;
        }

        if(g_MatrixMap->GetTime() - m_Team[i].m_WaitUnionLast < 5000) continue;

        m_Team[i].SetWaitUnion(!m_Team[i].IsStay() && (m_LogicGroup[groupms].m_Strength / m_Team[i].m_Strength) <= 0.8f);
        m_Team[i].m_WaitUnionLast = g_MatrixMap->GetTime();
    }

    /*
    CMatrixGroup *mg = m_GroupsList->m_FirstGroup;
    while(mg)
    {
        ++m_Team[mg->m_Team].m_GroupCnt;
        mg = mg->m_NextGroup;
    }
    */

    //Как далеко роботы врага
    cnt = 0;
    sme = 0;
    dist = 0;

    for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i)
    {
        if(m_Region[i].m_EnemyRobotCnt > 0)
        {
            m_RegionIndex[cnt] = i;
            ++cnt;
            m_Region[i].m_EnemyRobotDist = dist;
        }
    }

    next = cnt;
    ++dist;

    while(sme < cnt)
    {
        for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++i)
        {
            u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[i];
            if(m_Region[u].m_EnemyRobotDist >= 0) continue;

            m_RegionIndex[cnt] = u;
            ++cnt;
            m_Region[u].m_EnemyRobotDist = dist;
        }

        ++sme;

        if(sme >= next)
        {
            next = cnt;
            ++dist;
        }
    }

    //Как далеко постройки врага
    cnt = 0;
    sme = 0;
    dist = 0;

    for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i)
    {
        if(m_Region[i].m_EnemyBuildingCnt > 0)
        {
            m_RegionIndex[cnt] = i;
            ++cnt;
            m_Region[i].m_EnemyBuildingDist = dist;
        }
    }

    next = cnt;
    ++dist;
    while(sme < cnt)
    {
        for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++i)
        {
            u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[i];
            if(m_Region[u].m_EnemyBuildingDist >= 0) continue;

            m_RegionIndex[cnt] = u;
            ++cnt;
            m_Region[u].m_EnemyBuildingDist = dist;
        }

        ++sme;
        if(sme >= next)
        {
            next = cnt;
            ++dist;
        }
    }

    //Как далеко от наших баз
    cnt = 0;
    sme = 0;
    dist = 0;

    for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i)
    {
        if(m_Region[i].m_OurBaseCnt > 0)
        {
            m_RegionIndex[cnt] = i;
            ++cnt;
            m_Region[i].m_OurBaseDist = dist;
        }
    }

    next = cnt;
    ++dist;
    while(sme < cnt)
    {
        for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++i)
        {
            u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[i];
            if(m_Region[u].m_OurBaseDist >= 0) continue;

            m_RegionIndex[cnt] = u;
            ++cnt;
            m_Region[u].m_OurBaseDist = dist;
        }

        ++sme;
        if(sme >= next)
        {
            next = cnt;
            ++dist;
        }
    }

    //Регион отступления
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].m_RobotCnt <= 0) continue;
        if(m_Team[i].m_RegionMass < 0) continue;
        if(m_Team[i].m_RegionNearDanger < 0) continue;

        float md = 1e30f;
        for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_NearCnt; ++u)
        {
            t = g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_Near[u];

            if(max(1, m_Team[i].m_Strength * 0.4) >= m_Region[t].m_Danger)
            {
                if(m_Region[t].m_Danger < md)
                {
                    md = m_Region[t].m_Danger;
                    m_Team[i].m_RegionNearRetreat = t;
                }
                else if(m_Region[t].m_Danger == md && m_Team[i].m_RegionNearRetreat >= 0 && m_Region[t].m_OurBaseDist < m_Region[m_Team[i].m_RegionNearRetreat].m_OurBaseDist)
                {
                    md = m_Region[t].m_Danger;
                    m_Team[i].m_RegionNearRetreat = t;
                }
            }
        }
    }

    //Находим ближайшие базы (нужно переписать с учетом непроходимости, то есть по m_OurBaseDist)
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].m_RegionMass < 0) continue;

        ms = CMatrixMapStatic::GetFirstLogic();
        while(ms)
        {
            if(ms->GetObjectType() == OBJECT_TYPE_BUILDING)
            {
                CMatrixBuilding* building = ms->AsBuilding();

                if(building->m_Kind == BUILDING_BASE && building->m_Side == m_Id)
                {
                    u = GetRegion(
                        int(building->m_Pos.x / GLOBAL_SCALE_MOVE),
                        int(building->m_Pos.y / GLOBAL_SCALE_MOVE)
                    );

                    if(u < 0) continue;
                    t = g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionMass].m_Center.Dist2(g_MatrixMap->m_RoadNetwork.m_Region[u].m_Center);
                    if(m_Team[i].m_RegionNerestBase < 0 || t < dist)
                    {
                        m_Team[i].m_RegionNerestBase = u;
                        dist = t;
                        continue;
                    }
                }
            }

            ms = ms->GetNextLogic();
        }
    }

    //Строим робота
    if(SRobotTemplate::m_AIRobotTypesList[m_Id].size()) ChooseAndBuildAIRobot(m_Id); //Если имеются шаблоны роботов конкретно для текущей стороны, выбираем из них
    else ChooseAndBuildAIRobot(); //Иначе выбираем шаблон из общего списка

    //Строим пушки
    if((int)TURRET_KINDS_TOTAL) ChooseAndBuildAICannon();

    //Отвага
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].m_RobotCnt <= 0) continue;

        //if(m_Team[i].m_RegionNearDanger < 0)
        //{
        if(m_Team[i].m_Brave)
        {
            if((g_MatrixMap->GetTime() - m_Team[i].m_Brave) > 10000 && m_Team[i].m_Strength < m_Team[i].m_BraveStrengthCancel)
            {
                m_Team[i].m_Brave = 0;
            }
        }
        //}

        //if(m_Team[i].m_RegionNearDanger >= 0)
        //{
        if(!m_Team[i].m_Brave)
        {
            //if(m_Team[i].m_Strength >= 2000 || ((m_Team[i].m_RobotCnt >= 5) && (m_Team[i].m_Action.m_Type == mlat_Defence) && (g_MatrixMap->GetTime() - m_Team[i].m_ActionTime > 3 * 60 * 1000)))
            //{
            int brave_cnt = GetMaxSideRobots();
            brave_cnt = min(Float2Int(m_BraveMultiplier * float(brave_cnt)), brave_cnt);
            if(brave_cnt < 1) brave_cnt = 1;

            if(m_Team[i].m_RobotCnt >= brave_cnt)
            {
                m_Team[i].m_Brave = g_MatrixMap->GetTime();
                m_Team[i].m_BraveStrengthCancel = m_Team[i].m_Strength * 0.3f;
                if(m_Team[i].m_Action.m_Type == mlat_Retreat)
                {
                    m_Team[i].m_Action.m_Type = mlat_None;
                    m_Team[i].m_ActionTime = g_MatrixMap->GetTime();
                }
            }
        }
        //}
    }

    //Регионы для продвижения вперед
    //UpdateTargetRegion();
    //CalcForwardRegion();

    //Проверяем, корректна ли текущая задача
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].m_RobotCnt <= 0) continue;

        m_Team[i].SetlOk(true);

        if(m_Team[i].m_Action.m_Type == mlat_None)
        {
            m_Team[i].SetlOk(false);
        }
        else if(m_Team[i].m_Action.m_Type == mlat_Defence)
        {
            if(m_Team[i].IsWar()) continue;
            if(m_Team[i].m_Brave && !m_Team[i].IsWaitUnion())
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_Brave==true");
                m_Team[i].SetlOk(false);
                continue;
            }

            if(m_Team[i].m_RegionFarDanger < 0 && (g_MatrixMap->GetTime() - m_Team[i].m_ActionTime > 10000) && !m_Team[i].IsWaitUnion())
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_RegionFarDanger<<0");
                m_Team[i].SetlOk(false);
                continue;
            }

            if(m_Team[i].m_RegionFarDanger >= 0 && (g_MatrixMap->GetTime() - m_Team[i].m_ActionTime > 1000) && m_Region[m_Team[i].m_RegionFarDanger].m_Danger < m_Team[i].m_Strength && !m_Team[i].IsWaitUnion())
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_RegionFarDanger.m_Danger(<f>)<m_Strength(<f>)", m_Region[m_Team[i].m_RegionFarDanger].m_Danger, m_Team[i].m_Strength);
                m_Team[i].SetlOk(false);
                continue;
            }

            //if(m_Team[i].m_RegionMass >= 0 && (m_Region[m_Team[i].m_RegionMass].m_NeutralBuildingCnt > 0 || m_Region[m_Team[i].m_RegionMass].m_EnemyBuildingCnt > 0))
            //{
            if(m_Team[i].IsRobotInDesRegion() && (m_Region[m_Team[i].m_Action.m_Region].m_NeutralBuildingCnt > 0 || m_Region[m_Team[i].m_Action.m_Region].m_EnemyBuildingCnt > 0))
            {
                m_Team[i].m_Action.m_Type = mlat_Capture;
                m_Team[i].m_ActionTime = g_MatrixMap->GetTime();
                DMTeam(i, m_Team[i].m_Action.m_Type, 1, L"From Defence");
                continue;
            }
        }
        else if(m_Team[i].m_Action.m_Type == mlat_Attack)
        {
            m_Team[i].SetlOk(m_Team[i].IsWar());
            if(!m_Team[i].IslOk()) DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_War==false");
        }
        else if(m_Team[i].m_Action.m_Type == mlat_Forward)
        {
            if(m_Team[i].IsWaitUnion())
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_WaitUnion==true");
                m_Team[i].SetlOk(false);
                continue;
            }

            if(m_Team[i].m_RegionNext >= 0 && m_Region[m_Team[i].m_RegionNext].m_Danger * 0.6f > m_Team[i].m_Strength)
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_RegionNext.m_Danger(<f>)*0.6f>m_Strength(<f>)", m_Region[m_Team[i].m_RegionNext].m_Danger, m_Team[i].m_Strength);
                m_Team[i].SetlOk(false);
                continue;
            }

            m_Team[i].SetlOk(m_Team[i].m_RegionMass != m_Team[i].m_Action.m_Region);
            if(!m_Team[i].IslOk())
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_RegionMass==m_Action.m_Region");
            }

            if(m_Team[i].IslOk() && m_Team[i].IsWar())
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_War==true");
                m_Team[i].SetlOk(false);
            }

            //Если идём мимо завода, то попутно захватываем его
            if(m_Team[i].IslOk() && m_Team[i].m_RegionMass >= 0 && (m_Region[m_Team[i].m_RegionMass].m_NeutralBuildingCnt > 0 || m_Region[m_Team[i].m_RegionMass].m_EnemyBuildingCnt > 0))
            {
                for(u = 0; u < m_TeamCnt; ++u)
                {
                    if(u == i) continue;

                    if(
                        ((m_Team[u].m_Action.m_Type == mlat_Capture) || (m_Team[u].m_Action.m_Type == mlat_Forward)) &&
                        (m_Team[u].m_Action.m_Region == m_Team[i].m_RegionMass) &&
                        (m_Team[u].m_Action.m_Region == m_Team[u].m_RegionMass || g_MatrixMap->m_RoadNetwork.IsNearestRegion(m_Team[u].m_Action.m_Region, m_Team[u].m_RegionMass))
                        )
                    {
                        break;
                    }

                    //Если прошли весь список группы и не нашли в нём робота с приказом захвата или продвижения
                    if(u == m_TeamCnt - 1)
                    {
                        u = -1;
                        break;
                    }
                }

                if(u == -1)
                {
                    for(u = 0; u < m_TeamCnt; ++u)
                    {
                        if(u == i) continue;
                        if(
                            ((m_Team[u].m_Action.m_Type == mlat_Capture) || (m_Team[u].m_Action.m_Type == mlat_Forward)) &&
                            m_Team[u].m_Action.m_Region == m_Team[i].m_RegionMass
                            )
                        {
                            DMTeam(u, m_Team[u].m_Action.m_Type, -1, L"From Forward RegionMass");
                            m_Team[u].m_Action.m_Type = mlat_None;
                            m_Team[u].m_ActionTime = g_MatrixMap->GetTime();
                        }
                    }

                    m_Team[i].m_Action.m_Type = mlat_Capture;
                    m_Team[i].m_Action.m_Region = m_Team[i].m_RegionMass;
                    m_Team[i].m_ActionTime = g_MatrixMap->GetTime();
                    DMTeam(i, m_Team[i].m_Action.m_Type, 1, L"From Forward RegionMass");
                }
            }

            //Если не все роботы пришли в регион, но есть завод, который надо захватить, то меняем приказ на захват
            if(m_Team[i].IslOk() && (m_Region[m_Team[i].m_Action.m_Region].m_NeutralBuildingCnt > 0 || m_Region[m_Team[i].m_Action.m_Region].m_EnemyBuildingCnt > 0))
            {
                ms = CMatrixMapStatic::GetFirstLogic();
                while(ms)
                {
                    if(ms->IsRobotAlive() && ms->GetSide() == m_Id && GetObjTeam(ms) == i)
                    {
                        if(GetRegion(ms) == m_Team[i].m_Action.m_Region)
                        {
                            m_Team[i].m_Action.m_Type = mlat_Capture;
                            m_Team[i].m_ActionTime = g_MatrixMap->GetTime();
                            DMTeam(i, m_Team[i].m_Action.m_Type, 1, L"From Forward");
                            break;
                        }
                    }

                    ms = ms->GetNextLogic();
                }

                if(ms)
                {
                    --i;
                    continue;
                }
            }
        }
        else if(m_Team[i].m_Action.m_Type == mlat_Retreat)
        {
            m_Team[i].SetlOk(!m_Team[i].IsWar());
            if(!m_Team[i].IslOk()) DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_War==true");

            //if(m_Region[m_Team[i].m_RegionNearDanger].m_Danger > 0) m_Team[i].m_lOk = false;
            //if(m_Team[i].m_RegionNearDanger >= 0 && m_Team[i].m_Strength >= m_Region[m_Team[i].m_RegionNearDanger].m_Danger * 0.9f)
            //{
            //    m_Team[i].m_lOk = false;
            //    continue;
            //}

            if(m_Team[i].IslOk() && m_Team[i].m_Action.m_Region == m_Team[i].m_RegionMass)
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_Action.m_Region==m_RegionMass");

                if(m_Team[i].m_RegionFarDanger >= 0)
                {
                    if(m_Team[i].m_Strength * 0.9 > m_Region[m_Team[i].m_RegionFarDanger].m_Danger && !m_Team[i].IsWaitUnion())
                    {
                        m_Team[i].m_Action.m_Type = mlat_Forward;
                        m_Team[i].m_Action.m_Region = m_Team[i].m_RegionFarDanger;
                        m_Team[i].m_ActionTime = g_MatrixMap->GetTime();

                        DMTeam(i, m_Team[i].m_Action.m_Type, 1, L"From Retreat m_Strength(<f>)*0.9>m_RegionFarDanger.m_Danger(<f>)", m_Team[i].m_Strength, m_Region[m_Team[i].m_RegionFarDanger].m_Danger);
                        continue;
                    }
                    else if(m_Team[i].m_Strength > m_Region[m_Team[i].m_RegionFarDanger].m_Danger * 0.6)
                    {
                        m_Team[i].m_Action.m_Type = mlat_Defence;
                        m_Team[i].m_ActionTime = g_MatrixMap->GetTime();

                        DMTeam(i, m_Team[i].m_Action.m_Type, 1, L"From Retreat m_Strength(<f>)>m_RegionFarDanger.m_Danger*0.6(<f>)", m_Team[i].m_Strength, m_Region[m_Team[i].m_RegionFarDanger].m_Danger);
                        continue;
                    }
                }

                m_Team[i].SetlOk(false);
                continue;
            }

            //if(m_Team[i].m_RegionNearDanger >= 0) && (m_Team[i].m_Strength<m_Region[m_Team[i].m_RegionNearDanger] * 0.9);
        }
        else if(m_Team[i].m_Action.m_Type == mlat_Capture)
        {
            if(m_Team[i].m_RegionNext >= 0 && m_Region[m_Team[i].m_RegionNext].m_Danger * 0.6f > m_Team[i].m_Strength)
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_RegionNext.m_Danger(<f>)*0.6f>m_Strength(<f>)", m_Region[m_Team[i].m_RegionNext].m_Danger, m_Team[i].m_Strength);
                m_Team[i].SetlOk(false);
                continue;
            }

            m_Team[i].SetlOk(m_Region[m_Team[i].m_Action.m_Region].m_NeutralBuildingCnt > 0 || m_Region[m_Team[i].m_Action.m_Region].m_EnemyBuildingCnt > 0);
            if(!m_Team[i].IslOk())
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_NeutralBuildingCnt<<=0 && m_EnemyBuildingCnt<<=0");
            }

            if(m_Team[i].IslOk() && m_Team[i].IsWar())
            {
                DMTeam(i, m_Team[i].m_Action.m_Type, -1, L"m_War==true");
                m_Team[i].SetlOk(false);
            }
        }
        else if(m_Team[i].m_Action.m_Type == mlat_Intercept)
        {
            m_Team[i].SetlOk(false);
        }
    }

    //Находим всевозможные варианты продвижения вперед
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].IslOk()) continue;
        if(m_Team[i].m_RobotCnt <= 0) continue;
        if(m_Team[i].IsWaitUnion()) continue;

        for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u)
        {
            m_Region[u].m_Data = 0;
        }

        cnt = 0;
        m_RegionIndex[cnt] = m_Team[i].m_RegionMass;
        m_Region[m_Team[i].m_RegionMass].m_Data = 1;
        ++cnt;

        next = cnt;
        sme = 0;
        dist = 1;
        while(sme < cnt)
        {
            for(t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++t)
            {
                u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[t];
                if(m_Region[u].m_Data) continue;

                if(g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearMove[t] & m_Team[i].Move()) continue;

                if(!m_Team[i].m_Brave && m_Team[i].m_Strength < m_Region[u].m_Danger * 0.7) continue; // Если регион слишком опасный, то пропускаем

                m_Region[u].m_Data = 1 + dist;
                m_RegionIndex[cnt] = u;
                ++cnt;

                //Команды не должны идти в один регион (недоделанно)
                for(k = 0; k < m_TeamCnt; ++k)
                {
                    if(k == i) continue;
                    if(m_Team[k].m_RobotCnt <= 0) continue;

                    if(m_Team[k].IslOk())
                    {
                        if(m_Team[k].m_Action.m_Type != mlat_None && m_Team[k].m_Action.m_Region == u) break;
                    }
                    else
                    {
                        if(m_Team[k].m_ActionCnt > 0 && m_Team[k].m_ActionList[0].m_Type != mlat_None && m_Team[k].m_ActionList[0].m_Region == u) break;
                    }
                }
                if(k < m_TeamCnt) continue;

                if(m_Region[u].m_WarEnemyBuildingCnt > 0 || (m_Region[u].m_EnemyBuildingCnt > 0 && dist <= 1 && m_Team[i].m_Strength * 0.33 > m_Region[u].m_Danger))
                {
                    m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Type = mlat_Forward;
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt = dist + 1;
                    m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Region = u;
                    CalcRegionPath(m_Team[i].m_ActionList + m_Team[i].m_ActionCnt, u, m_Team[i].Move());
                    //DMSide(L"CalcRegionPath Start=<i>(<i>) End=<i>(<i>) Cnt=<i> Dist=<i>\n",
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPath[0],
                    //m_Team[i].m_RegionMass,
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPath[m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt-1],
                    //u, m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt, dist);

                    ++m_Team[i].m_ActionCnt;
                    LiveAction(i);

                }
                else if(m_Region[u].m_NeutralBuildingCnt > 0)
                {
                    m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Type = mlat_Forward;
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt=dist+1;
                    m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Region = u;
                    CalcRegionPath(m_Team[i].m_ActionList + m_Team[i].m_ActionCnt, u, m_Team[i].Move());
                    //DMSide(L"CalcRegionPath Start=<i>(<i>) End=<i>(<i>) Cnt=<i> Dist=<i>\n",
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPath[0],
                    //m_Team[i].m_RegionMass,
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPath[m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt-1],
                    //u,
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt,
                    //dist);

                    ++m_Team[i].m_ActionCnt;
                    LiveAction(i);

                }
                else if(m_Region[u].m_WarEnemyRobotCnt > 0)
                {
                    m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Type = mlat_Forward;
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt=dist+1;
                    m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Region = u;
                    CalcRegionPath(m_Team[i].m_ActionList + m_Team[i].m_ActionCnt, u, m_Team[i].Move());
                    //DMSide(L"CalcRegionPath Start=<i>(<i>) End=<i>(<i>) Cnt=<i> Dist=<i>\n",
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPath[0],
                    //m_Team[i].m_RegionMass,
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPath[m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt-1],
                    //u,
                    //m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt,
                    //dist);

                    ++m_Team[i].m_ActionCnt;
                    LiveAction(i);
                }
            }

            ++sme;

            if(sme >= next)
            {
                next = cnt;
                ++dist;
            }
        }

        BestAction(i);
    }
    //Если вариантов продвижения вперед нет, то создаем вариант защиты
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].m_RobotCnt <= 0) continue;
        if(m_Team[i].IslOk()) continue;
        if(m_Team[i].m_ActionCnt > 0) continue;
        if(m_Team[i].m_RegionMass < 0) continue;
        if(m_Team[i].IsWar()) continue;

        ac = m_Team[i].m_ActionList + m_Team[i].m_ActionCnt;
        ac->m_Type = mlat_Defence;
        ac->m_Region = m_Team[i].m_RegionMass;
        ac->m_RegionPathCnt = 1;
        ac->m_RegionPath[0] = m_Team[i].m_RegionMass;
        m_Team[i].m_ActionCnt++;
    }

    /*
    //Если вариантов продвижения вперед нет, то создаем вариант двигаться на вражескую базу или завод
    skipregioncnt = 0;
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].m_RobotCnt<=0) continue;
        if(!m_Team[i].m_lOk) continue;
        if(m_Team[i].m_Action.m_Type==mlat_None) continue;

        skipregion[skipregioncnt]=m_Team[i].m_Action.m_Region;
        skipregioncnt++;
    }

    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].m_RobotCnt<=0) continue;
        if(m_Team[i].m_lOk) continue;
        if(m_Team[i].m_ActionCnt>0) continue;
        if(m_Team[i].m_RegionMass<0) continue;
        if(m_Team[i].m_War) continue;

        u = FindNearRegionWithUTR(m_Team[i].m_RegionMass,skipregion,skipregioncnt, 2 + 4 + 8 + 16);
        if(u < 0) continue;
        ac=m_Team[i].m_ActionList + m_Team[i].m_ActionCnt;
        ac->m_Type=mlat_Forward;
        ac->m_Region=u;
        g_MatrixMap->m_RoadNetwork.FindPathInRegionInit();
        ac->m_RegionPathCnt=g_MatrixMap->m_RoadNetwork.FindPathInRegionRun(m_Team[i].m_Move,m_Team[i].m_RegionMass,u,ac->m_RegionPath,REGION_PATH_MAX_CNT,true);
        ++m_Team[i].m_ActionCnt;

        skipregion[skipregioncnt]=u;
        ++skipregioncnt;
    }
    */

    //Находим всевозможные варианты действий
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].IslOk()) continue;
        if(m_Team[i].m_RobotCnt <= 0) continue;

        /*
        SMatrixRegion *region = g_MatrixMap->m_RoadNetwork.GetRegion(m_Team[i].m_RegionMass);
        for(int t = 0; t < region->m_NearCnt; ++t)
        {
            region->m_Near[t];
        }
        */

        if(m_Team[i].IsWar() && m_Team[i].m_ActionCnt < 16)
        {
            ac = m_Team[i].m_ActionList + m_Team[i].m_ActionCnt;
            ac->m_Type = mlat_Attack;
            ac->m_Region = m_Team[i].m_RegionMass;
            ac->m_RegionPathCnt = 1;
            ac->m_RegionPath[0] = m_Team[i].m_RegionMass;
            ++m_Team[i].m_ActionCnt;
        }

        if(m_Team[i].IsWar() && m_Team[i].m_RegionNearEnemy >= 0 && m_Team[i].m_ActionCnt < 16)
        {
            ac = m_Team[i].m_ActionList + m_Team[i].m_ActionCnt;
            ac->m_Type = mlat_Attack;
            ac->m_Region = m_Team[i].m_RegionNearEnemy;
            ac->m_RegionPathCnt = 2;
            ac->m_RegionPath[0] = m_Team[i].m_RegionMass;
            ac->m_RegionPath[1] = ac->m_Region;
            ++m_Team[i].m_ActionCnt;
        }

        if(m_Team[i].IsWar() && m_Team[i].m_ActionCnt < 16)
        {
            ac = m_Team[i].m_ActionList + m_Team[i].m_ActionCnt;
            ac->m_Type = mlat_Defence;
            ac->m_Region = m_Team[i].m_RegionMass;
            ac->m_RegionPathCnt = 1;
            ac->m_RegionPath[0] = m_Team[i].m_RegionMass;
            ++m_Team[i].m_ActionCnt;
        }

        if(m_Team[i].IsWar() && m_Team[i].m_RegionNearDanger > 0 && m_Region[m_Team[i].m_RegionNearDanger].m_Danger > m_Team[i].m_Strength && m_Team[i].m_ActionCnt < 16)
        {
            ac = m_Team[i].m_ActionList + m_Team[i].m_ActionCnt;
            ac->m_Type = mlat_Defence;
            ac->m_Region = m_Team[i].m_RegionMass;
            ac->m_RegionPathCnt = 1;
            ac->m_RegionPath[0] = m_Team[i].m_RegionMass;
            ++m_Team[i].m_ActionCnt;
        }

        if(!m_Team[i].IsWar() && !m_Team[i].m_Brave && m_Team[i].m_RegionNearDanger > 0 && m_Region[m_Team[i].m_RegionNearDanger].m_Danger * 0.8 > m_Team[i].m_Strength && m_Team[i].m_RegionNearRetreat >= 0)
        {
            ac = m_Team[i].m_ActionList + m_Team[i].m_ActionCnt;
            ac->m_Type = mlat_Retreat;
            ac->m_Region = m_Team[i].m_RegionNearRetreat;
            ac->m_RegionPathCnt = 2;
            ac->m_RegionPath[0] = m_Team[i].m_RegionMass;
            ac->m_RegionPath[1] = ac->m_Region;
            ++m_Team[i].m_ActionCnt;
        }

        if(!m_Team[i].IsWar() && m_Team[i].IsWaitUnion())
        {
            ac = m_Team[i].m_ActionList + m_Team[i].m_ActionCnt;
            ac->m_Type = mlat_Defence;
            ac->m_Region = m_Team[i].m_RegionMass;
            ac->m_RegionPathCnt = 1;
            ac->m_RegionPath[0] = m_Team[i].m_RegionMass;
            ++m_Team[i].m_ActionCnt;
        }

        /*        if(m_Team[i].m_War && m_Team[i].m_RegionNearRetreat>=0 && m_Team[i].m_ActionCnt<16)
                  {
                    m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Type=mlat_Retreat;
                    m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Region=m_Team[i].m_RegionNearRetreat;
                    m_Team[i].m_ActionCnt++;
                }*/
                /*        if(m_Team[i].m_War && m_Team[i].m_RegionMass>=0 && m_Team[i].m_ActionCnt<16)
                {
                            m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Type=mlat_Defence;
                            m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Region=m_Team[i].m_RegionMass;
                            m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt=0;
                            ++m_Team[i].m_ActionCnt;
                        }
                        if(!m_Team[i].m_War && m_Team[i].m_RegionMass>=0 && m_Team[i].m_RadiusMass>50 && m_Team[i].m_ActionCnt<16)
                        {
                            m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Type=mlat_Defence;
                            m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Region=m_Team[i].m_RegionMass;
                            m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_RegionPathCnt=0;
                            ++m_Team[i].m_ActionCnt;
                        }*/
                        /*        if(!m_Team[i].m_War && m_Team[i].m_RegionNearForward>=0 && m_Team[i].m_ActionCnt<16)
                        {
                                    m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Type=mlat_Forward;
                                    m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Region=m_Team[i].m_RegionNearForward;
                                    ++m_Team[i].m_ActionCnt;
                                }*/
                                /*        if(!m_Team[i].m_War && m_Team[i].m_TargetRegion>=0 && m_Team[i].m_ActionCnt<16)
                                {
                                            m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Type=mlat_Forward;
                                            m_Team[i].m_ActionList[m_Team[i].m_ActionCnt].m_Region=m_Team[i].m_TargetRegion;
                                            ++m_Team[i].m_ActionCnt;
                                        }*/
        if(!m_Team[i].IsWar() && m_Team[i].m_RegionMass >= 0 &&
            //m_Team[i].m_TargetRegion >= 0 &&
            //m_Team[i].m_TargetRegion == m_Team[i].m_RegionMass &&
            (m_Region[m_Team[i].m_RegionMass].m_NeutralBuildingCnt > 0 || m_Region[m_Team[i].m_RegionMass].m_EnemyBuildingCnt > 0) &&
            m_Team[i].m_ActionCnt < 16
           )
        {
            //Если другая группа уже захватывает в этом регионе, то вариант не создаем
            for(u = 0; u < m_TeamCnt; ++u)
            {
                if(u == i) continue;
                if(!m_Team[u].IslOk()) continue;
                if(m_Team[u].m_RobotCnt <= 0) continue;
                if(m_Team[u].m_Action.m_Type != mlat_Capture) continue;
                if(m_Team[u].m_Action.m_Region != m_Team[i].m_RegionMass) continue;
                break;
            }

            if(u >= m_TeamCnt)
            {
                ac = m_Team[i].m_ActionList + m_Team[i].m_ActionCnt;
                ac->m_Type = mlat_Capture;
                ac->m_Region = m_Team[i].m_RegionMass;
                ac->m_RegionPathCnt = 1;
                ac->m_RegionPath[0] = m_Team[i].m_RegionMass;
                ++m_Team[i].m_ActionCnt;
            }
        }
    }

    // Находим лучший вариант
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].IslOk()) continue;

        BestAction(i);
        if(m_Team[i].m_ActionCnt > 0)
        {
            m_Team[i].m_Action = m_Team[i].m_ActionList[0];
            m_Team[i].m_ActionTime = g_MatrixMap->GetTime();

            DMTeam(i, m_Team[i].m_Action.m_Type, 1,
                L"m_Strange=<f> m_RegionMass.m_Danger=<f> m_RegionNearDanger.m_Danger=<f>",
                m_Team[i].m_Strength,
                m_Team[i].m_RegionMass < 0 ? 0 : m_Region[m_Team[i].m_RegionMass].m_Danger,
                m_Team[i].m_RegionNearDanger < 0 ? 0 : m_Region[m_Team[i].m_RegionNearDanger].m_Danger
            );

            m_Team[i].m_RoadPath->ClearFast();
            if(m_Team[i].m_Action.m_RegionPathCnt > 1)
            {
#if (defined _DEBUG) &&  !(defined _RELDEBUG)
                if(!g_TestLocal)
                {
                    bool test = false;
                    ms = CMatrixMapStatic::GetFirstLogic();
                    while(ms)
                    {
                        if(ms->IsRobotAlive() && ms->GetSide() == m_Id && ms->AsRobot()->GetTeam() == i && ms == g_TestRobot)
                        {
                            test = true;
                            break;
                        }
                        ms = ms->GetNextLogic();
                    }
                    g_MatrixMap->m_RoadNetwork.FindPathFromRegionPath(m_Team[i].Move(), m_Team[i].m_Action.m_RegionPathCnt, m_Team[i].m_Action.m_RegionPath, m_Team[i].m_RoadPath, test);
                }
#else
                g_MatrixMap->m_RoadNetwork.FindPathFromRegionPath(m_Team[i].Move(), m_Team[i].m_Action.m_RegionPathCnt, m_Team[i].m_Action.m_RegionPath, m_Team[i].m_RoadPath);
#endif
            }

            //g_MatrixMap->m_RoadNetwork.FindPathInRegionInit();
            //m_Team[i].m_Action.m_RegionPathCnt=g_MatrixMap->m_RoadNetwork.FindPathInRegionRun(m_Team[i].m_RegionMass,m_Team[i].m_Action.m_Region,m_Team[i].m_Action.m_RegionPath,16);
        }
        else
        {
            m_Team[i].m_Action.m_Type = mlat_None;
            m_Team[i].m_ActionTime = g_MatrixMap->GetTime();
        }
    }

    //Перегруппируем один раз за такт, так как статистика становится не правильной.
    int changeok = false;
    //Если отступаем, то пытаемся найти подмогу
    for(i = 0; i < m_TeamCnt; ++i)
    {
        if(m_Team[i].IslOk()) continue;
        if(m_Team[i].m_RobotCnt <= 0) continue;
        if(m_Team[i].m_Action.m_Type != mlat_Retreat) continue;
        if(m_Team[i].m_RegionFarDanger < 0) continue;

        //Создаем список групп, которые можно присоединить
        int gi[MAX_LOGIC_GROUP];
        int gid[MAX_LOGIC_GROUP];
        int gicnt = 0;

        for(u = 0; u < MAX_LOGIC_GROUP; ++u)
        {
            if(m_LogicGroup[u].RobotsCnt() <= 0) continue;
            if(m_LogicGroup[u].m_Team == i) continue;
            if(m_LogicGroup[u].IsWar()) continue;
            if(m_LogicGroup[u].m_Action.m_Type == mlat_Attack) continue;


            ms = CMatrixMapStatic::GetFirstLogic();
            while(ms)
            {
                if(ms->IsRobotAlive() && ms->GetSide() == m_Id && GetGroupLogic(ms) == u) break;
                ms = ms->GetNextLogic();
            }

            if(!ms) continue;

            int rr = GetRegion(ms);
            if(rr != m_Team[i].m_Action.m_Region)
            {
                int path[8];
                gid[gicnt] = g_MatrixMap->m_RoadNetwork.FindPathInRegionRun(m_Team[i].Move(), rr, m_Team[i].m_Action.m_Region, path, 8, false);
                if(gid[gicnt] <= 0) continue;

                for(t = 0; t < gid[gicnt]; ++t) //Врагов на пути не должно быть
                {
                    if(m_Region[path[t]].m_EnemyRobotCnt) break;
                    if(t >= 4 && m_Region[path[t]].m_Danger > 0) break;
                }
                if(t < gid[gicnt]) continue;

            }
            else gid[gicnt] = 0;

            if(gid[gicnt] >= 5) continue; //Слишком далеко

            gi[gicnt] = u;
            ++gicnt;
        }

        // Сортируем по дальности
        for(u = 0; u < gicnt - 1; ++u)
        {
            for(t = u + 1; t < gicnt; ++t)
            {
                if(gid[t] < gid[u])
                {
                    k = gid[t]; gid[t] = gid[u]; gid[u] = k;
                    k = gi[t]; gi[t] = gi[u]; gi[u] = k;
                }
            }
        }

        // Объединяем
        for(u = 0; u < gicnt; ++u)
        {
            ms = CMatrixMapStatic::GetFirstLogic();
            while(ms)
            {
                if(ms->IsRobotAlive() && ms->GetSide() == m_Id && GetGroupLogic(ms) == gi[u])
                {
                    DMSide(L"Robot=<b=16><u> Change team for defence <b=10><i>-><i>", dword(ms), k, i);
                    //--m_Team[ms->AsRobot()->GetTeam()].m_RobotCnt;
                    //m_Team[ms->AsRobot()->GetTeam()].m_Strength -= ms->AsRobot()->GetStrength();
                    ms->AsRobot()->SetGroupLogic(-1);
                    ms->AsRobot()->SetTeam(i);
                    m_Team[i].m_Strength += ms->AsRobot()->GetStrength();
                }
                ms = ms->GetNextLogic();
            }
            if(m_Team[i].m_Strength >= m_Team[i].m_RegionFarDanger) break;
        }

        changeok = true;
        m_LastTeamChange = g_MatrixMap->GetTime();
        break;
    }

    //Объединяем, если группа дерется, сил у нее мало, а рядом группа, которая не воюет
    if(!changeok && (g_MatrixMap->GetTime() - m_LastTeamChange) > 3000)
    {
        for(i = 0; i < m_TeamCnt; ++i)
        {
            //if(m_Team[i].m_lOk) continue;
            if(m_Team[i].m_RobotCnt <= 0) continue;
            if(!m_Team[i].IsWar()) continue;
            if(m_Team[i].m_RegionMass < 0) continue;
            if(m_Team[i].m_RegionFarDanger < 0) continue;
            if(m_Region[m_Team[i].m_RegionFarDanger].m_Danger < m_Team[i].m_Strength * 0.7f) continue;

            for(u = 0; u < m_TeamCnt; ++u)
            {
                if(i == u) continue;
                //if(m_Team[u].m_lOk) continue;
                if(m_Team[u].m_RobotCnt <= 0) continue;
                if(m_Team[u].IsWar()) continue;
                if(m_Team[u].m_RegionMass < 0) continue;
                if(m_Team[u].m_Action.m_Type == mlat_Capture) continue;
                if(m_Team[i].m_RegionMass != m_Team[u].m_RegionMass && !g_MatrixMap->m_RoadNetwork.IsNearestRegion(m_Team[i].m_RegionMass, m_Team[u].m_RegionMass)) continue;

                break;
            }
            if(u >= m_TeamCnt) continue;

            ms = CMatrixMapStatic::GetFirstLogic();
            while(ms)
            {
                if(ms->IsRobotAlive() && ms->GetSide() == m_Id && ms->AsRobot()->GetTeam() == u)
                {
                    DMSide(L"Robot=<b=16><u> Change team war <b=10><i>-><i>", dword(ms), k, i);
                    ms->AsRobot()->SetTeam(i);
                    ms->AsRobot()->SetGroupLogic(-1);
                }
                ms = ms->GetNextLogic();
            }

            changeok = true;
            m_LastTeamChange = g_MatrixMap->GetTime();
            break;
        }
    }

    // Объединяем группы если они близко, и если есть опасность для обеих групп
    if(!changeok && (g_MatrixMap->GetTime() - m_LastTeamChange) > 3000)
    {
        for(i = 0; i < m_TeamCnt; ++i)
        {
            if(m_Team[i].m_RobotCnt <= 0) continue;
            if(m_Team[i].IsWar()) continue;
            if(m_Team[i].m_RegionFarDanger < 0) continue;
            if(m_Team[i].m_RegionMass < 0) continue;
            if(m_Region[m_Team[i].m_RegionFarDanger].m_Danger < m_Team[i].m_Strength) continue;
            if(m_Team[i].m_Action.m_Type == mlat_Capture) continue;

            for(u = 0; u < m_TeamCnt; ++u)
            {
                if(u == i) continue;
                if(m_Team[u].m_RobotCnt <= 0) continue;
                if(m_Team[u].IsWar()) continue;
                if(m_Team[u].m_RegionFarDanger < 0) continue;
                if(m_Team[u].m_RegionMass < 0) continue;
                if(m_Region[m_Team[u].m_RegionFarDanger].m_Danger < m_Team[u].m_Strength) continue;
                if(m_Team[u].m_Action.m_Type == mlat_Capture) continue;

                if(m_Team[i].m_RegionMass != m_Team[u].m_RegionMass && !g_MatrixMap->m_RoadNetwork.IsNearestRegion(m_Team[i].m_RegionMass, m_Team[u].m_RegionMass)) continue;

                break;
            }
            if(u >= m_TeamCnt) continue;

            ms = CMatrixMapStatic::GetFirstLogic();
            while(ms)
            {
                if(ms->IsRobotAlive() && ms->GetSide() == m_Id && ms->AsRobot()->GetTeam() == u)
                {
                    DMSide(L"Robot=<b=16><u> Change team union group <b=10><i>-><i>", dword(ms), k, i);
                    ms->AsRobot()->SetTeam(i);
                    ms->AsRobot()->SetGroupLogic(-1);
                }
                ms = ms->GetNextLogic();
            }

            m_Team[i].m_Action.m_Type = mlat_None;
            m_Team[i].m_ActionTime = g_MatrixMap->GetTime();

            changeok = true;
            m_LastTeamChange = g_MatrixMap->GetTime();
            break;
        }
    }

    // Перераспределяем роботов для пустой группы, если нет опасности
    if(!changeok && (g_MatrixMap->GetTime() - m_LastTeamChange) > 3000)
    {
        for(i = 0; i < m_TeamCnt; ++i)
        {
            if(m_Team[i].m_RobotCnt > 0) continue;

            // Находим подходящую команду с максимальным кол-во роботов
            k = -1;
            for(u = 0; u < m_TeamCnt; ++u)
            {
                if(m_Team[u].m_RobotCnt < 2) continue; //Делить нечего
                if(m_Team[u].m_Action.m_Type != mlat_Forward && m_Team[u].m_Action.m_Type != mlat_Capture) continue; // Группа занята более важными делами
                if(m_Team[u].m_RegionFarDanger >= 0) continue;
                if(m_Team[u].IsRegroupOnlyAfterWar()) continue;

                if(k < 0) k = u;
                else if(m_Team[u].m_RobotCnt > m_Team[k].m_RobotCnt) k = u;
            }
            if(k < 0) break; //Делить нечего

            ClearTeam(i);

            //Делим по роботам
            if(m_Team[k].m_GroupCnt == 1)
            {
                u = m_Team[k].m_RobotCnt / 2;

                ms = CMatrixMapStatic::GetFirstLogic();
                while(ms && u)
                {
                    if(ms->IsRobotAlive() && ms->GetSide() == m_Id && ms->AsRobot()->GetTeam() == k)
                    {
                        DMSide(L"Robot=<b=16><u> Change team to empty <b=10><i>-><i>", dword(ms), k, i);
                        ms->AsRobot()->SetTeam(i);
                        ms->AsRobot()->SetGroupLogic(-1);
                        --u;
                    }
                    ms = ms->GetNextLogic();
                }

            }
            else // Делим по группам
            {
                u = m_Team[k].m_GroupCnt / 2;

                for(t = 0; t < MAX_LOGIC_GROUP && u; ++t)
                {
                    if(m_LogicGroup[t].RobotsCnt() <= 0) continue;
                    if(m_LogicGroup[t].m_Team != k) continue;

                    ms = CMatrixMapStatic::GetFirstLogic();
                    while(ms)
                    {
                        if(ms->IsRobotAlive() && ms->GetSide() == m_Id && GetGroupLogic(ms) == t)
                        {
                            DMSide(L"Robot=<b=16><u> Change team to empty <b=10><i>-><i>", dword(ms), k, i);
                            ms->AsRobot()->SetTeam(i);
                            ms->AsRobot()->SetGroupLogic(-1);
                        }
                        ms = ms->GetNextLogic();
                    }
                    --u;
                }
            }

            changeok = true;
            m_LastTeamChange = g_MatrixMap->GetTime();
            break;
        }
    }

    // Перераспределяем если команды близко, но численность роботов не равная, и нет опасности
    if(!changeok && (g_MatrixMap->GetTime() - m_LastTeamChange) > 3000)
    {
        for(i = 0; i < m_TeamCnt; ++i)
        {
            if(m_Team[i].m_RobotCnt <= 0) continue;
            if(m_Team[i].m_Action.m_Type != mlat_Forward && m_Team[i].m_Action.m_Type != mlat_Capture) continue;
            if(m_Team[i].m_RegionMass < 0) continue;
            if(m_Team[i].m_RegionFarDanger >= 0) continue;
            if(m_Team[i].IsRegroupOnlyAfterWar()) continue;

            k = -1;
            for(u = 0; u < m_TeamCnt; ++u)
            {
                if(i == u) continue;
                if(m_Team[u].m_RobotCnt <= 0) continue;
                if(m_Team[u].m_RegionMass < 0) continue;
                if(m_Team[u].m_Action.m_Type != mlat_Forward && m_Team[u].m_Action.m_Type != mlat_Capture) continue;
                if(m_Team[u].m_RegionMass != m_Team[i].m_RegionMass && !g_MatrixMap->m_RoadNetwork.IsNearestRegion(m_Team[u].m_RegionMass, m_Team[i].m_RegionMass)) continue;
                if(m_Team[u].m_RegionFarDanger >= 0) continue;
                if(m_Team[u].IsRegroupOnlyAfterWar()) continue;

                if(k < 0) k = u;
                else if(m_Team[u].m_RobotCnt > m_Team[k].m_RobotCnt) k = u;
            }
            if(k < 0) continue;

            u = m_Team[u].m_RobotCnt - m_Team[i].m_RobotCnt;
            if(u < 2) continue;
            u = u / 2;

            ms = CMatrixMapStatic::GetFirstLogic();
            while(ms && u)
            {
                if(ms->IsRobotAlive() && ms->GetSide() == m_Id && ms->AsRobot()->GetTeam() == k)
                {
                    DMSide(L"Robot=<b=16><u> Change team if nerest <b=10><i>-><i>", dword(ms), k, i);
                    ms->AsRobot()->SetTeam(i);
                    ms->AsRobot()->SetGroupLogic(-1);
                    --u;
                }
                ms = ms->GetNextLogic();
            }

            m_Team[i].m_Action.m_Type = mlat_None; // Отменяем приказ, так как он может существенно поменяться, так как место и положение роботов изменилось
            m_Team[i].m_ActionTime = g_MatrixMap->GetTime();

            changeok = true;
            m_LastTeamChange = g_MatrixMap->GetTime();
            break;
        }
    }

    // Объединяем радом стоящие группы, которые находятся в защите. Разбить новую группу можно только после войны.
    if(!changeok && (g_MatrixMap->GetTime() - m_LastTeamChange) > 3000)
    {
        for(i = 0; i < m_TeamCnt; ++i)
        {
            if(m_Team[i].m_RobotCnt <= 0) continue;
            if(m_Team[i].IsWaitUnion()) continue;
            if(m_Team[i].m_Action.m_Type != mlat_Defence) continue;
            if(m_Team[i].m_RegionMass < 0) continue;

            for(u = 0; u < m_TeamCnt; ++u)
            {
                if (i == u) continue;
                if (m_Team[u].m_RobotCnt <= 0) continue;
                if (m_Team[u].IsWaitUnion()) continue;
                if (m_Team[u].m_Action.m_Type != mlat_Defence) continue;
                if (m_Team[u].m_RegionMass < 0) continue;
                if (m_Team[u].IsRegroupOnlyAfterWar()) continue;
                if (!(m_Team[i].m_Action.m_Region == m_Team[u].m_Action.m_Region ||
                    g_MatrixMap->m_RoadNetwork.IsNearestRegion(m_Team[i].m_Action.m_Region, m_Team[u].m_Action.m_Region) ||
                    CanMoveNoEnemy(m_Team[i].Move() | m_Team[u].Move(), m_Team[i].m_Action.m_Region, m_Team[u].m_Action.m_Region))) continue;

                m_Team[u].m_RobotCnt = 0;
                m_Team[i].SetRegroupOnlyAfterWar(true);

                ms = CMatrixMapStatic::GetFirstLogic();
                while (ms)
                {
                    if (ms->IsRobotAlive() && ms->GetSide() == m_Id && ms->AsRobot()->GetTeam() == u)
                    {
                        DMSide(L"Robot=<b=16><u> Change team union defence group <b=10><i>-><i>", dword(ms), u, i);
                        ms->AsRobot()->SetTeam(i);
                        ms->AsRobot()->SetGroupLogic(-1);
                    }
                    ms = ms->GetNextLogic();
                }
            }
        }
    }

    //dword it2=timeGetTime();
    //DM(L"TactHL",CWStr().Format(L"<i>",it2-it1).Get());

    //#if (defined _DEBUG) &&  !(defined _RELDEBUG)
    //if(m_RobotsCnt>0) DM(CWStr().Format(L"Res2 Side=<i>",m_Id).Get(),CWStr().Format(L"<i> <i> <i> <i>",m_Resources[0],m_Resources[1],m_Resources[2],m_Resources[3]).Get());
    //#endif

    //Отображаем
#if (defined _DEBUG) &&  !(defined _RELDEBUG) && !(defined _DISABLE_AI_HELPERS)
//    if(m_Id==PLAYER_SIDE)
    for (i = 0; i < m_TeamCnt; ++i)
    {
        CHelper::DestroyByGroup(dword(m_Team + i));

        if (m_Team[i].m_Action.m_Type == mlat_None) continue;
        if (m_Team[i].m_RobotCnt <= 0) continue;

        dword colors[3] = { 0xffff0000,0xff00ff00,0xff0000ff };

        CPoint tp = g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_Action.m_Region].m_Center;
        float z = g_MatrixMap->GetZ(GLOBAL_SCALE_MOVE * tp.x, GLOBAL_SCALE_MOVE * tp.y);
        CHelper::Create(0, dword(m_Team + i))->Cone(D3DXVECTOR3(GLOBAL_SCALE_MOVE * tp.x, GLOBAL_SCALE_MOVE * tp.y, z), D3DXVECTOR3(GLOBAL_SCALE_MOVE * tp.x, GLOBAL_SCALE_MOVE * tp.y, z + 30 - 2.0f * i), 4.0f + 1.0f * i, 4.0f + 1.0f * i, colors[i], colors[i], 5);

        //// Отображаем путь по регионам
        //for(u=1; u<m_Team[i].m_Action.m_RegionPathCnt; ++u) {
        //    tp=g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_Action.m_RegionPath[u-1]].m_Center;
        //    D3DXVECTOR3 v1=D3DXVECTOR3(GLOBAL_SCALE_MOVE*tp.x,GLOBAL_SCALE_MOVE*tp.y,g_MatrixMap->GetZ(GLOBAL_SCALE_MOVE*tp.x,GLOBAL_SCALE_MOVE*tp.y)+100.0f);
        //    tp=g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_Action.m_RegionPath[u]].m_Center;
        //    D3DXVECTOR3 v2=D3DXVECTOR3(GLOBAL_SCALE_MOVE*tp.x,GLOBAL_SCALE_MOVE*tp.y,g_MatrixMap->GetZ(GLOBAL_SCALE_MOVE*tp.x,GLOBAL_SCALE_MOVE*tp.y)+100.0f);

        //    CHelper::Create(0,dword(m_Team+i))->Line(v1,v2,colors[i],colors[i]);
        //}
        //// Отображаем путь по дорогам
        //for(u=1; u<m_Team[i].m_RoadPath->m_Header[0].m_Cnt; ++u) {
        //    tp=m_Team[i].m_RoadPath->m_Modules[u-1].m_Crotch->m_Center;
        //    D3DXVECTOR3 v1=D3DXVECTOR3(GLOBAL_SCALE_MOVE*tp.x,GLOBAL_SCALE_MOVE*tp.y,g_MatrixMap->GetZ(GLOBAL_SCALE_MOVE*tp.x,GLOBAL_SCALE_MOVE*tp.y)+80.0f);
        //    tp=m_Team[i].m_RoadPath->m_Modules[u].m_Crotch->m_Center;
        //    D3DXVECTOR3 v2=D3DXVECTOR3(GLOBAL_SCALE_MOVE*tp.x,GLOBAL_SCALE_MOVE*tp.y,g_MatrixMap->GetZ(GLOBAL_SCALE_MOVE*tp.x,GLOBAL_SCALE_MOVE*tp.y)+80.0f);

        //    CHelper::Create(0,dword(m_Team+i))->Cone(v1,v2,1.0f,1.0f,colors[i],colors[i],5);
        //}
        //// Отображаем следующий регион
        //if(m_Team[i].m_RegionNext>=0) {
        //    tp=g_MatrixMap->m_RoadNetwork.m_Region[m_Team[i].m_RegionNext].m_Center;
        //    D3DXVECTOR3 v1=D3DXVECTOR3(GLOBAL_SCALE_MOVE*tp.x,GLOBAL_SCALE_MOVE*tp.y,g_MatrixMap->GetZ(GLOBAL_SCALE_MOVE*tp.x,GLOBAL_SCALE_MOVE*tp.y));

        //    CHelper::Create(0,dword(m_Team+i))->Line(v1,D3DXVECTOR3(v1.x,v1.y,v1.z+100.0f),colors[i],colors[i]);
        //}
    }
#endif
    /*#if (defined _DEBUG) &&  !(defined _RELDEBUG)
        CMatrixMapStatic * obj = CMatrixMapStatic::GetFirstLogic();
        while(obj) {
            if(obj->IsRobotAlive() && obj->GetSide()==m_Id) {
                tp=PLPlacePos(obj->AsRobot());
                if(tp.x>=0) {
                    D3DXVECTOR3 v1,v2,v3,v4;
                    v1.x=tp.x*GLOBAL_SCALE_MOVE; v1.y=tp.y*GLOBAL_SCALE_MOVE; v1.z=g_MatrixMap->GetZ(v1.x,v1.y)+1.0f;
                    v2.x=(tp.x+4)*GLOBAL_SCALE_MOVE; v2.y=tp.y*GLOBAL_SCALE_MOVE; v2.z=g_MatrixMap->GetZ(v2.x,v2.y)+1.0f;
                    v3.x=(tp.x+4)*GLOBAL_SCALE_MOVE; v3.y=(tp.y+4)*GLOBAL_SCALE_MOVE; v3.z=g_MatrixMap->GetZ(v3.x,v3.y)+1.0f;
                    v4.x=(tp.x)*GLOBAL_SCALE_MOVE; v4.y=(tp.y+4)*GLOBAL_SCALE_MOVE; v4.z=g_MatrixMap->GetZ(v4.x,v4.y)+1.0f;

                    CHelper::DestroyByGroup(dword(obj)+1);
                    CHelper::Create(10,dword(obj)+1)->Triangle(v1,v2,v3,0x8000ff00);
                    CHelper::Create(10,dword(obj)+1)->Triangle(v1,v3,v4,0x8000ff00);
                }
    //            D3DXVECTOR2 v=GetWorldPos(obj);
    //            CHelper::DestroyByGroup(dword(obj)+2);
    //            CHelper::Create(10,dword(obj)+2)->Cone(D3DXVECTOR3(v.x,v.y,0),D3DXVECTOR3(v.x,v.y,40),float(obj->AsRobot()->GetMinFireDist()),float(obj->AsRobot()->GetMinFireDist()),0x80ffff00,0x80ffff00,20);
    //            CHelper::Create(10,dword(obj)+2)->Cone(D3DXVECTOR3(v.x,v.y,0),D3DXVECTOR3(v.x,v.y,40),float(obj->AsRobot()->GetMaxFireDist()),float(obj->AsRobot()->GetMaxFireDist()),0x80ff0000,0x80ff0000,20);
            }
            obj = obj->GetNextLogic();
        }
    #endif*/

    //    GiveOrder();
}

int CMatrixSideUnit::FindNearRegionWithUTR(int from, int* exclude_list, int exclude_cnt, dword flags) // 1-our 2-netral 4-enemy 8-base 16-building 32-robot 64-cannon
{
    int i, u, t, next, cnt = 0, sme = 0, level = 1;

    for (i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i)
    {
        m_Region[i].m_Data = 0;
    }

    m_Region[from].m_Data = level;
    m_RegionIndex[cnt] = from;
    ++cnt;

    next = cnt;
    ++level;
    while (sme < cnt)
    {
        for (i = 0; i < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++i)
        {
            u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[i];
            if (m_Region[u].m_Data > 0) continue;

            for (t = 0; t < exclude_cnt; ++t)
            {
                if (u == exclude_list[t]) break;
            }
            if (t >= exclude_cnt)
            {
                if (flags & 1)
                {
                    if ((flags & 8) && (m_Region[u].m_OurBaseCnt)) return u;
                    else if ((flags & 16) && (m_Region[u].m_OurBuildingCnt)) return u;
                    else if ((flags & 32) && (m_Region[u].m_OurRobotCnt)) return u;
                    else if ((flags & 64) && (m_Region[u].m_OurCannonCnt)) return u;
                }
                if (flags & 2)
                {
                    if ((flags & 8) && (m_Region[u].m_NeutralBaseCnt)) return u;
                    else if ((flags & 16) && (m_Region[u].m_NeutralBuildingCnt)) return u;
                    //else if((flags & 32) && (m_Region[u].m_NeutralRobotCnt)) return u;
                    else if ((flags & 64) && (m_Region[u].m_NeutralCannonCnt)) return u;
                }
                if (flags & 4)
                {
                    if ((flags & 8) && (m_Region[u].m_EnemyBaseCnt)) return u;
                    else if ((flags & 16) && (m_Region[u].m_EnemyBuildingCnt)) return u;
                    else if ((flags & 32) && (m_Region[u].m_EnemyRobotCnt)) return u;
                    else if ((flags & 64) && (m_Region[u].m_EnemyCannonCnt)) return u;
                }
            }

            m_RegionIndex[cnt] = u;
            ++cnt;
            m_Region[u].m_Data = level;
        }
        ++sme;
        if (sme >= next)
        {
            next = cnt;
            ++level;
        }
    }
    return -1;
}

int CMatrixSideUnit::CompareAction(int team, SMatrixLogicAction* a1, SMatrixLogicAction* a2)
{
    DTRACE();

    int scale = 1;
    if (a1->m_Type > a2->m_Type)
    {
        SMatrixLogicAction* temp = a1; a1 = a2; a2 = temp;
        scale = -1;
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    if (a1->m_Type == mlat_Defence && a2->m_Type == mlat_Defence)
    {
        if (m_Region[a1->m_Region].m_Danger != m_Region[a2->m_Region].m_Danger)
        {
            if (m_Region[a1->m_Region].m_Danger < m_Region[a2->m_Region].m_Danger) return 1 * scale;
            else return -1 * scale;
        }
        return 0 * scale;
    }
    else if (a1->m_Type == mlat_Attack && a2->m_Type == mlat_Attack)
    {
        return 0 * scale;
    }
    else if (a1->m_Type == mlat_Forward && a2->m_Type == mlat_Forward)
    {
        if (a1->m_RegionPathCnt != a2->m_RegionPathCnt)
        {
            if (a1->m_RegionPathCnt > a2->m_RegionPathCnt) return -1 * scale;
            else return 1 * scale;
        }
        if (m_Region[a1->m_Region].m_EnemyBuildingCnt != m_Region[a2->m_Region].m_EnemyBuildingCnt)
        {
            if (m_Region[a1->m_Region].m_EnemyBuildingCnt < m_Region[a2->m_Region].m_EnemyBuildingCnt) return -1 * scale;
            else return 1 * scale;
        }
        if (m_Region[a1->m_Region].m_NeutralBuildingCnt != m_Region[a2->m_Region].m_NeutralBuildingCnt)
        {
            if (m_Region[a1->m_Region].m_NeutralBuildingCnt < m_Region[a2->m_Region].m_NeutralBuildingCnt) return -1 * scale;
            else return 1 * scale;
        }
        return 0 * scale;
    }
    else if (a1->m_Type == mlat_Retreat && a2->m_Type == mlat_Retreat)
    {
        return 0 * scale;
    }
    else if (a1->m_Type == mlat_Capture && a2->m_Type == mlat_Capture)
    {
        return 0 * scale;
    }
    else if (a1->m_Type == mlat_Intercept && a2->m_Type == mlat_Intercept)
    {
        return 0 * scale;
        ///////////////////////////////////////////////////////////////////////////////////////
    }
    else if (a1->m_Type == mlat_Defence && a2->m_Type == mlat_Attack)
    {
        if (!m_Team[team].IsWar() && m_Team[team].IsWaitUnion()) return 1 * scale;

        if (m_Team[team].m_RegionNearDanger >= 0 && m_Region[m_Team[team].m_RegionNearDanger].m_Danger > m_Team[team].m_Strength) return 1 * scale;
        return -1 * scale;
    }
    else if (a1->m_Type == mlat_Defence && a2->m_Type == mlat_Forward)
    {
        if (!m_Team[team].IsWar() && m_Team[team].IsWaitUnion()) return 1 * scale;
        if (m_Team[team].IsWar()) return -1 * scale;
        if (m_Region[a2->m_Region].m_Danger > 0)
        {
            if (m_Team[team].m_RegionNearDanger >= 0 && m_Region[m_Team[team].m_RegionNearDanger].m_Danger > m_Team[team].m_Strength) return -1 * scale;
            return 1 * scale;
        }
        else
        {
            if (m_Team[team].m_RegionNearDanger<0 || m_Region[m_Team[team].m_RegionNearDanger].m_Danger * 0.5>m_Team[team].m_Strength) return 1 * scale;
            return -1 * scale;
        }
    }
    else if (a1->m_Type == mlat_Defence && a2->m_Type == mlat_Retreat)
    {
        if (m_Team[team].IsWar()) return -1 * scale;
        if (m_Team[team].m_RegionNearDanger >= 0 && m_Region[m_Team[team].m_RegionNearDanger].m_Danger * 0.5f > m_Team[team].m_Strength) return 1 * scale;
        return -1 * scale;
    }
    else if (a1->m_Type == mlat_Defence && a2->m_Type == mlat_Capture)
    {
        if (!m_Team[team].IsWar() && m_Team[team].IsWaitUnion()) return 1 * scale;

        if (m_Team[team].IsWar()) return -1 * scale;
        if (m_Region[a2->m_Region].m_Danger > 0)
        {
            if (m_Team[team].m_RegionNearDanger >= 0 && m_Region[m_Team[team].m_RegionNearDanger].m_Danger > m_Team[team].m_Strength) return 1 * scale;
            return -1 * scale;
        }
        else
        {
            if (m_Team[team].m_RegionNearDanger >= 0 && m_Region[m_Team[team].m_RegionNearDanger].m_Danger * 0.5 > m_Team[team].m_Strength) return 1 * scale;
            return -1 * scale;
        }
    }
    else if (a1->m_Type == mlat_Defence && a2->m_Type == mlat_Intercept)
    {
        return -1 * scale;
    }
    else if (a1->m_Type == mlat_Attack && a2->m_Type == mlat_Forward)
    {
        return 1 * scale;
    }
    else if (a1->m_Type == mlat_Attack && a2->m_Type == mlat_Retreat)
    {
        if (m_Team[team].m_Strength >= m_Region[m_Team[team].m_RegionNearDanger].m_Danger * 0.8)
        {
            return 1 * scale;
        }
        else
        {
            return -1 * scale;
        }
    }
    else if (a1->m_Type == mlat_Attack && a2->m_Type == mlat_Capture)
    {
        return 1 * scale;
    }
    else if (a1->m_Type == mlat_Attack && a2->m_Type == mlat_Intercept)
    {
        return 1 * scale;
    }
    else if (a1->m_Type == mlat_Forward && a2->m_Type == mlat_Retreat)
    {
        if (m_Team[team].m_Strength >= m_Region[m_Team[team].m_RegionNearDanger].m_Danger * 0.8)
        {
            return 1 * scale;
        }
        else
        {
            return -1 * scale;
        }
    }
    else if (a1->m_Type == mlat_Forward && a2->m_Type == mlat_Capture)
    {
        return -1 * scale;
    }
    else if (a1->m_Type == mlat_Forward && a2->m_Type == mlat_Intercept)
    {
        return 1 * scale;
    }
    else if (a1->m_Type == mlat_Retreat && a2->m_Type == mlat_Capture)
    {
        return 1 * scale;
    }
    else if (a1->m_Type == mlat_Retreat && a2->m_Type == mlat_Intercept)
    {
        return 1 * scale;
    }
    else if (a1->m_Type == mlat_Capture && a2->m_Type == mlat_Intercept)
    {
        return 1 * scale;
    }

    return 0;
}

void CMatrixSideUnit::BestAction(int team)
{
    if(m_Team[team].m_ActionCnt <= 1) return;

    int k = 0;
    for(int u = 1; u < m_Team[team].m_ActionCnt; ++u)
    {
        if(CompareAction(team, m_Team[team].m_ActionList + k, m_Team[team].m_ActionList + u) < 0)
        {
            k = u;
        }
    }
    m_Team[team].m_ActionList[0] = m_Team[team].m_ActionList[k];
    m_Team[team].m_ActionCnt = 1;
}

void CMatrixSideUnit::LiveAction(int team)
{
    if(m_Team[team].m_ActionCnt >= 16) BestAction(team);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CMatrixSideUnit::TactTL()
{
    int i, u, x, y;
    CMatrixMapStatic* obj;
    CPoint tp;

    CMatrixRobotAI* rl[MAX_ROBOTS_ON_MAP];    //Список роботов на карте
    int ally_robots_cnt;                         //Кол-во роботов на карте
    int team;

    if(m_LastTactTL != 0 && (g_MatrixMap->GetTime() - m_LastTactTL) < 10) return;
    m_LastTactTL = g_MatrixMap->GetTime();

    //Для всех мест рассчитываем коэффициент вражеских объектов в зоне поражения
    if(m_LastTactUnderfire == 0 || (g_MatrixMap->GetTime() - m_LastTactUnderfire) > 500)
    {
        m_LastTactUnderfire = g_MatrixMap->GetTime();

        SMatrixPlace* place = g_MatrixMap->m_RoadNetwork.m_Place;
        for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_PlaceCnt; ++i, ++place) place->m_Underfire = 0;

        obj = CMatrixMapStatic::GetFirstLogic();
        while(obj)
        {
            if(obj->IsUnitAlive() && obj->GetSide() != m_Id)
            {
                tp = GetMapPos(obj);
                CRect rect(1000000000, 1000000000, -1000000000, -1000000000);
                rect.left = min(rect.left, tp.x);
                rect.top = min(rect.top, tp.y);
                rect.right = max(rect.right, tp.x + ROBOT_MOVECELLS_PER_SIZE);
                rect.bottom = max(rect.bottom, tp.y + ROBOT_MOVECELLS_PER_SIZE);

                tp.x += ROBOT_MOVECELLS_PER_SIZE >> 1;
                tp.y += ROBOT_MOVECELLS_PER_SIZE >> 1;

                int firedist = 0;
                int firedist2 = 0;
                if(obj->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
                {
                    firedist = Float2Int(((CMatrixRobotAI*)(obj))->GetMaxFireDist() + GLOBAL_SCALE_MOVE);
                    firedist2 = Float2Int(((CMatrixRobotAI*)(obj))->GetMinFireDist() + GLOBAL_SCALE_MOVE);
                }
                else if(obj->GetObjectType() == OBJECT_TYPE_TURRET)
                {
                    firedist = Float2Int(((CMatrixTurret*)(obj))->GetFireRadius() + GLOBAL_SCALE_MOVE);
                    firedist2 = firedist;
                }

                firedist = firedist / int(GLOBAL_SCALE_MOVE);
                firedist2 = firedist2 / int(GLOBAL_SCALE_MOVE);

                CRect plr = g_MatrixMap->m_RoadNetwork.CorrectRectPL(CRect(rect.left - firedist, rect.top - firedist, rect.right + firedist, rect.bottom + firedist));

                firedist *= firedist;
                firedist2 *= firedist2;

                SMatrixPlaceList* plist = g_MatrixMap->m_RoadNetwork.m_PLList + plr.left + plr.top * g_MatrixMap->m_RoadNetwork.m_PLSizeX;
                for(y = plr.top;y < plr.bottom; ++y, plist += g_MatrixMap->m_RoadNetwork.m_PLSizeX - (plr.right - plr.left))
                {
                    for(x = plr.left; x < plr.right; ++x, ++plist)
                    {
                        SMatrixPlace* place = g_MatrixMap->m_RoadNetwork.m_Place + plist->m_Sme;
                        for(u = 0; u < plist->m_Cnt; ++u, ++place)
                        {
                            int pcx = place->m_Pos.x + int(ROBOT_MOVECELLS_PER_SIZE / 2); //Center place
                            int pcy = place->m_Pos.y + int(ROBOT_MOVECELLS_PER_SIZE / 2);

                            int d = (POW2(tp.x - pcx) + POW2(tp.y - pcy));
                            if(firedist >= d) place->m_Underfire++;
                            if(firedist2 >= d) place->m_Underfire++;
                        }
                    }
                }
            }
            obj = obj->GetNextLogic();
        }
    }

    //Проверяем, завершился ли приказ, который нельзя прервать
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
        {
            if(GetEnv(obj)->m_OrderNoBreak && obj->AsRobot()->CanBreakOrder())
            {
                GetEnv(obj)->m_OrderNoBreak = false;
                GetEnv(obj)->m_Place = -1;
            }

            //Если не синхронизировано место и то куда идем то место отчищаем
            if(GetEnv(obj)->m_Place >= 0 && obj->AsRobot()->CanBreakOrder() && !obj->AsRobot()->GetCaptureBuilding() && !IsToPlace(obj->AsRobot(), GetEnv(obj)->m_Place))
            {
                GetEnv(obj)->m_Place = -1;
            }
        }
        obj = obj->GetNextLogic();
    }

    for(int g = 0; g < MAX_LOGIC_GROUP; ++g)
    {
        if(m_LogicGroup[g].RobotsCnt() <= 0) continue;
        if(m_LogicGroup[g].IsWar()) WarTL(g);
        else RepairTL(g);
    }

    //Раздаём персональные приказы роботам в логических группах доминаторов
    for(int g = 0; g < MAX_LOGIC_GROUP; ++g)
    {
        //Пропускаем группы без роботов
        if(m_LogicGroup[g].RobotsCnt() <= 0) continue;

        //Составляем список роботов в группе (для последующих быстрых переборов)
        ally_robots_cnt = 0;
        obj = CMatrixMapStatic::GetFirstLogic();
        while(obj)
        {
            if(obj->IsRobotAlive() && obj->GetSide() == m_Id && GetGroupLogic(obj) == g)
            {
                rl[ally_robots_cnt] = (CMatrixRobotAI*)obj;
                //rl[ally_robots_cnt]->CalcStrength();
                ++ally_robots_cnt;
            }
            obj = obj->GetNextLogic();
        }
        if(ally_robots_cnt <= 0) continue;

        team = rl[0]->GetTeam();
        //if(team < 0 || team >= m_TeamCnt) __asm int 3;
        //ASSERT(team >= 0);
        m_LogicGroup[g].RobotsCnt(ally_robots_cnt);

        bool order_ok = true; //Приказ не изменился, данные корректны
        //bool up_change_order =! ((m_LogicGroup[g].m_Action.m_Type == m_Team[team].m_Action.m_Type) && (m_LogicGroup[g].m_Action.m_Region == m_Team[team].m_Action.m_Region));
        //m_LogicGroup[g].m_Action = m_Team[team].m_Action;

        //Проверяем, корректен ли текущий приказ
        while(true)
        {
            if(m_LogicGroup[g].m_Action.m_Type == mlat_None)
            {
                CopyOrder(team, g);
                if(m_LogicGroup[g].m_Action.m_Type != mlat_None)
                {
                    order_ok = false;
                    continue;
                }
            }
            else if(m_LogicGroup[g].m_Action.m_Type == mlat_Capture)
            {
                if(!CmpOrder(team, g)) //Если приказ изменился, то повинуемся
                {
                    CopyOrder(team, g);
                    order_ok = false;
                    continue;
                }

                //Если нечего захватывать, меняем приказ
                if(m_Region[m_LogicGroup[g].m_Action.m_Region].m_NeutralBuildingCnt <= 0 && m_Region[m_LogicGroup[g].m_Action.m_Region].m_EnemyBuildingCnt <= 0)
                {
                    if(m_LogicGroup[g].m_Action.m_Region != m_Team[team].m_RegionMass) m_LogicGroup[g].m_Action.m_Type = mlat_Forward;
                    else m_LogicGroup[g].m_Action.m_Type = mlat_None;
                    order_ok = false;
                    break;
                }
                u = 0;
                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetTeam() == team)
                    {
                        if(obj->AsRobot()->FindOrderLikeThat(ROT_CAPTURE_BUILDING))
                        {
                            ++u;
                            break;
                        }
                    }
                    obj = obj->GetNextLogic();
                }
                order_ok = u > 0;
                if(order_ok)
                {
                    for(i = 0; i < ally_robots_cnt; ++i)
                    {
                        if(rl[i]->FindOrderLikeThat(ROT_CAPTURE_BUILDING));
                        else if (!PlaceInRegion(rl[i], rl[i]->GetEnv()->m_Place, m_LogicGroup[g].m_Action.m_Region))
                        {
                            if(CanChangePlace(rl[i]))
                            {
                                order_ok = false;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    //Если все роботы группы заняты, то приказ захвата считается успешным
                    for (i = 0; i < ally_robots_cnt; ++i)
                    {
                        if (rl[i]->CanBreakOrder()) break;
                    }
                    if (i >= ally_robots_cnt) order_ok = true;

                    //Если другая команда захватывает завод и группа в регионе, то приказ считаем успешным
                    if (!order_ok)
                    {
                        for (i = 0; i < ally_robots_cnt; ++i)
                        {
                            if (!PlaceInRegion(rl[i], rl[i]->GetEnv()->m_Place, m_LogicGroup[g].m_Action.m_Region)) break;
                        }
                        if (i >= ally_robots_cnt)
                        {
                            obj = CMatrixMapStatic::GetFirstLogic();
                            while (obj)
                            {
                                if (obj->IsRobotAlive() && obj->GetSide() == m_Id)
                                {
                                    CMatrixBuilding* cf = obj->AsRobot()->GetCaptureBuilding();
                                    if (cf && GetRegion(cf) == m_LogicGroup[g].m_Action.m_Region)
                                    {
                                        order_ok = true;
                                        break;
                                    }
                                }
                                obj = obj->GetNextLogic();
                            }
                        }
                    }
                }
            }
            else if(m_LogicGroup[g].m_Action.m_Type == mlat_Defence)
            {
                if(!m_LogicGroup[g].IsWar()) //Если не в состоянии войны
                {
                    for(i = 0; i < ally_robots_cnt; ++i)
                    {
                        if(rl[i]->GetEnv()->GetEnemyCnt()) break;
                    }
                    if(i < ally_robots_cnt && !m_LogicGroup[g].IsWar()) order_ok = false;

                    if(order_ok)
                    {
                        if(team >= 0 && !CmpOrder(team, g)) //Если приказ изменился и нет больше врагов, то повинуемся
                        {
                            CopyOrder(team, g);
                            order_ok = false;
                            continue;
                        }

                        for(i = 0; i < ally_robots_cnt; ++i)
                        {
                            if(CanChangePlace(rl[i]) && GetDesRegion(rl[i]) != m_LogicGroup[g].m_Action.m_Region)
                            {
                                order_ok = false;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    for(i = 0; i < ally_robots_cnt; ++i)
                    {
                        if(rl[i]->GetEnv()->GetEnemyCnt()) break;
                    }
                    if(i >= ally_robots_cnt && m_LogicGroup[g].IsWar()) order_ok = false;
                }
            }
            else if(m_LogicGroup[g].m_Action.m_Type == mlat_Attack)
            {
                u = 0;
                if(!m_LogicGroup[g].IsWar()) //Если не в состоянии войны
                {
                    order_ok = false;
                }
                else
                {
                    for(i = 0; i < ally_robots_cnt && order_ok; ++i)
                    {
                        if(rl[i]->GetEnv()->GetEnemyCnt()) //Видит ли робот врага
                        {
                            ++u;
                        }
                        else
                        {
                            //Робот находится в регионе назночения или идет туда
                            if(CanChangePlace(rl[i]) && GetDesRegion(rl[i]) != m_LogicGroup[g].m_Action.m_Region)
                            {
                                order_ok = false;
                                break;
                            }
                        }
                    }
                }

                if(u == 0 && order_ok) //Если нечего делать, но приказ остался, значит поблизости воюют - спешим им на помощь
                {
                    obj = CMatrixMapStatic::GetFirstLogic();
                    while(obj)
                    {
                        if(obj->IsRobotAlive() && obj->GetSide() == m_Id && GetObjTeam(obj) == team && GetGroupLogic(obj) != g)
                        {
                            if(GetEnv(obj)->m_Target != nullptr)
                            {
                                m_LogicGroup[g].m_Action.m_Region = GetRegion(GetEnv(obj)->m_Target);
                                u = ally_robots_cnt;
                                order_ok = false;
                                break;
                            }
                        }
                        obj = obj->GetNextLogic();
                    }
                }

                if(u == 0 && m_Team[team].m_Action.m_Type != mlat_Attack && !CmpOrder(team, g)) //Если приказ изменился и нет больше врагов, то повинуемся
                {
                    CopyOrder(team, g);
                    order_ok = false;
                    continue;
                }
            }
            else  if(m_LogicGroup[g].m_Action.m_Type == mlat_Forward)
            {
                if(!CmpOrder(team, g)) //Если приказ изменился, то повинуемся
                {
                    CopyOrder(team, g);
                    order_ok = false;
                    continue;
                }

                //u = 0;
                for(i = 0; i < ally_robots_cnt && order_ok; ++i)
                {
                    //Робот находится в регионе назночения или идет туда
                    if(CanChangePlace(rl[i]) && GetDesRegion(rl[i]) != m_LogicGroup[g].m_Action.m_Region)
                    {
                        order_ok = false;
                        break;
                    }

                    //if(IsInPlace(rl[i])) ++u;
                }
                //Если все пришли
                //if(order_ok && u == ally_robots_cnt) {}
            }
            else if(m_LogicGroup[g].m_Action.m_Type == mlat_Retreat)
            {
                if (!CmpOrder(team, g)) //Если приказ изменился, то повинуемся
                {
                    CopyOrder(team, g);
                    order_ok = false;
                    continue;
                }

                for (i = 0; i < ally_robots_cnt && order_ok; ++i)
                {
                    if (CanChangePlace(rl[i]) && GetDesRegion(rl[i]) != m_LogicGroup[g].m_Action.m_Region)
                    {
                        order_ok = false;
                        break;
                    }
                }
            }
            break;
        }
        if(order_ok) continue;

        //Хз зачем тут была двойная одинаковая проверка
        if(m_LogicGroup[g].m_Action.m_Type == mlat_Attack)// && !m_LogicGroup[g].IsWar())
        {
            if(!m_LogicGroup[g].IsWar()) //Если приказ только что установился, то старые места недействительны
            {
                for(i = 0; i < ally_robots_cnt; ++i) rl[i]->GetEnv()->m_Place = -1;
            }
        }

        m_LogicGroup[g].SetWar(false);

        //Распределяем непосредственные приказы по роботам в группе
        int group_ord = m_LogicGroup[g].m_Action.m_Type;
        if(group_ord == mlat_Defence)
        {
            for(i = 0; i < ally_robots_cnt; ++i)
            {
                if(rl[i]->GetEnv()->GetEnemyCnt()) break;
            }
            if(i < ally_robots_cnt)
            {
                m_LogicGroup[g].SetWar(true);
            }
            else
            {
                AssignPlace(g, m_LogicGroup[g].m_Action.m_Region);

                for(i = 0; i < ally_robots_cnt; ++i)
                {
                    if(PrepareBreakOrder(rl[i]))
                    {
                        //rl[i]->BreakAllOrders();
                        SMatrixPlace* place = GetPlacePtr(rl[i]->GetEnv()->m_Place);
                        if(place) rl[i]->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                    }
                }
            }
        }
        else if(group_ord == mlat_Attack)
        {
            //AssignPlace(g, m_LogicGroup[g].m_Action.m_Region);
            m_LogicGroup[g].SetWar(true);

            /*
            for(i = 0; i < ally_robots_cnt; ++i)
            {
                if(rl[i]->GetEnv()->GetEnemyCnt() <= 0) //Если робот не видит врага идем в регион
                {
                    SMatrixPlace *place = GetPlacePtr(rl[i]->GetEnv()->m_Place);
                }
            }
            */
        }
        else if(group_ord == mlat_Forward)
        {
            AssignPlace(g, m_LogicGroup[g].m_Action.m_Region);

            for(i = 0; i < ally_robots_cnt; ++i)
            {
                if(PrepareBreakOrder(rl[i]))
                {
                    SMatrixPlace* place = GetPlacePtr(rl[i]->GetEnv()->m_Place);
                    //rl[i]->BreakAllOrders();
                    if(place) rl[i]->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                }
            }
        }
        else if(group_ord == mlat_Retreat)
        {
            AssignPlace(g, m_LogicGroup[g].m_Action.m_Region);

            for(i = 0; i < ally_robots_cnt; ++i)
            {
                if(PrepareBreakOrder(rl[i]))
                {
                    SMatrixPlace* place = GetPlacePtr(rl[i]->GetEnv()->m_Place);
                    if(place) rl[i]->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                }
            }
        }
        //Обрабатываем приказ на захват, отданный группе доминаторских роботов
        else if(group_ord == mlat_Capture)
        {
            AssignPlace(g, m_LogicGroup[g].m_Action.m_Region);
            //for(i = 0; i < ally_robots_cnt; ++i) rl[i]->BreakAllOrders();

            //Распределяем, кто какие здания захватывает
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                //Перебираем враждебные или нейтральные здания на карте (только доступные для захвата)
                if(obj->IsBuildingAlive() && obj->GetSide() != m_Id && obj->AsBuilding()->CanBeCaptured())
                {
                    //Если здание находится в одном регионе с группой, то оно подходит ей в качестве цели
                    i = GetRegion(GetMapPos(obj));
                    if(i == m_LogicGroup[g].m_Action.m_Region)
                    {
                        //Перебираем всех роботов текущей фракции, ищем активный приказ на захват данного здания
                        CMatrixMapStatic *capturer = CMatrixMapStatic::GetFirstLogic();
                        while(capturer)
                        {
                            if(capturer->IsRobotAlive() && capturer->GetSide() == m_Id)
                            {
                                if(capturer->AsRobot()->FindOrder(ROT_CAPTURE_BUILDING, obj))
                                {
                                    break; //Здесь обрывался оригинальный код

                                    //Если нашли захватчика в составе текущей группы, то всё хорошо
                                    //Иначе считаем дистанцию до здания (активный захватчик может оказаться от него очень далеко)
                                    if(capturer->AsRobot()->GetGroupLogic() != g)
                                    {
                                        float capturer_dist = Dist2(GetWorldPos(capturer), GetWorldPos(obj));
                                        u = -1;
                                        for(i = 0; i < ally_robots_cnt; ++i)
                                        {
                                            if(rl[i]->FindOrderLikeThat(ROT_CAPTURE_BUILDING)) continue;
                                            float d = Dist2(GetWorldPos(rl[i]), GetWorldPos(obj));
                                            if(d < capturer_dist)
                                            {
                                                if(PrepareBreakOrder(rl[i]))
                                                {
                                                    capturer_dist = d;
                                                    u = i;
                                                }
                                            }
                                        }
                                        //Если текущий захватчик действительно оказался далеко,
                                        //то заносим его в текущую группу и переопределяем захватчика
                                        if(u >= 0)
                                        {
                                            if(PrepareBreakOrder(capturer->AsRobot()))
                                            {
                                                capturer->AsRobot()->StopCapture();
                                                g_MatrixMap->StaticDelete(capturer);
                                                //capturer->AsRobot()->SetGroupLogic(g);
                                                //rl[ally_robots_cnt] = capturer->AsRobot();
                                                //++ally_robots_cnt;

                                                /*
                                                int g2 = capturer->AsRobot()->GetGroupLogic();
                                                byte mm = 0;
                                                CMatrixMapStatic* shit = CMatrixMapStatic::GetFirstLogic();
                                                while(shit)
                                                {
                                                    if(shit->IsRobotAlive() && shit->GetSide() == m_Id && GetGroupLogic(shit) == g2)
                                                    {
                                                        shit->AsRobot()->SetGroupLogic(g);
                                                        rl[ally_robots_cnt] = (CMatrixRobotAI*)shit;
                                                        mm |= 1 << (shit->AsRobot()->m_Module[0].m_Kind - 1);
                                                        ++ally_robots_cnt;
                                                    }
                                                    shit = shit->GetNextLogic();
                                                }
                                                

                                                //Для отладки рисуем маркер под роботом, выбранным для захвата
                                                /*
                                                CPoint tp = CPoint(rl[u]->GetMapPosX(), rl[u]->GetMapPosY());
                                                if(tp.x >= 0)
                                                {
                                                    D3DXVECTOR3 v;
                                                    v.x = GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                                                    v.y = GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                                                    v.z = g_MatrixMap->GetZ(v.x, v.y);
                                                    CMatrixEffect::CreateMoveToAnim(v, 2);
                                                }
                                                */

                                                rl[u]->CaptureBuilding((CMatrixBuilding*)obj);
                                                capturer = rl[u];
                                            }
                                        }
                                    }
                                    else
                                    {
                                        //Для отладки
                                        /*
                                        CPoint tp = CPoint(capturer->AsRobot()->GetMapPosX(), capturer->AsRobot()->GetMapPosY());
                                        if(tp.x >= 0)
                                        {
                                            D3DXVECTOR3 v;
                                            v.x = GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                                            v.y = GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                                            v.z = g_MatrixMap->GetZ(v.x, v.y);
                                            CMatrixEffect::CreateMoveToAnim(v, 2);
                                        }
                                        */
                                    }
                                    break;
                                }
                            }
                            capturer = capturer->GetNextLogic();
                        }
                        //Если активных захватчиков не нашли (цикл прокрутился вплоть до nullptr),
                        //назначаем робота-захватчика из состава текущей группы
                        if(capturer == nullptr)
                        {
                            u = -1;
                            float min_dist = 1e20f; //1.000.000.000
                            for(i = 0; i < ally_robots_cnt; ++i)
                            {
                                if(rl[i]->FindOrderLikeThat(ROT_CAPTURE_BUILDING)) continue;
                                float d = Dist2(GetWorldPos(rl[i]), GetWorldPos(obj));
                                if(d < min_dist)
                                {
                                    if(PrepareBreakOrder(rl[i]))
                                    {
                                        min_dist = d;
                                        u = i;
                                    }
                                }
                            }

                            /*
                            if(u >= 0)
                            {
                                //Для отладки рисуем маркер под роботом, выбранным для захвата
                                CPoint tp = CPoint(rl[u]->GetMapPosX(), rl[u]->GetMapPosY());
                                if(tp.x >= 0)
                                {
                                    D3DXVECTOR3 v;
                                    v.x = GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                                    v.y = GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                                    v.z = g_MatrixMap->GetZ(v.x, v.y);
                                    CMatrixEffect::CreateMoveToAnim(v, 1);
                                }
                                rl[u]->CaptureBuilding((CMatrixBuilding*)obj);
                            }
                            */
                        }
                    }
                }

                obj = obj->GetNextLogic();
            }

            //Остальных роботов группы расставляем по местам
            for(i = 0; i < ally_robots_cnt; ++i)
            {
                if(rl[i]->FindOrderLikeThat(ROT_CAPTURE_BUILDING)) continue;

                if(PrepareBreakOrder(rl[i]))
                {
                    SMatrixPlace *place = GetPlacePtr(rl[i]->GetEnv()->m_Place);
                    if(place) rl[i]->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                }
            }
        }
    }
}

void CMatrixSideUnit::WarTL(int group)
{
    byte mm = 0;
    CMatrixMapStatic* obj;
    CMatrixRobotAI* ally_robots[MAX_ROBOTS_ON_MAP]; //Список роботов на карте
    int ally_robots_cnt = 0;                 //Кол-во роботов на карте
    bool ally_robots_ok_move[MAX_ROBOTS_ON_MAP];

    BufPrepare();

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id && GetGroupLogic(obj) == group && obj->AsRobot()->HaveRepair() != FULL_REPAIRER)
        {
            ally_robots[ally_robots_cnt] = obj->AsRobot();
            mm |= 1 << (obj->AsRobot()->m_Module[0].m_Kind - 1);
            ++ally_robots_cnt;
        }

        obj = obj->GetNextLogic();
    }

    if(ally_robots_cnt < 0) return;

    //Находим врага для всей группы
    for(int i = 0; i < ally_robots_cnt; ++i)
    {
        CMatrixRobotAI* robot = ally_robots[i];

        if(robot->GetEnv()->m_TargetAttack == robot) robot->GetEnv()->m_TargetAttack = nullptr;

        if(!robot->GetEnv()->m_TargetAttack)
        {
            //Находим ближайшего незакрытого врага
            float min_dist = 1e10f;
            CEnemy* enemy_find = nullptr;
            CEnemy* enemy = robot->GetEnv()->m_FirstEnemy;
            while(enemy)
            {
                if(enemy->m_Enemy->IsUnitAlive() && enemy->m_Enemy != robot)
                {
                    float cd = Dist2(GetWorldPos(enemy->m_Enemy), GetWorldPos(robot));
                    if(cd < min_dist)
                    {
                        //Проверяем не закрыт ли он своими
                        D3DXVECTOR3 des, from, dir, p;
                        float t, dist;

                        from = robot->GetGeoCenter();
                        des = SetPointOfAim(enemy->m_Enemy);
                        dist = sqrt(POW2(from.x - des.x) + POW2(from.y - des.y) + POW2(from.z - des.z));
                        if(dist > 0.0f)
                        {
                            t = 1.0f / dist;
                            dir.x = (des.x - from.x) * t;
                            dir.y = (des.y - from.y) * t;
                            dir.z = (des.z - from.z) * t;
                            obj = CMatrixMapStatic::GetFirstLogic();

                            while(obj)
                            {
                                if(obj->IsUnitAlive() && obj->GetSide() == m_Id && robot != obj)
                                {
                                    p = SetPointOfAim(obj);

                                    if(IsIntersectSphere(p, 25.0f, from, dir, t))
                                    {
                                        if(t >= 0.0f && t < dist) break;
                                    }
                                }
                                obj = obj->GetNextLogic();
                            }

                            if(!obj)
                            {
                                min_dist = cd;
                                enemy_find = enemy;
                            }
                        }
                    }
                }
                enemy = enemy->m_NextEnemy;
            }

            //Если не нашли открытого ищем закрытого
            if(!enemy_find)
            {
                enemy = robot->GetEnv()->m_FirstEnemy;
                while(enemy)
                {
                    if(enemy->m_Enemy->IsUnitAlive() && enemy->m_Enemy != robot)
                    {
                        float cd = Dist2(GetWorldPos(enemy->m_Enemy), GetWorldPos(robot));
                        if(cd < min_dist)
                        {
                            min_dist = cd;
                            enemy_find = enemy;
                        }
                    }
                    enemy = enemy->m_NextEnemy;
                }
            }

            if(enemy_find)
            {
                robot->GetEnv()->m_TargetAttack = enemy_find->m_Enemy;
                // Если новая цель пушка то меняем позицию
                if(robot->GetEnv()->m_TargetAttack->IsActiveTurretAlive())
                {
                    robot->GetEnv()->m_Place = -1;
                }
            }
        }
    }

    //Проверяем правильно ли роботы идут
    bool moveok = true;
    for(int i = 0; i < ally_robots_cnt; ++i)
    {
        CMatrixRobotAI* robot = ally_robots[i];
        ally_robots_ok_move[i] = true;

        if(!robot->CanBreakOrder()) continue; //Пропускаем если робот не может прервать текущий приказ
        if(!robot->GetEnv()->m_TargetAttack) //Если у робота нет цели то идем в регион назначения
        {
            if(GetDesRegion(robot) != m_LogicGroup[group].m_Action.m_Region)
            {
                if(!PlaceInRegion(robot, robot->GetEnv()->m_Place, m_LogicGroup[group].m_Action.m_Region))
                {
                    if(CanChangePlace(robot))
                    {
                        AssignPlace(robot, m_LogicGroup[group].m_Action.m_Region);

                        SMatrixPlace *place = ObjPlacePtr(robot);
                        if(place && PrepareBreakOrder(robot)) robot->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                    }
                }
            }

            continue;
        }

        if(robot->GetEnv()->m_Place < 0)
        {
            if(CanChangePlace(robot))
            {
                ally_robots_ok_move[i] = false;
                moveok = false;
                continue;
            }
        }

        D3DXVECTOR2 tv = GetWorldPos(robot->GetEnv()->m_TargetAttack);

        SMatrixPlace *place = GetPlacePtr(robot->GetEnv()->m_Place);
        if(place == nullptr)
        {
            if(CanChangePlace(robot))
            {
                robot->GetEnv()->m_Place = -1;
                ally_robots_ok_move[i] = false;
                moveok = false;
                continue;
            }
        }
        else
        {
            float dist2 = POW2(GLOBAL_SCALE_MOVE * place->m_Pos.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2 - tv.x) + POW2(GLOBAL_SCALE_MOVE * place->m_Pos.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2 - tv.y);
            if(dist2 > POW2(robot->GetMaxFireDist() - GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2))
            {
                if(CanChangePlace(robot))
                {
                    robot->GetEnv()->m_Place = -1;
                    ally_robots_ok_move[i] = false;
                    moveok = false;
                    continue;
                }
            }
        }

        /*
        if(!IsToPlace(robot, robot->GetEnv()->m_Place))
        {
            //IsToPlace(robot, robot->GetEnv()->m_Place);
            ally_robots_ok_move[i] = false;
            moveok = false;
            continue;
        }
        */
    }

    //Назначаем движение
    if(!moveok)
    {
        //Находим центр и радиус
        CPoint center, tp2, tp = CPoint(0, 0);
        int f = 0;
        for(int i = 0; i < ally_robots_cnt; ++i)
        {
            if(!ally_robots[i]->GetEnv()->m_TargetAttack) continue;
            tp += GetMapPos(ally_robots[i]->GetEnv()->m_TargetAttack);
            ++f;
        }

        if(f <= 0) return;
        tp.x = tp.x / f; tp.y = tp.y / f;
        f = 1000000000;
        
        for(int i = 0; i < ally_robots_cnt; ++i)
        {
            if(!ally_robots[i]->GetEnv()->m_TargetAttack) continue;
            tp2 = GetMapPos(ally_robots[i]->GetEnv()->m_TargetAttack);
            int f2 = POW2(tp.x - tp2.x) + POW2(tp.y - tp2.y);
            if(f2 < f)
            {
                f = f2;
                center = tp2;
            }
        }

        int radius = 0;
        int radius_robot = 0;
        for(int i = 0; i < ally_robots_cnt; ++i)
        {
            if(!ally_robots[i]->GetEnv()->m_TargetAttack) continue;
            tp2 = GetMapPos(ally_robots[i]->GetEnv()->m_TargetAttack);
            radius_robot = max(radius_robot, Float2Int(ally_robots[i]->GetMaxFireDist() / GLOBAL_SCALE_MOVE));
            radius = max(radius, Float2Int(sqrt(float(POW2(center.x - tp2.x) + POW2(center.y - tp2.y))) + ally_robots[i]->GetMaxFireDist() / GLOBAL_SCALE_MOVE + ROBOT_MOVECELLS_PER_SIZE));
        }

        //DM(L"RadiusSeek", CWStr().Format(L"<i>   <i>", radius, radiusrobot).Get());

        bool cplr = true;

        //Находим место
        int listcnt;
        if(g_MatrixMap->PlaceList(mm, GetMapPos(ally_robots[0]), center, radius, false, m_PlaceList, &listcnt) == 0)
        {
            for(int i = 0; i < ally_robots_cnt; ++i) ally_robots[i]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();
        }
        else
        {
            for(int i = 0; i < listcnt; ++i) g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[i]].m_Data = 0;
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsUnitAlive()) ObjPlaceData(obj, 1);
                obj = obj->GetNextLogic();
            }

            //Находим лучшее место для каждого робота
            //CRect plr = g_MatrixMap->m_RoadNetwork.CorrectRectPL(CRect(rect.left-growsizemax,rect.top-growsizemax,rect.right+growsizemax,rect.bottom+growsizemax));
            for(int i = 0; i < ally_robots_cnt; ++i)
            {
                if(ally_robots_ok_move[i]) continue;
                CMatrixRobotAI* robot = ally_robots[i];
                if(!robot->GetEnv()->m_TargetAttack) continue; // Если нет цели, то пропускаем

                bool havebomb = robot->HaveBomb();

                int placebest = -1;
                float s_f1 = 0.0f;
                int s_underfire = 0;
                bool s_close = false;

                float tvx, tvy; // To target
                int enemy_fire_dist;

                if(robot->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
                {
                    tvx = ((CMatrixRobotAI*)(robot->GetEnv()->m_TargetAttack))->m_PosX - robot->m_PosX;
                    tvy = ((CMatrixRobotAI*)(robot->GetEnv()->m_TargetAttack))->m_PosY - robot->m_PosY;
                    enemy_fire_dist = Float2Int(((CMatrixRobotAI*)(robot->GetEnv()->m_TargetAttack))->GetMaxFireDist());
                }
                else if(robot->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_TURRET)
                {
                    tvx = ((CMatrixTurret*)(robot->GetEnv()->m_TargetAttack))->m_Pos.x - robot->m_PosX;
                    tvy = ((CMatrixTurret*)(robot->GetEnv()->m_TargetAttack))->m_Pos.y - robot->m_PosY;
                    enemy_fire_dist = int(((CMatrixTurret*)(robot->GetEnv()->m_TargetAttack))->GetFireRadius() + GLOBAL_SCALE_MOVE);
                }
                else continue;
                float tsize2 = tvx * tvx + tvy * tvy;
                float tsize2o = 1.0f / tsize2;

                //SMatrixPlaceList* plist = g_MatrixMap->m_RoadNetwork.m_PLList + plr.left + plr.top * g_MatrixMap->m_RoadNetwork.m_PLSizeX;
                //for(y = plr.top; y < plr.bottom; ++y, plist += g_MatrixMap->m_RoadNetwork.m_PLSizeX - (plr.right - plr.left))
                //{
                //    for(x = plr.left; x < plr.right; ++x, ++plist)
                //    {
                //        SMatrixPlace* place = g_MatrixMap->m_RoadNetwork.m_Place + plist->m_Sme;
                //        for(u = 0; u < plist->m_Cnt; ++u, ++place)
                //        {
                for(int u = 0; u < listcnt; ++u)
                {
                    int iplace = m_PlaceList[u];
                    SMatrixPlace* place = g_MatrixMap->m_RoadNetwork.m_Place + iplace;

                    if(place->m_Data) continue; // Занятые места игнорируем
                    if(place->m_Move & (1 << (robot->m_Module[0].m_Kind - 1))) continue; // Если робот не может стоять на этом месте то пропускаем
                    if(robot->GetEnv()->IsBadPlace(iplace)) continue; // Плохое место пропускаем

                    float pcx = GLOBAL_SCALE_MOVE * place->m_Pos.x + (GLOBAL_SCALE_MOVE * 4.0f / 2.0f); // Center place
                    float pcy = GLOBAL_SCALE_MOVE * place->m_Pos.y + (GLOBAL_SCALE_MOVE * 4.0f / 2.0f);

                    float pvx = pcx - robot->m_PosX; // To place
                    float pvy = pcy - robot->m_PosY;

                    float k = (pvx * tvx + pvy * tvy) * tsize2o;
                    if(!havebomb && robot->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_TURRET)
                    {
                        //if(k > 1.5) continue; // Места за врагом не расматриваем
                    }
                    else if(!havebomb && robot->GetEnv()->m_TargetAttack->GetObjectType() != OBJECT_TYPE_BUILDING)
                    {
                        if(k > 0.95) continue; // Места за врагом не расматриваем
                    }
                    else if(!havebomb)
                    {
                        if(k > 1.2) continue; // Места сильно за врагом не расматриваем
                    }
                    //if(k > 0.95) continue; // Места за врагом не расматриваем
                    //if(k < 0.0) continue; // Места за роботом игнорируем
                    float m = (-pvx * tvy + pvy * tvx) * tsize2o;
                    float distfrom2 = POW2(-m * tvy) + POW2(m * tvx); // Дистанция отклонения
                    float distplace2 = POW2(tvx - pcx/*pvx*/) + POW2(tvy - pcx/*pvy*/); // Дистанция от места до врага
                    //if(distplace2 > POW2(0.95 * robot->GetMaxFireDist() - GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2)) continue; // Робот должен достовать врага
                    if((placebest < 0) || (robot->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_TURRET && (robot->GetMaxFireDist() - GLOBAL_SCALE_MOVE) > enemy_fire_dist))
                    {
                        if(distplace2 > POW2(0.95 * robot->GetMaxFireDist() - GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2)) continue; // Робот должен достовать врага
                    }
                    else
                    {
                        if(distplace2 > POW2(0.95 * robot->GetMinFireDist() - GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2)) continue; // Робот должен достовать врага
                    }

                    if(!havebomb && robot->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
                    {
                        if(distfrom2 > POW2(200 + 100)) continue; // Робот не должен отклониться слишком далеко
                    }

                    int underfire = place->m_Underfire;
                    if(distplace2 <= POW2(enemy_fire_dist)) ++underfire;

                    CMatrixMapStatic* trace_res = g_MatrixMap->Trace(nullptr, D3DXVECTOR3(pcx, pcy, g_MatrixMap->GetZ(pcx, pcy) + 20.0f) /*robot->GetGeoCenter()*/, SetPointOfAim(robot->GetEnv()->m_TargetAttack), TRACE_OBJECT | TRACE_NONOBJECT | TRACE_OBJECT_SPHERE | TRACE_SKIP_INVISIBLE, robot);
                    bool close = (IS_TRACE_STOP_OBJECT(trace_res) && trace_res->GetObjectType() == OBJECT_TYPE_MAPOBJECT) || (trace_res == TRACE_STOP_WATER) || (trace_res == TRACE_STOP_LANDSCAPE);

                    if(placebest >= 0) //Если уже найдено место то выбираем наилучшее
                    {
                        if(havebomb)
                        {
                            if(distplace2 > s_f1) continue; //Место дальше предыдущего пропускаем
                        }
                        else if(close != s_close)
                        {
                            if(close) continue;
                        }
                        else if(!underfire && s_underfire);
                        else if(underfire && !s_underfire) continue;
                        else if(underfire) //Если под обстрелом
                        {
                            if(underfire > s_underfire) continue; //Место более обстреливоемое пропускаем
                            if(distplace2 < s_f1) continue; //Место ближе предыдущего пропускаем
                        }
                        else //Если вне радиуса поражения противника
                        {
                            if(distplace2 > s_f1) continue; //Место дальше предыдущего пропускаем
                        }
                    }

                    s_close = close;
                    s_f1 = distplace2;
                    s_underfire = underfire;
                    placebest = iplace;
                }

                if(placebest >= 0)
                {
                    cplr = false;

                    SMatrixPlace* place = GetPlacePtr(placebest);
                    place->m_Data = 1;
                    robot->GetEnv()->m_Place = placebest;
                    if(PrepareBreakOrder(robot))
                    {
                        robot->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                    }
                }
                else if(cplr)
                {
                    cplr = false;
                    if(g_MatrixMap->PlaceList(mm, GetMapPos(ally_robots[0]), center, radius_robot, false, m_PlaceList, &listcnt) == 0)
                    {
                        for(int u = 0; u < ally_robots_cnt; ++u) ally_robots[u]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();
                        break;
                    }
                    else
                    {
                        for(int u = 0; u < listcnt; ++u) g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[u]].m_Data = 0;
                        obj = CMatrixMapStatic::GetFirstLogic();
                        while(obj)
                        {
                            if(obj->IsUnitAlive()) ObjPlaceData(obj, 1);
                            obj = obj->GetNextLogic();
                        }

                        i = -1;
                        continue;
                    }
                }
                else //Не нашли
                {
                    robot->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();

                    int iplace;
                    SMatrixPlace* place;

                    int t_cnt = 0;
                    for(int t_cnt = 0; t_cnt < listcnt; ++t_cnt)
                    {
                        iplace = m_PlaceList[t_cnt];
                        place = g_MatrixMap->m_RoadNetwork.m_Place + iplace;

                        if(place->m_Data) continue; // Занетые места игнорируем
                        if(place->m_Move & (1 << (robot->m_Module[0].m_Kind - 1))) continue; // Если робот не может стоять на этом месте то пропускаем
                        break;
                    }

                    if(t_cnt < listcnt)
                    {
                        place->m_Data = 1;
                        robot->GetEnv()->m_Place = iplace;
                        if(PrepareBreakOrder(robot))
                        {
                            robot->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                        }
                    }
                    else //Расширяем
                    {
                        if(g_MatrixMap->PlaceListGrow(mm, m_PlaceList, &listcnt, ally_robots_cnt) <= 0) continue;

                        for(int u = 0; u < listcnt; ++u) g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[u]].m_Data = 0;
                        obj = CMatrixMapStatic::GetFirstLogic();
                        while(obj)
                        {
                            if(obj->IsUnitAlive()) ObjPlaceData(obj, 1);
                            obj = obj->GetNextLogic();
                        }
                    }
                }
            }
        }
    }

    //Корректируем точку выстрела
    D3DXVECTOR3 des, from, dir, p;
    float t, dist;

    int curTime = g_MatrixMap->GetTime();

    for(int i = 0; i < ally_robots_cnt; ++i)
    {
        CMatrixRobotAI* robot = ally_robots[i];

        if(robot->GetEnv()->m_TargetAttack)
        {
            if(!robot->GetEnv()->m_TargetAttack->IsUnitAlive())
            {
                robot->StopFire();
                continue;
            }

            des = SetPointOfAim(robot->GetEnv()->m_TargetAttack);

            // Не стрелять из прямого оружия, если на пути к цели свои
            from = robot->GetGeoCenter();
            dist = sqrt(POW2(from.x - des.x) + POW2(from.y - des.y) + POW2(from.z - des.z));

            bool fireline = true;
            if(fireline && dist > 0.0f)
            {
                t = 1.0f / dist;
                dir.x = (des.x - from.x) * t;
                dir.y = (des.y - from.y) * t;
                dir.z = (des.z - from.z) * t;

                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsUnitAlive() && obj->GetSide() == m_Id && robot != obj)
                    {
                        p = SetPointOfAim(obj);

                        if(IsIntersectSphere(p, 25.0f, from, dir, t))
                        {
                            if(t >= 0.0f && t < dist)
                            {
                                //CHelper::DestroyByGroup(dword(this)+4);
                                //CHelper::Create(10,dword(this)+4)->Cone(from,des,1.0f,1.0f,0xffffffff,0xffffffff,3);
                                //CHelper::Create(10,dword(this)+4)->Sphere(D3DXVECTOR3(from.x+dir.x*t,from.y+dir.y*t,from.z+dir.z*t),2,5,0xffff0000);

                                fireline = false;
                                break;
                            }
                        }
                    }
                    obj = obj->GetNextLogic();
                }
            }

            //des.x += g_MatrixMap->RndFloat(-5.0f, 5.0f);
            //des.y += g_MatrixMap->RndFloat(-5.0f, 5.0f);
            //des.z += g_MatrixMap->RndFloat(-5.0f, 5.0f);

            CInfo* env = GetEnv(robot);
            if(env->m_TargetAttack != env->m_TargetLast)
            {
                env->m_TargetLast = env->m_TargetAttack;
                env->m_TargetChange = curTime;
            }

            if(fireline)
            {
                D3DXVECTOR3 v1 = robot->GetGeoCenter();
                D3DXVECTOR3 v2 = SetPointOfAim(robot->GetEnv()->m_TargetAttack);

                D3DXVECTOR3 temp = v2 - v1;
                fireline = D3DXVec3LengthSq(&temp) <= POW2(robot->GetMaxFireDist());

                if(fireline)
                {
                    CMatrixMapStatic* trace_res = g_MatrixMap->Trace(nullptr, v1, v2, TRACE_OBJECT | TRACE_NONOBJECT, robot);
                    fireline = !((IS_TRACE_STOP_OBJECT(trace_res) && trace_res->GetObjectType() == OBJECT_TYPE_MAPOBJECT) || (trace_res == TRACE_STOP_WATER) || (trace_res == TRACE_STOP_LANDSCAPE));
                }
            }

            if(fireline)
            {
                //Если у цели голова Firewall, то в неё сложнее попасть
                /*
                if(env->m_TargetAttack->IsRobot() && env->m_TargetAttack->AsRobot()->m_AimProtect > 0)
                {
                    if(env->m_Target != env->m_TargetAttack || fabs(env->m_TargetAngle) <= 1.0f * 1.1f * ToRad)
                    {
                        env->m_TargetAngle = 0.0f;

                        env->m_TargetAngle = min(30.0f * ToRad, (float)atan(env->m_TargetAttack->AsRobot()->m_AimProtect / sqrt(POW2(des.x - robot->m_PosX) + POW2(des.y - robot->m_PosY))));
                        if(g_MatrixMap->Rnd(0, 9) < 5) env->m_TargetAngle = -env->m_TargetAngle;
                    }
                    else if(env->m_TargetAngle > 0) env->m_TargetAngle -= 1.0f * ToRad;
                    else env->m_TargetAngle += 1.0f * ToRad;
                */
                 

                if(env->m_TargetAttack->IsRobot() && env->m_TargetAttack->AsRobot()->m_AimProtect > 0)
                {
                    if(env->m_Target != env->m_TargetAttack || fabs(env->m_TargetAngle) <= 1.3f * ToRad)//>=15.0f*env->m_TargetAttack->AsRobot()->m_AimProtect*ToRad) {
                    {
                        env->m_TargetAngle = 0.0f;

                        env->m_TargetAngle = min(8.0f * ToRad, (float)g_MatrixMap->Rnd(1, 100) / 100.0f * 16.0f * env->m_TargetAttack->AsRobot()->m_AimProtect * ToRad);
                        if (g_MatrixMap->Rnd(0, 9) < 5) env->m_TargetAngle = -env->m_TargetAngle;
                    }

                    /*
                    else if(env->m_TargetAngle > 0) env->m_TargetAngle += 1.0f * ToRad;
                    else if(env->m_TargetAngle < 0) env->m_TargetAngle -= 1.0f * ToRad;
                    else if(fabs(env->m_TargetAngle) > 1.0f) env->m_TargetAngle *= 0.7f;
                    else env->m_TargetAngle = (2 * g_MatrixMap->Rnd(0, 1) - 1) * ToRad;
                    */

                    else env->m_TargetAngle *= 0.75f;

                    if(env->m_TargetAngle != 0.0f)
                    {
                        float vx = des.x - robot->m_PosX;
                        float vy = des.y - robot->m_PosY;
                        float sa, ca;
                        SinCos(env->m_TargetAngle, &sa, &ca);
                        des.x = (ca * vx + sa * vy) + robot->m_PosX;
                        des.y = (-sa * vx + ca * vy) + robot->m_PosY;
                    }
                }

                env->m_Target = env->m_TargetAttack;
                env->m_LastFire = curTime;

                robot->OrderFire(des);

                //CHelper::DestroyByGroup(dword(this) + 6);
                //CHelper::Create(10, dword(this) + 6)->Cone(robot->GetGeoCenter(), des, 1.0f, 1.0f, 0xffffffff, 0xffffffff, 3);

                //Если стоим на месте
                if(IsInPlace(robot))
                {
                    // Если несколько врагов и в цель не попадаем в течении долгого времени, то переназначаем цель
                    if(env->m_EnemyCnt > 1 && (curTime - env->m_TargetChange) > 4000 && (curTime - env->m_LastHitTarget) > 4000)
                    {
                        env->m_TargetAttack = nullptr;
                    }

                    // Если один враг и в цель не попадаем в течении долгого времени и стоим на месте, то переназначаем место
                    if(env->m_EnemyCnt == 1 && (curTime - env->m_TargetChange) > 4000 && (curTime - env->m_LastHitTarget) > 4000)
                    {
                        env->AddBadPlace(env->m_Place);
                        env->m_Place = -1;
                    }

                    // Если очень долго не попадаем в цель, то меняем позицию
                    if((curTime - env->m_TargetChange) > 2000 && (curTime - env->m_LastHitTarget) > 10000)
                    {
                        env->AddBadPlace(env->m_Place);
                        env->m_Place = -1;
                    }
                }
                else env->m_TargetChange = curTime;
            }
            else
            {
                if(robot->HaveRepair() && (g_MatrixMap->GetTime() - robot->GetEnv()->m_TargetChangeRepair) > 1000) //Ищем робота для починки
                {
                    D3DXVECTOR2 v, v2;

                    if(robot->GetEnv()->m_Target && robot->GetEnv()->m_Target->IsUnitAlive() && robot->GetEnv()->m_Target->GetSide() == m_Id && robot->GetEnv()->m_Target->NeedRepair())
                    {
                        v = GetWorldPos(robot);
                        v2 = GetWorldPos(robot->GetEnv()->m_Target);
                        if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) > POW2(robot->GetRepairDist())) robot->GetEnv()->m_Target = nullptr;
                    }
                    else robot->GetEnv()->m_Target = nullptr;

                    if(!robot->GetEnv()->m_Target)
                    {
                        obj = CMatrixMapStatic::GetFirstLogic();
                        while(obj)
                        {
                            if(obj != robot && obj->IsUnitAlive() && obj->GetSide() == m_Id && obj->NeedRepair())
                            {
                                v = GetWorldPos(robot);
                                v2 = GetWorldPos(obj);
                                if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) < POW2(robot->GetRepairDist()))
                                {
                                    robot->GetEnv()->m_Target = obj;
                                    robot->GetEnv()->m_TargetChangeRepair = g_MatrixMap->GetTime();
                                    break;
                                }
                            }
                            obj = obj->GetNextLogic();
                        }
                    }
                }

                //Тип 2 означает, что выбранная для ведения огня цель дружественна стреляющему (этот же номер типа записываем в приказ на выстрел)
                if(robot->GetEnv()->TargetType() == TARGET_IS_FRIEND) robot->OrderFire(SetPointOfAim(robot->GetEnv()->m_Target), TARGET_IS_FRIEND);
                else robot->StopFire();

                // Если стоим на месте
                if(IsInPlace(robot))
                {
                    //Если несколько врагов, а текущий долго закрыт своими, то переназначаем цель
                    if(env->m_EnemyCnt > 1 && (curTime - env->m_TargetChange) > 4000 && (curTime - env->m_LastFire) > 4000)
                    {
                        env->m_TargetAttack = nullptr;
                    }

                    //Если долго закрыт своими, то меняем позицию
                    if((curTime - env->m_TargetChange) > 4000 && (curTime - env->m_LastFire) > 6000)
                    {
                        if(CanChangePlace(robot))
                        {
                            env->AddBadPlace(env->m_Place);
                            env->m_Place = -1;
                        }
                    }
                }
                else env->m_TargetChange = curTime;
            }
        }
    }
}

void CMatrixSideUnit::RepairTL(int group)
{
    CMatrixRobotAI* ally_robots[MAX_ROBOTS_ON_MAP]; //Список роботов на карте
    int ally_robots_cnt = 0;
    D3DXVECTOR2 v, v2;

    CMatrixMapStatic* obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id && GetGroupLogic(obj) == group && obj->AsRobot()->HaveRepair())
        {
            ally_robots[ally_robots_cnt] = obj->AsRobot();
            ++ally_robots_cnt;
        }

        obj = obj->GetNextLogic();
    }

    if(ally_robots_cnt <= 0) return;

    //Подбираем подходящие цели для всех роботов, способных произвести ремонт
    for(int i = 0; i < ally_robots_cnt; ++i)
    {
        CMatrixRobotAI* robot_healer = ally_robots[i];
        CInfo* env = robot_healer->GetEnv();

        if(env->m_Target && env->m_Target->IsAlive() && env->m_Target->GetSide() == m_Id && env->m_Target->NeedRepair())
        {
            v = GetWorldPos(robot_healer);
            v2 = GetWorldPos(env->m_Target);
            //Если установленная ранее для ремонта цель всё ещё нуждается в ремонте и находится в зоне действия ремонтника
            if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) < POW2(robot_healer->GetRepairDist())) continue;
        }

        env->m_Target = nullptr;

        CMatrixMapStatic* primary_target = nullptr;
        CMatrixMapStatic* secondary_target = nullptr;
        CMatrixMapStatic* third_rate_target = nullptr;

        obj = CMatrixMapStatic::GetFirstLogic();
        while(obj)
        {
            if(obj != robot_healer && obj->IsAlive() && obj->GetSide() == m_Id && obj->NeedRepair())
            {
                v = GetWorldPos(robot_healer);
                v2 = GetWorldPos(obj);
                //Если цель в пределах досягаемости ремонтника
                if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) < POW2(robot_healer->GetRepairDist()))
                {
                    EObjectType type = obj->GetObjectType();
                    if(type == OBJECT_TYPE_ROBOT_AI || type == OBJECT_TYPE_FLYER)
                    {
                        if(!primary_target || obj->GetHitpointsPercent() < primary_target->GetHitpointsPercent()) primary_target = obj;
                    }
                    else if(type == OBJECT_TYPE_TURRET)
                    {
                        if(!secondary_target || obj->GetHitpointsPercent() < secondary_target->GetHitpointsPercent()) secondary_target = obj;
                    }
                    else if(type == OBJECT_TYPE_BUILDING)
                    {
                        if(!third_rate_target || obj->GetHitpointsPercent() < third_rate_target->GetHitpointsPercent()) third_rate_target = obj;
                    }
                }
            }

            obj = obj->GetNextLogic();
        }

        if(primary_target) robot_healer->GetEnv()->m_Target = primary_target;
        else if(secondary_target) robot_healer->GetEnv()->m_Target = secondary_target;
        else if(third_rate_target) robot_healer->GetEnv()->m_Target = third_rate_target;
    }

    //Корректируем точку выстрела и отдаём приказы на отхил
    for(int i = 0; i < ally_robots_cnt; ++i)
    {
        CMatrixRobotAI* robot_healer = ally_robots[i];
        if(!robot_healer->GetEnv()->m_Target) continue;

        D3DXVECTOR3 des = SetPointOfAim(robot_healer->GetEnv()->m_Target);

        //Передаваемый тип 2 означает, что выбранная для ведения огня цель дружественна стреляющему
        robot_healer->OrderFire(des, TARGET_IS_FRIEND);
    }
}

/*
void CMatrixSideUnit::AssignPlace(CMatrixRobotAI* robot, int region)
{
    int i, u;
    CMatrixMapStatic* obj;
    SMatrixPlace* place;

    //Если текущее место в регионе, то оно нас устраивает
    if(PlaceInRegion(robot, robot->GetEnv()->m_Place, region)) return;

    //В текущем регионе помечаем все места как пустые
    SMatrixRegion* uregion = g_MatrixMap->m_RoadNetwork.GetRegion(region);
    for(i = 0; i < uregion->m_PlaceAllCnt; ++i) GetPlacePtr(uregion->m_PlaceAll[i])->m_Data = 0;

    //Помечаем занятые места кроме текущего робота
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsUnitAlive() && obj != robot) ObjPlaceData(obj, 1);
        obj = obj->GetNextLogic();
    }

    //Находим пустое место
    for(u = 0; u < uregion->m_PlaceAllCnt; ++u)
    {
        place = GetPlacePtr(uregion->m_PlaceAll[u]);
        if(place->m_Data) continue;
        if(!CanMove(place->m_Move, robot)) continue; //Если робот не может стоять на этом месте, то пропускаем

        robot->GetEnv()->m_Place = uregion->m_PlaceAll[u];
        place->m_Data = 1;
        break;
    }
}
*/

void CMatrixSideUnit::AssignPlace(
    CMatrixRobotAI* robot,
    int region,
    CPoint* target,
    std::vector<SMatrixRegion*>* all_regions)
{
    int i, u;
    CMatrixMapStatic* obj;
    SMatrixPlace* place;

    //Если текущее место в регионе, то оно нас устраивает
    if(PlaceInRegion(robot, robot->GetEnv()->m_Place, region)) return;

    //В текущем регионе помечаем все места как пустые
    SMatrixRegion* uregion = g_MatrixMap->m_RoadNetwork.GetRegion(region);
    for(i = 0; i < uregion->m_PlaceAllCnt; ++i) GetPlacePtr(uregion->m_PlaceAll[i])->m_Data = 0;

    //Помечаем занятые места кроме текущего робота
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsUnitAlive() && obj != robot) ObjPlaceData(obj, 1);
        obj = obj->GetNextLogic();
    }

    int closest_point = -1;
    double min_dist = 2000000000;

    if(target != nullptr)
    {
        int count = all_regions->size() < 3 ? all_regions->size() : 3;
        for(int i = 0; i < count; ++i)
        {
            SMatrixRegion* tempRegion = (*all_regions)[i];

            for(int j = 0; j < tempRegion->m_PlaceAllCnt; ++j)
            {
                place = GetPlacePtr(tempRegion->m_PlaceAll[j]);

                double x1 = target->x;
                double x2 = place->m_Pos.x;
                double y1 = target->y;
                double y2 = place->m_Pos.y;

                double distance = sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
                if(distance < min_dist)
                {
                    closest_point = tempRegion->m_PlaceAll[j];
                    min_dist = distance;
                }
            }
        }
    }

    if(target == nullptr || closest_point == -1)
    {
        //Находим пустое место
        for(u = 0; u < uregion->m_PlaceAllCnt; ++u)
        {
            place = GetPlacePtr(uregion->m_PlaceAll[u]);
            if(place->m_Data) continue;
            if(!CanMove(place->m_Move, robot)) continue; //Если робот не может стоять на этом месте, то пропускаем

            robot->GetEnv()->m_Place = uregion->m_PlaceAll[u];
            place->m_Data = 1;
            break;
        }
    }
    else if(closest_point != -1)
    {
        robot->GetEnv()->m_Place = closest_point;
        place = GetPlacePtr(closest_point);
        place->m_Data = 1;
    }
}

void CMatrixSideUnit::AssignPlace(int group, int region)
{
    float f;
    CPoint tp, tp2;
    int i, u, t;
    CMatrixMapStatic* obj;
    SMatrixPlace* place;
    byte mm = 0;

    CMatrixRobotAI* ally_robots[MAX_ROBOTS_ON_MAP]; // Список роботов на карте
    int ally_robots_cnt = 0;                  // Кол-во роботов на карте

    BufPrepare();

    //В текущем регионе помечаем все места как пустые
    SMatrixRegion* uregion = g_MatrixMap->m_RoadNetwork.GetRegion(region);
    for(i = 0; i < uregion->m_PlaceAllCnt; ++i) GetPlacePtr(uregion->m_PlaceAll[i])->m_Data = 0;

    //Помечаем занятые места кроме роботов текущей группы
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive())
        {
            CMatrixRobotAI* bot = obj->AsRobot();
            //Если робот уже добрался до точки начала захвата базы, то этот код идёт нахуй (в проверке ниже возможен непонятный краш)
            if(bot->GetOrder(0)->GetOrderPhase() != ROP_CAPTURE_IN_POSITION || !bot->GetOrder(0)->GetStatic()->AsBuilding()->IsBase())
            {
                if(bot->GetSide() == m_Id && GetGroupLogic(bot) == group)
                {
                    ally_robots[ally_robots_cnt] = bot;
                    mm |= 1 << (bot->m_Module[0].m_Kind - 1);
                    ++ally_robots_cnt;
                }
                if(bot->GetSide() != m_Id || GetGroupLogic(bot) != group) ObjPlaceData(bot, 1);
            }
        }
        else if(obj->IsActiveTurretAlive()) ObjPlaceData(obj, 1);
        obj = obj->GetNextLogic();
    }
    if(ally_robots_cnt <= 0) return;

    SortRobotList(ally_robots, ally_robots_cnt);

    //Рассчитываем вектор на врага
    D3DXVECTOR2 venemy;
    int cr = ally_robots[0]->GetRegion();
    int r = FindNearRegionWithUTR(cr, nullptr, 0, 4 + 32 + 64);
    if(r >= 0 && r != ally_robots[0]->GetRegion())
    {
        tp = g_MatrixMap->m_RoadNetwork.m_Region[r].m_Center;
        tp2 = g_MatrixMap->m_RoadNetwork.m_Region[cr].m_Center;
    }
    else if(m_LogicGroup[group].m_Team >= 0 && m_Team[m_LogicGroup[group].m_Team].m_RegionMassPrev != r)
    {
        tp = g_MatrixMap->m_RoadNetwork.m_Region[cr].m_Center;
        tp2 = g_MatrixMap->m_RoadNetwork.m_Region[m_Team[m_LogicGroup[group].m_Team].m_RegionMassPrev].m_Center;
    }
    else
    {
        tp = CPoint(0, 0);
        tp2 = CPoint(1, 1);
    }

    venemy.x = float(tp.x - tp2.x);
    venemy.y = float(tp.y - tp2.y);
    f = 1.0f / sqrt(POW2(venemy.x) + POW2(venemy.y));
    venemy.x *= f;
    venemy.y *= f;

    D3DXVECTOR2 vcenter;
    tp = g_MatrixMap->m_RoadNetwork.m_Region[region].m_Center;
    vcenter.x = float(tp.x);
    vcenter.y = float(tp.y);

    //Создаем список пустых мест
    int listcnt = 0;
    SMatrixRegion* pregion = g_MatrixMap->m_RoadNetwork.m_Region + region;
    for(i = 0; i < pregion->m_PlaceCnt; ++i)
    {
        if(g_MatrixMap->m_RoadNetwork.m_Place[pregion->m_Place[i]].m_Data) continue;
        m_PlaceList[listcnt] = pregion->m_Place[i];
        ++listcnt;
    }
    for(i = 0; i < listcnt - 1; ++i) //Сортируем по дальности
    {
        place = g_MatrixMap->m_RoadNetwork.m_Place + m_PlaceList[i];
        float pr1 = venemy.x * (place->m_Pos.x - vcenter.x) + venemy.y * (place->m_Pos.y - vcenter.y);
        for(u = i + 1; u < listcnt; ++u)
        {
            place = g_MatrixMap->m_RoadNetwork.m_Place + m_PlaceList[u];
            float pr2 = venemy.x * (place->m_Pos.x - vcenter.x) + venemy.y * (place->m_Pos.y - vcenter.y);
            if(pr2 > pr1)
            {
                int temp = m_PlaceList[u]; m_PlaceList[u] = m_PlaceList[i]; m_PlaceList[i] = temp;
                pr1 = pr2;
            }
        }
    }

    //Назначаем места
    for(t = 0; t < ally_robots_cnt; ++t)
    {
        for(i = 0; i < listcnt; ++i)
        {
            place = g_MatrixMap->m_RoadNetwork.m_Place + m_PlaceList[i];
            if (place->m_Data) continue;
            if (!CanMove(place->m_Move, ally_robots[t])) continue;
            break;
        }
        if(i < listcnt)
        {
            place->m_Data = 1;
            ally_robots[t]->GetEnv()->m_Place = m_PlaceList[i];
        }
        else //Если не нашли, то расширяем список
        {
            for(i = 0; i < ally_robots_cnt; ++i) ally_robots[i]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();

            if(g_MatrixMap->PlaceListGrow(mm, m_PlaceList, &listcnt, ally_robots_cnt) <= 0) continue;

            for(i = 0; i < listcnt - 1; ++i) //Сортируем по дальности
            {
                place = g_MatrixMap->m_RoadNetwork.m_Place + m_PlaceList[i];
                float pr1 = venemy.x * (place->m_Pos.x - vcenter.x) + venemy.y * (place->m_Pos.y - vcenter.y);
                for(u = i + 1; u < listcnt; ++u)
                {
                    place = g_MatrixMap->m_RoadNetwork.m_Place + m_PlaceList[u];
                    float pr2 = venemy.x * (place->m_Pos.x - vcenter.x) + venemy.y * (place->m_Pos.y - vcenter.y);
                    if(pr2 > pr1)
                    {
                        int temp = m_PlaceList[u]; m_PlaceList[u] = m_PlaceList[i]; m_PlaceList[i] = temp;
                        pr1 = pr2;
                    }
                }
            }
            for(i = 0; i < listcnt; ++i)
            {
                place = g_MatrixMap->m_RoadNetwork.m_Place + m_PlaceList[i];
                place->m_Data = 0;
            }
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive())
                {
                    if(obj->GetSide() != m_Id || GetGroupLogic(obj) != group) ObjPlaceData(obj, 1);
                }
                else if(obj->IsActiveTurretAlive()) ObjPlaceData(obj, 1);
                obj = obj->GetNextLogic();
            }
            t = -1;
            continue;
        }
    }

    /*
    //Помечаем места, которые устраивают
    t=0;
    for(i=0; i<ally_robots_cnt; ++i)
    {
        if(ally_robots[i]->GetEnv()->m_Place>=0 && GetPlacePtr(ally_robots[i]->GetEnv()->m_Place)->m_Region==region && GetPlacePtr(ally_robots[i]->GetEnv()->m_Place)->m_Data==0)
        {
            GetPlacePtr(ally_robots[i]->GetEnv()->m_Place)->m_Data=1;
            ++t;
        }
        else ally_robots[i]->GetEnv()->m_Place=-1;
    }
    if(t==ally_robots_cnt) return;

    // Назначаем остальные места
    for(i=0; i<ally_robots_cnt; ++i)
    {
        if(ally_robots[i]->GetEnv()->m_Place<0)
        {
            for(u=0; u<uregion->m_PlaceAllCnt; ++u)
            {
                place=GetPlacePtr(uregion->m_PlaceAll[u]);
                if(place->m_Data) continue;
                if(!CanMove(place->m_Move,ally_robots[i])) continue; // Если робот не может стоять на этом месте то, пропускаем

                ally_robots[i]->GetEnv()->m_Place=uregion->m_PlaceAll[u];
                place->m_Data=1;
                break;
            }
            if(u>=uregion->m_PlaceAllCnt)
            {
                ERROR_E;
            }
        }
    }*/
}

void CMatrixSideUnit::SortRobotList(CMatrixRobotAI** ally_robots, int ally_robots_cnt)
{
    if(ally_robots_cnt <= 1) return;

    CMatrixRobotAI* ally_robots_normals[MAX_ROBOTS_ON_MAP];
    CMatrixRobotAI* ally_robots_repairers[MAX_ROBOTS_ON_MAP];
    CMatrixRobotAI* ally_robots_bombers[MAX_ROBOTS_ON_MAP];
    int ally_robots_normals_cnt = 0;
    int ally_robots_repairers_cnt = 0;
    int ally_robots_bombers_cnt = 0;

    // Сортируем по силе
    for(int i = 0; i < ally_robots_cnt - 1; ++i)
    {
        for(int u = i + 1; u < ally_robots_cnt; ++u)
        {
            if(ally_robots[u]->GetStrength() < ally_robots[i]->GetStrength())
            {
                CMatrixRobotAI* temp = ally_robots[u];
                ally_robots[u] = ally_robots[i];
                ally_robots[i] = temp;
            }
        }
    }

    // Роботы с чинилкой, бомбой, и без, в разных списках
    for(int i = 0; i < ally_robots_cnt; ++i)
    {
        if(ally_robots[i]->HaveBomb())
        {
            ally_robots_bombers[ally_robots_bombers_cnt] = ally_robots[i];
            ++ally_robots_bombers_cnt;
        }
        else if(ally_robots[i]->HaveRepair())
        {
            ally_robots_repairers[ally_robots_repairers_cnt] = ally_robots[i];
            ++ally_robots_repairers_cnt;
        }
        else
        {
            ally_robots_normals[ally_robots_normals_cnt] = ally_robots[i];
            ++ally_robots_normals_cnt;
        }
    }

    // Обедняем списки. Каждый 2 робот с бомбой. Каждый 3 робот с чинилкой, начиная от робота с бомбой.
    ally_robots_cnt = 0;
    int s_normal = 0, s_bomb = 0, s_repair = 0;
    int i_bomb = 0, i_repair = 0;
    while((s_normal < ally_robots_normals_cnt) || (s_repair < ally_robots_repairers_cnt) || (s_bomb < ally_robots_bombers_cnt))
    {
        if(i_bomb >= 1 && s_bomb < ally_robots_bombers_cnt)
        {
            ally_robots[ally_robots_cnt] = ally_robots_bombers[s_bomb];
            ++s_bomb;
            ++ally_robots_cnt;

            i_bomb = 0;
            i_repair = 0;
        }
        else if(i_repair >= 2 && s_repair < ally_robots_repairers_cnt)
        {
            ally_robots[ally_robots_cnt] = ally_robots_repairers[s_repair];
            ++s_repair;
            ++ally_robots_cnt;

            i_repair = 0;
        }
        else if(s_normal < ally_robots_normals_cnt)
        {
            ally_robots[ally_robots_cnt] = ally_robots_normals[s_normal];
            ++s_normal;
            ++ally_robots_cnt;

            ++i_bomb;
            ++i_repair;
        }
        else
        {
            ++i_bomb;
            ++i_repair;
        }
    }
}

bool CMatrixSideUnit::PlaceInRegion(CMatrixRobotAI* robot, int place, int region)
{
    if(place < 0) return false;

    // Место в регионе
    SMatrixPlace* pl = GetPlacePtr(place);
    if(pl->m_Region == region) return true;

    // Если свободное место в регионе то возращаем false
    SMatrixRegion* uregion = g_MatrixMap->m_RoadNetwork.GetRegion(region);

    for(int i = 0; i < uregion->m_PlaceCnt; ++i)
    {
        pl = GetPlacePtr(uregion->m_Place[i]);
        pl->m_Data = 0;
    }

    CMatrixMapStatic* obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsUnitAlive() && obj != robot)
        {
            pl = ObjPlacePtr(obj);
            if(pl) pl->m_Data = 1;
        }

        obj = obj->GetNextLogic();
    }

    for(int i = 0; i < uregion->m_PlaceCnt; ++i)
    {
        pl = GetPlacePtr(uregion->m_Place[i]);
        if(pl->m_Data) continue;
        if(!CanMove(pl->m_Move, robot)) continue; // Если робот не может стоять на этом месте то пропускаем
        return false;
    }

    // Ближайшее место к региону
    for(int i = uregion->m_PlaceCnt; i < uregion->m_PlaceAllCnt; ++i)
    {
        if(uregion->m_PlaceAll[i] == place) return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CMatrixSideUnit::ChooseAndBuildAIRobot(int cur_side)
{
    if(m_WaitResForBuildRobot > g_MatrixMap->GetTime()) return; //Ждём прихода ресурсов

    CMatrixBuilding* base = nullptr;
    float base_in_danger = 0.0f; //Относительное превосходство врага в силе роботов возле сборочной базы
    int bases_cnt = 0;
    float res_sufficiency[MAX_RESOURCES];
    for(int i = 0; i < MAX_RESOURCES; ++i) res_sufficiency[i] = 0.0f;

    //Собираем информацию
    //Выбираем будущую базу для рождения, а также считаем общее число роботов у текущей стороны (ведь даб долбоёб, и не смог просто ввести список или хотя бы переменную подсчета сразу при рождении/смерти робота)
    int robots_cnt = 0;
    CMatrixMapStatic* object = CMatrixMapStatic::GetFirstLogic();
    while(object)
    {
        if(object->GetSide() == m_Id)
        {
            //Робот фракции (просто считаем количество)
            if(object->IsRobotAlive() && object->AsRobot()->GetTeam() >= 0)
            {
                ++robots_cnt;
            }
            //Здание фракции
            else if(object->IsBuildingAlive())
            {
                CMatrixBuilding* building = object->AsBuilding();

                //Сборочная база фракции
                if(building->IsBase())
                {
                    ++bases_cnt; //Общее число баз запоминаем для расчёта прироста ресурсов
                    robots_cnt += building->m_BuildingQueue.GetItemsCnt(); //В общий счётчик роботов фракции также записываем заказанных, но ещё не достроенных

                    //Если база у фракции всего одна, то вариантов нет, а если несколько, то будет выбрана та, что находится в данный момент в большей опасности
                    if(!base)
                    {
                        base = building;
                        base_in_danger = m_Region[GetRegion(base)].m_EnemyForces / max(m_Region[GetRegion(base)].m_OurForces, 0.0001f); //Получаем коэффициент угрозы базе
                    }
                    else
                    {
                        int cur_reg = GetRegion(base);
                        int new_reg = GetRegion(building);

                        //Если следующая по списку база находится в очевидно большей опасности, чем предыдущая, то выбираем её
                        float other_base_in_danger = m_Region[new_reg].m_EnemyForces / max(m_Region[new_reg].m_OurForces, 0.0001f);

                        if(base_in_danger != other_base_in_danger)
                        {
                            if(base_in_danger < other_base_in_danger)
                            {
                                base = building;
                                base_in_danger = other_base_in_danger;
                            }

                            goto NextObject;
                        }

                        //Если все базы находятся в равной опасности (что вероятнее всего, в нулевой), то выбираем базу для постройки исходя из близости вражеских роботов или, на худой конец, вражеского строения
                        if(m_Region[new_reg].m_EnemyRobotDist != m_Region[cur_reg].m_EnemyRobotDist)
                        {
                            if(m_Region[cur_reg].m_EnemyRobotDist < 0 || (m_Region[new_reg].m_EnemyRobotDist >= 0 && m_Region[new_reg].m_EnemyRobotDist < m_Region[cur_reg].m_EnemyRobotDist))
                            {
                                base = building;
                                base_in_danger = other_base_in_danger;
                            }

                            goto NextObject;
                        }

                        if(m_Region[new_reg].m_EnemyBuildingDist != m_Region[cur_reg].m_EnemyBuildingDist)
                        {
                            if(m_Region[cur_reg].m_EnemyBuildingDist < 0 || (m_Region[new_reg].m_EnemyBuildingDist >= 0 && m_Region[new_reg].m_EnemyBuildingDist < m_Region[cur_reg].m_EnemyBuildingDist))
                            {
                                base = building;
                                base_in_danger = other_base_in_danger;
                            }

                            goto NextObject;
                        }
                    }
                }
                //Завод фракции (запоминаем число заводов каждого типа у фракции)
                else
                {
                    switch(building->m_Kind)
                    {
                        case BUILDING_TITAN: ++res_sufficiency[TITAN]; break;
                        case BUILDING_ELECTRONIC: ++res_sufficiency[ELECTRONICS]; break;
                        case BUILDING_ENERGY: ++res_sufficiency[ENERGY]; break;
                        case BUILDING_PLASMA: ++res_sufficiency[PLASMA]; break;
                    }
                }
            }
        }

        NextObject:;
        object = object->GetNextLogic();
    }

    if(!base) return;
    if(base->State() == BASE_OPENING) return; //Если происходит непосредственный захват базы, поднимается подъёмник
    if(base->m_BuildingQueue.GetItemsCnt() > 0) return; //Если в очереди уже есть робот, то больше пока не заказываем (ебать, а я-то думал даб и очередь будет перебирать через все объекты на карте!)
    if(robots_cnt >= GetMaxSideRobots()) return; //У фракции и так уже достигнут лимит роботов

    //Начинается новый алгоритм выбора (старый под комментарием ниже)
    // Как это должно работать:
    // Если запас высокий, ИИ смело его потратит на сильных роботов за счет переоцененного запаса, если он низкий, но инкам высокий, ИИ чуть охотнее будет копить на сильных роботов за счет прибавки + 10. Можно вместо этой прибавки сделать переменную, которая бы также зависела от «ждать ресурсы на сильных роботов»
 
    //Считаем обеспеченность каждым видом ресурса: (round(запас ресурса ^ 1.1) + 10) * текущий приход ресурса
    int min_overrated_reserve = 2000000000;
    int min_income = 2000000000;
    for(int res = 0; res < MAX_RESOURCES; ++res)
    {
        int overrated_reserve = int(roundf((float)pow(m_Resources[res], 1.1)));
        if(overrated_reserve < min_overrated_reserve) min_overrated_reserve = overrated_reserve;

        int income = (RESOURCES_INCOME_FROM_FACTORY * int(res_sufficiency[res]) + (RESOURCES_INCOME_FROM_BASE * GetResourceForceUp() / 100 * bases_cnt));
        if(income < min_income) min_income = income;

        res_sufficiency[res] = float((overrated_reserve + 10) * (abs(income) + 10)); //В теории, в настройках карты можно задать отрицательный прирост ресурсов с базы, поэтому перестраховываемся от такого через модуль, чтобы бот поскорее потратил иссякающий ресурс
    }

    //Определяем профицитность/дефицитность каждого ресурса
    int average_sufficiency = int(roundf((res_sufficiency[TITAN] + res_sufficiency[ELECTRONICS] + res_sufficiency[ENERGY] + res_sufficiency[PLASMA]) / float(MAX_RESOURCES)));
    for(int res = 0; res < MAX_RESOURCES; ++res)
    {
        float val = res_sufficiency[res] / average_sufficiency;
        res_sufficiency[res] = 0; //Запас ресурса в норме
        if(val < 0.7) //Дефицит
        {
            if(res != TITAN) res_sufficiency[res] = -0.8f;
            else res_sufficiency[res] = -1.5f;
        }
        else if(val < 0.3) //Острый дефицит
        {
            if(res != TITAN) res_sufficiency[res] = -0.4f;
            else res_sufficiency[res] = -1.0f;
        }
        else if(val > 1.7) res_sufficiency[res] = 1.4f; //Профицит
        //else if(val > 2.5) res_sufficiency[res] = 2.4f; //Острый профицит
    }

    //Считаем относительный уровень актуального достатка и по нему определяем категорию шаблонов (1-5), из которых будет выбран робот для постройки (m_WaitResMultiplier задаётся в настройках карты)
    //Считаем достаток
    float prosperity_lvl = float((min_overrated_reserve + 180) * (abs(min_income) + 7));
    //Считаем актуальный достаток
    prosperity_lvl = (prosperity_lvl * m_WaitResMultiplier) + (prosperity_lvl * 0.3f);
    //Занижаем значение достатка, если база находится в серьёзной опасности, чтобы ИИ начал массово производить дешёвых роботов и смог отразить атаку не истощив резервы
    prosperity_lvl = prosperity_lvl * PortionInDiapason(base_in_danger, 0.6f, 10.0f, 1.0f, 0.1f);

    //Вывод значения актуального достатка для отладки
    /*
    if(m_Id == 1) SFT("Player's Prosperity: " + CStr(prosperity_lvl));
    else if(m_Id == 2) SFT("Blazer's Prosperity: " + CStr(prosperity_lvl));
    else if(m_Id == 3) SFT("Keller's Prosperity: " + CStr(prosperity_lvl));
    else if(m_Id == 4) SFT("Terron's Prosperity: " + CStr(prosperity_lvl));
    */

    int template_category;
    if(prosperity_lvl <= 5000.0f)
    {
        if(g_MatrixMap->Rnd(1, 100) > 10) template_category = 0;
        else template_category = 1;
    }
    else if(prosperity_lvl <= 10000.0f)
    {
        int roll = g_MatrixMap->Rnd(1, 100);
        if(roll > 15) template_category = 1;
        else if(roll <= 10) template_category = 2;
        else if(roll <= 15) template_category = 0;
    }
    else if(prosperity_lvl <= 20000.0f)
    {
        int roll = g_MatrixMap->Rnd(1, 100);
        if(roll > 35) template_category = 2;
        else if(roll <= 25) template_category = 3;
        else if(roll <= 30) template_category = 0;
        else if(roll <= 35) template_category = 1;
    }
    else if(prosperity_lvl <= 40000.0f)
    {
        int roll = g_MatrixMap->Rnd(1, 100);
        if(roll > 20) template_category = 3;
        else if(roll <= 10) template_category = 2;
        else if(roll <= 15) template_category = 0;
        else if(roll <= 20) template_category = 1;
    }
    else //if(prosperity_lvl > 40000.0f)
    {
        if(g_MatrixMap->Rnd(1, 100) > 25) template_category = 3;
        else template_category = 4;
    }

    //Для ограничения частоты постройки роботов с бомбой делаем проверку на задержку (задаётся в настройках карты) до возможности постройки следующего бомбера
    bool bomb_allowed = ((g_MatrixMap->GetTime() - m_TimeLastBomb) >= m_TimeNextBomb);
    //Для обеспечения достаточной частоты постройки ремонтников, проверяем, был ли построен хоть один за последние три выбранных шаблона
    bool enough_repairers = ((m_LastBuildedRobot  >= 0) && SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot].m_HaveRepair)  ||
                            ((m_LastBuildedRobot2 >= 0) && SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot2].m_HaveRepair) ||
                            ((m_LastBuildedRobot3 >= 0) && SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot3].m_HaveRepair);

    int list_begin = SRobotTemplate::m_AIRobotTypesCategories[cur_side][template_category].Begin;
    int list_end = SRobotTemplate::m_AIRobotTypesCategories[cur_side][template_category].End;
    int total_weight = 0;
    int total_weight_deficit = 0;
    for(int i = list_begin; i <= list_end; ++i)
    {
        SRobotTemplate* cur_template = &SRobotTemplate::m_AIRobotTypesList[cur_side][i];
        cur_template->m_TemplateWeight = 2; //Обязательно сперва сбрасываем вес в базовое значение в начале сортировки, чтобы нормировать значение с момента прошлого прогона списка

        //Если данного робота построить невозможно (по нехватке ресурсов, либо иным условиям), то удаляем его из вариантов выбора совсем
        if( i == m_LastBuildedRobot ||
            /*
            i == m_LastBuildedRobot2 ||
            i == m_LastBuildedRobot3 ||
            */

            m_Resources[TITAN] < cur_template->m_Resources[TITAN] ||
            m_Resources[ELECTRONICS] < cur_template->m_Resources[ELECTRONICS] ||
            m_Resources[ENERGY] < cur_template->m_Resources[ENERGY] ||
            m_Resources[PLASMA] < cur_template->m_Resources[PLASMA] ||

            (!enough_repairers && !cur_template->m_HaveRepair && base_in_danger <= 1.0f) || //Если база под атакой, но ситуация держится под контролем
            (cur_template->m_HaveRepair && base_in_danger > 2.0f) ||                        //Если база под серьёзной атакой, полностью запрещаем спавн ремонтников
            (!bomb_allowed && cur_template->m_HaveBomb) )
        {
            cur_template->m_TemplateWeight = 0;
        }
        //Если наблюдается профицит или дефицит ресурсов, необходимых для постройки данного робота, то повышаем или понижаем (но не обнуляем полностью) вероятность его выбора
        //Любой дефицитный ресурс в конструкции робота автоматически сделает его вес минусовым и исключит из первого списка, если только по какому-то другому ресурсу постройка такого робота не будет контрить профицит
        else
        {
            if(res_sufficiency[TITAN] > 0 && cur_template->m_ResourcesExpendability[TITAN] > res_sufficiency[TITAN])
            {
                cur_template->m_TemplateWeight += int(1.0f + ceil(cur_template->m_ResourcesExpendability[TITAN] - res_sufficiency[TITAN]));
            }
            else if(res_sufficiency[TITAN] < 0 && cur_template->m_ResourcesExpendability[TITAN] > abs(res_sufficiency[TITAN]))
            {
                cur_template->m_TemplateWeight -= int(cur_template->m_ResourcesExpendability[TITAN] - (4.0f + roundf(res_sufficiency[TITAN])));
            }

            if(res_sufficiency[ELECTRONICS] > 0 && cur_template->m_ResourcesExpendability[ELECTRONICS] > res_sufficiency[ELECTRONICS])
            {
                cur_template->m_TemplateWeight += int(1.0f + ceil(cur_template->m_ResourcesExpendability[ELECTRONICS] - res_sufficiency[ELECTRONICS]));
            }
            else if(res_sufficiency[ELECTRONICS] < 0 && cur_template->m_ResourcesExpendability[ELECTRONICS] > abs(res_sufficiency[ELECTRONICS]))
            {
                cur_template->m_TemplateWeight -= int(cur_template->m_ResourcesExpendability[ELECTRONICS] - (4.0f + roundf(res_sufficiency[ELECTRONICS])));
            }

            if(res_sufficiency[ENERGY] > 0 && cur_template->m_ResourcesExpendability[ENERGY] > res_sufficiency[ENERGY])
            {
                cur_template->m_TemplateWeight += int(1.0f + ceil(cur_template->m_ResourcesExpendability[ENERGY] - res_sufficiency[ENERGY]));
            }
            else if(res_sufficiency[ENERGY] < 0 && cur_template->m_ResourcesExpendability[ENERGY] > abs(res_sufficiency[ENERGY]))
            {
                cur_template->m_TemplateWeight -= int(cur_template->m_ResourcesExpendability[ENERGY] - (4.0f + roundf(res_sufficiency[ENERGY])));
            }

            if(res_sufficiency[PLASMA] > 0 && cur_template->m_ResourcesExpendability[PLASMA] > res_sufficiency[PLASMA])
            {
                cur_template->m_TemplateWeight += int(1.0f + ceil(cur_template->m_ResourcesExpendability[PLASMA] - res_sufficiency[PLASMA]));
            }
            else if(res_sufficiency[PLASMA] < 0 && cur_template->m_ResourcesExpendability[PLASMA] > abs(res_sufficiency[PLASMA]))
            {
                cur_template->m_TemplateWeight -= int(cur_template->m_ResourcesExpendability[PLASMA] - (4.0f + roundf(res_sufficiency[PLASMA])));
            }

            if(cur_template->m_TemplateWeight > 0) total_weight += cur_template->m_TemplateWeight;
            else
            {
                cur_template->m_TemplateWeight = -(cur_template->m_TemplateWeight + 15); //За счёт данного действия мы "выгибаем" минус в обратную сторону, сохраняя наименее желательный вариант для выбора таковым при будущем броске рандома (+15 взято с запасом)
                total_weight_deficit += cur_template->m_TemplateWeight;
            }
        }
    }

    //Выбираем случайный шаблон из выбранного ранее диапазона списка, отсортированного с помощью веса входящих в него шаблонов
    int template_num = -1;
    if(total_weight)
    {
        int roll = g_MatrixMap->Rnd(0, total_weight - 1);
        for(int i = list_begin; i <= list_end; ++i)
        {
            roll -= max(SRobotTemplate::m_AIRobotTypesList[cur_side][i].m_TemplateWeight, 0); //Учитываем только положительный вес
            if(roll < 0)
            {
                template_num = i;
                break;
            }
        }
    }
    //Если в списке не оказалось вариантов, которые бы способствовали выравниванию профицита, то выбираем что-то из "дефицитных" шаблонов
    else if(total_weight_deficit)
    {
        int roll = g_MatrixMap->Rnd(0, abs(total_weight_deficit) - 1);
        for(int i = list_begin; i <= list_end; ++i)
        {
            roll -= abs(min(SRobotTemplate::m_AIRobotTypesList[cur_side][i].m_TemplateWeight, 0)); //Учитываем только отрицательный вес
            if(roll < 0)
            {
                template_num = i;
                break;
            }
        }
    }

    //Если подходящих для постройки роботов не нашлось, то выставляем задержку повторной проверки до следующего пополнения ресурсов от выбранной базы
    if(template_num < 0)
    {
        m_WaitResForBuildRobot = g_MatrixMap->GetTime() + (g_Config.m_Timings[RESOURCE_BASE] - base->GetResourcePeriod());
        return;
    }

    if(base->m_BuildingQueue.GetItemsCnt() < 6) //В данном варианте кода эта проверка лишняя, но пусть пока будет
    {
        //Списываем ресурсы за постройку выбранного шаблона робота
        //SFT("---");
        //SFT(CStr("Current side: ") + m_Id);
        //SFT(CStr("Template: ") + CStr(SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_StringTemplate));

        for(int i = 0; i < MAX_RESOURCES; ++i)
        {
            //if(i == TITAN) SFT(CStr("Current Titan: ") + CStr(m_Resources[i]) + CStr(" / Cost Titan: ") + SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[i]);
            //else if(i == ELECTRONICS) SFT(CStr("Current Chip: ") + CStr(m_Resources[i]) + CStr(" / Cost Chip: ") + SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[i]);
            //else if(i == ENERGY) SFT(CStr("Current Bats: ") + m_Resources[i] + CStr(" / Cost Bats: ") + SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[i]);
            //else if(i == PLASMA) SFT(CStr("Current Plasma: ") + m_Resources[i] + CStr(" / Cost Plasma: ") + SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[i]);

            m_Resources[i] = max(0, m_Resources[i] - SRobotTemplate::m_AIRobotTypesList[cur_side][template_num].m_Resources[i]);
        }

        if(SRobotTemplate::m_AIRobotTypesList[cur_side][template_num].m_HaveBomb) m_TimeLastBomb = g_MatrixMap->GetTime(); //Запоминаем время постройки робота с бомбой для ограничения частоты постройки бомберов

        m_Constructor->SetBase(base);
        m_Constructor->BuildRobotByTemplate(SRobotTemplate::m_AIRobotTypesList[cur_side][template_num]);

        //Контроль цепочки построек до трёх шаблонов подряд здесь нужен, чтобы контролировать частоту спавна роботов-ремонтников
        m_LastBuildedRobot3 = m_LastBuildedRobot2;
        m_LastBuildedRobot2 = m_LastBuildedRobot;
        m_LastBuildedRobot = template_num;

        DMSide(L"BuildRobot BuildRobotStrength=<f>", SRobotTemplate::m_AIRobotTypesList[cur_side][template_num].m_Strength);
    }

    /*
    //Время, сколько можно подождать
    int wait_time = 10000;
    if(m_Region[GetRegion(base)].m_EnemyRobotDist >= 0) wait_time = 10000 * max(0, m_Region[GetRegion(base)].m_EnemyRobotDist - 1);
    else if(m_Region[GetRegion(base)].m_EnemyBuildingDist >= 0) wait_time = 10000 * max(0, m_Region[GetRegion(base)].m_EnemyBuildingDist - 1);
    wait_time = int(m_WaitResMultiplier * float(min(wait_time, 40000))); //Нет смысла долго ждать

    //Сколько нужно ждать до запланированного робота
    int wait_end = -1;
    if(m_WaitResForBuildRobot >= 0)
    {
        wait_end = 0;
        for(r = 0; r < MAX_RESOURCES; ++r)
        {
            k = (SRobotTemplate::m_AIRobotTypesList[cur_side][m_WaitResForBuildRobot].m_Resources[r] - m_Resources[r]);
            if(k > 0)
            {
                if(res_income[r] <= 0 && bases_cnt <= 0)
                {
                    wait_end = 1000000000;
                    break;
                }

                wait_end = max(wait_end, int(float(k * g_Config.m_Timings[r]) / float(res_income[r] * RESOURCES_INCOME_FROM_FACTORY + (RESOURCES_INCOME_FROM_BASE * GetResourceForceUp() / 100 * bases_cnt))));
            }
        }
    }

    //Сколько будет ресурсов, если немного подождать
    //(я хз что это за дабокостыли, если честно)
    int min_res_potential = 0; //Я же правильно тебя понял, даб? mr в твоём понимании значит именно это?
    for(r = 0; r < MAX_RESOURCES; ++r) min_res_potential += min(2000, m_Resources[r]);
    min_res_potential = int(float(min_res_potential / MAX_RESOURCES) * 0.6f);

    for(r = 0; r < MAX_RESOURCES; ++r)
    {
        res_income[r] = (RESOURCES_INCOME_FROM_FACTORY * res_income[r] + (RESOURCES_INCOME_FROM_BASE * GetResourceForceUp() / 100 * bases_cnt)) * (wait_time / g_Config.m_Timings[r]) + m_Resources[r];
    }

    //Создаём список роботов, которых можем построить сразу, и список тех, которых построим, если немного подождём
    int* list = (int*)HAlloc(SRobotTemplate::m_AIRobotTypesList[cur_side].size() * sizeof(int) * 2, Base::g_MatrixHeap);
    int* list_wait = list + SRobotTemplate::m_AIRobotTypesList[cur_side].size();
    int cnt = 0;
    list_wait_cnt = 0;

    bool BS_NO_BOMB = ((m_LastBuildedRobot >= 0) && SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot].m_HaveBomb)   ||
                      ((m_LastBuildedRobot2 >= 0) && SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot2].m_HaveBomb) ||
                      ((m_LastBuildedRobot3 >= 0) && SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot3].m_HaveBomb);

    if(!BS_NO_BOMB)
    {
        if((g_MatrixMap->GetTime() - m_TimeLastBomb) < m_TimeNextBomb) BS_NO_BOMB = true;
    }

    bool no_repair = ((m_LastBuildedRobot >= 0) && SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot].m_HaveRepair)   ||
                     ((m_LastBuildedRobot2 >= 0) && SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot2].m_HaveRepair) ||
                     ((m_LastBuildedRobot3 >= 0) && SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot3].m_HaveRepair);

    for(i = 0; i < SRobotTemplate::m_AIRobotTypesList[cur_side].size(); ++i)
    {
        if(BS_NO_BOMB && SRobotTemplate::m_AIRobotTypesList[cur_side][i].m_HaveBomb) continue;
        if(no_repair && SRobotTemplate::m_AIRobotTypesList[cur_side][i].m_HaveRepair) continue;
        if(!no_repair && !SRobotTemplate::m_AIRobotTypesList[cur_side][i].m_HaveRepair) continue;
        if(SRobotTemplate::m_AIRobotTypesList[cur_side][i].m_Strength < enemy_superiority) continue; // Сила должна быть больше минимальной

        for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < SRobotTemplate::m_AIRobotTypesList[cur_side][i].m_Resources[r]) break;

        if(r >= MAX_RESOURCES)
        {
            //Одинаковых роботов не строим, так как не разнообразно и некрасиво
            if(i != m_WaitResForBuildRobot && m_LastBuildedRobot >= 0 && SRobotTemplate::m_AIRobotTypesList[cur_side][i].DifWeapon(SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot]) > 0.6);
            else if(i != m_WaitResForBuildRobot && m_LastBuildedRobot2 >= 0 && SRobotTemplate::m_AIRobotTypesList[cur_side][i].DifWeapon(SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot2]) > 0.8);
            else
            {
                list[cnt] = i;
                ++cnt;
            }
        }
        else
        {
            for(r = 0; r < MAX_RESOURCES; ++r) if(res_income[r] < SRobotTemplate::m_AIRobotTypesList[cur_side][i].m_Resources[r]) break;
            if(r >= MAX_RESOURCES)
            {
                //Одинаковых роботов не строим, так как не разнообразно и некрасиво
                if(i != m_WaitResForBuildRobot && m_LastBuildedRobot >= 0 && SRobotTemplate::m_AIRobotTypesList[cur_side][i].DifWeapon(SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot]) > 0.6);
                else if(i != m_WaitResForBuildRobot && m_LastBuildedRobot2 >= 0 && SRobotTemplate::m_AIRobotTypesList[cur_side][i].DifWeapon(SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot2]) > 0.8);
                //else if(i != m_WaitResForBuildRobot && m_LastBuildedRobot3 >= 0 && SRobotTemplate::m_AIRobotTypesList[cur_side][i].DifWeapon(SRobotTemplate::m_AIRobotTypesList[cur_side][m_LastBuildedRobot3]) > 0.9);
                else
                {
                    list_wait[list_wait_cnt] = i;
                    ++list_wait_cnt;
                }
            }
        }
    }

    if(cnt + list_wait_cnt <= 0)
    {
        HFree(list, Base::g_MatrixHeap);
        return;
    }

    //Если дождались ожидаемого робота, то строим его
    if(m_WaitResForBuildRobot >= 0)
    {
        for(i = 0; i < cnt; ++i) if(list[i] == m_WaitResForBuildRobot) break;

        if(i < cnt)
        {
            if(base->m_BuildingQueue.GetItemsCnt() < 6)
            {
                for(r = 0; r < MAX_RESOURCES; ++r) m_Resources[r] = max(0, m_Resources[r] - SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[r]);
                m_Constructor->SetBase(base);
                m_Constructor->BuildRobotByTemplate(SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]]);

                if(SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_HaveBomb) m_TimeLastBomb = g_MatrixMap->GetTime();

                m_LastBuildedRobot3 = m_LastBuildedRobot2;
                m_LastBuildedRobot2 = m_LastBuildedRobot;
                m_LastBuildedRobot = list[i];

                DMSide(L"BuildRobot BuildWaitRobotStrength=<f>", SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Strength);
                HFree(list, Base::g_MatrixHeap);
            }

            m_WaitResForBuildRobot = -1;
            return;
        }
    }

    //Если период слишком большой, то сбрасываем ожидаемого робота
    if(wait_end >= 0 && wait_end > wait_time)
    {
        DMSide(L"BuildRobot CancelWaitRobotStrength=<f>", SRobotTemplate::m_AIRobotTypesList[cur_side][m_WaitResForBuildRobot].m_Strength);
        m_WaitResForBuildRobot = -1;
    }

    //Ожидаем, пока накопятся ресурсы
    if(m_WaitResForBuildRobot >= 0)
    {
        HFree(list, Base::g_MatrixHeap);
        return;
    }

    DMSide(L"BuildRobot MinStrength=<f>", enemy_superiority);
    DMSide(L"BuildRobot ImmediatelyCnt=<i> WaitCnt=<i>", cnt, list_wait_cnt);

    //Стоит ли ожидать
    if(cnt > 0 && list_wait_cnt > 0 && SRobotTemplate::m_AIRobotTypesList[cur_side][list_wait[0]].m_Strength * 0.6 > SRobotTemplate::m_AIRobotTypesList[cur_side][list[0]].m_Strength)
    {
        //Выбираем из лучших, случайно по приоритету
        for(i = 1; i < list_wait_cnt; ++i) if(SRobotTemplate::m_AIRobotTypesList[cur_side][list_wait[i]].m_Strength < 0.7 * SRobotTemplate::m_AIRobotTypesList[cur_side][list_wait[0]].m_Strength) break;
        list_wait_cnt = i;

        for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < min_res_potential) break;
        if(r < MAX_RESOURCES) //Сортируем по ресурсам, которых мало
        {
            for(i = 0; i < list_wait_cnt - 1; ++i)
            {
                ik = 0;
                for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < min_res_potential) ik += SRobotTemplate::m_AIRobotTypesList[cur_side][list_wait[i]].m_Resources[r];

                for(u = i + 1; u < list_wait_cnt; ++u)
                {
                    uk = 0;
                    for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < min_res_potential) uk += SRobotTemplate::m_AIRobotTypesList[cur_side][list_wait[u]].m_Resources[r];

                    if(uk < ik)
                    {
                        int temp = list_wait[u];
                        list_wait[u] = list_wait[i];
                        list_wait[i] = temp;
                        temp = uk;
                        uk = ik;
                        ik = temp;
                    }
                }
            }

            ik = 0;
            for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < min_res_potential) ik += SRobotTemplate::m_AIRobotTypesList[cur_side][list_wait[0]].m_Resources[r];
            ik += ik / 10;
            for(i = 1; i < list_wait_cnt; ++i)
            {
                uk = 0;
                for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < min_res_potential) uk += SRobotTemplate::m_AIRobotTypesList[cur_side][list_wait[i]].m_Resources[r];
                if(uk > ik) break;
            }

            list_wait_cnt = i;
        }

        k = 0;
        for(i = 0; i < list_wait_cnt; ++i) k += SRobotTemplate::m_AIRobotTypesList[cur_side][list_wait[i]].m_TemplatePriority;
        k = g_MatrixMap->Rnd(0, k - 1);

        for(i = 0; i < list_wait_cnt; ++i)
        {
            k -= SRobotTemplate::m_AIRobotTypesList[cur_side][list_wait[i]].m_TemplatePriority;
            if(k < 0) break;
        }
        if(i >= list_wait_cnt) ERROR_E;

        m_WaitResForBuildRobot = list_wait[i];
        DMSide(L"BuildRobot WaitRobotStrength=<f>", SRobotTemplate::m_AIRobotTypesList[cur_side][m_WaitResForBuildRobot].m_Strength);
    }
    else if(cnt > 0) //Строим сразу
    {
        //Выбираем из лучших, случайно по приоритету
        for(i = 1; i < cnt; ++i) if(SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Strength < 0.7 * SRobotTemplate::m_AIRobotTypesList[cur_side][list[0]].m_Strength) break;
        
        cnt = i;
        for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < min_res_potential) break;

        if(r < MAX_RESOURCES) //Сортируем по ресурсам, которых мало
        {
            for(i = 0; i < cnt - 1; ++i)
            {
                ik = 0;
                for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < min_res_potential) ik += SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[r];

                for(u = i + 1; u < cnt; ++u)
                {
                    uk = 0;
                    for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < min_res_potential) uk += SRobotTemplate::m_AIRobotTypesList[cur_side][list[u]].m_Resources[r];

                    if(uk < ik)
                    {
                        int temp = list[u];
                        list[u] = list[i];
                        list[i] = temp;
                        temp = uk;
                        uk = ik;
                        ik = temp;
                    }
                }
            }

            ik = 0;
            for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < min_res_potential) ik += SRobotTemplate::m_AIRobotTypesList[cur_side][list[0]].m_Resources[r];
            ik += ik / 10;
            for(i = 1; i < cnt; ++i)
            {
                uk = 0;
                for(r = 0; r < MAX_RESOURCES; ++r) if(m_Resources[r] < min_res_potential) uk += SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[r];
                if(uk > ik) break;
            }

            cnt = i;
        }

        k = 0;
        for(i = 0; i < cnt; ++i) k += SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_TemplatePriority;
        k = g_MatrixMap->Rnd(0, k - 1);

        for(i = 0; i < cnt; ++i)
        {
            k -= SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_TemplatePriority;
            if(k < 0) break;
        }

        if(i >= cnt) ERROR_E;

        if(base->m_BuildingQueue.GetItemsCnt() < 6)
        {
            //Списываем ресурсы за постройку выбранного шаблона робота
            //SFT("---");
            //SFT(CStr("Current side: ") + m_Id);
            //SFT(CStr("Template: ") + CStr(SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_StringTemplate));

            for(r = 0; r < MAX_RESOURCES; ++r)
            {
                //if(!r) SFT(CStr("Current Titan: ") + CStr(m_Resources[r]) + CStr(" / Cost Titan: ") + SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[r]);
                //else if(r == 1) SFT(CStr("Current Chip: ") + CStr(m_Resources[r]) + CStr(" / Cost Chip: ") + SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[r]);
                //else if(r == 2) SFT(CStr("Current Bats: ") + m_Resources[r] + CStr(" / Cost Bats: ") + SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[r]);
                //else SFT(CStr("Current Plasma: ") + m_Resources[r] + CStr(" / Cost Plasma: ") + SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[r]);

                m_Resources[r] = max(0, m_Resources[r] - SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Resources[r]);
            }

            m_Constructor->SetBase(base);
            m_Constructor->BuildRobotByTemplate(SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]]);

            if(SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_HaveBomb) m_TimeLastBomb = g_MatrixMap->GetTime();

            m_LastBuildedRobot3 = m_LastBuildedRobot2;
            m_LastBuildedRobot2 = m_LastBuildedRobot;
            m_LastBuildedRobot = list[i];

            DMSide(L"BuildRobot BuildRobotStrength=<f>", SRobotTemplate::m_AIRobotTypesList[cur_side][list[i]].m_Strength);
        }
    }

    HFree(list, Base::g_MatrixHeap);
    */
}

void CMatrixSideUnit::ChooseAndBuildAICannon()
{
    CMatrixBuilding* building = nullptr;
    int mdist = 0;

    int max_robot = GetMaxSideRobots();
    int cur_cannon_count = 0;

    CMatrixMapStatic* mps = CMatrixMapStatic::GetFirstLogic();
    while(mps)
    {
        if(mps->IsActiveTurretAlive() && mps->GetSide() == m_Id) ++cur_cannon_count;
        mps = mps->GetNextLogic();
    }

    if(m_RobotsCnt < max_robot && cur_cannon_count >= m_RobotsCnt) return;

    // Ищем здание для постройки пушки
    mps = CMatrixMapStatic::GetFirstLogic();
    while(mps)
    {
        if(mps->GetSide() == m_Id && mps->IsBuildingAlive())
        {
            CMatrixBuilding* cb = mps->AsBuilding();

            while(true)
            {
                if(cb->HaveMaxTurrets()) break; //Пропускаем если все места заняты

                int region = GetRegion(mps);
                if(m_Region[region].m_EnemyForces / max(m_Region[region].m_OurForces, 0.0001f) > 1.0 && !cb->IsBase()) break; //Пропускаем здание, если возле слишком много врагов (сравниваем по коэффициенту угрозы)

                if(building)
                {
                    if(m_Region[region].m_EnemyRobotDist >= mdist) break; //Выбираем для постройки турели здание, наиболее близкое к врагам
                }

                mdist = m_Region[region].m_EnemyRobotDist;
                building = cb;
            }
        }

        mps = mps->GetNextLogic();
    }

    if(!building) return;

    int ct[TURRET_KINDS_TOTAL + 1];
    memset(ct, 0, sizeof(ct));

    mps = CMatrixMapStatic::GetFirstLogic();
    while(mps)
    {
        if(mps->IsActiveTurretAlive() && mps->AsTurret()->m_ParentBuilding == building)
        {
            ASSERT(mps->AsTurret()->m_Num >= 1 && mps->AsTurret()->m_Num <= TURRET_KINDS_TOTAL);
            ++ct[mps->AsTurret()->m_TurretKind];
        }

        mps = mps->GetNextLogic();
    }

    mps = building->m_BuildingQueue.GetTopItem();
    while(mps)
    {
        if(mps->GetObjectType() == OBJECT_TYPE_TURRET)
        {
            CMatrixTurret* cannon = mps->AsTurret();
            ASSERT(cannon->m_Num >= 1 && cannon->m_Num <= TURRET_KINDS_TOTAL);
            ++ct[cannon->m_TurretKind];
        }

        mps = mps->m_NextQueueItem;
    }

    //Выбор подходящей турели из доступных
    int vmin = ct[1];
    for(int i = 2; i <= TURRET_KINDS_TOTAL; ++i) vmin = min(ct[i], vmin);

    int cur_type;
    while(true)
    {
        cur_type = g_MatrixMap->Rnd(1, TURRET_KINDS_TOTAL);
        if(vmin == ct[cur_type]) break;
    }

    //Если не хватает ресурсов на выбранную турель
    STurretsConsts* turret_data = &g_Config.m_TurretsConsts[cur_type];
    if(turret_data->cost_titan > m_Resources[TITAN]) return;
    if(turret_data->cost_electronics > m_Resources[ELECTRONICS]) return;
    if(turret_data->cost_energy > m_Resources[ENERGY]) return;
    if(turret_data->cost_plasma > m_Resources[PLASMA]) return;

    if(building->m_BuildingQueue.GetItemsCnt() > 0) return; //Если уже что-то строится, то не строим

    CPoint place_list[MAX_PLACES];
    int place_list_cnt = building->GetPlacesForTurrets(place_list);
    if(!place_list_cnt) return;

    //Списываем ресурсы за постройку
    m_Resources[TITAN] -= turret_data->cost_titan;
    m_Resources[ELECTRONICS] -= turret_data->cost_electronics;
    m_Resources[ENERGY] -= turret_data->cost_energy;
    m_Resources[PLASMA] -= turret_data->cost_plasma;

    CPoint place_cannon = place_list[g_MatrixMap->Rnd(0, place_list_cnt - 1)];
    CMatrixTurret* cannon = g_MatrixMap->StaticAdd<CMatrixTurret>(true);
    cannon->m_CurrentState = TURRET_UNDER_CONSTRUCTION;
    cannon->SetInvulnerability();

    cannon->m_Pos.x = place_cannon.x * GLOBAL_SCALE_MOVE;
    cannon->m_Pos.y = place_cannon.y * GLOBAL_SCALE_MOVE;

    cannon->m_Angle = 0;
    for(int i = 0; i < building->m_TurretsLimit; ++i)
    {
        if(building->m_TurretsPlaces[i].m_Coord == place_cannon)
        {
            cannon->m_Angle = building->m_TurretsPlaces[i].m_Angle;
            cannon->m_AddH = 0; //building->m_TurretsPlaces[i].m_AddH;
            break;
        }
    }

    cannon->m_Place = g_MatrixMap->m_RoadNetwork.FindInPL(place_cannon);
    cannon->SetSide(m_Id);
    cannon->ModelInit(cur_type);

    cannon->GetResources(MR_Matrix | MR_Graph);
    cannon->m_ParentBuilding = building;
    cannon->JoinToGroup();

    cannon->m_ParentBuilding->m_TurretsHave++;
    cannon->SetHitpoints(0);
    building->m_BuildingQueue.AddItem(cannon);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CMatrixSideUnit::TactPL(int only_group)
{
    int i, u, t, x, y;
    byte mm = 0;
    CPoint tp;
    CMatrixMapStatic* obj;
    CMatrixBuilding* building;
    CMatrixRobotAI* robot;
    CMatrixRobotAI* robot2;
    bool order_ok[MAX_LOGIC_GROUP];

    //Запускаем логику раз в 100 тактов
    if(m_LastTactHL && (g_MatrixMap->GetTime() - m_LastTactHL) < 100) return;
    m_LastTactHL = g_MatrixMap->GetTime();

    CalcStrength();
    EscapeFromBomb();

    // Для всех мест рассчитываем коэффициент вражеских объектов в зоне поражения
    if(m_LastTactUnderfire == 0 || (g_MatrixMap->GetTime() - m_LastTactUnderfire) > 500)
    {
        m_LastTactUnderfire = g_MatrixMap->GetTime();

        SMatrixPlace* place = g_MatrixMap->m_RoadNetwork.m_Place;
        for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_PlaceCnt; ++i, ++place) place->m_Underfire = 0;

        obj = CMatrixMapStatic::GetFirstLogic();
        while(obj)
        {
            if(obj->IsUnitAlive() && obj->GetSide() != m_Id)
            {
                tp = GetMapPos(obj);
                CRect rect(1000000000, 1000000000, -1000000000, -1000000000);
                rect.left = min(rect.left, tp.x);
                rect.top = min(rect.top, tp.y);
                rect.right = max(rect.right, tp.x + ROBOT_MOVECELLS_PER_SIZE);
                rect.bottom = max(rect.bottom, tp.y + ROBOT_MOVECELLS_PER_SIZE);

                tp.x += ROBOT_MOVECELLS_PER_SIZE >> 1;
                tp.y += ROBOT_MOVECELLS_PER_SIZE >> 1;

                int firedist = 0;
                int firedist2 = 0;
                if(obj->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
                {
                    firedist = Float2Int(((CMatrixRobotAI*)(obj))->GetMaxFireDist() + GLOBAL_SCALE_MOVE);
                    firedist2 = Float2Int(((CMatrixRobotAI*)(obj))->GetMinFireDist() + GLOBAL_SCALE_MOVE);
                }
                else if(obj->GetObjectType() == OBJECT_TYPE_TURRET)
                {
                    firedist = Float2Int(((CMatrixTurret*)(obj))->GetFireRadius() + GLOBAL_SCALE_MOVE);
                    firedist2 = firedist;
                }
                firedist = firedist / int(GLOBAL_SCALE_MOVE);
                firedist2 = firedist2 / int(GLOBAL_SCALE_MOVE);

                CRect plr = g_MatrixMap->m_RoadNetwork.CorrectRectPL(CRect(rect.left - firedist, rect.top - firedist, rect.right + firedist, rect.bottom + firedist));

                firedist *= firedist;
                firedist2 *= firedist2;

                SMatrixPlaceList* plist = g_MatrixMap->m_RoadNetwork.m_PLList + plr.left + plr.top * g_MatrixMap->m_RoadNetwork.m_PLSizeX;
                for(y = plr.top; y < plr.bottom; ++y, plist += g_MatrixMap->m_RoadNetwork.m_PLSizeX - (plr.right - plr.left))
                {
                    for(x = plr.left; x < plr.right; ++x, ++plist)
                    {
                        SMatrixPlace* place = g_MatrixMap->m_RoadNetwork.m_Place + plist->m_Sme;
                        for(u = 0; u < plist->m_Cnt; ++u, ++place)
                        {
                            int pcx = place->m_Pos.x + int(ROBOT_MOVECELLS_PER_SIZE / 2); // Center place
                            int pcy = place->m_Pos.y + int(ROBOT_MOVECELLS_PER_SIZE / 2);

                            int d = (POW2(tp.x - pcx) + POW2(tp.y - pcy));
                            if(firedist >= d) place->m_Underfire++;
                            if(firedist2 >= d) place->m_Underfire++;
                        }
                    }
                }
            }

            obj = obj->GetNextLogic();
        }
    }

    //Собираем статистику
    for(i = 0; i < MAX_LOGIC_GROUP; ++i) m_PlayerGroup[i].m_RobotCnt = 0;

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
        {
            if(GetEnv(obj)->m_OrderNoBreak && obj->AsRobot()->CanBreakOrder())
            {
                GetEnv(obj)->m_OrderNoBreak = false;
                if(obj->AsRobot()->GetGroupLogic() < 0 || m_PlayerGroup[obj->AsRobot()->GetGroupLogic()].Order() != mpo_MoveTo)
                {
                    GetEnv(obj)->m_Place = -1;
                    GetEnv(obj)->m_PlaceAdd = CPoint(-1, -1);
                }
            }

            if(obj->AsRobot()->GetGroupLogic() >= 0 && obj->AsRobot()->GetGroupLogic() < MAX_LOGIC_GROUP)
            {
                m_PlayerGroup[obj->AsRobot()->GetGroupLogic()].m_RobotCnt++;
            }
        }

        obj = obj->GetNextLogic();
    }

    //Проверяем, корректна ли цель
    for(i = 0; i < MAX_LOGIC_GROUP; ++i)
    {
        if(m_PlayerGroup[i].m_RobotCnt <= 0) continue;
        if(!m_PlayerGroup[i].m_Obj) continue;

        obj = CMatrixMapStatic::GetFirstLogic();
        while(obj)
        {
            if(obj == m_PlayerGroup[i].m_Obj) break;
            obj = obj->GetNextLogic();
        }
        if(!obj || !obj->IsAlive()) m_PlayerGroup[i].m_Obj = nullptr;

        // Если цель атаки близко, то заносим её во врагов
        if((m_PlayerGroup[i].Order() == mpo_Attack || m_PlayerGroup[i].Order() == mpo_AutoAttack || m_PlayerGroup[i].Order() == mpo_AutoDefence) && m_PlayerGroup[i].m_Obj && m_PlayerGroup[i].m_Obj->IsAlive())
        {
            tp = GetMapPos(m_PlayerGroup[i].m_Obj);

            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                {
                    robot = (CMatrixRobotAI*)obj;

                    if(tp.Dist2(GetMapPos(robot)) < POW2(30))
                    {
                        if(!robot->GetEnv()->SearchEnemy(m_PlayerGroup[i].m_Obj))
                        {
                            robot->GetEnv()->AddToList(m_PlayerGroup[i].m_Obj);
                        }
                    }
                }

                obj = obj->GetNextLogic();
            }
        }
    }

    for(i = 0; i < MAX_LOGIC_GROUP; ++i)
    {
        if(m_PlayerGroup[i].m_RobotCnt <= 0) continue;
        if(m_PlayerGroup[i].Order() == mpo_Repair) RepairPL(i);
        else if(m_PlayerGroup[i].IsWar()) WarPL(i);
        else if(!FirePL(i)) RepairPL(i);
    }

    //Успешно ли выполняется текущий приказ
    for(i = 0; i < MAX_LOGIC_GROUP; ++i)
    {
        if(only_group >= 0 && i != only_group) continue;
        if(m_PlayerGroup[i].m_RobotCnt <= 0) continue;
        order_ok[i] = true;

        bool prev_war = m_PlayerGroup[i].IsWar();
        m_PlayerGroup[i].SetWar(false);

        if(m_PlayerGroup[i].Order() == mpo_Stop)
        {
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                {
                    robot = (CMatrixRobotAI*)obj;
                    if(!PLIsToPlace(robot))
                    {
                        order_ok[i] = false;
                        break;
                    }
                }

                obj = obj->GetNextLogic();
            }
        }
        else if(m_PlayerGroup[i].Order() == mpo_MoveTo)
        {
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                {
                    robot = (CMatrixRobotAI*)obj;
                    if(!PLIsToPlace(robot))
                    {
                        if(CanChangePlace(robot))
                        {
                            order_ok[i] = false;
                            break;
                        }
                    }
                }

                obj = obj->GetNextLogic();
            }
        }
        else if(m_PlayerGroup[i].Order() == mpo_Patrol)
        {
            t = 1;
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                {
                    robot = obj->AsRobot();
                    if(robot->GetEnv()->GetEnemyCnt())
                    {
                        //order_ok[i] = true;
                        m_PlayerGroup[i].SetWar(true);
                        if(!prev_war) PGPlaceClear(i);
                        t = 0;
                        break;
                    }

                    if(!PLIsToPlace(robot))
                    {
                        if(CanChangePlace(robot))
                        {
                            order_ok[i] = false;
                            break;
                        }
                    }

                    if(!robot->PLIsInPlace()) t = 0;
                }
                obj = obj->GetNextLogic();
            }

            if(t)
            {
                order_ok[i] = false;
                m_PlayerGroup[i].SetPatrolReturn(!m_PlayerGroup[i].IsPatrolReturn());
            }
        }
        else if(m_PlayerGroup[i].Order() == mpo_Repair)
        {
            if(m_PlayerGroup[i].m_Obj == nullptr || !m_PlayerGroup[i].m_Obj->NeedRepair())
            {
                PGOrderStop(i);
            }
        }
        else if(m_PlayerGroup[i].Order() == mpo_Capture)
        {
            u = 0;
            t = 0;
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
                {
                    building = obj->AsRobot()->GetCaptureBuilding();
                    if(building && building == m_PlayerGroup[i].m_Obj)
                    {
                        ++u;
                        break;
                    }
                }
                if(m_PlayerGroup[i].m_Obj == obj && obj->IsBuildingAlive() && obj->GetSide() != m_Id)
                {
                    t = 1;
                }
                obj = obj->GetNextLogic();
            }

            if(!t) //Нечего захватывать
            {
                PGOrderStop(i);
                continue;
            }
            order_ok[i] = (u > 0);

            //Если приказ захватить базу, но есть пушки, то атаковать пушки
            if(m_PlayerGroup[i].m_Obj->IsBase() && m_PlayerGroup[i].m_Obj->AsBuilding()->m_TurretsHave)
            {
                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsActiveTurretAlive() && obj->AsTurret()->m_ParentBuilding == m_PlayerGroup[i].m_Obj)
                    {
                        CMatrixMapStatic *obj2 = CMatrixMapStatic::GetFirstLogic();
                        while(obj2)
                        {
                            if(obj2->IsRobotAlive() && obj2->GetSide() == m_Id && obj2->AsRobot()->GetGroupLogic() == i)
                            {
                                robot = obj2->AsRobot();
                                if(robot->GetEnv()->SearchEnemy(obj)) break;
                            }
                            obj2 = obj2->GetNextLogic();
                        }
                        if(obj2) break;
                    }
                    obj = obj->GetNextLogic();
                }

                if(obj)
                {
                    CMatrixMapStatic *obj2 = CMatrixMapStatic::GetFirstLogic();
                    while(obj2)
                    {
                        if(obj2->IsRobotAlive() && obj2->GetSide() == m_Id && ((CMatrixRobotAI*)obj2)->GetGroupLogic() == i)
                        {
                            robot = (CMatrixRobotAI*)obj2;
                            if(robot->GetCaptureBuilding() && robot->CanBreakOrder())
                            {
                                robot->BreakAllOrders();
                            }
                        }
                        obj2 = obj2->GetNextLogic();
                    }
                    m_PlayerGroup[i].SetWar(true);

                    if(FLAG(g_MatrixMap->m_Flags, MMFLAG_ENABLE_CAPTURE_FUCKOFF_SOUND))
                    {
                        CSound::Play(S_ORDER_CAPTURE_FUCK_OFF);
                        RESETFLAG(g_MatrixMap->m_Flags, MMFLAG_ENABLE_CAPTURE_FUCKOFF_SOUND);
                    }

                    if(!prev_war) PGPlaceClear(i);
                    order_ok[i] = true;
                    continue;
                }
            }

            if(order_ok[i]) //Проверяем, у всех ли правильно назначено место
            {
                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                    {
                        robot = (CMatrixRobotAI*)obj;

                        if(robot->GetEnv()->m_Place < 0 && robot->GetEnv()->m_PlaceAdd.x < 0) break;
                    }
                    obj = obj->GetNextLogic();
                }

                if(obj)
                {
                    if(CanChangePlace(robot))
                    {
                        order_ok[i] = false;
                    }
                }
            }
        }
        else if(m_PlayerGroup[i].Order() == mpo_Attack)
        {
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                {
                    robot = (CMatrixRobotAI*)obj;
                    if(m_PlayerGroup[i].m_Obj && robot->GetEnv()->SearchEnemy(m_PlayerGroup[i].m_Obj))
                    {
                        //if(robot->GetEnv()->m_Target != m_PlayerGroup[i].m_Obj) robot->GetEnv()->m_Target = m_PlayerGroup[i].m_Obj;
                        //order_ok[i] = true;
                        m_PlayerGroup[i].SetWar(true);
                        if(!prev_war) PGPlaceClear(i);
                        break;
                    }
                    else if(!m_PlayerGroup[i].m_Obj && robot->GetEnv()->GetEnemyCnt())
                    {
                        //order_ok[i] = true;
                        m_PlayerGroup[i].SetWar(true);
                        if(!prev_war) PGPlaceClear(i);
                        break;
                    }

                    if(!PLIsToPlace(robot))
                    {
                        if(CanChangePlace(robot))
                        {
                            order_ok[i] = false;
                            break;
                        }
                    }
                }
                obj = obj->GetNextLogic();
            }

            if(!m_PlayerGroup[i].IsWar() && prev_war) //Если до этого находились в состоянии войны, то заново назначаем маршрут
            {
                order_ok[i] = false;
                continue;
            }

            if(!m_PlayerGroup[i].IsWar() && order_ok[i]) //Если нет войны, и правильно идем до места, проверяем, сильно ли изменила цель свою позицию
            {
                tp = m_PlayerGroup[i].m_To;
                if(m_PlayerGroup[i].m_Obj) tp = GetMapPos(m_PlayerGroup[i].m_Obj);

                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                    {
                        robot = (CMatrixRobotAI*)obj;

                        if(tp.Dist2(PLPlacePos(robot)) < POW2(15)) break;
                    }
                    obj = obj->GetNextLogic();
                }

                if(!obj)
                {
                    order_ok[i] = false;
                    continue;
                }
            }
        }
        else if(m_PlayerGroup[i].Order() == mpo_Bomb)
        {
            t = 0;
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                {
                    robot = (CMatrixRobotAI*)obj;
                    if(robot->HaveBomb())
                    {
                        ++t;
                        if(robot->PLIsInPlace())
                        {
                            robot->BeginBombCountdown();
                            --t;
                        }
                        else if(PLPlacePos(robot).Dist2(GetMapPos(robot)) < POW2(2))
                        {
                            robot->BeginBombCountdown();
                            --t;
                        }
                        else if(m_PlayerGroup[i].m_Obj && m_PlayerGroup[i].m_Obj->IsAlive())
                        {
                            D3DXVECTOR2 temp = GetWorldPos(robot) - GetWorldPos(m_PlayerGroup[i].m_Obj);
                            if(D3DXVec2LengthSq(&temp) < POW2(150))
                            {
                                robot->BeginBombCountdown();
                                --t;
                            }
                        }
                    }

                    if(!PLIsToPlace(robot))
                    {
                        if(CanChangePlace(robot))
                        {
                            order_ok[i] = false;
                        }
                    }
                }
                obj = obj->GetNextLogic();
            }

            if(t <= 0)
            {
                PGOrderStop(i);
                continue;
            }

            if(order_ok[i]) // И правильно идем по места, проверяем, сильно ли изменила цель свою позицию
            {
                tp = m_PlayerGroup[i].m_To;
                if(m_PlayerGroup[i].m_Obj) tp = GetMapPos(m_PlayerGroup[i].m_Obj);

                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                    {
                        robot = (CMatrixRobotAI*)obj;

                        if(tp.Dist2(PLPlacePos(robot)) < POW2(10)) break;
                    }
                    obj = obj->GetNextLogic();
                }
                if(!obj)
                {
                    order_ok[i] = false;
                    continue;
                }
            }
        }
        else if(m_PlayerGroup[i].Order() == mpo_AutoCapture)
        {
            float strange = 0.0f;
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                {
                    strange += obj->AsRobot()->GetStrength();
                }
                obj = obj->GetNextLogic();
            }

            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                {
                    robot = (CMatrixRobotAI*)obj;
                    if(robot->GetEnv()->GetEnemyCnt())
                    {
                        //order_ok[i] = true;
                        if(strange >= 1.0f) m_PlayerGroup[i].SetWar(true);
                        if(!prev_war) PGPlaceClear(i);
                        break;
                    }
                }
                obj = obj->GetNextLogic();
            }
            if(m_PlayerGroup[i].IsWar()) continue;

            if(!m_PlayerGroup[i].m_Obj || !m_PlayerGroup[i].m_Obj->IsBuildingAlive() || m_PlayerGroup[i].m_Obj->GetSide() == m_Id)
            {
                m_PlayerGroup[i].m_Obj = nullptr;
                order_ok[i] = false;
                continue;
            }

            u = 0;
            t = 0;
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
                {
                    building = obj->AsRobot()->GetCaptureBuilding();
                    if(building && building == m_PlayerGroup[i].m_Obj)
                    {
                        ++u;
                        break;
                    }
                }

                if(m_PlayerGroup[i].m_Obj == obj && obj->IsBuildingAlive() && obj->GetSide() != m_Id) t = 1;
                obj = obj->GetNextLogic();
            }

            if(!t) // Нечего захватывать
            {
                order_ok[i] = false;
                continue;
            }

            order_ok[i] = (u > 0);
            if(order_ok[i]) // Проверяем у всех ли правильно назначено место
            {
                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                    {
                        robot = (CMatrixRobotAI*)obj;

                        if(robot->GetEnv()->m_Place < 0 && robot->GetEnv()->m_PlaceAdd.x < 0) break;
                    }
                    obj = obj->GetNextLogic();
                }
                if(obj)
                {
                    if(CanChangePlace(robot))
                    {
                        order_ok[i] = false;
                    }
                }
            }
        }
        else if(m_PlayerGroup[i].Order() == mpo_AutoAttack)
        {
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                {
                    robot = (CMatrixRobotAI*)obj;
                    if(robot->GetEnv()->GetEnemyCnt())
                    {
                        //order_ok[i] = true;
                        m_PlayerGroup[i].SetWar(true);
                        if(!prev_war) PGPlaceClear(i);
                        break;
                    }

                    if(!PLIsToPlace(robot))
                    {
                        if(CanChangePlace(robot))
                        {
                            order_ok[i] = false;
                            break;
                        }
                    }
                }
                obj = obj->GetNextLogic();
            }

            if(m_PlayerGroup[i].IsWar()) continue;

            if(!m_PlayerGroup[i].m_Obj || !m_PlayerGroup[i].m_Obj->IsAlive())
            {
                m_PlayerGroup[i].m_Obj = nullptr;
                order_ok[i] = false;
                continue;
            }

            if(!m_PlayerGroup[i].IsWar() && order_ok[i]) // Если нет войны, и правильно идем по места, проверяем, сильно ли изменила цель свою позицию
            {
                tp = m_PlayerGroup[i].m_To;
                if(m_PlayerGroup[i].m_Obj) tp = GetMapPos(m_PlayerGroup[i].m_Obj);

                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                    {
                        robot = (CMatrixRobotAI*)obj;
                        if(tp.Dist2(PLPlacePos(robot)) < POW2(15)) break;
                    }
                    obj = obj->GetNextLogic();
                }

                if(!obj)
                {
                    order_ok[i] = false;
                    continue;
                }
            }

        }
        else if(m_PlayerGroup[i].Order() == mpo_AutoDefence)
        {
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                {
                    robot = (CMatrixRobotAI*)obj;
                    if(robot->GetEnv()->GetEnemyCnt())
                    {
                        //order_ok[i] = true;
                        m_PlayerGroup[i].SetWar(true);
                        if(!prev_war) PGPlaceClear(i);
                        break;
                    }
                    if(!PLIsToPlace(robot))
                    {
                        order_ok[i] = false;
                        break;
                    }
                }
                obj = obj->GetNextLogic();
            }
            if(m_PlayerGroup[i].IsWar()) continue;

            if(!m_PlayerGroup[i].m_Obj || !m_PlayerGroup[i].m_Obj->IsRobotAlive() || m_PlayerGroup[i].m_Region < 0)
            {
                m_PlayerGroup[i].m_Obj = nullptr;
                order_ok[i] = false;
                continue;
            }
            if(!m_PlayerGroup[i].IsWar() && order_ok[i]) // Если нет войны, и правильно идем по места, проверяем, идет ли цель в регион назначения.
            {
                if(!m_PlayerGroup[i].m_Obj->AsRobot()->GetMoveToCoords(tp)) tp = GetMapPos(m_PlayerGroup[i].m_Obj);
                int reg = g_MatrixMap->GetRegion(tp);

                if(m_PlayerGroup[i].m_Region != reg)
                {
                    if(CanChangePlace(robot))
                    {
                        m_PlayerGroup[i].m_Obj = nullptr;
                        order_ok[i] = false;
                        continue;
                    }
                }
            }
        }
    }

    //Применяем отданный игроком группе (в группе может быть только один робот) приказ
    for(i = 0; i < MAX_LOGIC_GROUP; ++i)
    {
        if(only_group >= 0 && i != only_group) continue;
        if(m_PlayerGroup[i].m_RobotCnt <= 0) continue;
        if(order_ok[i]) continue;

        int cur_ord = m_PlayerGroup[i].Order();
        if(cur_ord == mpo_Stop)
        {
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i && !obj->IsManualControlLocked())
                {
                    robot = (CMatrixRobotAI*)obj;
                    //Только если робот ещё не был полностью остановлен
                    if(PLPlacePos(robot).x >= 0)//robot->m_Speed > ZERO_VELOCITY)
                    {
                        //robot->RemoveOrderFromTop();

                        //Если из ручного управления данным роботом только что отключился игрок, то ему маркер не рисуем, но вместо этого снимаем маркер
                        if(robot->IsAfterManual())
                        {
                            //Этот маркер снимаем чуть позже, при выделении робота, из которого только что был совершён выход
                            //robot->SetAfterManual(false);
                            obj = obj->GetNextLogic();
                            continue;
                        }

                        //Хотел прописать им по приказу "Stop" плавное торможение, если идут на полном ходу, но хер знает как это нормально сделать, да и в ваниле не было
                        /*
                        if(robot->IsInManualControl())
                        {
                            //Это дерьмо на плавную остановку ваще не работает, даже если бы я мог как-то отсюда дать ей тики (ms)
                            robot->LowLevelDecelerate(ms, true, true);
                            if(robot->m_Speed <= ZERO_VELOCITY)
                            {
                                robot->LowLevelStop();
                                robot->RemoveOrderFromTop();
                            }
                        }
                        else
                        {
                            robot->LowLevelStop();
                            robot->RemoveOrderFromTop();
                        }
                        */

                        //Если роботы двигались в момент получения приказа "Stop", то под каждым из них будет разово отрисован вейпоинт
                        CPoint tp = CPoint(robot->GetMapPosX(), robot->GetMapPosY());
                        if(tp.x >= 0)
                        {
                            D3DXVECTOR3 v;
                            v.x = GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                            v.y = GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                            v.z = g_MatrixMap->GetZ(v.x, v.y);
                            CMatrixEffect::CreateMoveToAnim(v, 1);
                        }
                    }
                }
                obj = obj->GetNextLogic();
            }
            //Эта хрень им выставляла групповые маркеры остановки на тех позициях, в которые они начинали перемещаться при получении приказа "стоп"
            //дерьмовые были времена...
            //if(m_PlayerGroup[i].IsShowPlace()) PGShowPlace(i);

            //Этим мы полностью обнуляем группе роботов точку движения, то есть, фактически, они (или он один) после этого замирают на месте
            PGPlaceClear(i);
        }
        else if(cur_ord == mpo_MoveTo)
        {
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i && !obj->IsManualControlLocked())
                {
                    robot = (CMatrixRobotAI*)obj;

                    tp = PLPlacePos(robot);
                    if(tp.x >= 0 && PrepareBreakOrder(robot)) robot->MoveToHigh(tp.x, tp.y);
                }
                obj = obj->GetNextLogic();
            }
            if(m_PlayerGroup[i].IsShowPlace()) PGShowPlace(i);
        }
        else if(cur_ord == mpo_Patrol)
        {
            if(!m_PlayerGroup[i].IsPatrolReturn()) PGAssignPlace(i, m_PlayerGroup[i].m_From);
            else PGAssignPlace(i, m_PlayerGroup[i].m_To);

            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i && !obj->IsManualControlLocked())
                {
                    robot = (CMatrixRobotAI*)obj;

                    tp = PLPlacePos(robot);
                    if(tp.x >= 0 && PrepareBreakOrder(robot))
                    {
                        robot->MoveToHigh(tp.x, tp.y);
                    }
                }
                obj = obj->GetNextLogic();
            }
            if(m_PlayerGroup[i].IsShowPlace()) PGShowPlace(i);
        }
        //Игрок выставил данной группе приказ захвата здания
        else if(cur_ord == mpo_Capture)
        {
            //Если у группы выставлен приказ захвата здания, то проверяем, чтобы в ней всегда был живой робот,
            //который и должен будет производить захват
            //(предполагается, что робот-захватчик одного здания для каждой фракции на всей карте всегда может быть только один)
            //Отдача приказа на захват игроком всегда сбрасывает всех уже назначенных для захвата данного здания роботов.
            CMatrixRobotAI* capturer = nullptr;
            CMatrixBuilding* targ_building = m_PlayerGroup[i].m_Obj->AsBuilding();
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && !obj->IsManualControlLocked())
                {
                    robot = (CMatrixRobotAI*)obj;
                    building = robot->GetCaptureBuilding();
                    if(building && building == targ_building)
                    {
                        capturer = robot;
                        break;
                    }
                }
                obj = obj->GetNextLogic();
            }
            //Если главного робота-захватчика уничтожили, ищем ему замену поближе к целевому зданию
            if(capturer == nullptr)
            {
                t = 1000000000;
                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
                    {
                        robot = (CMatrixRobotAI*)obj;
                        int gr = robot->GetGroupLogic();
                        if(m_PlayerGroup[gr].Order() == mpo_Capture && m_PlayerGroup[gr].m_Obj == targ_building)
                        {
                            if(!robot->IsManualControlLocked() && robot->CanBreakOrder())// && robot->GetGroupLogic() == i)
                            {
                                u = GetMapPos(robot).Dist2(GetMapPos(targ_building));
                                if(u < t)
                                {
                                    t = u;
                                    if(0)//capturer != nullptr)
                                    {
                                        //Я, короче, рот ебал этого дабокода. Переключение логической группы видится единственным
                                        //возможным способом прервать приказ исполнения роботом захвата здания,
                                        //которое было переназначено для роботу из другой группы.
                                        //Особых подводных камней такого решения пока что не вижу.
                                        
                                        //Блядская проблема ещё в том, что если приказать ближайшему к зданию роботу захватить его,
                                        //а затем выбрать более дальнего и его тоже отправить на захват, то приказ на движение к базе
                                        //дальний робот с какого-то хера не получает. я хз, пусть весь этот код издохнет в корчах!
                                        capturer->StopCapture();
                                        //capturer->GetEnv()->m_Place = -1;
                                        //capturer->GetEnv()->m_PlaceNotFound = -10000;
                                        int num = capturer->GetGroupLogic();
                                        capturer->SetGroupLogic(i);
                                        --m_PlayerGroup[num].m_RobotCnt;
                                        ++m_PlayerGroup[i].m_RobotCnt;

                                        //PGOrderCapture(capturer->GetGroupLogic(), targ_building);
                                        /*
                                        PGAssignPlace(num, GetMapPos(targ_building));
                                        CMatrixMapStatic *obj2 = CMatrixMapStatic::GetFirstLogic();
                                        while(obj2)
                                        {
                                            if(obj2->IsRobotAlive() && obj2->GetSide() == m_Id)
                                            {
                                                robot2 = (CMatrixRobotAI*)obj2;
                                                int gr2 = robot->GetGroupLogic();
                                                if(gr2 == num)
                                                {
                                                    tp = PLPlacePos(robot2);
                                                    if(tp.x >= 0 && PrepareBreakOrder(robot2)) robot2->MoveToHigh(tp.x, tp.y);
                                                }
                                            }
                                            obj2 = obj2->GetNextLogic();
                                        }
                                        */
                                        
                                        //tp = PLPlacePos(capturer);
                                        //if(tp.x >= 0) capturer->MoveToHigh(tp.x, tp.y);
                                        //g_MatrixMap->StaticDelete(capturer);
                                    }
                                    capturer = robot;
                                }
                            }
                        }
                    }
                    obj = obj->GetNextLogic();
                }
                if(capturer)
                {
                    if(PrepareBreakOrder(capturer))
                    {
                        SoundCapture(i);
                        capturer->CaptureBuilding(targ_building);
                    }
                }
            }

            //Распределяем по местам вокруг здания всех прочих роботов группы, которые не получили приказ непосредственного захвата
            CPoint temp = GetMapPos(targ_building);
            PGAssignPlace(i, temp);
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
                {
                    robot = (CMatrixRobotAI*)obj;
                    if(robot->GetGroupLogic() == i && robot != capturer && !robot->IsManualControlLocked())
                    {
                        tp = PLPlacePos(robot);
                        if(tp.x >= 0 && PrepareBreakOrder(robot)) robot->MoveToHigh(tp.x, tp.y);
                    }
                }

                obj = obj->GetNextLogic();
            }
            //Рисуем для всех роботов группы вейпоинты
            //Если код выше переопредели для захвата здания другого робота и объединил логическую группу,
            //то данная функция отрисует один лишний вейпоинт для робота, который ехал на захват первым,
            //поскольку он уже будет считаться частью текущей группы,
            //но я не придумал как это пофиксить
            if(m_PlayerGroup[i].IsShowPlace()) PGShowPlace(i);
        }
        else if(cur_ord == mpo_Attack)
        {
            if(m_PlayerGroup[i].m_Obj) PGAssignPlacePlayer(i, GetMapPos(m_PlayerGroup[i].m_Obj));
            else PGAssignPlacePlayer(i, m_PlayerGroup[i].m_To);

            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i && !obj->IsManualControlLocked())
                {
                    robot = (CMatrixRobotAI*)obj;

                    tp = PLPlacePos(robot);
                    if(tp.x >= 0 && PrepareBreakOrder(robot))
                    {
                        robot->MoveToHigh(tp.x, tp.y);
                    }
                }
                obj = obj->GetNextLogic();
            }

            if(m_PlayerGroup[i].IsShowPlace()) PGShowPlace(i);
        }
        else if(cur_ord == mpo_Bomb)
        {
            if(m_PlayerGroup[i].m_Obj) PGAssignPlacePlayer(i, GetMapPos(m_PlayerGroup[i].m_Obj));
            else PGAssignPlacePlayer(i, m_PlayerGroup[i].m_To);

            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i && !obj->IsManualControlLocked())
                {
                    robot = (CMatrixRobotAI*)obj;

                    tp = PLPlacePos(robot);
                    if(tp.x >= 0 && PrepareBreakOrder(robot)) robot->MoveToHigh(tp.x, tp.y);
                }
                obj = obj->GetNextLogic();
            }

            if(m_PlayerGroup[i].IsShowPlace()) PGShowPlace(i);
        }
        else if(cur_ord == mpo_AutoCapture)
        {
            if(!m_PlayerGroup[i].m_Obj) PGFindCaptureBuilding(i);
            if(!m_PlayerGroup[i].m_Obj)
            {
                PGOrderStop(i);
                continue;
            }

            robot2 = nullptr;
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
                {
                    robot = (CMatrixRobotAI*)obj;
                    building = robot->GetCaptureBuilding();
                    if(building && building == m_PlayerGroup[i].m_Obj)
                    {
                        robot2 = robot;
                        break;
                    }
                }
                obj = obj->GetNextLogic();
            }
            if(robot2 == nullptr)
            {
                t = 1000000000;
                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    //if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i)
                    //{
                    if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i && !obj->IsManualControlLocked())
                    {

                        robot = (CMatrixRobotAI*)obj;

                        if(robot->CanBreakOrder())
                        {
                            u = GetMapPos(robot).Dist2(GetMapPos(m_PlayerGroup[i].m_Obj));
                            if(u < t)
                            {
                                t = u;
                                robot2 = robot;
                            }
                        }
                    }
                    obj = obj->GetNextLogic();
                }

                if(robot2)
                {
                    if(PrepareBreakOrder(robot2))
                    {
                        SoundCapture(i);
                        robot2->CaptureBuilding((CMatrixBuilding*)m_PlayerGroup[i].m_Obj);
                    }
                }
            }

            CPoint temp = GetMapPos(m_PlayerGroup[i].m_Obj);
            PGAssignPlace(i, temp);
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                //if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i && obj != robot2) {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i && obj != robot2 && !obj->IsManualControlLocked())
                {
                    robot = (CMatrixRobotAI*)obj;

                    tp = PLPlacePos(robot);
                    if(tp.x >= 0 && PrepareBreakOrder(robot)) robot->MoveToHigh(tp.x, tp.y);
                }

                obj = obj->GetNextLogic();
            }

        }
        else if(cur_ord == mpo_AutoAttack)
        {
            if(!m_PlayerGroup[i].m_Obj) PGFindAttackTarget(i);

            if(!m_PlayerGroup[i].m_Obj) continue;

            if(m_PlayerGroup[i].m_Obj) PGAssignPlacePlayer(i, GetMapPos(m_PlayerGroup[i].m_Obj));
            else PGAssignPlacePlayer(i, m_PlayerGroup[i].m_To);

            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                //if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i) {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i && !obj->IsManualControlLocked())
                {
                    robot = (CMatrixRobotAI*)obj;

                    tp = PLPlacePos(robot);
                    if(tp.x >= 0 && PrepareBreakOrder(robot)) robot->MoveToHigh(tp.x, tp.y);
                }
                obj = obj->GetNextLogic();
            }
        }
        else if(cur_ord == mpo_AutoDefence)
        {
            if(!m_PlayerGroup[i].m_Obj) PGFindDefenceTarget(i);

            if(!m_PlayerGroup[i].m_Obj || m_PlayerGroup[i].m_Region < 0) continue;

            PGAssignPlace(i, g_MatrixMap->m_RoadNetwork.GetRegion(m_PlayerGroup[i].m_Region)->m_Center);

            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                //if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i) {
                if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == i && !obj->IsManualControlLocked())
                {

                    robot = (CMatrixRobotAI*)obj;

                    tp = PLPlacePos(robot);
                    if(tp.x >= 0 && PrepareBreakOrder(robot)) robot->MoveToHigh(tp.x, tp.y);
                }
                obj = obj->GetNextLogic();
            }
        }
    }
}

void CMatrixSideUnit::RepairPL(int group)
{
    CPoint tp;
    CMatrixMapStatic* obj;
    CMatrixRobotAI* player_robots[MAX_ROBOTS_ON_MAP]; // Список роботов на карте
    bool player_robots_ok[MAX_ROBOTS_ON_MAP];
    int player_robots_cnt = 0;                 // Кол-во роботов на карте
    byte mm = 0;
    SMatrixPlace* place;
    D3DXVECTOR2 v, v2;

    BufPrepare();

    //Готовим список роботов-ремонтников игрока
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id && GetGroupLogic(obj) == group && obj->AsRobot()->HaveRepair())
        {
            mm |= 1 << (obj->AsRobot()->m_Module[0].m_Kind - 1);
            player_robots[player_robots_cnt] = obj->AsRobot();
            player_robots_ok[player_robots_cnt] = true;
            ++player_robots_cnt;
        }
        obj = obj->GetNextLogic();
    }
    if(player_robots_cnt <= 0) return;

    if(m_PlayerGroup[group].Order() == mpo_Repair && m_PlayerGroup[group].m_Obj && m_PlayerGroup[group].m_Obj->IsAlive())
    {
        for(int i = 0; i < player_robots_cnt; ++i)
        {
            if(m_PlayerGroup[group].m_Obj == player_robots[i]) player_robots[i]->GetEnv()->m_Target = nullptr;
            else player_robots[i]->GetEnv()->m_Target = m_PlayerGroup[group].m_Obj;
        }

        //Проверяем достает ли чинилка
        bool ok = true;
        for(int i = 0; i < player_robots_cnt; ++i)
        {
            CMatrixRobotAI* robot_healer = player_robots[i];

            tp = PLPlacePos(robot_healer);
            D3DXVECTOR2 v;
            v.x = GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2.0f;
            v.y = GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2.0f;

            v2 = GetWorldPos(m_PlayerGroup[group].m_Obj);

            if((POW2(v2.x - v.x) + POW2(v2.y - v.y)) > POW2(robot_healer->GetRepairDist()))
            {
                if((g_MatrixMap->GetTime() - robot_healer->GetEnv()->m_PlaceNotFound) > 2000)
                {
                    player_robots_ok[i] = false;
                    ok = false;
                }
            }
        }

        //Назначаем новые места, если кто-нибудь не достает
        if(!ok)
        {
            int list_cnt = 0;
            if(g_MatrixMap->PlaceList(mm, GetMapPos(player_robots[0]), GetMapPos(m_PlayerGroup[group].m_Obj), Float2Int(player_robots[0]->GetRepairDist() * INVERT(GLOBAL_SCALE_MOVE)), false, m_PlaceList, &list_cnt) == 0)
            {
                for(int i = 0; i < player_robots_cnt; ++i) player_robots[i]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();
                return;
            }

            // Помечаем занятые места
            for(int t = 0; t < list_cnt; ++t)
            {
                g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[t]].m_Data = 0;
            }

            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsUnitAlive()) ObjPlaceData(obj, 1);
                obj = obj->GetNextLogic();
            }

            //Находим места
            for(int i = 0; i < player_robots_cnt; ++i)
            {
                if(player_robots_ok[i]) continue;

                place = nullptr;
                int u;
                for(u = 0; u < list_cnt; ++u)
                {
                    place = g_MatrixMap->m_RoadNetwork.m_Place + m_PlaceList[u];
                    if(place->m_Data) continue;

                    v.x = GLOBAL_SCALE_MOVE * place->m_Pos.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2.0f;
                    v.y = GLOBAL_SCALE_MOVE * place->m_Pos.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2.0f;

                    v2 = GetWorldPos(m_PlayerGroup[group].m_Obj);
                    if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) < POW2(player_robots[i]->GetRepairDist())) break;
                }

                if(u >= list_cnt) //Если роботы не влазят
                {
                    for(u = i; u < player_robots_cnt; ++u)
                    {
                        player_robots[u]->GetEnv()->m_Place = -1;
                        player_robots[u]->GetEnv()->m_PlaceAdd = CPoint(-1, -1);
                    }

                    u = 0;
                    for(; i < player_robots_cnt; ++i)
                    {
                        player_robots[i]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();

                        for(; u < list_cnt; ++u)
                        {
                            place = g_MatrixMap->m_RoadNetwork.m_Place + m_PlaceList[u];
                            if(place->m_Data) continue;
                            break;
                        }

                        if(u >= list_cnt)
                        {
                            // Выращиваем список
                            if(g_MatrixMap->PlaceListGrow(mm, m_PlaceList, &list_cnt, player_robots_cnt) <= 0) continue;
                            // Помечаем занятые места
                            for(int t = 0; t < list_cnt; ++t)
                            {
                                g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[t]].m_Data = 0;
                            }
                            obj = CMatrixMapStatic::GetFirstLogic();

                            while(obj)
                            {
                                if(obj->IsUnitAlive()) ObjPlaceData(obj, 1);
                                obj = obj->GetNextLogic();
                            }
                            --i;

                            continue;
                        }
                        else
                        {
                            place->m_Data = 1;
                            player_robots[i]->GetEnv()->m_Place = m_PlaceList[u];
                            player_robots[i]->GetEnv()->m_PlaceAdd = CPoint(-1, -1);

                            if(PrepareBreakOrder(player_robots[i]))
                            {
                                player_robots[i]->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                            }
                        }
                    }

                    break;
                }

                place->m_Data = 1;
                player_robots[i]->GetEnv()->m_Place = m_PlaceList[u];
                player_robots[i]->GetEnv()->m_PlaceAdd = CPoint(-1, -1);

                if(PrepareBreakOrder(player_robots[i]))
                {
                    player_robots[i]->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                }
            }
        }
    }
    else
    {
        //Если нет цели для починки, то ищем её
        for(int i = 0; i < player_robots_cnt; ++i)
        {
            CMatrixRobotAI* robot_healer = player_robots[i];
            CInfo* env = robot_healer->GetEnv();

            if(env->m_Target && env->m_Target->IsAlive() && env->m_Target->GetSide() == m_Id && env->m_Target->NeedRepair())
            {
                v = GetWorldPos(robot_healer);
                v2 = GetWorldPos(env->m_Target);
                if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) < POW2(robot_healer->GetRepairDist())) continue;
            }

            env->m_Target = nullptr;

            CMatrixMapStatic* primary_target = nullptr;
            CMatrixMapStatic* secondary_target = nullptr;
            CMatrixMapStatic* third_rate_target = nullptr;

            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj != robot_healer && obj->IsAlive() && obj->GetSide() == m_Id && obj->NeedRepair())
                {
                    v = GetWorldPos(player_robots[i]);
                    v2 = GetWorldPos(obj);
                    //Если цель в пределах досягаемости ремонтника
                    if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) < POW2(player_robots[i]->GetRepairDist()))
                    {
                        EObjectType type = obj->GetObjectType();
                        if(type == OBJECT_TYPE_ROBOT_AI || type == OBJECT_TYPE_FLYER)
                        {
                            if(!primary_target || obj->GetHitpointsPercent() < primary_target->GetHitpointsPercent()) primary_target = obj;
                        }
                        else if(type == OBJECT_TYPE_TURRET)
                        {
                            if(!secondary_target || obj->GetHitpointsPercent() < secondary_target->GetHitpointsPercent()) secondary_target = obj;
                        }
                        else if(type == OBJECT_TYPE_BUILDING)
                        {
                            if(!third_rate_target || obj->GetHitpointsPercent() < third_rate_target->GetHitpointsPercent()) third_rate_target = obj;
                        }
                    }
                }

                obj = obj->GetNextLogic();
            }

            if(primary_target) robot_healer->GetEnv()->m_Target = primary_target;
            else if(secondary_target) robot_healer->GetEnv()->m_Target = secondary_target;
            else if(third_rate_target) robot_healer->GetEnv()->m_Target = third_rate_target;
        }
    }

    //Корректируем точку выстрела и отдаём приказы на отхил
    for(int i = 0; i < player_robots_cnt; ++i)
    {
        if(!player_robots[i]->GetEnv()->m_Target) continue;

        D3DXVECTOR3 des = SetPointOfAim(player_robots[i]->GetEnv()->m_Target);
        player_robots[i]->OrderFire(des, TARGET_IS_FRIEND);
    }
}

bool CMatrixSideUnit::FirePL(int group)
{
    bool rv = false;
    int i;
    CMatrixMapStatic* obj;
    CMatrixRobotAI* rl[MAX_ROBOTS_ON_MAP]; // Список роботов на карте
    int ally_robots_cnt = 0;                     // Кол-во роботов на карте

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id && GetGroupLogic(obj) == group)
        {
            rl[ally_robots_cnt] = (CMatrixRobotAI*)obj;
            ++ally_robots_cnt;
        }
        obj = obj->GetNextLogic();
    }
    if(ally_robots_cnt <= 0) return false;

    // Находим врага для всей группы
    for(i = 0; i < ally_robots_cnt; ++i)
    {
        CInfo* env = rl[i]->GetEnv();
        if(env->m_TargetAttack && env->SearchEnemy(env->m_TargetAttack))
        {
            float cd = Dist2(GetWorldPos(env->m_TargetAttack), GetWorldPos(rl[i]));
            if(cd > POW2(rl[i]->GetMaxFireDist())) env->m_TargetAttack = nullptr;
        }
        if(!(env->m_TargetAttack && env->SearchEnemy(env->m_TargetAttack)))
        {
            float min_dist = 1e10f;
            CEnemy* enemy_find = nullptr;
            CEnemy* enemy = nullptr;
            // Находим ближайшего незакрытого врага
            enemy = env->m_FirstEnemy;
            while(enemy)
            {
                if(enemy->m_Enemy->IsUnitAlive() && enemy->m_Enemy != rl[i])
                {
                    float cd = Dist2(GetWorldPos(enemy->m_Enemy), GetWorldPos(rl[i]));
                    if (cd < min_dist)
                    {
                        // Проверяем не закрыт ли он своими
                        D3DXVECTOR3 des, from, dir, p;
                        float t, dist;

                        from = rl[i]->GetGeoCenter();
                        des = SetPointOfAim(enemy->m_Enemy);
                        dist = sqrt(POW2(from.x - des.x) + POW2(from.y - des.y) + POW2(from.z - des.z));
                        if(dist > 0.0f)
                        {
                            t = 1.0f / dist;
                            dir.x = (des.x - from.x) * t;
                            dir.y = (des.y - from.y) * t;
                            dir.z = (des.z - from.z) * t;
                            obj = CMatrixMapStatic::GetFirstLogic();
                            while(obj)
                            {
                                if(obj->IsUnitAlive() && obj->GetSide() == m_Id && rl[i] != obj && env->m_TargetAttack != obj)
                                {
                                    p = SetPointOfAim(obj);

                                    if(IsIntersectSphere(p, 25.0f, from, dir, t))
                                    {
                                        if(t >= 0.0f && t < dist) break;
                                    }
                                }
                                obj = obj->GetNextLogic();
                            }
                            if(!obj)
                            {
                                min_dist = cd;
                                enemy_find = enemy;
                            }
                        }
                    }
                }
                enemy = enemy->m_NextEnemy;
            }
            // Если не нашли открытого ищем закрытого
            if(!enemy_find)
            {
                enemy = env->m_FirstEnemy;
                while(enemy)
                {
                    if(enemy->m_Enemy->IsUnitAlive() && enemy->m_Enemy != rl[i])
                    {
                        float cd = Dist2(GetWorldPos(enemy->m_Enemy), GetWorldPos(rl[i]));
                        if(cd < min_dist)
                        {
                            min_dist = cd;
                            enemy_find = enemy;
                        }
                    }
                    enemy = enemy->m_NextEnemy;
                }
            }

            if(enemy_find) env->m_TargetAttack = enemy_find->m_Enemy;
        }
    }

    //Корректируем точку выстрела
    D3DXVECTOR3 des, from, dir, p;
    float t, dist;

    for(i = 0; i < ally_robots_cnt; ++i)
    {
        CInfo* env = rl[i]->GetEnv();
        if(env->m_TargetAttack && env->SearchEnemy(env->m_TargetAttack))
        {
            if(!env->m_TargetAttack->IsAlive())
            {
                rl[i]->StopFire();
                continue;
            }

            des = SetPointOfAim(env->m_TargetAttack);

            // Не стрелять из прямого оружия, если на пути к цели свои
            from = rl[i]->GetGeoCenter();
            dist = sqrt(POW2(from.x - des.x) + POW2(from.y - des.y) + POW2(from.z - des.z));

            bool fire_line = rl[i]->HaveRepair() != 2;

            if(fire_line && dist > 0.0f)
            {
                t = 1.0f / dist;
                dir.x = (des.x - from.x) * t;
                dir.y = (des.y - from.y) * t;
                dir.z = (des.z - from.z) * t;

                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsUnitAlive() && obj->GetSide() == m_Id && rl[i] != obj && env->m_TargetAttack != obj)
                    {
                        p = SetPointOfAim(obj);

                        if(IsIntersectSphere(p, 25.0f, from, dir, t))
                        {
                            if(t >= 0.0f && t < dist)
                            {
                                //CHelper::DestroyByGroup(dword(this) + 4);
                                //CHelper::Create(10, dword(this) + 4)->Cone(from, des, 1.0f, 1.0f, 0xffffffff, 0xffffffff, 3);
                                //CHelper::Create(10, dword(this) + 4)->Sphere(D3DXVECTOR3(from.x + dir.x * t, from.y + dir.y * t, from.z + dir.z * t), 2, 5, 0xffff0000);

                                fire_line = false;
                                break;
                            }
                        }
                    }
                    obj = obj->GetNextLogic();
                }
            }

            //des.x += g_MatrixMap->RndFloat(-5.0f, 5.0f);
            //des.y += g_MatrixMap->RndFloat(-5.0f, 5.0f);
            //des.z += g_MatrixMap->RndFloat(-5.0f, 5.0f);

            if(env->m_TargetAttack != env->m_TargetLast)
            {
                env->m_TargetLast = env->m_TargetAttack;
            }

            if(fire_line)
            {
                D3DXVECTOR3 v1 = rl[i]->GetGeoCenter();
                D3DXVECTOR3 v2 = SetPointOfAim(env->m_TargetAttack);

                D3DXVECTOR3 temp = v2 - v1;
                fire_line = D3DXVec3LengthSq(&temp) <= POW2(rl[i]->GetMaxFireDist());

                if(fire_line)
                {
                    CMatrixMapStatic* trace_res = g_MatrixMap->Trace(nullptr, v1, v2, TRACE_OBJECT | TRACE_NONOBJECT, rl[i]);
                    fire_line = !((IS_TRACE_STOP_OBJECT(trace_res) && trace_res->GetObjectType() == OBJECT_TYPE_MAPOBJECT) || (trace_res == TRACE_STOP_WATER) || (trace_res == TRACE_STOP_LANDSCAPE));
                }
            }

            if(fire_line)
            {
                rv = true;

                //Если у цели голова Firewall то в нее сложнее попасть
                /*
                if(env->m_TargetAttack->IsRobot() && env->m_TargetAttack->AsRobot()->m_AimProtect > 0)
                {
                    if(env->m_Target!=env->m_TargetAttack || fabs(env->m_TargetAngle) <= 1.0f * 1.1f * ToRad)
                    {
                        env->m_TargetAngle=0.0f;

                        env->m_TargetAngle = min(30.0f * ToRad, atan(env->m_TargetAttack->AsRobot()->m_AimProtect / sqrt(POW2(des.x - rl[i]->m_PosX) + POW2(des.y - rl[i]->m_PosY))));
                        if(g_MatrixMap->Rnd(0, 9) < 5) env->m_TargetAngle = -env->m_TargetAngle;
                    }
                    else if(env->m_TargetAngle > 0) env->m_TargetAngle -= 1.0f * ToRad;
                    else env->m_TargetAngle += 1.0f * ToRad;
                    */

                if(env->m_TargetAttack->IsRobot() && env->m_TargetAttack->AsRobot()->m_AimProtect > 0)
                {
                    if(env->m_Target != env->m_TargetAttack || fabs(env->m_TargetAngle) <= 1.3f * ToRad)// >= 15.0f * env->m_TargetAttack->AsRobot()->m_AimProtect * ToRad) {
                    {
                        env->m_TargetAngle = 0.0f;

                        env->m_TargetAngle = min(8.0f * ToRad, (float)g_MatrixMap->Rnd(1, 100) / 100.0f * 16.0f * env->m_TargetAttack->AsRobot()->m_AimProtect * ToRad);
                        if(g_MatrixMap->Rnd(0, 9) < 5) env->m_TargetAngle = -env->m_TargetAngle;
                    }
                    //else if(env->m_TargetAngle > 0) env->m_TargetAngle += 1.0f * ToRad;
                    //else if(env->m_TargetAngle < 0) env->m_TargetAngle -= 1.0f * ToRad;
                    //else if(fabs(env->m_TargetAngle) > 1.0f) env->m_TargetAngle *= 0.7f;
                    //else env->m_TargetAngle = (2 * g_MatrixMap->Rnd(0, 1) - 1) * ToRad;
                    else env->m_TargetAngle *= 0.75f;

                    if(env->m_TargetAngle != 0.0f)
                    {
                        float vx = des.x - rl[i]->m_PosX;
                        float vy = des.y - rl[i]->m_PosY;
                        float sa, ca;
                        SinCos(env->m_TargetAngle, &sa, &ca);
                        des.x = (ca * vx + sa * vy) + rl[i]->m_PosX;
                        des.y = (-sa * vx + ca * vy) + rl[i]->m_PosY;
                    }
                }

                env->m_Target = env->m_TargetAttack;
                rl[i]->OrderFire(des);
            }
            else
            {
                if(rl[i]->HaveRepair() && (g_MatrixMap->GetTime() - env->m_TargetChangeRepair) > 1000) //Ищем робота для починки
                {
                    D3DXVECTOR2 v, v2;

                    if(env->m_Target && env->m_Target->IsUnitAlive() && env->m_Target->GetSide() == m_Id && env->m_Target->NeedRepair())
                    {
                        v = GetWorldPos(rl[i]);
                        v2 = GetWorldPos(env->m_Target);
                        if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) > POW2(rl[i]->GetRepairDist())) env->m_Target = nullptr;
                    }
                    else env->m_Target = nullptr;

                    if(!env->m_Target)
                    {
                        obj = CMatrixMapStatic::GetFirstLogic();
                        while(obj)
                        {
                            if(obj != rl[i] && obj->IsUnitAlive() && obj->GetSide() == m_Id && obj->NeedRepair())
                            {
                                v = GetWorldPos(rl[i]);
                                v2 = GetWorldPos(obj);
                                if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) < POW2(rl[i]->GetRepairDist()))
                                {
                                    env->m_Target = obj;
                                    env->m_TargetChangeRepair = g_MatrixMap->GetTime();
                                    break;
                                }
                            }

                            obj = obj->GetNextLogic();
                        }
                    }
                }

                //Тип 2 означает, что выбранная для ведения огня цель дружественна стреляющему (этот же тип передаём в приказ на выстрел)
                if(env->TargetType() == TARGET_IS_FRIEND) rl[i]->OrderFire(SetPointOfAim(env->m_Target), TARGET_IS_FRIEND);
                else rl[i]->StopFire();

                //env->m_Target = nullptr;
                //rl[i]->StopFire();
            }
        }
    }

    return rv;
}

void CMatrixSideUnit::WarPL(int group)
{
    int i, u;//,x, y;
    byte mm = 0;
    CMatrixMapStatic* obj;
    CMatrixRobotAI* ally_robots[MAX_ROBOTS_ON_MAP]; // Список роботов на карте
    int ally_robots_cnt = 0;                  // Кол-во роботов на карте
    bool ally_robots_ok_move[MAX_ROBOTS_ON_MAP];

    BufPrepare();

    obj = CMatrixMapStatic::GetFirstLogic();
    while (obj)
    {
        if (obj->IsRobotAlive() && obj->GetSide() == m_Id && GetGroupLogic(obj) == group)
        {
            ally_robots[ally_robots_cnt] = (CMatrixRobotAI*)obj;
            mm |= 1 << (obj->AsRobot()->m_Module[0].m_Kind - 1);
            ++ally_robots_cnt;
        }
        obj = obj->GetNextLogic();
    }
    if(ally_robots_cnt < 0) return;

    // Находим врага для всей группы
    for(i = 0; i < ally_robots_cnt; ++i)
    {
        if(m_PlayerGroup[group].Order() == mpo_Attack && m_PlayerGroup[group].m_Obj && m_PlayerGroup[group].m_Obj != ally_robots[i] && m_PlayerGroup[group].m_Obj->IsAlive() && m_PlayerGroup[group].m_Obj != ally_robots[i]->GetEnv()->m_TargetAttack)
        {
            if (ally_robots[i]->GetEnv()->SearchEnemy(m_PlayerGroup[group].m_Obj))
            {
                ally_robots[i]->GetEnv()->m_TargetAttack = m_PlayerGroup[group].m_Obj;
            }
        }
        if(m_PlayerGroup[group].Order() == mpo_Capture && m_PlayerGroup[group].m_Obj && m_PlayerGroup[group].m_Obj->IsAlive() &&
            ally_robots[i]->GetEnv()->m_TargetAttack && ally_robots[i]->GetEnv()->m_TargetAttack->IsAlive() && (ally_robots[i]->GetEnv()->m_TargetAttack->GetObjectType() != OBJECT_TYPE_TURRET || ((CMatrixTurret*)(ally_robots[i]->GetEnv()->m_TargetAttack))->m_ParentBuilding != m_PlayerGroup[group].m_Obj))
        {
            float min_dist = 1e10f;
            CEnemy* enemy_find = nullptr;
            CEnemy* enemy = ally_robots[i]->GetEnv()->m_FirstEnemy;
            while(enemy)
            {
                if(enemy->m_Enemy->IsActiveTurretAlive() && enemy->m_Enemy->AsTurret()->m_ParentBuilding == m_PlayerGroup[group].m_Obj)
                {
                    while(true)
                    {
                        float cd = Dist2(GetWorldPos(enemy->m_Enemy), GetWorldPos(ally_robots[i]));
                        if (enemy_find)
                        {
                            if (min_dist < cd) break;
                        }

                        min_dist = cd;
                        enemy_find = enemy;
                        break;
                    }
                }
                enemy = enemy->m_NextEnemy;
            }
            if(enemy_find)
            {
                ally_robots[i]->GetEnv()->m_TargetAttack = enemy_find->m_Enemy;
                ally_robots[i]->GetEnv()->m_Place = -1;
            }
        }

        if(!ally_robots[i]->GetEnv()->m_TargetAttack)
        {
            float min_dist = 1e10f;
            CEnemy* enemy_find = nullptr;
            CEnemy* enemy = nullptr;
            // Находим цель которую указал игрок
            if(m_PlayerGroup[group].m_Obj && m_PlayerGroup[group].m_Obj != ally_robots[i])
            {
                enemy_find = ally_robots[i]->GetEnv()->SearchEnemy(m_PlayerGroup[group].m_Obj);
            }
            // Находим ближайшего незакрытого врага
            if(!enemy_find)
            {
                enemy = ally_robots[i]->GetEnv()->m_FirstEnemy;
                while(enemy)
                {
                    if(enemy->m_Enemy->IsUnitAlive() && enemy->m_Enemy != ally_robots[i])
                    {
                        float cd = Dist2(GetWorldPos(enemy->m_Enemy), GetWorldPos(ally_robots[i]));
                        if(cd < min_dist)
                        {
                            //Проверяем не закрыт ли он своими
                            D3DXVECTOR3 des, from, dir, p;
                            float t, dist;

                            from = ally_robots[i]->GetGeoCenter();
                            des = SetPointOfAim(enemy->m_Enemy);
                            dist = sqrt(POW2(from.x - des.x) + POW2(from.y - des.y) + POW2(from.z - des.z));
                            if(dist > 0.0f)
                            {
                                t = 1.0f / dist;
                                dir.x = (des.x - from.x) * t;
                                dir.y = (des.y - from.y) * t;
                                dir.z = (des.z - from.z) * t;
                                obj = CMatrixMapStatic::GetFirstLogic();
                                while(obj)
                                {
                                    if(obj->IsUnitAlive() && obj->GetSide() == m_Id && ally_robots[i] != obj && ally_robots[i]->GetEnv()->m_TargetAttack != obj)
                                    {
                                        p = SetPointOfAim(obj);

                                        if(IsIntersectSphere(p, 25.0f, from, dir, t))
                                        {
                                            if(t >= 0.0f && t < dist) break;
                                        }
                                    }
                                    obj = obj->GetNextLogic();
                                }
                                if(!obj)
                                {
                                    min_dist = cd;
                                    enemy_find = enemy;
                                }
                            }
                        }
                    }
                    enemy = enemy->m_NextEnemy;
                }
            }
            //Если не нашли открытого ищем закрытого
            if(!enemy_find)
            {
                enemy = ally_robots[i]->GetEnv()->m_FirstEnemy;
                while(enemy)
                {
                    if(enemy->m_Enemy->IsUnitAlive() && enemy->m_Enemy != ally_robots[i])
                    {
                        float cd = Dist2(GetWorldPos(enemy->m_Enemy), GetWorldPos(ally_robots[i]));
                        if(cd < min_dist)
                        {
                            min_dist = cd;
                            enemy_find = enemy;
                        }
                    }
                    enemy = enemy->m_NextEnemy;
                }
            }

            if(enemy_find)
            {
                ally_robots[i]->GetEnv()->m_TargetAttack = enemy_find->m_Enemy;
                // Если новая цель пушка или завод, то меняем позицию
                if(ally_robots[i]->GetEnv()->m_TargetAttack->IsActiveTurretAlive() || ally_robots[i]->GetEnv()->m_TargetAttack->IsBuildingAlive())
                {
                    ally_robots[i]->GetEnv()->m_Place = -1;
                }
            }
        }
    }

    // Проверяем правильно ли роботы идут
    bool moveok = true;
    for(i = 0; i < ally_robots_cnt; ++i)
    {
        ally_robots_ok_move[i] = true;

        if(!ally_robots[i]->CanBreakOrder()) continue; // Пропускаем если робот не может прервать текущий приказ
        if(!ally_robots[i]->GetEnv()->m_TargetAttack) // Если у робота нет цели то ждем завершение состояния войны
        {
/*            if(GetDesRegion(ally_robots[i])!=m_LogicGroup[group].m_Action.m_Region) {
                if(!PlaceInRegion(ally_robots[i],ally_robots[i]->GetEnv()->m_Place,m_LogicGroup[group].m_Action.m_Region)) {
                    if(CanChangePlace(ally_robots[i])) {
                    AssignPlace(ally_robots[i],m_LogicGroup[group].m_Action.m_Region);

                    SMatrixPlace * place=ObjPlacePtr(ally_robots[i]);
                    if(place && PrepareBreakOrder(ally_robots[i])) {
                        ally_robots[i]->MoveToHigh(place->m_Pos.x,place->m_Pos.y);
                    }
                }
                }
            }*/
            continue;
        }
        if (ally_robots[i]->GetEnv()->m_Place < 0)
        {
            if (CanChangePlace(ally_robots[i])) {
                ally_robots_ok_move[i] = false;
                moveok = false;
                continue;
            }
        }
        D3DXVECTOR2 tv = GetWorldPos(ally_robots[i]->GetEnv()->m_TargetAttack);

        SMatrixPlace* place = GetPlacePtr(ally_robots[i]->GetEnv()->m_Place);
        if (place == nullptr) {
            if (CanChangePlace(ally_robots[i]))
            {
                ally_robots[i]->GetEnv()->m_Place = -1;
                ally_robots_ok_move[i] = false;
                moveok = false;
                continue;
            }
        }
        else {
            float dist2 = POW2(GLOBAL_SCALE_MOVE * place->m_Pos.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2 - tv.x) + POW2(GLOBAL_SCALE_MOVE * place->m_Pos.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2 - tv.y);
            if (dist2 > POW2(ally_robots[i]->GetMaxFireDist() - GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2)) {
                if (CanChangePlace(ally_robots[i])) {
                    ally_robots[i]->GetEnv()->m_Place = -1;
                    ally_robots_ok_move[i] = false;
                    moveok = false;
                    continue;
                }
            }
        }
        /*        if(!IsToPlace(ally_robots[i],ally_robots[i]->GetEnv()->m_Place)) {
        //            IsToPlace(ally_robots[i],ally_robots[i]->GetEnv()->m_Place);
                    ally_robots_ok_move[i]=false;
                    moveok=false;
                    continue;
                }*/
    }

    // Назначаем движение
    if (!moveok) {
        // Находим место
        // Находим центр и радиус
        CPoint center, tp2, tp = CPoint(0, 0);
        int f = 0;
        for(i = 0; i < ally_robots_cnt; ++i)
        {
            if(!ally_robots[i]->GetEnv()->m_TargetAttack) continue;
            tp += GetMapPos(ally_robots[i]->GetEnv()->m_TargetAttack);
            ++f;
        }

        if(f <= 0) return;
        tp.x = tp.x / f; tp.y = tp.y / f;
        f = 1000000000;
        for(i = 0; i < ally_robots_cnt; ++i)
        {
            if(!ally_robots[i]->GetEnv()->m_TargetAttack) continue;
            tp2 = GetMapPos(ally_robots[i]->GetEnv()->m_TargetAttack);
            int f2 = POW2(tp.x - tp2.x) + POW2(tp.y - tp2.y);
            if(f2 < f)
            {
                f = f2;
                center = tp2;
            }
        }

        int radius = 0;
        int radiusrobot = 0;
        for(i = 0; i < ally_robots_cnt; ++i)
        {
            if(!ally_robots[i]->GetEnv()->m_TargetAttack) continue;
            tp2 = GetMapPos(ally_robots[i]->GetEnv()->m_TargetAttack);
            radiusrobot = max(radiusrobot, Float2Int(ally_robots[i]->GetMaxFireDist() / GLOBAL_SCALE_MOVE));
            radius = max(radius, Float2Int(sqrt(float(POW2(center.x - tp2.x) + POW2(center.y - tp2.y))) + ally_robots[i]->GetMaxFireDist() / GLOBAL_SCALE_MOVE + ROBOT_MOVECELLS_PER_SIZE));
        }

        //DM(L"RadiusSeek",CWStr().Format(L"<i>   <i>",radius,radiusrobot).Get());

        bool cplr = true;

        // Находим место
        int listcnt;
        if(g_MatrixMap->PlaceList(mm, GetMapPos(ally_robots[0]), center, radius, false, m_PlaceList, &listcnt) == 0)
        {
            for(i = 0; i < ally_robots_cnt; ++i) ally_robots[i]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();
        }
        else
        {
            /*CHelper::DestroyByGroup(524234);
            D3DXVECTOR3 v1;
            v1.x=center.x*GLOBAL_SCALE_MOVE;
            v1.y=center.y*GLOBAL_SCALE_MOVE;
            v1.z=g_MatrixMap->GetZ(v1.x,v1.y);
            CHelper::Create(3000,524234)->Cone(v1,D3DXVECTOR3(v1.x,v1.y,v1.z+50.0f),3.0f,3.0f,0xffffffff,0xffffffff,3);

            for(i=0;i<listcnt;i++) {
            D3DXVECTOR3 v1;
            v1.x=g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[i]].m_Pos.x*GLOBAL_SCALE_MOVE;
            v1.y=g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[i]].m_Pos.y*GLOBAL_SCALE_MOVE;
            v1.z=g_MatrixMap->GetZ(v1.x,v1.y);
            CHelper::Create(3000,524234)->Cone(v1,D3DXVECTOR3(v1.x,v1.y,v1.z+30.0f),1.0f,1.0f,0xffffffff,0xffffffff,3);
            }*/

            //CPoint tp;
            //CRect rect(1000000000,1000000000,-1000000000,-1000000000);
            //int growsizemin=0;
            //int growsizemax=0;

            //for(i=0;i<ally_robots_cnt;i++) {
            //    growsizemin=max(growsizemin,int(ally_robots[i]->GetMinFireDist()/GLOBAL_SCALE_MOVE));
            //    growsizemax=max(growsizemax,int(ally_robots[i]->GetMaxFireDist()/GLOBAL_SCALE_MOVE));

            //    CEnemy * enemy=ally_robots[i]->GetEnv()->m_FirstEnemy;
            //    while(enemy) {
            //        if(IsLive(enemy->m_Enemy)) {
            //            tp=GetMapPos(enemy->m_Enemy);
            //            rect.left=min(rect.left,tp.x);
            //            rect.top=min(rect.top,tp.y);
            //            rect.right=max(rect.right,tp.x+ROBOT_MOVECELLS_PER_SIZE);
            //            rect.bottom=max(rect.bottom,tp.y+ROBOT_MOVECELLS_PER_SIZE);
            //        }
            //        enemy=enemy->m_NextEnemy;
            //    }
            //}
            //if(!rect.IsEmpty()) {
                // Помечаем уже занетые места
    //            g_MatrixMap->m_RoadNetwork.ActionDataPL(CRect(rect.left-growsizemax,rect.top-growsizemax,rect.right+growsizemax,rect.bottom+growsizemax),0);
                /*
                for(i = 0; i < ally_robots_cnt; ++i)
                {
                    if(!ally_robots_ok_move[i]) continue;
                    if(ally_robots[i]->GetEnv()->m_Place<0) continue;
                    GetPlacePtr(ally_robots[i]->GetEnv()->m_Place)->m_Data=1;
                }
                */
            for(i = 0; i < listcnt; ++i) g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[i]].m_Data = 0;
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsUnitAlive()) ObjPlaceData(obj, 1);
                obj = obj->GetNextLogic();
            }

            //Находим лучшее место для каждого робота
            //CRect plr=g_MatrixMap->m_RoadNetwork.CorrectRectPL(CRect(rect.left-growsizemax,rect.top-growsizemax,rect.right+growsizemax,rect.bottom+growsizemax));
            for(i = 0; i < ally_robots_cnt; ++i)
            {
                if(ally_robots_ok_move[i]) continue;
                if(!ally_robots[i]->GetEnv()->m_TargetAttack) continue; // Если нет цели, то пропускаем

                bool havebomb = ally_robots[i]->HaveBomb();

                int placebest = -1;
                float s_f1 = 0.0f;
                int s_underfire = 0;
                bool s_close = false;

                float tvx, tvy; // To target
                int enemy_fire_dist;

                if(ally_robots[i]->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
                {
                    tvx = ((CMatrixRobotAI*)(ally_robots[i]->GetEnv()->m_TargetAttack))->m_PosX - ally_robots[i]->m_PosX;
                    tvy = ((CMatrixRobotAI*)(ally_robots[i]->GetEnv()->m_TargetAttack))->m_PosY - ally_robots[i]->m_PosY;
                    enemy_fire_dist = Float2Int(((CMatrixRobotAI*)(ally_robots[i]->GetEnv()->m_TargetAttack))->GetMaxFireDist());
                }
                else if(ally_robots[i]->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_TURRET)
                {
                    tvx = ((CMatrixTurret*)(ally_robots[i]->GetEnv()->m_TargetAttack))->m_Pos.x - ally_robots[i]->m_PosX;
                    tvy = ((CMatrixTurret*)(ally_robots[i]->GetEnv()->m_TargetAttack))->m_Pos.y - ally_robots[i]->m_PosY;
                    enemy_fire_dist = int(((CMatrixTurret*)(ally_robots[i]->GetEnv()->m_TargetAttack))->GetFireRadius() + GLOBAL_SCALE_MOVE);
                }
                else if(ally_robots[i]->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_BUILDING)
                {
                    tvx = ((CMatrixBuilding*)(ally_robots[i]->GetEnv()->m_TargetAttack))->m_Pos.x - ally_robots[i]->m_PosX;
                    tvy = ((CMatrixBuilding*)(ally_robots[i]->GetEnv()->m_TargetAttack))->m_Pos.y - ally_robots[i]->m_PosY;
                    enemy_fire_dist = 150;//int(GLOBAL_SCALE_MOVE*12.0f);
                }
                else continue;

                float tsize2 = tvx * tvx + tvy * tvy;
                float tsize2o = 1.0f / tsize2;

                //SMatrixPlaceList* plist = g_MatrixMap->m_RoadNetwork.m_PLList + plr.left+plr.top * g_MatrixMap->m_RoadNetwork.m_PLSizeX;
                //for(y = plr.top; y < plr.bottom; ++y, plist += g_MatrixMap->m_RoadNetwork.m_PLSizeX - (plr.right - plr.left)) {
                //    for(x = plr.left; x < plr.right; ++x, ++plist) {
                //        SMatrixPlace* place = g_MatrixMap->m_RoadNetwork.m_Place + plist->m_Sme;
                //        for(u = 0; u < plist->m_Cnt; ++u, ++place) {
                for(u = 0; u < listcnt; ++u)
                {
                    int iplace = m_PlaceList[u];
                    SMatrixPlace* place = g_MatrixMap->m_RoadNetwork.m_Place + iplace;

                    if(place->m_Data) continue; // Занетые места игнорируем
                    if(place->m_Move & (1 << (ally_robots[i]->m_Module[0].m_Kind - 1))) continue; // Если робот не может стоять на этом месте то пропускаем
                    if(ally_robots[i]->GetEnv()->IsBadPlace(iplace)) continue; // Плохое место пропускаем

                    float pcx = GLOBAL_SCALE_MOVE * place->m_Pos.x + (GLOBAL_SCALE_MOVE * 4.0f / 2.0f); // Center place
                    float pcy = GLOBAL_SCALE_MOVE * place->m_Pos.y + (GLOBAL_SCALE_MOVE * 4.0f / 2.0f);

                    float pvx = pcx - ally_robots[i]->m_PosX; // To place
                    float pvy = pcy - ally_robots[i]->m_PosY;

                    float k = (pvx * tvx + pvy * tvy) * tsize2o;
                    if(!havebomb && ally_robots[i]->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_TURRET)
                    {
                        //if(k>1.5) continue; // Места за врагом не рассматриваем
                    }
                    else if(!havebomb && ally_robots[i]->GetEnv()->m_TargetAttack->GetObjectType() != OBJECT_TYPE_BUILDING)
                    {
                        if(k > 0.95f) continue; // Места за врагом не рассматриваем
                    }
                    else if(!havebomb)
                    {
                        if(k > 1.20f) continue; // Места сильно за врагом не расматриваем
                    }
                    //if(k < 0.0f) continue; // Места за роботом игнорируем
                    float m = (-pvx * tvy + pvy * tvx) * tsize2o;
                    float distfrom2 = POW2(-m * tvy) + POW2(m * tvx); // Дистанция отклонения
                    float distplace2 = POW2(tvx - pcx/*pvx*/) + POW2(tvy - pcx/*pvy*/); // Дистанция от места до врага
                    if((placebest < 0) || (ally_robots[i]->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_TURRET && (ally_robots[i]->GetMaxFireDist() - GLOBAL_SCALE_MOVE) > enemy_fire_dist))
                    {
                        if(distplace2 > POW2(0.95 * ally_robots[i]->GetMaxFireDist() - GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2)) continue; // Робот должен достовать врага
                    }
                    else
                    {
                        if(distplace2 > POW2(0.95 * ally_robots[i]->GetMinFireDist() - GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2)) continue; // Робот должен достовать врага
                    }

                    if(!havebomb && ally_robots[i]->GetEnv()->m_TargetAttack->GetObjectType() == OBJECT_TYPE_ROBOT_AI)
                    {
                        if(distfrom2 > POW2(200 + 100)) continue; // Робот не должен отклонится слишком далеко
//                            } else {
//                                if(distfrom2>POW2(300+100)) continue; // Робот не должен отклонится слишком далеко
                    }

                    //// Если место слишком близко к постройке, то игнорируем
                    //if(ally_robots[i]->GetEnv()->m_TargetAttack->GetObjectType()==OBJECT_TYPE_BUILDING) {
                    //    if(distplace2<POW2(200)) continue;
                    //}

                    int underfire = place->m_Underfire;
                    if (distplace2 <= POW2(enemy_fire_dist)) underfire++;

                    CMatrixMapStatic* trace_res = g_MatrixMap->Trace(nullptr, D3DXVECTOR3(pcx, pcy, g_MatrixMap->GetZ(pcx, pcy) + 20.0f)/*ally_robots[i]->GetGeoCenter()*/, SetPointOfAim(ally_robots[i]->GetEnv()->m_TargetAttack), TRACE_OBJECT | TRACE_NONOBJECT | TRACE_OBJECT_SPHERE | TRACE_SKIP_INVISIBLE, ally_robots[i]);
                    bool close = (IS_TRACE_STOP_OBJECT(trace_res) && trace_res->GetObjectType() == OBJECT_TYPE_MAPOBJECT) || (trace_res == TRACE_STOP_WATER) || (trace_res == TRACE_STOP_LANDSCAPE);

                    if (placebest >= 0) { // Если уже найдено место то выбираем наилучшее
                        if (havebomb) {
                            if (distplace2 > s_f1) continue; // Место дальше предыдущего пропускаем
                        }
                        else if (close != s_close) {
                            if (close) continue;
                        }
                        else if (!underfire && s_underfire);
                        else if (underfire && !s_underfire) continue;
                        else if (underfire) { // Если под обстрелом
                            if (underfire > s_underfire) continue; // Место более обстреливоемое пропускаем
                            if (distplace2 < s_f1) continue; // Место ближе предыдущего пропускаем
                        }
                        else { // Если вне радиуса поражения противника
                            if (distplace2 > s_f1) continue; // Место дальше предыдущего пропускаем
                        }
                    }

                    s_close = close;
                    s_f1 = distplace2;
                    s_underfire = underfire;
                    placebest = iplace;
                }
                //    }
                //}

                if (placebest >= 0) {
                    cplr = false;
                    SMatrixPlace* place = GetPlacePtr(placebest);
                    place->m_Data = 1;
                    ally_robots[i]->GetEnv()->m_Place = placebest;
                    if (PrepareBreakOrder(ally_robots[i]))
                    {
                        ally_robots[i]->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                    }
                }
                else if(cplr)
                {
                    cplr = false;
                    if(g_MatrixMap->PlaceList(mm, GetMapPos(ally_robots[0]), center, radiusrobot, false, m_PlaceList, &listcnt) == 0)
                    {
                        for(u = 0; u < ally_robots_cnt; ++u) ally_robots[u]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();
                        break;
                    }
                    else
                    {
                        for(u = 0; u < listcnt; ++u) g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[u]].m_Data = 0;
                        obj = CMatrixMapStatic::GetFirstLogic();
                        while(obj)
                        {
                            if(obj->IsUnitAlive()) ObjPlaceData(obj, 1);
                            obj = obj->GetNextLogic();
                        }
                        i = -1;
                        continue;
                    }
                }
                else //Не нашли
                {
                    ally_robots[i]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();

                    int iplace;
                    SMatrixPlace* place;

                    for(u = 0; u < listcnt; ++u)
                    {
                        iplace = m_PlaceList[u];
                        place = g_MatrixMap->m_RoadNetwork.m_Place + iplace;

                        if(place->m_Data) continue; // Занетые места игнорируем
                        if(place->m_Move & (1 << (ally_robots[i]->m_Module[0].m_Kind - 1))) continue; // Если робот не может стоять на этом месте то пропускаем
                        break;
                    }

                    if(u < listcnt)
                    {
                        place->m_Data = 1;
                        ally_robots[i]->GetEnv()->m_Place = iplace;
                        if(PrepareBreakOrder(ally_robots[i]))
                        {
                            ally_robots[i]->MoveToHigh(place->m_Pos.x, place->m_Pos.y);
                        }
                    }
                    else //Расширяем
                    {
                        if(g_MatrixMap->PlaceListGrow(mm, m_PlaceList, &listcnt, ally_robots_cnt) <= 0) continue;

                        for(u = 0; u < listcnt; ++u) g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[u]].m_Data = 0;
                        obj = CMatrixMapStatic::GetFirstLogic();
                        while(obj)
                        {
                            if(obj->IsUnitAlive()) ObjPlaceData(obj, 1);
                            obj = obj->GetNextLogic();
                        }
                    }
                }
            }
        }
    }

    // Корректируем точку выстрела
    D3DXVECTOR3 des, from, dir, p;
    float t, dist;

    int curTime = g_MatrixMap->GetTime();

    for(i = 0; i < ally_robots_cnt; ++i)
    {
        if(ally_robots[i]->GetEnv()->m_TargetAttack)
        {
            if(!ally_robots[i]->GetEnv()->m_TargetAttack->IsAlive())
            {
                ally_robots[i]->StopFire();
                continue;
            }

            des = SetPointOfAim(ally_robots[i]->GetEnv()->m_TargetAttack);

            // Не стрелять из прямого оружия, если на пути к цели свои
            from = ally_robots[i]->GetGeoCenter();
            dist = sqrt(POW2(from.x - des.x) + POW2(from.y - des.y) + POW2(from.z - des.z));

            bool fire_line = ally_robots[i]->HaveRepair() != 2;

            if(fire_line && dist > 0.0f)
            {
                t = 1.0f / dist;
                dir.x = (des.x - from.x) * t;
                dir.y = (des.y - from.y) * t;
                dir.z = (des.z - from.z) * t;

                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsUnitAlive() && obj->GetSide() == m_Id && ally_robots[i] != obj && ally_robots[i]->GetEnv()->m_TargetAttack != obj)
                    {
                        p = SetPointOfAim(obj);

                        if(IsIntersectSphere(p, 25.0f, from, dir, t))
                        {
                            if(t >= 0.0f && t < dist)
                            {
                                //CHelper::DestroyByGroup(dword(this) + 4);
                                //CHelper::Create(10, dword(this) + 4)->Cone(from, des, 1.0f, 1.0f, 0xffffffff, 0xffffffff, 3);
                                //CHelper::Create(10, dword(this) + 4)->Sphere(D3DXVECTOR3(from.x + dir.x * t, from.y + dir.y * t, from.z + dir.z * t), 2, 5, 0xffff0000);

                                fire_line = false;
                                break;
                            }
                        }
                    }

                    obj = obj->GetNextLogic();
                }
            }

            //des.x += g_MatrixMap->RndFloat(-5.0f, 5.0f);
            //des.y += g_MatrixMap->RndFloat(-5.0f, 5.0f);
            //des.z += g_MatrixMap->RndFloat(-5.0f, 5.0f);

            CInfo* env = GetEnv(ally_robots[i]);
            if(env->m_TargetAttack != env->m_TargetLast)
            {
                env->m_TargetLast = env->m_TargetAttack;
                env->m_TargetChange = curTime;
            }

            if(fire_line)
            {
                D3DXVECTOR3 v1 = ally_robots[i]->GetGeoCenter();
                D3DXVECTOR3 v2 = SetPointOfAim(ally_robots[i]->GetEnv()->m_TargetAttack);

                D3DXVECTOR3 temp = v2 - v1;
                fire_line = D3DXVec3LengthSq(&temp) <= POW2(ally_robots[i]->GetMaxFireDist());

                if(fire_line)
                {
                    CMatrixMapStatic* trace_res = g_MatrixMap->Trace(nullptr, v1, v2, TRACE_OBJECT | TRACE_NONOBJECT, ally_robots[i]);
                    fire_line = !((IS_TRACE_STOP_OBJECT(trace_res) && trace_res->GetObjectType() == OBJECT_TYPE_MAPOBJECT) || (trace_res == TRACE_STOP_WATER) || (trace_res == TRACE_STOP_LANDSCAPE));
                }
            }

            if(fire_line)
            {
                //Если у цели голова Firewall то в нее сложнее попасть
                /*
                if(env->m_TargetAttack->IsRobot() && env->m_TargetAttack->AsRobot()->m_AimProtect > 0)
                {
                    if(env->m_Target!=env->m_TargetAttack || fabs(env->m_TargetAngle) <= 1.0f * 1.1f * ToRad)
                    {
                        env->m_TargetAngle = 0.0f;

                        env->m_TargetAngle=min(30.0f * ToRad, (float)atan(env->m_TargetAttack->AsRobot()->m_AimProtect / sqrt(POW2(des.x - ally_robots[i]->m_PosX) + POW2(des.y - ally_robots[i]->m_PosY))));
                        if(g_MatrixMap->Rnd(0, 9) < 5) env->m_TargetAngle = -env->m_TargetAngle;
                    }
                    else if(env->m_TargetAngle > 0) env->m_TargetAngle -= 1.0f * ToRad;
                    else env->m_TargetAngle += 1.0f * ToRad;
                */

                if(env->m_TargetAttack->IsRobot() && env->m_TargetAttack->AsRobot()->m_AimProtect > 0)
                {
                    if(env->m_Target != env->m_TargetAttack || fabs(env->m_TargetAngle) <= 1.3f * ToRad) //>= 15.0f * env->m_TargetAttack->AsRobot()->m_AimProtect * ToRad) {
                    {
                        env->m_TargetAngle = 0.0f;

                        env->m_TargetAngle = min(8.0f * ToRad, (float)g_MatrixMap->Rnd(1, 100) / 100.0f * 16.0f * env->m_TargetAttack->AsRobot()->m_AimProtect * ToRad);
                        if(g_MatrixMap->Rnd(0, 9) < 5) env->m_TargetAngle = -env->m_TargetAngle;
                    }
                    //else if(env->m_TargetAngle > 0) env->m_TargetAngle += 1.0f * ToRad;
                    //else if(env->m_TargetAngle < 0) env->m_TargetAngle -= 1.0f * ToRad;
                    //else if(fabs(env->m_TargetAngle) > 1.0f) env->m_TargetAngle *= 0.7f;
                    //else env->m_TargetAngle = (2 * g_MatrixMap->Rnd(0, 1) - 1) * ToRad;
                    else env->m_TargetAngle *= 0.75f;

                    if(env->m_TargetAngle != 0.0f)
                    {
                        float vx = des.x - ally_robots[i]->m_PosX;
                        float vy = des.y - ally_robots[i]->m_PosY;
                        float sa, ca;
                        SinCos(env->m_TargetAngle, &sa, &ca);
                        des.x = (ca * vx + sa * vy) + ally_robots[i]->m_PosX;
                        des.y = (-sa * vx + ca * vy) + ally_robots[i]->m_PosY;
                    }
                }

                env->m_Target = env->m_TargetAttack;
                env->m_LastFire = curTime;
                ally_robots[i]->OrderFire(des);

                //Если стоим на месте
                if(IsInPlace(ally_robots[i]))
                {
                    //Если несколько врагов и в цель не попадаем в течении долгого времени, то переназначаем цель
                    if(env->m_EnemyCnt > 1 && (curTime - env->m_TargetChange) > 4000 && (curTime - env->m_LastHitTarget) > 4000)
                    {
                        env->m_TargetAttack = nullptr;
                    }

                    //Если один враг и в цель не попадаем в течении долгого времени и стоим на месте, то переназначаем место
                    if(env->m_EnemyCnt == 1 && (curTime - env->m_TargetChange) > 4000 && (curTime - env->m_LastHitTarget) > 4000)
                    {
                        env->AddBadPlace(env->m_Place);
                        env->m_Place = -1;
                    }

                    //Если очень долго не попадаем в цель, то меняем позицию
                    if((curTime - env->m_TargetChange) > 2000 && (curTime - env->m_LastHitTarget) > 10000)
                    {
                        env->AddBadPlace(env->m_Place);
                        env->m_Place = -1;
                    }
                }
                else env->m_TargetChange = curTime;
            }
            else
            {
                if(ally_robots[i]->HaveRepair() && (g_MatrixMap->GetTime() - ally_robots[i]->GetEnv()->m_TargetChangeRepair) > 1000) //Ищем робота для починки
                {
                    D3DXVECTOR2 v, v2;

                    if(ally_robots[i]->GetEnv()->m_Target && ally_robots[i]->GetEnv()->m_Target->IsUnitAlive() && ally_robots[i]->GetEnv()->m_Target->GetSide() == m_Id && ally_robots[i]->GetEnv()->m_Target->NeedRepair())
                    {
                        v = GetWorldPos(ally_robots[i]);
                        v2 = GetWorldPos(ally_robots[i]->GetEnv()->m_Target);
                        if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) > POW2(ally_robots[i]->GetRepairDist())) ally_robots[i]->GetEnv()->m_Target = nullptr;
                    }
                    else ally_robots[i]->GetEnv()->m_Target = nullptr;

                    if(!ally_robots[i]->GetEnv()->m_Target)
                    {
                        obj = CMatrixMapStatic::GetFirstLogic();
                        while(obj)
                        {
                            if(obj != ally_robots[i] && obj->IsUnitAlive() && obj->GetSide() == m_Id && obj->NeedRepair())
                            {
                                v = GetWorldPos(ally_robots[i]);
                                v2 = GetWorldPos(obj);
                                if((POW2(v.x - v2.x) + POW2(v.y - v2.y)) < POW2(ally_robots[i]->GetRepairDist()))
                                {
                                    ally_robots[i]->GetEnv()->m_Target = obj;
                                    ally_robots[i]->GetEnv()->m_TargetChangeRepair = g_MatrixMap->GetTime();
                                    break;
                                }
                            }

                            obj = obj->GetNextLogic();
                        }
                    }
                }

                if(ally_robots[i]->GetEnv()->TargetType() == TARGET_IS_FRIEND) ally_robots[i]->OrderFire(SetPointOfAim(ally_robots[i]->GetEnv()->m_Target), TARGET_IS_FRIEND);
                else ally_robots[i]->StopFire();

                // Если стоим на месте
                if(IsInPlace(ally_robots[i]))
                {
                    // Если несколько врагов, а текущий долго закрыт своими, то переназначаем цель
                    if(env->m_EnemyCnt > 1 && (curTime - env->m_TargetChange) > 4000 && (curTime - env->m_LastFire) > 4000)
                    {
                        env->m_TargetAttack = nullptr;
                    }
                    // Если долго закрыт своими, то меняем позицию
                    if((curTime - env->m_TargetChange) > 4000 && (curTime - env->m_LastFire) > 6000)
                    {
                        if(CanChangePlace(ally_robots[i]))
                        {
                            env->AddBadPlace(env->m_Place);
                            env->m_Place = -1;
                        }
                    }
                }
                else env->m_TargetChange = curTime;
            }
        }
    }
}

int CMatrixSideUnit::SelGroupToLogicGroup()
{
    CMatrixMapStatic *obj;
    int i, no;

    for(i = 0; i < MAX_LOGIC_GROUP; ++i) m_PlayerGroup[i].m_RobotCnt = 0;

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
        {
            if(obj->AsRobot()->GetGroupLogic() >= 0 && obj->AsRobot()->GetGroupLogic() < MAX_LOGIC_GROUP)
            {
                ++m_PlayerGroup[obj->AsRobot()->GetGroupLogic()].m_RobotCnt;
            }
        }
        obj = obj->GetNextLogic();
    }

    for(no = 0; no < MAX_LOGIC_GROUP; ++no)
    {
        if(m_PlayerGroup[no].m_RobotCnt <= 0) break;
    }
    ASSERT(no < MAX_LOGIC_GROUP);

    m_PlayerGroup[no].Order(mpo_Stop);
    m_PlayerGroup[no].m_Obj = nullptr;
    m_PlayerGroup[no].SetWar(false);
    m_PlayerGroup[no].m_RoadPath->Clear();

    CMatrixGroupObject *objs = GetCurGroup()->m_FirstObject;
    while(objs)
    {
        if(objs->ReturnObject() && objs->ReturnObject()->IsRobotAlive())
        {
            m_PlayerGroup[no].m_RobotCnt++;
            objs->ReturnObject()->AsRobot()->SetGroupLogic(no);
        }
        objs = objs->m_NextObject;
    }

    return no;
}

int CMatrixSideUnit::RobotToLogicGroup(CMatrixRobotAI *robot)
{
    CMatrixMapStatic *obj;
    int i, no;

    for(i = 0; i < MAX_LOGIC_GROUP; ++i) m_PlayerGroup[i].m_RobotCnt = 0;

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
        {
            if(obj->AsRobot()->GetGroupLogic() >= 0 && obj->AsRobot()->GetGroupLogic() < MAX_LOGIC_GROUP)
            {
                ++m_PlayerGroup[obj->AsRobot()->GetGroupLogic()].m_RobotCnt;
            }
        }
        obj = obj->GetNextLogic();
    }

    for(no = 0; no < MAX_LOGIC_GROUP; ++no)
    {
        if(m_PlayerGroup[no].m_RobotCnt <= 0) break;
    }
    ASSERT(no < MAX_LOGIC_GROUP);

    m_PlayerGroup[no].Order(mpo_Stop);
    m_PlayerGroup[no].m_Obj = nullptr;
    m_PlayerGroup[no].SetWar(false);
    m_PlayerGroup[no].m_RoadPath->Clear();

    ++m_PlayerGroup[no].m_RobotCnt;
    robot->SetGroupLogic(no);

    return no;
}

void CMatrixSideUnit::PGOrderStop(int no)
{
    if(m_PlayerGroup[no].m_RobotCnt <= 0) return;

    m_PlayerGroup[no].Order(mpo_Stop);
    m_PlayerGroup[no].m_To = PGCalcCenterGroup(no);
    //PGPlaceClear(no);
    m_PlayerGroup[no].m_Obj = nullptr;

    //Маркер для приказа "Stop" рисуется только при его непосредственном применении
    m_PlayerGroup[no].SetShowPlace(false);
    m_PlayerGroup[no].m_RoadPath->Clear();

    PGRemoveAllPassive(no, m_PlayerGroup[no].m_Obj);

    //if(m_PlayerGroup[no].m_To.Dist2(PGCalcPlaceCenter(no)) > POW2(10))
    //{
    //    PGAssignPlacePlayer(no, m_PlayerGroup[no].m_To);
    //}
    //else PGShowPlace(no);

    m_LastTactHL = -1000;
    TactPL();
    //PGShowPlace(no);
}

//Функция полноценной выдачи приказа на движение юнита/группы (включая воспроизведение звуков и отрисовку вейпоинта)
//В первом аргументе получает номер группы юнитов, а во втором указатель точки на местности
//PG - в данном случае Player Group
void CMatrixSideUnit::PGOrderMoveTo(int no, const CPoint& tp)
{
    if(m_PlayerGroup[no].m_RobotCnt <= 0) return;
    
    //Играется случайный звук ответа юнита на приказ движения
    if(m_PlayerGroup[no].Order() == mpo_MoveTo)
    {
        //in progress
        if(!IRND(2)) CSound::Play(S_ORDER_INPROGRESS1, SL_ORDER);
        else CSound::Play(S_ORDER_INPROGRESS2, SL_ORDER);
    }
    else
    {
        if(GetCurGroup())
        {
            if(GetCurGroup()->GetObjectByN(GetCurSelNum())->IsRobotAlive())
            {
                //Играется случайный звук движения шасси робота
                int chassis_kind = (int)GetCurGroup()->GetObjectByN(GetCurSelNum())->AsRobot()->m_Module[0].m_Kind;
                int cnt = g_Config.m_RobotChassisConsts[chassis_kind].move_out_sound_name.size();
                if(cnt) CSound::Play(g_Config.m_RobotChassisConsts[chassis_kind].move_out_sound_name[IRND(cnt)], SL_ORDER);
            }
        }

        //int o = IRND(4);
        //if(o == 0) CSound::Play(S_ORDER_ACCEPT, SL_ORDER);
        //else if(o == 1) CSound::Play(S_ORDER_MOVE_TO_1, SL_ORDER);
        //else if(o == 2) CSound::Play(S_ORDER_MOVE_TO_2, SL_ORDER);
        //else if(o == 3) CSound::Play(S_ORDER_MOVE_TO_3, SL_ORDER);
    }

    //Выставляются маркера приказа движения
    m_PlayerGroup[no].Order(mpo_MoveTo);
    m_PlayerGroup[no].m_To = tp; //Куда идти
    m_PlayerGroup[no].m_Obj = nullptr; //В кого стрелять/захватывать
    m_PlayerGroup[no].SetShowPlace(true);
    m_PlayerGroup[no].m_RoadPath->Clear();

    PGRemoveAllPassive(no, m_PlayerGroup[no].m_Obj);

    PGAssignPlacePlayer(no, tp);

    m_LastTactHL = -1000;
    TactPL();
    PGShowPlace(no);
}

void CMatrixSideUnit::SoundCapture(int pg)
{
    if(FLAG(g_MatrixMap->m_Flags, MMFLAG_SOUND_ORDER_CAPTURE_ENABLED))
    {
        if(m_PlayerGroup[pg].Order() == mpo_Capture)
        {
            // in progress
            int o = IRND(2);
            if(o == 0) CSound::Play(S_ORDER_INPROGRESS1, SL_ORDER);
            else CSound::Play(S_ORDER_INPROGRESS2, SL_ORDER);
        }
        else
        {
            CSound::Play(S_ORDER_CAPTURE, SL_ORDER);
        }
    }

    RESETFLAG(g_MatrixMap->m_Flags, MMFLAG_SOUND_ORDER_CAPTURE_ENABLED);
}

void CMatrixSideUnit::PGOrderCapture(int no, CMatrixBuilding* building)
{
    if(m_PlayerGroup[no].m_RobotCnt <= 0) return;

    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_ENABLE_CAPTURE_FUCKOFF_SOUND);
    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_SOUND_ORDER_CAPTURE_ENABLED);

    if(m_PlayerGroup[no].Order() == mpo_Capture)
    {
        //in progress
        if(!IRND(2)) CSound::Play(S_ORDER_INPROGRESS1, SL_ORDER);
        else CSound::Play(S_ORDER_INPROGRESS2, SL_ORDER);
    }
    else CSound::Play(S_ORDER_CAPTURE_PUSH, SL_ORDER);

    m_PlayerGroup[no].Order(mpo_Capture);
    m_PlayerGroup[no].m_To = GetMapPos(building);
    m_PlayerGroup[no].m_Obj = building;
    m_PlayerGroup[no].SetShowPlace(true);
    m_PlayerGroup[no].m_RoadPath->Clear();

    PGRemoveAllPassive(no, m_PlayerGroup[no].m_Obj);

    CMatrixMapStatic *obj;
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
        {
            CMatrixRobotAI* robot = obj->AsRobot();
            CMatrixBuilding *b2 = robot->GetCaptureBuilding();
            if(robot->GetGroupLogic() == no)
            {
                if(b2 && b2 != building)
                {
                    if(robot->CanBreakOrder())
                    {
                        robot->StopCapture();
                        robot->GetEnv()->m_Place = -1;
                    }
                }
            }
            else
            {
                if(b2 && b2 == building)
                {
                    if(robot->CanBreakOrder())
                    {
                        robot->StopCapture();
                        robot->GetEnv()->m_Place = -1;
                    }
                }
            }
        }

        obj = obj->GetNextLogic();
    }

    m_LastTactHL = -1000;
    TactPL(no);
    PGShowPlace(no);
}

void CMatrixSideUnit::PGOrderAttack(int no, const CPoint& tp, CMatrixMapStatic* target_obj)
{
    if(m_PlayerGroup[no].m_RobotCnt <= 0) return;

    if(!FLAG(g_MatrixMap->m_Flags, MMFLAG_SOUND_ORDER_ATTACK_DISABLE))
    {
        if(m_PlayerGroup[no].Order() == mpo_Attack)
        {
            // in progress
            if(!IRND(2)) CSound::Play(S_ORDER_INPROGRESS1, SL_ORDER);
            else CSound::Play(S_ORDER_INPROGRESS2, SL_ORDER);
        }
        else
        {
            CSound::Play(S_ORDER_ATTACK, SL_ORDER);
        }
    }

    m_PlayerGroup[no].Order(mpo_Attack);
    m_PlayerGroup[no].m_To = tp;
    m_PlayerGroup[no].m_Obj = target_obj;
    m_PlayerGroup[no].SetShowPlace(true);
    m_PlayerGroup[no].m_RoadPath->Clear();

    PGRemoveAllPassive(no, m_PlayerGroup[no].m_Obj);

    CMatrixMapStatic* obj;
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no)
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;
            robot->GetEnv()->m_Place = -1;
            robot->GetEnv()->m_PlaceAdd = CPoint(-1, -1);
        }

        obj = obj->GetNextLogic();
    }

    if(target_obj)
    {
        // Точка сбора подальше от базы, а то могут вместе с ней зарваться
        if(target_obj->GetObjectType() == OBJECT_TYPE_BUILDING /*&& ((CMatrixBuilding *)target_obj)->m_Kind == 0*/)
        {
            switch(((CMatrixBuilding*)target_obj)->m_Angle)
            {
                case 0: m_PlayerGroup[no].m_To.y += 20; break;
                case 1: m_PlayerGroup[no].m_To.x -= 20; break;
                case 2: m_PlayerGroup[no].m_To.y -= 20; break;
                case 3: m_PlayerGroup[no].m_To.x += 20; break;
            }
        }

        CMatrixMapStatic* obj;
        obj = CMatrixMapStatic::GetFirstLogic();
        while(obj)
        {
            if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no)
            {
                CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;

                if(robot->GetEnv()->SearchEnemy(target_obj))
                {
                    robot->GetEnv()->m_Target = target_obj;
                }
                else
                {
                    //                    if(IsBuildingAlive(target_obj))
                    //                    {
                    //                        robot->GetEnv()->AddToList(target_obj);
                    //                        robot->GetEnv()->m_Target = target_obj;
                    //                    }
                }
            }
            obj = obj->GetNextLogic();
        }
    }

    m_LastTactHL = -1000;
    TactPL();
    PGShowPlace(no);
}

void CMatrixSideUnit::PGOrderPatrol(int no, const CPoint& tp)
{
    if(m_PlayerGroup[no].m_RobotCnt <= 0)  return;

    if(m_PlayerGroup[no].Order() == mpo_Patrol)
    {
        // in progress
        if(!IRND(2)) CSound::Play(S_ORDER_INPROGRESS1, SL_ORDER);
        else CSound::Play(S_ORDER_INPROGRESS2, SL_ORDER);
    }
    else
    {
        if(GetCurGroup())
        {
            if(GetCurGroup()->GetObjectByN(GetCurSelNum())->IsRobotAlive())
            {
                //Играется случайный звук движения шасси робота
                int chassis_kind = (int)GetCurGroup()->GetObjectByN(GetCurSelNum())->AsRobot()->m_Module[0].m_Kind;
                int cnt = g_Config.m_RobotChassisConsts[chassis_kind].move_out_sound_name.size();
                if(cnt) CSound::Play(g_Config.m_RobotChassisConsts[chassis_kind].move_out_sound_name[IRND(cnt)], SL_ORDER);
            }
        }
    }

    m_PlayerGroup[no].Order(mpo_Patrol);
    m_PlayerGroup[no].m_To = tp;
    m_PlayerGroup[no].m_From = PGCalcPlaceCenter(no);
    m_PlayerGroup[no].m_Obj = nullptr;
    m_PlayerGroup[no].SetPatrolReturn(false);
    m_PlayerGroup[no].SetShowPlace(true);
    m_PlayerGroup[no].m_RoadPath->Clear();

    PGRemoveAllPassive(no, m_PlayerGroup[no].m_Obj);

    PGAssignPlacePlayer(no, tp);

    m_LastTactHL = -1000;
    TactPL();
    PGShowPlace(no);
}

void CMatrixSideUnit::PGOrderRepair(int no, CMatrixMapStatic* target_obj)
{
    if(m_PlayerGroup[no].m_RobotCnt <= 0)  return;

    if(m_PlayerGroup[no].Order() == mpo_Repair)
    {
        // in progress
        if(!IRND(2)) CSound::Play(S_ORDER_INPROGRESS1, SL_ORDER);
        else CSound::Play(S_ORDER_INPROGRESS2, SL_ORDER);
    }
    else
    {
        if(!IRND(2)) CSound::Play(S_ORDER_ACCEPT, SL_ORDER);
        else CSound::Play(S_ORDER_REPAIR, SL_ORDER);
    }

    m_PlayerGroup[no].Order(mpo_Repair);
    m_PlayerGroup[no].m_To = GetMapPos(target_obj);
    m_PlayerGroup[no].m_From = CPoint(-1, -1);
    m_PlayerGroup[no].m_Obj = target_obj;
    m_PlayerGroup[no].SetShowPlace(true);
    m_PlayerGroup[no].m_RoadPath->Clear();

    PGRemoveAllPassive(no, m_PlayerGroup[no].m_Obj);

    //PGAssignPlacePlayer(no, m_PlayerGroup[no].m_To);

    m_LastTactHL = -1000;
    TactPL();
    PGShowPlace(no);
}

void CMatrixSideUnit::PGOrderBomb(int no, const CPoint& tp, CMatrixMapStatic* target_obj)
{
    if(m_PlayerGroup[no].m_RobotCnt <= 0) return;

    m_PlayerGroup[no].Order(mpo_Bomb);
    m_PlayerGroup[no].m_To = tp;
    m_PlayerGroup[no].m_Obj = target_obj;
    m_PlayerGroup[no].SetShowPlace(true);
    m_PlayerGroup[no].m_RoadPath->Clear();

    PGRemoveAllPassive(no, m_PlayerGroup[no].m_Obj);

    CMatrixMapStatic* obj;
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no)
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;
            robot->GetEnv()->m_Place = -1;
            robot->GetEnv()->m_PlaceAdd = CPoint(-1, -1);
        }
        obj = obj->GetNextLogic();
    }

    m_LastTactHL = -1000;
    TactPL();
    PGShowPlace(no);
}

void CMatrixSideUnit::PGOrderAutoCapture(int no)
{
    if(m_PlayerGroup[no].m_RobotCnt <= 0) return;

    if(m_PlayerGroup[no].Order() == mpo_AutoCapture)
    {
        // in progress
        if(!IRND(2)) CSound::Play(S_ORDER_INPROGRESS1, SL_ORDER);
        else CSound::Play(S_ORDER_INPROGRESS2, SL_ORDER);
    }
    else
    {
        CSound::Play(S_ORDER_AUTO_CAPTURE, SL_ORDER);
    }

    m_PlayerGroup[no].Order(mpo_AutoCapture);
    m_PlayerGroup[no].m_To = CPoint(-1, -1);
    m_PlayerGroup[no].m_From = CPoint(-1, -1);
    m_PlayerGroup[no].m_Obj = nullptr;
    m_PlayerGroup[no].SetShowPlace(false);
    m_PlayerGroup[no].m_RoadPath->Clear();

    PGRemoveAllPassive(no, m_PlayerGroup[no].m_Obj);
}

//Выставляем всем роботам группы игрока маркер автоподрыва бомб при нулевом HP
void CMatrixSideUnit::PGAutoBoomSet(CMatrixGroup* work_group, bool set)
{
    if(!work_group) return;

    //Подходящего звука, естественно, нет
    if(!IRND(2)) CSound::Play(S_ORDER_INPROGRESS1, SL_ORDER);
    else CSound::Play(S_ORDER_INPROGRESS2, SL_ORDER);

    CMatrixGroupObject* obj = work_group->m_FirstObject;
    while(obj)
    {
        CMatrixMapStatic* unit = obj->ReturnObject();
        if(unit->AsRobot()->HaveBomb()) unit->AsRobot()->AutoBoomSet(set);
        obj = obj->m_NextObject;
    }
}

void CMatrixSideUnit::PGOrderAutoAttack(int no)
{
    if(m_PlayerGroup[no].m_RobotCnt <= 0) return;

    if(m_PlayerGroup[no].Order() == mpo_AutoAttack)
    {
        // in progress
        int o = IRND(2);
        if(o == 0) CSound::Play(S_ORDER_INPROGRESS1, SL_ORDER);
        else CSound::Play(S_ORDER_INPROGRESS2, SL_ORDER);
    }
    else
    {
        CSound::Play(S_ORDER_AUTO_ATTACK, SL_ORDER);
    }

    m_PlayerGroup[no].Order(mpo_AutoAttack);
    m_PlayerGroup[no].m_To = CPoint(-1, -1);
    m_PlayerGroup[no].m_From = CPoint(-1, -1);
    m_PlayerGroup[no].m_Obj = nullptr;
    m_PlayerGroup[no].SetShowPlace(false);
    m_PlayerGroup[no].m_Region = -1;
    m_PlayerGroup[no].m_RoadPath->Clear();

    PGRemoveAllPassive(no, m_PlayerGroup[no].m_Obj);

    CMatrixMapStatic* obj;
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
        {
            CMatrixRobotAI* robot = obj->AsRobot();
            if(robot->GetGroupLogic() == no)
            {
                if(robot->GetCaptureBuilding())
                {
                    if(robot->CanBreakOrder())
                    {
                        robot->StopCapture();
                        robot->GetEnv()->m_Place = -1;
                        robot->GetEnv()->m_PlaceNotFound = -10000;
                    }
                }
            }
        }
        obj = obj->GetNextLogic();
    }
}

void CMatrixSideUnit::PGOrderAutoDefence(int no)
{
    if(m_PlayerGroup[no].m_RobotCnt <= 0) return;

    if(m_PlayerGroup[no].Order() == mpo_AutoDefence)
    {
        // in progress
        int o = IRND(2);
        if(o == 0) CSound::Play(S_ORDER_INPROGRESS1, SL_ORDER);
        else CSound::Play(S_ORDER_INPROGRESS2, SL_ORDER);

    }
    else
    {
        CSound::Play(S_ORDER_AUTO_DEFENCE, SL_ORDER);
    }

    CPoint tp(0, 0);
    int cnt = 0;
    CMatrixMapStatic* obj;
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id)
        {
            CMatrixRobotAI* robot = obj->AsRobot();
            if(robot->GetGroupLogic() == no)
            {
                tp.x += robot->GetMapPosX();
                tp.y += robot->GetMapPosY();
                ++cnt;
                if(robot->GetCaptureBuilding())
                {
                    if(robot->CanBreakOrder())
                    {
                        robot->StopCapture();
                        robot->GetEnv()->m_Place = -1;
                        robot->GetEnv()->m_PlaceNotFound = -10000;
                    }
                }
            }
        }
        obj = obj->GetNextLogic();
    }

    m_PlayerGroup[no].Order(mpo_AutoDefence);
    m_PlayerGroup[no].m_To = CPoint(-1, -1);
    m_PlayerGroup[no].m_From = CPoint(-1, -1);
    m_PlayerGroup[no].m_Obj = nullptr;
    m_PlayerGroup[no].SetShowPlace(false);
    m_PlayerGroup[no].m_Region = g_MatrixMap->GetRegion(tp.x / cnt, tp.y / cnt);
    m_PlayerGroup[no].m_RoadPath->Clear();

    PGRemoveAllPassive(no, m_PlayerGroup[no].m_Obj);

    PGAssignPlace(no, g_MatrixMap->m_RoadNetwork.GetRegion(m_PlayerGroup[no].m_Region)->m_Center);

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no)
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;

            tp = PLPlacePos(robot);
            if(tp.x >= 0 && PrepareBreakOrder(robot)) robot->MoveToHigh(tp.x, tp.y);
        }
        obj = obj->GetNextLogic();
    }
}

void CMatrixSideUnit::PGRemoveAllPassive(int no, CMatrixMapStatic* skip)
{
#define IsPassive(obj) (((obj)->GetObjectType() == OBJECT_TYPE_BUILDING) || ((obj)->IsAlive() && (obj)->GetSide() == m_Id))

    CMatrixMapStatic* obj;
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no)
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;
            CInfo* env = robot->GetEnv();

            if(env->m_Target && env->m_Target != skip && IsPassive(env->m_Target)) env->m_Target = nullptr;
            CEnemy* enemy = env->m_FirstEnemy;
            while(enemy)
            {
                CEnemy* e2 = enemy;
                enemy = enemy->m_NextEnemy;
                if(e2->GetEnemy() != skip && IsPassive(e2->GetEnemy())) env->RemoveFromList(e2);
            }
        }
        obj = obj->GetNextLogic();
    }
#undef IsPassive
}

void CMatrixSideUnit::PGSetPlace(CMatrixRobotAI* robot, const CPoint& p)
{
    int x, y, u;
    CRect plr = g_MatrixMap->m_RoadNetwork.CorrectRectPL(CRect(p.x - 4, p.y - 4, p.x + 4, p.y + 4));

    SMatrixPlaceList* plist = g_MatrixMap->m_RoadNetwork.m_PLList + plr.left + plr.top * g_MatrixMap->m_RoadNetwork.m_PLSizeX;
    for(y = plr.top; y < plr.bottom; ++y, plist += g_MatrixMap->m_RoadNetwork.m_PLSizeX - (plr.right - plr.left))
    {
        for(x = plr.left; x < plr.right; ++x, ++plist)
        {
            SMatrixPlace* place = g_MatrixMap->m_RoadNetwork.m_Place + plist->m_Sme;
            for(u = 0; u < plist->m_Cnt; ++u, ++place)
            {
                if(!(p.x >= (place->m_Pos.x + 4) || (p.x + 4) <= place->m_Pos.x) && !(p.y >= (place->m_Pos.y + 4) || (p.y + 4) <= place->m_Pos.y))
                {
                    robot->GetEnv()->m_Place = plist->m_Sme + u;
                    robot->GetEnv()->m_PlaceAdd = CPoint(-1, -1);
                    return;
                }
            }
        }
    }
    robot->GetEnv()->m_Place = -1;
    robot->GetEnv()->m_PlaceAdd = p;
}

CPoint CMatrixSideUnit::PGCalcCenterGroup(int no)
{
    CMatrixMapStatic* obj;

    CPoint tp(0, 0);
    int cnt = 0;

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && (obj->AsRobot()->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no))
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;

            tp.x += robot->GetMapPosX();
            tp.y += robot->GetMapPosY();
            ++cnt;
        }
        obj = obj->GetNextLogic();
    }

    return CPoint(tp.x / cnt, tp.y / cnt);
}

void CMatrixSideUnit::PGPlaceClear(int no)
{
    CMatrixMapStatic* obj;

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && (obj->AsRobot()->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no))
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;

            robot->GetEnv()->m_Place = -1;
            robot->GetEnv()->m_PlaceAdd = CPoint(-1, -1);
        }
        obj = obj->GetNextLogic();
    }
}

CPoint CMatrixSideUnit::PGCalcPlaceCenter(int no)
{
    CMatrixMapStatic* obj;

    CPoint tp(0, 0);
    int cnt = 0;

    obj = CMatrixMapStatic::GetFirstLogic();
    while (obj)
    {
        if (obj->IsRobotAlive() && (obj->AsRobot()->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no))
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;

            CPoint tp2 = PLPlacePos(robot);
            if (tp2.x >= 0)
            {
                tp.x += tp2.x;
                tp.y += tp2.y;
                ++cnt;
            }
        }
        obj = obj->GetNextLogic();
    }
    if(cnt <= 0) return PGCalcCenterGroup(no);
    else return CPoint(tp.x / cnt, tp.y / cnt);
}

//Разово отрисовывает групповые вейпоинты (точки, где роботы группы будут расположены по достижении нужного места)
void CMatrixSideUnit::PGShowPlace(int no)
{
    CMatrixMapStatic* obj;
    CMatrixEffect::DeleteAllMoveto();

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && (obj->AsRobot()->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no))
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;
            CMatrixBuilding* cf = robot->GetCaptureBuilding();

            if(cf)
            {
                D3DXVECTOR2 v;
                v = GetWorldPos(cf);
                CMatrixEffect::CreateMoveToAnim(D3DXVECTOR3(v.x, v.y, g_MatrixMap->GetZ(v.x, v.y) + 2.0f), 1);
            }
            else
            {
                CPoint tp = PLPlacePos(robot);
                if(tp.x >= 0)
                {
                    D3DXVECTOR3 v;
                    v.x = GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                    v.y = GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
                    v.z = g_MatrixMap->GetZ(v.x, v.y);
                    CMatrixEffect::CreateMoveToAnim(v, 1);
                }
            }
        }
        obj = obj->GetNextLogic();
    }
    m_PlayerGroup[no].SetShowPlace(false);
}

void CMatrixSideUnit::PGAssignPlace(int no, CPoint& center)
{
    int i, u;
    byte mm = 0;
    CMatrixMapStatic*obj;
    SMatrixPlace *place;
    CMatrixRobotAI *rl[MAX_ROBOTS_ON_MAP];
    int ally_robots_cnt = 0;
    int listcnt = 0;

    BufPrepare();

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive())
        {
            if(obj->GetSide() == m_Id && GetGroupLogic(obj) == no)
            {
                rl[ally_robots_cnt] = (CMatrixRobotAI*)obj;
                mm |= 1 << (obj->AsRobot()->m_Module[0].m_Kind - 1);
                ++ally_robots_cnt;
            }
        }

        obj = obj->GetNextLogic();
    }

    if(ally_robots_cnt <= 0) return;


    int x = center.x, y = center.y;
    if(g_MatrixMap->PlaceFindNear(rl[0]->m_Module[0].m_Kind - 1, 4, x, y, 0, nullptr, nullptr))
    {
        center.x = x;
        center.y = y;
    }

    m_PlaceList[listcnt] = g_MatrixMap->FindNearPlace(mm, center);
    if(m_PlaceList[listcnt] < 0)
    {
        for(i = 0; i < ally_robots_cnt; ++i) rl[i]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();
        return;
    }

    ++listcnt;
    if(ally_robots_cnt > 1 && g_MatrixMap->PlaceListGrow(mm, m_PlaceList, &listcnt, ally_robots_cnt - 1) <= 0)
    {
        for(i = 0; i < ally_robots_cnt; ++i) rl[i]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();
        return;
    }

    for(i = 0; i < listcnt; ++i) g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[i]].m_Data = 0;
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsUnitAlive()) ObjPlaceData(obj, 1);
        obj = obj->GetNextLogic();
    }

    for(i = 0; i < ally_robots_cnt; ++i)
    {
        for(u = 0; u < listcnt; ++u)
        {
            place = g_MatrixMap->m_RoadNetwork.m_Place + m_PlaceList[u];
            if(place->m_Data) continue;
            if(!CanMove(place->m_Move, rl[i])) continue;
            break;
        }
        if(u < listcnt)
        {
            place->m_Data = 1;
            rl[i]->GetEnv()->m_Place = m_PlaceList[u];
            rl[i]->GetEnv()->m_PlaceAdd = CPoint(-1, -1);
        }
        else
        {
            if(g_MatrixMap->PlaceListGrow(mm, m_PlaceList, &listcnt, ally_robots_cnt) <= 0)
            {
                for(; i < ally_robots_cnt; ++i) rl[i]->GetEnv()->m_PlaceNotFound = g_MatrixMap->GetTime();
                return;
            }
            for(u = 0; u < listcnt; ++u) g_MatrixMap->m_RoadNetwork.m_Place[m_PlaceList[u]].m_Data = 0;
            obj = CMatrixMapStatic::GetFirstLogic();
            while(obj)
            {
                if(obj->IsUnitAlive()) ObjPlaceData(obj, 1);
                obj = obj->GetNextLogic();
            }
            ++i;
            continue;
        }
    }
}

void CMatrixSideUnit::PGAssignPlacePlayer(int no, const CPoint& center)
{
    int other_cnt = 0;
    int other_size[200];
    CPoint other_des[200];

    CMatrixMapStatic* obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive())
        {
            CMatrixRobotAI* r = (CMatrixRobotAI*)obj;
            if(r->GetSide() != m_Id || r->GetGroupLogic() != no)
            {
                if(r->GetEnv()->m_Place >= 0)
                {
                    ASSERT(other_cnt < 200);

                    other_size[other_cnt] = 4;
                    other_des[other_cnt] = g_MatrixMap->m_RoadNetwork.GetPlace(r->GetEnv()->m_Place)->m_Pos;
                    ++other_cnt;
                }
                else if(r->GetEnv()->m_PlaceAdd.x >= 0)
                {
                    ASSERT(other_cnt < 200);

                    other_size[other_cnt] = 4;
                    other_des[other_cnt] = r->GetEnv()->m_PlaceAdd;
                    ++other_cnt;
                }
            }
        }
        else if(obj->IsTurretAlive())
        {
            ASSERT(other_cnt < 200);

            other_size[other_cnt] = 4;
            other_des[other_cnt] = g_MatrixMap->m_RoadNetwork.GetPlace(obj->AsTurret()->m_Place)->m_Pos;
            ++other_cnt;
        }
        obj = obj->GetNextLogic();
    }

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && obj->GetSide() == m_Id && GetGroupLogic(obj) == no)
        {
            CMatrixRobotAI* r = (CMatrixRobotAI*)obj;
            int mx = center.x;
            int my = center.y;
            g_MatrixMap->PlaceFindNear(r->m_Module[0].m_Kind - 1, 4, mx, my, other_cnt, other_size, other_des);
            PGSetPlace(r, CPoint(mx, my));

            if(r->GetEnv()->m_Place >= 0)
            {
                ASSERT(other_cnt < 200);

                other_size[other_cnt] = 4;
                other_des[other_cnt] = g_MatrixMap->m_RoadNetwork.GetPlace(r->GetEnv()->m_Place)->m_Pos;
                ++other_cnt;
            }
            else if(r->GetEnv()->m_PlaceAdd.x >= 0)
            {
                ASSERT(other_cnt < 200);

                other_size[other_cnt] = 4;
                other_des[other_cnt] = r->GetEnv()->m_PlaceAdd;
                ++other_cnt;
            }

            //obj->AsRobot()->MoveTo(mx, my);
            //obj->AsRobot()->GetEnv()->m_Place = -1;
        }

        obj = obj->GetNextLogic();
    }
}

void CMatrixSideUnit::PGCalcStat()
{
    CMatrixMapStatic* ms;
    int i, u, t; //cnt,sme,dist;
    CPoint tp;

    if(m_Region == nullptr) m_Region = (SMatrixLogicRegion*)HAllocClear(sizeof(SMatrixLogicRegion) * g_MatrixMap->m_RoadNetwork.m_RegionCnt, Base::g_MatrixHeap);
    if(m_RegionIndex == nullptr) m_RegionIndex = (int*)HAllocClear(sizeof(int) * g_MatrixMap->m_RoadNetwork.m_RegionCnt, Base::g_MatrixHeap);

    for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i)
    {
        m_Region[i].m_EnemyRobotCnt = 0;
        m_Region[i].m_EnemyCannonCnt = 0;
        m_Region[i].m_EnemyBuildingCnt = 0;
        m_Region[i].m_EnemyBaseCnt = 0;
        m_Region[i].m_NeutralCannonCnt = 0;
        m_Region[i].m_NeutralBuildingCnt = 0;
        m_Region[i].m_NeutralBaseCnt = 0;
        m_Region[i].m_OurRobotCnt = 0;
        m_Region[i].m_OurCannonCnt = 0;
        m_Region[i].m_OurBuildingCnt = 0;
        m_Region[i].m_OurBaseCnt = 0;
        m_Region[i].m_EnemyRobotDist = -1;
        m_Region[i].m_EnemyBuildingDist = -1;
        m_Region[i].m_OurBaseDist = -1;

        m_Region[i].m_Danger = 0.0f;
        m_Region[i].m_DangerAdd = 0.0f;

        m_Region[i].m_EnemyForces = 0.0f;
        m_Region[i].m_OurForces = 0.0f;
        m_Region[i].m_EnemyForcesAdd = 0.0f;
        m_Region[i].m_OurForcesAdd = 0.0f;
    }

    ms = CMatrixMapStatic::GetFirstLogic();
    while(ms)
    {
        switch(ms->GetObjectType())
        {
            case OBJECT_TYPE_BUILDING:
            {
                CMatrixBuilding* cur_building = ms->AsBuilding();

                i = GetRegion(
                    int(cur_building->m_Pos.x / GLOBAL_SCALE_MOVE),
                    int(cur_building->m_Pos.y / GLOBAL_SCALE_MOVE)
                );

                if(i >= 0)
                {
                    if(cur_building->m_Side == NEUTRAL_SIDE) ++m_Region[i].m_NeutralBuildingCnt;
                    else if(cur_building->m_Side != m_Id) ++m_Region[i].m_EnemyBuildingCnt;
                    else ++m_Region[i].m_OurBuildingCnt;

                    if(cur_building->m_Kind == BUILDING_BASE)
                    {
                        if(cur_building->m_Side == NEUTRAL_SIDE) ++m_Region[i].m_NeutralBaseCnt;
                        else if(cur_building->m_Side != m_Id) ++m_Region[i].m_EnemyBaseCnt;
                        else ++m_Region[i].m_OurBaseCnt;
                    }
                }

                break;
            }
            case OBJECT_TYPE_ROBOT_AI:
            {
                CMatrixRobotAI* cur_robot = ms->AsRobot();

                if(cur_robot->m_CurrentState != ROBOT_DIP)
                {
                    tp.x = int(cur_robot->m_PosX / GLOBAL_SCALE_MOVE);
                    tp.y = int(cur_robot->m_PosY / GLOBAL_SCALE_MOVE);
                    i = GetRegion(tp);

                    if(i >= 0)
                    {
                        if(cur_robot->m_Side == NEUTRAL_SIDE);
                        else if(cur_robot->m_Side != m_Id)
                        {
                            ++m_Region[i].m_EnemyRobotCnt;
                            float strength = cur_robot->GetStrength();
                            m_Region[i].m_Danger += strength;
                            m_Region[i].m_EnemyForces += strength;
                        }
                        else
                        {
                            ++m_Region[i].m_OurRobotCnt;
                            m_Region[i].m_OurForces += cur_robot->GetStrength();
                        }
                    }
                }

                break;
            }
            // ;(
            /*
            case OBJECT_TYPE_FLYER:
            {
                CMatrixFlyer* cur_flyer = ms->AsFlyer();
            }
            */
            case OBJECT_TYPE_TURRET:
            {
                CMatrixTurret* cur_cannon = ms->AsTurret();

                tp.x = int(cur_cannon->m_Pos.x / GLOBAL_SCALE_MOVE);
                tp.y = int(cur_cannon->m_Pos.y / GLOBAL_SCALE_MOVE);
                i = GetRegion(tp);
                if(i >= 0)
                {
                    if(cur_cannon->GetSide() == NEUTRAL_SIDE)
                    {
                        ++m_Region[i].m_NeutralCannonCnt;
                        m_Region[i].m_DangerAdd += cur_cannon->GetStrength();
                    }
                    else if(cur_cannon->GetSide() != m_Id)
                    {
                        ++m_Region[i].m_EnemyCannonCnt;
                        float strength = cur_cannon->GetStrength();
                        m_Region[i].m_DangerAdd += strength;
                        //m_Region[i].m_EnemyForces += strength;
                        m_Region[i].m_EnemyForcesAdd += strength;
                    }
                    else
                    {
                        ++m_Region[i].m_OurCannonCnt;
                        //m_Region[i].m_OurForces += cur_cannon->GetStrength();
                        m_Region[i].m_OurForcesAdd += cur_cannon->GetStrength();
                    }
                }

                break;
            }
        }

        ms = ms->GetNextLogic();
    }

    // Выращиваем опасность
    for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i)
    {
        if(m_Region[i].m_Danger <= 0.0f) continue;

        SMatrixLogicRegion* lr = m_Region;
        for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u, ++lr) lr->m_Data = 0;

        for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_Region[i].m_NearCnt; ++u)
        {
            t = g_MatrixMap->m_RoadNetwork.m_Region[i].m_Near[u];
            if(m_Region[t].m_Data > 0) continue;

            m_Region[t].m_DangerAdd += m_Region[i].m_Danger;
            m_Region[t].m_EnemyForcesAdd += m_Region[i].m_EnemyForces;
            m_Region[t].m_OurForcesAdd += m_Region[i].m_OurForces;
            m_Region[t].m_Data = 1;
        }

        /*
        cnt = 0; sme = 0; dist = 0;
        m_RegionIndex[cnt] = i;
        m_Region[i].m_Data = 3;
        ++cnt;

        while(sme < cnt)
        {
            for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++u)
            {
                t = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[u];
                if(m_Region[t].m_Data > 0) continue;

                m_Region[t].m_DangerAdd += m_Region[i].m_Danger;

                if(m_Region[t].m_Danger > 0)
                {
                    m_RegionIndex[cnt] = t;
                    m_Region[t].m_Data = 3;
                    ++cnt;
                }
                else
                {
                    m_Region[t].m_Data=m_Region[m_RegionIndex[sme]].m_Data - 1;
                    if(m_Region[t].m_Data >= 2)
                    {
                        m_RegionIndex[cnt] = t;
                        ++cnt;
                    }
                }
            }
            ++sme;
        }
        */
    }

    for(i = 0; i < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++i)
    {
        m_Region[i].m_Danger += m_Region[i].m_DangerAdd;
        m_Region[i].m_EnemyForces += m_Region[i].m_EnemyForcesAdd;
        m_Region[i].m_OurForces += m_Region[i].m_OurForcesAdd;
    }
}

void CMatrixSideUnit::PGFindCaptureBuilding(int no)
{
    PGCalcStat();

    int u, t, k, cnt, sme, dist, next;
    CMatrixMapStatic* obj;
    byte mm = 0;

    cnt = 0;
    sme = 0;
    dist = 0;

    int region_mass = g_MatrixMap->GetRegion(PGCalcCenterGroup(no));
    float strength = 0;

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && (obj->AsRobot()->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no))
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;

            mm |= 1 << (robot->m_Module[0].m_Kind - 1);

            //robot->CalcStrength();
            strength += robot->GetStrength();
            if(region_mass < 0) region_mass = g_MatrixMap->GetRegion(GetMapPos(robot));
        }

        obj = obj->GetNextLogic();
    }

    if(region_mass < 0) return;

    //В текущем регионе
    //Если регион слишком опасный, то пропускаем
    if(strength >= m_Region[region_mass].m_Danger * 1.0)
    {
        obj = CMatrixMapStatic::GetFirstLogic();
        while(obj)
        {
            if(obj->IsBuildingAlive() && (obj->GetSide() != m_Id) && GetRegion(obj) == region_mass)
            {
                m_PlayerGroup[no].m_Obj = obj;

                m_PlayerGroup[no].m_RegionPathCnt = 0;
                m_PlayerGroup[no].m_RoadPath->ClearFast();

                return;
            }

            obj = obj->GetNextLogic();
        }
    }

    //В других регионах (безопасных/опасных)
    for(int type = 0; type <= 1; ++type)
    {
        for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u)
        {
            m_Region[u].m_Data = 0;
        }

        cnt = 0; sme = 0;
        m_RegionIndex[cnt] = region_mass;
        m_Region[region_mass].m_Data = 1;
        ++cnt;

        next = cnt;
        dist = 1;
        while(sme < cnt)
        {
            for(t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++t)
            {
                u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[t];
                if(m_Region[u].m_Data) continue;

                if(g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearMove[t] & mm) continue;

                if(type == 0 && strength < m_Region[u].m_Danger * 1.0) continue; //Если регион слишком опасный, то пропускаем

                m_Region[u].m_Data = 1 + dist;
                m_RegionIndex[cnt] = u;
                ++cnt;

                //Группы не должны идти в один регион
                for(k = 0; k < MAX_LOGIC_GROUP; ++k)
                {
                    if(k == no) continue;
                    if(m_PlayerGroup[k].m_RobotCnt <= 0) continue;
                    if(m_PlayerGroup[k].Order() != mpo_Capture && m_PlayerGroup[k].Order() != mpo_AutoCapture) continue;
                    if(!m_PlayerGroup[k].m_Obj) continue;
                    if(!m_PlayerGroup[k].m_Obj->IsBuildingAlive()) continue;

                    if(u != GetRegion(m_PlayerGroup[k].m_Obj)) continue;
                    break;

                }

                if(k < MAX_LOGIC_GROUP) continue;

                if(m_Region[u].m_EnemyBuildingCnt > 0 || m_Region[u].m_NeutralBuildingCnt > 0)
                {
                    obj = CMatrixMapStatic::GetFirstLogic();
                    while(obj)
                    {
                        if(obj->IsBuildingAlive() && (obj->GetSide() != m_Id) && GetRegion(obj) == u)
                        {
                            m_PlayerGroup[no].m_Obj = obj;
                            PGCalcRegionPath(m_PlayerGroup + no, u, mm);

                            m_PlayerGroup[no].m_RoadPath->ClearFast();
                            if(m_PlayerGroup[no].m_RegionPathCnt > 1)
                            {
                                g_MatrixMap->m_RoadNetwork.FindPathFromRegionPath(mm, m_PlayerGroup[no].m_RegionPathCnt, m_PlayerGroup[no].m_RegionPath, m_PlayerGroup[no].m_RoadPath);
                            }

                            return;
                        }

                        obj = obj->GetNextLogic();
                    }
                }
            }

            ++sme;

            if(sme >= next)
            {
                next = cnt;
                ++dist;
            }
        }
    }
}

void CMatrixSideUnit::PGFindAttackTarget(int no)
{
    PGCalcStat();

    int u, t, k, cnt, sme, dist, next;
    CMatrixMapStatic* obj;
    byte mm = 0;

    cnt = 0; sme = 0; dist = 0;

    int regionmass = g_MatrixMap->GetRegion(PGCalcCenterGroup(no));
    float strength = 0;

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && (obj->AsRobot()->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no))
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;

            mm |= 1 << (robot->m_Module[0].m_Kind - 1);

            //robot->CalcStrength();
            strength += robot->GetStrength();
            if(regionmass < 0) regionmass = g_MatrixMap->GetRegion(GetMapPos(robot));
        }
        obj = obj->GetNextLogic();
    }
    if(regionmass < 0) return;

    // В текущем регионе
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsUnitAlive() && (obj->GetSide() != m_Id) && GetRegion(obj) == regionmass)
        {
            m_PlayerGroup[no].m_Obj = obj;

            m_PlayerGroup[no].m_RegionPathCnt = 0;
            m_PlayerGroup[no].m_RoadPath->ClearFast();

            return;
        }
        obj = obj->GetNextLogic();
    }

    //В других регионах
    //(с учетом/без учета своих групп)
    for (int withourgroup = 0; withourgroup <= 1; ++withourgroup)
    {
        for (u = 0; u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u)
        {
            m_Region[u].m_Data = 0;
        }

        cnt = 0; sme = 0;
        m_RegionIndex[cnt] = regionmass;
        m_Region[regionmass].m_Data = 1;
        ++cnt;

        next = cnt;
        dist = 1;
        while (sme < cnt)
        {
            for (t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++t)
            {
                u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[t];
                if (m_Region[u].m_Data) continue;

                if (g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearMove[t] & mm) continue;

                m_Region[u].m_Data = 1 + dist;
                m_RegionIndex[cnt] = u;
                ++cnt;

                if (withourgroup == 0)
                {
                    // Группы не должны идти в один регион
                    for (k = 0; k < MAX_LOGIC_GROUP; ++k)
                    {
                        if (k == no) continue;
                        if (m_PlayerGroup[k].m_RobotCnt <= 0) continue;
                        if (m_PlayerGroup[k].Order() != mpo_Attack && m_PlayerGroup[k].Order() != mpo_AutoAttack) continue;
                        if (!m_PlayerGroup[k].m_Obj) continue;
                        if (!m_PlayerGroup[k].m_Obj->IsUnitAlive()) continue;

                        if (u != GetRegion(m_PlayerGroup[k].m_Obj)) continue;
                        break;
                    }
                    if (k < MAX_LOGIC_GROUP) continue;
                }

                if(m_Region[u].m_EnemyRobotCnt > 0 || m_Region[u].m_EnemyCannonCnt > 0 || m_Region[u].m_NeutralCannonCnt > 0)
                {

                    obj = CMatrixMapStatic::GetFirstLogic();
                    while(obj)
                    {
                        if(obj->IsUnitAlive() && (obj->GetSide() != m_Id) && GetRegion(obj) == u)
                        {
                            m_PlayerGroup[no].m_Obj = obj;
                            PGCalcRegionPath(m_PlayerGroup + no, u, mm);

                            m_PlayerGroup[no].m_RoadPath->ClearFast();
                            if(m_PlayerGroup[no].m_RegionPathCnt > 1)
                            {
                                g_MatrixMap->m_RoadNetwork.FindPathFromRegionPath(mm, m_PlayerGroup[no].m_RegionPathCnt, m_PlayerGroup[no].m_RegionPath, m_PlayerGroup[no].m_RoadPath);
                            }

                            return;
                        }

                        obj = obj->GetNextLogic();
                    }
                }
            }

            ++sme;

            if(sme >= next)
            {
                next = cnt;
                ++dist;
            }
        }
    }

    //Если нет врагов идем уничтожать вражескую базу
    for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u)
    {
        m_Region[u].m_Data = 0;
    }

    cnt = 0; sme = 0;
    m_RegionIndex[cnt] = regionmass;
    m_Region[regionmass].m_Data = 1;
    ++cnt;

    next = cnt;
    dist = 1;
    while(sme < cnt)
    {
        for(t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++t)
        {
            u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[t];
            if(m_Region[u].m_Data) continue;

            if(g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearMove[t] & mm) continue;

            m_Region[u].m_Data = 1 + dist;
            m_RegionIndex[cnt] = u;
            ++cnt;

            if(m_Region[u].m_EnemyBaseCnt > 0)
            {

                obj = CMatrixMapStatic::GetFirstLogic();
                while(obj)
                {
                    if(obj->IsBuildingAlive() && obj->IsBase() && (obj->GetSide() != m_Id) && GetRegion(obj) == u)
                    {
                        m_PlayerGroup[no].m_Obj = obj;
                        PGCalcRegionPath(m_PlayerGroup + no, u, mm);

                        m_PlayerGroup[no].m_RoadPath->ClearFast();
                        if(m_PlayerGroup[no].m_RegionPathCnt > 1)
                        {
                            g_MatrixMap->m_RoadNetwork.FindPathFromRegionPath(mm, m_PlayerGroup[no].m_RegionPathCnt, m_PlayerGroup[no].m_RegionPath, m_PlayerGroup[no].m_RoadPath);
                        }

                        return;
                    }

                    obj = obj->GetNextLogic();
                }
            }
        }

        ++sme;

        if(sme >= next)
        {
            next = cnt;
            ++dist;
        }
    }
}

void CMatrixSideUnit::PGFindDefenceTarget(int no)
{
    PGCalcStat();

    int u, t, k, cnt, sme, dist, next;
    CMatrixMapStatic* obj;
    byte mm = 0;

    cnt = 0; sme = 0; dist = 0;

    int region_mass = g_MatrixMap->GetRegion(PGCalcCenterGroup(no));
    float strength = 0;

    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsRobotAlive() && (obj->AsRobot()->GetSide() == m_Id && obj->AsRobot()->GetGroupLogic() == no))
        {
            CMatrixRobotAI* robot = (CMatrixRobotAI*)obj;

            mm |= 1 << (robot->m_Module[0].m_Kind - 1);

            //robot->CalcStrength();
            strength += robot->GetStrength();
            if(region_mass < 0) region_mass = g_MatrixMap->GetRegion(GetMapPos(robot));
        }
        obj = obj->GetNextLogic();
    }

    if(region_mass < 0) return;

    // В текущем регионе
    obj = CMatrixMapStatic::GetFirstLogic();
    while(obj)
    {
        if(obj->IsUnitAlive() && (obj->GetSide() != m_Id) && GetRegion(obj) == region_mass)
        {
            m_PlayerGroup[no].m_Obj = obj;
            m_PlayerGroup[no].m_Region = region_mass;

            m_PlayerGroup[no].m_RegionPathCnt = 0;
            m_PlayerGroup[no].m_RoadPath->ClearFast();

            return;
        }
        obj = obj->GetNextLogic();
    }

    //От наших баз/От текущего места
    //с учетом/без учета своих групп
    for(int type = 0; type <= 3; ++type)
    {
        cnt = 0; sme = 0;
        if(type <= 1)
        {
            for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u)
            {
                m_Region[u].m_Data = 0;

                if(m_Region[u].m_OurBaseCnt > 0)
                {
                    m_RegionIndex[cnt] = u;
                    m_Region[u].m_Data = 1;
                    ++cnt;
                }
            }
        }
        else
        {
            for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u) m_Region[u].m_Data = 0;

            m_RegionIndex[cnt] = region_mass;
            m_Region[region_mass].m_Data = 1;
            ++cnt;
        }

        next = cnt;
        dist = 1;
        while(sme < cnt)
        {
            for(t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++t)
            {
                u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[t];
                if(m_Region[u].m_Data) continue;

                if(g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearMove[t] & mm) continue;

                if(m_Region[u].m_EnemyBuildingCnt > 0) continue;
                if(m_Region[u].m_EnemyCannonCnt > 0) continue;

                m_Region[u].m_Data = 1 + dist;
                m_RegionIndex[cnt] = u;
                ++cnt;

                if(type == 0 || type == 2)
                {
                    // Группы не должны идти в один регион
                    for(k = 0; k < MAX_LOGIC_GROUP; ++k)
                    {
                        if(k == no) continue;
                        if(m_PlayerGroup[k].m_RobotCnt <= 0) continue;
                        if(m_PlayerGroup[k].Order() != mpo_Attack && m_PlayerGroup[k].Order() != mpo_AutoAttack) continue;
                        if(!m_PlayerGroup[k].m_Obj) continue;
                        if(!m_PlayerGroup[k].m_Obj->IsUnitAlive()) continue;

                        if(u != GetRegion(m_PlayerGroup[k].m_Obj)) continue;
                        break;

                    }

                    if(k < MAX_LOGIC_GROUP) continue;
                }

                if(m_Region[u].m_OurBuildingCnt > 0 || m_Region[u].m_OurCannonCnt > 0)
                {

                    obj = CMatrixMapStatic::GetFirstLogic();
                    while(obj)
                    {
                        if(obj->IsRobotAlive() && (obj->GetSide() != m_Id))
                        {
                            CPoint tp;
                            if(!obj->AsRobot()->GetMoveToCoords(tp)) tp = GetMapPos(obj);
                            int reg = g_MatrixMap->GetRegion(tp);
                            
                            if(reg >= 0 && reg == u /* && (GetRegion(obj) == reg || g_MatrixMap->m_RoadNetwork.IsNearestRegion(reg, GetRegion(obj))) */)
                            {
                                m_PlayerGroup[no].m_Region = reg;
                                m_PlayerGroup[no].m_Obj = obj;
                                PGCalcRegionPath(m_PlayerGroup + no, u, mm);

                                m_PlayerGroup[no].m_RoadPath->ClearFast();
                                if(m_PlayerGroup[no].m_RegionPathCnt > 1)
                                {
                                    g_MatrixMap->m_RoadNetwork.FindPathFromRegionPath(mm, m_PlayerGroup[no].m_RegionPathCnt, m_PlayerGroup[no].m_RegionPath, m_PlayerGroup[no].m_RoadPath);
                                }

                                return;
                            }
                        }

                        obj = obj->GetNextLogic();
                    }
                }
            }

            ++sme;

            if(sme >= next)
            {
                next = cnt;
                ++dist;
            }
        }
    }
}

void CMatrixSideUnit::PGCalcRegionPath(SMatrixPlayerGroup* pg, int rend, byte mm)
{
    int t, u, i;

    pg->m_RegionPathCnt = 0;
    pg->m_RegionPath[REGION_PATH_MAX_CNT - 1 - pg->m_RegionPathCnt] = rend;
    ++pg->m_RegionPathCnt;

    dword level = m_Region[rend].m_Data;

    while(true)
    {
        i = -1;
        for(t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[rend].m_NearCnt; ++t)
        {
            u = g_MatrixMap->m_RoadNetwork.m_Region[rend].m_Near[t];
            if(!m_Region[u].m_Data) continue;
            if(m_Region[u].m_Data >= level) continue;

            if(g_MatrixMap->m_RoadNetwork.m_Region[rend].m_NearMove[t] & mm) continue;

            i = u;
            break;
        }

        if(i < 0) ERROR_S(L"CMatrixSideUnit::PGCalcRegionPath!");

        ASSERT(pg->m_RegionPathCnt < REGION_PATH_MAX_CNT);

        pg->m_RegionPath[REGION_PATH_MAX_CNT - 1 - pg->m_RegionPathCnt] = i;
        pg->m_RegionPathCnt++;

        rend = i;
        level = m_Region[rend].m_Data;

        if(level <= 1) break;
    }

    if(pg->m_RegionPathCnt < REGION_PATH_MAX_CNT) MoveMemory(pg->m_RegionPath, pg->m_RegionPath + REGION_PATH_MAX_CNT - pg->m_RegionPathCnt, pg->m_RegionPathCnt * sizeof(int));

    //CHelper::DestroyByGroup(101);
    //for( i= 1; i < pg->m_RegionPathCnt; ++i)
    //{
    //    D3DXVECTOR3 v1, v2;
    //    v1.x = g_MatrixMap->m_RoadNetwork.m_Region[pg->m_RegionPath[i - 1]].m_Center.x * GLOBAL_SCALE_MOVE;
    //    v1.y = g_MatrixMap->m_RoadNetwork.m_Region[pg->m_RegionPath[i - 1]].m_Center.y * GLOBAL_SCALE_MOVE;
    //    v1.z = g_MatrixMap->GetZ(v1.x, v1.y) + 150.0f;
    //    v2.x = g_MatrixMap->m_RoadNetwork.m_Region[pg->m_RegionPath[i]].m_Center.x * GLOBAL_SCALE_MOVE;
    //    v2.y = g_MatrixMap->m_RoadNetwork.m_Region[pg->m_RegionPath[i]].m_Center.y * GLOBAL_SCALE_MOVE;
    //    v2.z = g_MatrixMap->GetZ(v2.x, v2.y) + 150.0f;
    //    CHelper::Create(0, 101)->Cone(v1, v2, 1.0f, 1.0f, 0xff00ff00, 0xff00ff00, 3);
    //}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline bool PrepareBreakOrder(CMatrixMapStatic* robot)
{
    return !(((CMatrixRobotAI*)robot)->GetEnv()->m_OrderNoBreak = !robot->AsRobot()->CanBreakOrder());
}

inline CPoint GetMapPos(CMatrixMapStatic* obj)
{
    switch(obj->GetObjectType())
    {
        case OBJECT_TYPE_ROBOT_AI: return CPoint(obj->AsRobot()->GetMapPosX(), obj->AsRobot()->GetMapPosY());
        case OBJECT_TYPE_FLYER: return CPoint((int)obj->AsFlyer()->GetMapPosX(), (int)obj->AsFlyer()->GetMapPosY());
        case OBJECT_TYPE_TURRET: return CPoint(int(obj->AsTurret()->m_Pos.x / GLOBAL_SCALE_MOVE), int(obj->AsTurret()->m_Pos.y / GLOBAL_SCALE_MOVE));
        case OBJECT_TYPE_BUILDING: return CPoint(int(obj->AsBuilding()->m_Pos.x / GLOBAL_SCALE_MOVE), int(obj->AsBuilding()->m_Pos.y / GLOBAL_SCALE_MOVE));
    }

    if(obj->GetObjectType() == OBJECT_TYPE_MAPOBJECT && obj->IsSpecial())
    {
        return CPoint(int(((CMatrixMapObject*)obj)->GetGeoCenter().x / GLOBAL_SCALE_MOVE), int(obj->AsBuilding()->GetGeoCenter().y / GLOBAL_SCALE_MOVE));
    }

    ERROR_S(L"GetMapPos Error!");
}

inline D3DXVECTOR2 GetWorldPos(CMatrixMapStatic* obj)
{
    switch(obj->GetObjectType())
    {
        case OBJECT_TYPE_ROBOT_AI: return D3DXVECTOR2(obj->AsRobot()->m_PosX, obj->AsRobot()->m_PosY);
        case OBJECT_TYPE_FLYER: return D3DXVECTOR2(obj->AsFlyer()->m_Pos.x, obj->AsFlyer()->m_Pos.y);
        case OBJECT_TYPE_TURRET: return obj->AsTurret()->m_Pos;
        case OBJECT_TYPE_BUILDING: return obj->AsBuilding()->m_Pos;
        default: ERROR_S(L"GetWorldPos Error!");
    }
}

inline bool IsToPlace(CMatrixRobotAI* robot, int place)
{
    if(place < 0) return false;

    SMatrixPlace* pl = GetPlacePtr(place);

    CPoint tp;
    if(robot->GetMoveToCoords(tp))
    {
        if(robot->FindOrderLikeThat(ROT_CAPTURE_BUILDING)) return false;
        else if((pl->m_Pos.x == tp.x) && (pl->m_Pos.y == tp.y)) return true;
        else if(robot->GetReturnCoords(tp) && (pl->m_Pos.x == tp.x) && (pl->m_Pos.y == tp.y)) return true;
        else return false;
    }
    else
    {
        if(robot->GetReturnCoords(tp) && (pl->m_Pos.x == tp.x) && (pl->m_Pos.y == tp.y)) return true;

        return (robot->GetMapPosX() == pl->m_Pos.x) && (robot->GetMapPosY() == pl->m_Pos.y);
        //return fabs(robot->m_PosX-GLOBAL_SCALE_MOVE * (ROBOT_MOVECELLS_PER_SIZE >> 1) - GLOBAL_SCALE_MOVE * pl->m_Pos.x) < 0.9f && fabs(robot->m_PosY - GLOBAL_SCALE_MOVE * (ROBOT_MOVECELLS_PER_SIZE >> 1) - GLOBAL_SCALE_MOVE * pl->m_Pos.y) < 0.9f;
    }
}

inline bool IsInPlace(CMatrixRobotAI* robot, int place)
{
    SMatrixPlace* pl = GetPlacePtr(place);

    if(pl == nullptr) return false;

    if(robot->FindOrderLikeThat(ROT_MOVE_TO)) return false;

    return (robot->GetMapPosX() == pl->m_Pos.x) && (robot->GetMapPosY() == pl->m_Pos.y);
}

inline bool IsInPlace(CMatrixRobotAI* robot)
{
    return IsInPlace(robot, RobotPlace(robot));
}

inline int RobotPlace(CMatrixRobotAI* robot)
{
    return robot->GetEnv()->m_Place;
}

inline int TurretPlace(CMatrixTurret* turret)
{
    return turret->m_Place;
}

inline int FlyerPlace(CMatrixFlyer* flyer)
{
    return flyer->m_Place;
}

inline int ObjPlace(CMatrixMapStatic* obj)
{
    if(obj->IsRobot()) return RobotPlace(obj->AsRobot());
    else if(obj->IsFlyer()) return FlyerPlace(obj->AsFlyer());
    else if(obj->IsTurret()) return TurretPlace(obj->AsTurret());

    ERROR_S(L"ObjPlace Error! Object type is " + obj->GetObjectType());
}

inline SMatrixPlace* ObjPlacePtr(CMatrixMapStatic* obj)
{
    int i = ObjPlace(obj);
    if(i >= 0) return GetPlacePtr(i);
    else return nullptr;
}

inline dword ObjPlaceData(CMatrixMapStatic* obj)
{
    int i = ObjPlace(obj);
    if(i >= 0) return GetPlacePtr(i)->m_Data;
    else return 0;
}

inline void ObjPlaceData(CMatrixMapStatic* obj, dword data)
{
    int i = ObjPlace(obj);
    if(i >= 0) GetPlacePtr(i)->m_Data = data;
}

inline bool CanMove(byte move_type, CMatrixRobotAI* robot)
{
    return !(move_type & (1 << (robot->m_Module[0].m_Kind - 1)));
}

inline int GetDesRegion(CMatrixRobotAI* robot)
{
    SMatrixPlace* pl = ObjPlacePtr(robot);
    if(!pl) return -1;
    return pl->m_Region;

    /*
    CPoint tp;

    if(robot->GetMoveToCoords(tp)) return GetRegion(tp);
    else return GetRegion(robot->GetMapPosX(), robot->GetMapPosY());
    */
}

inline int GetRegion(const CPoint& tp)
{
    return g_MatrixMap->GetRegion(tp);
}

inline int GetRegion(int x, int y)
{
    return g_MatrixMap->GetRegion(x, y);
}

inline int GetRegion(CMatrixMapStatic* obj)
{
    return GetRegion(GetMapPos(obj));
}

inline D3DXVECTOR3 SetPointOfAim(CMatrixMapStatic* obj)
{
    D3DXVECTOR3 p;

    switch(obj->GetObjectType())
    {
        case OBJECT_TYPE_ROBOT_AI:
        {
            p = obj->GetGeoCenter();
            p.z += 5.0f;

            break;
        }
        case OBJECT_TYPE_FLYER:
        {
            p = obj->GetGeoCenter();
            p.z += 5.0f;

            break;
        }
        case OBJECT_TYPE_TURRET:
        {
            p = obj->GetGeoCenter();
            p.z += 5.0f;

            /*
            p.x = obj->AsTurret()->m_Pos.x;
            p.y = obj->AsTurret()->m_Pos.y;
            p.z = g_MatrixMap->GetZ(p.x, p.y) + 25.0f;
            */

            break;
        }
        case OBJECT_TYPE_BUILDING:
        {
            p = obj->GetGeoCenter();
            p.z = g_MatrixMap->GetZ(p.x, p.y) + 20.0f;

            break;
        }

        default: ASSERT(0);
    }

    return p;
}

inline bool PLIsToPlace(CMatrixRobotAI* robot)
{
    CPoint tp, ptp;

    if(robot->GetEnv()->m_Place >= 0)
    {
        ptp = GetPlacePtr(robot->GetEnv()->m_Place)->m_Pos;
    }
    else if(robot->GetEnv()->m_PlaceAdd.x >= 0)
    {
        ptp = robot->GetEnv()->m_PlaceAdd;
    }
    else return false;

    if(robot->GetMoveToCoords(tp))
    {
        if(robot->FindOrderLikeThat(ROT_CAPTURE_BUILDING)) return false;
        else if((ptp.x == tp.x) && (ptp.y == tp.y)) return true;
        else if(robot->GetReturnCoords(tp) && (ptp.x == tp.x) && (ptp.y == tp.y)) return true;
        else return false;

    }
    else
    {
        if(robot->GetReturnCoords(tp) && (ptp.x == tp.x) && (ptp.y == tp.y)) return true;

        return (robot->GetMapPosX() == ptp.x) && (robot->GetMapPosY() == ptp.y);
    }
}

inline CPoint PLPlacePos(CMatrixRobotAI* robot)
{
    if(robot->GetEnv()->m_Place >= 0)
    {
        return GetPlacePtr(robot->GetEnv()->m_Place)->m_Pos;
    }
    else if(robot->GetEnv()->m_PlaceAdd.x >= 0)
    {
        return robot->GetEnv()->m_PlaceAdd;
    }
    else return CPoint(-1, -1);
}

void CMatrixSideUnit::CalcRegionPath(SMatrixLogicAction* ac, int rend, byte mm)
{
    int t, u, i;

    ac->m_RegionPathCnt = 0;
    ac->m_RegionPath[REGION_PATH_MAX_CNT - 1 - ac->m_RegionPathCnt] = rend;
    ++ac->m_RegionPathCnt;

    dword level = m_Region[rend].m_Data;

    while(true)
    {
        i = -1;
        for(t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[rend].m_NearCnt; ++t)
        {
            u = g_MatrixMap->m_RoadNetwork.m_Region[rend].m_Near[t];
            if(!m_Region[u].m_Data) continue;
            if(m_Region[u].m_Data >= level) continue;

            if(g_MatrixMap->m_RoadNetwork.m_Region[rend].m_NearMove[t] & mm) continue;

            i = u;
            break;
        }

        if(i < 0) ERROR_S(L"CMatrixSideUnit::CalcRegionPath Error!");

        ASSERT(ac->m_RegionPathCnt < REGION_PATH_MAX_CNT);

        ac->m_RegionPath[REGION_PATH_MAX_CNT - 1 - ac->m_RegionPathCnt] = i;
        ac->m_RegionPathCnt++;

        rend = i;
        level = m_Region[rend].m_Data;

        if(level <= 1) break;
    }

    if(ac->m_RegionPathCnt < REGION_PATH_MAX_CNT) MoveMemory(ac->m_RegionPath, ac->m_RegionPath + REGION_PATH_MAX_CNT - ac->m_RegionPathCnt, ac->m_RegionPathCnt * sizeof(int));
}

bool CMatrixSideUnit::CanMoveNoEnemy(byte mm, int r1, int r2)
{
    int u, t, sme, cnt, dist, dist2, next;
    for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u) m_Region[u].m_Data = 0;

    sme = 0;
    cnt = 0;
    dist = 1;
    m_RegionIndex[cnt] = r1;
    m_Region[r1].m_Data = dist;
    ++cnt;
    ++dist;

    next = cnt;

    while(sme < cnt)
    {
        for(t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++t)
        {
            u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[t];
            if(m_Region[u].m_Data) continue;

            if(g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearMove[t] & mm) continue;

            if(u == r2) break;

            m_RegionIndex[cnt] = u;
            m_Region[u].m_Data = dist;
            ++cnt;
        }

        if(t < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt) break;

        ++sme;
        if(sme >= next)
        {
            next = cnt;
            ++dist;
        }
    }

    if(sme >= cnt) return false;

    for(u = 0; u < g_MatrixMap->m_RoadNetwork.m_RegionCnt; ++u) m_Region[u].m_Data = 0;

    sme = 0;
    cnt = 0;
    dist2 = 1;
    m_RegionIndex[cnt] = r1;
    m_Region[r1].m_Data = dist2;
    ++cnt;
    ++dist2;

    next = cnt;

    while(sme < cnt)
    {
        for(t = 0; t < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt; ++t)
        {
            u = g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_Near[t];
            if(m_Region[u].m_Data) continue;

            if(g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearMove[t] & mm) continue;

            if(u == r2) break;

            if(m_Region[u].m_EnemyRobotCnt > 0 || m_Region[u].m_EnemyCannonCnt > 0) continue;

            m_RegionIndex[cnt] = u;
            m_Region[u].m_Data = dist2;
            ++cnt;
        }

        if(t < g_MatrixMap->m_RoadNetwork.m_Region[m_RegionIndex[sme]].m_NearCnt) break;

        ++sme;
        if(sme >= next)
        {
            next = cnt;
            ++dist2;
        }
    }

    if(sme >= cnt) return false;

    return dist2 <= Float2Int(1.3f * dist);
}

void CMatrixSideUnit::DMTeam(int team, EMatrixLogicActionType ot, int state, const wchar* format, ...)
{
    const wchar_t* ots[] = { L"mlat_None", L"mlat_Defence", L"mlat_Attack", L"mlat_Forward", L"mlat_Retreat", L"mlat_Capture", L"mlat_Intercept" };
    const wchar_t* sstate;
    if(state < 0) sstate = L" Cancel";
    else if(state > 0) sstate = L" Accept";
    else sstate = L"";

    va_list marker;
    va_start(marker, format);
    //DM( CWStr().Format(L"Size.<i>.Team.<i>", m_Id, team).Get(),
    //    CWStr().Format(L"[<s><s>] <s>", ots[ot], sstate, CWStr().Format(format,marker).Get()).Get()
    //    );
}

void CMatrixSideUnit::DMSide(const wchar *format, ...)
{
    va_list marker;
    va_start(marker, format);
    //DM( CWStr().Format(L"Size.<i>", m_Id).Get(),
    //    CWStr().Format(format, marker).Get()
    //    );
}