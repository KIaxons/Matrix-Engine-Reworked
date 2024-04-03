// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

// MatrixGame.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "MatrixGame.h"
#include "MatrixMap.hpp"
#include "MatrixFormGame.hpp"
#include "Interface/CInterface.h"
#include "MatrixRenderPipeline.hpp"
#include "MatrixGameDll.hpp"
#include "ShadowStencil.hpp"
#include "MatrixLoadProgress.hpp"
#include "MatrixFlyer.hpp"
#include "MatrixMapStatic.hpp"
#include "MatrixMultiSelection.hpp"
#include "MatrixTerSurface.hpp"
#include "MatrixSkinManager.hpp"
#include "MatrixSoundManager.hpp"
#include "Interface/CIFaceMenu.h"
#include "Interface/MatrixHint.hpp"
#include "Interface/CHistory.h"
#include "MatrixInstantDraw.hpp"
#include "MatrixSampleStateManager.hpp"

#include <stdio.h>
#include <time.h>

#include <ddraw.h>

////////////////////////////////////////////////////////////////////////////////
CHeap* Base::g_MatrixHeap;
CBlockPar* Base::g_MatrixData;
CMatrixMapLogic* g_MatrixMap;
CRenderPipeline* g_Render;
CLoadProgress* g_LoadProgress;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE ,
                     LPTSTR    ,
                     int       )
{
    //CBitmap b, bb;
    //b.LoadFromPNG(L"test.png");
    //bb.CreateRGB(b.SizeX(), b.SizeY());
    //bb.Copy(CPoint(0, 0), b.Size(), b, CPoint(0, 0));
    //bb.SaveInDDSUncompressed(L"out.dds");

    //CBuf bla;
    //bla.LoadFromFile(L"test.dds");
    //byte* data = (byte*)bla.Get();
    //DDSURFACEDESC2 *desc = (DDSURFACEDESC2*)(data + 4);

    //int x = FP_NORM_TO_BYTE2(1.0f);
    //x = FP_NORM_TO_BYTE2(0.9999f);
    //x = FP_NORM_TO_BYTE2(0.5f);
    //x = FP_NORM_TO_BYTE2(0.1f);
    //x = FP_NORM_TO_BYTE2(0.0f);
    //x = FP_NORM_TO_BYTE2(2.0f);
    //x = FP_NORM_TO_BYTE2(200.6f);
    //x = FP_NORM_TO_BYTE2(255.6f);
    //x = FP_NORM_TO_BYTE2(256.6f);

    //float t, k;
    //k = 10.0f; FP_INV(t, k);
    //k = 20.0f; FP_INV(t, k);
    //k = 25.0f; FP_INV(t, k);

    const wchar* cmd = GetCommandLineW();

    int numarg;
    wchar** args = CommandLineToArgvW(cmd, &numarg);
    wchar* map = nullptr;

    if(numarg > 1) map = args[1];

	try
    {
        srand((unsigned)time(nullptr));
        MatrixGameInit(hInstance, nullptr, map);
        CFormMatrixGame* formgame = HNew(nullptr) CFormMatrixGame();
        FormChange(formgame);

		timeBeginPeriod(1);

        //dword* buf = (dword*)HAlloc(124, nullptr);
        //*(buf - 1) = 1;
        //HFree(buf, nullptr);

        if(map)
        {
            FILE* file;
            file = fopen("calcvis.log", "a");
            CStr name(g_MatrixMap->MapName(), Base::g_MatrixHeap);
            fwrite(name.Get(), name.Len(), 1, file);
            fwrite(" ...", 4, 1, file);
            fclose(file);

            g_MatrixMap->CalcVis();

            file = fopen("calcvis.log", "a");
            fwrite("done\n", 5, 1, file);
            fclose(file);
        }
        else L3GRun();

		timeEndPeriod(1);

		MatrixGameDeinit();

        FormChange(nullptr);
        HDelete(CFormMatrixGame, formgame, nullptr);

        g_Cache->Clear();
        L3GDeinit();
		CacheDeinit();

        CMain::BaseDeInit();
    }
    catch(CException* ex)
    {
        ClipCursor(nullptr);
#ifdef ENABLE_HISTORY
        CDebugTracer::SaveHistory();
#endif
        g_Cache->Clear();
		L3GDeinit();
        
        SFT(CStr("Exception:") + CStr(ex->Info()).Get());
		MessageBox(nullptr, CStr(ex->Info()).Get(), "Exception:", MB_OK);

		delete ex;
	}
    catch(...)
    {
#ifdef ENABLE_HISTORY
        CDebugTracer::SaveHistory();
#endif
        SFT("Exception: Unknown bug :(");
        MessageBox(nullptr, "Unknown bug :(", "Exception:", MB_OK);
    }
    
    ClipCursor(nullptr);

	return 1;
}

static void static_init()
{
    // Base
    CMain::BaseInit();

    // 3G
    CCacheData::StaticInit();
    CVectorObject::StaticInit();
    CVOShadowProj::StaticInit();
    CVOShadowStencil::StaticInit();
    CSprite::StaticInit();
    CSpriteSequence::StaticInit();
    CForm::StaticInit();

    // Game
    CMatrixProgressBar::StaticInit();
    CMatrixMapStatic::StaticInit();
    CMatrixMapObject::StaticInit();
    CMatrixFlyer::StaticInit();
    CMatrixMapGroup::StaticInit();
    CInterface::StaticInit();
    CMultiSelection::StaticInit();
    CTerSurface::StaticInit();
    CBottomTextureUnion::StaticInit();
    CSkinManager::StaticInit();
    CMatrixHint::StaticInit();
    CInstDraw::StaticInit();
    SInshorewave::StaticInit();

    g_Flags = 0; //GFLAG_FORMACCESS;
}

