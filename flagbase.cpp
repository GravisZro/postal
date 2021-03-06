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
// flagbase.cpp
// Project: Postal
//
// This module implements the flag object for capture the flag gameplay.
//
// History:
//
//		06/30/97 BRH	Started this file for the challenge levels.
//
//		07/12/97 BRH	Added FlagID so that flags can be matched with their
//							bases.  Added loading/saving thereof.  Also added
//							EditModify dialog so that the value can be set.
//
//		07/14/97 BRH	Changed to using the CSmash::Flagbase bits to identify
//							the base.  Also added checking for Flags to update
//							and incrementing the m_sFlagbaseCaptured value in realm
//							when the proper flag meets the base.
//
//		07/16/97 BRH	Changed to using the correct base files rather than
//							the bandguy as a placeholder.
//
//		08/03/97	JMI	Init() was setting the looping parms on a phot which no
//							longer exists.  Now the looping parms are passed via the
//							Get() call in GetResources() instead so they will get set
//							via the CAnim3D which should know which ones are okay to
//							use.
//
//		08/11/97 BRH	Added flagbase color option as a variable that is loaded
//							and saved and can be changed in the EditModify dialog.
//
//		08/18/97	JMI	Changed State_Dead to call DeadRender3D() (which used to be
//							known/called as just another Render() overload).
//
//		08/28/97 BRH	Set the correct bits to detect the flag base.   Finished
//							the code for capturing the flagbase.
//
////////////////////////////////////////////////////////////////////////////////

#include "flagbase.h"

#include "flag.h"
#include "realm.h"
#include "SampleMaster.h"

////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

#define GUI_FLAGID_EDIT_ID		103
#define GUI_COLOR_EDIT_ID		104

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

// These are default values -- actually values are set using the editor!
double CFlagbase::ms_dInRange = 30 * 30;			// Sq distance to base

// Let this auto-init to 0
int16_t CFlagbase::ms_sFileCount;

#ifdef UNUSED_VARIABLES
// These are the points that are checked on the attribute map relative to his origin
static RP3d ms_apt3dAttribCheck[] =
{
	{-6, 0, -6},
	{ 0, 0, -6},
	{ 6, 0, -6},
	{-6, 0,  6},
	{ 0, 0,  6},
	{ 6, 0,  6},
};
#endif

CFlagbase::CFlagbase(void)
{
  m_sSuspend = 0;
  rotation.y = 0;
  position.x = position.y = position.z = m_dVel = m_dAcc = 0;
  m_panimCur = nullptr;
  //			m_sprite.m_pthing	= this;
  m_u16FlagID = 1;
  m_u16Color = 0;
}

