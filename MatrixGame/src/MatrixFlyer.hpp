// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#ifndef MATRIX_FLYER_INCLUDE
#define MATRIX_FLYER_INCLUDE

#include "MatrixConfig.hpp"
#include "MatrixMapStatic.hpp"
#include "MatrixProgressBar.hpp"

class CMatrixEffectSelection;

#define FLYER_ALT_EMPTY     70
#define FLYER_ALT_CARRYING  110
#define FLYER_ALT_MIN       20

#define FLYER_MOUSE_DEAD_AREA_TOP      0.3f  // 0.2 of screen width
#define FLYER_MOUSE_DEAD_AREA_BOTTOM   0.05f // 0.0 of screen width

#define FLYER_SEEKBASE_MATCH_RADIUS    10
#define FLYER_TARGET_MATCH_RADIUS      50
#define FLYER_FIRETARGET_MATCH_RADIUS  20

#define FLYER_SPINUP_TIME              3000 //в оригинале 3000, время до полного раскручивания винта перед взлётом

//Эти параметры устарели, вместо них теперь массив по разным типам вертушек
//#define FLYER_MIN_SPEED 0.002f
#define FLYER_MAX_SPEED                0.2f
#define FLYER_MAX_BACK_SPEED           0.10f
#define FLYER_MAX_STRAFE_SPEED         0.12f

#define FLYER_MAX_FIRE_SPEED           0.8f
#define FLYER_MAX_CLIMB_SPEED          0.1f

#define FLYER_RADIUS                   15


#define ENGINE_ANGLE_BREAK  GRAD2RAD(179.9)
#define ENGINE_ANGLE_STAY   GRAD2RAD(90)
#define ENGINE_ANGLE_MOVE   GRAD2RAD(0)

//#define FLYER_WEAPON1_HFOV        GRAD2RAD(90)
//#define FLYER_WEAPON1_UP_ANGLE    GRAD2RAD(0)
//#define FLYER_WEAPON1_DOWN_ANGLE  GRAD2RAD(85)

#define FLYER_SELECTION_HEIGHT 2
#define FLYER_SELECTION_SIZE 20

class CMatrixSideUnit;
class CMatrixEffectWeapon;
class CMatrixRobot;
class CMatrixBuilding;
class CMatrixEffectElevatorField;
class CMatrixEffectFireStream;

enum EWeapon;

enum EFlyerUnitType
{
    FLYER_UNIT_BODY = 0,
    FLYER_UNIT_PROPELLER = 1,
    FLYER_UNIT_ENGINE = 2,
    FLYER_UNIT_WEAPON = 3,
    FLYER_UNIT_WEAPON_HOLLOW = 4
};

enum EFlyerKind
{
    FLYER_SPEED = 1,     //Разведчик
    FLYER_ATTACK = 2,    //Ударный
    FLYER_BOMB = 3,      //Бомбардировщик
    FLYER_TRANSPORT = 4  //Транспорт
};

struct SFlyerSpecifications
{
    float hitpoints = 5000.0f;

    float forward_speed = 0.0f;
    float reverse_speed = 0.0f;
    float strafe_speed = 0.0f;
    float climb_speed = 0.0f;       //Как вниз так и вверх

    //В радианах
    float pitch_angle_stay = 0.0f;  //Угол тангажа во время ожидания на месте
    float pitch_angle_move = 0.0f;  //Угол тангажа при движении вертолёта вперед/назад
    float pitch_angle_break = 0.0f; //Угол тангажа при торможении вертолёта
    float max_roll_angle = 0.0f;    //Максимально допустимый угол крена во время разворотов вертолёта
};

//Технические характеристики разных моделей вертолётов (костыли, чтобы не дублировать объект по всем юнитам кода)
extern const SFlyerSpecifications FlyerSpecifications[];
#ifdef FLYER_SPECIFICATIONS
const SFlyerSpecifications FlyerSpecifications[] =
{
    { 0.0f,    0.0f,  0.0f,  0.0f,  0.0f,  GRAD2RAD(0), GRAD2RAD(0),   GRAD2RAD(0),  GRAD2RAD(0)  }, //Это пропускаем
    { 1250.0f, 0.25f, 0.15f, 0.15f, 0.07f, GRAD2RAD(0), GRAD2RAD(-20), GRAD2RAD(70), GRAD2RAD(75) }, //Разведчик
    { 2500.0f, 0.20f, 0.10f, 0.12f, 0.05f, GRAD2RAD(0), GRAD2RAD(-18), GRAD2RAD(60), GRAD2RAD(64) }, //Ударный
    { 5000.0f, 0.17f, 0.08f, 0.10f, 0.04f, GRAD2RAD(0), GRAD2RAD(-17), GRAD2RAD(50), GRAD2RAD(50) }, //Бомбардировщик
    { 4000.0f, 0.15f, 0.07f, 0.08f, 0.03f, GRAD2RAD(0), GRAD2RAD(-17), GRAD2RAD(40), GRAD2RAD(40) }  //Транспорт
};
#endif

