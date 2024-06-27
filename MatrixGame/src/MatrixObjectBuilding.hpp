// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

#include "MatrixMap.hpp"
#include "MatrixProgressBar.hpp"
#include "MatrixSoundManager.hpp"

#define BASE_FLOOR_Z         (-63.0f) //Высота подъёма готового юнита из недр базы (на визуальную глубину залегания подъемника не влияет)
#define BASE_FLOOR_SPEED     0.0008f  //Скорость воспроизведения подъёма лифта и юнита на нём (так можно ускорить спавн)
#define MAX_CAPTURE_CIRCLES  14       //Максимальное число кругов захвата на площадках заводов

#define BUILDING_EXPLOSION_PERIOD_SND_1  100
#define BUILDING_EXPLOSION_PERIOD_SND_2  500
#define BUILDING_EXPLOSION_PERIOD        10
#define BUILDING_EXPLOSION_TIME          1000
#define BUILDING_BASE_EXPLOSION_TIME     2000

#define RESOURCES_INCOME_FROM_FACTORY    10
#define RESOURCES_INCOME_FROM_BASE       3

#define GATHERING_POINT_SHOW_TIME  1650

#define BUILDING_SELECTION_SIZE    50.0f
#define TURRET_SELECTION_SIZE      27.5f

#define CAPTURE_RADIUS             50
#define CAPTURE_SEEK_ROBOT_PERIOD  500

#define DISTANCE_CAPTURE_ME        300

#define MAX_PLACES                 4

//#define RES_TITAN_PERIOD       10000
//#define RES_PLASMA_PERIOD      5000
//#define RES_ENERGY_PERIOD      15000
//#define RES_ELECTRO_PERIOD     8500
//
//#define ROBOT_BUILD_TIME       5000
//#define FLYER_BUILD_TIME       5000
//#define TURRET_BUILD_TIME      5000

#define BUILDING_QUEUE_LIMIT        6

enum EFlyerKind;

enum EBuildingTurrets
{
    CONSTRUCTION_BASE_TURRETS_LIMIT = 4,
    TITAN_FACTORY_TURRETS_LIMIT = 4,
    ELECTRONIC_FACTORY_TURRETS_LIMIT = 4,
    ENERGY_FACTORY_TURRETS_LIMIT = 4,
    PLASMA_FACTORY_TURRETS_LIMIT = 4,
    REPAIR_FACTORY_TURRETS_LIMIT = 4
};

enum EBaseState
{
	BASE_CLOSED,
	BASE_OPENING,
    BASE_OPENED,
	BASE_CLOSING,

    BUILDING_DIP,
    BUILDING_DIP_EXPLODED
};

enum ECaptureStatus
{
    CAPTURE_DONE,        // захват данным цветом завершен
    CAPTURE_IN_PROGRESS, // в процессе
    CAPTURE_TOO_FAR,     // робот далеко. подъедь ближе
    CAPTURE_BUSY,        // база занята делами. обратитесь позже
};

struct STrueColor
{
    int m_ColoredCnt = 0;
    dword m_Color = 0;

    STrueColor()
    {
        m_ColoredCnt = 0;
        m_Color = 0;
    }
};

struct SResource
{
    EBuildingType m_Type = BUILDING_BASE;
    int           m_Amount = 0;
    //int         m_BaseRCycle = 0;
};

class CBuildingQueue : public CMain //Очередь строительства юнитов?
{
    int                m_Items = 0;
    int                m_Timer = 0;
    CMatrixMapStatic*  m_Top = nullptr;
    CMatrixMapStatic*  m_Bottom = nullptr;
    CMatrixBuilding*   m_ParentBase = nullptr;
    CMatrixProgressBar m_ProgressBar;

public:
    CBuildingQueue(CMatrixBuilding* base) : m_ParentBase(base) {}
    ~CBuildingQueue();

