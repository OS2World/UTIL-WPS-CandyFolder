/*
 *      This file incorporates code from the following:
 *      -- Monte Copeland, IBM Boca Ration, Florida, USA (1993)
 *      -- Roman Stangl, from the Program Commander/2 sources
 *         (1997-98)
 *      -- Marc Fiammante, John Currier, Kim Rasmussen,
 *         Anthony Cruise (EXCEPT3.ZIP package for a generic
 *         exception handling DLL, available at Hobbes).
 *
 *      In the following code, if not explicitly stated otherwise,
 *      the code has been written by me, Ulrich M”ller.
 *
 *      This file Copyright (C) 1992-99 Ulrich M”ller,
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
/*
 *@@sourcefile except.c:
 *      this file contains all the XFolder exception handlers.
 *      These are specific to XFolder in that they write additional
 *      debugging information to the log files, but most of this code
 *      could be used independently of XFolder also.
 *
 *      Note: except.h declares a few handy macros to easily
 *      install XFolder's exception handling for a section
 *      of code.
 *
 *      These macros automatically insert
 *      code for properly registering and deregistering the
 *      handlers, so you won't accidentally forget to
 *      deregister an exception handler.
 *      The general usage is like this:
 *
 +          int your_func(int ...)
 +          {
 +              TRY_LOUD(id)
 +              {
 +                  ....        // the stuff in here is protected by
 +                              // the excHandlerLoud excpt handler
 +              }
 +              CATCH(id)
 +              {
 +                  ....        // exception occured: react here
 +              } END_CATCH;    // always needed!
 +          } // end of your_func
 *
 *      TRY_LOUD  is for excHandlerLoud (see below).
 *      TRY_QUIET is for excHandlerQuiet (see below).
 *      CATCH / END_CATCH are the same for the two. This
 *      is where the exception handler jumps to if an
 *      exception occurs.
 *      This block is _required_ even if you do nothing
 *      in there.
 *
 *      "id" can be any identifier which is not used in
 *      your current variable scope, e.g. "excpt1". This
 *      is used for creating an auto-variable EXCEPTSTRUCT
 *      of that name. The "id"'s in TRY_* and CATCH must
 *      match, since this is where the macros store the
 *      exception handler data.
 *
 *      It is possible to nest these handlers. You should
 *      use different "ids" then. Avoid using gotos to
 *      jump between the different CATCH blocks, because
 *      this might not properly deregister the handlers.
 *
 *      Here's how to deal with mutex semaphores: WHENEVER
 *      you request a mutex semaphore, enclose the block
 *      with TRY/CATCH in case an error occurs, like this:
 *
 +          int your_func(int)
 +          {
 +              HMTX hmtx = ...
 +              BOOL fSemOwned = FALSE;
 +
 +              TRY_QUIET(id)
 +              {
 +                  fSemOwned = (DosRequestMutexSem(hmtx, ...) == NO_ERROR);
 +                  if (fSemOwned)
 +                  {       ... // work on your data
 +                  }
 +              }
 +              CATCH(id) { } END_CATCH;    // always needed!
 +
 +              if (fSemOwned) {
 +                  DosReleaseMutexSem(hmtx);
 +                  fSemOwned = FALSE;
 +              }
 +          } // end of your_func
 *
 *      This way your mutex semaphore gets released even if
 *      exceptions occur in your code. If you don't do this,
 *      threads waiting for that semaphore will be blocked
 *      forever when exceptions occur. As a result, depending
 *      on the thread, PM will hang, or the application will
 *      never terminate.
 *
 *      More useful debug information can be found in the "OS/2 Debugging
 *      Handbook", which is now available in INF format on the IBM
 *      DevCon site ("http://service2.boulder.ibm.com/devcon/").
 *      This book shows worked examples of how to unwind a stack dump.
 *
 *@@include #define INCL_DOSEXCEPTIONS
 *@@include #define INCL_DOSPROCESS
 *@@include #include <os2.h>
 *@@include #include <stdio.h>
 *@@include #include <setjmp.h>
 *@@include #include <assert.h>
 *@@include #include "except.h"
 */


#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS         // DosSleep, priorities, PIDs etc.
#define INCL_DOSSEMAPHORES      // needed for xthreads.h
#define INCL_DOSEXCEPTIONS      // needed for except.h
#define INCL_DOSERRORS
#define INCL_DOSMISC

