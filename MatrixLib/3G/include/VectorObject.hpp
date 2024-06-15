// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

extern IDirect3DDevice9* g_D3DD;

#define FREE_TEXTURE_TIME_PERIOD 7000
#define FREE_VO_TIME_PERIOD      30000

#define FRAME_NOT_CHANGED 0
#define FRAME_CHANGED 1
#define FRAMES_LOOP_DONE 2

#define ANIM_REVERSED true

#define AT_FIRST_FRAME false
#define AT_LAST_FRAME true

#define ANIM_LOOP_DEFAULT -1
#define ANIM_LOOP_OFF 0
#define ANIM_LOOP_ON 1

#include <bitset>
#include "Cache.hpp"
#include "D3DControl.hpp"
#include "BigVB.hpp"
#include "BigIB.hpp"
#include "ShadowProj.hpp"
#include "CBillboard.hpp"

#define GLOSS_TEXTURE_SUFFIX L"_gloss"

#define SHADOW_ALTITUDE (0.7f)

// resources
#define VOUF_MATRIX         SETBIT(0)
#define VOUF_SHADOWSTENCIL  SETBIT(1)
#define VOUF_RES            (VOUF_MATRIX | VOUF_SHADOWSTENCIL)

#define VOUF_MATRIX_USE     SETBIT(2)
#define VOUF_STENCIL_ON     SETBIT(3)

enum EObjectLoad
{
    OLF_NO_TEX,
    OLF_MULTIMATERIAL_ONLY,
    OLF_AUTO,
    OLF_SIMPLE
};

struct SVOSurface;
struct SSkin;

typedef const SSkin* (*SKIN_GET)(const wchar* tex, dword param);  // skin setup

typedef void (*SKIN_SETUP_TEX)(const SSkin* vo, dword user_param, int pass);
typedef bool (*SKIN_SETUP_STAGES)(const SSkin* vo, dword user_param, int pass);
typedef void (*SKIN_SETUP_CLEAR)(const SSkin* vo, dword user_param);
typedef void (*SKIN_SETUP_SHADOW)(const SSkin* vo);
typedef void (*SKIN_PRELOAD)(const SSkin* vo);

#define DEFAULT_ANIM_FRAME_PERIOD 100

// file formats
struct SVOHeader {
    dword m_Id = 0;				// 0x00006f76
    dword m_Ver = 0;			// Версия
    dword m_Flags = 0;			// 1-16 битный индекс иначе 32 битный, 2-откуда брать текстуры
    dword dummy = 0;
	int   m_MaterialCnt = 0;	// Список материалов(текстур) SMaterial
    dword m_MaterialSme = 0;	// Положение от начала заголовка
	int   m_GroupCnt = 0;		// Инвормация по группам (Смещение верши и треугольников)
    dword m_GroupSme = 0;
	int   m_VerCnt = 0;			// Список всех вершин SVertexNorTex
    dword m_VerSme = 0;
	int   m_TriCnt = 0;			// Список всех треугольников. Кол-во индексов (3 индкса для каждого трегольника по 2 или 4 байта взависимости от флага)
    dword m_TriSme = 0;
	int   m_FrameCnt = 0;		// Список кадров SVOFrame
    dword m_FrameSme = 0;
	int   m_AnimCnt = 0;		// Список анимаций SVOAnimHeader
    dword m_AnimSme = 0;
	int   m_MatrixCnt = 0;		// Список матриц SVOExpMatrixHeader
    dword m_MatrixSme = 0;
    dword m_EdgeCnt = 0;		// Список всех граней
    dword m_EdgeSme = 0;
};

struct SVOVertexNorTex {
    D3DXVECTOR3 v = { 0.0f, 0.0f, 0.0f };
	D3DXVECTOR3 n = { 0.0f, 0.0f, 0.0f };
	float tu = 0.0f, tv = 0.0f;
};

struct SVOMaterial {
    dword dummy = 0;
    float dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
    float ar = 0.0f, ag = 0.0f, ab = 0.0f, aa = 0.0f;
    float sr = 0.0f, sg = 0.0f, sb = 0.0f, sa = 0.0f;
    float er = 0.0f, eg = 0.0f, eb = 0.0f, ea = 0.0f;
	float power = 0.0f;
    wchar tex_diffuse[32] { L'\0' };
};

