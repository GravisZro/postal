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
// Warp.h
// Project: Nostril (aka Postal)
// 
// History:
//		06/02/97 JMI	Started.
//
//		06/07/97	JMI	Added warp in options enum.
//							Also, constructor now initializes static ms_stockpile
//							to CDude defaults, if it has not yet been done.
//							Also, added CreateWarpFromDude().
//
//		06/15/97	JMI	Moved initialization of ms_stockpile into warp.cpp since
//							the stockpile is non aggregatable.
//
//		07/19/97	JMI	Added m_sRotY, the dude's initial rotation around the Y
//							axis.
//
//		07/21/97	JMI	Added GetX(), GetY(), and GetZ().	
//
//////////////////////////////////////////////////////////////////////////////
//
// This CThing-derived class will represent places and settings for dudes to
// 'warp' in at/with.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef WARP_H
#define WARP_H

#include "thing.h"
#include <newpix/sprite_base.h>

#include "StockPile.h"

class CDude;

class CWarp
    : public sprite_base_t,
      public CSprite2
	{
	//---------------------------------------------------------------------------
	// Types, enums, etc.
	//---------------------------------------------------------------------------
	public:

		enum	// Warp options.
			{
			None				= 0x0000,
			// Only one of these options may be specified per WarpIn*().
			CopyStockPile	= 0x0001,	// Copy warp stockpile to Dude's.
			UnionStockPile	= 0x0002,	// Union warp stockpile with Dude's.
			AddStockPile	= 0x0003,	// Add stockpile to dude.

			// Mask for stockpile options.
			StockPileMask	= 0x000F

			// These options can be combined per WarpIn*() call.

			};

	//---------------------------------------------------------------------------
	// Variables
	//---------------------------------------------------------------------------
   public:
		int16_t	m_sRotY;										// Dude's initial rotation 
																// around the Y axis.

      int16_t m_sSuspend;									// Suspend flag


	protected:
														
	//---------------------------------------------------------------------------
	// Static Variables
	//---------------------------------------------------------------------------
	public:


		// The stockpile of ammo and health used by all CWarps.
		static CStockPile	ms_stockpile;

		// Tracks file counter so we know when to load/save "common" data 
		static int16_t ms_sFileCount;


	//---------------------------------------------------------------------------
	// Constructor(s) / destructor
	//---------------------------------------------------------------------------
	public:
      CWarp(void);
      virtual ~CWarp(void);

	//---------------------------------------------------------------------------
	// Required virtual functions (implementing them as inlines doesn't pay!)
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

		// Called by editor to get the clickable pos/area of an object in 2D.
		virtual	// Overridden here.
		void EditRect(				// Returns nothing.
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
	// Handy external functions
	//---------------------------------------------------------------------------
	public:

		// Stocks, rejuvenates, and places a CDude.  The dude can be passed to this
		// function or allocated by this function.
		int16_t WarpIn(			// Returns 0 on success.
         managed_ptr<CDude>&	ppdude,	// In:  CDude to 'warp in', *ppdude = nullptr to create one.
									// Out: Newly created CDude, if no CDude passed in.
			int16_t	sOptions);	// In:  Options for 'warp in'.

		// Stocks, rejuvenates, and places a CDude at a random warp.  The dude can 
		// be passed to this function or allocated by this function.
		static int16_t WarpInAnywhere(	// Returns 0 on success.
			CRealm*	prealm,				// In:  Realm in which to choose CWarp.
         managed_ptr<CDude>&	ppdude,				// In:  CDude to 'warp in', *ppdude = nullptr to create one.
												// Out: Newly created CDude, if no CDude passed in.
			int16_t	sOptions);				// In:  Options for 'warp in'.

		// Creates a warp based on a dude's settings.
		static int16_t CreateWarpFromDude(	// Returns 0 on success.
			CRealm*	prealm,					// In:  Realm in which to choose CWarp.
         managed_ptr<CDude>	pdude,					// In:  Dude to create warp from.
         managed_ptr<CWarp>&	ppwarp,					// Out: New warp on success.
			bool		bCopyStockPile);		// In:  true to copy stockpile, false otherwise.


	//---------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------
	protected:
		// Get all required resources
		int16_t GetResources(void);						// Returns 0, if successfull, non-zero otherwise
		
		// Free all resources
		int16_t FreeResources(void);						// Returns 0, if successfull, non-zero otherwise

		// Initialize object.
		int16_t Init(void);									// Returns 0, on success.
	};


#endif // WARP_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
