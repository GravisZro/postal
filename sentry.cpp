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
// sentry.cpp
// Project: Postal
//
// This module implements the automatic sentry gun
//
// History:
//		06/02/97 BRH	Created this Sentry gun from gunner.cpp.
//
//		06/05/97	JMI	Changed m_sHitPoints to m_stockpile.m_sHitPoints to 
//							accommodate new m_stockpile in base class, CThing3d (which
//							used to contain the m_sHitPoints).
//
//		06/13/97 BRH	Added Turret and Base for the Sentry.
//
//		06/16/97 BRH	Added blown up animation, dialog box for weapon selection
//							and settings.  Fixed positioning problems.
//
//		06/17/97 BRH	Added SetRangeToTarget call for weapons that 
//							require range adjustment.
//
//		06/18/97 BRH	Changed over to using GetRandom()
//
//		06/25/97	JMI	Now calls PrepareShadow() in Init() which loads and sets up
//							a shadow sprite.
//
//		06/30/97	JMI	Added override for EditRect() and EditHotSpot().
//							Now sets priority and layer for turret from base's values.
//
//					MJR	Replaced SAFE_GUI_REF with new GuiItem.h-defined macro.
//
//					BRH	Caches the sound effects during the static portion of
//							load so that the sound effect will be ready on any
//							level that has a sentry gun.
//
//		07/01/97 BRH	Added angular velocity to allow tuning of the rotation
//							rate of the sentry gun.  Still need to edit the dialog
//							box and EditModify to set the change.
//
//		07/02/97 BRH	Added angular velocity setting to edit modify dialog.
//
//		07/18/97	JMI	Got rid of bogus immitation PlaySample functions.
//							Now there is one PlaySample() function.  Also, you now
//							MUST specify a category and you don't have to specify a
//							SoundInstance ptr to specify a volume.
//
//		07/31/97	JMI	Changed m_sPriority to use 3D z position like CThin3d does.
//
//		08/01/97 BRH	Took out reference to animation hots since we are not
//							using those.
//
//		08/08/97	JMI	Now dynamically builds the weapons list for the 
//							EditModify().
//							Also, since it doesn't call the base class Update(), it
//							has to monitor the flamage sample.
//
//		08/11/97	JMI	Added transform for base, m_transBase.
//							Also, made UpdatePosition() bypass its logic when the 
//							animations are not yet set.
//
//		08/16/97 BRH	Added collision bits for the Sentry gun to pass to 
//							ShootWeapon.
//
//		08/18/97	JMI	Now plays impact animation when hit by bullets.
//
//		08/18/97	JMI	Changed State_Dead to call DeadRender3D() (which used to be
//							known/called as just another Render() overload).
//
//		08/20/97 BRH	Changed ricochet sounds from Destruction to Weapon volume
//							slider.
//
//		08/26/97 BRH	Changed sentry gun getting hit by bullets sound.
//
//		09/02/97	JMI	Changed use of Misc bit to Sentry bit.
//
//		09/03/97	JMI	Sentries now exclude CSmash::Bads and CSmash::Civilians.
//
////////////////////////////////////////////////////////////////////////////////

#include "sentry.h"

#include "realm.h"
#include "reality.h"
#include "game.h"
#include "SampleMaster.h"


////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

// Anim for when a barrel is hit by a bullet.
#define SENTRY_HIT_RES_NAME	"Ricochet.aan"

#define HULL_RADIUS				(m_sRadius / 2)

// Gets a GetRandom()om between -range / 2 and range / 2.
#define RAND_SWAY(sway)		((GetRandom() % sway) - sway / 2)

// Tunable bullet parameters.
#define MAX_BULLET_RANGE		400
#define MAX_BULLETS_PER_SEC	6

#define MS_BETWEEN_BULLETS		(1000 / MAX_BULLETS_PER_SEC)

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

