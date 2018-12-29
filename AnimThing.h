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
// AnimThing.h
// Project: Nostril (aka Postal)
// 
// History:
//		02/18/97 JMI	Started.
//
//		02/19/97	JMI	Added ability to send any message to any CThing when done
//							with animation.
//
//		02/19/97	JMI	Unprotected more members.
//
//		02/24/97	JMI	Changed declaration of m_sprite from CAlphaSprite2 to 
//							CSprite2.
//
//		02/24/97	JMI	Changed m_pthingSendMsg to m_u16IdSendMsg.
//
//		02/26/97	JMI	Now sets m_sprite.m_pthing = this on construction.
//
//		03/13/97	JMI	Load now takes a version number.
//
//		06/24/97	JMI	Now intializes m_msg's priority to 0 on construction for
//							safety.
//
//		07/21/97	JMI	Added GetX(), GetY(), and GetZ().	
//
//////////////////////////////////////////////////////////////////////////////
//
// This CThing-derived class will play an animation to its finish and then
// destroy itself.  It's a mini Postal movie player.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef ANIMTHING_H
#define ANIMTHING_H

#include "thing.h"
#include <newpix/sprite_base.h>

#include "AlphaAnimType.h"

class CAnimThing
    : public sprite_base_t,
      public CSprite2
	{
	//---------------------------------------------------------------------------
   // Types, enums, etc.
	//---------------------------------------------------------------------------
	public:

		typedef RChannel<CAlphaAnim> ChannelAA;

	//---------------------------------------------------------------------------
	// Variables
	//---------------------------------------------------------------------------
   public:
		int16_t m_sSuspend;							// Suspend flag
		int16_t	m_sLoop;								// Loops, if true.
		char	m_szResName[PATH_MAX];		// Resource name.
														
      milliseconds_t	m_lAnimTime;						// Cummulative animation time.
      milliseconds_t	m_lAnimPrevTime;					// Last animation time.
														
      managed_ptr<CThing> m_sender;			// ID of CThing to send msg to when done.
		GameMessage	m_msg;						// Message to send to m_pthingSendMsg.

   protected:
		ChannelAA*	m_paachannel;				// Animation (with or without alpha).
														
	//---------------------------------------------------------------------------
	// Constructor(s) / destructor
	//---------------------------------------------------------------------------
   public:
      CAnimThing(void);
      virtual ~CAnimThing(void);

	//---------------------------------------------------------------------------
	// Required virtual functions (implimenting them as inlines doesn't pay!)
	//---------------------------------------------------------------------------
	public:
		// Load object (should call base class version!)
		int16_t Load(													// Returns 0 if successfull, non-zero otherwise
			RFile* pFile,											// In:  File to load from
			bool bEditMode,										// In:  True for edit mode, false otherwise
			int16_t sFileCount,										// In:  File count (unique per file, never 0)
			uint32_t	ulFileVersion);								// In:  Version of file format to load.

		// Save object (should call base class version!)
		int16_t Save(													// Returns 0 if successfull, non-zero otherwise
			RFile* pFile,											// In:  File to save to
			int16_t sFileCount);									// In:  File count (unique per file, never 0)

		// Startup object
      void Startup(void);										// Returns 0 if successfull, non-zero otherwise

		// Suspend object
		void Suspend(void);

		// Resume object
		void Resume(void);

		// Update object
		void Update(void);

		// Render object
		void Render(void);

		int16_t Setup(												// Returns 0 on success.
			int16_t sX,												// In: New x coord
			int16_t sY,												// In: New y coord
			int16_t sZ);												// In: New z coord

#if !defined(EDITOR_REMOVED)
		// Called by editor to init new object at specified position
		int16_t EditNew(												// Returns 0 if successfull, non-zero otherwise
			int16_t sX,												// In:  New x coord
			int16_t sY,												// In:  New y coord
			int16_t sZ);												// In:  New z coord

		// Called by editor to modify object
		int16_t EditModify(void);									// Returns 0 if successfull, non-zero otherwise

		// Called by editor to move object to specified position
		int16_t EditMove(											// Returns 0 if successfull, non-zero otherwise
			int16_t sX,												// In:  New x coord
			int16_t sY,												// In:  New y coord
			int16_t sZ);												// In:  New z coord

		// Called by editor to get the clickable pos/area of an object in 2D.
		virtual	// Overridden here.
		void EditRect(				// Returns nothiing.
			RRect*	prc);			// Out: Clickable pos/area of object.

		// Called by editor to get the hotspot of an object in 2D.
		virtual	// Overridden here.
		void EditHotSpot(			// Returns nothiing.
			int16_t*	psX,			// Out: X coord of 2D hotspot relative to
										// EditRect() pos.
			int16_t*	psY);			// Out: Y coord of 2D hotspot relative to
										// EditRect() pos.

		// Called by editor to update object
		void EditUpdate(void);

		// Called by editor to render object
		void EditRender(void);
#endif // !defined(EDITOR_REMOVED)

	//---------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------
	protected:
		// Get all required resources
		int16_t GetResources(void);						// Returns 0 if successfull, non-zero otherwise
		
		// Free all resources
		int16_t FreeResources(void);						// Returns 0 if successfull, non-zero otherwise
	};


#endif // ANIMTHING_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
