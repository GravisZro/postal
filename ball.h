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
// ball.h
// Project: Nostril (aka Postal)
//
//	History:
//	
//		01/19/97	JMI	Added some initializations to constructor.
//
//		01/27/97	JMI	Added override for EditRect() to make this object clickable.
//
//		02/02/97	JMI	Added EditHotSpot() override.
//
//		02/07/97	JMI	Added members for 3D.
//
//		02/07/97	JMI	Removed m_pipeline and associated members and setup since
//							the CScene now owns the pipeline.
//
//		02/13/97	JMI	Changing RForm3d to RSop.
//
//		02/23/97	JMI	Brought up to date so we can continue to use this as a
//							test object.
//
//		02/26/97	JMI	Now sets m_sprite.m_pthing = this on construction.
//
//		03/13/97	JMI	Load now takes a version number.
//
//		07/21/97	JMI	Added GetX(), GetY(), and GetZ().	
//
////////////////////////////////////////////////////////////////////////////////
#ifndef BALL_H
#define BALL_H

#include "thing.h"
#include <newpix/sprite_base.h>
#include "Anim3D.h"

// This is a sample game object
class CBall
    : public sprite_base_t,
      public CSprite3
	{
	//---------------------------------------------------------------------------
	// Types, enums, etc.
	//---------------------------------------------------------------------------
	protected:

	//---------------------------------------------------------------------------
	// Variables
	//---------------------------------------------------------------------------
	protected:
		double m_dDX;
		double m_dDY;
		double m_dDZ;
		int16_t m_sPrevHeight;
		int16_t m_sSuspend;

      int32_t	m_lPrevTime;

		CAnim3D		m_anim;		// 3D animation.

		RTransform	m_trans;		// Current transformation.

		int16_t			m_sCurRadius;	// Objects radius (currently fudged).
		

		// Tracks file counter so we know when to load/save "common" data 
		static int16_t ms_sFileCount;

	//---------------------------------------------------------------------------
	// Constructor(s) / destructor
   //---------------------------------------------------------------------------
    public:
      CBall(void);
      virtual ~CBall(void);

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

		// Called by editor to update object
		void EditUpdate(void);

		// Called by editor to render object
		void EditRender(void);

		// Called by editor to get the clickable pos/area of an object.
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
#endif // !defined(EDITOR_REMOVED)

	//---------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------
	protected:
		// Get all required resources
		int16_t GetResources(void);								// Returns 0 if successfull, non-zero otherwise
		
		// Free all resources
		int16_t FreeResources(void);								// Returns 0 if successfull, non-zero otherwise
	};


#endif //BALL_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
