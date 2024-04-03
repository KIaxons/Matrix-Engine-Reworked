// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

#include "matrixwater.hpp"

//#define TEXTURES_OFF

//#define LANDSCAPE_BOTTOM_USE_NORMALES
#define LANDSCAPE_TOP_USE_NORMALES
#define LANDSCAPE_SURF_USE_NORMALES

struct SPreInshorewave
{
    D3DXVECTOR2 pos = { 0.0f, 0.0f };
    D3DXVECTOR2 dir = { 0.0f, 0.0f };
    bool        used = false;
};


#ifdef LANDSCAPE_BOTTOM_USE_NORMALES
#define MATRIX_MAP_BOTTOM_VERTEX_FORMAT (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX2)
#else
#define MATRIX_MAP_BOTTOM_VERTEX_FORMAT (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2)
#endif

struct Svtc
{
    float u = 0.0f;
    float v = 0.0f;
};

struct SMatrixMapVertexBottomInt
{
    D3DXVECTOR3 v = { 0.0f, 0.0f, 0.0f };
#ifdef LANDSCAPE_BOTTOM_USE_NORMALES
    D3DXVECTOR3 n = { 0.0f, 0.0f, 0.0f };
#endif
	dword defcol = 0;
};

struct SMatrixMapVertexBottom
{
    SMatrixMapVertexBottomInt ivd;
    Svtc                tc[2];

    static const dword FVF = MATRIX_MAP_BOTTOM_VERTEX_FORMAT;
};

struct SMapZVertex
{
    D3DXVECTOR3 v = { 0.0f, 0.0f, 0.0f };
    float       weight = 0.0f;

    static const dword FVF = D3DFVF_XYZB1;
};

class CMatrixMapStatic;
class CMatrixMapTextureFinal;
class CMatrixMapGroup;
class CTerSurface;
class CMatrixShadowProj;


//#define GRPFLAG_VISIBLE   SETBIT(0)
#define GRPFLAG_HASWATER    SETBIT(1)
//#define GRPFLAG_HASFLYER  SETBIT(2)
#define GRPFLAG_HASBASE     SETBIT(3)

struct SBottomGeometry
{
    int idxbase = 0;   // index base
    int tricnt = 0;    // primitives count
    int vertcnt = 0;   // verts count
    int minvidx = 0;   // min vertex index
    int texture = 0;   // texture index
};

struct SObjectCore;

class CMatrixMapGroup : public CMain
{
    SRemindCore m_RemindCore; // must be at begining of class!!!!!!!!

    //static CBigVB<SMapZVertex>*          m_BigVB_Z;
    static CBigVB<SMatrixMapVertexBottom>* m_BigVB_bottom;
    static CBigIB* m_BigIB_bottom;
    //SBigVBSource<SMapZVertex>            m_VertsSource_Z;

    SBigVBSource<SMatrixMapVertexBottom> m_VertsSource_bottom;
    SBigIBSource     m_IdxsSource_bottom;
    SBottomGeometry* m_BottomGeometry = nullptr;
    int              m_BottomGeometryCount = 0;

    int         m_PosX = 0;
    int         m_PosY = 0;
    D3DXVECTOR2 p0 = { 0.0f, 0.0f };
    D3DXVECTOR2 p1 = { 0.0f, 0.0f };
    D3DXMATRIX  m_Matrix = {};

    float m_maxz = 0.0f; // maximum height
    float m_minz = 0.0f; // minimum height
    float m_maxz_obj = 0.0f;        // with objects (except robots and flyers)
    float m_maxz_obj_robots = 0.0f; // same as m_maxz_obj, but with robots

    dword m_Flags = 0;

    CTextureManaged*   m_WaterAlpha = nullptr;

    SInshorewave*      m_Inshorewaves = nullptr;
    int                m_InshorewavesCnt = 0;
    int                m_InshoreTime = 0;
    SPreInshorewave*   m_PreInshorewaves = nullptr;
    int                m_PreInshorewavesCnt = 0;

    CMatrixMapStatic** m_Objects = nullptr;
    int                m_ObjectsAllocated = 0;
    int                m_ObjectsContained = 0;

    CTerSurface**      m_Surfaces = nullptr;
    int                m_SurfacesCnt = 0;

    CMatrixShadowProj** m_Shadows = nullptr;
    int                m_ShadowsCnt = 0;

