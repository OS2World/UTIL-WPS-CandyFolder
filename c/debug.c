
/*
 *@@sourcefile debug.c:
 *      this file contains debugging functions for the XFolder
 *      exception handlers. When exceptions occur with the
 *      "loud" exception handler (excHandlerLoud, except.c),
 *      we jump here to try to find out where the exception
 *      occured, either by analyzing the DLL's debug code,
 *      or, if none exists, by trying to read a SYM file.
 *
 *@@include #define INCL_BASE
 *@@include #include <os2.h>
 *@@include #include <stdio.h>
 *@@include #include "debug.h"
 */

/*
 *      This file incorporates code from the following:
 *      -- Marc Fiammante, John Currier, Kim Rasmussen,
 *         Anthony Cruise (EXCEPT3.ZIP package for a generic
 *         exception handling DLL, available at Hobbes).
 *
 *      This file Copyright (C) 1992-99 Ulrich M봪ler,
 *                                      Kim Rasmussen,
 *                                      Marc Fiammante,
 *                                      John Currier,
 *                                      Anthony Cruise.
 *      This file is part of the XFolder source package.
 *      XFolder is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published
 *      by the Free Software Foundation, in version 2 as it comes in the
 *      "COPYING" file of the XFolder main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */
/*
 * Changes for CandyFolder (C) Chris Wohlgemuth 2001
 * This file is part of the CandyFolder package
 *
 * Visit
 *
 * http://candybarz.netlabs.org
 *
 * or
 *
 * http://www.geocities.com/SiliconValley/Sector/5785/
 *
 * for more information
 *
 */
#pragma info(none)
#define INCL_BASE
#include <os2.h>

#include <exe.h>
#include <newexe.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"

// avoid define conflicts between newexe.h and exe386.h
#define  FOR_EXEHDR  1
#ifndef DWORD
#define DWORD long int
#endif

#ifndef WORD
#define WORD  short int
#endif

#include <exe386.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <share.h>
#include <io.h>

#ifndef DWORD
#define DWORD unsigned long
#endif
#ifndef WORD
#define WORD  unsigned short
#endif

#pragma info(restore)

#pragma stack16(512)
#define HF_STDERR 2

/* ******************************************************************
 *                                                                  *
 *   Global variables                                               *
 *                                                                  *
 ********************************************************************/

UCHAR          *ProcessName = "DEBUGGEE.EXE";
FILE           *hTrap;

static BOOL     f32bit = TRUE;

struct debug_buffer {
    ULONG           Pid;        // Debuggee Process ID
    ULONG           Tid;        // Debuggee Thread ID
    LONG            Cmd;        // Command or Notification
    LONG            Value;      // Generic Data Value
    ULONG           Addr;       // Debuggee Address
    ULONG           Buffer;     // Debugger Buffer Address
    ULONG           Len;        // Length of Range
    ULONG           Index;      // Generic Identifier Index
    ULONG           MTE;        // Module Handle
    ULONG           EAX;        // Register Set
    ULONG           ECX;
    ULONG           EDX;
    ULONG           EBX;
    ULONG           ESP;
    ULONG           EBP;
    ULONG           ESI;
    ULONG           EDI;
    ULONG           EFlags;
    ULONG           EIP;
    ULONG           CSLim;      // Byte Granular Limits
    ULONG           CSBase;     // Byte Granular Base
    UCHAR           CSAcc;      // Access Bytes
    UCHAR           CSAtr;      // Attribute Bytes
    USHORT          CS;
    ULONG           DSLim;
    ULONG           DSBase;
    UCHAR           DSAcc;
    UCHAR           DSAtr;
    USHORT          DS;
    ULONG           ESLim;
    ULONG           ESBase;
    UCHAR           ESAcc;
    UCHAR           ESAtr;
    USHORT          ES;
    ULONG           FSLim;
    ULONG           FSBase;
    UCHAR           FSAcc;
    UCHAR           FSAtr;
    USHORT          FS;
    ULONG           GSLim;
    ULONG           GSBase;
    UCHAR           GSAcc;
    UCHAR           GSAtr;
    USHORT          GS;
    ULONG           SSLim;
    ULONG           SSBase;
    UCHAR           SSAcc;
    UCHAR           SSAtr;
    USHORT          SS;
} DbgBuf;

// (*UM)
#ifdef DBG_O_OBJMTE
    #undef DBG_O_OBJMTE
#endif

#define DBG_O_OBJMTE       0x10000000L
#define DBG_C_NumToAddr            13
#define DBG_C_AddrToObject         28
#define DBG_C_Connect              21
#define DBG_L_386                   1

RESULTCODES     ReturnCodes;
UCHAR           LoadError[40];  /*DosExecPGM buffer */
USHORT          Passes;
UCHAR           Translate[17];
UCHAR           OldStuff[16];

ULONG           func_ofs;
ULONG           pubfunc_ofs;
char            func_name[128];
ULONG           var_ofs = 0;

struct {
    BYTE            name[128];
    ULONG           stack_offset;
    USHORT          type_idx;
} autovar_def[100];

HMODULE         hMod;
ULONG           ObjNum;
ULONG           Offset;

/*-------------------------------------*/
CHAR            Buffer[CCHMAXPATH];

#pragma pack(1)

//  Global Data Section
typedef struct qsGrec_s
{
    ULONG           cThrds;     // number of threads in use
    ULONG           Reserved1;
    ULONG           Reserved2;
}
qsGrec_t;

// Thread Record structure *   Holds all per thread information.
typedef struct qsTrec_s
{
    ULONG           RecType;    // Record Type
    // Thread rectype = 100
    USHORT          tid;        // thread ID
    USHORT          slot;       // "unique" thread slot number
    ULONG           sleepid;    // sleep id thread is sleeping on
    ULONG           priority;   // thread priority
    ULONG           systime;    // thread system time
    ULONG           usertime;   // thread user time
    UCHAR           state;      // thread state
    UCHAR           PADCHAR;
    USHORT          PADSHORT;
}
qsTrec_t;

// Process and Thread Data Section
typedef struct qsPrec_s
{
    ULONG           RecType;    // type of record being processed
    // process rectype = 1
    qsTrec_t       *pThrdRec;   // ptr to 1st thread rec for this prc
    USHORT          pid;        // process ID
    USHORT          ppid;       // parent process ID
    ULONG           type;       // process type
    ULONG           stat;       // process status
    ULONG           sgid;       // process screen group
    USHORT          hMte;       // program module handle for process
    USHORT          cTCB;       // # of TCBs in use in process
    ULONG           Reserved1;
    void           *Reserved2;
    USHORT          c16Sem;     /*# of 16 bit system sems in use by proc */
    USHORT          cLib;       // number of runtime linked libraries
    USHORT          cShrMem;    // number of shared memory handles
    USHORT          Reserved3;
    USHORT         *p16SemRec;  /*ptr to head of 16 bit sem inf for proc */
    USHORT         *pLibRec;    /*ptr to list of runtime lib in use by */
    /*process */
    USHORT         *pShrMemRec; /*ptr to list of shared mem handles in */
    /*use by process */
    USHORT         *Reserved4;
}
qsPrec_t;

// 16 Bit System Semaphore Section
typedef struct qsS16Headrec_s
{
    ULONG           RecType;    // semaphore rectype = 3
    ULONG           Reserved1;  // overlays NextRec of 1st qsS16rec_t
    ULONG           Reserved2;
    ULONG           S16TblOff;  // index of first semaphore,SEE PSTAT OUTPUT
    // System Semaphore Information Section
}
qsS16Headrec_t;

//  16 bit System Semaphore Header Record Structure
typedef struct qsS16rec_s
{
    ULONG           NextRec;    // offset to next record in buffer
    UINT            s_SysSemOwner;  // thread owning this semaphore
    UCHAR           s_SysSemFlag;   // system semaphore flag bit field
    UCHAR           s_SysSemRefCnt;     // number of references to this
    //  system semaphore
    UCHAR           s_SysSemProcCnt;    /*number of requests by sem owner  */
    UCHAR           Reserved1;
    ULONG           Reserved2;
    UINT            Reserved3;
    CHAR            SemName[1]; // start of semaphore name string
}
qsS16rec_t;

