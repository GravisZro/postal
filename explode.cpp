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
// explode.cpp
// Project: Postal
//
// This module implements the CExplode weapon class which is an unguided
//	rocket missile.
// 
//
// History:
//		01/17/97 BRH	Started this weapon object.
//
//		01/23/97 BRH	Updated the time to GetGameTime rather than using
//							real time.
//
//		02/04/97	JMI	Changed LoadDib() call to Load() (which now supports
//							loading of DIBs).
//
//		02/06/97 BRH	Added RAnimSprite animation of the explosion for now.  
//							We are going to do an Alpha effect on the explosion, so
//							there are two animations, one of the image and one of
//							the Alpha information stored as a BMP8 animation.  When
//							the Alpha effect is ready, we will pass a frame from
//							each animation to a function to draw it.
//
//		02/06/97 BRH	Fixed problem with timer.  Since all Explosion objects
//							are using the same resource managed animation, they cannot
//							use the animation timer, they have to do the timing 
//							themselves.
//
//		02/07/97 BRH	Changed the sprite from CSprite2 to CSpriteAlpha2 for
//							the Alpha Blit effect.
//
//		02/10/97	JMI	rspReleaseResource() now takes a ptr to a ptr.
//
//		02/18/97 BRH	Changed the explosion to use the Channel Animations
//							rather than the RAnimSprite animations.
//
//		02/19/97 BRH	Empties the message queue in update so it doesn't fill up.
//							Also checks for collisions with other Characters and sends
//							them an explosion message.
//
//		02/23/97 BRH	Explosion now checks for all things that it blew up, not
//							just the first thing in the list.
//
//		02/23/97 BRH	Added Preload() function so that explosions are cached
//							by the resource manager before play begins.
//
//		02/24/97 BRH	Added Map3Dto2D so that the explosions were mapping the Y
//							coordinate also, before they were assuming they were on the
//							ground so explosions in the air weren't working correctly.
//
//		02/24/97	JMI	No longer sets the m_type member of the m_sprite b/c it
//							is set by m_sprite's constructor.
//
//		03/05/97 BRH	Added center of and velocity of explosion to the explosion
//							message.
//
//		03/13/97	JMI	Load now takes a version number.
//
//		03/17/97	JMI	Now includes CSmash::Item in the things that can be
//							exploded.
//
//		04/10/97 BRH	Updated this to work with the new multi layer attribute
//							maps.
//
//		04/20/97 BRH	Added an additional parameter to Setup to allow the
//							explosion to use different animations. In this case we 
//							want the standard explosion for rockets and barrels, and
//							a special one for the grenades.  This will allow you to
//							pass a number to incicate which animation to use.
//
//		04/21/97 BRH	Added second animation file and changed filename of second
//							asset to match.
//
//		04/23/97	JMI	CExplode no longer puts it's m_smash in the smashatorium.
//							Now sends messages to Characters, Miscs, Barrels, and 
//							Mines.
//
//		05/29/97	JMI	Removed ASSERT on realm()->m_pAttribMap which no longer
//							exists.
//
//		06/07/97 BRH	Added smoke to the end of all explosions.
//
//		06/11/97 BRH	Pass the shooter ID on through the explosion message.
//
//		06/18/97 BRH	Changed over to using GetRandom()
//
//		06/26/97 BRH	Added CSmash::AlmostDead bits to the explosion check
//							so that writhing guys can be blown up.
//
//		07/09/97	JMI	Now uses realm()->Make2dResPath() to get the fullpath
//							for 2D image components.
//
//		07/09/97	JMI	Changed Preload() to take a pointer to the calling realm
//							as a parameter.
//
//		07/27/97	JMI	Changed to use Z position (i.e., X/Z plane) instead of
//							Y2 position (i.e., viewing plane) position for draw 
//							priority.
//
//		07/30/97	JMI	Added m_u16ExceptID (an ID to except when sending 
//							explosion messages).
//
//		08/15/97 BRH	Fixed problem with stationary smoke which had been
//							started under ground.
//
//		08/28/97 BRH	Now caches the grenade explosion animation as well in
//							its Preload function.
//
//		09/02/97	JMI	Now targets CSmash::Sentry too.
//
//		09/03/97	JMI	Now marks Civilian as a dont care bit.
//
////////////////////////////////////////////////////////////////////////////////

#include "explode.h"

#include "realm.h"
#include "dude.h"
#include "game.h"
#include "reality.h"
#include "fire.h"

////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

//#define IMAGE_FILE			"res/explode.bmp"
//#define ANIM_FILE				"2d/explode.anm"
//#define ALPHA_FILE			"2d/explode_a.anm"

// Minimum elapsed time (in milliseconds)
//#define MIN_ELAPSED_TIME	10

#define AA_FILE				"explo.aan"
#define GE_FILE				"GExplo.aan"

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

// These are default values -- actually values are set using the editor!

// Let this auto-init to 0
int16_t CExplode::ms_sFileCount;
int16_t CExplode::ms_sBlastRadius = 30;
int16_t CExplode::ms_sProjectVelocity = 180;