// These are default values -- actually values are set using the editor!
double CSentry::ms_dTooCloseDistance = 190*190;	// Close enough to hit CDude
double CSentry::ms_dLongRange = 500*500;	// Squared distance (500 pixels away)
double CSentry::ms_dInRangeLow = 30*30;	// Squared distance to be in range with weapon
double CSentry::ms_dInRangeHigh = 230*230;// Squared distance to be in range with weapon
double CSentry::ms_dGravity = -19.5;		// Cheater gravity
double CSentry::ms_dBlowupVelocity = 190;	// Initial vertical velocity
int32_t CSentry::ms_lRandomAvoidTime = 200;	// Time to wander before looking again
int32_t CSentry::ms_lReseekTime = 1000;		// Do a 'find' again 
int32_t CSentry::ms_lWatchWaitTime = 2500;		// Time to watch shot go
int32_t CSentry::ms_lPatrolTime = 5000;		// Time to patrol before shooting
int32_t CSentry::ms_lDeathTimeout = 20000;	// Wait around after dying
int32_t CSentry::ms_lBurningRunTime = 50;		// Run this time before turning
int16_t CSentry::ms_sHitLimit = 150;			// Number of starting hit points
int16_t CSentry::ms_sBurntBrightness = -40;	// Brightness after being burnt
int32_t CSentry::ms_lMaxShootTime = MS_BETWEEN_BULLETS;		// Maximum in ms of continuous shooting.
int32_t CSentry::ms_lReselectDudeTime	= 3000;	// Time to go without finding a dude
															// before calling SelectDude() to find
															// possibly a closer one.
uint32_t CSentry::ms_u32WeaponIncludeBits = CSmash::Character | CSmash::Barrel | CSmash::Misc;
uint32_t CSentry::ms_u32WeaponDontcareBits = CSmash::Good | CSmash::Bad;
uint32_t CSentry::ms_u32WeaponExcludeBits = CSmash::SpecialBarrel | CSmash::Ducking | CSmash::Bad | CSmash::Civilian;

// Let this auto-init to 0
int16_t CSentry::ms_sFileCount;

CSentry::CSentry(void)
{
  m_sSuspend = 0;
  rotation.y = 0;
  position.x = position.y = position.z = m_dVel = m_dAcc = 0;
  m_panimCur = m_panimPrev = nullptr;
  m_panimCurBase	= nullptr;
  m_sNumRounds = 0;
  m_sRoundsPerShot = 0;
  m_lSqDistRange = 0;
  m_lShootDelay = 0;
  m_dAngularVelocity = 360.0;
}

