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
#include <ORANGE/CDT/Queue.h>

// Platform //////////////////////////////////////////////////////////////////
#include "platform.h"

// C++ ///////////////////////////////////////////////////////////////////////
#include <map>


#define MAX_EVENTS    256
// Only set value if not nullptr.
#define SET(ptr, val)        if((ptr) != nullptr) { *(ptr) = (val); }
#define INC_N_WRAP(i, max)    (i = (i + 1) % max)

struct RSP_SK_EVENT
{
  uint32_t lKey;
  milliseconds_t lTime;
};

namespace keyboard
{
  union keycode
  {
    struct
    {
      uint16_t key;
      uint16_t modifiers;
    };
    uint32_t data;

    keycode(uint32_t d) : data(d) { }
  };

  static bool keyRepeat = false;

  static std::map<keycode, uint8_t> bios_to_rws_keymap;
  static std::map<keycode, uint16_t> bios_to_rws_gkeymap;
  static uint8_t keystates[128];
  static uint8_t ms_au8KeyStatus[128];

  // Non-dynamic memory for RSP_SK_EVENTs in queue.
  static RSP_SK_EVENT	ms_akeEvents[MAX_EVENTS];

  // Queue of keyboard events.
  static RQueue<RSP_SK_EVENT, MAX_EVENTS>	ms_qkeEvents;
}
#if 0
static int leds_ok = TRUE;

static int in_a_terrupt = FALSE;

static int extended_key = FALSE;



// Wait for the keyboard controller to set the ready-for-write bit.
static inline int kb_wait_for_write_ready(void)
{
   int timeout = 4096;

   while ((timeout > 0) && (readHW8(0x64) & 2))
      timeout--;

   return (timeout > 0);
}



// Wait for the keyboard controller to set the ready-for-read bit.
static inline int kb_wait_for_read_ready(void)
{
   int timeout = 16384;

   while ((timeout > 0) && (!(readHW8(0x64) & 1)))
      timeout--;

   return (timeout > 0);
}



// Sends a byte to the keyboard controller. Returns 1 if all OK.
static inline int kb_send_data(unsigned char data)
{
   int resends = 4;
   int timeout, temp;

   do {
      if (!kb_wait_for_write_ready())
         return 0;

      writeHW8(0x60, data);
      timeout = 4096;

      while (--timeout > 0) {
         if (!kb_wait_for_read_ready())
            return 0;

         temp = readHW8(0x60);

         if (temp == 0xFA)
            return 1;

         if (temp == 0xFE)
            break;
      }
   } while ((resends-- > 0) && (timeout > 0));

   return 0;
}



// Updates the LED state.
static void pcdos_set_leds(int leds)
{
   if (!leds_ok)
      return;

   if (!in_a_terrupt)
      DISABLE();

   if (!kb_send_data(0xED)) {
      kb_send_data(0xF4);
      leds_ok = FALSE;
   }
   else if (!kb_send_data((leds>>8) & 7)) {
      kb_send_data(0xF4);
      leds_ok = FALSE;
   }

   if (!in_a_terrupt)
      ENABLE();
}



// Sets the key repeat rate.
static void pcdos_set_rate(int delay, int rate)
{
   if (!leds_ok)
      return;

   if (delay < 375)
      delay = 0;
   else if (delay < 625)
      delay = 1;
   else if (delay < 875)
      delay = 2;
   else
      delay = 3;

   rate = MID(0, (rate-33) * 31 / (500-33), 31);

   DISABLE();

   if ((!kb_send_data(0xF3)) || (!kb_send_data((delay << 5) | rate)))
      kb_send_data(0xF4);

   ENABLE();
}



//  Hardware level keyboard interrupt (int 9) handler.
static int keyint(void)
{
   int code = readHW8(0x60);

   in_a_terrupt = TRUE;

   _handle_pckey(code);

   in_a_terrupt = FALSE;

#if defined(__DJGPP__)
      /* three-finger salute for killing the program */
      if (three_finger_flag) {
         if (((code == 0x4F) || (code == 0x53)) &&
             (_key_shifts & KB_CTRL_FLAG) && (_key_shifts & KB_ALT_FLAG)) {
            asm (
               "  movb $0x79, %%al ; "
               "  call ___djgpp_hw_exception "
            : : : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory"
            );
         }

         /* also handle ctrl+break, like the standard djgpp libc */
         if ((code == 0x46) && (extended_key) &&
             (_key_shifts & KB_CTRL_FLAG)) {
            asm (
               "  movb $0x1B, %%al ; "
               "  call ___djgpp_hw_exception "
            : : : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory"
            );
         }
      }
#else
NOTE("No interrupt key sequence defined!")
#endif

   extended_key = (code == 0xE0);

   writeHW8(0x20, 0x20);
   return 0;
}



