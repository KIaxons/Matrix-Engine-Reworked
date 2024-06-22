// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

//#define MAX_EFFECTS_COUNT   1280
#define MAX_EFFECT_PRIORITY 10000

#include "../stdafx.h"
#include <vector>

#define MAX_EFFECT_DISTANCE_SQ  ((3000) * (3000))

//Базовые спрайты, которые по умолчанию задействованы в ПБ (не менять размер с int, т.к. массив базовых спрайтов расширяется после загрузки новых из конфига)
enum ESpriteTextureSort : int
{
    SPR_SMOKE_PART = 0,
    SPR_BRIGHT_SMOKE_PART,
    SPR_FIRE_PART,
    SPR_BRIGHT_FIRE_PART,

    SPR_SPOT_LIGHT,
    SPR_SELECTION_PART,
    SPR_PATH_PART,

    BBT_INTENSE,
    SPR_GUN_FLASH_1,
    SPR_GUN_FLASH_2,
    SPR_GUN_FLASH_3,
    SPR_CONTRAIL,

    SPR_JET_STREAM_ANIM_FRAME_1,
    SPR_JET_STREAM_ANIM_FRAME_2,
    SPR_JET_STREAM_ANIM_FRAME_3,
    SPR_JET_STREAM_ANIM_FRAME_4,
    SPR_JET_STREAM_ANIM_FRAME_5,

    SPR_COOLING_FAN_ANIM_FRAME_1,
    SPR_COOLING_FAN_ANIM_FRAME_2,
    SPR_COOLING_FAN_ANIM_FRAME_3,
    SPR_COOLING_FAN_ANIM_FRAME_4,
    SPR_COOLING_FAN_ANIM_FRAME_5,
    SPR_COOLING_FAN_ANIM_FRAME_6,
    SPR_COOLING_FAN_ANIM_FRAME_7,
    SPR_COOLING_FAN_ANIM_FRAME_8,
    SPR_COOLING_FAN_ANIM_FRAME_9,
    SPR_COOLING_FAN_ANIM_FRAME_10,

    SPR_SPARK,

    SPR_FLAME_ANIM_FRAME_1,
    SPR_FLAME_ANIM_FRAME_2,
    SPR_FLAME_ANIM_FRAME_3,
    SPR_FLAME_ANIM_FRAME_4,
    SPR_FLAME_ANIM_FRAME_5,
    SPR_FLAME_ANIM_FRAME_6,
    SPR_FLAME_ANIM_FRAME_7,
    SPR_FLAME_ANIM_FRAME_8,

    SPR_FLAMETHROWER_SPOT,
    SPR_FLAMETHROWER_BEAM,

    SPR_LASER_SPOT,
    SPR_LASER_BEAM,

    SPR_PLASMA_SPOT,
    SPR_PLASMA_RECT,

    SPR_DISCHARGER_SPOT,
    SPR_DISCHARGER_BEAM,

    SPR_REPAIRER_SPOT,
    SPR_REPAIRER_RECT,

    SPR_SCORE_PLUS_SIGN,
    SPR_SCORE_NUMBER_0,
    SPR_SCORE_NUMBER_1,
    SPR_SCORE_NUMBER_2,
    SPR_SCORE_NUMBER_3,
    SPR_SCORE_NUMBER_4,
    SPR_SCORE_NUMBER_5,
    SPR_SCORE_NUMBER_6,
    SPR_SCORE_NUMBER_7,
    SPR_SCORE_NUMBER_8,
    SPR_SCORE_NUMBER_9,

    SPR_TITAN_ICON,
    SPR_ELECTRONICS_ICON,
    SPR_ENERGY_ICON,
    SPR_PLASMA_ICON,
    SPR_RESOURCES_ICON,

    SPR_LAST,
    SPR_NONE = -1
};

struct SSpriteTextureArrayElement
{
    bool m_Single = false;
    bool IsSingleBrightTexture() const { return m_Single; };
    void SetSingleBrightTexture(bool set) { m_Single = set; };

    SSpriteTexture   spr_tex = { 0.0f, 0.0f, 0.0f, 0.0f }; // for sorted
    CTextureManaged* tex = nullptr;  // for intense
};

