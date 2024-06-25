// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#ifndef MATRIX_CONFIG_INCLUDE
#define MATRIX_CONFIG_INCLUDE

#include "Effects/MatrixEffect.hpp"
#include "MatrixMapStatic.hpp"
#include <vector>
#include <bitset>

#define DAMAGE_PER_TIME     10000
#define SECRET_VALUE        1000000.0f
#define LOGIC_TACT_PERIOD   10 //В миллисекундах, итого имеем 100 логических тактов в секунду

struct SStringPair
{
    //Если объявить заранее, то на этапе первичного заполнения конфига будут крашить, хз
    CWStr key = CWStr(L"");
    CWStr val = CWStr(L"");
};

enum ERes
{
    TITAN,
    ELECTRONICS,
    ENERGY,
    PLASMA,

    MAX_RESOURCES
};

enum ERobotModuleType
{
    MRT_EMPTY = 0,
    MRT_CHASSIS = 1,
    MRT_WEAPON = 2,
    MRT_HULL = 3,
    MRT_HEAD = 4
};

enum ERobotModuleKind
{
    RUK_EMPTY = 0, //Если слот модуля пуст (модуль не выбран)

    //Корпуса
    RUK_HULL_MONOSTACK        = 1,
    RUK_HULL_BIREX            = 2,
    RUK_HULL_DIPLOID          = 3,
    RUK_HULL_PARAGON          = 4,
    RUK_HULL_TRIDENT          = 5,
    RUK_HULL_FULLSTACK        = 6,

    //Шасси
    RUK_CHASSIS_PNEUMATIC      = 1,
    RUK_CHASSIS_WHEEL          = 2,
    RUK_CHASSIS_TRACK          = 3,
    RUK_CHASSIS_HOVERCRAFT     = 4,
    RUK_CHASSIS_ANTIGRAVITY    = 5,

    //Головы
    RUK_HEAD_BLOCKER           = 1,
    RUK_HEAD_DYNAMO            = 2,
    RUK_HEAD_LOCATOR           = 3,
    RUK_HEAD_FIREWALL          = 4,
    RUK_HEAD_REPAIRATOR        = 5, //Оригинально назывался RUK_HEAD_RAPID
    RUK_HEAD_DESIGN            = 6,
    RUK_HEAD_SPEAKER           = 7,

    //Пилоны под оружие
    RUK_WEAPON_PYLON_1         = 0,
    RUK_WEAPON_PYLON_2         = 1,
    RUK_WEAPON_PYLON_3         = 2,
    RUK_WEAPON_PYLON_4         = 3,
    RUK_WEAPON_PYLON_5         = 4,

    RUK_WEAPON_PYLONS_COUNT    = 5,

    //Оружие
    RUK_WEAPON_MACHINEGUN      = 1,
    RUK_WEAPON_CANNON          = 2,
    RUK_WEAPON_MISSILES        = 3,
    RUK_WEAPON_FLAMETHROWER    = 4,
    RUK_WEAPON_MORTAR          = 5,
    RUK_WEAPON_LASER           = 6,
    RUK_WEAPON_BOMB            = 7,
    RUK_WEAPON_PLASMAGUN       = 8,
    RUK_WEAPON_DISCHARGER      = 9,
    RUK_WEAPON_REPAIRER        = 10,
};

enum ETurretKind
{
    TURRET_LIGHT_CANNON = 1,
    TURRET_HEAVY_CANNON,
    TURRET_LASER_CANNON,
    TURRET_MISSILE_CANNON,

    TURRET_KINDS_TOTAL = 4
};

//Дебилы, как всегда, сохраняли данные о зданиях в файле карты в виде чисел, а не строк, поэтому нумерация здесь нахуй сбита (относительно последовательности ресурсов в игре) и без правки всех карт изменена быть не может
//В противном случае все заданные заводы на картах изменят тип
enum EBuildingType
{
    BUILDING_BASE = 0,
    BUILDING_TITAN = 1,
    BUILDING_ELECTRONIC = 3,
    BUILDING_ENERGY = 4,
    BUILDING_PLASMA = 2,
    BUILDING_REPAIR = 5,

    BUILDING_KINDS_TOTAL
};

