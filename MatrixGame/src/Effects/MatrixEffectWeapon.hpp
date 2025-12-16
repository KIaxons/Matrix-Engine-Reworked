// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

// weapon
#define FLAME_PUFF_TTL               2000
#define LASER_WIDTH                  10

#define WEAPON_MAX_HEAT              1000

extern float g_WeaponDamageNormalCoef;
extern float g_WeaponDamageArcadeCoef;
extern float g_UnitSpeedArcadeCoef;

//Раньше использовалось для определения типа орудия по номеру,
// но теперь оно переопределяется в момент загрузки конфигов, так что особого смысла больше не имеет
enum EWeapon
{
    WEAPON_INSTANT_DEATH = -1,
    WEAPON_NONE = 0

    //WEAPON_MACHINEGUN = 1,
    //WEAPON_FLAMETHROWER = 4,
    //WEAPON_MORTAR = 5,
    //WEAPON_LASER = 6,
    //WEAPON_BOMB = 7,
    //WEAPON_PLASMAGUN = 8,
    //WEAPON_DISCHARGER = 9,
    //WEAPON_REPAIRER = 10,
    //WEAPON_TURRET_LASER = 13
};

class CLaser : public CMatrixEffect
{
    CSpriteSequence m_Sprites;
    CSprite     m_end;

    virtual ~CLaser()
    {
        m_Sprites.Release();
        m_end.Release();
    }

public:
    CLaser(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1);

    virtual void BeforeDraw(void) {};
    virtual void Draw(void);
    virtual void Tact(float) {};
    virtual void Release(void)
    {
        HDelete(CLaser, this, m_Heap);
    };
    virtual int  Priority(void) { return MAX_EFFECT_PRIORITY; };

    void SetPos(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1)
    {
        m_Sprites.SetPos(pos0, pos1);
        m_end.m_Pos = pos0;
    }
};

class CMachinegun : public CMatrixEffect
{
    CMatrixEffectCone m_Cone;
    CSpriteSequence   m_Sprite1;
    CSpriteSequence   m_Sprite2;
    CSpriteSequence   m_Sprite3;

    //Значения по умолчанию, задаются при вызове конструктора
    float m_SpritesLenght = 15.0f;
    float m_FireConeLenght = 5.0f;
    float m_FireConeRadius = 5.0f;

    virtual ~CMachinegun()
    {
        m_Sprite1.Release();
        m_Sprite2.Release();
        m_Sprite3.Release();
    }

public:
    CMachinegun(const D3DXVECTOR3& start, const D3DXVECTOR3& dir, float angle, float sprites_lenght, float sprites_width, float fire_cone_lenght, float fire_cone_radius, dword color, int sprite_num_1, int sprite_num_2, int sprite_num_3);

    virtual void BeforeDraw()
    {
        m_Cone.BeforeDraw();
    };
    virtual void Draw();
    virtual void Tact(float) {};
    virtual void Release()
    {
        HDelete(CMachinegun, this, m_Heap);
    };

    virtual int Priority() { return MAX_EFFECT_PRIORITY; };

    float GetSpritesLenght() { return m_SpritesLenght; };

    void SetPos(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, const D3DXVECTOR3& dir)
    {
        m_Sprite1.SetPos(pos0, pos1);
        m_Sprite2.SetPos(pos0, pos1);
        m_Sprite3.SetPos(pos0, pos1);
        m_Cone.Modify(pos0, dir);
    }
    void SetPos(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, const D3DXVECTOR3& dir, float angle)
    {
        m_Sprite1.SetPos(pos0, pos1);
        m_Sprite2.SetPos(pos0, pos1);
        m_Sprite3.SetPos(pos0, pos1);
        m_Cone.Modify(pos0, dir, m_FireConeRadius, m_FireConeLenght, angle);
    }
};

class CMatrixEffectWeapon : public CMatrixEffect
{
    int    m_WeaponNum = 0;
    float  m_WeaponDist = 0.0f;
    float  m_WeaponCoefficient = 1.0f;

    dword  m_Sound = SOUND_ID_EMPTY;
    ESound m_ShotSoundType = S_NONE;

    dword  m_User = 0;
    FIRE_END_HANDLER m_Handler = nullptr;
    int    m_Ref = 1;

    D3DXVECTOR3 m_Pos = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3 m_Dir = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3 m_Speed = { 0.0f, 0.0f, 0.0f };

    float  m_Time = 0.0f;
    float  m_CoolDown = 0.0f;
    CMatrixMapStatic* m_Skip = nullptr;

    int    m_FireCount = 0;

    SEffectHandler m_Effect;

    union
    {
        CMachinegun*         m_Machinegun = nullptr;
        CLaser*              m_Laser;// = nullptr;
        CMatrixEffectRepair* m_Repair;// = nullptr;
    };

    SObjectCore* m_Owner = nullptr;
    int          m_SideStorage = 0; // side storage (if owner will be killed)

    CMatrixEffectWeapon(const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, dword user, FIRE_END_HANDLER handler, int type, int cooldown);
    virtual ~CMatrixEffectWeapon();

    void FireWeapon();

public:
    D3DXVECTOR3 GetPos()            { return m_Pos; }
    void  SetDefaultCoefficient()   { m_WeaponCoefficient = g_WeaponDamageNormalCoef; }
    void  SetArcadeCoefficient()    { m_WeaponCoefficient = g_WeaponDamageArcadeCoef; }
    int   GetWeaponNum() const  { return m_WeaponNum; }
    float GetWeaponDist() const { return m_WeaponDist * m_WeaponCoefficient; }
    friend class CMatrixEffect;

    static void WeaponHit(CMatrixMapStatic* hiti, const D3DXVECTOR3& pos, dword user, dword flags);
    static void SelfRepairEffect();

    void SetOwner(CMatrixMapStatic* ms);
    int  GetSideStorage() const { return m_SideStorage; };
    CMatrixMapStatic* GetOwner();

    virtual void BeforeDraw() {};
    virtual void Draw() {};
    virtual void Tact(float step);
    virtual void Release();

    int GetShotsDelay() { return int(m_CoolDown); };

    virtual int Priority() { return MAX_EFFECT_PRIORITY; };

    void Modify(const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, const D3DXVECTOR3& speed);

    bool IsFire() { return FLAG(m_Flags, WEAPFLAGS_FIRE); }
    bool IsFireWas() { bool r = FLAG(m_Flags, WEAPFLAGS_FIREWAS); RESETFLAG(m_Flags, WEAPFLAGS_FIREWAS); return r; }
    bool IsHitWas() { bool r = FLAG(m_Flags, WEAPFLAGS_HITWAS); RESETFLAG(m_Flags, WEAPFLAGS_HITWAS); return r; }

    void ResetFireCount() { m_FireCount = 0; }
    int GetFireCount() const { return m_FireCount; }

    void ModifyCoolDown(float addk) { m_CoolDown += m_CoolDown * addk; }
    void ModifyDist(float addk) { m_WeaponDist += m_WeaponDist * addk; }

    void FireBegin(const D3DXVECTOR3& speed, CMatrixMapStatic* skip)
    {
        if(IsFire()) return;

        m_Speed = speed;
        SETFLAG(m_Flags, WEAPFLAGS_FIRE);
        RESETFLAG(m_Flags, WEAPFLAGS_FIREWAS);
        RESETFLAG(m_Flags, WEAPFLAGS_HITWAS);

        m_Skip = skip;

        //CHelper::Create(10000, 0)->Line(m_Pos, m_Pos + m_Dir * 100);
    }
    void FireEnd();

    static void SoundHit(int weapon_num, const D3DXVECTOR3& pos);
};