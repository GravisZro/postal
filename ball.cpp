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
// ball.cpp
// Project: Nostril (aka Postal)
//
// This module impliments the CBall class, which will hopefully become a model
// for most other game objects.
//
// History:
//		12/18/96 MJR	Started.
//
//		01/19/97	JMI	Now EditNew() actually loads and puts up a dialog.
//
//		01/23/97	JMI	The positioning in Render() for Y was + sY / 2.  Changed to
//							- sY.
//
//		01/27/97	JMI	Added override for EditRect() to make this object clickable.
//
//		01/29/97	JMI	Now Load() and Save() call the base class versions.
//
//		02/02/97	JMI	Added EditHotSpot().
//
//		02/04/97	JMI	Changed LoadDib() call to Load() (which now supports
//							loading of DIBs).
//
//		02/06/97	JMI	Made this from a 2D object to a 3D one.
//
//		02/07/97	JMI	Removed m_pipeline and associated members and setup since
//							the CScene now owns the pipeline.
//
//		02/11/97	JMI	Fixed bug in EditHotSpot.
//
//		02/23/97	JMI	In progress, do not use.
//
//		02/23/97	JMI	Brought up to date so we can continue to use this as a
//							test object.
//
//		02/24/97	JMI	For the gravity bounce, I was reversing the current 
//							velocity but setting the last known position.  This caused
//							one extra iteration of acceleration.  Now I use both the
//							old position and the old velocity.
//
//		02/24/97	JMI	AdjustPosVel() now uses default param (gravity).
//
//		02/24/97	JMI	No longer sets the m_type member of the m_sprite b/c it
//							is set by m_sprite's constructor.
//
//		03/06/97	JMI	Upgraded to current rspMod360 usage.
//
//		03/13/97	JMI	Load now takes a version number.
//
//		04/10/97 BRH	Changed ball to use new multi layer attribute maps and the
//							helper functions that go with them.
//
//		05/29/97	JMI	Got rid of 'old way' of using attributes and put in the
//							new.
//
//		06/29/97	JMI	Converted EditRect(), EditRender(), and/or Render() to
//							use Map3Dto2D().
//							Also, fixed priority.
//
//		07/01/97	JMI	Replaced use of GetTerrainAttributes() with 
//							GetHeightAndNoWalk().
//
//		08/05/97	JMI	Changed priority to use Z position rather than 2D 
//							projected Y position.
//
////////////////////////////////////////////////////////////////////////////////

#include "ball.h"

#include "hood.h"
#include "game.h"
#include "reality.h"
#include "realm.h"

////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////


#define BALL_GUI_FILE			"res/editor/ball.gui"

// GUI IDs.
#define GUI_ID_OK					1
#define GUI_ID_CANCEL			2
#define GUI_ID_X_OFFSET			3
#define GUI_ID_Y_OFFSET			4
#define GUI_ID_Z_OFFSET			5

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

int16_t CBall::ms_sFileCount;


CBall::CBall(void)
{
  m_sSuspend	= 0;
  m_dDX			= 0;
  m_dDY			= 0;
  m_dDZ			= 0;
}

