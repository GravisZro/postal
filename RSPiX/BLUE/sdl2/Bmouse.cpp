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
//	bmouse.cpp
// 
// History:
//		06/03/04 RCG  Started.
//
//////////////////////////////////////////////////////////////////////////////
//
// Handles all SDL2 specific mouse stuff.
//
//////////////////////////////////////////////////////////////////////////////

// RSPix /////////////////////////////////////////////////////////////////////
#include <BLUE/Blue.h>
#include <ORANGE/CDT/Queue.h>

// Platform //////////////////////////////////////////////////////////////////
#include <SDL2/SDL.h>


extern SDL_Window *sdlWindow;
extern SDL_Surface *sdlShadowSurface;
extern int sdlWindowWidth;
extern int sdlWindowHeight;

struct mouse_event_t
{
  int16_t	sX;
  int16_t	sY;
  int16_t	sButton;
  int32_t	lTime;
  int16_t	sType;
};

#define MAX_EVENTS	256
// Only set value if not nullptr.
#define SET(ptr, val)        if((ptr) != nullptr) { *(ptr) = (val); }

static mouse_event_t	ms_ameEvents[MAX_EVENTS];

static RQueue<mouse_event_t, MAX_EVENTS>	ms_qmeEvents;

extern bool mouse_enabled;
extern bool mouse_grabbed;

///////////////////////////////////////////////////////////////////////////////
// Module specific (static) globals.
///////////////////////////////////////////////////////////////////////////////
static int16_t				ms_sCursorShowLevel	= 0;

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
      int16_t* psX,				// Current x position is returned here (unless nullptr)
      int16_t* psY,				// Current y position is returned here (unless nullptr)
      int16_t* psButton)		// Current button status is returned here (unless nullptr)
{

  if (!mouse_enabled)
  {
    SET(psX, 0);
    SET(psY, 0);
    SET(psButton, 0);
    return;  // drop mouse events if input isn't enabled.
  }

  int x, y;
  const uint32_t buttons = SDL_GetMouseState(&x, &y);
  SET(psX, x);
  SET(psY, y);

  if (psButton != nullptr)
  {
    *psButton  = RSP_BUTTON_MASK(buttons);
    *psButton |= RSP_BUTTON_MASK(MouseWheelState & RSP_MWHEEL_UP);
    *psButton |= RSP_BUTTON_MASK(MouseWheelState & RSP_MWHEEL_DOWN);
  }

  MouseWheelState = 0;
}