enum EKeyAction
{
    KA_CAM_SCROLL_LEFT,
    KA_CAM_SCROLL_RIGHT,
    KA_CAM_SCROLL_UP,
    KA_CAM_SCROLL_DOWN,

    KA_CAM_SCROLL_LEFT_ALT,
    KA_CAM_SCROLL_RIGHT_ALT,
    KA_CAM_SCROLL_UP_ALT,
    KA_CAM_SCROLL_DOWN_ALT,

    KA_CAM_ROTATE_LEFT,
    KA_CAM_ROTATE_RIGHT,
    KA_CAM_ROTATE_UP,
    KA_CAM_ROTATE_DOWN,

    KA_CAM_SPEED_UP,

    KA_UNIT_FORWARD,
    KA_UNIT_BACKWARD,
    KA_ROBOT_STRAFE_LEFT,
    KA_ROBOT_STRAFE_RIGHT,
    KA_ROBOT_ROTATE_LEFT,
    KA_ROBOT_ROTATE_RIGHT,
    KA_FLYER_STRAFE_LEFT,
    KA_FLYER_STRAFE_RIGHT,
    KA_FLYER_UP,
    KA_FLYER_DOWN,

    KA_UNIT_FORWARD_ALT,
    KA_UNIT_BACKWARD_ALT,
    KA_ROBOT_STRAFE_LEFT_ALT,
    KA_ROBOT_STRAFE_RIGHT_ALT,
    KA_ROBOT_ROTATE_LEFT_ALT,
    KA_ROBOT_ROTATE_RIGHT_ALT,
    KA_FLYER_STRAFE_LEFT_ALT,
    KA_FLYER_STRAFE_RIGHT_ALT,
    KA_FLYER_UP_ALT,
    KA_FLYER_DOWN_ALT,

    KA_UNIT_SET_GUN_1,
    KA_UNIT_SET_GUN_2,
    KA_UNIT_SET_GUN_3,
    KA_UNIT_SET_GUN_4,
    KA_UNIT_SET_GUN_5,
    KA_UNIT_REAR_VIEW,

    KA_FIRE,
    KA_AUTO,

    KA_SHIFT,
    KA_CTRL,
    KA_ALL_UNITS_SELECT,

    KA_ROTATE_LEFT_ALT,
    KA_ROTATE_RIGHT_ALT,

    KA_MINIMAP_ZOOMIN,
    KA_MINIMAP_ZOOMOUT,

    KA_CAM_SETDEFAULT,
    KA_TAKE_SCREENSHOT,
    KA_SAVE_SCREENSHOT,
    KA_GAME_PAUSED,

    KA_AUTOORDER_CAPTURE,
    KA_AUTOORDER_ATTACK,
    KA_AUTOORDER_DEFEND,

    KA_ORDER_MOVE,
    KA_ORDER_STOP,
    KA_ORDER_CAPTURE,
    KA_ORDER_PATROL,
    KA_ORDER_EXPLODE,
    KA_ORDER_REPAIR,
    KA_ORDER_ATTACK,
    KA_ORDER_ROBOT_SWITCH1,
    KA_ORDER_ROBOT_SWITCH2,

    KA_ORDER_CANCEL,

    KA_UNIT_BOOM,
    KA_UNIT_ENTER, //(! if not dialog mode)
    KA_UNIT_ENTER_ALT, //(! if not dialog mode)

    KA_GATHERING_POINT,
    KA_BUILD_ROBOT,
    KA_BUILD_ROBOT_START,
    KA_BUILD_ROBOT_QUANTITY_UP,
    KA_BUILD_ROBOT_QUANTITY_DOWN,
    KA_BUILD_ROBOT_CHOOSE_LEFT,
    KA_BUILD_ROBOT_CHOOSE_RIGHT,
    KA_BUILD_TURRET,
    KA_DISMANTLE_TURRET,
    KA_CALL_REINFORCEMENTS,

    KA_TURRET_CANNON,
    KA_TURRET_GUN,
    KA_TURRET_LASER,
    KA_TURRET_ROCKET,

    KA_LAST
};

enum ETimings
{
    RESOURCE_TITAN,
    RESOURCE_ELECTRONICS,
    RESOURCE_ENERGY,
    RESOURCE_PLASMA,
    RESOURCE_BASE,

    UNIT_ROBOT,
    UNIT_FLYER,

