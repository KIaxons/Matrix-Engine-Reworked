// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "../stdafx.h"
#include "MatrixEffect.hpp"
#include "../MatrixMap.hpp"
#include "../MatrixObject.hpp"
#include "../MatrixRobot.hpp"
#include "../MatrixFlyer.hpp"
#include "../MatrixObjectCannon.hpp"
#include <math.h>

CMatrixEffectWeapon::CMatrixEffectWeapon(const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, dword user, FIRE_END_HANDLER handler, int type, int cooldown) :
    CMatrixEffect(), m_WeaponNum(type), m_User(user), m_Handler(handler), m_Pos(pos), m_Dir(dir)
{
    m_EffectType = EFFECT_WEAPON;
    m_WeaponCoefficient = g_WeaponDamageNormalCoef;

    m_WeaponDist = g_Config.m_WeaponsConsts[m_WeaponNum].shot_range;
    m_CoolDown = g_Config.m_WeaponsConsts[m_WeaponNum].shots_delay;
    m_ShotSoundType = g_Config.m_WeaponsConsts[m_WeaponNum].shot_sound_num;

    if(!g_Config.m_WeaponsConsts[m_WeaponNum].shot_sound_looped)
    {
        RESETFLAG(m_Flags, WEAPFLAGS_SND_OFF);
        RESETFLAG(m_Flags, WEAPFLAGS_SND_SKIP);
    }
    else
    {
        SETFLAG(m_Flags, WEAPFLAGS_SND_OFF);
        SETFLAG(m_Flags, WEAPFLAGS_SND_SKIP);
    }
}

CMatrixEffectWeapon::~CMatrixEffectWeapon()
{
    FireEnd();

    if(g_Config.m_WeaponsConsts[m_WeaponNum].primary_effect == WEAPON_EFFECT_FLAMETHROWER)
    {
        if(m_Effect.effect)
        {
            m_Effect.Unconnect();
        }
    }

    m_Effect.Release();

    if(m_Owner)
    {
        m_Owner->Release();
    }
}

void CMatrixEffectWeapon::WeaponHit(CMatrixMapStatic* hiti, const D3DXVECTOR3& pos, dword user, dword flags)
{
    CMatrixEffectWeapon* w = (CMatrixEffectWeapon*)user;

    bool give_damage = true;
    int prim_effect = g_Config.m_WeaponsConsts[w->m_WeaponNum].primary_effect;
    if(prim_effect == WEAPON_EFFECT_CANNON || prim_effect == WEAPON_EFFECT_ROCKET_LAUNCHER)
    {
        if(POW2(w->m_WeaponDist * w->m_WeaponCoefficient) < m_Dist2) give_damage = false;
    }

    bool odead = false;

    dword flags_add = 0;

    if(give_damage && IS_TRACE_STOP_OBJECT(hiti))
    {
        if(prim_effect == WEAPON_EFFECT_BOMB)
        {
            D3DXVECTOR3 dir(pos - w->m_Pos);
            D3DXVec3Normalize(&dir, &dir);
            odead = hiti->TakingDamage(w->m_WeaponNum, pos, dir, w->GetSideStorage(), w->GetOwner());
        }
        else odead = hiti->TakingDamage(w->m_WeaponNum, pos, w->m_Dir, w->GetSideStorage(), w->GetOwner());

        if(!odead)
        {
            if(hiti->IsRobot()) flags_add = FEHF_DAMAGE_ROBOT;
        }
    }
    else if(hiti == TRACE_STOP_WATER && prim_effect != WEAPON_EFFECT_FLAMETHROWER && prim_effect != WEAPON_EFFECT_BOMB)
    {
        CMatrixEffect::CreateConeSplash(pos, D3DXVECTOR3(0.0f, 0.0f, 1.0f), 10.0f, 5.0f, FSRND((float)M_PI), 1000.0f, 0xFFFFFFFF, true, m_SpriteTextures[SPR_WATER_SPLASH].tex);
    }

    if(odead) hiti = TRACE_STOP_NONE;

    if(w->m_Handler) w->m_Handler(hiti, pos, w->m_User, flags | flags_add);

    if(FLAG(flags, FEHF_LASTHIT)) w->Release();
}

void CMatrixEffectWeapon::Release()
{
    --m_Ref;

    if(m_Ref <= 0)
    {
        SetDIP();
        HDelete(CMatrixEffectWeapon, this, m_Heap);
    }
}

void CMatrixEffectWeapon::Tact(float step)
{
    if(m_Time < 0) m_Time += step;
    if(m_Time > 0 && !IsFire()) m_Time = 0;

    if(m_Sound != SOUND_ID_EMPTY) m_Sound = CSound::ChangePos(m_Sound, m_ShotSoundType, m_Pos);

    if(IsFire())
    {
        while(m_Time >= 0)
        {
            FireWeapon();
            SETFLAG(m_Flags, WEAPFLAGS_FIREWAS);

            m_Time -= m_CoolDown;
        }
    }
}

