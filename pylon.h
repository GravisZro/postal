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
// pylon.h
// Project: Nostril (aka Postal)
//
//	History:
//
//		05/01/97 BRH	Started this object from the bouys.  It will take over
//							the duty of logic suggestions and markers and the bouys
//							will go back to strictly navigation.
//
//		06/17/97 MJR	Moved some vars that were CPylon statics into the realm
//							so they could be instantiated on a realm-by-realm basis.
//
//		06/30/97	JMI	Moved EditRect() and EditHotSpot() from pylon.h to 
//							pylon.cpp.
//
//		07/08/97	JMI	Now removes its smash from the smashatorium in the 
//							destructor.
//
//		07/14/97	JMI	Now memsets m_msg to 0s before initializing the few members
//							that can be accessed via msg_Generic type.
//
//		07/17/97 BRH	Chagned Triggeed function to trigger only if the dude
//							on the traget area is alive.
//
//		07/21/97	JMI	Added GetX(), GetY(), and GetZ().	
//
////////////////////////////////////////////////////////////////////////////////
#ifndef PYLON_H
#define PYLON_H

#include <newpix/collisiondetection.h>

#define PYLON_MAX_PYLONS 254

// CPylon is the class for navigation
class CPylon
    : public Collidable,
      public CSprite2
	{
	friend class CNavigationNet;
	//---------------------------------------------------------------------------
	// Types, enums, etc.
	//---------------------------------------------------------------------------
	public:

	//---------------------------------------------------------------------------
	// Variables
	//---------------------------------------------------------------------------
	public:
		uint8_t	m_ucID;								// Pylon ID
		GameMessage m_msg;						// Place for storing hint messages
		uint16_t	m_u16TargetDudeID;				// ID of dude you are supposed to attack;

   protected:
		CSmash	m_smash;							// Collision region

		int16_t m_sSuspend;							// Suspend flag

		// Tracks file counter so we know when to load/save "common" data 
		static int16_t ms_sFileCount;

		// "Constant" values that we want to be able to tune using the editor

	//---------------------------------------------------------------------------
	// Constructor(s) / destructor
	//---------------------------------------------------------------------------
   public:
      CPylon(void);
      virtual ~CPylon(void);

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

		// Give Edit a rectangle around this object
		void EditRect(RRect* pRect);

		// Called by editor to get the hotspot of an object in 2D.
		void EditHotSpot(			// Returns nothiing.
			int16_t*	psX,			// Out: X coord of 2D hotspot relative to
										// EditRect() pos.
			int16_t*	psY);			// Out: Y coord of 2D hotspot relative to
										// EditRect() pos.
#endif // !defined(EDITOR_REMOVED)

		// Search the list of pylons and return a pointer to the one with the given ID
      managed_ptr<CPylon> GetPylon(uint8_t ucPylonID);

		// Search the list of pylons and return the instance ID of the one with
		// the given pylon id.
		uint16_t GetPylonUniqueID(uint8_t ucPylonID);

		// Return true if the pylon was triggered in the last interation
      bool Triggered(void);


	//---------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------
	protected:
		// Get all required resources
		int16_t GetResources(void);						// Returns 0 if successfull, non-zero otherwise
		
		// Free all resources
		int16_t FreeResources(void);						// Returns 0 if successfull, non-zero otherwise

		// GetFreePylonID - get the next ID that is not in use
		uint8_t GetFreePylonID(void);
		
		// Process Messages - look for DudeTrigger message
		void ProcessMessages(void);

	};


#endif //PYLON_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
