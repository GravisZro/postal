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
// ostrich.cpp
// Project: Postal
//
//	This module implements the ostrich.
//
// History:
//
//		05/09/97 BRH	Started the ostrich from the band member logic.
//
//		05/22/97 BRH	Render skips the Doofus render and goes right to the
//							Character render since its not using the m_dAnimRot.
//
//		05/26/97 BRH	Changed the call to OnDead to the Doofus version
//							rather than the Character version.
//
//		05/29/97	JMI	Changed instance of REALM_ATTR_FLOOR_MASK to 
//							REALM_ATTR_NOT_WALKABLE.
//
//		06/05/97	JMI	Changed m_sHitPoints to m_stockpile.m_sHitPoints to 
//							accommodate new m_stockpile in base class, CThing3d (which
//							used to contain the m_sHitPoints).
//
//		06/18/97 BRH	Changed over to using GetRandom()
//
//		06/25/97	JMI	Now calls PrepareShadow() in Init() which loads and sets up
//							a shadow sprite.
//
//		06/30/97 MJR	Replaced SAFE_GUI_REF with new GuiItem.h-defined macro.
//
//		06/30/97 BRH	Cached the sound samples in static portion of Load
//							function so that the sound effects will be ready
//							whenever this item is used on a level.
//
//		07/01/97	JMI	Replaced GetFloorMapValue() with GetHeightAndNoWalk() call.
//
//		07/09/97	JMI	Now uses realm()->Make2dResPath() to get the fullpath
//							for 2D image components.
//
//		07/12/97 BRH	Changed panic motion to the same as the base class, but
//							it still didn't fix the problem of them going through the
//							wall of the train car on the farm level.  Also increased
//							their hit points, and made them not die in one shot.
//
//		07/18/97	JMI	Got rid of bogus immitation PlaySample functions.
//							Now there is one PlaySample() function.  Also, you now
//							MUST specify a category and you don't have to specify a
//							SoundInstance ptr to specify a volume.
//
//		07/21/97 BRH	Changed the burning state so that the ostrich will burn 
//							longer.
//
//		08/06/97 BRH	Changed smash bits to zero when dying starts so that things
//							won't hit him as he falls slowly, and so you can shoot 
//							past him.  Also allows the shot animation restart when shot
//							to get a little more reaction since his shot animation
//							is so long.
//
//		08/10/97	JMI	Commented out the code that deleted all the sound things in
//							a realm when the ostriches paniced.
//
//		08/12/97 BRH	Set the flag for victim rather than hostile so that
//							the score will display the correct numbers of each
//							type.
//
//		09/03/97	JMI	Replaced Good Smash bit with Civilian.
//
//		09/03/97 BRH	Put in the real ostrich sounds.
//
//		09/04/97 BRH	Added ostrich dying sound.
//
////////////////////////////////////////////////////////////////////////////////

#include "ostrich.h"

#include "realm.h"
#include "SampleMaster.h"
#include "item3d.h"

////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

#define BRIGHTNESS_PER_LIGHT_ATTRIBUTE 15
#define NUM_ELEMENTS(a) (sizeof(a) / sizeof(a[0]) )

// Notification message lParm1's.
#define BLOOD_POOL_DONE_NOTIFICATION	1	// Blood pool is done animating.

// Random amount the blood splat can adjust.
#define BLOOD_SPLAT_SWAY		10

// Gets a random between -range / 2 and range / 2.
#define RAND_SWAY(sway)	((GetRandom() % sway) - sway / 2)

#define GUI_ID_OK						1

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

double COstrich::ms_dExplosionVelocity = 180.0;
double COstrich::ms_dMaxMarchVel = 30.0;
double COstrich::ms_dMaxRunVel = 80.0;
int32_t COstrich::ms_lStateChangeTime = 10000;
int16_t COstrich::ms_sStartingHitPoints = 100;

// Let this auto-init to 0
int16_t COstrich::ms_sFileCount;

COstrich::COstrich(void)
{
  m_sRotDirection = 0;
}

