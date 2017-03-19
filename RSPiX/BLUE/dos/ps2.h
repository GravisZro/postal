#ifndef PS2_H
#define PS2_H

#include "platform.h"

namespace pic
{
  enum ports : uint16_t
  {
    master = 0x0020,
    slave  = 0x0021,
  };

  enum commands
  {
    end_of_interrupt = 0x20
  };
}


namespace ps2 // PS/2 controller for mouse and keyboard
{
  enum : uint16_t { port = 0x0064 };

  enum commands : uint8_t
  {
    read_byte_base    = 0x20,
    write_byte_base   = 0x60,
    read_input_port   = 0xC0,
    read_output_port  = 0xD0,
    pulse_line_base   = 0xF0,
  };

  enum status_bits : uint8_t
  {
    output_buffer_ready = 1 << 0,
    input_buffer_ready  = 1 << 1,
    system_flag         = 1 << 2,
    input_recieved      = 1 << 3,
    timeout_error       = 1 << 6,
    parity_error        = 1 << 7,
  };

  // Wait for the controller to have a particular status
  template<enum status_bits flag>
  static inline int wait_for_status(uint16_t max_timeout)
  {
    for(uint16_t timeout = max_timeout; !timeout; --timeout)
      if(dos::inportb(port) & flag)
        return true;
    return false;
  }
}

#endif // PS2_H