    void AddItem(CMatrixMapStatic* item);
    int DeleteItem(int no);
    void DeleteItem(CMatrixMapStatic* item);
    void ClearStack();
    CMatrixMapStatic* GetTopItem()              { return m_Top; }
    void TickTimer(int ms);                                          
    int GetItemsCnt() const                     { return m_Items; }

    //void SetParentBase(CMatrixBuilding* base) { m_ParentBase = base; }
    CMatrixBuilding* GetParentBase()            { return m_ParentBase; }
    bool IsMaxItems()                           { return m_Items >= BUILDING_QUEUE_LIMIT; }

    void ReturnRobotResources(CMatrixRobotAI* robot);
    void ReturnTurretResources(CMatrixTurret* turret);

    int GetRobotsCnt() const;
    void KillBar();
};

struct STurretPlace
{
    CPoint m_Coord = { 0, 0 };
    float  m_Angle = 0.0f;

    // dynamic members
    int    m_CannonType = 0;
};

struct SGatheringPoint
{
    bool   IsSet = false;

    int    CoordX = 0;
    int    CoordY = 0;

    CPoint Coords = { 0, 0 };
};

class CMatrixBuilding : public CMatrixMapStatic 
{
    union
    {
        struct //dip
        {
            int m_NextExplosionTime;// = 0;
            int m_NextExplosionTimeSound;// = 0;
        };
        struct 
        {
            int m_ResourcePeriod;// = 0;
            SEffectHandler* m_PlacesShow;// = nullptr;
        };
    };

    int m_LastDelayDamageSide = 0;
    int m_NextTimeAblaze = 0;

    CMatrixEffectSelection* m_Selection = nullptr;
    int m_UnderAttackTime = 0;
    int m_CaptureMeNextTime = 0;
    int m_CtrlGroup = 0;

    bool m_ConstantAnim = true; //Маркер о необходимости постоянной прокрутки фоновой анимации здания
    bool m_OccasionalAnimRun = false; //Если предполагается, что анимация здания должна прокручиваться однократно, и в строго определённые моменты (ремонтный завод), то активируем этот маркер

public:
    D3DXVECTOR3 m_TopPoint = { 0.0f, 0.0f, 0.0f };
    CWStr       m_Name = (CWStr)L"FACTORY"; //Хз нафига эта переменная
    int         m_defHitPoint = 0;
    CBuildingQueue m_BuildingQueue = this;

    //Точка сбора для базы
    SGatheringPoint m_GatheringPoint;
    int m_ShowGatheringPointTime = 0;

    D3DXVECTOR2 m_Pos = { 0.0f, 0.0f };
	int m_Angle = 0;

	int m_Side = NEUTRAL_SIDE; // 1-8

	EBuildingType    m_Kind = BUILDING_BASE; //По умолчанию любое здание считается сборочной базой
    EBuildingTurrets m_TurretsLimit = CONSTRUCTION_BASE_TURRETS_LIMIT;
    int              m_TurretsHave = 0;
    STurretPlace     m_TurretsPlaces[MAX_PLACES];
    int              m_TurretsPlacesCnt = 0;

    float m_BaseFloor = 0.2f;
    float m_BuildZ = 0.0f;

    CVectorObjectGroup* m_GGraph = nullptr;

    EBaseState m_State = BASE_CLOSING;

    CMatrixEffectCaptureCircles* m_CaptureCircles = nullptr;
    STrueColor m_TrueColor;

    int m_InCaptureTime = 0;
    union
    {
        int m_InCaptureNextTimeErase = 0;
        int m_CaptureNextTimeRollback;// = 0;
    };
    int               m_InCaptureNextTimePaint = 0;
    int               m_CaptureSeekRobotNextTime = 0;
    CMatrixMapStatic* m_Capturer = nullptr; // used only for check

    // hitpoint
    CMatrixProgressBar m_HealthBar;
    int         m_ShowHitpointsTime = 0;
    float       m_Hitpoints = 0.0f;
    float       m_MaxHitpoints = 0.0f;
    float       m_MaxHitpointsInversed = 0.0f; // for normalized calcs

