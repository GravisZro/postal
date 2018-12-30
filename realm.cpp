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
// realm.cpp
// Project: Postal
//
// This module impliments the CRealm class.
//
// History:
//		12/31/96 MJR	Started.
//
//		01/20/97 MJR	Changed Update loop to save a temporary iterator so
//							objects that delete themselves during the update
//							loop will still have a valid iterator to get the next
//							item in the Things list.
//
//		01/23/97	JMI	Added instantiation of static member ms_asAttribToLayer[].
//
//		01/27/97	JMI	Now 4 bits are used from the attribute to determine the 
//							proper layer for dudes.
//
//		01/28/97	JMI	Added ms_apszLayerNames[] to access the names of the 
//							layers.
//
//		01/29/97	JMI	Reordered ms_apszLayerNames to match the new order in the
//							CRealm::Layers enum (fixed to the way Steve is doing it
//							(the opaque layers are now before alpha equivalents)).
//
//		02/03/97	JMI	The Load(char*) will now Open a SAK with the same base name
//							as the *.rlm file, if one exists.  If it does not exist,
//							m_resmgr's BasePath is set to the No SAK Dir.  Once the
//							load is complete, no more new resources are to be requested
//							from m_resmgr.  To enfore this, m_resmgr's base path is
//							set to a condescending message.  If you are not careful, it
//							may taunt you a second time-a!
//
//		02/04/97 BRH	Temporarily (or permanently) took out timeout check in
//							Startup because it caused problems debugging startup 
//							code.  
//
//		02/04/97 BRH	Added ms_pCurrentNavNet to the realm, moving it from
//							its previous position in gameedit.
//
//		02/10/97	JMI	Now all loops iterate to the next iterator in the STL list
//							before dooing iterative processing just in case the current
//							iterator is invalidated during processing.
//
//		02/13/97	JMI	Added hood pointer to CRealm.
//
//		02/17/97	JMI	Added set lists to CRealm.  Now there is a list for each
//							set of Things.  The sets are enumerated in CThing.  Now
//							new'ed in CRealm() and delete'ed in ~CRealm().
//
//		02/18/97	JMI	Changed uses of CThing::ThingSet to CThing::Things.
//
//		02/19/97	JMI	Got rid of stuff regarding old collision sets and added
//							new CSmashitorium usage.
//
//		02/23/97	JMI	Added m_iNext and m_bUpdating so RemoveThing() and 
//							AddThing() can make the necessary adjustments to the next
//							iterator processed in the Update() loop.
//
//		02/23/97 MJR	Added call to class-based Preload() in CRealm::Load().
//
//		02/24/97	JMI	Added same protection for Render() that we implemented for
//							Update() on 02/23/97.
//
//		02/25/97	JMI	The way I had done the parens for *m_iNext++ for Render()
//							and Update() was scaring me so I separated it out more.
//
//		03/13/97	JMI	Load() now passes the file version number to the CThing
//							Load()'s.
//
//		03/13/97	JMI	Now, instead of the file's version number having to
//							exactly match FileVersion, it must just be less than or
//							equal.  If a file's version is greater than the current
//							version known by the code, the CThing that caused the
//							version number to increase will have more, less, or 
//							different data to load that it cannot possibly know about.
//
//		03/25/97	JMI	Load() no longer opens a SAK with the same title as the
//							.rlm file (This is now opened by the CHood).
//
//		04/09/97 BRH	Added RMultiGrid for the multi layer attribute maps, but
//							haven't taken out the RAttributeMap yet so that the
//							game will continue to work until we completely switch over.
//
//		04/16/97 BRH	Added Jon's template class CListNode head and tail nodes
//							to CRealm to replace the STL containers that provided the
//							linked lists of CThings.  Once the new methods are proven
//							to work, we will get rid of the m_everthing and m_apthings
//							arrays of STL lists.
//
//		04/17/97 BRH	Took timeout check out of Shutdown and Startup (it was
//							already commented out of Startup).  The 1 second timeout
//							when calling shutdown caused large levels like parade
//							to abort before saving everything.
//
//		04/18/97	JMI	Now Suspend() suspends the game time and Resume() resumes
//							it.  This should alleviate the need for the CThing derived
//							objects to do their own time compensation.
//
//		04/21/97	JMI	Added m_sNumSuspends and IsSuspended().
//
//		05/04/97 BRH	Removed STL references since the CListNode lists seem
//							to be working properly.
//
//		05/13/97	JMI	Added IsPathClear() map functions to determine if a path
//							is clear of terrain that cannot be surmounted.
//
//		05/16/97	JMI	Changed layer bits to be 8 bits (instead of 4).  This
//							caused the REALM_ATTR_EFFECT_MASK to lose 4 bits and the
//							REALM_ATTR_LIGHT_BIT to move up 4 bits (0x0010 to 0x0100).
//							Also, the table ms_asAttribToLayer to be increased from
//							16 entries to 256 entries.
//							Also, removed GetLayerFromAttrib() which was left over from
//							the pre RMultiGrid days.
//
//		05/17/97	JMI	In EditUpdate() no argument list was supplied in the line:
//							pCur->m_powner->EditUpdate;
//							Added a set of parens.
//
//		05/26/97	JMI	Added DrawStatus() function.
//
//		05/27/97	JMI	Changed format of DrawStatus() output string.
//							
//		05/29/97	JMI	Changed occurences of m_pHeightMap to m_pTerrainMap.
//							Changed occurences of m_pAttrMap to m_pLayerMap.
//							Also, removed occurences of m_pAttribMap.
//							Also, added CreateLayerMap() that creates the attribute
//							to layer map now that it is so huge.
//
//		06/04/97 BRH	Turned off drawing lines in IsPathClear function.
//
//		06/09/97	JMI	Changed wording of realm status.
//							Also, Suspend() and Resume() pause and resume the sound.
//
//		06/10/97	JMI	Now Resume() does not let you 'over'-resume.
//
//		06/17/97 MJR	Moved some vars that were CPylon statics into the realm
//							so they could be instantiated on a realm-by-realm basis.
//
//		06/20/97 JRD	Replaced code needed to manage allocation and deallocation
//							for the new CSmashatorium
//
//		06/26/97	JMI	Moved Map2DTo3D from reality.h to here.
//							When converting to 2D, Y is now scaled based on the view
//							angle.  This was not applied to Z.  It could be a bug, but
//							it just looked better this way.  I suspect it is a 
//							bug/feature of the way most of the code was written
//							(probably a product of being more aware of the noticable 
//							difference Y coord has when mapped to 2D when comparing 45
//							to 90 degree type levels while originally designing/coding
//							CThings, the editor, and CScene).
//
//		06/28/97	JMI	Moved attribute access functions from realm.h to realm.cpp
//							while we're getting all the conversion from 3D to the X/Z
//							plane stuff right.  Compiling the entire project for any
//							tweak just doesn't sound very fun.  Hopefully, though,
//							there'll won't be many tweaks.
//							Also, added ScaleZ() functions to scale just Z (useful
//							for attribute map access).
//							Changed references to GetWorldRotX() to GetRealmRotX().
//
//		06/29/97	JMI	Added version of ScaleZ() that takes shorts.
//							Changed both versions of ScaleZ() to MapZ3DtoY2D()
//							and added two versions of MapY2DtoZ3D().
//
//		06/30/97	JMI	Now uses CRealm's new GetRealmWidth() and *Height()
//							for dimensions of realm's X/Z plane.
//
//		06/30/97	JMI	Added bCheckExtents parm to IsPathClear().  See proto for
//							details.
//
//		07/01/97	JMI	Added MapY2DtoY3D() and MapY3DtoY2D().
//							Also, GetHeight() now scales the height into realm 
//							coordinates.
//
//		07/01/97	JMI	IsPathClear() had 'step-up' logic (TM) for land based
//							things (like doofuses) that did not work for hovering
//							things (like missiles).  Fixed.
//
//		07/01/97	JMI	Changed the file version for the Hood so it can load and
//							save whether to use the attribute map heights with or
//							without scaling based on the view angle.
//							Also, added function to scale heights that checks the
//							hood value.
//
//		07/07/97 BRH	Added EditModify function to process the realm properties
//							dialog box where you can select the scoring mode and
//							play mode for the realm.
//
//		07/08/97 BRH	Added loading and saving of properties as of version 27.
//
//		07/09/97	JMI	Added function to get the full path for a 2D resource
//							based on the current hood setting for 'Use top-view 2Ds'.
//
//		07/09/97	JMI	Moved m_s2dResPathIndex from CHood to CRealm b/c of order
//							issues when loading.
//
//		07/10/97 BRH	Added cases to IsEndOfLevelGoalMet for the different 
//							scoring modes.
//
//		07/11/97	JMI	Minor change in IsEndOfLevelGoalMet() to make it so, if
//							there are zero births, the goal is considered met.  A
//							special case for example levels that have no enemies.
//
//		07/11/97 BRH	Added time calculations here for expiration date.  Checked
//							in again to update source safe with correct date.
//
//		07/12/97	JMI	Added m_bMultiplayer, which signifies whether this is
//							a multi or single player game.
//							Also, added Init().  See proto for details.
//							Moved things that were being initialized both in CRealm()
//							and in Clear() to Init() which is called by these two
//							functions.
//							Also, moved the initialization of the population statistics
//							into Init() even though they were only being done on a 
//							Clear().  This might be bad but it did not seem like it 
//							could hurt.
//
//		07/12/97 BRH	Made minor change to the EditModify dialog so that the
//							seconds always appears as two digits.
//
//		07/14/97	JMI	Moved initialization of Pylon stuff into Init() and also
//							no m_ucNextPylonId is 1 instead of 0.
//
//		07/14/97 BRH	Fixed a bug that caused the score timer to always count up.
//							Fixed problems with the goal stopping conditions and added
//							m_sFlagbaseCaptured to keep track of flags that were 
//							successfully returned to their base.
//
//		07/15/97 BRH	Added the end of level key flag as a parameter to 
//							the end of level goal check.
//
//		07/16/97 BRH	Fixed a problem with standard scoring mode on levels
//							that didn't have any enemies, they would end right
//							away using the new method of checking the end of
//							the level.
//
//		07/17/97 BRH	Added the time adjustment to the mac version.
//
//		07/27/97 BRH	Added m_lScoreInitialTime and set it to the same
//							initial value as m_lScoreDisplayTimer.  This was required
//							in order to calculate the time elapsed for the high
//							scores.
//
//		07/30/97 BRH	Added a string to hold the path of the realm which will
//							be used by the high score function to identify the
//							current realm file.
//
//		08/05/97	JMI	Now pauses only active sounds so that new sounds can 
//							continue to play while the realm is suspended.
//
//		08/05/97	JMI	Added CRealm::Flags struct and an instance in CRealm, 
//							m_flags.
//
//		08/08/97	JMI	Now displays a useful message about the thing that failed
//							to load should one do so.
//
//		08/09/97	JMI	Added progress callback and components.  There is now a
//							callback member that is called (when non-zero) that passes
//							the number of items processed so far and the total number of
//							items to process.  The return value of the callback dictates
//							whether to proceed with the operation.  Although, this is 
//							meant to be generic so we can use it for any type of 
//							process, it is currently only used by Load() and Save().
//
//		08/11/97 BRH	Changed both Capture the flag goals to flagbases captured
//							rather than flags captured.  
//
//		08/19/97 BRH	Fixed end of level goal for the checkpoint scoring so
//							if a specific number of flags is not set in the realm
//							file, it will just continue until the time runs out.
//
//		08/20/97	JMI	Made ms_apsz2dResPaths[] a static class member (was just
//							defined at realm.cpp file scope) and added enum macro for
//							number of elements.
//
//		08/28/97 BRH	Changed level goals for challenge scoring modes to use
//							the population numbers rather than just the hostile numbers
//							so that the victims count also.
//
//		08/30/97	JMI	IsEndOfLevelGoalMet() now allows a player to go on in
//							Timed and Checkpoint, if they hit the 'next level' key.
//
//		08/30/97	JMI	Now initializes hostile deaths and population deaths in
//							Startup().
//
//		09/02/97	JMI	Now resets all population statistics in Startup().
//
//		09/04/97 BRH	Realm::Load now attempts several paths.  It first tries
//							the path passed in for the case where the user
//							specified a full path using the open dialog.  Then it
//							tries the HD path and then the CD path if those fail.
//							This would allow us to send updated realm files
//							that could be loaded instead of the ones on the CD 
//							just by putting them in the mirror path on the HD.
//
//		09/07/97 BRH	Changed end of level goal to never end the MPFrag if
//							the kills goal is set to zero.  That will mean no 
//							frag limit.
//
//		09/09/97	JMI	Now checks number of flags against the actual number of
//							flags when using CheckPoint.
//
//		09/11/97 MJR	Added DoesFileExist(), which uses the same logic as Load()
//							to determine if a realm file exists.  Also added Open()
//							as a common function for DoesFileExist() and Load() to use.
//
//		09/12/97	JMI	Now, if ENABLE_PLAY_SPECIFIC_REALMS_ONLY is defined, we
//							try to load the .RLM out of memory using 
//							GetMemFileResource().
//
//		09/12/97 MJR	Now Open() will detect an empty filename as an error,
//							which avoids the ASSERT() that the Microsoft Runtime
//							library does when you pass open() and empty string.
//
//		11/21/97	JMI	Added bCoopMode flag indicating whether we're in cooperative
//							or deathmatch mode when in multiplayer.
//
////////////////////////////////////////////////////////////////////////////////

