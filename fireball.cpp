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
// fireball.cpp
// Project: Nostril (aka Postal)
//
// This module implements the CFireball weapon class which is a burning flame
//	for several different effects and weapons.
// 
//
// History:
//		01/17/97 BRH	Started this fireball ammo for use in making a flamethrower
//
//		04/25/97	JMI	PreLoad() was not properly assigning into sResult.
//
//		04/29/97	JMI	Changed name of ProcessMessages() to 
//							ProcessFireballMessages() to avoid conflicts with 
//							CWeapon's ProcessMessages().
//							Also, Update() was not moving the m_smash (it was merely
//							set the one time in Init()) which was causing it to not
//							collide with anything (I guess it might've collided with
//							things hovering around the upper, left corner of the realm).
//
//		05/29/97	JMI	Removed ASSERT on realm()->m_pAttribMap which no longer
//							exists.
//
//		06/11/97 BRH	Passed on shooter ID to the message it sends when it burns
//							someone.
//
//		06/17/97	JMI	Converted all occurrences of rand() to GetRand() and
//							srand() to SeedRand().
//
//		07/02/97 BRH	Added CFirestream object that creates several CFireball
//							objects.
//
//		07/04/97 BRH	Added auto alpha based on the time to live.
//
//		07/07/97 BRH	Fixed casing on MIN to compile on both PC and Mac.
//
//		07/08/97	JMI	Put in code in Render() to avoid an unlikely divide by zero.
//							Still have a divide by zero release mode problem though.
//							Also, there was initialization typo in CFireball::Setup().
//
//		07/08/97	JMI	Fixed Render() to distribute the homogeneous alpha level
//							better.  Still needs tuning.
//
//					JMI	Made heavier again.
//
//		07/09/97	JMI	Now uses realm()->Make2dResPath() to get the fullpath
//							for 2D image components.
//
//		07/09/97	JMI	Changed Preload() to take a pointer to the calling realm
//							as a parameter.
//
//		07/10/97	JMI	Now uses alpha mask and level for animation.
//
//		07/12/97 BRH	Added IsPathClear check to path of fireball since its
//							velocity is so high that it can pass through walls
//							otherwise.
//
//		07/13/97 BRH	Added a check to the Firestream creation of the
//							fireballs to make sure that the iniial position is
//							not in a wall.  If it is, then the other two fireballs
//							with a further offset should not be created, otherwise
//							it will shoot through walls.  
//
//							Also changed the alpha to use 1 mask and tuned the levels
//							so that you can see it at the gun end.
//
//		07/14/97	JMI	Now will detach itself from its parent via 
//							m_sprite.m_psprParent->RemoveChild() in the rare event 
//							that it is still parented going into the Render().
//							Also, threw in some random to the rotation.y before they are
//							separated from the CFirestream.
//
//		07/27/97	JMI	Changed to use Z position (i.e., X/Z plane) instead of
//							Y2 position (i.e., viewing plane) position for draw 
//							priority.
//
//		07/30/97	JMI	Changed several deletes that occurred just before reading
//							instantiable member variables.  Now both 
//							ProcessFireballMessage() functions (there are two (one for
//							CFireballs and one for CFireStreams) ) do NOT delete this
//							when they receive a delete message.  They instead set the
//							state to delete and allow the calling function to do the
//							delete.  The problem here was that the calling function,
//							Update(), needed to look at one more member var, m_eState,
//							before returning, but, at that point (that is, after the
//							delete) this was invalid.  This works find though when
//							using SmartHeap I think b/c of SmartHeap's bounds checking
//							areas to detect overwrites.  The Alpha does not have these
//							areas (no SmartHeap) and, so, crashes easily in these cases.
//							There was also a problem in Update() where it deleted this
//							but did not return right away and one more member var access
//							was done after that point (probably added later).  Fixed.
//
//		07/30/97	JMI	Now uses m_dHorizVel for its velocity which is initially 
//							set to ms_dFireVelocity but can be overridden after the
//							Setup() call.
//
//		08/08/97	JMI	Now CFirestream::ProcessFireballMessages() passes on
//							delete messages to any fireballs it owns.
//
//		08/08/97	JMI	Changed m_pFireball1, 2, & 3 to m_idFireball1, 2, & 3.
//
////////////////////////////////////////////////////////////////////////////////

