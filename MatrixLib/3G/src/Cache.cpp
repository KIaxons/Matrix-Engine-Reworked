// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#include "stdafx.h"
#include "Cache.hpp"
#include "VectorObject.hpp"

bool   CCacheData::m_dip; 

//bool CacheFileGetA(CWStr & outname,const wchar * mname,const wchar * exts,bool withpar)
//{
//    DTRACE();
//
//	int len=WStrLen(mname);
//	const wchar * str=mname;
//
//	int lenfile=0; while(lenfile<len && str[lenfile]!='?') lenfile++;
//
//	CWStr filename(str,lenfile,outname.GetHeap());
//
//	WIN32_FIND_DATAA fd;
//	HANDLE fh=FindFirstFileA(CStr(filename).Get(),&fd);
//	if(fh!=INVALID_HANDLE_VALUE) { FindClose(fh); if(withpar) outname=mname; else outname=filename; return true; }
//	
//
//	fh=FindFirstFileA(CStr(CWStr(str,lenfile,outname.GetHeap())+L".*",outname.GetHeap()).Get(),&fd);
//	if(fh==INVALID_HANDLE_VALUE) return false;
//	if(exts!=nullptr) {
//		CWStr curname(outname.GetHeap());
//		for(;;)
//        {
//			curname.Set(CStr(fd.cFileName));
//			int sme=curname.FindR(L'.')+1;
//			if(sme>0 && sme<curname.GetLen()) {
//				curname.LowerCase(sme);
//				const wchar * str=curname.Get()+sme;
//				int len = curname.GetLen()-sme;
//
//				const wchar * exts2=exts;
//				int cntok=0;
//				int lenext=0;
//				for(;;)
//                {
//					if(*exts2==0 || * exts2==L'~') {
//						if(cntok==len && lenext==len) break;
//						cntok=0;
//						lenext=-1;
//						if(*exts2==0) break;
//					} else if(*exts2==str[cntok]) cntok++;
//					exts2++;
//					lenext++;
//				}
//				if(cntok==len) break;
//			}
//
//			if(!FindNextFileA(fh,&fd)) { FindClose(fh); return false; }
//		}
//	}
//	FindClose(fh);
//
//	int lenpath=lenfile; while(lenpath>0 && str[lenpath-1]!='\\' && str[lenpath-1]!='/') lenpath--;
//
//	if(lenpath>0)
//    {
//        outname.Set(str,lenpath);
//        outname.Add( CWStr( CStr(fd.cFileName,outname.GetHeap())));
//    } else outname.Set(CStr(fd.cFileName,outname.GetHeap()));
//
//	if(withpar && lenfile<len) outname.Add(str+lenfile,len-lenfile);
//
//	return true;
//}
//
//bool CacheFileGetW(CWStr & outname,const wchar * mname,const wchar * exts,bool withpar)
//{
//    DTRACE();
//
//	int len=WStrLen(mname);
//	const wchar * str=mname;
//
//	int lenfile=0; while(lenfile<len && str[lenfile]!='?') lenfile++;
//
//	CWStr filename(str,lenfile,outname.GetHeap());
//
//	WIN32_FIND_DATAW fd;
//	HANDLE fh=FindFirstFileW(filename,&fd);
//	if(fh!=INVALID_HANDLE_VALUE) { FindClose(fh); if(withpar) outname=mname; else outname=filename; return true; }
//	
//
//	fh=FindFirstFileW((CWStr(str,lenfile,outname.GetHeap())+L".*").Get(),&fd);
//	if(fh==INVALID_HANDLE_VALUE) return false;
//	if(exts!=nullptr) {
//		CWStr curname(outname.GetHeap());
//		for(;;)
//        {
//			curname.Set(fd.cFileName);
//			int sme=curname.FindR(L'.')+1;
//			if(sme>0 && sme<curname.GetLen()) {
//				curname.LowerCase(sme);
//				const wchar * str=curname.Get()+sme;
//				int len=curname.GetLen()-sme;
//
//				const wchar * exts2=exts;
//				int cntok=0;
//				int lenext=0;
//				for(;;)
//                {
//					if(*exts2==0 || * exts2==L'~') {
//						if(cntok==len && lenext==len) break;
//						cntok=0;
//						lenext=-1;
//						if(*exts2==0) break;
//					} else if(*exts2==str[cntok]) cntok++;
//					exts2++;
//					lenext++;
//				}
//				if(cntok==len) break;
//			}
//
//			if(!FindNextFileW(fh,&fd)) { FindClose(fh); return false; }
//		}
//	}
//	FindClose(fh);
//
//	int lenpath=lenfile; while(lenpath>0 && str[lenpath-1]!='\\' && str[lenpath-1]!='/') lenpath--;
//
//	if(lenpath>0) { outname.Set(str,lenpath); outname.Add(fd.cFileName); }
//	else outname.Set(fd.cFileName);
//
//	if(withpar && lenfile<len) outname.Add(str+lenfile,len-lenfile);
//
//	return true;
//}
//
//bool CacheFileGet(CWStr & outname,const wchar * mname,const wchar * exts,bool withpar)
//{
//    DTRACE();
//
//	if(GetVersion()<0x80000000) return CacheFileGetW(outname,mname,exts,withpar);
//	else return CacheFileGetA(outname,mname,exts,withpar);
//}

