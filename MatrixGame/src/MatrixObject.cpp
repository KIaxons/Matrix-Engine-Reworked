// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "stdafx.h"
#include "MatrixObject.hpp"
#include "MatrixObjectBuilding.hpp"
#include "MatrixShadowManager.hpp"
#include "ShadowStencil.hpp"
#include "MatrixSkinManager.hpp"
#include "Interface/CConstructor.h"
#include <stdio.h>

// При перемещении объекта не нужно заного расчитывать проэкционную текстуру

CMatrixMapObject::SObjectShadowTexture* CMatrixMapObject::m_ShadowTextures;
int CMatrixMapObject::m_ShadowTexturesCount;

void CMatrixMapObject::InitTextures(int n)
{
    if(!n) return;
    m_ShadowTexturesCount = n;
    m_ShadowTextures = (CMatrixMapObject::SObjectShadowTexture*)HAllocClear(sizeof(CMatrixMapObject::SObjectShadowTexture) * n, g_MatrixHeap);
}

void CMatrixMapObject::ClearTextures()
{
    if(m_ShadowTextures) HFree(m_ShadowTextures, g_MatrixHeap);
    m_ShadowTexturesCount = 0;
    m_ShadowTextures = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool CMatrixMapObject::TakingDamage(
    int weap,
    const D3DXVECTOR3&,
    const D3DXVECTOR3&,
    int attacker_side,
    CMatrixMapStatic* attaker
)
{
    if(attacker_side != PLAYER_SIDE && FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL)) return false; // only player can hit this objects

    if(m_BehaviorFlag == BEHF_BURN || attaker == this)
    {
        if(attaker == this) //Если урон был получен от процесса сгорания
        {
            if(g_Config.m_WeaponsConsts[weap].map_objects_ignition.burning_sound_num != S_NONE)
            {
                CSound::AddSound(g_Config.m_WeaponsConsts[weap].map_objects_ignition.burning_sound_num, GetGeoCenter(), SL_ALL, SEF_SKIP);
            }
        }
        else if(g_Config.m_WeaponsConsts[weap].map_objects_ignition.is_present)
        {
            //Будет применён наиболее приоритетный, либо самый последний эффект воспламенения объектов
            int effect = weap;
            int new_priority = g_Config.m_WeaponsConsts[effect].map_objects_ignition.priority;

            if(IsAblaze()) effect = new_priority >= g_Config.m_WeaponsConsts[IsAblaze()].map_objects_ignition.priority ? effect : IsAblaze(); //Если наложенный ранее эффект горения имеет более высокий приоритет, то обновляем эффект по его статам
            MarkAblaze(effect);

            //Если объект ещё не выгорел, то и гореть он сможет дольше
            int add_duration = GetAblazeTTL() + g_Config.m_WeaponsConsts[effect].map_objects_ignition.duration_per_hit;
            SetAblazeTTL(!FLAG(m_ObjectFlags, OBJECT_STATE_BURNED) ? min(add_duration, g_Config.m_WeaponsConsts[effect].map_objects_ignition.max_duration) : min(add_duration, g_Config.m_WeaponsConsts[effect].map_objects_ignition.max_duration / 10));

            m_NextTime = g_MatrixMap->GetTime();
            AddLT();
        }
    }
    else if(m_BehaviorFlag == BEHF_TERRON && !FLAG(m_ObjectFlags, OBJECT_STATE_TERRON_EXPL))
    {
        m_BreakHitPoint = int(max(m_BreakHitPoint - g_Config.m_WeaponsConsts[weap].damage.to_objects, g_Config.m_WeaponsConsts[weap].non_lethal_threshold.to_objects));

        if(m_HealthBar == nullptr)
        {
            m_HealthBar = HNew(g_MatrixHeap) CMatrixProgressBar();
            m_HealthBar->Modify(1000000.0f, 0.0f, GetRadius() * 1.2f, 1.0f);
        }

        if(!m_Graph->IsAnim(L"Pain"))
        {
            m_Graph->SetAnimByName(L"Pain", ANIM_LOOP_OFF);
            int idx = IRND(4);
            switch(idx)
            {
                case 0: CSound::AddSound(S_TERRON_PAIN1, GetGeoCenter()); break;
                case 1: CSound::AddSound(S_TERRON_PAIN2, GetGeoCenter()); break;
                case 2: CSound::AddSound(S_TERRON_PAIN3, GetGeoCenter()); break;
                case 3: CSound::AddSound(S_TERRON_PAIN4, GetGeoCenter()); break;
            }
        }

        if(m_BreakHitPoint <= 0)
        {
            if(FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL))
            {
                g_MatrixMap->SetMusicVolume(0);
                SETFLAG(g_MatrixMap->m_Flags, MMFLAG_TERRON_DEAD);
            }

            SETFLAG(m_ObjectFlags, OBJECT_STATE_TERRON_EXPL);
            m_Graph->SetAnimByName(L"Death", ANIM_LOOP_OFF);
            CSound::AddSound(S_TERRON_KILLED, GetGeoCenter());
            SetAblazeTTL(5000);

            m_NextExplosionTime = g_MatrixMap->GetTime();
            m_NextExplosionTimeSound = g_MatrixMap->GetTime();
            g_MatrixMap->RemoveEffectSpawnerByObject(this);

            if(m_HealthBar != nullptr)
            {
                HDelete(CMatrixProgressBar, m_HealthBar, g_MatrixHeap);
                m_HealthBar = nullptr;
            }
        }
    }
    else if(m_BehaviorFlag == BEHF_BREAK)
    {
        m_BreakHitPoint = int(max(m_BreakHitPoint - g_Config.m_WeaponsConsts[weap].damage.to_objects, g_Config.m_WeaponsConsts[weap].non_lethal_threshold.to_objects));

        if(FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL) && m_HealthBar == nullptr)
        {
            m_HealthBar = HNew(g_MatrixHeap) CMatrixProgressBar();
            m_HealthBar->Modify(1000000.0f, 0.0f, PB_SPECIAL_WIDTH, 1.0f);
        }

        if(m_BreakHitPoint <= 0)
        {
            if(FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL))
            {
                RESETFLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL);
                --g_MatrixMap->m_BeforeWinCount;
                g_MatrixMap->GetPlayerSide()->IncStatValue(STAT_BUILDING_KILL);

                if(g_MatrixMap->m_BeforeWinCount <= 0)
                {
                    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_SPECIAL_BROKEN);
                    g_MatrixMap->GetPlayerSide()->SetStatus(SS_JUST_WIN);
                }
            }

            g_MatrixMap->RemoveEffectSpawnerByObject(this);

            if(m_HealthBar != nullptr)
            {
                HDelete(CMatrixProgressBar, m_HealthBar, g_MatrixHeap);
                m_HealthBar = nullptr;
            }

            CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
            if(temp.GetStrPar(3, L",") == STR_BREAK_TYPE_EXPLODE)
            {
                CMatrixEffect::CreateExplosion(m_Core->m_GeoCenter, ExplosionObject, true);
            }
            //m_ShadowType = SHADOW_OFF;
            //RChange(MR_ShadowProjGeom | MR_ShadowProjTex | MR_ShadowStencil);
            //GetResources(MR_ShadowProjGeom | MR_ShadowProjTex | MR_ShadowStencil);
            RESETFLAG(m_ObjectFlags, OBJECT_STATE_SHADOW_SPECIAL);
            Init(temp.GetIntPar(1, L","));
        }
    }
    else if(m_BehaviorFlag == BEHF_ANIM)
    {
        m_BreakHitPoint = int(max(m_BreakHitPoint - g_Config.m_WeaponsConsts[weap].damage.to_objects, g_Config.m_WeaponsConsts[weap].non_lethal_threshold.to_objects));

        if(FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL) && m_HealthBar == nullptr)
        {
            m_HealthBar = HNew(g_MatrixHeap) CMatrixProgressBar();
            m_HealthBar->Modify(1000000.0f, 0.0f, PB_SPECIAL_WIDTH, 1.0f);
        }

        if(m_BreakHitPoint <= 0)
        {
            if(FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL))
            {
                RESETFLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL);
                --g_MatrixMap->m_BeforeWinCount;
                g_MatrixMap->GetPlayerSide()->IncStatValue(STAT_BUILDING_KILL);

                if(g_MatrixMap->m_BeforeWinCount <= 0)
                {
                    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_SPECIAL_BROKEN);
                    g_MatrixMap->GetPlayerSide()->SetStatus(SS_JUST_WIN);
                }
            }

            if(m_HealthBar != nullptr)
            {
                HDelete(CMatrixProgressBar, m_HealthBar, g_MatrixHeap);
                m_HealthBar = nullptr;
            }

            CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*").GetStrPar(1, L","), g_CacheHeap);
            CWStr temp1(g_CacheHeap);

            int cnt = temp.GetCountPar(L"#");
            for(int i = 0; i < cnt; ++i)
            {
                temp1 = temp.GetStrPar(i, L"#");
                if(temp1.GetIntPar(0, L":") == m_AnimState)
                {
                    // found
                    int newstate = temp1.GetIntPar(3, L":");
                    if(newstate >= 0)
                    {
                        ApplyAnimState(newstate);
                        break;
                    }
                }
            }
        }
    }

    return false;
}