    TIMING_LAST
};

enum ESide
{
    NEUTRAL_SIDE = 0,
    PLAYER_SIDE = 1,
    BLAZER_SIDE = 2,
    KELLER_SIDE = 3,
    TERRON_SIDE = 4,

    TOTAL_SIDES,

    //По нумерации должны идти сразу за последней стороной
    REINFORCEMENTS_TEMPLATES = 5,
    ROBOTS_SPAWNER_TEMPLATES = 6
};

enum EAnimation
{
    ANIMATION_OFF = 0,
    ANIMATION_STAY,

    ANIMATION_BEGIN_MOVE,
    ANIMATION_MOVE,
    ANIMATION_END_MOVE,

    ANIMATION_BEGIN_MOVE_BACK,
    ANIMATION_MOVE_BACK,
    ANIMATION_END_MOVE_BACK,

    ANIMATION_BEGIN_ROTATE_LEFT,
    ANIMATION_ROTATE_LEFT,
    ANIMATION_END_ROTATE_LEFT,

    ANIMATION_BEGIN_ROTATE_RIGHT,
    ANIMATION_ROTATE_RIGHT,
    ANIMATION_END_ROTATE_RIGHT,

    ANIMATIONS_COUNT //Это всегда должно оставаться внизу
};

struct SAnimsToNames
{
    EAnimation num = ANIMATION_OFF;
    const wchar* name = L"";
};

static SAnimsToNames AnimsToNames[] =
{
    { ANIMATION_OFF,                L""                               },
    { ANIMATION_STAY,               ANIMATION_NAME_STAY               },

    { ANIMATION_BEGIN_MOVE,         ANIMATION_NAME_BEGIN_MOVE         },
    { ANIMATION_MOVE,               ANIMATION_NAME_MOVE               },
    { ANIMATION_END_MOVE,           ANIMATION_NAME_END_MOVE           },

    { ANIMATION_BEGIN_MOVE_BACK,    ANIMATION_NAME_BEGIN_MOVE_BACK    },
    { ANIMATION_MOVE_BACK,          ANIMATION_NAME_MOVE_BACK          },
    { ANIMATION_END_MOVE_BACK,      ANIMATION_NAME_END_MOVE_BACK      },

    { ANIMATION_BEGIN_ROTATE_LEFT,  ANIMATION_NAME_BEGIN_ROTATE_LEFT  },
    { ANIMATION_ROTATE_LEFT,        ANIMATION_NAME_ROTATE_LEFT        },
    { ANIMATION_END_ROTATE_LEFT,    ANIMATION_NAME_END_ROTATE_LEFT    },

    { ANIMATION_BEGIN_ROTATE_RIGHT, ANIMATION_NAME_BEGIN_ROTATE_RIGHT },
    { ANIMATION_ROTATE_RIGHT,       ANIMATION_NAME_ROTATE_RIGHT       },
    { ANIMATION_END_ROTATE_RIGHT,   ANIMATION_NAME_END_ROTATE_RIGHT   }
};

struct SCamParam
{
    float m_CamMouseWheelStep = 0.0f;
    float m_CamRotSpeedX = 0.0f;
    float m_CamRotSpeedZ = 0.0f;
    float m_CamRotAngleMin = 0.0f;
    float m_CamRotAngleMax = 0.0f;
    float m_CamDistMin = 0.0f;
    float m_CamDistMax = 0.0f;
    float m_CamDistParam = 0.0f;
    float m_CamAngleParam = 0.0f;
    float m_CamHeight = 0.0f;
};

struct SGammaVals
{
    float brightness = 0.5f;
    float contrast = 0.5f;
    float gamma = 1.0f;
};

enum EMovingAnimType
{
    NO_MOVING_ANIM = -1,
    NORMAL_MOVING_ANIM = 0,
    SPEED_DEPENDENT_MOVING_ANIM = 1
};

enum
{
    CAMERA_STRATEGY,
    CAMERA_ARCADE,

    CAMERA_PARAM_CNT
};