void CMatrixEffectWeapon::Modify(const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, const D3DXVECTOR3& speed)
{
    m_Pos = pos;
    m_Dir = dir;
    m_Speed = speed;

    switch(g_Config.m_WeaponsConsts[m_WeaponNum].primary_effect)
    {
        case WEAPON_EFFECT_MACHINEGUN:
        {
            if(m_Machinegun)
            {
                m_Machinegun->SetPos(pos, pos + dir * m_Machinegun->GetSpritesLenght(), dir);
            }

            break;
        }

        case WEAPON_EFFECT_PLASMAGUN:
        {
            if(m_Effect.effect && m_Effect.effect->GetType() == EFFECT_CONE)
            {
                ((CMatrixEffectCone*)m_Effect.effect)->Modify(pos, dir);
            }

            break;
        }

        case WEAPON_EFFECT_DISCHARGER:
        {
            if(m_Effect.effect && m_Effect.effect->GetType() == EFFECT_LIGHTENING)
            {
                D3DXVECTOR3 hitpos(m_Pos + m_Dir * m_WeaponDist * m_WeaponCoefficient);
                g_MatrixMap->Trace(&hitpos, m_Pos, hitpos, TRACE_ALL, m_Skip);
                ((CMatrixEffectLightening*)m_Effect.effect)->SetPos(m_Pos, hitpos);
            }

            break;
        }

        case WEAPON_EFFECT_LASER:
        {
            if(m_Laser)
            {
                D3DXVECTOR3 hitpos(m_Pos + m_Dir * m_WeaponDist * m_WeaponCoefficient);
                g_MatrixMap->Trace(&hitpos, m_Pos, hitpos, TRACE_ALL, m_Skip);

                m_Laser->SetPos(m_Pos, hitpos);
            }

            break;
        }

        default:
        {
            if(g_Config.m_WeaponsConsts[m_WeaponNum].is_repairer)
            {
                if(m_Repair)
                {
                    m_Repair->UpdateData(pos, dir);
                }
            }

            break;
        }
    }
}