void CMatrixMapObject::ApplyAnimState(int anims)
{
    m_AnimState = anims;

    CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*").GetStrPar(1, L","), g_CacheHeap);
    CWStr temp1(g_CacheHeap);

    int cnt = temp.GetCountPar(L"#");
    for(int i = 0; i < cnt; ++i)
    {
        temp1 = temp.GetStrPar(i, L"#");
        if(temp1.GetIntPar(0, L":") == anims)
        {
            // found
            if(m_Graph) m_Graph->SetAnimById(temp1.GetIntPar(1, L":"));
            m_BreakHitPoint = temp1.GetIntPar(2, L":");
            if(!m_BreakHitPoint)
            {
                m_BreakHitPoint = 2000000000;
            }

            break;
        }
    }
}

void CMatrixMapObject::ClearTexture(CBaseTexture* tex)
{
    for(int i = 0; i < m_ShadowTexturesCount; ++i)
    {
        if( m_ShadowTextures[i].tex == (CTextureManaged*)tex ) m_ShadowTextures[i].tex = nullptr;
        if( m_ShadowTextures[i].tex_burn == (CTextureManaged*)tex ) m_ShadowTextures[i].tex_burn = nullptr;
    }
}

void CMatrixMapObject::FreeShadowTexture()
{
    if(m_ShadowProj && m_ShadowType == SHADOW_PROJ_STATIC)
    {
        if(m_ShadowProj->GetTexture() != nullptr && m_ShadowProj->GetTexture()->m_Ref == 1 && m_ShadowTextures)
        {
            ClearTexture(m_ShadowProj->GetTexture());
        }
    }
}

void CMatrixMapObject::SetupMatricesForShadowTextureCalc()
{
    D3DXVECTOR3 light, camup;

    D3DXVec3TransformNormal(&light, &g_MatrixMap->m_LightMain, &m_Core->m_IMatrix);
    D3DXVec3Normalize(&light, &light);

    D3DXMATRIX mWorld;
	D3DXMatrixIdentity(&mWorld);
	g_D3DD->SetTransform(D3DTS_WORLD, &mWorld);

    D3DXVECTOR3 temp = { 0.0f, 0.0f, 1.0f };
    D3DXVec3TransformNormal(&camup, &temp, &m_Core->m_IMatrix);
    D3DXVec3Normalize(&camup, &camup);

	D3DXMATRIX mView;
    temp = m_ShCampos + light;
	D3DXMatrixLookAtLH(&mView, &m_ShCampos, &temp, &camup);

    float _sx = 1.0f / m_ShDim.x;
    float _sy = 1.0f / m_ShDim.y;

    mView._11 *= _sx; mView._12 *= _sy;
    mView._21 *= _sx; mView._22 *= _sy;
    mView._31 *= _sx; mView._32 *= _sy;
    mView._41 *= _sx; mView._42 *= _sy;

	g_D3DD->SetTransform(D3DTS_VIEW, &mView);

	D3DXMATRIX mProg;
	D3DXMatrixOrthoLH(&mProg, -1.0f, 1.0f, 1.0f, 1000.0f);
    g_D3DD->SetTransform(D3DTS_PROJECTION, &mProg);
}

