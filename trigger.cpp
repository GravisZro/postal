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
// trigger.cpp
// Project: Nostril (aka Postal)
//
// This module impliments the CTrigger class -> holds trigger attributes
//
// History:
//		12/25/96 MJR	Started.
//
//		05/12/97	JRD	Turned this into a CTrigger to load the trigger attributes
//
////////////////////////////////////////////////////////////////////////////////

#include <RSPiX.h>
#include "trigger.h"
#include "game.h"
#include "realm.h" 


////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Constructor
// (protected).
////////////////////////////////////////////////////////////////////////////////
CTrigger::CTrigger(void)
   {
  position = invalid_position;
	// Insert a default instance into the realm:
	m_pmgi = nullptr;
   realm()->m_pTriggerMapHolder = this;
	realm()->m_pTriggerMap = m_pmgi;

	// Assume we don't know the UID's fdor the pylons yet, so clear them all out:
	for (int16_t i=0;i < 256;i++)
		{
      m_ausPylonUIDs[i] = realm()->m_asPylonUIDs[i] = 0;
		}
	}

////////////////////////////////////////////////////////////////////////////////
// Destructor
// (public).
////////////////////////////////////////////////////////////////////////////////
CTrigger::~CTrigger()
	{
	if (m_pmgi) delete m_pmgi;
	m_pmgi = nullptr;

	// Clear it from the realm:
	realm()->m_pTriggerMap = nullptr;
   realm()->m_pTriggerMapHolder.reset();
	}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CTrigger::Load(								// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,									// In:  File to load from
	bool bEditMode,								// In:  True for edit mode, false otherwise
	int16_t sFileCount,								// In:  File count (unique per file, never 0)
	uint32_t	ulFileVersion)							// In:  Version of file format to load.
	{
	int16_t sResult = SUCCESS;

	// In most cases, the base class Load() should be called.
	sResult	= CThing::Load(pFile, bEditMode, sFileCount, ulFileVersion);
	if (sResult == SUCCESS)
		{
		// Load object data
		realm()->m_pTriggerMap = nullptr; // clear the shadow
		// ASSUME THERE WILL ALREADY BE AN EMPTY TRIGGER MAP HOLDER!
      if (!realm()->m_pTriggerMapHolder)
        realm()->m_pTriggerMapHolder = realm()->AddThing<CTrigger>();

		if (m_pmgi) delete m_pmgi;
		m_pmgi = nullptr;

		int16_t sData = 0;
		pFile->Read(&sData);

		// Was there something here?
		if (sData)
			{
			m_pmgi = new RMultiGridIndirect;
			if (m_pmgi) 
				{
				if (m_pmgi->Load(pFile) != SUCCESS)
					{
					TRACE("CTrigger::Load(): Warning - couldn't load trigger attributes!\n");
					sResult = FAILURE;
					}
				else
					{
					// Load the ID list:
					if (pFile->Read(m_ausPylonUIDs,256) != 256) // Grab the ID's
						{
						TRACE("CTrigger::Load(): Warning - I lost my pylon IDs!\n");
						sResult = FAILURE;
						}
					else
						{
						// Install into the Realm
                  realm()->m_pTriggerMapHolder = this;
						realm()->m_pTriggerMap = m_pmgi;
						
						// Copy the Pylons to the realm!
						for (int16_t i=0;i < 256;i++) realm()->m_asPylonUIDs[i] = m_ausPylonUIDs[i];
						}
					}
				}
			}

		// Make sure there were no file errors or format errors . . .
		if (!pFile->Error() && sResult == SUCCESS)
			{
			}
		else
			{
			sResult = FAILURE;
			TRACE("CTrigger::Load(): Error reading from file!\n");
			}
		}
	else
		{
		TRACE("CTrigger::Load(): CThing::Load() failed.\n");
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CTrigger::Save(								// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,									// In:  File to save to
	int16_t sFileCount)								// In:  File count (unique per file, never 0)
	{
	int16_t sResult = SUCCESS;

	// In most cases, the base class Save() should be called.
	sResult	= CThing::Save(pFile, sFileCount);
	if (sResult == SUCCESS)
		{
		// Save object data
		int16_t sData = 0; // NO DATA

		if (m_pmgi == nullptr) 
			{
			sData = 0;
			pFile->Write(sData);
			}
		else
			{
			// There IS an attribute:
			sData = 1;
			pFile->Write(sData);
			if (m_pmgi->Save(pFile) != SUCCESS)
				{
				sResult = FAILURE;
				TRACE("CTrigger::Save(): Error - coudln't save trigger attributes.\n");
				}
			else
				{
				// Save the Pylon Data:
				if (pFile->Write(m_ausPylonUIDs,256) != 256)
					{
					sResult = FAILURE;
					TRACE("CTrigger::Save(): Error - coudln't save Pylon IDs.\n");
					}
				}
			}

		sResult	= pFile->Error();
		if (sResult == SUCCESS)
			{
			}
		else
			{
			TRACE("CTrigger::Save(): Error writing to file.\n");
			}
		}
	else
		{
		TRACE("CTrigger::Save(): CThing::Save() failed.\n");
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Startup object
////////////////////////////////////////////////////////////////////////////////
void CTrigger::Startup(void)								// Returns 0 if successfull, non-zero otherwise
   {
	}


////////////////////////////////////////////////////////////////////////////////
// Suspend object
////////////////////////////////////////////////////////////////////////////////
void CTrigger::Suspend(void)
	{
	}


////////////////////////////////////////////////////////////////////////////////
// Resume object
////////////////////////////////////////////////////////////////////////////////
void CTrigger::Resume(void)
	{
	}


////////////////////////////////////////////////////////////////////////////////
// Update object
////////////////////////////////////////////////////////////////////////////////
void CTrigger::Update(void)
	{
	}


////////////////////////////////////////////////////////////////////////////////
// Render object
////////////////////////////////////////////////////////////////////////////////
void CTrigger::Render(void)
	{
	}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CTrigger::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
	{
  UNUSED(sX, sY, sZ);
   return SUCCESS;
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to modify object
////////////////////////////////////////////////////////////////////////////////
int16_t CTrigger::EditModify(void)
	{
   return SUCCESS;
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to move object to specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CTrigger::EditMove(									// Returns 0 if successfull, non-zero otherwise
	int16_t /*sX*/,											// In:  New x coord
	int16_t /*sY*/,											// In:  New y coord
	int16_t /*sZ*/)											// In:  New z coord
	{
   return SUCCESS;
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to update object
////////////////////////////////////////////////////////////////////////////////
void CTrigger::EditUpdate(void)
	{
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to render object
////////////////////////////////////////////////////////////////////////////////
void CTrigger::EditRender(void)
	{
	}
#endif // !defined(EDITOR_REMOVED)

////////////////////////////////////////////////////////////////////////////////
// Add myself into the realm that created me.
////////////////////////////////////////////////////////////////////////////////
void	CTrigger::AddData(RMultiGridIndirect* pmgi)
	{
	realm()->m_pTriggerMap = m_pmgi = pmgi;

	// Copy my Pylon Data into the Realm's:
	for (int16_t i=0;i < 256;i++) realm()->m_asPylonUIDs[i] = m_ausPylonUIDs[i];
	}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
