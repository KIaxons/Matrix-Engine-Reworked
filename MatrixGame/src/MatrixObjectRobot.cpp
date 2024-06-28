// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "stdafx.h"
#include "MatrixObject.hpp"
#include "MatrixObjectBuilding.hpp"
#include "MatrixObjectRobot.hpp"
#include "MatrixShadowManager.hpp"
#include "MatrixFlyer.hpp"
#include "MatrixRenderPipeline.hpp"
#include "ShadowStencil.hpp"
#include "MatrixRobot.hpp"


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//Непосредственное подключение модели указанного модуля (но не оружия) к общей модели робота
void CMatrixRobot::ModuleInsert(int before_module, ERobotModuleType type, ERobotModuleKind kind)
{
    ASSERT(m_ModulesCount < MR_MAX_MODULES);
    ASSERT(before_module >= 0 && before_module <= m_ModulesCount);

    if(before_module < m_ModulesCount) MoveMemory(m_Module + before_module + 1, m_Module + before_module, (m_ModulesCount - before_module) * sizeof(SMatrixRobotModule));
    ZeroMemory(m_Module + before_module, sizeof(SMatrixRobotModule));

	++m_ModulesCount;

	m_Module[before_module].m_Type = type;
	m_Module[before_module].m_Kind = kind;
	D3DXMatrixIdentity(&m_Module[before_module].m_Matrix);

    m_Module[before_module].m_NextAnimTime = (float)g_MatrixMap->GetTime();
	
	RChange(MR_ShadowStencil | MR_ShadowProjGeom | MR_ShadowProjTex);

    if(type == MRT_CHASSIS)
    {
        //Создаём объекты эффектов реактивных струй для шасси робота и заносим их в массив указателей
        int cnt = g_Config.m_RobotChassisConsts[kind].jet_stream.size();
        for(int i = 0; i < cnt; ++i)
        {
            SRobotSpriteAnims* anim_pars = &g_Config.m_RobotChassisConsts[kind].jet_stream[i];
            CMatrixEffectFireStream* str = CMatrixEffect::CreateFireStream(D3DXVECTOR3(0.0f, 0.0f, 0.0f), D3DXVECTOR3(10.0f, 0.0f, 0.0f), anim_pars->width, anim_pars->color, anim_pars->sprites_num);
            m_ChassisData.JetStream.push_back(str);
        }

        //Создаём объекты эффектов декоративных спрайтовых анимаций для шасси робота и заносим их в массив указателей
        cnt = g_Config.m_RobotChassisConsts[kind].linked_sprite_anim.size();
        for(int i = 0; i < cnt; ++i)
        {
            SRobotSpriteAnims* anim_pars = &g_Config.m_RobotChassisConsts[kind].linked_sprite_anim[i];
            CMatrixEffectLinkedSpriteAnim* str = CMatrixEffect::CreateLinkedSpriteAnim(D3DXVECTOR3(0.0f, 0.0f, 0.0f), D3DXVECTOR3(10.0f, 0.0f, 0.0f), anim_pars->width, anim_pars->color, anim_pars->frame_delay, anim_pars->camera_oriented, anim_pars->sprites_num);
            m_ChassisData.LinkedSpriteAnim.push_back(str);
        }
        
        //Инициализируем вектор для отрисовки следов от колёсных/гусеничных типов шасси
        if(g_Config.m_RobotChassisConsts[kind].is_rolling)
        {
            m_ChassisData.m_LastSolePos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        }

        //По умолчанию у всех шасси выставлено NORMAL_MOVING_ANIM, равное 0
        if(g_Config.m_RobotChassisConsts[kind].moving_anim_type)
        {
            m_ChassisData.m_MovingAnimType = g_Config.m_RobotChassisConsts[kind].moving_anim_type;
        }
        m_ChassisData.m_MovingAnimSpeed = g_Config.m_RobotChassisConsts[kind].anim_move_speed;
    }
    else if(type == MRT_HULL) m_HullModule = &m_Module[before_module];
}

//Непосредственное подключение модели указанного оружия к общей модели робота
void CMatrixRobot::WeaponInsert(
    int before_module,
    ERobotModuleType type,
    ERobotModuleKind kind,
    int hull_num,
    int model_pilon_num
)
{
    ASSERT(m_ModulesCount < MR_MAX_MODULES);
	ASSERT(before_module >= 0 && before_module <= m_ModulesCount);

    if(before_module < m_ModulesCount) MoveMemory(m_Module + before_module + 1, m_Module + before_module, (m_ModulesCount - before_module) * sizeof(SMatrixRobotModule));
    ZeroMemory(m_Module + before_module, sizeof(SMatrixRobotModule));

	++m_ModulesCount;

	m_Module[before_module].m_Type = type;
	m_Module[before_module].m_Kind = kind;

    m_Module[before_module].m_LinkMatrix = g_Config.m_RobotHullsConsts[hull_num].weapon_pylon_data[model_pilon_num].id;
    m_Module[before_module].m_Invert = g_Config.m_RobotHullsConsts[hull_num].weapon_pylon_data[model_pilon_num].inverted_model;
    m_Module[before_module].m_ConstructorPylonNum = model_pilon_num;

    D3DXMatrixIdentity(&m_Module[before_module].m_Matrix);

    m_Module[before_module].m_NextAnimTime = (float)g_MatrixMap->GetTime();

	RChange(MR_ShadowStencil | MR_ShadowProjTex);
}

void CMatrixRobot::ModuleDelete(int module_num)
{
	ASSERT(module_num >= 0 && module_num < m_ModulesCount);

    //Очищаем объекты реактивных струй и прочих декоративных спрайтовых анимаций для шасси робота
    if(m_CurrentState != ROBOT_DIP && m_Module[module_num].m_Type == MRT_CHASSIS)
    {
        int cnt = g_Config.m_RobotChassisConsts[m_Module[module_num].m_Kind].jet_stream.size();
        if(cnt)
        {
            for(int i = 0; i < cnt; ++i)
            {
                m_ChassisData.JetStream[i]->Release();
            }
            m_ChassisData.JetStream.clear();
        }

        cnt = g_Config.m_RobotChassisConsts[m_Module[module_num].m_Kind].linked_sprite_anim.size();
        if(cnt)
        {
            for(int i = 0; i < cnt; ++i)
            {
                m_ChassisData.LinkedSpriteAnim[i]->Release();
            }
            m_ChassisData.LinkedSpriteAnim.clear();
        }
    }

	if(m_Module[module_num].m_Graph)
    {
        UnloadObject(m_Module[module_num].m_Graph, g_MatrixHeap);
        m_Module[module_num].m_Graph = nullptr;
    }

    if(m_CurrentState == ROBOT_DIP)
    {
        if(m_Module[module_num].Smoke().effect)
        {
            ((CMatrixEffectSmoke*)m_Module[module_num].Smoke().effect)->SetTTL(1000);
            m_Module[module_num].Smoke().Unconnect();
        }
    }
    else
    {
        if(m_Module[module_num].m_WeaponRepairData)
        {
            m_Module[module_num].m_WeaponRepairData->Release();
            m_Module[module_num].m_WeaponRepairData = nullptr;
        }
	    if(m_Module[module_num].m_ShadowStencil)
        {
            HDelete(CVOShadowStencil,m_Module[module_num].m_ShadowStencil,g_MatrixHeap);
            m_Module[module_num].m_ShadowStencil=nullptr;
        }
    }

    if(module_num < (m_ModulesCount - 1)) memcpy(m_Module + module_num, m_Module + module_num + 1, (m_ModulesCount - 1 - module_num) * sizeof(SMatrixRobotModule));
	--m_ModulesCount;
    memset(m_Module + m_ModulesCount, 0, sizeof(SMatrixRobotModule));

    if(m_CurrentState == ROBOT_DIP)
    {
        while(module_num < m_ModulesCount)
        {
            m_Module[module_num].Smoke().Rebase();
            ++module_num;
        }
    }

	RChange(MR_ShadowStencil | MR_ShadowProjGeom | MR_ShadowProjTex);
}

void CMatrixRobot::ModulesClear(void)
{
    while(m_ModulesCount)
    {
        ModuleDelete(m_ModulesCount - 1);
    }

	RChange(MR_ShadowStencil | MR_ShadowProjGeom | MR_ShadowProjTex);
}

void CMatrixRobot::BoundGet(D3DXVECTOR3& bmin, D3DXVECTOR3& bmax)
{
    bmin = D3DXVECTOR3(1e30f, 1e30f, 1e30f);
    bmax = D3DXVECTOR3(-1e30f, -1e30f, -1e30f);

	D3DXVECTOR3 tmin;
	D3DXVECTOR3 tmax;
	D3DXVECTOR3 v[8];

	for(int i = 0; i < m_ModulesCount; ++i)
    {
        m_Module[i].m_Graph->GetBound(tmin, tmax);

        v[0] = D3DXVECTOR3(tmin.x, tmin.y, tmin.z);
        v[1] = D3DXVECTOR3(tmax.x, tmin.y, tmin.z);
        v[2] = D3DXVECTOR3(tmax.x, tmax.y, tmin.z);
        v[3] = D3DXVECTOR3(tmin.x, tmax.y, tmin.z);
        v[4] = D3DXVECTOR3(tmin.x, tmin.y, tmax.z);
        v[5] = D3DXVECTOR3(tmax.x, tmin.y, tmax.z);
        v[6] = D3DXVECTOR3(tmax.x, tmax.y, tmax.z);
        v[7] = D3DXVECTOR3(tmin.x, tmax.y, tmax.z);

        for(int u = 0; u < 8; ++u)
        {
            D3DXVec3TransformCoord(&(v[u]), &(v[u]), &(m_Module[i].m_Matrix));
            bmin.x = min(bmin.x, v[u].x);
            bmin.y = min(bmin.y, v[u].y);
            bmin.z = min(bmin.z, v[u].z);
            bmax.x = max(bmax.x, v[u].x);
            bmax.y = max(bmax.y, v[u].y);
            bmax.z = max(bmax.z, v[u].z);
        }
	}
}

float CMatrixRobot::GetChassisHeight() const
{
    // 1st matrix, ConnectUp, chassis connector
    const D3DXMATRIX* tm = m_Module[0].m_Graph->GetMatrixById(1);
    // get z coord from matrix
    return tm->_43;
}

float CMatrixRobot::GetEyeHeight(bool from_the_floor) const
{
    // initialize with chassis height, worst case :)
    float eye_level = 0.0f;
    if(from_the_floor) eye_level += GetChassisHeight();

    // 1st matrix, ConnectUp, head connector
    const D3DXMATRIX* tm = m_HullModule->m_Graph->GetMatrixById(1);
    // get z coord from matrix, add to chassis height
    return eye_level += tm->_43;
}

float CMatrixRobot::GetHullHeight() const
{
    //Ищем наиболее высоко расположенную матрицу на корпусе
    float max_height = 0;
    for(int j = 0; j < m_HullModule->m_Graph->GetMatrixCount(); ++j)
    {
        const D3DXMATRIX* tm = m_HullModule->m_Graph->GetMatrix(j);
        if(tm->_43 > max_height) max_height = tm->_43;
    }
    return max_height;
}

float CMatrixRobot::Z_From_Pos()
{
    float robot_z = g_MatrixMap->GetZ(m_PosX, m_PosY);
    if(robot_z < WATER_LEVEL)
    {
        SETFLAG(m_ObjectFlags, ROBOT_FLAG_ON_WATER);

        //Роботы на парящих шасси провалиться под воду не могут
        if(IsHoveringChassis()) robot_z = WATER_LEVEL;

        if(robot_z < WATER_LEVEL - 100)
        {
            //Робот утонул (упал под карту)
            MustDie();
        }
    }
    else RESETFLAG(m_ObjectFlags, ROBOT_FLAG_ON_WATER);

    return robot_z;
}