/* pcdos_key_init:
 *  Installs the keyboard handler.
 */
static int pcdos_key_init(void)
{
   int s1, s2, s3;

   _pckeys_init();

   LOCK_VARIABLE(leds_ok);
   LOCK_VARIABLE(in_a_terrupt);
   LOCK_VARIABLE(extended_key);
   LOCK_FUNCTION(pcdos_set_leds);
   LOCK_FUNCTION(keyint);

   /* read the current BIOS keyboard state */
   while (kbhit())
      simulate_keypress(getch());

   _farsetsel(_dos_ds);

   s1 = farRead8(0x417);
   s2 = farRead8(0x418);
   s3 = farRead8(0x496);

   _key_shifts = 0;

   if (s1 & 1) { _key_shifts |= KB_SHIFT_FLAG; key[KEY_RSHIFT]   = TRUE; }
   if (s1 & 2) { _key_shifts |= KB_SHIFT_FLAG; key[KEY_LSHIFT]   = TRUE; }
   if (s2 & 1) { _key_shifts |= KB_CTRL_FLAG;  key[KEY_LCONTROL] = TRUE; }
   if (s2 & 2) { _key_shifts |= KB_ALT_FLAG;   key[KEY_ALT]      = TRUE; }
   if (s3 & 4) { _key_shifts |= KB_CTRL_FLAG;  key[KEY_RCONTROL] = TRUE; }
   if (s3 & 8) { _key_shifts |= KB_ALT_FLAG;   key[KEY_ALTGR]    = TRUE; }

   if (s1 & 16) _key_shifts |= KB_SCROLOCK_FLAG;
   if (s1 & 32) _key_shifts |= KB_NUMLOCK_FLAG;
   if (s1 & 64) _key_shifts |= KB_CAPSLOCK_FLAG;

   key_shifts = _key_shifts;

   _install_irq(keyboard_interrupt, keyint);

   pcdos_set_leds(_key_shifts);

   return 0;
}



/* pcdos_key_exit:
 *  Removes the keyboard handler.
 */
static void pcdos_key_exit(void)
{
   int s1, s2, s3;

   _remove_irq(keyboard_interrupt);

   /* transfer state info back to the BIOS */
   _farsetsel(_dos_ds);

   s1 = farRead8(0x417) & 0x80;
   s2 = farRead8(0x418) & 0xFC;
   s3 = farRead8(0x496) & 0xF3;

   if (key[KEY_RSHIFT])   { s1 |= 1; }
   if (key[KEY_LSHIFT])   { s1 |= 2; }
   if (key[KEY_LCONTROL]) { s2 |= 1; s1 |= 4; }
   if (key[KEY_ALT])      { s1 |= 8; s2 |= 2; }
   if (key[KEY_RCONTROL]) { s1 |= 4; s3 |= 4; }
   if (key[KEY_ALTGR])    { s1 |= 8; s3 |= 8; }

   if (_key_shifts & KB_SCROLOCK_FLAG) s1 |= 16;
   if (_key_shifts & KB_NUMLOCK_FLAG)  s1 |= 32;
   if (_key_shifts & KB_CAPSLOCK_FLAG) s1 |= 64;

   _farsetsel(_dos_ds);

   _farnspokeb(0x417, s1);
   _farnspokeb(0x418, s2);
   _farnspokeb(0x496, s3);
}
#endif
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

    if (ms_qkeEvents.IsFull() == FALSE)
    {
      // Create event.
      static int16_t sEventIndex = 0;
      RSP_SK_EVENT* pkeEvent = ms_akeEvents + INC_N_WRAP(sEventIndex, MAX_EVENTS);
      // Fill event.
      pkeEvent->lTime    = SDL_GetTicks();
      pkeEvent->lKey = ((gkey) ? gkey : key);

      // Enqueue event . . .
      if (ms_qkeEvents.EnQ(pkeEvent) == 0)
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
  /*
  int16_t sReturn = keyboard::haveKey() ? TRUE : FALSE;    // Assume no key.

  if(sReturn == TRUE)
  {
    SET(plKey, keyboard::getKey().data);
    SET(plTime, rspGetMilliseconds());
  }
  return sReturn;
  */
  return FALSE;
}

