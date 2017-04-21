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
//	bdisp.cpp
// 
// History:
//		06/04/04 RCG	Started.
//
//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////

// RSPix /////////////////////////////////////////////////////////////////////
#include <BLUE/Blue.h>
#include <ORANGE/CDT/slist.h>

// Platform //////////////////////////////////////////////////////////////////
#include <SDL2/SDL.h>

// C++ ///////////////////////////////////////////////////////////////////////
#include <cctype>
#include <cstddef>
#include "sharedarray.h"

extern SDL_Window *sdlWindow;
static const char *sdlAppName;
static SDL_Renderer *sdlRenderer;
static SDL_Texture *sdlTexture;
static int RequestedWidth = 0;
static int RequestedHeight = 0;
static int FramebufferWidth = 0;
static int FramebufferHeight = 0;
static shared_arr<color32_t> TexturePointer;
static shared_arr<uint8_t> PalettedTexturePointer;

typedef struct		// Stores information on usable video modes.
	{
	int16_t				sWidth;
	int16_t				sHeight;
	int16_t				sColorDepth;
	int16_t				sPages;
   } video_mode_t;

static RSList<video_mode_t, int16_t>	slvmModes;	// List of available video modes.


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

extern bool mouse_grabbed;

//////////////////////////////////////////////////////////////////////////////
// Module specific macros.
//////////////////////////////////////////////////////////////////////////////

// Only set value if not nullptr.
#define SET(ptr, val)        if((ptr) != nullptr) { *(ptr) = (val); }


int16_t CompareModes(video_mode_t* pvm1, video_mode_t* pvm2);

extern void Disp_Init(void)	// Returns nothing.
{
    extern char **_argv;
    const int arg = rspCommandLine("resolution");
    if ((arg) && (_argv[arg+1]))
    {
        if (SDL_sscanf(_argv[arg+1], "%dx%d", &RequestedWidth, &RequestedHeight) != 2)
            RequestedWidth = RequestedHeight = 0;
    }

    if (RequestedWidth <= 0 || RequestedHeight <= 0)
    {
        if (rspCommandLine("windowed"))
        {
            RequestedWidth = 1024;
            RequestedHeight = 768;
        }
        else
        {
            RequestedWidth = 0;
            RequestedHeight = 0;
        }
    }

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

	slvmModes.SetCompareFunc(CompareModes);
}

extern void rspSetApplicationName(
   const char* pszName)								// In: Application name
{
  sdlAppName = pszName;
  if (sdlWindow)
    SDL_SetWindowTitle(sdlWindow, sdlAppName);
}