//Подгружаем необходимые модели и текстуры для роботов
void CMatrixRobot::GetResources(dword need)
{
    if(m_CurrentState == ROBOT_DIP || IsMustDie()) return;

	if(need & m_RChange & (MR_Graph))
    {
        m_RChange &= ~MR_Graph;

        //float hp = (float)g_Config.m_RobotHitPoint;
        //InitMaxHitpoints(hp);

        CWStr texture(g_CacheHeap), temp(g_CacheHeap);
        CWStr path(g_CacheHeap);//, path_e(g_CacheHeap);

        for(int i = 0; i < m_ModulesCount; ++i)
        {
            if(!m_Module[i].m_Graph)
            {
                switch(m_Module[i].m_Type)
                {
                    case MRT_HULL: path = g_Config.m_RobotHullsConsts[m_Module[i].m_Kind].model_path; break;
                    case MRT_CHASSIS: path = g_Config.m_RobotChassisConsts[m_Module[i].m_Kind].model_path; break;
                    case MRT_HEAD: path = g_Config.m_RobotHeadsConsts[m_Module[i].m_Kind].model_path; break;
                    case MRT_WEAPON: path = g_Config.m_RobotWeaponsConsts[m_Module[i].m_Kind].model_path; break;
                    default: ERROR_S2(L"Unknown robot unit type ", CWStr(m_Module[i].m_Type));
                }

                // below we expect that proper cache.txt being used
                if(m_Side != PLAYER_SIDE)
                {
                    //path_e = path + L".Enemy";
                    texture = g_CacheData->ParPathGet(path + L".TextureEnemy");
                    //if(type != MRT_HEAD) texture = g_CacheData->ParPathGet(path + L".Texture");
                    //else texture = g_CacheData->ParPathGet(path + L".Texture");
                    
                    // if enemy's texture has own gloss map, use it
                    if(CFile::FileExist(temp, (g_CacheData->ParPathGet(path + L".TextureEnemy") + GLOSS_TEXTURE_SUFFIX).Get(), L"dds~png"))
                    {
                        texture += L"*" + temp;
                    }
                    // else fallback to gloss map of player's texture
                    else if(CFile::FileExist(temp, (g_CacheData->ParPathGet(path + L".Texture") + GLOSS_TEXTURE_SUFFIX).Get(), L"dds~png"))
                    {
                        texture += L"*" + temp;
                    }
                }
                else texture = g_CacheData->ParPathGet(path + L".Texture");

                m_Module[i].m_Graph = LoadObject((g_CacheData->ParPathGet(path + L".Model") + L".vo").Get(), g_MatrixHeap, true, texture);
                //m_Module[i].m_Graph->SetAnimByName(ANIMATION_NAME_IDLE);
                m_Module[i].m_Graph->SetAnimByName(ANIMATION_NAME_STAY);
			}
		}

        //Задаём битсет имеющихся у шасси данного робота набора анимаций, для ускорения проверок их переключения в SwitchAnimation()
        //Перебор одних и тех же моделей для каждого построенного робота - это тупо, но как ещё больше улучшить систему я пока не придумал
        CVectorObjectAnim* model_anim = m_Module[0].m_Graph;
        CVectorObject* model = model_anim->VO();
        for(int i = 1; i < ANIMATIONS_COUNT; ++i)
        {
            if(model->GetAnimByName(AnimsToNames[i].name) > -1) model_anim->SetAnimAvailability(i, true);
        }
	}

    if(need & m_RChange & (MR_Matrix))
    {
        m_RChange &= ~MR_Matrix;
        float robot_z;
        D3DXVECTOR3 side, up;
        if(m_CurrentState == ROBOT_IN_SPAWN || m_CurrentState == ROBOT_CAPTURING_BASE)
        {
            //Получаем базу по текущим координатам робота
            SMatrixMapUnit* mu = g_MatrixMap->UnitGet(TruncFloat(m_PosX * INVERT(GLOBAL_SCALE)), TruncFloat(m_PosY * INVERT(GLOBAL_SCALE)));
            if(mu->m_MapUnitBase) robot_z = mu->m_MapUnitBase->GetFloorZ(); //Поднимаем/опускаем робота вместе с лифтом базы
            else //Если по каким-то причинам робот потерял базу в тот момент, когда никак не должен был этого делать (либо же базу просто уничтожили) - это повод его убить
            {
                MustDie();
                return;
            }
            SwitchAnimation(ANIMATION_STAY);
        }
        else if(m_CurrentState == ROBOT_CARRYING)
        {
            D3DXVec3Normalize(&up, &m_CargoFlyer->GetCarryData()->m_RobotUp);
            D3DXVec3Cross(&side, &m_ChassisForward, &up);
            D3DXVec3Normalize(&side, &side);
            D3DXVec3Cross(&m_ChassisForward, &up, &side);

            m_Core->m_Matrix._11 = side.x;             m_Core->m_Matrix._12 = side.y;             m_Core->m_Matrix._13 = side.z;             m_Core->m_Matrix._14 = 0;
            m_Core->m_Matrix._21 = m_ChassisForward.x; m_Core->m_Matrix._22 = m_ChassisForward.y; m_Core->m_Matrix._23 = m_ChassisForward.z; m_Core->m_Matrix._24 = 0;
            m_Core->m_Matrix._31 = up.x;               m_Core->m_Matrix._32 = up.y;               m_Core->m_Matrix._33 = up.z;               m_Core->m_Matrix._34 = 0;
            //m_Core->m_Matrix._41 = m_PosX;           m_Core->m_Matrix._42 = m_PosY;             m_Core->m_Matrix._43 = roboz;                        
            m_Core->m_Matrix._44 = 1; 

            m_HullForward = m_ChassisForward;
           
            SwitchAnimation(ANIMATION_OFF);

            goto skip_matrix;
        }
        else if(m_CurrentState == ROBOT_BASE_MOVEOUT)
        {
            //Получение базы по координатам рождающегося робота (по какой-то убогой причине реализовали именно так)
            SMatrixMapUnit* u = g_MatrixMap->UnitGet(TruncFloat(m_PosX * INVERT(GLOBAL_SCALE)), TruncFloat(m_PosY * INVERT(GLOBAL_SCALE)));
            if(u->IsLand() || u->IsWater()) robot_z = g_MatrixMap->GetZ(m_PosX, m_PosY);
            else robot_z = u->m_MapUnitBase->GetFloorZ();

            SwitchAnimation(ANIMATION_MOVE);
        }
        else if(m_CurrentState == ROBOT_FALLING)
        {
            SwitchAnimation(ANIMATION_OFF);
            goto skip_matrix;
        }
        else if(m_CurrentState == ROBOT_EMBRYO)
        {
            robot_z = 0;
            SwitchAnimation(ANIMATION_STAY);
        }
        else
        {
            robot_z = Z_From_Pos();
        }

        {
            D3DXVECTOR3 tmp_forward;
           
            //D3DXVec3Normalize(&up, (D3DXVECTOR3*)&m_Module[0].m_Matrix._31);
            D3DXVec3Normalize(&up, (D3DXVECTOR3*)&m_Core->m_Matrix._31);
            //g_MatrixMap->GetNormal(&up, m_PosX,m_PosY);
            D3DXVec3Cross(&side, &m_ChassisForward, &up);
            D3DXVec3Normalize(&side, &side);
            D3DXVec3Cross(&tmp_forward, &up, &side);

            m_Core->m_Matrix._11 = side.x;        m_Core->m_Matrix._12 = side.y;        m_Core->m_Matrix._13 = side.z;        m_Core->m_Matrix._14 = 0;
            m_Core->m_Matrix._21 = tmp_forward.x; m_Core->m_Matrix._22 = tmp_forward.y; m_Core->m_Matrix._23 = tmp_forward.z; m_Core->m_Matrix._24 = 0;
            m_Core->m_Matrix._31 = up.x;          m_Core->m_Matrix._32 = up.y;          m_Core->m_Matrix._33 = up.z;          m_Core->m_Matrix._34 = 0;
            m_Core->m_Matrix._41 = m_PosX;        m_Core->m_Matrix._42 = m_PosY;        m_Core->m_Matrix._43 = robot_z;       m_Core->m_Matrix._44 = 1;
        }
skip_matrix:
        /*
        if(m_Module[0].m_Kind == RUK_CHASSIS_HOVERCRAFT)
        {
            double a = ((((dword)this) >> 3) & 1024) / 1024 * M_PI + double(g_MatrixMap->GetTime() & 1023) / 1024 * 2 * M_PI;
            double r = 1;

            m_Matrix._43 += float(r * sin(a));
        }
        */

        D3DXMatrixInverse(&m_Core->m_IMatrix, nullptr, &m_Core->m_Matrix);

		//int cntweaponset = 0;
		int hull_num = -1;

		D3DXMATRIX m;
        const D3DXMATRIX* tm;
        D3DXVECTOR3 p;

        // calc main matrix
        {
		    ASSERT(m_Module[0].m_Type == MRT_CHASSIS);
            ASSERT(m_Module[0].m_Graph);
            tm = m_Module[0].m_Graph->GetMatrixById(1);
            p = *(D3DXVECTOR3*)&tm->_41;

            m_Module[0].m_Matrix = m_Core->m_Matrix;
            m_Module[0].m_IMatrix = m_Core->m_IMatrix;
        }

        bool www = false;

		for(int i = 1; i < m_ModulesCount; ++i)
        {
            if(m_Module[i].m_Type == MRT_WEAPON)
            {
                ASSERT(hull_num >= 0);

                if(!www && g_Config.m_RobotWeaponsConsts[m_Module[i].m_Kind].is_module_repairer) www = true;

                tm = m_Module[hull_num].m_Graph->GetMatrixById(m_Module[i].m_LinkMatrix);
                m_Module[i].m_Matrix = (*tm) * m_Module[hull_num].m_Matrix;
                
                if(m_Module[i].m_Invert)
                {
                    D3DXMATRIX* temp = &m_Module[i].m_Matrix;
                    temp->_11 = -temp->_11;
                    temp->_12 = -temp->_12;
                    temp->_13 = -temp->_13;
                }

                D3DXMatrixInverse(&m_Module[i].m_IMatrix, nullptr, &m_Module[i].m_Matrix);
			}
            else if(m_Module[i].m_Type == MRT_HULL)
            {
                hull_num = i;
                D3DXMatrixIdentity(&m);
                D3DXVECTOR3 th;
                D3DXVec3TransformNormal(&th, &m_HullForward, &m_Core->m_IMatrix);

                m._21 =  th.x;
                m._22 =  th.y;
                m._11 =  th.y;
                m._12 = -th.x;

                if(g_MatrixMap->GetPlayerSide()->GetUnitUnderManualControl() == this && IsWalkingChassis())
                {
                    p = g_MatrixMap->GetPlayerSide()->CorrectWalkingManualControlledRobotHullPos(p, this);
                }

                goto calc;
			}
            else if(m_Module[i].m_Type == MRT_HEAD)
            {
                D3DXMatrixIdentity(&m);
                D3DXVECTOR3 th;
                D3DXVec3TransformNormal(&th, &m_HullForward, &m_Core->m_IMatrix);

                m._21 = th.x;
                m._22 = th.y;
                m._11 = th.y;
                m._12 = -th.x;

                goto calc;
            }
			else
            {
                D3DXMatrixRotationZ(&m, m_Module[i].m_Angle);
calc:
                *(D3DXVECTOR3*)&m._41 = p;
                m_Module[i].m_Matrix = m * m_Core->m_Matrix;
                D3DXMatrixInverse(&m_Module[i].m_IMatrix, nullptr, &m_Module[i].m_Matrix);

                ASSERT(m_Module[i].m_Graph);
                tm = m_Module[i].m_Graph->GetMatrixById(1);

                p.x += tm->_41 * m._11 + tm->_42 * m._21;
                p.y += tm->_41 * m._12 + tm->_42 * m._22;
                p.z += tm->_43;
            }
        }

        //Обновляем координаты отрисовки объектов спрайтовых анимаций для шасси
        int cnt = g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].jet_stream.size();
        if(cnt)
        {
            D3DXVECTOR3 spos;
            const D3DXMATRIX* m;
            D3DXVECTOR3 sdir;
            int chassis_num = m_Module[0].m_Kind;

            for(int i = 0; i < cnt; ++i)
            {
                m = m_Module[0].m_Graph->GetMatrixById(g_Config.m_RobotChassisConsts[chassis_num].jet_stream[i].matrix_id);
                sdir = -*(D3DXVECTOR3*)&m->_21;
                D3DXVec3TransformCoord(&spos, (D3DXVECTOR3*)&m->_41, &m_Module[0].m_Matrix);
                D3DXVec3TransformNormal(&sdir, &sdir, &m_Module[0].m_Matrix);
                m_ChassisData.JetStream[i]->SetPos(spos, spos + sdir * g_Config.m_RobotChassisConsts[chassis_num].jet_stream[i].length);
            }
        }

        cnt = g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].linked_sprite_anim.size();
        if(cnt)
        {
            D3DXVECTOR3 spos;
            const D3DXMATRIX* m;
            D3DXVECTOR3 sdir;
            int chassis_num = m_Module[0].m_Kind;

            /*
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
            */

            for(int i = 0; i < cnt; ++i)
            {
                m = m_Module[0].m_Graph->GetMatrixById(g_Config.m_RobotChassisConsts[chassis_num].linked_sprite_anim[i].matrix_id);
                sdir = -*(D3DXVECTOR3*)&m->_11;//_11;
                D3DXVec3TransformCoord(&spos, (D3DXVECTOR3*)&m->_41, &m_Module[0].m_Matrix);
                D3DXVec3TransformNormal(&sdir, &sdir, &m_Module[0].m_Matrix);
                // - sdir * (g_Config.m_RobotChassisConsts[chassis_num].linked_sprite_anim[i].width / 2.0f)
                m_ChassisData.LinkedSpriteAnim[i]->SetPos(spos, spos + sdir * g_Config.m_RobotChassisConsts[chassis_num].linked_sprite_anim[i].length);
            }
        }

        if(www)
        {
            if(IsInterfaceDraw())
            {
                //if(false)
                for(int i = 0; i < m_ModulesCount; ++i)
                {
                    if(m_Module[i].m_Type == MRT_WEAPON && g_Config.m_RobotWeaponsConsts[m_Module[i].m_Kind].is_module_repairer)
                    {
                        if(m_Module[i].m_WeaponRepairData == nullptr)
                        {
                            m_Module[i].m_WeaponRepairData = SWeaponRepairData::Allocate();
                        }

                        m_Module[i].m_WeaponRepairData->Update(m_Module + i);
                    }
                }

                need &= ~(MR_ShadowStencil);
                m_RChange &= ~(MR_ShadowStencil);
            }
            else
            {
                int cnt = this->AsRobot()->m_WeaponsCount;
                for(int i = 0; i < cnt; ++i)
                {
                    SRobotWeapon* w = this->AsRobot()->m_RobotWeapons + i;
                    w->UpdateRepair();
                }
            }
        }
	}
    
    /*
    if(need & m_RChange & (MR_Sort))
    {
        m_RChange &= ~MR_Sort;

        D3DXVECTOR3 pz = D3DXVECTOR3(0, 0, 0);
        D3DXVec3TransformCoord(&pz, &pz, &(m_Matrix * g_MatrixMap->GetSortMatrix()));
        m_Z = pz.z;
    }
    if(need & m_RChange & (MR_GraphSort))
    {
        m_RChange &= ~MR_GraphSort;

        for(int i = 0; i < m_ModulesCount; ++i)
        {
            ASSERT(m_Module[i].m_Graph);
            m_Module[i].m_Graph->SortIndexForAlpha(m_Matrix * g_MatrixMap->GetViewMatrix());
        }
    }
    */
    
    if(need & m_RChange & MR_ShadowStencil)
    {
        m_RChange &= ~MR_ShadowStencil;

        if(m_ShadowType != SHADOW_STENCIL)
        {
            for(int i = 0; i < m_ModulesCount; ++i)
            {
                if(m_Module[i].m_ShadowStencil)
                {
                    HDelete(CVOShadowStencil, m_Module[i].m_ShadowStencil, g_MatrixHeap);
                    m_Module[i].m_ShadowStencil = nullptr;
                }
            }
        }
        else
        {
			for(int i = 0; i < m_ModulesCount; ++i)
            {
				ASSERT(m_Module[i].m_Graph);
				ASSERT(m_Module[i].m_Graph->VO());

				//if(m_Module[i].m_Graph->VO()->EdgeExist()) {

                if(!m_Module[i].m_ShadowStencil) m_Module[i].m_ShadowStencil = HNew(g_MatrixHeap) CVOShadowStencil(g_MatrixHeap);

                //if(!(m_Module[i].m_Graph->VO()->EdgeExist())) m_Module[i].m_Graph->VO()->EdgeBuild();

                //STENCIL
		        //m_Module[i].m_ShadowStencil->Build(*(m_Module[i].m_Graph->VO()), m_Module[i].m_Graph->FrameVO(), g_MatrixMap->m_LightMain,m_Module[i].m_Matrix,IsNearBase()?g_MatrixMap->m_ShadowPlaneCutBase:g_MatrixMap->m_ShadowPlaneCut);
					
                D3DXVECTOR3 light;
                D3DXVec3TransformNormal(&light, &g_MatrixMap->m_LightMain, &m_Module[i].m_IMatrix);

                //D3DXPLANE cutpl;
                //D3DXPlaneTransform(&cutpl, IsNearBase() ? (&g_MatrixMap->m_ShadowPlaneCutBase) : (&g_MatrixMap->m_ShadowPlaneCut), &m_Module[i].m_IMatrix);

                float len = (GetRadius() * 2) + m_Module[i].m_Matrix._43 - (IsNearBase() ? g_MatrixMap->m_GroundZBase : g_MatrixMap->m_GroundZ);

                m_Module[i].m_ShadowStencil->CreateShadowVO(*(m_Module[i].m_Graph->VO()), m_Module[i].m_Graph->GetVOFrame(), light, len, m_Module[i].m_Invert);

                if(!m_Module[i].m_ShadowStencil->IsReady())
                {
                    HDelete(CVOShadowStencil, m_Module[i].m_ShadowStencil, g_MatrixHeap);
                    m_Module[i].m_ShadowStencil = nullptr;
                }
			}
		}
	}

	if(need & m_RChange & MR_ShadowProjGeom)
    {
        m_RChange &= ~MR_ShadowProjGeom;

		if(m_ShadowType != SHADOW_PROJ_DYNAMIC)
        {
            if(m_ShadowProj)
            {
                HDelete(CVOShadowProj, m_ShadowProj, g_MatrixHeap);
                m_ShadowProj = nullptr;
            }
		}
        else 
        {
            if(!m_ShadowProj) m_ShadowProj = HNew(g_MatrixHeap) CVOShadowProj(g_MatrixHeap);

            byte* buf = (byte*)_malloca(sizeof(CVectorObjectAnim*) * m_ModulesCount + sizeof(int) * m_ModulesCount + sizeof(D3DXMATRIX) * m_ModulesCount);
            if(!buf) ERROR_S(L"Not enough memory for buf after _malloca() call in CMatrixRobot::GetResources function!");
            CVectorObjectAnim** obj = (CVectorObjectAnim**)buf;
            int* noframe = (int*)(buf + sizeof(CVectorObjectAnim*) * m_ModulesCount);
            D3DXMATRIX* wm = (D3DXMATRIX*)(buf + sizeof(CVectorObjectAnim*) * m_ModulesCount + sizeof(int) * m_ModulesCount);

			for(int i = 0; i < m_ModulesCount; ++i)
            {
                ASSERT(m_Module[i].m_Graph);
                ASSERT(m_Module[i].m_Graph->VO());

                obj[i] = m_Module[i].m_Graph;
                noframe[i] = m_Module[i].m_Graph->GetVOFrame();
                wm[i] = m_Module[i].m_Matrix * m_Core->m_IMatrix;
			}
            
            ShadowProjBuildGeomList(*m_ShadowProj, m_ModulesCount, obj, noframe, wm, m_Core->m_Matrix, m_Core->m_IMatrix, g_MatrixMap->m_LightMain, int(100 / GLOBAL_SCALE), false);

            if(!(m_ShadowProj->IsProjected()))
            {
                HDelete(CVOShadowProj, m_ShadowProj, g_MatrixHeap);
                m_ShadowProj = nullptr;
            }

            _freea(buf); //Очистка после вызова _malloca
		}
	}

	if(need & m_RChange & MR_ShadowProjTex)
    {
        m_RChange &= ~MR_ShadowProjTex;

        if(m_ShadowProj != nullptr)
        {
            byte* buf = (byte*)_malloca(sizeof(CVectorObjectAnim*) * m_ModulesCount + sizeof(int) * m_ModulesCount + sizeof(D3DXMATRIX) * m_ModulesCount);
            if(!buf) ERROR_S(L"Not enough memory for buf after _malloca() call in CMatrixRobot::Z_From_Pos function!");
            CVectorObjectAnim** obj = (CVectorObjectAnim**)buf;
            int* noframe = (int*)(buf + sizeof(CVectorObjectAnim*) * m_ModulesCount);
            D3DXMATRIX* wm = (D3DXMATRIX*)(buf + sizeof(CVectorObjectAnim*) * m_ModulesCount + sizeof(int) * m_ModulesCount);

            for(int i = 0; i < m_ModulesCount; ++i)
            {
                ASSERT(m_Module[i].m_Graph);
                ASSERT(m_Module[i].m_Graph->VO());

                obj[i] = m_Module[i].m_Graph;
                noframe[i] = m_Module[i].m_Graph->GetVOFrame();
                wm[i] = m_Module[i].m_Matrix * m_Core->m_IMatrix;
            }

            CTexture* tex = nullptr;

            ShadowProjBuildTextureList(*m_ShadowProj, m_ModulesCount, obj, tex, noframe, wm, m_Core->m_Matrix, m_Core->m_IMatrix, g_MatrixMap->m_LightMain, m_ShadowSize);

            _freea(buf); //Очистка после вызова _malloca
        }
    }
}