//  Executable Module Section
typedef struct qsLrec_s
{
    void           *pNextRec;   // pointer to next record in buffer
    USHORT          hmte;       // handle for this mte
    USHORT          Reserved1;  // Reserved
    ULONG           ctImpMod;   // # of imported modules in table
    ULONG           Reserved2;  // Reserved
//  qsLObjrec_t * Reserved3;       Reserved
    ULONG          *Reserved3;  // Reserved
    UCHAR          *pName;      // ptr to name string following stru
}
qsLrec_t;

//  Shared Memory Segment Section
typedef struct qsMrec_s
{
    struct qsMrec_s *MemNextRec;    // offset to next record in buffer
    USHORT          hmem;       // handle for shared memory
    USHORT          sel;        // shared memory selector
    USHORT          refcnt;     // reference count
    CHAR            Memname[1]; // start of shared memory name string
}
qsMrec_t;

//  Pointer Record Section
typedef struct qsPtrRec_s
{
    qsGrec_t       *pGlobalRec; // ptr to the global data section
    qsPrec_t       *pProcRec;   // ptr to process record section
    qsS16Headrec_t *p16SemRec;  // ptr to 16 bit sem section
    void           *Reserved;   // a reserved area
    qsMrec_t       *pShrMemRec; // ptr to shared mem section
    qsLrec_t       *pLibRec;    /*ptr to exe module record section */
}
qsPtrRec_t;

/*-------------------------*/
ULONG          *pBuf, *pTemp;
USHORT          Selector;
qsPtrRec_t     *pRec;
qsLrec_t       *pLib;
qsMrec_t       *pShrMemRec;     // ptr to shared mem section
qsPrec_t       *pProc;
qsTrec_t       *pThread;
ULONG           ListedThreads = 0;
APIRET16 APIENTRY16 DOS16ALLOCSEG(
                        USHORT cbSize,  // number of bytes requested
                 USHORT * _Seg16 pSel,  // sector allocated (returned)
                      USHORT fsAlloc);  // sharing attributes of the allocated segment
typedef struct
{
    short int       ilen;       // Instruction length
    long            rref;       // Value of any IP relative storage reference
    unsigned short  sel;        // Selector of any CS:eIP storage reference.
    long            poff;       // eIP value of any CS:eIP storage reference.
    char            longoper;   // YES/NO value. Is instr in 32 bit operand mode? *
    char            longaddr;   // YES/NO value. Is instr in 32 bit address mode? *
    char            buf[40];    // String holding disassembled instruction *
} *_Seg16 RETURN_FROM_DISASM;

RETURN_FROM_DISASM CDECL16 DISASM(CHAR * _Seg16 Source, USHORT IPvalue, USHORT segsize);
RETURN_FROM_DISASM AsmLine;
char           *SourceBuffer = 0;
static USHORT   BigSeg;
static ULONG    Version[2];

BYTE           *type_name[] =
{
    "8 bit signed                     ",
    "16 bit signed                    ",
    "32 bit signed                    ",
    "Unknown (0x83)                   ",
    "8 bit unsigned                   ",
    "16 bit unsigned                  ",
    "32 bit unsigned                  ",
    "Unknown (0x87)                   ",
    "32 bit real                      ",
    "64 bit real                      ",
    "80 bit real                      ",
    "Unknown (0x8B)                   ",
    "64 bit complex                   ",
    "128 bit complex                  ",
    "160 bit complex                  ",
    "Unknown (0x8F)                   ",
    "8 bit boolean                    ",
    "16 bit boolean                   ",
    "32 bit boolean                   ",
    "Unknown (0x93)                   ",
    "8 bit character                  ",
    "16 bit characters                ",
    "32 bit characters                ",
    "void                             ",
    "15 bit unsigned                  ",
    "24 bit unsigned                  ",
    "31 bit unsigned                  ",
    "Unknown (0x9B)                   ",
    "Unknown (0x9C)                   ",
    "Unknown (0x9D)                   ",
    "Unknown (0x9E)                   ",
    "Unknown (0x9F)                   ",
    "near pointer to 8 bit signed     ",
    "near pointer to 16 bit signed    ",
    "near pointer to 32 bit signed    ",
    "Unknown (0xA3)                   ",
    "near pointer to 8 bit unsigned   ",
    "near pointer to 16 bit unsigned  ",
    "near pointer to 32 bit unsigned  ",
    "Unknown (0xA7)                   ",
    "near pointer to 32 bit real      ",
    "near pointer to 64 bit real      ",
    "near pointer to 80 bit real      ",
    "Unknown (0xAB)                   ",
    "near pointer to 64 bit complex   ",
    "near pointer to 128 bit complex  ",
    "near pointer to 160 bit complex  ",
    "Unknown (0xAF)                   ",
    "near pointer to 8 bit boolean    ",
    "near pointer to 16 bit boolean   ",
    "near pointer to 32 bit boolean   ",
    "Unknown (0xB3)                   ",
    "near pointer to 8 bit character  ",
    "near pointer to 16 bit characters",
    "near pointer to 32 bit characters",
    "near pointer to void             ",
    "near pointer to 15 bit unsigned  ",
    "near pointer to 24 bit unsigned  ",
    "near pointer to 31 bit unsigned  ",
    "Unknown (0xBB)                   ",
    "Unknown (0xBC)                   ",
    "Unknown (0xBD)                   ",
    "Unknown (0xBE)                   ",
    "Unknown (0xBF)                   ",
    "far pointer to 8 bit signed      ",
    "far pointer to 16 bit signed     ",
    "far pointer to 32 bit signed     ",
    "Unknown (0xC3)                   ",
    "far pointer to 8 bit unsigned    ",
    "far pointer to 16 bit unsigned   ",
    "far pointer to 32 bit unsigned   ",
    "Unknown (0xC7)                   ",
    "far pointer to 32 bit real       ",
    "far pointer to 64 bit real       ",
    "far pointer to 80 bit real       ",
    "Unknown (0xCB)                   ",
    "far pointer to 64 bit complex    ",
    "far pointer to 128 bit complex   ",
    "far pointer to 160 bit complex   ",
    "Unknown (0xCF)                   ",
    "far pointer to 8 bit boolean     ",
    "far pointer to 16 bit boolean    ",
    "far pointer to 32 bit boolean    ",
    "Unknown (0xD3)                   ",
    "far pointer to 8 bit character   ",
    "far pointer to 16 bit characters ",
    "far pointer to 32 bit characters ",
    "far pointer to void              ",
    "far pointer to 15 bit unsigned   ",
    "far pointer to 24 bit unsigned   ",
    "far pointer to 31 bit unsigned   ",
};

typedef _PFN16 *_Seg16 PFN16;

// Thanks to John Currier :
// Do not call 16 bit code in myHandler function to prevent call
// to __EDCThunkProlog and problems is guard page exception handling
// Also reduce the stack size to 1K for true 16 bit calls.
// 16 bit calls thunk will now only occur on fatal exceptions
#pragma stack16(1024)

// ------------------------------------------------------------------
// Last 8 bytes of 16:16 file when CODEVIEW debugging info is present
#pragma pack(1)
struct _eodbug
{
    unsigned short  dbug;       // 'NB' signature
    unsigned short  ver;        // version
    unsigned long   dfaBase;    // size of codeview info
}
eodbug;

#define         DBUGSIG         0x424E
#define         SSTMODULES      0x0101
#define         SSTPUBLICS      0x0102
#define         SSTTYPES        0x0103
#define         SSTSYMBOLS      0x0104
#define         SSTSRCLINES     0x0105
#define         SSTLIBRARIES    0x0106
#define         SSTSRCLINES2    0x0109
#define         SSTSRCLINES32   0x010B

struct _base
{
    unsigned short  dbug;       // 'NB' signature
    unsigned short  ver;        // version
    unsigned long   lfoDir;     // file offset to dir entries
}
base;

struct ssDir
{
    unsigned short  sst;        // SubSection Type
    unsigned short  modindex;   // Module index number
    unsigned long   lfoStart;   // Start of section
    unsigned short  cb;         // Size of section
};