CFlagbase::~CFlagbase(void)
{
  realm()->m_smashatorium.Remove(&m_smash);

  // Free resources
  FreeResources();
}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CFlagbase::Load(				// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,						// In:  File to load from
	bool bEditMode,					// In:  True for edit mode, false otherwise
	int16_t sFileCount,					// In:  File count (unique per file, never 0)
	uint32_t	ulFileVersion)				// In:  Version of file format to load.
{
	int16_t sResult = SUCCESS;
	// Call the base load function to get ID, position, etc.
	sResult = CThing3d::Load(pFile, bEditMode, sFileCount, ulFileVersion);
	if (sResult == SUCCESS)
	{
		// Load common data just once per file (not with each object)
		if (ms_sFileCount != sFileCount)
		{
			ms_sFileCount = sFileCount;

			// Load static data		
			switch (ulFileVersion)
			{
				default:
				case 1:
					pFile->Read(&ms_dInRange);
					break;
			}
		}

		// Load other values
		switch (ulFileVersion)
		{
			default:
			case 45:
				pFile->Read(&m_u16Color);

			case 44:
			case 43:
			case 42:
			case 41:
			case 40:
			case 39:
			case 38:
			case 37:
			case 36:
			case 35:
			case 34:
			case 33:
			case 32:
			case 31:
			case 30:
			case 29:
			case 28:
			case 27:
			case 26:
			case 25:
			case 24:
			case 23:
			case 22:
			case 21:
			case 20:
			case 19:
			case 18:
			case 17:
			case 16:
			case 15:
			case 14:
			case 13:
			case 12:
			case 11:
			case 10:
			case 9:
			case 8:
			case 7:
			case 6:
			case 5:
			case 4:
			case 3:
			case 2:
			case 1:
			case 0:
				pFile->Read(&m_u16FlagID);
				break;
		}

		// Make sure there were no file errors or format errors . . .
		if (!pFile->Error() && sResult == SUCCESS)
		{
			// Get resources
			sResult = GetResources();
		}
		else
		{
			sResult = FAILURE;
			TRACE("CFlagbase::Load(): Error reading from file!\n");
		}
	}
	else
	{
		TRACE("CFlagbase::Load():  CDoofus::Load() failed.\n");
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CFlagbase::Save(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to save to
	int16_t sFileCount)										// In:  File count (unique per file, never 0)
{
	// Swap the hotspot we want to save in.

	int16_t sResult;

	// Call the base class save to save the instance ID, position, etc
	CThing3d::Save(pFile, sFileCount);

	// Save common data just once per file (not with each object)
	if (ms_sFileCount != sFileCount)
	{
		ms_sFileCount = sFileCount;

		// Save static data
		pFile->Write(&ms_dInRange);
	}

	// Save additinal stuff here.
	pFile->Write(&m_u16Color);
	pFile->Write(&m_u16FlagID);

	if (!pFile->Error())
	{
		sResult = SUCCESS;
	}
	else
	{
		TRACE("CFlagbase::Save() - Error writing to file\n");
		sResult = FAILURE;
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Init - Call this after the resources are in place
////////////////////////////////////////////////////////////////////////////////

int16_t CFlagbase::Init(void)
{
	int16_t sResult = SUCCESS;

	// Prepare shadow (get resources and setup sprite).
	sResult	= PrepareShadow();

	// Init other stuff
	m_dVel = 0.0;
   rotation.y = 0.0;
	// Set to different starting state based on the design of the animation, but
	// for now, ok.  Then also set his current animation.
	m_state = CFlagbase::State_Wait;
	m_panimCur = &m_animFlagWave;
	m_lAnimTime = 0;
	m_lTimer = realm()->m_time.GetGameTime() + 500;

	m_smash.m_bits = CSmash::FlagBase;
   m_smash.m_pThing = this;

	m_sBrightness = 0;	// Default Brightness level

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Startup object
////////////////////////////////////////////////////////////////////////////////
void CFlagbase::Startup(void)								// Returns 0 if successfull, non-zero otherwise
{
	// Set the current height, previous time, and Nav Net
	CThing3d::Startup();

	// Init other stuff
   Init();
}


////////////////////////////////////////////////////////////////////////////////
// Update object
////////////////////////////////////////////////////////////////////////////////
void CFlagbase::Update(void)
{
//	int16_t sHeight = m_sPrevHeight;
   milliseconds_t lThisTime;
   milliseconds_t lTimeDifference;
//	int32_t lSqDistanceToDude = 0;
	CSmash* pSmashed = nullptr;

	if (!m_sSuspend)
	{
		// Get new time
		lThisTime = realm()->m_time.GetGameTime();
		lTimeDifference = lThisTime - m_lPrevTime;

		// Calculate elapsed time in seconds
//		double dSeconds = (double)(lThisTime - m_lPrevTime) / 1000.0;

		// Check for new messages that may change the state
		ProcessMessages();


		switch(m_state)
		{
        UNHANDLED_SWITCH;
         case CFlagbase::State_Wait:
				if (lThisTime > m_lTimer)
				{
					m_state = CFlagbase::State_Guard;
				}
	
				// Update sphere.
            m_smash.m_sphere.sphere.X			= position.x;
            m_smash.m_sphere.sphere.Y			= position.y;
            m_smash.m_sphere.sphere.Z			= position.z;
				m_smash.m_sphere.sphere.lRadius	= 20; //m_spriteBase.m_sRadius;

				// Update the smash.
				realm()->m_smashatorium.Update(&m_smash);
				break;

//-----------------------------------------------------------------------
// Guard - normal operation
//-----------------------------------------------------------------------

			case CFlagbase::State_Guard:
				realm()->m_smashatorium.QuickCheckReset(&m_smash, CSmash::Flag, 0, 0);
				while (realm()->m_smashatorium.QuickCheckNext(&pSmashed))
				{
					if (pSmashed->m_pThing->type() == CFlagID)
					{
                  if (managed_ptr<CFlag>(pSmashed->m_pThing)->m_u16FlagID == m_u16FlagID)
						{
							realm()->m_sFlagbaseCaptured++;
							m_state = State_Dead;
						}	
					}
				}
				break;

//-----------------------------------------------------------------------
// Blownup - You were blown up so pop up into the air and come down dead
//-----------------------------------------------------------------------

				case CFlagbase::State_BlownUp:
					// Make her animate
					m_lAnimTime += lTimeDifference;

					if (!WhileBlownUp())
						m_state = State_Dead;
					else
					{
						UpdateFirePosition();
					}

					break;


//-----------------------------------------------------------------------
// Dead - You are dead, so lay there and decompose, then go away
//-----------------------------------------------------------------------

            case CFlagbase::State_Dead:
					// Render current dead frame into background to stay.
					realm()->Scene()->DeadRender3D(
                  realm()->Hood()->m_pimBackground,		// Destination image.
                  this,						// Tree of 3D sprites to render.
                  realm()->Hood());							// Dst clip rect.

                Object::enqueue(SelfDestruct);
               return;
		}


				// Update sphere.
            m_smash.m_sphere.sphere.X			= position.x;
            m_smash.m_sphere.sphere.Y			= position.y;
            m_smash.m_sphere.sphere.Z			= position.z;
				m_smash.m_sphere.sphere.lRadius	= 20; //m_spriteBase.m_sRadius;

				// Update the smash.
				realm()->m_smashatorium.Update(&m_smash);


		// Save time for next time
		m_lPrevTime = lThisTime;
		m_lAnimPrevUpdateTime = m_lAnimTime;
	}
}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CFlagbase::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
	int16_t sResult = SUCCESS;

	sResult = CThing3d::EditNew(sX, sY, sZ);

	if (sResult == SUCCESS)
	{
		// Load resources
		sResult = GetResources();
		if (sResult == SUCCESS)
		{
			sResult	= Init();
		}
	}
	else
	{
		sResult = FAILURE;
	}

	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// Edit Move
////////////////////////////////////////////////////////////////////////////////
int16_t CFlagbase::EditMove(int16_t sX, int16_t sY, int16_t sZ)
{
	int16_t sResult = CThing3d::EditMove(sX, sY, sZ);

	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// Give Edit a rectangle around this object
////////////////////////////////////////////////////////////////////////////////
void CFlagbase::EditRect(RRect* pRect)
	{
	// Call base class.
	CThing3d::EditRect(pRect);

	}

////////////////////////////////////////////////////////////////////////////////
// Called by editor to get the hotspot of an object in 2D.
// (virtual (Overridden here)).
////////////////////////////////////////////////////////////////////////////////
void CFlagbase::EditHotSpot(			// Returns nothiing.
	int16_t*	psX,						// Out: X coord of 2D hotspot relative to
											// EditRect() pos.
	int16_t*	psY)						// Out: Y coord of 2D hotspot relative to
											// EditRect() pos.
	{
	// Get rectangle.
	RRect	rc;
	EditRect(&rc);
	// Get 2D hotspot.
	int16_t	sX;
	int16_t	sY;
   realm()->Map3Dto2D(position.x, position.y, position.z,
                      sX, sY);

	// Get relation.
	*psX	= sX - rc.sX;
	*psY	= sY - rc.sY;
	}

////////////////////////////////////////////////////////////////////////////////
// Called by editor to modify object
////////////////////////////////////////////////////////////////////////////////
int16_t CFlagbase::EditModify(void)
{
	int16_t sResult = SUCCESS;
	uint16_t u16OrigColor = m_u16Color;
	RGuiItem* pguiRoot = RGuiItem::LoadInstantiate(FullPathVD("res/editor/flagbase.gui"));
	if (pguiRoot != nullptr)
	{
		REdit* peditFlagID = (REdit*) pguiRoot->GetItemFromId(GUI_FLAGID_EDIT_ID);
		REdit* peditColor  = (REdit*) pguiRoot->GetItemFromId(GUI_COLOR_EDIT_ID);

		if (peditFlagID != nullptr && peditColor != nullptr)
		{
			ASSERT(peditFlagID->m_type == RGuiItem::Edit);
			ASSERT(peditColor->m_type == RGuiItem::Edit);

			peditFlagID->SetText("%d", m_u16FlagID);
			peditFlagID->Compose();
			peditColor->SetText("%d", m_u16Color);
			peditColor->Compose();

			sResult = DoGui(pguiRoot);
			if (sResult == 1)
			{
				m_u16FlagID = peditFlagID->GetVal();
				m_u16Color = MIN((int32_t) (CFlag::EndOfColors - 1), peditColor->GetVal());
			}
		}
	}
	delete pguiRoot;

	// If the user switched colors, get the new resources
	if (m_u16Color != u16OrigColor)
	{
		FreeResources();
		GetResources();
	}

   return SUCCESS;
}
#endif // !defined(EDITOR_REMOVED)

////////////////////////////////////////////////////////////////////////////////
// Get all required resources
////////////////////////////////////////////////////////////////////////////////
int16_t CFlagbase::GetResources(void)						// Returns 0 if successfull, non-zero otherwise
{
  if(m_animFlagWave.Get(m_u16Color == CFlag::Blue ? "bbase" : "rbase"))
  {
    m_animFlagWave.SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);
    return SUCCESS;
  }
  return FAILURE;
}


////////////////////////////////////////////////////////////////////////////////
// Free all resources
////////////////////////////////////////////////////////////////////////////////
int16_t CFlagbase::FreeResources(void)						// Returns 0 if successfull, non-zero otherwise
{
	m_animFlagWave.Release();

   return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Message handlers
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Shot Message
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Explosion message
////////////////////////////////////////////////////////////////////////////////

void CFlagbase::OnExplosionMsg(Explosion_Message* pMessage)
{
  UNUSED(pMessage);
	if (
	    m_state != State_BlownUp	&&
		 m_state != State_Die		&& 
		 m_state != State_Dead)
	{
//		CCharacter::OnExplosionMsg(pMessage);
		
//		PlaySample(g_smidBlownupFemaleYell);
//		m_ePreviousState = m_state;
		m_state = State_BlownUp;
//		m_panimPrev = m_panimCur;
//		m_panimCur = &m_animDie;
		m_lAnimTime = 0;
//		m_stockpile.m_sHitPoints = 0;
		m_lTimer = realm()->m_time.GetGameTime();

		m_dExtHorzVel *= 1.4; //2.5;
		m_dExtVertVel *= 1.1; //1.4;
		// Send it spinning.
		m_dExtRotVelY	= GetRandom() % 720;
		m_dExtRotVelZ	= GetRandom() % 720;

//		m_panimCur = &m_animDie;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Burning message
////////////////////////////////////////////////////////////////////////////////

void CFlagbase::OnBurnMsg(Burn_Message* pMessage)
{
  UNUSED(pMessage);
	// For now we made the sentry fireproof, the only
	// way it can be destroyed is by blowing it up.
}


////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
