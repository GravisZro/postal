#ifndef VESA_H
#define VESA_H

#include <BLUE/System.h>

#include "platform.h"

namespace video
{
  enum : int { interrupt = 0x10 };

  enum registers : uint16_t // write to AX
  {
    sequence_controller_index   = 0x03C4,
    sequence_controller_data    = 0x03C5,
    palette_index               = 0x03C8, // 6-bits per channel (unless adjustible)
    palette_data                = 0x03C9,
    graphics_controller_index   = 0x03CE,
    graphics_controller_data    = 0x03CF,
    graphics_status             = 0x03DA,
    graphics_card_info          = 0x4F00,
    graphics_mode_info          = 0x4F01,
    set_graphics_mode           = 0x4F02,
    get_graphics_mode           = 0x4F03,
// VBE 2
    graphics_mode_backup        = 0x4F04,
    window_control              = 0x4F05,
    scanline_length             = 0x4F06,
    display_start               = 0x4F07,
    palette_control             = 0x4F08,
    palette_interface           = 0x4F09,
    serial_controller_interface = 0x4F15, // i2c interface

  };

  enum vbe2_options : uint8_t // write to BL
  {
    set = 0x00,
    get = 0x01,
  };

  enum window_control_options : uint16_t // write to BX
  {
    set_bank = 0x0000,
    get_bank = 0x0100,
    windowA  = 0x0000,
    windowB  = 0x0001,
  };

  enum serial_controller_interface_functions : uint16_t // write to BX (CX = monitor port number)
  {
    get_capabilities = 0x0010,
    begin_control,
    end_control,
    write_clock_line,
    write_data_line,
    read_clock_line,
    read_data_line,
  };

  enum addresses : uint32_t
  {
    framebuffer = 0x000A0000,
  };
}

struct vesa_capabilities_t
{
  uint32_t adjustible_palette : 1; // Set if the DAC can switch width, clear if it is fixed 6bits per primary color
  uint32_t non_vga_controller : 1; // non-VGA controller
  uint32_t dac_blank_bit      : 1; // Programmed DAC with blank bit
};

struct vesa_info_t
{
  uint8_t             VESASignature[4];
  uint16_t            VESAVersion;
  dos::farpointer_t   OEMStringPtr;
  vesa_capabilities_t Capabilities;
  dos::farpointer_t   VideoModePtr;
  uint16_t            TotalMemory;
  uint16_t            OemSoftwareRev;
  dos::farpointer_t   OemVendorNamePtr;
  dos::farpointer_t   OemProductNamePtr;
  dos::farpointer_t   OemProductRevPtr;
  uint8_t             Reserved[222];
  uint8_t             OemData[256];
} __attribute__ ((packed));

static_assert(sizeof(vesa_info_t) == 512, "packing failed?");

struct mode_attributes_t
{
  uint16_t hw_support   : 1 ; // 0 = not support in hardware
  uint16_t              : 1 ; // reserved
  uint16_t bios_support : 1 ; // 0 = not supported by bios
  uint16_t is_color     : 1 ; // 0 = monochrome; 1 = color
  uint16_t is_graphics  : 1 ; // 0 = text mode; 1 = graphics mode
};

static_assert(sizeof(mode_attributes_t) == 2, "definition failed?");

struct window_attributes_t
{
  uint8_t supported : 1; // 1 = window is supported
  uint8_t readable  : 1; // 1 = window is readable
  uint8_t writeable : 1; // 1 = window is writable
};

static_assert(sizeof(window_attributes_t) == 1, "definition failed?");


enum memory_model_e : uint8_t
{
  text_mode     = 0,
  CGA_graphics,
  HerculesGraphics,
  QuadplanePlanar,
  PackedPixel,
  NonChain4_256colors,
  DirectColor,
  YUV,
};
static_assert(sizeof(memory_model_e) == 1, "definition failed?");

struct direct_color_mode_t
{
  uint8_t is_programmable : 1; // 0 = fixed; 1 = programmable
  uint8_t reserved_mask_usable : 1; // 1 = ReservedMask* is usable
};

struct mode_info_t
{
  mode_attributes_t ModeAttributes;
  window_attributes_t WinAAttributes;
  window_attributes_t WinBAttributes;
  uint16_t WinGranularity; // in kilobytes
  uint16_t WinSize; // in kilobytes
  uint16_t WinASegment;
  uint16_t WinBSegment;
  dos::farpointer_t WinFuncPtr;
  uint16_t BytesPerScanLine;
  uint16_t XResolution; // x pixel
  uint16_t YResolution; // y pixels
  uint8_t XCharSize;
  uint8_t YCharSize;
  uint8_t NumberOfPlanes;
  uint8_t BitsPerPixel;
  uint8_t NumberOfBanks;
  memory_model_e MemoryModel;
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
  direct_color_mode_t DirectColorModeInfo;
  dos::farpointer_t PhysBasePtr; // address
  uint32_t OffScreenMemOffset;
  uint16_t OffScreenMemSize;
  uint8_t Reserved[206];
} __attribute__ ((packed));


static_assert(sizeof(mode_info_t) == 256, "packing failed?");

#endif // VESA_H
