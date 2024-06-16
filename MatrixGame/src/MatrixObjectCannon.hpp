// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

#include "MatrixProgressBar.hpp"
#include "Effects/MatrixEffect.hpp"
#include <vector>

class CMatrixBuilding;
class CMatrixRovotAI;

#define CANNON_FIRE_THINK_PERIOD    100
#define CANNON_NULL_TARGET_TIME     1000
#define CANNON_TIME_FROM_FIRE       1000

#define CANNON_COLLIDE_R            20
enum ECannonState
{
    CANNON_IDLE,
    CANNON_UNDER_CONSTRUCTION,
    CANNON_DIP
};

enum ECannonUnitType
{
    TURRET_PART_EMPTY,

    TURRET_PART_BASE,
    TURRET_PART_MOUNT,
    TURRET_PART_GUN
};

#define MR_MAX_TURRET_UNIT 3


struct SMatrixCannonUnit
{
	ECannonUnitType m_Type = TURRET_PART_EMPTY;

	CVectorObjectAnim* m_Graph = nullptr;
    D3DXMATRIX m_Matrix = {};
    SEffectHandler m_Smoke;

    union
    {
        struct
        {
	        CVOShadowStencil* m_ShadowStencil;
	        float       m_DirectionAngle; //Текущий угол направления модуля турели
            int         m_LinkMatrix;
            D3DXMATRIX  m_IMatrix; //Состоит из 16 float
	        dword       m_Invert;
        };
        struct
        {
            D3DXVECTOR3 m_Pos;
            D3DXVECTOR3 m_Velocity;
            float       m_TTL;
            float       m_dp;
            float       m_dr;
            float       m_dy;
        };
    };

    //Только для корректной инициализации переменных в union
    SMatrixCannonUnit() : m_ShadowStencil(nullptr), m_DirectionAngle(0.0f), m_LinkMatrix(0), m_IMatrix(), m_Invert(0) {}
};

class CMatrixCannon : public CMatrixMapStatic
{
    static void FireHandler(CMatrixMapStatic* hit, const D3DXVECTOR3& pos, dword user, dword flags);

protected:
    // hitpoint
    CMatrixProgressBar m_HealthBar;
    int       m_ShowHitpointsTime = 0;
    float     m_Hitpoints = 0.0f;      //Текущее количество здоровья
	float     m_MaxHitpoints = 0.0f;   //Максимальное количество здоровья
    union
    {
        float m_MaxHitpointsInversed = 0.0f; // for normalized calcs
        float m_TargetAngle;// = 0.0f;
    };

    int m_UnderAttackTime = 0;

private:
    int m_Side = NEUTRAL_SIDE;	// 1-8

public:
	EShadowType m_ShadowType = SHADOW_OFF;
    int         m_ShadowSize = 128; // texture size for proj

	CMatrixBuilding* m_ParentBuilding = nullptr;
    D3DXVECTOR2 m_Pos = { 0.0f, 0.0f };

    int m_Place = -1; //Место, в котором установлена пушка (всегда должно быть инициализировано)

    int m_TurretKind = 0;

    float m_AddH = 0.0f;
    float m_Angle = 0.0f;
    float m_VerticalGuidanceAngle = 0.0f;

    int m_FireNextThinkTime = g_MatrixMap->GetTime() + CANNON_FIRE_THINK_PERIOD;
    int m_NullTargetTime = 0;
    int m_TimeFromFire = CANNON_TIME_FROM_FIRE;

	int m_ModulesCount = 0;
	SMatrixCannonUnit m_Module[MR_MAX_TURRET_UNIT];

	CMatrixShadowProj* m_ShadowProj = nullptr;

	ECannonState m_CurrState = CANNON_IDLE;

    int m_NextTimeAblaze = 0;
    int m_NextTimeShorted = 0;

    D3DXVECTOR3 m_TargetDisp = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3 m_FireCenter = { 0.0f, 0.0f, 0.0f };

    struct STurretWeapon
    {
        CMatrixEffectWeapon* m_Weapon = nullptr;
        int m_TurretWeaponMatrixId = 0;
        int m_AsyncAwaitTimer = 0; //Текущий таймер задержки на данном орудии (пока не истечёт, стрелять оно может)

        D3DXVECTOR3 m_FireFrom = { 0.0f, 0.0f, 0.0f };
        D3DXVECTOR3 m_FireDir = { 0.0f, 0.0f, 0.0f };
    };
    std::vector<STurretWeapon> m_TurretWeapon;
    float m_TurretWeaponsTopRange = 0.0f;

