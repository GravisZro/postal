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

// RSPix /////////////////////////////////////////////////////////////////////
#include <BLUE/Blue.h>
#include <CYAN/Cyan.h>
#include <ORANGE/CDT/slist.h>

// Platform //////////////////////////////////////////////////////////////////
#include "platform.h"
#include "video.h"

// C++ ///////////////////////////////////////////////////////////////////////
#include <cctype>
#include <memory>
#include <cstddef>


// Only set value if not nullptr.
#define SET(ptr, val)         if((ptr) != nullptr) { *(ptr) = (val); }

#define OPERATION_SUPPORTED   0x4F

inline uint16_t return_value(void) { return regs16.bx; }
inline bool has_error(void) { return regs8.al != OPERATION_SUPPORTED || regs8.ah != SUCCESS; }

namespace palette
{
  namespace aligned_memory
  {
    static std::aligned_storage<sizeof(color32_t), alignof(std::max_align_t)>::type buffer[size];
    static std::aligned_storage<sizeof(color32_t), alignof(std::max_align_t)>::type map   [size];
  }
  static color32_t* buffer = reinterpret_cast<color32_t*>(aligned_memory::buffer);
  static color32_t* map    = reinterpret_cast<color32_t*>(aligned_memory::map);
  static int8_t locks[size];	// TRUE, if an indexed entry is locked. FALSE, if not.
}

namespace display
{
  uint16_t get_mode(void);
  uint16_t set_mode(uint16_t mode);
  uint16_t probe(void);

  static dos::mem<vesa_info_t> vesa_info;
  static std::map<uint16_t, dos::mem<mode_info_t>> mode_info;
  static mode_info_t* current_mode_info = nullptr;

  static uint16_t initial_mode = get_mode();
  static uint16_t current_mode = initial_mode;

#if !defined(DOS_MEM)
  namespace aligned_memory
  {
    static std::aligned_storage<sizeof(uint8_t), alignof(std::max_align_t)>::type framebuffer[640 * 480];
  }
  static void* framebuffer = reinterpret_cast<void*>(aligned_memory::framebuffer);
#else
  static dos::mem<uint8_t, std::max_align_t> framebuffer(640 * 480);
#endif

  uint16_t set_mode(uint16_t mode)
  {
    regs8.ah  = video::vesa_function;
    regs8.al  = video::set_graphics_mode;
    regs16.bx = mode;
    dos::int86(video::interrupt);
    if (has_error())
      return FAILURE;

    current_mode_info = mode_info[mode];
    return SUCCESS;
  }

  uint16_t get_mode(void)
  {
    regs8.ah  = video::vesa_function;
    regs8.al  = video::get_graphics_mode;
    dos::int86(video::interrupt);
    if (has_error())
      return FAILURE;
    return return_value();
  }

  void set_bank(uint16_t bank_number)
  {
    regs8.ah  = video::vesa_function;
    regs8.al  = video::window_control;
    regs16.bx = video::set_bank | video::windowA;
    regs16.dx = bank_number;
    dos::int86(video::interrupt);
  }

