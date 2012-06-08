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
#define INCL_GPI
#define INCL_DOS
#include <os2.h>

#include "candyinc.h"
#include "candyfld.ih"
#include <wpdesk.ih>

#include "debug.h"
#include "except.h"

extern PFNWP pfnwpGenericNB;
extern PFNWP pfnwpBgNB;

extern char *pvImageData;
extern BITMAPINFO2 * pbmpInfo2;

extern HMODULE hRessource;
extern ULONG ulRedDefault;
extern ULONG ulGreenDefault;
extern ULONG ulBlueDefault;

extern BOOL getMessage(char* text,ULONG ulID, LONG lSizeText, HMODULE hResource,HWND hwnd);
extern BOOL winhAssertWarp4Notebook(HWND hwndDlg,
                    USHORT usIdThreshold,    // in: ID threshold
                    ULONG ulDownUnits);       // in: dialog units or 0

static MRESULT EXPENTRY transparencyExampleProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) ;

void enableImageControls(HWND hwnd, BOOL bEnable)
{
  /* Groupbox */
  if(bEnable)
    WinEnableWindow(WinWindowFromID(hwnd, 0x12c),bEnable);
  /* Text */
  WinEnableWindow(WinWindowFromID(hwnd, 0x12d),bEnable);
  /* Combo box */
  WinEnableWindow(WinWindowFromID(hwnd, 0x12e),bEnable);
  /* Button 'create' */
  WinEnableWindow(WinWindowFromID(hwnd, 0x130),bEnable);
  /* Button 'edit' */
  WinEnableWindow(WinWindowFromID(hwnd, 0x12f),bEnable);
  /* Button 'search' */
  WinEnableWindow(WinWindowFromID(hwnd, 0x85d),bEnable);
  /* Radio Button 'normal' */
  WinEnableWindow(WinWindowFromID(hwnd, 0x132),bEnable);
  /* Radio button */
  WinEnableWindow(WinWindowFromID(hwnd, 0x133),bEnable);
  /* Radio button */
  WinEnableWindow(WinWindowFromID(hwnd, 0x134),bEnable);
  /* Text */
  WinEnableWindow(WinWindowFromID(hwnd, 0x643),bEnable);
  /* Spin button */
  WinEnableWindow(WinWindowFromID(hwnd, 0x13a),bEnable);

}

void showImageControl(HWND hwnd, BOOL bShow)
{

  /* Two cases prevents flickering */
  if(!bShow) {
    WinShowWindow(WinWindowFromID(hwnd, IDST_IMAGERECT), bShow);
    WinShowWindow(WinWindowFromID(hwnd, 0x138), !bShow);
  }
  else
    {
      WinShowWindow(WinWindowFromID(hwnd, 0x138), !bShow);
      WinShowWindow(WinWindowFromID(hwnd, IDST_IMAGERECT), bShow);
  }

}


/* Extended background page proc. This procedure is used for the handling of the
   transparency checkbox */
