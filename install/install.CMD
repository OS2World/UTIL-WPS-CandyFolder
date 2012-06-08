/*           Install  
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

/* Get curent directory */
curdir = directory()

/* Clear screen */
'cls'

SAY nlsStrings.1
SAY nlsStrings.2
SAY ''
say nlsStrings.3
say ''
say '>'
parse value SysCurPos() with aktzeile aktspalte
newposition=SysCurPos(aktzeile-1,2)
pull

say nlsStrings.4||curdir
say ''

classdll=curdir'\candywps.dll'

say nlsStrings.5||'CandyFolder...'
if SysRegisterObjectClass("CandyFolder",classdll)<>1 then
	do	 	
                say ''
		say nlsStrings.6||'CandyFolder'
		say nlsStrings.7
		exit 	
	end 

/* Replacing classes */
'repclass WPFolder CandyFolder'

setupstring='BROWSER=netscape.exe;URL='||curdir'\docs\candy.htm;OBJECTID=<CANDY_README>'

if SysCreateObject("WPUrl",nlsStrings.8, '<WP_DESKTOP>',setupstring, 'update')<>1 then
	do	 	
                say ''
		say nlsStrings.9
		say nlsStrings.7
		exit 	
	end 

rc=SysSetObjectData('<CANDY_README>','OPEN=DEFAULT')

SAY ''
say nlsStrings.10
say nlsStrings.11
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