CSentry::~CSentry(void)
{
  realm()->Scene()->RemoveSprite(&m_spriteBase);
  realm()->m_smashatorium.Remove(&m_smash);

  // Free resources
  FreeResources();
}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CSentry::Load(				// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,						// In:  File to load from
	bool bEditMode,					// In:  True for edit mode, false otherwise
	int16_t sFileCount,					// In:  File count (unique per file, never 0)
	uint32_t	ulFileVersion)				// In:  Version of file format to load.
{
	int16_t sResult = SUCCESS;
	// Call the base load function to get ID, position, etc.
	sResult = CDoofus::Load(pFile, bEditMode, sFileCount, ulFileVersion);
	if (sResult == SUCCESS)
	{
		// Load common data just once per file (not with each object)
		if (ms_sFileCount != sFileCount)
		{
			ms_sFileCount = sFileCount;

			CacheSample(g_smidShotSentry1);
			CacheSample(g_smidShotSentry2);
			CacheSample(g_smidShotSentry3);


			// Load static data		
			switch (ulFileVersion)
			{
				default:
				case 1:
					pFile->Read(&ms_dTooCloseDistance);
					pFile->Read(&ms_dLongRange);
					pFile->Read(&ms_dInRangeLow);
					pFile->Read(&ms_dInRangeHigh);
					pFile->Read(&ms_lRandomAvoidTime);
					pFile->Read(&ms_lReseekTime);
					pFile->Read(&ms_lWatchWaitTime);
					pFile->Read(&ms_lPatrolTime);
					break;
			}
		}

		// Load other values
		// for now, temporarily set values here to default values
		pFile->Read(&m_sNumRounds);
		pFile->Read(&m_sRoundsPerShot);
		pFile->Read(&m_lSqDistRange);
		pFile->Read(&m_lShootDelay);
      pFile->Read(reinterpret_cast<uint8_t*>(&m_eWeaponType));

		if (ulFileVersion > 24)
			pFile->Read(&m_dAngularVelocity);
//		m_sNumRounds = 32000;
//		m_sRoundsPerShot = 2;
//		m_lSqDistRange = 280*280;
//		m_eWeaponType = CShotGunID;
//		m_eWeaponType = CShotGunID;
//		m_lShootDelay = 500;

		// Make sure there were no file errors or format errors . . .
		if (!pFile->Error() && sResult == SUCCESS)
		{
			// Get resources
			sResult = GetResources();
		}
		else
		{
			sResult = FAILURE;
			TRACE("CSentry::Load(): Error reading from file!\n");
		}
	}
	else
	{
		TRACE("CSentry::Load():  CDoofus::Load() failed.\n");
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CSentry::Save(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to save to
	int16_t sFileCount)										// In:  File count (unique per file, never 0)
{
	// Swap the hotspot we want to save in.
   double dTempX = position.x;
   double dTempY = position.y;
   double dTempZ = position.z;
   position.x = m_base.x;
   position.y = m_base.y;
   position.z = m_base.z;

	int16_t sResult;

	// Call the base class save to save the instance ID, position, etc
	CDoofus::Save(pFile, sFileCount);

	// Save common data just once per file (not with each object)
	if (ms_sFileCount != sFileCount)
	{
		ms_sFileCount = sFileCount;

		// Save static data
		pFile->Write(&ms_dTooCloseDistance);
		pFile->Write(&ms_dLongRange);
		pFile->Write(&ms_dInRangeLow);
		pFile->Write(&ms_dInRangeHigh);
		pFile->Write(&ms_lRandomAvoidTime);
		pFile->Write(&ms_lReseekTime);
		pFile->Write(&ms_lWatchWaitTime);
		pFile->Write(&ms_lPatrolTime);
	}

	// Save additinal stuff here.
	pFile->Write(&m_sNumRounds);
	pFile->Write(&m_sRoundsPerShot);
	pFile->Write(&m_lSqDistRange);
	pFile->Write(&m_lShootDelay);
   pFile->Write(uint8_t(m_eWeaponType));
	pFile->Write(&m_dAngularVelocity);

	if (!pFile->Error())
	{
		sResult = SUCCESS;
	}
	else
	{
		TRACE("CSentry::Save() - Error writing to file\n");
		sResult = FAILURE;
	}

   position.x = dTempX;
   position.y = dTempY;
   position.z = dTempZ;

	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// Render - Override to skip over CDoofus::Render right to CCharacter::Render
////////////////////////////////////////////////////////////////////////////////

void CSentry::Render(void)
{

	// Do our own render of the stationary base
	uint16_t	u16CombinedAttributes;
	int16_t	sLightTally;
   GetEffectAttributes(m_base.x, m_base.z, u16CombinedAttributes, sLightTally);

	// Brightness.
	m_spriteBase.m_sBrightness	= m_sBrightness + sLightTally * gsGlobalBrightnessPerLightAttribute;

	// If no parent . . .
   if (!isChild())
		{
		// Reset transform back to start to set absolute rather than cummulative rotation
      m_trans.makeIdentity();
//		m_transBase.makeIdentity(); Not currently needed since the base does not change its transform.

      m_trans.Ry(rspMod360(rotation.y) );
      m_trans.Rz(rspMod360(rotation.z) );

		// Map from 3d to 2d coords
      realm()->Map3Dto2D(m_base.x, m_base.y, m_base.z,
                         m_spriteBase.m_sX2, m_spriteBase.m_sY2);

		// Layer should be based on info from attribute map.
      GetLayer(m_base.x, m_base.z, m_spriteBase.m_sLayer);

		// Priority is based on bottom edge of sprite which is currently the origin
      m_spriteBase.m_sPriority = m_base.z;

		// Update sprite in scene
      realm()->Scene()->UpdateSprite(&m_spriteBase);
		
		// Set transform.
		m_spriteBase.m_ptrans = &m_transBase;
		}

	ASSERT(m_panimCurBase != nullptr);

   m_spriteBase.m_pmesh = &m_panimCurBase->m_pmeshes->atTime(m_lAnimTime);
   m_spriteBase.m_psop = &m_panimCurBase->m_psops->atTime(m_lAnimTime);
   m_spriteBase.m_ptex = & m_panimCurBase->m_ptextures->atTime(m_lAnimTime);
   m_spriteBase.m_psphere = & m_panimCurBase->m_pbounds->atTime(m_lAnimTime);

	CCharacter::Render();

	// The turret is always at a just higher priority than the base.
   m_sPriority	= m_spriteBase.m_sPriority + 1;
   m_sLayer		= m_spriteBase.m_sLayer;

   Object::enqueue(SpriteUpdate); // Update sprite in scene
   //realm()->Scene()->UpdateSprite(&m_sprite);
}

////////////////////////////////////////////////////////////////////////////////
// Init - Call this after the resources are in place
////////////////////////////////////////////////////////////////////////////////

int16_t CSentry::Init(void)
{
	int16_t sResult = SUCCESS;

	// Prepare shadow (get resources and setup sprite).
	sResult	= PrepareShadow();

	// Init other stuff
	m_dVel = 0.0;
   rotation.y = 0.0;
	m_dShootAngle = 0.0;
	// Set to different starting state based on the design of the animation, but
	// for now, ok.  Then also set his current animation.
	m_state = CSentry::State_Wait;
	m_dAcc = ms_dAccUser;
	m_panimCur = &m_animStand;
	m_panimCurBase = &m_animBaseStand;
	m_lAnimTime = 0;
   m_lTimer = realm()->m_time.GetGameTime() + 500;

	// Set up the animations that are supposed to loop.
	m_animShoot.m_psops->SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);
	m_animShoot.m_pmeshes->SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);
	m_animShoot.m_ptextures->SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);
//	m_animShoot.m_phots->SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);
	m_animShoot.m_pbounds->SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);

	// Set up the base sprite so we can get the position
   m_spriteBase.m_pmesh = &m_panimCurBase->m_pmeshes->atTime(m_lAnimTime);
   m_spriteBase.m_psop = &m_panimCurBase->m_psops->atTime(m_lAnimTime);
   m_spriteBase.m_ptex = & m_panimCurBase->m_ptextures->atTime(m_lAnimTime);
   m_spriteBase.m_psphere = & m_panimCurBase->m_pbounds->atTime(m_lAnimTime);
   m_spriteBase.m_ptrans = & m_panimCurBase->m_ptransRigid->atTime(m_lAnimTime);

   // Update base and turret position via position.x, Y, & Z.
	UpdatePosition();

	m_stockpile.m_sHitPoints = ms_sHitLimit;

	m_smash.m_bits = CSmash::Bad | CSmash::Sentry;
   m_smash.m_pThing = this;

	m_sBrightness = 0;	// Default Brightness level

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Position the base and turret based on position.x, Y, & Z.
////////////////////////////////////////////////////////////////////////////////
void CSentry::UpdatePosition(void)
	{
	// Move Base and Turret into position
   m_base.x = position.x;
   m_base.y = position.y;
   m_base.z = position.z;

	if (m_panimCurBase != nullptr)
		{
		// Below was copied from DetachChild in Thing3d

		// Update its position.
		// set up translation based on the combined last character and child transforms
		RTransform transChildAbsolute;
      RTransform*	ptransRigid	= &m_panimCurBase->m_ptransRigid->atTime(m_lAnimTime);

		// Apply child and parent to transChildAbs
      transChildAbsolute.Mul(m_trans, *ptransRigid);
		// Set up pt at origin for child.
		Vector3D pt3Src = {0, 0, 0, 1};
		Vector3D pt3Dst;
		// Get last transition position by mapping origin.
      realm()->Scene()->TransformPtsToRealm(&transChildAbsolute, &pt3Src, &pt3Dst, 1);
		// Set child position to character's position offset by rigid body's realm offset.
      position.x = m_base.x + pt3Dst.x();
      position.y = m_base.y + pt3Dst.y();
      position.z = m_base.z + pt3Dst.z();
		}
	}