void CMatrixMapObject::GetResources(dword need)
{
    bool joined = false;
	if(need & m_RChange & (MR_Matrix))
    {
        m_RChange &= ~MR_Matrix;

        D3DXVECTOR3 pos = *(D3DXVECTOR3*)&m_Core->m_Matrix._41;

        // rotate

        D3DXMATRIX mz, mx, my;
        D3DXMatrixRotationZ(&mz, m_AngleZ);
        D3DXMatrixRotationX(&mx, m_AngleX);
        D3DXMatrixRotationY(&my, m_AngleY);

        //D3DXMatrixRotationZ(&m_Core->m_Matrix,m_AngleZ);
        m_Core->m_Matrix = mx * my * mz;


        // scale
        m_Core->m_Matrix._11 *= m_Scale; m_Core->m_Matrix._12 *= m_Scale; m_Core->m_Matrix._13 *= m_Scale;
        m_Core->m_Matrix._21 *= m_Scale; m_Core->m_Matrix._22 *= m_Scale; m_Core->m_Matrix._23 *= m_Scale;
        m_Core->m_Matrix._31 *= m_Scale; m_Core->m_Matrix._32 *= m_Scale; m_Core->m_Matrix._33 *= m_Scale;

        // translate

        *(D3DXVECTOR3*)&m_Core->m_Matrix._41 = pos;
        

        //D3DXMATRIX m1,m2,m3;
        //D3DXMatrixScaling(&m1,m_Scale,m_Scale,m_Scale);
        //D3DXMatrixRotationZ(&m2,m_Angle);
        //D3DXMatrixTranslation(&m3,GLOBAL_SCALE*m_MapPos.x+GLOBAL_SCALE*m_MapSmeX,GLOBAL_SCALE*m_MapPos.y+GLOBAL_SCALE*m_MapSmeY,m_Height+map.UnitCalcHeight(m_MapPos.x,m_MapPos.y,m_MapSmeX,m_MapSmeY));
        //m_Matrix = m1 * m2 * m3;

        D3DXMatrixInverse(&m_Core->m_IMatrix, nullptr, &m_Core->m_Matrix);

        if(m_Graph)
        {
            JoinToGroup();
            joined = true;
        }
	}
	if(need & m_RChange & (MR_Graph))
    {
        m_RChange &= ~MR_Graph;

        if(m_Graph) UnloadObject(m_Graph, g_MatrixHeap);
        
        // prepare texture
        CWStr path(g_CacheHeap), tex(g_CacheHeap), vo(g_CacheHeap), temp(g_CacheHeap);
        path = g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_PATH, L"*");

        if(m_BehaviorFlag != BEHF_PORTRET)
        {

            tex = path + g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE,L"*");

            tex += L"*";
            temp = g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_GLOSS, L"*");
            if (!temp.IsEmpty())  tex += path + temp;

            tex += L"*";
            temp = g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_BACK, L"*");
            if (!temp.IsEmpty())  tex += path + temp;

            tex += L"*";
            temp = g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_MASK, L"*");
            if (!temp.IsEmpty())  tex += path + temp;

            tex += L"*" + g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_SCROLL, L"*");
        } else
        {
            if (m_PrevStateRobotsInRadius == 0 || m_Photo == 1)
            {
                tex = path + g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_MASK, L"*");
                tex += L"**";
                tex += path + g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_BACK, L"*");
                tex += L"*";
                tex += path + g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_MASK, L"*");
                tex += L"*" + g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_SCROLL, L"*");
            } else
            {
                tex = path + L"p" + CWStr(m_PrevStateRobotsInRadius, g_CacheHeap);
                tex += L"**";
                tex += path + g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_BACK, L"*");
                tex += L"*";
                tex += path + g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_MASK, L"*") + L"2";;
                tex += L"*" + g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE_SCROLL, L"*");

            }

        }

        vo = path + g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_VO, L"*") + L".vo";
        
        m_Graph = LoadObject(vo.Get(), g_MatrixHeap, false, tex.Get());


        //if (FLAG(m_ObjectFlags,OBJECT_STATE_BURNED))
        //{
        //    if (m_ResponseFlag == RESPF_BURN_VO)
        //    {
        //        if(m_Graph)
        //        {
        //            UnloadObject(m_Graph,g_MatrixHeap);
        //        }

        //        CWStr name(g_MatrixMap->IdsGet(m_iName).Get() + m_BurnNameDisp, g_CacheHeap);
        //        int idx = name.Find(L"?",1,0);
        //        if (idx >=0) name.SetLen(idx);
        //        name.Add(L".vo");

        //        CWStr nameo(g_CacheHeap);

        //        CacheReplaceFileNameAndExt(nameo, g_MatrixMap->IdsGet(m_iName).Get(), name);

        //        //idx = nameo.Find(L"?",1,0);
        //        //if (idx >=0) nameo.SetLen(idx);

        //        m_Graph = LoadObject(nameo, g_MatrixHeap);

        //    } else if (m_ResponseFlag == RESPF_BURN_TEX)
        //    {
        //        // burned objects uses other then basic textures
        //        m_BurnTexVis = 255;

        //        if(!m_Graph)
        //        {
        //            m_Graph = LoadObject(g_MatrixMap->IdsGet(m_iName).Get(), g_MatrixHeap);
        //        }
        //        //m_Graph->VO()->ReleaseTextures();

        //    }

        //} else
        //{
        //    if(!m_Graph)
        //    {
        //        m_Graph = LoadObject(g_MatrixMap->IdsGet(m_iName).Get(), g_MatrixHeap);
        //    }
        //}

        if(m_BehaviorFlag == BEHF_ANIM) ApplyAnimState(m_AnimState);
        else m_Graph->SetFirstFrame();
        if(!joined) JoinToGroup();
	}

	if(need & m_RChange & MR_ShadowStencil)
    {
        m_RChange &= ~MR_ShadowStencil;

		if(m_ShadowType != SHADOW_STENCIL)
        {
			if(m_ShadowStencil)
            {
                HDelete(CVOShadowStencil, m_ShadowStencil, g_MatrixHeap);
                m_ShadowStencil = nullptr;
            }
		}
        else 
        {
            ASSERT(m_Graph);
            ASSERT(m_Graph->VO());

            if(!m_ShadowStencil) m_ShadowStencil = HNew(g_MatrixHeap) CVOShadowStencil(g_MatrixHeap);

            //STENCIL
            D3DXVECTOR3 light;
            D3DXVec3TransformNormal(&light, &g_MatrixMap->m_LightMain, &m_Core->m_IMatrix);

            //D3DXPLANE cutpl;
            //D3DXPlaneTransform(&cutpl, &g_MatrixMap->m_ShadowPlaneCut, &m_IMatrix);
            float len = (GetRadius() * 2) + m_Core->m_Matrix._43 - (IsNearBase() ? g_MatrixMap->m_GroundZBase : g_MatrixMap->m_GroundZ);
            m_ShadowStencil->CreateShadowVO(*(m_Graph->VO()), m_Graph->GetVOFrame(), light, len, false);
            if(!m_ShadowStencil->IsReady())
            {
                m_ShadowType = SHADOW_OFF;
                HDelete(CVOShadowStencil, m_ShadowStencil, g_MatrixHeap);
                m_ShadowStencil = nullptr;
            }
        }
	}

	if(need & m_RChange & MR_ShadowProjGeom)
    {
        // projecting shadows geometry were requested
        m_RChange &= ~MR_ShadowProjGeom;

        if(m_ShadowType != SHADOW_PROJ_STATIC && m_ShadowType != SHADOW_PROJ_DYNAMIC)
        {
            if(m_ShadowProj)
            {
                HDelete(CMatrixShadowProj, m_ShadowProj, g_MatrixHeap);
                m_ShadowProj = nullptr;
            }
		}
        else 
        {
			ASSERT(m_Graph);
			ASSERT(m_Graph->VO());
			if(m_ShadowProj == nullptr)
            {
                m_ShadowProj = HNew(g_MatrixHeap) CMatrixShadowProj(g_MatrixHeap, this);
                ShadowProjBuildGeom(*m_ShadowProj, *m_Graph,m_Graph->GetVOFrame(), m_Core->m_Matrix, m_Core->m_IMatrix, g_MatrixMap->m_LightMain, int(100 / GLOBAL_SCALE), true);
			    if(!(m_ShadowProj->IsProjected()))
                {
				    HDelete(CMatrixShadowProj, m_ShadowProj, g_MatrixHeap);
				    m_ShadowProj = nullptr;
                    m_ShadowType = SHADOW_OFF;
			    }
            }
        }
    }

	if(need & m_RChange & MR_ShadowProjTex)
    {
        // projecting shadows geometry were requested
		m_RChange &= ~MR_ShadowProjTex;

        //m_ShadowProj создаётся и вешается на объект непосредственно на загрузке карты, в юните MatrixMapPrepare.cpp
        if(m_ShadowProj)
        {
            if(m_ShadowType == SHADOW_PROJ_STATIC)
            {
                FreeShadowTexture();
                m_ShadowProj->DestroyTexture();

				CTextureManaged* tex = nullptr;

                const SSkin* store = nullptr;

                int st = g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_SHADOW, L"*").GetIntPar(2, L",");
                ASSERT(st < m_ShadowTexturesCount);
                if(FLAG(m_ObjectFlags, OBJECT_STATE_BURNED))
                {
                    tex = m_ShadowTextures[st].tex_burn;

                    if(!tex)
                    {
                        store = m_Graph->GetSkin();
                        m_Graph->SetSkin(m_BurnSkin);
                    }
                }
                else
                {
                    tex = m_ShadowTextures[st].tex;
                }

                if(tex)
                {
                    tex->RefInc();
                    m_ShadowProj->SetTexture(tex);
                }
                else
                {
                    int ss = g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_SHADOW, L"*").GetIntPar(1, L",");

                    if(FLAG(m_ObjectFlags, OBJECT_STATE_SHADOW_SPECIAL))
                    {
                        ShadowProjBuildTexture(this,*m_ShadowProj, *m_Graph, m_Graph->GetVOFrame(), m_Core->m_IMatrix, ss, false);
                    }
                    else
                    {
                        ShadowProjBuildTexture(*m_ShadowProj, *m_Graph, m_Graph->GetVOFrame(), m_Core->m_IMatrix, g_MatrixMap->m_LightMain, ss, false);
                    }
                }

                tex = (CTextureManaged*)m_ShadowProj->GetTexture();
                if(FLAG(m_ObjectFlags, OBJECT_STATE_BURNED))
                {
                    m_ShadowTextures[st].tex_burn = tex;
                    if(store) m_Graph->SetSkin(store);
                }
                else m_ShadowTextures[st].tex = tex;
            }
            else
            {
                int ss = (m_Type >= 0) ? g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_SHADOW, L"*").GetIntPar(1, L",") : 128;

                if(FLAG(m_ObjectFlags, OBJECT_STATE_SHADOW_SPECIAL))
                {
                    ShadowProjBuildTexture(this, *m_ShadowProj, *m_Graph, m_Graph->GetVOFrame(), m_Core->m_IMatrix, ss, true);
                }
                else
                {
                    ShadowProjBuildTexture(*m_ShadowProj, *m_Graph, m_Graph->GetVOFrame(), m_Core->m_IMatrix, g_MatrixMap->m_LightMain, ss, true);
                }
            }
        }
    }

	if(need & m_RChange & (MR_MiniMap))
    {
        m_RChange &= ~MR_MiniMap;
        g_MatrixMap->m_Minimap.RenderObjectToBackground(this);
	}
}

