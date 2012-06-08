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
#ifndef CANDYINC_H_INCLUDED
#define CANDYINC_H_INCLUDED

/* Warp 4 notebook styles */
#ifndef BS_NOTEBOOKBUTTON
#define BS_NOTEBOOKBUTTON          8L
#endif

/* Key fo transparency enabling */
#define IDKEY_TRANSPARENCY  4500
/* These keys hold the color adjustment values */
#define IDKEY_TRANSRED      4501
#define IDKEY_TRANSGREEN      4502
#define IDKEY_TRANSBLUE      4503

/* The transparency enable bit saved in key IDKEY_TRANSPARENCY */
#define ID_TRANSPARENCY     0x1
#define ID_COLORONLY        0x2
#define TRANSP_ALLBIT     (ID_TRANSPARENCY | ID_COLORONLY)

/* Window IDs */ 
#define IDCB_TRANSPARENCY     555
#define ID_OBJECTSTOREWINDOW  556
/* Predefined window IDs from the Warp 4 background page */ 
#define IDCB_COLORONLY        0x127
#define IDRB_NORMAL           0x138
#define IDPB_CANCEL           0x13b
#define IDDD_FILENAME         0x12e

/* Transparency adjustment dialog IDs*/
#define IDDLG_TRANSPARENCY    100
#define IDST_IMAGERECT        110
#define IDSL_RED              120
#define IDSL_GREEN            121
#define IDSL_BLUE             122
#define IDSB_RED              123
#define IDSB_GREEN            124
#define IDSB_BLUE             125
#define IDST_RED              126
#define IDST_GREEN            127
#define IDST_BLUE             128
#define IDPB_STANDARD          3

/* String IDs for NLS */
#define IDSTR_CLASSTITLE          1600
#define IDSTR_TRANSCHECKBOXTEXT   1601
/* Used to calculate the color value from the slider position */
#define SLIDERSCALE    255

#endif       /* CANDYINC_H_INCLUDED */

