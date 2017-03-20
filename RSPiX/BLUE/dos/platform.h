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

//
// platform.h: I'd call it dos.h, but the name's taken
//

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <BLUE/System.h>
#include <new>
#include <limits>

static_assert(sizeof(int        ) == 4, "architecture mismatch");
static_assert(sizeof(long       ) == 4, "architecture mismatch");
static_assert(sizeof(uintptr_t  ) == 4, "architecture mismatch");
static_assert(sizeof(std::size_t) == 4, "architecture mismatch");

#define regs32 dos::regs.r32
#define regs16 dos::regs.r16
#define regs8  dos::regs.r8

namespace dos
{
  void* malloc(std::size_t size) noexcept;
  void free(void* ptr) noexcept;

  template<typename T>
  class mem
  {
  public:
    mem(void) noexcept { m_data = reinterpret_cast<T*>(dos::malloc(sizeof(T))); }
   ~mem(void) noexcept { dos::free(m_data); m_data = nullptr; }
    T* operator ->(void) noexcept { return m_data; }
    operator T*(void) noexcept { return m_data; }

  private:
    T* m_data;
  };

  struct farpointer_t
  {
    // convience shortcut functions
    template<typename T> farpointer_t& operator = (T* in) noexcept { return operator =(reinterpret_cast<void*>(in)); }
    template<typename T> operator T*(void) noexcept { return reinterpret_cast<T*>(operator void*()); }

    // actual functions
    farpointer_t& operator = (uintptr_t in) noexcept { real = in; return *this; }
    farpointer_t& operator = (void* in) noexcept;
    operator uintptr_t(void) const noexcept { return real; } // far pointer
    operator void*(void) const noexcept; // normal pointer

    union
    {
      struct
      {
        uintptr_t es : 28; // segment / upper 28 bits
        uintptr_t di :  4; // offset  / lower 4 bits
      };
      struct
      {
        uintptr_t low  : 16; // lower 16 bits
        uintptr_t high : 16; // upper 16 bits
      };
      uintptr_t real;
      void*     pointer;
    };
  };

  static_assert(sizeof(farpointer_t) == 4, "your compiler is broken");


  uint16_t lockmem  (void* addr, std::size_t size) noexcept;
  uint16_t unlockmem(void* addr, std::size_t size) noexcept;
  void memcpy(uintptr_t dest, void* src, std::size_t length);

  struct regs_t
  {
    union
    {
      struct {
        uint32_t edi;
        uint32_t esi;
        uint32_t ebp;
        uint32_t res;
        uint32_t ebx;
        uint32_t edx;
        uint32_t ecx;
        uint32_t eax;
      } r32;
      struct {
        uint16_t di, di_hi;
        uint16_t si, si_hi;
        uint16_t bp, bp_hi;
        uint16_t res, res_hi;
        uint16_t bx, bx_hi;
        uint16_t dx, dx_hi;
        uint16_t cx, cx_hi;
        uint16_t ax, ax_hi;
        uint16_t flags;
        uint16_t es;
        uint16_t ds;
        uint16_t fs;
        uint16_t gs;
        uint16_t ip;
        uint16_t cs;
        uint16_t sp;
        uint16_t ss;
      } r16;
      struct {
        uint8_t edi[4];
        uint8_t esi[4];
        uint8_t ebp[4];
        uint8_t res[4];
        uint8_t bl, bh, ebx_b2, ebx_b3;
        uint8_t dl, dh, edx_b2, edx_b3;
        uint8_t cl, ch, ecx_b2, ecx_b3;
        uint8_t al, ah, eax_b2, eax_b3;
      } r8;
    };
  };

  static_assert(sizeof(regs_t::r32) == 32, "bad size!");
  static_assert(sizeof(regs_t::r16) == 50, "bad size!");
  static_assert(sizeof(regs_t::r8 ) == 32, "bad size!");
  static_assert(sizeof(regs_t) == 52, "Do not pack the registers!");

  extern regs_t regs;

  uint8_t  inportb(uint16_t port) noexcept;
  uint16_t inportw(uint16_t port) noexcept;
  void outportb(uint16_t port, uint8_t  val) noexcept;
  void outportw(uint16_t port, uint16_t val) noexcept;

  void irqenable(void) noexcept;
  void irqdisable(void) noexcept;
  void registerintr(uint8_t intr, void (*handler)(void)) noexcept;
  void restoreintr(uint8_t intr) noexcept;

  bool int86(uint8_t vec) noexcept; // Returns true on success
  bool int386(uint8_t vec, regs_t* inregs, regs_t* outregs) noexcept;

  void usleep(milliseconds_t usecs) noexcept;

  std::size_t getheapsize(void) noexcept;

#if 0
  template<typename T>
  class allocator {
  public:
    //    typedefs
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

  public:
    //    convert an allocator<T> to allocator<U>
    template<typename U>
    struct rebind
      { typedef allocator<U> other; };

  public:
    allocator(void) { }
    allocator(const allocator& other) { }
   ~allocator(void) { }

    //    address
    pointer address(reference x) const { return &x; }
    const_pointer address(const_reference x) const { return &x; }

    //    memory allocation
    pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0 )
      { UNUSED(hint); return reinterpret_cast<pointer>(dos::malloc(n * sizeof (T))); }

    void deallocate(pointer p, size_type)
    {
      dos::free(p);
      p = nullptr;
    }

    //    size
    size_type max_size(void) const
      { return std::numeric_limits<size_type>::max() / sizeof(T); }

    bool operator==(allocator const&) { return true; }
    bool operator!=(allocator const&) { return false; }
  };    //    end of class allocator
#endif
}

#endif	// _PLATFORM_H_