MRESULT EXPENTRY bgNBProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) 
{
  CandyFolderData *somThis;  
  CandyFolder  *fldrPtr;
  MRESULT mr;
  
  switch(msg)
    {
    case WM_CONTROL:
      {
        BOOL bReturnMR=FALSE;

        TRY_LOUD(CONTROL_EXT) {        
          switch(SHORT1FROMMP(mp1))
            {
            case IDDD_FILENAME:
              if(SHORT2FROMMP(mp1)==CBN_EFCHANGE) {
                /* A new filename was selected. */ 
                fldrPtr=(CandyFolder*) WinQueryWindowULong(WinWindowFromID(hwnd, ID_OBJECTSTOREWINDOW),QWL_USER);// object ptr.
                if(somIsObj(fldrPtr)) {
                  somThis = CandyFolderGetData(fldrPtr);
                  if(somResolveByName((WPDesktop*)fldrPtr, "wpIsCurrentDesktop")!=NULL) {
                    mr=pfnwpBgNB(hwnd, msg, mp1, mp2);
                    somThis->ulTrans&=~ID_COLORONLY;
                    _wpSaveImmediate(fldrPtr);
                    bReturnMR=TRUE;
                    //     return mr;
                  }
                }
              }
              break;
            case IDCB_COLORONLY:
              /* Color only check box */
              if(SHORT2FROMMP(mp1)==BN_CLICKED) {
                fldrPtr=(CandyFolder*) WinQueryWindowULong(WinWindowFromID(hwnd, ID_OBJECTSTOREWINDOW),QWL_USER);// object ptr.
                if(somIsObj(fldrPtr)) {
                  somThis = CandyFolderGetData(fldrPtr);
                  if(WinQueryButtonCheckstate(hwnd, IDCB_COLORONLY)==1) {
                    WinCheckButton(hwnd, IDCB_TRANSPARENCY, 0 );
                    somThis->ulTrans&=~ID_TRANSPARENCY;
                    if(somResolveByName((WPDesktop*)fldrPtr, "wpIsCurrentDesktop")==NULL)
                      _cfRefreshContainerBackgrounds(fldrPtr, FALSE);
                    else{
                      /* It's the desktop */
                      //      somThis->ulTrans&=~ID_TRANSPARENCY; /* Desktop never has transparency. */
                      somThis->ulTrans|=ID_COLORONLY;
                      _wpSaveImmediate(fldrPtr);
                    }
                  }
                  else {
                    if(somResolveByName((WPDesktop*)fldrPtr, "wpIsCurrentDesktop")==NULL)
                      _cfRefreshContainerBackgrounds(fldrPtr, FALSE);
                    else{
                      /* It's the desktop */
                      /* Color only is unchecked */
                      somThis->ulTrans&=~ID_COLORONLY;
                      _wpSaveImmediate(fldrPtr);
                    }              
                  }
                }/* if(somIsObj(fldrPtr)) */
                
                mr=pfnwpBgNB(hwnd, msg, mp1, mp2);
                if(somIsObj(fldrPtr)) {
                  if(WinQueryButtonCheckstate(hwnd, IDCB_COLORONLY)==1) {
                    /* Color only selected */
                    WinEnableWindow(WinWindowFromID(hwnd, IDCB_TRANSPARENCY), FALSE);
                    showImageControl(hwnd, FALSE);
                  }
                  else {
                    /* Image */
                    WinEnableWindow(WinWindowFromID(hwnd, IDCB_TRANSPARENCY), TRUE);
                    /* Groupbox */
                    //WinEnableWindow(WinWindowFromID(hwnd, 0x12c),TRUE);
                    if(WinQueryButtonCheckstate(hwnd, IDCB_TRANSPARENCY)==1) {
                      showImageControl(hwnd, TRUE);
                      enableImageControls(hwnd, FALSE);/* if transparency disable the image controls */
                    }
                    else {
                      showImageControl(hwnd, FALSE);
                      enableImageControls(hwnd, TRUE);
                    }
                  }
                }
                bReturnMR=TRUE;
                //  return mr;
              }/* if(SHORT2FROMMP(mp1)==BN_CLICKED) */
              break;
            case IDCB_TRANSPARENCY:
              fldrPtr=(CandyFolder*) WinQueryWindowULong(WinWindowFromID(hwnd, ID_OBJECTSTOREWINDOW),QWL_USER);// object ptr.
              if(somIsObj(fldrPtr)) {
                somThis = CandyFolderGetData(fldrPtr);
                if(WinQueryButtonCheckstate(hwnd, IDCB_TRANSPARENCY)==1) {
                  somThis->ulTrans|=ID_TRANSPARENCY;
                  enableImageControls(hwnd, FALSE);/* if transparency disable the image controls */
                  showImageControl(hwnd, TRUE);
                }
                else {
                  somThis->ulTrans&=~ID_TRANSPARENCY;
                  enableImageControls(hwnd, TRUE);
                  showImageControl(hwnd, FALSE);
                }
                _cfRefreshContainerBackgrounds(fldrPtr, TRUE);
              }
              mr=(MRESULT)0;
              bReturnMR=TRUE;
              break;
              //  return (MRESULT)0;
            default:
              break;
            }/* switch */
        }
        CATCH(CONTROL_EXT)
          {
          } END_CATCH;
          
          if(bReturnMR)
            return mr;
          
          break;
      }/* case WM_CONTROL */
    case WM_COMMAND:
      switch(SHORT1FROMMP(mp1))
        {
        case IDPB_CANCEL:
          /* Widerrufen */
          mr=pfnwpBgNB(hwnd, msg, mp1, mp2);
          fldrPtr=(CandyFolder*) WinQueryWindowULong(WinWindowFromID(hwnd, ID_OBJECTSTOREWINDOW),QWL_USER);// object ptr.
          if(somIsObj(fldrPtr)) {
            somThis = CandyFolderGetData(fldrPtr);
            if(WinQueryWindowULong(WinWindowFromID(hwnd, IDCB_TRANSPARENCY), QWL_USER) & ID_TRANSPARENCY) {
              WinCheckButton(hwnd, IDCB_TRANSPARENCY, 1 );
              WinCheckButton(hwnd, IDCB_COLORONLY, 0 );
              somThis->ulTrans|=ID_TRANSPARENCY;
              showImageControl(hwnd, TRUE);
              //            if(WinQueryButtonCheckstate(hwnd, 0x127)==0) 
              WinEnableWindow(WinWindowFromID(hwnd, IDCB_TRANSPARENCY), TRUE);
            }
            else {
              WinCheckButton(hwnd, IDCB_TRANSPARENCY, 0 );
              somThis->ulTrans&=~ID_TRANSPARENCY;
              showImageControl(hwnd, FALSE);
              //   if(WinQueryButtonCheckstate(hwnd, 0x127)==1) 
              WinEnableWindow(WinWindowFromID(hwnd, IDCB_TRANSPARENCY), FALSE);
            }
            _cfRefreshContainerBackgrounds(fldrPtr, FALSE);
          }
          return mr;
        default:
          break;
        }
      break;
    case WM_DESTROY:
      fldrPtr=(CandyFolder*) WinQueryWindowULong(WinWindowFromID(hwnd, ID_OBJECTSTOREWINDOW),QWL_USER);// object ptr.
      if(somIsObj(fldrPtr)) {
        somThis = CandyFolderGetData(fldrPtr);

        if(WinQueryButtonCheckstate(hwnd, IDCB_COLORONLY)==0) {
          /* Color only is unchecked */
          somThis->ulTrans&=~ID_COLORONLY;
          if(WinQueryButtonCheckstate(hwnd, IDCB_TRANSPARENCY)==1) {
            /* Transparency selected */
            somThis->ulTrans|=ID_TRANSPARENCY;           
            /* Save status immediately so we can change the selected bmp in wpSaveState() to color only. This
               way the WPS does not load an additional background image and paints it when openeing the folder.
               Mark in Instance data that we have to change the folder to color only. So we do it only one
               time in _wpSaveState(). */
            somThis->ulHaveToSave=1;
          }
          else
            somThis->ulTrans&=~ID_TRANSPARENCY;
        }
        else {
          somThis->ulTrans&=~ID_TRANSPARENCY; /* If we have color only, we can't have transparency. */
          somThis->ulTrans|=ID_COLORONLY;
        }
        _wpSaveImmediate(fldrPtr);
      }
      break;
    default:
      break;
    }
  return  pfnwpBgNB(hwnd, msg, mp1, mp2);
}