void CMatrixEffectWeapon::FireWeapon()
{
    ++m_FireCount;

    if(m_ShotSoundType != S_NONE)
    {
        if(FLAG(m_Flags, WEAPFLAGS_SND_SKIP)) m_Sound = CSound::Play(m_Sound, m_ShotSoundType, m_Pos);
        else m_Sound = CSound::Play(m_ShotSoundType, m_Pos);
    }

    SWeaponsConsts* weapon_data = &g_Config.m_WeaponsConsts[m_WeaponNum];

    if(weapon_data->primary_effect)
    {
        switch(weapon_data->primary_effect)
        {
            case WEAPON_EFFECT_MACHINEGUN:
            {
                if(m_Machinegun) //Если эффект вспышки уже создан, то просто корректируем его позицию
                {
                    float ang = float(2.0 * M_PI / 4096.0) * (g_MatrixMap->GetTime() & 4095);
                    m_Machinegun->SetPos(m_Pos, m_Pos + m_Dir * m_Machinegun->GetSpritesLenght(), m_Dir, ang);
                }
                else if(weapon_data->sprite_set[MACHINEGUN_MUZZLE_FLASH_SPRITES_NUM].sprites_count) //Здесь создаём и линкуем к дулу пулемёта эффект вспышки от выстрелов
                {
                    float ang = float(2.0 * M_PI / 4096.0) * (g_MatrixMap->GetTime() & 4095);
                    m_Machinegun = HNew(m_Heap)
                        CMachinegun( m_Pos, m_Dir, ang,
                        weapon_data->sprites_lenght,   weapon_data->sprites_width,
                        weapon_data->fire_cone_lenght, weapon_data->fire_cone_radius,
                        weapon_data->hex_ABGR_sprites_color,
                        weapon_data->sprite_set[MACHINEGUN_MUZZLE_FLASH_SPRITES_NUM].sprites_num[0],
                        weapon_data->sprite_set[MACHINEGUN_MUZZLE_FLASH_SPRITES_NUM].sprites_num[1],
                        weapon_data->sprite_set[MACHINEGUN_MUZZLE_FLASH_SPRITES_NUM].sprites_num[2] );

                    if(!g_MatrixMap->AddEffect(m_Machinegun))
                    {
                        m_Machinegun = nullptr;
                    }
                }

                dword flags_add = FEHF_LASTHIT;

                D3DXVECTOR3 hit_pos;
                D3DXVECTOR3 splash;
                CMatrixMapStatic* s = g_MatrixMap->Trace(&hit_pos, m_Pos, m_Pos + m_Dir * m_WeaponDist * m_WeaponCoefficient, TRACE_ALL, m_Skip);

                if(s == TRACE_STOP_NONE)
                {
                    break;
                }

                //Попали в какую-то цель
                if(IS_TRACE_STOP_OBJECT(s))
                {
                    bool dead = s->TakingDamage(m_WeaponNum, hit_pos, m_Dir, GetSideStorage(), GetOwner());
                    SETFLAG(m_Flags, WEAPFLAGS_HITWAS);
                    
                    if(dead)
                    {
                        s = TRACE_STOP_NONE;
                    }

                    splash = -m_Dir;
                }
                //Попали в землю
                else if(s != TRACE_STOP_WATER && weapon_data->sprite_set[MACHINEGUN_GROUND_HIT_SPRITES_NUM].sprites_count)
                {
                    g_MatrixMap->GetNormal(&splash, hit_pos.x, hit_pos.y);
                    CMatrixEffect::CreateCone(nullptr, hit_pos, splash, weapon_data->ground_hit.radius_1, weapon_data->ground_hit.height_1, FSRND((float)M_PI), 300.0f, weapon_data->ground_hit.hex_ABGR_sprites_color, true, m_SpriteTextures[weapon_data->sprite_set[MACHINEGUN_GROUND_HIT_SPRITES_NUM].sprites_num[0]].tex);
                    if(weapon_data->sprite_set[MACHINEGUN_GROUND_HIT_SPRITES_NUM].sprites_count > 2 && FRND(1.0f) < 0.5f)
                    {
                        CMatrixEffect::CreateCone(nullptr, hit_pos, splash, weapon_data->ground_hit.radius_2, weapon_data->ground_hit.height_2, FSRND((float)M_PI), 300.0f, weapon_data->ground_hit.hex_ABGR_sprites_color, true, m_SpriteTextures[weapon_data->sprite_set[MACHINEGUN_GROUND_HIT_SPRITES_NUM].sprites_num[2]].tex);
                    }
                    else
                    {
                        CMatrixEffect::CreateCone(nullptr, hit_pos, splash, weapon_data->ground_hit.radius_2, weapon_data->ground_hit.height_2, FSRND((float)M_PI), 300.0f, weapon_data->ground_hit.hex_ABGR_sprites_color, true, m_SpriteTextures[weapon_data->sprite_set[MACHINEGUN_GROUND_HIT_SPRITES_NUM].sprites_num[1]].tex);
                    }
                }
                //Попали в воду
                else if(weapon_data->sprite_set[MACHINEGUN_WATER_HIT_SPRITES_NUM].sprites_count)
                {
                    splash = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
                    CMatrixEffect::CreateConeSplash(hit_pos, splash, weapon_data->water_hit.radius, weapon_data->water_hit.height, FSRND((float)M_PI), 1000.0f, weapon_data->water_hit.hex_ABGR_sprites_color, true, m_SpriteTextures[weapon_data->sprite_set[MACHINEGUN_WATER_HIT_SPRITES_NUM].sprites_num[0]].tex);
                }

                //С некоторым шансом рисуем инверсионный след от пули (от дула к месту попадания)
                if(weapon_data->sprite_set[MACHINEGUN_CONTRAIL_SPRITES_NUM].sprites_count && FRND(1.0f) < weapon_data->contrail_chance)
                {
                    CMatrixEffect::CreateSpritesLine(nullptr, m_Pos, hit_pos, weapon_data->contrail_width, weapon_data->hex_ABGR_contrail_color, 0x00000000, weapon_data->contrail_duration, m_SpriteTextures[weapon_data->sprite_set[MACHINEGUN_CONTRAIL_SPRITES_NUM].sprites_num[0]].tex);
                }

                if(m_Handler)
                {
                    m_Handler(s, hit_pos, m_User, FEHF_LASTHIT);
                }

                break;
            }

            //Оружие, спавнящее проджектайлы
            case WEAPON_EFFECT_CANNON:
            {
                SMOProps mo;
                mo.gun.maxdist = m_WeaponDist * m_WeaponCoefficient;
                mo.gun.splash_radius = g_Config.m_WeaponsConsts[m_WeaponNum].projectile_splash_radius;

                //Для отрисовки инверсионного следа от снаряда (рисуется с шансом)
                if(
                        weapon_data->sprite_set[CANNON_CONTRAIL_SPRITES_NUM].sprites_count
                    && (weapon_data->contrail_chance >= 1.0f || FRND(1.0f) < weapon_data->contrail_chance))
                {
                    mo.gun.contrail_sprite_num = (ESpriteTextureSort)weapon_data->sprite_set[CANNON_CONTRAIL_SPRITES_NUM].sprites_num[0];
                    mo.gun.contrail_width = weapon_data->contrail_width;
                    mo.gun.contrail_duration = weapon_data->contrail_duration;
                    mo.gun.contrail_color = weapon_data->hex_ABGR_contrail_color;
                }
                else
                {
                    mo.gun.contrail_sprite_num = SPR_NONE;
                }

                mo.startpos = m_Pos;
                mo.target = m_Speed; //Нахуя?
                mo.curpos = mo.startpos;

                mo.side = m_SideStorage;
                //mo.attacker = m_Owner;
                //if(mo.attacker) mo.attacker->RefInc();

                mo.velocity = m_Dir * weapon_data->projectile_full_velocity + m_Speed;
                mo.object = LoadObject(weapon_data->projectile_model_path, m_Heap);
                mo.handler = MO_Cannon_Round_Tact;

                ++m_Ref;
                CMatrixEffect::CreateMovingObject(nullptr, mo, TRACE_ALL, m_Skip, WeaponHit, (dword)this);

                if(weapon_data->light_radius)
                {
                    SEffectHandler eh;
                    CMatrixEffect::CreatePointLight(&eh, m_Pos, weapon_data->light_radius, weapon_data->hex_ABGR_light_color, false);
                    if(eh.effect)
                    {
                        ((CMatrixEffectPointLight*)eh.effect)->Kill(weapon_data->light_duration);
                        eh.Unconnect();
                    }
                }

                //Рисуем дульную вспышку от выстрела
                if(weapon_data->sprite_set[CANNON_MUZZLE_FLASH_SPRITES_NUM].sprites_count) //На случай, если спрайтовую "вспышку" отрубили совсем
                {
                    int f = IRND(weapon_data->sprite_set[CANNON_MUZZLE_FLASH_SPRITES_NUM].sprites_count);
                    CMatrixEffect::CreateSpritesLine(nullptr, m_Pos, m_Pos + m_Dir * weapon_data->sprites_lenght, weapon_data->sprites_width, weapon_data->hex_ABGR_sprites_color, 0x00000000, 1000.0f, m_SpriteTextures[weapon_data->sprite_set[CANNON_MUZZLE_FLASH_SPRITES_NUM].sprites_num[f]].tex);
                }

                break;
            }

            //Оружие, спавнящее проджектайлы
            case WEAPON_EFFECT_ROCKET_LAUNCHER:
            {
                SMOProps mo;
                mo.startpos = m_Pos;
                mo.target = m_Pos + m_Dir * 5000.0f;
                mo.curpos = mo.startpos;

                mo.hm.full_velocity_reach = weapon_data->projectile_full_velocity_reach;
                if(!mo.hm.full_velocity_reach) //Если у ракеты нет периода начальной скорости, то стартует она сразу с полной
                {
                    mo.velocity = m_Dir * weapon_data->projectile_full_velocity + m_Speed;
                    mo.hm.is_full_velocity = true;
                }
                else
                {
                    mo.velocity = m_Dir * weapon_data->projectile_start_velocity + m_Speed;
                    mo.hm.full_velocity = m_Dir * weapon_data->projectile_full_velocity + m_Speed;
                }

                mo.hm.acceleration_coef = weapon_data->projectile_acceleration_coef;
                mo.hm.target_capture_angle_cos = weapon_data->projectile_target_capture_angle_cos;
                mo.hm.homing_speed = weapon_data->projectile_homing_speed;

                if(m_User)
                {
                    /* хуй почему знает, но такое выставление цели крашит игру где-то в переборе эффектов
                    if(((CMatrixMapStatic*)m_User)->IsTurret())
                    {
                        CMatrixTurret* cannon = (CMatrixTurret*)m_User;
                        if(cannon->m_TargetCore) mo.hm.target = cannon->m_TargetCore;
                    }
                    else
                    */
                    if(((CMatrixMapStatic*)m_User)->IsRobot())
                    {
                        //Если у робота установлен модуль, изменяющий систему наведения ракет
                        CMatrixRobotAI* robot = (CMatrixRobotAI*)m_User;
                        if(robot->m_MissileTargetCaptureAngleCos != 1.0f)
                        {
                            mo.hm.target_capture_angle_cos = mo.hm.target_capture_angle_cos * robot->m_MissileTargetCaptureAngleCos - weapon_data->projectile_target_capture_angle_sin * robot->m_MissileTargetCaptureAngleSin;
                        }
                        mo.hm.homing_speed += robot->m_MissileHomingSpeed;
                    }
                }
                mo.hm.splash_radius = weapon_data->projectile_splash_radius;
                mo.hm.max_lifetime = (float)weapon_data->projectile_max_lifetime;

                if(m_User)
                {
                    mo.hm.missile_owner = (CMatrixMapStatic*)m_User;
                }

                mo.hm.fire_effect_starts = weapon_data->contrail_fire_effect_starts;
                mo.hm.close_color_rgb = weapon_data->close_color_rgb;
                mo.hm.far_color_rgb = weapon_data->far_color_rgb;

                mo.object = LoadObject(weapon_data->projectile_model_path, m_Heap);
                mo.handler = MO_Homing_Missile_Tact;

                mo.side = m_SideStorage;
                //mo.attacker = m_Owner;
                //if(mo.attacker) mo.attacker->RefInc();

                mo.shleif = (SEffectHandler*)HAlloc(sizeof(SEffectHandler), m_Heap);
                mo.shleif->effect = nullptr;
                if(weapon_data->contrail_duration) CMatrixEffect::CreateShleif(mo.shleif);
                ++m_Ref;
                CMatrixEffect::CreateMovingObject(nullptr, mo, TRACE_ALL, m_Skip, WeaponHit, (dword)this);

                if(weapon_data->light_radius)
                {
                    SEffectHandler eh;
                    CMatrixEffect::CreatePointLight(&eh, m_Pos, weapon_data->light_radius, weapon_data->hex_ABGR_light_color, false);
                    if(eh.effect)
                    {
                        ((CMatrixEffectPointLight*)eh.effect)->Kill(weapon_data->light_duration);
                        eh.Unconnect();
                    }
                }

                if(weapon_data->sprite_set[0].sprites_count) //На случай, если спрайтовую "вспышку" отрубили совсем
                {
                    int f = IRND(weapon_data->sprite_set[0].sprites_count);
                    CMatrixEffect::CreateSpritesLine(nullptr, m_Pos, m_Pos + m_Dir * weapon_data->sprites_lenght, weapon_data->sprites_width, 0xFFFFFFFF, 0x00000000, 1000.0f, m_SpriteTextures[weapon_data->sprite_set[0].sprites_num[f]].tex);
                }

                break;
            }

            case WEAPON_EFFECT_FLAMETHROWER:
            {
                if(m_Effect.effect == nullptr)
                {
                    float ttl = m_WeaponDist * m_WeaponCoefficient * 8.333f;
                    CMatrixEffect::CreateFlame(&m_Effect, ttl, TRACE_ALL, m_Skip, (dword)this, WeaponHit);
                    if(m_Effect.effect == nullptr) break;
                    ++m_Ref;
                }

                ((CMatrixEffectFlame*)m_Effect.effect)->AddPuff(m_Pos, m_Dir, m_Speed);
                
                break;
            }

            case WEAPON_EFFECT_MORTAR:
            {
                SMOProps mo;
                mo.startpos = m_Pos;
                mo.curpos = mo.startpos;

                //CHelper::Create(100, 0)->Line(m_Speed, m_Speed + D3DXVECTOR3(0.0f, 0.0f, 100.0f));

                D3DXVECTOR3 temp = m_Pos - m_Speed;
                float len = D3DXVec3Length(&temp);
                D3DXVECTOR3 dir((m_Pos - m_Speed) * (1.0f / len));
                if(len > m_WeaponDist * m_WeaponCoefficient) len = m_WeaponDist * m_WeaponCoefficient;

                mo.target = m_Pos - dir * len;
                D3DXVECTOR3 mid((m_Pos + mo.target) * 0.5f);

                //SPlane pl;
                //SPlane::BuildFromPointNormal(pl, mid, dir);
                //float ttt;
                //pl.FindIntersect(m_Pos, m_Dir, ttt);

                int pcnt = 5;
                D3DXVECTOR3 pts[5];

                pts[0] = m_Pos;

                if(m_Dir.z < 0)
                {
                    // on flyer bombomet
                    pts[1] = m_Pos + m_Dir * 70.0f - D3DXVECTOR3(0.0f, 0.0f, 25.0f);
                    pts[2] = m_Pos + m_Dir * 110.0f - D3DXVECTOR3(0.0f, 0.0f, 100.0f);
                    pts[3] = m_Pos + m_Dir * 130.0f - D3DXVECTOR3(0.0f, 0.0f, 300.0f);

                    pcnt = 4;

                    mo.velocity.x = 0.0006f;
                }
                else
                {
                    pts[1] = mid + dir * len * 0.15f + D3DXVECTOR3(0.0f, 0.0f, len * 0.5f);
                    pts[2] = mid - dir * len * 0.15f + D3DXVECTOR3(0.0f, 0.0f, len * 0.5f);
                    pts[3] = mo.target;
                    // more beautiful trajectory
                    pts[4] = mo.target - dir * len * 0.35f - D3DXVECTOR3(0.0f, 0.0f, len * 0.5f);

                    mo.velocity.x = 0.0005f;
                }

                //CHelper::Create(1, 0)->Line(pts[0], pts[1]);
                //CHelper::Create(1, 0)->Line(pts[1], pts[2]);
                //CHelper::Create(1, 0)->Line(pts[2], pts[3]);

                mo.bomb.pos = 0;
                mo.bomb.trajectory = HNew(g_MatrixHeap) CTrajectory(g_MatrixHeap, pts, pcnt);

                //CHelper::DestroyByGroup(123);

                //D3DXVECTOR3 pp;
                //mo.common.bomb.trajectory->CalcPoint(pp,0);
                //for(float t = 0.01f; t <= 1.0f; t += 0.01f)
                //{
                //    D3DXVECTOR3 ppp;
                //    mo.common.bomb.trajectory->CalcPoint(ppp, t);
                //    CHelper::Create(100000, 123)->Line(pp, ppp);

                //    pp = ppp;
                //}

                //float speed = D3DXVec3Length(&(m_Pos-m_Speed)) / 100;
                //if(speed < 1) speed = 1;
                //if(speed > 3) speed = 3;

                //speed -= (speed - 1.2f) * 0.3f;

                //mo.velocity = m_Dir * speed * 1.5f;
                mo.object = LoadObject(L"Matrix\\Bullets\\mina.vo", m_Heap);
                mo.handler = MO_Grenade_Tact;

                mo.side = m_SideStorage;
                //mo.attacker = m_Owner;
                //if(mo.attacker) mo.attacker->RefInc();

                mo.shleif = (SEffectHandler*)HAlloc(sizeof(SEffectHandler), m_Heap);
                mo.shleif->effect = nullptr;
                CMatrixEffect::CreateShleif(mo.shleif);

                ++m_Ref;
                CMatrixEffect::CreateMovingObject(nullptr, mo, TRACE_ALL, m_Skip, WeaponHit, (dword)this);

                break;
            }

            case WEAPON_EFFECT_LASER:
            {
                D3DXVECTOR3 hitpos = m_Pos + m_Dir * m_WeaponDist * m_WeaponCoefficient;
                CMatrixMapStatic* s = g_MatrixMap->Trace(&hitpos, m_Pos, hitpos, TRACE_ALL, m_Skip);

                if(IS_TRACE_STOP_OBJECT(s))
                {
                    bool dead = s->TakingDamage(m_WeaponNum, hitpos, m_Dir, GetSideStorage(), GetOwner());
                    SETFLAG(m_Flags, WEAPFLAGS_HITWAS);
                    if(dead)
                    {
                        s = TRACE_STOP_NONE;
                    }

                    /*
                    D3DXVECTOR3 pos1(hitpos.x + FSRND(20.0f), hitpos.y + FSRND(20.0f), 0);
                    pos1.z = g_MatrixMap->GetZ(pos1.x, pos1.y);
                    e = CMatrixEffect::CreateShorted(hitpos, pos1, int(FRND(500.0f) + 400.0f));
                    g_MatrixMap->AddEffect(e);
                    */

                    CMatrixEffect::CreateExplosion(hitpos, ExplosionLaserHit);

                    if(m_Handler)
                    {
                        m_Handler(s, hitpos, m_User, FEHF_LASTHIT);
                    }
                }
                else if(s == TRACE_STOP_LANDSCAPE)
                {
                    CMatrixEffect::CreateLandscapeSpot(nullptr, D3DXVECTOR2(hitpos.x, hitpos.y), FSRND((float)M_PI), FRND(1.0f) + 0.5f, SPOT_PLASMA_HIT);
                    CMatrixEffect::CreateExplosion(hitpos, ExplosionLaserHit);
                }
                else if(s == TRACE_STOP_WATER)
                {
                    CMatrixEffect::CreateSmoke(nullptr, hitpos, 100, 1400, 10, 0xFFFFFF);
                }

                if(m_Laser)
                {
                    m_Laser->SetPos(m_Pos, hitpos);
                }
                else
                {
                    m_Laser = HNew(m_Heap) CLaser(m_Pos, hitpos);
                    if(!g_MatrixMap->AddEffect(m_Laser))
                    {
                        m_Laser = nullptr;
                    }
                }

                if(m_Handler)
                {
                    m_Handler(s, hitpos, m_User, FEHF_LASTHIT);
                }

                break;
            }

            case WEAPON_EFFECT_PLASMAGUN:
            {
                float ang = float(2 * M_PI / 4096.0) * (g_MatrixMap->GetTime() & 4095);
                CMatrixEffect::CreateCone(&m_Effect, m_Pos, m_Dir, 10.0f, 10.0f, ang, 300.0f, 0xFFFFFFFF, true, nullptr);

                ++m_Ref;

                CMatrixEffect::CreateFirePlasma(m_Pos, m_Pos + (m_Dir * m_WeaponDist * m_WeaponCoefficient), 0.5f, TRACE_ALL, m_Skip, WeaponHit, (dword)this);

                break;
            }

            case WEAPON_EFFECT_REPAIRER:
            {
                if(m_Repair) m_Repair->UpdateData(m_Pos, m_Dir);
                else
                {
                    m_Repair = (CMatrixEffectRepair*)CreateRepair(m_Pos, m_Dir, m_WeaponDist * m_WeaponCoefficient, m_Skip, (ESpriteTextureSort)g_Config.m_WeaponsConsts[m_WeaponNum].sprite_set[0].sprites_num[0]);
                    if(!g_MatrixMap->AddEffect(m_Laser))
                    {
                        m_Repair = nullptr;
                    }
                }

                if(m_Repair)
                {
                    CMatrixMapStatic* ms = m_Repair->GetTarget();
                    if(ms)
                    {
                        if(m_Handler) m_Handler(ms, ms->GetGeoCenter(), m_User, FEHF_LASTHIT);
                        SETFLAG(m_Flags, WEAPFLAGS_HITWAS);
                        ms->TakingDamage(m_WeaponNum, ms->GetGeoCenter(), m_Dir, GetSideStorage(), GetOwner());
                    }
                    else if(m_Handler)
                    {
                        m_Handler(TRACE_STOP_NONE, m_Pos, m_User, FEHF_LASTHIT);
                    }
                }

                break;
            }

            case WEAPON_EFFECT_DISCHARGER:
            {
                D3DXVECTOR3 hitpos = m_Pos + m_Dir * m_WeaponDist * m_WeaponCoefficient;
                CMatrixMapStatic* s = g_MatrixMap->Trace(&hitpos, m_Pos, hitpos, TRACE_ALL, m_Skip);

                //float l = D3DXVec3Length(&(m_Pos - hitpos));
                //CDText::T("len", CStr(l));

                //CHelper::Create(10, 0)->Line(m_Pos, hitpos);
                //CHelper::Create(10, 0)->Line(hitpos, hitpos + D3DXVECTOR3(0, 0, 100));

                if(IS_TRACE_STOP_OBJECT(s))
                {
                    bool dead = s->TakingDamage(m_WeaponNum, hitpos, m_Dir, GetSideStorage(), GetOwner());
                    SETFLAG(m_Flags, WEAPFLAGS_HITWAS);
                    if(dead)
                    {
                        s = TRACE_STOP_NONE;
                    }
                    else
                    {
                        if(!s->IsFlyer())
                        {
                            D3DXVECTOR3 pos1(hitpos.x + FSRND(20), hitpos.y + FSRND(20), 0);
                            pos1.z = g_MatrixMap->GetZ(pos1.x, pos1.y);
                            CMatrixEffect::CreateShorted(hitpos, pos1, FRND(500) + 400);
                        }
                    }
                }
                else if(s == TRACE_STOP_LANDSCAPE)
                {
                    CMatrixEffect::CreateLandscapeSpot(nullptr, D3DXVECTOR2(hitpos.x, hitpos.y), FSRND((float)M_PI), FRND(1) + 0.5f, SPOT_PLASMA_HIT);

                    //e = CMatrixEffect::CreateShorted(hitpos + D3DXVECTOR3(FSRND(5), FSRND(5), 0), hitpos + D3DXVECTOR3(FSRND(5), FSRND(5), 0), int(FRND(500) + 400));
                    //g_MatrixMap->AddEffect(e);
                }

                if(m_Effect.effect && m_Effect.effect->GetType() == EFFECT_LIGHTENING)
                {
                    ((CMatrixEffectLightening*)m_Effect.effect)->SetPos(m_Pos, hitpos);
                }
                else
                {
                    CMatrixEffect::CreateLightening(&m_Effect, m_Pos, hitpos, 1000000, 3, LIGHTENING_WIDTH);
                }

                if(m_Handler)
                {
                    m_Handler(s, hitpos, m_User, FEHF_LASTHIT);
                }

                break;
            }

            case WEAPON_EFFECT_BOMB:
            {
                ++m_Ref;
                CMatrixEffect::CreateBigBoom(m_Pos, m_WeaponDist * m_WeaponCoefficient, 300, TRACE_ALL, nullptr/*m_Skip*/, (dword)this, WeaponHit);
                CMatrixEffect::CreateBigBoom(m_Pos, m_WeaponDist, 350, 0, 0, 0, 0);
                CMatrixEffect::CreateBigBoom(m_Pos, m_WeaponDist, 400, 0, 0, 0, 0, 0xFFAFAF40);
                CMatrixEffect::CreateExplosion(m_Pos, ExplosionBigBoom, true);

                break;
            }

            default:
            {
                CMatrixEffect::CreateExplosion(m_Pos, ExplosionRobotBoom, true);
            }
        }
    }
}

