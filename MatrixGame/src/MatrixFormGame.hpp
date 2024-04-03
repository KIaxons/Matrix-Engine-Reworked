// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#pragma once

#define MAX_SCANS			  16
#define MOUSE_BORDER		  4
#define DOUBLESCAN_TIME_DELTA 200

struct SKeyScan
{
    dword time = 0;
    dword scan = 0;
};

class CFormMatrixGame : public CForm
{
private:
	float m_LastWorldX = 0.0f;
	float m_LastWorldY = 0.0f;
	int m_Action = 0;

	SKeyScan m_LastScans[MAX_SCANS];

public:
	CFormMatrixGame()
	{
		m_Name = L"MatrixGame";
	}
	~CFormMatrixGame() = default;

	virtual void Enter();
	virtual void Leave();
	virtual void Draw();

	virtual void Tact(int step);

	virtual void MouseMove(int x, int y);
	virtual void MouseKey(ButtonStatus status, int key, int x, int y);

	virtual void Keyboard(bool down, int scan);
	virtual void SystemEvent(ESysEvent se);
};