void CMatrixMapObject::Tact(int cms)
{
    if(m_BehaviorFlag == BEHF_PORTRET)
    {
        m_PhotoTime -= cms;
        if(m_PhotoTime < 0)
        {
            if(m_Photo) m_PhotoTime = g_MatrixMap->Rnd(3000, 5000);
            else m_PhotoTime = g_MatrixMap->Rnd(100, 200);
            m_Photo ^= 1;
            RChange(MR_Graph);
        }
    }

    if(m_Graph)
    {
		if(m_Graph->VectorObjectAnimTact(cms))
        {
            if(m_ShadowType == SHADOW_STENCIL) RChange(MR_ShadowStencil);
            else if(m_ShadowType == SHADOW_PROJ_DYNAMIC) RChange(MR_ShadowProjTex);

            if(m_BehaviorFlag == BEHF_ANIM)
            {
                if(m_Graph->IsAnimEnd())
                {
                    CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
                    CWStr temp1(g_CacheHeap);

                    int cnt = temp.GetCountPar(L"#");
                    for(int i = 0; i < cnt; ++i)
                    {
                        temp1 = temp.GetStrPar(i, L"#");
                        if(temp1.GetIntPar(0, L":") == m_AnimState)
                        {
                            //found
                            int newstate = temp1.GetIntPar(4, L":");
                            if(newstate >= 0)
                            {
                                ApplyAnimState(newstate);
                                break;
                            }
                        }
                    }
                }
            }
		}
	}
}

bool CMatrixMapObject::Pick(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt) const
{
	if(!m_Graph) return false;
    return m_Graph->Pick(m_Core->m_Matrix, m_Core->m_IMatrix, orig, dir, outt);
}

bool CMatrixMapObject::PickFull(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt) const
{
    if(!m_Graph) return false;
    return m_Graph->PickFull(m_Core->m_Matrix, m_Core->m_IMatrix, orig, dir, outt);
}

void CMatrixMapObject::DrawShadowStencil()
{
    if(m_ShadowType != SHADOW_STENCIL) return;
    m_ShadowStencil->Render(m_Core->m_Matrix);
}

void CMatrixMapObject::DrawShadowProj()
{
    /*
    if(!m_ShadowProj) return;

    D3DXMATRIX m = g_MatrixMap->GetIdentityMatrix();
    m._41 = m_ShadowProj->GetDX();
    m._42 = m_ShadowProj->GetDY();
    ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD, &m));

    m_ShadowProj->Render();

    CTexture* tex = (CTexture*)g_Cache->Get(cc_Texture, TEXTURE_PATH_SELECTION, nullptr);
    m_ShadowProj->RenderCustom(tex->Tex());
    */
}

void CMatrixMapObject::FreeDynamicResources()
{
    if(m_ShadowProj)
    {
        if(m_Graph->VO()->GetFramesCnt() > 1 && m_ShadowType == SHADOW_PROJ_DYNAMIC)
        {
            m_ShadowProj->DestroyTexture();
            RChange(MR_ShadowProjTex);
        }
        else m_ShadowProj->DX_Free();
    }
    else if(m_ShadowStencil) m_ShadowStencil->DX_Free();
}

void CMatrixMapObject::BeforeDraw()
{
	//GetResources(MR_Matrix | MR_Graph | MR_GraphSort | MR_ShadowStencil | MR_ShadowProj | MR_MiniMap);

    dword sh = (g_Config.m_ShowProjShadows ? (MR_ShadowProjTex | MR_ShadowProjGeom) : 0) | (g_Config.m_ShowStencilShadows ? MR_ShadowStencil : 0);
    GetResources(MR_Matrix | MR_Graph | MR_MiniMap | sh);

    m_Graph->BeforeDraw();
    if(m_ShadowType != SHADOW_OFF)
    {
        if(m_ShadowType == SHADOW_STENCIL && g_Config.m_ShowStencilShadows) m_ShadowStencil->BeforeRender();
        else if(g_Config.m_ShowProjShadows) m_ShadowProj->BeforeRender();
    }

    if((m_BehaviorFlag == BEHF_BURN) && (m_BurnSkinVis != 0))
    {
        m_BurnSkin->m_Preload(m_BurnSkin);
    }

    if(m_BehaviorFlag == BEHF_TERRON && m_HealthBar && m_BreakHitPoint > 0)
    {
        D3DXVECTOR3 temp = g_MatrixMap->m_Camera.GetFrustumCenter() - GetGeoCenter();
        float dist = D3DXVec3Length(&temp);

        if(dist < 1000)
        {
            D3DXVECTOR3 pos;
            float r = GetRadius() * 1.2f;
            pos = GetGeoCenter();

            r *= 0.5f;

            if(TRACE_STOP_NONE == g_MatrixMap->Trace(nullptr, g_MatrixMap->m_Camera.GetFrustumCenter(), pos, TRACE_LANDSCAPE, nullptr))
            {
                D3DXVECTOR2 p = g_MatrixMap->m_Camera.Project(pos, g_MatrixMap->GetIdentityMatrix());
                m_HealthBar->Modify(p.x - r, p.y - r * 0.5f, float(m_BreakHitPoint) / float(m_BreakHitPointMax));
            }
        }
    }
   
    if(m_BehaviorFlag == BEHF_BREAK && FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL) && m_HealthBar && m_BreakHitPoint > 0)
    {
        D3DXVECTOR3 temp = g_MatrixMap->m_Camera.GetFrustumCenter() - GetGeoCenter();
        float dist = D3DXVec3Length(&temp);

        if(dist < 1000)
        {
            D3DXVECTOR3 pos;
            pos = GetGeoCenter();
            pos.z += 20;

            if(TRACE_STOP_NONE == g_MatrixMap->Trace(nullptr, g_MatrixMap->m_Camera.GetFrustumCenter(), pos, TRACE_LANDSCAPE, nullptr))
            {
                D3DXVECTOR2 p = g_MatrixMap->m_Camera.Project(pos, g_MatrixMap->GetIdentityMatrix());
                m_HealthBar->Modify(p.x - (PB_SPECIAL_WIDTH*0.5f), p.y - GetRadius(), float(m_BreakHitPoint) / float(m_BreakHitPointMax));
            }
        }
    }

    if(m_BehaviorFlag == BEHF_ANIM && FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL) && m_HealthBar && m_BreakHitPoint > 0)
    {
        D3DXVECTOR3 temp = g_MatrixMap->m_Camera.GetFrustumCenter() - GetGeoCenter();
        float dist = D3DXVec3Length(&temp);

        if(dist < 1000)
        {
            D3DXVECTOR3 pos;
            pos = GetGeoCenter();
            pos.z += 20;

            if(TRACE_STOP_NONE == g_MatrixMap->Trace(nullptr, g_MatrixMap->m_Camera.GetFrustumCenter(), pos, TRACE_LANDSCAPE, nullptr))
            {
                D3DXVECTOR2 p = g_MatrixMap->m_Camera.Project(pos, g_MatrixMap->GetIdentityMatrix());
                m_HealthBar->Modify(p.x - (PB_SPECIAL_WIDTH*0.5f), p.y - GetRadius(), float(m_BreakHitPoint) / float(m_BreakHitPointMax));
            }
        }
    }

    //if(m_ResponseFlag == RESPF_BURN_TEX && m_BurnTex) m_BurnTex->Preload();
    //if(m_BurnTexGloss) m_BurnTexGloss->Preload();
}

