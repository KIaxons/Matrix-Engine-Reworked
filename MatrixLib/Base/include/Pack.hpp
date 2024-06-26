// MatrixGame - SR2 Planetary battles engine
// Copyright (C) 2012, Elemental Games, Katauri Interactive, CHK-Games
// Licensed under GPLv2 or any later version
// Refer to the LICENSE file included

#ifndef PACK_INCLUDE
#define PACK_INCLUDE

#include "../src/stdafx.h"

#include "CStr.hpp"
#include "CWStr.hpp"
#include "CException.hpp"
#include "CMain.hpp"
#include "CHeap.hpp"
#include "CBuf.hpp"

namespace Base {

//#define RAISE_ALL_EXCEPTIONS
//#define HANDLE_OUT_OF_PACK_FILES
//#define SUPPORT_IN_MEMORY_STRUCTURES

enum EFileType
{
    FILEEC_TEXT         = 0,
    FILEEC_BINARY       = 1,
    FILEEC_COMPRESSED   = 2,
    FILEEC_FOLDER       = 3,

    FILEEC_FORCE_DWORD   = 0x7fffffff
};

#define MAX_VIRTUAL_HANDLE_COUNT_BITS   4
#define MAX_FILENAME_LENGTH         63 // Максимальная длина имени файла



#define MAX_VIRTUAL_HANDLE_COUNT    (1 << MAX_VIRTUAL_HANDLE_COUNT_BITS) // do not modify this

//Подравниваем эту хрень по размеру, чтобы было точное соответствие с пакетной структурой, где установлен другой режим байтовой упаковки:
//Project Settings > C/C++ > Code Generation > Struct Member Alignment > Zp1 (1 byte) (в проекте ПБ был переставлен на Default, ибо так лучше и стабильнее)
#pragma pack(push)
#pragma pack(1)
struct SFileRec
{
    dword   m_Size;                 // Размер файла (+4) - это размер блока
    dword   m_RealSize;             // Настоящий размер файла (не сжатого)
    char    m_Name[MAX_FILENAME_LENGTH];      // Имя файла
    char    m_RealName[MAX_FILENAME_LENGTH];  // Имя файла в файловой системе с учетом регистров
    EFileType   m_Type;                 // Тип файла, на который указывает запись
    EFileType   m_NType;                // Тип который должен быть присвоен файлу
    dword   m_Free;                 // Свободна данная структура или занята
    dword   m_Date;                 // Дата и время файла
    dword   m_Offset;               // Смещение данных относительно начала файла
    dword   m_Extra;                // Данные во время работы объекта
};
#pragma pack(pop)
typedef SFileRec *PSFileRec;

struct SFolderRec
{
    dword   m_Size;             // Размер всей директории
    dword   m_Recnum;           // Количество записей в ней
    dword   m_RecSize;          // Размер одной записи файла
};
typedef SFolderRec *PSFolderRec;

struct SFileHandleRec
{

    dword   m_Handle;       // Логический номер файла
    dword   m_StartOffset;  // Смещение начала файла относительно начала пакетного файла
    dword   m_Offset;       // Настоящее смещение
    dword   m_Size;         // Размер файла
    byte   *m_SouBuf;       // Буфер для чтения сжатых данных
    byte   *m_DesBuf;       // Буфер для расжатых данных
    int     m_Blocknumber;  // Номер блока в сжатом файле
    bool    m_Free;         // Запись свободна
    bool    m_Compressed;   // Является ли файл сжатым
    word    dummy00;    // align
};


typedef bool (*FILENAME_CALLBACK_FUNC)(bool Dir, bool Compr, const CStr &name);
class CHsFolder;

struct SSearchRec
{
    CStr    Name = (CStr)"";
    CStr    Path = (CStr)"";
    int     Ind = 0;
    dword   T = 0;
    CHsFolder* Folder = nullptr;

    SSearchRec(CHeap* heap) : Name(heap), Path(heap) {}
};

class CHsFolder : public CMain
{
    CHeap       *m_Heap;