void CMatrixEffectWeapon::FireEnd()
{
    if(!IsFire()) return;
    RESETFLAG(m_Flags, WEAPFLAGS_FIRE);

    if(m_ShotSoundType != S_NONE)
    {
        if(FLAG(m_Flags, WEAPFLAGS_SND_OFF))
        {
            CSound::StopPlay(m_Sound);
            m_Sound = SOUND_ID_EMPTY;
        }
    }

    switch(g_Config.m_WeaponsConsts[m_WeaponNum].primary_effect)
    {
        case WEAPON_EFFECT_MACHINEGUN:
        {
            if(m_Machinegun)
            {
                g_MatrixMap->SubEffect(m_Machinegun);
                m_Machinegun = nullptr;
            }

            break;
        }

        case WEAPON_EFFECT_FLAMETHROWER:
        {
            if(m_Effect.effect)
            {
                ((CMatrixEffectFlame*)m_Effect.effect)->Break();
            }

            break;
        }

        case WEAPON_EFFECT_LASER:
        {
            if(m_Laser)
            {
                g_MatrixMap->SubEffect(m_Laser);
                m_Laser = nullptr;
            }

            break;
        }

        case WEAPON_EFFECT_DISCHARGER:
        {
            m_Effect.Release();

            break;
        }

        default:
        {
            if(g_Config.m_WeaponsConsts[m_WeaponNum].is_repairer)
            {
                if(m_Repair)
                {
                    //CMatrixMapStatic* ms = m_Repair->GetTarget();
                    //if(ms && m_Handler) m_Handler(ms, ms->GetGeoCenter(), m_User, FEHF_LASTHIT);
                    g_MatrixMap->SubEffect(m_Repair);
                    m_Repair = nullptr;
                }
            }

            break;
        }
    }
}