struct SVOGroup
{
    dword m_Material = 0;
    dword m_Flags = 0;			// 0-list
    dword m_VerCnt = 0;
    dword m_VerStart = 0;
    dword m_TriCnt = 0;			// Кол-во индексов
    dword m_TriStart = 0;
    dword dummy[2] = { 0, 0 };
};

struct SVOFrame {
    dword m_GroupIndexCnt = 0;
    dword m_GroupIndexSme = 0;	// Каждый индекс 4-байтный указатель
	float m_CenterX = 0.0f, m_CenterY = 0.0f, m_CenterZ = 0.0f;
	float m_RadiusCenter = 0.0f;
	float m_MinX = 0.0f, m_MinY = 0.0f, m_MinZ = 0.0f;
	float m_MaxX = 0.0f, m_MaxY = 0.0f, m_MaxZ = 0.0f;
	float m_RadiusBox = 0.0f;
    dword m_EdgeCnt = 0;
    dword m_EdgeStart = 0;
    dword dummy = 0;
};

struct SVOAnimHeader {
    dword m_Id = 0;
    wchar m_Name[32] { L'\0' };
    dword m_ModulesCount = 0;
    dword m_ModuleSme = 0;
    dword r1 = 0;
};

struct SVOMatrixHeader {
    dword m_Id = 0;
    wchar m_Name[32] { L'\0' };
    dword m_MatrixSme = 0; // Спиок SVOMatrix   (кол-во по количеству m_FrameCnt)
    dword r1 = 0;
    dword r2 = 0;
};

struct SVOFrameEdge {
    dword m_SideTri1 = 0; // 0xf0000000 - triangle side		0x0ff00000 - group   0x000fffff - triangle
    dword m_SideTri2 = 0; // 0xf0000000 - triangle side		0x0ff00000 - group   0x000fffff - triangle
};

struct SVOEdge {
	int m_Tri1 = 0;
	int m_Tri2 = 0;
	byte m_Edge1 = 0;
	byte m_Enge2 = 0;
};

struct SVOEdgeGroup {
	int m_EdgeSme = 0;
	int m_EdgeCnt = 0;
};

////////////////
//memory VO

#define VO_FVF (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)
struct SVOVertex
{
	D3DXVECTOR3 v = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3 n = { 0.0f, 0.0f, 0.0f };
    float       tu = 0.0f;
    float       tv = 0.0f;

    static const dword FVF = VO_FVF;
};


struct SVOTriangle
{
    union
    {
        struct { int i0, i1, i2; };
        int i[3] = { 0 };
    };

    //D3DXVECTOR3 norm = { 0.0f, 0.0f, 0.0f };
};

struct SVOIndices16
{
    word i0 = 0, i1 = 0, i2 = 0;
};

struct SVOIndices32
{
    dword i0 = 0, i1 = 0, i2 = 0;
};

struct SVOSurface
{
    const SSkin* skin = nullptr;
    CWStr texname = (CWStr)L"";

    //float dr = 0.0f, dg = 0.0f, db = 0.0f, da = 0.0f;
	//float ar = 0.0f, ag = 0.0f, ab = 0.0f, aa = 0.0f;
	//float sr = 0.0f, sg = 0.0f, sb = 0.0f, sa = 0.0f;
	//float er = 0.0f, eg = 0.0f, eb = 0.0f, ea = 0.0f;
	//float power = 0.0f;
};

struct SVOUnion
{
	int m_Surface = 0;
    int m_Base = 0;            //used for draw indexed primitives
    union
    {
	    int m_VerMinIndex = 0; // used for draw indexed primitives // always zero for optimized meshes (or negative, see m_IBase)
        int m_IBase;// = 0;    // negative value! 
    };
	int m_VerCnt = 0;          //used for draw indexed primitives
	int m_TriCnt = 0;
	int m_TriStart = 0;
};

struct SVOFrameModel
{
    D3DXVECTOR3 m_Min = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3 m_Max = { 0.0f, 0.0f, 0.0f };
    D3DXVECTOR3 m_GeoCenter = { 0.0f, 0.0f, 0.0f };
    float       m_Radius = 0.0f;

    int         m_UnionStart = 0;
    int         m_UnionCnt = 0;
    int         m_EdgeStart = 0;
    int         m_EdgeCnt = 0;

    int         m_VerCnt = 0;    // used in pick
    int         m_VerStart = 0;  // used in pick
};

