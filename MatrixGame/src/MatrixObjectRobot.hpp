// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

#include "MatrixMap.hpp"
#include "MatrixConfig.hpp"
#include "MatrixProgressBar.hpp"
#include "Effects/MatrixEffect.hpp"


class CMatrixFlyer;

#define CARRYING_DISTANCE       20.0f
#define CARRYING_SPEED          0.996

#define KEELWATER_SPAWN_FACTOR  0.01f
#define DUST_SPAWN_FACTOR       0.007f

#define MR_MAX_MODULES		    9

struct SMatrixRobotModule;
struct SRobotWeapon;

struct SWeaponRepairData
{
    CSprite     m_b0;
    CSprite     m_b1;
    CSpriteSequence m_Sprites;
    D3DXVECTOR3 m_pos0 = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3 m_pos1 = { 0.0f, 0.0f, 0.0f };

    int m_DeviceStartMatrixId = 0;
    int m_DeviceEndMatrixId = 0;

    dword m_Flags = 0;
    static const dword CAN_BE_DRAWN = SETBIT(0);

    void Release();
    void Update(SMatrixRobotModule* unit);
    void Draw(bool now);

    static SWeaponRepairData* Allocate(ESpriteTextureSort sprite_spot = SPR_REPAIRER_SPOT, ESpriteTextureSort sprite_rect = SPR_REPAIRER_RECT, int dev_start_id = 2, int dev_end_id = 1);
};

struct SChassisData
{
    EMovingAnimType m_MovingAnimType = NORMAL_MOVING_ANIM;
    float m_MovingAnimSpeed = 1.0f;

    //Массив указателей на объекты с эффектами реактивных следов от шасси
    std::vector<CMatrixEffectFireStream*> JetStream;
    //Массив указателей на объекты с эффектами спрайтовых анимаций от шасси
    std::vector<CMatrixEffectLinkedSpriteAnim*> LinkedSpriteAnim;

    //Для шасси на воздушной подушке
    float m_DustCount = 0.0f;

    //Для шасси траков
    D3DXVECTOR3 m_LastSolePos = { 0.0f, 0.0f, 0.0f };

    //Для шасси пневматики
    D3DXVECTOR2 m_LinkPos = { 0.0f, 0.0f };
    int         m_LinkPrevFrame = 0;
};

struct SMatrixRobotModule
{
    ERobotModuleType   m_Type = MRT_EMPTY; // MRT_EMPTY - ничего, MRT_CHASSIS - шасси, MRT_WEAPON - оружие, MRT_HULL - корпус, MRT_HEAD - голова
    //Указатели на связанную подструктуру модуля
    //union {
    SRobotWeapon*      m_Weapon = nullptr; //Запоминается в CMatrixRobotAI::RobotWeaponInit()
    //};

	CVectorObjectAnim* m_Graph = nullptr;
    D3DXMATRIX         m_Matrix;

    SWeaponRepairData* m_WeaponRepairData = nullptr;
	CVOShadowStencil*  m_ShadowStencil = nullptr;
    ERobotModuleKind   m_Kind = RUK_EMPTY;
    float              m_NextAnimTime = 0.0f;
	float              m_Angle = 0.0f;
    int                m_LinkMatrix = 0;
    D3DXMATRIX         m_IMatrix;
	bool               m_Invert = false; //Маркер о необходимости инвертирования модели данного модуля (используется при установке оружия на левую сторону корпуса)
    byte               m_ConstructorPylonNum = 0; //Номер слота в корпусе, в котором установлен данный модуль (пока актуально только для оружия, нумерация с 0)

    bool               m_ReadyToExplode = false; //Если выставлен, значит, данный модуль является бомбой, и эта бомба уже готовиться к детонации

    D3DXVECTOR3        m_Pos = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3        m_Velocity = { 0.0f, 0.0f, 0.0f };
    float              m_TTL = 0.0f;
    float              m_dp = 0.0f;
    float              m_dr = 0.0f;
    float              m_dy = 0.0f;
            
    byte               m_SmokeEffect[sizeof(SEffectHandler)] = { 0 };

    SEffectHandler& Smoke() { return *(SEffectHandler*)&m_SmokeEffect; }
    void PrepareForDIP();
};

struct SWalkingChassisData
{
    D3DXVECTOR2 foot = { 0.0f, 0.0f };
    D3DXVECTOR2 other_foot = { 0.0f, 0.0f }; // if relink occurs, this contained new foot (relink coord)
    dword       newlink = 0;
};

class CMatrixRobot : public CMatrixMapStatic
{
protected:
    CMatrixBuilding* m_Base = nullptr; //база из который вышел робот
    //hitpoint
    CMatrixProgressBar m_HealthBar;
    int         m_ShowHitpointsTime = 0;
    float       m_Hitpoints = 0.0f;
    float       m_MaxHitpoints = 0.0f; // Максимальное кол-во здоровья
    float       m_MaxHitpointsInversed = 0.0f; // for normalized calcs

