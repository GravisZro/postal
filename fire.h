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
// fire.h
// Project: Postal
// 
// History:
//		01/17/97 BRH	Started this weapon object.
//
//		02/24/97	JMI	Changed declaration of m_sprite from CAlphaSprite2 to 
//							CSprite2.
//
//		03/13/97	JMI	Load now takes a version number.
//
//		04/24/97 BRH	Added a static variable for wind direction that will
//							be changed slightly every time a smoke is created, but
//							will generally cause them all to drift in the same
//							direction.
//
//		05/02/97	JMI	Added GetTimeLeftToLive() which returns the amount of
//							time left before the fire goes out or the smoke thins
//							out.
//
//		06/11/97 BRH	Added m_shooter to store the shooter ID which
//							will get passed along in the Burn Message.
//
//		06/17/97	JMI	Converted all occurrences of rand() to GetRand() and
//							srand() to SeedRand().
//
//		07/01/97 BRH	Added Small smoke animation.
//
//		07/04/97 BRH	Added starting time used to calculate the alpha
//							level based on time to live.
//
//		07/09/97	JMI	Changed Preload() to take a pointer to the calling realm
//							as a parameter.
//
//		07/13/97 BRH	Added some variables and changed others for the new
//							method of using 1 alpha mask and setting the level
//							in the code.
//
//		07/23/97 BRH	Changed the limits on the wind velocity so it can
//							be higher.
//
//		09/02/97	JMI	Added m_fireStarter.  This is used for a special case
//							when the starter of the fire is not the thing using the
//							fire as a weapon (e.g., when a guy catches fire he can
//							use the fire on other people by running into them causing
//							them to catch on fire; however, if his own fire kills him
//							it is to the creator of the fire's credit that he dies).
//
//////////////////////////////////////////////////////////////////////////////
//
// Fire.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef FIRE_H
#define FIRE_H

#include <newpix/collisiondetection.h>

#include "AlphaAnimType.h"


class CThing3d;
// CFire is a burning flame weapon class
class CFire
    : public Collidable,
      public CSprite2
	{
	//---------------------------------------------------------------------------
	// Types, enums, etc.
	//---------------------------------------------------------------------------
	public:

	typedef enum
	{
		State_Idle,
		State_Fire,
		State_Find,
		State_Chase,
      State_Explode
	} CFireState;

	typedef uint8_t FireAnim;

   enum
	{
		LargeFire,
		SmallFire,
		Smoke,
		SmallSmoke
	};

	typedef RChannel<CAlphaAnim> ChannelAA;

	//---------------------------------------------------------------------------
	// Variables
	//---------------------------------------------------------------------------

	public:
		int16_t m_sRot;
		bool  m_bTurnRight;					// A Random number will determine if the 
													// smoke will curl left or right when it hits an
													// obstacle.
        bool m_bIsBurningDude;

      managed_ptr<CThing3d> m_shooter;				// Store the shooter ID to pass along in the burn message
      managed_ptr<CThing3d> m_fireStarter;			// Fire's creator.  The ID of the thing that
													// caused this fire to be created.  Generally
													// used by a thing3d when creating an internal
													// fire in response to Burn messages.


	protected:
		int32_t m_lTimer;							// General purpose timer
		int32_t m_lCollisionTimer;				// Check for collisions when this expires
		int32_t m_lBurnUntil;					// Burn until this time.
		int32_t m_lCurrentAlphaTimeout;		// Use current Alpha until this time, then switch
		int32_t m_lBrightAlphaInterval;		// Show each alpha for this amount of time
		int32_t m_lDimAlphaInterval;			// Show dim alpha level for this amount of time
		int32_t m_lTimeToLive;					// Total time to show this animation
		int32_t m_lAlphaBreakPoint;			// Time to switch from Bright to Dim
		int32_t m_lStartTime;					// Starting time used to calc the Alpha %
		int16_t m_sCurrentAlphaLevel;		// Use this Alpha level
		int16_t m_sTotalAlphaChannels;
		uint32_t	m_u32CollideIncludeBits;	// bits to use for collision checking
		uint32_t	m_u32CollideDontcareBits;	// bits to use for collision checking
		uint32_t	m_u32CollideExcludeBits;	// bits to use for collision checking
		bool	m_bSendMessages;				// Whether or not to send messages to other
													// objects telling them to burn or not.
		FireAnim m_eFireAnim;				// Which animation to use for the fire		

		int32_t m_lPrevTime;						// Previous update time

		ChannelAA*	m_pAnimChannel;		// Alpha animation stored as a channel.
												
		int16_t m_sSuspend;						// Suspend flag

		CSmash		m_smash;					// Collision class

		// Tracks file counter so we know when to load/save "common" data 
		static int16_t ms_sFileCount;
		static int16_t ms_sLargeRadius;
		static int16_t ms_sSmallRadius;
		static int32_t  ms_lCollisionTime;	// Check for collisions this often
		static int32_t  ms_lSmokeTime;		// Time to let smoke run
		static int16_t ms_sWindDirection;	// Direction the wind is blowing, will
													// get changed slightly by each new smoke.
		static double ms_dWindVelocity;	// Smoke drift velocity

		// "Constant" values that we want to be able to tune using the editor

	//---------------------------------------------------------------------------
	// Constructor(s) / destructor
	//---------------------------------------------------------------------------
   public:
      CFire(void);
      virtual ~CFire(void);

	//---------------------------------------------------------------------------
	// Optional static functions
	//---------------------------------------------------------------------------

		// Function for this class that is called before play begins to make
		//	the resource manager cache the resources for this object.
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
			int16_t sZ,												// In: New z coord
			int32_t lTimeToLive = 1000,							// In: Milliseconds to burn
			bool bThick = true,									// In: Use thick fire (more opaque)
			FireAnim eFireAnim = LargeFire);					// In: Which anim to use

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
#endif // !defined(EDITOR_REMOVED)

		// Allows whoever creates the fire to control what gets burned by it
		// the defaults are set initially to Characters
		void SetCollideBits(uint32_t u32Include, uint32_t u32Dontcare, uint32_t u32Exclude)
		{
			m_u32CollideIncludeBits = u32Include;
			m_u32CollideDontcareBits = u32Dontcare;
			m_u32CollideExcludeBits = u32Exclude;
      }

		// Turns messages on which will send burn messages to things the fire
		// is touching.
		void MessagesOn(void)
		{
			m_bSendMessages = true;
		}

		// Turns messages off which allows for fire that is just a visual effect
		void MessagesOff(void)
		{
			m_bSendMessages = false;
		}

      bool IsBurning(void) const
		{
			return m_eFireAnim != Smoke && m_eFireAnim != SmallSmoke;
		}

		// Get the time left to live.
		// Check IsBurning() to determine whether this applies to the fire or
		// the smoke.
      int32_t GetTimeLeftToLive(void);

	//---------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------
	protected:
		// Get all required resources
		int16_t GetResources(void);						// Returns 0 if successfull, non-zero otherwise
		
		// Free all resources
		int16_t FreeResources(void);						// Returns 0 if successfull, non-zero otherwise

		// Initialize the fire for large or small objects
		int16_t Init(void);

		// Fire it out, let the smoke start.  This funciton will change animations,
		// remove from the smash so it won't collide with anything, reset the timers, etc.
		int16_t Smokeout(void);

      void WindDirectionUpdate(void);
	};


#endif //DOOFUS_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