void CMatrixRobot::DoAnimation(int cms)
{
    bool frame_changed = false;

    //Крутим фоновые анимации для всех модулей, кроме шасси (например, анимации топлива в баллонах огнемётов)
    for(int i = 1; i < m_ModulesCount; ++i)
    {
        CVectorObjectAnim* model = m_Module[i].m_Graph;
        if(model)
        {
            if(model->VectorObjectAnimTact(cms)) frame_changed = true;

            //Если фоновая анимация завершилась, тут же запускаем её по новой
            if(model->IsAnimEnd()) //Эта проверка сработает только в случае, если анимации не был выставлен LOOP
            {
                //model->SetAnimByName(ANIMATION_NAME_IDLE); //Неоптимально каждый раз передёргивать это дерьмо
                //model->SetFirstFrame(); //А так, к сожалению, он не сбрасывает анимации на дефолт после стрельбы

                if(!m_Module[i].m_ReadyToExplode) model->SetAnimByIndex(0); //Перезапускаем стандартную анимацию (предполагается, что анимация Idle всегда будет первая в списке анимаций модели)
                else AsMatrixRobotAI()->SetBombStatus(BS_TIME_TO_EXPLODE); //Если данный модуль - взведённая бомба, то передаём "эстафету" в LogicTact()
            }
        }
    }

    CVectorObjectAnim* robot_chassis_model = m_Module[0].m_Graph;
    if(!robot_chassis_model) goto end_anim;

    //При таком маркере на шасси код будет постоянно проигрывать все анимации шасси как фоновые
    if(m_ChassisData.m_MovingAnimType == NO_MOVING_ANIM)
    {
        if(robot_chassis_model->VectorObjectAnimTact(cms)) frame_changed = true;

        //Если фоновая анимация завершилась, тут же запускаем её по новой
        if(robot_chassis_model->IsAnimEnd()) robot_chassis_model->SetAnimByName(ANIMATION_NAME_IDLE);

        goto end_anim;
    }

    //Ниже воспроизводим только анимации шасси
    //Тут играются анимации, которым не нужно учитывать текущую скорость движения робота
    if(m_ChassisData.m_MovingAnimType == NORMAL_MOVING_ANIM)
    {
        if(robot_chassis_model->VectorObjectAnimTact(cms)) frame_changed = true;
    }
    //Тут играются анимации, привязанные к скорости движения (колёса, траки, шагающие и т.д.)
    else //if(m_ChassisData.m_MovingAnimType == SPEED_DEPENDENT_MOVING_ANIM)
    {
        //Для данных типов анимаций учитывать скорость смысла нет
        if(m_Animation == ANIMATION_STAY || m_Animation == ANIMATION_END_MOVE || m_Animation == ANIMATION_END_MOVE_BACK)
        {
            if(robot_chassis_model->VectorObjectAnimTact(cms)) frame_changed = true;
        }
        else if(m_Animation == ANIMATION_ROTATE_LEFT || m_Animation == ANIMATION_ROTATE_RIGHT)
        {
            float speed_factor = 0.01f / m_MaxRotationSpeed; //Базовой скоростью вращения шасси считаем 0.01, и корректируем анимацию разворота, опираясь на это значение
            if(robot_chassis_model->VectorObjectAnimTact(cms, speed_factor)) frame_changed = true;
        }
        else
        {
            float speed_factor = min(m_ChassisData.m_MovingAnimSpeed / m_Speed, 3.0f);
            if(robot_chassis_model->VectorObjectAnimTact(cms, speed_factor)) frame_changed = true;
        }
    }

end_anim:;

    //Если сменился фрейм анимации, обновляем тени под моделью
    if(frame_changed)
    {
        if(m_ShadowType == SHADOW_STENCIL) RChange(MR_ShadowStencil);
        else if(m_ShadowType == SHADOW_PROJ_DYNAMIC) RChange(MR_ShadowProjTex);
    }
}

