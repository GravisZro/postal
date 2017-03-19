#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "platform.h"
#include "ps2.h"

namespace keyboard // AT or PS/2 Keyboard
{
  enum : uint8_t  { interrupt = 0x09 };
  enum : uint16_t { port = 0x0060 };

  enum command : uint8_t
  {
    led_state           = 0xED,
    scancodeset         = 0xF0,
    set_rate_and_delay  = 0xF3,
    enable_scanning     = 0xF4,
    disable_scanning    = 0xF5,
    load_defaults       = 0xF6,
    reset_keyboard      = 0xFF,
  };

  enum scancodesets : uint8_t
  {
    get  = 0x00,
    set1 = 0x01,
    set2 = 0x02,
    set3 = 0x03,
  };

  enum responses : uint8_t
  {
    // Key detection error or internal buffer overrun
    internal_error0     = 0x00,
    internal_error1     = 0xFF,

    // Diagnostic results - sent after "0xFF (reset)" command or keyboard power up
    diagnostic_success  = 0xAA, // Diagnostic passed
    diagnostic_failure0 = 0xFC, // Diagnostic failed
    diagnostic_failure1 = 0xFD, // Diagnostic failed

    echo                = 0xEE, // Response to "0xEE (echo)" command
    acknowledged        = 0xFA, // Command acknowledged (ACK)
    resend              = 0xFE, // Resend last command (keyboard/program wants program/keyboard to repeat last command it sent)
  };

  union keycode
  {
    struct
    {
      uint16_t key;
      uint16_t modifiers;
    };
    uint32_t data;

    keycode(void) : data(0) { }
    keycode(uint32_t d) : data(d) { }
  };

  static inline bool write(uint8_t cmd)
  {
    for(uint8_t attempts = 4; attempts; --attempts)
    {
      if (!ps2::wait_for_status<ps2::output_buffer_ready>(0x4000))
        return false; // keyboard has timed out!

      dos::outportb(port, cmd);

      for (uint16_t timeout = 0x1000; timeout; --timeout)
      {
        if (!ps2::wait_for_status<ps2::input_buffer_ready>(0x1000))
          return false; // keyboard has timed out!

        switch(dos::inportb(port))
        {
          case internal_error0:
          case internal_error1: // perhaps set a flag for the internal errors?
          case diagnostic_failure0:
          case diagnostic_failure1:
            return false;

          case diagnostic_success:
          case echo:
          case acknowledged:
            return true;

          case resend: // forcibly expire timer
            timeout = 1;
            break;
          default: // unknown reply... read the next bytes
            break;
        }
      }
    }
    return false; // failed to complete in time!
  }

  static inline bool write(command cmd, uint8_t arg)
    { return write(cmd) && write(arg); }
  static inline bool reset(void)
    { return write(reset_keyboard); }
}

#endif // KEYBOARD_H