extern void Mouse_Event(SDL_Event *event)
{
  if (!mouse_enabled)
    return;  // drop mouse events if input isn't enabled.

  // Get next event.  We do not "new" a mouse_event_t here to avoid
  // memory fragmentation.
  mouse_event_t*	pme = ms_ameEvents;
  pme->lTime = SDL_GetTicks();
#if SDL_MAJOR_VERSION > 2 || (SDL_MAJOR_VERSION == 2 && (SDL_MINOR_VERSION > 0 || SDL_PATCHLEVEL >= 2))
  pme->sType = event->type == SDL_MOUSEBUTTONDOWN ? event->button.clicks : RSP_BUTTON_RELEASED;  // click count (if clicking)
#else
  pme->sType = event->type == SDL_MOUSEBUTTONDOWN ? RSP_BUTTON_PRESSED : RSP_BUTTON_RELEASED; // no double click for you! (need to fix!)
#endif

  static int16_t buttonState = 0;

  bool bQueueMouseWheelRelease = false;

  buttonState &= ~(RSP_MWHEEL_UP | RSP_MWHEEL_DOWN); // unset mousewheel

  switch (event->type)
  {
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
    {
      pme->sType |= RSP_BUTTON_MASK(event->button.button);
      pme->sX = event->button.x;
      pme->sY = event->button.y;

      if (event->button.state == SDL_PRESSED)
        buttonState |= RSP_BUTTON_MASK(event->button.button); // set
      else
        buttonState &= ~RSP_BUTTON_MASK(event->button.button); // unset

      pme->sButton = buttonState;
      break;
    }
    case SDL_MOUSEWHEEL:
    {
      if (event->wheel.y > 0)
        MouseWheelState = RSP_MWHEEL_UP;
      else if (event->wheel.y < 0)
        MouseWheelState = RSP_MWHEEL_DOWN;
      else
        MouseWheelState = 0;

      pme->sType = MouseWheelState | RSP_BUTTON_PRESSED;
      buttonState |= MouseWheelState;
      pme->sButton = buttonState;
      bQueueMouseWheelRelease = true;
      break;
    }

    default:
      ASSERT(false);
      break;
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
    TRACE("Mouse_Message(): Unable to enqueue mouse event.\n");
  }

  // Add "dummy" mouse wheel button release event.
  if (bQueueMouseWheelRelease)
  {
    mouse_event_t* newpme = ms_ameEvents;
    newpme->lTime = SDL_GetTicks();
    newpme->sType = MouseWheelState;
    newpme->sButton = MouseWheelState;
    ms_qmeEvents.EnQ(newpme);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Sets the mouse position to your shorts.
// Returns nothing.
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspSetMouse(
		int16_t sX,				// New x position.
		int16_t sY)				// New y position.
	{
        if (!mouse_enabled)
            return;  // drop mouse events if input isn't enabled.
        SDL_WarpMouseInWindow(sdlWindow, sX, sY);
	}

//////////////////////////////////////////////////////////////////////////////
//
// Get most recent (last) mouse event from queue (using short coords).
// This function tosses out any events ahead of the last event in the queue!
// 
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspGetLastMouseEvent(	// Returns 0 if no event was available, non-zero otherwise
   int16_t*	psX,						// Event's X position is returned here (unless nullptr)
   int16_t*	psY,						// Event's Y position is returned here (unless nullptr)
   int16_t*	psButton,				// Event's button status is returned here (unless nullptr)
   int32_t*		plTime,					// Event's time stamp returned here (unless nullptr)
   int16_t*	psType /*= nullptr*/)	// Event's type (as per OS) is returned here (unless nullptr)
	{
	int16_t sResult	= TRUE;	// Assume success.

   mouse_event_t*	peEvent;
   int16_t sNumEvents = ms_qmeEvents.NumItems();

	// Are there any events?
	if (sNumEvents > 0)
		{
		while (sNumEvents-- > 0)
			{
			peEvent	= ms_qmeEvents.DeQ();
			}

      if (peEvent != nullptr)
			{
			SET(psX,			peEvent->sX);
			SET(psY,			peEvent->sY);
			SET(psButton,	peEvent->sButton);
			SET(plTime,		peEvent->lTime);
			SET(psType,		peEvent->sType);
			}
		else
			{
			TRACE("rspGetLastMouseEvent(): Unable to dequeue last event.\n");
			sResult = FALSE;
			}
		}
	else
		{
		sResult	= FALSE;
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
	// Decrement show cursor count.
    if (--ms_sCursorShowLevel < 0)
       SDL_ShowCursor(SDL_FALSE);
	}

//////////////////////////////////////////////////////////////////////////////
//
// Show OS mouse cursor
// Returns nothing.
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspShowMouseCursor(void)
	{
	// Increment show cursor count.
	if (++ms_sCursorShowLevel >= 0)
       SDL_ShowCursor(SDL_TRUE);
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
int16_t rspGetMouseCursorShowLevel(void)	// Returns current mouse cursor show level:
													// Positive indicates cursor is shown.
													// Non-positive indicates cursor is hidden.
	{
	return ms_sCursorShowLevel;
	}

///////////////////////////////////////////////////////////////////////////////
// 
// Sets current mouse cursor show level.
// 
///////////////////////////////////////////////////////////////////////////////
void rspSetMouseCursorShowLevel(	// Returns nothing.
	int16_t sNewShowLevel)				// In:  Current mouse cursor show level:        
											// Positive indicates cursor is shown.     
											// Non-positive indicates cursor is hidden.
	{
	while (ms_sCursorShowLevel < sNewShowLevel)
		{
		rspShowMouseCursor();
		}

	while (ms_sCursorShowLevel > sNewShowLevel)
		{
		rspHideMouseCursor();
		}
	}

//////////////////////////////////////////////////////////////////////////////
// EOF
//////////////////////////////////////////////////////////////////////////////