//Предполагалось использовать для отрисовки эффекта автохила, но не разобрался с конвертацией подходящей модели для головы
void CMatrixEffectWeapon::SelfRepairEffect()
{
    /*
    if(m_Repair) m_Repair->UpdateData(m_Pos, m_Dir);
    else
    {
        m_Repair = (CMatrixEffectRepair*)CreateRepair(m_Pos, m_Dir, m_WeaponDist * m_WeaponCoefficient, m_Skip, (ESpriteTextureSort)g_Config.m_WeaponsConsts[m_Type].sprite_set[0].sprites_num[0]);
        if(!g_MatrixMap->AddEffect(m_Laser)) m_Repair = nullptr;
    }

    if(m_Repair)
    {
        CMatrixMapStatic* ms = m_Repair->GetTarget();
        if(ms)
        {
            if(m_Handler) m_Handler(ms, ms->GetGeoCenter(), m_User, FEHF_LASTHIT);
            SETFLAG(m_Flags, WEAPFLAGS_HITWAS);
            ms->TakingDamage(m_WeaponNum, ms->GetGeoCenter(), m_Dir, GetSideStorage(), GetOwner());
        }
        else if(m_Handler) m_Handler(TRACE_STOP_NONE, m_Pos, m_User, FEHF_LASTHIT);
    }
    */
}

