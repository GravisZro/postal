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
////////////////////////////////////////////////////////////////////////////////
//
// Does all Dreamcast specific time stuff.
//
//////////////////////////////////////////////////////////////////////////////

// RSPix /////////////////////////////////////////////////////////////////////
#include <BLUE/Blue.h>

// Platform //////////////////////////////////////////////////////////////////
#include <time.h>
#include <sys/time.h>

static microseconds_t MicrosecondsBase = 0;

//////////////////////////////////////////////////////////////////////////////
// The following is taken from SDL 1.2.15
//////////////////////////////////////////////////////////////////////////////

/* The first ticks value of the application */
#ifdef HAVE_CLOCK_GETTIME
static struct timespec start;
#else
static struct timeval start;
#endif /* HAVE_CLOCK_GETTIME */


void SDL_StartTicks(void)
{
   /* Set first ticks value */
#if HAVE_CLOCK_GETTIME
   clock_gettime(CLOCK_MONOTONIC,&start);
#else
   gettimeofday(&start, NULL);
#endif
}


uint32_t SDL_GetTicks (void)
{
#if HAVE_CLOCK_GETTIME
   uint32_t ticks;
   struct timespec now;
   clock_gettime(CLOCK_MONOTONIC,&now);
   ticks=(now.tv_sec-start.tv_sec)*1000+(now.tv_nsec-start.tv_nsec)/1000000;
   return(ticks);
#else
   uint32_t ticks;
   struct timeval now;
   gettimeofday(&now, NULL);
   ticks=(now.tv_sec-start.tv_sec)*1000+(now.tv_usec-start.tv_usec)/1000;
   return(ticks);
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// Initializes the time module.
//
//////////////////////////////////////////////////////////////////////////////
//static itimerval g_timer;

extern void Time_Init(void)
{
  SDL_StartTicks();
  MicrosecondsBase = rspGetAppMicroseconds();
}


//////////////////////////////////////////////////////////////////////////////
//
// Get the current Windows' time.
// Returns the time in a long.
//
//////////////////////////////////////////////////////////////////////////////
extern milliseconds_t rspGetMilliseconds(void)
{
  return SDL_GetTicks();
}

//////////////////////////////////////////////////////////////////////////////
//
// Get time since last rspGetMicroseconds(TRUE) call in microseconds.  May 
// not always be accurate to the nearest microsecond.  It is always on the
// Mac but possibly not in Windows; however, every machine tested produced
// good to excellent resolution.
// Returns the time in a long.
//
//////////////////////////////////////////////////////////////////////////////
extern microseconds_t rspGetMicroseconds(	// Returns microseconds between now and last
    int16_t sReset /*= FALSE*/)		// Set to TRUE to reset timer.  If you never reset the timer, it will wrap within just over 35 minutes.
{
  microseconds_t microsecs = rspGetAppMicroseconds();
  microseconds_t lTime = microsecs - MicrosecondsBase; // sorry, no time travel allowed

  // If reset requested . . .
  if (sReset != FALSE)
    MicrosecondsBase = microsecs;

  return lTime;
}

//////////////////////////////////////////////////////////////////////////////
//
// Get time since App started in microseconds.  This is safe for global use
// because it cannot be reset by anyone.  It requires 64-bit mathm however!
//
// May not always be accurate to the nearest microsecond.  It is always on the
// Mac but possibly not in Windows; however, every machine tested produced
// good to excellent resolution.
//
// Returns the time in an __int64.
//
//////////////////////////////////////////////////////////////////////////////
extern microseconds_t rspGetAppMicroseconds()
{
  return rspGetMilliseconds() * 1000;
}

//////////////////////////////////////////////////////////////////////////////
// EOF
//////////////////////////////////////////////////////////////////////////////