    float              m_CamDistSq = 0.0f; // квадрат растояния до камеры. вычисляется только в графическом такте.

    SObjectCore*       m_RenderShadowObject = nullptr;


    D3DXVECTOR3* m_VertsTrace = nullptr;
    int*         m_IdxsTrace = nullptr;
    int          m_IdxsTraceCnt = 0;
    int          m_VertsTraceCnt = 0;

public:

#ifdef _DEBUG
        static int m_DPCalls;
#endif

    CMatrixMapGroup();
	~CMatrixMapGroup();

    static void StaticInit()
    {
        //m_BigVB_Z = nullptr;
        m_BigVB_bottom = nullptr;
        m_BigIB_bottom = nullptr;
    }

    CMatrixMapStatic* ReturnObject(int i) const;
    int ObjectsCnt() const;

    //bool IsVisible(void) const { return FLAG(m_Flags, GRPFLAG_VISIBLE); }
    //void SetVisible(bool vis) { INITFLAG(m_Flags, GRPFLAG_VISIBLE, vis); }

    bool HasWater() const { return FLAG(m_Flags, GRPFLAG_HASWATER); }
    //bool HasFlyer() const { return FLAG(m_Flags, GRPFLAG_HASFLYER); }
    CTextureManaged* GetWaterAlpha() { return m_WaterAlpha; }

    bool IsBaseOn() const
    {
        return FLAG(m_Flags, GRPFLAG_HASBASE);
    }

    const D3DXVECTOR2& GetPos0() const { return p0; }
    const D3DXVECTOR2& GetPos1() const { return p1; }

    float GetMinZ() const           { return m_minz; }
    float GetMaxZObj() const        { return m_maxz_obj; }
    float GetMaxZObjRobots() const  { return m_maxz_obj_robots; }
    float GetMaxZLand() const       { return m_maxz; }

    void  RecalcMaxZ();
    void  AddNewZObj(float z)       { if(z > m_maxz_obj) { m_maxz_obj = z; } if(m_maxz_obj_robots < m_maxz_obj) { m_maxz_obj_robots = m_maxz_obj; } }
    void  AddNewZObjRobots(float z) { if(z > m_maxz_obj_robots) m_maxz_obj_robots = z; }

    bool  IsInFrustum() const;
    inline bool IsPointIn(const D3DXVECTOR2& p)
    {
        return (p.x >= p0.x) && (p.x < p1.x) && (p.y >= p0.y) && (p.y < p1.y);
    }

    void AddObject(CMatrixMapStatic* obj);
    void SubObject(CMatrixMapStatic* obj);

    CMatrixMapStatic* FindObjectAny(dword mask, const D3DXVECTOR3& pos, float maxdist, float scale_radius, int& i);
    CMatrixMapStatic* FindObjectAny(dword mask, const D3DXVECTOR2& pos, float maxdist, float scale_radius, int& i);

    void Clear();

    void BuildBottom(int x, int y, byte* rawbottom);
    void BuildWater(int x, int y);
    void InitInshoreWaves(int n, const float* xx, const float* yy, const float *nxx, const float* nyy);

    void RemoveShadow(const CMatrixShadowProj* s);
    void AddShadow(CMatrixShadowProj* s);
    void DrawShadowProj();

    void AddSurface(CTerSurface* surf);

    static void MarkAllBuffersNoNeed();

    static void BeforeDrawAll()
    {
        m_BigVB_bottom->BeforeDraw();
        m_BigIB_bottom->BeforeDraw();
    };
    void BeforeDraw();
	void Draw();
    //void DrawZ(void);

    void DX_Free()
    {
        m_VertsSource_bottom.MarkNoNeed(m_BigVB_bottom);
        m_IdxsSource_bottom.MarkNoNeed(m_BigIB_bottom);
    }
    void DX_Prepare()
    {
        m_VertsSource_bottom.Prepare(m_BigVB_bottom);
        m_IdxsSource_bottom.Prepare(m_BigIB_bottom);
    };
    void BeforeDrawSurfaces();

    void DrawInshorewaves();
    void SortObjects(const D3DXMATRIX& sort);
    void GraphicTact(int step);
    void PauseTact(int step);
        
    bool Pick(const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, float& t);

#ifdef _DEBUG
    void DrawBBox();
#endif
};