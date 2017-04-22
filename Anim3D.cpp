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
// Anim3D.CPP
// Project: Postal
// 
// History:
//		05/23/97 JMI	Started.  Moved here from thing.h.
//							Also, added events.
//
//		08/01/97 BRH	Took out loading/releasing .hot channels since
//							we aren't using them.
//
//		08/07/97	JMI	Added m_ptransWeapon rigid body transforms channel for
//							weapons.
//							Also, added SetLooping().
//							Also, removed all (including commented out) references to
//							hots and floors.
//
//		08/11/97	JMI	Now has TRACEs to complain when an animation fails to 
//							load.
//							Also, on the first component to fail, no more are loaded.
//							Also, if any fail to load, all that were loaded are
//							released.
//
//		08/12/97 BRH	Added yet another overloaded version of Get which
//							takes a number of a texture file to be loaded.
//
//		12/17/97	JMI	Now if the sTextureScheme values is less than zero, no 
//							number is used (i.e., the default texture scheme is used).
//
//		10/07/99	JMI	Added conditional compile options for some error messages.
//
//////////////////////////////////////////////////////////////////////////////
//
// 3D animation class that is a collection of channels that make up a 3D
// animation.
//
//////////////////////////////////////////////////////////////////////////////

#include <RSPiX.h>

#include "Anim3D.h"
#include "game.h"

////////////////////////////////////////////////////////////////////////////////
// Macros/types/etc.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Variables/data
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Functions.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Default constructor.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Get the various components of this animation from the resource names
// specified in the provided array of pointers to strings.
// (virtual)
////////////////////////////////////////////////////////////////////////////////
int16_t CAnim3D::Get(				// Returns 0 on success.
   const char**	ppszFileNames)		// Pointer to array of pointers to filenames.
										// These filenames should be in the order
										// the members are listed in this class's
										// definition.
	{
	int16_t sResult;
	int16_t	sComplainIndex;	// If sResult is non-zero, this is the index of the
									// resname that could not load.

	// NOTE:  If you add any new channel loads here, please make sure
	// you maintain the order described above.
	sComplainIndex	= 0;
   sResult	=  rspGetResource(g_GameSAK, ppszFileNames[sComplainIndex], m_psops) ? SUCCESS : FAILURE;
   if (sResult == SUCCESS)
		{
		sComplainIndex	= 1;
      sResult	= rspGetResource(g_GameSAK, ppszFileNames[sComplainIndex], m_pmeshes) ? SUCCESS : FAILURE;
      if (sResult == SUCCESS)
			{
			sComplainIndex	= 2;
         sResult	= rspGetResource(g_GameSAK, ppszFileNames[sComplainIndex], m_ptextures) ? SUCCESS : FAILURE;
         if (sResult == SUCCESS)
				{
				sComplainIndex	= 4;
            sResult	= rspGetResource(g_GameSAK, ppszFileNames[sComplainIndex], m_pbounds) ? SUCCESS : FAILURE;
            if (sResult == SUCCESS)
					{
					sComplainIndex	= 6;
					if (ppszFileNames[sComplainIndex] != nullptr)
						{
                  sResult	= rspGetResource(g_GameSAK, ppszFileNames[sComplainIndex], m_ptransRigid) ? SUCCESS : FAILURE;
						}
					}
				}
			}
		}

	// No current support for events since that might break existing
	// arrays of size 6 elements.

	// No current support for weapon transforms either.

	// If an error occurred . . .
   if (sResult != SUCCESS)
		{
#if defined(VERBOSE)
		// Complain.
		TRACE("Get(): Unable to load \"%s\".\n", ppszFileNames[sComplainIndex] );
#endif
		// Clean.
		Release();
		}

	return sResult;
	}

////////////////////////////////////////////////////////////////////////////////
// Get the various components of this animation from the resource names
// specified in the provided array of pointers to strings.
// (virtual)
////////////////////////////////////////////////////////////////////////////////
int16_t CAnim3D::Get(				// Returns 0 on success.
   const char**	ppszFileNames,		// Pointer to array of pointers to filenames.
										// These filenames should be in the order
										// the members are listed in this class's
										// definition.
	int16_t		sLoopFlags)			// Looping flags to apply to all channels in this anim
	{
	int16_t sResult	= Get(ppszFileNames);
	// If successful . . .
   if (sResult == SUCCESS)
		{
		SetLooping(sLoopFlags);
		}

	return sResult;
	}