#include <os2.h>

// C library headers
#include <stdio.h>              // needed for except.h
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

#include "except.h"             // XFolder exception handling
#include "debug.h"


/* ******************************************************************
 *                                                                  *
 *   XFolder-specific stuff                                         *
 *                                                                  *
 ********************************************************************/

const CHAR acDriveLetters[28] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*
 *@@ doshQueryBootDrive:
 *      returns the letter of the boot drive
 */

CHAR doshQueryBootDrive(VOID)
{
    ULONG ulBootDrive;

    DosQuerySysInfo(5, 5, &ulBootDrive, sizeof(ulBootDrive));
    return (acDriveLetters[ulBootDrive]);
}

/*
 *@@ excOpenTraplogFile:
 *      this opens or creates C:\XFLDTRAP.LOG and writes
 *      a debug header into it (date and time); returns
 *      a FILE* pointer for fprintf(), so additional data can
 *      be written. You should use fclose() to close the file.
 */

FILE* excOpenTraplogFile(VOID)
{
    CHAR        szFileName[CCHMAXPATH];
    FILE        *file;
    DATETIME    DT;

    // ULONG ulCopied;

    sprintf(szFileName, "%c:\\%s", doshQueryBootDrive(), "Candyfld.log");
    file = fopen(szFileName, "a");

    if (file) {
        DosGetDateTime(&DT);
        fprintf(file, "\nCandyFolder trap message -- Date: %02d/%02d/%04d, Time: %02d:%02d:%02d\n",
            DT.month, DT.day, DT.year,
            DT.hours, DT.minutes, DT.seconds);
        fprintf(file, "--------------------------------------------------------\n"
                      "\nAn internal error occurred within CandyFolder.\n");

    }
    return (file);
}

/* ******************************************************************
 *                                                                  *
 *   XFolder-independent stuff                                      *
 *                                                                  *
 ********************************************************************/

/*
 *@@ excDescribePage:
 *
 */

VOID excDescribePage(FILE *file, ULONG ulCheck)
{
    APIRET arc;
    ULONG ulCountPages = 1;
    ULONG ulFlagsPage = 0;
    arc = DosQueryMem((PVOID)ulCheck, &ulCountPages, &ulFlagsPage);

    if (arc == NO_ERROR) {
        fprintf(file, "valid, flags: ");
        if (ulFlagsPage & PAG_READ)
            fprintf(file, "read ");
        if (ulFlagsPage & PAG_WRITE)
            fprintf(file, "write ");
        if (ulFlagsPage & PAG_EXECUTE)
            fprintf(file, "execute ");
        if (ulFlagsPage & PAG_GUARD)
            fprintf(file, "guard ");
        if (ulFlagsPage & PAG_COMMIT)
            fprintf(file, "committed ");
        if (ulFlagsPage & PAG_SHARED)
            fprintf(file, "shared ");
        if (ulFlagsPage & PAG_FREE)
            fprintf(file, "free ");
        if (ulFlagsPage & PAG_BASE)
            fprintf(file, "base ");
    }
    else if (arc == ERROR_INVALID_ADDRESS)
        fprintf(file, "invalid");
}

/*
 *@@ excExplainException:
 *      used by the exception handlers below to write
 *      information about the exception into a logfile.
 */