////////////////////////////////////////////////////////////////////////////////
// Startup object
////////////////////////////////////////////////////////////////////////////////
void CSentry::Startup(void)								// Returns 0 if successfull, non-zero otherwise
{
	// Set the current height, previous time, and Nav Net
	CDoofus::Startup();

	// Init other stuff
   Init();
}

////////////////////////////////////////////////////////////////////////////////
// Suspend object
////////////////////////////////////////////////////////////////////////////////
void CSentry::Suspend(void)
{
	// Call base class suspend, and add anything else here if necessary
	CDoofus::Suspend();
}

////////////////////////////////////////////////////////////////////////////////
// Resume object
////////////////////////////////////////////////////////////////////////////////
void CSentry::Resume(void)
{
	// Call the base class resume and add anything else you suspended
	CDoofus::Resume();
}


////////////////////////////////////////////////////////////////////////////////
// Update object
////////////////////////////////////////////////////////////////////////////////
void CSentry::Update(void)
{
#ifdef UNUSED_VARIABLE
	int16_t sHeight = m_sPrevHeight;
#endif
   milliseconds_t lThisTime;
   milliseconds_t lTimeDifference;
	int32_t lSqDistanceToDude = 0;
	int16_t sTargetAngle;
	int16_t sAngleCCL;
	int16_t sAngleCL;
	int16_t sAngleDistance;
	double dRotDistance;
	bool bShootThisTime = false;

	if (!m_sSuspend)
	{
		// Get new time
      lThisTime = realm()->m_time.GetGameTime();
		lTimeDifference = lThisTime - m_lPrevTime;

		// Calculate elapsed time in seconds
		double dSeconds = (double)(lThisTime - m_lPrevTime) / 1000.0;

		// Check for new messages that may change the state
		ProcessMessages();


		switch(m_state)
		{
        UNHANDLED_SWITCH;
			case CSentry::State_Wait:
				if (lThisTime > m_lTimer)
				{
					m_state = CSentry::State_Guard;
				}
	
				// Update sphere.
            m_smash.m_sphere.sphere.X			= m_base.x;
            m_smash.m_sphere.sphere.Y			= m_base.y;
            m_smash.m_sphere.sphere.Z			= m_base.z;
				m_smash.m_sphere.sphere.lRadius	= 30; //m_spriteBase.m_sRadius;

				// Update the smash.
            realm()->m_smashatorium.Update(&m_smash);
				break;

//-----------------------------------------------------------------------
// Guard - normal operation
//-----------------------------------------------------------------------

			case CSentry::State_Guard:

				// Target closest Dude
				SelectDude();

				sTargetAngle = CDoofus::FindDirection();
            sAngleCCL = rspMod360(sTargetAngle - rotation.y);
            sAngleCL  = rspMod360((360 - sTargetAngle) + rotation.y);
				sAngleDistance = MIN(sAngleCCL, sAngleCL);
				// Calculate the amount it can turn this time.
				dRotDistance = dSeconds * m_dAngularVelocity;
				// Don't over rotate - if it is within reach this time, then its OK to shoot
				if (sAngleDistance < dRotDistance)
				{
					dRotDistance = sAngleDistance;
					bShootThisTime = true;
				}
				if (sAngleCCL < sAngleCL)
				// Rotate Counter Clockwise
				{
					m_dShootAngle = rspMod360(m_dShootAngle + dRotDistance);
               rotation.y = m_dAnimRot = m_dShootAngle;
				}
				else
				// Rotate Clockwise
				{
					m_dShootAngle = rspMod360(m_dShootAngle - dRotDistance);
               rotation.y = m_dAnimRot = m_dShootAngle;
				}

				// Turn to him directly for now.
//				rotation.y = m_dAnimRot = m_dShootAngle = CDoofus::FindDirection();
				lSqDistanceToDude = CDoofus::SQDistanceToDude();

				if (bShootThisTime && 
                m_dude &&
				    lSqDistanceToDude < m_lSqDistRange &&
					 lThisTime > m_lTimer &&
					 m_sNumRounds > 0)
				{

					if (TryClearShot(m_dShootAngle, 3))
					{
						m_panimCur = &m_animShoot;
						m_lAnimTime += lTimeDifference;
                  PrepareWeapon();
                  if (m_weapon)
                     m_weapon->SetRangeToTarget(rspSqrt(lSqDistanceToDude));
						ShootWeapon(ms_u32WeaponIncludeBits, ms_u32WeaponDontcareBits, ms_u32WeaponExcludeBits);
						m_sNumRounds--;
						m_lTimer = lThisTime + m_lShootDelay;
					}
					else
					{
						m_lAnimTime = 0;
						m_panimCur = &m_animStand;
					}
				}
				break;

//-----------------------------------------------------------------------
// Blownup - You were blown up so pop up into the air and come down dead
//-----------------------------------------------------------------------

				case CSentry::State_BlownUp:
					// Make her animate
					m_lAnimTime += lTimeDifference;

					if (!WhileBlownUp())
						m_state = State_Dead;
					else
					{
						if (lThisTime > m_lTimer && m_sNumRounds > 0)
						{
                     m_dShootAngle = rotation.y;
							PrepareWeapon();
							ShootWeapon(ms_u32WeaponIncludeBits, ms_u32WeaponDontcareBits, ms_u32WeaponExcludeBits);
							m_sNumRounds--;
							m_lTimer = lThisTime + m_lShootDelay;
						}
						UpdateFirePosition();
					}

					break;


//-----------------------------------------------------------------------
// Dead - You are dead, so lay there and decompose, then go away
//-----------------------------------------------------------------------

            case CSentry::State_Dead:
					// Render current dead frame into background to stay.
               realm()->Scene()->DeadRender3D(
                  realm()->Hood()->m_pimBackground,		// Destination image.
						&m_spriteBase,					// Tree of 3D sprites to render.
                  realm()->Hood());							// Dst clip rect.

					CDoofus::OnDead();
              Object::enqueue(SelfDestruct);
               return;
		}


		// Here's a little piece of CCharacter::Update() since this class
		// doesn't use the base class update.

		// If we have a weapon sound play instance . . .
		if (m_siLastWeaponPlayInstance)
			{
			// If time has expired . . .
         if (realm()->m_time.GetGameTime() > m_lStopLoopingWeaponSoundTime)
				{
				// Stop looping the sound.
				StopLoopingSample(m_siLastWeaponPlayInstance);
				// Forget about it.
				m_siLastWeaponPlayInstance	= 0;
				}
			}

		// Save time for next time
		m_lPrevTime = lThisTime;
		m_lAnimPrevUpdateTime = m_lAnimTime;
	}
}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CSentry::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
	int16_t sResult = SUCCESS;

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
	else
	{
		sResult = FAILURE;
	}

	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// Edit Move
