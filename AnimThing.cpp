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
// AnimThing.cpp
// Project: Nostril (aka Postal)
// 
// History:
//		02/18/97 JMI	Started.
//
//		02/19/97	JMI	Added ability to send any message to any CThing when done
//							with animation.
//
//		02/20/97	JMI	Tweaked positioning; I think it's more correct.
//
//		02/20/97 MJR	Added mini-runtime-editor.
//
//		02/21/97	JMI	After tweaking the position, I hosed the sprite priority.
//							Fixed.  Now it's beauteous if you place a dude just a
//							bit in front of a looping explosion.
//
//		02/24/97	JMI	No longer sets the m_type member of the m_sprite b/c it
//							is set by m_sprite's constructor.
//
//		02/24/97	JMI	Changed m_pthingSendMsg to m_u16IdSendMsg.
//
//		02/25/97	JMI	Forgot to call FreeResources() in GetResources().
//
//		03/13/97	JMI	Load now takes a version number.
//
//		03/27/97	JMI	Now EditModify() returns failure if there's no resource
//							name.  GetResources() should've taken care of this via
//							rspGetResource() failing, but it seems to crash in Release
//							mode, so now EditModify() checks.
//
//		04/10/97 BRH	Updated this to work with the new multi layer attribute
//							maps.
//
//		05/29/97	JMI	Removed ASSERT on realm()->m_pAttribMap which no longer
//							exists.
//
//		06/29/97	JMI	Converted EditRect(), EditRender(), and/or Render() to
//							use Map3Dto2D().
//							Also, now determines priority via 2D mapped Y.
//
//		07/27/97	JMI	Now determines priority totally based on Z.
//							Also, now uses CRealm::Make2dResPath() on res names.
//
//////////////////////////////////////////////////////////////////////////////
//
// This CThing-derived class will play an animation to its finish and then
// destroy itself.  It's a mini Postal movie player.
//
//////////////////////////////////////////////////////////////////////////////

#include "AnimThing.h"

#include "realm.h"
#include "dude.h"
#include "game.h"
#include "message.h"

////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

#define GUI_FILE_NAME	"res/editor/EditAnim.gui"

#define GUI_ID_RESOURCE	3
#define GUI_ID_OPTIONS	4
#define GUI_ID_DONTLOOP	5
#define GUI_ID_DOLOOP	6

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

CAnimThing::CAnimThing(void)
{
  m_paachannel = nullptr;
  m_sSuspend = 0;
  m_sLoop = TRUE;
  m_szResName[0] = '\0';
  m_msg.msg_Generic.sPriority	= 0;
}