//classes
class CDebris : public CVectorObjectAnim
{
    int m_DebrisType = 0;

public:
    void SetDebType(int type) { m_DebrisType = type; };
    int  GetDebType()         { return m_DebrisType; }
};

class CMatrixEffectExplosion;
class CMatrixEffectFire;
class CMatrixEffectFirePlasma;
class CMatrixEffectMovingObject;
class CMatrixEffectPointLight;
class CMatrixEffectLandscapeSpot;
class CMatrixMapStatic;
class CMatrixEffectBillboard;
class CMatrixEffectLinkedSpriteAnim;
class CMatrixEffectFireStream;
class CMatrixEffectFlame;
class CMatrixEffectFireAnim;

struct SExplosionProperties;
struct SMOProps;


typedef void (*ADD_TAKT)(CMatrixEffectBillboard* bb, float ms);

#define FEHF_LASTHIT        SETBIT(0)
#define FEHF_DAMAGE_ROBOT   SETBIT(1)

typedef void (*FIRE_END_HANDLER)(CMatrixMapStatic* hit, const D3DXVECTOR3& pos, dword user, dword flags);

#define SMOKE_SPEED (1.0f / 25.0f)
// fire
#define FIRE_SPEED  (0.5f / 25.0f)

// selection
#define SEL_COLOR_DEFAULT 0xFF00FF00
#define SEL_COLOR_TMP     0xFF305010
enum EBuoyType
{
    BUOY_RED = 0,
    BUOY_GREEN,
    BUOY_BLUE
};

enum ESpotType
{
    SPOT_CONSTANT = 0,
	SPOT_SELECTION,
    SPOT_PLASMA_HIT,
	//SPOT_MOVE_TO,
    SPOT_POINTLIGHT,
    SPOT_VORONKA,
    SPOT_TURRET,

    SPOT_TYPES_CNT
};

typedef void (*SPOT_TAKT_FUNC)(CMatrixEffectLandscapeSpot* spot, float ms);

#define LSFLAG_INTENSE          SETBIT(0)
#define LSFLAG_SCALE_BY_NORMAL  SETBIT(1)

struct SSpotProperties
{
    SPOT_TAKT_FUNC    func = nullptr;
    CTextureManaged*  texture = nullptr;
    dword             color = 0;
    float             ttl = 0.0f; //"время жизни"
    dword             flags = 0;
};

#define MAX_EFFECTS_EXPLOSIONS      50
#define MAX_EFFECTS_POINT_LIGHTS    50
#define MAX_EFFECTS_SMOKEFIRES      100
#define MAX_EFFECTS_LANDSPOTS       100
#define MAX_EFFECTS_FIREANIM        50

// common
enum EEffectType
{
    /* 0  */ EFFECT_UNDEFINED = 0,
    /* 1  */ EFFECT_EXPLOSION,
    /* 2  */ EFFECT_SMOKE,
    /* 3  */ EFFECT_BRIGHT_SMOKE,
    /* 4  */ EFFECT_FIRE_PLASMA,
    /* 5  */ EFFECT_LANDSCAPE_SPOT,
    /* 6  */ EFFECT_FIRE,
    /* 7  */ EFFECT_BRIGHT_FIRE,
    /* 8  */ EFFECT_MOVING_OBJECT,
    /* 9  */ EFFECT_SELECTION,
    /* 10 */ EFFECT_SELECTION_LAND,
    /* 11 */ EFFECT_MOVETO,
    /* 12 */ EFFECT_PATH,
    /* 13 */ EFFECT_CAPTURE_CIRCLES,
    /* 14 */ EFFECT_ELEVATOR_FIELD,
    /* 15 */ EFFECT_POINT_LIGHT,
    /* 16 */ EFFECT_KONUS,
    /* 17 */ EFFECT_BILLBOARD,
    /* 18 */ EFFECT_SPRITES_LINE,
    /* 19 */ EFFECT_FLAME,
    /* 20 */ EFFECT_BIG_BOOM,
    /* 21 */ EFFECT_LIGHTENING,
    /* 22 */ EFFECT_SHORTED,
    /* 23 */ EFFECT_LASER,
    /* 24 */ EFFECT_VOLCANO,
    /* 25 */ EFFECT_LINKED_ANIM,
    /* 26 */ EFFECT_FIRE_STREAM,
    /* 27 */ EFFECT_DUST,
    /* 28 */ EFFECT_SMOKE_SHLEIF,
    /* 29 */ EFFECT_FIRE_ANIM,
    /* 30 */ EFFECT_SCORE,
    /* 31 */ EFFECT_REPAIR,
    /* 32 */ EFFECT_WEAPON,
    /* 33 */ EFFECT_SPLASH,

