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
// SoundThing.H
// Project: Nostril (aka Postal)
// 
// History:
//		02/24/97 MJR	Stole infrastructure from Jon's AnimThing.
//
//		03/07/97	JMI	Added m_psndChannel member and ProcessMessages() and
//							m_state (with enums).
//
//		03/13/97	JMI	Load now takes a version number.
//
//		07/17/97	JMI	Changed RSnd*'s to SampleMaster::SoundInstances.
//							Now uses new SampleMaster interface for volume and play
//							instance reference.
//
//		07/18/97	JMI	Got rid of bogus immitation PlaySample functions.
//							Now there is one PlaySample() function.  Also, you now
//							MUST specify a category and you don't have to specify a
//							SoundInstance ptr to specify a volume.
//							Also, added user edittable member m_lVolumeHalfLife.
//
//		07/21/97	JMI	Added GetX(), GetY(), and GetZ().	
//
//		08/01/97	JMI	Added looping parameters.
//
//		08/04/97	JMI	Now defaults to enabled.
//
//		08/04/97	JMI	Added m_sAmbient indicating whether or not this sound
//							is ambient (i.e., non-essential).
//							Also, implemented a special random number generator
//							strictly for sound things so they can be merry and random
//							and not de-synchronize.
//
//		08/11/97	JMI	Added RelayVolume() and m_lCollectiveVolume so 
//							CSoundRelays can update their CSoundThing parents.
//
//		09/24/97	JMI	Now initializes bFemalePain member of m_id.  This member
//							indicates whether the sample is of a female in pain which
//							some countries (so far just UK) don't want in the game.
//
//		10/07/97	JMI	Changed bFemalePain to usDescFlags.
//
//////////////////////////////////////////////////////////////////////////////
//
// This CThing-derived class will play sounds with various options.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef SOUNDTHING_H
#define SOUNDTHING_H

#include "thing.h"
#include <newpix/sprite_base.h>

#include "SampleMaster.h"

// This class has its own GetRandom() to keep it from de-synching the game.
#ifdef GetRandom
	#undef GetRandom
#endif

#ifdef GetRand
	#undef GetRand
#endif


class CSoundThing
    : public sprite_base_t,
      public CSprite2
	{
	//---------------------------------------------------------------------------
	// Types, enums, etc.
	//---------------------------------------------------------------------------
	public:

		typedef enum
			{
         State_Happy,		// La, la, la.
			} State;

	//---------------------------------------------------------------------------
	// Variables
	//---------------------------------------------------------------------------
	public:
		bool m_bInitiallyEnabled;
		bool m_bInitiallyRepeats;
      milliseconds_t m_lMinTime[2];
      milliseconds_t m_lRndTime[2];
		char	m_szResName[PATH_MAX];		// Resource name

		SampleMasterID m_id;

		SampleMaster::SoundInstance	m_siChannel;

      milliseconds_t m_lLastStartTime;
      milliseconds_t m_lNextStartTime;
		int16_t m_sWhichTime;
		bool m_bEnabled;
		bool m_bRepeats;

		int16_t	m_sUseLooping;						// TRUE, to use looping parameters.
      milliseconds_t	m_lStopLoopingTime;				// Time that we stop looping the sample.
      uint32_t	m_lNumLoopBacks;					// Number of times to play loop area of sample.
      milliseconds_t	m_lLoopBackTo;						// Where to loop back to.
      milliseconds_t	m_lLoopBackFrom;					// Where to loop back from.

		int16_t m_sSuspend;							// Suspend flag

		State	m_state;								// Current state.

		int32_t	m_lVolumeHalfLife;				// Half life of the current sound.

		int16_t	m_sPurgeSampleWhenDone;			// TRUE, to purge sample when done.

		int16_t	m_sAmbient;							// TRUE, if ambient (i.e., non-essential) sound.

		int32_t	m_lCollectiveVolume;				// Collective volume from this object and
														// its child satellites.
														
	protected:

		static int16_t	ms_sFileCount;			// File count.
		static int32_t		ms_lGetRandomSeed;	// Seed for get random.
														
	//---------------------------------------------------------------------------
	// Constructor(s) / destructor
	//---------------------------------------------------------------------------
   public:
      CSoundThing(void);
      virtual ~CSoundThing(void);


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

		// Get the coordinates of this thing.
		virtual					// Overriden here.
      double GetX(void)	const { return m_position.x; }

		virtual					// Overriden here.
      double GetY(void)	const { return m_position.y; }

		virtual					// Overriden here.
      double GetZ(void)	const { return m_position.z; }

	//---------------------------------------------------------------------------
	// External functions
	//---------------------------------------------------------------------------
	public:
		// Relay the volume to add to this CSoundThing's collective volume.
		void RelayVolume(	// Returns nothing.
			int32_t lVolume);	// In:  Volume to relay.

	//---------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------
	protected:
		// Init object
		int16_t Init(void);											// Returns 0 if successfull, non-zero otherwise

		// Don't call this from outside of CSoundThing.  It should affect only
		// CSoundThing stuff.
		static int32_t GetRandom(void);
		static int32_t GetRand(void)
			{
			return GetRandom();
			}

	};


#endif // SOUNDTHING_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