struct ssDir32
{
    unsigned short  sst;        // SubSection Type
    unsigned short  modindex;   // Module index number
    unsigned long   lfoStart;   // Start of section
    unsigned long   cb;         // Size of section
};

struct ssModule
{
    unsigned short  csBase;     // code segment base
    unsigned short  csOff;      // code segment offset
    unsigned short  csLen;      // code segment length
    unsigned short  ovrNum;     // overlay number
    unsigned short  indxSS;     // Index into sstLib or 0
    unsigned short  reserved;
    char            csize;      // size of prefix string
}
ssmod;

struct ssModule32
{
    unsigned short  csBase;     // code segment base
    unsigned long   csOff;      // code segment offset
    unsigned long   csLen;      // code segment length
    unsigned long   ovrNum;     // overlay number
    unsigned short  indxSS;     // Index into sstLib or 0
    unsigned long   reserved;
    char            csize;      // size of prefix string
}
ssmod32;

struct ssPublic
{
    unsigned short  offset;
    unsigned short  segment;
    unsigned short  type;
    char            csize;
}
sspub;

struct ssPublic32
{
    unsigned long   offset;
    unsigned short  segment;
    unsigned short  type;
    char            csize;
}
sspub32;

typedef struct _SSLINEENTRY32
{
    unsigned short  LineNum;
    unsigned short  FileNum;
    unsigned long   Offset;
}
SSLINEENTRY32;
typedef struct _FIRSTLINEENTRY32
{
    unsigned short  LineNum;
    unsigned char   entry_type;
    unsigned char   reserved;
    unsigned short  numlines;
    unsigned short  segnum;
}
FIRSTLINEENTRY32;

typedef struct _SSFILENUM32
{
    unsigned long   first_displayable;  // Not used
    unsigned long   number_displayable;     // Not used
    unsigned long   file_count; // number of source files
}
SSFILENUM32;

struct DbugRec
{                               // debug info struct ure used in linked * list
    struct DbugRec /* far */ *pnext;  /* next node *//* 013 */
    char /* far */       *SourceFile; /* source file name *013 */
    unsigned short  TypeOfProgram;  // dll or exe *014*
    unsigned short  LineNumber; // line number in source file
    unsigned short  OffSet;     // offset into loaded module
    unsigned short  Selector;   // code segment 014
    unsigned short  OpCode;     // Opcode replaced with BreakPt
    unsigned long   Count;      // count over Break Point
};

typedef struct DbugRec DBUG, /* far */ * DBUGPTR;     /* 013 */

// buffers for Read... funcs
char            szNrFile[300];
char            szNrLine[300];
char            szNrPub[300];

struct new_seg *pseg;
struct o32_obj *pobj;           // Flat .EXE object table entry
struct ssDir   *pDirTab;
struct ssDir32 *pDirTab32;
unsigned char  *pEntTab;
unsigned long   lfaBase;

#pragma pack()

/* ******************************************************************
 *                                                                  *
 *   PART 1: ANALYZE DEBUG CODE                                     *
 *                                                                  *
 ********************************************************************/

int Read16CodeView(FILE *LogFile, int fh, int TrapSeg, int TrapOff, CHAR *FileName);
int Read32PmDebug(FILE *LogFile, int fh, int TrapSeg, int TrapOff, CHAR *FileName);

/*
 *@@ dbgPrintDebugInfo:
 *      this is the main entry point into analyzing debug
 *      code.
 *      This analyzes a given address and tries to find
 *      debug code descriptions for this address.
 *      This returns NO_ERROR if the could was successfully
 *      analyzed or something != 0 if we failed.
 *
 *      New with V0.84.
 */

APIRET dbgPrintDebugInfo(FILE *LogFile,   // out: log file to write to
                     CHAR *FileName,        // in: module file name
                     ULONG Object,          // in: trapping object
                     ULONG TrapOffset)      // in: trapping address
{
    APIRET                  rc;
    int                     ModuleFile;
    static struct exe_hdr   OldExeHeader;
    static struct new_exe   NewExeHeader;
    static struct e32_exe   e32;

    /* strcpy(szNrFile, "............");
    strcpy(szNrPub, "...N/A.");
    strcpy(szNrLine, "...N/A.."); */

    // open the module file to analyze the code
    ModuleFile = sopen(FileName, O_RDONLY | O_BINARY, SH_DENYNO);

    if (ModuleFile != -1)
    {
        // file found:
        // read old Exe header
        if (read(ModuleFile, (void *)&OldExeHeader, 64) == -1L)
        {
            fprintf(LogFile, "Error %d reading old exe header\n", errno);
            close(ModuleFile);
            return 2;
        }
        // seek to new Exe header
        if (lseek(ModuleFile, (long)E_LFANEW(OldExeHeader), SEEK_SET) == -1L)
        {
            fprintf(LogFile, "Error %d seeking to new exe header\n", errno);
            close(ModuleFile);
            return 3;
        }
        if (read(ModuleFile, (void *)&NewExeHeader, 64) == -1L)
        {
            fprintf(LogFile, "Error %d reading new exe header\n", errno);
            close(ModuleFile);
            return 4;
        }

        // check EXE signature
        if (NE_MAGIC(NewExeHeader) == E32MAGIC)
        {
            /*
             * flat 32 executable:
             *
             */

            // do analysis for 32-bit code
            rc = Read32PmDebug(LogFile, ModuleFile, Object + 1, TrapOffset, FileName);
            if (rc == 0)
            {
                fprintf(LogFile, "%s", szNrFile);
                fprintf(LogFile, "%s", szNrLine);
                fprintf(LogFile, "%s", szNrPub);
            }                   // endif
            close(ModuleFile);

            // rc !=0 try with DBG file
            if (rc != 0)
            {
                strcpy(FileName + strlen(FileName) - 3, "DBG");     // Build DBG File name
                ModuleFile = sopen(FileName, O_RDONLY | O_BINARY, SH_DENYNO);
                if (ModuleFile != -1)
                {
                    rc = Read32PmDebug(LogFile, ModuleFile, Object + 1, TrapOffset, FileName);
                    if (rc == 0)
                    {
                        fprintf(LogFile, "%s", szNrFile);
                        fprintf(LogFile, "%s", szNrLine);
                        fprintf(LogFile, "%s", szNrPub);
                    }           // endif
                    close(ModuleFile);
                }
            }                   // endif
            return rc;
        }
        else
        {
            if (NE_MAGIC(NewExeHeader) == NEMAGIC)
            {
                /*
                 * 16:16 executable:
                 *
                 */

                if ((pseg = (struct new_seg *)calloc(NE_CSEG(NewExeHeader), sizeof(struct new_seg))) == NULL)
                {
                    fprintf(LogFile, "Out of memory!");
                    close(ModuleFile);
                    return -1;
                }
                if (lseek(ModuleFile, E_LFANEW(OldExeHeader) + NE_SEGTAB(NewExeHeader), SEEK_SET) == -1L)
                {
                    fprintf(LogFile, "Error %u seeking segment table in %s\n", errno, FileName);
                    free(pseg);
                    close(ModuleFile);
                    return 9;
                }

                if (read(ModuleFile, (void *)pseg, NE_CSEG(NewExeHeader) * sizeof(struct new_seg)) == -1)
                {
                    fprintf(LogFile, "Error %u reading segment table from %s\n", errno, FileName);
                    free(pseg);
                    close(ModuleFile);
                    return 10;
                }
                rc = Read16CodeView(LogFile, ModuleFile, Object + 1, TrapOffset, FileName);
                if (rc == 0)
                {
                    fprintf(LogFile, "%s", szNrFile);
                    fprintf(LogFile, "%s", szNrLine);
                    fprintf(LogFile, "%s", szNrPub);
                }               // endif
                free(pseg);
                close(ModuleFile);
                // rc !=0 try with DBG file
                if (rc != 0)
                {
                    strcpy(FileName + strlen(FileName) - 3, "DBG");     // Build DBG File name
                    ModuleFile = sopen(FileName, O_RDONLY | O_BINARY, SH_DENYNO);
                    if (ModuleFile != -1)
                    {
                        rc = Read16CodeView(LogFile, ModuleFile, Object + 1, TrapOffset, FileName);
                        if (rc == 0)
                        {
                            fprintf(LogFile, "%s", szNrFile);
                            fprintf(LogFile, "%s", szNrLine);
                            fprintf(LogFile, "%s", szNrPub);
                        }       // endif
                        close(ModuleFile);
                    }
                }               // endif
                return rc;

            }
            else
            {
                /*
                 * Unknown executable:
                 *
                 */

                fprintf(LogFile, "Error, could not find exe signature");
                close(ModuleFile);
                return 11;
            }
        }
        // Read new Exe header
    }
    else
    {
        fprintf(LogFile, "Error %d opening module file %s", errno, FileName);
        return 1;
    }                           // endif
    return 0;
}