struct SVOFrameRuntime
{
    int m_EdgeVertexIndexMin = 0;
    int m_EdgeVertexIndexCount = 0;
};

struct SVOAnimation
{
    dword m_Id = 0;
    wchar m_Name[32] { L'\0' };
    dword m_FramesCnt = 0;
    dword m_FramesStart = 0;
};

struct SVOMatrix
{
    dword m_Id = 0;
	wchar m_Name[32] { L'\0' };
	dword m_MatrixStart = 0; // Спиок D3DXMATRIX  (кол-во по количеству кадров)
};

struct SVOFrameIndex
{
    int frame = 0;
    int time = 0;
};

struct SVONormal
{
    union
    {
        struct { byte x, y, z, s; };
        dword all = 0;
    };
};

struct SVOFrameEdgeModel
{
    int         v00 = 0, v01 = 0; //Verts of triangles with common edge (disp in bytes)
    SVONormal   n0, n1;   //Normals
};

struct SVOGeometrySimple
{
    struct SVOGS_asis
    {
        int         m_TriCnt = 0;
        int         m_VertsCnt = 0;
        D3DXVECTOR3 m_Mins = { 0.0f, 0.0f, 0.0f };
        D3DXVECTOR3 m_Maxs = { 0.0f, 0.0f, 0.0f };
        D3DXVECTOR3 m_Center = { 0.0f, 0.0f, 0.0f };
        float       m_Radius = 0.0f;

    } m_AsIs;

    SVOTriangle* m_Tris = nullptr;
    D3DXVECTOR3* m_Verts = nullptr;
};

struct SVOGeometry
{
    int* m_VerticesIdx = nullptr; // idxs for frames
    
    SBigVBSource<SVOVertex> m_Vertices;
    //SVOVertex*     m_Vertices;
    //int            m_VerticesCnt;

    SVOTriangle* m_Triangles = nullptr;
    int          m_TrianglesCnt = 0;
    //union
    //{
    //    SVOIndices16* m_Idx16;
    //    SVOIndices32* m_Idx32;
    //};
    //int            m_IdxCnt;
    SBigIBSource     m_Idxs;
    //int            m_IdxStride;    // 2 or 4

    dword*           m_UnionsIdx = nullptr;    // idxs for frames
    SVOUnion*        m_Unions = nullptr;
    int              m_UnionsCnt = 0;

    SVOSurface*      m_Surfaces = nullptr;
    int              m_SurfacesCnt = 0;

    SVOFrameEdgeModel* m_Edges = nullptr;
    int              m_EdgesCnt = 0;

    SVOFrameIndex*   m_FramesIdx = nullptr;    // idx for animations
    SVOFrameModel*   m_Frames = nullptr;
    int              m_FramesCnt = 0;

    SVOFrameRuntime* m_FramesRuntime = nullptr;

    SVOAnimation*    m_Animations = nullptr;
    int              m_AnimationsCnt = 0;

    SVOMatrix*       m_Matrixs = nullptr;
    int              m_MatrixsCnt = 0;

    D3DXMATRIX*      m_AllMatrixs = nullptr;
    //SVONormal*     m_Normals;
};

typedef bool (*ENUM_VERTS_HANDLER)(const SVOVertex& v, dword data);

class CVectorObjectAnim;
class CVectorObject : public CCacheData
{
    static CBigVB<SVOVertex>* m_VB;
    static CBigIB* m_IB;

    SVOGeometry        m_Geometry;
    SVOGeometrySimple* m_GeometrySimple = nullptr;

    CBlockPar       m_Props;

    static          SVOHeader* Header(const CBuf& buf) { return (SVOHeader*)buf.Get(); }

    bool PickSimple(const D3DXMATRIX& ma, const D3DXMATRIX& ima, const D3DXVECTOR3& origin, const D3DXVECTOR3& dir, float* outt) const;

    SRemindCore m_RemindCore;

public:
    friend class CVOShadowStencil;

    SVOGeometrySimple* GetGS() { return m_GeometrySimple; }

    static void StaticInit()
    {
        m_VB = nullptr;
        m_IB = nullptr;
    }

	CVectorObject();
	virtual ~CVectorObject();

    static void DrawBegin() { m_IB->BeforeDraw(); m_VB->BeforeDraw(); ASSERT_DX(g_D3DD->SetFVF(VO_FVF)); }
    static void DrawEnd() {}