//////////////////////////////////////////////////////////////////////////////
//
// Compares two video modes in order of sColorDepth, sWidth, sHeight,
// sPageFlippage.
// Returns 0            if *pvm1 == *pvm2.
// Returns negative    if *pvm1 < *pvm2.
// Returns positive    if *pvm1 > *pvm2.
//
//////////////////////////////////////////////////////////////////////////////
extern int16_t CompareModes(            // Returns as described above.
        video_mode_t* pvm1,               // First video mode to compare.
        video_mode_t* pvm2)               // Second video mode to compare.
{
  int16_t sReturn = 1;    // Assume *pvm1 > *pvm2.

  if (pvm1->sColorDepth == pvm2->sColorDepth)
  {
    if (pvm1->sWidth == pvm2->sWidth)
    {
      if (pvm1->sHeight == pvm2->sHeight)
      {
        if (pvm1->sPages == pvm2->sPages)
        {
          sReturn = 0;
        }
        else
        {
          if (pvm1->sPages < pvm2->sPages)
          {
            sReturn = -1;
          }
        }
      }
      else
      {
        if (pvm1->sHeight < pvm2->sHeight)
        {
          sReturn = -1;
        }
      }
    }
    else
    {
      if (pvm1->sWidth < pvm2->sWidth)
      {
        sReturn = -1;
      }
    }
  }
  else
  {
    if (pvm1->sColorDepth < pvm2->sColorDepth)
    {
      sReturn = -1;
    }
  }

  return sReturn;
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
extern int16_t rspSuggestVideoMode(		// Returns 0 if successfull, non-zero otherwise
	int16_t		sDepth,							// In:  Required depth
	int16_t		sWidth,							// In:  Requested width
	int16_t		sHeight,							// In:  Requested height
	int16_t		sPages,							// In:  Required pages
	int16_t		sScaling,						// In:  Requested scaling
	int16_t*	psDeviceWidth /*= nullptr*/,	// Out: Suggested device width (unless nullptr)
	int16_t*	psDeviceHeight /*= nullptr*/,	// Out: Suggested device height (unless nullptr)
	int16_t*	psScaling /*= nullptr*/)		// Out: Suggested scaling (unless nullptr)
	{
	int16_t sResult = SUCCESS;	// Assume success.

	// Store video mode that the app is currently iterating.
//	video_mode_t*	pvmOldModeQuery	= slvmModes.GetCurrent();

	rspQueryVideoModeReset();

	// Query results.
	int16_t	sModeWidth;
	int16_t	sModeHeight;
	int16_t sModeColorDepth;
	int16_t	sModePages;

	// Best results.
   int16_t sBestModeWidth		= INT16_MAX;
   int16_t	sBestModeHeight	= INT16_MAX;
	int16_t	sModeFound			= FALSE;

	while (rspQueryVideoMode(&sModeColorDepth, &sModeWidth, &sModeHeight, &sModePages) == SUCCESS)
		{
		// Must be same color depth.
		if (sModeColorDepth == sDepth && sPages == sModePages)
			{
			// If the desired resolution would fit into this mode . . .
			if (sWidth <= sModeWidth && sHeight <= sModeHeight)
				{
				// If this mode is closer than a previous one . . .
				float	fFactorOld	= ((float)sBestModeWidth	* (float)sBestModeHeight)
										/ ((float)sWidth				* (float)sHeight);
				float	fFactorNew	= ((float)sModeWidth			* (float)sModeHeight)
										/ ((float)sWidth				* (float)sHeight);
				if (fFactorNew < fFactorOld)
					{
					sBestModeWidth		= sModeWidth;
					sBestModeHeight	= sModeHeight;
					sModeFound			= TRUE;
					}
				}
			}
		}

	// If we found an acceptable mode . . .
	if (sModeFound != FALSE)
		{
		// If pixel doubling was specified . . .
		if (psScaling != nullptr)
			{
			// If pixel doubling is allowed . . .
			if (sScaling != FALSE)
				{
				// If the chosen mode is more than or equal to twice the 
				// requested mode . . . 
				if (sWidth * 2 <= sBestModeWidth 
					&& sHeight * 2 <= sBestModeHeight)
					{
					// Okay to pixel double.  Leave *psPixelDoubling as TRUE.
					// Reduce best width and height appropriately.
					sBestModeWidth		/= 2;
					sBestModeHeight	/= 2;
					}
				else
					{
					// No pixel doubling possible for this mode.
					*psScaling	= FALSE;
					}
				}
			}

		*psDeviceWidth		= sBestModeWidth;
		*psDeviceHeight	= sBestModeHeight;
		}
	else
		{
		// Failed to find an acceptable mode.
		sResult = FAILURE;
		}

	return sResult;
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
	int16_t*	psDeviceDepth,				// Hardware display color depth returned here 
												// (unless nullptr).
	int16_t*	psDeviceWidth,				// Hardware display width returned here 
												// (unless nullptr).
	int16_t*	psDeviceHeight,			// Hardware display height returned here 
												// (unless nullptr).
	int16_t*	psDevicePages,				// Hardware display back buffers returned here
												// (unless nullptr).
	int16_t*	psWidth,						// Display area width returned here 
												// (unless nullptr).
	int16_t*	psHeight,					// Display area height returned here
												// (unless nullptr).
	int16_t*	psPages/*= nullptr*/,			// Number of pages (1 to n) returned here 
												// (unless nullptr).  More than 1 indicates a 
												// page flipping scenario.
	int16_t*	psPixelScaling/*= nullptr*/)	// Pixel scaling in effect (1) or not (0)
													// (unless nullptr).
{
    // lie about everything.
    SET(psPixelScaling, 0);
    SET(psDevicePages, 0);
    SET(psPages, 1);
    SET(psWidth, FramebufferWidth);
    SET(psHeight, FramebufferHeight);
    SET(psDeviceDepth, 8);
    SET(psDeviceHeight, FramebufferWidth);
    SET(psDeviceWidth, FramebufferHeight);

    return SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//
// Reset the query process.  Must be called before calling QueryVideoModes()
// for the first time.  See QueryVideoModes() for more.
//
//////////////////////////////////////////////////////////////////////////////

static void addMode(int w, int h, int depth)
{
    video_mode_t* pvm;

    if (depth >= 8)
    {
        pvm = new video_mode_t;
        pvm->sWidth = w;
        pvm->sHeight = h;
        pvm->sColorDepth = 8;
        pvm->sPages = 1;
        slvmModes.Insert(pvm);
    }

    if (depth >= 16)
    {
        pvm = new video_mode_t;
        pvm->sWidth = w;
        pvm->sHeight = h;
        pvm->sColorDepth = 16;
        pvm->sPages = 1;
        slvmModes.Insert(pvm);
    }

    if (depth >= 24)
    {
        pvm = new video_mode_t;
        pvm->sWidth = w;
        pvm->sHeight = h;
        pvm->sColorDepth = 32;
        pvm->sPages = 1;
        slvmModes.Insert(pvm);
    }
}

extern void rspQueryVideoModeReset(void)
{
    static bool enumerated = false;
    if (!enumerated)
    {
        ASSERT(SDL_WasInit(SDL_INIT_VIDEO));

		// Attempt to grab user's current desktop resolution instead of forcing 640x480
#ifndef MOBILE
		SDL_DisplayMode dm_Mode;
		int i_Result = SDL_GetDesktopDisplayMode(0, &dm_Mode);
		if (!i_Result)
			addMode(dm_Mode.w, dm_Mode.h, 8);
		else // Fall back to 640x480
#endif
			addMode(640, 480, 8);

        enumerated = true;
    }

	slvmModes.GetHead();
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
extern int16_t rspQueryVideoMode(			// Returns 0 for each valid mode, then non-zero thereafter
	int16_t* psColorDepth,						// Color depth (8, 15, 16, 24, 32)
													// Unless nullptr.
	int16_t* psWidth /*= nullptr*/,				// Width returned here
													// Unless nullptr.
	int16_t* psHeight /*= nullptr*/,			// Height returned here
													// Unless nullptr.
	int16_t* psPages /*= nullptr*/)				// Number of video pages possible.
	{
	int16_t sResult = SUCCESS;	// Assume success.

   video_mode_t*	pvm	= slvmModes.GetCurrent();

	if (pvm != nullptr)
		{
		SET(psColorDepth,	pvm->sColorDepth);
		SET(psWidth,			pvm->sWidth);
		SET(psHeight,			pvm->sHeight);

		SET(psPages,			pvm->sPages);

		// Goto next video mode.
		slvmModes.GetNext();
		}
	else
		{
		sResult = 1;
		}

	return sResult;
	}


static SDL_Renderer *createRendererToggleVsync(SDL_Window *window, const int index, bool vsync)
{
    SDL_Renderer *retval = nullptr;
    if (vsync)
        retval = SDL_CreateRenderer(window, index, SDL_RENDERER_PRESENTVSYNC);
    if (!retval)
        retval = SDL_CreateRenderer(window, index, 0);
    return retval;
}

static SDL_Renderer *createRendererByName(SDL_Window *window, const char *name)
{
    const bool vsync = !rspCommandLine("novsync");
    if (name == nullptr)
        return createRendererToggleVsync(window, -1, vsync);
    else
    {
        const int max = SDL_GetNumRenderDrivers();
        for (int i = 0; i < max; i++)
        {
            SDL_RendererInfo info;
            if ((SDL_GetRenderDriverInfo(i, &info) == SUCCESS) && (SDL_strcmp(info.name, name) == SUCCESS))
                return createRendererToggleVsync(window, i, vsync);
        }
    }
    return nullptr;
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
extern int16_t rspSetVideoMode(	// Returns 0 if successfull, non-zero otherwise
	int16_t	sDeviceDepth,			// Specify required device video depth here.
	int16_t	sDeviceWidth,			// Specify required device resolution width here.
	int16_t	sDeviceHeight,			// Specify required device resolution height here.
	int16_t sWidth,					// Specify width of display area on screen.
	int16_t sHeight,					// Specify height of display area on screen.
	int16_t	sPages /*= 1*/,		// Specify number of video pages.  More than 1
										// indicates a page flipping scenario.
	int16_t	sPixelDoubling	/*= FALSE*/)
										// TRUE indicates to set the video mode
										// to twice that indicated by sDeviceWidth,
										// sDeviceHeight and double the coordinate
										// system and blts.
										// FALSE indicates not to use this garbage.
	{
		TRACE("rspSetVideoMode(%i, %i, %i, %i, %i, %i, %i)\n", sDeviceDepth, sDeviceWidth, sDeviceHeight, sWidth, sHeight, sPages, sPixelDoubling);
        ASSERT(sDeviceDepth == 8);
        //ASSERT(sDeviceWidth == 0);
        //ASSERT(sDeviceHeight == 0);
        //ASSERT(sWidth == 640);
        ASSERT(sHeight == 480);

        for (uint16_t i = 0; i < palette::size; i++)
            palette::buffer[i].alpha= 0xFF;

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

        if (sPixelDoubling)
        {
            fprintf(stderr, "STUBBED: pixel doubling? %s:%d\n", __FILE__, __LINE__);
            return FAILURE;
        }

        FramebufferWidth = sWidth;
        FramebufferHeight = sHeight;

        mouse_grabbed = !rspCommandLine("nomousegrab");

        uint32_t flags = 0;
        if (!rspCommandLine("windowed"))
        {
            if ((!RequestedWidth) || (!RequestedHeight))
                flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
            else
                flags |= SDL_WINDOW_FULLSCREEN;
        }

        if (mouse_grabbed)
            flags |= SDL_WINDOW_INPUT_GRABBED;

#if PLATFORM_IOS
        flags |= SDL_WINDOW_BORDERLESS;   // don't show the status bar
#endif
        //TRACE("RequestedWidth %d   RequestedHeight %d\n",RequestedWidth,RequestedHeight);

        if(sdlWindow)
          SDL_DestroyWindow(sdlWindow);

        const char *title = sdlAppName ? sdlAppName : "";
        sdlWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, RequestedWidth, RequestedHeight, flags);
        if (!sdlWindow)
        {
            char buf[128];
            SDL_snprintf(buf, sizeof (buf), "Couldn't create window: %s.", SDL_GetError());
            fprintf(stderr, "POSTAL: %s\n", buf);
            SDL_Quit();
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "POSTAL", buf, nullptr);
            exit(1);
        }
		int w = 0, h = 0;
		SDL_GetWindowSize(sdlWindow, &w, &h);
		TRACE("SDL Window initialized at %ix%i\n", w, h);

        bool bRequestedRenderer = true;
        if (rspCommandLine("direct3d"))
            sdlRenderer = createRendererByName(sdlWindow, "direct3d");
        else if (rspCommandLine("opengl"))
            sdlRenderer = createRendererByName(sdlWindow, "opengl");
        else if (rspCommandLine("software"))
            sdlRenderer = createRendererByName(sdlWindow, "software");
        else
        {
            bRequestedRenderer = false;
            sdlRenderer = createRendererByName(sdlWindow, nullptr);
        }

        if (!sdlRenderer)
        {
            char buf[128];
            SDL_snprintf(buf, sizeof (buf), "Couldn't create %s renderer: %s", bRequestedRenderer ? "requested" : "a", SDL_GetError());
            fprintf(stderr, "POSTAL: %s\n", buf);
            SDL_DestroyWindow(sdlWindow);
            sdlWindow = nullptr;
            SDL_Quit();
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "POSTAL", buf, nullptr);
            exit(1);
        }

        SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 0xFF);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderPresent(sdlRenderer);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderPresent(sdlRenderer);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderPresent(sdlRenderer);
#ifndef MOBILE //Need to remove this for the mouse point to be in the correct place, Android And IOS
        SDL_RenderSetLogicalSize(sdlRenderer, FramebufferWidth, FramebufferHeight);
		TRACE("SDL Renderer set: %ix%i\n", FramebufferWidth, FramebufferHeight);
#endif
        sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, FramebufferWidth, FramebufferHeight);
        if (!sdlTexture)
        {
            char buf[128];
            SDL_snprintf(buf, sizeof (buf), "Couldn't create texture: %s", SDL_GetError());
            fprintf(stderr, "POSTAL: %s\n", buf);
            SDL_DestroyRenderer(sdlRenderer);
            sdlRenderer = nullptr;
            SDL_DestroyWindow(sdlWindow);
            sdlWindow = nullptr;
            SDL_Quit();
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "POSTAL", buf, nullptr);
            exit(1);
        }

        TexturePointer.allocate(FramebufferWidth * FramebufferHeight);
        PalettedTexturePointer.allocate(FramebufferWidth * FramebufferHeight);
        SDL_memset(TexturePointer, '\0', FramebufferWidth * FramebufferHeight * sizeof (color32_t));
        SDL_memset(PalettedTexturePointer, '\0', FramebufferWidth * FramebufferHeight * sizeof (uint8_t));
        SDL_UpdateTexture(sdlTexture, nullptr, TexturePointer, FramebufferWidth * 4);

      SDL_ShowCursor(0);
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
    /* no-op ... SDL_Quit() will catch this. */
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
    /* no-op ... SDL_Quit() will catch this. */
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
	int16_t sX,					// x coord of upper-left corner of area to update
	int16_t sY,					// y coord of upper-left corner of area to update
	int16_t sWidth,				// Width of area to update
	int16_t sHeight)				// Height of area to update
{
  UNUSED(sX,sY,sWidth,sHeight);
}