char            fname[128], ModName[80];
char            ename[128], dummy[128];

#define MAX_USERDEFS 150
#define MAX_POINTERS 150

USHORT          userdef_count;
USHORT          pointer_count;

struct one_userdef_rec
{
    USHORT          idx;
    USHORT          type_index;
    BYTE            name[33];
}
one_userdef[MAX_USERDEFS];

struct one_pointer_rec
{
    USHORT          idx;
    USHORT          type_index;
    BYTE            type_qual;
    BYTE            name[33];
}
one_pointer[MAX_POINTERS];

/*
 * Read32PmDebug:
 *
 */

int Read32PmDebug(FILE *LogFile,        // out: log file to write to
                  int ModuleFile,       // in: module file opened with sopen()
                  int TrapSeg,
                  int TrapOff,
                  CHAR *FileName)
{
    static unsigned int CurrSymSeg, NrSymbol,
                    offset, NrPublic,
                    NrFile, NrLine, NrEntry,
                    numdir, namelen,
                    numlines, line;
    static int      ModIndex;
    static int      bytesread, i, j;
    static int      pOffset;
    static SSLINEENTRY32 LineEntry;
    static SSFILENUM32 FileInfo;
    static FIRSTLINEENTRY32 FirstLine;
    static BYTE     dump_vars = FALSE;
    static USHORT   idx;
    static BOOL     read_types;
    static LONG     lSize;
    static BOOL     is_new_debug;

    ModIndex = 0;
    // See if any CODEVIEW info
    if (lseek(ModuleFile, -8L, SEEK_END) == -1)
    {
        fprintf(LogFile, "Error %u seeking CodeView table in %s\n", errno, FileName);
        return (18);
    }

    if (read(ModuleFile, (void *)&eodbug, 8) == -1)
    {
        fprintf(LogFile, "Error %u reading debug info from %s\n", errno, FileName);
        return (19);
    }
    if (eodbug.dbug != DBUGSIG)
    {
        /*fprintf(LogFile,"\nNo CodeView information stored.\n"); */
        return (100);
    }

    if ((lfaBase = lseek(ModuleFile, -eodbug.dfaBase, SEEK_END)) == -1L)
    {
        fprintf(LogFile, "Error %u seeking base codeview data in %s\n", errno, FileName);
        return (20);
    }

    if (read(ModuleFile, (void *)&base, 8) == -1)
    {
        fprintf(LogFile, "Error %u reading base codeview data in %s\n", errno, FileName);
        return (21);
    }

    if (lseek(ModuleFile, base.lfoDir - 8 + 4, SEEK_CUR) == -1)
    {
        fprintf(LogFile, "Error %u seeking dir codeview data in %s\n", errno, FileName);
        return (22);
    }

    if (read(ModuleFile, (void *)&numdir, 4) == -1)
    {
        fprintf(LogFile, "Error %u reading dir codeview data in %s\n", errno, FileName);
        return (23);
    }

    // Read dir table into buffer
    if ((pDirTab32 = (struct ssDir32 *)calloc(numdir, sizeof(struct ssDir32))) == NULL)
    {
        fprintf(LogFile, "Out of memory!");
        return (-1);
    }

    if (read(ModuleFile, (void *)pDirTab32, numdir * sizeof(struct ssDir32)) == -1)
    {
        fprintf(LogFile, "Error %u reading codeview dir table from %s\n", errno, FileName);
        free(pDirTab32);
        return (24);
    }

    i = 0;
    while (i < numdir)
    {
        if (pDirTab32[i].sst != SSTMODULES)
        {
            i++;
            continue;
        }
        NrPublic = 0x0;
        NrSymbol = 0;
        NrLine = 0x0;
        NrFile = 0x0;
        CurrSymSeg = 0;
        // point to subsection
        lseek(ModuleFile, pDirTab32[i].lfoStart + lfaBase, SEEK_SET);
        read(ModuleFile, (void *)&ssmod32.csBase, sizeof(ssmod32));
        read(ModuleFile, (void *)ModName, (unsigned)ssmod32.csize);
        ModIndex = pDirTab32[i].modindex;
        ModName[ssmod32.csize] = '\0';
        i++;

        read_types = FALSE;

        while (pDirTab32[i].modindex == ModIndex && i < numdir)
        {
            // point to subsection
            lseek(ModuleFile, pDirTab32[i].lfoStart + lfaBase, SEEK_SET);
            switch (pDirTab32[i].sst)
            {
                case SSTPUBLICS:
                    bytesread = 0;
                    while (bytesread < pDirTab32[i].cb)
                    {
                        bytesread += read(ModuleFile, (void *)&sspub32.offset, sizeof(sspub32));
                        bytesread += read(ModuleFile, (void *)ename, (unsigned)sspub32.csize);
                        ename[sspub32.csize] = '\0';
                        if ((sspub32.segment == TrapSeg) &&
                            (sspub32.offset <= TrapOff) &&
                            (sspub32.offset >= NrPublic))
                        {
                            NrPublic = pubfunc_ofs = sspub32.offset;
                            read_types = TRUE;
                            sprintf(szNrPub, "%s %s (%s) %04X:%08X\n",
                                    (sspub32.type == 1) ? " Abs" : " ", ename,
                                    ModName, // ()
                                    sspub32.segment, sspub32.offset
                                );
                            // but continue, because there might be a
                            // symbol that comes closer
                        }
                    }
                    break;

                // Read symbols, so we can dump the variables on the stack
                case SSTSYMBOLS:
                    if (TrapSeg != ssmod32.csBase)
                        break;

                    bytesread = 0;
                    while (bytesread < pDirTab32[i].cb)
                    {
                        static USHORT   usLength;
                        static BYTE     b1,
                                        b2;
                        static BYTE     bType,
                                       *ptr;
                        static ULONG    ofs;
                        static ULONG    last_addr = 0;
                        static BYTE     str[256];
                        static struct symseg_rec symseg;
                        static struct symauto_rec symauto;
                        static struct symproc_rec symproc;

                        // Read the length of this subentry
                        bytesread += read(ModuleFile, &b1, 1);
                        if (b1 & 0x80)
                        {
                            bytesread += read(ModuleFile, &b2, 1);
                            usLength = ((b1 & 0x7F) << 8) + b2;
                        }
                        else
                            usLength = b1;

                        ofs = tell(ModuleFile);

                        bytesread += read(ModuleFile, &bType, 1);

                        switch (bType)
                        {
                            case SYM_CHANGESEG:
                                read(ModuleFile, &symseg, sizeof(symseg));
                                CurrSymSeg = symseg.seg_no;
                                break;

                            case SYM_PROC:
                            case SYM_CPPPROC:
                                read(ModuleFile, &symproc, sizeof(symproc));
                                read(ModuleFile, str, symproc.name_len);
                                str[symproc.name_len] = 0;

                                if ((CurrSymSeg == TrapSeg) &&
                                    (symproc.offset <= TrapOff) &&
                                    (symproc.offset >= NrSymbol))
                                {

                                    dump_vars = TRUE;
                                    var_ofs = 0;
                                    NrSymbol = symproc.offset;
                                    func_ofs = symproc.offset;

                                    strcpy(func_name, str);
                                }
                                else
                                {
                                    dump_vars = FALSE;
                                }
                                break;

                            case SYM_AUTO:
                                if (!dump_vars)
                                    break;

                                read(ModuleFile, &symauto, sizeof(symauto));
                                read(ModuleFile, str, symauto.name_len);
                                str[symauto.name_len] = 0;

                                strcpy(autovar_def[var_ofs].name, str);
                                autovar_def[var_ofs].stack_offset = symauto.stack_offset;
                                autovar_def[var_ofs].type_idx = symauto.type_idx;
                                var_ofs++;
                                break;

                        }

                        bytesread += usLength;

                        lseek(ModuleFile, ofs + usLength, SEEK_SET);
                    }
                    break;

                case SSTTYPES:
//               if (ModIndex != TrapSeg)
                    if (!read_types)
                        break;

                    bytesread = 0;
                    idx = 0x200;
                    userdef_count = 0;
                    pointer_count = 0;
                    while (bytesread < pDirTab32[i].cb)
                    {
                        static struct type_rec type;
                        static struct type_userdefrec udef;
                        static struct type_pointerrec point;
                        static struct type_funcrec func;
                        static struct type_structrec struc;
                        static struct type_list1 list1;
                        static struct type_list2 list2;
                        static struct type_list2_1 list2_1;
                        static ULONG    ofs;
                        static BYTE     str[256],
                                        b1,
                                        b2;
                        static USHORT   n;

                        // Read the length of this subentry
                        ofs = tell(ModuleFile);

                        read(ModuleFile, &type, sizeof(type));
                        bytesread += sizeof(type);

                        switch (type.type)
                        {
                            case TYPE_USERDEF:
                                if (userdef_count > MAX_USERDEFS)
                                    break;

                                read(ModuleFile, &udef, sizeof(udef));
                                read(ModuleFile, str, udef.name_len);
                                str[udef.name_len] = 0;

                                // Insert userdef in table
                                one_userdef[userdef_count].idx = idx;
                                one_userdef[userdef_count].type_index = udef.type_index;
                                memcpy(one_userdef[userdef_count].name, str, min(udef.name_len + 1, 32));
                                one_userdef[userdef_count].name[32] = 0;
                                userdef_count++;
                                break;

                            case TYPE_POINTER:
                                if (pointer_count > MAX_POINTERS)
                                    break;

                                read(ModuleFile, &point, sizeof(point));
                                read(ModuleFile, str, point.name_len);
                                str[point.name_len] = 0;

                                // Insert userdef in table
                                one_pointer[pointer_count].idx = idx;
                                one_pointer[pointer_count].type_index = point.type_index;
                                memcpy(one_pointer[pointer_count].name, str, min(point.name_len + 1, 32));
                                one_pointer[pointer_count].name[32] = 0;
                                one_pointer[pointer_count].type_qual = type.type_qual;
                                pointer_count++;
                                break;
                        }

                        ++idx;

                        bytesread += type.length;

                        lseek(ModuleFile, ofs + type.length + 2, SEEK_SET);
                    }
                    break;

                case SSTSRCLINES32:
                    if (TrapSeg != ssmod32.csBase)
                        break;

                    // read first line
                    do
                    {
                        read(ModuleFile, (void *)&FirstLine, sizeof(FirstLine));

                        if (FirstLine.LineNum != 0)
                        {
                            fprintf(LogFile, "Missing Line table information\n");
                            break;
                        }       // endif
                        numlines = FirstLine.numlines;
                        // Other type of data skip 4 more bytes
                        if (FirstLine.entry_type < 4)
                        {
                            read(ModuleFile, (void *)&lSize, 4);
                            if (FirstLine.entry_type == 3)
                                lseek(ModuleFile, lSize, SEEK_CUR);
                        }
                    }
                    while (FirstLine.entry_type == 3);

                    for (j = 0; j < numlines; j++)
                    {
                        switch (FirstLine.entry_type)
                        {
                            case 0:
                                read(ModuleFile, (void *)&LineEntry, sizeof(LineEntry));
                                // Changed by Kim Rasmussen 26/06 1996 to ignore linenumber 0
                                //               if (LineEntry.Offset+ssmod32.csOff<=TrapOff && LineEntry.Offset+ssmod32.csOff>=NrLine) {
                                if (LineEntry.LineNum && LineEntry.Offset + ssmod32.csOff <= TrapOff && LineEntry.Offset + ssmod32.csOff >= NrLine)
                                {
                                    NrLine = LineEntry.Offset;
                                    NrFile = LineEntry.FileNum;
                                    /*pOffset =sprintf(szNrLine,"%04X:%08X  line #%hu ",
                                     * ssmod32.csBase,LineEntry.Offset,
                                     * LineEntry.LineNum); */
                                    sprintf(szNrLine, "% 6hu", LineEntry.LineNum);
                                }
                                break;

                            case 1:
                                lseek(ModuleFile, sizeof(struct linlist_rec), SEEK_CUR);

                                break;

                            case 2:
                                lseek(ModuleFile, sizeof(struct linsourcelist_rec), SEEK_CUR);

                                break;

                            case 3:
                                lseek(ModuleFile, sizeof(struct filenam_rec), SEEK_CUR);

                                break;

                            case 4:
                                lseek(ModuleFile, sizeof(struct pathtab_rec), SEEK_CUR);

                                break;

                        }
                    }


                    if (NrFile != 0)
                    {
                        read(ModuleFile, (void *)&FileInfo, sizeof(FileInfo));
                        namelen = 0;
                        for (j = 1; j <= FileInfo.file_count; j++)
                        {
                            namelen = 0;
                            read(ModuleFile, (void *)&namelen, 1);
                            read(ModuleFile, (void *)ename, namelen);
                            if (j == NrFile)
                                break;
                        }
                        ename[namelen] = '\0';
                        //  pOffset=sprintf(szNrLine+pOffset," (%s) (%s)\n",ename,ModName);
                        sprintf(szNrFile, "% 12.12s ", ename);
                    }
                    else
                    {
                        // strcat(szNrLine,"\n"); avoid new line for empty name fill
                        strcpy(szNrFile, "            ");
                    }           // endif
                    break;
            }                   // end switch

            i++;
        }                       // end while modindex
    }                           // End While i < numdir
    free(pDirTab32);
    return (0);
}

