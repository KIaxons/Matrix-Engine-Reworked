// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once


// flame for flamethrower

#define FLAME_NUM_BILLS     10

#define FLAME_SCALE_FACTOR  3.6f //2.16f
//#define FLAME_DIST_FACTOR   0.3f
#define FLAME_DIR_SPEED     0.36f

#define FLAME_TIME_SEEK_PERIOD  20

struct SFlameBill
{
    CSprite m_Flame;
    float   m_Sign = 0.0f;

    ~SFlameBill()
    {
        m_Flame.Release();
    }
};

class CMatrixEffectFlame;

class CFlamePuff : public CMain
{
    CMatrixEffectFlame* m_Owner = nullptr;

    SEffectHandler m_Light;

    float m_Alpha = 0.0f;
    int   m_CurAlpha = 0;

    SFlameBill m_Flames[FLAME_NUM_BILLS];

    float m_Time = 0.0f;
    float m_NextSmokeTime = 0.0f;
    float m_NextSeekTime = 0.0f;

    D3DXVECTOR3 m_Pos = { 0.0f,0.0f,0.0f };
    D3DXVECTOR3 m_Dir = { 0.0f,0.0f,0.0f };
    D3DXVECTOR3 m_Speed = { 0.0f,0.0f,0.0f };

    float m_Scale = 0.0f;

    CSpriteSequence* m_Shleif = nullptr;
    dword        m_Break = 0;

    CFlamePuff* m_Next = nullptr;
    CFlamePuff* m_Prev = nullptr;

    CFlamePuff(CMatrixEffectFlame* owner, const D3DXVECTOR3& start, const D3DXVECTOR3& dir, const D3DXVECTOR3& speed);
	~CFlamePuff();

public:
    friend class CMatrixEffectFlame;

    void Draw();
    void Tact(float step);
    void Release();
};

class CMatrixEffectWeapon;

class CMatrixEffectFlame : public CMatrixEffect
{
    CFlamePuff* m_First = nullptr;
    CFlamePuff* m_Last = nullptr;

    CFlamePuff* m_Stream = nullptr;
    
    dword m_User = 0;
    FIRE_END_HANDLER m_Handler;
    dword m_hitmask = 0;
    CMatrixMapStatic* m_skip = nullptr;

    float m_TTL = 0.0f;
    float _m_TTL = 0.0f;

    SEffectHandler m_Smokes;

    CMatrixEffectFlame(float ttl, dword hitmask, CMatrixMapStatic* skip, dword user, FIRE_END_HANDLER handler);
	virtual ~CMatrixEffectFlame();

    void CreateSmoke(const D3DXVECTOR3& pos);

public:
    friend class CMatrixEffect;
    friend class CFlamePuff;

    virtual void BeforeDraw(void);
    virtual void Draw(void);
    virtual void Tact(float step);
    virtual void Release(void);

    virtual int  Priority(void) { return MAX_EFFECT_PRIORITY; };


    void AddPuff(const D3DXVECTOR3& start, const D3DXVECTOR3& dir, const D3DXVECTOR3& speed);
    void SubPuff(CFlamePuff* puf);
    void Break(void) { m_Stream = nullptr; };
};