void SMatrixRobotModule::PrepareForDIP()
{
    if(m_ShadowStencil)
    {
        HDelete(CVOShadowStencil, m_ShadowStencil, g_MatrixHeap);
        m_ShadowStencil = nullptr;
    }

    if(m_WeaponRepairData)
    {
        m_WeaponRepairData->Release();
        m_WeaponRepairData = nullptr;
    }

    Smoke().effect = nullptr;
}

void CMatrixRobot::ApplyNaklon(const D3DXVECTOR3& dir)
{
    D3DXVECTOR3 temp = *(D3DXVECTOR3*)&m_Core->m_Matrix._31 + dir;
    D3DXVec3Normalize((D3DXVECTOR3*)&m_Core->m_Matrix._31, &temp);
}

void CMatrixRobot::Tact(int cms)
{
    if(m_CurrentState != ROBOT_CARRYING && m_CurrentState != ROBOT_FALLING && m_CurrentState != ROBOT_IN_SPAWN && m_CurrentState != ROBOT_CAPTURING_BASE)
    {
        //Если робот на воде, он будет оставлять за собой волнение
        if(FLAG(m_ObjectFlags, ROBOT_FLAG_ON_WATER))
        {
            m_KeelWaterCount += m_Speed * KEELWATER_SPAWN_FACTOR * cms;
        }
        //Шасси будет оставлять под собой пылевой след
        else if(g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].is_dust_trace)
        {
            m_ChassisData.m_DustCount += (m_Speed + 1) * DUST_SPAWN_FACTOR * cms;
        }

        while(m_KeelWaterCount > 1.0)
        {
            CMatrixEffect::CreateBillboard(nullptr, D3DXVECTOR3(m_PosX, m_PosY, WATER_LEVEL), 5, 40, 0xFFFFFFFF, 0x00FFFFFF, 3000, 0, TEXTURE_PATH_KEELWATER, m_ChassisForward);
            m_KeelWaterCount -= 1.0f;
        }

        if(g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].is_dust_trace)
        {
            while(m_ChassisData.m_DustCount > 1.0)
            {
                //CDText::T("spd", CStr(m_Speed));
                D3DXVECTOR2 spd(m_Velocity.x, m_Velocity.y);
                spd *= float(cms) / LOGIC_TACT_PERIOD;
                CMatrixEffect::CreateDust(nullptr, *(D3DXVECTOR2*)&GetGeoCenter(), spd, 500);
                m_ChassisData.m_DustCount -= 1.0f;
            }
        }
    }

    //Обрабатываем логику спрайтовых анимаций для шасси робота
    int cnt = g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].linked_sprite_anim.size();
    if(cnt)
    {
        for(int i = 0; i < cnt; ++i)
        {
            m_ChassisData.LinkedSpriteAnim[i]->Tact((float)cms);
        }
    }

    if(!IsWalkingChassis()) DoAnimation(cms); //Анимация движения шагающего шасси вызывается вместе с StepLinkWalkingChassis

	//RChange(MR_Matrix);
}

bool CMatrixRobot::PickFull(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt) const
{
    if(m_CurrentState == ROBOT_DIP) return false;
	for(int i = 0; i < m_ModulesCount; ++i)
    {
		if(m_Module[i].m_Graph)
        {
            if(m_Module[i].m_Graph->Pick(m_Module[i].m_Matrix, m_Module[i].m_IMatrix, orig, dir, outt)) return true;
		}
	}
	return false;
}

bool CMatrixRobot::Pick(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt) const
{
    if(m_CurrentState == ROBOT_DIP) return false;
	for(int i = 0; i < m_ModulesCount; ++i)
    {
		if(m_Module[i].m_Graph)
        {
            if(m_Module[i].m_Graph->Pick(m_Module[i].m_Matrix, m_Module[i].m_IMatrix, orig, dir, outt)) return true;
		}
	}
	return false;
}

void CMatrixRobot::FreeDynamicResources()
{
    if(m_ShadowProj && (m_ShadowType == SHADOW_PROJ_DYNAMIC))
    {

        HDelete(CVOShadowProj, m_ShadowProj, g_MatrixHeap);
        m_ShadowProj = nullptr;
        RChange(MR_ShadowProjGeom);
    }
    else if(m_ShadowType == SHADOW_STENCIL)
    {
        for(int i = 0; i < m_ModulesCount; ++i)
        {
            if(m_Module[i].m_ShadowStencil) m_Module[i].m_ShadowStencil->DX_Free();
        }
    }
}

void CMatrixRobot::BeforeDraw()
{
    if(IsMustDie()) return;

    //GetResources(MR_Matrix | MR_Graph | MR_GraphSort | MR_ShadowStencil | MR_ShadowProj);
    dword sh = (g_Config.m_ShowProjShadows ? (MR_ShadowProjGeom | MR_ShadowProjTex) : 0) | (g_Config.m_ShowStencilShadows ? MR_ShadowStencil : 0);

    //Не работает отладка, сука. Нет связи между CMatrixRobot и CMatrixRobotAI
    /*
    if(this->AsRobot()->GetOrder(0)->GetOrderPhase() < ROP_CAPTURE_SETTING_UP || !this->AsRobot()->GetOrder(0)->GetStatic()->AsBuilding()->IsBase())
    {
        CPoint tp = CPoint(this->AsRobot()->GetMapPosX(), this->AsRobot()->GetMapPosY());
        if(tp.x >= 0)
        {
            D3DXVECTOR3 v;
            v.x = GLOBAL_SCALE_MOVE * tp.x + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
            v.y = GLOBAL_SCALE_MOVE * tp.y + GLOBAL_SCALE_MOVE * ROBOT_MOVECELLS_PER_SIZE / 2;
            v.z = g_MatrixMap->GetZ(v.x, v.y);
            CMatrixEffect::CreateMoveToAnim(v, 2);
        }
    }
    */

    GetResources(MR_Matrix | MR_Graph | sh);

    if(m_ShowHitpointsTime > 0 && m_Hitpoints > 0 && m_CurrentState != ROBOT_DIP)
    {
        D3DXVECTOR3 pos(*(D3DXVECTOR3*)&m_Core->m_Matrix._41);
        pos.z += 20;

        if(TRACE_STOP_NONE == g_MatrixMap->Trace(nullptr, g_MatrixMap->m_Camera.GetFrustumCenter(), pos, TRACE_LANDSCAPE, nullptr))
        {
            D3DXVECTOR2 p = g_MatrixMap->m_Camera.Project(pos, g_MatrixMap->GetIdentityMatrix());
            m_HealthBar.Modify(p.x - (PB_ROBOT_WIDTH * 0.5f), p.y - GetRadius(), m_Hitpoints * m_MaxHitpointsInversed);
        }
    }

    if(g_Config.m_ShowStencilShadows && !IsInterfaceDraw())
    {
	    for(int i = 0; i < m_ModulesCount; ++i)
        {
            m_Module[i].m_Graph->BeforeDraw();
            if(m_CurrentState != ROBOT_DIP && m_Module[i].m_ShadowStencil) m_Module[i].m_ShadowStencil->BeforeRender();
	    }
    }

    if(m_CurrentState != ROBOT_DIP && m_ShadowProj && g_Config.m_ShowProjShadows) m_ShadowProj->BeforeRender();
}

void CMatrixRobot::Draw()
{
    //Маркер IsInterfaceDraw() используется для отрисовки картинок превью роботов и прочей фигни, не связанной с отрисовкой роботов реал-тайм

    dword col_tex = (dword)g_MatrixMap->GetSideColorTexture(m_Side)->Tex();
	//g_D3DD->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

    for(int i = 0; i <= 3; ++i)
    {
        ASSERT_DX(g_D3DD->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD) (&g_MatrixMap->m_BiasRobots))));
    }

    if(m_CurrentState == ROBOT_DIP)
    {
        for(int i = 0; i < m_ModulesCount; ++i)
        {
            SMatrixRobotModule* cur_module = &m_Module[i];

            if(cur_module->m_TTL <= 0) continue;
    	    g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF808080);
		    ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD, &(cur_module->m_Matrix)));
		    if(cur_module->m_Invert)
            {
                g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
                cur_module->m_Graph->Draw(col_tex);
		        g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
            }
            else cur_module->m_Graph->Draw(col_tex);
	    }
    }
    else
    {
	    for(int i = 0; i < m_ModulesCount; ++i)
        {
            SMatrixRobotModule* cur_module = &m_Module[i];

		    ASSERT(model);
	        g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR, m_Core->m_TerrainColor);

		    ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD, &(cur_module->m_Matrix)));
		    if(cur_module->m_Invert)
            {
                g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
                cur_module->m_Graph->Draw(col_tex);
		        g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
            }
            else cur_module->m_Graph->Draw(col_tex);
	    }

        //Вызов DrawNow для световых спрайтов ПОСЛЕ рендера модели, создаёт наложение одного поверх другого,
        //однако рендерить эти световые спрайты последовательно с прочими модулям нельзя т.к. это ломает рендер
	    for(int i = 0; i < m_ModulesCount; ++i)
        {
            SMatrixRobotModule* cur_module = &m_Module[i];

            if(!IsInterfaceDraw()) cur_module->m_Graph->DrawLightSprites(false, cur_module->m_Matrix, nullptr);
            else cur_module->m_Graph->DrawLightSprites(true, cur_module->m_Matrix, &g_MatrixMap->m_Camera.GetDrawNowIView());
        }
    }

    if(m_CurrentState == ROBOT_DIP) return;

    if(m_CamDistSq > MAX_EFFECT_DISTANCE_SQ) return;

    if(!FLAG(g_MatrixMap->m_Flags, MMFLAG_OBJECTS_DRAWN))
    {
        if(!IsInterfaceDraw())
        {
            int cnt = this->AsRobot()->m_WeaponsCount;
            for(int i = 0; i < cnt; ++i)
            {
                SRobotWeapon* w = this->AsRobot()->m_RobotWeapons + i;
                w->Draw(this->AsRobot());
            }
        }
        else
        {
            for(int i = 0; i < m_ModulesCount; ++i)
            {
                if(m_Module[i].m_Type == MRT_WEAPON && g_Config.m_RobotWeaponsConsts[m_Module[i].m_Kind].is_module_repairer)
                {
                    if(m_Module[i].m_WeaponRepairData)
                    {
                        // draw
                        m_Module[i].m_WeaponRepairData->Draw(true);
                    }
                }
            }
        }

        //Рисуем реактивные струи для шасси робота в интерфейсе и непосредственно в игре
        int cnt = g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].jet_stream.size();
        if(cnt)
        {
            if(!IsInterfaceDraw())
            {
                for(int i = 0; i < cnt; ++i)
                {
                    m_ChassisData.JetStream[i]->Draw();
                }
            }
            else
            {
                CVectorObject::DrawEnd();

                for(int i = 0; i < cnt; ++i)
                {
                    m_ChassisData.JetStream[i]->Draw(true);
                }
            }
        }

        //Рисуем прочие спрайтовые анимации для шасси робота в интерфейсе и непосредственно в игре
        cnt = g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].linked_sprite_anim.size();
        if(cnt)
        {
            if(!IsInterfaceDraw())
            {
                for(int i = 0; i < cnt; ++i)
                {
                    m_ChassisData.LinkedSpriteAnim[i]->Draw();
                }
            }
            else
            {
                CVectorObject::DrawEnd();

                for(int i = 0; i < cnt; ++i)
                {
                    m_ChassisData.LinkedSpriteAnim[i]->Draw(true);
                }
            }
        }
    }
}