void CMatrixMapObject::Draw()
{
    ASSERT(m_Graph);

    if(m_BehaviorFlag == BEHF_PORTRET)
    {
        if (!FLAG(g_MatrixMap->m_Flags, MMFLAG_SHOWPORTRETS))
        {
            if (!FLAG(g_MatrixMap->m_Flags, MMFLAG_ROBOT_IN_POSITION)) return;
        }
        m_Core->m_TerrainColor = 0xFFFFFFFF;
    }

    for(int i = 0; i < 4; ++i)
    {
        ASSERT_DX(g_D3DD->SetSamplerState(i,D3DSAMP_MIPMAPLODBIAS, *((LPDWORD) (&m_TexBias))));
    }

    if(FLAG(m_ObjectFlags, OBJECT_STATE_NORMALIZENORMALS)) g_D3DD->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

    //g_D3DD->SetRenderState(D3DRS_AMBIENT, m_TerrainColor);

	ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD, &m_Core->m_Matrix));
	
    g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR, m_Core->m_TerrainColor);

    bool ordinal = (m_BehaviorFlag != BEHF_BURN || !m_BurnSkinVis);
    if(ordinal)
    {
        m_Graph->Draw(0);
    }
    else
    {
        m_Graph->Draw((dword)this);
    }

    if(FLAG(m_ObjectFlags, OBJECT_STATE_NORMALIZENORMALS)) g_D3DD->SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);

    if(!FLAG(g_MatrixMap->m_Flags,MMFLAG_DISABLE_DRAW_OBJECT_LIGHTS))
    {
        m_Graph->DrawLightSprites(false, m_Core->m_Matrix, nullptr);
    }
}

void CMatrixMapObject::Init(int ids)
{
    if(m_Type == ids) return;

    if(m_BehaviorFlag == BEHF_TERRON)
    {
        SETFLAG(g_MatrixMap->m_Flags, MMFLAG_TERRON_ONMAP);

        if(m_HealthBar != nullptr) HDelete(CMatrixProgressBar, m_HealthBar, g_MatrixHeap);
    }
    
    if(m_BehaviorFlag == BEHF_BREAK)
    {
        if(m_HealthBar != nullptr) HDelete(CMatrixProgressBar, m_HealthBar, g_MatrixHeap);
    }

    if(m_BehaviorFlag == BEHF_ANIM)
    {
        if(m_HealthBar != nullptr) HDelete(CMatrixProgressBar, m_HealthBar, g_MatrixHeap);
    }

    if(m_BehaviorFlag == BEHF_SPAWNER && m_SpawnRobotCore)
    {
        m_SpawnRobotCore->Release();
    }

    m_UID = -1;

	if(m_Graph)
    {
        UnloadObject(m_Graph, g_MatrixHeap);
        m_Graph = nullptr;
    }

	if(m_ShadowStencil)
    {
        HDelete(CVOShadowStencil, m_ShadowStencil, g_MatrixHeap);
        m_ShadowStencil = nullptr;
    }

    // TODO : сделать, чтобы геометрия проекционной текстуры не пересчитывалась при смене объекта

	if(m_ShadowProj)
    {
        FreeShadowTexture();
        HDelete(CMatrixShadowProj, m_ShadowProj, g_MatrixHeap);
        m_ShadowProj = nullptr;
    }

    m_Type = ids;
    RChange(MR_Graph | MR_ShadowStencil | MR_ShadowProjGeom | MR_ShadowProjTex);

    m_TexBias = g_MatrixMap->IdsGet(m_Type).GetFloatPar(OTP_BIAS, L"*");

    m_BehaviorFlag = BEHF_STATIC;

    CWStr temp = CWStr(L"");

    m_ShadowType = SHADOW_OFF;
    if(g_Config.m_ObjectShadows)
    {
        temp = g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_SHADOW, L"*");
        if(temp.CompareFirst(L"Stencil"))
        {
            m_ShadowType = SHADOW_STENCIL;
        }
        else if(temp.CompareFirst(L"Proj,"))
        {
            m_ShadowType = SHADOW_PROJ_STATIC;
        }
        else if(temp.CompareFirst(L"ProjEx,"))
        {
            m_ShadowType = SHADOW_PROJ_DYNAMIC;
        }
    }

    int pcnt = g_MatrixMap->IdsGet(m_Type).GetCountPar(L"*");
    if(OTP_INVLOGIC < pcnt)
    {
        temp = g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_INVLOGIC, L"*");
        if(temp == L"1")
        {
            SETFLAG(m_ObjectFlags, OBJECT_STATE_TRACE_INVISIBLE);
        }
        else
        {
            RESETFLAG(m_ObjectFlags, OBJECT_STATE_TRACE_INVISIBLE);
        }
    }

    temp = g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*");

    if(temp[0] == '+')
    {
        temp.Del(0,1);
        ++g_MatrixMap->m_BeforeWinCount;

        SETFLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL);
    }

    if(temp.CompareFirst(L"Burn"))
    {
        m_BehaviorFlag = BEHF_BURN; 
        m_NextTime = 0;
        m_BurnTimeTotal = 0;
        m_BurnSkinVis = 0;

        if(temp.GetStrPar(2, L",") == L"Tex")
        {
            //CWStr trans(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_TEXTURE, L"*"), g_CacheHeap);
            //int idx = trans.Find(L" ? Trans");
            //if(idx >= 0) trans = L" ? Trans";
            //else trans.SetLen(0);

            m_BurnSkin = (SMatrixSkin*)CSkinManager::GetSkin((g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_PATH, L"*") + temp.GetStrPar(1, L",")).Get(), GSP_ORDINAL);
        }
    }
    else if(temp.CompareFirst(L"Break"))
    {
        m_BreakHitPoint = temp.GetIntPar(2, L",");
        m_BreakHitPointMax = m_BreakHitPoint;

        if(temp.GetStrPar(3, L",") == STR_BREAK_TYPE_TERRON)
        {
            m_BehaviorFlag = BEHF_TERRON;
            m_HealthBar = nullptr;
            AddLT();
        }
        else
        {
            m_BehaviorFlag = BEHF_BREAK;
            m_HealthBar = nullptr;
            AddLT();
        }
    }
    else if(temp.CompareFirst(L"Anim"))
    {
        if(temp[5] == 'P')
        {
            m_BehaviorFlag = BEHF_PORTRET;
            m_PrevStateRobotsInRadius = 0;
            m_PhotoTime = g_MatrixMap->Rnd(1000,2000);
            m_Photo = 0;
        }
        else
        {
            m_BehaviorFlag = BEHF_ANIM;
            ApplyAnimState(0);
            m_HealthBar = nullptr;
            AddLT();
        }
    }
    else if(temp.CompareFirst(L"Sens"))
    {
        m_BehaviorFlag = BEHF_SENS;
        m_PrevStateRobotsInRadius = -1;
        m_SensRadius = temp.GetStrPar(1, L",").GetFloatPar(0, L":");

        SetAblazeTTL(101);
        AddLT();
    }
    else if(temp.CompareFirst(L"Spawn"))
    {
        m_BehaviorFlag = BEHF_SPAWNER;
        m_PrevStateRobotsInRadius = -1;
        m_SensRadius = temp.GetStrPar(1, L",").GetFloatPar(0, L":");

        SetAblazeTTL(101 + g_MatrixMap->GetTime());
        m_SpawnRobotCore = nullptr;
        AddLT();
    }
}