    EFFECT_TYPE_COUNT
};

// before draw done flags
#define BDDF_EXPLOSION    SETBIT(0)
#define BDDF_DUST         SETBIT(1)
//#define BDDF_FIRE       SETBIT(2)
//#define BDDF_SMOKE      SETBIT(3)
//#define BDDF_PLASMA     SETBIT(4)
//#define BDDF_FLAME      SETBIT(5)
//#define BDDF_LIGHTENING SETBIT(6)
#define BDDF_PLIGHT       SETBIT(7)
//#define BDDF_FIRESTREAM SETBIT(8)



// m_Flags

#define EFFF_DIP            SETBIT(0)    // effect is under destruction

// only weapon flags
#define WEAPFLAGS_FIRE      SETBIT(1)
#define WEAPFLAGS_FIREWAS   SETBIT(2)
#define WEAPFLAGS_HITWAS    SETBIT(3)   // only for instant weapons (LASER, VOLCANO, LIGHTENING)
#define WEAPFLAGS_SND_OFF   SETBIT(4)   // выключать при FireEnd (использовать на зацикленных звуках (Laser, lightening))
#define WEAPFLAGS_SND_SKIP  SETBIT(5)   // если звук уже звучит, пропускать (volcano)

// only repair flags
#define ERF_LOST_TARGET     SETBIT(1)
#define ERF_OFF_TARGET      SETBIT(2)
#define ERF_FOUND_TARGET    SETBIT(3)
#define ERF_WITH_TARGET     SETBIT(4)
#define ERF_MAX_AMP         SETBIT(5)

// only PointLight flags
#define PLF_KIP             SETBIT(1)

// only Selection flags
#define SELF_KIP            SETBIT(1)

// only BeginBombCountdown flags
#define BIGBOOMF_PREPARED   SETBIT(1)

// only MoveTo flags
#define MOVETOF_PREPARED    SETBIT(1)


struct SGradient
{
    float c = 0.0f;
    float t = 0.0f;
};

inline  float CalcGradient(float t, const SGradient* grad)
{
    for(int i = 1; ; ++i)
    {
        if(t >= grad[i].t) continue;
        return (grad[i].c - grad[i - 1].c) * (t - grad[i - 1].t) / (grad[i].t - grad[i - 1].t) + grad[i - 1].c;
    }
}

class CMatrixEffect;

struct SEffectHandler
{
    CMatrixEffect* effect = nullptr;

    SEffectHandler() = default;
    ~SEffectHandler() { Release(); }

    void Release();
    void Unconnect();
    void Rebase();
};

#define ELIST_NAME(n, x)            m_EffectsList##n##x
#define ELIST_FIRST(n)              ELIST_NAME(n, _First)
#define ELIST_LAST(n)               ELIST_NAME(n, _Last)
#define ELIST_CNT(n)                ELIST_NAME(n, _Count)
#define ELIST_DECLARE_INCLASS(n)    static CMatrixEffect* ELIST_FIRST(n); static CMatrixEffect* ELIST_LAST(n); static int ELIST_CNT(n)
#define ELIST_DECLARE_OUTCLASS(n)   CMatrixEffect* CMatrixEffect::ELIST_FIRST(n); CMatrixEffect* CMatrixEffect::ELIST_LAST(n); int CMatrixEffect::ELIST_CNT(n)
#define ELIST_INIT(n)               CMatrixEffect::ELIST_FIRST(n) = nullptr; CMatrixEffect::ELIST_LAST(n) = nullptr; CMatrixEffect::ELIST_CNT(n) = 0
#define ELIST_ADD(n)                LIST_ADD(this, ELIST_FIRST(n), ELIST_LAST(n), m_TypePrev, m_TypeNext); ++ELIST_CNT(n)
#define ELIST_DEL(n)                LIST_DEL(this, ELIST_FIRST(n), ELIST_LAST(n), m_TypePrev, m_TypeNext); --ELIST_CNT(n)