    static void MarkAllBuffersNoNeed() { if(m_VB) m_VB->ReleaseBuffers(); if(m_IB) m_IB->ReleaseBuffers(); }

    //bool DX_IsEmpty(void) { return !IS_VB(m_IB); }
    void DX_Prepare() { m_Geometry.m_Vertices.Prepare(m_VB); m_Geometry.m_Idxs.Prepare(m_IB); };
    void DX_Free() { m_Geometry.m_Vertices.MarkNoNeed(m_VB); m_Geometry.m_Idxs.MarkNoNeed(m_IB); }

    int GetFramesCnt() const                              { return m_Geometry.m_FramesCnt; }
    const SVOFrameModel* GetFrame(int no) const           { return m_Geometry.m_Frames + no; }

    const wchar* GetAnimName(int i)                       { return m_Geometry.m_Animations[i].m_Name; }

    int GetAnimCount() const                              { return m_Geometry.m_AnimationsCnt; }
	int GetAnimById(dword id);
	int GetAnimByName(const wchar* name);
    int GetAnimFramesCount(int no) const    		      { return m_Geometry.m_Animations[no].m_FramesCnt; }
    int GetAnimFrameTime(int anim, int frame) const       { return abs(m_Geometry.m_FramesIdx[m_Geometry.m_Animations[anim].m_FramesStart + frame].time); }
    int GetAnimLooped(int anim) const                     { return m_Geometry.m_FramesIdx[m_Geometry.m_Animations[anim].m_FramesStart].time > 0; }
    int GetAnimFrameIndex(int anim, int frame) const      { return m_Geometry.m_FramesIdx[m_Geometry.m_Animations[anim].m_FramesStart + frame].frame; }

    CWStr GetPropValue(const wchar* name) const           { return m_Props.ParGetNE(name); }

    const D3DXVECTOR3& GetFrameGeoCenter(int frame) const { return m_Geometry.m_Frames[frame].m_GeoCenter; }

    int GetMatrixCount() const                            { return m_Geometry.m_MatrixsCnt; }
	const D3DXMATRIX* GetMatrixById(int frame, dword id) const;
	const D3DXMATRIX* GetMatrixByName(int frame, const wchar* name) const;
    const D3DXMATRIX* GetMatrix(int frame, int matrix) const { return m_Geometry.m_AllMatrixs + (m_Geometry.m_Matrixs + matrix)->m_MatrixStart + frame; }
	const wchar* GetMatrixNameById(dword id) const;
    const wchar* GetMatrixName(int idx) const             { return m_Geometry.m_Matrixs[idx].m_Name; }
    dword GetMatrixId(int idx) const                      { return m_Geometry.m_Matrixs[idx].m_Id; }

    bool Pick(int noframe, const D3DXMATRIX& ma, const D3DXMATRIX& ima, const D3DXVECTOR3& origin, const D3DXVECTOR3& dir, float* outt) const;
    bool PickFull(int noframe, const D3DXMATRIX& ma, const D3DXMATRIX& ima, const D3DXVECTOR3& origin, const D3DXVECTOR3& dir, float* outt) const;

    void EnumFrameVerts(int noframe, ENUM_VERTS_HANDLER evh, dword data) const;

    const CWStr& GetSurfaceFileName(int i) const { return m_Geometry.m_Surfaces[i].texname; }

	//void EdgeClear();
	//void EdgeBuild();
    //bool EdgeExist() { return m_Edge != nullptr; }

    void GetBound(int noframe, const D3DXMATRIX& objma, D3DXVECTOR3& bmin, D3DXVECTOR3& bmax) const;

    void CalcShadowProjMatrix(int noframe, SProjData& pd, D3DXVECTOR3& dir, float addsize);
    static void CalcShadowProjMatrix(int cnt, CVectorObjectAnim** obj, const  int* noframe, const D3DXMATRIX* wm, SProjData& pd, D3DXVECTOR3& dir, float addsize);
    static CTextureManaged* CalcShadowTexture(int cnt, CVectorObjectAnim** obj, const int* noframe, const D3DXMATRIX* wm, const SProjData& pd, int texsize, CVOShadowCliper* cliper, CBaseTexture* tex_to_update = nullptr);
    static CTextureManaged* CalcShadowTextureWOMat(int cnt, CVectorObjectAnim** obj, const int* noframe, const D3DXMATRIX* wm, int texsize, CVOShadowCliper* cliper, CBaseTexture* tex_to_update = nullptr);

