// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#ifndef MATRIX_TER_SURFACE_INCLUDE
#define MATRIX_TER_SURFACE_INCLUDE

#define SURFF_MACRO     SETBIT(0)
#define SURFF_GLOSS     SETBIT(1)
#define SURFF_WHITE     SETBIT(2)

#define SURFF_WRAPY     SETBIT(3)

#define SURF_TYPES_COUNT    8

#define SURF_TYPE                       (0)
#define SURF_TYPE_MACRO                 (SURFF_MACRO)
#define SURF_TYPE_GLOSS                 (SURFF_GLOSS)
#define SURF_TYPE_MACRO_GLOSS           (SURFF_MACRO | SURFF_GLOSS)
#define SURF_TYPE_WHITE                 (SURFF_WHITE)
#define SURF_TYPE_MACRO_WHITE           (SURFF_MACRO | SURFF_WHITE)
#define SURF_TYPE_GLOSS_WHITE           (SURFF_GLOSS | SURFF_WHITE)
#define SURF_TYPE_MACRO_GLOSS_WHITE     (SURFF_MACRO | SURFF_GLOSS | SURFF_WHITE)
#define SURF_FLAG_MASK                  (SURFF_MACRO | SURFF_GLOSS | SURFF_WHITE)

class CTerSurface : public Base::CMain
{
    struct STerSurfVertex
    {
        D3DXVECTOR3 p = { 0.0f, 0.0f, 0.0f };
        D3DXVECTOR3 n = { 0.0f, 0.0f, 0.0f };
        dword       color = 0;
        float       tu = 0.0f;
        float       tv = 0.0f;

        static const dword FVF = (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    };
    struct STerSurfVertexM
    {
        D3DXVECTOR3 p = { 0.0f, 0.0f, 0.0f };
        D3DXVECTOR3 n = { 0.0f, 0.0f, 0.0f };
        dword       color = 0;
        float       tu = 0.0f;
        float       tv = 0.0f;
        float       tum = 0.0f;
        float       tvm = 0.0f;

        static const dword FVF = (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX2);
    };

    dword            m_Flags = 0;
    dword            m_Color = 0;
    CTextureManaged* m_Tex = nullptr;
    CTextureManaged* m_TexGloss = nullptr;

    int              m_DrawFrameMarker = -1;
    int              m_Index = 0; // draw order


    static CBigIB*                  m_BigIB;
    static CBigVB<STerSurfVertex>*  m_BigVB;
    static CBigVB<STerSurfVertexM>* m_BigVBM;

    union
    {
        struct { SBigVBSource<STerSurfVertex>  m_VertsSource; };
        struct { SBigVBSource<STerSurfVertexM> m_VertsSourceM; };
    };
    SBigIBSource m_IdxsSource;

    static CTerSurface*  m_Surfaces;
    static int           m_SurfacesCnt;

    static CTerSurface** m_SurfacesDraw;
    static int           m_SurfaceLeft;
    static int           m_SurfaceRite;

    float m_DispX = 0.0f;
    float m_DispY = 0.0f;

    // private draw! use DrawAll. its draw all surfaces BeforeDraw was called.
    void Draw();

public:
    static void StaticInit()
    {
        m_BigIB = nullptr;
        m_BigVB = nullptr;
        m_BigVBM = nullptr;
        m_Surfaces = nullptr;
        m_SurfacesCnt = 0;
    }

//#ifdef _DEBUG
//
//    static int GetSufracesCount()         { return m_SurfacesCnt; }
//    static CTerSurface* GetSufrace(int i) { return m_Surfaces + i; }
//    dword unstable = 0;
//
//#endif

    static bool IsSurfacesPresent() { return m_SurfacesCnt != 0; };
    static void AllocSurfaces(int n);
    //static void PrepareSurfaces(bool macro);
    static void ClearSurfaces();

    static void Load(int i, byte* raw)  { m_Surfaces[i].Load(raw); };
    static void LoadM(int i, byte* raw) { m_Surfaces[i].LoadM(raw); };

    static void BeforeDrawAll()
    {
        if(m_BigVB) m_BigVB->BeforeDraw();
        if(m_BigVBM) m_BigVBM->BeforeDraw();
        m_BigIB->BeforeDraw();

        m_SurfaceLeft = m_SurfacesCnt >> 1;
        m_SurfaceRite = m_SurfacesCnt >> 1;
    }

    static void MarkAllBuffersNoNeed()
    {
        if(m_BigVB) m_BigVB->ReleaseBuffers();
        if(m_BigVBM) m_BigVBM->ReleaseBuffers();
        if(m_BigIB) m_BigIB->ReleaseBuffers();
    }

    static void DrawAll();
    static void SetFVF();
    static void SetFVFM();

    CTerSurface() {};
    ~CTerSurface();

    void BeforeDraw();

    void Load(byte* raw);
    void LoadM(byte* raw);
};

#endif