void LoadModList(CWStr& modlist)
{
    CWStr modcfg_name(Base::g_MatrixHeap);
    CBlockPar modcfg(Base::g_MatrixHeap);
    
    if(CFile::FileExist(modcfg_name, L"Mods\\ModCFG.txt"))
    {
        modcfg.LoadFromTextFile(L"Mods\\ModCFG.txt");
        modlist.Set(modcfg.ParGet(L"CurrentMod"));
    }
    else modlist.Set(L"");
}

void LoadCfgFromMods(CWStr& modlist, CBlockPar& base_bp, const wchar* lang, const wchar* bp_name)
{
    CWStr curmod(Base::g_MatrixHeap);
    CWStr moddata_name(Base::g_MatrixHeap);

    CBlockPar moddata(Base::g_MatrixHeap);

    for(int i = -1; i < modlist.GetCountPar(L","); ++i)
    {
        //Мод PBEngine заносится в список вне очереди
        if(i < 0) curmod = L"PlanetaryBattles\\PBEngine";
        else
        {
            curmod = modlist.GetStrPar(i, L",");
            curmod.Trim();

            if(curmod == L"PlanetaryBattles\\PBEngine") continue;
        }

        moddata_name.Clear();
        moddata_name += L"Mods\\";
        moddata_name += curmod;
        moddata_name += L"\\";
        moddata_name += FILE_CONFIGURATION_LOCATION;

        if(lang != nullptr)
        {
            moddata_name += lang;
            moddata_name += L"\\";
        }
        moddata_name += L"Robots\\";
        moddata_name += bp_name;

        if(CFile::FileExist(moddata_name, moddata_name.Get()))
        {
            moddata.LoadFromTextFile(moddata_name);
            base_bp.BlockMerge(moddata);
        }
    }
}