    void BeforeDraw();
    void Draw(int noframe, dword user_param, const SSkin* ds);

    void LoadSpecial(EObjectLoad flags, SKIN_GET sg, dword gsp);
    void PrepareSpecial(EObjectLoad flags, SKIN_GET sg, dword gsp) { if(!IsLoaded()) LoadSpecial(flags, sg, gsp); }

    bool IsNoSkin(int surf) { return m_Geometry.m_Surfaces[surf].skin == nullptr; }

    virtual bool IsLoaded() { return (m_Geometry.m_Vertices.verts != nullptr); }
	virtual void Unload();    
    virtual void Load() { LoadSpecial(OLF_NO_TEX, nullptr, 0); }
};

struct SSkin
{
    SKIN_SETUP_TEX     m_SetupTex = nullptr;
    SKIN_SETUP_STAGES  m_SetupStages = nullptr;
    SKIN_SETUP_CLEAR   m_SetupClear = nullptr;
    SKIN_SETUP_SHADOW  m_SetupTexShadow = nullptr;   // used only for shadow projecting
    SKIN_PRELOAD       m_Preload = nullptr;
};

struct SColorInterval
{
    dword color1 = 0;
    dword color2 = 0;
    int   time1 = 0;
    int   time2 = 0;
};

struct SLightData
{
    SColorInterval* intervals = nullptr;
    int             intervals_cnt = 0;
    int             time = 0;
    int             period = 0;
    dword           matrix_id = 0;

    byte            bbytes[sizeof(CSprite)] = { 0 };

    CSprite& BB() { return *(CSprite*)&bbytes; }

    void Release();  // just free internal data. not itself
};

class CVectorObjectAnim : public CMain
{
    CVectorObject* m_VO = nullptr;
    const SSkin* m_Skin = nullptr;

	int m_Time = 0;
	int m_TimeNext = 0;

	int m_AnimFrame = 0;
    int m_VOFrame = 0;
    int m_Anim = 0;

    bool m_AnimLooped = true;
    bool m_AnimReversed = false;

    SLightData* m_Lights = nullptr;
    int         m_LightsCount = 0;

    std::bitset<32> m_AnimationsAvailability = { 0 };

    void SetLoopStatus() { if(m_VO->GetAnimLooped(m_Anim)) m_AnimLooped = true; else m_AnimLooped = false; }

public:
    CVectorObjectAnim() = default;
	~CVectorObjectAnim();

	void Clear();

    CVectorObject* VO()					    { return m_VO; }
    const SSkin*   GetSkin() const          { return m_Skin; }
    void           SetSkin(const SSkin* s)  { m_Skin = s; }

    void InitVOAnim(CVectorObject* vo, CTextureManaged* tex_light, const SSkin* skin = nullptr);
    void InitVOAnimLights(CTextureManaged* tex_light);

    bool IsAnim(const wchar* name) { return !(wcscmp(VO()->GetAnimName(m_Anim), name)); }

    void SetAnimAvailability(int bit, bool set) { if(set) m_AnimationsAvailability.set(bit); else m_AnimationsAvailability.reset(bit); }
    bool CheckAnimAvailability(int bit)         { return m_AnimationsAvailability.test(bit); }

    CWStr GetAnimName() { return CWStr(VO()->GetAnimName(m_Anim)); }

    int  GetAnimIndex() const                               { return m_Anim; }
    void SetAnimDefault()                                   { m_Anim = 0; SetFirstFrame(); SetLoopStatus();}
    void SetAnimDefault(bool loop)                          { m_Anim = 0; SetFirstFrame(); m_AnimLooped = loop;}
    
    void SetAnimByIndex(int idx)                            { ASSERT(m_VO); m_Anim = idx; SetFirstFrame(); SetLoopStatus(); }
    void SetAnimByIndex(int idx, bool loop)                 { ASSERT(m_VO); m_Anim = idx; SetFirstFrame(); m_AnimLooped = loop; }

    void SetAnimById(dword id)                              { ASSERT(m_VO); m_Anim = VO()->GetAnimById(id); if(m_Anim < 0) m_Anim = 0; SetFirstFrame(); SetLoopStatus(); }
    void SetAnimById(dword id, bool loop)                   { ASSERT(m_VO); m_Anim = VO()->GetAnimById(id); if(m_Anim < 0) m_Anim = 0; SetFirstFrame(); m_AnimLooped = loop; }
    