    EShadowType m_ShadowType = SHADOW_OFF; // 0-off 1-proj 2-proj with anim 3-stencil
    int         m_ShadowSize = 128; // texture size for proj
    CMatrixShadowProj* m_ShadowProj = nullptr;

    //void SetResourceAmount(int amount)
    //{
    //    m_ResourceAmount = amount;
    //}

	CMatrixBuilding() : m_NextExplosionTime(0), m_NextExplosionTimeSound(0)//, m_ResourcePeriod(0), m_PlacesShow(nullptr)
    {
        m_Core->m_Type = OBJECT_TYPE_BUILDING;
        m_GGraph = HNew(Base::g_MatrixHeap) CVectorObjectGroup();
        if(g_Config.m_BuildingShadows) m_ShadowType = SHADOW_STENCIL;
    }
	~CMatrixBuilding();

    virtual bool IsUnitAlive() { return false; }

    int GetRobotsBuildingQueueCount() const { return m_BuildingQueue.GetRobotsCnt(); }

    bool GatheringPointIsSet() { return m_GatheringPoint.IsSet; }
    void SetGatheringPoint(int x, int y)
    {
        m_GatheringPoint.IsSet = true;
        //Чистые X и Y здесь нужны для расчёта точки отрисовки сборочного пункта
        m_GatheringPoint.CoordX = x;
        m_GatheringPoint.CoordY = y;
        //А в координаты построенным роботам передаётся уже вот это значение
        //Так запоминаются координаты для AssignPlace > GetRegion
        m_GatheringPoint.Coords = CPoint(int(x / GLOBAL_SCALE_MOVE), int(y / GLOBAL_SCALE_MOVE));
        //А так запоминаются координаты для PGOrderMoveTo
        //m_GatheringPoint.Coords = CPoint(x - ROBOT_MOVECELLS_PER_SIZE / 2, y - ROBOT_MOVECELLS_PER_SIZE / 2);
        m_ShowGatheringPointTime = g_MatrixMap->GetTime();
        //Заносим указатель на здание в массив для перебора и отрисовки всех установленных точек сбора
        g_MatrixMap->AddGatheringPoint(this);
    }
    CPoint GetGatheringPoint() { if(GatheringPointIsSet()) return m_GatheringPoint.Coords; return { 0, 0 }; }
    void ClearGatheringPoint()
    {
        //Убираем маркер наличия точки сбора у здания
        m_GatheringPoint.IsSet = false;
        //И удаляем указатель на это здание из массива, перебирающего все здания со сборными точками для их отрисовки
        g_MatrixMap->RemoveGatheringPoint(this);
    }
    void ShowGatheringPointTact(int cms);

    bool CreateCaptureCirclesEffect();

    bool HaveMaxTurrets() const { return m_TurretsHave >= (int)m_TurretsLimit; }

    bool CanBeCaptured() const  { return !FLAG(m_ObjectFlags, BUILDING_CAPTURE_IN_PROGRESS); }
    bool IsCaptured() const     { return FLAG(m_ObjectFlags, BUILDING_CAPTURE_IN_PROGRESS); }
    void SetCapturedBy(CMatrixMapStatic* ms)
    {
        SETFLAG(m_ObjectFlags, BUILDING_CAPTURE_IN_PROGRESS);
        m_Capturer = ms;
    }
    void ResetCaptured()
    {
        RESETFLAG(m_ObjectFlags, BUILDING_CAPTURE_IN_PROGRESS);
        m_Capturer = nullptr;
    }

    int  GetCtrlGroup()          { return m_CtrlGroup; }
    void SetCtrlGroup(int group) { m_CtrlGroup = group; }

    ECaptureStatus Capture(CMatrixRobotAI* by);

    //Вызов подкрепления
    void Reinforcements();

    //Постройка вертолёта
    bool BuildFlyer(EFlyerKind kind, int side = NEUTRAL_SIDE);
    
