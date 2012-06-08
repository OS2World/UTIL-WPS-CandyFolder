
/*
 * except.h:
 *      header file for except.c.
 */

/*
 *      Copyright (C) 1999 Ulrich M”ller.
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, in version 2 as it comes in the COPYING
 *      file of the XFolder main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

#ifndef EXCEPT_HEADER_INCLUDED
    #define EXCEPT_HEADER_INCLUDED

#include <setjmp.h>
    /********************************************************************
     *                                                                  *
     *   Declarations                                                   *
     *                                                                  *
     ********************************************************************/

    // replacement EXCEPTIONREGISTRATIONRECORD struct
    // for thread exception handling
    typedef struct REGREC2_STRUCT {
        PVOID     pNext;
        PFN       pfnHandler;
        jmp_buf   jmpThread;
    } REGREC2, *PREGREC2;

    typedef struct _EXCEPTSTRUCT {
        REGREC2               RegRec2;
        ULONG                 ulExcpt;
        APIRET                arc;
    } EXCEPTSTRUCT, *PEXCEPTSTRUCT;

    /********************************************************************
     *                                                                  *
     *   Macros                                                         *
     *                                                                  *
     ********************************************************************/

    /* See except.c for explanations how to use these. */

    #define TRY_LOUD(excptstruct)                                          \
            {   /* register the "loud" exception handler */                \
                EXCEPTSTRUCT          excptstruct;                         \
                excptstruct.RegRec2.pfnHandler = (PFN)excHandlerLoud;      \
                excptstruct.arc = DosSetExceptionHandler(                  \
                            (PEXCEPTIONREGISTRATIONRECORD)&(excptstruct.RegRec2)); \
                if (excptstruct.arc)                                       \
                    DosBeep(100, 1000);                                    \
                excptstruct.ulExcpt = setjmp(excptstruct.RegRec2.jmpThread);               \
                if (excptstruct.ulExcpt == 0)                              \
                {   /* no exception occured: */

    #define TRY_QUIET(excptstruct)                                         \
            {   /* register the "quiet" exception handler */               \
                EXCEPTSTRUCT          excptstruct;                         \
                excptstruct.RegRec2.pfnHandler = (PFN)excHandlerQuiet;     \
                excptstruct.arc = DosSetExceptionHandler(                  \
                            (PEXCEPTIONREGISTRATIONRECORD)&(excptstruct.RegRec2)); \
                if (excptstruct.arc)                                       \
                    DosBeep(100, 1000);                                    \
                excptstruct.ulExcpt = setjmp(excptstruct.RegRec2.jmpThread);               \
                if (excptstruct.ulExcpt == 0)                              \
                {   /* no exception occured: */

    #define CATCH(excptstruct)                                             \
                    DosUnsetExceptionHandler(                              \
                            (PEXCEPTIONREGISTRATIONRECORD)&(excptstruct.RegRec2));   \
                } /* end if (try_ulExcpt == 0) */                          \
                else { /* exception occured: */                            \
                    DosUnsetExceptionHandler(                              \
                            (PEXCEPTIONREGISTRATIONRECORD)&(excptstruct.RegRec2));
    #define END_CATCH                                                      \
                } /* end else (excptstruct.ulExcpt == 0) */                \
            } /* end variable scope block */


    /*
     * CRASH:
     *      this macro is helpful for testing
     *      the exception handlers.
     *      This is not for general use. ;-)
     */

    #define CRASH {PSZ p = NULL; *p = 'a'; }

    /********************************************************************
     *                                                                  *
     *   Prototypes                                                     *
     *                                                                  *
     ********************************************************************/

    FILE* excOpenTraplogFile(VOID);

    ULONG _System excHandlerLoud(PEXCEPTIONREPORTRECORD pReportRec,
                                         PREGREC2 pRegRec2,
                                         PCONTEXTRECORD pContextRec,
                                         PVOID pv);

    ULONG _System excHandlerQuiet(PEXCEPTIONREPORTRECORD pReportRec,
                                         PREGREC2 pRegRec2,
                                         PCONTEXTRECORD pContextRec,
                                         PVOID pv);

#endif