#define ROBOT_HULLS_COUNT g_Config.m_RobotHullsCount
struct SRobotHullsWeaponPylonData
{
    int id = 0;                    //Id матрицы для установки оружия на модели корпуса
    bool inverted_model = false;   //Маркер о необходимости инвертировать модель установленного в данный пилон оружия (требуется для оружия на левой стороне корпуса)
    std::bitset<128> fit_weapon = { 0 };   //Какое оружие может быть установлено в данный пилон, до 128 пушек (нумерация с 1)
    byte constructor_slot_num = 0; //Номер слота в конструкторе, который соответствует данному пилону (нумерация с 0)
};
struct SRobotHullsConsts
{
    byte for_player_side = 0;     //Маркер, означающий, что данный модуль разрешён для использования игроком в конструкторе

    CWStr name;                   //Название модуля
    CWStr robot_part_name;        //Обозначение модуля, из которого собирается название робота (есть только у корпуса, шасси и головы)
    CWStr short_designation;      //Обозначение данного модуля в строке шаблона робота

    CWStr chars_description;      //Описание характеристик корпуса
    CWStr art_description;        //Лоровое (художественное) описание корпуса

    std::bitset<RUK_WEAPON_PYLONS_COUNT> constructor_weapon_slots_used = { 0 }; //Используется для быстрого определения, какие из орудийных слотов в конструкторе должны быть открыты для данного корпуса (нумерация с 0)
    std::vector<SRobotHullsWeaponPylonData> weapon_pylon_data;

    CWStr constructor_button_name;
    CWStr constructor_image_name;
    CWStr constructor_label_name;
    CWStr hull_sound_name;
    ESound hull_sound_num;         //Номер звукового объекта из hull_sound_name в общем массиве m_Sound
    CWStr model_path;

    //Основные характеристики
    float structure;
    float rotation_speed;

    int module_hit_effect = WEAPON_NONE;

    int cost_titan = 0;
    int cost_electronics = 0;
    int cost_energy = 0;
    int cost_plasma = 0;
};

struct SRobotSpriteAnims
{
    byte matrix_id = 0;               //Id матрицы на модели, к которой необходимо прикрепить спрайтовую анимацию
    //CWStr matrix_name = (CWStr)L""; //Название матрицы на модели, к которой необходимо прикрепить спрайтовую анимацию

    CWStr sprites_name = (CWStr)L"";  //Спрайты для анимации
    std::vector<int> sprites_num;
    byte sprites_count = 0;

    float length = 0.0f; //Длина растяжения спрайта вдоль линии направления
    float width = 0.0f;  //Ширина растяжения спрайта
    dword color = 0xFFFFFFFF; //В формате ABGR, в который конвертится из строки RGBA
    int   frame_delay = 1;
    bool camera_oriented = true; //Если включено, то все спрайты будут перед отрисовкой автоматически ориентироваться "лицом" в камеру
};

#define ROBOT_CHASSIS_COUNT g_Config.m_RobotChassisCount
struct SRobotChassisTraceMatrix
{
    byte matrix_id = 0;               //Id матрицы на модели, к которой необходимо линковать след шага
    //CWStr matrix_name = (CWStr)L""; //Название матрицы на модели, к которой необходимо линковать след шага
};
struct SRobotChassisGroundTrace
{
    int trace_num = 0;               //Запоминаем сюда номер элемента в массиве следов m_SpotProperties

    CWStr texture_path = (CWStr)L"";
    float texture_scale = 0.0f;

    dword trace_redraw_distance = 0; //Частота отрисовки текстур на земле (актуально только для колёсных и гусеничных шасси, т.к. шагающее оставляет следы под матрицами на ступнях)
    dword trace_duration = 0;
};
struct SRobotChassisConsts
{
    byte for_player_side;         //Маркер, означающий, что данный модуль разрешён для использования игроком в конструкторе

    CWStr name;                   //Название шасси
    CWStr robot_part_name;        //Обозначение модуля, из которого собирается название робота (есть только у корпуса, шасси и головы)
    CWStr short_designation;      //Обозначение данного шасси в строке шаблона робота

    CWStr chars_description;      //Описание характеристик шасси
    CWStr art_description;        //Лоровое (художественное) описание шасси

    CWStr constructor_button_name;
    CWStr constructor_image_name;
    CWStr constructor_label_name;
    CWStr arcade_enter_sound_name;
    ESound arcade_enter_sound_num; //Номер звукового объекта из arcade_enter_sound_name в общем массиве m_Sound (играется в момент включения режима прямого управления роботом)
    std::vector<CWStr> move_out_sound_name; //Звуки шасси, используемые во время отдачи роботу приказа на движение или патруль
    CWStr model_path = (CWStr)L"";