    CStr        m_Name;             // Имя папки без учета регистров
    CStr        m_RealName;         // Имя папки с учетом регистров
    SFolderRec  m_FolderRec;
    CHsFolder  *m_Parent;
    SFileRec   *m_Files;            // Память, связанная с файлами
    //bool        m_ToUpdate;         // Необходимо обновить информацию в пакетном файле
    //bool        m_ToSave;           // Необходимо записать на новое место в пакетный файл

    SFileRec *  GetFileRec(int ind) const // Возвращает запись по номеру
    {
        if (dword(ind)<m_FolderRec.m_Recnum)
        {
            return (SFileRec *)(((byte *)m_Files) + m_FolderRec.m_RecSize *ind);
        }
        return nullptr;
    }


    SFileRec *  GetFreeRec(void) const;
    bool        AddFileEx(const CStr &name,dword date, dword size, EFileType ftype); // Добавляет файл в пакетный файл
public:
    CHsFolder(const CStr &name, CHeap *heap);                    // Создает пустую папку, не пригодную к работе
    CHsFolder(const CStr &name,CHsFolder *Parent, CHeap *heap);  // Создает пустую папку, не пригодную к работе
    ~CHsFolder() {Clear();};                                     // Уничтожает все данные, связанные с объектом

    bool    ReadFolder(dword Handle, dword Offset); // Читает данные из пакетного файла
    //void    AllocEmptyFolder(void);

    void    Clear(void);                            // Очищает память и информацию о файлах
    bool    FileExists(const CStr &name) const;     // Существует ли указанный файл
    bool    PathExists(const CStr &name) const;     // Существует ли указанный путь
    SFileRec*   GetFileRec(const CStr &name) const;       // Возвращает запись по имени файла
    SFileRec*   GetFileRecEx(const CStr &name) const;     // Возвращает запись по пути файла
    CHsFolder*  GetFolderEx(const CStr &path);
    int         FileRecsNumber(void) const         // Возвращает количество записей в директории
    {
        return m_FolderRec.m_Recnum;
    }

    //bool        ReAllocFileRecs(int number);        // Изменяет количество записей в директории (только в сторону увеличения)
    void        UpdateFileRec(void);                // Обновляет информацию во внутренней файловой системе

    SFolderRec*  GetFolderRec(void)
    {
        return &m_FolderRec;
    }

        //*** Функции, используемые при упаковке в пакетный файл

    //bool    CreateFolder(const CStr &name);
    //bool    AddPath(const CStr &name);
    //bool    AddPath(const CStr &name,EFileType ftype);
    //bool    AddFile(const CStr &name);
    //bool    AddFile(const CStr &name,EFileType ftype);
    //bool    DeleteFile(const CStr &name);
    CStr    GetFullPath(const CStr &name);

    dword   CompressedFileSize(dword Handle, const CStr &filename);
    dword   DecompressedFileSize(dword Handle, const CStr &filename);
    int     GetSize(void)
    {
        return GetLocalSize();
    }

    int     GetLocalSize(void);
    int     GetFolderSize(void)
    {
        return sizeof(SFolderRec) + m_FolderRec.m_Recnum*m_FolderRec.m_RecSize;
    }

    //dword   Pack(dword Handle,dword Offset,int *PInt,const CStr &PStr);
    //dword   PackFile(dword Handle,const CStr &name,dword Offset);
    //dword   PackAndCompressFile(dword Handle,const CStr &name,dword Offset);

    //dword   DecodeFile(dword Handle,SFileRec *PFile,dword Offset);
    //dword   EncodeFile(dword Handle,SFileRec *PFile,dword Offset);

    //dword   CompressFile(dword SouHandle,dword Handle,const CStr &name,dword Offset);
    //dword   Compress(dword SouHandle,dword Handle,dword Offset,int *PInt,const CStr &PStr);
    //void    CompressFolder(void);

    //    //*** Функции, используемые при распаковке пакетного файла
    //bool    UnpackFile(dword SouHandle,const CStr &name);
    //bool    UnpackCompressedFile(dword SouHandle,const CStr &name);
    //bool    UnpackFolder(const CStr &name);
    //bool    Unpack(dword SouHandle,int *PInt,const CStr &PStr);