void CMatrixMapObject::OnLoad()
{
    if(m_Scale > 1.2f || m_Scale < 0.7f) 
    {
        SETFLAG(m_ObjectFlags, OBJECT_STATE_NORMALIZENORMALS);
    }
}

//void CMatrixMapObject::InitAsBaseRuins(CMatrixBuilding* b, const CWStr& namev, const CWStr& namet, bool shadow)
void CMatrixMapObject::InitAsBaseRuins(const D3DXVECTOR2& pos, int angle, const CWStr& namev, const CWStr& namet, bool shadow)
{
    //m_Core->m_Matrix._41 = b->m_Pos.x;
    //m_Core->m_Matrix._42 = b->m_Pos.y;
    //m_Core->m_Matrix._43 = g_MatrixMap->GetZ(b->m_Pos.x, b->m_Pos.y);
    //m_AngleZ = b->m_Angle * GRAD2RAD(90);

    m_Core->m_Matrix._41 = pos.x;
    m_Core->m_Matrix._42 = pos.y;
    m_Core->m_Matrix._43 = g_MatrixMap->GetZ(pos.x, pos.y);
    m_AngleZ = angle * GRAD2RAD(90);

    m_AngleX = 0;
    m_AngleY = 0;

    m_Scale=1.0f;
    RESETFLAG(m_ObjectFlags, OBJECT_STATE_NORMALIZENORMALS);


    m_ShadowType = shadow?SHADOW_PROJ_DYNAMIC:SHADOW_OFF;

    m_TexBias = g_MatrixMap->m_BiasBuildings;

    m_Type = -1;

	m_RChange&=~MR_Graph;

    m_Graph = LoadObject(namev.Get(), g_MatrixHeap, false, namet.Get());
	m_Graph->SetFirstFrame();

    GetResources(MR_Matrix);
}

bool CMatrixMapObject::CalcBounds(D3DXVECTOR3 &minv, D3DXVECTOR3 &maxv)
{
    DTRACE();

    if (!m_Graph) return true;

	D3DXVECTOR3 bminout,bmaxout;

	minv.x=1e30f; minv.y=1e30f; minv.z=1e30f;
	maxv.x=-1e30f; maxv.y=-1e30f; maxv.z=-1e30f;

	int cnt=m_Graph->VO()->GetFramesCnt();
	for(int i=0;i<cnt;i++)
    {
		m_Graph->VO()->GetBound(i,m_Core->m_Matrix,bminout,bmaxout);

        minv.x = min(minv.x,bminout.x);
        minv.y = min(minv.y,bminout.y);
        minv.z = min(minv.z,bminout.z);
        maxv.x = max(maxv.x,bmaxout.x);
        maxv.y = max(maxv.y,bmaxout.y);
        maxv.z = max(maxv.z,bmaxout.z);
	}

	return false;


/*    SVOFrame *f = m_Graph->VO()->FrameGet(0);
    minv.x = f->m_MinX;
    minv.y = f->m_MinY;
    maxv.x = f->m_MaxX;
    maxv.y = f->m_MaxY;

    int cnt = m_Graph->VO()->Header()->m_FrameCnt;
    for (int i = 1; i<cnt; ++i)
    {
        SVOFrame *f = m_Graph->VO()->FrameGet(i);

        minv.x = min(minv.x,f->m_MinX);
        minv.y = min(minv.y,f->m_MinY);
        maxv.x = max(maxv.x,f->m_MaxX);
        maxv.y = max(maxv.y,f->m_MaxY);
    }

    return false;*/
}

void CMatrixMapObject::PauseTact(int cms)
{
    if(m_BehaviorFlag == BEHF_TERRON && m_HealthBar)
    {
        m_HealthBar->Modify(100000.0f, 0);
    }
    
    if(m_BehaviorFlag == BEHF_BREAK && FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL) && m_HealthBar)
    {
        m_HealthBar->Modify(100000.0f, 0);
    }

    if(m_BehaviorFlag == BEHF_ANIM && FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL) && m_HealthBar)
    {
        m_HealthBar->Modify(100000.0f, 0);
    }
}


static bool FindOnlyPlayerRobots(const D3DXVECTOR3& fpos, CMatrixMapStatic* ms, dword user)
{
    if(ms->GetSide() == PLAYER_SIDE)
    {
        *(bool*)user = true;
        return false;
    }
    return true;
}

static bool FindOnlyPlayerRobotsTgt(const D3DXVECTOR3& fpos, CMatrixMapStatic* ms, dword user)
{
    if(ms->GetSide() == PLAYER_SIDE)
    {
        *(CMatrixRobotAI**)user = ms->AsRobot();
        return false;
    }
    return true;
}

