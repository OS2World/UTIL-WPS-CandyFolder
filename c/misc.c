/*
 * This file is (C) Chris Wohlgemuth 2001
 * It is part of the CandyFolder package
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
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#define INCL_WIN
#define INCL_DOS

#include <os2.h>

#include <os2me.h>
#include <stdio.h>
#include <mmioos2.h>

#include <stdio.h>
#include <wpclsmgr.h> /* For wpModuleForClass() */
#include "candyinc.h"

BOOL getMessage(char* text, ULONG ulID, LONG lSizeText, HMODULE hResource, HWND hwnd)
{
  if(!WinLoadString(WinQueryAnchorBlock(hwnd),hResource,ulID,lSizeText,text)) {
    sprintf(text,"");
    return FALSE;
  }
  return TRUE;
}

/* This function returns the module handle of our class-dll */
HMODULE queryModuleHandle(void)
{
  static HMODULE hmod=0;

  if(!hmod) {
    PSZ pathname=  //Query Pathname of class file
      _wpModuleForClass(SOMClassMgrObject,"CandyFolder");

    DosQueryModuleHandle(pathname,&hmod);
  }
  return hmod;
}

/****************************************************
 *                                                  *
 * This funktion returns the running OS version:    *
 *                                                  *
 * 30: Warp 3, 40 Warp 4 etc.                       *
 *                                                  *
 ****************************************************/
ULONG cwQueryOSRelease(void)
{
  static ULONG ulVersionMinor=0;

  if(!ulVersionMinor)
    if(DosQuerySysInfo(QSV_VERSION_MINOR, QSV_VERSION_MINOR, &ulVersionMinor, sizeof(ulVersionMinor)))
      ulVersionMinor=30;/* Default Warp 3 */

  return ulVersionMinor;

}

/*
 *@@ winhAssertWarp4Notebook:
 *      this takes hwndDlg as a notebook dialog page and
 *      goes thru all its controls. If a control with an
 *      ID <= udIdThreshold is found, this is assumed to
 *      be a button which is to be given the BS_NOTEBOOKBUTTON
 *      style. You should therefore give all your button
 *      controls which should be moved such an ID.
 *      You can also specify how many dialog units
 *      all the other controls will be moved downward in
 *      ulDownUnits; this is useful to fill up the space
 *      which was used by the buttons before moving them.
 *      Returns TRUE if anything was changed.
 *
 *      This function is useful if you wish to create
 *      notebook pages using dlgedit.exe which are compatible
 *      with both Warp 3 and Warp 4. This should be executed
 *      in WM_INITDLG of the notebook dlg function if the app
 *      has determined that it is running on Warp 4.
 */
/*
 *      Copyright (C) 1997-99 Ulrich M”ller.
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
 *   Modified by Chris Wohlgemuth 2001 for use with CandyFolder
 */
BOOL winhAssertWarp4Notebook(HWND hwndDlg,
                    USHORT usIdThreshold,    // in: ID threshold
                    ULONG ulDownUnits)       // in: dialog units or 0
{
    BOOL brc = FALSE;
    POINTL ptl;
    HAB hab = WinQueryAnchorBlock(hwndDlg);
    BOOL    fIsVisible = WinIsWindowVisible(hwndDlg);
    if (ulDownUnits) {
        ptl.x = 0;
        ptl.y = ulDownUnits;
        WinMapDlgPoints(hwndDlg, &ptl, 1, TRUE);
    }

    WinEnableWindowUpdate(hwndDlg, FALSE);

    if (cwQueryOSRelease()>=40) {
        HENUM henum = WinBeginEnumWindows(hwndDlg);
        HWND hwndItem;
        while ((hwndItem = WinGetNextWindow(henum))!=NULLHANDLE) {
            USHORT usId = WinQueryWindowUShort(hwndItem, QWS_ID);

            if (usId <= usIdThreshold) {
                // pushbutton to change:
              WinSetWindowBits(hwndItem, QWL_STYLE,
                        BS_NOTEBOOKBUTTON, BS_NOTEBOOKBUTTON);
                brc = TRUE;
            } else
                // no pushbutton to change: move downwards
                // if desired
                if (ulDownUnits)
                {
                    SWP swp;

                    WinQueryWindowPos(hwndItem, &swp);
                    WinSetWindowPos(hwndItem, 0,
                        swp.x,
                        swp.y - ptl.y,
                        0, 0,
                        SWP_MOVE);
                }
        }
        WinEndEnumWindows(henum);
    }
    if (fIsVisible)
        WinShowWindow(hwndDlg, TRUE);
    return (brc);
}
