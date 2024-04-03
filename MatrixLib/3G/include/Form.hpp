// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

enum ButtonStatus
{
    B_DOWN,
    B_UP,
    B_DOUBLE,

    B_WHEEL
};

enum ESysEvent
{
    SYSEV_DEACTIVATING,
    SYSEV_ACTIVATED,
};

class CForm : public CMain {
private:
	CForm* m_FormPrev = nullptr;
	CForm* m_FormNext = nullptr;

protected:
	CWStr m_Name = CWStr(L"");		// Нужно всегда задавать имя для дочерних классов

public:
    static void StaticInit();

	CForm();
	~CForm();

	virtual void Enter() = 0;
	virtual void Leave() = 0;
	virtual void Draw() = 0;

	virtual void Tact(int) = 0;

	virtual void MouseMove(int, int) = 0;
	virtual void MouseKey(ButtonStatus, int, int, int) = 0;

    virtual void Keyboard(bool, int) = 0;

    virtual void SystemEvent(ESysEvent se) = 0;
};

extern CForm* g_FormFirst;
extern CForm* g_FormLast;
extern CForm* g_FormCur;

void FormChange(CForm* form);