void CacheReplaceFileExt(CWStr& outname, const wchar* mname, const wchar* ext)
{
	int len = WStrLen(mname);
	const wchar* str = mname;

	int lenfile = 0;
	while(lenfile < len && str[lenfile] != '?') ++lenfile;

	int smeext = lenfile - 1;
	while(smeext >= 0)
	{
		if(str[smeext] == L'\\' || str[smeext] == L'/')
		{
			smeext = lenfile;
			break;
		}
		else if(str[smeext] == L'.') break;

		--smeext;
	}

	if(smeext < 0) smeext = lenfile;

	outname.Set(mname, smeext);
	if(ext) outname.Add(ext);

	if(lenfile < len) outname.Add(mname + lenfile, len - lenfile);
}

void CacheReplaceFileNameAndExt(CWStr& outname, const wchar* mname, const wchar* replname)
{
	int len = WStrLen(mname);
	const wchar* str = mname;

	int lenfile = 0;
	while(lenfile < len && str[lenfile] != '?') ++lenfile;

	int smefile = lenfile - 1;
	while(smefile >= 0)
	{
		if(str[smefile] == L'\\' || str[smefile] == L'/')
		{
			++smefile;
			break;
		}
		--smefile;
	}

	if(smefile < 0)
	{
		smefile = 0;
		//outname = mname;
		//return;
	}

	outname.Set(mname, smefile);
	if(replname) outname.Add(replname);
	if(lenfile < len) outname.Add(mname + lenfile, len - lenfile);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void CCacheData::Prepare()
{
	if(!IsLoaded()) Load();
	if(m_Cache && !m_Prev) m_Cache->Up(this); 
}

void CCacheData::LoadFromFile(CBuf& buf, const wchar* exts)
{
	CWStr tstr;
	CWStr tname;
    tname = m_Name.GetStrPar(0, L"?");

	if(!CFile::FileExist(tstr, tname.Get(), exts, false))
    {
		//�������, ����� ���� ����������� ������ ���� �������������� �������� � ������ ������, ��-������� �� �� ��������, �.�. ��� ���������� ���������
		CWStr alt_path = CWStr(tname.Get());
		CWStr folder_name = alt_path.GetStrPar(1, L"\\");

		if(folder_name == L"TerTop") alt_path = g_CacheData->BlockPathGet(L"Map.Textures.TerrainApplications")->ParGetNE(alt_path.GetStrPar(2, L"\\"));
		else if(folder_name == L"Obj") alt_path = g_CacheData->BlockPathGet(L"Map.Textures.Objects")->ParGetNE(alt_path.GetStrPar(3, L"\\"));

		if(!CFile::FileExist(tstr, alt_path, exts, false)) ERROR_S4(L"File not found: ", alt_path, L"   Exts: ", exts);
    }

	buf.LoadFromFile(tstr.Get());

	/*
	ASSERT(m_Name.Len() > 0);

	int len = m_Name.Len();
	wchar* str = m_Name.Get();

	int lenfile = 0;
	while(lenfile < len && str[lenfile] != '?') ++lenfile;
	ASSERT(lenfile > 0);

	CFile fi(g_CacheHeap);
	fi.Init(str, lenfile);
	if(fi.OpenReadNE())
	{
		buf.Len(fi.Size());
		fi.Read(buf.Get(), buf.Len());
		buf.Pointer(0);
		return;
	}

	WIN32_FIND_DATA fd;
	HANDLE fh = FindFirstFile(CStr(CWStr(str, lenfile, g_CacheHeap) + L".*").Get(), &fd);
	if(fh == INVALID_HANDLE_VALUE) ERROR_S2(L"File not found: ", m_Name.Get());
	FindClose(fh);

	int lenpath = lenfile;
	while(lenpath > 0 && str[lenpath - 1] != '\\' && str[lenpath - 1] != '/') --lenpath;
	
	if(lenpath > 0) fi.Init(CWStr(str, lenpath, g_CacheHeap) + CWStr(CStr(fd.cFileName, g_CacheHeap), g_CacheHeap));
	else fi.Init(CWStr(CStr(fd.cFileName, g_CacheHeap), g_CacheHeap));

	if(fi.OpenReadNE())
	{
		buf.Len(fi.Size());
		fi.Read(buf.Get(), buf.Len());
		buf.Pointer(0);
		return;
	}

	ERROR_S2(L"Error open file: ", m_Name.Get());
	*/
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CCache::~CCache()
{

}

void CCache::Add(CCacheData* cd)
{
	ASSERT(cd);
	ASSERT(cd->m_Prev == nullptr && cd->m_Next == nullptr);

	LIST_INSERT(m_First, cd, m_First, m_Last, m_Prev, m_Next);
	cd->m_Cache = this;
}

void CCache::Delete(CCacheData* cd)
{
	ASSERT(cd);
	ASSERT(cd->m_Cache == this);

	LIST_DEL(cd, m_First, m_Last, m_Prev, m_Next);
	cd->m_Cache = nullptr;
}

void CCache::Up(CCacheData* cd)
{
	ASSERT(cd);
	ASSERT(cd->m_Cache == this);

	LIST_DEL(cd, m_First, m_Last, m_Prev, m_Next);
	LIST_INSERT(m_First, cd, m_First, m_Last, m_Prev, m_Next);
}

void CCache::PreLoad()
{
	CCacheData* cd = m_First;
	while(cd)
	{
		if(cd->m_Type == cc_VO)
		{
			if(!cd->IsLoaded()) cd->Load();
		}
		cd = cd->m_Next;
	}
}

CCacheData* CCache::Find(CacheClass cc, const wchar* name)
{
	CCacheData* cd = m_First;
	while(cd)
	{
		if(cd->m_Type == cc && cd->m_Name == name) return cd;
		cd = cd->m_Next;
	}

	return nullptr;
}

CCacheData* CCache::Get(CacheClass cc, const wchar* name)
{
	CCacheData* cd = Find(cc, name);
	if(cd) return cd;

#ifdef FORCE_ENABLE_MEM_SPY
	cd = Create(cc, __FILE__, __LINE__);
#else
	cd = Create(cc);
#endif
	cd->m_Name = name;
	Add(cd);

	return cd;
}

CCacheData* CCache::Create(CacheClass cc)
{
    CCacheData* el;

	switch(cc)
	{
		case cc_Texture: el = HNew(g_CacheHeap) CTexture; break;
		case cc_TextureManaged: el = HNew(g_CacheHeap) CTextureManaged; break;
		case cc_VO: el = HNew(g_CacheHeap) CVectorObject; break;
		default: ERROR_S(L"CCache::Create Error!");
	}

    return el;
}

void CCacheData::StaticInit()
{
    m_dip = false;
    CBaseTexture::StaticInit();
}

void CCache::Destroy(CCacheData* cd)
{
	ASSERT(cd);
    HDelete(CCacheData, cd, g_CacheHeap);
}

#ifdef _DEBUG
#include <stdio.h>
void CCache::Dump()
{
    char    buf[65536];
    strcpy(buf, "Cache Dump\n");
	CCacheData* cd = d_First;
    int cnt = 0;
	while(cd)
    {
        ++cnt;
        char *                            type = "Unknown       ";

        if(cd->m_Type==cc_Texture)        type = "Texture       ";
        if(cd->m_Type==cc_TextureManaged) type = "TextureManaged";
        if(cd->m_Type==cc_VO)             type = "Object        ";

        CStr    name((cd->m_Name == nullptr) ? L"nullptr" : cd->m_Name, g_CacheHeap);

        char *loaded;
        if(cd->IsLoaded()) loaded = "+";
		else loaded = "-";

        int l = strlen(buf);
        if(l < 65000)
        {
            sprintf(buf + l, "%s%s : %s (%s : %i)\n", loaded,type, name.Get(), cd->d_file, cd->d_line);
        }
		cd=cd->d_Next;
    }

    int l = strlen(buf);
    sprintf(buf + l, "count: %i\n", cnt);

	SFT(buf);
    //MessageBox(g_Wnd, buf, "D3D Dump", MB_ICONINFORMATION);

    CBuf b(g_CacheHeap);
    b.Len(strlen(buf));
    memcpy(b.Get(), &buf, strlen(buf));
    b.SaveInFile(L"debug_dump.txt");
}
#endif


void CCache::Clear(void)
{
	CCacheData::m_dip = true;

	CCacheData* cd2, * cd = m_First;
	while(cd)
	{
		cd2 = cd;
		cd = cd->m_Next;

		Delete(cd2);
		CCache::Destroy(cd2);
	}

    CCacheData::m_dip = false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CCache* g_Cache;
CBlockPar* g_CacheData; // confusing name, but best of possible, imo
CHeap* g_CacheHeap;
const wchar* CacheExtsTex = L"png~jpg~bmp~dds~strg";

void CacheInit(void)
{
DTRACE();

    g_CacheHeap = HNew(nullptr) CHeap();
    g_CacheData = HNew(g_CacheHeap) CBlockPar();
    g_Cache = HNew(g_CacheHeap) CCache;
}

void CacheDeinit()
{
DTRACE();

	if(g_Cache)
    {
		ASSERT(g_CacheHeap);
        g_Cache->Clear();
		HDelete(CCache, g_Cache, g_CacheHeap);
		g_Cache = nullptr;
	}

	if(g_CacheData)
	{
		ASSERT(g_CacheHeap);
		g_CacheData->Clear();
		HDelete(CBlockPar, g_CacheData, g_CacheHeap);
		g_CacheData = nullptr;
	}

	if(g_CacheHeap)
	{
		HDelete(CHeap, g_CacheHeap, nullptr);
		g_CacheHeap = nullptr;
	}
}


