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
// Handles all Dreamcast specific keyboard stuff.
//
//////////////////////////////////////////////////////////////////////////////

// RSPix /////////////////////////////////////////////////////////////////////
#include <BLUE/Blue.h>
#include <BLUE/BlueKeys.h>

// Platform //////////////////////////////////////////////////////////////////
#include "platform.h"
#include "keyboard.h"

// C++ ///////////////////////////////////////////////////////////////////////
#include <map>
#include <queue>

// Only set value if not nullptr.
#define SET(ptr, val)        if((ptr) != nullptr) { *(ptr) = (val); }
#define INC_N_WRAP(i, max)    (i = (i + 1) % max)

namespace keyboard
{
  namespace scancodes
  {
    static const uint8_t set2[0x80] =
    {
      0,

      RSP_SK_ESCAPE,
      RSP_SK_1,
      RSP_SK_2,
      RSP_SK_3,
      RSP_SK_4,
      RSP_SK_5,
      RSP_SK_6,
      RSP_SK_7,
      RSP_SK_8,
      RSP_SK_9,
      RSP_SK_0,
      RSP_SK_MINUS,
      RSP_SK_EQUALS,
      RSP_SK_BACKSPACE,

      RSP_SK_TAB,
      RSP_SK_Q,
      RSP_SK_W,
      RSP_SK_E,
      RSP_SK_R,
      RSP_SK_T,
      RSP_SK_Y,
      RSP_SK_U,
      RSP_SK_I,
      RSP_SK_O,
      RSP_SK_P,
      RSP_SK_LBRACKET,
      RSP_SK_RBRACKET,
      RSP_SK_ENTER,

      RSP_SK_LCONTROL,
      RSP_SK_A,
      RSP_SK_S,
      RSP_SK_D,
      RSP_SK_F,
      RSP_SK_G,
      RSP_SK_H,
      RSP_SK_J,
      RSP_SK_K,
      RSP_SK_L,
      RSP_SK_SEMICOLON,
      RSP_SK_RQUOTE,
      RSP_SK_LQUOTE,

      RSP_SK_LSHIFT,
      RSP_SK_Z,
      RSP_SK_X,
      RSP_SK_C,
      RSP_SK_V,
      RSP_SK_B,
      RSP_SK_N,
      RSP_SK_M,
      RSP_SK_CONTROL,
      RSP_SK_PERIOD,
      RSP_SK_SLASH,
      RSP_SK_RSHIFT,

      RSP_SK_NUMPAD_ASTERISK,

      RSP_SK_LALT,
      RSP_SK_SPACE,
      RSP_SK_CAPSLOCK,
      RSP_SK_F1,
      RSP_SK_F2,
      RSP_SK_F3,
      RSP_SK_F4,
      RSP_SK_F5,
      RSP_SK_F6,
      RSP_SK_F7,
      RSP_SK_F8,
      RSP_SK_F9,
      RSP_SK_F10,

      RSP_SK_NUMLOCK,
      RSP_SK_SCROLL,

      RSP_SK_NUMPAD_7,
      RSP_SK_NUMPAD_8,
      RSP_SK_NUMPAD_9,
      RSP_SK_NUMPAD_MINUS,

      RSP_SK_NUMPAD_4,
      RSP_SK_NUMPAD_5,
      RSP_SK_NUMPAD_6,
      RSP_SK_NUMPAD_PLUS,

      RSP_SK_NUMPAD_1,
      RSP_SK_NUMPAD_2,
      RSP_SK_NUMPAD_3,
      RSP_SK_NUMPAD_0,
      RSP_SK_NUMPAD_DECIMAL,

      0,0,0,

      RSP_SK_F11,
      RSP_SK_F12,

      0,0,0,0
    };
  }

  enum keystate : uint8_t
  {
    released = 0,
    pressed  = 1,
  };

  static keystate keystates[256];

  struct key_event_t
  {
    milliseconds_t lTime;
    uint32_t lKey;
    key_event_t(void) : lTime(0), lKey(0) { }
    key_event_t(milliseconds_t t, uint32_t k)
      : lTime(t), lKey(k) { }
  };

  // Queue of keyboard events.
  static std::queue<key_event_t> keyEvents;