////////////////////////////////////////////////////////////////////////////////
// Get the various components of this animation from the resource names
// specified by base name, optionally, with a rigid name.
// (virtual)
////////////////////////////////////////////////////////////////////////////////
int16_t CAnim3D::Get(					// Returns 0 on success.
   const char*		pszBaseFileName,		// In:  Base string for resource filenames.
   const char*		pszRigidName,			// In:  String to add for rigid transform channel,
											// "", or nullptr for none.
   const char*		pszEventName,			// In:  String to add for event states channel,
											// "", or nullptr for none.
   const char*		pszWeaponTransName,	// In:  String to add for weapon transforms channel,
											// "", or nullptr for none.
	int16_t		sLoopFlags)				// In:  Looping flags to apply to all channels
											// in this anim.
	{
	int16_t sResult;
	char	szResName[PATH_MAX];
	sprintf(szResName, "%s.sop", pszBaseFileName);
   sResult	=  rspGetResource(g_GameSAK, szResName, m_psops) ? SUCCESS : FAILURE;
   if (sResult == SUCCESS)
		{
		sprintf(szResName, "%s.mesh", pszBaseFileName);
      sResult	= rspGetResource(g_GameSAK, szResName, m_pmeshes) ? SUCCESS : FAILURE;
      if (sResult == SUCCESS)
			{
			sprintf(szResName, "%s.tex", pszBaseFileName);
         sResult	= rspGetResource(g_GameSAK, szResName, m_ptextures) ? SUCCESS : FAILURE;
         if (sResult == SUCCESS)
				{
				sprintf(szResName, "%s.bounds", pszBaseFileName);
            sResult	= rspGetResource(g_GameSAK, szResName, m_pbounds) ? SUCCESS : FAILURE;
            if (sResult == SUCCESS)
					{
					if (pszRigidName != nullptr)
						{
						if (*pszRigidName != '\0')
							{
							sprintf(szResName, "%s_%s.trans", pszBaseFileName, pszRigidName);
                     sResult	= rspGetResource(g_GameSAK, szResName, m_ptransRigid) ? SUCCESS : FAILURE;
							}
						}

               if (sResult == SUCCESS)
						{
						if (pszEventName != nullptr)
							{
							if (*pszEventName != '\0')
								{
								sprintf(szResName, "%s_%s.event", pszBaseFileName, pszEventName);
                        sResult	= rspGetResource(g_GameSAK, szResName, m_pevent) ? SUCCESS : FAILURE;
								}
							}

                  if (sResult == SUCCESS)
							{
							if (pszWeaponTransName != nullptr)
								{
								if (*pszWeaponTransName != '\0')
									{
									sprintf(szResName, "%s_%s.trans", pszBaseFileName, pszWeaponTransName);
                           sResult	= rspGetResource(g_GameSAK, szResName, m_ptransWeapon) ? SUCCESS : FAILURE;
									}
								}
							}
						}
					}
				}
			}
		}

	// If successful . . .
   if (sResult == SUCCESS)
      {
		SetLooping(sLoopFlags);
		}
	else
		{
#if defined(VERBOSE)
		TRACE("Get(): Unable to load \"%s\".\n", szResName);
#endif
		// Clean.
		Release();
		}

	return sResult;
	}

////////////////////////////////////////////////////////////////////////////////
// Get the various components of this animation from the resource names
// specified by base name, optionally, with a rigid name.
// (virtual)
////////////////////////////////////////////////////////////////////////////////
int16_t CAnim3D::Get(					// Returns 0 on success.
   const char*		pszBaseFileName,		// In:  Base string for resource filenames.
   const char*		pszVerb,					// In:  Action name to be appended to the base
   const char*		pszRigidName,			// In:  String to add for rigid transform channel,
											// "", or nullptr for none.
   const char*		pszEventName,			// In:  String to add for event states channel,
											// "", or nullptr for none.
   const char*		pszWeaponTransName,	// In:  String to add for weapon transforms channel,
											// "", or nullptr for none.
	int16_t		sLoopFlags)				// In:  Looping flags to apply to all channels
											// in this anim.
	{
	char	szVerbedBaseName[PATH_MAX];
	sprintf(szVerbedBaseName, "%s_%s", pszBaseFileName, pszVerb);
	
	return Get(szVerbedBaseName, pszRigidName, pszEventName, pszWeaponTransName, sLoopFlags);
	}

