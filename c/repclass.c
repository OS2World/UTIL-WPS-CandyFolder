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
#define INCL_DOSMISC       /* DOS Miscellaneous values */
#define INCL_DOSERRORS     /* DOS Error values         */
#define INCL_DOSFILEMGR
#define INCL_WINWORKPLACE

#include <os2.h>
#include <stdio.h>
 
void usage(char *progName) {
  printf("Program to replace a WPS class.\n");
  printf("(c) Chris Wohlgemuth 2001\n");
  printf("http://www.geocities.com/SiliconValley/Sector/5785/\n\n");
      
  fprintf(stderr,"Usage:\n");
  fprintf(stderr,"%s <Class to replace> <Replacement class> [u]\n",progName);
  fprintf(stderr,"If 'u' is given the class will be unreplaced.\n\n");

}

int main(int argc, char* argv[])  {
	

	APIRET  rc                  = NO_ERROR;  /* Return code                    */
    BOOL bReplace=TRUE;
	
    if(argc<3) {
      usage(argv[0]);
      exit(1);
    }
    if(argc==4) 
      if(argv[3]=="u")
        bReplace=FALSE;
	
    if(WinReplaceObjectClass(argv[1] , argv[2], bReplace)){
      if(bReplace)
        printf("Class replaced.\n");
      else
        printf("Class unreplaced.\n");
      printf("You have to reboot so the change can take effect.\n");
    }
    else{
      printf("!! Couldn't replace the class !!.\n\n");
      DosBeep(100,200);
      return 1;
	}
	printf("Done.\n\n");
	
	return 0;
}