VOID excExplainException(FILE *file,                   // in: logfile from fopen()
                         PSZ pszHandlerName,           // in: descriptive string
                         PEXCEPTIONREPORTRECORD pReportRec, // in: excpt info
                         PCONTEXTRECORD pContextRec)   // in: excpt info
{
    PTIB        ptib = NULL;
    PPIB        ppib = NULL;
    HMODULE     hMod1, hMod2;
    CHAR        szMod1[CCHMAXPATH] = "unknown",
                szMod2[CCHMAXPATH] = "unknown";
    ULONG       ulObjNum,
                ulOffset,
                ulCountPages,
                ulFlagsPage;
    APIRET      arc;
    PULONG      pulStackWord,
                pulStackBegin;
    ULONG       ul;

    ULONG       ulOldPriority = 0x0100; // regular, delta 0

    // raise this thread's priority, because this
    // might take some time
    if (DosGetInfoBlocks(&ptib, &ppib) == NO_ERROR)
        if (ptib)
            if (ptib->tib_ptib2) {
                ulOldPriority = ptib->tib_ptib2->tib2_ulpri;
                DosSetPriority(PRTYS_THREAD,
                        PRTYC_REGULAR,
                        PRTYD_MAXIMUM,
                        0);     // current thread
            }

    // make some noise
    DosBeep(4000, 30);
    DosBeep(2000, 30);
    DosBeep(1000, 30);
    DosBeep( 500, 30);
    DosBeep( 250, 30);
    DosBeep( 500, 30);
    DosBeep(1000, 30);
    DosBeep(2000, 30);
    DosBeep(4000, 30);

    // generic exception info
    fprintf(file,
            "\n%s:\n    Exception type: %08X\n    Address:        %08X\n    Params:         ",
            pszHandlerName,
            pReportRec->ExceptionNum,
            pReportRec->ExceptionAddress);
    for (ul = 0;  ul < pReportRec->cParameters;  ul++)
    {
        fprintf(file, "%08X  ",
            pReportRec->ExceptionInfo[ul]);
    }

    // now explain the exception in a bit more detail;
    // depending on the exception, pReportRec->ExceptionInfo
    // contains some useful data
    switch (pReportRec->ExceptionNum)
    {
        case XCPT_ACCESS_VIOLATION: {
            fprintf(file, "\nAccess violation: ");
            if (pReportRec->ExceptionInfo[0] & XCPT_READ_ACCESS)
                fprintf(file, "Invalid read access from 0x%04X:%08X.\n",
                            pContextRec->ctx_SegDs, pReportRec->ExceptionInfo[1]);
            else if (pReportRec->ExceptionInfo[0] & XCPT_WRITE_ACCESS)
                fprintf(file, "Invalid write access to 0x%04X:%08X.\n",
                            pContextRec->ctx_SegDs, pReportRec->ExceptionInfo[1]);
            else if (pReportRec->ExceptionInfo[0] & XCPT_SPACE_ACCESS)
                fprintf(file, "Invalid space access at 0x%04X.\n",
                            pReportRec->ExceptionInfo[1]);
            else if (pReportRec->ExceptionInfo[0] & XCPT_LIMIT_ACCESS)
                fprintf(file, "Invalid limit access occurred.\n");
            else if (pReportRec->ExceptionInfo[0] == XCPT_UNKNOWN_ACCESS)
                fprintf(file, "unknown at 0x%04X:%08X\n",
                            pContextRec->ctx_SegDs, pReportRec->ExceptionInfo[1]);
            fprintf(file,
                        "Explanation: An attempt was made to access a memory object which does\n"
                        "             not belong to the current process. Most probable causes\n"
                        "             for this are that an invalid pointer was used, there was\n"
                        "             confusion with administering memory or error conditions \n"
                        "             were not properly checked for.\n");
            break;
        }

        case XCPT_INTEGER_DIVIDE_BY_ZERO: {
            fprintf(file, "\nInteger division by zero.\n");
            fprintf(file,
                        "Explanation: An attempt was made to divide an integer value by zero,\n"
                        "             which is not defined.\n");
            break;
        }

        case XCPT_ILLEGAL_INSTRUCTION: {
            fprintf(file, "\nIllegal instruction found.\n");
            fprintf(file,
                        "Explanation: An attempt was made to execute an instruction that\n"
                        "             is not defined on this machine's architecture.\n");
            break;
        }

        case XCPT_PRIVILEGED_INSTRUCTION: {
            fprintf(file, "\nPrivileged instruction found.\n");
            fprintf(file,
                        "Explanation: An attempt was made to execute an instruction that\n"
                        "             is not permitted in the current machine mode or that\n"
                        "             XFolder had no permission to execute.\n");
            break;
        }

        case XCPT_INTEGER_OVERFLOW:
            fprintf(file, "\nInteger overflow.\n");
            fprintf(file,
                        "Explanation: An integer operation generated a carry-out of the most\n"
                        "             significant bit. This is a sign of an attempt to store\n"
                        "             a value which does not fit into an integer variable.\n");
    }

    if (DosGetInfoBlocks(&ptib, &ppib) == NO_ERROR)
    {
        /*
         * process info:
         *
         */

        if (ppib)
        {
            if (pContextRec->ContextFlags & CONTEXT_CONTROL)
            {
                // get the main module
                hMod1 = ppib->pib_hmte;
                DosQueryModuleName(hMod1,
                                   sizeof(szMod1),
                                   szMod1);

                // get the trapping module
                DosQueryModFromEIP(&hMod2,
                                   &ulObjNum,
                                   sizeof(szMod2),
                                   szMod2,
                                   &ulOffset,
                                   pContextRec->ctx_RegEip);
                DosQueryModuleName(hMod2,
                                   sizeof(szMod2),
                                   szMod2);
            }

            fprintf(file,
                    "\nProcess information: PID 0x%lX"
                    "\n    Module handle:   0x%lX (%s)"
                    "\n    Trapping module: 0x%lX (%s)"
                    "\n    Object: %d\n",
                    ppib->pib_ulpid,
                    hMod1, szMod1,
                    hMod2, szMod2,
                    ulObjNum);
        } else
            fprintf(file, "\nProcess information was not available.");



        // *** registers

        fprintf(file, "\nRegisters:");
        if (pContextRec->ContextFlags & CONTEXT_INTEGER)
        {
            // EAX
            fprintf(file, "\n    EAX = %08X  ", pContextRec->ctx_RegEax);
            excDescribePage(file, pContextRec->ctx_RegEax);
            // EBX
            fprintf(file, "\n    EBX = %08X  ", pContextRec->ctx_RegEbx);
            excDescribePage(file, pContextRec->ctx_RegEbx);
            // ECX
            fprintf(file, "\n    ECX = %08X  ", pContextRec->ctx_RegEcx);
            excDescribePage(file, pContextRec->ctx_RegEcx);
            // EDX
            fprintf(file, "\n    EDX = %08X  ", pContextRec->ctx_RegEdx);
            excDescribePage(file, pContextRec->ctx_RegEdx);
            // ESI
            fprintf(file, "\n    ESI = %08X  ", pContextRec->ctx_RegEsi);
            excDescribePage(file, pContextRec->ctx_RegEsi);
            // EDI
            fprintf(file, "\n    EDI = %08X  ", pContextRec->ctx_RegEdi);
            excDescribePage(file, pContextRec->ctx_RegEdi);
            fprintf(file, "\n");
        } else
            fprintf(file, " not available\n");

        if (pContextRec->ContextFlags & CONTEXT_CONTROL)
        {

            // *** instruction

            fprintf(file, "Instruction:\n    CS:EIP = %04X:%08X  ",
                    pContextRec->ctx_SegCs,
                    pContextRec->ctx_RegEip);
            excDescribePage(file, pContextRec->ctx_RegEip);

            // *** CPU flags

            fprintf(file, "\n    FLG    = %08X", pContextRec->ctx_EFlags);

            /*
             * stack:
             *
             */

            fprintf(file, "\nStack:\n    Base:         %08X\n    Limit:        %08X",
                (ptib ? ptib->tib_pstack : 0),
                (ptib ? ptib->tib_pstacklimit :0));
            fprintf(file, "\n    SS:ESP = %04X:%08X  ",
                pContextRec->ctx_SegSs, pContextRec->ctx_RegEsp);
            excDescribePage(file, pContextRec->ctx_RegEsp);

            fprintf(file, "\n    EBP    =      %08X  ", pContextRec->ctx_RegEbp);
            excDescribePage(file, pContextRec->ctx_RegEbp);

            dbgPrintStack(file,
                        (PUSHORT)ptib->tib_pstack,
                        (PUSHORT)ptib->tib_pstacklimit,
                        (PUSHORT)pContextRec->ctx_RegEbp,
                        (PUSHORT)pReportRec->ExceptionAddress);

            /*
             * stack dump:
             *      this code is mostly (W) Roman Stangl.
             */

            if (ptib != 0)
            {
                #ifdef DEBUG_EXCPT_STACKDUMP
                    // start with Stack dump; if EBP is within stack and smaller
                    // than ESP, use EBP, otherwise use ESP (which points into the
                    // stack anyway, so no check needed)
                    fprintf(file, "\n\nStack dump :      +0         +4         +8         +C");
                    if (pContextRec->ctx_RegEbp < pContextRec->ctx_RegEsp)
                        pulStackWord = (PULONG)(pContextRec->ctx_RegEbp & 0xFFFFFFF0);
                    else
                        pulStackWord = (PULONG)(pContextRec->ctx_RegEsp & 0xFFFFFFF0);

                    for (pulStackBegin = pulStackWord;
                         pulStackWord <= (PULONG)ptib->tib_pstacklimit;
                        /* NOP */)
                    {
                        if (((ULONG)pulStackWord & 0x00000FFF) == 0x00000000)
                        {
                            // we're on a page boundary: check access
                            ulCountPages = 0x1000;
                            ulFlagsPage = 0;
                            arc = DosQueryMem((void *)pulStackWord,
                                    &ulCountPages, &ulFlagsPage);
                            if (    (arc != NO_ERROR)
                                 || (   (arc == NO_ERROR)
                                      && ( !( ((ulFlagsPage & (PAG_COMMIT|PAG_READ))
                                            == (PAG_COMMIT|PAG_READ)) ) )
                                    )
                               )
                            {
                                fprintf(file, "\n    %08X: ", pulStackWord);
                                fprintf(file, "Page inaccessible");
                                pulStackWord += 0x1000;
                                continue; // for
                            }
                        }

                        if (((ULONG)pulStackWord & 0x0000000F) == 0)
                            fprintf(file, "\n    %08X: ", pulStackWord);
                        if (    (*pulStackWord >= (ULONG)pulStackBegin)
                             && (*pulStackWord <= (ULONG)ptib->tib_pstacklimit)
                           )
                            fprintf(file, "<%08X> ", *pulStackWord);
                        else
                            fprintf(file, " %08X  ", *pulStackWord);
                        pulStackWord++;
                    } // end for
                #endif

                // *** stack frames

                fprintf(file, "\n\nStack frames:\n              Address   Module:Object\n");
                pulStackWord = (PULONG)pContextRec->ctx_RegEbp;
                while (    (pulStackWord != 0)
                        && (pulStackWord < (PULONG)ptib->tib_pstacklimit)
                      )
                {
                    fprintf(file, "    %08X: %08X ",
                            pulStackWord,
                            *(pulStackWord+1));
                    if (DosQueryModFromEIP(&hMod1, &ulObjNum,
                                sizeof(szMod1), szMod1,
                                &ulOffset,
                                *(pulStackWord+1))
                       == NO_ERROR)
                    {
                        fprintf(file, " %s:%d\n",
                            szMod1, ulObjNum);
                    }
                    pulStackWord = (PULONG)*(pulStackWord);
                } // end while
            }
        }
    }
    fprintf(file, "\n");

    // reset old priority
    DosSetPriority(PRTYS_THREAD,
            (ulOldPriority & 0x0F00) >> 8,
            (UCHAR)ulOldPriority,
            0);     // current thread
}

