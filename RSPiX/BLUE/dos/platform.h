#ifndef PLATFORM_H
#define PLATFORM_H

#include <BLUE/System.h>

static_assert(sizeof(uintptr_t) == 4, "DOS is 32-bit!");

#if defined(__DJGPP__)

# include <pc.h>
# include <dpmi.h>
# include <go32.h>
# include <sys/farptr.h>

static __dpmi_regs regs; // optimized out?

# define load_AX(val)           regs.x.ax = val
# define load_BX(val)           regs.x.bx = val
# define load_CX(val)           regs.x.cx = val
# define load_DX(val)           regs.x.dx = val
# define load_DI(val)           regs.x.di = val
# define load_ES(val)           regs.x.es = val

# define get_AH                 regs.h.ah
# define get_BX                 regs.x.bx

# define interrupt(x)           __dpmi_int(int(x), &regs)

# define farRead16(addr)        _farpeekw(_dos_ds, addr)
# define farRead8(addr)         _farpeekb(_dos_ds, addr)

# define farWrite16(addr, val)  _farpokew(_dos_ds, addr, val)
# define farWrite8(addr, val)   _farpokeb(_dos_ds, addr, val)


# define transaction_buffer     __tb

# define readHW8(x)             inportb(x)
# define readHW16(x)            inportw(x)

# define writeHW8(x, val)       outportb(x, val)
# define writeHW16(x, val)      outportw(x, val)

#elif defined(_MSC_VER)

# error You need to implement a bunch of x86/DOS support in platform.h!

#endif


#define bios_error_code       get_AH
#define biod_return_value     get_BX

enum class interrupts : int
{
  keyboard = 0x09,
  video    = 0x10,
};


namespace vesa
{
  enum registers : uint16_t // write to AX
  {
    color_number    = 0x03C8,
    color_value     = 0x03C9,
    get_card_info   = 0x4F00,
    get_mode_info   = 0x4F01,
    set_mode        = 0x4F02,
    get_mode        = 0x4F03,
    window_control  = 0x4F05,
  };

  enum options : uint16_t // write to BX
  {
    set_bank = 0x0000,
    get_bank = 0x0100,
    windowA  = 0x0000,
    windowB  = 0x0001,
  };

  enum addresses : uint32_t
  {
    framebuffer = 0x000A0000,
  };
}


struct vesa_info_t
{
  uint8_t VESASignature[4];
  uint16_t VESAVersion;
  uint32_t OEMStringPtr;
  uint8_t Capabilities[4];
  uint32_t VideoModePtr;
  uint16_t TotalMemory;
  uint16_t OemSoftwareRev;
  uint32_t OemVendorNamePtr;
  uint32_t OemProductNamePtr;
  uint32_t OemProductRevPtr;
  uint8_t Reserved[222];
  uint8_t OemData[256];
} __attribute__ ((packed));

static_assert(sizeof(vesa_info_t) == 512, "packing failed?");

struct mode_info_t
{
  uint16_t ModeAttributes;
  uint8_t WinAAttributes;
  uint8_t WinBAttributes;
  uint16_t WinGranularity; // in kilobytes
  uint16_t WinSize; // in kilobytes
  uint16_t WinASegment;
  uint16_t WinBSegment;
  uint32_t WinFuncPtr;
  uint16_t BytesPerScanLine;
  uint16_t XResolution; // x pixel
  uint16_t YResolution; // y pixels
  uint8_t XCharSize;
  uint8_t YCharSize;
  uint8_t NumberOfPlanes;
  uint8_t BitsPerPixel;
  uint8_t NumberOfBanks;
  uint8_t MemoryModel;
  uint8_t BankSize;
  uint8_t NumberOfImagePages;
  uint8_t Reserved_page;
  uint8_t RedMaskSize;
  uint8_t RedMaskPos;
  uint8_t GreenMaskSize;
  uint8_t GreenMaskPos;
  uint8_t BlueMaskSize;
  uint8_t BlueMaskPos;
  uint8_t ReservedMaskSize;
  uint8_t ReservedMaskPos;
  uint8_t DirectColorModeInfo;
  uint32_t PhysBasePtr; // address
  uint32_t OffScreenMemOffset;
  uint16_t OffScreenMemSize;
  uint8_t Reserved[206];
} __attribute__ ((packed));

#endif // PLATFORM_H