/*
 * Read16CodeView:
 *
 */

int Read16CodeView(FILE *LogFile, int fh, int TrapSeg, int TrapOff, CHAR *FileName)
{
    static unsigned short int offset,
                    NrPublic, NrLine,
                    NrEntry, numdir,
                    namelen, numlines,
                    line;
    static int      ModIndex;
    static int      bytesread, i, j;

    ModIndex = 0;
    // See if any CODEVIEW info
    if (lseek(fh, -8L, SEEK_END) == -1)
    {
        fprintf(LogFile, "Error %u seeking CodeView table in %s\n", errno, FileName);
        return (18);
    }

    if (read(fh, (void *)&eodbug, 8) == -1)
    {
        fprintf(LogFile, "Error %u reading debug info from %s\n", errno, FileName);
        return (19);
    }
    if (eodbug.dbug != DBUGSIG)
    {
        // fprintf(LogFile,"\nNo CodeView information stored.\n");
        return (100);
    }

    if ((lfaBase = lseek(fh, -eodbug.dfaBase, SEEK_END)) == -1L)
    {
        fprintf(LogFile, "Error %u seeking base codeview data in %s\n", errno, FileName);
        return (20);
    }

    if (read(fh, (void *)&base, 8) == -1)
    {
        fprintf(LogFile, "Error %u reading base codeview data in %s\n", errno, FileName);
        return (21);
    }

    if (lseek(fh, base.lfoDir - 8, SEEK_CUR) == -1)
    {
        fprintf(LogFile, "Error %u seeking dir codeview data in %s\n", errno, FileName);
        return (22);
    }

    if (read(fh, (void *)&numdir, 2) == -1)
    {
        fprintf(LogFile, "Error %u reading dir codeview data in %s\n", errno, FileName);
        return (23);
    }

    // Read dir table into buffer
    if ((pDirTab = (struct ssDir *)calloc(numdir, sizeof(struct ssDir))) == NULL)
    {
        fprintf(LogFile, "Out of memory!");
        return (-1);
    }

    if (read(fh, (void *)pDirTab, numdir * sizeof(struct ssDir)) == -1)
    {
        fprintf(LogFile, "Error %u reading codeview dir table from %s\n", errno, FileName);
        free(pDirTab);
        return (24);
    }

    i = 0;
    while (i < numdir)
    {
        if (pDirTab[i].sst != SSTMODULES)
        {
            i++;
            continue;
        }
        NrPublic = 0x0;
        NrLine = 0x0;
        // point to subsection
        lseek(fh, pDirTab[i].lfoStart + lfaBase, SEEK_SET);
        read(fh, (void *)&ssmod.csBase, sizeof(ssmod));
        read(fh, (void *)ModName, (unsigned)ssmod.csize);
        ModIndex = pDirTab[i].modindex;
        ModName[ssmod.csize] = '\0';
        i++;
        while (pDirTab[i].modindex == ModIndex && i < numdir)
        {
            // point to subsection
            lseek(fh, pDirTab[i].lfoStart + lfaBase, SEEK_SET);
            switch (pDirTab[i].sst)
            {
                case SSTPUBLICS:
                    bytesread = 0;
                    while (bytesread < pDirTab[i].cb)
                    {
                        bytesread += read(fh, (void *)&sspub.offset, sizeof(sspub));
                        bytesread += read(fh, (void *)ename, (unsigned)sspub.csize);
                        ename[sspub.csize] = '\0';
                        if ((sspub.segment == TrapSeg) &&
                            (sspub.offset <= TrapOff) &&
                            (sspub.offset >= NrPublic))
                        {
                            NrPublic = sspub.offset;
                            sprintf(szNrPub, "%s %s (%s) %04hX:%04hX\n",
                                    (sspub.type == 1) ? " Abs" : " ", ename,
                                    ModName, // ()
                                    sspub.segment, sspub.offset
                                );
                        }
                    }
                    break;

                case SSTSRCLINES2:
                case SSTSRCLINES:
                    if (TrapSeg != ssmod.csBase)
                        break;
                    namelen = 0;
                    read(fh, (void *)&namelen, 1);
                    read(fh, (void *)ename, namelen);
                    ename[namelen] = '\0';
                    // skip 2 zero bytes
                    if (pDirTab[i].sst == SSTSRCLINES2)
                        read(fh, (void *)&numlines, 2);
                    read(fh, (void *)&numlines, 2);
                    for (j = 0; j < numlines; j++)
                    {
                        read(fh, (void *)&line, 2);
                        read(fh, (void *)&offset, 2);
                        if (offset <= TrapOff && offset >= NrLine)
                        {
                            NrLine = offset;
                            sprintf(szNrFile, "% 12.12s ", ename);
                            sprintf(szNrLine, "% 6hu", line);
                            /*sprintf(szNrLine,"%04hX:%04hX  line #%hu  (%s) (%s)\n",
                             * ssmod.csBase,offset,line,ModName,ename); */
                        }
                    }
                    break;
            }                   // end switch
            i++;
        }                       // end while modindex
    }                           // End While i < numdir
    free(pDirTab);
    return (0);
}