#include "realm.h"


#include <RSPiX.h>

#include <ctime>

#include "game.h"
#include "reality.h"
#include "score.h"
#include "MemFileFest.h"

#include "thing.h"
#include "hood.h"
#include "dude.h"
#include "doofus.h"
#include "rocket.h"
#include "grenade.h" // CGrenade, CDynamite
#include "ball.h"
#include "explode.h"
#include "bouy.h"
#include "navnet.h"
#include "gameedit.h"
#include "napalm.h"
#include "fire.h"
#include "firebomb.h" // CFirebomb, CFirefrag
#include "AnimThing.h"
#include "SoundThing.h"
#include "band.h"
#include "item3d.h"
#include "barrel.h"
#include "mine.h" // CProximityMine, CTimedMine, CBouncingBettyMine, CRemoteControlMine
#include "dispenser.h"
#include "fireball.h" // CFireball, CFirestream
#include "weapon.h" // CPistol, CMachineGun, CShotGun, CAssaultWeapon
#include "person.h"
#include "pylon.h"
#include "PowerUp.h"
#include "ostrich.h"
#include "trigger.h"
#include "heatseeker.h"
#include "chunk.h"
#include "sentry.h"
#include "warp.h"
#include "demon.h"
#include "character.h"
#include "goaltimer.h"
#include "flag.h"
#include "flagbase.h"
#include "deathWad.h"
#include "SndRelay.h"

uint64_t g_things_added = 0;
uint64_t g_things_removed = 0;

//#define RSP_PROFILE_ON


#include <ORANGE/Debug/profile.h>

////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////

// Sets the specified value into the data pointed, if the ptr is not nullptr.
#define SET(ptr, val)        if((ptr) != nullptr) { *(ptr) = (val); }

// Time, in ms, between status updates.
#define STATUS_UPDATE_INTERVAL	1000

#define STATUS_PRINT_X					0
#define STATUS_PRINT_Y					0

#define STATUS_FONT_SIZE				19
#define STATUS_FONT_FORE_INDEX		2
#define STATUS_FONT_BACK_INDEX		0
#define STATUS_FONT_SHADOW_INDEX		0

// Determines the number of elements in the passed array at compile time.
#define NUM_ELEMENTS(a)		(sizeof(a) / sizeof(a[0]) )

#define MAX_SMASH_DIAMETER				20

#define REALM_DIALOG_FILE				"res/editor/realm.gui"