#define ELSIT_REMOVE_OLDEST(n)             { g_MatrixMap->SubEffect(ELIST_FIRST(n)); }
#define ELSIT_REMOVE_OLDEST_PRIORITY(n, p) { CMatrixEffect* ef = ELIST_FIRST(n); CMatrixEffect* ef2d = ef; for(; ef; ef = ef->m_TypeNext) { if(ef->Priority() < (p)) { ef2d = ef; break;} } { if(ef2d == ELIST_LAST(n)) { ef2d = ELIST_FIRST(n); }} g_MatrixMap->SubEffect(ef2d); }

struct SFloatRGBColor
{
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
};
class CMatrixEffect : public CMain
{
protected:
    //DPTR_MEM_SIGNATURE_DECLARE();

    EEffectType     m_EffectType = EFFECT_UNDEFINED;
    SEffectHandler* m_EffectHandler = nullptr;
    dword           m_Flags = 0;


    CMatrixEffect* m_TypePrev = nullptr;
    CMatrixEffect* m_TypeNext = nullptr;


    static dword    m_before_draw_done;
    static CDebris* m_Debris;
    static int      m_DebrisCnt;
    static std::vector<SSpotProperties> m_SpotProperties;
    static std::vector<SSpriteTextureArrayElement> m_SpriteTextures;

    ELIST_DECLARE_INCLASS(EFFECT_EXPLOSION);
    ELIST_DECLARE_INCLASS(EFFECT_POINT_LIGHT);
    ELIST_DECLARE_INCLASS(EFFECT_SMOKE);
    ELIST_DECLARE_INCLASS(EFFECT_LANDSCAPE_SPOT);
    ELIST_DECLARE_INCLASS(EFFECT_MOVETO);
    ELIST_DECLARE_INCLASS(EFFECT_FIRE_ANIM);

public:
    static Base::CHeap* m_Heap;
    CMatrixEffect*      m_Prev = nullptr;
    CMatrixEffect*      m_Next = nullptr;
    static float        m_Dist2;    // used by movin objects (missiles cannons)

protected:
    CMatrixEffect() : CMain()
    {
        //DPTR_MEM_SIGNATURE_INIT(sizeof(CMatrixEffect));
        DCS_INCONSTRUCTOR();
    };
    ~CMatrixEffect()
    {
        if(m_EffectHandler) m_EffectHandler->effect = nullptr;
    };

    void SetDIP() { SETFLAG(m_Flags, EFFF_DIP); }

public:
    friend class CMatrixEffectExplosion;
    friend class CMatrixEffectSmoke;
    friend class CMatrixEffectFire;
    friend class CMatrixEffectFirePlasma;
    friend class CMatrixEffectLandscapeSpot;
    friend class CMatrixEffectPointLight;
    friend class CMatrixEffectMoveto;
    friend class CMatrixEffectFireAnim;

    void            SetHandler(SEffectHandler* eh) { m_EffectHandler = eh; }
    SEffectHandler* GetHandler() { return m_EffectHandler; }
    void            Unconnect() { if(m_EffectHandler) m_EffectHandler->effect = nullptr; }
    EEffectType     GetType() { return m_EffectType; }

    static void     CreatePoolDefaultResources();
    static void     ReleasePoolDefaultResources();

    static void     InitEffects(CBlockPar& bp);
    static void     ClearEffects();

    static const SSpriteTexture* GetSpriteTex(ESpriteTextureSort t) { return &m_SpriteTextures[t].spr_tex; };
    static CTextureManaged* GetSingleBrightSpriteTex(ESpriteTextureSort t) { return m_SpriteTextures[t].tex; };
    static int AddSpriteTexture(CWStr sprite_name, int sprite_num, int sprites_count, CBlockPar* cache_path_bp, CBlockPar& check_bp);

    static void            CreateExplosion(const D3DXVECTOR3& pos, const SExplosionProperties& props, bool fire = false); // automaticaly adds to Effects list; can return nullptr
    
