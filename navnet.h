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
// navigationnet.h
// Project: Nostril (aka Postal)
//
// History:
//		01/28/97 BRH	Added navigation net objects as the thing that holds 
//							a group of bouys together in a navigational network.
//
//		03/13/97	JMI	Load now takes a version number.
//
//		06/30/97	JMI	Moved definitions of EditRect() and EditHotSpot() into 
//							navnet.cpp.
//
//		07/21/97	JMI	Added GetX(), GetY(), and GetZ().	
//
//		08/08/97 BRH	Added EditPostLoad function which adds the Nav Net name
//							to the editor's list box, just as it does when EditNew is
//							called.  This way the NavNets loaded from the realm file
//							are now correctly displayed.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef NAVIGATIONNET_H
#define NAVIGATIONNET_H

#include <map>

#include "thing.h"
#include <newpix/sprite_base.h>

#include "bouy.h" // remove when TreeListNode is no longer used

class CBouy;
// CNavigationNet is the class for navigation
class CNavigationNet
    : public sprite_base_t,
      public CSprite2
   {

	//---------------------------------------------------------------------------
	// Variables
	//---------------------------------------------------------------------------
	public:
      std::map<uint8_t, managed_ptr<CBouy>>	m_NodeMap;										// Map of ID's to CBouy nodes

	protected:
		uint8_t	 m_ucNextID;
      uint8_t	 m_ucNumSavedBouys;
		RString  m_rstrNetName;									// Name of Nav Net
		TreeListNode m_BouyTreeListHead;						// Head of sorted list
		TreeListNode m_BouyTreeListTail;						// Tail of sorted list

		int16_t m_sSuspend;											// Suspend flag

		// Tracks file counter so we know when to load/save "common" data 
		static int16_t ms_sFileCount;

		// "Constant" values that we want to be able to tune using the editor

	//---------------------------------------------------------------------------
	// Constructor(s) / destructor
	//---------------------------------------------------------------------------
   public:
      CNavigationNet(void);
      ~CNavigationNet(void);

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

		// Called by editor to init nav net after it is loaded
		int16_t EditPostLoad(void);

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

		// Add a bouy to this network and assign it an ID
		uint8_t AddBouy(CBouy* pBouy);

		// Remove a bouy from the network
		void RemoveBouy(uint8_t ucBouyID);

		// Get the address of the Bouy with this ID
      managed_ptr<CBouy> GetBouy(uint8_t ucBouy);

		// Find the bouy closest to this location in the world
		uint8_t FindNearestBouy(int16_t sX, int16_t sZ);

		// Preprocess the routing tables by pinging all nodes
		void UpdateRoutingTables(void);

		// Print the routing tables for debugging purposes
		void PrintRoutingTables(void);

		// Ping - return minimum number of hops from source to destination nodes
//		uint8_t Ping(uint8_t dst, uint8_t src, uint8_t depth, uint8_t maxdepth);
		uint8_t Ping(uint8_t dst, uint8_t src, uint8_t depth);

		uint8_t GetNumNodes(void)
			{ return m_ucNextID;}

		// Set this NavNet as the default one for the Realm.  
      int16_t SetAsDefault(void);

		// Delete all bouys from this network.
		int16_t DeleteNetwork(void);

	//---------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------
	protected:
		// Get all required resources
		int16_t GetResources(void);						// Returns 0 if successfull, non-zero otherwise
		
		// Free all resources
		int16_t FreeResources(void);						// Returns 0 if successfull, non-zero otherwise
	};


#endif //DOOFUS_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