void CMatrixMapObject::LogicTact(int cms)
{
    if(m_BehaviorFlag == BEHF_TERRON)
    {
        if(m_HealthBar)
        {
            m_HealthBar->Modify(100000.0f, 0.0f);
        }

        if(FLAG(m_ObjectFlags, OBJECT_STATE_TERRON_EXPL))
        {
            SetAblazeTTL(GetAblazeTTL() - cms);

            if(GetAblazeTTL() < 0)
            {
                if(FLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL))
                {
                    RESETFLAG(m_ObjectFlags, OBJECT_STATE_SPECIAL);
                    --g_MatrixMap->m_BeforeWinCount;
                    g_MatrixMap->GetPlayerSide()->IncStatValue(STAT_BUILDING_KILL);

                    g_MatrixMap->GetPlayerSide()->SetStatus(SS_ACTIVE);

                    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_WIN);
                    //if(g_MatrixMap->m_BeforeWinCount <= 0)
                    //{
                    g_MatrixMap->GetPlayerSide()->SetStatus(SS_JUST_WIN);
                    //}
                }

                CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
                Init(temp.GetIntPar(1, L","));
            }
            else if(GetAblazeTTL() < 100 && !FLAG(m_ObjectFlags, OBJECT_STATE_TERRON_EXPL2))
            {
                if(g_Config.m_TerronConsts.explosion_weapon_type != WEAPON_NONE)
                {
                    SETFLAG(m_ObjectFlags, OBJECT_STATE_TERRON_EXPL2);
                    CSound::AddSound(S_EXPLOSION_BUILDING_BOOM4, GetGeoCenter());
                    //DCNT("boom");
                    CMatrixEffectWeapon* e = (CMatrixEffectWeapon*)CMatrixEffect::CreateWeapon(GetGeoCenter(), D3DXVECTOR3(0, 0, 1), 0, nullptr, g_Config.m_TerronConsts.explosion_weapon_type);
                    e->SetOwner(this);
                    e->FireBegin(D3DXVECTOR3(0, 0, 0), this);
                    e->Tact(1);
                    e->FireEnd();
                    e->Release();
                    //g_MatrixMap->RestoreMusicVolume();
                }
            }
            else if(GetAblazeTTL() < 1000 && !FLAG(m_ObjectFlags, OBJECT_STATE_TERRON_EXPL1))
            {
                if(g_Config.m_TerronConsts.explosion_weapon_type != WEAPON_NONE)
                {
                    SETFLAG(m_ObjectFlags, OBJECT_STATE_TERRON_EXPL1);
                    CSound::AddSound(S_EXPLOSION_BUILDING_BOOM4, GetGeoCenter());
                    //DCNT("boom");
                    CMatrixEffectWeapon* e = (CMatrixEffectWeapon*)CMatrixEffect::CreateWeapon(GetGeoCenter(), D3DXVECTOR3(0, 0, 1), 0, nullptr, g_Config.m_TerronConsts.explosion_weapon_type);
                    e->SetOwner(this);
                    e->FireBegin(D3DXVECTOR3(0, 0, 0), this);
                    e->Tact(1);
                    e->FireEnd();
                    e->Release();
                }
            }
            else
            {
                // explosions
                while(g_MatrixMap->GetTime() > m_NextExplosionTimeSound)
                {
                    m_NextExplosionTimeSound += IRND(BUILDING_EXPLOSION_PERIOD_SND_2-BUILDING_EXPLOSION_PERIOD_SND_1) + BUILDING_EXPLOSION_PERIOD_SND_1;
                    CSound::AddSound(S_EXPLOSION_BUILDING_BOOM, GetGeoCenter());
                }
                while(g_MatrixMap->GetTime() > m_NextExplosionTime)
                {
                    m_NextExplosionTime += BUILDING_EXPLOSION_PERIOD;

                    D3DXVECTOR3 dir, pos, pos0;
                    float t;

                    int cnt = 4;
                    do
                    {
                        //pos0.x = m_Pos.x - m_Core->m_Matrix._21 * 60;
                        //pos0.y = m_Pos.y - m_Core->m_Matrix._22 * 60;
                        //pos0.z = m_Core->m_Matrix._43;
                        pos0 = GetGeoCenter();

                        pos = pos0 + D3DXVECTOR3(FSRND(GetRadius()), FSRND(GetRadius()), FRND(2 * GetRadius()));

                        D3DXVECTOR3 temp = pos0 - pos;
                        D3DXVec3Normalize(&dir, &temp);
                    }
                    while(!Pick(pos, dir, &t) && (--cnt > 0));

                    if(cnt > 0)
                    {
                        //CHelper::Create(1,0)->Line(pos, pos+dir * 10);

                        if(FRND(1) < 0.04f) CMatrixEffect::CreateExplosion(pos + dir * (t + 2), ExplosionBuildingBoom2);
                        else CMatrixEffect::CreateExplosion(pos + dir * (t + 2), ExplosionBuildingBoom);
                    }
                }
            }

        }
        else if(m_Graph->IsAnimEnd()) m_Graph->SetAnimByName(L"Main");
        return;
    }
    else if(m_BehaviorFlag == BEHF_BREAK)
    {
        if(m_HealthBar) m_HealthBar->Modify(100000.0f, 0.0f);
    }
    else if(m_BehaviorFlag == BEHF_ANIM)
    {
        if(m_HealthBar) m_HealthBar->Modify(100000.0f, 0.0f);
    }
    else if(m_BehaviorFlag == BEHF_SPAWNER)
    {
        if(m_PrevStateRobotsInRadius < 0)
        {
            m_PrevStateRobotsInRadius = 0;
            CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
            m_Graph->SetAnimById(temp.GetStrPar(1,L",").GetIntPar(4,L":"));
        }
        else if(!m_PrevStateRobotsInRadius)
        {
            if(g_MatrixMap->GetTime() > GetAblazeTTL())
            {
                int addt = 107;
                bool found = false;
                g_MatrixMap->FindObjects(*(D3DXVECTOR3*)&m_Core->m_Matrix._41, m_SensRadius, 1, TRACE_ROBOT, nullptr, FindOnlyPlayerRobots, (dword)&found);
                if(found)
                {
                    CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
                    m_PrevStateRobotsInRadius = 1;
                    addt = temp.GetStrPar(1, L",").GetIntPar(1, L":");

                    m_Graph->SetAnimById(temp.GetStrPar(1, L",").GetIntPar(5, L":"), 0);

                    CWStr snd(temp.GetStrPar(1, L",").GetStrPar(8, L":"), g_CacheHeap);
                    if(!snd.IsEmpty()) CSound::AddSound(snd.Get(), *(D3DXVECTOR3*)&m_Core->m_Matrix._41);
                }
                SetAblazeTTL(g_MatrixMap->GetTime() + addt);
            }
        }
        //Происходит автоспавн роботов из "сортиров" Террона в случае, если рядом с "сортирами" обнаружены роботы игрока
        else if(m_PrevStateRobotsInRadius == 1)
        {
            //waiting before spawn
            if(m_Graph->IsAnimEnd())
            {
                //spawn robot
                CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);

                int robot;
                if(!temp.GetStrPar(1, L",").TrimFull().IsEmpty()) robot = g_MatrixMap->Rnd(temp.GetStrPar(1, L",").GetIntPar(2, L":"), temp.GetStrPar(1, L",").GetIntPar(3, L":"));
                else robot = g_MatrixMap->Rnd(0, SRobotTemplate::m_AIRobotTypesList[ROBOTS_SPAWNER_TEMPLATES].size() - 1);

                SRobotTemplate bot = SRobotTemplate::m_AIRobotTypesList[ROBOTS_SPAWNER_TEMPLATES][robot];
                CMatrixRobotAI* r = bot.CreateRobotByTemplate(*(D3DXVECTOR3*)&m_Core->m_Matrix._41, 4 /* Terron? */);
                g_MatrixMap->AddObject(r, true);
                r->CreateTextures();

                m_SpawnRobotCore = r->GetCore(DEBUG_CALL_INFO);
                *(D3DXVECTOR3*)&m_SpawnRobotCore->m_Matrix._41 = *(D3DXVECTOR3*)&m_Core->m_Matrix._41;

                r->m_CalcBoundsLastTime = g_MatrixMap->GetTime() - 10000;
                D3DXVECTOR3 minv, maxv;
                r->RChange(MR_Matrix);
                r->CalcBounds(minv, maxv);
                m_SpawnRobotCore->m_GeoCenter = (minv + maxv) * 0.5f;

                D3DXVECTOR3 temp_vec = minv - maxv;
                m_SpawnRobotCore->m_Radius = D3DXVec3Length(&temp_vec);

                m_Graph->SetAnimById(temp.GetStrPar(1, L",").GetIntPar(6, L":"), 0);

                CWStr snd(temp.GetStrPar(1, L",").GetStrPar(9, L":"), g_CacheHeap);
                if(!snd.IsEmpty()) CSound::AddSound(snd.Get(), *(D3DXVECTOR3*)&m_Core->m_Matrix._41);
                m_PrevStateRobotsInRadius = 2;
            }
        }
        else if(m_PrevStateRobotsInRadius == 2)
        {
            if(m_Graph->IsAnimEnd() && m_SpawnRobotCore)
            {
                if(m_SpawnRobotCore->m_Object)
                {
                    CMatrixRobotAI* r = m_SpawnRobotCore->m_Object->AsRobot();

                    CMatrixRobotAI* tgt = nullptr;
                    g_MatrixMap->FindObjects(*(D3DXVECTOR3*)&m_Core->m_Matrix._41, m_SensRadius, 1, TRACE_ROBOT, nullptr, FindOnlyPlayerRobotsTgt, (dword)&tgt);

                    CPoint pos(Float2Int(m_Core->m_Matrix._41 * INVERT(GLOBAL_SCALE_MOVE)), Float2Int(m_Core->m_Matrix._42 * INVERT(GLOBAL_SCALE_MOVE)));
                    
                    if(tgt)
                    {
                        pos.x = Float2Int(tgt->m_PosX * INVERT(GLOBAL_SCALE_MOVE));
                        pos.y = Float2Int(tgt->m_PosY * INVERT(GLOBAL_SCALE_MOVE));
                    }

                    CMatrixSideUnit *su = g_MatrixMap->GetSideById(r->GetSide());

                    SETFLAG(g_MatrixMap->m_Flags, MMFLAG_SOUND_ORDER_ATTACK_DISABLE);
                    su->PGOrderAttack(su->RobotToLogicGroup(r), pos,tgt);
                    RESETFLAG(g_MatrixMap->m_Flags, MMFLAG_SOUND_ORDER_ATTACK_DISABLE);
                }

                m_SpawnRobotCore->Release();
                m_SpawnRobotCore = nullptr;

                CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
                m_Graph->SetAnimById(temp.GetStrPar(1, L",").GetIntPar(7, L":"),0);

                CWStr snd(temp.GetStrPar(1, L",").GetStrPar(10, L":"), g_CacheHeap);
                if(!snd.IsEmpty()) CSound::AddSound(snd.Get(), *(D3DXVECTOR3*)&m_Core->m_Matrix._41);
                m_PrevStateRobotsInRadius = 3;
            }
        }
        else if(m_PrevStateRobotsInRadius == 3)
        {
            if(m_Graph->IsAnimEnd())
            {
                CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
                m_Graph->SetAnimById(temp.GetStrPar(1, L",").GetIntPar(4, L":"));
                m_PrevStateRobotsInRadius = 0;
            }
        }
        return;

    }
    else if(m_BehaviorFlag == BEHF_SENS)
    {
        if(m_PrevStateRobotsInRadius < 0)
        {
            m_PrevStateRobotsInRadius = 0;
            CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
            m_Graph->SetAnimById(temp.GetStrPar(1, L",").GetIntPar(1, L":"));
        }

        SetAblazeTTL(GetAblazeTTL() - cms);
        if(GetAblazeTTL() < 0)
        {
            while(GetAblazeTTL() < 0) SetAblazeTTL(GetAblazeTTL() + 107);

            if(g_MatrixMap->FindObjects(*(D3DXVECTOR3*)&m_Core->m_Matrix._41, m_SensRadius, 1, TRACE_ROBOT, nullptr, nullptr, 0))
            {
                if(!m_PrevStateRobotsInRadius)
                {
                    CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
                    m_Graph->SetAnimById(temp.GetStrPar(1, L",").GetIntPar(2, L":"));
                    m_PrevStateRobotsInRadius = 1;

                    if(temp.GetStrPar(1, L",").GetCountPar(L":") >= 5)
                    {
                        CSound::AddSound(temp.GetStrPar(1, L",").GetStrPar(4, L":").Get(), GetGeoCenter());
                        //CSound::Play(temp.GetStrPar(1, L",").GetStrPar(4, L":").Get(), GetGeoCenter());
                    }
                }
            }
            else
            {
                if(m_PrevStateRobotsInRadius != 0)
                {
                    CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
                    m_Graph->SetAnimById(temp.GetStrPar(1, L",").GetIntPar(3, L":"));
                    m_PrevStateRobotsInRadius = 0;

                    if(temp.GetStrPar(1, L",").GetCountPar(L":") >= 6)
                    {
                        CSound::AddSound(temp.GetStrPar(1, L",").GetStrPar(5, L":"), GetGeoCenter());
                    }
                }
            }
        }
        return;
    }
    
	if(IsAblaze())
    {
        int cur_ablaze_num = IsAblaze();

        m_BurnTimeTotal += cms;
        while(g_MatrixMap->GetTime() > m_NextTime)
        {
            m_NextTime += ABLAZE_LOGIC_PERIOD;

            if(IRND(10000) < GetAblazeTTL())
            {
                D3DXVECTOR3 dir, pos;
                float t;

                int cnt = 4;
                do
                {
                    pos.x = m_Core->m_Matrix._41 + FSRND(GetRadius());
                    pos.y = m_Core->m_Matrix._42 + FSRND(GetRadius());
                    pos.z = m_Core->m_Matrix._43 + FRND(GetRadius() * 2);

                    D3DXVECTOR3 temp = { m_Core->m_Matrix._41 - pos.x, m_Core->m_Matrix._42 - pos.y, m_Core->m_Matrix._43 - pos.z };
                    D3DXVec3Normalize(&dir, &temp);
                    
                } while(!PickFull(pos, dir, &t) && (--cnt > 0));

                if(cnt > 0)
                {
                    if(GetAblazeTTL() > g_Config.m_WeaponsConsts[cur_ablaze_num].map_objects_ignition.burning_starts_at) //Спустя некоторое время начинается активное горение объекта
                    {
                        CMatrixEffect::CreateFireAnim(nullptr, pos + dir * (t + 6), 20, 30 + FRND(20), GetAblazeTTL() - Float2Int(FRND(4000)), g_Config.m_WeaponsConsts[cur_ablaze_num].map_objects_ignition.sprites_num);
                        CMatrixEffect::CreateSmoke(nullptr, pos + dir * (t + 6) + D3DXVECTOR3(0, 0, 10), 100, 1300, 20, 0x80303030, false, 1.0f / 30.0f);

                        //На декоративных объектах это выглядит как куча жёлтого говна
                        //CMatrixEffect::CreateFire(nullptr, pos + dir * (t + 2), 100, 2500, 10, 2.5f, false);
                    }
                    else //До начала горения объект просто дымится
                    {
                        CMatrixEffect::CreateSmoke(nullptr, pos + dir * (t + 2), 100, 1300, 20, 0x40303030, false, 1.0f / 30.0f);
                    }
                    //CMatrixEffect::CreateFire(nullptr, pos + dir * (t + 2), 100, 2000, 10, 2, false);
                }

                //Если объект был уничтожен пламенем
                if(TakingDamage(cur_ablaze_num, pos, dir, 0, this)) return; //Нанесение объектом урона самому себе трактуется как горение
            }
        }

        if(!FLAG(m_ObjectFlags, OBJECT_STATE_BURNED))
        {
            CWStr temp(g_MatrixMap->IdsGet(m_Type).GetStrPar(OTP_BEHAVIOUR, L"*"), g_CacheHeap);
            CWStr bt(temp.GetStrPar(2, L","), g_CacheHeap);
            if(m_BurnTimeTotal > 5000)
            {
                SETFLAG(m_ObjectFlags, OBJECT_STATE_BURNED);
                
                if(bt == L"Type")
                {
                    // replace object
                    Init(temp.GetIntPar(1, L","));
                    m_BehaviorFlag = BEHF_STATIC; // сгорела :(
                }
                else if(m_BurnSkin->m_Tex->Flags() & TF_ALPHATEST) RChange(MR_ShadowProjTex);
            }
            else if(bt == L"Tex") m_BurnSkinVis = Float2Int(float(m_BurnTimeTotal) * (255.0f / 5000.0f));
        }
    }
    else
    {
        //if(InLT()) DCNT("lt_out");
        DelLT();
    }
}