class CMatrixFlyer;

struct SMatrixFlyerUnit
{
    EFlyerUnitType      m_Type = FLYER_UNIT_BODY;
    CVectorObjectAnim*  m_Graph = nullptr;
    CVOShadowStencil*   m_ShadowStencil = nullptr;
    D3DXMATRIX          m_Matrix;
    D3DXMATRIX          m_IMatrix;

    union
    {
        struct
        {
            int               m_MatrixID; // matrix id
            D3DMATRIX         m_Matrix;
            CTextureManaged*  m_Tex;

            dword             m_Inversed;
	        float             m_Angle;
            float             m_AngleSpeedMax;

            int m_SpinupCountdown; //Если винт на старте сложен, то перед раскруткой необходимо будет его развернуть

        } m_Propeller;

        struct
        {
            CMatrixEffectWeapon* m_Weapon;
            int m_MatrixID; // matrix id

            union
            {
                struct
                {
                    dword m_Inversed;
	                float m_AngleZ;
	                float m_AngleX;
                    float m_HFOV;
                    float m_UpAngle;
                    float m_DownAngle;
                };

                struct
                {
                    int m_Module;
                };
            };

        } m_Weapon;

        struct
        {
            int   m_MatrixID; // matrix id
            dword m_Inversed;
	        float m_Angle;
        } m_Engine;

    };

    void Release(void);
    bool Tact(CMatrixFlyer* owner, float ms);
};

#define MF_TARGETMOVE               SETBIT(0)
#define MF_TARGETFIRE               SETBIT(1)
//#define MF_SEEKBASE               SETBIT(2)
//#define MF_SEEKBASEOK             SETBIT(3)
//#define MF_SEEKBASEFOUND          SETBIT(4)

#define FLYER_ACTION_MOVE_FORWARD   SETBIT(5)
#define FLYER_ACTION_MOVE_BACKWARD  SETBIT(6)
#define FLYER_ACTION_MOVE_LEFT      SETBIT(7)
#define FLYER_ACTION_MOVE_RIGHT     SETBIT(8)
#define FLYER_ACTION_ROT_LEFT       SETBIT(9)
#define FLYER_ACTION_ROT_RIGHT      SETBIT(10)
#define FLYER_ACTION_CLIMB_UP       SETBIT(11)
#define FLYER_ACTION_CLIMB_DOWN     SETBIT(12)
#define FLYER_MANUAL                SETBIT(13) //Маркер выставления одной из ручных команд управления вертолётом (любой из них)

#define FLYER_BODY_MATRIX_DONE      SETBIT(14)
#define FLYER_IN_SPAWN              SETBIT(15)
#define FLYER_IN_SPAWN_SPINUP       SETBIT(16) //Вертолёт только выкатился с базы и раскручивает лопасти
#define FLYER_BREAKING              SETBIT(17) // тормозит (в стратегическом режиме сход с траектории)
#define FLYER_SGROUP                SETBIT(18)
#define FLYER_SARCADE               SETBIT(19)

enum EFlyerState
{
    STATE_FLYER_IN_SPAWN,
    STATE_FLYER_BASE_TAKE_OFF,
    STATE_FLYER_READY_TO_ORDERS, //Успешно построен и ожидает приказов
    STATE_FLYER_CARRYING_ROBOT,
    STATE_FLYER_IS_BUSY, //Выставляется для вертолётов поддержки
    STATE_FLYER_DIP //Запущен процесс уничтожения вертолёта (death in progress)
};

struct SFlyerTactData;

enum EFlyerOrder
{
    FO_GIVE_BOT,
    FO_FIRE,
};

struct SFireStream
{
    union
    {
        CMatrixEffectFireStream* effect = nullptr;
        CBlockPar* bp;// = nullptr;
    };

    int matrix = 0; // matrix id
    int unit = 0;   // unit index
};