/*
 *@@ excHandlerLoud:
 *      this is the "sophisticated" exception handler;
 *      which gives forth a loud sequence of beeps thru the
 *      speaker, writes a trap log and then returns back
 *      to the thread to continue execution, i.e. the
 *      default OS/2 exception handler will never get
 *      called.
 *
 *      This calls the functions in debug.c to try to
 *      find debug code or SYM file information about
 *      what source code corresponds to the error.
 *
 *      Note that to get meaningful debugging information
 *      in this handler's traplog, you need the following:
 *      a)  have a MAP file created at link time (/MAP)
 *      b)  convert the MAP to a SYM file using MAPSYM
 *      c)  put the SYM file in the same directory of
 *          the executable. This must have the same
 *          filestem as the executable.
 *
 *      This thing intercepts the following exceptions:
 *      --  XCPT_ACCESS_VIOLATION         (traps 0x0d, 0x0e)
 *      --  XCPT_INTEGER_DIVIDE_BY_ZERO   (trap 0)
 *      --  XCPT_ILLEGAL_INSTRUCTION      (trap 6)
 *      --  XCPT_PRIVILEGED_INSTRUCTION
 *      --  XCPT_INTEGER_OVERFLOW         (trap 4)
 *
 *      See the "Control Programming Guide and Reference"
 *      for details.
 *      All other exceptions are passed to the next handler
 *      in the exception handler chain. This might be the
 *      C/C++ compiler handler or the default OS/2 handler,
 *      which will probably terminate the process.
 *
 *      This is best registered thru the TRY_LOUD macro
 *      (new with V0.84, described in except.c).
 */

