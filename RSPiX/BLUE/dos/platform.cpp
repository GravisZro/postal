/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "platform.h"

#if defined(__DJGPP__)

#include <unistd.h>
#include <cstring>

#include <sys/segments.h>
#include <go32.h>
#include <sys/nearptr.h>
#include <dos.h>
#include <dpmi.h>
#include <bios.h>


//_go32_dpmi_registers hmm;
namespace dos
{
  /////////////////////////////////////////////////////////////////////////////////
  // ======= DOS memory interface =======
  /////////////////////////////////////////////////////////////////////////////////

  inline uintptr_t ptr2real(void* ptr) noexcept { return reinterpret_cast<uintptr_t>(ptr) - __djgpp_conventional_base; }
  inline void* real2ptr(uintptr_t real) noexcept { return reinterpret_cast<void*>(real + __djgpp_conventional_base); }

  // Because of a quirk in dj's alloc-dos-memory wrapper, you need to keep
  // the seginfo structure around for when you free the mem.
  static _go32_dpmi_seginfo seginfo[64] = { { 0, 0, 0, 0, 0 } };

  void* malloc(std::size_t size) noexcept
  {
    static bool firsttime = true;
    _go32_dpmi_seginfo info;

    if (firsttime)
    {
      ASSERT(__djgpp_nearptr_enable());
      firsttime = false;
    }

    info.size = (size + 15) / 16;
    if (_go32_dpmi_allocate_dos_memory(&info))
      return nullptr;

    for(_go32_dpmi_seginfo& seg : seginfo)
    {
      if(seg.rm_segment != FALSE)
      {
        seg = info;
        break;
      }
    }
    void* ptr = real2ptr(info.rm_segment << 4);
    std::memset(ptr, 0, size); // initialize memory to zeros
    return ptr;
  }

  void free(void* ptr) noexcept
  {
    uintptr_t segment = ptr2real(ptr) >> 4;
    for(_go32_dpmi_seginfo& seg : seginfo)
    {
      if (seg.rm_segment == segment)
      {
        _go32_dpmi_free_dos_memory(&seg);
        seg.rm_segment = FALSE;
        break;
      }
    }
  }

  farpointer_t& farpointer_t::operator =(void* in) noexcept // ptr2far
  {
    pointer = in;
    real -= __djgpp_conventional_base; // ptr2real
    es >>= 4;
    return *this;
  }

  farpointer_t::operator void*(void) const noexcept // far2ptr
    { return reinterpret_cast<void*>(__djgpp_conventional_base + (high << 4) + low); }




  /////////////////////////////////////////////////////////////////////////////////
  // ======= DOS hardware interface =======
  /////////////////////////////////////////////////////////////////////////////////

  // globals
  regs_t regs;
  void (*error_func)(const char* msg, ...);

  int inportb(int port) noexcept { return ::inportb(port); }
  int inportw(int port) noexcept { return ::inportw(port); }
  void outportb(int port, uint32_t val) noexcept { ::outportb(port, val); }
  void outportw(int port, uint32_t val) noexcept { ::outportw(port, val); }
  void irqenable (void) noexcept { ::enable (); }
  void irqdisable(void) noexcept { ::disable(); }

  int int86(int vec) noexcept
  {
    int rc;
    regs.r16.ss = regs.r16.sp = 0;
    rc = _go32_dpmi_simulate_int(vec, (_go32_dpmi_registers *) &regs);
    return rc || (regs.r16.flags & 0x0001);
  }

  int int386(int vec, regs_t* inregs, regs_t* outregs) noexcept
  {
    int rc;
    std::memcpy(outregs, inregs, sizeof(regs_t));
    outregs->r16.ss = outregs->r16.sp = 0;
    rc = _go32_dpmi_simulate_int(vec, reinterpret_cast<_go32_dpmi_registers*>(outregs));
    return rc || (outregs->r16.flags & 0x0001);
  }

  static struct handlerhistory_s
  {
    uint8_t intr;
    _go32_dpmi_seginfo pm_oldvec;
  } handlerhistory[64] = { { UINT8_MAX, { 0, 0, 0, 0, 0 } } };

  static uint8_t handlercount = 0;

  void registerintr(uint8_t intr, void (*handler)(void)) noexcept
  {
    _go32_dpmi_seginfo info;
    struct handlerhistory_s *oldstuff;

    oldstuff = &handlerhistory[handlercount];

    // remember old handler
    _go32_dpmi_get_protected_mode_interrupt_vector(intr, &oldstuff->pm_oldvec);
    oldstuff->intr = intr;

    info.pm_offset = reinterpret_cast<uintptr_t>(handler);
    _go32_dpmi_allocate_iret_wrapper(&info);

    // set new protected mode handler
    _go32_dpmi_set_protected_mode_interrupt_vector(intr, &info);

    ++handlercount;
  }

  void restoreintr(uint8_t intr) noexcept
  {
    handlerhistory_s* end = handlerhistory + handlercount;

    // find and reinstall previous interrupt
    for (handlerhistory_s* pos = handlerhistory; pos != end; ++pos)
    {
      if (pos->intr == intr)
      {
        _go32_dpmi_set_protected_mode_interrupt_vector(intr, &pos->pm_oldvec);
        pos->intr = UINT8_MAX;
        break;
      }
    }
  }

  void usleep(milliseconds_t usecs) noexcept { ::usleep(usecs); }

  int getheapsize(void) noexcept { return _go32_dpmi_remaining_physical_memory(); }

  uint16_t lockmem(void* addr, std::size_t size) noexcept
  {
    __dpmi_meminfo info;
    info.address = reinterpret_cast<uintptr_t>(addr) + __djgpp_base_address;
    info.size = size;
    if (__dpmi_lock_linear_region(&info))
      return __dpmi_error;
    return 0;
  }

  uint16_t unlockmem(void* addr, std::size_t size) noexcept
  {
    __dpmi_meminfo info;
    info.address = reinterpret_cast<uintptr_t>(addr) + __djgpp_base_address;
    info.size = size;
    if (__dpmi_unlock_linear_region(&info))
      return __dpmi_error;
    return 0;
  }
}

#else
#error You need to implement the functions listed in dosplatform.h
#endif