#ifdef UNUSED_FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
//
// Check if a key is available in the keyboard queue via rspGetKey.
//
//////////////////////////////////////////////////////////////////////////////
extern int16_t rspIsKey(void)        // Returns TRUE if a key is available; FALSE if not.
{
  return (ms_qkeEvents.IsEmpty() == FALSE) ? TRUE : FALSE;
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
#define SET_sdl_to_rws_keymap2(x,y) keyboard::sdl_to_rws_keymap[SDLK_##x] = RSP_SK_##y
#define SET_sdl_to_rws_keymap(x) SET_sdl_to_rws_keymap2(x, x)
#define SET_sdl_to_rws_gkeymap2(x,y) keyboard::sdl_to_rws_gkeymap[SDLK_##x] = RSP_GK_##y
#define SET_sdl_to_rws_gkeymap(x) SET_sdl_to_rws_gkeymap2(x, x)
extern void Key_Init(void)
{
  memset(keyboard::ms_au8KeyStatus, '\0', sizeof (keyboard::ms_au8KeyStatus));

  while(!keyboard::ms_qkeEvents.IsEmpty())  // just in case.
    keyboard::ms_qkeEvents.DeQ();

  /*
  SET_sdl_to_rws_keymap(END);
  SET_sdl_to_rws_keymap(HOME);
  SET_sdl_to_rws_keymap(LEFT);
  SET_sdl_to_rws_keymap(UP);
  SET_sdl_to_rws_keymap(DOWN);
  SET_sdl_to_rws_keymap(RIGHT);
  SET_sdl_to_rws_keymap(BACKSPACE);
  SET_sdl_to_rws_keymap(TAB);
  SET_sdl_to_rws_keymap(INSERT);
  SET_sdl_to_rws_keymap(DELETE);
  SET_sdl_to_rws_keymap2(RETURN, ENTER);
  SET_sdl_to_rws_keymap2(LSHIFT, SHIFT);
  SET_sdl_to_rws_keymap2(RSHIFT, SHIFT);
  SET_sdl_to_rws_keymap2(LCTRL, CONTROL);
  SET_sdl_to_rws_keymap2(RCTRL, CONTROL);
  SET_sdl_to_rws_keymap2(LALT, ALT);
  SET_sdl_to_rws_keymap2(RALT, ALT);
  SET_sdl_to_rws_keymap(PAGEUP);
  SET_sdl_to_rws_keymap(PAGEDOWN);
  SET_sdl_to_rws_keymap(ESCAPE);
  SET_sdl_to_rws_keymap(PAUSE);
  SET_sdl_to_rws_keymap(SPACE);
#ifdef SDL2_JUNK
  SET_sdl_to_rws_keymap(PRINTSCREEN);
#endif
  SET_sdl_to_rws_keymap2(QUOTE, RQUOTE);
  SET_sdl_to_rws_keymap(COMMA);
  SET_sdl_to_rws_keymap(MINUS);
  SET_sdl_to_rws_keymap(PERIOD);
  SET_sdl_to_rws_keymap(SLASH);
  SET_sdl_to_rws_keymap(0);
  SET_sdl_to_rws_keymap(1);
  SET_sdl_to_rws_keymap(2);
  SET_sdl_to_rws_keymap(3);
  SET_sdl_to_rws_keymap(4);
  SET_sdl_to_rws_keymap(5);
  SET_sdl_to_rws_keymap(6);
  SET_sdl_to_rws_keymap(7);
  SET_sdl_to_rws_keymap(8);
  SET_sdl_to_rws_keymap(9);
  SET_sdl_to_rws_keymap(SEMICOLON);
  SET_sdl_to_rws_keymap(EQUALS);
  SET_sdl_to_rws_keymap2(a, A);
  SET_sdl_to_rws_keymap2(b, B);
  SET_sdl_to_rws_keymap2(c, C);
  SET_sdl_to_rws_keymap2(d, D);
  SET_sdl_to_rws_keymap2(e, E);
  SET_sdl_to_rws_keymap2(f, F);
  SET_sdl_to_rws_keymap2(g, G);
  SET_sdl_to_rws_keymap2(h, H);
  SET_sdl_to_rws_keymap2(i, I);
  SET_sdl_to_rws_keymap2(j, J);
  SET_sdl_to_rws_keymap2(k, K);
  SET_sdl_to_rws_keymap2(l, L);
  SET_sdl_to_rws_keymap2(m, M);
  SET_sdl_to_rws_keymap2(n, N);
  SET_sdl_to_rws_keymap2(o, O);
  SET_sdl_to_rws_keymap2(p, P);
  SET_sdl_to_rws_keymap2(q, Q);
  SET_sdl_to_rws_keymap2(r, R);
  SET_sdl_to_rws_keymap2(s, S);
  SET_sdl_to_rws_keymap2(t, T);
  SET_sdl_to_rws_keymap2(u, U);
  SET_sdl_to_rws_keymap2(v, V);
  SET_sdl_to_rws_keymap2(w, W);
  SET_sdl_to_rws_keymap2(x, X);
  SET_sdl_to_rws_keymap2(y, Y);
  SET_sdl_to_rws_keymap2(z, Z);
  SET_sdl_to_rws_keymap2(LEFTBRACKET, LBRACKET);
  SET_sdl_to_rws_keymap(BACKSLASH);
  SET_sdl_to_rws_keymap2(RIGHTBRACKET, RBRACKET);
  SET_sdl_to_rws_keymap2(KP_EQUALS, NUMPAD_EQUAL);
  SET_sdl_to_rws_keymap2(BACKQUOTE, LQUOTE);
#ifdef SDL2_JUNK
  SET_sdl_to_rws_keymap2(KP_0, NUMPAD_0);
  SET_sdl_to_rws_keymap2(KP_1, NUMPAD_1);
  SET_sdl_to_rws_keymap2(KP_2, NUMPAD_2);
  SET_sdl_to_rws_keymap2(KP_3, NUMPAD_3);
  SET_sdl_to_rws_keymap2(KP_4, NUMPAD_4);
  SET_sdl_to_rws_keymap2(KP_5, NUMPAD_5);
  SET_sdl_to_rws_keymap2(KP_6, NUMPAD_6);
  SET_sdl_to_rws_keymap2(KP_7, NUMPAD_7);
  SET_sdl_to_rws_keymap2(KP_8, NUMPAD_8);
  SET_sdl_to_rws_keymap2(KP_9, NUMPAD_9);
#endif
  SET_sdl_to_rws_keymap2(KP_MULTIPLY, NUMPAD_ASTERISK);
  SET_sdl_to_rws_keymap2(KP_PLUS, NUMPAD_PLUS);
  SET_sdl_to_rws_keymap2(KP_MINUS, NUMPAD_MINUS);
  SET_sdl_to_rws_keymap2(KP_PERIOD, NUMPAD_DECIMAL);
  SET_sdl_to_rws_keymap2(KP_DIVIDE, NUMPAD_DIVIDE);

  // Map the keypad enter to regular enter key; this lets us
  //  use it in menus, and it can't be assigned to game usage anyhow.
  //SET_sdl_to_rws_keymap2(KP_ENTER, NUMPAD_ENTER);
  SET_sdl_to_rws_keymap2(KP_ENTER, ENTER);

  SET_sdl_to_rws_keymap(F1);
  SET_sdl_to_rws_keymap(F2);
  SET_sdl_to_rws_keymap(F3);
  SET_sdl_to_rws_keymap(F4);
  SET_sdl_to_rws_keymap(F5);
  SET_sdl_to_rws_keymap(F6);
  SET_sdl_to_rws_keymap(F7);
  SET_sdl_to_rws_keymap(F8);
  SET_sdl_to_rws_keymap(F9);
  SET_sdl_to_rws_keymap(F10);
  SET_sdl_to_rws_keymap(F11);
  SET_sdl_to_rws_keymap(F12);
#ifdef SDL2_JUNK
  SET_sdl_to_rws_keymap2(LGUI, SYSTEM);
  SET_sdl_to_rws_keymap2(RGUI, SYSTEM);
#endif

  // These "stick" until you hit them again, so we should probably
  //  just not pass them on to the app.  --ryan.
  //SET_sdl_to_rws_keymap(CAPSLOCK);
  //SET_sdl_to_rws_keymap(NUMLOCKCLEAR);
  //SET_sdl_to_rws_keymap(SCROLL);

  SET_sdl_to_rws_gkeymap(END);
  SET_sdl_to_rws_gkeymap(HOME);
  SET_sdl_to_rws_gkeymap(LEFT);
  SET_sdl_to_rws_gkeymap(UP);
  SET_sdl_to_rws_gkeymap(RIGHT);
  SET_sdl_to_rws_gkeymap(DOWN);
  SET_sdl_to_rws_gkeymap(INSERT);
  SET_sdl_to_rws_gkeymap(DELETE);
  SET_sdl_to_rws_gkeymap2(LSHIFT, SHIFT);
  SET_sdl_to_rws_gkeymap2(RSHIFT, SHIFT);
  SET_sdl_to_rws_gkeymap2(LCTRL, CONTROL);
  SET_sdl_to_rws_gkeymap2(RCTRL, CONTROL);
  SET_sdl_to_rws_gkeymap2(LALT, ALT);
  SET_sdl_to_rws_gkeymap2(RALT, ALT);
  SET_sdl_to_rws_gkeymap(PAGEUP);
  SET_sdl_to_rws_gkeymap(PAGEDOWN);
  SET_sdl_to_rws_gkeymap(PAUSE);
#ifdef SDL2_JUNK
  SET_sdl_to_rws_gkeymap(PRINTSCREEN);
  SET_sdl_to_rws_gkeymap2(KP_0, NUMPAD_0);
  SET_sdl_to_rws_gkeymap2(KP_1, NUMPAD_1);
  SET_sdl_to_rws_gkeymap2(KP_2, NUMPAD_2);
  SET_sdl_to_rws_gkeymap2(KP_3, NUMPAD_3);
  SET_sdl_to_rws_gkeymap2(KP_4, NUMPAD_4);
  SET_sdl_to_rws_gkeymap2(KP_5, NUMPAD_5);
  SET_sdl_to_rws_gkeymap2(KP_6, NUMPAD_6);
  SET_sdl_to_rws_gkeymap2(KP_7, NUMPAD_7);
  SET_sdl_to_rws_gkeymap2(KP_8, NUMPAD_8);
  SET_sdl_to_rws_gkeymap2(KP_9, NUMPAD_9);
#endif
  SET_sdl_to_rws_gkeymap2(KP_MULTIPLY, NUMPAD_ASTERISK);
  SET_sdl_to_rws_gkeymap2(KP_PLUS, NUMPAD_PLUS);
  SET_sdl_to_rws_gkeymap2(KP_MINUS, NUMPAD_MINUS);
  SET_sdl_to_rws_gkeymap2(KP_PERIOD, NUMPAD_DECIMAL);
  SET_sdl_to_rws_gkeymap2(KP_DIVIDE, NUMPAD_DIVIDE);
  SET_sdl_to_rws_gkeymap(F1);
  SET_sdl_to_rws_gkeymap(F2);
  SET_sdl_to_rws_gkeymap(F3);
  SET_sdl_to_rws_gkeymap(F4);
  SET_sdl_to_rws_gkeymap(F5);
  SET_sdl_to_rws_gkeymap(F6);
  SET_sdl_to_rws_gkeymap(F7);
  SET_sdl_to_rws_gkeymap(F8);
  SET_sdl_to_rws_gkeymap(F9);
  SET_sdl_to_rws_gkeymap(F10);
  SET_sdl_to_rws_gkeymap(F11);
  SET_sdl_to_rws_gkeymap(F12);
#ifdef SDL2_JUNK
  SET_sdl_to_rws_gkeymap2(LGUI, SYSTEM);
  SET_sdl_to_rws_gkeymap2(RGUI, SYSTEM);
#endif

  //SET_sdl_to_rws_gkeymap(CAPSLOCK);
  //SET_sdl_to_rws_gkeymap(NUMLOCKCLEAR);
  //SET_sdl_to_rws_gkeymap(SCROLL);
*/
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
  return keyboard::ms_au8KeyStatus;
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
  keyboard::keyRepeat = bEnable;
}


//////////////////////////////////////////////////////////////////////////////
// EOF
//////////////////////////////////////////////////////////////////////////////