/* ******************************************************************
 *                                                                  *
 *   PART 2: ANALYZE VARIABLES                                      *
 *                                                                  *
 ********************************************************************/

/*
 * var_value:
 *
 */

BYTE* var_value(void *varptr, BYTE type)
{
    static BYTE     value[128];
    APIRET          rc;

    strcpy(value, "Unknown");

    if (type == 0)
        sprintf(value, "%hd", *(signed char *)varptr);
    else if (type == 1)
        sprintf(value, "%hd", *(signed short *)varptr);
    else if (type == 2)
        sprintf(value, "%ld", *(signed long *)varptr);
    else if (type == 4)
        sprintf(value, "%hu", *(BYTE *) varptr);
    else if (type == 5)
        sprintf(value, "%hu", *(USHORT *) varptr);
    else if (type == 6)
        sprintf(value, "%lu", *(ULONG *) varptr);
    else if (type == 8)
        sprintf(value, "%f", *(float *)varptr);
    else if (type == 9)
        sprintf(value, "%f", *(double *)varptr);
    else if (type == 10)
        sprintf(value, "%f", *(long double *)varptr);
    else if (type == 16)
        sprintf(value, "%s", *(char *)varptr ? "TRUE" : "FALSE");
    else if (type == 17)
        sprintf(value, "%s", *(short *)varptr ? "TRUE" : "FALSE");
    else if (type == 18)
        sprintf(value, "%s", *(long *)varptr ? "TRUE" : "FALSE");
    else if (type == 20)
        sprintf(value, "%c", *(char *)varptr);
    else if (type == 21)
        sprintf(value, "%lc", *(short *)varptr);
    else if (type == 22)
        sprintf(value, "%lc", *(long *)varptr);
    else if (type == 23)
        sprintf(value, "void");
    else if (type >= 32)
    {
        ULONG           Size, Attr;

        Size = 1;
        rc = DosQueryMem((void *)*(ULONG *) varptr, &Size, &Attr);
        if (rc)
        {
            sprintf(value, "0x%p invalid", *(ULONG *) varptr);
        }
        else
        {
            sprintf(value, "0x%p", *(ULONG *) varptr);
            if (Attr & PAG_FREE)
            {
                strcat(value, " unallocated memory");
            }
            else
            {
                if ((Attr & PAG_COMMIT) == 0x0U)
                {
                    strcat(value, " uncommitted");
                }               // endif
                if ((Attr & PAG_WRITE) == 0x0U)
                {
                    strcat(value, " unwritable");
                }               // endif
                if ((Attr & PAG_READ) == 0x0U)
                {
                    strcat(value, " unreadable");
                }               // endif
            }                   // endif
        }                       // endif
    }

    return value;
}

/*
 * search_userdefs:
 *      Search the table of userdef's - return TRUE if found
 */

BOOL search_userdefs(FILE *LogFile, ULONG stackofs, USHORT var_no)
{
    USHORT          pos;

    for (pos = 0; pos < userdef_count && one_userdef[pos].idx != autovar_def[var_no].type_idx; pos++);

    if (pos < userdef_count)
    {
        if (one_userdef[pos].type_index >= 0x80 && one_userdef[pos].type_index <= 0xDA)
        {
            fprintf(LogFile, "     %- 6d %- 20.20s %- 33.33s %s\n",
                    autovar_def[var_no].stack_offset,
             autovar_def[var_no].name,
                one_userdef[pos].name,
                    var_value((void *)(stackofs + autovar_def[var_no].stack_offset),
                              one_userdef[pos].type_index - 0x80));
            return TRUE;
        }
        else
            // If the result isn't a simple type, let's act as we didn't find it
            return FALSE;
    }

    return FALSE;
}

/*
 * search_pointers:
 *
 */

BOOL
search_pointers(FILE *LogFile, ULONG stackofs, USHORT var_no)
{
    USHORT          pos, upos;
    static BYTE     str[35];
    BYTE            type_index;

    for (pos = 0; pos < pointer_count && one_pointer[pos].idx != autovar_def[var_no].type_idx; pos++);

    if (pos < pointer_count)
    {
        if (one_pointer[pos].type_index >= 0x80 && one_pointer[pos].type_index <= 0xDA)
        {
            strcpy(str, type_name[one_pointer[pos].type_index - 0x80]);
            strcat(str, " *");
            fprintf(LogFile, "     %- 6d %- 20.20s %- 33.33s %s\n",
                    autovar_def[var_no].stack_offset,
             autovar_def[var_no].name,
                    str,
                    var_value((void *)(stackofs + autovar_def[var_no].stack_offset), 32));
            return TRUE;
        }
        else
        {
            // If the result isn't a simple type, look for it in the other lists
            for (upos = 0; upos < userdef_count && one_userdef[upos].idx != one_pointer[pos].type_index; upos++);

            if (upos < userdef_count)
            {
                strcpy(str, one_userdef[upos].name);
                strcat(str, " *");
                fprintf(LogFile, "     %- 6d %- 20.20s %- 33.33s %s\n",
                        autovar_def[var_no].stack_offset,
                        autovar_def[var_no].name,
                        str,
                        var_value((void *)(stackofs + autovar_def[var_no].stack_offset), 32));
                return TRUE;
            }
            else
            {                   // If it isn't a userdef, for now give up and just print as much as we know
                sprintf(str, "Pointer to type 0x%X", one_pointer[pos].type_index);

                fprintf(LogFile, "     %- 6d %- 20.20s %- 33.33s %s\n",
                        autovar_def[var_no].stack_offset,
                        autovar_def[var_no].name,
                        str,
                        var_value((void *)(stackofs + autovar_def[var_no].stack_offset), 32));

                return TRUE;
            }
        }
    }

    return FALSE;
}