void MatrixGameInit
(
    HINSTANCE inst,       //???
    HWND wnd,             //???
    wchar* map,           //Путь к файлу карты
    SRobotsSettings* set, //???
    wchar* lang,          //Язык, Rus или Eng, используется при построении путей к папкам с языковыми файлами
    wchar* txt_start,     //Вступительный текст
    wchar* txt_win,       //Текст при победе
    wchar* txt_loss,      //Текст при поражении
    wchar* planet         //Название планеты
)
{
    static_init();

    Base::g_MatrixHeap = HNew(nullptr) CHeap;

    CFile::AddPackFile(L"DATA\\robots.pkg", nullptr);
    CFile::OpenPackFiles();

    CWStr modlist(Base::g_MatrixHeap);
    LoadModList(modlist);

    CLoadProgress     lp;
    g_LoadProgress = &lp;

    CStorage    stor_cfg(Base::g_MatrixHeap);
    bool        stor_cfg_present = false;
    CWStr       stor_cfg_name(Base::g_MatrixHeap);
	wchar       conf_file[80];
	wcscpy(conf_file, FILE_CONFIGURATION_LOCATION);

	if(lang != nullptr)
    {
	    wcscat(conf_file, lang);
	    wcscat(conf_file, L"\\");
    }

	wcscat(conf_file, FILE_CONFIGURATION);

    if(CFile::FileExist(stor_cfg_name, conf_file))
    {
        stor_cfg.Load(conf_file);
        stor_cfg_present = true;
    }

    Base::g_MatrixData = HNew(Base::g_MatrixHeap) CBlockPar(1, Base::g_MatrixHeap);
    if(stor_cfg_present)
    {
        stor_cfg.RestoreBlockPar(L"da", *Base::g_MatrixData);
		//stor_cfg.RestoreBlockPar(L"if", *g_MatrixData);
        //g_MatrixData->SaveInTextFile(L"bbb.txt");

        if(CFile::FileExist(stor_cfg_name, L"cfg\\robots\\cfg.txt"))
        {
            CBlockPar* bpc = Base::g_MatrixData->BlockGet(BLOCK_PATH_MAIN_CONFIG);
            bpc->LoadFromTextFile(L"cfg\\robots\\cfg.txt");
        }
    }
    else Base::g_MatrixData->LoadFromTextFile(L"cfg\\robots\\data.txt");

    // Code for loading data configs from mods
    LoadCfgFromMods(modlist, *Base::g_MatrixData, lang, L"data.txt");

    CBlockPar* repl = Base::g_MatrixData->BlockGetAdd(PAR_REPLACE);

    // init menu replaces

    CBlockPar* rr = Base::g_MatrixData->BlockGet(IF_LABELS_BLOCKPAR)->BlockGet(L"Replaces");
    int cnt = rr->ParCount();
    for(int i = 0; i < cnt; ++i) repl->ParAdd(rr->ParGetName(i), rr->ParGet(i));

    if(txt_start)
    {
        if(txt_start[0] >= '1' && txt_start[0] <= '6')
        {
            repl->ParSetAdd(PAR_REPLACE_BEGIN_ICON_RACE, CWStr(txt_start, 1, Base::g_MatrixHeap));
            repl->ParSetAdd(PAR_REPLACE_DIFFICULTY, CWStr(txt_start + 1, 2, Base::g_MatrixHeap));
            repl->ParSetAdd(PAR_REPLACE_BEGIN_TEXT, txt_start + 3);
        }
        else
        {
            repl->ParSetAdd(PAR_REPLACE_BEGIN_ICON_RACE, L"1");
            repl->ParSetAdd(PAR_REPLACE_BEGIN_TEXT, txt_start);
        }
    }
    else
    {
        repl->ParSetAdd(PAR_REPLACE_BEGIN_ICON_RACE, L"1");
        repl->ParSetAdd(PAR_REPLACE_BEGIN_TEXT, L"Go! Go! Go!");
    }

    if(txt_win) repl->ParSetAdd(PAR_REPLACE_END_TEXT_WIN, txt_win);
    else repl->ParSetAdd(PAR_REPLACE_END_TEXT_WIN, L"Good job man :)");

    if(txt_loss) repl->ParSetAdd(PAR_REPLACE_END_TEXT_LOOSE, txt_loss);
    else repl->ParSetAdd(PAR_REPLACE_END_TEXT_LOOSE, L"Sux :(");

    if(planet) repl->ParSetAdd(PAR_REPLACE_END_TEXT_PLANET, planet);
    else repl->ParSetAdd(PAR_REPLACE_END_TEXT_PLANET, L"Luna");

	CacheInit();
    LoadCfgFromMods(modlist, *g_CacheData, lang, L"cache.txt");

    /*
    if(g_RangersInterface)
    {
        // run from rangers
        // do not set fullscreen
        g_MatrixData->BlockGet(BLOCK_PATH_MAIN_CONFIG)->ParSet(L"FullScreen", L"0");
    }
    */

    //Вынос всех собранных конфигов в отдельный txt
    //g_MatrixData->SaveInTextFile(L"Reboot.txt");

	if(set) L3GInitAsDLL(inst, *Base::g_MatrixData->BlockGet(BLOCK_PATH_MAIN_CONFIG), L"MatrixGame", L"Matrix Game", wnd, set->FDirect3D, set->FD3DDevice);
	else L3GInitAsEXE(inst, *Base::g_MatrixData->BlockGet(BLOCK_PATH_MAIN_CONFIG), L"MatrixGame", L"Matrix Game");

    if(set)
    {
        g_ScreenX = set->m_ResolutionX;
        g_ScreenY = set->m_ResolutionY;
    }

    g_Render = HNew(Base::g_MatrixHeap) CRenderPipeline;   // prepare pipelines

	ShowWindow(g_Wnd, SW_SHOWNORMAL);
	UpdateWindow(g_Wnd);

    g_Config.SetDefaults();
    g_Config.ReadParams();
    if(set)
	{
		g_Config.ApplySettings(set);
		g_Sampler.ApplySettings(set);
	}

    CStorage stor(g_CacheHeap);

    CWStr map_name(g_CacheHeap);
    if(map)
    {
		if(wcschr(map, '\\') == nullptr)
        {
			map_name.Set(L"Matrix\\Map\\");
			map_name.Add(map);
		}
        else map_name.Set(map);
    }
    else map_name = Base::g_MatrixData->BlockGet(BLOCK_PATH_MAIN_CONFIG)->Par(L"Map");

    stor.Load(map_name);

    // Load replacements for MatrixData from map
    if(stor.IsTablePresent(L"da"))
    {
        CBlockPar map_data(g_CacheHeap);
        stor.RestoreBlockPar(L"da", map_data);
        Base::g_MatrixData->BlockMerge(map_data);
    }

	g_MatrixMap = HNew(Base::g_MatrixHeap) CMatrixMapLogic;

	g_MatrixMap->LoadSide(*Base::g_MatrixData->BlockGet(L"Side"));
	//g_MatrixMap->LoadTactics(*g_MatrixData->BlockGet(L"Tactics"));
	g_IFaceList = HNew(Base::g_MatrixHeap) CIFaceList;

	// load new map
    g_MatrixMap->RobotPreload();

    if(0 > g_MatrixMap->PrepareMap(stor, map_name))
    {
        ERROR_S(L"Unable to load map. Error happens.");
    }

    CWStr mn(g_MatrixMap->MapName(), Base::g_MatrixHeap);
    mn.LowerCase();
    if(mn.Find(L"demo") >= 0)
    {
        SETFLAG(g_MatrixMap->m_Flags, MMFLAG_AUTOMATIC_MODE | MMFLAG_FLYCAM | MMFLAG_FULLAUTO);
    }

    g_MatrixMap->CalcCannonPlace();

    SRobotTemplate::ClearAIRobotType(); //На всякий случай

    //Загружаем шаблоны роботов для всех противоборствующих сторон ИИ
    CBlockPar* robot_templates = Base::g_MatrixData->BlockPathGet(L"RobotsConfig.RobotTemplates.ForAI");
    for(int i = 0; i < TOTAL_SIDES; ++i)
    {
        switch(i)
        {
            case NEUTRAL_SIDE: SRobotTemplate::LoadAIRobotType(robot_templates->BlockGetNE(L"Common"), NEUTRAL_SIDE); break;
            case PLAYER_SIDE: SRobotTemplate::LoadAIRobotType(robot_templates->BlockGetNE(L"PlayerSide"), PLAYER_SIDE); break;
            case BLAZER_SIDE: SRobotTemplate::LoadAIRobotType(robot_templates->BlockGetNE(L"BlazerSide"), BLAZER_SIDE); break;
            case KELLER_SIDE: SRobotTemplate::LoadAIRobotType(robot_templates->BlockGetNE(L"KellerSide"), KELLER_SIDE); break;
            case TERRON_SIDE: SRobotTemplate::LoadAIRobotType(robot_templates->BlockGetNE(L"TerronSide"), TERRON_SIDE); break;
        }
    }

    //Загружаем шаблоны роботов для вызова подкреплений с вертолётов
    robot_templates = Base::g_MatrixData->BlockPathGet(L"RobotsConfig.RobotTemplates.ForReinforcements");
    SRobotTemplate::LoadAIRobotType(robot_templates, REINFORCEMENTS_TEMPLATES);

    //Загружаем шаблоны роботов для кабинки-спавнера (как пример, возле Террона такие есть)
    robot_templates = Base::g_MatrixData->BlockPathGet(L"RobotsConfig.RobotTemplates.ForRobotsSpawner");
    SRobotTemplate::LoadAIRobotType(robot_templates, ROBOTS_SPAWNER_TEMPLATES);

    g_LoadProgress->SetCurLP(LP_PREPARININTERFACE);
    g_LoadProgress->InitCurLP(701);

    CBlockPar bpi(1, g_CacheHeap);
    if(stor_cfg_present)
    {
        stor_cfg.RestoreBlockPar(L"if", bpi);
    }
    else
    {
        bpi.LoadFromTextFile(IF_PATH);

        //CStorage stor(g_CacheHeap);
        //stor.StoreBlockPar(L"if", bpi);
        //stor.StoreBlockPar(L"da", *g_MatrixData);
        //stor.Save(FILE_CONFIGURATION, true);
    }

    // The code for loading iface configs from mods
    LoadCfgFromMods(modlist, bpi, lang, L"iface.txt");

    g_ConfigHistory = HNew(Base::g_MatrixHeap) CHistory;
    CInterface* pInterface = nullptr;

    g_LoadProgress->SetCurLPPos(10);

    CMatrixHint::PreloadBitmaps();

    bool iface_save = false;

    g_PopupHull = (SMenuItemText*)HAlloc(sizeof(SMenuItemText) * ROBOT_HULLS_COUNT, Base::g_MatrixHeap);
    for(int i = 0; i < ROBOT_HULLS_COUNT; ++i) new(&g_PopupHull[i]) SMenuItemText();
    //for(int i = 0; i < ROBOT_HULLS_COUNT; ++i) g_PopupHull[i].SMenuItemText::SMenuItemText();

    g_PopupChassis = (SMenuItemText*)HAlloc(sizeof(SMenuItemText) * ROBOT_CHASSIS_COUNT, Base::g_MatrixHeap);
    for(int i = 0; i < ROBOT_CHASSIS_COUNT; ++i) new(&g_PopupChassis[i]) SMenuItemText();
    //for(int i = 0; i < ROBOT_CHASSIS_COUNT; ++i) g_PopupChassis[i].SMenuItemText::SMenuItemText();

    g_PopupHead = (SMenuItemText*)HAlloc(sizeof(SMenuItemText) * (ROBOT_HEADS_COUNT + 1), Base::g_MatrixHeap);
    for(int i = 0; i < (ROBOT_HEADS_COUNT + 1); ++i) new(&g_PopupHead[i]) SMenuItemText();
    //for(int i = 0; i < (ROBOT_HEADS_COUNT + 1); ++i) g_PopupHead[i].SMenuItemText::SMenuItemText();

    g_PopupWeapon = (SMenuItemText*)HAlloc(sizeof(SMenuItemText) * (ROBOT_WEAPONS_COUNT + 1), Base::g_MatrixHeap);
    for(int i = 0; i < (ROBOT_WEAPONS_COUNT + 1); ++i) new(&g_PopupWeapon[i]) SMenuItemText();
    //for(int i = 0; i < (ROBOT_WEAPONS_COUNT + 1); ++i) g_PopupWeapon[i].SMenuItemText::SMenuItemText();

    CIFaceMenu::m_MenuGraphics = HNew(Base::g_MatrixHeap) CInterface;

    g_PopupMenu = HNew(Base::g_MatrixHeap) CIFaceMenu;

    pInterface = HNew(Base::g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_TOP);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_LoadProgress->SetCurLPPos(100);

    pInterface = HNew(Base::g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_MINI_MAP);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_LoadProgress->SetCurLPPos(200);

    pInterface = HNew(Base::g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_RADAR);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_LoadProgress->SetCurLPPos(300);

    //Инициализация и загрузка элементов интерфейса из раздела Base
    pInterface = HNew(Base::g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_BASE);
    LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_IFaceList->m_BaseX = Float2Int(pInterface->m_xPos);
    g_IFaceList->m_BaseY = Float2Int(pInterface->m_yPos);
    g_LoadProgress->SetCurLPPos(400);

    pInterface = HNew(Base::g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_MAIN);
    g_IFaceList->SetMainPos(pInterface->m_xPos, pInterface->m_yPos);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_LoadProgress->SetCurLPPos(500);

    pInterface = HNew(Base::g_MatrixHeap) CInterface;
	iface_save |= pInterface->Load(bpi, IF_HINTS);
	LIST_ADD(pInterface, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);
    g_IFaceList->m_Hints = pInterface;

    g_LoadProgress->SetCurLPPos(600);

    iface_save |= CIFaceMenu::LoadMenuGraphics(bpi);
	//LIST_ADD(CIFaceMenu::m_MenuGraphics, g_IFaceList->m_First, g_IFaceList->m_Last, m_PrevInterface, m_NextInterface);

    g_IFaceList->ConstructorButtonsInit();
   
    g_LoadProgress->SetCurLPPos(700);

    //Слова "нет" для пустых слотов модулей голов и оружия
    g_PopupHead[0].text = Base::g_MatrixData->BlockPathGet(MODULES_CONFIG_PATH_HEADS)->ParGet(L"NoneChoosed");
    g_PopupWeapon[0].text = Base::g_MatrixData->BlockPathGet(MODULES_CONFIG_PATH_WEAPONS)->ParGet(L"NoneChoosed");

    if(g_RangersInterface)
    {
        g_MatrixMap->m_Transition.BuildTexture();
		//g_D3DD->Reset(&g_D3Dpp);
        g_RangersInterface->m_Begin();
    }

    if(set) set->ApplyVideoParams();
	else g_D3DD->Reset(&g_D3Dpp);

	/*
    IDirect3DSurface9* surf;
	g_D3DD->GetRenderTarget(0, &surf);
	if(!(surf == nullptr)) g_D3DD->ColorFill(surf, nullptr, 0);
	surf->Release();
    */

    //Где-то здесь крашит, если отрубить всё модули, необходимые для загрузки стартовых роботов на карте
    g_MatrixMap->m_Transition.RenderToPrimaryScreen();

    CMatrixEffect::InitEffects(*Base::g_MatrixData);
    g_MatrixMap->CreatePoolDefaultResources(true); //Отсюда же в итоге вызывается сборка стартовых роботов на карте
    g_MatrixMap->InitObjectsLights();

    g_MatrixMap->GetPlayerSide()->Select(BUILDING, g_MatrixMap->GetPlayerSide()->m_ActiveObject);
    g_MatrixMap->m_Cursor.Select(CURSOR_ARROW);

    if(!FLAG(g_MatrixMap->m_Flags, MMFLAG_FULLAUTO)) g_MatrixMap->EnterDialogMode(TEMPLATE_DIALOG_BEGIN);

    //Проверяем, включена ли настройка сохранения шаблонов в конфиге
    int maxDesignsToSave = Base::g_MatrixData->BlockGet(BLOCK_PATH_MAIN_CONFIG)->ParGet(CFG_DESIGNS_TO_SAVE).GetInt();
    //Загружаем сохранённые шаблоны роботов игрока из txt-конфига и заносим их в конструктор роботов
    if(maxDesignsToSave > 0)
    {
        wchar robotsCFG_path[MAX_PATH];
        SHGetSpecialFolderPathW(0, robotsCFG_path, CSIDL_PERSONAL, true);
        wcscat(robotsCFG_path, L"\\SpaceRangersHD\\RobotsCFG.txt");

        //Если обнаружили искомый конфиг
        CWStr temp_path(Base::g_MatrixHeap);
        if(CFile::FileExist(temp_path, robotsCFG_path))
        {
            CBlockPar robotsCFG = CBlockPar(true, Base::g_MatrixHeap);
            robotsCFG.LoadFromTextFile(robotsCFG_path);
            CBlockPar* bot_designs = robotsCFG.BlockGetNE(L"BotDesigns");
            if(bot_designs != nullptr)
            {
                int counter = 0;
                //Расшифровываем строку цифро-буквенного шаблона робота, полученную из txt-конфига
                CWStr confStr;
                SRobotConfig conf;
                while(counter < maxDesignsToSave)
                {
                    confStr = bot_designs->ParGetNE(L"Bot" + CWStr(counter + 1));
                    if(confStr.IsEmpty()) break;

                    //Исключаем заведомо невалидные по длине шаблоны
                    if(confStr.GetCountPar(L",") < 2) continue;

                    //Предварительно очищаем и инициализируем шаблон
                    ZeroMemory(&conf, sizeof(conf)); 
                    conf.m_Chassis.m_nType = MRT_CHASSIS; 
                    conf.m_Hull.m_Module.m_nType = MRT_HULL; 
                    conf.m_Head.m_nType = MRT_HEAD; 
                    for(int cnt = 0; cnt < RUK_WEAPON_PYLONS_COUNT; ++cnt)
                    {
                        conf.m_Weapon[cnt].m_nType = MRT_WEAPON;
                    }

                    //Определяем шасси
                    CWStr t_str = confStr.GetStrPar(0, L",");
                    t_str.Trim();
                    conf.m_Chassis.m_nKind = RUK_CHASSIS_PNEUMATIC; //По умолчанию, на случай, если в шаблоне невалид
                    for(int i = 1; i <= ROBOT_CHASSIS_COUNT; ++i)
                    {
                        if(t_str == g_Config.m_RobotChassisConsts[i].short_designation)
                        {
                            if(g_Config.m_RobotChassisConsts[i].for_player_side) conf.m_Chassis.m_nKind = (ERobotModuleKind)i;
                            break;
                        }
                    }

                    //Определяем корпус
                    t_str = confStr.GetStrPar(1, L",");
                    t_str.Trim();
                    conf.m_Hull.m_Module.m_nKind = RUK_HULL_MONOSTACK; //По умолчанию, на случай, если в шаблоне невалид
                    for(int i = 1; i <= ROBOT_HULLS_COUNT; ++i)
                    {
                        if(t_str == g_Config.m_RobotHullsConsts[i].short_designation)
                        {
                            if(g_Config.m_RobotHullsConsts[i].for_player_side) conf.m_Hull.m_Module.m_nKind = (ERobotModuleKind)i;
                            break;
                        }
                    }

                    //Записываем в конфиг предельное число оружия для данного типа корпуса
                    conf.m_Hull.m_MaxWeaponsCnt = g_Config.m_RobotHullsConsts[conf.m_Hull.m_Module.m_nKind].weapon_pylon_data.size();

                    //Определяем оружие
                    //По умолчанию во всех слотах пусто
                    for(int i = 0; i < RUK_WEAPON_PYLONS_COUNT; ++i)
                    {
                        conf.m_Weapon[i].m_nType = MRT_WEAPON;
                        conf.m_Weapon[i].m_nKind = RUK_EMPTY;
                    }

                    if(confStr.GetCountPar(L",") >= 3)
                    {
                        int cur_hull_kind = conf.m_Hull.m_Module.m_nKind;

                        t_str = confStr.GetStrPar(2, L",");
                        t_str.Trim();

                        for(int i = 0; i < conf.m_Hull.m_MaxWeaponsCnt; ++i)
                        {
                            int constructor_slot_num = g_Config.m_RobotHullsConsts[cur_hull_kind].weapon_pylon_data[i].constructor_slot_num;

                            CWStr gun_des = t_str.GetStrPar(constructor_slot_num, L".");
                            if(gun_des == L"-") continue; //Пустой слот
                            for(int j = 1; j <= ROBOT_WEAPONS_COUNT; ++j)
                            {
                                if(gun_des == g_Config.m_RobotWeaponsConsts[j].short_designation)
                                {
                                    //Если оружие доступно игроку и может быть установлено в данный пилон
                                    if(g_Config.m_RobotWeaponsConsts[j].for_player_side && g_Config.m_RobotHullsConsts[cur_hull_kind].weapon_pylon_data[i].fit_weapon.test(j)) conf.m_Weapon[constructor_slot_num].m_nKind = (ERobotModuleKind)j;
                                    break;
                                }
                            }
                        }
                    }

                    //Определяем голову
                    if(confStr.GetCountPar(L",") >= 4)
                    {
                        t_str = confStr.GetStrPar(3, L",");
                        t_str.Trim();

                        conf.m_Head.m_nKind = RUK_EMPTY; //На случай, если в шаблоне записана несуществующая голова
                        for(int i = 1; i <= ROBOT_HEADS_COUNT; ++i)
                        {
                            if(t_str == g_Config.m_RobotHeadsConsts[i].short_designation)
                            {
                                if(g_Config.m_RobotHeadsConsts[i].for_player_side) conf.m_Head.m_nKind = (ERobotModuleKind)i;
                                break;
                            }
                        }
                    }

                    //Заносим готовый конфиг в историю
                    g_ConfigHistory->AddConfig(&conf);

                    ++counter;
                }
                
                //Восстанавливаем номер последнего использованного игроком чертежа
                int des_num = 1;
                confStr = bot_designs->ParGetNE(L"ChoosedDesign");
                if(!confStr.IsEmpty()) des_num = confStr.GetDword();

                //Убираем начальный стоковый чертёж завода и заменяем его на первый из списка загруженных
                g_ConfigHistory->m_CurrentConfig = g_ConfigHistory->m_FirstConfig;
                while(des_num > 1)
                {
                    if(g_ConfigHistory->m_CurrentConfig->m_NextConfig == nullptr) break; //На случай, если в чертежи .txt попали дубликаты (или номер активного конфига оказался выше их количества в списке), которые AddConfig() успешно потёр
                    g_ConfigHistory->m_CurrentConfig = g_ConfigHistory->m_CurrentConfig->m_NextConfig;
                    --des_num;
                }

                g_ConfigHistory->LoadCurrentConfigToConstructor();
            }
        }
    }

    //if(iface_save) bpi.SaveInTextFile(IF_PATH, true);
}