    //Основные характеристики
    float structure = 10.0f;
    float move_speed = 1.0f;
    float strafe_speed = 1.0f;        //Скорость стрейфа не должна быть выше move_speed
    float anim_move_speed = 1.0f;     //Скорость прокрутки анимации движения шасси, если таковая анимация у шасси имеется
    float move_uphill_factor = 0.7f;
    float move_downhill_factor = 2.5f;
    float move_in_water_factor = 0.5f;
    float rotation_speed = 0.03f;
    byte passability = 0;             //Проходимость задаётся числом от 0 до 5, от худшего к лучшему: Worst, Bad, Average, Good, Best, Cheat (Cheat полностью отключает коллизию с любыми объектами)

    bool is_walking = false;                               //Маркер шагающего шасси, под каждой ступней рисуются одиночные следы
    std::vector<SRobotChassisTraceMatrix> trace_matrix;
    bool is_rolling = false;                               //Маркер колёсной/гусеничной техники, означает, что под шасси нужно рисовать непрерывные следы
    SRobotChassisGroundTrace ground_trace;

    EMovingAnimType moving_anim_type = NORMAL_MOVING_ANIM; //NO_MOVING_ANIM - у шасси нет специальной анимации движения, а потому код игнорирует её переключение; NORMAL_MOVING_ANIM - анимация движения играется независимо от скорости движения робота; SPEED_DEPENDENT_MOVING_ANIM - скорость прокрутки анимации движения привязана к скорости движения робота
    bool is_hovering = false;                              //Маркер парящего над землёй шасси, робот с данным шасси не может провалиться под воду (под карту) (для "Экроплана" и "Антиграва")
    bool is_dust_trace = false;                            //Шасси будет оставлять под собой пылевой след (для "Экроплана")

    std::vector<SRobotSpriteAnims> jet_stream; //Количество и данные для реактивных следов, которые нужно добавить на модель шасси (для "Антиграва")
    std::vector<SRobotSpriteAnims> linked_sprite_anim; //Количество и данные для декоративных спрайтовых анимаций, которые нужно добавить на модель шасси (для "Антиграва")

    int module_hit_effect = WEAPON_NONE;

    int cost_titan = 0;
    int cost_electronics = 0;
    int cost_energy = 0;
    int cost_plasma = 0;
};

#define ROBOT_HEADS_COUNT g_Config.m_RobotHeadsCount
struct SRobotHeadsConsts
{
    byte for_player_side;         //Маркер, означающий, что данный модуль разрешён для использования игроком в конструкторе

    CWStr name;                   //Название модуля
    CWStr robot_part_name;        //Обозначение модуля, из которого собирается название робота (есть только у корпуса, шасси и головы)
    CWStr short_designation;      //Обозначение данного модуля в строке шаблона робота

    CBlockPar* effects = nullptr; //Блок с параметрами эффектов (было проще запомнить сюда указатель весь блок, чем менять их подгрузку)

    CWStr effects_description;    //Описание эффектов головы
    CWStr art_description;        //Лоровое (художественное) описание модуля

    CWStr constructor_button_name;
    CWStr constructor_image_name;
    CWStr constructor_label_name;
    CWStr model_path;

    int module_hit_effect = WEAPON_NONE;

    int cost_titan = 0;
    int cost_electronics = 0;
    int cost_energy = 0;
    int cost_plasma = 0;
};

#define ROBOT_WEAPONS_COUNT g_Config.m_RobotWeaponsCount
struct SRobotWeaponsConsts
{
    byte for_player_side;         //Маркер, означающий, что данное оружие разрешено для использования игроком в конструкторе

    CWStr name;                   //Название модуля
    CWStr short_designation;      //Обозначение данного оружия в строке шаблона робота

    CWStr chars_description;      //Описание характеристик оружия
    CWStr art_description;        //Лоровое (художественное) описание оружия

    CWStr constructor_button_name;
    CWStr constructor_image_name;
    CWStr constructor_label_name;
    CWStr status_panel_image;
    CWStr model_path;

    int weapon_type;

    bool is_module_bomb = false;
    bool is_module_repairer = false;