bool CMatrixMapObject::InRect(const CRect& rect) const
{
    D3DXVECTOR3 dir;
    g_MatrixMap->m_Camera.CalcPickVector(CPoint(rect.left, rect.top), dir);
    if(Pick(g_MatrixMap->m_Camera.GetFrustumCenter(), dir, nullptr)) return true;

    D3DXMATRIX s, t;
    SEVH_data d;

    d.m = m_Core->m_Matrix * g_MatrixMap->m_Camera.GetViewMatrix() * g_MatrixMap->m_Camera.GetProjMatrix();
    D3DXMatrixScaling(&s, float(g_ScreenX / 2), float(-g_ScreenY / 2), 1);
    s._41 = s._11;
    s._42 = float(g_ScreenY / 2);
    d.m *= s;
    d.rect = &rect;
    d.found = false;
    m_Graph->EnumFrameVerts(EnumVertsHandler, (dword)&d);
    if(d.found) return true;
    
    g_MatrixMap->m_Camera.CalcPickVector(CPoint(rect.left, rect.bottom), dir);
    if (Pick(g_MatrixMap->m_Camera.GetFrustumCenter(), dir, nullptr)) return true;

    g_MatrixMap->m_Camera.CalcPickVector(CPoint(rect.right, rect.top), dir);
    if (Pick(g_MatrixMap->m_Camera.GetFrustumCenter(), dir, nullptr)) return true;
    
    g_MatrixMap->m_Camera.CalcPickVector(CPoint(rect.right, rect.bottom), dir);
    if (Pick(g_MatrixMap->m_Camera.GetFrustumCenter(), dir, nullptr)) return true;

    return false;
}
