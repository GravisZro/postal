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
// Handles all Dreamcast specific mouse stuff.
//
//////////////////////////////////////////////////////////////////////////////

// RSPix /////////////////////////////////////////////////////////////////////
#include <BLUE/Blue.h>
#include <ORANGE/CDT/Queue.h>

// Platform //////////////////////////////////////////////////////////////////



struct mouse_event_t
{
  int16_t sX;
  int16_t sY;
  int16_t sButton;
  milliseconds_t lTime;
  int16_t sType;
};

#define MAX_EVENTS    256
// Only set value if not nullptr.
#define SET(ptr, val)        if((ptr) != nullptr) { *(ptr) = (val); }

static mouse_event_t    ms_ameEvents[MAX_EVENTS];

static RQueue<mouse_event_t, MAX_EVENTS>    ms_qmeEvents;

extern bool mouse_enabled;
extern bool mouse_grabbed;

///////////////////////////////////////////////////////////////////////////////
// Module specific (static) globals.
///////////////////////////////////////////////////////////////////////////////
static int16_t ms_sCursorShowLevel = 0;

///////////////////////////////////////////////////////////////////////////////
// Functions.
///////////////////////////////////////////////////////////////////////////////
static int MouseWheelState = 0;


extern void rspActivateMouse(bool enable)
{
  mouse_enabled = enable;
}


extern bool rspMouseGrabbed(void)
{
  return mouse_grabbed;
}


///////////////////////////////////////////////////////////////////////////////
//
// Puts the coordinates of the mouse position in your shorts.
// Note GetAsyncKeyState returns current button state info (unlike
// GetKeyState); however, if we do not have keyboard focus, it returns 0, or so
// it is documented.
// Returns nothing.
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspGetMouse(
    int16_t* psX,                       // Current x position is returned here (unless nullptr)
    int16_t* psY,                       // Current y position is returned here (unless nullptr)
    int16_t* psButton)                  // Current button status is returned here (unless nullptr)
{

  if (!mouse_enabled)
  {
    SET(psX, 0);
    SET(psY, 0);
    SET(psButton, 0);
    return;  // drop mouse events if input isn't grabbed.
  }
/*
  int x, y;
  const uint32_t buttons = SDL_GetMouseState(&x, &y);
  SET(psX, x);
  SET(psY, y);
  // TRACE("x = %d, y = %d");

  if (psButton != nullptr)
  {
    *psButton = (buttons & SDL_BUTTON_LMASK) ? 0x0001 : 0;
    *psButton |= (buttons & SDL_BUTTON_RMASK) ? 0x0002 : 0;
    *psButton |= (buttons & SDL_BUTTON_MMASK) ? 0x0004 : 0;
    *psButton |= (buttons & SDL_BUTTON_X1MASK) ? 0x0008 : 0;
    *psButton |= (buttons & SDL_BUTTON_X2MASK) ? 0x0010 : 0;
    *psButton |= (MouseWheelState & 0x0020) ? 0x0020 : 0;
    *psButton |= (MouseWheelState & 0x0040) ? 0x0040 : 0;
  }
*/
  MouseWheelState = 0;
}

/*
extern void Mouse_Event(SDL_Event *event)
{
  if (!mouse_grabbed)
    return;  // drop mouse events if input isn't grabbed.

  // Get next event.  We do not "new" a mouse_event_t here to avoid
  // memory fragmentation.
  mouse_event_t* pme = ms_ameEvents;
  pme->lTime = SDL_GetTicks();
  pme->sType = event->type;

  static int16_t buttonState = 0;

  bool bQueueMouseWheelRelease = false;

  buttonState &= ~(0x0020 | 0x0040);

  switch (event->type)
  {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
      pme->sX = event->button.x;
      pme->sY = event->button.y;
      int val;
      switch (event->button.button)
      {
        case SDL_BUTTON_LEFT: val = 0x0001; break;
        case SDL_BUTTON_RIGHT: val = 0x0002; break;
        case SDL_BUTTON_MIDDLE: val = 0x0004; break;
        case SDL_BUTTON_X1MASK: val = 0x0008; break;
        case SDL_BUTTON_X2MASK: val = 0x0010; break;
        default: val = 0; break;
      }

      if (event->button.state == SDL_PRESSED)
        buttonState |= val;
      else
        buttonState &= ~val;

      pme->sButton = buttonState;
      break;
    }
    default:  // uh?
      ASSERT(0 && "unexpected mouse event!");
      return;
  }

  if (ms_qmeEvents.IsFull() != FALSE)
  {
    // Discard oldest event.
    ms_qmeEvents.DeQ();
  }

  // Enqueue event . . .
  if (ms_qmeEvents.EnQ(pme) == SUCCESS)
  {
    // Success.
  }
  else
  {
    TRACE("Mouse_Message(): Unable to enqueue mouse event.");
  }

  // Add "dummy" mouse wheel button release event.
  if (bQueueMouseWheelRelease)
  {
    mouse_event_t* newpme = ms_ameEvents;
    newpme->lTime = SDL_GetTicks();
    newpme->sType = SDL_MOUSEBUTTONUP;
    newpme->sButton = MouseWheelState;
    ms_qmeEvents.EnQ(newpme);
  }
}
*/
///////////////////////////////////////////////////////////////////////////////
//
// Sets the mouse position to your shorts.
// Returns nothing.
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspSetMouse(
    int16_t sX,                         // New x position.
    int16_t sY)                         // New y position.
{
  if (!mouse_enabled)
    return;  // drop mouse events if input isn't grabbed.
}