    //Для ремонтника - точки, по которым рисуется линия эффекта свечения
    int dev_start_id = 0;
    int dev_end_id = 0;

    float strength = 100.0f; //Параметр относительной силы данного орудийного модуля, по которой его опасность оценивают боты

    int heating_speed;
    int cooling_speed;
    int cooling_delay;

    int module_hit_effect = WEAPON_NONE;

    int cost_titan = 0;
    int cost_electronics = 0;
    int cost_energy = 0;
    int cost_plasma = 0;
};

struct STurretsConsts
{
    byte for_player_side = 0;     //Маркер, означающий, что данная турель разрешена для постройки игроком

    CWStr name;                   //Название турели
    CWStr chars_description;      //Описание характеристик оружия
    CWStr model_path;

    float structure = 500.0f;     //Запас HP (параметр для турелей)
    float strength = 1000.0f;     //Параметр относительной силы турели, по которой её опасность оценивают боты
    int construction_time = 15000;
    int deconstruction_time = 15000;

    struct STurretGun
    {
        int matrix_id = 0;
        //CWStr matrix_name = (CWStr)L"";
        int weapon_type = WEAPON_NONE;
    };
    std::vector<STurretGun> guns; //Количество стволов турели, в которых записаны их матрицы и соответствующие типы орудий
    int guns_async_time;          //Время рассинхрона между выстрелами турели с несколькими орудиями

    float seek_target_range;      //Дальность предварительного выбора цели (применяется только для турелей)

    float rotation_speed = 0.0f;  //Cкорость поворота турели за логический такт в радианах
    float vertical_speed = 0.0f;  //Cкорость вертикального наведения орудия турели за логический такт в радианах
    float highest_vertical_angle; //Углы вертикального наведения
    float lowest_vertical_angle;

    int mount_part_hit_effect = WEAPON_NONE;
    int gun_part_hit_effect = WEAPON_NONE;

    int cost_titan = 0;
    int cost_electronics = 0;
    int cost_energy = 0;
    int cost_plasma = 0;
};

enum //EPrimaryWeaponEffect
{
    EFFECT_CANNON = 1,
    EFFECT_ROCKET_LAUNCHER,
    EFFECT_MORTAR,
    EFFECT_REPAIRER,
    EFFECT_BOMB
};

enum //ESecondaryWeaponEffect
{
    SECONDARY_EFFECT_ABLAZE = 1,
    SECONDARY_EFFECT_SHORTED_OUT
};

struct SWeaponsConsts
{
    CWStr type_name = (CWStr)L"";
    byte primary_effect = 0;   //Маркер и номер для оружия, являющегося основным эффектом (различные выстрелы)
    byte secondary_effect = 0; //Маркер и номер (SECONDARY_EFFECT_ABLAZE, SECONDARY_EFFECT_SHORTED_OUT и т.д.) для оружия, являющегося несамостоятельным дополнительным эффектом
    int effect_priority = 0;   //Приоритет эффекта имеет смысл, когда необходимо выбрать, какой из эффектов одного типа нужно наложить или при выборе замены уже наложенного эффекта

    bool is_bomb = false;
    bool is_repairer = false;

    //Общий массив с информацией обо всех оружейных эффектах (используются как роботами, так вертолётами и турелями)
    struct SObjectsVaraity
    {
        float to_robots = 0.0f;
        float to_flyers = 0.0f;
        float to_turrets = 0.0f;
        float to_buildings = 0.0f;
        float to_objects = 0.0f;
    };
    SObjectsVaraity damage;               //Обычный урон, наносимый данным оружием по врагам
    //SObjectsVaraity friendly_damage;    //Урон, наносимый данным оружием по дружественным целям (как правило, всё равно перемножается на ситуативный коэффициент, так что особого смысла не имеет)
    SObjectsVaraity non_lethal_threshold; //При нанесении урона данным оружием, HP атакуемого объекта не будет опускаться ниже данного значения (задавать выше 0 только для нелетального оружия / типа урона)

    float shots_delay = 0.0f;              //Задержка в ms между выстрелами
    float shot_range = 0.0f;               //Дальность стрельбы

    //Для горения объектов у каждого оружия есть свой отдельный блок
    struct SMapObjectsIgnition
    {
        bool is_present = false;