class CMatrixFlyer : public CMatrixMapStatic
{
    static D3D_VB     m_VB;
    static int        m_VB_ref;

    int               m_Side = NEUTRAL_SIDE;

    SMatrixFlyerUnit* m_Modules = nullptr;
    int               m_ModulesCount = 0;
    int               m_EngineUnit = -1;

    SFireStream*      m_Streams = nullptr;
    int               m_StreamsCount = 0;
    float             m_StreamLen = 10;

	D3DXVECTOR2       m_Target = { 200.0f, 280.0f };
    D3DXVECTOR3       m_FireTarget = { 0.0f, 0.0f, 0.0f };

    dword             m_Sound = SOUND_ID_EMPTY;

    SEffectHandler    m_Ablaze;

    //union
    //{
	//float m_TargetAlt = FLYER_ALT_EMPTY;
    CMatrixBuilding* m_Base = nullptr;
    //};

    //Все углы представлены в радианах
    float m_DAngle = 0.0f;
    float m_AngleZ = 0.0f;
    float m_AngleZSin = 0.0f;
    float m_AngleZCos = 0.0f;

    float m_RotZSpeed = 0.0f;
	float m_MoveSpeed = 0.0f;
    float m_StrafeSpeed = 0.0f; // <0 - left, >0 rite

    float m_Pitch = 0.0f; //Угол тангажа, при нормальном положении вертолёта равен 0, наклон вперёд - , наклон назад +
	float m_Roll = 0.0f; //Угол крена, при нормальном положении вертолёта равен 0, крен влево + , крен вправо -

    float m_AltitudeCorrection = 0.0f; //Значение для управляемой корректировки высоты полёта (суммируется с автоматической базовой высотой)

    float m_TargetEngineAngle = ENGINE_ANGLE_STAY;
    float m_TargetPitchAngle = 0.0f;
    float m_TargetRollAngle = 0.0f;

    //float          m_BaseLandAngle;
    //D3DXVECTOR3    m_BaseLandPos;
    CTrajectory* m_Trajectory = nullptr;
    union
    {
        struct // in breaking mode
        {
            D3DXVECTOR3 m_StoreTarget;
        };
        struct
        {
            float m_TrajectoryPos; // [0..1]
            float m_TrajectoryLen; 
            float m_TrajectoryLenRev; 
            float m_TrajectoryTargetAngle;
        };
    };

    int m_TgtUpdateCount = 0;

    int m_NextTimeAblaze = 0;
    int m_LastDelayDamageSide = 0;

    CWStr m_Name = (CWStr)L"FLYER";

    //hitpoint
    CMatrixProgressBar m_HealthBar;
    int         m_ShowHitpointsTime = 0;
    float       m_Hitpoints = 0.0f;
	float       m_MaxHitpoints = 0.0f;  // Максимальное кол-во здоровья
    float       m_MaxHitpointsInversed = 0.0f; // for normalized calcs

    CTextureManaged* m_BigTexture = nullptr;
    CTextureManaged* m_MedTexture = nullptr;
#ifdef USE_SMALL_TEXTURE_IN_ROBOT_ICON
    CTextureManaged* m_SmallTexture = nullptr;
#endif

    EShadowType m_ShadowType = SHADOW_OFF;

    void CalcMatrix();
    void CalcBodyMatrix();
    void LogicTactArcade(SFlyerTactData& td);
    void LogicTactStrategy(SFlyerTactData& td);
    bool LogicTactOrder(SFlyerTactData& td);

    void CalcCollisionDisplace(SFlyerTactData& td);

    //void CalcTrajectory(const D3DXVECTOR3 &target, const D3DXVECTOR3 &dir);
    void CalcTrajectory(const D3DXVECTOR3& target);
    void ProceedTrajectory(SFlyerTactData& td);
    void CancelTrajectory();
    bool IsTrajectoryEnd() const { return m_TrajectoryPos >= 0.99f; }

    float CalcFlyerZInPoint(float x, float y);

    CMatrixEffectSelection* m_Selection = nullptr;
    int m_CtrlGroup = 0;

public:
    D3DXVECTOR3 m_Pos = { 200.0f, 280.0f, FLYER_ALT_EMPTY }; //origin

    //carrying
    struct SCarryData
    {
        CMatrixRobot* m_Robot = nullptr;
        D3DXVECTOR3   m_RobotForward = { 0.0f, 0.0f, 0.0f };
        D3DXVECTOR3   m_RobotUp = { 0.0f, 0.0f, 0.0f };
        D3DXVECTOR3   m_RobotUpBack = { 0.0f, 0.0f, 0.0f };
        float         m_RobotAngle = 0.0f;
        float         m_RobotMassFactor = 0.0f;
        CMatrixEffectElevatorField* m_RobotElevatorField = nullptr;

    } m_CarryData;

