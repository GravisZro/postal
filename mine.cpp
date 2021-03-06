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
// mine.cpp
// Project: Nostril (aka Postal)
//
// This module implements the CMine weapon class which is a hand
// thrown grenade weapon.
// 
//
// History:
//		03/19/97 BRH	Started this weapon object.
//
//		03/19/97 BRH	Added 4 types of mines to this file.  Still need to
//							create a dialog in the gui editor to select which type
//							of mine to place in the editor.  Also need to add motion
//							functions to the base class and use them for the 
//							Bouncing Betty in Update.
//
//		03/20/97 BRH	Added dialog box for selecting mine type.  Still need to
//							do the bouncing betty mine.
//
//		03/21/97 BRH	Added the Bouncing Betty functionality.
//
//		04/10/97 BRH	Updated this to work with the new multi layer attribute
//							maps.
//
//		04/15/97 BRH	Added fuse timer dialog item to edit modify to allow
//							the timed mine fuse to be set.  Also added an overloaded
//							Setup() function to set the time for the fuse.  Changed
//							Load and Save to deal with this new fuse value.
//
//		04/23/97	JMI	Now sets its m_smash's bits to Mine instead of Item.
//
//		04/25/97	JMI	Added angle Z (vertical angle adjustment) of 0 to the
//							CBulletFest::Fire(...) call.
//
//		04/29/97	JMI	Changed State_Fire to merely jump to State_Idle.  The
//							reason is that CCharacter uses State_Fire to notify things
//							that they should arm.  Perhaps this state should be changed
//							to 'arm', but, technically, some things don't really even
//							arm until after that.  Perhaps it should be State_Go or
//							something.
//							Anyways, in order to use State_Fire as the 'arm' trigger
//							from the placer, I had to change State_Go to do what State_Fire
//							used to for the bouncing betty; namely, wait for the time
//							to expire.
//							Also, Render() now subtracts half the width and half the
//							height from the 2D render location in an attempt to better
//							center the image.  This should help especially when this
//							object is the child of another to make it appear in the
//							right spot.
//
//		04/30/97	JMI	Changed the Setup() override of the CWeapon's Setup() to
//							pass the current mine type to the Setup() with eType.
//							Changed Construct() to take an ID as a parameter and added
//							ConstructProximity(), ConstructTimed(), 
//							ConstructBouncingBetty(), and ConstructRemoteControl() to 
//							allocate that type of mine.
//							Removed m_eMineType (now uses Class ID instead).
//							Removed Setup() that took an eType.
//							Filled in PreLoad() (but it still needs to convert each
//							RImage to whatever type is most efficient).
//							Also, GetResources() was new'ing m_pImage and then calling
//							rspGetResource() (which gives you an entirely new instance
//							of an image); so it was basically wasting an RImage worth
//							of memory.
//
//		05/28/97 BRH	Increased arming time for Betty and Proximity mines to make
//							them easier to place and get away.
//
//		05/29/97	JMI	Removed ASSERT on realm()->m_pAttribMap which no longer
//							exists.
//
//		06/11/97 BRH	Added shooter ID's to the shot messages and passed it
//							along to the explosion.
//
//		06/12/97 BRH	Added shooter ID to the call to Setup for the explosion.
//
//		06/13/97	JMI	Now obeys State_Hide.
//
//		06/30/97 BRH	Added cache sound effects to Preload function.
//
//		07/09/97	JMI	Now uses realm()->Make2dResPath() to get the fullpath
//							for 2D image components.
//
//		07/09/97	JMI	Changed Preload() to take a pointer to the calling realm
//							as a parameter.
//
//		07/18/97	JMI	Got rid of bogus immitation PlaySample functions.
//							Now there is one PlaySample() function.  Also, you now
//							MUST specify a category and you don't have to specify a
//							SoundInstance ptr to specify a volume.
//
//		07/21/97	JMI	Now handles delete messages.
//
//		08/05/97	JMI	Changed priority to use Z position rather than 2D 
//							projected Y position.
//
//		08/16/97 BRH	Added an arming beep sound and a click sound when the
//							mine is armed.
//
//		08/17/97	JMI	Now, instead of aborting the sample, when the mine arms,
//							it stops the sample's loopage.
//							Now sets the volume every iteration b/c, although the mine
//							stays still, the distance to the local dude varies as the
//							local dude moves.
//							Also, changed m_pthingParent to m_idParent.
//
//		08/28/97 BRH	Added preload function to cache the sounds and images.
//
////////////////////////////////////////////////////////////////////////////////