////////////////////////////////////////////////////////////////////////////////
int16_t CSentry::EditMove(int16_t sX, int16_t sY, int16_t sZ)
{
	int16_t sResult = CDoofus::EditMove(sX, sY, sZ);

	UpdatePosition();

	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// Give Edit a rectangle around this object
////////////////////////////////////////////////////////////////////////////////
void CSentry::EditRect(RRect* pRect)
	{
	// Swap the hotspot and anim we want to get the rect of in.
   double dTempX = position.x;
   double dTempY = position.y;
   double dTempZ = position.z;
	
   position.x = m_base.x;
   position.y = m_base.y;
   position.z = m_base.z;

	CAnim3D*	panimTemp	= m_panimCur;

	m_panimCur				= m_panimCurBase;

	// Call base class.
	CDoofus::EditRect(pRect);

	// Restore.
   position.x	= dTempX;
   position.y	= dTempY;
   position.z	= dTempZ;

	m_panimCur	= panimTemp;
	}

////////////////////////////////////////////////////////////////////////////////
// Called by editor to get the hotspot of an object in 2D.
// (virtual (Overridden here)).
////////////////////////////////////////////////////////////////////////////////
void CSentry::EditHotSpot(			// Returns nothiing.
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
int16_t CSentry::EditModify(void)
{
	int16_t sResult = SUCCESS;
	RGuiItem* pGuiItem = nullptr;
	RGuiItem* pGui = RGuiItem::LoadInstantiate(FullPathVD("res/editor/sentry.gui"));
	if (pGui)
	{
		RListBox* pWeaponList = (RListBox*) pGui->GetItemFromId(4);
		REdit* peditAmmoCount = (REdit*) pGui->GetItemFromId(101);
		REdit* peditShotDelay = (REdit*) pGui->GetItemFromId(102);
		REdit* peditRange     = (REdit*) pGui->GetItemFromId(103);
		REdit* peditRotVel    = (REdit*) pGui->GetItemFromId(104);
		if (	pWeaponList && peditAmmoCount && peditShotDelay && peditRange && peditRotVel) 
		{
			// Verify these are the type we think they are before accessing type specific
			// members.
			ASSERT(pWeaponList->m_type == RGuiItem::ListBox);
			ASSERT(peditAmmoCount->m_type == RGuiItem::Edit);
			ASSERT(peditShotDelay->m_type == RGuiItem::Edit);
			ASSERT(peditRange->m_type == RGuiItem::Edit);
			ASSERT(peditRotVel->m_type == RGuiItem::Edit);

#if 0
			// Show which weapon is currently selected
			pWeaponList->SetSel(pGui->GetItemFromId(m_eWeaponType));
#else
			// Empty list box.  We don't want to modify the .GUI resource just yet
			// so we don't screw up people using the current .EXE on the server.
			pWeaponList->RemoveAll();

			// Fill in the list box with current available weapons.
			int16_t	i;
			for (i = 0; i < NumWeaponTypes; i++)
				{
				if (	(i != DeathWad || CStockPile::ms_sEnableDeathWad) &&
						(i != DoubleBarrel || CStockPile::ms_sEnableDoubleBarrel) &&
						(i != ProximityMine) &&
						(i != TimedMine) &&
						(i != RemoteControlMine) &&
						(i != BouncingBettyMine) )
					{
					pGuiItem	= pWeaponList->AddString(ms_awdWeapons[i].pszName);
					if (pGuiItem != nullptr)
						{
						// Store class ID so we can determine user selection
						pGuiItem->m_lId	= ms_awdWeapons[i].id;
						}
					}
				}

			pWeaponList->AdjustContents();

			// Show which weapon is currently selected
			pGuiItem	= pGui->GetItemFromId(m_eWeaponType);
			if (pGuiItem)
				{
				pWeaponList->SetSel(pGuiItem);
				pWeaponList->EnsureVisible(pGuiItem);
				}

#endif

			// Set current Ammo Count
			peditAmmoCount->SetText("%d", m_sNumRounds);
			// Reflect changes
			peditAmmoCount->Compose();

			// Set current range
			peditRange->SetText("%d", rspSqrt(m_lSqDistRange));
			// Reflect changes
			peditRange->Compose();

			// Set current delay
			peditShotDelay->SetText("%d", m_lShootDelay);
			// Reflect changes
			peditShotDelay->Compose();

			// Set current rotational velocity
			peditRotVel->SetText("%d", (int16_t) m_dAngularVelocity);
			peditRotVel->Compose();
				
			sResult = DoGui(pGui);
			if (sResult == 1)
			{
				{
					RGuiItem* pSelection = pWeaponList->GetSel();
					if (pSelection)
					{
                  m_eWeaponType	= ClassIDType(pSelection->m_lId);
					}

					m_sNumRounds = RSP_SAFE_GUI_REF(peditAmmoCount, GetVal());
					int32_t lDist = RSP_SAFE_GUI_REF(peditRange, GetVal());
					m_lSqDistRange = lDist * lDist;
					m_lShootDelay = RSP_SAFE_GUI_REF(peditShotDelay, GetVal());
					m_dAngularVelocity = (double) RSP_SAFE_GUI_REF(peditRotVel, GetVal());
				}
			}
		}
	}
	delete pGui;

   return SUCCESS;
}
#endif // !defined(EDITOR_REMOVED)

////////////////////////////////////////////////////////////////////////////////
// Get all required resources
////////////////////////////////////////////////////////////////////////////////
int16_t CSentry::GetResources(void)						// Returns 0 if successfull, non-zero otherwise
{
  bool bResult = true;
  bResult &= m_animShoot.Get("sentry_shoot");
  bResult &= m_animStand.Get("sentry_still");
  bResult &= m_animDie.Get("sentry_damaged");
  bResult &= m_animBaseStand.Get("stand_still");
  bResult &= m_animBaseDie.Get("stand_damaged");
  return bResult ? SUCCESS : FAILURE;;
}


////////////////////////////////////////////////////////////////////////////////
// Free all resources
////////////////////////////////////////////////////////////////////////////////
int16_t CSentry::FreeResources(void)						// Returns 0 if successfull, non-zero otherwise
{
	m_animShoot.Release();
	m_animStand.Release();
	m_animDie.Release();
	m_animBaseStand.Release();
	m_animBaseDie.Release();

   return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Message handlers
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Shot Message
////////////////////////////////////////////////////////////////////////////////

void CSentry::OnShotMsg(Shot_Message* pMessage)
{
	// Audible and visual feedback.
	int16_t sSound = GetRandom() % 3;

	switch (sSound)
	{
		case 0:
			PlaySample(g_smidShotSentry1, SampleMaster::Weapon);
			break;

		case 1:
			PlaySample(g_smidShotSentry2, SampleMaster::Weapon);
			break;

		case 2:
			PlaySample(g_smidShotSentry3, SampleMaster::Weapon);
			break;
	}

	// X/Z position depends on angle of shot (it is opposite).
	int16_t	sDeflectionAngle	= rspMod360(pMessage->sAngle + 180);
   double	dHitX	= position.x + COSQ[sDeflectionAngle] * HULL_RADIUS + RAND_SWAY(4);
   double	dHitZ	= position.z - SINQ[sDeflectionAngle] * HULL_RADIUS + RAND_SWAY(4);
	StartAnim(
		SENTRY_HIT_RES_NAME, 
		dHitX, 
      position.y + RAND_SWAY(10),
		dHitZ,
		false);

	// Fow now we made the sentry bulletproof, the only
	// way it can be destroyed is by blowing it up.
}

////////////////////////////////////////////////////////////////////////////////
// Explosion message
////////////////////////////////////////////////////////////////////////////////

void CSentry::OnExplosionMsg(Explosion_Message* pMessage)
{
	if (
	    m_state != State_BlownUp	&&
		 m_state != State_Die		&& 
		 m_state != State_Dead)
	{
		CCharacter::OnExplosionMsg(pMessage);
		
		m_ePreviousState = m_state;
		m_state = State_BlownUp;
		m_panimPrev = m_panimCur;
		m_panimCur = &m_animDie;
		m_lAnimTime = 0;
		m_stockpile.m_sHitPoints = 0;
      m_lTimer = realm()->m_time.GetGameTime();

		m_dExtHorzVel *= 1.4; //2.5;
		m_dExtVertVel *= 1.1; //1.4;
		// Send it spinning.
		m_dExtRotVelY	= GetRandom() % 720;
		m_dExtRotVelZ	= GetRandom() % 720;

		// Show the gun as damaged
		m_panimCurBase = &m_animBaseDie;
		m_panimCur = &m_animDie;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Burning message
////////////////////////////////////////////////////////////////////////////////

void CSentry::OnBurnMsg(Burn_Message* pMessage)
{
  UNUSED(pMessage);
	// For now we made the sentry fireproof, the only
	// way it can be destroyed is by blowing it up.
}


////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