    dword m_Flags = MF_TARGETFIRE | FLYER_IN_SPAWN;
    int   m_Place = -1; //Место нахождения вертолёта на карте

    friend struct SMatrixFlyerUnit;

    int m_Team = 0;
    int m_Group = 0;

    EFlyerState m_CurrentState = STATE_FLYER_IN_SPAWN;

    CMatrixFlyer(EFlyerKind kind = FLYER_SPEED, int side = NEUTRAL_SIDE);
    ~CMatrixFlyer();

    virtual bool IsUnitAlive()
    {
        if(m_CurrentState != STATE_FLYER_DIP) return true;
        return false;
    }

    float       GetMapPosX() const { return m_Pos.x; }
    float       GetMapPosY() const { return m_Pos.y; }
    bool        SelectByGroup();
    bool        SelectArcade();
    void        UnSelect();
    bool        IsSelected();
    CMatrixEffectSelection* GetSelection()  { return m_Selection; }
    bool        CreateSelection();
    void        KillSelection();
    void        MoveSelection();
    CWStr*      GetName()                   { return &m_Name; }
    void        SetName(const CWStr& name)  { m_Name = name; }
    int         GetCtrlGroup()              { return m_CtrlGroup; } 
    void        SetCtrlGroup(int group)     { m_CtrlGroup = group; }

    CMatrixBuilding* GetBase() const        { return m_Base; }

    bool        IsDeliveryCopter() const    { return FLAG(m_ObjectFlags, FLYER_FLAG_DELIVERY_COPTER); }
    void        SetDeliveryCopter(bool set) { INITFLAG(m_ObjectFlags, FLYER_FLAG_DELIVERY_COPTER, set); }

    void        SetSide(int side) { m_Side = side; }

    EFlyerKind  m_FlyerKind = FLYER_SPEED;

    static void StaticInit()
    {
        m_VB = nullptr;
        m_VB_ref = 0;
    }

    static void MarkAllBuffersNoNeed();
    static void InitBuffers();

    void ApplyOrder(const D3DXVECTOR2& pos, int side, EFlyerOrder order, float ang, int place, const CPoint& bpos, int robot_template);

    // carry robot
    bool IsCarryingRobot() const { return m_CarryData.m_Robot != nullptr; }
    CMatrixRobot* GetCarryingRobot() { return m_CarryData.m_Robot; }
    CMatrixFlyer::SCarryData* GetCarryData() { return &m_CarryData; };

    void  ShowHitpoints() { m_ShowHitpointsTime = HITPOINT_SHOW_TIME; }
    float GetHitpoints() const { return 0.1f * m_Hitpoints; }
    float GetMaxHitpoints() { return 0.1f * m_MaxHitpoints; }
    void  InitMaxHitpoints(float hp) { m_Hitpoints = hp; m_MaxHitpoints = hp; m_MaxHitpointsInversed = 1.0f / hp; }

    void SetHitpoint(float hp) { m_Hitpoints = hp; }

    const D3DXVECTOR3& GetPos() const { return m_Pos; } //Возвращает текущие фактические координаты вертолёта по всем трём осям
    const D3DXVECTOR3  GetPos(float ahead_to) const { return GetPos() + D3DXVECTOR3(-m_AngleZSin, m_AngleZCos, 0.0f) * ahead_to; } //Возвращает координатам на удалении, равном значению аргумента, по направлению вперёд ровно от носа вертолёта, можно использовать для расчёта вектора прямолинейного движения
    const D3DXVECTOR3  GetForward(float ahead_to) const { return D3DXVECTOR3(-m_AngleZSin, m_AngleZCos, 0.0f) * ahead_to; } //Возвращает указанную в аргументе прибавку к координатам (не точные координаты, а именно плюс к фактической m_Pos) ровно перед носом вертолёта, по которым считается вектор направления и которые можно использовать, чтобы отдавать команду двигаться строго вперёд

    const D3DXVECTOR2  GetTarget() const { return m_Target; }
    //void             SetAlt(float alt) { m_TargetAlt = alt; }