#define TIMER_MIN_EDIT_ID				201
#define TIMER_SEC_EDIT_ID				202
#define KILLS_NUM_EDIT_ID				203
#define KILLS_PCT_EDIT_ID				204
#define FLAGS_NUM_EDIT_ID				205
#define SCORE_MODE_LB_ID				99
#define SCORE_MODE_LIST_BASE			100



////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

// File counter
int16_t CRealm::ms_sFileCount;

// Maps the layer portion of an attribute to the appropriate
// layer.
// Now that this table is 32K, we generate it table at run time to avoid adding
// an extra 32K of uncompressable space to the exe.
int16_t CRealm::ms_asAttribToLayer[CRealm::LayerAttribMask + 1];

// Names of layers.  Use Layer enum values to index.
const char* CRealm::ms_apszLayerNames[TotalLayers]	=
	{
	"Background",

 	"Sprite1",
	"Opaque1",

	"Sprite2",
	"Alpha1",

	"Sprite3",
	"Opaque2",

	"Sprite4",
	"Alpha2",

	"Sprite5",
	"Opaque3",

	"Sprite6",
	"Alpha3",

	"Sprite7",
	"Opaque4",

	"Sprite8",
	"Alpha4",

	"Sprite9",
	"Opaque5",

	"Sprite10",
	"Alpha5",

	"Sprite11",
	"Opaque6",

	"Sprite12",
	"Alpha6",

	"Sprite13",
	"Opaque7",

	"Sprite14",
	"Alpha7",

	"Sprite15",
	"Opaque8",

	"Sprite16",
	};

// These are the various 2d paths that we currently support.  Eventually, if
// there's more than two, this can be presented in listbox form (instead of
// checkbox form).
const char*	CRealm::ms_apsz2dResPaths[Num2dPaths]	=
	{
   "2d/top/",
   "2d/side/",
   "2d/sidebright/",
	};

// Used for CRealm oriented drawing tasks.
static RPrint	ms_print;


////////////////////////////////////////////////////////////////////////////////
// Default (and only) constructor
////////////////////////////////////////////////////////////////////////////////
CRealm::CRealm(void)
	{
	time_t lTime;
	time(&lTime);
#if defined(__unix__)
   // UNIX TIME! adjusment back to UTC time.
	lTime -= ((365 * 70UL) + 17) * 24 * 60 * 60; // time_fudge 1900->1970
#endif
	g_lRegValue = lTime - g_lRegTime;
	g_lExpValue = g_lExpTime - lTime; 

	CreateLayerMap();
	
	// Setup render object (it's constructor was automatically called)
   m_scene.SetLayers(TotalLayers);

	// Set attribute map to a safe (but invalid) value
	m_pTerrainMap = 0;
	m_pLayerMap = 0;
   m_pTriggerMap = 0;

/*
	// Create a container of things for each element in the array
	short	s;
   for (s = 0; s < TotalIDs; s++)
		m_apthings[s] = new CThing::Things;
*/

	m_sNumSuspends	= 0;

	// Setup print.
	ms_print.SetFont(STATUS_FONT_SIZE, &g_fontBig);
	ms_print.SetColor(
		STATUS_FONT_FORE_INDEX, 
		STATUS_FONT_BACK_INDEX, 
		STATUS_FONT_SHADOW_INDEX);

	// Initialize flags to defaults for safety.
	// This might be a bad idea if we want to guarantee they get set in which
	// case we should maybe set them to absurd values.
	m_flags.bMultiplayer	= false;
	m_flags.bCoopMode		= false;
	m_flags.bEditing		= false;
	m_flags.bEditPlay		= false;
	m_flags.sDifficulty	= 5;

	m_fnProgress			= nullptr;

	m_bPressedEndLevelKey = false;

	// Initialize.
	Init();

   Object::connect(Startup, this, &CRealm::started  );
   Object::connect(Suspend, this, &CRealm::suspended);
   Object::connect(Resume , this, &CRealm::resumed  );
   Object::connect(Update , this, &CRealm::updated  );
	}


////////////////////////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////////////////////////
CRealm::~CRealm(void)
	{
	// Clear the realm (in case this hasn't been done yet)
	Clear();

	}


////////////////////////////////////////////////////////////////////////////////
// This will set all values that are to be set on construction and during
// a Clear().  This is called by CRealm() and Clear().  This gives us one 
// spot to implement these, rather than having to do it twice.
////////////////////////////////////////////////////////////////////////////////
void CRealm::Init(void)		// Returns nothing.  Cannot fail.
	{
	m_dKillsPercentGoal = 80.0;
	m_sKillsGoal = 0;
	m_sFlagsGoal = 0;
	m_lScoreTimeDisplay = 0;
	m_lScoreInitialTime = 0;
	m_ScoringMode = Standard;
	m_sFlagsCaptured = 0;
	m_sFlagbaseCaptured = 0;

	// Reset the Population statistics
	m_sPopulationBirths = 0;
	m_sPopulation = 0;
	m_sPopulationDeaths = 0;
	m_sHostiles = 0;
	m_sHostileBirths = 0;
	m_sHostileKills = 0;

	// Initial index for 2D resoruce paths array.
	m_s2dResPathIndex = 1;

	// Reset timer.
	m_lLastStatusDrawTime	= -STATUS_UPDATE_INTERVAL;

	// Pylon stuff
	int16_t i;
	for (i=0;i < 256;i++)
		m_asPylonUIDs[i] = 0; // clear the Pylon UIDs!
	m_sNumPylons = 0;
	m_ucNextPylonID = 1;
	}

////////////////////////////////////////////////////////////////////////////////
// Clear the realm
////////////////////////////////////////////////////////////////////////////////
void CRealm::Clear()
	{
	// Shutdown the realm (in case this hasn't been done yet)
//	Shutdown();

   m_thing_by_type.clear();
   m_thing_by_id.clear();
   m_id_by_thing.clear();

	// Clear out any sprites that didn't already remove themselves
   m_scene.RemoveAllSprites();

	// Reset smashatorium.

#ifdef NEW_SMASH // need to become final at some point...
	m_smashatorium.Destroy();
#else
	m_smashatorium.Reset();
#endif

	// Re-Initialize.
	Init();
	}


////////////////////////////////////////////////////////////////////////////////
// Determine if specified file exists according to same rules used by Load()
////////////////////////////////////////////////////////////////////////////////
// static
bool CRealm::DoesFileExist(							// Returns true if file exists, false otherwise
	const char* pszFileName)							// In:  Name of file
	{
	bool bResult = false;
	RFile file;
   if (Open(pszFileName, &file) == SUCCESS)
		{
		file.Close();
		bResult = true;
		}
	return bResult;
	}



