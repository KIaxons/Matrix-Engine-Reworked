// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#ifndef MATRIX_SKIN_MANAGER
#define MATRIX_SKIN_MANAGER

enum GSParam
{
    GSP_SIDE,
    GSP_ORDINAL,
    GSP_SIDE_NOALPHA,

    GSP_COUNT
};

struct SMatrixSkin : public SSkin
{
    CTextureManaged* m_Tex = nullptr;     // if nullptr, default texturing
    CTextureManaged* m_TexGloss = nullptr;
    CTextureManaged* m_TexMask = nullptr;
    CTextureManaged* m_TexBack = nullptr;
    float m_tu = 0.0f, m_tv = 0.0f, m_dtu = 0.0f, m_dtv = 0.0f;

    bool operator == (const SMatrixSkin& sk) const
    {
        return  m_Tex == sk.m_Tex &&
                m_TexGloss == sk.m_TexGloss &&
                m_TexMask == sk.m_TexMask &&
                m_TexBack == sk.m_TexBack &&
                m_dtu == sk.m_dtu &&
                m_dtv == sk.m_dtv; // &&
                //m_SetupClear == sk.m_SetupClear &&
                //m_SetupStages == sk.m_SetupStages &&
                //m_SetupTex == sk.m_SetupTex &&
                //m_SetupTexShadow == sk.m_SetupTexShadow;
    }

    void Prepare(GSParam gsp);

    void Prepare6Side();
    void Prepare6SideNA();
    void Prepare6Ordinal();

    void Prepare4Side();
    void Prepare4SideNA();
    void Prepare4Ordinal();

    void Prepare3Side();
    void Prepare3SideNA();
    void Prepare3Ordinal();

    void Prepare2Side();
    void Prepare2SideNA();
    void Prepare2Ordinal();
};


class CSkinManager : public CMain
{
    static SMatrixSkin** m_Skins[GSP_COUNT];
    static int           m_SkinsCount[GSP_COUNT];

public:
    static void StaticInit()
    {
        m_Skins[GSP_SIDE] = nullptr;
        m_SkinsCount[GSP_SIDE] = 0;
        m_Skins[GSP_SIDE_NOALPHA] = nullptr;
        m_SkinsCount[GSP_SIDE_NOALPHA] = 0;
        m_Skins[GSP_ORDINAL] = nullptr;
        m_SkinsCount[GSP_ORDINAL] = 0;
    }

    static const SSkin* GetSkin(const wchar* tex, dword param);

    static void Clear();
    static void Tact(float cms);
};

#endif