  uint16_t probe(void)
  {
    dos::farpointer_t addr; // DOS address

    // call the VESA function
    addr = vesa_info;
    regs8.ah  = video::vesa_function;
    regs8.al  = video::graphics_card_info;
    regs16.es = addr.es;
    regs16.di = addr.di;
    dos::int86(video::interrupt);

    if (has_error()) // abort upon error
      return FAILURE;

    if (std::memcmp(vesa_info->VESASignature, "VESA", 4) != 0) // test for the magic number VESA
      return FAILURE;

    TRACE("VESA version: %04x\n", vesa_info->VESAVersion);

    if(vesa_info->VESAVersion >= 0x0200) // if VBE 2.0 or higher
    {
      vesa_info->initV2();

      // call the VESA function
      regs8.ah  = video::vesa_function;
      regs8.al  = video::graphics_card_info;
      regs16.es = addr.es;
      regs16.di = addr.di;
      dos::int86(video::interrupt);

      if (has_error()) // abort upon error
        return FAILURE;

      if (std::memcmp(vesa_info->VESASignature, "VESA", 4) != 0) // test for the magic number VESA
        return FAILURE;
    }

    if(vesa_info->Capabilities.adjustible_palette) // adjust to 8-bit if possible!
    {
      regs8.ah  = video::vesa_function;
      regs8.al  = video::palette_control;
      regs8.bl  = video::set;
      regs8.bh  = 8;
      dos::int86(video::interrupt);
      if (has_error())
        return FAILURE; // this is a worst case senario
    }

    for(uint16_t* pos = vesa_info->VideoModePtr; *pos != UINT16_MAX; ++pos)
    {
      // call the VESA function
      addr = current_mode_info = mode_info[*pos];
      regs8.ah  = video::vesa_function;
      regs8.al  = video::graphics_mode_info;
      regs16.es = addr.es;
      regs16.di = addr.di;
      regs16.cx = *pos;
      dos::int86(video::interrupt);
      TRACE("mode: %04x :: %ix%i @ %ibpp\n", *pos,
          current_mode_info->XResolution,
          current_mode_info->YResolution,
          current_mode_info->BitsPerPixel);
    }
    current_mode_info = mode_info[current_mode];
    return SUCCESS;
  }

  void blit_to_screen(void* memory_buffer, uint32_t buffer_length)
  {
    uint8_t* memory = static_cast<uint8_t*>(memory_buffer);
    uint32_t bank_size = current_mode_info->WinSize << 10;
    uint32_t bank_granularity = current_mode_info->WinGranularity << 10;
    uint32_t bank_number = 0;
    uint32_t remaining = buffer_length;
    uint32_t copy_size;

    //TRACE("Vesa DOS Address: %08x\n", current_mode_info->PhysBasePtr);
    //TRACE("Vesa ptr Address: %p\n", current_mode_info->PhysBasePtr.operator void *());
    //TRACE("Vesa fixed Address: %08x\n", video::framebuffer);
    ASSERT(buffer_length);
    ASSERT(bank_granularity);

    while (remaining > 0)
    {
      // select the appropriate bank
      set_bank(bank_number);

      // how much can we copy in one go?
      if (remaining > bank_size)
        copy_size = bank_size;
      else
        copy_size = remaining;

      // copy a bank of data to the screen
      dos::memcpy(video::buffer_address, memory, copy_size);

      // move on to the next bank of data
      remaining -= copy_size;
      memory += copy_size;
      bank_number += bank_size / bank_granularity;
    }
  }

  void load_pallette(color32_t* base, uint16_t count)
  {
    color32_t* color = base;
    if(vesa_info->Capabilities.adjustible_palette) // if possible, it will be 8-bit
    {
      for(uint16_t i = 0; i < count; ++i, ++color)
      {
        dos::outportb(video::palette_index, i);
        dos::outportb(video::palette_data, color->red  );
        dos::outportb(video::palette_data, color->green);
        dos::outportb(video::palette_data, color->blue );
      }
    }
    else // 6-bits per channel palette (reduce colors to fit)
    {
      for(uint16_t i = 0; i < count; ++i, ++color)
      {
        dos::outportb(video::palette_index, i);
        dos::outportb(video::palette_data, color->red   >> 2);
        dos::outportb(video::palette_data, color->green >> 2);
        dos::outportb(video::palette_data, color->blue  >> 2);
      }
    }
  }
}

extern void Disp_Init(void)    // Returns nothing.
{
  TRACE("VESA probe: %s\n", (display::probe() == SUCCESS ? "succeeded" : "failed"));

  // Initialize maps to indentities.
  for (uint8_t i = 1; i; ++i)
  {
    palette::map[i].red   = i;
    palette::map[i].green = i;
    palette::map[i].blue  = i;
  }

  // Never ever ever unlock these.
  palette::locks[0x00] = TRUE;
  palette::locks[0xFF] = TRUE;
}

extern void rspSetApplicationName(
   const char* pszName)                                // In: Application name
{
}


