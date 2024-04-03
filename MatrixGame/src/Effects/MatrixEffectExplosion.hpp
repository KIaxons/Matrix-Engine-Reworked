// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

#include "MatrixEffectSmokeAndFire.hpp"
#include "../MatrixSoundManager.hpp"

// explosion


#define EXPLOSION_TIME_PERIOD   10
#define DEBRIS_SPEED        (1.0f / 100.0f)
#define INTENSE_INIT_SIZE   4
#define INTENSE_END_SIZE    60
#define INTENSE_DISPLACE_RADIUS  10
#define INTENSE_TTL         1300

#define EXPLOSION_SPARK_WIDTH 3

struct SExplosionProperties
{
    float min_speed;
    float max_speed;
    float min_speed_z;
    float max_speed_z;

    int   sparks_deb_cnt;
    int   fire_deb_cnt;
    int   deb_cnt;
    int   deb_min_ttl;
    int   deb_max_ttl;

    int   intense_cnt;

    int   deb_type;

    dword  light;   // bool
    float light_radius1;
    float light_radius2;
    dword light_color1;
    dword light_color2;
    float   light_time1;
    float   light_time2;

    ESound  sound;

    ESpotType voronka;
    float     voronka_scale;
};

extern const SExplosionProperties ExplosionNormal;
extern const SExplosionProperties ExplosionMissile;
extern const SExplosionProperties ExplosionRobotHit;
extern const SExplosionProperties ExplosionRobotBoom;
extern const SExplosionProperties ExplosionRobotBoomSmall;
extern const SExplosionProperties ExplosionBigBoom;
extern const SExplosionProperties ExplosionLaserHit;
extern const SExplosionProperties ExplosionBuildingBoom;
extern const SExplosionProperties ExplosionBuildingBoom2;
extern const SExplosionProperties ExplosionObject;


enum EDebType
{
    DEB_FIRE = -1,
    DEB_INTENSE = -2,
    DEB_SPARK = -3,
    
#if defined _DEBUG || defined _TRACE
    DEB_DEAD = -4, // special flag
#endif

    EDebType_FORCE_DWORD = 0x7FFFFFFF
};

typedef struct _SDebris
{
    D3DXVECTOR3 v = { 0.0f, 0.0f, 0.0f };

    union
    {
        int      index;        // if < 0 then fire
        EDebType type;
    };

    float ttl = 0; // time to live

    union
    {
        struct // debris
        {
            D3DXVECTOR3 pos;
            union
            {
                struct
                {
                    float alpha;  // 1.0 - visible
                    float scale;
                    float angley;
                    float anglep;
                    float angler;
                };

                struct // sparks
                {
                    D3DXVECTOR3  prepos; // distance between prepos and pos cannot be larger then len
                    CSpriteSequence* bline;
                    //float      len;
                    float        unttl;
                };
            };
        };
        struct // intense
        {
            float    ttm;
            CSprite* billboard;
        };
        struct // fire
        {
            SEffectHandler fire;
            SEffectHandler light;
        };
    };

    //��������� ������ union-�� ��������� ����� ����������� _SDebris, �, �������������, ������ ����� ������ � ������������� �����������, �������� _SDebris ��� ��������� �� ��������� �� �����
    //��� ��� ���� ������ ������ ���������� �����������, �� ���������� ����� ��������� ������� ������ ���������� ����������� � _SDebris
    ~_SDebris() {};

} SDebris;


class CMatrixEffectExplosion : public CMatrixEffect
{
    SDebris* m_Deb = nullptr;
    int      m_DebCnt = 0;
    float    m_Time = 0.0f;

    const SExplosionProperties* m_Props = nullptr;

    SEffectHandler m_Light;

    CMatrixEffectExplosion(const D3DXVECTOR3& pos, const SExplosionProperties& props);
	virtual ~CMatrixEffectExplosion();

    void RemoveDebris(int deb);

public:
    friend class CMatrixEffect;

    virtual void BeforeDraw();
    virtual void Draw();
    virtual void Tact(float step);
    virtual void Release();

    virtual int  Priority() { return m_DebCnt ? MAX_EFFECT_PRIORITY : 0; };
};