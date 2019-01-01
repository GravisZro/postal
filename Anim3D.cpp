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

#include <string>

#include "Anim3D.h"
#include "game.h"

////////////////////////////////////////////////////////////////////////////////
// Get the various components of this animation from the resource names
// specified by base name, optionally, with a rigid name.
// (virtual)
////////////////////////////////////////////////////////////////////////////////

/*
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
  sprintf(szResName, "%s.mesh", szVerbedBaseName);
  sResult	= rspGetResource(g_GameSAK, szResName, m_pmeshes) ? SUCCESS : FAILURE;
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
  sprintf(szResName, "%s.bounds", szVerbedBaseName);
  sResult	= rspGetResource(g_GameSAK, szResName, m_pbounds) ? SUCCESS : FAILURE;

  if (pszRigidName != nullptr)
  {
    if (*pszRigidName != '\0')
    {
      sprintf(szResName, "%s_%s.trans", szVerbedBaseName, pszRigidName);
      sResult	= rspGetResource(g_GameSAK, szResName, m_ptransRigid) ? SUCCESS : FAILURE;
    }
  }

  if (pszEventName != nullptr)
  {
    if (*pszEventName != '\0')
    {
      sprintf(szResName, "%s_%s.event", szVerbedBaseName, pszEventName);
      sResult	= rspGetResource(g_GameSAK, szResName, m_pevent) ? SUCCESS : FAILURE;
    }
  }

  if (pszWeaponTransName != nullptr)
  {
    if (*pszWeaponTransName != '\0')
    {
      sprintf(szResName, "%s_%s.trans", szVerbedBaseName, pszWeaponTransName);
      sResult	= rspGetResource(g_GameSAK, szResName, m_ptransWeapon) ? SUCCESS : FAILURE;
    }
  }
}
*/

bool CAnim3D::Get(					// Returns 0 on success.
   const char* pszBaseFileName,		// In:  Base string for resource filenames.
   uint8_t     sTextureScheme,		// In:  Number to append after name for texture file or zero for none.
   const char*	pszVerb,					// In:  Action name to be appended to the base       or nullptr for none.
   const char* pszRigidName,			// In:  String to add for rigid transform channel    or nullptr for none.
   const char* pszWeaponTransName, 	// In:  String to add for weapon transforms channel  or nullptr for none.
   const char* pszEventName)			// In:  String to add for event states channel       or nullptr for none.
{
  std::string base = "3d/";
  base.append(pszBaseFileName);
  std::string vbase = base;
  if(pszVerb != nullptr)
    vbase.append("_").append(pszVerb);

  if(!rspGetResource(g_GameSAK, vbase + ".sop", m_psops))
    return false;

  if(!rspGetResource(g_GameSAK, vbase + ".mesh", m_pmeshes))
    return false;

  if(sTextureScheme)
  {
    if(!rspGetResource(g_GameSAK, base + std::to_string(sTextureScheme) + ".tex", m_ptextures))
      return false;
  }
  else if(!rspGetResource(g_GameSAK, vbase + ".tex", m_ptextures))
    return false;

  if(!rspGetResource(g_GameSAK, vbase + ".bounds", m_pbounds))
    return false;

  vbase.push_back('_');

  if(pszRigidName && !rspGetResource(g_GameSAK, vbase + pszRigidName + ".trans", m_ptransRigid))
    return false;

  if(pszWeaponTransName && !rspGetResource(g_GameSAK, vbase + pszWeaponTransName + ".trans", m_ptransWeapon))
    return false;

  if(pszEventName && !rspGetResource(g_GameSAK, vbase + pszEventName + ".event", m_pevent))
    return false;

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Release all resources.
// (virtual)
////////////////////////////////////////////////////////////////////////////////
void CAnim3D::Release(void)	// Returns nothing.
{
  rspReleaseResource(m_psops);
  rspReleaseResource(m_pmeshes);
  rspReleaseResource(m_ptextures);
  rspReleaseResource(m_pbounds);
  rspReleaseResource(m_ptransRigid);
  rspReleaseResource(m_pevent);
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
  m_pbounds->loopFlags = AnimatedResource<Raw<Vector3D>>::loop_t(sLoopFlags);

  if(m_ptextures)
    m_ptextures->loopFlags = AnimatedResource<RTexture>::loop_t(sLoopFlags);

  if(m_ptransRigid)
    m_ptransRigid->loopFlags = AnimatedResource<RTransform>::loop_t(sLoopFlags);

  if(m_pevent)
    m_pevent->loopFlags = AnimatedResource<Raw<uint8_t>>::loop_t(sLoopFlags);

  if(m_ptransWeapon)
    m_ptransWeapon->loopFlags = AnimatedResource<RTransform>::loop_t(sLoopFlags);
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
