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
#include <CYAN/Cyan.h> // For rspMsgBox() used by rspTrace().
#endif

//////////////////////////////////////////////////////////////////////////////
// Macros.
//////////////////////////////////////////////////////////////////////////////
#if defined(RSP_DEBUG_OUT_FILE) && !defined(RSP_TRACE_LOG_NAME)
#define RSP_TRACE_LOG_NAME	trace_sdl2.txt
#endif	// RSP_DEBUG_OUT_FILE / RSP_TRACE_LOG_NAME

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"DUKE", __VA_ARGS__))
#endif
///////////////////////////////////////////////////////////////////////////////
//
// Output a formatted debug string to the debug terminal/window.
//
///////////////////////////////////////////////////////////////////////////////
void rspTrace(const char *frmt, ... )
{
  static int16_t sSem = 0;
#if defined(RSP_DEBUG_OUT_FILE)
  static FILE* fs = nullptr;
  if(fs == nullptr)
  {
    fs = fopen(QUOTE(RSP_TRACE_LOG_NAME), "wt");
    ASSERT(fs);
    fprintf(fs, "======== Postal build %s %s ========\n", __DATE__, __TIME__);
    time_t sysTime = time(nullptr);
    fprintf(fs, "Debug log file initialized: %s\n", ctime(&sysTime));
    fclose(fs);
  }
#endif // RSP_DEBUG_OUT_FILE

  // If something called by TRACE calls TRACE, we'd be likely to continue
  // forever until stack overflow occurred.  So don't allow re-entrance.
  if (++sSem == 1)
  {
    va_list varp;
    va_start(varp, frmt);
#if defined(RSP_DEBUG_OUT_FILE)
    fs = fopen(QUOTE(RSP_TRACE_LOG_NAME), "a+");
    ASSERT(fs);
    vfprintf(fs, frmt, varp);
    fclose(fs);
#else
    vfprintf(stderr, frmt, varp);
#endif
    va_end(varp);

#if defined(RSP_DEBUG_OUT_MESSAGEBOX)
      if (rspMsgBox(
         RSP_MB_ICN_INFO | RSP_MB_BUT_YESNO,
         "rspTrace",
         "\"%s\"\n"
         "Continue?",
         szOutput) == RSP_MB_RET_NO)
         {
# if defined(_WIN32)
         DebugBreak();
# endif
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