        int priority = 0;

        int duration_per_hit = 0; //Какая продолжительность горения набрасывается на объект за каждое попадание
        int burning_starts_at = 0; //Начало горения объекта по общему накрученному таймеру (до этого времени он просто дымится)
        int max_duration = 0;

        CWStr sprites_name = (CWStr)L"";
        std::vector<int> sprites_num;
        byte sprites_count = 0;

        ESound burning_sound_num = S_NONE;
        CWStr burning_sound_name = (CWStr)L"";

    } map_objects_ignition;

    //Используются для всяких эффектов и подробных параметров оружия
    CWStr projectile_model_path = (CWStr)L"";
    float projectile_start_velocity = 0.0f;
    float projectile_full_velocity = 0.0f;
    int   projectile_full_velocity_reach = 0;
    float projectile_acceleration_coef = 0.0f;
    float projectile_target_capture_angle_cos = 0.0f; //Записывается сюда как косинус угла, используется для самонаведения ракеты
    float projectile_target_capture_angle_sin = 0.0f; //Для суммирования косинусов перед запуском ракеты
    float projectile_homing_speed = 0.0f; //Используется для самонаведения ракеты
    float projectile_splash_radius = 0.0f;
    int   projectile_max_lifetime = 0;

    int   sprites_lenght = 0;
    int   sprites_width = 0;

    int   contrail_width = 0;
    int   contrail_duration = 0;
    int   contrail_fire_effect_starts = 0;
    dword hex_BGRA_sprites_color = 0;

    float light_radius = 0.0f;
    float light_duration = 0.0f;
    dword hex_BGRA_light_color = 0;

    SFloatRGBColor close_color_rgb = { 0.0f, 0.0f, 0.0f };
    SFloatRGBColor far_color_rgb = { 0.0f, 0.0f, 0.0f };

    //Для примера объявления обычных массивов дефолтным аргументом
    //void A(const int(&arr)[2] = { 2, 2 }) {}

    struct SWeaponExtraEffect {
        int type = -1;

        int duration_per_hit = 0;
        int max_duration = 0;
    };
    std::vector<SWeaponExtraEffect> extra_effects; //Массив с номерами дополнительных эффектов, которые накладывает данное оружие при нанесении урона

    //Спрайты, использующиеся для эффектов и анимаций оружия (могут использоваться отдельные спрайты, либо наборы для спрайтовых анимаций)
    struct SSpriteSet{
        CWStr sprites_name = (CWStr)L"";
        std::vector<int> sprites_num;
        byte sprites_count = 0;
    };
    std::vector<SSpriteSet> sprite_set; //Также используется для эффектов, использующих всего один, либо несколько отдельных спрайтов

    CWStr shot_sound_name = (CWStr)L"";
    ESound shot_sound_num = S_NONE;
    bool shot_sound_looped = false; //Маркер о необходимости непрерывного воспроизведения звука выстрела, пока зажата гашетка, без необходимости разделять его на отдельные выстрелы (как для огнемёта лазера, разрядника и ремонтника)
    CWStr hit_sound_name = (CWStr)L"";
    ESound hit_sound_num = S_NONE;
    bool explosive_hit = false;
};

//Не забывать обновлять, ResetMenu и выборе модулей по клавишам с клавиатуры (//Были нажаты цифровые клавиши 1-9 (по местной нумерации 2-10)), чтобы не крашила при невалиде в шаблоне!


struct SBuildingsConsts {
    CWStr type_name = (CWStr)L"";
    float structure = 50000.0f;

    CWStr name = (CWStr)L"";
    CWStr description = (CWStr)L"";
};

//Не забыть про лимиты установки модулей MR_MAX_MODULES и RUK_WEAPON_PYLONS_COUNT
class CMatrixConfig : public CMain
{
public:
    int m_RobotHullsCount = 0;
    std::vector<SRobotHullsConsts> m_RobotHullsConsts;
    int m_RobotChassisCount = 0;
    std::vector<SRobotChassisConsts> m_RobotChassisConsts;
    int m_RobotHeadsCount = 0;
    std::vector<SRobotHeadsConsts> m_RobotHeadsConsts;
    int m_RobotWeaponsCount = 0;
    std::vector<SRobotWeaponsConsts> m_RobotWeaponsConsts;