void CMatrixRobot::DrawShadowStencil()
{
    if(m_ShadowType != SHADOW_STENCIL) return;

    if(this->AsRobot() == g_MatrixMap->GetPlayerSide()->GetUnitUnderManualControl())
    {
        if(!g_MatrixMap->m_Camera.IsInFrustum(*(D3DXVECTOR3*)&m_Module[1].m_Matrix._41)) return;
    }

    if(m_CurrentState == ROBOT_CARRYING)
    {
        if(m_PosX < 0 || m_PosY < 0 || m_PosX > (GLOBAL_SCALE * g_MatrixMap->m_Size.x) || m_PosY > (GLOBAL_SCALE * g_MatrixMap->m_Size.y)) return;
    }

    for(int i = 0; i < m_ModulesCount; ++i)
    {
		if(m_Module[i].m_ShadowStencil)
        {
            m_Module[i].m_ShadowStencil->Render(m_Module[i].m_Matrix);
		}
	}
}

void CMatrixRobot::DrawShadowProj()
{
	if(!m_ShadowProj) return;

    D3DXMATRIX m = g_MatrixMap->GetIdentityMatrix();
    m._41 = m_ShadowProj->GetDX();
    m._42 = m_ShadowProj->GetDY();
	ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD,&m));

    m_ShadowProj->Render();
}

bool CMatrixRobot::CalcBounds(D3DXVECTOR3& minv, D3DXVECTOR3& maxv)
{
    if(!m_ModulesCount) return true;

    int delta = abs(m_CalcBoundsLastTime - g_MatrixMap->GetTime());
    if(delta < 100)
    {
        minv = m_CalcBoundMin;
        maxv = m_CalcBoundMax;
        return false;
    }

	GetResources(MR_Matrix | MR_Graph);

	D3DXVECTOR3 bminout, bmaxout;

    minv.x = 1e30f; minv.y = 1e30f; minv.z = 1e30f;
    maxv.x = -1e30f; maxv.y = -1e30f; maxv.z = -1e30f;

    for(int u = 0; u < m_ModulesCount; ++u)
    {
		int cnt = 1;//m_Module[u].m_Graph->VO()->GetFramesCnt();
		for(int i = 0; i < cnt; ++i)
        {
            m_Module[u].m_Graph->VO()->GetBound(i, m_Module[u].m_Matrix, bminout, bmaxout);

            minv.x = min(minv.x, bminout.x);
            minv.y = min(minv.y, bminout.y);
            minv.z = min(minv.z, bminout.z);
            maxv.x = max(maxv.x, bmaxout.x);
            maxv.y = max(maxv.y, bmaxout.y);
            maxv.z = max(maxv.z, bmaxout.z);
		}
	}

    m_CalcBoundMin = minv;
    m_CalcBoundMax = maxv;
    m_CalcBoundsLastTime = g_MatrixMap->GetTime();

    return false;

    /*
	GetResources(MR_Graph | MR_Matrix);

    SVOFrame* f = m_Module[0].m_Graph->VO()->FrameGet(0);
    minv.x = f->m_MinX;
    minv.y = f->m_MinY;
    maxv.x = f->m_MaxX;
    maxv.y = f->m_MaxY;

    for(int u = 0; u < m_ModulesCount; ++u)
    {
        int cnt = m_Module[u].m_Graph->VO()->Header()->m_FrameCnt;
        for(int i = 1; i < cnt; ++i)
        {
            SVOFrame* f = m_Module[u].m_Graph->VO()->FrameGet(i);

            minv.x = min(minv.x, f->m_MinX);
            minv.y = min(minv.y, f->m_MinY);
            maxv.x = max(maxv.x, f->m_MaxX);
            maxv.y = max(maxv.y, f->m_MaxY);
        }
    }

    return false;
    */
}

bool CMatrixRobot::Carry(CMatrixFlyer* cargo, bool quick_connect)
{
    /*
    if(side_index < 0)
    {
        if(!g_MatrixMap->PlaceIsEmpty(m_Module[0].m_Kind, 4, int(m_PosX / GLOBAL_SCALE), int(m_PosY / GLOBAL_SCALE)))
        {
            return false;
        }
    }
    */

    if(m_CurrentState == ROBOT_CARRYING)
    {
        CMatrixRobot* r = m_CargoFlyer->GetCarryingRobot();
        if(r != this)
        {
            if(!r->Carry(nullptr)) return false;
        }
        m_CargoFlyer->GetCarryData()->m_Robot = nullptr;
        //m_CargoFlyer->SetAlt(FLYER_ALT_EMPTY);
        if(m_CargoFlyer->GetCarryData()->m_RobotElevatorField != nullptr)
        {
            g_MatrixMap->SubEffect(m_CargoFlyer->GetCarryData()->m_RobotElevatorField);
            m_CargoFlyer->GetCarryData()->m_RobotElevatorField = nullptr;
        }

    }
    if(cargo == nullptr)
    {
        //if (m_CurrentState == ROBOT_MUST_DIE) return true;
        m_CurrentState = ROBOT_FALLING;
        m_Velocity = D3DXVECTOR3(0, 0, 0);
        m_FallingSpeed = 0;
        JoinToGroup();
        return true;
    }
    if(cargo->IsCarryingRobot())
    {
        cargo->GetCarryingRobot()->Carry(nullptr);
    }

    m_CargoFlyer = cargo;
    m_CurrentState = ROBOT_CARRYING;
    SwitchAnimation(ANIMATION_OFF);

    cargo->GetCarryData()->m_Robot = this;
    cargo->GetCarryData()->m_RobotForward = m_ChassisForward;
    D3DXVec3Normalize(&cargo->GetCarryData()->m_RobotUp, (D3DXVECTOR3*)&m_Core->m_Matrix._31);
    cargo->GetCarryData()->m_RobotUpBack = D3DXVECTOR3(0, 0, 0);

    *((D3DXVECTOR2*)&cargo->GetCarryData()->m_RobotForward) = RotatePoint(*(D3DXVECTOR2*)&m_ChassisForward, -cargo->GetDirectionAngle());
    cargo->GetCarryData()->m_RobotForward.z = m_ChassisForward.z;

    cargo->GetCarryData()->m_RobotAngle = cargo->GetDirectionAngle();
    //cargo->SetAlt(FLYER_ALT_CARRYING);
    cargo->GetCarryData()->m_RobotElevatorField = nullptr;
    //cargo->GetCarryData()->m_RobotQuickConnect = quick_connect;
    cargo->GetCarryData()->m_RobotMassFactor = quick_connect ? 1.0f : 0;

    UnjoinGroup();
    ClearSelection();
    return true;
}

void CMatrixRobot::ClearSelection()
{
    if(m_Side == PLAYER_SIDE && g_MatrixMap->GetPlayerSide()->m_ActiveObject == this) g_MatrixMap->GetPlayerSide()->Select(NOTHING);
}