    //Угол направления носа вертолёта
    float GetDirectionAngle() const { return m_AngleZ; };
    void SetDirectionAngle(float a)
    {
        a = (float)AngleNorm(a);
        if(a != m_AngleZ)
        {
            m_AngleZ = a;
            SinCos(a, &m_AngleZSin, &m_AngleZCos);
        }
    }
    //Для внешних вызовов
    void SetRollAngle(float angle) { m_TargetRollAngle = angle; }

    void SetTarget(const D3DXVECTOR2& tgt);
    void SetFireTarget(const D3DXVECTOR3& tgt)
    {
        //if(!FLAG(m_Flags, MF_SEEKBASE))
        //{
        m_FireTarget = tgt;
        RESETFLAG(m_Flags, MF_TARGETFIRE);
        //}
    };

    float GetSpeed() const { return m_MoveSpeed; }
    float GetSpeedNorm() const { return m_MoveSpeed / FlyerSpecifications[m_FlyerKind].forward_speed; }

    void MoveForward() { SETFLAG(m_Flags, FLYER_ACTION_MOVE_FORWARD); }
    void MoveBackward() { SETFLAG(m_Flags, FLYER_ACTION_MOVE_BACKWARD); }
    void MoveLeft() { SETFLAG(m_Flags, FLYER_ACTION_MOVE_LEFT); }
    void MoveRight() { SETFLAG(m_Flags, FLYER_ACTION_MOVE_RIGHT); }
    void RotLeft() { SETFLAG(m_Flags, FLYER_ACTION_ROT_LEFT); }
    void RotRight() { SETFLAG(m_Flags, FLYER_ACTION_ROT_RIGHT); }
    void MoveUp(const SFlyerSpecifications& corr) { m_AltitudeCorrection += corr.climb_speed;/*SETFLAG(m_Flags, FLYER_ACTION_CLIMB_UP);*/ }
    void MoveUp() { m_AltitudeCorrection += FLYER_MAX_CLIMB_SPEED; }
    void MoveDown(const SFlyerSpecifications& corr) { m_AltitudeCorrection -= corr.climb_speed;/*SETFLAG(m_Flags, FLYER_ACTION_CLIMB_DOWN);*/ }
    void MoveDown() { m_AltitudeCorrection -= FLYER_MAX_CLIMB_SPEED; }

    void FireBegin();
    void FireEnd();

    //void DownToBase(CMatrixBuilding* building);

    void Begin(CMatrixBuilding* b);

    void DrawPropeller();

    void ReleaseMe();

    void CreateTextures();
    
    CTextureManaged* GetBigTexture()   { return m_BigTexture; }
    CTextureManaged* GetMedTexture()   { return m_MedTexture; }
#ifdef USE_SMALL_TEXTURE_IN_ROBOT_ICON
    CTextureManaged* GetSmallTexture() { return m_SmallTexture; }
#endif

    void CreateHealthBarClone(float x, float y, float width, EPBCoord clone_type);
    void DeleteHealthBarClone(EPBCoord clone_type);

    virtual bool TakingDamage(int weap, const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, int attacker_side = NEUTRAL_SIDE, CMatrixMapStatic* attaker = nullptr);
	virtual void GetResources(dword need); // Запрашиваем нужные ресурсы объекта

	virtual void Tact(int cms);
    virtual void LogicTact(int cms);

    virtual bool Pick(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt) const;

    virtual void BeforeDraw();
    virtual void Draw();
    virtual void DrawShadowStencil();
    virtual void DrawShadowProj() {};

    virtual void FreeDynamicResources();

    virtual void Load(CBuf& buf, CTemporaryLoadData* td) {};

    virtual bool CalcBounds(D3DXVECTOR3& omin, D3DXVECTOR3& omax);
    virtual int GetSide() const { return m_Side; };
    virtual float NeedRepair() const { return m_MaxHitpoints - m_Hitpoints; }
    virtual float GetHitpointsPercent() const { return m_Hitpoints / m_MaxHitpoints * 100.0f; }

    virtual bool InRect(const CRect& rect)const;

    void OnOutScreen() {};
};

inline bool CMatrixMapStatic::IsFlyerAlive() const
{
    return IsFlyer() && ((CMatrixFlyer*)this)->m_CurrentState != STATE_FLYER_DIP;
}

inline bool CMatrixMapStatic::IsFlyerControllable() const
{
    return IsFlyer() && ((CMatrixFlyer*)this)->m_CurrentState != STATE_FLYER_DIP && !FLAG(m_ObjectFlags, FLYER_FLAG_DELIVERY_COPTER);
}

#endif