CBall::~CBall(void)
{
  // Free resources
  FreeResources();
}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CBall::Load(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to load from
	bool bEditMode,										// In:  True for edit mode, false otherwise
	int16_t sFileCount,										// In:  File count (unique per file, never 0)
	uint32_t	ulFileVersion)									// In:  Version of file format to load.
	{
	int16_t sResult = SUCCESS;

	// In most cases, the base class Load() should be called.
	sResult	= CThing::Load(pFile, bEditMode, sFileCount, ulFileVersion);
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
				//	pFile->Read(ms_acImageName);
					break;
				}
			}

		// Load object data
		switch (ulFileVersion)
			{
			default:
			case 1:
            pFile->Read(&position.x);
            pFile->Read(&position.y);
            pFile->Read(&position.z);
				pFile->Read(&m_dDX);
				pFile->Read(&m_dDY);
				pFile->Read(&m_dDZ);
				break;
			}

		// Make sure there were no file errors or format errors . . .
		if (!pFile->Error() && sResult == SUCCESS)
			{
			sResult = GetResources();
			}
		else
			{
			sResult = FAILURE;
			TRACE("CBall::Load(): Error reading from file!\n");
			}
		}
	else
		{
		TRACE("CBall::Load(): CThing::Load() failed.\n");
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CBall::Save(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to save to
	int16_t sFileCount)										// In:  File count (unique per file, never 0)
	{
	int16_t sResult = SUCCESS;

	// In most cases, the base class Save() should be called.
	sResult	= CThing::Save(pFile, sFileCount);
	if (sResult == SUCCESS)
		{
		// Save common data just once per file (not with each object)
		if (ms_sFileCount != sFileCount)
			{
			ms_sFileCount = sFileCount;

			// Save static data
			//	pFile->Write(ms_acImageName);
			}

		// Save object data
      pFile->Write(&position.x);
      pFile->Write(&position.y);
      pFile->Write(&position.z);
		pFile->Write(&m_dDX);
		pFile->Write(&m_dDY);
		pFile->Write(&m_dDZ);

		sResult	= pFile->Error();
		}
	else
		{
		TRACE("CBall::Save(): CThing::Save() failed.\n");
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Startup object
////////////////////////////////////////////////////////////////////////////////
void CBall::Startup(void)								// Returns 0 if successfull, non-zero otherwise
   {
	// At this point we can assume the CHood was loaded, so we init our height
   m_sPrevHeight = realm()->GetHeight(position.x, position.z);

	// HARD-WIRED CODE ALERT!
	// Eventually, this should be set via the bounding sphere radius.
	m_sCurRadius	= 64;

   m_lPrevTime		= realm()->m_time.GetGameTime();
	}

////////////////////////////////////////////////////////////////////////////////
// Suspend object
////////////////////////////////////////////////////////////////////////////////
void CBall::Suspend(void)
	{
	m_sSuspend++;
	}


////////////////////////////////////////////////////////////////////////////////
// Resume object
////////////////////////////////////////////////////////////////////////////////
void CBall::Resume(void)
	{
	m_sSuspend--;
	}


////////////////////////////////////////////////////////////////////////////////
// Update object
////////////////////////////////////////////////////////////////////////////////
void CBall::Update(void)
	{
	if (!m_sSuspend)
		{
		int32_t	lCurTime				= realm()->m_time.GetGameTime();
		double dDeltaSeconds		= (lCurTime - m_lPrevTime) / 1000.0;

		// Adjust vertical velocity and calculate new position.
      double	dNewY		= position.y;
		double	dNewDY	= m_dDY;
		AdjustPosVel(&dNewY, &dNewDY, dDeltaSeconds);

		// Calculate new position.
      double dNewX = position.x + m_dDX;
      double dNewZ = position.z + m_dDZ;

		// Bounce off edges of world.

		// Get height and 'no walk' status at new position.
		bool		bNoWalk;
		int16_t		sHeight	= realm()->GetHeightAndNoWalk(dNewX, dNewY, &bNoWalk);

		// If new Y position is less than terrain height or 'no walk' zone . . .
		if (dNewY < sHeight || bNoWalk == true)
			{
			// If at the last position we would be above the terrain . . .
			if (dNewY > m_sPrevHeight)
				{
				// We've hit a wall.
				// Reverse both directions (this is cheesy, but it's just a demo object!)
				m_dDX = -m_dDX;
				m_dDZ = -m_dDZ;

				// Restore previous position to avoid getting embedded in anything
            dNewX = position.x;
            dNewZ = position.z;
				}
			else
				{
				// We've hit flat terrain.
				// Use previous velocity.  Otherwise, we'd have accelerated past the
				// ground.
				dNewDY = -m_dDY;

				// Restore previous position to avoid getting embedded in anything
            dNewY	= position.y;
				}
			}

		// Save new height
		m_sPrevHeight = sHeight;

		// Update position
      position.x = dNewX;
      position.y = dNewY;
      position.z = dNewZ;

		// Update velocities.
		m_dDY	= dNewDY;

		// Store time.
		m_lPrevTime	= lCurTime;
		}
	}


////////////////////////////////////////////////////////////////////////////////
// Render object
////////////////////////////////////////////////////////////////////////////////
void CBall::Render(void)
{
  flags.clear();
  // Map from 3d to 2d coords
  realm()->Map3Dto2D(position.x, position.y, position.z,
                     m_sX2, m_sY2);

  // Priority is based on 3D hotspot which is where we're drawn.
  m_sPriority = position.z;

  m_sLayer = CRealm::GetLayerViaAttrib(
                        realm()->GetLayer((int16_t) position.x, (int16_t) position.z));


  // Cheese festival rotation.
  m_trans.Ry(rspMod360(m_dDX));
  m_trans.Rz(rspMod360(m_dDZ));

  int32_t lTime = realm()->m_time.GetGameTime();

  m_pmesh		= &m_anim.m_pmeshes->atTime(lTime);
  m_psop		= &m_anim.m_psops->atTime(lTime);
  m_ptex		= &m_anim.m_ptextures->atTime(lTime);
  m_psphere	= &m_anim.m_pbounds->atTime(lTime);

  m_ptrans	= &m_trans;

  m_sRadius	= m_sCurRadius;

  Object::enqueue(SpriteUpdate); // Update sprite in scene
}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CBall::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
	{
	int16_t sResult = SUCCESS;
	
	// Use specified position
   position.x = sX;
   position.y = sY;
   position.z = sZ;

	// Load resources.
	sResult = GetResources();
	if (sResult == SUCCESS)
		{

     m_trans.makeIdentity();
			// Attempt to startup as if in a real play . . .
          Startup();
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to modify object
////////////////////////////////////////////////////////////////////////////////
int16_t CBall::EditModify(void)
	{
	int16_t sResult = SUCCESS;

	// Load GUI . . .
	RGuiItem*	pguiRoot	= RGuiItem::LoadInstantiate(FullPath(GAME_PATH_VD, BALL_GUI_FILE) );
	if (pguiRoot != nullptr)
		{
		// Modal loop.
		rspClearAllInputEvents();

		// Get the two items that can cause donage.
		RGuiItem*	pguiOk		= pguiRoot->GetItemFromId(GUI_ID_OK);
		RGuiItem*	pguiCancel	= pguiRoot->GetItemFromId(GUI_ID_CANCEL);
		
		if (pguiOk != nullptr && pguiCancel != nullptr)
			{
			// Prepare values.
			// These should definitely check to make sure they exist.
			// A nice inline helper function that takes varargs would do.
			RGuiItem*	pguiEditX	= pguiRoot->GetItemFromId(GUI_ID_X_OFFSET);
			if (pguiEditX != nullptr)
				{
				pguiEditX->SetText("%g", m_dDX);
				// Compose with new text.
				pguiEditX->Compose();
				}

			RGuiItem*	pguiEditY	= pguiRoot->GetItemFromId(GUI_ID_Y_OFFSET);
			if (pguiEditY != nullptr)
				{
				pguiEditY->SetText("%g", m_dDY);
				// Compose with new text.
				pguiEditY->Compose();
				}

			RGuiItem*	pguiEditZ	= pguiRoot->GetItemFromId(GUI_ID_Z_OFFSET);
			if (pguiEditZ != nullptr)
				{
				pguiEditZ->SetText("%g", m_dDZ);
				// Compose with new text.
				pguiEditZ->Compose();
				}

			if (DoGui(pguiRoot) == GUI_ID_OK)
				{
				// Free any existing resources.
				FreeResources();

				// Set values.
				if (pguiEditX != nullptr)
					{
					m_dDX	= strtod(pguiEditX->m_szText, nullptr);
					}
				if (pguiEditY != nullptr)
					{
					m_dDY	= strtod(pguiEditY->m_szText, nullptr);
					}
				if (pguiEditZ != nullptr)
					{
					m_dDZ	= strtod(pguiEditZ->m_szText, nullptr);
					}
				
				// Load resources.
				sResult = GetResources();
				if (sResult == SUCCESS)
               {
              m_trans.makeIdentity();
						// Attempt to startup as if in a real play . . .
                  Startup();
					}
				}
			else
				{
				// User aborted.
				sResult	= 3;
				}

			rspClearAllInputEvents();
			}
		else
			{
			TRACE("EditNew(): GUI missing item(s) with ID %d or %d.\n",
				GUI_ID_OK,
				GUI_ID_CANCEL);
			sResult	= 2;
			}

		// Done with GUIs.
		delete pguiRoot;
		}
	else
		{
		TRACE("EditNew(): Failed to load GUI file \"%s\".\n", BALL_GUI_FILE);
		sResult = FAILURE;
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to move object to specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CBall::EditMove(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
	{
   position.x = sX;
   position.y = sY;
   position.z = sZ;

   return SUCCESS;
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to update object
////////////////////////////////////////////////////////////////////////////////
void CBall::EditUpdate(void)
	{
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to render object
////////////////////////////////////////////////////////////////////////////////
void CBall::EditRender(void)
	{
	// In some cases, object's might need to do a special-case render in edit
	// mode because Startup() isn't called.  In this case it doesn't matter, so
	// we can call the normal Render().
	Render();
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to get the clickable pos/area of an object.
//	(virtual (Overridden here)).
////////////////////////////////////////////////////////////////////////////////
void CBall::EditRect(	// Returns nothiing.
	RRect*	prc)			// Out: Clickable pos/area of object.
{
  realm()->Map3Dto2D(position.x, position.y, position.z,
                     prc->sX, prc->sY);

  prc->sX -= m_sCurRadius;
  prc->sY -= m_sCurRadius;
  prc->sW = m_sCurRadius * 2;
  prc->sH = m_sCurRadius * 2;
}

////////////////////////////////////////////////////////////////////////////////
// Called by editor to get the hotspot of an object in 2D.
// (virtual (Overridden here)).
////////////////////////////////////////////////////////////////////////////////
void CBall::EditHotSpot(			// Returns nothiing.
	int16_t*	psX,						// Out: X coord of 2D hotspot relative to
											// EditRect() pos.
	int16_t*	psY)						// Out: Y coord of 2D hotspot relative to
											// EditRect() pos.
	{
	*psX	= m_sCurRadius;
	*psY	= m_sCurRadius;
	}
#endif // !defined(EDITOR_REMOVED)

////////////////////////////////////////////////////////////////////////////////
// Get all required resources
////////////////////////////////////////////////////////////////////////////////
int16_t CBall::GetResources(void)						// Returns 0 if successfull, non-zero otherwise
{
  if(m_anim.Get("main_bobbing"))
  {
    m_anim.SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);
    return SUCCESS;
  }
  return FAILURE;
}


////////////////////////////////////////////////////////////////////////////////
// Free all resources
////////////////////////////////////////////////////////////////////////////////
int16_t CBall::FreeResources(void)						// Returns 0 if successfull, non-zero otherwise
	{
	int16_t sResult = SUCCESS;

	m_anim.Release();

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