//Получает общую команду на выставление той или иной анимации для шасси робота,
//после чего сама определяет (и выставляет), в какой последовательности нужно запустить имеющиеся в модели анимации для создания эффекта непрерывного движения
void CMatrixRobot::SwitchAnimation(EAnimation target_anim)
{
    //if(m_Animation == target_anim) return; //Не думаю, что это хорошая мысль
    if(m_ChassisData.m_MovingAnimType == NO_MOVING_ANIM) return; //При таком маркере код будет постоянно проигрывать анимации на шасси как фоновые
    if((target_anim == ANIMATION_ROTATE_LEFT || target_anim == ANIMATION_ROTATE_RIGHT) && !FLAG(m_ObjectFlags, ROBOT_FLAG_ROTATING)) return; //Анимация разворотов вторична по отношению к анимациям движения
    CVectorObjectAnim* robot_chassis_graph = m_Module[0].m_Graph;

    if(target_anim != m_Animation)
    {
        RChange(MR_Matrix | MR_ShadowStencil | MR_ShadowProjTex);
    }

    if(target_anim != ANIMATION_OFF)
    {
        if(m_CurrentState == ROBOT_DIP || m_CurrentState == ROBOT_CARRYING || IsShorted()) return;
        if(target_anim != m_Animation)
        {
            for(int i = 0; i < m_ModulesCount; ++i)
            {
                m_Module[i].m_NextAnimTime = float(g_MatrixMap->GetTime());
            }
        }
    }

    //Необходимо переключить анимацию шасси робота на движение вперёд
    switch(target_anim)
    {
        //Необходимо переключить анимацию шасси робота на стояние на месте (завершаем анимации любого движения и переключаемся на остановочную)
        case ANIMATION_STAY:
        {
        SetAnimStay:;

            RESETFLAG(m_ObjectFlags, ROBOT_FLAG_MOVING_BACK);

            if(m_Animation == ANIMATION_MOVE)
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_END_MOVE))
                {
                    m_Animation = ANIMATION_END_MOVE;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_END_MOVE, ANIM_LOOP_OFF);
                }
                else
                {
                    //Сперва пробуем реверсировать анимацию начала движения вперёд
                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_MOVE))
                    {
                        m_Animation = ANIMATION_BEGIN_MOVE;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_MOVE, ANIM_LOOP_OFF, ANIM_REVERSED);
                    }
                    else
                    {
                        m_Animation = ANIMATION_STAY;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_STAY, ANIM_LOOP_ON);
                    }
                }
            }
            else if(m_Animation == ANIMATION_MOVE_BACK)
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_END_MOVE_BACK))
                {
                    m_Animation = ANIMATION_END_MOVE_BACK;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_END_MOVE_BACK, ANIM_LOOP_OFF);
                }
                else
                {
                    //Сперва пробуем реверсировать анимацию начала движения назад
                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_MOVE_BACK))
                    {
                        m_Animation = ANIMATION_BEGIN_MOVE_BACK;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_MOVE_BACK, ANIM_LOOP_OFF, ANIM_REVERSED);
                    }
                    else
                    {
                        m_Animation = ANIMATION_STAY;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_STAY, ANIM_LOOP_ON);
                    }
                }
            }
            else if(m_Animation == ANIMATION_ROTATE_LEFT)
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_END_ROTATE_LEFT))
                {
                    m_Animation = ANIMATION_END_ROTATE_LEFT;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_END_ROTATE_LEFT, ANIM_LOOP_OFF);
                }
                else
                {
                    //Сперва пробуем реверсировать анимацию начала движения вперёд
                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_ROTATE_LEFT))
                    {
                        m_Animation = ANIMATION_BEGIN_ROTATE_LEFT;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_ROTATE_LEFT, ANIM_LOOP_OFF, ANIM_REVERSED);
                    }
                    else
                    {
                        m_Animation = ANIMATION_STAY;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_STAY, ANIM_LOOP_ON);
                    }
                }
            }
            else if(m_Animation == ANIMATION_ROTATE_RIGHT)
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_END_ROTATE_RIGHT))
                {
                    m_Animation = ANIMATION_END_ROTATE_RIGHT;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_END_ROTATE_RIGHT, ANIM_LOOP_OFF);
                }
                else
                {
                    //Сперва пробуем реверсировать анимацию начала движения назад
                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_ROTATE_RIGHT))
                    {
                        m_Animation = ANIMATION_BEGIN_ROTATE_RIGHT;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_ROTATE_RIGHT, ANIM_LOOP_OFF, ANIM_REVERSED);
                    }
                    else
                    {
                        m_Animation = ANIMATION_STAY;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_STAY, ANIM_LOOP_ON);
                    }
                }
            }
            else if( m_Animation == ANIMATION_BEGIN_MOVE        || m_Animation == ANIMATION_BEGIN_MOVE_BACK ||
                     m_Animation == ANIMATION_BEGIN_ROTATE_LEFT || m_Animation == ANIMATION_BEGIN_ROTATE_RIGHT )
            {
                //Реверсируем анимацию, пока она не вернёт модель в положение остановки
                if(!robot_chassis_graph->IsAnimEnd(AT_FIRST_FRAME)) robot_chassis_graph->SetAnimReversed(true);
                else
                {
                    m_Animation = ANIMATION_STAY;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_STAY, ANIM_LOOP_ON);
                }
            }
            else if( m_Animation == ANIMATION_END_MOVE        || m_Animation == ANIMATION_END_MOVE_BACK ||
                     m_Animation == ANIMATION_END_ROTATE_LEFT || m_Animation == ANIMATION_END_ROTATE_RIGHT )
            {
                if(robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME))
                {
                    m_Animation = ANIMATION_STAY;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_STAY, ANIM_LOOP_ON);
                }
            }
            else if(m_Animation == ANIMATION_OFF)
            {
                robot_chassis_graph->SetAnimByName(ANIMATION_NAME_STAY, ANIM_LOOP_ON);
                m_Animation = ANIMATION_STAY;
            }

            break;
        }

        case ANIMATION_MOVE:
        {
            RESETFLAG(m_ObjectFlags, ROBOT_FLAG_MOVING_BACK);

            //Стандартная ситуация: робот стоит на месте, либо крутиться, а анимации движения вперёд-назад можно включать без ограничений
            if( m_Animation == ANIMATION_STAY ||
                m_Animation == ANIMATION_OFF )
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_MOVE))
                {
                    m_Animation = ANIMATION_BEGIN_MOVE;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_MOVE, ANIM_LOOP_OFF);
                }
                else //Если такой анимации в списке модели не нашлось, то сразу переключаем на ANIMATION_MOVE
                {
                    m_Animation = ANIMATION_MOVE;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE, ANIM_LOOP_ON);

                    if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                }
            }
            else if( m_Animation >= ANIMATION_BEGIN_ROTATE_LEFT && m_Animation <= ANIMATION_END_ROTATE_RIGHT ||
                     m_Animation == ANIMATION_BEGIN_MOVE_BACK   || m_Animation == ANIMATION_END_MOVE_BACK )
            {
                goto SetAnimStay;
            }
            else if(m_Animation == ANIMATION_MOVE_BACK)
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_END_MOVE_BACK))
                {
                    m_Animation = ANIMATION_END_MOVE_BACK;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_END_MOVE_BACK, ANIM_LOOP_OFF);
                }
                else //Если такой анимации в списке модели не нашлось, то сразу переключаем на ANIMATION_BEGIN_MOVE
                {
                    //Сперва пробуем реверсировать анимацию начала движения назад BEGIN_MOVE_BACK
                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_MOVE_BACK))
                    {
                        m_Animation = ANIMATION_BEGIN_MOVE_BACK;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_MOVE_BACK, ANIM_LOOP_OFF, ANIM_REVERSED);
                    }
                    else
                    {
                        if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_MOVE))
                        {
                            m_Animation = ANIMATION_BEGIN_MOVE;
                            robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_MOVE, ANIM_LOOP_OFF);
                        }
                        else //Если такой анимации в списке модели не нашлось, то сразу переключаем на ANIMATION_MOVE
                        {
                            m_Animation = ANIMATION_MOVE;

                            //Если в модели отсутствует анимация MOVE_BACK, значит, переключение анимаций движения в ней осуществляется посредством инверсии,
                            //и нам нет смысла сбрасывать текущий кадр в самое начало
                            if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_MOVE_BACK))
                            {
                                robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE, ANIM_LOOP_ON);
                            }
                            else
                            {
                                robot_chassis_graph->SetAnimReversed(false);
                                if(robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME)) robot_chassis_graph->SetFirstFrame(); //На всякий случай сразу же перезапускаем остановившуюся в конце секвенции анимацию
                            }

                            if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                        }
                    }
                }
            }
            //Нестандартная ситуация: робот уже движется, либо начал движение вперёд или назад
            else if(m_Animation == ANIMATION_END_MOVE)
            {
                if(robot_chassis_graph->GetAnimName() == ANIMATION_NAME_END_MOVE)
                {
                    //Реверсируем анимацию END_MOVE, пока она не вернёт модель в положение движения назад
                    if(!robot_chassis_graph->IsAnimEnd(AT_FIRST_FRAME)) robot_chassis_graph->SetAnimReversed(true);
                    else //И только после этого запускаем анимацию движения назад
                    {
                        m_Animation = ANIMATION_MOVE;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE, ANIM_LOOP_ON);

                        if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                    }
                }
                else
                {
                    //Реверсируем анимацию BEGIN_MOVE (заменяющую END_MOVE), пока она не вернёт модель в положение движения назад
                    if(!robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME)) robot_chassis_graph->SetAnimReversed(false);
                    else //И только после этого запускаем анимацию движения назад
                    {
                        m_Animation = ANIMATION_MOVE;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE, ANIM_LOOP_ON);

                        if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                    }
                }
            }
            //Предполагается, что если уже включена ANIMATION_BEGIN_MOVE, то до она никогда не должна быть реверсирована
            else if(m_Animation == ANIMATION_BEGIN_MOVE)
            {
                robot_chassis_graph->SetAnimReversed(false);

                if(robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME))
                {
                    m_Animation = ANIMATION_MOVE;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE, ANIM_LOOP_ON);

                    if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                }
            }

            break;
        }

        //Необходимо переключить анимацию шасси робота на движение назад
        case ANIMATION_MOVE_BACK:
        {
            //Стандартная ситуация: робот стоит на месте, либо крутиться, а анимации движения вперёд-назад можно включать без ограничений
            if( m_Animation == ANIMATION_STAY ||
                m_Animation == ANIMATION_OFF )
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_MOVE_BACK))
                {
                    m_Animation = ANIMATION_BEGIN_MOVE_BACK;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_MOVE_BACK, ANIM_LOOP_OFF);
                }
                else //Если такой анимации в списке модели не нашлось, то сразу переключаем на ANIMATION_MOVE_BACK
                {
                    SETFLAG(m_ObjectFlags, ROBOT_FLAG_MOVING_BACK);
                    m_Animation = ANIMATION_MOVE_BACK;

                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_MOVE_BACK)) robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE_BACK, ANIM_LOOP_ON);
                    //Если анимации движения назад в списке модели также не нашлось (например, для колёс или траков), то просто инвертируем анимацию MOVE
                    else robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE, ANIM_LOOP_ON, ANIM_REVERSED);

                    if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                }
            }
            else if( m_Animation >= ANIMATION_BEGIN_ROTATE_LEFT && m_Animation <= ANIMATION_END_ROTATE_RIGHT ||
                     m_Animation == ANIMATION_BEGIN_MOVE        || m_Animation == ANIMATION_END_MOVE )
            {
                goto SetAnimStay;
            }
            //Здесь перезапускаем реверсированную анимацию MOVE_BACK, если в её последовательности закончились кадры
            else if(m_Animation == ANIMATION_MOVE_BACK)
            {
                if(robot_chassis_graph->IsAnimReversed())
                {
                    if(robot_chassis_graph->IsAnimEnd(AT_FIRST_FRAME)) robot_chassis_graph->SetLastFrame();
                }
            }
            else if (m_Animation == ANIMATION_MOVE)
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_END_MOVE))
                {
                    m_Animation = ANIMATION_END_MOVE;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_END_MOVE, ANIM_LOOP_OFF);
                }
                else //Если такой анимации в списке модели не нашлось, то сразу переключаем на ANIMATION_BEGIN_MOVE_BACK
                {
                    //Сперва пробуем реверсировать анимацию начала движения BEGIN_MOVE
                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_MOVE))
                    {
                        m_Animation = ANIMATION_BEGIN_MOVE;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_MOVE, ANIM_LOOP_OFF, ANIM_REVERSED);
                    }
                    else
                    {
                        if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_MOVE_BACK))
                        {
                            m_Animation = ANIMATION_BEGIN_MOVE_BACK;
                            robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_MOVE_BACK, ANIM_LOOP_OFF);
                        }
                        else //Если такой анимации в списке модели не нашлось (вернулось true), то сразу переключаем на ANIMATION_MOVE_BACK
                        {
                            SETFLAG(m_ObjectFlags, ROBOT_FLAG_MOVING_BACK);
                            m_Animation = ANIMATION_MOVE_BACK;

                            if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_MOVE_BACK)) robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE_BACK);
                            //Если анимации движения назад в списке модели также не нашлось (например, для колёс или траков), то просто инвертируем анимацию MOVE
                            //А поскольку прямо сейчас на модели и так выставлена анимация MOVE, сбрасывать текущий кадр в ней нам не имеет смысла
                            else
                            {
                                robot_chassis_graph->SetAnimReversed(true);
                                if(robot_chassis_graph->IsAnimEnd(AT_FIRST_FRAME)) robot_chassis_graph->SetLastFrame(); //На всякий случай сразу же перезапускаем остановившуюся в конце секвенции анимацию
                            }

                            if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                        }
                    }
                }
            }
            //Нестандартная ситуация: робот уже движется, либо начал движение вперёд или назад
            else if(m_Animation == ANIMATION_END_MOVE_BACK)
            {
                if(robot_chassis_graph->GetAnimName() == ANIMATION_NAME_END_MOVE_BACK)
                {
                    //Реверсируем анимацию END_MOVE_BACK, пока она не вернёт модель в положение движения назад
                    if(!robot_chassis_graph->IsAnimEnd(AT_FIRST_FRAME)) robot_chassis_graph->SetAnimReversed(true);
                    //И только после этого запускаем анимацию движения назад
                    else
                    {
                        m_Animation = ANIMATION_MOVE_BACK;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE_BACK, ANIM_LOOP_ON);
                        SETFLAG(m_ObjectFlags, ROBOT_FLAG_MOVING_BACK);

                        if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                    }
                }
                else
                {
                    //Реверсируем анимацию BEGIN_MOVE_BACK (заменяющую END_MOVE_BACK), пока она не вернёт модель в положение движения назад
                    if(!robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME)) robot_chassis_graph->SetAnimReversed(false);
                    //И только после этого запускаем анимацию движения назад
                    else
                    {
                        m_Animation = ANIMATION_MOVE_BACK;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE_BACK, ANIM_LOOP_ON);
                        SETFLAG(m_ObjectFlags, ROBOT_FLAG_MOVING_BACK);

                        if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                    }
                }
            }
            //Предполагается, что если уже включена ANIMATION_BEGIN_MOVE_BACK, то она никогда не должна быть реверсирована
            else if(m_Animation == ANIMATION_BEGIN_MOVE_BACK)
            {
                robot_chassis_graph->SetAnimReversed(false);

                if(robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME))
                {
                    SETFLAG(m_ObjectFlags, ROBOT_FLAG_MOVING_BACK);
                    m_Animation = ANIMATION_MOVE_BACK;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_MOVE_BACK, ANIM_LOOP_ON);
                    if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                }
            }

            break;
        }

        //Необходимо переключить анимацию шасси робота на анимацию вращения на месте:
        //Необходимо переключить анимацию шасси робота на разворот налево
        case ANIMATION_ROTATE_LEFT:
        {
            if( m_Animation == ANIMATION_STAY ||
                m_Animation == ANIMATION_OFF )
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_ROTATE_LEFT))
                {
                    m_Animation = ANIMATION_BEGIN_ROTATE_LEFT;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_ROTATE_LEFT, ANIM_LOOP_OFF);
                }
                else //Если такой анимации в списке модели не нашлось, то сразу переключаем на ANIMATION_ROTATE_LEFT
                {
                    //Если анимация разворота в модели шасси вообще имеется
                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_ROTATE_LEFT) || robot_chassis_graph->CheckAnimAvailability(ANIMATION_ROTATE_RIGHT))
                    {
                        m_Animation = ANIMATION_ROTATE_LEFT;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_LEFT, ANIM_LOOP_ON);

                        if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                    }
                    else
                    {
                        m_Animation = ANIMATION_STAY;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_STAY, ANIM_LOOP_ON);
                        return;
                    }
                }
            }
            else if( m_Animation >= ANIMATION_BEGIN_MOVE         && m_Animation <= ANIMATION_END_MOVE_BACK ||
                     m_Animation == ANIMATION_BEGIN_ROTATE_RIGHT || m_Animation == ANIMATION_END_ROTATE_RIGHT )
            {
                goto SetAnimStay;
            }
            else if(m_Animation == ANIMATION_ROTATE_RIGHT)
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_END_ROTATE_RIGHT))
                {
                    m_Animation = ANIMATION_END_ROTATE_RIGHT;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_END_ROTATE_RIGHT, ANIM_LOOP_OFF);
                }
                else //Если такой анимации в списке модели не нашлось, то сразу переключаем на ANIMATION_BEGIN_ROTATE_LEFT
                {
                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_ROTATE_RIGHT))
                    {
                        m_Animation = ANIMATION_BEGIN_ROTATE_RIGHT;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_ROTATE_RIGHT, ANIM_LOOP_OFF, ANIM_REVERSED);
                    }
                    else
                    {
                        if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_ROTATE_LEFT))
                        {
                            m_Animation = ANIMATION_BEGIN_ROTATE_LEFT;
                            robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_ROTATE_LEFT, ANIM_LOOP_OFF);
                        }
                        else //Если такой анимации в списке модели не нашлось, то сразу переключаем на ANIMATION_ROTATE_LEFT
                        {
                            m_Animation = ANIMATION_ROTATE_LEFT;

                            //Если в модели отсутствует анимация ROTATE_RIGHT, значит, переключение анимаций разворота в ней осуществляется посредством инверсии,
                            //и нам нет смысла сбрасывать текущий кадр в самое начало
                            if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_ROTATE_RIGHT))
                            {
                                robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_LEFT, ANIM_LOOP_ON);
                            }
                            else
                            {
                                robot_chassis_graph->SetAnimReversed(false);
                                if(robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME)) robot_chassis_graph->SetFirstFrame(); //На всякий случай сразу же перезапускаем остановившуюся в конце секвенции анимацию
                            }

                            if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                        }
                    }
                }
            }
            //Нестандартная ситуация: робот уже разворачивается, либо начал разворот
            else if(m_Animation == ANIMATION_END_ROTATE_LEFT)
            {
                if(robot_chassis_graph->GetAnimName() == ANIMATION_NAME_END_ROTATE_LEFT)
                {
                    if(!robot_chassis_graph->IsAnimEnd(AT_FIRST_FRAME)) robot_chassis_graph->SetAnimReversed(true);
                    else
                    {
                        m_Animation = ANIMATION_ROTATE_LEFT;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_LEFT, ANIM_LOOP_ON);

                        if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                    }
                }
                else
                {
                    if(!robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME)) robot_chassis_graph->SetAnimReversed(false);
                    {
                        m_Animation = ANIMATION_ROTATE_LEFT;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_LEFT, ANIM_LOOP_ON);

                        if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                    }
                }
            }
            //Предполагается, что если уже включена ANIMATION_BEGIN_ROTATE_LEFT, то до она никогда не должна быть реверсирована
            else if(m_Animation == ANIMATION_BEGIN_ROTATE_LEFT)
            {
                robot_chassis_graph->SetAnimReversed(false);

                if(robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME))
                {
                    m_Animation = ANIMATION_ROTATE_LEFT;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_LEFT, ANIM_LOOP_ON);

                    if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                }
            }

            break;
        }

        //Необходимо переключить анимацию шасси робота на разворот направо
        case ANIMATION_ROTATE_RIGHT:
        {
            if( m_Animation == ANIMATION_STAY ||
                m_Animation == ANIMATION_OFF )
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_ROTATE_RIGHT))
                {
                    m_Animation = ANIMATION_BEGIN_ROTATE_RIGHT;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_ROTATE_RIGHT, ANIM_LOOP_OFF);
                }
                else //Если такой анимации в списке модели не нашлось, то сразу переключаем на ANIMATION_ROTATE_RIGHT
                {
                    //Если анимация разворота в модели шасси вообще имеется
                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_ROTATE_LEFT))
                    {
                        m_Animation = ANIMATION_ROTATE_RIGHT;

                        if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_ROTATE_RIGHT)) robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_RIGHT, ANIM_LOOP_ON);
                        //Если анимации движения назад в списке модели также не нашлось (например, для колёс или траков), то просто инвертируем анимацию MOVE
                        else robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_LEFT, ANIM_LOOP_ON, ANIM_REVERSED);

                        if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                    }
                    else
                    {
                        m_Animation = ANIMATION_STAY;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_STAY, ANIM_LOOP_ON);
                        return;
                    }
                }
            }
            else if( m_Animation >= ANIMATION_BEGIN_MOVE        && m_Animation <= ANIMATION_END_MOVE_BACK ||
                     m_Animation == ANIMATION_BEGIN_ROTATE_LEFT || m_Animation == ANIMATION_END_ROTATE_LEFT )
            {
                goto SetAnimStay;
            }
            //Здесь перезапускаем реверсированную анимацию MOVE_BACK, если в её последовательности закончились кадры
            else if(m_Animation == ANIMATION_ROTATE_RIGHT)
            {
                if(robot_chassis_graph->IsAnimReversed())
                {
                    if(robot_chassis_graph->IsAnimEnd(AT_FIRST_FRAME)) robot_chassis_graph->SetLastFrame();
                }
            }
            else if(m_Animation == ANIMATION_ROTATE_LEFT)
            {
                if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_END_ROTATE_LEFT))
                {
                    m_Animation = ANIMATION_END_ROTATE_LEFT;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_END_ROTATE_LEFT, ANIM_LOOP_OFF);
                }
                else //Если такой анимации в списке модели не нашлось, то сразу переключаем на ANIMATION_BEGIN_ROTATE_RIGHT
                {
                    if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_ROTATE_LEFT))
                    {
                        m_Animation = ANIMATION_BEGIN_ROTATE_LEFT;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_ROTATE_LEFT, ANIM_LOOP_OFF, ANIM_REVERSED);
                    }
                    else
                    {
                        if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_BEGIN_ROTATE_RIGHT))
                        {
                            m_Animation = ANIMATION_BEGIN_ROTATE_RIGHT;
                            robot_chassis_graph->SetAnimByName(ANIMATION_NAME_BEGIN_ROTATE_RIGHT, ANIM_LOOP_OFF);
                        }
                        else //Если такой анимации в списке модели не нашлось (вернулось true), то сразу переключаем на ANIMATION_ROTATE_RIGHT
                        {
                            m_Animation = ANIMATION_ROTATE_RIGHT;

                            if(robot_chassis_graph->CheckAnimAvailability(ANIMATION_ROTATE_RIGHT)) robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_RIGHT);
                            //Если анимации разворота вправо в списке модели также не нашлось, то просто инвертируем анимацию ROTATE_LEFT
                            //А поскольку прямо сейчас на модели и так выставлена анимация ROTATE_LEFT, сбрасывать текущий кадр в ней нам не имеет смысла
                            else
                            {
                                robot_chassis_graph->SetAnimReversed(true);
                                if(robot_chassis_graph->IsAnimEnd(AT_FIRST_FRAME)) robot_chassis_graph->SetLastFrame(); //На всякий случай сразу же перезапускаем остановившуюся в конце секвенции анимацию
                            }

                            if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                        }
                    }
                }
            }
            //Нестандартная ситуация: робот уже вращается, либо начал вращение
            else if(m_Animation == ANIMATION_END_ROTATE_RIGHT)
            {
                if(robot_chassis_graph->GetAnimName() == ANIMATION_NAME_END_ROTATE_RIGHT)
                {
                    //Реверсируем анимацию END_ROTATE_RIGHT, пока она не вернёт модель в положение движения назад
                    if(!robot_chassis_graph->IsAnimEnd(AT_FIRST_FRAME)) robot_chassis_graph->SetAnimReversed(true);
                    //И только после этого запускаем анимацию движения назад
                    else
                    {
                        m_Animation = ANIMATION_ROTATE_RIGHT;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_RIGHT, ANIM_LOOP_ON);

                        if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                    }
                }
                else
                {
                    //Реверсируем анимацию BEGIN_ROTATE_RIGHT (заменяющую END_ROTATE_RIGHT), пока она не вернёт модель в положение движения назад
                    if(!robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME)) robot_chassis_graph->SetAnimReversed(false);
                    //И только после этого запускаем анимацию движения назад
                    else
                    {
                        m_Animation = ANIMATION_ROTATE_RIGHT;
                        robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_RIGHT, ANIM_LOOP_ON);

                        if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                    }
                }
            }
            //Предполагается, что если уже включена ANIMATION_BEGIN_ROTATE_RIGHT, то она никогда не должна быть реверсирована
            else if(m_Animation == ANIMATION_BEGIN_ROTATE_RIGHT)
            {
                robot_chassis_graph->SetAnimReversed(false);

                if(robot_chassis_graph->IsAnimEnd(AT_LAST_FRAME))
                {
                    m_Animation = ANIMATION_ROTATE_RIGHT;
                    robot_chassis_graph->SetAnimByName(ANIMATION_NAME_ROTATE_RIGHT, ANIM_LOOP_ON);
                    if(IsWalkingChassis()) FirstStepLinkWalkingChassis();
                }
            }

            break;
        }

        default: m_Animation = target_anim; //А это чё за хуйня, нахуя?
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