    static void            CreateSmoke(SEffectHandler* eh, const D3DXVECTOR3& pos, float ttl, float puffttl, float spawntime, dword color, bool is_bright = false, float speed = SMOKE_SPEED);   // automaticaly adds to Effects list; can return nullptr
    static void            CreateFire(SEffectHandler* eh, const D3DXVECTOR3& pos, float ttl, float puffttl, float spawntime, float dispfactor, bool is_bright = false, float speed = FIRE_SPEED, const SFloatRGBColor& close_color = { 1.0f, 1.0f, 0.1f }, const SFloatRGBColor& far_color = { 0.5f, 0.0f, 0.0f }); //automaticaly adds to Effects list; can return nullptr
    static void            CreateFireAnim(SEffectHandler* eh, const D3DXVECTOR3& pos, float anim_width, float anim_height, int time_to_live, const std::vector<int>& sprites_id = { SPR_FLAME_ANIM_FRAME_1, SPR_FLAME_ANIM_FRAME_2, SPR_FLAME_ANIM_FRAME_3, SPR_FLAME_ANIM_FRAME_4, SPR_FLAME_ANIM_FRAME_5, SPR_FLAME_ANIM_FRAME_6, SPR_FLAME_ANIM_FRAME_7, SPR_FLAME_ANIM_FRAME_8 }); // automaticaly adds to Effects list; can return nullptr
    static void            CreateFirePlasma(const D3DXVECTOR3& start, const D3DXVECTOR3& end, float speed, dword hitmask, CMatrixMapStatic* skip, FIRE_END_HANDLER handler, dword user); // automaticaly adds to Effects list; can return nullptr
    static void            CreateFlame(SEffectHandler* eh, float ttl, dword hitmask, CMatrixMapStatic* skip, dword user, FIRE_END_HANDLER handler);

    static void            CreateLandscapeSpot(SEffectHandler* eh, const D3DXVECTOR2& pos, float angle, float scale, ESpotType type = SPOT_CONSTANT);  // automaticaly adds to Effects list; can return nullptr
    static void            CreateMovingObject(SEffectHandler* eh, const SMOProps& props, dword hitmask, CMatrixMapStatic* skip, FIRE_END_HANDLER = nullptr, dword user = 0); // automaticaly adds to Effects list; can return nullptr
    static void            CreateBuoy(SEffectHandler* eh, const D3DXVECTOR3& pos, EBuoyType bt);
    
    static void            CreateMoveToAnim(int type);
    static void            CreateMoveToAnim(const D3DXVECTOR3& pos, int type);
    static void            DeleteAllMoveto(void);
    
    static CMatrixEffect*  CreateSelection(const D3DXVECTOR3& pos, float r, dword color = SEL_COLOR_DEFAULT);
    static CMatrixEffect*  CreatePath(const D3DXVECTOR3* pos, int cnt);
    static void            CreatePointLight(SEffectHandler* eh, const D3DXVECTOR3& pos, float radius, dword color, bool drawbill = false);  // automaticaly adds to Effects list; can return nullptr
    
    static void            CreateKonus(SEffectHandler* eh, const D3DXVECTOR3& start, const D3DXVECTOR3& dir, float radius, float height, float angle, float ttl, bool is_bright, CTextureManaged* tex = nullptr);
    static void            CreateKonusSplash(const D3DXVECTOR3& start, const D3DXVECTOR3& dir, float radius, float height, float angle, float ttl, bool is_bright, CTextureManaged* tex = nullptr);
    
    static CMatrixEffect*  CreateWeapon(const D3DXVECTOR3& start, const D3DXVECTOR3& dir, dword user, FIRE_END_HANDLER handler, int type, int cooldown = 0);
    static void            CreateBigBoom(const D3DXVECTOR3& pos, float radius, float ttl, dword hitmask, CMatrixMapStatic* skip, dword user, FIRE_END_HANDLER handler, dword light = 0);   // automaticaly adds to Effects list; can return nullptr
    
    static void            CreateLightening(SEffectHandler* eh, const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, float ttl, float dispers, float width, dword color = 0xFFFFFFFF, int spot_sprite_num = SPR_DISCHARGER_SPOT, int beam_sprite_num = SPR_DISCHARGER_BEAM); // automaticaly adds to Effects list; can return nullptr
    static void            CreateShorted(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, float ttl, dword color = 0xFFFFFFFF, int beam_sprite_num = SPR_DISCHARGER_BEAM); // automaticaly adds to Effects list; can return nullptr
   