////////////////////////////////////////////////////////////////////////////////
// Get the various components of the animation using the given resource names,
// but load only 1 .tex file based on the color scheme number passed in.
////////////////////////////////////////////////////////////////////////////////
int16_t CAnim3D::Get(					// Returns 0 on success.
   const char*		pszBaseFileName,		// In:  Base string for resource filenames.
	int16_t		sTextureScheme,		// In:  Number to append after name for texture file
   const char*		pszVerb,					// In:  Action name to be appended to the base
   const char*		pszRigidName,			// In:  String to add for rigid transform channel,
											// "", or nullptr for none.
   const char*		pszEventName,			// In:  String to add for event states channel,
											// "", or nullptr for none.
   const char*		pszWeaponTransName,	// In:  String to add for weapon transforms channel,
											// "", or nullptr for none.
	int16_t		sLoopFlags)				// In:  Looping flags to apply to all channels
											// in this anim.
{
	char	szVerbedBaseName[PATH_MAX];
	sprintf(szVerbedBaseName, "%s_%s", pszBaseFileName, pszVerb);

	int16_t sResult;
	char	szResName[PATH_MAX];
	sprintf(szResName, "%s.sop", szVerbedBaseName);
   sResult	=  rspGetResource(g_GameSAK, szResName, m_psops) ? SUCCESS : FAILURE;
   if (sResult == SUCCESS)
		{
		sprintf(szResName, "%s.mesh", szVerbedBaseName);
      sResult	= rspGetResource(g_GameSAK, szResName, m_pmeshes) ? SUCCESS : FAILURE;
      if (sResult == SUCCESS)
			{
			// If there's an associated texture scheme . . .
			if (sTextureScheme >= 0)
				{
				sprintf(szResName, "%s%d.tex", pszBaseFileName, sTextureScheme);
				}
			else
				{
				sprintf(szResName, "%s.tex", szVerbedBaseName);
				}

         sResult	= rspGetResource(g_GameSAK, szResName, m_ptextures) ? SUCCESS : FAILURE;
         if (sResult == SUCCESS)
				{
				sprintf(szResName, "%s.bounds", szVerbedBaseName);
            sResult	= rspGetResource(g_GameSAK, szResName, m_pbounds) ? SUCCESS : FAILURE;
            if (sResult == SUCCESS)
					{
					if (pszRigidName != nullptr)
						{
						if (*pszRigidName != '\0')
							{
							sprintf(szResName, "%s_%s.trans", szVerbedBaseName, pszRigidName);
                     sResult	= rspGetResource(g_GameSAK, szResName, m_ptransRigid) ? SUCCESS : FAILURE;
							}
						}

               if (sResult == SUCCESS)
						{
						if (pszEventName != nullptr)
							{
							if (*pszEventName != '\0')
								{
								sprintf(szResName, "%s_%s.event", szVerbedBaseName, pszEventName);
                        sResult	= rspGetResource(g_GameSAK, szResName, m_pevent) ? SUCCESS : FAILURE;
								}
							}

                  if (sResult == SUCCESS)
							{
							if (pszWeaponTransName != nullptr)
								{
								if (*pszWeaponTransName != '\0')
									{
									sprintf(szResName, "%s_%s.trans", szVerbedBaseName, pszWeaponTransName);
                           sResult	= rspGetResource(g_GameSAK, szResName, m_ptransWeapon) ? SUCCESS : FAILURE;
									}
								}
							}
						}
					}
				}
			}
		}

	// If successful . . .
   if (sResult == SUCCESS)
		{
		SetLooping(sLoopFlags);
		}
	else
		{
#if defined(VERBOSE)
		TRACE("Get(): Unable to load \"%s\".\n", szResName);
#endif
		// Clean.
		Release();
		}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
// Release all resources.
// (virtual)
////////////////////////////////////////////////////////////////////////////////
void CAnim3D::Release(void)	// Returns nothing.
	{
   if (m_psops)
      rspReleaseResource(m_psops);

   if (m_pmeshes)
      rspReleaseResource(m_pmeshes);

   if (m_ptextures)
      rspReleaseResource(m_ptextures);

   if (m_pbounds)
      rspReleaseResource(m_pbounds);

   if (m_ptransRigid)
      rspReleaseResource(m_ptransRigid);

   if (m_pevent)
      rspReleaseResource(m_pevent);

	if (m_ptransWeapon)
      rspReleaseResource(m_ptransWeapon);
	}

////////////////////////////////////////////////////////////////////////////////
// Set looping flags for this channel.
// (virtual)
////////////////////////////////////////////////////////////////////////////////
void CAnim3D::SetLooping(		// Returns nothing.
	int16_t		sLoopFlags)			// In:  Looping flags to apply to all channels
										// in this anim.
{
  m_psops->loopFlags = AnimatedResource<RSop>::loop_t(sLoopFlags);
  m_pmeshes->loopFlags = AnimatedResource<RMesh>::loop_t(sLoopFlags);
  m_ptextures->loopFlags = AnimatedResource<RTexture>::loop_t(sLoopFlags);
  m_pbounds->loopFlags = AnimatedResource<Raw<RP3d>>::loop_t(sLoopFlags);

  if (m_ptransRigid)
    m_ptransRigid->loopFlags = AnimatedResource<RTransform>::loop_t(sLoopFlags);

  if (m_pevent)
    m_pevent->loopFlags = AnimatedResource<Raw<uint8_t>>::loop_t(sLoopFlags);

  if (m_ptransWeapon)
    m_ptransWeapon->loopFlags = AnimatedResource<RTransform>::loop_t(sLoopFlags);
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
