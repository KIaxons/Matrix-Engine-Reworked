// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "../stdafx.h"
#include "MatrixEffect.hpp"
#include "../MatrixMap.hpp"
#include <math.h>


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//������ ������������� ���� �� ���������� �������� ����
CMatrixEffectSmoke::CMatrixEffectSmoke(
    const D3DXVECTOR3& pos,
    float ttl,
    float puffttl,
    float spawntime,
    dword color,
    bool is_bright,
    float speed
) : CMatrixEffect(), m_Pos(pos), m_Mins(pos), m_Maxs(pos), m_TTL(ttl), m_PuffTTL(puffttl), m_Spawn(spawntime), m_Color(color), m_Speed(speed)
{
    m_EffectType = is_bright ? EFFECT_BRIGHT_SMOKE : EFFECT_SMOKE;
    ELIST_ADD(EFFECT_SMOKE);

    m_Puffs = (SSmokePuff*)HAlloc(sizeof(SSmokePuff) * MAX_PUFF_COUNT, m_Heap);
}

CMatrixEffectSmoke::~CMatrixEffectSmoke()
{
    while(m_PuffCnt > 0) m_Puffs[--m_PuffCnt].m_Puff.Release();
    HFree(m_Puffs, m_Heap);
    ELIST_DEL(EFFECT_SMOKE);
}

void CMatrixEffectSmoke::Release()
{
    SetDIP();
    HDelete(CMatrixEffectSmoke, this, m_Heap);
}

void CMatrixEffectSmoke::BeforeDraw()
{

}

void CMatrixEffectSmoke::Draw()
{
    if(!(g_MatrixMap->m_Camera.IsInFrustum(m_Mins,m_Maxs))) return;

    //CHelper::Create(1, 0)->BoundBox(m_Mins, m_Maxs);

    int i = 0;
    while(i < m_PuffCnt)
    {
        m_Puffs[i].m_Puff.Sort(g_MatrixMap->m_Camera.GetViewMatrix());
        ++i;
    }
}

void CMatrixEffectSmoke::Tact(float step)
{
    float dtime = (m_Speed * step);

    m_Time += step;
    m_TTL -= step;
    while((m_Time > m_NextSpawnTime) && m_TTL > 0)
    {
        SpawnPuff();
        m_NextSpawnTime += m_Spawn;
    }

    if((!m_PuffCnt) && (m_TTL < 0))
    {
        g_MatrixMap->SubEffect(this);
        return;
    }

    int i = 0;
    while(i < m_PuffCnt)
    {
        m_Puffs[i].m_PuffTTL -= step;
        if(m_Puffs[i].m_PuffTTL < 0)
        {
            m_Puffs[i].m_Puff.Release();
            m_Puffs[i] = m_Puffs[--m_PuffCnt];
            continue;
        }

        ++i;
    }

    i = 0;
    while(i < m_PuffCnt)
    {
        float life = m_Puffs[i].m_PuffTTL / m_PuffTTL;

        m_Puffs[i].m_Puff.m_Pos.z += dtime * m_Speed * (5 + (m_Puffs[i].m_Puff.m_Pos.z - m_Puffs[i].m_PuffOrig.z));
        //if(m_Puff[i].m_Pos.z > m_MaxZ) m_MaxZ = m_Puff[i].m_Pos.z;
        m_Puffs[i].m_Puff.SetScale(LERPFLOAT(life, PUFF_SCALE2, PUFF_SCALE1));
        m_Puffs[i].m_Puff.SetAlpha((byte)Float2Int((m_Color >> 24) * life));
        float a = M_PI_MUL(life * m_Speed * 25);
        SET_SIGN_FLOAT(a, (i & 1));
        m_Puffs[i].m_Puff.SetAngle(m_Puffs[i].m_PuffAngle + a, 0, 0);

        ++i;
    }

    // update bbox
    if(m_PuffCnt > 0)
    {
        m_Mins = m_Puffs[0].m_Puff.m_Pos;
        m_Maxs = m_Mins;
        i = 0;
        while(i < m_PuffCnt)
        {
            AddSphereToBox(m_Mins, m_Maxs, m_Puffs[i].m_Puff.m_Pos, m_Puffs[i].m_Puff.GetScale());
            ++i;
        }
    }
    else
    {
        m_Mins = m_Pos;
        m_Maxs = m_Pos;
    }
}

