////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 RWS Inc, All Rights Reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of version 2 of the GNU General Public License as published by
// the Free Software Foundation
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
///////////////////////////////////////////////////////////////////////////////

// RSPix /////////////////////////////////////////////////////////////////////
#include <BLUE/System.h>
#include <CYAN/Cyan.h>

// Platform //////////////////////////////////////////////////////////////////
//#include <SDL/SDL.h>

// C++ ///////////////////////////////////////////////////////////////////////
#include <cstdio>
#include <cstdarg>


extern int16_t rspMsgBox(	// Returns RSP_MB_RET_*.  See switch statement below.
   uint16_t usFlags,		// MB_BUT/ICO_* flags specifying buttons and icons.
   const char *pszTitle,		// Title for box.
   const char *pszFrmt,			// Format for string.
   ...)						// Various shit.
{
  UNUSED(usFlags);

  char szOutput[4096];
  va_list varp;
  // Get pointer to the arguments.
  va_start(varp, pszFrmt);
  // Compose string.
  vsnprintf(szOutput, sizeof(szOutput), pszFrmt, varp);
  // Done with var arguments.
  va_end(varp);

  fprintf(stderr, "%s: %s", pszTitle, szOutput);

  fprintf(stderr, "STUBBED: %s:%d\n", __FILE__, __LINE__);
  return FAILURE;
}

extern int16_t rspOpenBox(								// Returns 0 if successfull, non-zero otherwise
	const char* pszBoxTitle,							// In:  Title of box
	const char*	pszDefaultPath,						// In:  Default directory and file
	char* pszSelectedFile,								// Out: File that user selected
	int16_t sSelectedFileBufSize,						// In:  Size of buffer pointed to by pszSelectedFile
   const char*	pszFilter /*= nullptr*/)				// In:  Filename filter or nullptr for none
{
  UNUSED(pszBoxTitle, pszDefaultPath, pszSelectedFile, sSelectedFileBufSize, pszFilter);
    fprintf(stderr, "STUBBED: %s:%d\n", __FILE__, __LINE__);
    return FAILURE;
}


extern int16_t rspSaveBox(			// Returns 0 on success.
	const char* pszBoxTitle,				// In:  Title of box.
	const char*	pszDefFileName,			// In:  Default filename.
	char* pszChosenFileName,		// Out: User's choice.
	int16_t sStrSize,					// In:  Amount of memory pointed to by pszChosenFileName.
   const char*	pszFilter /*= nullptr*/)	// In:  If not nullptr, '.' delimited extension based filename
											//	filter specification.  Ex: ".cpp.h.exe.lib" or "cpp.h.exe.lib"
											// Note: Cannot use '.' in filter.  Preceding '.' ignored.
{
  UNUSED(pszBoxTitle, pszBoxTitle, pszDefFileName, pszChosenFileName, sStrSize, pszFilter);
    fprintf(stderr, "STUBBED: %s:%d\n", __FILE__, __LINE__);
    return FAILURE;
}


extern void rspSetCursor(
	int16_t sCursorID)						// In:  ID of built-in cursor (use RSP_CURSOR_* macros)
{
  UNUSED(sCursorID);
    fprintf(stderr, "STUBBED: %s:%d\n", __FILE__, __LINE__);
}