    bool SetAnimByName(const wchar* name, signed char loop = ANIM_LOOP_DEFAULT, bool reverse = false);

    bool SetAnimByNameNoBegin(const wchar* name)            { ASSERT(m_VO); int i = VO()->GetAnimByName(name); if(i < 0) { return true; } SetLoopStatus(); if(m_Anim != i) { m_Anim = i;  SetFirstFrame(); } return false; }
    bool SetAnimByNameNoBegin(const wchar* name, bool loop) { ASSERT(m_VO); int i = VO()->GetAnimByName(name); if(i < 0) { return true; } m_AnimLooped = loop; if(m_Anim != i) { m_Anim = i;  SetFirstFrame(); } return false; }
    
    void SetAnimLooped(bool loop)                           { m_AnimLooped = loop; }
    bool IsAnimReversed()                                   { return m_AnimReversed; }
    void SetAnimReversed(bool set)                          { m_AnimReversed = set; }

    int GetVOFrame() const                                  { return m_VOFrame; }
    int GetFrame() const                                    { return m_AnimFrame; }

	//int FrameCnt()							            { return m_AnimCnt; }
	//int Frame()								            { return m_AnimFrame; }
	//void Frame(int zn)							        { m_AnimFrame = zn; if(m_AnimFrame < 0) m_AnimFrame = 0; else if(m_AnimFrame >= m_AnimCnt) m_AnimFrame = 0; }
    const int GetMatrixCount()                              { return m_VO->GetMatrixCount(); }
    const D3DXMATRIX* GetMatrixById(dword id) const             { ASSERT(m_VO); return m_VO->GetMatrixById(m_VOFrame, id); }
    const D3DXMATRIX* GetMatrixByName(const wchar* name) const  { ASSERT(m_VO); return m_VO->GetMatrixByName(m_VOFrame, name); }
    const D3DXMATRIX* GetMatrix(int no) const                   { ASSERT(m_VO); return m_VO->GetMatrix(m_VOFrame, no); }

    void CalcShadowProjMatrix(int no_frame, SProjData& pd, D3DXVECTOR3& dir, float add_size)
    {
        m_VO->CalcShadowProjMatrix(no_frame, pd, dir, add_size);
    }

    void EnumFrameVerts(ENUM_VERTS_HANDLER evh, dword data) const { m_VO->EnumFrameVerts(m_VOFrame, evh, data); };

	byte VectorObjectAnimTact(int cms, float speed_factor = 0.0f); //Возвращает FRAME_NOT_CHANGED, если фрейм анимации объекта не поменялся, FRAME_CHANGED, если анимация сменила фрейм и FRAMES_LOOP_DONE, если та сменила фрейм и на этом же фрейме последовательность кадров вернулась в начало цикла
    void UpdateLightSprites(int cms);
    dword NextFrame();
    dword PrevFrame();
    void SetFirstFrame();
    void SetLastFrame();
    bool IsAnimEnd(bool at_what_frame = AT_LAST_FRAME) const { return !m_AnimLooped && ((at_what_frame == AT_LAST_FRAME && m_AnimFrame == (m_VO->GetAnimFramesCount(m_Anim) - 1)) || (at_what_frame == AT_FIRST_FRAME && m_AnimFrame == 0)); }

    void GetBound(D3DXVECTOR3& bmin, D3DXVECTOR3& bmax) const;

    bool Pick(const D3DXMATRIX& ma, const D3DXMATRIX& ima, const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt) const;
    bool PickFull(const D3DXMATRIX& ma, const D3DXMATRIX& ima, const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt) const;

    void BeforeDraw();
    void Draw(dword user_param) { VO()->Draw(m_VOFrame, user_param, m_Skin); }
    void DrawLightSprites(bool now, const D3DXMATRIX& objma, const D3DXMATRIX* iview);
};


class CVectorObjectGroup;

class CVectorObjectGroupUnit : public CMain {
public:
	dword m_Flags = VOUF_MATRIX | VOUF_SHADOWSTENCIL; // Какой ресурс объекта изменился. При созданнии класса устанавливается в 0xffffffff

