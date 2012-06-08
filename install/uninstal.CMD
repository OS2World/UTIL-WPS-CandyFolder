/*                 Uninstall
 *
 *                   (C) Chris Wohlgemuth 2001
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

 call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
 call SysLoadFuncs

nlsStrings.=''
call getStrings

'cls'

say nlsStrings.1
say ''
say '>'
parse value SysCurPos() with aktzeile aktspalte
newposition=SysCurPos(aktzeile-1,2)
pull

/* Clear screen */
'cls'

/* Get curent directory */
curdir = directory()

say nlsStrings.2||curdir
say ''


SAY nlsStrings.3
'repclass WPFolder CandyFolder u'

say ''
say nlsStrings.4||'CandyFolder'
		if SysDeregisterObjectClass("CandyFolder") <>1 then
		do
		  say nlsStrings.5||'CandyFolder'
		 end


say nlsStrings.6
say ''
say '>'
parse value SysCurPos() with aktzeile aktspalte
newposition=SysCurPos(aktzeile-1,2)
pull

exit

/* The following subroutine was appended while building the distribution:

GetStrings:
nlsStrings.1="..."
...
return

The routine adds the NLS strings to this script. For creating other NLS-versions
translate the files in /nls and change the makefile to include the new NLS-version.
*/

