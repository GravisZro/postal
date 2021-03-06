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
// bouy.cpp
// Project: Nostril (aka Postal)
//
// This module implements the bouy marker for use with the network navagation
//	system that will help the enemy guys get around the world.
//
// History:
//		01/28/97 BRH	Added bouy's to the editor which will help with navagation
//
//		02/02/97 BRH	Added NextRouteNode function which will tell you which
//							bouy you should go to next in order to get to your 
//							destination.
//
//		02/03/97 BRH	Added info to the Load and Save functions to save
//							information needed to reconnect the Bouy network after
//							loading it.  
//		
//		02/04/97 BRH	Added GetRouteTableEntry() function that can be called
//							from the CNavigationNet's ping function which will safely
//							return an entry from the routing table.  If the table
//							was not large enough, it expands the the current
//							number of nodes and initializes the data.
//
//		02/04/97	JMI	Changed LoadDib() call to Load() (which now supports
//							loading of DIBs).
//
//		02/23/97 BRH	Changed bouy resource to be under Resource Manager control.
//							Also changed Render to do nothing and moved its 
//							functionality to EditRender so that the Bouys are not
//							drawn during game play but only in the editor.
//
//		02/24/97	JMI	No longer sets the m_type member of the m_sprite b/c it
//							is set by m_sprite's constructor.
//
//		03/06/97 BRH	Changed to the new calling of Ping which doesn't have the
//							maxdepth parameter.  Also added a commented out version of
//							saving the route table but didn't want to include it yet
//							since it would change the format of the realm files.
//
//		03/07/97	JMI	Now draws the bouy number into it's m_pImage.
//
//		03/07/97	JMI	Now sets the color of the text to be safe.
//
//		03/07/97 BRH	Played some with the font for the bouys so it would
//							be easier to see and so it would show 2 digits.
//
//		03/13/97 BRH	Made a few changes to update the routing tables
//							correctly.  I will also be adding a hops table
//							in addition to the route table to cut down on the
//							number of pings required to fill in the tables.  Then
//							the hops tables can be freed since they aren't needed for
//							gameplay.
//
//		03/13/97	JMI	Load now takes a version number.
//
//		04/10/97 BRH	Updated this to work with the new multi layer attribute
//							maps.
//
//		04/11/97 BRH	Adding BuildRoutingTable function which will use a
//							a Breadth-First search of the tree to determine the
//							shortest route to all reachable nodes, and will then
//							use the temporary BSF tree to fill in the routing table.
//
//		04/15/97 BRH	Taking out the old routing method leaving just the new
//							which seems to be working.
//
//		04/20/97 BRH	Added MessageRequest function that will send the
//							bouy's function if it has one.  Also now loads and saves
//							the message using the message's Load and Save functions.
//
//		04/21/97 BRH	Changed to multiple dialogs for each type of message to
//							avoid special code to display the correct fields for each
//							different type of message.
//
//		04/24/97 BRH	Fixed problem in Load with new version number.
//
//		05/01/97 BRH	Removed messages for logic suggestions and put those
//							into the CPylon class instead.
//
//		05/29/97	JMI	Removed ASSERT on realm()->m_pAttribMap which no longer
//							exists.
//
//		06/06/97 BRH	Freed three arrays used in BuildRouteTable that had
//							previously been a source of memory leaks.
//
//		06/25/97 BRH	Took out the STL set "linkset" because it was causing
//							sync problems with the game.  Apparently, the set uses
//							a tree to store its data, and when building that tree, 
//							uses some kind of random function to balance the tree, but
//							not the standard library rand() function, but a different
//							random function that we don't reset from realm to realm.
//							This caused the routing tables to be build differently
//							each time the game was played, and so the demo mode and
//							network mode were out of sync.
//
//		06/29/97 MJR	Removed last trace of STL, replacing vector with RFList.
//
//		06/29/97	JMI	Converted EditRect(), EditRender(), and/or Render() to
//							use Map3Dto2D().
//							Also, moved definitions of EditRect() and EditHotSpot() to
//							here from bouy.h.
//
//		06/30/97	JMI	Now maps the Z to 3D when loading fileversions previous to
//							24.
//
//		07/07/97 BRH	Fixed bug when saving bouy networks where bouys had been
//							deleted.  The Unlink was not correctly decrementing the
//							number of direct links so it was expecting more direct
//							links when the file was reloaded.
//
//		07/09/97	JMI	Now uses realm()->Make2dResPath() to get the fullpath
//							for 2D image components.
//
//		07/25/97 BRH	Fixed the problem of bouys greater than 254 being
//							created in the editor.
//
//		07/30/97 BRH	Added a flag to indicate whether the bouys should be shown
//							or not so that they can be turned off in the editor.
//
//		08/02/97	JMI	Made bouy font smaller (was 22, now 15), made bouy font
//							brighter (was 1 (dark red), now 249 (bright red) ), and
//							widened bouy graphic in an attempt to make IDs more read-
//							able.
//
//		08/05/97	JMI	Changed priority to use Z position rather than 2D 
//							projected Y position.  
//
//		08/05/97 BRH	Defaulted the bouy network to ON.
//
//		08/08/97 BRH	Only the bouys of the current Nav Net are displayed now.
//							(and their hots are disabled when they are not shown).
//							This will help cut down the confusion and prevent
//							users from joining the networks which would be bad.  
//
//		08/08/97	JMI	Now calls GetResources() on Load() and only in edit mode.
//
//		08/08/97	JMI	Now calls GetResources() in Startup() but checks the realm
//							flag indicating whether we're in edit mode first to make
//							sure we are.
//
////////////////////////////////////////////////////////////////////////////////