	CVectorObjectGroup* m_Parent = nullptr;
	CVectorObjectGroupUnit* m_Prev = nullptr;
	CVectorObjectGroupUnit* m_Next = nullptr;

	int m_Id = 0;

    CWStr m_Name = (CWStr)L"";

    CVectorObjectAnim* m_Obj = HNew(g_CacheHeap) CVectorObjectAnim();

	CVectorObjectGroupUnit* m_Link = nullptr;
	int m_LinkMatrixId = -1; // -1 - center, -2 - by name, >= 0 - by Id
	CWStr m_LinkMatrixName = (CWStr)L"";

	D3DXMATRIX m_Matrix;				// Дополнительная матрица позиционирования в локальных координатах.

	D3DXMATRIX m_MatrixWorld;			// Конечная матрица трансформации объекта в мировые координаты.
    D3DXMATRIX m_IMatrixWorld;

	CVOShadowStencil* m_ShadowStencil = nullptr;

public:
    CVectorObjectGroupUnit()
    {
        D3DXMatrixIdentity(&m_Matrix);
        D3DXMatrixIdentity(&m_MatrixWorld);
    }
    ~CVectorObjectGroupUnit();
        
	void GetResources(dword need);	//Запрашиваем необходимые ресурсы объекта
    void RChange(dword zn) { m_Flags |= VOUF_RES & zn; } //Меняем указанные ресурсы объекта

	void ShadowStencilOn(bool zn = true);
};

class CVectorObjectGroup : public CMain {
public:
	CVectorObjectGroupUnit* m_First = nullptr;
	CVectorObjectGroupUnit* m_Last = nullptr;

	D3DXMATRIX* m_GroupToWorldMatrix = nullptr;	//Матрица позиционирования группы в мировом пространстве.

	CWStr m_Name = (CWStr)L"";				    //Имя файла

    D3DXVECTOR3 m_ShadowStencilLight = { 0.0f, 0.0f, -1.0f };
	//D3DXPLANE m_ShadowStencilCutPlane = { 0.0f, 0.0f, 1.0f, 0.0f };
    float       m_GroundZ = 0.0f; // for stencil shadow len

public:
    CVectorObjectGroup() = default;
    ~CVectorObjectGroup() { Clear(); }

	void Clear() { while(m_First) Delete(m_Last); }

    void Delete(CVectorObjectGroupUnit* un);
    CVectorObjectGroupUnit* Add();
    CVectorObjectGroupUnit* GetByName(const wchar* name);
    CVectorObjectGroupUnit* GetByNameNE(const wchar* name);
    CVectorObjectGroupUnit* GetById(int id);
    CVectorObjectGroupUnit* GetByIdNE(int id);

    D3DXVECTOR3 GetPosByName(const wchar* name) const;

    //void ChangeSetupFunction(OBJECT_SETUP_TEX setup_tex, OBJECT_SETUP_STAGES setup_stages);
	void GetResources(dword need);		// Запрашиваем нужные ресурсы объекта
	void RChange(dword zn);				// Указываем какие ресурсы изменились
	void RChangeByLink(CVectorObjectGroupUnit* link, dword zn);

    bool IsAlreadyLoaded() { return m_First != nullptr; }

	bool Tact(int cms); //Крутит анимацию на всех моделях, входящих в группу (используется для зданий), возвращая true при каждой смене кадра
    byte OccasionalTact(int cms); //Аналогично крутит анимацию на всех моделях, однако возвращает false, как только очередной луп анимации завершился, и кадр дошёл до начала цикла

	CVectorObjectGroupUnit* Pick(const D3DXVECTOR3& orig, const D3DXVECTOR3& dir, float* outt) const;

    void EnumFrameVerts_transform_position(ENUM_VERTS_HANDLER evh, dword data) const;

	void BoundGet(D3DXVECTOR3& bmin, D3DXVECTOR3& bmax);
	void BoundGetAllFrame(D3DXVECTOR3& bmin, D3DXVECTOR3& bmax);

	void ShadowStencilOn(bool zn = true);
        
    void ShadowStencil_DX_Free();

	void Load(const wchar* filename, CTextureManaged* lt, SKIN_GET sg, dword gsp);

    void BeforeDraw(bool proceed_shadows);
	void Draw(dword user_param);
    void DrawLightSprites(bool now = false, const D3DXMATRIX* i_view = nullptr);
	void ShadowStencilDraw();
};