ULONG _System excHandlerLoud(PEXCEPTIONREPORTRECORD pReportRec,
                                     PREGREC2 pRegRec2,
                                     PCONTEXTRECORD pContextRec,
                                     PVOID pv)
{
    /* From the VAC++3 docs:
     *     "The first thing an exception handler should do is check the
     *     exception flags. If EH_EXIT_UNWIND is set, meaning
     *     the thread is ending, the handler tells the operating system
     *     to pass the exception to the next exception handler. It does the
     *     same if the EH_UNWINDING flag is set, the flag that indicates
     *     this exception handler is being removed.
     *     The EH_NESTED_CALL flag indicates whether the exception
     *     occurred within an exception handler. If the handler does
     *     not check this flag, recursive exceptions could occur until
     *     there is no stack remaining."
     * So for all these conditions, we exit immediately.
     */

    if (pReportRec->fHandlerFlags & EH_EXIT_UNWIND)
       return (XCPT_CONTINUE_SEARCH);
    if (pReportRec->fHandlerFlags & EH_UNWINDING)
       return (XCPT_CONTINUE_SEARCH);
    if (pReportRec->fHandlerFlags & EH_NESTED_CALL)
       return (XCPT_CONTINUE_SEARCH);

    switch (pReportRec->ExceptionNum)
    {
        case XCPT_ACCESS_VIOLATION:
        case XCPT_INTEGER_DIVIDE_BY_ZERO:
        case XCPT_ILLEGAL_INSTRUCTION:
        case XCPT_PRIVILEGED_INSTRUCTION:
        case XCPT_INTEGER_OVERFLOW:
        {
           // "real" exceptions:
           // write error log
            FILE *file = excOpenTraplogFile();
            excExplainException(file, "excHandlerLoud",
                                pReportRec,
                                pContextRec);
            fclose(file);
            // jump back to failing routine
            longjmp(pRegRec2->jmpThread, pReportRec->ExceptionNum);
        break; }
    }

    // not handled
    return (XCPT_CONTINUE_SEARCH);
}