#include "bouy.h"

#include "navnet.h"
#include "realm.h"

////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

#define IMAGE_FILE			"bouy.bmp"

#define BOUY_ID_FONT_HEIGHT	15
#define BOUY_ID_FONT_COLOR		249

////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

// These are default values -- actually values are set using the editor!

// Let this auto-init to 0
int16_t CBouy::ms_sFileCount;
bool  CBouy::ms_bShowBouys = true;


CBouy::CBouy(void)
{
  m_sSuspend = 0;
  m_paucRouteTable = nullptr;
  m_sRouteTableSize = 0;
}

CBouy::~CBouy(void)
{
  if (m_paucRouteTable != nullptr)
    free(m_paucRouteTable);

  // Free resources
  FreeResources();
}

////////////////////////////////////////////////////////////////////////////////
// Load object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CBouy::Load(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to load from
	bool bEditMode,										// In:  True for edit mode, false otherwise
	int16_t sFileCount,										// In:  File count (unique per file, never 0)
	uint32_t	ulFileVersion)									// In:  Version of file format to load.
{
	GameMessage msg;
	// Call the base load to get the u16InstanceID
	int16_t sResult = CThing::Load(pFile, bEditMode, sFileCount, ulFileVersion);
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
					break;
			}
		}

		// Load object data
      pFile->Read(&position.x);
      pFile->Read(&position.y);
      pFile->Read(&position.z);

		uint16_t u16Data;
		uint16_t u16NumLinks;
		uint16_t i;
		// Get number of links to be read
		pFile->Read(&u16NumLinks);
		for (i = 0; i < u16NumLinks; i++)
		{
         pFile->Read(&u16Data);
         m_aplDirectLinks.insert(realm()->GetOrAddThingById<CBouy>(u16Data, CBouyID));
		}

		// Get the instance ID for the NavNet
		pFile->Read(&u16Data);
      m_pParentNavNet = realm()->GetOrAddThingById<CNavigationNet>(u16Data, CNavigationNetID);
      ASSERT(m_pParentNavNet);
      m_pParentNavNet->AddBouy(this); // register with parent navnet

		// Switch on the parts that have changed
		switch (ulFileVersion)
		{
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
				msg.Load(pFile);
				break;

			default:
				break;

		}

		// If the file version is earlier than the change to real 3D coords . . .
      if (ulFileVersion < 24)
        realm()->MapY2DtoZ3D(position.z, position.z); // Convert to 3D.

		// Make sure there were no file errors or format errors . . .
		if (!pFile->Error() && sResult == SUCCESS)
		{
#if 0
		// If you were thinking of doing something like this, stop.
		// It is too early to create the image for displaying this
		// bouy (we don't yet have the bouy ID).
			// ONLY IN EDIT MODE . . .
			if (bEditMode == true)
				{
				// Get resources
				sResult = GetResources();
				}
#endif
		}
		else
		{
			sResult = FAILURE;
			TRACE("CBouy::Load(): Error reading from file!\n");
		}
	}
	else
	{
		TRACE("CBouy::Load(): CThing::Load() failed.\n");
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Save object (should call base class version!)
////////////////////////////////////////////////////////////////////////////////
int16_t CBouy::Save(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to save to
	int16_t sFileCount)										// In:  File count (unique per file, never 0)
{
	// Call the base class save to save the u16InstanceID
	CThing::Save(pFile, sFileCount);

	// Save common data just once per file (not with each object)
	if (ms_sFileCount != sFileCount)
	{
		ms_sFileCount = sFileCount;

		// Save static data
	}

	// Save object data
   pFile->Write(&position.x);
   pFile->Write(&position.y);
   pFile->Write(&position.z);

   uint16_t u16Data = m_aplDirectLinks.size();
   pFile->Write(&u16Data); // Save the number of links that will follow in the file
   for(const managed_ptr<CBouy>& pLinkedBouy : m_aplDirectLinks)
   {
     u16Data = pLinkedBouy->GetInstanceID();
     pFile->Write(&u16Data);
   }

	// Save the instance ID for the parent NavNet so it can be connected
	// again after load
	u16Data = m_pParentNavNet->GetInstanceID();
	pFile->Write(&u16Data);

	if (pFile->Error())
		return FAILURE;
	else
		return SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// Startup object
////////////////////////////////////////////////////////////////////////////////
void CBouy::Startup(void)								// Returns 0 if successfull, non-zero otherwise
{
	// At this point we can assume the CHood was loaded, so we init our height
   position.y = realm()->GetHeight((int16_t) position.x, (int16_t) position.z);

	// Init other stuff
	// Get pointer to Navigation Net
	// If we don't have a pointer to the nav net yet, get it from the ID

   // Re-register yourself with the network.
   //if(m_pParentNavNet)
    //m_pParentNavNet->AddBouy(this);
	
		// Only in edit mode . . .
      if (realm()->m_flags.bEditing == true)
         GetResources();
}

////////////////////////////////////////////////////////////////////////////////
// Suspend object
////////////////////////////////////////////////////////////////////////////////
void CBouy::Suspend(void)
{
	m_sSuspend++;
}


////////////////////////////////////////////////////////////////////////////////
// Resume object
////////////////////////////////////////////////////////////////////////////////
void CBouy::Resume(void)
{
	m_sSuspend--;

	// If we're actually going to start updating again, we need to reset
	// the time so as to ignore any time that passed while we were suspended.
	// This method is far from precise, but I'm hoping it's good enough.
}


////////////////////////////////////////////////////////////////////////////////
// AddLink - Add a 1 hop direct link to the routing table
////////////////////////////////////////////////////////////////////////////////

int16_t CBouy::AddLink(managed_ptr<CBouy> pBouy)
{
   int16_t sResult = SUCCESS;
	
   m_aplDirectLinks.insert(pBouy);
	
   return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// Render object
////////////////////////////////////////////////////////////////////////////////
void CBouy::Render(void)
{
}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// Called by editor to init new object at specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CBouy::EditNew(									// Returns 0 if successfull, non-zero otherwise
	int16_t sX,												// In:  New x coord
	int16_t sY,												// In:  New y coord
	int16_t sZ)												// In:  New z coord
{
  ASSERT(realm()->NavNet());
   int16_t sResult = SUCCESS;
	
	// Use specified position
   position.x = sX;
   position.y = sY;
   position.z = sZ;

	// Since we were created in the editor, set our Nav Net
   m_pParentNavNet = realm()->NavNet();
	if (m_pParentNavNet->AddBouy(this) == SUCCESS)
		sResult = FAILURE;
	else
		// Load resources
		sResult = GetResources();

	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// Called by editor to modify object
////////////////////////////////////////////////////////////////////////////////
int16_t CBouy::EditModify(void)
{
	return SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// Called by editor to move object to specified position
////////////////////////////////////////////////////////////////////////////////
int16_t CBouy::EditMove(									// Returns 0 if successfull, non-zero otherwise
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
// Called by editor to render object
////////////////////////////////////////////////////////////////////////////////
void CBouy::EditRender(void)
{
  flags.Hidden = true;
  if (ms_bShowBouys)
  {
    if (m_pParentNavNet == realm()->NavNet())
    {
      flags.Hidden = false;
      m_phot->SetActive(TRUE);
    }
    else
      m_phot->SetActive(FALSE);
  }

	// Map from 3d to 2d coords
   realm()->Map3Dto2D(position.x, position.y, position.z,
                      m_sX2, m_sY2);

	// Center on image.
   m_sX2	-= m_pImage->m_sWidth / 2;
   m_sY2	-= m_pImage->m_sHeight;

	// Priority is based on bottom edge of sprite
   m_sPriority = position.z;

	// Layer should be based on info we get from attribute map.
   m_sLayer = CRealm::GetLayerViaAttrib(realm()->GetLayer((int16_t) position.x, (int16_t) position.z));

   Object::enqueue(SpriteUpdate); // Update sprite in scene
}

////////////////////////////////////////////////////////////////////////////////
// Give Edit a rectangle around this object
////////////////////////////////////////////////////////////////////////////////
void CBouy::EditRect(RRect* pRect)
{
   realm()->Map3Dto2D(position.x, position.y, position.z,
                      pRect->sX, pRect->sY);

	pRect->sW	= 10;	// Safety.
	pRect->sH	= 10;	// Safety.

	if (m_pImage != nullptr)
		{
		pRect->sW	= m_pImage->m_sWidth;
		pRect->sH	= m_pImage->m_sHeight;
		}

	pRect->sX	-= pRect->sW / 2;
	pRect->sY	-= pRect->sH;
}

////////////////////////////////////////////////////////////////////////////////
// Called by editor to get the hotspot of an object in 2D.
////////////////////////////////////////////////////////////////////////////////
void CBouy::EditHotSpot(	// Returns nothiing.
	int16_t*	psX,				// Out: X coord of 2D hotspot relative to
									// EditRect() pos.
	int16_t*	psY)				// Out: Y coord of 2D hotspot relative to
									// EditRect() pos.
	{
	// Base of bouy is hotspot.
	*psX	= (m_pImage->m_sWidth / 2);
	*psY	= m_pImage->m_sHeight;
	}
#endif // !defined(EDITOR_REMOVED)

////////////////////////////////////////////////////////////////////////////////
// Get all required resources
////////////////////////////////////////////////////////////////////////////////
int16_t CBouy::GetResources(void)						// Returns 0 if successfull, non-zero otherwise
	{
   int16_t sResult = SUCCESS;
	
	if (m_pImage == 0)
		{
		RImage*	pimBouyRes;
      sResult = rspGetResource(&g_resmgrGame, realm()->Make2dResPath(IMAGE_FILE), &pimBouyRes);
		if (sResult == SUCCESS)
			{
			// Allocate image . . .
			m_pImage	= new RImage;
			if (m_pImage != nullptr)
				{
				// Allocate image data . . .
				if (m_pImage->CreateImage(
					pimBouyRes->m_sWidth,
					pimBouyRes->m_sHeight,
					RImage::BMP8) == SUCCESS)
					{
					// Blt bouy res.
					rspBlit(
						pimBouyRes,		// Src.
						m_pImage,		// Dst.
						0,					// Dst.
						0,					// Dst.
						nullptr);			// Dst clip.

					// Put in ID.
					RPrint	print;
					print.SetFont(BOUY_ID_FONT_HEIGHT, &g_fontBig);
					print.SetColor(BOUY_ID_FONT_COLOR);
					print.SetJustifyCenter();
					print.print(
						m_pImage,												// Dst.
						0,															// Dst.
						m_pImage->m_sHeight - BOUY_ID_FONT_HEIGHT,	// Dst.
						"%d",														// Format.
						(int16_t)m_ucID);										// Src.
																					
					// Convert to efficient transparent blit format . . .
					if (m_pImage->Convert(RImage::FSPR8) != RImage::FSPR8)
						{
                  sResult = FAILURE * 3;
						TRACE("CBouy::GetResource() - Couldn't convert to FSPR8\n");
						}
					}
				else
					{
               sResult = FAILURE * 2;
					TRACE("CBouy::GetResource() - m_pImage->CreateImage() failed.\n");
					}

				// If an error occurred after allocation . . .
				if (sResult != SUCCESS)
					{
					delete m_pImage;
					m_pImage	= nullptr;
					}
				}
			else
				{
				sResult = FAILURE;
				TRACE("CBouy::GetResource(): Failed to allocate RImage.\n");
				}
			
			rspReleaseResource(&g_resmgrGame, &pimBouyRes);
			}
		}
	
	return sResult;
	}

////////////////////////////////////////////////////////////////////////////////
// Free all resources
////////////////////////////////////////////////////////////////////////////////
int16_t CBouy::FreeResources(void)						// Returns 0 if successfull, non-zero otherwise
{
	if (m_pImage != nullptr)
		{
		delete m_pImage;
		m_pImage	= nullptr;
		}

	return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Unlink - Visit your direct links and unlink yourself from them, then free
//				your own links.
////////////////////////////////////////////////////////////////////////////////

void CBouy::Unlink(void)
{
	// Follow all of this bouy's direct links and unlink this bouy
   for(const managed_ptr<CBouy>& pLinkedBouy : m_aplDirectLinks)
      pLinkedBouy->m_aplDirectLinks.erase(this);

	// Erase all of your own links to other bouys
   m_aplDirectLinks.clear();

	// Remove this bouy from the network
	m_pParentNavNet->RemoveBouy(m_ucID);
}


////////////////////////////////////////////////////////////////////////////////
// BuildRoutingTable - Fills in the routing table by building a BSF tree and
//							  using the hop counts and parent tree, fills in the
//							  routing table.
////////////////////////////////////////////////////////////////////////////////

int16_t CBouy::BuildRoutingTable(void)
{
	int16_t sResult = SUCCESS;
	uint8_t* aVisited = nullptr;
	uint8_t* aDistance = nullptr;
	uint8_t* aParent = nullptr;
	uint8_t* pucCurrentNode = nullptr;
	uint8_t* pucAdjNode = nullptr;
   managed_ptr<CBouy> pTraverseBouy;

   ASSERT(m_pParentNavNet);
	int16_t sCurrentNumNodes = m_pParentNavNet->GetNumNodes();
	RQueue <uint8_t, 256> bfsQueue;

	// Make sure there is enough space in the routing table, or
	// reallocate it if there isn't enough.
	if (m_sRouteTableSize < sCurrentNumNodes)
	{
		if (m_paucRouteTable != nullptr)
			free(m_paucRouteTable);
		m_paucRouteTable = (uint8_t*) malloc(sCurrentNumNodes);
		m_sRouteTableSize = sCurrentNumNodes;
	}

	// Allocate memory for use in building the BSF tree
	aVisited = (uint8_t*) malloc(sCurrentNumNodes);
	aDistance = (uint8_t*) malloc(sCurrentNumNodes);
	aParent = (uint8_t*) malloc(sCurrentNumNodes);

	if (m_paucRouteTable != nullptr &&
	    aVisited != nullptr &&
		 aDistance != nullptr &&
		 aParent != nullptr)
	{
		// Initialize the table to unreachable and initialize the
		// BSF data structures.
      std::memset(m_paucRouteTable, 255, m_sRouteTableSize);
      std::memset(aVisited, 0, sCurrentNumNodes);
      std::memset(aDistance, 255, sCurrentNumNodes);
      std::memset(aParent, 0, sCurrentNumNodes);

		// Breadth-First Search
		aVisited[m_ucID] = TRUE;
		aDistance[m_ucID] = 0;
		bfsQueue.EnQ(&m_ucID);

		while (!bfsQueue.IsEmpty())
		{
			pucCurrentNode = bfsQueue.DeQ();
			pTraverseBouy = m_pParentNavNet->GetBouy(*pucCurrentNode);
         for(const managed_ptr<CBouy>& pLinkedBouy : pTraverseBouy->m_aplDirectLinks)
         {
				pucAdjNode = &(pLinkedBouy->m_ucID);
				if (aVisited[*pucAdjNode] == FALSE)
				{
					aVisited[*pucAdjNode] = TRUE;
					aParent[*pucAdjNode] = *pucCurrentNode;
					aDistance[*pucAdjNode] = aDistance[*pucCurrentNode] + 1;
					bfsQueue.EnQ(pucAdjNode);
            }
			}
		}

		// Breadth-First Search complete.

		// Now the aDistance contains the hop count to all other connected nodes
		// and aParent provides a way to build the routing table by traversing
		// backwards.

		uint8_t ucCurrentDistance;
		uint8_t curr;
		int16_t j;

		for (j = 1; j < sCurrentNumNodes; j++)
		{
			ucCurrentDistance = aDistance[j];
			// If distance is 255 (infinite flag) then mark that node
			// as unreachable in the routing table using 255 as the flag
			if (ucCurrentDistance == 255)
			{
				m_paucRouteTable[j] = 255;
			}
			else
			{
				// If distance is 0, then this is the node, and mark it in the
				// routing table as 0, meaning you have reached your final destination
				if (j == m_ucID)
				{
					m_paucRouteTable[j] = 0;
				}
				else
				{
					// If the distance is 1, it is a directly connected node, so put
					// its ID in the routing table as the node to go to.
					if (ucCurrentDistance == 1)
					{
						m_paucRouteTable[j] = j;
					}
					else
					{
						// Else, its a child of a directly connected node, so find out which
						// one by tracing back along the parent tree.
						curr = j;

						while (aParent[curr] != m_ucID)
							curr = aParent[curr];

						m_paucRouteTable[j] = curr;
					}
				}
			}
		}
	}
	else
	{
		TRACE("CBouy::BuildRoutingTable: Error allocating memory for tables for bouy %d\n", m_ucID);
		sResult = FAILURE;
	}

	if (aVisited)
		free(aVisited);
	if (aDistance)
		free(aDistance);
	if (aParent)
		free(aParent);

	return sResult;
}

////////////////////////////////////////////////////////////////////////////////
// NextRouteNode - Tells you which node to go to next to get to your destination
////////////////////////////////////////////////////////////////////////////////

uint8_t CBouy::NextRouteNode(uint8_t dst)
{
	if (dst >= m_pParentNavNet->GetNumNodes())
		return 255;
	else
		return m_paucRouteTable[dst];	
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