CAnimThing::~CAnimThing(void)
{
  // Remove sprite from scene (this is safe even if it was already removed!)
  //realm()->Scene()->RemoveSprite(&m_sprite);

  // Free resources
  FreeResources();
}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CAnimThing::Load(								// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,										// In:  File to load from
	bool bEditMode,									// In:  True for edit mode, false otherwise
	int16_t sFileCount,									// In:  File count (unique per file, never 0)
	uint32_t	ulFileVersion)								// In:  Version of file format to load.
	{
	int16_t sResult = CThing::Load(pFile, bEditMode, sFileCount, ulFileVersion);
	if (sResult == SUCCESS)
		{
		switch (ulFileVersion)
			{
			default:
			case 1:
            pFile->Read(&position.x);
            pFile->Read(&position.y);
            pFile->Read(&position.z);
				pFile->Read(m_szResName);
				pFile->Read(&m_sLoop);
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
			TRACE("CAnimThing::Load(): Error reading from file!\n");
			}
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CAnimThing::Save(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to save to
	int16_t sFileCount)										// In:  File count (unique per file, never 0)
	{
   int16_t sResult	= CThing::Save(pFile, sFileCount);
	if (sResult == SUCCESS)
		{
      pFile->Write(&position.x);
      pFile->Write(&position.y);
      pFile->Write(&position.z);
		pFile->Write(m_szResName);
		pFile->Write(&m_sLoop);

		// Make sure there were no file errors
		sResult	= pFile->Error();
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Startup object
////////////////////////////////////////////////////////////////////////////////
void CAnimThing::Startup(void)								// Returns 0 if successfull, non-zero otherwise
	{
	m_lAnimPrevTime	= realm()->m_time.GetGameTime();
   m_lAnimTime			= 0;
	}

////////////////////////////////////////////////////////////////////////////////
// Suspend object
////////////////////////////////////////////////////////////////////////////////
void CAnimThing::Suspend(void)
	{
	m_sSuspend++;
	}


////////////////////////////////////////////////////////////////////////////////
// Resume object
////////////////////////////////////////////////////////////////////////////////
void CAnimThing::Resume(void)
	{
	m_sSuspend--;

	// If we're actually going to start updating again, we need to reset
	// the time so as to ignore any time that passed while we were suspended.
	// This method is far from precise, but I'm hoping it's good enough.
	if (m_sSuspend == 0)
		{
		m_lAnimPrevTime	= realm()->m_time.GetGameTime();
		}
	}


////////////////////////////////////////////////////////////////////////////////
// Update object
////////////////////////////////////////////////////////////////////////////////
void CAnimThing::Update(void)
	{
	if (!m_sSuspend)
		{
		if (m_lAnimTime >= m_paachannel->TotalTime() && m_sLoop == FALSE)
			{
			// If there's a thing to send a message to . . .
         if (m_sender)
				{
				// Send the message.
            SendThingMessage(m_msg, m_sender);
				}
         Object::enqueue(SelfDestruct);
			}
		}
	}


////////////////////////////////////////////////////////////////////////////////
// Render object
////////////////////////////////////////////////////////////////////////////////
int16_t sEdit;
double dScale = 1.0;
double dScaleInc = 0.025;

void CAnimThing::Render(void)
	{
	// Get current time diff.
	int32_t lCurTime		= realm()->m_time.GetGameTime(); 
	m_lAnimTime			+= lCurTime - m_lAnimPrevTime;
	m_lAnimPrevTime	= lCurTime;

   CAlphaAnim* paa = m_paachannel->GetAtTime(m_lAnimTime);
	if (paa != nullptr)
      {
     flags.clear();
		
		// Map from 3d to 2d coords
//		m_sX2 = position.x + paa->m_sX;
//		m_sY2 = position.z - (position.y - paa->m_sY);
      realm()->Map3Dto2D(position.x, position.y, position.z,
                         m_sX2, m_sY2);

		// Offset by hotspot.
      m_sX2	+= paa->m_sX;
      m_sY2	+= paa->m_sY;
		
		// Priority is based on our position in 3D realm coords.
      m_sPriority = position.z;
		
		// Layer should be based on info we get from attribute map.
      m_sLayer = CRealm::GetLayerViaAttrib(realm()->GetLayer(int16_t(position.x), int16_t(position.z)));

		// Copy the color info and the alpha channel to the Alpha Sprite
      m_pImage		= &(paa->m_imColor);
      m_pimAlpha	= &(paa->m_pimAlphaArray[0]);	// This is an array?  What changes my index?!

///////////////////////////////////////////////////////////
// Tiny little built-in editor that gets acctivated via
// the debugger -- just set sEdit to non-zero.  It assumes
// the alpha anim has 2 layers, with the second layer at
// 100%.  It copies that layer to the first layer, scaling
// it by the specified value.  Press LEFT and RIGHT arrows
// (quickly) to modify the scaling value and watch as the
// alpha effect changes.
///////////////////////////////////////////////////////////
if (sEdit && (paa->m_sNumAlphas > 2))
	{
	uint8_t auc[128];
	rspScanKeys(auc);
	if (auc[RSP_SK_LEFT])
		{
		dScale -= dScaleInc;
		if (dScale < 0.0)
			dScale = 0;
		}
	if (auc[RSP_SK_RIGHT])
		{
		dScale += dScaleInc;
		}
	for (int16_t y = 0; y < paa->m_pimAlphaArray[1].m_sHeight; y++)
		{
		uint8_t* pSrc = paa->m_pimAlphaArray[1].m_pData + (y * paa->m_pimAlphaArray[1].m_lPitch);
		uint8_t* pDst = paa->m_pimAlphaArray[0].m_pData + (y * paa->m_pimAlphaArray[0].m_lPitch);

		for (int16_t x = 0; x < paa->m_pimAlphaArray[1].m_sWidth; x++)
			{
			double dVal = (double)(*pSrc);
			dVal *= dScale;
			if (dVal < 256.0)
				*pDst = (uint8_t)dVal;
			else
            *pDst = 0xFF;
			pSrc++;
			pDst++;
			}
		}
	}
///////////////////////////////////////////////////////////

      Object::enqueue(SpriteUpdate);
		}
	}


int16_t CAnimThing::Setup(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
	{
   int16_t sResult = SUCCESS;
	
	// Use specified position
   position.x = sX;
   position.y = sY;
   position.z = sZ;

	m_lAnimPrevTime = realm()->m_time.GetGameTime();

	// Load resources
	sResult = GetResources();

	return sResult;
	}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CAnimThing::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
	{
   int16_t sResult = SUCCESS;
	
	// Use specified position
   position.x = sX;
   position.y = sY;
   position.z = sZ;

	sResult	= EditModify();

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to modify object
////////////////////////////////////////////////////////////////////////////////
int16_t CAnimThing::EditModify(void)
	{
   int16_t sResult = SUCCESS;

	RGuiItem* pgui = RGuiItem::LoadInstantiate(FullPathVD(GUI_FILE_NAME));
	if (pgui != nullptr)
		{
      RListBox* plb = static_cast<RListBox*>(pgui->GetItemFromId(GUI_ID_OPTIONS));
		if (plb != nullptr)
			{
			ASSERT(plb->m_type == RGuiItem::ListBox);
			// Select current setting.
			plb->SetSel(pgui->GetItemFromId((m_sLoop == FALSE) ? GUI_ID_DONTLOOP : GUI_ID_DOLOOP) );

         REdit* pedit = static_cast<REdit*>(pgui->GetItemFromId(GUI_ID_RESOURCE));
			if (pedit != nullptr)
				{
				ASSERT(pedit->m_type == RGuiItem::Edit);

				// Set text.
				pedit->SetText("%s", m_szResName);
				// Realize text.
				pedit->Compose();
				
				if (DoGui(pgui) == 1)
					{
					RGuiItem* pguiSel = plb->GetSel();
					if (pguiSel)
						{
						switch (pguiSel->m_lId)
							{
							case GUI_ID_DONTLOOP:
								m_sLoop = FALSE;
								break;
								
							case GUI_ID_DOLOOP:
								m_sLoop = TRUE;
								break;
							}
						}

					// Get new resource name.
					pedit->GetText(m_szResName, sizeof(m_szResName) );

					// If no resource name . . .
					if (m_szResName[0] == '\0')
						{
						sResult = FAILURE;
						}
					}
				else
					{
					sResult = FAILURE;
					}
				}
			else
				{
            sResult = FAILURE * 3;
				}
			}
		else
			{
         sResult = FAILURE * 2;
			}
		
		// Done with GUI.
		delete pgui;
		}
	else
		{
      sResult = FAILURE;
		}

	// If successful so far . . .
	if (sResult == SUCCESS)
		{
		// Load resources
		sResult = GetResources();
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to move object to specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CAnimThing::EditMove(									// Returns 0 if successfull, non-zero otherwise
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
// Called by editor to get the clickable pos/area of an object in 2D.
// (virtual	(Overridden here)).
////////////////////////////////////////////////////////////////////////////////
void CAnimThing::EditRect(	// Returns nothiing.
	RRect*	prc)				// Out: Clickable pos/area of object.
{
  realm()->Map3Dto2D(position.x, position.y, position.z,
                     prc->sX, prc->sY);

  prc->sW = 10;	// Safety.
  prc->sH = 10;	// Safety.

  if (m_paachannel != nullptr)
  {
    CAlphaAnim* paa = m_paachannel->GetAtTime(m_lAnimTime);
    if (paa != nullptr)
    {
      // Offset by hotspot.
      prc->sX	+= paa->m_sX;
      prc->sY	+= paa->m_sY;
      prc->sW	= paa->m_imColor.m_sWidth;
      prc->sH	= paa->m_imColor.m_sHeight;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Called by editor to get the hotspot of an object in 2D.
// (virtual	(Overridden here)).
////////////////////////////////////////////////////////////////////////////////
void CAnimThing::EditHotSpot(	// Returns nothiing.
	int16_t*	psX,					// Out: X coord of 2D hotspot relative to
										// EditRect() pos.
	int16_t*	psY)					// Out: Y coord of 2D hotspot relative to
										// EditRect() pos.
	{
	*psX	= 0;	// Safety.
	*psY	= 0;	// Safety.

	if (m_paachannel != nullptr)
		{
      CAlphaAnim*	paa	= m_paachannel->GetAtTime(m_lAnimTime);
		if (paa != nullptr)
			{
			*psX	= -paa->m_sX;
			*psY	= -paa->m_sY;
			}
		}
	}

////////////////////////////////////////////////////////////////////////////////
// Called by editor to update object
////////////////////////////////////////////////////////////////////////////////
void CAnimThing::EditUpdate(void)
	{
	}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to render object
////////////////////////////////////////////////////////////////////////////////
void CAnimThing::EditRender(void)
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
int16_t CAnimThing::GetResources(void)						// Returns 0 if successfull, non-zero otherwise
	{
   int16_t sResult = SUCCESS;

	// Safe to call even if no resource.
	FreeResources();

	sResult	= rspGetResource(
		&g_resmgrGame, 
		realm()->Make2dResPath(m_szResName), 
		&m_paachannel);

	if (sResult == SUCCESS)
		{
		m_lAnimTime			= 0;
		m_lAnimPrevTime	= realm()->m_time.GetGameTime();

		if (m_sLoop != FALSE)
			{
			m_paachannel->SetLooping(RChannel_LoopAtStart | RChannel_LoopAtEnd);
			}
		else
			{
			m_paachannel->SetLooping(0);
			}
		}
	else
		{
		TRACE("GetResources(): Failed to load resource \"%s\".\n",
			realm()->Make2dResPath(m_szResName) );
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Free all resources
////////////////////////////////////////////////////////////////////////////////
int16_t CAnimThing::FreeResources(void)						// Returns 0 if successfull, non-zero otherwise
	{
   int16_t sResult = SUCCESS;

	if (m_paachannel != nullptr)
		{
		rspReleaseResource(&g_resmgrGame, &m_paachannel);
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