    std::vector<STurretsConsts> m_TurretsConsts;
    std::vector<SBuildingsConsts> m_BuildingsConsts;
    std::vector<SWeaponsConsts> m_WeaponsConsts;

    SStringPair* m_Cursors = nullptr;
    int          m_CursorsCnt = 0;

    int          m_ReinforcementsTime = 0;

    //int m_PlayerRobotsCnt = 0;
    //int m_CompRobotsCnt = 0;

    // camera params

    SCamParam m_CamParams[CAMERA_PARAM_CNT];

    float m_CamBaseAngleZ = 0.0f; // GRAD2RAD(38.0f);
    float m_CamMoveSpeed = 1.05f;
    float m_CamMoveSpeedUp = 1.05f; //Суммируется с базовой m_CamMoveSpeed
    float m_CamInRobotForward0 = 10.0f;
    float m_CamInRobotForward1 = 30.0f;

    //float m_CamRotAngleMinInFlyer = 0.0f;
    //float m_CamDistMinInFlyer = 0.0f;

    SGammaVals m_GammaR, m_GammaG, m_GammaB;

    // params
    dword m_DIFlags = 0;
    //int m_TexTopMinSize = 32;
    //int m_TexTopDownScalefactor = 0;
    //int m_TexBotMinSize = 32;
    //int m_TexBotDownScalefactor = 0;

    bool m_ShowStencilShadows = true;
    bool m_ShowProjShadows = true;
    bool m_CannonsLogic = true;
    //bool m_LandTextures16 = false;

    bool m_LandTexturesGloss = true;
    bool m_SoftwareCursor = false;
    bool m_VertexLight = true;

    bool m_ObjTexturesGloss = true;
    //bool m_ObjTextures16 = true;

    bool m_IzvratMS = false;
    byte m_SkyBox = 1;                  // 0 - none, 1 - dds (low quality), 2 - png (high quality)
    byte m_DrawAllObjectsToMinimap = 2; // 0 - none, 1 - force yes, 2 - auto

    int  m_CaptureTimeErase = 750;
    int  m_CaptureTimePaint = 500;
    int  m_CaptureTimeRolback = 1500;

    int  m_KeyActions[KA_LAST] = { 0 };
    bool m_IsManualMissileControl = false; //Маркер активации режима ручного управления наведением ракет для робота под прямым управлением игрока

    int  m_Timings[TIMING_LAST] = { 0 };

    float m_RobotRadarRadius = 1000.0f;
    float m_FlyerRadarRadius = 1200.0f;

    EShadowType m_UnitShadows = SHADOW_STENCIL;
    EShadowType m_BuildingShadows = SHADOW_STENCIL;
    EShadowType m_ObjectShadows = SHADOW_PROJ_DYNAMIC;

    CMatrixConfig() = default;
    ~CMatrixConfig() = default;

    void Clear();

    void SetDefaults();
    void ReadParams();
    void ApplySettings(SRobotsSettings* set);

    bool IsManualMissileControl() { return m_IsManualMissileControl; }; //Состояние маркера активации режима ручного управления наведением ракет

    void ApplyGammaRamp();

    static const wchar* KeyActionCode2KeyName(const int& num);

    dword RGBAStringToABGRColorHEX(const CWStr* par)
    {
        //Собираем цвет из параметра RGBA в ABGR HEX
        CWStr par_color = par->GetStrPar(0, L":");
        int red = par_color.GetStrPar(0, L",").GetInt();
        int green = par_color.GetStrPar(1, L",").GetInt();
        int blue = par_color.GetStrPar(2, L",").GetInt();
        int alpha = int(255 * par->GetStrPar(1, L":").GetDouble());
        return (blue > 255 ? 255 : blue) | ((green > 255 ? 255 : green) << 8) | ((red > 255 ? 255 : red) << 16) | ((alpha > 255 ? 255 : alpha) << 24);
    }
};

extern CMatrixConfig g_Config;

inline int WeapName2Weap(const CWStr& weapon_name)
{
    for(int i = 0; i < (int)g_Config.m_WeaponsConsts.size(); ++i)
    {
        if(g_Config.m_WeaponsConsts[i].type_name == weapon_name) return i;
    }
    return WEAPON_NONE;
}

#endif