/*
 *@@ dbgPrintVariables:
 *
 *      New with V0.84.
 */

void dbgPrintVariables(FILE *LogFile, ULONG stackofs)
{
    USHORT          n, pos;
    BOOL            AutoVarsFound = FALSE;

//   stackofs += stack_ebp;
    if (1 || func_ofs == pubfunc_ofs)
    {
        for (n = 0; n < var_ofs; n++)
        {
            if (AutoVarsFound == FALSE)
            {
                AutoVarsFound = TRUE;
                fprintf(LogFile, "     List of auto variables at EBP %p in %s:\n", stackofs, func_name);
                fprintf(LogFile, "     Offset Name                 Type                              Value            \n");
                fprintf(LogFile, "     컴컴컴 컴컴컴컴컴컴컴컴컴컴 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴� 컴컴컴컴컴컴컴컴�\n");
            }

            // If it's one of the simple types
            if (autovar_def[n].type_idx >= 0x80 && autovar_def[n].type_idx <= 0xDA)
            {
                fprintf(LogFile, "     %- 6d %- 20.20s %- 33.33s %s\n",
                        autovar_def[n].stack_offset,
                  autovar_def[n].name,
                        type_name[autovar_def[n].type_idx - 0x80],
                        var_value((void *)(stackofs + autovar_def[n].stack_offset),
                                  autovar_def[n].type_idx - 0x80));
            }
            else
            {                   // Complex type, check if we know what it is
                if (!search_userdefs(LogFile, stackofs, n))
                {
                    if (!search_pointers(LogFile, stackofs, n))
                    {
                        fprintf(LogFile, "     %- 6d %-20.20s 0x%X\n",
                                autovar_def[n].stack_offset,
                                autovar_def[n].name,
                                autovar_def[n].type_idx);
                    }
                }
            }
        }
        /* if (AutoVarsFound == FALSE)
        {
            fprintf(LogFile, "  No auto variables found in %s.\n", func_name);
        } */
        fprintf(LogFile, "\n");
    }
}

/* ******************************************************************
 *                                                                  *
 *   PART 3: ANALYZE SYMBOL (.SYM) FILE                             *
 *                                                                  *
 ********************************************************************/

/*
 *@@ dbgPrintSYMInfo:
 *      this gets called by dbgPrintStack to check if a SYM
 *      file with the same filename exists and try to
 *      get the info from there.
 *
 *      This gets called for every line of the stack
 *      walk, but only if getting the information from
 *      the debug code failed, e.g. because no debug code
 *      was available.
 *      The file pointer is in the "Source file" column
 *      every time.
 *
 *      New with V0.84.
 */

void dbgPrintSYMInfo(FILE *file, CHAR * SymFileName, ULONG Object, ULONG TrapOffset)
{
    static FILE    *SymFile;
    static MAPDEF   MapDef;
    static SEGDEF   SegDef;
    static SEGDEF  *pSegDef;
    static SYMDEF32 SymDef32;
    static SYMDEF16 SymDef16;
    static char     Buffer[256];
    static int      SegNum, SymNum, LastVal;
    static unsigned long int SegOffset,
                    SymOffset, SymPtrOffset;

    // open .SYM file
    SymFile = fopen(SymFileName, "rb");
    if (SymFile == 0)
    {
        fprintf(file, "Could not open symbol file %s\n", SymFileName);
        return;
    }                           // endif

    // read in first map definition
    fread(&MapDef, sizeof(MAPDEF), 1, SymFile);

    SegOffset = SEGDEFOFFSET(MapDef);

    // go thru all segments
    for (SegNum = 0; SegNum < MapDef.cSegs; SegNum++)
    {
        // printf("Scanning segment #%d Offset %4.4hX\n",SegNum+1,SegOffset);
        if (fseek(SymFile, SegOffset, SEEK_SET))
        {
            fprintf(file, "Seek error.\n");
            return;
        }

        // read in segment definition
        fread(&SegDef, sizeof(SEGDEF), 1, SymFile);
        if (SegNum == Object)
        {
            // stack object found:
            Buffer[0] = 0x00;
            LastVal = 0;

            // go thru all symbols in this object
            for (SymNum = 0; SymNum < SegDef.cSymbols; SymNum++)
            {

                // read in symbol offset USHORT
                SymPtrOffset = SYMDEFOFFSET(SegOffset, SegDef, SymNum);
                fseek(SymFile, SymPtrOffset, SEEK_SET);
                fread(&SymOffset, sizeof(unsigned short int), 1, SymFile);

                // go to symbol definition
                fseek(SymFile, SymOffset + SegOffset, SEEK_SET);

                if (SegDef.bFlags & 0x01)
                {
                    // 32-bit symbol:
                    fread(&SymDef32, sizeof(SYMDEF32), 1, SymFile);
                    if (SymDef32.wSymVal > TrapOffset)
                    {
                        // symbol found
                        fprintf(file, "between %s + 0x%X ", Buffer, TrapOffset - LastVal);
                        fprintf(file, "(ppLineDef: 0x%lX) ",
                                    LINEDEFOFFSET(SegDef)
                                    );
                        fprintf(file, "\n");
                    }

                    LastVal = SymDef32.wSymVal;
                    Buffer[0] = SymDef32.achSymName[0];
                    fread(&Buffer[1], 1, SymDef32.cbSymName, SymFile);
                    Buffer[SymDef32.cbSymName] = 0x00;

                    if (SymDef32.wSymVal > TrapOffset)
                    {
                        // symbol found, as above
                        fprintf(file, "                                         "
                                      "and %s - 0x%X ", Buffer, LastVal - TrapOffset);
                        fprintf(file, "\n");
                        break;
                    }
                    /*printf("32 Bit Symbol <%s> Address %p\n",Buffer,SymDef32.wSymVal); */
                }
                else
                {
                    // 16-bit symbol:
                    fread(&SymDef16, sizeof(SYMDEF16), 1, SymFile);
                    if (SymDef16.wSymVal > TrapOffset)
                    {
                        fprintf(file, "between %s + %X", Buffer, TrapOffset - LastVal);
                        fprintf(file, "\n");
                    }
                    LastVal = SymDef16.wSymVal;
                    Buffer[0] = SymDef16.achSymName[0];
                    fread(&Buffer[1], 1, SymDef16.cbSymName, SymFile);
                    Buffer[SymDef16.cbSymName] = 0x00;
                    if (SymDef16.wSymVal > TrapOffset)
                    {
                        fprintf(file, "                                         "
                                      "and %s - %X ", Buffer, LastVal - TrapOffset);
                        fprintf(file, "\n");
                        break;
                    }
                    /*printf("16 Bit Symbol <%s> Address %p\n",Buffer,SymDef16.wSymVal); */
                }               // endif
            }
            break;
        }                       // endif
        SegOffset = NEXTSEGDEFOFFSET(SegDef);
    }                           // endwhile
    fclose(SymFile);
}

/* ******************************************************************
 *                                                                  *
 *   PART 4: dbgPrintStack                                          *
 *                                                                  *
 ********************************************************************/

/*
 *@@ dbgPrintStack:
 *      this takes stack data from the TIB and
 *      context record data structures and tries
 *      to analyse what the different stack frames
 *      correspond to in the .SYM file.
 *
 *      New with V0.84.
 */