void SRobotsSettings::ApplyVideoParams()
{
    int bpp;
	D3DDISPLAYMODE d3ddm;

	D3DDEVICE_CREATION_PARAMETERS params;
	g_D3DD->GetCreationParameters(&params);
	//ASSERT_DX(g_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm));
	ASSERT_DX(g_D3D->GetAdapterDisplayMode(params.AdapterOrdinal, &d3ddm));

    if(d3ddm.Format == D3DFMT_X8R8G8B8 || d3ddm.Format == D3DFMT_A8R8G8B8)
    {
        d3ddm.Format=D3DFMT_A8R8G8B8;
        bpp = 32;
    }
    else bpp = 16;

    bool change_refresh_rate = m_RefreshRate != 0 && m_RefreshRate != d3ddm.RefreshRate;
    int  refresh_rate_required = change_refresh_rate ? m_RefreshRate : 0;

	RECT rect;
    GetClientRect(g_Wnd, &rect);
	bool was_in_window_mode = (rect.right != d3ddm.Width || rect.bottom != d3ddm.Height);
	bool now_in_window_mode = was_in_window_mode && (bpp == m_BPP) && !change_refresh_rate;
	
	if (m_FSAASamples > 16) ERROR_S(L"Invalid multisample type");
	_D3DMULTISAMPLE_TYPE expectedMultiSampleType = (_D3DMULTISAMPLE_TYPE)m_FSAASamples;//(m_FSAASamples >> 24);	
	
	ZeroMemory(&g_D3Dpp, sizeof(g_D3Dpp));

	if(now_in_window_mode)
    {
        RESETFLAG(g_Flags, GFLAG_FULLSCREEN);
        g_D3Dpp.Windowed = TRUE;

        RECT r1, r2;
        GetWindowRect(g_Wnd, &r1);
        GetClientRect(g_Wnd, &r2);
        SetWindowPos(g_Wnd, nullptr, 0, 0, m_ResolutionX + (r1.right - r1.left - r2.right), m_ResolutionY + (r1.bottom - r1.top - r2.bottom), SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
    }
    else
    {
        SETFLAG(g_Flags, GFLAG_FULLSCREEN);
        g_D3Dpp.Windowed = FALSE;

        if(was_in_window_mode)
        {
            SetWindowLong(g_Wnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            MoveWindow(g_Wnd, 0, 0, m_ResolutionX, m_ResolutionY, false);
        }
	}

    g_D3Dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_D3Dpp.BackBufferFormat = (m_BPP == 16) ? D3DFMT_R5G6B5 : D3DFMT_A8R8G8B8;
    g_D3Dpp.EnableAutoDepthStencil = TRUE;
    g_D3Dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    g_D3Dpp.PresentationInterval = m_VSync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    g_D3Dpp.FullScreen_RefreshRateInHz = refresh_rate_required;
    g_D3Dpp.BackBufferWidth = m_ResolutionX;
    g_D3Dpp.BackBufferHeight = m_ResolutionY;
    g_D3Dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    g_D3Dpp.hDeviceWindow = g_Wnd;

	if(SUCCEEDED(g_D3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_D3Dpp.BackBufferFormat, g_D3Dpp.Windowed, expectedMultiSampleType, nullptr)))
	{
		g_D3Dpp.MultiSampleType = expectedMultiSampleType;			
	}

    SETFLAG(g_Flags, GFLAG_STENCILAVAILABLE);


    if(D3D_OK != g_D3DD->Reset(&g_D3Dpp))
    {
        if(bpp == 16) g_D3Dpp.AutoDepthStencilFormat = D3DFMT_D16;
        else g_D3Dpp.AutoDepthStencilFormat = D3DFMT_D24X8;

        if(D3D_OK != g_D3DD->Reset(&g_D3Dpp)) ERROR_S(L"Sorry, unable to set this params.");
        RESETFLAG(g_Flags, GFLAG_STENCILAVAILABLE);
    }

	D3DVIEWPORT9 ViewPort;
	ZeroMemory(&ViewPort, sizeof(D3DVIEWPORT9));

    ViewPort.X      = 0;
	ViewPort.Y      = 0;
    ViewPort.Width  = m_ResolutionX;
	ViewPort.Height = m_ResolutionY;

	ViewPort.MinZ   = 0.0f;
	ViewPort.MaxZ   = 1.0f;
	
	ASSERT_DX(g_D3DD->SetViewport(&ViewPort));
   

	S3D_Default();

    g_D3DD->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, (g_D3Dpp.MultiSampleType != D3DMULTISAMPLE_NONE));

    D3DMATERIAL9 mtrl;
    ZeroMemory(&mtrl, sizeof(D3DMATERIAL9));
    mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
    mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
    mtrl.Diffuse.b = mtrl.Ambient.b = 1.0f;
    mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
    mtrl.Specular.r = 0.5f;
    mtrl.Specular.g = 0.5f;
    mtrl.Specular.b = 0.5f;
    mtrl.Specular.a = 0.5f;
    g_D3DD->SetMaterial(&mtrl);
    g_D3DD->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);

    D3DXVECTOR3 vecDir;
    D3DLIGHT9 light;
    ZeroMemory(&light, sizeof(D3DLIGHT9));
    light.Type = D3DLIGHT_DIRECTIONAL;//D3DLIGHT_POINT;//D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = GetColorR(g_MatrixMap->m_LightMainColorObj);
    light.Diffuse.g = GetColorG(g_MatrixMap->m_LightMainColorObj);
    light.Diffuse.b = GetColorB(g_MatrixMap->m_LightMainColorObj);
    light.Ambient.r = 0.0f;
    light.Ambient.g = 0.0f;
    light.Ambient.b = 0.0f;
    light.Specular.r = GetColorR(g_MatrixMap->m_LightMainColorObj);
    light.Specular.g = GetColorG(g_MatrixMap->m_LightMainColorObj);
    light.Specular.b = GetColorB(g_MatrixMap->m_LightMainColorObj);
    //light.Range = 0;
    light.Direction = g_MatrixMap->m_LightMain;
    //light.Direction=D3DXVECTOR3(250.0f, -50.0f, -250.0f);
    //D3DXVec3Normalize((D3DXVECTOR3*)(&(light.Direction)), (D3DXVECTOR3*)(&(light.Direction)));
    ASSERT_DX(g_D3DD->SetLight(0, &light));
    ASSERT_DX(g_D3DD->LightEnable(0, TRUE));
}

