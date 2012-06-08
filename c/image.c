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
#define INCL_OS2MM
#define INCL_MCIOS2
#define INCL_GPI
#define INCL_WIN
#define INCL_DOS

#include <os2.h>
#include <os2me.h>
#include <mmioos2.h>
#include <stdlib.h>
#include <string.h>

BOOL readBitmapDataFromFile( char *pszFileName, char** pvImageData, PBITMAPINFO2 * pbmpInfo2 )
{
  FOURCC fcc;
  MMFORMATINFO mmFormatInfo={0};
  MMIOINFO mmIOInfo={0};
  MMIMAGEHEADER mmImgHeader={0};
  HMMIO hMMIO;
  long    cbHeader;
  long cbData, cbRow, cbRead;
  HPS hps;
  HDC hdc;
  HAB hab=WinQueryAnchorBlock(HWND_DESKTOP);
  SIZEL slHPS;
  HBITMAP hbm;
  /* The following is for the color adjustment */
  BITMAPINFOHEADER2 *ptr;
  RGB2* rgb2;
  int a, r,g,b;
  char *pvImage;
  BITMAPINFOHEADER2 * pbmpIHdr2;

  //WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,pszFileName ,chrDesktopBG,1234, MB_OK|MB_MOVEABLE);

  if( mmioIdentifyFile( pszFileName, NULL, &mmFormatInfo, &fcc, 0UL, 0UL ) != MMIO_SUCCESS )
    {
      /* Unknown file format */
      return( FALSE );
    }

  if( mmFormatInfo.fccIOProc == FOURCC_DOS )
    {
      return( FALSE );
    }
  
  if( ( mmFormatInfo.ulMediaType != MMIO_MEDIATYPE_IMAGE ) ||
      ( ( mmFormatInfo.ulFlags & MMIO_CANREADTRANSLATED ) == 0 ) )
    {
      /* No image or can't translate it to bitmap */
      return( FALSE );
    }
  
  mmIOInfo.fccIOProc = mmFormatInfo.fccIOProc;
  mmIOInfo.ulTranslate = MMIO_TRANSLATEHEADER | MMIO_TRANSLATEDATA;
  /* Everything's setup for reading the image file */

  
    /* open the image file */
    if( ( hMMIO = mmioOpen( pszFileName,
                            &mmIOInfo,
                            MMIO_READ | MMIO_DENYNONE | MMIO_NOIDENTIFY ) ) == NULLHANDLE )
    {
        return( FALSE );
    }
    
    if( mmioQueryHeaderLength(  hMMIO, &cbHeader, 0UL, 0UL ) != MMIO_SUCCESS ||
        cbHeader != sizeof( MMIMAGEHEADER ) )
      {
        mmioClose( hMMIO, 0L );
        return( FALSE );
      }

    /* get the image header */    
    if( mmioGetHeader( hMMIO, &mmImgHeader, ( long )sizeof( MMIMAGEHEADER ), &cbHeader, 0L, 0L )
        != MMIO_SUCCESS )
      {
        mmioClose( hMMIO, 0L );
        return( FALSE );
      }
    
    /* For memory reasons we only allow 8bit images */
      if(8 != mmImgHeader.mmXDIBHeader.BMPInfoHeader2.cBitCount) {
        mmioClose( hMMIO, 0L );
        return (FALSE);
      }
    
      /* The image must have the size of the screen */
      if(mmImgHeader.mmXDIBHeader.BMPInfoHeader2.cx != WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN)
         /*|| mmImgHeader.mmXDIBHeader.BMPInfoHeader2.cy != WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) 
           Removed this check so people who adapted their desktop image size to prevent scaling by the warpcenter can use the class. */ 
         ) {
        mmioClose( hMMIO, 0L );
        return (FALSE);
      }

      cbRow = ((mmImgHeader.mmXDIBHeader.BMPInfoHeader2.cx *
                       mmImgHeader.mmXDIBHeader.BMPInfoHeader2.cBitCount + 31) / 32) * 4;
      cbData = cbRow * mmImgHeader.mmXDIBHeader.BMPInfoHeader2.cy;
      
      /* Alloc mem for the image data */
      if (DosAllocMem((PVOID)&(pvImage), cbData, PAG_READ | PAG_WRITE | PAG_COMMIT) != NO_ERROR)
        {
          mmioClose( hMMIO, 0L );
          return ( FALSE );
        }
      /* Read image data */
      if( ( cbRead =  mmioRead( hMMIO, pvImage, cbData ) ) == MMIO_ERROR || !cbRead )
        {
          DosFreeMem(pvImage);
          mmioClose( hMMIO, 0L );
          return ( FALSE );
        }

      /* Copy BMP-info and palette */
      if(DosAllocMem((PVOID)&pbmpIHdr2, mmImgHeader.mmXDIBHeader.BMPInfoHeader2.cbFix+256*sizeof(RGB2),
                     PAG_COMMIT|PAG_READ|PAG_WRITE)==NO_ERROR) {
        memcpy(pbmpIHdr2,& (mmImgHeader.mmXDIBHeader.BMPInfoHeader2),
               mmImgHeader.mmXDIBHeader.BMPInfoHeader2.cbFix+256*sizeof(RGB2));
      }
      else
        {
          DosFreeMem(pvImage);
          mmioClose( hMMIO, 0L );
          mmioClose( hMMIO, 0L );
          return ( FALSE );
        }

      /* Close the image file */
      mmioClose( hMMIO, 0L );

      *pvImageData=pvImage;
      *pbmpInfo2=(PBITMAPINFO2) pbmpIHdr2 ;

      return TRUE;
}