extern void rspPresentFrame(void)
{
    if (!sdlWindow) return;

    // !!! FIXME: I imagine this is not fast. Maybe keep the dirty rect code at least?
    static_assert(sizeof(color32_t) == sizeof (uint32_t), "broken compiler!");
    const uint8_t *src = PalettedTexturePointer;
    color32_t *dst = TexturePointer;
    for (int y = 0; y < FramebufferHeight; y++)
    {
        for (int x = 0; x < FramebufferWidth; x++, src++, dst++)
            *dst = palette::buffer[*src];
        }

    SDL_UpdateTexture(sdlTexture, nullptr, TexturePointer, FramebufferWidth * 4);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, nullptr);
    SDL_RenderPresent(sdlRenderer);  // off to the screen with you.

    static uint32_t lastframeticks = 0;
    const uint32_t now = SDL_GetTicks();

    if ((lastframeticks) && (lastframeticks <= now))
    {
        const uint32_t elapsed = (now - lastframeticks);
        if (elapsed <= 5)  // going WAY too fast, maybe OpenGL (and/or no vsync)?
            SDL_Delay(16 - elapsed);  // try to get closer to 60fps.
    }

    lastframeticks = now;

    #if 0
    static uint32_t ticks = 0;
    static uint32_t frames = 0;
    frames++;
    if ((now - ticks) > 5000)
    {
        if (ticks > 0)
            TRACE("fps: %f\n", (((double) frames) / ((double) (now - ticks))) * 1000.0);
        ticks = now;
        frames = 0;
    }
    #endif
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
	int16_t sX,					// x coord of upper-left corner of area to update
	int16_t sY,					// y coord of upper-left corner of area to update
	int16_t sWidth,				// Width of area to update
	int16_t sHeight)				// Height of area to update
{
  UNUSED(sX,sY,sWidth,sHeight);
}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspLockVideoPage(	// Returns 0 if screen memory could be locked.
											// Returns non-zero otherwise.
	void**	ppvMemory,				// Pointer to display memory returned here.
											// nullptr returned if not supported.
	int32_t*		plPitch)					// Pitch of display memory returned here.
	{
  UNUSED(ppvMemory,plPitch);
	/* no-op. */
   return FAILURE;
	}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern void rspUnlockVideoPage(void)	// Returns nothing.
	{
	/* no-op. */
	}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspLockVideoFlipPage(	// Returns 0 if flip screen memory could be 
											// locked.  Returns non-zero otherwise.
	void**	ppvMemory,				// Pointer to flip screen memory returned here.
											// nullptr returned on failure.
	int32_t*		plPitch)					// Pitch of flip screen memory returned here.
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
extern void rspUnlockVideoFlipPage(void)	// Returns nothing.
	{
	}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspLockVideoBuffer(	// Returns 0 if system buffer could be locked.
												// Returns non-zero otherwise.
	void**	ppvBuffer,					// Pointer to system buffer returned here.
												// nullptr returned on failure.
	int32_t*		plPitch)						// Pitch of system buffer returned here.
	{
    if (!sdlWindow)
        return FAILURE;

    *ppvBuffer = PalettedTexturePointer;
    *plPitch = FramebufferWidth;

    return(0);
	}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern void rspUnlockVideoBuffer(void)	// Returns nothing.
	{
	}