void CMatrixEffectWeapon::SoundHit(int weapon_num, const D3DXVECTOR3& pos)
{
    if(g_Config.m_WeaponsConsts[weapon_num].hit_sound_num == S_NONE) return;
    //CSound::AddSound(g_Config.m_WeaponsConsts[8].hit_sound_num, pos, SL_ALL, SEF_SKIP); //plasma hit
    CSound::AddSound(g_Config.m_WeaponsConsts[weapon_num].hit_sound_num, pos, SL_ALL, SEF_SKIP);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

CLaser::CLaser(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1) :
    CMatrixEffect(), m_Sprites(TRACE_PARAM_CALL pos0, pos1, LASER_WIDTH, 0xFFFFFFFF, m_SpriteTextures[SPR_LASER_BEAM].tex)
{
    m_EffectType = EFFECT_LASER;

    if(m_SpriteTextures[SPR_LASER_SPOT].IsSingleBrightTexture()) m_end = CSprite(TRACE_PARAM_CALL pos0, LASER_WIDTH * 0.5f, 0, 0xFFFFFFFF, m_SpriteTextures[SPR_LASER_SPOT].tex);
    else m_end = CSprite(TRACE_PARAM_CALL pos0, LASER_WIDTH * 0.5f, 0, 0xFFFFFFFF, &m_SpriteTextures[SPR_LASER_SPOT].spr_tex);
}

void CLaser::Draw()
{
    byte a = g_MatrixMap->IsPaused()?240:(byte(FRND(128) + 128));

    m_Sprites.SetAlpha(a);
    m_Sprites.AddToDrawQueue();

    m_end.SetAlpha(a);
    m_end.Sort(g_MatrixMap->m_Camera.GetViewMatrix());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

CMachinegun::CMachinegun(const D3DXVECTOR3& start, const D3DXVECTOR3& dir, float angle, float sprites_lenght, float sprites_width, float fire_cone_lenght, float fire_cone_radius, dword color, int sprite_num_1, int sprite_num_2, int sprite_num_3) :
    m_SpritesLenght(sprites_lenght), m_FireConeLenght(fire_cone_lenght), m_FireConeRadius(fire_cone_radius), m_Cone(start, dir, m_FireConeRadius, m_FireConeLenght, angle, 300.0f, color, true, nullptr),
    m_Sprite1(TRACE_PARAM_CALL start, start + dir * m_SpritesLenght, sprites_width, color, m_SpriteTextures[sprite_num_1].tex),
    m_Sprite2(TRACE_PARAM_CALL start, start + dir * m_SpritesLenght, sprites_width, color, m_SpriteTextures[sprite_num_2].tex),
    m_Sprite3(TRACE_PARAM_CALL start, start + dir * m_SpritesLenght, sprites_width, color, m_SpriteTextures[sprite_num_3].tex)
{
    m_EffectType = EFFECT_MACHINEGUN;
}

void CMachinegun::Draw()
{
    m_Cone.Draw();

    switch(g_MatrixMap->IsPaused() ? 0 : (IRND(3)))
    {
        case 0: m_Sprite1.AddToDrawQueue(); break;
        case 1: m_Sprite2.AddToDrawQueue(); break;
        case 2: m_Sprite3.AddToDrawQueue(); break;
    }
}