  //  Hardware level keyboard interrupt (int 9) handler.
  static void interruptCallback(void)
  {
    static uint16_t data;
    data |= dos::inportb(port); // read next key

    if(data == 0x00E0)
      data = 0x0100;
    else
    {
      if(data < 0x0080)
      {
        if(keystates[scancodes::set2[data]] != pressed)
        {
          keystates[scancodes::set2[data]] = pressed;
          keyEvents.emplace(rspGetMilliseconds(), scancodes::set2[data]);
        }
      }
      else if(data < 0x0100)
        keystates[scancodes::set2[data & 0x007F]] = released;
      else
      {
        switch(data & 0x00FF)
        {
          case 0x1C: keyEvents.emplace(rspGetMilliseconds(), RSP_SK_NUMPAD_ENTER); break;
          case 0x47: keyEvents.emplace(rspGetMilliseconds(), RSP_GK_HOME); break;
          case 0x48: keyEvents.emplace(rspGetMilliseconds(), RSP_GK_UP); break;
          case 0x49: keyEvents.emplace(rspGetMilliseconds(), RSP_GK_PAGEUP); break;
          case 0x4B: keyEvents.emplace(rspGetMilliseconds(), RSP_GK_LEFT); break;
          case 0x4D: keyEvents.emplace(rspGetMilliseconds(), RSP_GK_RIGHT); break;
          case 0x4F: keyEvents.emplace(rspGetMilliseconds(), RSP_GK_END); break;
          case 0x50: keyEvents.emplace(rspGetMilliseconds(), RSP_GK_DOWN); break;
          case 0x51: keyEvents.emplace(rspGetMilliseconds(), RSP_GK_PAGEDOWN); break;
          case 0x52: keyEvents.emplace(rspGetMilliseconds(), RSP_GK_INSERT); break;
          case 0x53: keyEvents.emplace(rspGetMilliseconds(), RSP_GK_DELETE); break;

        }
      }
      data = 0;
    }

    dos::outportb(pic::master, pic::end_of_interrupt); // clear interrupt
  }
}


//////////////////////////////////////////////////////////////////////////////
// Extern functions.
//////////////////////////////////////////////////////////////////////////////

extern void rspSetQuitStatus(int16_t sQuitStatus);

extern void pollKeyboard(void)
{
#if 0
  if(keyboard::haveKey())
  {

  }
  ASSERT((event->type == SDL_KEYUP) || (event->type == SDL_KEYDOWN));
  //ASSERT(event->key.keysym.sym < SDLK_LAST);

  const uint8_t pushed = (event->type == SDL_KEYDOWN);
#ifdef SDL2_JUNK
  if ((pushed) && (event->key.repeat) && (!keyboard::keyRepeat))
    return;  // drop it.
#endif

  uint8_t key = keyboard::sdl_to_rws_keymap[event->key.keysym.sym];
  uint16_t gkey = keyboard::sdl_to_rws_gkeymap[event->key.keysym.sym];
  uint8_t* pu8KeyStatus = (&keyboard::ms_au8KeyStatus[key]);

  if (key == 0)
    return;

  if (pushed)
  {
    if (event->key.keysym.sym == SDLK_g && event->key.keysym.mod & KMOD_CTRL) // ctrl-g
    {
#ifdef SDL2_JUNK
      const SDL_bool mode = SDL_GetWindowGrab(sdlWindow) ? SDL_FALSE : SDL_TRUE;
      //SDL_SetRelativeMouseMode(mode);
      SDL_SetWindowGrab(sdlWindow, mode);
      mouse_grabbed = (mode == SDL_TRUE);
#endif
      return;  // don't pass this key event on to the game.
    }

    if (event->key.keysym.sym == SDLK_RETURN && event->key.keysym.mod & KMOD_ALT) // alt-enter
    {
#ifdef SDL2_JUNK
      if (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_FULLSCREEN)
        SDL_SetWindowFullscreen(sdlWindow, 0);
      else
        SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN);
#endif
      return;  // don't pass this key event on to the game.
    }

    if (keyEvents.IsFull() == FALSE)
    {
      // Create event.
      static int16_t sEventIndex = 0;
      RSP_SK_EVENT* pkeEvent = ms_akeEvents + INC_N_WRAP(sEventIndex, MAX_EVENTS);
      // Fill event.
      pkeEvent->lTime    = SDL_GetTicks();
      pkeEvent->lKey = ((gkey) ? gkey : key);

      // Enqueue event . . .
      if (keyEvents.EnQ(pkeEvent) == 0)
      {
        // Success.
      }
      else
      {
        TRACE("Key_Message(): Unable to enqueue key event.");
      }
    }

    if (key < sizeof(keyboard::ms_au8KeyStatus))
    {
      if(*pu8KeyStatus & 1) // If key is even . . .
        *pu8KeyStatus += 2; // Go to next odd state.
      else
        ++(*pu8KeyStatus); // Go to next odd state.
    }
  }
  else if(key < sizeof(keyboard::ms_au8KeyStatus) && *pu8KeyStatus & 1) // If key is odd . . .
    ++(*pu8KeyStatus);

  if (key < sizeof(keyboard::keystates))
    keyboard::keystates[key] = pushed;
#endif
}

extern void rspClearKeyEvents(void)
{
//  while(keyboard::haveKey())
//    keyboard::readKey();
}

//////////////////////////////////////////////////////////////////////////////
//
// Read state of entire keyboard
// Returns nothing.
//
//////////////////////////////////////////////////////////////////////////////
extern void rspScanKeys(
    uint8_t* pucKeys)    // Array of 128 unsigned chars is returned here.
{
  std::memcpy(pucKeys, keyboard::keystates, sizeof(keyboard::keystates));
}