#include "fireball.h"

#include "realm.h"
#include "reality.h"
#include "game.h"

#include "Thing3d.h"

////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

#define SMALL_FILE			"tinyfire.aan"

#define MIN_ALPHA				30
#define MAX_ALPHA				200
// Note the sum of these two values should not exceed 255

// Gets a random between -range / 2 and range / 2.
#define RAND_SWAY(sway)	((GetRand() % sway) - sway / 2)

#define FIREBALL_SWAY		15

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

// Let this auto-init to 0
int16_t CFirestream::ms_sFileCount;
int16_t CFirestream::ms_sOffset1 = 12;		// pixels from 1st to 2nd fireball
int16_t CFirestream::ms_sOffset2 = 24;	// pixels from 1st to 3rd fireball



CFirestream::CFirestream(void)
{
  m_sSuspend = 0;
  m_lPrevTime = 0;
  m_bSendMessages = true;
  m_sTotalAlphaChannels = 0;
}

CFirestream::~CFirestream(void)
{
}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CFirestream::Load(										// Returns 0 if successfull, non-zero otherwise
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

			// Load static data.
			switch (ulFileVersion)
			{
				default:
				case 1:
					break;
			}
		}

		// Load instance data.
		switch (ulFileVersion)
		{
			default:
			case 1:
				break;
		}

		// Make sure there were no file errors or format errors . . .
		if (!pFile->Error() && sResult == SUCCESS)
		{
		}
		else
		{
			sResult = FAILURE;
			TRACE("CFirestream::Load(): Error reading from file!\n");
		}
	}
	else
	{
		TRACE("CFirestream::Load():  CThing::Load() failed.\n");
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CFirestream::Save(										// Returns 0 if successfull, non-zero otherwise
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
		TRACE("CFirestream::Save(): CThing::Save() failed.\n");
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Startup object
////////////////////////////////////////////////////////////////////////////////
void CFirestream::Startup(void)								// Returns 0 if successfull, non-zero otherwise
{
   Init();
}

////////////////////////////////////////////////////////////////////////////////
// Suspend object
////////////////////////////////////////////////////////////////////////////////
void CFirestream::Suspend(void)
{
	m_sSuspend++;
}

////////////////////////////////////////////////////////////////////////////////
// Resume object
////////////////////////////////////////////////////////////////////////////////
void CFirestream::Resume(void)
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
void CFirestream::Update(void)
{
#ifdef UNUSED_VARIABLES
	int32_t lThisTime;
#endif

	if (!m_sSuspend)
	{
#ifdef UNUSED_VARIABLES
      lThisTime = realm()->m_time.GetGameTime();
#endif
		// Update the other fireballs
      if (m_fireball1)
			{
         m_fireball1->position.x		= position.x;
         m_fireball1->position.y		= position.y;
         m_fireball1->position.z		= position.z;
         m_fireball1->rotation.y	= rspMod360(rotation.y + RAND_SWAY(FIREBALL_SWAY) );
			}
		else
			{
         m_fireball1.reset();
			}

      if (m_fireball2)
			{
         m_fireball2->position.x		= position.x + COSQ[(int16_t) rotation.y] * ms_sOffset1;
         m_fireball2->position.y		= position.y;
         m_fireball2->position.z		= position.z - SINQ[(int16_t) rotation.y] * ms_sOffset1;
         m_fireball2->rotation.y	= rspMod360(rotation.y + RAND_SWAY(FIREBALL_SWAY) );
			}
		else
         {
        m_fireball2.reset();
			}

      if (m_fireball3)
			{
         m_fireball3->position.x		= position.x + COSQ[(int16_t) rotation.y] * ms_sOffset2;
         m_fireball3->position.y		= position.y;
         m_fireball3->position.z		= position.z - SINQ[(int16_t) rotation.y] * ms_sOffset2;
         m_fireball3->rotation.y	= rspMod360(rotation.y + RAND_SWAY(FIREBALL_SWAY) );
			}
		else
         {
        m_fireball3.reset();
			}

		if (m_eState == CWeapon::State_Fire)
		{
         if (m_fireball1)
            m_fireball1->m_eState = CWeapon::State_Fire;
         if (m_fireball2)
            m_fireball2->m_eState = CWeapon::State_Fire;
         if (m_fireball3)
            m_fireball3->m_eState = CWeapon::State_Fire;
         Object::enqueue(SelfDestruct);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Render object
////////////////////////////////////////////////////////////////////////////////
void CFirestream::Render(void)
{
	// If we have a parent . . .
   if (m_psprParent)
     m_psprParent->RemoveChild(this);

	// This should never ever be rendered.
   ASSERT(m_psprParent == nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// Setup
////////////////////////////////////////////////////////////////////////////////

int16_t CFirestream::Setup(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ,												// In:  New z coord
	int16_t sDir,												// In:  Direction of travel
	int32_t lTimeToLive,										// In:  Number of milliseconds to burn, default 1sec
   managed_ptr<CThing3d> shooter)										// In:  Shooter's ID so you don't hit him
{
	int16_t sResult = SUCCESS;
	double dX;
	double dZ;
	
	// Use specified position
   position.x = (double)sX;
   position.y = (double)sY;
   position.z = (double)sZ;
   rotation.y = sDir;
	m_lPrevTime = realm()->m_time.GetGameTime();
   m_shooter = shooter;

	// Make sure that the starting positions are valid before creating
	// fireballs here, otherwise they will shoot through walls.
   dX = position.x + COSQ[(int16_t) rotation.y] * ms_sOffset2;	// Second interval
   dZ = position.z - SINQ[(int16_t) rotation.y] * ms_sOffset2;
	if (realm()->IsPathClear(		// Returns true, if the entire path is clear.                 
											// Returns false, if only a portion of the path is clear.     
											// (see *psX, *psY, *psZ).                                    
         (int16_t) position.x, 				// In:  Starting X.
         (int16_t) position.y, 				// In:  Starting Y.
         (int16_t) position.z, 				// In:  Starting Z.
			3.0, 							// In:  Rate at which to scan ('crawl') path in pixels per    
											// iteration.                                                 
											// NOTE: Values less than 1.0 are inefficient.                
											// NOTE: We scan terrain using GetHeight()                    
											// at only one pixel.                                         
											// NOTE: We could change this to a speed in pixels per second 
											// where we'd assume a certain frame rate.                    
			(int16_t) dX,		 			// In:  Destination X.                                        
			(int16_t) dZ,					// In:  Destination Z.                                        
			0,								// In:  Max traverser can step up.                      
			nullptr,							// Out: If not nullptr, last clear point on path.                
			nullptr,							// Out: If not nullptr, last clear point on path.                
			nullptr,							// Out: If not nullptr, last clear point on path.                
			false) )						// In:  If true, will consider the edge of the realm a path
											// inhibitor.  If false, reaching the edge of the realm    
											// indicates a clear path.                                 
	{
		m_lTimeToLive = realm()->m_time.GetGameTime() + lTimeToLive;

      m_fireball1 = realm()->AddThing<CFireball>();
      if (m_fireball1)
      {
         m_fireball1->Setup(position.x, position.y, position.z, sDir, lTimeToLive, shooter);
         Object::connect(SelfDestruct, m_fireball1->SelfDestruct);
      }

      dX = position.x + COSQ[(int16_t) rotation.y] * ms_sOffset1;	// First interval
      dZ = position.z - SINQ[(int16_t) rotation.y] * ms_sOffset1;
      m_fireball2 = realm()->AddThing<CFireball>();
      if (m_fireball2)
      {
         m_fireball2->Setup(dX, position.y, dZ, sDir, lTimeToLive, shooter);
         Object::connect(SelfDestruct, m_fireball2->SelfDestruct);
      }

      dX = position.x + COSQ[(int16_t) rotation.y] * ms_sOffset2;	// Second interval
      dZ = position.z - SINQ[(int16_t) rotation.y] * ms_sOffset2;
      m_fireball3 = realm()->AddThing<CFireball>();
      if (m_fireball3)
      {
         m_fireball3->Setup(dX, position.y, dZ, sDir, lTimeToLive, shooter);
         Object::connect(SelfDestruct, m_fireball3->SelfDestruct);
      }

		if (sResult == SUCCESS)
			sResult = Init();
	}

	return sResult;

}

////////////////////////////////////////////////////////////////////////////////
// Init
////////////////////////////////////////////////////////////////////////////////

int16_t CFirestream::Init(void)
{
	int16_t sResult = SUCCESS;
	
	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CFirestream::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
	int16_t sResult = SUCCESS;
	
	// Use specified position
   position.x = (double)sX;
   position.y = (double)sY;
   position.z = (double)sZ;
	m_lTimer = GetRand(); //realm()->m_time.GetGameTime() + 1000;
	m_lPrevTime = realm()->m_time.GetGameTime();

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to modify object
////////////////////////////////////////////////////////////////////////////////
int16_t CFirestream::EditModify(void)
{
	return SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to move object to specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CFirestream::EditMove(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
   position.x = (double)sX;
   position.y = (double)sY;
   position.z = (double)sZ;

	return SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to update object
////////////////////////////////////////////////////////////////////////////////
void CFirestream::EditUpdate(void)
{
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to render object
////////////////////////////////////////////////////////////////////////////////
void CFirestream::EditRender(void)
{
	// In some cases, object's might need to do a special-case render in edit
	// mode because Startup() isn't called.  In this case it doesn't matter, so
	// we can call the normal Render().
	Render();
}



////////////////////////////////////////////////////////////////////////////////
// Preload - basically trick the resource manager into caching resources for fire
//				 animations before play begins so that when a fire is set for
//				 the first time, there won't be a delay while it loads.
////////////////////////////////////////////////////////////////////////////////

int16_t CFirestream::Preload(
	CRealm* /*prealm*/)			// In:  Calling realm.
{
return SUCCESS;
}


////////////////////////////////// Fireball ////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

#define SMALL_FILE			"tinyfire.aan"

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

// Let this auto-init to 0
int16_t CFireball::ms_sFileCount;
int16_t CFireball::ms_sSmallRadius = 8;
int32_t  CFireball::ms_lCollisionTime = 250;			// Check for collisions this often
double CFireball::ms_dFireVelocity = 300;


CFireball::CFireball(void)
{
  m_sSuspend = 0;
  m_lPrevTime = 0;
  m_bSendMessages = true;
  m_u32CollideIncludeBits = 0;
  m_u32CollideDontcareBits = 0;
  m_u32CollideExcludeBits = 0;
  m_sTotalAlphaChannels = 0;
  m_smash.m_bits = 0;
  m_bMoving = true;
  m_lAnimTime = 0;

  //			m_sprite.m_pthing = this;
}

CFireball::~CFireball(void)
{
  // Remove yourself from the collision list if it was in use
  // (switching to smoke removes it from the smashatorium and sets
  // the m_pThing field to nullptr)
  realm()->m_smashatorium.Remove(&m_smash);

  // Free resources
  FreeResources();
}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CFireball::Load(										// Returns 0 if successfull, non-zero otherwise
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

			// Load static data.
			switch (ulFileVersion)
			{
				default:
				case 1:
					break;
			}
		}

		// Load instance data.
		switch (ulFileVersion)
		{
			default:
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
			TRACE("CFireball::Load(): Error reading from file!\n");
		}
	}
	else
	{
		TRACE("CFireball::Load():  CThing::Load() failed.\n");
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CFireball::Save(										// Returns 0 if successfull, non-zero otherwise
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
		TRACE("CFireball::Save(): CThing::Save() failed.\n");
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Startup object
////////////////////////////////////////////////////////////////////////////////
void CFireball::Startup(void)								// Returns 0 if successfull, non-zero otherwise
{
   Init();
}

////////////////////////////////////////////////////////////////////////////////
// Suspend object
////////////////////////////////////////////////////////////////////////////////
void CFireball::Suspend(void)
{
	m_sSuspend++;
}

////////////////////////////////////////////////////////////////////////////////
// Resume object
////////////////////////////////////////////////////////////////////////////////
void CFireball::Resume(void)
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
void CFireball::Update(void)
{
	int32_t lThisTime;
	double dSeconds;
	double dDistance;
	double dNewX;
	double dNewZ;

	if (!m_sSuspend)
	{
		lThisTime = realm()->m_time.GetGameTime();
		m_lAnimTime += lThisTime - m_lPrevTime;

      if(m_eState == CWeaponState::State_Fire)
      {
				if (lThisTime < m_lTimeToLive)
				{
					if (m_bMoving)
					{
						// Update position using wind direction and velocity
						dSeconds = ((double) lThisTime - (double) m_lPrevTime) / 1000.0;
						// Apply internal velocity.
						dDistance	= m_dHorizVel * dSeconds;
                  dNewX	= position.x + COSQ[(int16_t) rotation.y] * dDistance;
                  dNewZ	= position.z - SINQ[(int16_t) rotation.y] * dDistance;

						// Check attribute map for walls, and if you hit a wall, 
						// set the timer so you will die off next time around.
						int16_t sHeight = realm()->GetHeight(int16_t(dNewX), int16_t(dNewZ));
						// If it hits a wall taller than itself, then it will rotate in the
						// predetermined direction until it is free to move.
                  if ((int16_t) position.y < sHeight ||
							!realm()->IsPathClear(	// Returns true, if the entire path is clear.                 
															// Returns false, if only a portion of the path is clear.     
															// (see *psX, *psY, *psZ).                                    
                     (int16_t) position.x, 				// In:  Starting X.
                     (int16_t) position.y, 				// In:  Starting Y.
                     (int16_t) position.z, 				// In:  Starting Z.
							3.0, 							// In:  Rate at which to scan ('crawl') path in pixels per    
															// iteration.                                                 
															// NOTE: Values less than 1.0 are inefficient.                
															// NOTE: We scan terrain using GetHeight()                    
															// at only one pixel.                                         
															// NOTE: We could change this to a speed in pixels per second 
															// where we'd assume a certain frame rate.                    
							(int16_t) dNewX, 			// In:  Destination X.                                        
							(int16_t) dNewZ,				// In:  Destination Z.                                        
							0,								// In:  Max traverser can step up.                      
							nullptr,							// Out: If not nullptr, last clear point on path.                
							nullptr,							// Out: If not nullptr, last clear point on path.                
							nullptr,							// Out: If not nullptr, last clear point on path.                
							false) 						// In:  If true, will consider the edge of the realm a path
															// inhibitor.  If false, reaching the edge of the realm    
															// indicates a clear path.                                 
						)
						{
							// Stop moving and fix yourself on a random spot on the wall.
							m_bMoving = false;				
                     position.x += (-3 + GetRand() % 7);
                     position.y += (-3 + GetRand() % 7);
                     position.z += (-3 + GetRand() % 7);

							// Update sphere
                     m_smash.m_sphere.sphere.X = position.x;
                     m_smash.m_sphere.sphere.Y = position.y;
                     m_smash.m_sphere.sphere.Z = position.z;
						}
						else
						{
                     position.x = dNewX;
                     position.z = dNewZ;

							// Update sphere
                     m_smash.m_sphere.sphere.X = position.x;
                     m_smash.m_sphere.sphere.Y = position.y;
                     m_smash.m_sphere.sphere.Z = position.z;

							// Check for collisions
							CSmash* pSmashed = nullptr;
							GameMessage msg;
							msg.msg_Burn.eType = typeBurn;
							msg.msg_Burn.sPriority = 0;
							msg.msg_Burn.sDamage = 10;
                     msg.msg_Burn.shooter = m_shooter;
							realm()->m_smashatorium.QuickCheckReset(&m_smash, m_u32CollideIncludeBits,
																				  m_u32CollideDontcareBits, 
																				  m_u32CollideExcludeBits);
							while (realm()->m_smashatorium.QuickCheckNext(&pSmashed))
                        if (pSmashed->m_pThing != m_shooter)
                           SendThingMessage(msg, pSmashed->m_pThing);
						}
					}
				}
            else
              Object::enqueue(SelfDestruct);
		}
		m_lPrevTime = lThisTime;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Render object
////////////////////////////////////////////////////////////////////////////////
void CFireball::Render(void)
{
	
	CAlphaAnim* pAnim;

   pAnim = (CAlphaAnim*) m_pAnimChannel->GetAtTime(m_lAnimTime % m_pAnimChannel->TotalTime());

	if (pAnim) // && m_sCurrentAlphaChannel >= 0)
	{
		// No special flags
      flags.clear();

		// Map from 3d to 2d coords
      realm()->Map3Dto2D(position.x, position.y, position.z,
                         m_sX2, m_sY2);
		// Offset by animations 2D offsets.
      m_sX2	+= pAnim->m_sX;
      m_sY2	+= pAnim->m_sY;

		// Priority is based on our Z position.
      m_sPriority = position.z;

		// Layer should be based on info we get from attribute map.
      m_sLayer = CRealm::GetLayerViaAttrib(realm()->GetLayer((int16_t) position.x, (int16_t) position.z));

//		m_sAlphaLevel = 200;
		if (m_lTotalFlameTime == 0)
			{
			// Safety.
			m_lTotalFlameTime = 500;
			}

//		m_sAlphaLevel = MIN((long)255, (long) (((m_lTimeToLive - realm()->m_time.GetGameTime()) / m_lTotalFlameTime) * 255));

		// Copy the color info and the alpha channel to the Alpha Sprite
      m_pImage = &(pAnim->m_imColor);

		// Now there is only 1 alpha mask.
		m_sCurrentAlphaChannel = 0; //MIN(m_sCurrentAlphaChannel, (short) (m_sTotalAlphaChannels - 1));
      m_pimAlpha = &(pAnim->m_pimAlphaArray[0]);
		// Adjust level between 0 and max so it gets more opaque with time.
      m_sAlphaLevel = MIN_ALPHA + MAX_ALPHA - (MAX_ALPHA * (m_lTimeToLive - realm()->m_time.GetGameTime()) ) / m_lTotalFlameTime ;
		// Keep in range.
      if (m_sAlphaLevel < 0)
         m_sAlphaLevel = 0;
      else if (m_sAlphaLevel > MAX_ALPHA)
         m_sAlphaLevel = MAX_ALPHA;

      // Update sprite in scene
      Object::enqueue(SpriteUpdate);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Setup
////////////////////////////////////////////////////////////////////////////////

int16_t CFireball::Setup(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ,												// In:  New z coord
	int16_t sDir,												// In:  Direction of travel
	int32_t lTimeToLive,										// In:  Number of milliseconds to burn, default 1sec
   managed_ptr<CThing3d> shooter)										// In:  Shooter's ID so you don't hit him
{
	int16_t sResult = SUCCESS;
	
	// Use specified position
   position.x = (double)sX;
   position.y = (double)sY;
   position.z = (double)sZ;
   rotation.y = sDir;
	m_lPrevTime = realm()->m_time.GetGameTime();
	m_lCollisionTimer = m_lPrevTime + ms_lCollisionTime;
   m_shooter = shooter;
	m_lAnimTime = GetRandom();
	m_dHorizVel	= ms_dFireVelocity; 

	m_lTotalFlameTime = lTimeToLive;
	m_lTimeToLive = realm()->m_time.GetGameTime() + lTimeToLive;
	m_sCurrentAlphaChannel = 0;
	
	// Load resources
	sResult = GetResources();

	if (sResult == SUCCESS)
		sResult = Init();

	return sResult;

}

////////////////////////////////////////////////////////////////////////////////
// Init
////////////////////////////////////////////////////////////////////////////////

int16_t CFireball::Init(void)
{
	int16_t sResult = SUCCESS;
//	CAlphaAnim* pAnim = nullptr;

	// Update sphere
   m_smash.m_sphere.sphere.X = position.x;
   m_smash.m_sphere.sphere.Y = position.y;
   m_smash.m_sphere.sphere.Z = position.z;
	m_smash.m_sphere.sphere.lRadius = ms_sSmallRadius;
	m_smash.m_bits = CSmash::Fire;
   m_smash.m_pThing = this;
	// Update the smash
	realm()->m_smashatorium.Update(&m_smash);
	m_eState = CWeapon::State_Idle;

	// Set the collision bits
	m_u32CollideIncludeBits = CSmash::Character | CSmash::Barrel | CSmash::Mine | CSmash::Misc;
	m_u32CollideDontcareBits = CSmash::Good | CSmash::Bad;
	m_u32CollideExcludeBits = 0;

	return sResult;
}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CFireball::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
	int16_t sResult = SUCCESS;
	
	// Use specified position
   position.x = (double)sX;
   position.y = (double)sY;
   position.z = (double)sZ;
	m_lTimer = GetRand(); //realm()->m_time.GetGameTime() + 1000;
	m_lPrevTime = realm()->m_time.GetGameTime();

	// Load resources
	sResult = GetResources();

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to modify object
////////////////////////////////////////////////////////////////////////////////
int16_t CFireball::EditModify(void)
{
	return SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to move object to specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CFireball::EditMove(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
   position.x = (double)sX;
   position.y = (double)sY;
   position.z = (double)sZ;

	return SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to update object
////////////////////////////////////////////////////////////////////////////////
void CFireball::EditUpdate(void)
{
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to render object
////////////////////////////////////////////////////////////////////////////////
void CFireball::EditRender(void)
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
int16_t CFireball::GetResources(void)			// Returns 0 if successfull, non-zero otherwise
{
	int16_t sResult = SUCCESS;

	sResult = rspGetResource(&g_resmgrGame, realm()->Make2dResPath(SMALL_FILE), &m_pAnimChannel, RFile::LittleEndian);

	if (sResult != SUCCESS)
		TRACE("CFireball::GetResources - Error getting fire animation resource\n");

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Free all resources
////////////////////////////////////////////////////////////////////////////////
int16_t CFireball::FreeResources(void)						// Returns 0 if successfull, non-zero otherwise
{
	int16_t sResult = SUCCESS;

	rspReleaseResource(&g_resmgrGame, &m_pAnimChannel);

	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// Preload - basically trick the resource manager into caching resources for fire
//				 animations before play begins so that when a fire is set for
//				 the first time, there won't be a delay while it loads.
////////////////////////////////////////////////////////////////////////////////

int16_t CFireball::Preload(
	CRealm* prealm)				// In:  Calling realm.
{
	int16_t sResult;
	ChannelAA* pRes;
	sResult = rspGetResource(&g_resmgrGame, prealm->Make2dResPath(SMALL_FILE), &pRes, RFile::LittleEndian);
	rspReleaseResource(&g_resmgrGame, &pRes);
	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