VOID dbgPrintStack(FILE *file,
                   PUSHORT StackBottom,
                   PUSHORT StackTop,
                   PUSHORT Ebp,
                   PUSHORT ExceptionAddress)
{
    PUSHORT         RetAddr;
    PUSHORT         LastEbp;
    APIRET          rc;
    ULONG           Size, Attr;
    USHORT          Cs, Ip, Bp, Sp;
    static char     Name[CCHMAXPATH];
    HMODULE         hMod;
    ULONG           ObjNum;
    ULONG           Offset;
    BOOL            fExceptionAddress = TRUE;   // Use Exception Addr 1st time thru

    // Note: we can't handle stacks bigger than 64K for now...
    Sp = (USHORT) (((ULONG) StackBottom) >> 16);
    Bp = (USHORT) (ULONG) Ebp;

    if (!f32bit)
        Ebp = (PUSHORT) MAKEULONG(Bp, Sp);

    fprintf(file, "\n\nCall Stack:\n");
    fprintf(file, "                                        Source    Line      Nearest\n");
    fprintf(file, "   EBP      Address    Module  Obj#      File     Numbr  Public Symbol\n");
    fprintf(file, " 컴컴컴컴  컴컴컴컴-  컴컴컴컴 컴컴  컴컴컴컴컴컴 컴컴�  컴컴컴컴컴컴-\n");

    do
    {
        Size = 10;
        rc = DosQueryMem((PVOID) (Ebp + 2), &Size, &Attr);
        if (rc != NO_ERROR || !(Attr & PAG_COMMIT) || (Size < 10))
        {
            fprintf(file, "Invalid EBP: %8.8p\n", Ebp);
            break;
        }

        if (fExceptionAddress)
            RetAddr = ExceptionAddress;
        else
            RetAddr = (PUSHORT) (*((PULONG) (Ebp + 2)));

        if (RetAddr == (PUSHORT) 0x00000053)
        {
            // For some reason there's a "return address" of 0x53 following
            // EBP on the stack and we have to adjust EBP by 44 bytes to get
            // at the real return address.  This has something to do with
            // thunking from 32bits to 16bits...
            // Serious kludge, and it's probably dependent on versions of C(++)
            // runtime or OS, but it works for now!
            Ebp += 22;
            RetAddr = (PUSHORT) (*((PULONG) (Ebp + 2)));
        }

        // Get the (possibly) 16bit CS and IP
        if (fExceptionAddress)
        {
            Cs = (USHORT) (((ULONG) ExceptionAddress) >> 16);
            Ip = (USHORT) (ULONG) ExceptionAddress;
        }
        else
        {
            Cs = *(Ebp + 2);
            Ip = *(Ebp + 1);
        }

        // if the return address points to the stack then it's really just
        // a pointer to the return address (UGH!).
        if ((USHORT) (((ULONG) RetAddr) >> 16) == Sp)
            RetAddr = (PUSHORT) (*((PULONG) RetAddr));

        if (Ip == 0 && *Ebp == 0)
        {
            // End of the stack so these are both shifted by 2 bytes:
            Cs = *(Ebp + 3);
            Ip = *(Ebp + 2);
        }

        // 16bit programs have on the stack:
        //   BP:IP:CS
        //   where CS may be thunked
        //
        //         in dump                 swapped
        //    BP        IP   CS          BP   CS   IP
        //   4677      53B5 F7D0        7746 D0F7 B553
        //
        // 32bit programs have:
        //   EBP:EIP
        // and you'd have something like this (with SP added) (not
        // accurate values)
        //
        //         in dump               swapped
        //      EBP       EIP         EBP       EIP
        //   4677 2900 53B5 F7D0   0029 7746 D0F7 B553
        //
        // So the basic difference is that 32bit programs have a 32bit
        // EBP and we can attempt to determine whether we have a 32bit
        // EBP by checking to see if its 'selector' is the same as SP.
        // Note that this technique limits us to checking stacks < 64K.
        //
        // Soooo, if IP (which maps into the same USHORT as the swapped
        // stack page in EBP) doesn't point to the stack (i.e. it could
        // be a 16bit IP) then see if CS is valid (as is or thunked).
        //
        // Note that there's the possibility of a 16bit return address
        // that has an offset that's the same as SP so we'll think it's
        // a 32bit return address and won't be able to successfully resolve
        // its details.
        if (Ip != Sp)
        {
            if (DOS16SIZESEG(Cs, &Size) == NO_ERROR)
            {
                RetAddr = (USHORT * _Seg16) MAKEULONG(Ip, Cs);
                f32bit = FALSE;
            }
            else if (DOS16SIZESEG((Cs << 3) + 7, &Size) == NO_ERROR)
            {
                Cs = (Cs << 3) + 7;
                RetAddr = (USHORT * _Seg16) MAKEULONG(Ip, Cs);
                f32bit = FALSE;
            }
            else
                f32bit = TRUE;
        }
        else
            f32bit = TRUE;


        // "EBP" column
        if (fExceptionAddress)
            fprintf(file, " Trap  ->  ");
        else
            fprintf(file, " %8.8p  ", Ebp);

        // "Address" column
        if (f32bit)
            fprintf(file, ":%8.8p  ", RetAddr);
        else
            fprintf(file, "%04.04X:%04.04X  ", Cs, Ip);

        // Version check omitted; the following requires
        // OS/2 2.10 or later (*UM)
        // if (Version[0] >= 20 && Version[1] >= 10)
        {
            // Make a 'tick' sound to let the user know we're still alive
            DosBeep(2000, 10);

            Size = 10;    // Inserted by Kim Rasmussen 26/06 1996 to avoid error 87 when Size is 0

            // "Module"/"Object" columns
            rc = DosQueryMem((PVOID) RetAddr, &Size, &Attr);
            if (rc != NO_ERROR || !(Attr & PAG_COMMIT))
            {
                fprintf(file, "Invalid RetAddr: %8.8p\n", RetAddr);
                break;          // avoid infinite loops
            }
            else
            {
                rc = DOSQUERYMODFROMEIP(&hMod, &ObjNum, sizeof(Name),
                                        Name, &Offset, (PVOID) RetAddr);
                if (rc == NO_ERROR && ObjNum != -1)
                {
                    static char     szJunk[_MAX_FNAME];
                    static char     szName[_MAX_FNAME];

                    DosQueryModuleName(hMod, sizeof(Name), Name);
                    _splitpath(Name, szJunk, szJunk, szName, szJunk);

                    // print module and object
                    fprintf(file, "%-8s %04X  ", szName, ObjNum + 1);

                    if (strlen(Name) > 3)
                    {
                        // "Source file"... columns

                        // first attempt to analyze the debug code
                        rc = dbgPrintDebugInfo(file, Name, ObjNum, Offset);
                        // if no debug code is available, analyze
                        // the SYM file instead
                        if (rc != NO_ERROR)
                        {
                            strcpy(Name + strlen(Name) - 3, "SYM");
                            dbgPrintSYMInfo(file, Name, ObjNum, Offset);
                        }
                    }
                }
                else
                {
                    fprintf(file, "*Unknown*\n");
                }
            }
        }

        Bp = *Ebp;
        if (Bp == 0 && (*Ebp + 1) == 0)
        {
            fprintf(file, "End of Call Stack\n");
            break;
        }

        if (!fExceptionAddress)
        {
            LastEbp = Ebp;
#if 0
            Ebp = (PUSHORT) MAKEULONG(Bp, Sp);
#else // Inserted by Kim Rasmussen 26/06 1996 to allow big stacks
            if (f32bit)
                Ebp = (PUSHORT) * (PULONG) LastEbp;
            else
                Ebp = (PUSHORT) MAKEULONG(Bp, Sp);
#endif
            if (f32bit)
            {
                dbgPrintVariables(file, (ULONG) Ebp);
            }                   // endif

            if (Ebp < LastEbp)
            {
                fprintf(file, "Lost Stack chain - new EBP below previous\n");
                break;
            }
        }
        else
            fExceptionAddress = FALSE;

        Size = 4;
        rc = DosQueryMem((PVOID) Ebp, &Size, &Attr);
        if ((rc != NO_ERROR) || (Size < 4))
        {
            fprintf(file, "Lost Stack chain - invalid EBP: %8.8p\n", Ebp);
            break;
        }
    } while (TRUE);

    fprintf(file, "\n");
}