/* The new notebook proc */
MRESULT EXPENTRY candyNBProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) 
{
  CandyFolderData *somThis;  
  CandyFolder  *fldrPtr;
  HWND hwndCB, hwndPtr;
  SWP swp;

  switch(msg)
    {
    case BKM_SETPAGEWINDOWHWND:
      /* A notebook page is inserted into the notebook */
      fldrPtr=(CandyFolder*) WinQueryWindowULong(WinWindowFromID(hwnd, ID_OBJECTSTOREWINDOW),QWL_USER);// object ptr.
      if(somIsObj(fldrPtr)) {
        somThis = CandyFolderGetData(fldrPtr);
        /* Check if it's the background page */
        if(somThis->ulBackgroundPageID==LONGFROMMP(mp1)) {
          SWP swp={0};
          char cbText[50];
          
          /* Get size and position of the 'normal' radio button */
          WinQueryWindowPos(WinWindowFromID(HWNDFROMMP(mp2),0x132),&swp);
          /* Get text for the transparency checkbox from the ressource file */
          if(!getMessage(cbText, IDSTR_TRANSCHECKBOXTEXT, sizeof(cbText), hRessource, hwnd))
            strcpy(cbText,"Transparency");
          
          /* Create the transparency checkbox */
          if((hwndCB=WinCreateWindow(HWNDFROMMP(mp2), WC_BUTTON, cbText, WS_VISIBLE|BS_AUTOCHECKBOX,
                                     swp.x+swp.cx+20, swp.y, 120, swp.cy,HWNDFROMMP(mp2),HWND_TOP, IDCB_TRANSPARENCY,NULL,NULL))!=NULLHANDLE) {
            /* Create a hidden window to store the object ptr */
            if((hwndPtr=WinCreateWindow(HWNDFROMMP(mp2), WC_STATIC, "Obj_Ptr_Save",SS_TEXT,0,0,0,0, 
                                        HWNDFROMMP(mp2),HWND_TOP, ID_OBJECTSTOREWINDOW,NULL,NULL))!=NULLHANDLE) {
              WinSetWindowULong(hwndPtr,QWL_USER,(ULONG) fldrPtr);//Save object ptr.
              
              /* Create an image example rect used if transparency is selected */
              WinQueryWindowPos(WinWindowFromID(HWNDFROMMP(mp2), IDRB_NORMAL),&swp);
              if((hwndPtr=WinCreateWindow(HWNDFROMMP(mp2), WC_STATIC, "",SS_FGNDRECT|WS_VISIBLE,swp.x,swp.y,swp.cx,swp.cy, 
                                          HWNDFROMMP(mp2),HWND_TOP, IDST_IMAGERECT,NULL,NULL))!=NULLHANDLE) {
                WinSubclassWindow(hwndPtr, transparencyExampleProc);
              }
            }
            /* Check if we are a desktop folder */
            if(somResolveByName(fldrPtr, "wpIsCurrentDesktop")!=NULL) /* If yes, hide the transparency checkbox */
              WinShowWindow(WinWindowFromID(HWNDFROMMP(mp2), IDCB_TRANSPARENCY), FALSE);
            
            if(somThis->ulTrans & ID_TRANSPARENCY) {
              WinCheckButton(HWNDFROMMP(mp2), IDCB_TRANSPARENCY, 1 );
              /* Uncheck 'color only' check button */
              WinCheckButton(HWNDFROMMP(mp2), IDCB_COLORONLY, 0);
              WinSetWindowPos(WinWindowFromID(HWNDFROMMP(mp2),0x138),NULLHANDLE,0,0,0,0,SWP_HIDE);
            }
            else {
              WinCheckButton(HWNDFROMMP(mp2), IDCB_TRANSPARENCY, 0 );
              //showImageControl(hwnd, FALSE);
              WinSetWindowPos(WinWindowFromID(HWNDFROMMP(mp2),IDST_IMAGERECT),NULLHANDLE,0,0,0,0,SWP_HIDE);
            }
            /* Save state of transparency for undo */
            WinSetWindowULong(WinWindowFromID(HWNDFROMMP(mp2), IDCB_TRANSPARENCY), QWL_USER, somThis->ulTrans);
            
            if(WinQueryButtonCheckstate(HWNDFROMMP(mp2), IDCB_COLORONLY)==1) {
              /* Disable the transparency checkbox if 'color only' is selected */
              WinEnableWindow(WinWindowFromID(HWNDFROMMP(mp2), IDCB_TRANSPARENCY), FALSE);
            }
          }
          pfnwpBgNB=WinSubclassWindow(HWNDFROMMP(mp2),bgNBProc);
        }
        somThis->ulHaveToSave=0;
      }
      break;
    default:
      break;
    }
  return  pfnwpGenericNB(hwnd, msg, mp1, mp2);
}


