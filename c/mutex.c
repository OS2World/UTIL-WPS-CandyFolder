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
#define INCL_DOSSEMAPHORES
#include <os2.h>

HMTX hmtxBMP;

ULONG cfCreateMutex(void) {

  return DosCreateMutexSem( NULL, &hmtxBMP, 0, FALSE);

}

ULONG cfCloseMutex(void) {

  return DosCloseMutexSem( hmtxBMP );

}

ULONG cfRequestMutex(ULONG ulTimeOut) {

  return DosRequestMutexSem( hmtxBMP, ulTimeOut );

}

ULONG cfReleaseMutex(void) {

  return DosReleaseMutexSem( hmtxBMP );

}