///////////////////////////////////////////////////////////////////////////////
//
// Maps calls to API that matches display type.
// See function comment in appropriate CPP (BGDisp/BXDisp).
//
///////////////////////////////////////////////////////////////////////////////
extern int16_t rspAllowPageFlip(void)	// Returns 0 on success.
	{
	return SUCCESS;
	}

//////////////////////////////////////////////////////////////////////////////
//	External Palette module functions.
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// Set several palette entries.  Separate pointers to each component
// combined with caller-specified increment for all pointers allows
// use of this function with any (non-packed) arrangement of RGB data.
// Hardware palette is not updated until UpdatePalette() is called.
//
///////////////////////////////////////////////////////////////////////////////
/*
extern void rspSetPaletteEntries(
   palindex_t sStartIndex,			// Palette entry to start copying to (has no effect on source!)
   palindex_t sCount,					// Number of palette entries to do
   channel_t* pucRed,		// Pointer to first red component to copy from
   channel_t* pucGreen,	// Pointer to first green component to copy from
   channel_t* pucBlue,		// Pointer to first blue component to copy from
   uint32_t lIncBytes)				// Number of bytes by which to increment pointers after each copy
{
  static int i = 0;
  TRACE("instance: %i\n", ++i);
  // Set up destination pointers.
  color32_t* pColor = palette::buffer + sStartIndex;

  // Set up lock pointer.
  int8_t* psLock = palette::locks + sStartIndex;

  while (sCount-- > 0)
  {
    if (*psLock++ == 0)
    {
      pColor->red		= *pucRed;
      pColor->green	= *pucGreen;
      pColor->blue	= *pucBlue;
      TRACE("color #%i - %i: %02x %02x %02x\n", sStartIndex, sCount, pColor->red, pColor->green, pColor->blue);
    }

    // Increment source.
    pucRed	+= lIncBytes;
    pucGreen += lIncBytes;
    pucBlue	+= lIncBytes;

    // Increment destination.
    ++pColor;
  }
}
*/
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
   palindex_t sEntry,				// Palette entry (0x00 to 0xFF)
   channel_t ucRed,				   // Red component (0x00 to 0xFF)
   channel_t ucGreen,				// Green component (0x00 to 0xFF)
   channel_t ucBlue)				// Blue component (0x00 to 0xFF)
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
   palindex_t sEntry,				// Palette entry (0x00 to 0xFF)
   channel_t* psRed,				// Red component (0x00 to 0xFF) returned if not nullptr.
   channel_t* psGreen,			// Green component (0x00 to 0xFF) returned if not nullptr.
   channel_t* psBlue)				// Blue component (0x00 to 0xFF) returned if not nullptr.
{
  ASSERT(sEntry < palette::size);

  SET(psRed,   palette::buffer[sEntry].red);
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
   palindex_t sStartIndex,			// Palette entry to start copying from
   palindex_t sCount,					// Number of palette entries to do
   channel_t* pucRed,		// Pointer to first red component to copy to
   channel_t* pucGreen,	// Pointer to first green component to copy to
   channel_t* pucBlue,		// Pointer to first blue component to copy to
   uint32_t lIncBytes)				// Number of bytes by which to increment pointers after each copy
{
  for(color32_t* pColor = palette::buffer + sStartIndex;
      sCount-- > 0;
      pucRed    += lIncBytes,
      pucGreen  += lIncBytes,
      pucBlue   += lIncBytes,
      ++pColor)
  {
    *pucRed   = pColor->red;
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
   palindex_t sStartIndex,			// Map entry to start copying to (has no effect on source!)
   palindex_t sCount,					// Number of map entries to do
   channel_t* pucRed,		// Pointer to first red component to copy from
   channel_t* pucGreen,	// Pointer to first green component to copy from
   channel_t* pucBlue,		// Pointer to first blue component to copy from
   uint32_t lIncBytes)				// Number of bytes by which to increment pointers after each copy
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
   palindex_t sStartIndex,			// Map entry to start copying from (has no effect on dest!)
   palindex_t sCount,					// Number of map entries to do
   channel_t* pucRed,		// Pointer to first red component to copy to
   channel_t* pucGreen,	// Pointer to first green component to copy to
   channel_t* pucBlue,		// Pointer to first blue component to copy to
   uint32_t lIncBytes)				// Number of bytes by which to increment pointers after each copy
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
   palindex_t sStartIndex,			// Palette entry at which to start locking.
   palindex_t sCount)					// Number of palette entries to lock.
{
  for(int8_t* psLock = palette::locks + sStartIndex; sCount-- > 0; ++psLock)
    *psLock = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Unlock several palette entries previously locked by rspLockPaletteEntries().
///////////////////////////////////////////////////////////////////////////////
extern void rspUnlockPaletteEntries(
   int16_t	sStartIndex,			// Palette entry at which to start locking.
   int16_t	sCount)					// Number of palette entries to lock.
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
//	External Background module functions.
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// Set a callback to be called when the application moves into the background.
//
///////////////////////////////////////////////////////////////////////////////
extern void rspSetBackgroundCallback(	// Returns nothing.
	void (BackgroundCall)(void))			// Callback when app processing becomes
													// background.  nullptr to clear.
{
  UNUSED(BackgroundCall);
  /* no-op. */
}

///////////////////////////////////////////////////////////////////////////////
//
// Set a callback to be called when the application moves into the foreground.
//
///////////////////////////////////////////////////////////////////////////////
extern void rspSetForegroundCallback(	// Returns nothing.
	void (ForegroundCall)(void))			// Callback when app processing becomes
													// foreground.  nullptr to clear.
{
  UNUSED(ForegroundCall);
  /* no-op. */
}

extern bool rspIsBackground(void)			// Returns TRUE if in background, FALSE otherwise
{
  extern bool GSDLAppIsActive;
  return (!GSDLAppIsActive);
}
//////////////////////////////////////////////////////////////////////////////
// EOF
//////////////////////////////////////////////////////////////////////////////
