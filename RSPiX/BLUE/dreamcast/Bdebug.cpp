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
//
//	bdebug.cpp
// 
// History:
//		05/30/96 JMI	Started.
//
//		07/08/96	JMI	Now using exit(0) instead of FatalAppExit so that
//							atexit functions will get called.  Took out asm int 3
//							since I already had DebugBreak().
//
//		07/08/96	JMI	EXIT_SUCCESS now passed to exit instead of 0.
//
//		07/12/96	JMI	Removed excess param sExpr passed rspAssert.
//
//		07/16/96	JMI	Removed #if'd outness of this file in release mode.
//
//		10/18/96	JMI	szOutput in rspTrace is now static so it doesn't use
//							any stack space.  Changed str size for szOutput to
//							1024 bytes.  Changed ASSERT's string to same.
//
//		11/19/97	JMI	Added more debug output options via macros:
//							RSP_DEBUG_OUT_MESSAGEBOX, RSP_DEBUG_OUT_FILE, 
//							RSP_DEBUG_ASSERT_PASSIVE, & RSP_TRACE_LOG_NAME.
//							See below for details.
//
//		11/19/97	JMI	sSem was being decremented in the wrong spot.
//
//////////////////////////////////////////////////////////////////////////////
//
// Does all Windows specific debug stuff.
//
//	NOTE:  Define the following Macros at the compiler settings level to get
// other than default behavior:
//
//	- RSP_DEBUG_OUT_MESSAGEBOX	-- Use an rspMsgBox() for all TRACE calls.
// - RSP_DEBUG_OUT_FILE			-- Use a file for all TRACE calls.
// - RSP_DEBUG_ASSERT_PASSIVE	-- Just TRACE (do NOT show messagebox) for all
//										ASSERT failures -- Assumes user option 'Ignore'.
// - RSP_TRACE_LOG_NAME			-- Override the default name for the TRACE log
//										file.
//
//////////////////////////////////////////////////////////////////////////////

// RSPix /////////////////////////////////////////////////////////////////////
#include <BLUE/Blue.h>

// C++ ///////////////////////////////////////////////////////////////////////
#include <cstdio>
#include <cstdarg>
#include <csignal>
#include <ctime>


#ifdef RSP_DEBUG_OUT_MESSAGEBOX
#include <CYAN/cyan.h> // For rspMsgBox() used by rspTrace().
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Output a formatted debug string to the debug terminal/window.
//
///////////////////////////////////////////////////////////////////////////////
void rspTrace(const char *frmt, ... )
	{
	static int16_t	sSem	= 0;

	// If something called by TRACE calls TRACE, we'd be likely to continue
	// forever until stack overflow occurred.  So don't allow re-entrance.
	if (++sSem == 1)
		{
      va_list varp;
		va_start(varp, frmt);    
		vfprintf(stderr, frmt, varp);
		va_end(varp);

#if defined(RSP_DEBUG_OUT_MESSAGEBOX)
		if (rspMsgBox(
			RSP_MB_ICN_INFO | RSP_MB_BUT_YESNO,
			"rspTrace",
			"\"%s\"\n"
			"Continue?",
			szOutput) == RSP_MB_RET_NO)
			{
			DebugBreak();
			exit(EXIT_SUCCESS);
			}
#endif	// RSP_DEBUG_OUT_MESSAGEBOX
		}

	// Remember to reduce.
	sSem--;
	}

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////
