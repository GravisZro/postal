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
#include <dc/video.h>
#include <dc/pvr.h>

// C++ ///////////////////////////////////////////////////////////////////////
#include <cctype>
#include <memory>

// C /////////////////////////////////////////////////////////////////////////
#include <unistd.h>



// Only set value if not nullptr.
#define SET(ptr, val)        if((ptr) != nullptr) { *(ptr) = (val); }

namespace palette
{
  namespace aligned_memory
  {
    static std::aligned_storage<sizeof(color32_t), alignof(uint128_t)>::type buffer[size];
    static std::aligned_storage<sizeof(color32_t), alignof(uint128_t)>::type map   [size];
  }
  static color32_t* buffer = reinterpret_cast<color32_t*>(aligned_memory::buffer);
  static color32_t* map    = reinterpret_cast<color32_t*>(aligned_memory::map);
  static uint8_t locks[size];	// TRUE, if an indexed entry is locked. FALSE, if not.
}

namespace display
{
  const uint16_t width  = 640;
  const uint16_t height = 480;
  const uint16_t depth  = 8;

  namespace aligned_memory
  {
    static std::aligned_storage<sizeof(uint8_t), alignof(uint128_t)>::type framebuffer[width * height];
  }
  static void* framebuffer = reinterpret_cast<void*>(aligned_memory::framebuffer);

  static pvr_ptr_t texture;
}



extern void Disp_Init(void)    // Returns nothing.
{
  vid_init(DM_640x480, PM_RGB888);
  vid_border_color(0xdd, 0, 0);

  pvr_init_defaults();
  pvr_set_pal_format(PVR_PAL_ARGB8888);
  display::texture = pvr_mem_malloc(display::width * display::height);
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
  // lie about everything.
  SET(psPixelScaling, 0);
  SET(psDevicePages, 0);
  SET(psPages, 1);
  SET(psWidth,        display::width);
  SET(psHeight,       display::height);
  SET(psDeviceDepth,  display::depth);
  SET(psDeviceHeight, display::width);
  SET(psDeviceWidth,  display::height);

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
  SET(psColorDepth, display::depth);
  SET(psWidth,      display::width);
  SET(psHeight,     display::height);
  SET(psPages,      1);

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
  TRACE("rspSetVideoMode(%i, %i, %i, %i, %i, %i, %i)", sDeviceDepth, sDeviceWidth, sDeviceHeight, sWidth, sHeight, sPages, sPixelDoubling);
  ASSERT(sDeviceDepth == display::depth);
  //ASSERT(sDeviceWidth == 0);
  //ASSERT(sDeviceHeight == 0);
  ASSERT(sWidth == display::width);
  ASSERT(sHeight == display::height);

  if (sPixelDoubling)
  {
    fprintf(stderr, "STUBBED: pixel doubling? %s:%d\n", __FILE__, __LINE__);
    return FAILURE;
  }

  return SUCCESS;
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
  // no-op
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

void draw_back(void) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;

    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_PAL8BPP, display::width, display::height, display::texture, PVR_FILTER_NONE);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;

    vert.x = 1;
    vert.y = 1;
    vert.z = 1;
    vert.u = 0.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = display::width;
    vert.y = 1;
    vert.z = 1;
    vert.u = 1.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 1;
    vert.y = display::height;
    vert.z = 1;
    vert.u = 0.0;
    vert.v = 1.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = display::width;
    vert.y = display::height;
    vert.z = 1;
    vert.u = 1.0;
    vert.v = 1.0;
    vert.flags = PVR_CMD_VERTEX_EOL;
    pvr_prim(&vert, sizeof(vert));
}

extern void rspPresentFrame(void)
{
  pvr_wait_ready();
  pvr_scene_begin();

  pvr_list_begin(PVR_LIST_OP_POLY);
  draw_back();
  pvr_list_finish();

  pvr_list_begin(PVR_LIST_TR_POLY);

  pvr_list_finish();
  pvr_scene_finish();
}

extern void rspUpdateDisplay(void)
{
  ASSERT(pvr_txr_load_dma(display::framebuffer, display::texture, display::width * display::height, TRUE, nullptr, nullptr) == SUCCESS);
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
    *plPitch = display::width;

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
    palindex_t sStartIndex,                // In: Palette entry to start copying to (has no effect on source!)
    palindex_t sCount,                     // In: Number of palette entries to do
    channel_t* pucRed,                    // In: Pointer to first red component to copy from
    channel_t* pucGreen,                  // In: Pointer to first green component to copy from
    channel_t* pucBlue,                   // In: Pointer to first blue component to copy from
    uint32_t lIncBytes)                  // In: Number of bytes by which to increment pointers after each copy
{
  uint8_t* psLock;
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
    palindex_t sEntry,                     // Palette entry (0x00 to 0xFF)
    channel_t ucRed,                      // Red component (0x00 to 0xFF)
    channel_t ucGreen,                    // Green component (0x00 to 0xFF)
    channel_t ucBlue)                     // Blue component (0x00 to 0xFF)
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
    palindex_t sEntry,                       // Palette entry (0x00 to 0xFF)
    channel_t* psRed,                       // Red component (0x00 to 0xFF) returned if not nullptr.
    channel_t* psGreen,                     // Green component (0x00 to 0xFF) returned if not nullptr.
    channel_t* psBlue)                      // Blue component (0x00 to 0xFF) returned if not nullptr.
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
    palindex_t sStartIndex,                  // Palette entry to start copying from
    palindex_t sCount,                       // Number of palette entries to do
    channel_t* pucRed,                      // Pointer to first red component to copy to
    channel_t* pucGreen,                    // Pointer to first green component to copy to
    channel_t* pucBlue,                     // Pointer to first blue component to copy to
    uint32_t lIncBytes)                    // Number of bytes by which to increment pointers after each copy
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
  memcpy(reinterpret_cast<void*>(PVR_PALETTE_TABLE_BASE),
         reinterpret_cast<void*>(palette::buffer),
         sizeof(color32_t) * palette::size);
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
    palindex_t sStartIndex,                  // Map entry to start copying to (has no effect on source!)
    palindex_t sCount,                       // Number of map entries to do
    channel_t* pucRed,                      // Pointer to first red component to copy from
    channel_t* pucGreen,                    // Pointer to first green component to copy from
    channel_t* pucBlue,                     // Pointer to first blue component to copy from
    uint32_t lIncBytes)                    // Number of bytes by which to increment pointers after each copy
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
    palindex_t sStartIndex,                  // Map entry to start copying from (has no effect on dest!)
    palindex_t sCount,                       // Number of map entries to do
    channel_t* pucRed,                      // Pointer to first red component to copy to
    channel_t* pucGreen,                    // Pointer to first green component to copy to
    channel_t* pucBlue,                     // Pointer to first blue component to copy to
    uint32_t lIncBytes)                    // Number of bytes by which to increment pointers after each copy
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
  for(uint8_t* psLock = palette::locks + sStartIndex; sCount-- > 0; ++psLock)
    *psLock = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Unlock several palette entries previously locked by rspLockPaletteEntries().
///////////////////////////////////////////////////////////////////////////////
extern void rspUnlockPaletteEntries(
    palindex_t    sStartIndex,               // Palette entry at which to start locking.
    palindex_t    sCount)                    // Number of palette entries to lock.
{
  for(uint8_t* psLock = palette::locks + sStartIndex; sCount-- > 0; ++psLock)
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