HBITMAP createBitmapFromData(char * pvImage, BITMAPINFO2 * pbmpI2, ULONG ulRed, ULONG ulGreen, ULONG ulBlue)
{
  HAB hab=WinQueryAnchorBlock(HWND_DESKTOP);
  HPS hps;
  HDC hdc;
  SIZEL slHPS;
  HBITMAP hbm;
  PBITMAPINFO2 pbmpInfoTable2;
  RGB2* rgb2;
  int a, r,g,b, r2,g2,b2;
  BITMAPINFOHEADER2 *ptr;
  
  if(!pbmpI2 || !pvImage)
    return NULLHANDLE;

  if((pbmpInfoTable2=(PBITMAPINFO2)malloc(sizeof(BITMAPINFOHEADER2)+256*sizeof(RGB2)))==NULLHANDLE)
    return NULLHANDLE;

  memcpy(pbmpInfoTable2, pbmpI2, sizeof(BITMAPINFOHEADER2)+256*sizeof(RGB2));

  /* Adapt colors */
  ptr=(BITMAPINFOHEADER2*)pbmpInfoTable2;
  ptr++;/* Jump to end of bmp header */
  rgb2=(RGB2*) ptr;

  r2=((BYTE)ulRed)-128;
  g2=((BYTE)ulGreen)-128;
  b2=((BYTE)ulBlue)-128;
  
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
  
  // convert data to system bitcount
  hdc = DevOpenDC(hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA) NULL, NULLHANDLE);
  slHPS.cx = pbmpI2->cx; 
  slHPS.cy = pbmpI2->cy; 
  hps = GpiCreatePS(hab,
                    hdc,
                    &slHPS,
                    PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);

  //pbmpI2->cy-=250;

  hbm = GpiCreateBitmap(hps,
                        (PBITMAPINFOHEADER2)pbmpI2,
                        CBM_INIT,
                        pvImage,
                        pbmpInfoTable2);
  // pbmpI2->cy+=250;

  GpiDestroyPS(hps);
  DevCloseDC(hdc);
  
  free(pbmpInfoTable2);

  return hbm;
}