    EAnimation  m_Animation = ANIMATION_OFF;

    static SWalkingChassisData* m_WalkingChassis;

    bool        m_AutoBoom = false; //По умолчанию автоподрыв бомбы у робота всегда выключен

public:
	EShadowType m_ShadowType = SHADOW_STENCIL; // 0-off 1-proj 2-proj with anim 3-stencil
    int         m_ShadowSize = 128; // texture size for proj

    CWStr m_Name = CWStr(L"ROBOT");
    int m_defHitPoint = Float2Int(m_Hitpoints);

    float m_Speed = 0.0f;
    float m_PosX = 0.0f;
    float m_PosY = 0.0f;

	int m_Side = NEUTRAL_SIDE; // 1-8

    int m_CalcBoundsLastTime = -101;   // need for calculation of bound only 10 times per second
    D3DXVECTOR3 m_CalcBoundMin = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3 m_CalcBoundMax = { 0.0f, 0.0f, 0.0f };

	int m_ModulesCount = 0;
    SMatrixRobotModule m_Module[MR_MAX_MODULES];
    SMatrixRobotModule* m_HullModule = nullptr;

    SChassisData m_ChassisData;

    int m_TimeWithBase = 0;

	D3DXVECTOR3 m_ChassisForward = { 0.0f, 0.0f, 0.0f };    //Вектор текущего направления шасси робота (его текущее физическое положение)
    D3DXVECTOR3 m_ChassisCamForward = { 0.0f, 0.0f, 0.0f }; //Вектор условного направления шасси робота, к которому линкуется положение камеры в ручном контроле (используется для осуществления стрейфов)
	D3DXVECTOR3 m_HullForward = { 0.0f, 0.0f, 0.0f };       //Вектор текущего направления корпуса робота (его текущее физическое положение)

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
//Steering behavior's try
    D3DXVECTOR3 m_Velocity = { 0.0f, 0.0f, 0.0f }; //Вектор скорости, длина равна скорости, направление движения

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

    float          m_SpeedWaterCorr = 1.0f;     // коррекция скорости при движении по воде
    float          m_SpeedSlopeCorrDown = 1.0f; // коррекция скорости при движении с горы
    float          m_SpeedSlopeCorrUp = 1.0f;   // коррекция скорости при движении в гору

    float          m_MaxSpeed = 0.0f;		    // максимально развиваемая скорость
    float          m_MaxStrafeSpeed = 0.0f;		// максимально развиваемая скорость стрейфа (не должна быть выше m_MaxSpeed)
    float          m_MaxHullSpeed = 0.0f;       // скорость поворота корпуса
    float          m_MaxRotationSpeed = 0.0f;   // скорость вращения робота на месте

    CVOShadowProj* m_ShadowProj = nullptr;

	ERobotState    m_CurrentState = ROBOT_IN_SPAWN;

    float          m_FallingSpeed = 0.0f;

    CMatrixFlyer*  m_CargoFlyer = nullptr;
    float          m_KeelWaterCount = 0.0f;

    int m_MiniMapFlashTime = 0;

    CMatrixRobot()
    {
        m_Core->m_Type = OBJECT_TYPE_ROBOT_AI;

        m_HealthBar.Modify(1000000.0f, 0.0f, PB_ROBOT_WIDTH, 1.0f);

        if(g_PlayerRobotsAutoBoom == 1) m_AutoBoom = true; //Если подключён мод на автоподрыв (и в конфиге выставлено включение подрыва по умолчанию), то по умолчанию он будет включён
    }
    ~CMatrixRobot()
    {
        ModulesClear();
        if(m_ShadowProj)
        {
            HDelete(CVOShadowProj, m_ShadowProj, Base::g_MatrixHeap);
            m_ShadowProj = nullptr;
        }
    }

    //Каст на логический подкласс CMatrixRobot
    CMatrixRobotAI* AsMatrixRobotAI() { return reinterpret_cast<CMatrixRobotAI*>(this); }

    void ShowHitpoints() { m_ShowHitpointsTime = HITPOINT_SHOW_TIME; }
    //Добавил функцию, чтобы иметь возможность чинить роботов напрямую из логики зданий, Klaxons
    void ModifyHitpoints(int num)
    {
        m_Hitpoints = max(min(m_Hitpoints + num, m_MaxHitpoints), 0);
        m_HealthBar.Modify(m_Hitpoints * m_MaxHitpointsInversed);
    }