static MRESULT EXPENTRY transparencyExampleProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) 
{
  RECTL rcl;
  HPS hps;
  POINTL ptl[4];
  HBITMAP hbm, hbmOld;
  BITMAPINFOHEADER2 *ptr;
  RGB2* rgb2;
  int a, r,g,b,r2,g2,b2;
  char *pvImage;
  CandyFolder  *fldrPtr;
  CandyFolderData *somThis;

  switch(msg)
    {
    case WM_PAINT:
      hps=WinBeginPaint(hwnd, NULLHANDLE, NULL);

      TRY_LOUD(PAINT) {
        WinQueryWindowRect(hwnd, &rcl);
        
        fldrPtr=(CandyFolder*) WinQueryWindowULong(WinWindowFromID(WinQueryWindow(hwnd,QW_PARENT),ID_OBJECTSTOREWINDOW) ,QWL_USER);// object ptr.
        if(somIsObj(fldrPtr)) {
          somThis = CandyFolderGetData(fldrPtr);
          
          if(pbmpInfo2 && pvImageData) {
            hbm=GpiCreateBitmap(hps, (PBITMAPINFOHEADER2)pbmpInfo2, 0, NULLHANDLE, pbmpInfo2);
            hbmOld=GpiSetBitmap(hps,hbm);
            
            ptr=malloc(sizeof(BITMAPINFOHEADER2)+sizeof(RGB2)*256);
            if(ptr) {
              memcpy(ptr, pbmpInfo2, sizeof(BITMAPINFOHEADER2)+sizeof(RGB2)*256);
              
              /* Change the image colors */
              ptr++;/* Jump to end of bmp header */
              rgb2=(RGB2*) ptr;
              ptr--;        
              
              r2=somThis->ulRed-128;
              g2=somThis->ulGreen-128;
              b2=somThis->ulBlue-128;
              
              for(a=16;a<255;a++,rgb2++) {
                r=r2;
                g=g2;
                b=b2;
                
                b+=rgb2->bBlue;
                g+=rgb2->bGreen;
                r+=rgb2->bRed;
                if(r>255)
                  r=255;
                if(r<0)
                  r=0;
                rgb2->bRed=r;
                
                if(g>255)
                  g=255;
                if(g<0)
                  g=0;
                rgb2->bGreen=g;
                
                if(b>255)
                  b=255;
                if(b<0)
                  b=0;
                rgb2->bBlue=b;        
              }
              
              ptl[0].x=0;
              ptl[0].y=0;
              ptl[1].x=rcl.xRight;
              ptl[1].y=rcl.yTop;
              ptl[2].x=0;
              ptl[2].y=0;
              ptl[3].x=pbmpInfo2->cx-1;
              ptl[3].y=pbmpInfo2->cy-1;
              
              GpiDrawBits(hps, pvImageData, (PBITMAPINFO2) ptr, 4L, ptl, ROP_SRCCOPY, BBO_IGNORE);
              free(ptr);
            }/* if(ptr) */
            GpiSetBitmap(hps, hbmOld);
            GpiDeleteBitmap(hbm);
          }/* End of if(pbmpInfo2 && pvImageData) */
          else
            WinFillRect(hps, &rcl, CLR_WHITE);
        }/* if(somIsObj) */        
        else
          WinFillRect(hps, &rcl, CLR_WHITE);
      }
      CATCH(PAINT)
        {
        } END_CATCH;
        
        WinEndPaint(hps);
      return (MRESULT)0;
    default:
      break;
    }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY transparencySettingsProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) 
{
  SHORT sLen, sPos;
  MRESULT mr;
  CandyFolder  *fldrPtr;
  CandyFolderData *somThis;
  RECTL rcl;
  HWND hwndPtr;
  ULONG ulValue;

    switch (msg)
    {
    case WM_INITDLG:
      {
        // WinSetWindowULong(WinWindowFromID(hwnd, IDST_IMAGERECT), QWL_USER, (ULONG)PVOIDFROMMP(mp2));
        fldrPtr=(CandyFolder*)PVOIDFROMMP(mp2);
        if((hwndPtr=WinCreateWindow(hwnd, WC_STATIC, "Obj_Ptr_Save",SS_TEXT,0,0,0,0,hwnd,HWND_TOP, ID_OBJECTSTOREWINDOW,NULL,NULL))!=NULLHANDLE)
          WinSetWindowULong(hwndPtr,QWL_USER,(ULONG)PVOIDFROMMP(mp2));//Save object ptr.
        WinSubclassWindow(WinWindowFromID(hwnd, IDST_IMAGERECT),transparencyExampleProc);
        mr=WinSendMsg(WinWindowFromID(hwnd, IDSL_RED),SLM_QUERYSLIDERINFO,
                      MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),NULL);
        
        /* Set limits for color spin buttons */
        WinSendMsg(WinWindowFromID(hwnd,IDSB_RED),SPBM_SETLIMITS,MPFROMLONG(128),MPFROMLONG(-127));
        WinSendMsg(WinWindowFromID(hwnd,IDSB_GREEN),SPBM_SETLIMITS,MPFROMLONG(128),MPFROMLONG(-127));
        WinSendMsg(WinWindowFromID(hwnd,IDSB_BLUE),SPBM_SETLIMITS,MPFROMLONG(128),MPFROMLONG(-127));

        sLen=SHORT2FROMMR(mr)-1; /* Len of slidershaft in pixel */
        if(somIsObj(fldrPtr)) {
          somThis = CandyFolderGetData(fldrPtr);

          /* Set slider positions */
          WinPostMsg(WinWindowFromID(hwnd, IDSL_RED),SLM_SETSLIDERINFO,
                     MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),
                     MPFROMSHORT((SHORT)((sLen*somThis->ulRed)/SLIDERSCALE)));
          /* Set slider positions */
          WinPostMsg(WinWindowFromID(hwnd, IDSL_GREEN),SLM_SETSLIDERINFO,
                     MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),
                     MPFROMSHORT((SHORT)((sLen*somThis->ulGreen)/SLIDERSCALE)));
          /* Set slider positions */
          WinPostMsg(WinWindowFromID(hwnd, IDSL_BLUE),SLM_SETSLIDERINFO,
                     MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),
                     MPFROMSHORT((SHORT)((sLen*somThis->ulBlue)/SLIDERSCALE)));
          
          WinPostMsg(WinWindowFromID(hwnd,IDSB_RED),SPBM_SETCURRENTVALUE,(MPARAM)(somThis->ulRed-127),MPFROMLONG(0));
          WinPostMsg(WinWindowFromID(hwnd,IDSB_GREEN),SPBM_SETCURRENTVALUE,(MPARAM)(somThis->ulGreen-127),MPFROMLONG(0));
          WinPostMsg(WinWindowFromID(hwnd,IDSB_BLUE),SPBM_SETCURRENTVALUE,(MPARAM)(somThis->ulBlue-127),MPFROMLONG(0));
        }
        
        WinSendMsg(WinWindowFromID(hwnd, IDSL_RED),SLM_SETSCALETEXT,MPFROMSHORT(0),"-");
        WinSendMsg(WinWindowFromID(hwnd, IDSL_RED),SLM_SETSCALETEXT,MPFROMSHORT(4),"0");
        WinSendMsg(WinWindowFromID(hwnd, IDSL_RED),SLM_SETSCALETEXT,MPFROMSHORT(8),"+");

        WinSendMsg(WinWindowFromID(hwnd, IDSL_GREEN),SLM_SETSCALETEXT,MPFROMSHORT(0),"-");
        WinSendMsg(WinWindowFromID(hwnd, IDSL_GREEN),SLM_SETSCALETEXT,MPFROMSHORT(4),"0");
        WinSendMsg(WinWindowFromID(hwnd, IDSL_GREEN),SLM_SETSCALETEXT,MPFROMSHORT(8),"+");

        WinSendMsg(WinWindowFromID(hwnd, IDSL_BLUE),SLM_SETSCALETEXT,MPFROMSHORT(0),"-");
        WinSendMsg(WinWindowFromID(hwnd, IDSL_BLUE),SLM_SETSCALETEXT,MPFROMSHORT(4),"0");
        WinSendMsg(WinWindowFromID(hwnd, IDSL_BLUE),SLM_SETSCALETEXT,MPFROMSHORT(8),"+");

        ulValue=somThis->ulRed*65536+somThis->ulGreen*256+somThis->ulBlue;
        WinSetWindowULong(WinWindowFromID(hwnd, IDST_IMAGERECT), QWL_USER, ulValue);

        /* Move default buttons on Warp 4 */
        winhAssertWarp4Notebook(hwnd,
                                IDDLG_TRANSPARENCY,    // in: ID threshold
                                12);       // in: dialog units or 0

        break;
      }
    case WM_DESTROY:
      fldrPtr=(CandyFolder*) WinQueryWindowULong(WinWindowFromID(hwnd, ID_OBJECTSTOREWINDOW),QWL_USER);// object ptr.
      if(somIsObj(fldrPtr)) {
        somThis = CandyFolderGetData(fldrPtr);
        WinSendMsg(WinWindowFromID(hwnd,IDSB_RED),SPBM_QUERYVALUE,MPFROMP(&ulValue),
                   MPFROM2SHORT(0,SPBQ_ALWAYSUPDATE));
        somThis->ulRed=ulValue+127;
        WinSendMsg(WinWindowFromID(hwnd,IDSB_GREEN),SPBM_QUERYVALUE,MPFROMP(&ulValue),
                   MPFROM2SHORT(0,SPBQ_ALWAYSUPDATE));
        somThis->ulGreen=ulValue+127;
        WinSendMsg(WinWindowFromID(hwnd,IDSB_BLUE),SPBM_QUERYVALUE,MPFROMP(&ulValue),
                   MPFROM2SHORT(0,SPBQ_ALWAYSUPDATE));
        somThis->ulBlue=ulValue+127;
      }
      break;
    case WM_COMMAND:
      switch(SHORT1FROMMP(mp1))
      {
      case DID_CANCEL:
        ulValue=WinQueryWindowULong(WinWindowFromID(hwnd, IDST_IMAGERECT), QWL_USER);
        /* Set values of controls */
        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_RED), QWL_USER, 1);
        WinSendMsg(WinWindowFromID(hwnd,IDSB_RED),SPBM_SETCURRENTVALUE,(MPARAM)(((ulValue>>16) & 0xff)-127),MPFROMLONG(0));
        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_RED), QWL_USER, 0);
        
        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_GREEN), QWL_USER, 1);
        WinSendMsg(WinWindowFromID(hwnd,IDSB_GREEN),SPBM_SETCURRENTVALUE,(MPARAM)(((ulValue>>8) & 0xFF)-127), MPFROMLONG(0));
        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_GREEN), QWL_USER, 0);

        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_BLUE), QWL_USER, 1);
        WinSendMsg(WinWindowFromID(hwnd,IDSB_BLUE),SPBM_SETCURRENTVALUE,(MPARAM)((ulValue & 0xff)-127),MPFROMLONG(0));        
        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_BLUE), QWL_USER, 0);

        return (MRESULT)0;
      case IDPB_STANDARD:
        /* Set values of controls */
        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_RED), QWL_USER, 1);
        WinSendMsg(WinWindowFromID(hwnd,IDSB_RED),SPBM_SETCURRENTVALUE,(MPARAM)(ulRedDefault-127),MPFROMLONG(0));
        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_RED), QWL_USER, 0);

        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_GREEN), QWL_USER, 1);
        WinSendMsg(WinWindowFromID(hwnd,IDSB_GREEN),SPBM_SETCURRENTVALUE,(MPARAM)(ulGreenDefault-127), MPFROMLONG(0));
        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_GREEN), QWL_USER, 0);

        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_BLUE), QWL_USER, 1);
        WinSendMsg(WinWindowFromID(hwnd,IDSB_BLUE),SPBM_SETCURRENTVALUE,(MPARAM)(ulBlueDefault-127),MPFROMLONG(0));        
        WinSetWindowULong(WinWindowFromID(hwnd, IDSB_BLUE), QWL_USER, 0);
       
        return (MRESULT)0;
      default:
        break;
      }
    break;
    case WM_CONTROL:
      {
        fldrPtr=(CandyFolder*) WinQueryWindowULong(WinWindowFromID(hwnd, ID_OBJECTSTOREWINDOW),QWL_USER);// object ptr.
        if(somIsObj(fldrPtr)) {
          somThis = CandyFolderGetData(fldrPtr); 
          switch SHORT2FROMMP(mp1)
            {
            case SPBN_SETFOCUS:
              WinSetWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER, 1);
              break;
            case SPBN_KILLFOCUS:
              WinSetWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER, 0);
              break;
            case SPBN_CHANGE:
              if(WinIsWindowVisible(HWNDFROMMP(mp2))) {
                /* Make sure user is changing the value not system while building the dialog */
                
                switch (SHORT1FROMMP(mp1))
                  {
                  case IDSB_RED:
                    if(WinQueryWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER) & 1) {                 
                      WinSendMsg(WinWindowFromID(hwnd,IDSB_RED),SPBM_QUERYVALUE,MPFROMP(&ulValue),
                                 MPFROM2SHORT(0,SPBQ_ALWAYSUPDATE));
                      ulValue+=127;
                      somThis->ulRed=ulValue;
                      /* Set slider position */
                      mr=WinSendMsg(WinWindowFromID(hwnd, IDSL_RED),SLM_QUERYSLIDERINFO,
                                    MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),NULL);
                      sLen=SHORT2FROMMR(mr)-1;
                      WinSendMsg(WinWindowFromID(hwnd, IDSL_RED),SLM_SETSLIDERINFO,
                                 MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),
                                 MPFROMSHORT((SHORT)((sLen*ulValue)/SLIDERSCALE)));
                      WinInvalidateRect(WinWindowFromID(hwnd, IDST_IMAGERECT), NULL, FALSE);
                      _cfRefreshContainerBackgrounds(fldrPtr, TRUE);
                    }
                    return (MRESULT) 0;
                  case IDSB_GREEN:
                    if(WinQueryWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER) & 1) {
                      WinSendMsg(WinWindowFromID(hwnd,IDSB_GREEN),SPBM_QUERYVALUE,MPFROMP(&ulValue),
                                 MPFROM2SHORT(0,SPBQ_ALWAYSUPDATE));
                      ulValue+=127;
                      somThis->ulGreen=ulValue;
                      /* Set slider position */
                      mr=WinSendMsg(WinWindowFromID(hwnd, IDSL_GREEN),SLM_QUERYSLIDERINFO,
                                    MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),NULL);
                      sLen=SHORT2FROMMR(mr)-1;
                      WinSendMsg(WinWindowFromID(hwnd, IDSL_GREEN),SLM_SETSLIDERINFO,
                                 MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),
                                 MPFROMSHORT((SHORT)((sLen*ulValue)/SLIDERSCALE)));
                      WinInvalidateRect(WinWindowFromID(hwnd, IDST_IMAGERECT), NULL, FALSE);
                      _cfRefreshContainerBackgrounds(fldrPtr, TRUE);
                    }
                    return (MRESULT) 0;
                  case IDSB_BLUE:
                    if(WinQueryWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER) & 1) {
                      WinSendMsg(WinWindowFromID(hwnd,IDSB_BLUE),SPBM_QUERYVALUE,MPFROMP(&ulValue),
                                 MPFROM2SHORT(0,SPBQ_ALWAYSUPDATE));
                      ulValue+=127;
                      somThis->ulBlue=ulValue;
                      /* Set slider position */
                      mr=WinSendMsg(WinWindowFromID(hwnd, IDSL_BLUE),SLM_QUERYSLIDERINFO,
                                    MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),NULL);
                      sLen=SHORT2FROMMR(mr)-1;
                      WinSendMsg(WinWindowFromID(hwnd, IDSL_BLUE),SLM_SETSLIDERINFO,
                                 MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),
                                 MPFROMSHORT((SHORT)((sLen*ulValue)/SLIDERSCALE)));
                      WinInvalidateRect(WinWindowFromID(hwnd, IDST_IMAGERECT), NULL, FALSE);
                      _cfRefreshContainerBackgrounds(fldrPtr, TRUE);
                    }
                    return (MRESULT) 0;
                  default:
                    break;
                  }
              }
              break;
            case SLN_SETFOCUS:
              WinSetWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER, 1);
              //   WinSetWindowULong( WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER, WinQueryWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER) | 1);
              break;
            case SLN_KILLFOCUS:
              WinSetWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER, 0);
              //WinSetWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER, 
              //          WinQueryWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER) & 0xfffffffe);
              break;
            case SLN_CHANGE:
            case SLN_SLIDERTRACK:
              switch (SHORT1FROMMP(mp1))
                {
                case IDSL_RED:
                  if(WinQueryWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER) & 1) {
                    mr=WinSendMsg(WinWindowFromID(hwnd, IDSL_RED),SLM_QUERYSLIDERINFO,
                                  MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),NULL);
                    sPos=SHORT1FROMMR(mr);
                    sLen=SHORT2FROMMR(mr)-1;
                    somThis->ulRed=(sPos*SLIDERSCALE/sLen);
                    /* Set spin button value */
                    WinSendMsg(WinWindowFromID(hwnd,IDSB_RED),SPBM_SETCURRENTVALUE,(MPARAM)(somThis->ulRed-127),MPFROMLONG(0));
                    WinInvalidateRect(WinWindowFromID(hwnd, IDST_IMAGERECT), NULL, FALSE);
                    _cfRefreshContainerBackgrounds(fldrPtr, TRUE);
                  }
                  return (MRESULT) 0;  
                case IDSL_GREEN:
                  if(WinQueryWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER) & 1) {
                    mr=WinSendMsg(WinWindowFromID(hwnd, IDSL_GREEN),SLM_QUERYSLIDERINFO,
                                  MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),NULL);
                    sPos=SHORT1FROMMR(mr);
                    sLen=SHORT2FROMMR(mr)-1;
                    somThis->ulGreen=sPos*SLIDERSCALE/sLen;
                    /* Set spin button value */
                    WinSendMsg(WinWindowFromID(hwnd,IDSB_GREEN),SPBM_SETCURRENTVALUE,(MPARAM)(somThis->ulGreen-127),MPFROMLONG(0));
                    WinInvalidateRect(WinWindowFromID(hwnd, IDST_IMAGERECT), NULL, FALSE);
                    _cfRefreshContainerBackgrounds(fldrPtr, TRUE);
                  }
                  return (MRESULT) 0;  
                case IDSL_BLUE:
                  if(WinQueryWindowULong(WinWindowFromID(hwnd, SHORT1FROMMP(mp1)), QWL_USER) & 1) {
                    mr=WinSendMsg(WinWindowFromID(hwnd, IDSL_BLUE),SLM_QUERYSLIDERINFO,
                                  MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE),NULL);
                    sPos=SHORT1FROMMR(mr);
                    sLen=SHORT2FROMMR(mr)-1;
                    somThis->ulBlue=sPos*SLIDERSCALE/sLen;
                    /* Set spin button value */
                    WinSendMsg(WinWindowFromID(hwnd,IDSB_BLUE),SPBM_SETCURRENTVALUE,(MPARAM)(somThis->ulBlue-127),MPFROMLONG(0));
                    WinInvalidateRect(WinWindowFromID(hwnd, IDST_IMAGERECT), NULL, FALSE);
                    _cfRefreshContainerBackgrounds(fldrPtr, TRUE);
                  }
                  return (MRESULT) 0;  
                default:
                  break;
                }/* switch (SHORT1FROMMP(mp1))*/
              break;
            default:
              break;
            }/* switch (SHORT2FROMMP(mp1))*/
        }
        break;
      }
    default:
      break;
    }
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