void MatrixGameDeinit(void)
{
    //Сохраняем указанное число шаблонов роботов игрока, если настройка MaxDesignsToSave не равна нулю
    int maxDesignsToSave = Base::g_MatrixData->BlockGet(BLOCK_PATH_MAIN_CONFIG)->ParGet(CFG_DESIGNS_TO_SAVE).GetInt();

    if(maxDesignsToSave > 0)
    {
        //Если есть что сохранять (игрок построил хотя бы одного робота за матч)
        if(g_ConfigHistory->m_CurrentConfig != nullptr)
        {
            CBlockPar robotsCFG = CBlockPar(true, Base::g_MatrixHeap);
            wchar robotsCFG_Path[MAX_PATH];
            SHGetSpecialFolderPathW(0, robotsCFG_Path, CSIDL_PERSONAL, true);
            wcscat(robotsCFG_Path, L"\\SpaceRangersHD\\RobotsCFG.txt");

            //Если обнаружили искомый конфиг, то загружаем его во избежание потери прочих, возможно содержащихся в нём, данных
            CWStr temp_path(Base::g_MatrixHeap);
            if(CFile::FileExist(temp_path, robotsCFG_Path)) robotsCFG.LoadFromTextFile(robotsCFG_Path);
            //Иначе создаём совершенно новый объект документа
            else robotsCFG = CBlockPar(true, Base::g_MatrixHeap);
            //Перед записью шаблонов удаляем возможно уже имевшийся там ранее блок
            robotsCFG.BlockDeleteNE(L"BotDesigns", 10);
            CBlockPar* bot_designs = robotsCFG.BlockAdd(L"BotDesigns");

            SRobotConfig* cur_config = g_ConfigHistory->m_LastConfig;

            //Начинаем перебор шаблонов до начала списка, откуда позже начнём их сохранение
            int counter = maxDesignsToSave;
            //Заодно определяем, какой конфиг по счёту был выбран в конструкторе на конец боя
            int des_num = 1;
            cur_config = g_ConfigHistory->m_CurrentConfig;
            while(counter > 1)
            {
                if(cur_config != g_ConfigHistory->m_FirstConfig && cur_config != nullptr)
                {
                    --counter;
                    ++des_num;
                    cur_config = cur_config->m_PrevConfig;
                }
                else break;
            }

            CWStr to_config(Base::g_MatrixHeap);
            counter = 0;
            while((cur_config != nullptr) && (counter < maxDesignsToSave))
            {
                to_config.Clear();

                //Формируем строку цифро-буквенного шаблона робота для записи в txt-конфиг
                //Определяем шасси
                to_config = g_Config.m_RobotChassisConsts[cur_config->m_Chassis.m_nKind].short_designation;

                //Определяем туловище
                auto cur_gear = cur_config->m_Hull.m_Module.m_nKind;
                to_config += L",";
                to_config += g_Config.m_RobotHullsConsts[cur_gear].short_designation;

                //Определяем оружие
                for(int i = 0; i < RUK_WEAPON_PYLONS_COUNT; ++i)
                {
                    cur_gear = cur_config->m_Weapon[i].m_nKind;
                    if(!i) to_config += L",";

                    if(cur_gear) to_config += g_Config.m_RobotWeaponsConsts[cur_gear].short_designation;
                    else to_config += L"-"; //Если слот оружия пуст
                    if(i < RUK_WEAPON_PYLONS_COUNT - 1) to_config += L".";
                }

                //Определяем голову
                cur_gear = cur_config->m_Head.m_nKind;
                if(cur_gear)
                {
                    to_config += L",";
                    to_config += g_Config.m_RobotHeadsConsts[cur_gear].short_designation;
                }

                //Заносим готовый шаблон в список сохранения
                bot_designs->ParAdd(L"Bot" + CWStr(counter + 1), to_config);

                //Выбираем следующий конфиг
                cur_config = cur_config->m_NextConfig;

                ++counter;
            }

            //Также запоминаем номер последнего выбранного конфига
            bot_designs->ParAdd(L"ChoosedDesign", CWStr(des_num));
            //Записываем готовые шаблоны в txt-конфиг
            robotsCFG.SaveInTextFile(robotsCFG_Path);
        }
    }

    SRobotTemplate::ClearAIRobotType();

    if(g_Render)
    {
        HDelete(CRenderPipeline, g_Render, Base::g_MatrixHeap);
        g_Render = nullptr;
    }

    CMatrixHint::ClearAll();
	
	if(g_MatrixMap)
    {
		ASSERT(g_MatrixHeap);

		HDelete(CMatrixMapLogic, g_MatrixMap, Base::g_MatrixHeap);
		g_MatrixMap = nullptr;
	}

    if(Base::g_MatrixData)
    {
        HDelete(CBlockPar, Base::g_MatrixData, nullptr);
        Base::g_MatrixData = nullptr;
    }

    CMatrixRobot::DestroyWalkingChassisData();

    if(g_IFaceList)
    {
		ASSERT(Base::g_MatrixHeap);
		HDelete(CIFaceList, g_IFaceList, Base::g_MatrixHeap);
        g_IFaceList = nullptr;
    }

    if(g_ConfigHistory)
    {
		ASSERT(Base::g_MatrixHeap);
		HDelete(CHistory, g_ConfigHistory, Base::g_MatrixHeap);
        g_ConfigHistory = nullptr;
    }
	
    if(CIFaceMenu::m_MenuGraphics)
    {
        HDelete(CInterface, CIFaceMenu::m_MenuGraphics, Base::g_MatrixHeap);
        CIFaceMenu::m_MenuGraphics = nullptr;
    }

    if(g_PopupMenu)
    {
        HDelete(CIFaceMenu, g_PopupMenu, Base::g_MatrixHeap);
        g_PopupMenu = nullptr;
    }

    if(g_PopupHull)
    {
        for(int i = 0; i < ROBOT_HULLS_COUNT; ++i)
        {
            g_PopupHull[i].text.CWStr::~CWStr();
        }
        
        HFree(g_PopupHull, Base::g_MatrixHeap);
        g_PopupHull = nullptr;
    }

    if(g_PopupChassis)
    {
        for(int i = 0; i < ROBOT_CHASSIS_COUNT; ++i)
        {
            g_PopupChassis[i].text.CWStr::~CWStr();
        }

        HFree(g_PopupChassis, Base::g_MatrixHeap);
        g_PopupChassis = nullptr;
    }

    if(g_PopupHead)
    {
        for(int i = 0; i < ROBOT_HEADS_COUNT + 1; ++i)
        {
            g_PopupHead[i].text.CWStr::~CWStr();
        }

        HFree(g_PopupHead, Base::g_MatrixHeap);
        g_PopupHead = nullptr;
    }

    if(g_PopupWeapon)
    {
        for(int i = 0; i < ROBOT_WEAPONS_COUNT + 1; ++i)
        {
            g_PopupWeapon[i].text.CWStr::~CWStr();
        }
        
        HFree(g_PopupWeapon, Base::g_MatrixHeap);
        g_PopupWeapon = nullptr;
    }

    CInstDraw::ClearAll();
    CFile::ReleasePackFiles();

    g_Config.Clear();

    if(Base::g_MatrixHeap)
    {
        HDelete(CHeap, Base::g_MatrixHeap, nullptr);
        Base::g_MatrixHeap = nullptr;
	}
}

LPCSTR PathToOutputFiles(LPCSTR dest)
{
    ITEMIDLIST* pidl;
    static char lpPath[MAX_PATH];

    HRESULT hRes = SHGetSpecialFolderLocation(nullptr, CSIDL_PERSONAL, &pidl);
    if(hRes == NOERROR)
    {
        SHGetPathFromIDList(pidl, lpPath);

        strcat(lpPath, "\\SpaceRangersHD");
        CreateDirectory(lpPath, nullptr);
        strcat(lpPath, "\\");
        strcat(lpPath, dest);
    }
    else strcpy(lpPath, "");

    LPCSTR result;
    result = lpPath;

     //FILE* fp = fopen("1.log", "w+t");
     //fprintf(fp, "Direct3D=%s\n", result);
     //fclose(fp);

    return result;
}