COstrich::~COstrich(void)
{
  realm()->m_smashatorium.Remove(&m_smash);

  // Free resources
  FreeResources();
}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t COstrich::Load(					// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,						// In:  File to load from
	bool bEditMode,					// In:  True for edit mode, false otherwise
	int16_t sFileCount,					// In:  File count (unique per file, never 0)
	uint32_t	ulFileVersion)				// In:  Version of file format to load.
{
	int16_t sResult = SUCCESS;

	// Call the base class load to get the instance ID, position, motion etc.
	sResult	= CDoofus::Load(pFile, bEditMode, sFileCount, ulFileVersion);
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
					CacheSample(g_smidOstrichShot);
					CacheSample(g_smidOstrichBurning);
					CacheSample(g_smidOstrichBlownup);
					CacheSample(g_smidOstrichDie);
					break;
				}
			}

		// Load Rocket Man specific data
			switch (ulFileVersion)
			{
				default:
				case 4:
				case 3:
				case 2:
				case 1:
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
			TRACE("COstrich::Load(): Error reading from file!\n");
		}
	}
	else
	{
	TRACE("CGrenader::Load(): CDoofus::Load() failed.\n");
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t COstrich::Save(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to save to
	int16_t sFileCount)										// In:  File count (unique per file, never 0)
{
	int16_t sResult = SUCCESS;

	// Call the base class save to save the instance ID, position etc.
	CDoofus::Save(pFile, sFileCount);

	// Save common data just once per file (not with each object)
	if (ms_sFileCount != sFileCount)
	{
		ms_sFileCount = sFileCount;

		// Save static data
	}


	if (!pFile->Error())
	{
		sResult = SUCCESS;
	}
	else
	{
		TRACE("COstrich::Save() - Error writing to file\n");
		sResult = FAILURE;
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Init - Call this after the resources are in place
////////////////////////////////////////////////////////////////////////////////

int16_t COstrich::Init(void)
{
	int16_t sResult = SUCCESS;

	// Prepare shadow (get resources and setup sprite).
	sResult	= PrepareShadow();

	// Init position, rotation and velocity
	m_dVel = 0.0;
   rotation.y = rspMod360(GetRandom());
	m_lPrevTime = realm()->m_time.GetGameTime();
	m_state = CCharacter::State_Idle;
	m_lTimer = realm()->m_time.GetGameTime() + 500;
	m_sBrightness = 0;	// Default brightness

	m_smash.m_bits		= CSmash::Civilian | CSmash::Character;
   m_smash.m_pThing = this;

	m_lAnimTime = 0;
	m_panimCur = &m_animStand;
	m_lTimer = realm()->m_time.GetGameTime();
	m_stockpile.m_sHitPoints = ms_sStartingHitPoints;

	m_state = CCharacter::State_Stand;
	m_dAcc = 150;

	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// Startup object
////////////////////////////////////////////////////////////////////////////////
void COstrich::Startup(void)								// Returns 0 if successfull, non-zero otherwise
{
	// Register this as a victim rather than a hostile.
	m_bCivilian = true;

	// Set the current height, previous time, and Nav Net by calling the
	// base class startup
	CDoofus::Startup();

	// Init other stuff
   Init();
}


////////////////////////////////////////////////////////////////////////////////
// Update object
////////////////////////////////////////////////////////////////////////////////
void COstrich::Update(void)
{
	int16_t sHeight = m_sPrevHeight;
	double dNewX;
	double dNewY;
	double dNewZ;
	double dX;
	double dZ;
   milliseconds_t lThisTime;
   milliseconds_t lTimeDifference;


	if (!m_sSuspend)
	{
		// Get new time
		lThisTime = realm()->m_time.GetGameTime(); 
		lTimeDifference = lThisTime - m_lPrevTime;

		// Calculate elapsed time in seconds
		double dSeconds = (double)(lThisTime - m_lPrevTime) / 1000.0;

		// Check for new messages that may change the state
		ProcessMessages();

		// Increment animation time
		m_lAnimTime += lTimeDifference;

		// Check the current state
		switch (m_state)
		{
        UNHANDLED_SWITCH;

//-----------------------------------------------------------------------
// Stand or Hide - Either one waits in the current position for the
//						 random timer to expire.
//-----------------------------------------------------------------------

			case State_Stand:
			case State_Hide:
				if (lThisTime > m_lTimer)
					ChangeRandomState();
				break;

//-----------------------------------------------------------------------
// Walk - Walk around a bit
//-----------------------------------------------------------------------

			case State_Walk:
				if (lThisTime > m_lTimer)
				{
					ChangeRandomState();
				}
				else
				{
               dX = position.x;
               dZ = position.z;

					m_dAcc = ms_dAccUser;
					UpdateVelocities(dSeconds, ms_dMaxMarchVel, ms_dMaxMarchVel);
					GetNewPosition(&dNewX, &dNewY, &dNewZ, dSeconds);
					if (MakeValidPosition(&dNewX, &dNewY, &dNewZ, 10) == true)
					{
					// Update Values /////////////////////////////////////////////////////////

                  position.x	= dNewX;
                  position.y	= dNewY;
                  position.z	= dNewZ;

						UpdateFirePosition();
					}
					else
					{
					// Restore Values ////////////////////////////////////////////////////////

						m_dVel			-= m_dDeltaVel;
					}

					// If he didn't move at all, then turn him so he will
					// avoid the wall
               if (dX == position.x && dZ == position.z)
					{
						// Turn to avoid wall next time
						if (m_sRotDirection == 0)
                     rotation.y = rspMod360(rotation.y - 20);
						else
                     rotation.y = rspMod360(rotation.y + 20);
					}
				}
		
				break;


//-----------------------------------------------------------------------
// Panic - Pick a random bouy and run to it.  When you are there, pick
//			  a different random bouy and run to it.  
//-----------------------------------------------------------------------

			case State_Panic:
            dX = position.x;
            dZ = position.z;

				DeluxeUpdatePosVel(dSeconds);  
/*

				m_dAcc = ms_dAccUser;
				UpdateVelocities(dSeconds, ms_dMaxRunVel, ms_dMaxRunVel);
				GetNewPosition(&dNewX, &dNewY, &dNewZ, dSeconds);

				// Get height and 'no walk' status at new position.
				bool		bNoWalk;
				sHeight	= realm()->GetHeightAndNoWalk(dNewX, dNewY, &bNoWalk);

				// If too big a height difference or completely not walkable . . .
				if (bNoWalk == true
					|| (sHeight - dNewY > 10) )// && m_bAboveTerrain == false && m_dExtHorzVel == 0.0))
				{
					TRACE("****** - loose ostrish\n");
				// Move like smoke
				}

				if (MakeValidPosition(&dNewX, &dNewY, &dNewZ, 10) == true)
				{
				// Update Values /////////////////////////////////////////////////////////

               position.x	= dNewX;
               position.y	= dNewY;
               position.z	= dNewZ;

					UpdateFirePosition();
				}
				else
				{
				// Restore Values ////////////////////////////////////////////////////////
	
					m_dVel			-= m_dDeltaVel;
				}
*/
				// If he didn't move at all, then turn him so he will
				// avoid the wall
            if (dX == position.x && dZ == position.z)
				{
					// Turn to avoid wall next time
					if (m_sRotDirection == 0)
                  rotation.y = rspMod360(rotation.y - 20);
					else
                  rotation.y = rspMod360(rotation.y + 20);
				}


				break;

//-----------------------------------------------------------------------
// Burning - Run around on fire until dead
//-----------------------------------------------------------------------

			case State_Burning:
				if (!CCharacter::WhileBurning())
				{
					if (m_stockpile.m_sHitPoints <= 0)
					{
						m_state = State_Die;
						m_lAnimTime = 0;
						m_panimCur = &m_animDie;
						PlaySample(
							g_smidOstrichDie,
							SampleMaster::Voices);
					}
					else
					{
						m_state = State_Panic;
					}
				}				
				break;

//-----------------------------------------------------------------------
// Blown Up - Do motion into the air until you hit the ground again
//-----------------------------------------------------------------------

			case State_BlownUp:
				if (!CCharacter::WhileBlownUp())
					m_state = State_Dead;
				else
					UpdateFirePosition();
				break;

//-----------------------------------------------------------------------
// Shot - Dies in one shot
//-----------------------------------------------------------------------

			case State_Shot:
				if (!CCharacter::WhileShot())
				{
					if (m_stockpile.m_sHitPoints <= 0)
					{
						m_state = State_Die;
						m_panimCur = &m_animDie;
						m_lAnimTime = 0;
						PlaySample(
							g_smidOstrichDie,
							SampleMaster::Voices);
					}
					else
					{
						m_state = State_Panic;
						m_panimCur = &m_animRun;
						m_lAnimTime = 0;
					}
				}
				break;

//-----------------------------------------------------------------------
// Die - run die animation until done, the you are dead
//-----------------------------------------------------------------------

			case State_Die:
				m_smash.m_bits = 0;
				if (!CCharacter::WhileDying())
					m_state = State_Dead;
				else
					UpdateFirePosition();
				
				break;

//-----------------------------------------------------------------------
// Dead - paste yourself in the background and delete yourself
//-----------------------------------------------------------------------

			case State_Dead:
            OnDead();
            Object::enqueue(SelfDestruct);
            return;
		}

      m_smash.m_sphere.sphere.X			= position.x;
		// Fudge center of sphere as half way up the dude.
		// Doesn't work if dude's feet leave the origin.
      m_smash.m_sphere.sphere.Y			= position.y + m_sRadius;
      m_smash.m_sphere.sphere.Z			= position.z;
      m_smash.m_sphere.sphere.lRadius	= m_sRadius;

		// Update the smash.
		realm()->m_smashatorium.Update(&m_smash);

		// Save height for next time
		m_sPrevHeight = sHeight;

		// Save time for next time
		m_lPrevTime = lThisTime;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Render object
////////////////////////////////////////////////////////////////////////////////
void COstrich::Render(void)
{
	// Call base class.
	CCharacter::Render();

}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t COstrich::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
	int16_t sResult = SUCCESS;

	// Call the base class to place the item.
	sResult = CDoofus::EditNew(sX, sY, sZ);

	if (sResult == SUCCESS)
	{
		// Load resources
		sResult = GetResources();
		if (sResult == SUCCESS)
		{
			sResult	= Init();
		}
	}
	
	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// EditModify - Show dialog box for selecting starting bouy
////////////////////////////////////////////////////////////////////////////////

int16_t COstrich::EditModify(void)
{
   return SUCCESS;
}
#endif // !defined(EDITOR_REMOVED)

////////////////////////////////////////////////////////////////////////////////
// Get all required resources
////////////////////////////////////////////////////////////////////////////////
int16_t COstrich::GetResources(void)						// Returns 0 if successfull, non-zero otherwise
{
  bool bResult = true;

  if(bResult &= m_animRun.Get("ostrich_run"))
    m_animRun.SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);

  if(bResult &= m_animStand.Get("ostrich_stand"))
    m_animStand.SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);

  if(bResult &= m_animWalk.Get("ostrich_walk"))
    m_animWalk.SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);

  bResult &= m_animShot.Get("ostrich_shot");
  bResult &= m_animBlownup.Get("ostrich_blownup");
  bResult &= m_animHide.Get("ostrich_hide");
  bResult &= m_animDie.Get("ostrich_die");

  return bResult ? SUCCESS : FAILURE;;
}


////////////////////////////////////////////////////////////////////////////////
// Free all resources
////////////////////////////////////////////////////////////////////////////////
int16_t COstrich::FreeResources(void)						// Returns 0 if successfull, non-zero otherwise
{
	m_animRun.Release();
	m_animStand.Release();
	m_animWalk.Release();
	m_animShot.Release();
	m_animBlownup.Release();
	m_animHide.Release();
	m_animDie.Release();		

   return SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// ProcessMessages - Similar to the base class version but handles a few more
////////////////////////////////////////////////////////////////////////////////

void COstrich::ProcessMessages(void)
{
  while (!m_MessageQueue.empty())
  {
    GameMessage& msg = m_MessageQueue.front();
    ProcessMessage(&msg);
    if(msg.msg_Generic.eType == typePanic)
      OnPanicMsg(&(msg.msg_Panic));
    m_MessageQueue.pop_front();
  }
}

////////////////////////////////////////////////////////////////////////////////
// Message handlers
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Shot Message
////////////////////////////////////////////////////////////////////////////////

void COstrich::OnShotMsg(Shot_Message* pMessage)
{
    UnlockAchievement(ACHIEVEMENT_FIGHT_AN_OSTRICH);

	m_stockpile.m_sHitPoints -= pMessage->sDamage;

	if (m_state != State_BlownUp &&
//	    m_state != State_Shot &&
		 m_state != State_Die &&
		 m_state != State_Dead)
	{
		CCharacter::OnShotMsg(pMessage);

		// Restart the animation even if he is shot, 
		m_lAnimTime = 0;
		m_panimCur = &m_animShot;

		if (m_state != State_Shot)
		{
			PlaySample(
				g_smidOstrichShot,
				SampleMaster::Voices);
			m_state = State_Shot;
			AlertFlock();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// Explosion message
////////////////////////////////////////////////////////////////////////////////

void COstrich::OnExplosionMsg(Explosion_Message* pMessage)
{
	if (m_state != State_BlownUp)
	{
        UnlockAchievement(ACHIEVEMENT_FIGHT_AN_OSTRICH);

		CCharacter::OnExplosionMsg(pMessage);

		// Explosion kills the guy
		m_stockpile.m_sHitPoints = 0;
//		m_dExtVertVel = ms_dExplosionVelocity;
		m_state = State_BlownUp;
		m_lAnimTime = 0;
		m_panimCur = &m_animBlownup;
		PlaySample(
			g_smidOstrichBlownup,
			SampleMaster::Voices);
		AlertFlock();
	}
}

////////////////////////////////////////////////////////////////////////////////
// Burning message
////////////////////////////////////////////////////////////////////////////////

void COstrich::OnBurnMsg(Burn_Message* pMessage)
{
    UnlockAchievement(ACHIEVEMENT_FIGHT_AN_OSTRICH);

	CCharacter::OnBurnMsg(pMessage);
	m_stockpile.m_sHitPoints -= MAX(pMessage->sDamage / 4, 1);

	if (m_state != State_Burning && 
	    m_state != State_BlownUp &&
		 m_state != State_Die &&
		 m_state != State_Dead)
	{
		m_state = State_Burning;
		m_panimCur = &m_animRun;
		m_lAnimTime = 0;
		PlaySample(
			g_smidOstrichBurning,
			SampleMaster::Voices);
		AlertFlock();
	}
}

////////////////////////////////////////////////////////////////////////////////
// Panic message
////////////////////////////////////////////////////////////////////////////////

void COstrich::OnPanicMsg(Panic_Message* pMessage)
{
  UNUSED(pMessage);
	if (m_state != State_Die &&
	    m_state != State_Dead &&
		 m_state != State_BlownUp &&
		 m_state != State_Shot &&
		 m_state != State_Burning &&
		 m_state != State_Panic &&
		 m_state != State_Hide)
	{
		m_state = State_Panic;
		m_panimCur = &m_animRun;
		m_lAnimTime = GetRandom() % m_panimCur->m_psops->totalTime;
	}
}

////////////////////////////////////////////////////////////////////////////////
// AlertFlock
////////////////////////////////////////////////////////////////////////////////

void COstrich::AlertFlock(void)
{
	GameMessage msg;
//	GameMessage msgStopSound;

//	msgStopSound.msg_ObjectDelete.eType = typeObjectDelete;
//	msgStopSound.msg_ObjectDelete.sPriority = 0;

	msg.msg_Panic.eType = typePanic;
	msg.msg_Panic.sPriority = 0;
   msg.msg_Panic.sX = (int16_t) position.x;
   msg.msg_Panic.sY = (int16_t) position.y;
   msg.msg_Panic.sZ = (int16_t) position.z;

   for(managed_ptr<CThing>& pThing : realm()->GetThingsByType(COstrichID))
     if(pThing != this)
       SendThingMessage(msg, pThing);
}


////////////////////////////////////////////////////////////////////////////////
// Implements basic one-time functionality for each time State_Dead is
// entered.
////////////////////////////////////////////////////////////////////////////////
void COstrich::OnDead(void)
	{
	// Call base class.
	CDoofus::OnDead();
	}

////////////////////////////////////////////////////////////////////////////////
// ChangeRandomState - Choose among the 3 normal states, stand, hide or walk
////////////////////////////////////////////////////////////////////////////////

void COstrich::ChangeRandomState(void)
{
	int16_t sMod;
	m_lTimer = realm()->m_time.GetGameTime() + ms_lStateChangeTime + GetRandom() % 5000;
	sMod = m_lTimer % 3;
   rotation.y = rspMod360(rotation.y - 10 + (GetRandom() % 20));
	m_sRotDirection = m_lTimer % 2;

	switch (sMod)
	{
		case 0:
			m_state = State_Hide;
			if (m_panimCur != &m_animHide)
			{
				m_panimCur = &m_animHide;
				m_lAnimTime = 0;
			}
			break;

		case 1:
			m_state = State_Stand;
			if (m_panimCur != &m_animStand)
			{
				m_panimCur = &m_animStand;
				m_lAnimTime = 0;
			}
			break;

		case 2:
			m_state = State_Walk;
			if (m_panimCur != &m_animWalk)
			{
				m_panimCur = &m_animWalk;
				m_lAnimTime = 0;
			}
			break;

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
