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
// firebomb.h
// Project: Nostril (aka Postal)
//
// History:
//		01/17/97 BRH	Started this weapon object.
//
//		02/26/97	JMI	Now sets m_sprite.m_pthing = this on construction.
//
//		02/28/97 BRH	Derived this from the CWeapon base class
//
//		03/03/97 BRH	Moved 3D sprite to CWeapon base class.
//
//		03/13/97	JMI	Load()s now take a version number.
//
//		04/29/97	JMI	Now CFirebomb defines m_sprite (as a CSprite3), which was 
//							previously defined in the base class CWeapon.
//							Also, added GetSprite() virtual override to CFirebomb and
//							CFireFrag to provide access to the sprite from a lower 
//							level.
//
//		05/09/97 BRH	Added SetRangeToTarget function to vary the velocity
//							of the weapon before it is shot in order to hit
//							your target.  
//
//		05/20/97 BRH	Fixed problem with SetRangeToTarget.
//
//		06/17/97 BRH	Fixed a bug in SetRangeToTarget and adjusted the
//							Min range up a bit so the enemies don't set themselves
//							on fire.
//
//		07/09/97	JMI	Changed Preload() to take a pointer to the calling realm
//							as a parameter.
//
//		08/28/97	JMI	Added a explode counter so we can cap the number of 
//							explosions a firefrag can make.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef FIREBOMB_H
#define FIREBOMB_H

#include "weapon.h"

#include "Anim3D.h"

// CFirebomb is hand thrown fire grenade weapon class
class CFirebomb
    : public CWeapon,
      public CSprite3
{
	protected:
		int16_t m_sPrevHeight;										// Previous height

		CAnim3D		m_anim;										// 3D animation
      RTransform	m_trans;										// Transform

		// Tracks file counter so we know when to load/save "common" data 
		static int16_t ms_sFileCount;

		// "Constant" values that we want to be able to tune using the editor
		static double ms_dCloseDistance;						// Close enough to hit CDude
		static double ms_dThrowVertVel;						// Throw up at this velocity
		static double ms_dThrowHorizVel;						// Throw out at this velocity

	//---------------------------------------------------------------------------
	// Constructor(s) / destructor
	//---------------------------------------------------------------------------
   public:
      CFirebomb(void);
      virtual ~CFirebomb(void);

	//---------------------------------------------------------------------------
	// Optional static functions
	//---------------------------------------------------------------------------

		// Called before play begins to cache the resource for this object
		static int16_t Preload(
			CRealm* prealm);				// In:  Calling realm.

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

		// Update object
		void Update(void);

		// Render object
		void Render(void);

		// Called by the object that is creating this weapon
		int16_t Setup(
			int16_t sX,												// In: New x coord
			int16_t sY,												// In: New y coord
			int16_t sZ);												// In: New z coord

		// Get this class's sprite.  Note that the type will vary.
		// This is a pure virtual functionin the base class.
		virtual			// Overriden here.
		CSprite* GetSprite(void)	// Returns this weapon's sprite.
			{
         return this;
			}

		// Function to modify the velocity for a requested range
		virtual int16_t SetRangeToTarget(int16_t sRequestedRange)
		{
			int16_t sSetRange;
			// Must go at least 60 or at most 400 pixels
			sSetRange = MAX(sRequestedRange, (int16_t) 60);
			sSetRange = MIN(sSetRange, (int16_t) 400);
			m_dHorizVel = (double) sSetRange / 0.8986; //0.7366;
			return sSetRange;
		}

	//---------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------
	protected:
		// Get all required resources
		int16_t GetResources(void);						// Returns 0 if successfull, non-zero otherwise
		
		// Free all resources
		int16_t FreeResources(void);						// Returns 0 if successfull, non-zero otherwise

};

class CFire;
// CFirefrag a fragment that comes out of the CFirebomb weapon
class CFirefrag
    : public CWeapon,
      public CSprite2
{
	//---------------------------------------------------------------------------
	// Types, enums, etc.
	//---------------------------------------------------------------------------
	public:

	//---------------------------------------------------------------------------
	// Variables
	//---------------------------------------------------------------------------
	public:

	protected:
		int16_t m_sPrevHeight;										// Previous height
      managed_ptr<CFire> m_fire;											// Pointer to controlled fire object
      int16_t	m_sNumExplosions;								// Total Number of explosions.

		// Tracks file counter so we know when to load/save "common" data 
		static int16_t ms_sFileCount;

		// "Constant" values that we want to be able to tune using the editor
		static double ms_dAccUser;								// Acceleration due to user
		static double ms_dAccDrag;								// Acceleration due to drag (always towards 0)

		static double ms_dGravity;								// Acceleration due to gravity
		static double ms_dThrowVertVel;						// Throw up at this velocity
		static double ms_dThrowHorizVel;						// Throw out at this velocity
		static double ms_dMinBounceVel;						// Minimum velocity needed to bounce up
		static double ms_dVelTransferFract;					// Amount of velocity to bounce back up
		static int16_t ms_sMaxExplosions;						// Maximum explosions before death.

	//---------------------------------------------------------------------------
	// Constructor(s) / destructor
	//---------------------------------------------------------------------------
   public:
      CFirefrag(void);
      virtual ~CFirefrag(void);

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

		// Update object
		void Update(void);


		// Called by the object that is creating this weapon
		int16_t Setup(
			int16_t sX,												// In: New x coord
			int16_t sY,												// In: New y coord
			int16_t sZ);												// In: New z coord

		// Get this class's sprite.  Note that the type will vary.
		// This is a pure virtual functionin the base class.
		virtual			// Overriden here.
		CSprite* GetSprite(void)	// Returns this weapon's sprite.
			{
         return this;
			}

	//---------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------
	protected:
		// Get all required resources
		int16_t GetResources(void);						// Returns 0 if successfull, non-zero otherwise
		
		// Free all resources
		int16_t FreeResources(void);						// Returns 0 if successfull, non-zero otherwise

};


#endif //FIREBOMB_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