    static void            CreateBillboard(SEffectHandler* eh, const D3DXVECTOR3& pos, float radius1, float radius2, dword color1, dword color2, float ttl, float delay, const wchar* tex, const D3DXVECTOR3& dir, ADD_TAKT addtakt = nullptr); // automaticaly adds to Effects list; can return nullptr
    static void            CreateBillboardScore(const wchar* n, const D3DXVECTOR3& pos, dword color);
    static void            CreateSpritesLine(SEffectHandler* eh, const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, float width, dword color1, dword color2, float ttl, CTextureManaged* tex); // automaticaly adds to Effects list; can return nullptr
    
    static CMatrixEffect*  CreateCaptureCircles(const D3DXVECTOR3& pos, float radius, float angle, int cnt);
    static CMatrixEffect*  CreateElevatorField(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, float radius, const D3DXVECTOR3& fwd);
    static void            CreateDust(SEffectHandler* eh, const D3DXVECTOR2& pos, const D3DXVECTOR2& add_dir, float ttl); // automaticaly adds to Effects list; can return nullptr
    static CMatrixEffectFireStream* CreateFireStream(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, float width, dword color, const std::vector<int>& sprites_num = { SPR_JET_STREAM_ANIM_FRAME_1, SPR_JET_STREAM_ANIM_FRAME_2, SPR_JET_STREAM_ANIM_FRAME_3, SPR_JET_STREAM_ANIM_FRAME_4, SPR_JET_STREAM_ANIM_FRAME_5 });
    static CMatrixEffectLinkedSpriteAnim* CreateLinkedSpriteAnim(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, float width, dword color, int frame_delay = 1, bool camera_oriented = true, const std::vector<int>& sprites_num = { SPR_COOLING_FAN_ANIM_FRAME_1, SPR_COOLING_FAN_ANIM_FRAME_2, SPR_COOLING_FAN_ANIM_FRAME_3, SPR_COOLING_FAN_ANIM_FRAME_4, SPR_COOLING_FAN_ANIM_FRAME_5, SPR_COOLING_FAN_ANIM_FRAME_6, SPR_COOLING_FAN_ANIM_FRAME_7, SPR_COOLING_FAN_ANIM_FRAME_8, SPR_COOLING_FAN_ANIM_FRAME_9, SPR_COOLING_FAN_ANIM_FRAME_10 });
    static void            CreateShleif(SEffectHandler* eh);
    static CMatrixEffect*  CreateRepair(const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, float seek_radius, CMatrixMapStatic* skip, ESpriteTextureSort sprite_spot = SPR_REPAIRER_SPOT);


    static void DrawBegin();
    static void DrawEnd();


    virtual void BeforeDraw() = 0;
    virtual void Draw() = 0;
    virtual void Tact(float step) = 0;
    virtual void Release() = 0;

    virtual int  Priority() = 0;    // priority of effect

    bool         IsDIP() const { return FLAG(m_Flags, EFFF_DIP); }
};

inline  void SEffectHandler::Rebase()
{
    if(effect) effect->SetHandler(this);
}

inline  void SEffectHandler::Unconnect()
{
DTRACE();
#ifdef _DEBUG
    ASSERT(effect);
    if(effect->GetHandler() == this)
    {
        effect->SetHandler(nullptr);
    }
    else
    {
        _asm int 3
    }
    effect = nullptr;
#else
    if(effect) 
    {
        effect->SetHandler(nullptr);
        effect = nullptr;
    }
#endif
}


enum EEffectSpawnerType
{
    EST_SMOKE       = 1,
    EST_FIRE        = 2,
    EST_SOUND       = 3,
    EST_LIGHTENING  = 4,
};

struct SpawnEffectProps;

void EffectSpawnerFire(SpawnEffectProps* props);
void EffectSpawnerSmoke(SpawnEffectProps* props);
void EffectSpawnerSound(SpawnEffectProps* props);
void EffectSpawnerLightening(SpawnEffectProps* props);

void SpawnEffectLighteningDestructor(SpawnEffectProps* props);

struct SpawnEffectProps
{
    typedef void (*SEP_DESTRUCTOR)(SpawnEffectProps* props);
    typedef void (*EFFECT_SPAWNER)(SpawnEffectProps* props);

    int         m_Size = 0;                // structure size
    EEffectType m_Type = EFFECT_UNDEFINED; // effect type
    D3DXVECTOR3 m_Pos = { 0.0f,0.0f,0.0f };
    SEP_DESTRUCTOR m_Destructor = nullptr;
    EFFECT_SPAWNER m_Spawner = nullptr;
};