    void    ListFileNames(FILENAME_CALLBACK_FUNC Func);

        //*** Функции, используемые при установке типа файла *****
    //void    SetFileType(const CStr &name, EFileType NType);
    //void    SetFolderType(const CStr &name, EFileType NType);

    int     FindNext(SSearchRec &S);
};

#ifdef SUPPORT_IN_MEMORY_STRUCTURES
#define PFFLAG_EMPTY    SETBIT(0)       // Структура физически не связана ни с каким файлом
#endif

class CPackFile : public CMain
{
public:
    CHeap          *m_Heap;
    CWStr           m_FileName;                 // Имя пакетного файла

#ifdef SUPPORT_IN_MEMORY_STRUCTURES
    dword           m_Flags;
#endif

        // Позволяет создавать виртуальные пакеты в памяти - не связаны с файлом
    dword           m_Handle;                   // Файловый номер
    CHsFolder      *m_RootFolder;               // Корневой каталог
    SFileHandleRec  m_Handles[MAX_VIRTUAL_HANDLE_COUNT];
    dword           m_RootOffset;               // Смещение корневого каталога относительно начала файла
    //dword           m_WorkFileSize;             // Размер рабочего файла (не пакетного)
    //dword           m_WorkFileOffset;           // Смещение рабочего файла
    //dword           m_WorkFileStartOffset;      // Начальное смещение рабочего файла
    //dword           m_ID;                       // Идентификационный номер пакета в группе.
    
    int             GetFreeHandle(void);
    
    // Устанавливает указатель в файл - возвращает размер сжатого блока
    dword           SetCompressedBlockPointer(dword StartOffset,int nBlock);
public:
    CPackFile(CHeap *heap, const wchar *name);
    ~CPackFile(void);

    const CWStr GetName(void) const {return m_FileName;}

        // ******* Процедуры работы с пакетным файлом ******** //
    bool    OpenPacketFile(void);               // Открывает пакетный файл и считывает данные чтение
    bool    ClosePacketFile(void);              // Закрывает пакетный файл и уничтожает посторонние объекты чтение
    //bool    CreatePacketFileEx(void);               // Создает новый пакетный файл чтение/запись
    bool    OpenPacketFileEx(void);             // Открывает пакетный файл и считывает данные запись/чтение
    bool    ClosePacketFileEx(void)            // Закрывает пакетный файл и уничтожает посторонние объекты запись/чтение
    {
        return ClosePacketFile();
    }
    dword   GetHandle(void) const {return m_Handle;}
    void    Clear(void);
        //***** Процедуры работы с файлами -- позиционирование указателя в файл ложится на объект PackFile
    dword   Open(const CStr &filename,dword modeopen=GENERIC_READ|GENERIC_WRITE);
    bool    Close(dword Handle);
    bool    Read(dword Handle, void *buf,int Size);
#ifdef HANDLE_OUT_OF_PACK_FILES
    bool    Write(dword Handle, const void *buf,int Size);
#endif
    bool    SetPos(dword Handle, dword Pos, int ftype=FILE_BEGIN);
    dword   GetPos(dword Handle);
    dword   GetSize(dword Handle);
    dword   GetHandle(dword Handle);

        //***** Общесистемные процедуры работы с файлами ****

#ifdef HANDLE_OUT_OF_PACK_FILES
    bool    PathExists(const CStr &path);
    bool    FileExists(const CStr &path);
#else
    bool    PathExists(const CStr &path)
    {
        return m_RootFolder->PathExists(path);
    }
    bool    FileExists(const CStr &path)
    {
        return m_RootFolder->FileExists(path);
    }
#endif
    dword   CompressedFileSize(const CStr &filename)
    {
        if (m_Handle == 0xFFFFFFFF) return 0xFFFFFFFF;
        if (m_RootFolder) return m_RootFolder->CompressedFileSize(m_Handle,filename);
        return 0xFFFFFFFF;
    }

    dword   DecompressedFileSize(const CStr &filename)
    {
        if (m_Handle == 0xFFFFFFFF) return 0xFFFFFFFF;
        if (m_RootFolder) return m_RootFolder->DecompressedFileSize(m_Handle,filename);
        return 0xFFFFFFFF;
    }