/*
 *@@ excHandlerQuiet:
 *      "quiet" xcpt handler, which simply suppresses exceptions;
 *      this is useful for certain error-prone functions, where
 *      exceptions are likely to appear, for example used by
 *      cmnCheckObject to implement a fail-safe SOM object check.
 *
 *      This does _not_ write an error log and makes _no_ sound.
 *      This simply jumps back to the trapping thread.
 *
 *      Other than that, this behaves like excHandlerLoud.
 *
 *      This is best registered thru the TRY_QUIET macro
 *      (new with V0.84, described in except.c).
 */

ULONG _System excHandlerQuiet(PEXCEPTIONREPORTRECORD pReportRec,
                                     PREGREC2 pRegRec2,
                                     PCONTEXTRECORD pContextRec,
                                     PVOID pv)
{
    if (pReportRec->fHandlerFlags & EH_EXIT_UNWIND)
       return (XCPT_CONTINUE_SEARCH);
    if (pReportRec->fHandlerFlags & EH_UNWINDING)
       return (XCPT_CONTINUE_SEARCH);
    if (pReportRec->fHandlerFlags & EH_NESTED_CALL)
       return (XCPT_CONTINUE_SEARCH);

    switch (pReportRec->ExceptionNum)
    {
        case XCPT_ACCESS_VIOLATION:
        case XCPT_INTEGER_DIVIDE_BY_ZERO:
        case XCPT_ILLEGAL_INSTRUCTION:
        case XCPT_PRIVILEGED_INSTRUCTION:
        case XCPT_INTEGER_OVERFLOW:

            // write excpt explanation only if the
            // resp. debugging #define is set (common.h)
            #ifdef DEBUG_WRITEQUIETEXCPT
            {
                FILE *file = excOpenTraplogFile();
                excExplainException(file, "excHandlerQuiet",
                                    pReportRec,
                                    pContextRec);
                fclose(file);
            }
            #endif

            // jump back to failing routine
            longjmp(pRegRec2->jmpThread, pReportRec->ExceptionNum);
        break;

        default:
             break;
    }
    return (XCPT_CONTINUE_SEARCH);
}