////////////////////////////////////////////////////////////////////////////////
// Open the specified realm file
////////////////////////////////////////////////////////////////////////////////
// static
int16_t CRealm::Open(										// Returns 0 if successfull, non-zero otherwise
	const char* pszFileName,							// In:  Name of file to load from
	RFile* pfile)											// I/O: RFile to be used
	{
	int16_t sResult = SUCCESS;
	
	if (strlen(pszFileName) > 0)
		{

		#if !defined(ENABLE_PLAY_SPECIFIC_REALMS_ONLY)
			// Try the given path first since it may already have a full path in the
			// case of loading a level, then try the path with the HD path prepended, 
			// then try the CD path.
         sResult = pfile->Open(rspPathToSystem(pszFileName), "rb", RFile::LittleEndian);
			if (sResult != SUCCESS)
				{
            char pszFullPath[PATH_MAX];
            std::strcpy(pszFullPath, FullPathHD(pszFileName));
            sResult = pfile->Open(pszFullPath, "rb", RFile::LittleEndian);
				if (sResult != SUCCESS)
					{
               std::strcpy(pszFullPath, FullPathCD(pszFileName));
               sResult = pfile->Open(pszFullPath, "rb", RFile::LittleEndian);
					}
				}
		#else
			// There's only one place it can possibly be and, if it's not there,
			// no realm for you!
			sResult	= GetMemFileResource(pszFileName, RFile::LittleEndian, pfile);
		#endif	// ENABLE_PLAY_SPECIFIC_REALMS_ONLY
		}
	else
		{
		sResult = FAILURE;
		TRACE("CRealm::Open(): Empty file name!\n");
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Load the realm
////////////////////////////////////////////////////////////////////////////////
int16_t CRealm::Load(										// Returns 0 if successfull, non-zero otherwise
	const char* pszFileName,							// In:  Name of file to load from
	bool bEditMode)										// In:  Use true for edit mode, false otherwise
	{
	int16_t sResult = SUCCESS;

	// Copy the name to use later for high score purposes
	m_rsRealmString = pszFileName;

	// Open file
	RFile file;
	sResult = Open(pszFileName, &file);
   if (sResult == SUCCESS)
		{
		// Use alternate load to do most of the work
		sResult = Load(&file, bEditMode);

		file.Close();
		}
	else
		{
		sResult = FAILURE;
		TRACE("CRealm::Load(): Couldn't open file: %s !\n", pszFileName);
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Load realm
////////////////////////////////////////////////////////////////////////////////
int16_t CRealm::Load(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile,											// In:  File to load from
	bool bEditMode)										// In:  Use true for edit mode, false otherwise
	{
	int16_t sResult = SUCCESS;
	
	// Clear the realm before loading this new stuff
	Clear();

	// Increment file count
	ms_sFileCount++;

	// Read & validate file ID
	uint32_t ulFileID;
	if (pFile->Read(&ulFileID) == 1)
		{
		if (ulFileID == CRealm::FileID)
			{

			// Read & validate file version
			uint32_t ulFileVersion;
			if (pFile->Read(&ulFileVersion) == 1)
				{
				// If a known version . . .
				if (ulFileVersion <= CRealm::FileVersion)
					{
					// Read properties for the realm
					switch (ulFileVersion)
					{
						default:
						case 30:
							pFile->Read(&m_s2dResPathIndex);

						case 29:
							pFile->Read(&m_ScoringMode);
						case 28:
						case 27:
						{
							int16_t sUp;
							pFile->Read(&m_lScoreTimeDisplay);
							m_lScoreInitialTime = m_lScoreTimeDisplay;
							pFile->Read(&sUp);
							if (sUp == 1)
								m_bScoreTimerCountsUp = true;
							else
								m_bScoreTimerCountsUp = false;
							pFile->Read(&m_sKillsGoal);
							pFile->Read(&m_sFlagsGoal);
							pFile->Read(&m_dKillsPercentGoal);
							break;
						}
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
							break;
					}

#if defined (RELEASE) && 0
					// Scan through class info structs and for each non-0 preload func,
					// call it to give that class a chance to preload stuff.  The intention
					// is to give classes whose objects don't exist at the start of a level
					// a chance to preload resources now rather than during gameplay.
               for (int16_t sPre = 0; sPre < TotalIDs; sPre++)
						{
						CThing::FuncPreload func = CThing::ms_aClassInfo[sPre].funcPreload;
						if (func != 0)
							{
							sResult = (*func)(this);
							if (sResult != SUCCESS)
								{
								TRACE("CRealm::Load(): Error reported by Preload() for CThing class ID = %hd\n", (int16_t)sPre);
								break;
								}
							}
						}
#endif
               if (sResult == SUCCESS)
						{

						// Read number of things that were written to file (could be 0!)
						palindex_t sCount;
						if (pFile->Read(&sCount) == 1)
							{

                     uint8_t	idLastThingLoaded	= TotalIDs;

							// If there's a callback . . .
							if (m_fnProgress)
								{
								// Call it . . .
								if (m_fnProgress(0, sCount) == true)
									{
									// Callback is happy to continue.
									}
								else
									{
									// Callback has decided to end this operation.
									sResult = FAILURE;
									}
								}

							// Load each object that was written to the file (could be 0!)
							for (int16_t s = 0; (s < sCount) && !sResult; s++)
								{

								// Read class ID of next object in file
                        uint8_t type_id;
                        uint16_t instance_id;
                        if (pFile->Read(&type_id) == 1 &&
                            pFile->Read(&instance_id) == 1)
                        {
                          auto pThing = GetOrAddThingById<CThing>(instance_id, ClassIDType(type_id)); // find or make new thing
                           if (pThing)
                           {
                             if(type_id == CNavigationNetID)
                               m_navnet = pThing;
                             if(type_id == CHoodID)
                               m_hood = pThing;

										// Load object assocated with this class ID
										sResult = pThing->Load(pFile, bEditMode, ms_sFileCount, ulFileVersion);

										// If successful . . .
                              if (sResult == SUCCESS)
											{
											// Store last thing to successfully load.
                                 idLastThingLoaded	= type_id;
											// If there's a callback . . .
											if (m_fnProgress)
												{
												// Call it . . .
												if (m_fnProgress(s + 1, sCount) == true)
													{
													// Callback is happy to continue.
													}
												else
													{
													// Callback has decided to end this operation.
													sResult = FAILURE;
													}
												}
											}
										}
									}
								else
									{
									sResult = FAILURE;
									TRACE("CRealm::Load(): Error reading class ID!\n");
									}
								}

							// Check for I/O errors (only matters if no errors were reported so far)
							if (!sResult && pFile->Error())
								{
								sResult = FAILURE;
								TRACE("CRealm::Load(): Error reading file!\n");
								}

							// If any errors occurred . . .
							if (sResult)
								{
								// Better clean up stuff that did load.
								Clear();
								}
							}
						else
							{
							sResult = FAILURE;
							TRACE("CRealm::Load(): Error reading count of objects in file!\n");
							}
						}
					}
				else
					{
					sResult = FAILURE;
					TRACE("CRealm::Load(): Incorrect file version (should be 0x%lx or less, was 0x%lx)!\n", CRealm::FileVersion, ulFileVersion);
					}
				}
			else
				{
				sResult = FAILURE;
				TRACE("CRealm::Load(): Error reading file version!\n");
				}
			}
		else
			{
			sResult = FAILURE;
			TRACE("CRealm::Load(): Incorrect file ID (should be 0x%lx, was 0x%lx)!\n", CRealm::FileID, ulFileID);
			}
		}
	else
		{
		sResult = FAILURE;
		TRACE("CRealm::Load(): Error reading file ID!\n");
		}

#ifdef NEW_SMASH
   if (sResult == SUCCESS) // a success....
		{
		/* For now, let's see if this is necessary...

		// Allocate the Smashatorium:
		// Kill old...*
		short	sOldW = m_smashatorium.m_sWorldW;
		short	sOldH = m_smashatorium.m_sWorldH;
		short sOldTileW = m_smashatorium.m_sTileW;
		short sOldTileH = m_smashatorium.m_sTileH;


		if (m_smashatorium.m_pGrid) m_smashatorium.Destroy();

		if (m_smashatorium.Alloc(sOldW,sOldH,sOldTileW,sOldTileH) != SUCCESS)
			{
			TRACE("CRealm::Load(): Error reallocating the smashatorium!\n");
			sResult = FAILURE;
			}
		*/
		}
#endif

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Save the realm
////////////////////////////////////////////////////////////////////////////////
int16_t CRealm::Save(										// Returns 0 if successfull, non-zero otherwise
	const char* pszFile)									// In:  Name of file to save to
	{
	int16_t sResult = SUCCESS;

	// Open file
	RFile file;
	sResult = file.Open((char*)pszFile, "wb", RFile::LittleEndian);
   if (sResult == SUCCESS)
		{

		// Use alternate save to do most of the work
		sResult = Save(&file);

		file.Close();

		// Would this be an appropriate time to build the SAK file???
		}
	else
		{
		sResult = FAILURE;
		TRACE("CRealm::Save(): Couldn't open file: %s !\n", pszFile);
		}

	return sResult;
	}


////////////////////////////////////////////////////////////////////////////////
// Save the realm
////////////////////////////////////////////////////////////////////////////////
int16_t CRealm::Save(										// Returns 0 if successfull, non-zero otherwise
	RFile* pFile)											// In:  File to save to
	{
	int16_t sResult = SUCCESS;

	// Increment file count
	ms_sFileCount++;

	// Write out file ID and version
	pFile->Write((uint32_t)CRealm::FileID);
	pFile->Write((uint32_t)CRealm::FileVersion);

	// Save properties for the realm
	pFile->Write(m_s2dResPathIndex);
	pFile->Write(&m_ScoringMode);
	pFile->Write(&m_lScoreTimeDisplay);
	int16_t sUp = 0;
	if (m_bScoreTimerCountsUp)
		sUp = 1;
	pFile->Write(&sUp);
	pFile->Write(&m_sKillsGoal);
	pFile->Write(&m_sFlagsGoal);
	pFile->Write(&m_dKillsPercentGoal);

   int16_t count = 0;
   for(auto& pair : m_thing_by_type)
     count += pair.second.size();

	// Write out number of objects
   pFile->Write(count);

	// If there's a callback . . .
	if (m_fnProgress)
		{
		// Call it . . .
      if (m_fnProgress(0, count) == true)
			{
			// Callback is happy to continue.
			}
		else
			{
			// Callback has decided to end this operation.
			sResult = FAILURE;
			}
		}

   int16_t	sCurItemNum	= 0;
   for(const managed_ptr<CThing>& pthing : m_every_thing)
   {
     pFile->Write(uint8_t(pthing->type()));
     pFile->Write(pthing->GetInstanceID());
     sResult = pthing->Save(pFile, ms_sFileCount);
     if(!sResult)
       break;
     sCurItemNum++;
   }

	// Check for I/O errors (only matters if no errors were reported so far)
	if (!sResult && pFile->Error())
		{
		sResult = FAILURE;
		TRACE("CRealm::Save(): Error writing file!\n");
		}

	return sResult;
	}

#if !defined(EDITOR_REMOVED)
////////////////////////////////////////////////////////////////////////////////
// EditModify - Run dialog for realm scoring and play options
////////////////////////////////////////////////////////////////////////////////
void CRealm::EditModify(void)
{
	RGuiItem*	pguiRoot	= RGuiItem::LoadInstantiate(FullPathVD(REALM_DIALOG_FILE));
	RProcessGui	guiDialog;

	if (pguiRoot != nullptr)
	{
		RGuiItem*	pguiOk		= pguiRoot->GetItemFromId(1);
		RGuiItem*	pguiCancel	= pguiRoot->GetItemFromId(2);

		REdit* peditMinutes = (REdit*) pguiRoot->GetItemFromId(TIMER_MIN_EDIT_ID);
		REdit* peditSeconds = (REdit*) pguiRoot->GetItemFromId(TIMER_SEC_EDIT_ID);
		REdit* peditKillsNum = (REdit*) pguiRoot->GetItemFromId(KILLS_NUM_EDIT_ID);
		REdit* peditKillsPct = (REdit*) pguiRoot->GetItemFromId(KILLS_PCT_EDIT_ID);
		REdit* peditFlagsNum = (REdit*) pguiRoot->GetItemFromId(FLAGS_NUM_EDIT_ID);
		RListBox* plbScoreModes = (RListBox*) pguiRoot->GetItemFromId(SCORE_MODE_LB_ID);
		RGuiItem* pguiItem = nullptr;
		int32_t lMinutes;
		int32_t lSeconds;

		if (peditMinutes != nullptr && peditSeconds != nullptr && peditKillsNum != nullptr &&
		    peditKillsPct != nullptr && peditFlagsNum != nullptr && plbScoreModes != nullptr)
		{
			ASSERT(peditMinutes->m_type == RGuiItem::Edit);
			ASSERT(peditSeconds->m_type == RGuiItem::Edit);
			ASSERT(peditKillsNum->m_type == RGuiItem::Edit);
			ASSERT(peditKillsPct->m_type == RGuiItem::Edit);
			ASSERT(peditFlagsNum->m_type == RGuiItem::Edit);
			ASSERT(plbScoreModes->m_type == RGuiItem::ListBox);

			lMinutes = m_lScoreTimeDisplay / 60000;
			lSeconds = (m_lScoreTimeDisplay / 1000) % 60;

         peditMinutes->SetText("%i", lMinutes);
			peditSeconds->SetText("%2.2ld", lSeconds);
			peditKillsNum->SetText("%d", m_sKillsGoal);
			peditKillsPct->SetText("%3.1f", m_dKillsPercentGoal);
			peditFlagsNum->SetText("%d", m_sFlagsGoal);
			peditMinutes->Compose();
			peditSeconds->Compose();
			peditKillsNum->Compose();
			peditKillsPct->Compose();
			peditFlagsNum->Compose();
			
			pguiItem = plbScoreModes->GetItemFromId(SCORE_MODE_LIST_BASE + m_ScoringMode);
			if (pguiItem != nullptr)
			{
				plbScoreModes->SetSel(pguiItem);
				plbScoreModes->AdjustContents();
				plbScoreModes->EnsureVisible(pguiItem);

			}
			
			if (guiDialog.DoModal(pguiRoot, pguiOk, pguiCancel) == 1)
			{
				lMinutes = peditMinutes->GetVal();
				lSeconds = peditSeconds->GetVal() % 60;
				m_lScoreInitialTime = m_lScoreTimeDisplay = (lMinutes * 60000) + (lSeconds * 1000);
				if (m_lScoreTimeDisplay == 0)
					m_bScoreTimerCountsUp = true;
				else
					m_bScoreTimerCountsUp = false;
				m_sKillsGoal = (int16_t) peditKillsNum->GetVal();
				m_sFlagsGoal = (int16_t) peditFlagsNum->GetVal();
				m_dKillsPercentGoal = (double) peditKillsPct->GetVal();

				pguiItem = plbScoreModes->GetSel();
				if (pguiItem != nullptr)
					m_ScoringMode = pguiItem->m_lId - SCORE_MODE_LIST_BASE;
			}
		}
	}
}
#endif // !defined(EDITOR_REMOVED)

#if defined(__ANDROID__)
extern "C"
{
#include "android/android.h"
}
#endif
////////////////////////////////////////////////////////////////////////////////
// IsEndOfLevelGoalMet - check to see if level is complete based on the
//								 scoring and game play mode.
////////////////////////////////////////////////////////////////////////////////
bool CRealm::IsEndOfLevelGoalMet(bool bEndLevelKey)
{
#if defined(__ANDROID__)
	bool showAndroidKey = true;
	switch (m_ScoringMode)
	{
	case Standard:
		if (m_sHostileBirths != 0)
			if (((m_sHostileKills * 100) / m_sHostileBirths < m_dKillsPercentGoal))
				showAndroidKey = false;
		break;
	default: //Hide the next key for anything else for the moment
		showAndroidKey = false;
	}
	AndroidSetShowEndLevelKey(showAndroidKey);
#endif

	bool bEnd = true;

	if (m_bPressedEndLevelKey)
	{
		m_bPressedEndLevelKey = false;
		// Hack: don't let the level end immediately if the player is using the debug level skip
		if (m_time.GetGameTime() > 1000)
			bEndLevelKey = true;
	}

	switch (m_ScoringMode)
	{
		// In a standard level, the user is done when the percentage of hostiles killed
		// is greater than the minimum set in the level and the user presses the
		// 'next level' key.
		case Standard:
			if (m_sHostileBirths != 0)
				{
				if (((m_sHostileKills * 100) / m_sHostileBirths < m_dKillsPercentGoal) || !bEndLevelKey)
					bEnd = false;
				}
				else
				{
					if (!bEndLevelKey)
						bEnd = false;
				}
			break;

		// In a timed level, the user is done when the time runs out, the population
		// runs out, or the user presses the 'next level' key.
		case Timed:
			if (m_lScoreTimeDisplay > 0 && m_sPopulation > 0 && !bEndLevelKey)
				bEnd = false;
			break;

		// In a timed goal level, the user must meet the goal within the specified
		// time.
		case TimedGoal:
			if (m_lScoreTimeDisplay > 0 && m_sPopulationDeaths < m_sKillsGoal)
				bEnd = false;
			break;

		// In a timed flag level, the user must get the flag to a base before
		// the goal is considered met.
		case TimedFlag:
		case MPTimedFlag:
//			if (m_lScoreTimeDisplay > 0 && m_sFlagsCaptured < m_sFlagsGoal)
			if (m_lScoreTimeDisplay > 0 && m_sFlagbaseCaptured < m_sFlagsGoal)
				bEnd = false;
			break;

		// In a capture the flag level, a user must capture a flag and return it
		// to a base to complete the level.
		case CaptureFlag:
		case MPCaptureFlag:
			if (m_sFlagbaseCaptured < m_sFlagsGoal)
				bEnd = false;
			break;

		// In a goal level, the user can only be done when they meet the goal.
		case Goal:
			if (m_sPopulationDeaths < m_sKillsGoal)
				bEnd = false;
			break;

		// In a checkpoint level, the user collects as many flags as possible and
		// can choose to end the level whenever they want (they'll just get a lower
		// score, if they have not gotten all the flags).
		case Checkpoint:
			if (m_sFlagsGoal == 0)
			{
            if (m_lScoreTimeDisplay > 0 && m_sFlagsCaptured < int16_t(GetThingsByType(CFlagID).size()))
					bEnd = false;
			}
			else
			{
				if (m_lScoreTimeDisplay > 0 && m_sFlagsCaptured < m_sFlagsGoal && !bEndLevelKey)
					bEnd = false;
			}
			break;

		case MPFrag:
			// Get highest number of kills from score module and 
			if ((m_sKillsGoal < 1) || (ScoreHighestKills(this) < m_sKillsGoal))
				bEnd = false;
			break;

		case MPTimedFrag:
			if (m_lScoreTimeDisplay > 0 && ScoreHighestKills(this) < m_sKillsGoal)
				bEnd = false;
			break;

		case MPLastMan:
			// if (ScorePlayersRemaining() > 1)
				bEnd = false;
			break;

		case MPTimed:
			if (m_lScoreTimeDisplay > 0)
				bEnd = false;
			break;
	}

#if defined(DEBUG_LEVEL_CHEAT)
	bEnd = bEndLevelKey;
#endif

	return bEnd;
}


////////////////////////////////////////////////////////////////////////////////
// Determine if a path is clear of terrain.
////////////////////////////////////////////////////////////////////////////////
bool CRealm::IsPathClear(			// Returns true, if the entire path is clear.
											// Returns false, if only a portion of the path is clear.
											// (see *psX, *psY, *psZ).
	int16_t sX,							// In:  Starting X.
	int16_t	sY,							// In:  Starting Y.
	int16_t sZ,							// In:  Starting Z.
	int16_t sRotY,						// In:  Rotation around y axis (direction on X/Z plane).
	double dCrawlRate,				// In:  Rate at which to scan ('crawl') path in pixels per
											// iteration.
											// NOTE: Values less than 1.0 are inefficient.
											// NOTE: We scan terrain using GetHeight()
											// at only one pixel.
											// NOTE: We could change this to a speed in pixels per second
											// where we'd assume a certain frame rate.
	int16_t	sDistanceXZ,				// In:  Distance on X/Z plane.
	int16_t sVerticalTolerance /*= 0*/,	// In:  Max traverser can step up.
	int16_t* psX /*= nullptr*/,			// Out: If not nullptr, last clear point on path.
	int16_t* psY /*= nullptr*/,			// Out: If not nullptr, last clear point on path.
	int16_t* psZ /*= nullptr*/,			// Out: If not nullptr, last clear point on path.
	bool bCheckExtents /*= true*/)	// In:  If true, will consider the edge of the realm a path
												// inhibitor.  If false, reaching the edge of the realm
												// indicates a clear path.
	{
	bool	bEntirelyClear	= false;	// Assume entire path is not clear.

	////////////////////////// Traverse path ///////////////////////////////////

	// Get most efficient increments that won't miss any attributes.
	// For the rates we use trig with a hypotenuse of 1 which will give
	// us a rate <= 1.0 and then multiply by the the crawl for
	// a reasonable increase in the speed of this alg.
	
	// sAngle must be between 0 and 359.
	sRotY	= rspMod360(sRotY);

	float	fRateX		= COSQ[sRotY] * dCrawlRate;
	float	fRateZ		= -SINQ[sRotY] * dCrawlRate;
	float	fRateY		= 0.0;	// If we ever want vertical movement . . .

	// Set initial position to first point to check (NEVER checks original position).
	float	fPosX			= sX + fRateX;
	float	fPosY			= sY + fRateY;
	float	fPosZ			= sZ + fRateZ;

	// Determine amount traveled per iteration on X/Z plane just once.
	float	fIterDistXZ		= rspSqrt(ABS2(fRateX, fRateZ) );

	float	fTotalDistXZ	= 0.0F;

	// Store extents.
	int16_t	sMaxX			= GetRealmWidth();
	int16_t	sMaxZ			= GetRealmHeight();

	int16_t	sMinX			= 0;
	int16_t	sMinZ			= 0;

	int16_t	sCurH;

	bool	bInsurmountableHeight	= false;

	// Scan while in realm.
	while (
			fPosX > sMinX 
		&& fPosZ > sMinZ 
		&& fPosX < sMaxX 
		&& fPosZ < sMaxZ
		&& fTotalDistXZ < sDistanceXZ)
		{
		sCurH	= GetHeight((int16_t)fPosX, (int16_t)fPosZ);
		// If too big a height difference . . .
		if (sCurH - fPosY > sVerticalTolerance)
			{
			bInsurmountableHeight	= true;
			break;
			}

		// Update position.
		fPosX	+= fRateX;
		fPosY	=	MAX(fPosY, (float)sCurH);
		fPosZ	+= fRateZ;
		// Update distance travelled on X/Z plane.
		fTotalDistXZ	+= fIterDistXZ;
		}

	// Set end pt.
	SET(psX, fPosX);
	SET(psY, fPosY);
	SET(psZ, fPosZ);

	// If we made it the whole way . . .
	if (fTotalDistXZ >= sDistanceXZ)
		{
		bEntirelyClear	= true;
		}
	// Else, if we didn't hit any terrain . . .
	else if (bInsurmountableHeight == false)
		{
		// Only clear if we are not checking extents.
		bEntirelyClear	= !bCheckExtents;
		}

#if 0
	// FEEDBACK.
	// Create a line sprite.
	CSpriteLine2d*	psl2d	= new CSpriteLine2d;
	if (psl2d != nullptr)
		{
		Map3Dto2D(
			sX, 
			sY, 
			sZ, 
			&(psl2d->m_sX2), 
			&(psl2d->m_sY2) );
		Map3Dto2D(
			fPosX, 
			fPosY, 
			fPosZ, 
			&(psl2d->m_sX2End), 
			&(psl2d->m_sY2End) );
		psl2d->m_sPriority	= sZ;
		psl2d->m_sLayer		= GetLayerViaAttrib(GetLayer(sX, sZ));
		psl2d->m_u8Color		= (bEntirelyClear == false) ? 249 : 250;
		// Destroy when done.
      psl2d->flags.DeleteOnRender = true;
		// Put 'er there.
      m_scene.UpdateSprite(psl2d);
		}
#endif

	return bEntirelyClear;
	}

////////////////////////////////////////////////////////////////////////////////
// Determine if a path is clear of terrain.
////////////////////////////////////////////////////////////////////////////////
bool CRealm::IsPathClear(			// Returns true, if the entire path is clear.
											// Returns false, if only a portion of the path is clear.
											// (see *psX, *psY, *psZ).
	int16_t sX,							// In:  Starting X.
	int16_t	sY,							// In:  Starting Y.
	int16_t sZ,							// In:  Starting Z.
	double dCrawlRate,				// In:  Rate at which to scan ('crawl') path in pixels per
											// iteration.
											// NOTE: Values less than 1.0 are inefficient.
											// NOTE: We scan terrain using GetHeight()
											// at only one pixel.
											// NOTE: We could change this to a speed in pixels per second
											// where we'd assume a certain frame rate.
	int16_t	sDstX,						// In:  Destination X.
	int16_t	sDstZ,						// In:  Destination Z.
	int16_t sVerticalTolerance /*= 0*/,	// In:  Max traverser can step up.
	int16_t* psX /*= nullptr*/,			// Out: If not nullptr, last clear point on path.
	int16_t* psY /*= nullptr*/,			// Out: If not nullptr, last clear point on path.
	int16_t* psZ /*= nullptr*/,			// Out: If not nullptr, last clear point on path.
	bool bCheckExtents /*= true*/)	// In:  If true, will consider the edge of the realm a path
												// inhibitor.  If false, reaching the edge of the realm
												// indicates a clear path.
	{
	int16_t	sDistanceXZ	= rspSqrt(ABS2(sDstX - sX, sZ - sDstZ) );
	int16_t	sRotY			= rspATan(sZ - sDstZ, sDstX - sX);

	return IsPathClear(		// Returns true, if the entire path is clear.
									// Returns false, if only a portion of the path is clear.
									// (see *psX, *psY, *psZ).
		sX,						// In:  Starting X.
		sY,						// In:  Starting Y.
		sZ,						// In:  Starting Z.
		sRotY,					// In:  Rotation around y axis (direction on X/Z plane).
		dCrawlRate,				// In:  Rate at which to scan ('crawl') path in pixels per
									// iteration.
									// NOTE: Values less than 1.0 are inefficient.
									// NOTE: We scan terrain using GetHeight()
									// at only one pixel.
									// NOTE: We could change this to a speed in pixels per second
									// where we'd assume a certain frame rate.
		sDistanceXZ,			// In:  Distance on X/Z plane.
		sVerticalTolerance,	// In:  Max traverser can step up.
		psX,						// Out: If not nullptr, last clear point on path.
		psY,						// Out: If not nullptr, last clear point on path.
		psZ,						// Out: If not nullptr, last clear point on path.
		bCheckExtents);		// In:  If true, will consider the edge of the realm a path
									// inhibitor.  If false, reaching the edge of the realm
									// indicates a clear path.
	}

////////////////////////////////////////////////////////////////////////////////
// Gives this realm an opportunity and drawing surface to display its 
// current status.
////////////////////////////////////////////////////////////////////////////////
void CRealm::DrawStatus(	// Returns nothing.
	RImage*	pim,				// In:  Image in which to draw status.
	RRect*	prc)				// In:  Rectangle in which to draw status.  Clips to.
	{
	int32_t	lCurTime	= m_time.GetGameTime();
	if (lCurTime > m_lLastStatusDrawTime + STATUS_UPDATE_INTERVAL)
		{
		// Set print/clip to area.
		RRect	rcDst;
		rcDst.sX	= prc->sX + STATUS_PRINT_X;
		rcDst.sY = prc->sY + STATUS_PRINT_Y;
		rcDst.sW = prc->sW - STATUS_PRINT_X;
		rcDst.sH	= prc->sH - STATUS_PRINT_Y;
		// Clear.
		rspRect(RSP_BLACK_INDEX, pim, rcDst.sX, rcDst.sY, rcDst.sW, rcDst.sH);

		ms_print.SetDestination(pim, &rcDst);
		ms_print.print(
			pim, 
			rcDst.sX, 
			rcDst.sY, 
			"      Population %d                 Body Count %d (%d%%)                       Goal %d%%",
			m_sPopulationBirths,
			m_sPopulationDeaths,
			m_sPopulationDeaths * 100 / ((m_sPopulationBirths != 0) ? m_sPopulationBirths : 1),
			(int16_t)m_dKillsPercentGoal
			);

		m_lLastStatusDrawTime	= lCurTime;
		}
	}

////////////////////////////////////////////////////////////////////////////////
// If enabled, scales the specified height based on the view angle.
////////////////////////////////////////////////////////////////////////////////
void CRealm::MapAttribHeight(	// Returns nothing.
   int16_t  sHIn,					// In.
   int16_t&	sHOut)				// Out.
	{
	// If scaling attrib map heights . . .
   if (m_hood->m_sScaleAttribHeights != FALSE)
		{
      int16_t	sRotX	= m_hood->GetRealmRotX();

		// Scale into realm.
      ::MapY2DtoY3D(sHIn, sHOut, sRotX);
		}
	else
		{
      sHOut	= sHIn;
		}
	}

////////////////////////////////////////////////////////////////////////////////
//// Terrrain map access functions /////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Note these had no comments describing their function so I made some very
// vague comments that I hope were accurate -- JMI	06/28/97.

// Get the terrain height at an x/z position.
// Zero, if off map.
int16_t CRealm::GetHeight(int16_t sX, int16_t sZ)
	{
   int16_t	sRotX	= m_hood->GetRealmRotX();
	// Scale the Z based on the view angle.
   ::MapZ3DtoY2D(sZ, sZ, sRotX);

	int16_t	sH = 4 * (m_pTerrainMap->GetVal(sX, sZ, 0x0000) & REALM_ATTR_HEIGHT_MASK); 

	// Scale into realm.
   MapAttribHeight(sH, sH);

	return sH;
	}

// Get the height and 'not walkable' status at the specified location.
// 'No walk', if off map.
int16_t CRealm::GetHeightAndNoWalk(	// Returns height at new location.
	int16_t sX,								// In:  X position to check on map.
	int16_t	sZ,								// In:  Z position to check on map.
	bool* pbNoWalk)						// Out: true, if 'no walk'.
	{
   int16_t	sRotX	= m_hood->GetRealmRotX();
	// Scale the Z based on the view angle.
   ::MapZ3DtoY2D(sZ, sZ, sRotX);

	uint16_t	u16Attrib	= m_pTerrainMap->GetVal(sX, sZ, REALM_ATTR_NOT_WALKABLE);

	int16_t	sH = 4 * (u16Attrib & REALM_ATTR_HEIGHT_MASK); 

	// Scale into realm.
   MapAttribHeight(sH, sH);

	// Get 'no walk'.
	if (u16Attrib & REALM_ATTR_NOT_WALKABLE)
		{
		*pbNoWalk	= true;
		}
	else
		{
		*pbNoWalk	= false;
		}

	return sH;
	}

// Get the terrain attributes at an x/z position.
// 'No walk', if off map.
int16_t CRealm::GetTerrainAttributes(int16_t sX, int16_t sZ)
	{
	// Scale the Z based on the view angle.
   ::MapZ3DtoY2D(sZ, sZ, m_hood->GetRealmRotX());

	return m_pTerrainMap->GetVal(sX, sZ, REALM_ATTR_NOT_WALKABLE); 
	}

// Get the floor attributes at an x/z position.
// Zero, if off map.
int16_t CRealm::GetFloorAttribute(int16_t sX, int16_t sZ)
	{
	// Scale the Z based on the view angle.
   ::MapZ3DtoY2D(sZ, sZ, m_hood->GetRealmRotX());

	return m_pTerrainMap->GetVal(sX, sZ, 0) & REALM_ATTR_FLOOR_MASK; 
	}

// Get the floor value at an x/z position.
// sMask, if off map.
int16_t CRealm::GetFloorMapValue(int16_t sX, int16_t sZ, int16_t sMask/* = 0x007F*/)
	{
	// Scale the Z based on the view angle.
   ::MapZ3DtoY2D(sZ, sZ, m_hood->GetRealmRotX());

	return m_pTerrainMap->GetVal(sX, sZ, sMask); 
	}

// Get the all alpha and opaque layer bits at an x/z position.
// Zero, if off map.
int16_t CRealm::GetLayer(int16_t sX, int16_t sZ)
	{
	// Scale the Z based on the view angle.
   ::MapZ3DtoY2D(sZ, sZ, m_hood->GetRealmRotX());

	return m_pLayerMap->GetVal(sX, sZ, 0) & REALM_ATTR_LAYER_MASK; 
	}

// Get effect attributes at an x/z position.
// Zero, if off map.
int16_t CRealm::GetEffectAttribute(int16_t sX, int16_t sZ)
	{
	// Scale the Z based on the view angle.
   ::MapZ3DtoY2D(sZ, sZ, m_hood->GetRealmRotX());

	return m_pTerrainMap->GetVal(sX, sZ, 0) & REALM_ATTR_EFFECT_MASK; 
	}

// Get effect value at an x/z position.
// Zero, if off map.
int16_t CRealm::GetEffectMapValue(int16_t sX, int16_t sZ)
	{
	// Scale the Z based on the view angle.
   ::MapZ3DtoY2D(sZ, sZ, m_hood->GetRealmRotX());

	return m_pTerrainMap->GetVal(sX, sZ, 0); 
	}
	
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Makes a 2D path based on the current hood setting for 'Use top-view 2Ds'.
// Note that this function returns to you a ptr to its one and only static
// string of length PATH_MAX.  Do not write to this string and do not
// store this string.  It is best to just use this call to pass a string to
// a function that will just use it right away (i.e., will not store it or
// modify it).
////////////////////////////////////////////////////////////////////////////////
const char* CRealm::Make2dResPath(	// Returns a ptr to an internal static buffer
												// containing the passed string, pszResName,
												// preceded by the appropriate directory based
												// on the current hood settings.
	const char* pszResName)				// In:  Resource name to prepend path to.
	{
   static char	szFullPath[PATH_MAX];

	ASSERT(m_s2dResPathIndex < NUM_ELEMENTS(ms_apsz2dResPaths) );

	// Get resource path.
   const char*	pszPath	= ms_apsz2dResPaths[m_s2dResPathIndex];
	
	ASSERT(strlen(pszPath) + strlen(pszResName) < sizeof(szFullPath) );

	std::strcpy(szFullPath, pszPath);
	strcat(szFullPath, pszResName);

	return szFullPath;
	}

////////////////////////////////////////////////////////////////////////////////
// Creates the layer map, if it has not already been done.
// Now that the layer map needs to be 32K of uncompressable data, we create it
// at run time.
// (static)
////////////////////////////////////////////////////////////////////////////////
void CRealm::CreateLayerMap(void)
	{
	// If table needs to be built . . .
	if (ms_asAttribToLayer[0] != LayerSprite16)
      {
      for (size_t l = 0; l < NUM_ELEMENTS(ms_asAttribToLayer); l++)
			{
			if (l & 0x0001)
				ms_asAttribToLayer[l]	= LayerSprite1;
			else if (l & 0x0002)
				ms_asAttribToLayer[l]	= LayerSprite2;
			else if (l & 0x0004)
				ms_asAttribToLayer[l]	= LayerSprite3;
			else if (l & 0x0008)
				ms_asAttribToLayer[l]	= LayerSprite4;
			else if (l & 0x0010)
				ms_asAttribToLayer[l]	= LayerSprite5;
			else if (l & 0x0020)
				ms_asAttribToLayer[l]	= LayerSprite6;
			else if (l & 0x0040)
				ms_asAttribToLayer[l]	= LayerSprite7;
			else if (l & 0x0080)
				ms_asAttribToLayer[l]	= LayerSprite8;
			else if (l & 0x0100)
				ms_asAttribToLayer[l]	= LayerSprite9;
			else if (l & 0x0200)
				ms_asAttribToLayer[l]	= LayerSprite10;
			else if (l & 0x0400)
				ms_asAttribToLayer[l]	= LayerSprite11;
			else if (l & 0x0800)
				ms_asAttribToLayer[l]	= LayerSprite12;
			else if (l & 0x1000)
				ms_asAttribToLayer[l]	= LayerSprite13;
			else if (l & 0x2000)
				ms_asAttribToLayer[l]	= LayerSprite14;
			else if (l & 0x4000)
				ms_asAttribToLayer[l]	= LayerSprite15;
			else
				ms_asAttribToLayer[l]	= LayerSprite16;
			}
		}
	}


CThing* CRealm::makeType(ClassIDType type_id)
{
  switch(type_id)
  {
    case CHoodID             : return new (type_id, "CHood"             , this, false) CHood             ;
    case CDudeID             : return new (type_id, "CDude"             , this, false) CDude             ;
    case CDoofusID           : return new (type_id, "CDoofus"           , this, false) CDoofus           ;
    case CRocketID           : return new (type_id, "CRocket"           , this, false) CRocket           ;
    case CGrenadeID          : return new (type_id, "CGrenade"          , this, false) CGrenade          ;
    case CBallID             : return new (type_id, "CBall"             , this, false) CBall             ;
    case CExplodeID          : return new (type_id, "CExplode"          , this, false) CExplode          ;
    case CBouyID             : return new (type_id, "CBouy"             , this,  true) CBouy             ;
    case CNavigationNetID    : return new (type_id, "CNavigationNet"    , this,  true) CNavigationNet    ;
    case CGameEditThingID    : return new (type_id, "CGameEditThing"    , this, false) CGameEditThing    ;
    case CNapalmID           : return new (type_id, "CNapalm"           , this, false) CNapalm           ;
    case CFireID             : return new (type_id, "CFire"             , this, false) CFire             ;
    case CFirebombID         : return new (type_id, "CFirebomb"         , this, false) CFirebomb         ;
    case CFirefragID         : return new (type_id, "CFirefrag"         , this, false) CFirefrag         ;
    case CAnimThingID        : return new (type_id, "CAnimThing"        , this,  true) CAnimThing        ;
    case CSoundThingID       : return new (type_id, "CSoundThing"       , this,  true) CSoundThing       ;
    case CBandID             : return new (type_id, "CBand"             , this,  true) CBand             ;
    case CItem3dID           : return new (type_id, "CItem3d"           , this,  true) CItem3d           ;
    case CBarrelID           : return new (type_id, "CBarrel"           , this,  true) CBarrel           ;
    case CProximityMineID    : return new (type_id, "CProximityMine"    , this,  true) CProximityMine    ;
    case CDispenserID        : return new (type_id, "CDispenser"        , this,  true) CDispenser        ;
    case CFireballID         : return new (type_id, "CFireball"         , this, false) CFireball         ;
    case CPersonID           : return new (type_id, "CPerson"           , this,  true) CPerson           ;
    case CTimedMineID        : return new (type_id, "CTimedMine"        , this,  true) CTimedMine        ;
    case CBouncingBettyMineID: return new (type_id, "CBouncingBettyMine", this,  true) CBouncingBettyMine;
    case CRemoteControlMineID: return new (type_id, "CRemoteControlMine", this, false) CRemoteControlMine;
    case CPylonID            : return new (type_id, "CPylon"            , this,  true) CPylon            ;
    case CPowerUpID          : return new (type_id, "CPowerUp"          , this,  true) CPowerUp          ;
    case COstrichID          : return new (type_id, "COstrich"          , this,  true) COstrich          ;
    case CTriggerID          : return new (type_id, "CTrigger"          , this, false) CTrigger          ;
    case CHeatseekerID       : return new (type_id, "CHeatseeker"       , this, false) CHeatseeker       ;
    case CChunkID            :
      if(!g_GameSettings.m_sParticleEffects) // Don't allow chunks when disabled . . .
        return nullptr;
                               return new (type_id, "CChunk"            , this, false) CChunk            ;
    case CSentryID           : return new (type_id, "CSentry"           , this,  true) CSentry           ;
    case CWarpID             : return new (type_id, "CWarp"             , this,  true) CWarp             ;
    case CDemonID            : return new (type_id, "CDemon"            , this,  true) CDemon            ;
    case CCharacterID        : return new (type_id, "CCharacter"        , this, false) CCharacter        ;
    case CGoalTimerID        : return new (type_id, "CGoalTimer"        , this, false) CGoalTimer        ;
    case CFlagID             : return new (type_id, "CFlag"             , this,  true) CFlag             ;
    case CFlagbaseID         : return new (type_id, "CFlagbase"         , this,  true) CFlagbase         ;
    case CFirestreamID       : return new (type_id, "CFirestream"       , this, false) CFirestream       ;
    case CDeathWadID         : return new (type_id, "CDeathWad"         , this, false) CDeathWad         ;
    case CDynamiteID         : return new (type_id, "CDynamite"         , this, false) CDynamite         ;
    case CSndRelayID         : return new (type_id, "CSndRelay"         , this,  true) CSndRelay         ;
    case TotalIDs            : return nullptr; // alias for no weapon
    default:
      ASSERT(false);
      return nullptr;
  }
}
