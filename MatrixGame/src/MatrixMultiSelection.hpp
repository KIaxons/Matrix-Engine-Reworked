// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#ifndef MATRIX_MULTI_SELECTION
#define MATRIX_MULTI_SELECTION

#include "StringConstants.hpp"

class CMatrixMapStatic;

typedef void (*SELECT_ENUM)(CMatrixMapStatic* ms, dword param);

#define MS_DIP_TIME 50

#define MS_FLAG_DIP         SETBIT(0)
#define MS_FLAG_ROBOTS      SETBIT(1)
#define MS_FLAG_TURRET      SETBIT(2)
#define MS_FLAG_BUILDING    SETBIT(3)

#define MULTISEL_FVF (D3DFVF_XYZRHW | D3DFVF_TEX1)
struct SMultiSelVertex
{
    D3DXVECTOR4 p = { 0.0f, 0.0f, 0.0f, 0.0f };
    float       u = 0.0f, v = 0.0f;
};


class CMultiSelection : public CMain
{
    static CMultiSelection* m_First;
    static CMultiSelection* m_Last;
    CMultiSelection* m_Next = nullptr;
    CMultiSelection* m_Prev = nullptr;

    Base::CPoint m_LT = { 0, 0 };
    Base::CPoint m_RB = { 0, 0 };

    dword m_Flags = 0;

    int m_TimeBeforeDip = 0;

    CBuf m_SelItems;

    static int m_Time;

    //CTextureManaged* m_Tex = nullptr;

    CMultiSelection(const Base::CPoint& pos);
    ~CMultiSelection()
    {
        if(CMultiSelection::m_GameSelection == this) CMultiSelection::m_GameSelection = nullptr;
        LIST_DEL(this, m_First, m_Last, m_Prev, m_Next);
    };

    void Draw();

    bool DrawPass1();
    void DrawPass2();
    void DrawPassEnd();
    
    void RemoveSelItems()
    {
        RESETFLAG(m_Flags, MS_FLAG_ROBOTS);
        RESETFLAG(m_Flags, MS_FLAG_TURRET);
        RESETFLAG(m_Flags, MS_FLAG_BUILDING);

        m_SelItems.Clear();
    }

public:

    static CMultiSelection* m_GameSelection;

    static void StaticInit()
    {
        m_First = nullptr;
        m_Last = nullptr;
        m_Time = 0;
        m_GameSelection = nullptr;
        //m_FirstItem = nullptr;
        //m_LastItem = nullptr;
    }

    static bool DrawAllPass1Begin()
    {
        bool ret = false;
        CMultiSelection* f = m_First;
        while(f)
        {
            ret |= f->DrawPass1();
            f = f->m_Next;
        }
        return ret;
    }

    static void DrawAllPass2Begin()
    {
        CMultiSelection* f = m_First;
        while(f)
        {
            f->DrawPass2();
            f = f->m_Next;
        }
    }

    static void DrawAllPassEnd()
    {
        CMultiSelection* f = m_First;
        while(f)
        {
            f->DrawPassEnd();
            f = f->m_Next;
        }
    }

    static void DrawAll()
    {
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));
        g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

        CMultiSelection* f = m_First;
        while(f)
        {
            // this izvrat required because Draw method can delete object itself
            // lol xdd, chetka zakodili, chtob videlenie videliyemie objecti udalialo (Klaxons)
            CMultiSelection* next = f->m_Next;
            f->Draw();
            f = next;
        }

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));
        g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    }
    
    static CMultiSelection* Begin(const Base::CPoint& pos);

    void Update(const Base::CPoint& pos) { m_RB = pos; }
    void Update(const Base::CPoint& pos, dword mask, SELECT_ENUM callback, dword param);
    void End(bool add_to_selection = true);


    static void AddTime(int ms) { m_Time += ms; }

    bool FindItem(const CMatrixMapStatic* o);
    void Remove(const CMatrixMapStatic* o);
};

#endif