//////////////////////////////////////////////////////////////////////////////
//
// Get most recent (last) mouse event from queue (using short coords).
// This function tosses out any events ahead of the last event in the queue!
// 
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspGetLastMouseEvent(    // Returns 0 if no event was available, non-zero otherwise
    int16_t* psX,                       // Event's X position is returned here (unless nullptr)
    int16_t* psY,                       // Event's Y position is returned here (unless nullptr)
    int16_t* psButton,                  // Event's button status is returned here (unless nullptr)
    int32_t* plTime,                    // Event's time stamp returned here (unless nullptr)
    int16_t* psType)                    // Event's type (as per OS) is returned here (unless nullptr)
{
  int16_t sResult    = TRUE;    // Assume success.

  mouse_event_t* peEvent;
  int16_t sNumEvents = ms_qmeEvents.NumItems();

  // Are there any events?
  if (sNumEvents > 0)
  {
    while (sNumEvents-- > 0)
    {
      peEvent    = ms_qmeEvents.DeQ();
    }

    if (peEvent != nullptr)
    {
      SET(psX,            peEvent->sX);
      SET(psY,            peEvent->sY);
      SET(psButton,    peEvent->sButton);
      SET(plTime,        peEvent->lTime);
      SET(psType,        peEvent->sType);
    }
    else
    {
      TRACE("rspGetLastMouseEvent(): Unable to dequeue last event.");
      sResult = FALSE;
    }
  }
  else
  {
    sResult    = FALSE;
  }

  return sResult;
}

//////////////////////////////////////////////////////////////////////////////
//
// Get next mouse button event from queue (using short coords).
// Returns 0 on success.
// 
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspGetMouseEvent(        // Returns 0 if no event was available, non-zero otherwise
    int16_t* psX,                       // Event's X position is returned here (unless nullptr)
    int16_t* psY,                       // Event's Y position is returned here (unless nullptr)
    int16_t* psButton,                  // Event's button status is returned here (unless nullptr)
    int32_t* plTime,                    // Event's time stamp returned here (unless nullptr)
    int16_t* psType)                    // Event's type (as per OS) is returned here (unless nullptr)
{
  mouse_event_t* peEvent = ms_qmeEvents.DeQ();
  if (peEvent == nullptr)
    return FALSE;

  SET(psX,      peEvent->sX);
  SET(psY,      peEvent->sY);
  SET(psButton, peEvent->sButton);
  SET(plTime,   peEvent->lTime);
  SET(psType,   peEvent->sType);

  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// Clear mouse event queue
// Returns nothing.
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspClearMouseEvents(void)
{
  while (ms_qmeEvents.DeQ() != nullptr);
}

//////////////////////////////////////////////////////////////////////////////
//
// Hide OS mouse cursor
// Returns nothing.
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspHideMouseCursor(void)
{
}

//////////////////////////////////////////////////////////////////////////////
//
// Show OS mouse cursor
// Returns nothing.
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspShowMouseCursor(void)
{
}

///////////////////////////////////////////////////////////////////////////////
// 
// Shield the OS mouse cursor from screen updates.
// This function hides the cursor in the fastest way possible for the current
// type of screen updatage.  Calls to rspShieldMouseCursor() and 
// rspUnshieldMouseCursor() encapsulating screen updates is the preferrable 
// way to shield the cursor from screen updates.
// Note that to get data from the screen you should first hide the cursor
// with rspHideMouseCursor() as this function may not actually 'erase' the
// cursor.
// This is NOT synonymous to rspHideMouseCursor().
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspShieldMouseCursor(void)
{
}

///////////////////////////////////////////////////////////////////////////////
// 
// Unshield the OS mouse cursor from screen updates (i.e., show the cursor
// after a rspShieldMouseCursor() call to protect the cursor from a direct
// screen write).
// This is NOT synonymous to rspShowMouseCursor().
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspUnshieldMouseCursor(void)
{
}

///////////////////////////////////////////////////////////////////////////////
// 
// Reports current mouse cursor show level.
// 
///////////////////////////////////////////////////////////////////////////////
int16_t rspGetMouseCursorShowLevel(void)// Returns current mouse cursor show level:
// Positive indicates cursor is shown.
// Non-positive indicates cursor is hidden.
{
}

///////////////////////////////////////////////////////////////////////////////
// 
// Sets current mouse cursor show level.
// 
///////////////////////////////////////////////////////////////////////////////
void rspSetMouseCursorShowLevel(        // Returns nothing.
    int16_t sNewShowLevel)              // In:  Current mouse cursor show level:
                                        // Positive indicates cursor is shown.
                                        // Non-positive indicates cursor is hidden.
{
}

//////////////////////////////////////////////////////////////////////////////
// EOF
//////////////////////////////////////////////////////////////////////////////