//////////////////////////////////////////////////////////////////////////////
//
// Attempts to find a mode that is sWidth by sHeight or larger
// with the given color depth.  An available mode that closest matches
// the width and height is chosen, if successful.  If no mode is found, 
// *psWidth, *psHeight.  If psPixelDoubling is not nullptr and *psPixelDoubling
// is TRUE, a mode may be returned that requires pixel doubling.  If a 
// mode requires pixel doubling, *psPixelDoubling will be TRUE on return;
// otherwise, it will be FALSE.  Passing psPixelDoubling as nullptr is
// equivalent to passing *psPixelDoubling with FALSE.
// Utilizes rspQueryVideoMode to find the mode.  Does not affect the current 
// rspQueryVideoMode.
// This function should not be a part of Blue.  It is always implementable
// via rspQueryVideoMode.
//
//////////////////////////////////////////////////////////////////////////////
extern int16_t rspSuggestVideoMode(     // Returns 0 if successfull, non-zero otherwise
    int16_t  sDepth,                    // In:  Required depth
    int16_t  sWidth,                    // In:  Requested width
    int16_t  sHeight,                   // In:  Requested height
    int16_t  sPages,                    // In:  Required pages
    int16_t  sScaling,                  // In:  Requested scaling
    int16_t* psDeviceWidth,             // Out: Suggested device width (unless nullptr)
    int16_t* psDeviceHeight,            // Out: Suggested device height (unless nullptr)
    int16_t* psScaling)                 // Out: Suggested scaling (unless nullptr)
{
  TRACE("rspSuggestVideoMode\n");
  return SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// Puts parameters about the hardware video mode into your shorts.
// You may call this function even when in "no mode" (e.g., before 
// rspSetVideoMode is first called, after it fails, or after rspKillVideMode
// is called).  This way you can get information on the user's current mode.
// If in "no mode", psWidth, psHeight, and psPages will receive 0, if not nullptr.
//
//////////////////////////////////////////////////////////////////////////////
extern int16_t rspGetVideoMode(
    int16_t* psDeviceDepth,             // Out: Hardware display color depth
    int16_t* psDeviceWidth,             // Out: Hardware display width returned here
    int16_t* psDeviceHeight,            // Out: Hardware display height returned here
    int16_t* psDevicePages,             // Out: Hardware display back buffers returned here
    int16_t* psWidth,                   // Out: Display area width returned here
    int16_t* psHeight,                  // Out: Display area height returned here
    int16_t* psPages,                   // Out: Number of pages (1 to n). More than 1 indicates a page flipping scenario.
    int16_t* psPixelScaling)            // Out: Pixel scaling in effect (1) or not (0)
{
  SET(psPixelScaling, 0);
  SET(psDevicePages,  display::current_mode_info->NumberOfImagePages);
  SET(psPages,        display::current_mode_info->NumberOfImagePages);
  SET(psWidth,        display::current_mode_info->XResolution);
  SET(psHeight,       display::current_mode_info->YResolution);
  SET(psDeviceDepth,  display::current_mode_info->BitsPerPixel);
  SET(psDeviceWidth,  display::current_mode_info->XResolution);
  SET(psDeviceHeight, display::current_mode_info->YResolution);
  return SUCCESS;
}


extern void rspQueryVideoModeReset(void)
{
}

//////////////////////////////////////////////////////////////////////////////
//
// Query the available video modes.  The modes are returned in sorted order
// based on increasing color depth, width, and height, in that order.  The
// next time the function is called after the last actual mode was reported,
// it will return non-zero (failure) to indicate that no more modes are
// available, and will continue to do so until QueryVideoReset() is
// called to reset it back to the first video mode.  When the return value is
// non-zero, the other parameters are not updated.
//
//////////////////////////////////////////////////////////////////////////////
extern int16_t rspQueryVideoMode(       // Returns 0 for each valid mode, then non-zero thereafter
    int16_t* psColorDepth,              // Out: Color depth (8, 15, 16, 24, 32)
    int16_t* psWidth,                   // Out: Width returned here
    int16_t* psHeight,                  // Out: Height returned here
    int16_t* psPages)                   // Out: Number of video pages possible.
{
  SET(psColorDepth, display::current_mode_info->BitsPerPixel);
  SET(psWidth,      display::current_mode_info->XResolution);
  SET(psHeight,     display::current_mode_info->YResolution);
  SET(psPages,      display::current_mode_info->NumberOfImagePages);
  return FAILURE;
}


//////////////////////////////////////////////////////////////////////////////
//
// Set a new video mode.  Specified color depth, width, and height for the
// device is absolute.  If these parameters cannot be met, the function will
// fail. If the requested resolution is higher than the requested display area,
// it will be centered with a black border around it.  This function can be
// called multiple times to change modes, but it does not create new
// display areas; instead, the previous buffer is destroyed and a new buffer
// is created to take it's place.
// If pixel doubling is allowed (with rspAllowPixelDoubling) and the requested
// display area is less than or equal to half the requested hardware resolution, 
// pixel doubling will be activated.
// If this function fails, you will be in no mode; meaning you may not access
// the display (i.e., call display/palette functions) even if you were in a mode
// before calling this function.  See rspKillVideoMode.
// Before this function is called, you may not call functions that manipulate
// the display.
//
//////////////////////////////////////////////////////////////////////////////
extern int16_t rspSetVideoMode(         // Returns 0 if successfull, non-zero otherwise
    int16_t sDeviceDepth,               // Specify required device video depth here.
    int16_t sDeviceWidth,               // Specify required device resolution width here.
    int16_t sDeviceHeight,              // Specify required device resolution height here.
    int16_t sWidth,                     // Specify width of display area on screen.
    int16_t sHeight,                    // Specify height of display area on screen.
    int16_t sPages,                     // Specify number of video pages.  More than 1 indicates a page flipping scenario.
    int16_t sPixelDoubling)             // TRUE indicates to set the video mode
                                        // to twice that indicated by sDeviceWidth,
                                        // sDeviceHeight and double the coordinate
                                        // system and blts.
                                        // FALSE indicates not to use this garbage.
{
  TRACE("rspSetVideoMode(%i, %i, %i, %i, %i, %i, %i)\n", sDeviceDepth, sDeviceWidth, sDeviceHeight, sWidth, sHeight, sPages, sPixelDoubling);

  if (sPixelDoubling)
  {
    fprintf(stderr, "STUBBED: pixel doubling? %s:%d\n", __FILE__, __LINE__);
    return FAILURE;
  }

  for(auto& mode : display::mode_info)
  {
    if(mode.second->BitsPerPixel == sDeviceDepth &&
       mode.second->XResolution == sWidth &&
       mode.second->YResolution == sHeight &&
       mode.second->NumberOfImagePages >= sPages)
    {
      display::set_mode(mode.first);
      return SUCCESS;
    }
  }

  return FAILURE;
}

//////////////////////////////////////////////////////////////////////////////
//
// Puts you in a state of not having display access.  After this function is
// called (similar to before rspSetVideoMode is called) you may not call
// functions that manipulate the display.  This is similar to the situation
// achieved if rspSetVideoMode fails.
//
//////////////////////////////////////////////////////////////////////////////
extern void rspKillVideoMode(void)
{
  display::set_mode(display::initial_mode); // restore initial text mode
}

//////////////////////////////////////////////////////////////////////////////
//
// Frees the memory (and, perhaps, structure(s)) associated with the memory
// stored in system RAM that is allocate rspSetVideoMode.  If you are not 
// using the system buffer (i.e., you are not calling rspLockVideoBuffer), 
// you can call this to free up some additional memory.  Calls to 
// rspLockVideoBuffer will fail after a call to this function without a 
// subsequent call to rspSetVideoMode.
//
//////////////////////////////////////////////////////////////////////////////
extern void rspKillVideoBuffer(void)
{
  // no-op
}

//////////////////////////////////////////////////////////////////////////////
//
// Variation #1: Update the entire display from the buffer.  Includes
// pixel doubling as appropriate - see elsewhere for details.
//
//////////////////////////////////////////////////////////////////////////////

extern void rspUpdateDisplayRects(void)
{
  // no-op, just blast it all to the GPU.
}

extern void rspCacheDirtyRect(
    int16_t sX,                         // x coord of upper-left corner of area to update
    int16_t sY,                         // y coord of upper-left corner of area to update
    int16_t sWidth,                     // Width of area to update
    int16_t sHeight)                    // Height of area to update
{
  UNUSED(sX,sY,sWidth,sHeight);
}

extern void rspPresentFrame(void)
{
  display::blit_to_screen(display::framebuffer, display::current_mode_info->XResolution * display::current_mode_info->YResolution);
}

extern void rspUpdateDisplay(void)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern void rspUpdateDisplay(
    int16_t sX,                         // x coord of upper-left corner of area to update
    int16_t sY,                         // y coord of upper-left corner of area to update
    int16_t sWidth,                     // Width of area to update
    int16_t sHeight)                    // Height of area to update
{
  UNUSED(sX,sY,sWidth,sHeight);
}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspLockVideoPage(        // Returns 0 if screen memory could be locked. Returns non-zero otherwise.
    void**   ppvMemory,                 // Out: Pointer to display memory.  nullptr returned if not supported.
    int32_t* plPitch)                   // Out: Pitch of display memory.
{
  UNUSED(ppvMemory,plPitch);
  // no-op
  return FAILURE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern void rspUnlockVideoPage(void)    // Returns nothing.
{
  // no-op
}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspLockVideoFlipPage(    // Returns 0 if flip screen memory could be locked.  Returns non-zero otherwise.
    void**   ppvMemory,                 // Out: Pointer to flip screen memory.  nullptr returned on failure.
    int32_t* plPitch)                   // Out: Pitch of flip screen memory.
{
  UNUSED(ppvMemory,plPitch);
  return FAILURE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern void rspUnlockVideoFlipPage(void)// Returns nothing.
{
}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspLockVideoBuffer(      // Returns 0 if system buffer could be locked.  Returns non-zero otherwise.
    void**   ppvBuffer,                 // Out: Pointer to system buffer.  nullptr returned on failure.
    int32_t* plPitch)                   // Out: Pitch of system buffer.
{
  if(ppvBuffer != nullptr)
    *ppvBuffer = display::framebuffer;
  if(plPitch != nullptr)
    *plPitch = display::current_mode_info->BytesPerScanLine;

#if defined(DOS_MEM)
  ASSERT(display::framebuffer.lock());
#endif
  return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern void rspUnlockVideoBuffer(void)  // Returns nothing.
{
#if defined(DOS_MEM)
  ASSERT(display::framebuffer.unlock());
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspAllowPageFlip(void)   // Returns 0 on success.
{
  return SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//    External Palette module functions.
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// Set several palette entries.  Separate pointers to each component
// combined with caller-specified increment for all pointers allows
// use of this function with any (non-packed) arrangement of RGB data.
// Hardware palette is not updated until UpdatePalette() is called.
//
///////////////////////////////////////////////////////////////////////////////
extern void rspSetPaletteEntries(
    palindex_t sStartIndex,               // In: Palette entry to start copying to (has no effect on source!)
    palindex_t sCount,                    // In: Number of palette entries to do
    channel_t* pucRed,                    // In: Pointer to first red   component to copy from
    channel_t* pucGreen,                  // In: Pointer to first green component to copy from
    channel_t* pucBlue,                   // In: Pointer to first blue  component to copy from
    uint32_t lIncBytes)                   // In: Number of bytes by which to increment pointers after each copy
{
  int8_t* psLock;
  color32_t* pColor;

  for(psLock = palette::locks + sStartIndex, // Set up lock pointer
      pColor = palette::buffer + sStartIndex; // Set up destination
      sCount-- > 0; // loop condition
      ++psLock, // Increment lock
      ++pColor, // Increment destination
      pucRed   += lIncBytes, // Increment sources
      pucGreen += lIncBytes,
      pucBlue  += lIncBytes)
  {
    if (*psLock == FALSE)
    {
      pColor->red   = *pucRed; // transfer data
      pColor->green = *pucGreen;
      pColor->blue  = *pucBlue;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Set palette entry.  Hardware palette is not updated until
// UpdatePalette() is called.
//
///////////////////////////////////////////////////////////////////////////////
void rspSetPaletteEntry(
    palindex_t sEntry,                    // Palette entry (0x00 to 0xFF)
    channel_t ucRed,                      // Red   component (0x00 to 0xFF)
    channel_t ucGreen,                    // Green component (0x00 to 0xFF)
    channel_t ucBlue)                     // Blue  component (0x00 to 0xFF)
{
  ASSERT(sEntry < palette::size);

  if (palette::locks[sEntry] == FALSE)
  {
    palette::buffer[sEntry].red   = ucRed;
    palette::buffer[sEntry].green = ucGreen;
    palette::buffer[sEntry].blue  = ucBlue;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Get palette entry.  This reflects the palette that may or may NOT have
// been set by calling rspUpdatePalette.
//
///////////////////////////////////////////////////////////////////////////////
void rspGetPaletteEntry(
    palindex_t sEntry,                      // Palette entry (0x00 to 0xFF)
    channel_t* psRed,                       // Red   component (0x00 to 0xFF) returned if not nullptr.
    channel_t* psGreen,                     // Green component (0x00 to 0xFF) returned if not nullptr.
    channel_t* psBlue)                      // Blue  component (0x00 to 0xFF) returned if not nullptr.
{
  ASSERT(sEntry < palette::size);

  SET(psRed,   palette::buffer[sEntry].red); // transfer data
  SET(psGreen, palette::buffer[sEntry].green);
  SET(psBlue,  palette::buffer[sEntry].blue);
}

///////////////////////////////////////////////////////////////////////////////
//
// Get several palette entries.  Separate pointers to each component
// combined with caller-specified increment for all pointers allows
// use of this function with any (non-packed) arrangement of RGB data.
// This is the palette that may not necessarily be updated yet in the hardware
// (i.e., some may have set the palette and not called UpdatePalette()).
//
///////////////////////////////////////////////////////////////////////////////
extern void rspGetPaletteEntries(
    palindex_t sStartIndex,                 // Palette entry to start copying from
    palindex_t sCount,                      // Number of palette entries to do
    channel_t* pucRed,                      // Pointer to first red   component to copy to
    channel_t* pucGreen,                    // Pointer to first green component to copy to
    channel_t* pucBlue,                     // Pointer to first blue  component to copy to
    uint32_t lIncBytes)                     // Number of bytes by which to increment pointers after each copy
{
  for(color32_t* pColor = palette::buffer + sStartIndex; // Set up source
      sCount-- > 0; // loop condition
      ++pColor, // Increment source
      pucRed   += lIncBytes, // Increment destination
      pucGreen += lIncBytes,
      pucBlue  += lIncBytes)
  {
    *pucRed   = pColor->red; // transfer data
    *pucGreen = pColor->green;
    *pucBlue  = pColor->blue;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Update hardware palette using information that has been set
// using SetPaletteEntry() and SetPaletteEntries().
// In the future, this may support optional VBLANK-synced updates.
//
///////////////////////////////////////////////////////////////////////////////
extern void rspUpdatePalette(void)
{
  display::load_pallette(palette::buffer, palette::size);
}

///////////////////////////////////////////////////////////////////////////////
//
// Set entries in the color map used to tweak values set via 
// rspSetPaletteEntries().  Those colors' values will be used as indices
// into this map when rspUpdatePalette() is called.  The resulting values
// will be updated to the hardware.
// rspGetPaletteEntries/Entry() will still return the original values set 
// (not mapped values).
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspSetPaletteMaps(
    palindex_t sStartIndex,                 // Map entry to start copying to (has no effect on source!)
    palindex_t sCount,                      // Number of map entries to do
    channel_t* pucRed,                      // Pointer to first red   component to copy from
    channel_t* pucGreen,                    // Pointer to first green component to copy from
    channel_t* pucBlue,                     // Pointer to first blue  component to copy from
    uint32_t lIncBytes)                     // Number of bytes by which to increment pointers after each copy
{
  for(color32_t* pColor = palette::map + sStartIndex; // Set up destination
      sCount-- > 0; // loop condition
      ++pColor, // Increment destination
      pucRed   += lIncBytes, // Increment sources
      pucGreen += lIncBytes,
      pucBlue  += lIncBytes)
  {
    pColor->red   = *pucRed; // transfer data
    pColor->green = *pucGreen;
    pColor->blue  = *pucBlue;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Get entries in the color map used to tweak values set via 
// rspSetPaletteEntries().  Those colors' values will be used as indices
// into this map when rspUpdatePalette() is called.  The resulting values
// will be updated to the hardware.
// rspGetPaletteEntries/Entry() will still return the original values set 
// (not mapped values).
// 
///////////////////////////////////////////////////////////////////////////////
extern void rspGetPaletteMaps(
    palindex_t sStartIndex,                 // Map entry to start copying from (has no effect on dest!)
    palindex_t sCount,                      // Number of map entries to do
    channel_t* pucRed,                      // Pointer to first red   component to copy to
    channel_t* pucGreen,                    // Pointer to first green component to copy to
    channel_t* pucBlue,                     // Pointer to first blue  component to copy to
    uint32_t lIncBytes)                     // Number of bytes by which to increment pointers after each copy
{
  for(color32_t* pColor = palette::map + sStartIndex; // Set up source
      sCount-- > 0; // loop condition
      ++pColor, // Increment source
      pucRed   += lIncBytes, // Increment destinations
      pucGreen += lIncBytes,
      pucBlue  += lIncBytes)
  {
    *pucRed   = pColor->red; // transfer data
    *pucGreen = pColor->green;
    *pucBlue  = pColor->blue;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Lock several palette entries.  Locking an entry keeps it from
// being updated by rspSetPaletteEntries() (until it is unlocked
// with rspUnlockPaletteEntries() ).
///////////////////////////////////////////////////////////////////////////////
extern void rspLockPaletteEntries(
    palindex_t    sStartIndex,               // Palette entry at which to start locking.
    palindex_t    sCount)                    // Number of palette entries to lock.
{
  for(int8_t* psLock = palette::locks + sStartIndex; sCount-- > 0; ++psLock)
    *psLock = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Unlock several palette entries previously locked by rspLockPaletteEntries().
///////////////////////////////////////////////////////////////////////////////
extern void rspUnlockPaletteEntries(
    palindex_t    sStartIndex,               // Palette entry at which to start locking.
    palindex_t    sCount)                    // Number of palette entries to lock.
{
  for(int8_t* psLock = palette::locks + sStartIndex; sCount-- > 0; ++psLock)
    *psLock = FALSE;

  // Never ever ever unlock these.
  palette::locks[0x00] = TRUE;
  palette::locks[0xFF] = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Dyna schtuff.
///////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//    External Background module functions.
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// Set a callback to be called when the application moves into the background.
//
///////////////////////////////////////////////////////////////////////////////
extern void rspSetBackgroundCallback(     // Returns nothing.
    void (BackgroundCall)(void))          // Callback when app processing becomes
                                          // background.  nullptr to clear.
{
  UNUSED(BackgroundCall);
  // no-op
}

///////////////////////////////////////////////////////////////////////////////
//
// Set a callback to be called when the application moves into the foreground.
//
///////////////////////////////////////////////////////////////////////////////
extern void rspSetForegroundCallback(     // Returns nothing.
    void (ForegroundCall)(void))          // Callback when app processing becomes foreground.  nullptr to clear.
{
  UNUSED(ForegroundCall);
  // no-op
}

extern bool rspIsBackground(void)      // Returns TRUE if in background, FALSE otherwise
{
  // no-op
  return false;
}
//////////////////////////////////////////////////////////////////////////////
// EOF
//////////////////////////////////////////////////////////////////////////////