        //***** Процедуры работы с пакетным файлом **********
    //bool    AddFile(const CStr &name);
    //bool    AddFile(const CStr &name,EFileType ftype);
    //bool    AddFiles(CBlockPar &block, CStr &log);
    //bool    AddFiles(CBlockPar &block);
    //bool    AddPath(const CStr &name);
    //bool    AddPath(const CStr &name,EFileType ftype);
    //bool    DeleteFile(const CStr &name);
    //bool    CreateFolder(const CStr &name);

    //bool    Compress(int *PInt,CStr &PStr);
    //bool    Compress(const CStr &name,int *PInt,CStr &PStr);
    //bool    Pack(int *PInt,CStr &PStr);
    //bool    Unpack(int *PInt,CStr &PStr);
    //bool    Compress(void);
    //bool    Compress(const CStr &name);
    //bool    Pack(void);
    //bool    Unpack(void);
    //bool    UnpackFile(const CStr &souname);
    //bool    UnpackFile(const CStr &souname,const CStr &desname);

    void    ListFileNames(FILENAME_CALLBACK_FUNC Func)
    {
        if (m_Handle == 0xFFFFFFFF) return;
        if (m_RootFolder)
        {
            m_RootFolder->ListFileNames(Func);
        }
    }

        //*** Функции, используемые при установке типа файла *****
    //void    SetFileType(const CStr &name, EFileType NType)
    //{
    //    if (m_RootFolder == nullptr) return;
    //    if (m_Handle == 0xFFFFFFFF) return;
    //    m_RootFolder->SetFileType(name, NType);
    //}
    //void    SetFolderType(const CStr &name, EFileType NType)
    //{
    //    if (m_RootFolder == nullptr) return;
    //    if (m_Handle == 0xFFFFFFFF) return;
    //    m_RootFolder->SetFolderType(name, NType);
    //}



    int     FindFirst(const CStr &path,dword Attr, SSearchRec &S);
    int     FindNext(SSearchRec &S)
    {
        if (m_RootFolder == nullptr) return 1;
        if (S.Folder == nullptr) return 1;
        return S.Folder->FindNext(S);
    }
    void    FindClose(SSearchRec &S)
    {
        S.Folder = nullptr;
        S.Path = "";
        S.Name = "";
    }
};

typedef CPackFile        *PCPackFile;

class CPackCollection : public CMain
{
public:
    CHeap           *m_Heap;
    CBuf             m_PackFiles;

public:
    CPackCollection(CHeap *heap):m_Heap(heap), m_PackFiles(heap) {}
    ~CPackCollection() { Clear(); };

    //******** Процедуры работы со списком пакетных файлом ********//
    void    Clear(void);
    void    AddPacketFile(const wchar *filename);
    void    DelPacketFile(const wchar *filename);
    //******** Процедуры для работы с пакетными файлами ***********//
    bool        OpenPacketFiles(void);
    bool        ClosePacketFiles(void);
    bool        OpenPacketFilesEx(void);
    bool        ClosePacketFilesEx(void);
    CPackFile*  GetPacketFile(int i) {return m_PackFiles.Buff<PCPackFile>()[i];};
    //******** Процедуры для работы файлами ***********//
    bool        FileExists(const CStr &name);
    bool        PathExists(const CStr &path);
    //******* работа с виртуальными номерами объектов CPackFile
    dword       Open(const CStr&name, dword modeopen=GENERIC_READ);
    bool        Close(dword Handle);
    bool        Read(dword Handle, void *Buf, int Size);
    //bool        Write(dword Handle, const void *Buf, int Size);
    bool        SetPos(dword Handle, dword Pos, int ftype=FILE_BEGIN);
    dword       GetPos(dword Handle) ;
    dword       GetSize(dword Handle) ;
    dword       GetHandle(dword Handle);
    //******* работа с извлекаемыми файлами *********************
    //bool        UnpackFile(const CStr&souname) {UnpackFile(souname,souname);};
    //bool        UnpackFile(const CStr&souname,const CStr&desname);

};


} // namespace Base
#endif