    //Открывается сборочная камера и поднимается подъёмник с готовым роботом/вертолётом
    void OpenAssemblyChamber()
    {
        if(m_State == BUILDING_DIP || m_State == BUILDING_DIP_EXPLODED) return;
        m_State = BASE_OPENING;
        CSound::AddSound(S_DOORS_OPEN, GetGeoCenter());
        CSound::AddSound(S_PLATFORM_UP, GetGeoCenter());
    }
    //Сборочная камера закрывается, подъёмник опускается
    void CloseAssemblyChamber()
    {
        if(m_State == BUILDING_DIP || m_State == BUILDING_DIP_EXPLODED) return;
        if(IsSpawningUnit()) return; // cannot close while spawn
        m_State = BASE_CLOSING;
        CSound::AddSound(S_DOORS_CLOSE, GetGeoCenter());
        CSound::AddSound(S_PLATFORM_DOWN, GetGeoCenter());
    }
    EBaseState State() const       { return m_State; }

    bool IsSpawningUnit() const    { return FLAG(m_ObjectFlags, BUILDING_SPAWNING_UNIT); };
    void SetSpawningUnit(bool set) { INITFLAG(m_ObjectFlags, BUILDING_SPAWNING_UNIT, set); };

    void ShowHitpoints()
    {
        m_ShowHitpointsTime = HITPOINT_SHOW_TIME;
    }
    void InitMaxHitpoints(float hp)
    {
        m_Hitpoints = hp;
        m_MaxHitpoints = hp;
        m_MaxHitpointsInversed = 1.0f / hp;
    }
    float GetMaxHitpoints() { return 0.1f * m_MaxHitpoints; }
    float GetHitpoints() { return 0.1f * m_Hitpoints; }
    void ReleaseMe();

    void SetNeutral();

    D3DXVECTOR3 GetPlacePos() const { return m_GGraph->GetPosByName(MATRIX_BASE_PLACE); }

	virtual void GetResources(dword need); //Загружаем/обновляем необходимые для данного объекта ресурсы

	virtual void Tact(int cms);

	virtual void LogicTact(int cms);
    void PauseTact(int cms);

    float GetFloorZ();
    int GetResourcePeriod() { return m_ResourcePeriod; };

    void CreateHealthBarClone(float x, float y, float width, EPBCoord clone_type);
    void DeleteHealthBarClone(EPBCoord clone_type);

    bool CreateSelection();
    void UnSelect();

    virtual bool Pick(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt) const;

    virtual bool TakingDamage(int weap, const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, int attacker_side = NEUTRAL_SIDE, CMatrixMapStatic* attaker = nullptr);
	virtual void BeforeDraw();
	virtual void Draw();
	virtual void DrawShadowStencil();
	virtual void DrawShadowProj();

    virtual void FreeDynamicResources();

	void OnLoad();

    virtual bool CalcBounds(D3DXVECTOR3& omin, D3DXVECTOR3& omax);

    virtual int  GetSide() const { return m_Side; };
    virtual float NeedRepair() const { return m_MaxHitpoints - m_Hitpoints; }
    virtual float GetHitpointsPercent() const { return m_Hitpoints / m_MaxHitpoints * 100.0f; }
    virtual bool InRect(const CRect& rect) const;

    void OnOutScreen();

    //STUB:
    int  GetPlacesForTurrets(CPoint* places);
    void CreatePlacesShow();
    void DeletePlacesShow();
};

inline bool CMatrixMapStatic::IsBase() const
{
    if(GetObjectType() == OBJECT_TYPE_BUILDING)
    {
        if(((CMatrixBuilding*)this)->m_Kind == BUILDING_BASE) return true;
    }
    return false;
}

inline bool CMatrixMapStatic::IsBuildingAlive(void) const
{
    return IsBuilding() && ((CMatrixBuilding*)this)->m_State!=BUILDING_DIP && ((CMatrixBuilding*)this)->m_State != BUILDING_DIP_EXPLODED;
}