#include "mine.h"

#include <cmath>

#include "realm.h"
#include "explode.h"
#include "SampleMaster.h"
#include "reality.h"

#include "Thing3d.h"

////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

#define TIMEDMINE_FILE				"TimeMine.img"
#define PROXIMITYMINE_FILE			"ProxMine.img"
#define BOUNCINGBETTYMINE_FILE	"BettyMine.img"
#define REMOTEMINE_FILE				"RemoteMine.img"

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

// These are default values -- actually values are set using the editor!
int16_t CMine::ms_sProximityRadius	= 10;
int16_t CMine::ms_sBettyRadius = 60;
int16_t CMine::ms_sBettyRange = 1000;
int32_t CMine::ms_lFuseTime = 6000;
int32_t CMine::ms_lArmingTime = 5000;
int32_t CMine::ms_lExplosionDelay = 150;
double CMine::ms_dInitialBounceVelocity = 80.0;

// Let this auto-init to 0
int16_t CMine::ms_sFileCount;

CMine::CMine(void)
{
  m_lFuseTime = 0;
  m_siMineBeep = 0;
}

CMine::~CMine(void)
{
  // Stop sound, if any.
  StopLoopingSample(m_siMineBeep);

  realm()->m_smashatorium.Remove(&m_smash);

  // Free resources
  FreeResources();
}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CMine::Load(				// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,						// In:  File to load from
	bool bEditMode,					// In:  True for edit mode, false otherwise
	int16_t sFileCount,					// In:  File count (unique per file, never 0)
	uint32_t	ulFileVersion)				// In:  Version of file format to load.
{
	int16_t sResult = SUCCESS;

	sResult = CWeapon::Load(pFile, bEditMode, sFileCount, ulFileVersion);
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
					pFile->Read(&ms_sProximityRadius);
					pFile->Read(&ms_sBettyRadius);
					pFile->Read(&ms_sBettyRange);
					pFile->Read(&ms_lFuseTime);
					pFile->Read(&ms_lArmingTime);
					pFile->Read(&ms_lExplosionDelay);
					pFile->Read(&ms_dInitialBounceVelocity);
					break;
			}
		}

		// Load object data
		MineType	type;
		switch (ulFileVersion)
		{
			case 1:
			case 2:
			case 3:
			case 4:
				pFile->Read(&type);
				break;

			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
				pFile->Read(&type);
			
			case 11:
			default:
				pFile->Read(&m_lFuseTime);
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
			TRACE("CMine::Load(): Error reading from file!\n");
		}
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CMine::Save(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to save to
	int16_t sFileCount)										// In:  File count (unique per file, never 0)
{
	// In most cases, the base class Save() should be called.  In this case it
	// isn't because the base class doesn't have a Save()!
	CWeapon::Save(pFile, sFileCount);

	// Save common data just once per file (not with each object)
	if (ms_sFileCount != sFileCount)
	{
		ms_sFileCount = sFileCount;

		// Save static data
		pFile->Write(&ms_sProximityRadius);
		pFile->Write(&ms_sBettyRadius);
		pFile->Write(&ms_sBettyRange);
		pFile->Write(&ms_lFuseTime);
		pFile->Write(&ms_lArmingTime);
		pFile->Write(&ms_lExplosionDelay);
		pFile->Write(&ms_dInitialBounceVelocity);
	}

	// Save object data
	pFile->Write(&m_lFuseTime);

   return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Startup
////////////////////////////////////////////////////////////////////////////////

void CMine::Startup(void)
{
   Init();
}

////////////////////////////////////////////////////////////////////////////////
// Init
////////////////////////////////////////////////////////////////////////////////

int16_t CMine::Init(void)
{
	int16_t sResult = SUCCESS;

	m_eState = State_Idle;

	int32_t lThisTime = realm()->m_time.GetGameTime();

	if (m_lFuseTime == 0)
		m_lFuseTime = ms_lFuseTime;

   switch (type())
	{
		case CTimedMineID:
			m_lTimer = m_lFuseTime + lThisTime;

			//This is needed to fix the crash when dropping timed mine
			//Bug is something to do with collision detection later on in smash.cpp
			m_sCurRadius = ms_sProximityRadius;

			break;

		case CProximityMineID:
			m_lTimer = ms_lArmingTime + lThisTime;
			m_sCurRadius = ms_sProximityRadius;
			break;

		case CBouncingBettyMineID:
			m_lTimer = ms_lArmingTime + lThisTime;
			m_sCurRadius = ms_sBettyRadius;
			break;

		case CRemoteControlMineID:
			m_lTimer = lThisTime;
			break;
     default:
       break;
	}

	// Set up collision object
	m_smash.m_bits = CSmash::Mine;
   m_smash.m_pThing = this;

	// Load resources
	sResult = GetResources();

	return sResult;

}

////////////////////////////////////////////////////////////////////////////////
// This inline updates a specified velocity with a specified drag over a
// specified time.
////////////////////////////////////////////////////////////////////////////////
inline
bool UpdateVelocity(		// Returns true if velocity reaches zero because of the
								// supplied accelration, false otherwise.
	double* pdVel,			// In:  Initial velocity.
								// Out: New velocity.
	double* pdDeltaVel,	// Out: Delta velocity.
	double dAcc,			// In:  Acceleration.
	double dSeconds)		// In:  Elapsed time in seconds.
	{
	bool	bAcceleratedToZero	= false;

	double	dVelPrev	= *pdVel;
	*pdDeltaVel			= dAcc * dSeconds;
	*pdVel				+= *pdDeltaVel;

	// I think this can be consdensed into a subtraction and one or two comparisons,
	// but I'm not sure that's really faster than the max 3 comparisons here.
	// If previously traveling forward . . .
	if (dVelPrev > 0.0)
		{
		// Passing 0 is considered at rest . . .
		if (*pdVel < 0.0)
			{
			// Update delta.
			*pdDeltaVel	-= *pdVel;
			// Zero velocity.
			*pdVel	= 0.0;
			}
		}
	else
		{
		// If previously traveling backward . . .
		if (dVelPrev < 0.0)
			{
			// Passing 0 is considered at rest . . .
			if (*pdVel > 0.0)
				{
				// Update delta.
				*pdDeltaVel	-= *pdVel;
				// Zero velocity.
				*pdVel	= 0.0;
				}
			}
		}

	// If velocity is now zero . . .
	if (*pdVel == 0.0)
		{
		// If drag opposed the previous velocity . . .
		if ((dVelPrev > 0.0 && dAcc < 0.0) || (dVelPrev < 0.0 && dAcc > 0.0))
			{
			// Drag has achieved its goal.
			bAcceleratedToZero = true;
			}
		}

	return bAcceleratedToZero;
	}

////////////////////////////////////////////////////////////////////////////////
// Update object
////////////////////////////////////////////////////////////////////////////////
void CMine::Update(void)
{
	CSmash* pSmashed = nullptr;
	double dDistance;
	int16_t sShootAngle;
	int16_t sShotX;
	int16_t sShotY;
	int16_t sShotZ;
   managed_ptr<sprite_base_t> pShotThing;
	GameMessage msg;

	if (!m_sSuspend)
	{
		// Get new time
		int32_t lThisTime = realm()->m_time.GetGameTime(); 

      ProcessMessages();
		// Calculate elapsed time in seconds
		double dSeconds = (double)(lThisTime - m_lPrevTime) / 1000.0;

		// Check the current state
		switch (m_eState)
		{
        UNHANDLED_SWITCH;
//-----------------------------------------------------------------------
// Idle - waiting to arm
//-----------------------------------------------------------------------
			case CWeapon::State_Idle:
				if (lThisTime > m_lTimer)
				{
					// End the looping arming sound
					if (m_siMineBeep != 0)
					{
						StopLoopingSample(m_siMineBeep);
						m_siMineBeep = 0;
					}
               switch (type())
					{
						case CTimedMineID:
							m_eState = State_Explode;
							break;

						case CProximityMineID:
						case CBouncingBettyMineID:
							m_eState = State_Armed;
							PlaySample(
								g_smidMineSet,				// Sample to play
								SampleMaster::Weapon,	// Category for user sound adjustment
                        DistanceToVolume(position.x, position.y, position.z, MineSndHalfLife) ); //Pos
							break;

						case CRemoteControlMineID:
						default:
							break;
					}
				}
				else
				{
               int16_t	sX	= position.x;
               int16_t	sY	= position.y;
               int16_t	sZ	= position.z;

               // If we have a parent . . .
               if (parent())
						{
						// Add in its position.
                  sX	+= parent3d()->GetX();
                  sY	+= parent3d()->GetY();
                  sZ	+= parent3d()->GetZ();
						}

					// Update sound position.
					SetInstanceVolume(m_siMineBeep, DistanceToVolume(sX, sY, sZ, MineSndHalfLife) );
				}
				break;

//-----------------------------------------------------------------------
// Armed - State for Proximity mine & bouncing betty where they check
//			  for collisions with other characters and react.
//-----------------------------------------------------------------------

			case CWeapon::State_Armed:
				if (realm()->m_smashatorium.QuickCheck(&m_smash, 
															CSmash::Character, 
														   CSmash::Good | CSmash::Bad,
															0, &pSmashed))
				{
               switch (type())
					{
						case CBouncingBettyMineID:
							m_eState = State_Go;
							m_dVertVel = ms_dInitialBounceVelocity;
							// Make it go off right away
							m_lTimer = lThisTime;
							break;

						case CProximityMineID:
							m_eState = State_Explode;
							// Make it go off right away
							m_lTimer = lThisTime;
							break;
                 default:
                   break;
					}
				}
				break;

//-----------------------------------------------------------------------
// Fire - Initial triggering notification from character (if placed by
//			 character).
//-----------------------------------------------------------------------

			case CWeapon::State_Fire:
				// Go back to waiting to arm.
				m_eState	= State_Idle;

				break;


//-----------------------------------------------------------------------
// Go - used for bouncing betty when it is bouncing up
//-----------------------------------------------------------------------
			case CWeapon::State_Go:
				if (lThisTime > m_lTimer)
				{
					PlaySample(
						g_smidBounceLaunch,
						SampleMaster::Weapon,
                  DistanceToVolume(position.x, position.y, position.z, LaunchSndHalfLife) );	// In:  Initial Sound Volume (0 - 255)
					// Do motion
					///////////////////////// Vertical Velocity /////////////////////////////////
					UpdateVelocity(&m_dVertVel, &m_dVertDeltaVel, g_dAccelerationDueToGravity, dSeconds);
					
					// Apply external vertical velocity.
					dDistance	= (m_dVertVel - m_dVertDeltaVel / 2) * dSeconds;
               position.y = position.y + dDistance;

					// If velocity is negative, then explode and shoot in 
					// several directions using deluxe shot or something.
					if (m_dVertVel <= 0.0)
					{
						// Make a small explosion noise
						PlaySample(
							g_smidBounceExplode,
							SampleMaster::Weapon,
                     DistanceToVolume(position.x, position.y, position.z, ExplosionSndHalfLife) );	// In:  Initial Sound Volume (0 - 255)

						// Draw some kind of flash on the mine
                  m_bulletfest.Impact(0, position.x, position.y, position.z, realm());
                  m_bulletfest.Impact(0, position.x + 5, position.y, position.z, realm());
                  m_bulletfest.Impact(0, position.x - 5, position.y, position.z, realm());
                  m_bulletfest.Impact(0, position.x, position.y + 5, position.z, realm());
                  m_bulletfest.Impact(0, position.x, position.y - 5, position.z, realm());

						// Shoot in all directions
						for (sShootAngle = 0; sShootAngle < 360; sShootAngle += 20)
						{
							m_bulletfest.Fire(sShootAngle,
													0,
                                       (int16_t) position.x,
                                       (int16_t) position.y,
                                       (int16_t) position.z,
													ms_sBettyRange,
													realm(),
													CSmash::Character,
													CSmash::Good | CSmash::Bad,
													0,
													&sShotX,
													&sShotY,
													&sShotZ,
                                       pShotThing);
                     if (pShotThing)
							{
								msg.msg_Shot.eType = typeShot;
								msg.msg_Shot.sPriority = 0;
								msg.msg_Shot.sDamage = 50;
                        msg.msg_Shot.sAngle = rspATan(position.z - sShotZ, sShotX - position.x);
								msg.msg_Shot.shooter = m_shooter;
								// Tell this thing that it got shot
								SendThingMessage(msg, pShotThing);
							}
						}
						// Get rid of the mine
                  Object::enqueue(SelfDestruct);
						return;
					}
				}

				break;

//-----------------------------------------------------------------------
// Explode
//-----------------------------------------------------------------------
			case CWeapon::State_Explode:

				if (lThisTime > m_lTimer)
				{
					// Start an explosion object and then kill rocket
					// object
              managed_ptr<CExplode> pExplosion = realm()->AddThing<CExplode>();
              if (pExplosion)
					{
                  pExplosion->Setup(position.x, position.y, position.z, m_shooter);
						PlaySample(
							g_smidGrenadeExplode,
							SampleMaster::Destruction,
                     DistanceToVolume(position.x, position.y, position.z, ExplosionSndHalfLife) );	// In:  Initial Sound Volume (0 - 255)
					}

               Object::enqueue(SelfDestruct);
					return;
				}
				break;
		}

		// Save time for next time
		m_lPrevTime = lThisTime;

		// Update sphere.
      m_smash.m_sphere.sphere.X			= position.x;
      m_smash.m_sphere.sphere.Y			= position.y;
      m_smash.m_sphere.sphere.Z			= position.z;
		m_smash.m_sphere.sphere.lRadius	= m_sCurRadius;

		// Update the smash.
		realm()->m_smashatorium.Update(&m_smash);

	}
}


////////////////////////////////////////////////////////////////////////////////
// Render object
////////////////////////////////////////////////////////////////////////////////
void CMine::Render(void)
{
   if (m_pImage != nullptr)
	{
		// Image would normally animate, but doesn't for now
      m_pImage = m_pImage;

      flags.Hidden = m_eState == State_Hide;

		// Map from 3d to 2d coords
      realm()->Map3Dto2D(position.x, position.y, position.z,
                         m_sX2, m_sY2);

		// Center on image.
      m_sX2	-= m_pImage->m_sWidth / 2;
      m_sY2	-= m_pImage->m_sHeight / 2;

		// Priority is based on bottom edge of sprite
      m_sPriority = position.z;

		// Layer should be based on info we get from attribute map.
      m_sLayer = CRealm::GetLayerViaAttrib(realm()->GetLayer((int16_t) position.x, (int16_t) position.z));

      Object::enqueue(SpriteUpdate); // Update sprite in scene
	}
}

////////////////////////////////////////////////////////////////////////////////
// Setup new object - called by object that created this object
//							 This version is meant for the timed mine so that
//							 the fuse time can be set.
////////////////////////////////////////////////////////////////////////////////

int16_t CMine::Setup(									// Returns 0 if successful, non-zero otherwise
	int16_t sX,											// In:  X coord placement
	int16_t sY,											// In:  Y coord placement
	int16_t sZ,											// In:  Z coord placement
	int32_t lFuseTime)									// In:  ms before mine goes off (timed mine only)
{
	m_lFuseTime = lFuseTime;
	return Setup(sX, sY, sZ);
}

////////////////////////////////////////////////////////////////////////////////
// Setup new object - called by object that created this object
////////////////////////////////////////////////////////////////////////////////

int16_t CMine::Setup(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,											// In:  New x coord
	int16_t sY,											// In:  New y coord
	int16_t sZ)											// In:  New z coord
	{
	// Loop the Arming sound
	PlaySample(
		g_smidMineBeep,								// In:  sound to play
		SampleMaster::Weapon,						// In:  user volume adjustment category
		DistanceToVolume(sX, sY, sZ, MineSndHalfLife), // Position
		&m_siMineBeep,									// Out: Handle to sound so it can be stopped
		nullptr,												// Out: Sample duration in ms
		0,													// In:  Where to loop to
		-1,												// In:  Where to loop from
															// In:  If less than 1, the end + lLoopEndTime is used.
		false);											// In:  Call ReleaseAndPurge rather than Release at end

	
	int16_t sResult	=  CWeapon::Setup(sX, sY, sZ);
	if (sResult == SUCCESS)
		{
		sResult	= Init();
		}
	
	return sResult;
	}

////////////////////////////////////////////////////////////////////////////////
// Get all required resources
////////////////////////////////////////////////////////////////////////////////
int16_t CMine::GetResources(void)						// Returns 0 if successfull, non-zero otherwise
{
	int16_t sResult = SUCCESS;

   if (m_pImage == nullptr)
	{
      switch (type())
      {
        UNHANDLED_SWITCH;
			case CTimedMineID:
				sResult = rspGetResource(&g_resmgrGame, realm()->Make2dResPath(TIMEDMINE_FILE), &m_pImage, RFile::LittleEndian);
				break;

			case CProximityMineID:
				sResult = rspGetResource(&g_resmgrGame, realm()->Make2dResPath(PROXIMITYMINE_FILE), &m_pImage, RFile::LittleEndian);
				break;

			case CBouncingBettyMineID:
				sResult = rspGetResource(&g_resmgrGame, realm()->Make2dResPath(BOUNCINGBETTYMINE_FILE), &m_pImage, RFile::LittleEndian);
				break;

			case CRemoteControlMineID:
				sResult = rspGetResource(&g_resmgrGame, realm()->Make2dResPath(REMOTEMINE_FILE), &m_pImage, RFile::LittleEndian);
            break;
		}
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Free all resources
////////////////////////////////////////////////////////////////////////////////
int16_t CMine::FreeResources(void)						// Returns 0 if successfull, non-zero otherwise
{
	rspReleaseResource(&g_resmgrGame, &m_pImage);

   return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Helper inline to get a GUI, set its text to the value, and recompose it.
////////////////////////////////////////////////////////////////////////////////
inline
void SetText(					// Returns nothing.
	RGuiItem*	pguiRoot,	// In:  Root GUI.
	int32_t			lId,			// In:  ID of GUI to set text.
	int32_t			lVal)			// In:  Value to set text to.
	{
	RGuiItem*	pgui	= pguiRoot->GetItemFromId(lId);
	if (pgui != nullptr)
		{
		pgui->SetText("%i", lVal);
		pgui->Compose(); 
		}
	}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// Edit Modify
////////////////////////////////////////////////////////////////////////////////

int16_t CMine::EditModify(void)
{
	int16_t sResult = SUCCESS;
	RGuiItem* pGui = RGuiItem::LoadInstantiate(FullPathVD("res/editor/mine.gui"));
	if (pGui)
	{
		SetText(pGui, 7, m_lFuseTime);

		sResult = DoGui(pGui);
		if (sResult == 1)
		{
			m_lFuseTime = pGui->GetVal(7);
		}
	}
	delete pGui;

   return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CMine::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
	int16_t sResult = SUCCESS;

	if (sResult == SUCCESS)
	{
		// Load resources
		sResult = Setup(sX, sY, sZ);
	}
	else
	{
		sResult = FAILURE;
	}

	return sResult;
}


void CMine::EditRect(RRect* pRect)
{
   if (m_pImage != nullptr)
   {
      // Map from 3d to 2d coords
     realm()->Map3Dto2D(position.x, position.y, position.z,
                        pRect->sX, pRect->sY);

      // Center on image.
      pRect->sX	-= m_pImage->m_sWidth / 2;
      pRect->sY	-= m_pImage->m_sHeight / 2;
      pRect->sW	= m_pImage->m_sWidth;
      pRect->sH	= m_pImage->m_sHeight;
   }
}

void CMine::EditHotSpot(			// Returns nothiing.
   int16_t*	psX,			// Out: X coord of 2D hotspot relative to
                        // EditRect() pos.
   int16_t*	psY)			// Out: Y coord of 2D hotspot relative to
                        // EditRect() pos.
   {
   if (m_pImage != nullptr)
      {
      *psX	= m_pImage->m_sWidth / 2;
      *psY	= m_pImage->m_sHeight / 2;
      }
   else
      {
      CWeapon::EditHotSpot(psX, psY);
      }
   }
#endif // !defined(EDITOR_REMOVED)

////////////////////////////////////////////////////////////////////////////////
// Preload - basically trick the resource manager into caching resources 
//				 for this object so there won't be a delay the first time it is
//				 created.
////////////////////////////////////////////////////////////////////////////////

int16_t CMine::Preload(
	CRealm* prealm)				// In:  Calling realm.
{
	int16_t sResult;
	RImage*	pim;

	sResult = rspGetResource(&g_resmgrGame, prealm->Make2dResPath(TIMEDMINE_FILE), &pim, RFile::LittleEndian);
	if (sResult == SUCCESS)
		{
		rspReleaseResource(&g_resmgrGame, &pim);
		sResult = rspGetResource(&g_resmgrGame, prealm->Make2dResPath(PROXIMITYMINE_FILE), &pim, RFile::LittleEndian);
		if (sResult == SUCCESS)
			{
			rspReleaseResource(&g_resmgrGame, &pim);
			sResult = rspGetResource(&g_resmgrGame, prealm->Make2dResPath(BOUNCINGBETTYMINE_FILE), &pim, RFile::LittleEndian);
			if (sResult == SUCCESS)
				{
				rspReleaseResource(&g_resmgrGame, &pim);
				}
			}
		}

	CacheSample(g_smidBounceLaunch);
	CacheSample(g_smidBounceExplode);
	CacheSample(g_smidGrenadeExplode);
	CacheSample(g_smidMineBeep);
	CacheSample(g_smidMineSet);

	// This should convert each one of these images.
	return sResult; 
}

////////////////////////////////////////////////////////////////////////////////
// Explosion Message handler
////////////////////////////////////////////////////////////////////////////////

void CMine::OnExplosionMsg(Explosion_Message* pMessage)
{
  UNUSED(pMessage);
	// If we got blown up, go off in whatever manner this type
	// of mine would normally go off.
   switch (type())
	{
		// If its a bouncing betty, and hasn't already been
		// triggered, then trigger it to bounce up.
		case CBouncingBettyMineID:
			if (m_eState == State_Idle || m_eState == State_Armed) 
			{
				m_eState = State_Go;
				m_dVertVel = ms_dInitialBounceVelocity;
				m_lTimer = realm()->m_time.GetGameTime() + ms_lExplosionDelay;
			}
			break;

		// If its any other type, just make it explode if it
		// is not exploding already
		default:
			if (m_eState != State_Explode)
			{
				m_eState = State_Explode;
				m_lTimer = realm()->m_time.GetGameTime() + ms_lExplosionDelay;
			}
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Trigger Message handler
////////////////////////////////////////////////////////////////////////////////

void CMine::OnTriggerMsg(Trigger_Message* pMessage)
{
  UNUSED(pMessage);
	// If we are a remote control mine & we got the trigger message,
	// then blow up.
   if (type() == CRemoteControlMineID)
		m_eState = State_Explode;
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