SWalkingChassisData* CMatrixRobot::m_WalkingChassis;

int g_fcnt = 0; //Каким-то образом используется в линковке шагов шагающего шасси для отрисовки под ступнями следов - вероятно, чей-то кривой костыль (fcnt == fucking no trace?)

//Здесь рисуются следы под шасси "Пневматика"
void CMatrixRobot::StepLinkWalkingChassis()
{
    if(!FLAG(m_ObjectFlags, ROBOT_FLAG_LINKED)) return; // never linked
    if(!m_Module[0].m_Graph->IsAnim(ANIMATION_NAME_MOVE) && !m_Module[0].m_Graph->IsAnim(ANIMATION_NAME_MOVE_BACK)) return;

    int vof = m_Module[0].m_Graph->GetFrame();
    int vof_add = 0;

    if(FLAG(m_ObjectFlags, ROBOT_FLAG_MOVING_BACK))
    {
        //reverse linking
        vof_add = g_fcnt;
    }

    if(m_ChassisData.m_LinkPrevFrame == vof) goto correction;

    //bool newlink = false;
    while(m_ChassisData.m_LinkPrevFrame != vof)
    {
        if(m_WalkingChassis[m_ChassisData.m_LinkPrevFrame + vof_add].newlink)
        {
            D3DXVec2TransformCoord(&m_ChassisData.m_LinkPos, &m_WalkingChassis[m_ChassisData.m_LinkPrevFrame + vof_add].other_foot, &m_Core->m_Matrix);

            int x = TruncFloat(m_ChassisData.m_LinkPos.x * INVERT(GLOBAL_SCALE));
            int y = TruncFloat(m_ChassisData.m_LinkPos.y * INVERT(GLOBAL_SCALE));
            SMatrixMapUnit* mu = g_MatrixMap->UnitGetTest(x, y);
            if(mu && mu->IsLand() && !mu->IsBridge())
            {
                float ang = (float)atan2(-m_ChassisForward.x, m_ChassisForward.y);
                CMatrixEffect::CreateLandscapeSpot(nullptr, m_ChassisData.m_LinkPos, ang, g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].ground_trace.texture_scale, (ESpotType)g_Config.m_RobotChassisConsts[m_Module[0].m_Kind].ground_trace.trace_num);
            }
        }

        ++m_ChassisData.m_LinkPrevFrame;

        if(m_ChassisData.m_LinkPrevFrame >= g_fcnt) m_ChassisData.m_LinkPrevFrame = 0;
    }