//////////////////////////////////////////////////////////////////////////////
//
// Read next key from keyboard queue.
//
//////////////////////////////////////////////////////////////////////////////
extern int16_t rspGetKey(             // Returns TRUE if a key was available; FALSE if not.
    int32_t* plKey,                   // Out: Key info returned here (or 0 if no key available)
    int32_t* plTime)                  // Out: Key's timestamp (unless nullptr)
{
  int16_t sReturn = keyboard::keyEvents.empty() ? FALSE : TRUE;    // Assume no key.

  if(sReturn == TRUE)
  {
    SET(plKey,  keyboard::keyEvents.front().lKey );
    SET(plTime, keyboard::keyEvents.front().lTime);
    keyboard::keyEvents.pop();
  }
  return sReturn;
}

#ifdef UNUSED_FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
//
// Check if a key is available in the keyboard queue via rspGetKey.
//
//////////////////////////////////////////////////////////////////////////////
extern int16_t rspIsKey(void)        // Returns TRUE if a key is available; FALSE if not.
{
  return (keyEvents.IsEmpty() == FALSE) ? TRUE : FALSE;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Initializes arrays for this module.  See the overview for details involving
// the initializations here.
//
// Returns nothing.
//
//////////////////////////////////////////////////////////////////////////////
extern void Key_Init(void)
{
  std::memset(keyboard::keystates, keyboard::released, sizeof(keyboard::keystates));
  keyboard::write(keyboard::scancodeset, keyboard::set2);
  dos::registerintr(keyboard::interrupt, keyboard::interruptCallback);
  rspClearKeyEvents();
}


//////////////////////////////////////////////////////////////////////////////
// This function returns a pointer to an array of 128 bytes.  Each byte indexed
// by an RSP_SK_* macro indicates the status of that key.  If any element in
// the array is 0 when the corresponding key is pressed, that key is set to 1.
// When that key is released, it is incremented to 2.  When it is pressed again,
// it is incremented to 3, etc., etc..  This array is only cleared by the caller
// for maximum flexibility.  Note that, if the array is cleared, and a key is
// released the entry put into the array will be 2 (not 1) so that the caller
// can rely upon the meaning of evens vs. odds (key currently down vs. up).
// Also, note that this array is static and, therefore, this function need NOT
// be called for every use of the array.  As a matter of fact, you may only need
// to call this function once for an entire program's execution of scans and 
// clears of the array.
//////////////////////////////////////////////////////////////////////////////
uint8_t* rspGetKeyStatusArray(void)    // Returns a ptr to the key status array.
{
  return reinterpret_cast<uint8_t*>(keyboard::keystates);
}

//////////////////////////////////////////////////////////////////////////////
// Set keys that must be pressed in combination with the system 'quit' key.
//////////////////////////////////////////////////////////////////////////////
extern void rspSetQuitStatusFlags(    // Returns nothing.
    int32_t lKeyFlags)                // In:  New keyflags (RSP_GKF_*).  0 to clear.
{
  UNUSED(lKeyFlags);
  fprintf(stderr, "STUBBED: %s:%d\n", __FILE__, __LINE__);
}

//////////////////////////////////////////////////////////////////////////////
// This function returns the state of the three toggle keys (Caps Lock,
// Num Lock, and Scroll Lock).  A key that is 'on' has one of the following
// corresponding bits set in the returned long.
// Note that the values returned by this function are not guaranteed to be
// in synchronization with any of the other key functions.  Their state is
// obtained as close to the current key status as is possible dependent
// upon the platform.
//////////////////////////////////////////////////////////////////////////////
#define RSP_CAPS_LOCK_ON        0x00000001
#define RSP_NUM_LOCK_ON            0x00000002
#define RSP_SCROLL_LOCK_ON        0x00000004

extern int32_t rspGetToggleKeyStates(void)    // Returns toggle key state flags.
{
  int32_t lkeystates = 0;
#if 0  // !!! FIXME
  uint8_t *states = SDL_GetKeyState(nullptr);
  if (states[SDLK_CAPSLOCK]) lkeystates |= RSP_CAPS_LOCK_ON;
  if (states[SDLK_NUMLOCKCLEAR]) lkeystates |= RSP_NUM_LOCK_ON;
  if (states[SDLK_SCROLLLOCK]) lkeystates |= RSP_SCROLL_LOCK_ON;
#endif
  return lkeystates;
}


extern void rspKeyRepeat(bool bEnable)
{
  if(bEnable)
    keyboard::write(keyboard::load_defaults);
  else if(keyboard::write(keyboard::set_rate_and_delay, 0x7F)) // switch to a 1000ms delay with 2Hz repeat rate
    keyboard::write(keyboard::enable_scanning);
}


//////////////////////////////////////////////////////////////////////////////
// EOF
//////////////////////////////////////////////////////////////////////////////