CExplode::CExplode(void)
{
  m_sSuspend = 0;
}

CExplode::~CExplode(void)
{
  // Remove sprite from scene (this is safe even if it was already removed!)
  //realm()->Scene()->RemoveSprite(&m_sprite);
  realm()->m_smashatorium.Remove(&m_smash);

  // Free resources
  FreeResources();
}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CExplode::Load(									// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to load from
	bool bEditMode,										// In:  True for edit mode, false otherwise
	int16_t sFileCount,										// In:  File count (unique per file, never 0)
	uint32_t	ulFileVersion)									// In:  Version of file format to load.
	{
	int16_t sResult = CThing::Load(pFile, bEditMode, sFileCount, ulFileVersion);

	if (sResult == SUCCESS)
	{
		// Load common data just once per file (not with each object)
		if (ms_sFileCount != sFileCount)
		{
			ms_sFileCount = sFileCount;
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
			TRACE("CExplode::Load(): Error reading from file!\n");
		}
	}
	else
	{
		TRACE("CExplode::Load(): CThing::Load() failed.\n");
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CExplode::Save(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to save to
	int16_t sFileCount)										// In:  File count (unique per file, never 0)
{
	int16_t sResult	= CThing::Save(pFile, sFileCount);
	if (sResult == SUCCESS)
		{
		// Save common data just once per file (not with each object)
		if (ms_sFileCount != sFileCount)
			{
			ms_sFileCount = sFileCount;

			// Save static data
			}
		}
	else
		{
		TRACE("CExplode::Save(): CThing::Save() failed.\n");
		}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Suspend object
////////////////////////////////////////////////////////////////////////////////
void CExplode::Suspend(void)
{
	m_sSuspend++;
}


////////////////////////////////////////////////////////////////////////////////
// Resume object
////////////////////////////////////////////////////////////////////////////////
void CExplode::Resume(void)
{
	m_sSuspend--;

	// If we're actually going to start updating again, we need to reset
	// the time so as to ignore any time that passed while we were suspended.
	// This method is far from precise, but I'm hoping it's good enough.
	if (m_sSuspend == 0)
		m_lPrevTime = realm()->m_time.GetGameTime();
}


////////////////////////////////////////////////////////////////////////////////
// Update object
////////////////////////////////////////////////////////////////////////////////
void CExplode::Update(void)
{
	if (!m_sSuspend)
	{
		// Since we don't process any messages, empty the queue
      m_MessageQueue.clear();

		// Get new time
      milliseconds_t lThisTime = realm()->m_time.GetGameTime();
		
		if (m_lTimer < m_pAnimChannel->TotalTime())
		{
			m_lTimer += lThisTime - m_lPrevTime;
			m_lPrevTime = lThisTime;
		}
		else
		{
         int16_t a;
			for (a = 0; a < 8; a++)
			{
           managed_ptr<CFire> pSmoke = realm()->AddThing<CFire>();
            if (pSmoke)
               pSmoke->Setup(m_position.x - 4 + GetRandom() % 9, MAX(m_position.y-20, 0.0), m_position.z - 4 + GetRandom() % 9, 4000, true, CFire::Smoke);
			}

         Object::enqueue(SelfDestruct);
			return;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Render object
////////////////////////////////////////////////////////////////////////////////
void CExplode::Render(void)
{
	CAlphaAnim* pAnim = (CAlphaAnim*) m_pAnimChannel->GetAtTime(m_lTimer);

	if (pAnim)
	{
		// No special flags
      m_sInFlags = 0; //CSprite::InXrayable;

		// Map from 3d to 2d coords
//		m_sX2 = m_position.x + pAnim->m_sX;
//		m_sY2 = m_position.z + pAnim->m_sY;
      Map3Dto2D((int16_t) (m_position.x + pAnim->m_sX), (int16_t) m_position.y, (int16_t) (m_position.z + pAnim->m_sY), &m_sX2, &m_sY2);

		// Priority is based on our Z position.
      m_sPriority = m_position.z;

		// Layer should be based on info we get from attribute map.
      m_sLayer = CRealm::GetLayerViaAttrib(realm()->GetLayer((int16_t) m_position.x, (int16_t) m_position.z));

		// Copy the color info and the alpha channel to the Alpha Sprite
      m_pImage = &(pAnim->m_imColor);
      m_pimAlpha = &(pAnim->m_pimAlphaArray[0]);

      Object::enqueue(SpriteUpdate); // Update sprite in scene
	}
}

////////////////////////////////////////////////////////////////////////////////
// Setup - Called by the object that is creating this explosion to set its
//			  position and initial settings
////////////////////////////////////////////////////////////////////////////////

int16_t CExplode::Setup(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ,												// In:  New z coord
   managed_ptr<CThing3d> shooter,									// In:  Who is responsible for this explosion
	int16_t sAnim)											// In:  Which animation to use
{
	int16_t sResult = SUCCESS;
	
	// Use specified position
   m_position.x = (double)sX;
   m_position.y = (double)sY;
   m_position.z = (double)sZ;
   m_lPrevTime = realm()->m_time.GetGameTime();
	m_lTimer = 0;
   m_shooter = shooter;

	// Load resources
	sResult = GetResources(sAnim);

		// Update sphere.
   m_smash.m_sphere.sphere.X			= m_position.x;
   m_smash.m_sphere.sphere.Y			= m_position.y;
   m_smash.m_sphere.sphere.Z			= m_position.z;
	m_smash.m_sphere.sphere.lRadius	= ms_sBlastRadius;

	// Update the smash.
	ASSERT (realm() != nullptr);
//	realm()->m_smashatorium.Update(&m_smash);

	m_smash.m_bits		= 0;
   m_smash.m_pThing = this;

	// See who we blew up and send them a message
	CSmash* pSmashed = nullptr;
	GameMessage msg;
	msg.msg_Explosion.eType = typeExplosion;
	msg.msg_Explosion.sPriority = 0;
	msg.msg_Explosion.sDamage = 100;
   msg.msg_Explosion.sX = (int16_t) m_position.x;
   msg.msg_Explosion.sY = (int16_t) m_position.y;
   msg.msg_Explosion.sZ = (int16_t) m_position.z;
	msg.msg_Explosion.sVelocity = ms_sProjectVelocity;
   msg.msg_Explosion.shooter = m_shooter;
	realm()->m_smashatorium.QuickCheckReset(
		&m_smash, 
		CSmash::Character | CSmash::Misc | CSmash::Barrel | CSmash::Mine | CSmash::AlmostDead | CSmash::Sentry,
		CSmash::Good | CSmash::Bad | CSmash::Civilian,
		0);
	while (realm()->m_smashatorium.QuickCheckNext(&pSmashed))
		{
		ASSERT(pSmashed->m_pThing);
		// If not the excepted thing . . .
      if (pSmashed->m_pThing != m_except)
			{
         SendThingMessage(msg, pSmashed->m_pThing);
			}
		}

	return sResult;
}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CExplode::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
	int16_t sResult = SUCCESS;
	
	// Use specified position
   m_position.x = (double)sX;
   m_position.y = (double)sY;
   m_position.z = (double)sZ;
	m_lTimer = realm()->m_time.GetGameTime() + 1000;
	m_lPrevTime = realm()->m_time.GetGameTime();

	// Load resources
	sResult = GetResources();

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to modify object
////////////////////////////////////////////////////////////////////////////////
int16_t CExplode::EditModify(void)
{
	return SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to move object to specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CExplode::EditMove(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
   m_position.x = (double)sX;
   m_position.y = (double)sY;
   m_position.z = (double)sZ;

	return SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to update object
////////////////////////////////////////////////////////////////////////////////
void CExplode::EditUpdate(void)
{
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to render object
////////////////////////////////////////////////////////////////////////////////
void CExplode::EditRender(void)
{
	// In some cases, object's might need to do a special-case render in edit
	// mode because Startup() isn't called.  In this case it doesn't matter, so
	// we can call the normal Render().
	Render();
}
#endif // !defined(EDITOR_REMOVED)

////////////////////////////////////////////////////////////////////////////////
// Get all required resources
////////////////////////////////////////////////////////////////////////////////
int16_t CExplode::GetResources(int16_t sAnim)						// Returns 0 if successfull, non-zero otherwise
{
	int16_t sResult = SUCCESS;

	if (sAnim == 0)
		sResult = rspGetResource(&g_resmgrGame, realm()->Make2dResPath(AA_FILE), &m_pAnimChannel, RFile::LittleEndian);
	else
		sResult = rspGetResource(&g_resmgrGame, realm()->Make2dResPath(GE_FILE), &m_pAnimChannel, RFile::LittleEndian);

	if (sResult != SUCCESS)
		TRACE("CExplosion::GetResources - Error getting explosion animation\n");

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Free all resources
////////////////////////////////////////////////////////////////////////////////
int16_t CExplode::FreeResources(void)						// Returns 0 if successfull, non-zero otherwise
{
	rspReleaseResource(&g_resmgrGame, &m_pAnimChannel);
	return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Preload - basically trick the resource manager into caching a CExplode 
//				 animation before play begins so that when an explosion occurs for
//				 the first time, there won't be a delay.
////////////////////////////////////////////////////////////////////////////////

int16_t CExplode::Preload(
	CRealm* prealm)				// In:  Calling realm.
{
	ChannelAA* pRes;
	int16_t sResult = rspGetResource(&g_resmgrGame, prealm->Make2dResPath(AA_FILE), &pRes, RFile::LittleEndian);
	rspReleaseResource(&g_resmgrGame, &pRes);
	sResult = rspGetResource(&g_resmgrGame, prealm->Make2dResPath(GE_FILE), &pRes, RFile::LittleEndian);
	rspReleaseResource(&g_resmgrGame, &pRes);
	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