void CMatrixEffectSmoke::SpawnPuff()
{
    if(m_PuffCnt >= MAX_PUFF_COUNT) return;
    m_Puffs[m_PuffCnt].m_PuffAngle = FRND((float)M_PI);

    ESpriteTextureSort tex = SPR_SMOKE_PART;
    switch(m_EffectType)
    {
        case EFFECT_SMOKE:         tex = SPR_SMOKE_PART; break;
        case EFFECT_BRIGHT_SMOKE:  tex = SPR_BRIGHT_SMOKE_PART; break;
        case EFFECT_FIRE:          tex = SPR_FIRE_PART; break;
        case EFFECT_BRIGHT_FIRE:   tex = SPR_BRIGHT_FIRE_PART; break;
    }

    if(m_SpriteTextures[tex].IsSingleBrightTexture()) m_Puffs[m_PuffCnt].m_Puff.CSprite::CSprite(TRACE_PARAM_CALL m_Pos, 5 - FRND(2), m_Puffs[m_PuffCnt].m_PuffAngle, m_Color, m_SpriteTextures[tex].tex);
    else m_Puffs[m_PuffCnt].m_Puff.CSprite::CSprite(TRACE_PARAM_CALL m_Pos, 5 - FRND(2), m_Puffs[m_PuffCnt].m_PuffAngle, m_Color, &m_SpriteTextures[tex].spr_tex);

    m_Puffs[m_PuffCnt].m_PuffOrig = m_Pos;
    m_Puffs[m_PuffCnt].m_PuffTTL = m_PuffTTL;
    ++m_PuffCnt;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//������ ������������� ������� �� ���������� �������� ���� 
CMatrixEffectFire::CMatrixEffectFire(
    const D3DXVECTOR3& pos,
    float ttl,
    float puffttl,
    float spawntime,
    float dispfactor,
    bool is_bright,
    float speed,
    const SFloatRGBColor& close_color,
    const SFloatRGBColor& far_color
) : CMatrixEffect(), m_Pos(pos), m_Mins(pos), m_Maxs(pos), m_TTL(ttl), m_PuffTTL(puffttl), m_Spawn(spawntime), m_Speed(speed), m_DispFactor(dispfactor), m_CloseColor(close_color), m_FarColor(far_color)
{
    m_EffectType = is_bright ? EFFECT_BRIGHT_FIRE : EFFECT_FIRE;
    ELIST_ADD(EFFECT_SMOKE);

    m_Puffs = (SFirePuff*)HAlloc(sizeof(SFirePuff) * MAX_PUFF_COUNT, m_Heap);
}

CMatrixEffectFire::~CMatrixEffectFire()
{
    while(m_PuffCnt > 0) m_Puffs[--m_PuffCnt].m_Puff.Release(); 

    HFree(m_Puffs, m_Heap);
    ELIST_DEL(EFFECT_SMOKE);
}

void CMatrixEffectFire::Release()
{
    SetDIP();
    HDelete(CMatrixEffectFire, this, m_Heap);
}

void CMatrixEffectFire::BeforeDraw()
{

}

void CMatrixEffectFire::Draw()
{
    if(!(g_MatrixMap->m_Camera.IsInFrustum(m_Mins,m_Maxs))) return;
    //CHelper::Create(1, 0)->BoundBox(m_Mins, m_Maxs);

    int i = 0;
    while(i < m_PuffCnt)
    {
        m_Puffs[i].m_Puff.Sort(g_MatrixMap->m_Camera.GetViewMatrix());
        ++i;
    }
}

void CMatrixEffectFire::SpawnPuff()
{
    if(m_PuffCnt >= MAX_PUFF_COUNT) return;
    m_Puffs[m_PuffCnt].m_PuffAngle = FRND((float)M_PI);

    ESpriteTextureSort tex = SPR_FIRE_PART;
    switch(m_EffectType)
    {
        case EFFECT_FIRE:          tex = SPR_FIRE_PART; break;
        case EFFECT_BRIGHT_FIRE:   tex = SPR_BRIGHT_FIRE_PART; break;
        case EFFECT_SMOKE:         tex = SPR_SMOKE_PART; break;
        case EFFECT_BRIGHT_SMOKE:  tex = SPR_BRIGHT_SMOKE_PART; break;
    }

    if(m_SpriteTextures[tex].IsSingleBrightTexture()) m_Puffs[m_PuffCnt].m_Puff.CSprite::CSprite(TRACE_PARAM_CALL m_Pos, 5 - FRND(2.0f), m_Puffs[m_PuffCnt].m_PuffAngle, 0xFFFFFFFF, m_SpriteTextures[tex].tex);
    else m_Puffs[m_PuffCnt].m_Puff.CSprite::CSprite(TRACE_PARAM_CALL m_Pos, 5 - FRND(2.0f), m_Puffs[m_PuffCnt].m_PuffAngle, 0xFFFFFFFF, &m_SpriteTextures[tex].spr_tex);
    
    m_Puffs[m_PuffCnt].m_PuffOrig = m_Pos;
    m_Puffs[m_PuffCnt].m_PuffTTL = m_PuffTTL;
    m_Puffs[m_PuffCnt].m_PuffDir = D3DXVECTOR2(0.0f, 0.0f);
    ++m_PuffCnt;
}

void CMatrixEffectFire::Tact(float step)
{
    float dtime = (SMOKE_SPEED * step);

    m_Time += step;
    m_TTL -= step;
    while((m_Time > m_NextSpawnTime) && m_TTL > 0)
    {
        SpawnPuff();
        m_NextSpawnTime += m_Spawn;
    }

    if((!m_PuffCnt) && (m_TTL < 0))
    {
        g_MatrixMap->SubEffect(this);
        return;
    }

    int i;
    i = 0;
    while(i < m_PuffCnt)
    {
        m_Puffs[i].m_PuffTTL -= step;
        if(m_Puffs[i].m_PuffTTL < 0)
        {
            m_Puffs[i].m_Puff.Release();
            m_Puffs[i] = m_Puffs[--m_PuffCnt];
            //m_MaxZ = m_Pos.z;
            continue;
        }

        ++i;
    }

    i = 0;
    while(i < m_PuffCnt)
    {
        float life = 1.0f - (m_Puffs[i].m_PuffTTL / m_PuffTTL);

        m_Puffs[i].m_Puff.m_Pos.z += dtime * m_Speed * (10 + (m_Puffs[i].m_Puff.m_Pos.z - m_Puffs[i].m_PuffOrig.z));
        //if(m_Puff[i].m_Pos.z > m_MaxZ) m_MaxZ = m_Puff[i].m_Pos.z;
        m_Puffs[i].m_Puff.SetScale(PUFF_FIRE_SCALE); //(1.0f - life) * (END_OF_LIFE_SCALE - 1.0f) + 1.0f;
        //m_Puff[i].SetAlpha((byte)(255 * life));

        //���� �������� � ����������� �� �������� ��������� ���� �� ����� �������
        byte a = (byte)Float2Int(255.0f * (KSCALE(life, 0.0f, 0.3f) - (KSCALE(life, 0.3f, 1.0f))));
        //byte a = (byte)(255.0f * (KSCALE(life, 0.0f, 0.2f)));
        byte r = (byte)Float2Int((1.0f - KSCALE(life, m_FarColor.red, m_CloseColor.red)) * 255);
        byte g = (byte)Float2Int((1.0f - KSCALE(life, m_FarColor.green, m_CloseColor.green)) * 255);
        byte b = (byte)Float2Int((1.0f - KSCALE(life, m_FarColor.blue, m_CloseColor.blue)) * 255);

        m_Puffs[i].m_Puff.SetColor((a << 24) | (r << 16) | (g << 8) | b);

        float xl = -m_DispFactor, xr = m_DispFactor;
        float yl = -m_DispFactor, yr = m_DispFactor;

        float dx = (m_Puffs[i].m_Puff.m_Pos.x - m_Puffs[i].m_PuffOrig.x);
        float dy = (m_Puffs[i].m_Puff.m_Pos.y - m_Puffs[i].m_PuffOrig.y);

        if(dx > m_DispFactor) xr = m_DispFactor / dx;
        else if(dx < -m_DispFactor) xl = m_DispFactor / dx;

        if(dy > m_DispFactor) yr = m_DispFactor / dy;
        else if(dy < -m_DispFactor) yl = m_DispFactor / dy;

        m_Puffs[i].m_PuffDir.x += float(0.01 * dtime * RND(xl, xr));
        m_Puffs[i].m_PuffDir.y += float(0.01 * dtime * RND(yl, yr));
        m_Puffs[i].m_Puff.m_Pos.x += m_Puffs[i].m_PuffDir.x;
        m_Puffs[i].m_Puff.m_Pos.y += m_Puffs[i].m_PuffDir.y;

        ++i;
    }
    
    // update bbox
    if(m_PuffCnt > 0)
    {
        m_Mins = m_Puffs[0].m_Puff.m_Pos;
        m_Maxs = m_Mins;
        i = 0;
        while(i < m_PuffCnt)
        {
            AddSphereToBox(m_Mins, m_Maxs, m_Puffs[i].m_Puff.m_Pos, m_Puffs[i].m_Puff.GetScale());
            ++i;
        }
    }
    else
    {
        m_Mins = m_Pos;
        m_Maxs = m_Pos;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//�������� ������� �������� �����, ������� ������������ � ���������� ��������� � ��� ����������
CMatrixEffectFireStream::CMatrixEffectFireStream(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, float width, dword color, const std::vector<int>& sprites_num) :
    CMatrixEffect(), m_SpritesCount((byte)sprites_num.size()), m_Sprites(new CSpriteSequence[m_SpritesCount])
{
    for(int i = 0; i < m_SpritesCount; ++i)
    {
        m_Sprites[i].CSpriteSequence::CSpriteSequence(TRACE_PARAM_CALL pos0, pos1, width, color, m_SpriteTextures[sprites_num[i]].tex);
    }

    m_EffectType = EFFECT_FIRE_STREAM;
}

void CMatrixEffectFireStream::BeforeDraw()
{

}

void CMatrixEffectFireStream::Draw(bool now)
{
    int idx = g_MatrixMap->IsPaused() ? 1 : (IRND(m_SpritesCount)); //����� ���������� ������ �����
    if(now) m_Sprites[idx].DrawNow(g_MatrixMap->m_Camera.GetDrawNowFC());
    else m_Sprites[idx].AddToDrawQueue();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//�������� ������������ ���������� ��������, ����������� � �������� �� ������
CMatrixEffectLinkedSpriteAnim::CMatrixEffectLinkedSpriteAnim(const D3DXVECTOR3& pos0, const D3DXVECTOR3& pos1, float width, dword color, int frame_delay, bool camera_oriented, const std::vector<int>& sprites_num) :
    m_FrameDelay(frame_delay), m_SpritesCount(sprites_num.size()), m_Sprites(new CSpriteSequence[m_SpritesCount])
{
    for(int i = 0; i < m_SpritesCount; ++i)
    {
        //if(m_SpriteTextures[sprites_num[i]].IsSingleBrightTexture())
        m_Sprites[i].CSpriteSequence::CSpriteSequence(TRACE_PARAM_CALL pos0, pos1, width, color, m_SpriteTextures[sprites_num[i]].tex, camera_oriented);
        //else m_Sprites[i].CSpriteSequence::CSpriteSequence(TRACE_PARAM_CALL pos0, pos1, width, color, &m_SpriteTextures[sprites_num[i]].spr_tex, camera_oriented);
    }

    //��� �������
    //if (m_SpriteTextures[tex].IsSingleBrightTexture()) m_Puffs[m_PuffCnt].m_Puff.CSprite::CSprite(TRACE_PARAM_CALL m_Pos, 5 - FRND(2), m_Puffs[m_PuffCnt].m_PuffAngle, m_Color, m_SpriteTextures[tex].tex);
    //else m_Puffs[m_PuffCnt].m_Puff.CSprite::CSprite(TRACE_PARAM_CALL m_Pos, 5 - FRND(2), m_Puffs[m_PuffCnt].m_PuffAngle, m_Color, &m_SpriteTextures[tex].spr_tex);

    m_EffectType = EFFECT_LINKED_ANIM;
}

void CMatrixEffectLinkedSpriteAnim::Tact(float tick)
{
    m_NextFrameDelay = max(m_NextFrameDelay - tick, 0);
    if(!m_NextFrameDelay)
    {
        if(!g_MatrixMap->IsPaused()) m_CurFrame = m_CurFrame + 1 < m_SpritesCount ? m_CurFrame + 1 : 0; //������ ��������
        m_NextFrameDelay = m_FrameDelay;
    }
}

void CMatrixEffectLinkedSpriteAnim::BeforeDraw()
{

}

void CMatrixEffectLinkedSpriteAnim::Draw(bool now)
{
    if(now) m_Sprites[m_CurFrame].DrawNow(g_MatrixMap->m_Camera.GetDrawNowFC());
    else m_Sprites[m_CurFrame].AddToDrawQueue();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//�������� ������� ������� ������������ �������� �� �����
CMatrixEffectFireAnim::CMatrixEffectFireAnim(const D3DXVECTOR3& pos, float anim_width, float anim_height, int time_to_live, const std::vector<int>& sprites_num, int sprites_count) :
    CMatrixEffect(), m_Pos(pos), m_Width(anim_width), m_Height(anim_height), m_SpritesCount(sprites_count), m_Sprites(new CSpriteSequence[sprites_count])
{
    m_EffectType = EFFECT_FIRE_ANIM;
    ELIST_ADD(EFFECT_FIRE_ANIM);
    m_NextTime = g_MatrixMap->GetTime();

    m_TTLcurr = float(time_to_live);
    m_TTL = 1.0f / m_TTLcurr;

    m_AnimFrame = IRND(m_SpritesCount); //��� ������ �������� ������ �������� ��������� ����

    for(int i = 0; i < m_SpritesCount; ++i)
    {
        m_Sprites[i].CSpriteSequence::CSpriteSequence(TRACE_PARAM_CALL pos, pos + D3DXVECTOR3(0.0f, 0.0f, 0.0f), 0.0f, 0xFFFFFFFF, m_SpriteTextures[sprites_num[i]].tex);
    }
}

void CMatrixEffectFireAnim::BeforeDraw()
{

}

void CMatrixEffectFireAnim::Tact(float tick)
{
    m_TTLcurr -= tick;
    if(m_TTLcurr <= 0)
    {
        g_MatrixMap->SubEffect(this);
        return;
    }

    float k = m_TTLcurr * m_TTL;
    if(k < FIREFRAME_TTL_POROG)
    {
        k *= INVERT(FIREFRAME_TTL_POROG);
        m_HeightCurr = LERPFLOAT(k, m_Height * FIREFRAME_H_DEAD, m_Height);
        m_WidthCurr = LERPFLOAT(k, m_Width * FIREFRAME_W_DEAD, m_Width);
    }
    else
    {
        k = (k - FIREFRAME_TTL_POROG) * INVERT(1 - FIREFRAME_TTL_POROG);
        m_HeightCurr = LERPFLOAT(k, m_Height, m_Height * FIREFRAME_H_DEAD);
        m_WidthCurr = LERPFLOAT(k, m_Width, m_Width * FIREFRAME_W_DEAD);
    }

    //if(m_HeightCurr > m_Height || m_WidthCurr > m_Width)
    //{
    //    _asm int 3
    //}

    Update();

    while(g_MatrixMap->GetTime() > m_NextTime)
    {
        m_NextTime += FIREFRAME_ANIM_PERIOD;
        ++m_AnimFrame;
        if(m_AnimFrame >= m_SpritesCount - 1) m_AnimFrame = 0;
    }
}

void CMatrixEffectFireAnim::Draw(bool now)
{
    if(now) m_Sprites[m_AnimFrame].DrawNow(g_MatrixMap->m_Camera.GetDrawNowFC());
    else m_Sprites[m_AnimFrame].AddToDrawQueue();
}