correction:

    if(m_CurrentState != ROBOT_CARRYING && !FLAG(m_ObjectFlags, ROBOT_FLAG_COLLISION))
    {
        D3DXMATRIX m;
        D3DXVECTOR3 tmp_forward, up, side;

        //D3DXVec3Normalize(&up, (D3DXVECTOR3*)&m_Module[0].m_Matrix._31);
        D3DXVec3Normalize(&up, (D3DXVECTOR3*)&m_Core->m_Matrix._31);
        //g_MatrixMap->GetNormal(&up, m_PosX, m_PosY);
        D3DXVec3Cross(&side, &m_ChassisForward, &up);
        D3DXVec3Normalize(&side, &side);
        D3DXVec3Cross(&tmp_forward, &up, &side);

        m._11 = side.x;         m._12 = side.y;         m._13 = side.z;         m._14 = 0;
        m._21 = tmp_forward.x;  m._22 = tmp_forward.y;  m._23 = tmp_forward.z;  m._24 = 0;
        m._31 = up.x;           m._32 = up.y;           m._33 = up.z;           m._34 = 0;
        m._41 = m_PosX;         m._42 = m_PosY;         m._43 = Z_From_Pos();   m._44 = 1; 

        D3DXVECTOR2 curlink;
        D3DXVec2TransformCoord(&curlink, &m_WalkingChassis[vof + vof_add].foot, &m);

        curlink -= m_ChassisData.m_LinkPos;

        m_PosX -= curlink.x;
        m_PosY -= curlink.y;
    }
}

void CMatrixRobot::FirstStepLinkWalkingChassis(void)
{
    if(!m_Module[0].m_Graph->IsAnim(ANIMATION_NAME_MOVE) && !m_Module[0].m_Graph->IsAnim(ANIMATION_NAME_MOVE_BACK)) return;

    SETFLAG(m_ObjectFlags, ROBOT_FLAG_LINKED);

    D3DXMATRIX m;
    D3DXVECTOR3 tmp_forward, up, side;

    //D3DXVec3Normalize(&up, (D3DXVECTOR3*)&m_Module[0].m_Matrix._31);
    D3DXVec3Normalize(&up, (D3DXVECTOR3*)&m_Core->m_Matrix._31);
    //g_MatrixMap->GetNormal(&up, m_PosX,m_PosY);
    D3DXVec3Cross(&side, &m_ChassisForward, &up);
    D3DXVec3Normalize(&side, &side);
    D3DXVec3Cross(&tmp_forward, &up, &side);

    m._11 = side.x;         m._12 = side.y;         m._13 = side.z;         m._14 = 0;
    m._21 = tmp_forward.x;  m._22 = tmp_forward.y;  m._23 = tmp_forward.z;  m._24 = 0;
    m._31 = up.x;           m._32 = up.y;           m._33 = up.z;           m._34 = 0;
    m._41 = m_PosX;         m._42 = m_PosY;         m._43 = Z_From_Pos();   m._44 = 1; 

    int vof = m_Module[0].m_Graph->GetFrame();
    int vof_add = 0;

    if(FLAG(m_ObjectFlags, ROBOT_FLAG_MOVING_BACK))
    {
        //reverse linking
        vof_add = g_fcnt;
    }

    D3DXVec2TransformCoord(&m_ChassisData.m_LinkPos, &m_WalkingChassis[vof + vof_add].foot, &m);
    m_ChassisData.m_LinkPrevFrame = vof;
}


void CMatrixRobot::BuildWalkingChassisData(CVectorObject* vo, int chassis_num)
{
    int moveanim = vo->GetAnimByName(ANIMATION_NAME_MOVE);
    g_fcnt = vo->GetAnimFramesCount(moveanim);

    m_WalkingChassis = (SWalkingChassisData*)HAlloc(sizeof(SWalkingChassisData) * g_fcnt * 2, g_MatrixHeap);

    //Для движения вперёд
    {
        int leftf = -1;
        int ritef = -1;

        float maxl = -100;
        float maxr = -100;

        for(int i = 0; i < g_fcnt; ++i)
        {
            int frame = vo->GetAnimFrameIndex(moveanim, i);

            const D3DXMATRIX* ml = vo->GetMatrixById(frame, 2);
            const D3DXMATRIX* mr = vo->GetMatrixById(frame, 3);

            if(ml->_42 > maxl)
            {
                maxl = ml->_42;
                leftf = i;
            }

            if(mr->_42 > maxr)
            {
                maxr = mr->_42;
                ritef = i;
            }
        }

        int prev = (leftf < ritef) ? 3 : 2;
        for(int i = 0; i < g_fcnt; ++i)
        {
            /*
            if(i == fcnt)
            {
                // check first frame of animation (may be it looped with last frame)
                fcnt = 0;
                i = 0;
            }
            */

            //Здесь сейчас запоминаем матрицы только для двух ног, но, в целом, нужно бы расширить этот диапазон
            int frame = vo->GetAnimFrameIndex(moveanim, i);
            const D3DXMATRIX* ml = vo->GetMatrixById(frame, g_Config.m_RobotChassisConsts[chassis_num].trace_matrix[0].matrix_id);//2);
            const D3DXMATRIX* mr = vo->GetMatrixById(frame, g_Config.m_RobotChassisConsts[chassis_num].trace_matrix[1].matrix_id);//3);

            int newv = prev;

            if(i == leftf) newv = 2;
            else if(i == ritef) newv = 3;

            if(newv == 2)
            {
                m_WalkingChassis[i].newlink = prev != newv;
                prev = newv;
                if(m_WalkingChassis[i].newlink)
                {
                    m_WalkingChassis[i].other_foot.x = ml->_41;
                    m_WalkingChassis[i].other_foot.y = ml->_42;

                    m_WalkingChassis[i].foot.x = mr->_41;
                    m_WalkingChassis[i].foot.y = mr->_42;
                }
                else
                {
                    m_WalkingChassis[i].foot.x = ml->_41;
                    m_WalkingChassis[i].foot.y = ml->_42;
                }
            }
            else
            {
                m_WalkingChassis[i].newlink = prev != 3;
                prev = newv;
                if(m_WalkingChassis[i].newlink)
                {
                    m_WalkingChassis[i].other_foot.x = mr->_41;
                    m_WalkingChassis[i].other_foot.y = mr->_42;

                    m_WalkingChassis[i].foot.x = ml->_41;
                    m_WalkingChassis[i].foot.y = ml->_42;
                }
                else
                {
                    m_WalkingChassis[i].foot.x = mr->_41;
                    m_WalkingChassis[i].foot.y = mr->_42;
                }
            }
        }
    }

    //Для движения назад
    {
        int leftf = -1;
        int ritef = -1;

        float maxl = 100;
        float maxr = 100;

        moveanim = vo->GetAnimByName(ANIMATION_NAME_MOVE_BACK);

        for(int i = 0; i < g_fcnt; ++i)
        {
            //Здесь сейчас запоминаем матрицы только для двух ног, но, в целом, нужно бы расширить этот диапазон
            int frame = vo->GetAnimFrameIndex(moveanim, i);
            const D3DXMATRIX* ml = vo->GetMatrixById(frame, g_Config.m_RobotChassisConsts[chassis_num].trace_matrix[0].matrix_id);//2);
            const D3DXMATRIX* mr = vo->GetMatrixById(frame, g_Config.m_RobotChassisConsts[chassis_num].trace_matrix[1].matrix_id);//3);

            if(ml->_42 < maxl)
            {
                maxl = ml->_42;
                leftf = i;
            }

            if(mr->_42 < maxr)
            {
                maxr = mr->_42;
                ritef = i;
            }
        }

        int prev = (leftf < ritef)?3:2;
        for(int i = 0; i < g_fcnt; ++i)
        {
            //if(i == fcnt)
            //{
            //    // check first frame of animation (may be it looped with last frame)
            //    fcnt = 0;
            //    i = 0;
            //}

            int frame = vo->GetAnimFrameIndex(moveanim, i);

            const D3DXMATRIX* ml = vo->GetMatrixById(frame, 2);
            const D3DXMATRIX* mr = vo->GetMatrixById(frame, 3);

            int newv = prev;

            if(i == leftf) newv = 2;
            else if(i == ritef) newv = 3;

            if(newv == 2)
            {
                m_WalkingChassis[i + g_fcnt].newlink = prev != newv;
                prev = newv;
                if(m_WalkingChassis[i + g_fcnt].newlink)
                {
                    m_WalkingChassis[i + g_fcnt].other_foot.x = ml->_41;
                    m_WalkingChassis[i + g_fcnt].other_foot.y = ml->_42;

                    m_WalkingChassis[i + g_fcnt].foot.x = mr->_41;
                    m_WalkingChassis[i + g_fcnt].foot.y = mr->_42;
                }
                else
                {
                    m_WalkingChassis[i + g_fcnt].foot.x = ml->_41;
                    m_WalkingChassis[i + g_fcnt].foot.y = ml->_42;
                }
            }
            else
            {
                m_WalkingChassis[i + g_fcnt].newlink = prev != 3;
                prev = newv;
                if(m_WalkingChassis[i + g_fcnt].newlink)
                {
                    m_WalkingChassis[i + g_fcnt].other_foot.x = mr->_41;
                    m_WalkingChassis[i + g_fcnt].other_foot.y = mr->_42;

                    m_WalkingChassis[i + g_fcnt].foot.x = ml->_41;
                    m_WalkingChassis[i + g_fcnt].foot.y = ml->_42;
                }
                else
                {
                    m_WalkingChassis[i + g_fcnt].foot.x = mr->_41;
                    m_WalkingChassis[i + g_fcnt].foot.y = mr->_42;
                }
            }
        }
    }
}
void CMatrixRobot::DestroyWalkingChassisData(void)
{
    if(m_WalkingChassis) HFree(m_WalkingChassis, g_MatrixHeap);
}

bool CMatrixRobot::InRect(const CRect& rect) const
{
    if(m_CurrentState == ROBOT_DIP) return false;

    D3DXVECTOR3 dir;
    g_MatrixMap->m_Camera.CalcPickVector(CPoint(rect.left, rect.top), dir);
    if(Pick(g_MatrixMap->m_Camera.GetFrustumCenter(), dir, nullptr)) return true;

    D3DXMATRIX s, t;
    SEVH_data d;

    t = g_MatrixMap->m_Camera.GetViewMatrix() * g_MatrixMap->m_Camera.GetProjMatrix();
    D3DXMatrixScaling(&s, float(g_ScreenX / 2), float(-g_ScreenY / 2), 1);
    s._41 = s._11;
    s._42 = float(g_ScreenY / 2);
    t *= s;
    d.rect = &rect;

    for(int i = 0; i < m_ModulesCount; ++i)
    {
        if(m_Module[i].m_Graph)
        {
            d.found = false;
            d.m = m_Module[i].m_Matrix * t;
            m_Module[i].m_Graph->EnumFrameVerts(EnumVertsHandler, (dword)&d);
            if(d.found) return true;
        }
    }
    
    g_MatrixMap->m_Camera.CalcPickVector(CPoint(rect.left, rect.bottom), dir);
    if(Pick(g_MatrixMap->m_Camera.GetFrustumCenter(), dir, nullptr)) return true;

    g_MatrixMap->m_Camera.CalcPickVector(CPoint(rect.right, rect.top), dir);
    if(Pick(g_MatrixMap->m_Camera.GetFrustumCenter(), dir, nullptr)) return true;
    
    g_MatrixMap->m_Camera.CalcPickVector(CPoint(rect.right, rect.bottom), dir);
    if(Pick(g_MatrixMap->m_Camera.GetFrustumCenter(), dir, nullptr)) return true;

    return false;
}