    float GetHitpoints() const { return 0.1f * m_Hitpoints; }
    float GetMaxHitpoints() { return 0.1f * m_MaxHitpoints; }
    void  InitMaxHitpoints(float hp) { m_Hitpoints = hp; m_MaxHitpoints = hp; m_MaxHitpointsInversed = 1.0f / hp; }

    void MarkCrazy() { SETFLAG(m_ObjectFlags, ROBOT_CRAZY); }
    void UnMarkCrazy() { RESETFLAG(m_ObjectFlags, ROBOT_CRAZY); }
    bool IsCrazy() const { return FLAG(m_ObjectFlags, ROBOT_CRAZY); }

    //Каков тип двигательной части шасси: шагающий, колёсный, парящий (при желании типы можно (но не нужно) комбинировать, если, к примеру, нужно создать робота, шагающего по воде)
    bool IsWalkingChassis() { return g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].is_walking; }
    bool IsRollingChassis() { return g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].is_rolling; }
    bool IsHoveringChassis() { return g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].is_hovering; }

    void MarkInPosition() { SETFLAG(m_ObjectFlags, ROBOT_FLAG_IN_POSITION); }
    void UnMarkInPosition() { RESETFLAG(m_ObjectFlags, ROBOT_FLAG_IN_POSITION); }
    bool IsInPosition() const { return FLAG(m_ObjectFlags, ROBOT_FLAG_IN_POSITION); }

    bool IsMustDie() const { return FLAG(m_ObjectFlags, ROBOT_MUST_DIE_FLAG); }
    void MustDie() { SETFLAG(m_ObjectFlags, ROBOT_MUST_DIE_FLAG); }
    void ResetMustDie() { RESETFLAG(m_ObjectFlags, ROBOT_MUST_DIE_FLAG); }

    void MarkCaptureInformed() { SETFLAG(m_ObjectFlags, ROBOT_CAPTURE_INFORMED); }
    void UnMarkCaptureInformed() { RESETFLAG(m_ObjectFlags, ROBOT_CAPTURE_INFORMED); }
    bool IsCaptureInformed() const { return FLAG(m_ObjectFlags, ROBOT_CAPTURE_INFORMED); }


    void SetBase(CMatrixBuilding* b) { m_Base = b; m_TimeWithBase = 0; }
    CMatrixBuilding* GetBase() const { return m_Base; }

    EAnimation GetAnimation() { return m_Animation; }
    void SwitchAnimation(EAnimation a);

    bool Carry(CMatrixFlyer* cargo, bool quick_connect = false); // nullptr to off
    void ClearSelection();

    static void BuildWalkingChassisData(CVectorObject* vo, int chassis_num);
    static void DestroyWalkingChassisData();
    void StepLinkWalkingChassis();
    void FirstStepLinkWalkingChassis();

    float GetChassisHeight() const;
    float GetEyeHeight(bool from_the_floor = true) const;
    float GetHullHeight() const;

    float Z_From_Pos();

    void ApplyNaklon(const D3DXVECTOR3& dir);

	void ModuleInsert(int before_module, ERobotModuleType type, ERobotModuleKind kind);
	void WeaponInsert(int before_module, ERobotModuleType type, ERobotModuleKind kind, int hull_num, int model_pilon_num);
    void ModuleDelete(int module_num);
	void ModulesClear();

    void BoundGet(D3DXVECTOR3& bmin, D3DXVECTOR3& bmax);

    void DoAnimation(int cms);

    virtual bool TakingDamage(int weap, const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, int attacker_side = NEUTRAL_SIDE, CMatrixMapStatic* attaker = nullptr) = 0;
    virtual void GetResources(dword need);

	virtual void Tact(int cms);
    virtual void LogicTact(int cms) = 0;

    virtual bool Pick(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt)  const;
    bool         PickFull(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt)  const;

	virtual void BeforeDraw();
	virtual void Draw();
	virtual void DrawShadowStencil();
	virtual void DrawShadowProj();

    virtual void FreeDynamicResources();

    void OnLoad() {};

    virtual bool CalcBounds(D3DXVECTOR3& omin, D3DXVECTOR3& omax);
    virtual int  GetSide() const { return m_Side; };
    virtual float NeedRepair() const { return m_MaxHitpoints - m_Hitpoints; }
    virtual float GetHitpointsPercent() const { return m_Hitpoints / m_MaxHitpoints * 100.0f; }
    virtual bool InRect(const CRect& rect)const;

    void OnOutScreen() {};

    bool AutoBoomSet()         { return m_AutoBoom; }
    void AutoBoomSet(bool set) { m_AutoBoom = set; }
};

inline bool CMatrixMapStatic::IsRobotAlive() const
{
    return IsRobot() && ((CMatrixRobot*)this)->m_CurrentState != ROBOT_DIP;
}