    //Для асинхронной стрельбы (каждая пушка стреляет с задержкой после предыдущей)
    byte m_NextGunToShot = 0;       //Чья конкретно очередь (среди пушек) сейчас стрелять
    int  m_AsyncDelay = 0;          //Какая задержка записывается на каждое орудие после выстрела (если 0, то все орудия стреляют синхронно)
    bool m_IsAsyncCooldown = false; //Идёт ли в текущий момент ожидание кулдауна задержки - маркер используется, чтобы запускать обработчик по отсчёту и сбросу этого кулдауна
    int  m_EndFireAfterAsync = 0;   //Задержка после полного завершения стрельбы спустя которую m_NextGunToShot сбросится обратно на 0, то есть на первое в списке орудие

    SObjectCore* m_TargetCore = nullptr;

    int m_LastDelayDamageSide = 0;
    int m_MiniMapFlashTime = 0;

    void ReleaseMe();

    void BeginFireAnimation();
    void EndFireAnimation();

public:
    CMatrixCannon()
    {
        m_Core->m_Type = OBJECT_TYPE_CANNON;

        InitMaxHitpoint(8000);
        m_HealthBar.Modify(1000000, 0, PB_CANNON_WIDTH, 1);
        if(g_Config.m_BuildingShadows) m_ShadowType = SHADOW_STENCIL;
    }
	~CMatrixCannon();

    void DIPTact(float ms);

    void  ShowHitpoint()            { m_ShowHitpointsTime = HITPOINT_SHOW_TIME; }
    float GetHitPoint() const       { return m_Hitpoints; }
    float GetMaxHitPoint()          { return m_MaxHitpoints; }
    void  InitMaxHitpoint(float hp) { m_Hitpoints = hp; m_MaxHitpoints = hp; m_MaxHitpointsInversed = 1.0f / hp; }
    void  SetHitPoint(float hp)     { m_Hitpoints = hp; }
    float GetMaxHitPointInversed()  { return m_MaxHitpointsInversed; }
    float GetSeekRadius();
    float GetFireRadius()           { return m_TurretWeaponsTopRange; }

    bool IsRefProtect() const       { return FLAG(m_ObjectFlags, OBJECT_CANNON_REF_PROTECTION); }
    void SetRefProtectHit()         { SETFLAG(m_ObjectFlags, OBJECT_CANNON_REF_PROTECTION_HIT); }

    void SetPBOutOfScreen()         { m_HealthBar.Modify(100000.0f, 0); }

    void  SetMustBeAngle(float a)   { m_TargetAngle = a; }
    float GetMustBeAngle()          { return m_TargetAngle; }

    void SetSide(int id)            { m_Side = id; }

    void ModelInit(int num)          { m_TurretKind = num; RChange(MR_ShadowStencil | MR_ShadowProjGeom | MR_ShadowProjTex | MR_Graph); }

	void ModuleClear();

    void BoundGet(D3DXVECTOR3& bmin, D3DXVECTOR3& bmax);

    virtual bool TakingDamage(int weap, const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, int attacker_side = NEUTRAL_SIDE, CMatrixMapStatic* attaker = nullptr);
    virtual void GetResources(dword need);

    virtual void Tact(int cms);
    virtual void LogicTact(int cms);
    void PauseTact(int cms);

    virtual bool Pick(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt)  const;

	virtual void BeforeDraw();
	virtual void Draw();
	virtual void DrawShadowStencil();
	virtual void DrawShadowProj();

    virtual void FreeDynamicResources();

	void OnLoad();

    virtual bool CalcBounds(D3DXVECTOR3& omin, D3DXVECTOR3& omax);
    virtual int GetSide() const     { return m_Side; };
    virtual bool NeedRepair() const { return m_CurrState != CANNON_UNDER_CONSTRUCTION && (m_Hitpoints < m_MaxHitpoints); }
        
    virtual bool InRect(const CRect &rect)const;

    //void OnOutScreen() {};

    float GetStrength();
};

inline bool CMatrixMapStatic::IsCannonAlive() const
{
    return IsCannon() && ((CMatrixCannon*)this)->m_CurrState != CANNON_DIP; 
}

inline bool CMatrixMapStatic::IsActiveCannonAlive() const
{
    return IsCannon() && ((CMatrixCannon*)this)->m_CurrState != CANNON_DIP && ((CMatrixCannon*)this)->m_CurrState != CANNON_UNDER_CONSTRUCTION; 
}