struct SpawnEffectSmoke : public SpawnEffectProps
{
    SpawnEffectSmoke()
    {
        memset(this, 0, sizeof(SpawnEffectSmoke));
        m_Size = sizeof(SpawnEffectSmoke);
        m_Spawner = EffectSpawnerSmoke;
    }

    float m_ttl = 0.0f;
    float m_puffttl = 0.0f;
    float m_spawntime = 0.0f;
    dword m_color = 0;
    bool  m_IsBright = false; //Маркер выбора текстуры спрайта с подсвечивающим рендером (более яркая)
    float m_speed = 0.0f;
};

struct SpawnEffectFire : public SpawnEffectProps
{
    SpawnEffectFire()
    {
        memset(this, 0, sizeof(SpawnEffectFire));
        m_Size = sizeof(SpawnEffectFire);
        m_Spawner = EffectSpawnerFire;
    }

    float m_ttl = 0.0f;
    float m_puffttl = 0.0f;
    float m_spawntime = 0.0f;
    float m_dispfactor = 0.0f;
    bool  m_IsBright = false; //Маркер выбора текстуры спрайта с подсвечивающим рендером (более яркая)
    float m_speed = 0.0f;
};

struct SpawnEffectSound : public SpawnEffectProps
{
    SpawnEffectSound() 
    {
        //memset(this, 0, sizeof(SpawnEffectSound));
        m_Size = sizeof(SpawnEffectSound);
        m_Spawner = EffectSpawnerSound;
    }

    float m_pan0 = 0.0f, m_pan1 = 0.0f;
    float m_vol0 = 0.0f, m_vol1 = 0.0f;
    float m_attn = 0.0f;
    wchar m_name[32] = { L"" };
    //bool m_looped = false;
};

struct SpawnEffectLightening : public SpawnEffectProps
{
    SpawnEffectLightening()
    {
        memset(this, 0, sizeof(SpawnEffectLightening));
        m_Size = sizeof(SpawnEffectLightening);
        m_Spawner = EffectSpawnerLightening;
        m_Destructor = SpawnEffectLighteningDestructor;
    }

    union
    {
        SpawnEffectLightening* m_Pair = nullptr;
        CWStr*                 m_Tag;// = nullptr;
    };

    float          m_ttl = 0.0f;
    float          m_Width = 0.0f;
    float          m_Dispers = 0.0f;   // if < 0 then non host effect. not produced
    dword          m_Color = 0;

    SEffectHandler m_Effect;
};

class CEffectSpawner : public CMain
{
    SpawnEffectProps* m_Props = nullptr;
    int               m_NextTime = 0;
    int               m_TTL = 0;
    int               m_MIN_period = 0;
    int               m_MAX_period = 0;

    CMatrixMapStatic* m_Under = nullptr;

public:
    CEffectSpawner(int minperiod, int maxperiod, int ttl, SpawnEffectProps* props);
    ~CEffectSpawner();

    SpawnEffectProps* Props() { return m_Props; }
    CMatrixMapStatic* GetUnder() { return m_Under; };

    bool OutOfTime() const;
    void Tact(float ms);
};


#include "MatrixEffectExplosion.hpp"
#include "MatrixEffectSmokeAndFire.hpp"
#include "MatrixEffectFirePlasma.hpp"
#include "MatrixEffectLandscapeSpot.hpp"
#include "MatrixEffectMovingObject.hpp"
#include "MatrixEffectSelection.hpp"
#include "MatrixEffectPath.hpp"
#include "MatrixEffectPointLight.hpp"
#include "MatrixEffectKonus.hpp"
#include "MatrixEffectFlame.hpp"
#include "MatrixEffectBigBoom.hpp"
#include "MatrixEffectLightening.hpp"
#include "MatrixEffectBillboard.hpp"
#include "MatrixEffectZahvat.hpp"
#include "MatrixEffectElevatorField.hpp"
#include "MatrixEffectDust.hpp"
#include "MatrixEffectMoveTo.hpp"
#include "MatrixEffectShleif.hpp"
#include "MatrixEffectRepair.hpp"

#include "MatrixEffectWeapon.hpp"
