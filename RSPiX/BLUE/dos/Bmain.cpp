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

// RSPix /////////////////////////////////////////////////////////////////////
#include <BLUE/Blue.h>

// Platform //////////////////////////////////////////////////////////////////


// C++ ///////////////////////////////////////////////////////////////////////
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

bool RspBlueReady = false;
bool mouse_grabbed = false;

//////////////////////////////////////////////////////////////////////////////
// 
//    Call this to init the blue library.  Calls init in the various blue modules
// that needing an init call.
//
// Returns 0 on success.
// 
//////////////////////////////////////////////////////////////////////////////

extern void Disp_Init(void);
extern void Key_Init(void);
extern void Time_Init(void);

int16_t rspInitBlue(void)
{
  int16_t sResult = SUCCESS;    // Assume success.

  Disp_Init();
  Key_Init();
  Time_Init();

  RspBlueReady = sResult == SUCCESS;
  return sResult;
}


//////////////////////////////////////////////////////////////////////////////
// 
//    Call this to kill the blue library (IF you called Blu_Init).
//
// Returns nothing.
// 
//////////////////////////////////////////////////////////////////////////////
void rspKillBlue(void)
{
}

//////////////////////////////////////////////////////////////////////////////
// 
// Does tasks critical to Windows:
//        - Services our Windows message queue.
// Returns nothing.
// 
//////////////////////////////////////////////////////////////////////////////

extern void rspSetQuitStatus(int16_t sQuitStatus);

extern void rspDoSystem(void)           // Returns nothing.
{
  rspPresentFrame();
/*
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
#if 0
      //defined(ALLOW_JOYSTICK)
      case SDL_JOYAXISMOTION:   // Joystick axis motion
      case SDL_JOYHATMOTION:    // Joystick hat position change
      case SDL_JOYBUTTONDOWN:   // Joystick button pressed
      case SDL_JOYBUTTONUP:     // Joystick button released
        Joy_Event(&event);
        break;
#endif

//    case SDL_MOUSEMOTION:
//    case SDL_JOYBALLMOTION:
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        Mouse_Event(&event);
        break;

      case SDL_KEYDOWN:
      case SDL_KEYUP:
        Key_Event(&event);
        break;

//    case SDL_ACTIVEEVENT:
//      if (event.active.state & SDL_APPINPUTFOCUS)
//        GSDLAppIsActive = (event.active.gain != 0);
//      break;

    }
  }
*/
}

//////////////////////////////////////////////////////////////////////////////
// 
// Sets the current mode with which the rspDoSystem cooperates with the OS.
// 
//////////////////////////////////////////////////////////////////////////////
extern void rspSetDoSystemMode(         // Returns nothing.
    int16_t sCooperativeLevel)          // In:  One of the RSP_DOSYSTEM_* macros defining what level of cooperation to use.
{
  UNUSED(sCooperativeLevel);
  /* no-op */
}

////////////////////////////////////////////////////////////////////////////////
//
// Get system-specific quit status.
//
////////////////////////////////////////////////////////////////////////////////
extern int16_t rspGetQuitStatus(void)   // Returns TRUE if quit detected, FALSE otherwise
{
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
// Set system-specific quit status.
//
// This allows the app to either clear the status to 0 or to set it to whatever
// other value that app might like.
//
////////////////////////////////////////////////////////////////////////////////
extern void rspSetQuitStatus(
    int16_t sStatus)                    // In:  New status
{
}

extern int _argc;
extern char **_argv;
extern int rspCommandLine(const char *cmd)
{
  for (int i = 1; i < _argc; i++)
  {
    const char *arg = _argv[i];
    if (*arg == '-') arg++;
    if (*arg == '-') arg++;

    if (strcasecmp(cmd, arg) == 0)
      return i;
  }

  return 0;
}

extern void rspPlatformInit(void)
{
  TRACE("rspPlatformInit\n");
}


//////////////////////////////////////////////////////////////////////////////
// EOF
//////////////////////////////////////////////////////////////////////////////
