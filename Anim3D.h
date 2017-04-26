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
// Anim3D.h
// Project: Postal
// 
// History:
//		05/23/97 JMI	Started.  Moved here from thing.h.
//							Also, added events.
//
//		08/07/97	JMI	Added m_ptransWeapon rigid body transforms channel for
//							weapons.
//							Also, added SetLooping().
//
//		08/12/97 BRH	Added yet another overloaded Get function which 
//							in addition to the base name and verb, takes a number
//							of a texture scheme to load.
//
//////////////////////////////////////////////////////////////////////////////
//
// 3D animation class that is a collection of channels that make up a 3D
// animation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef ANIM3D_H
#define ANIM3D_H

#include <3dtypes.h>
#include <animatedresource.h>
#include <resourcemanager.h>

// These are the basic components of any 3D animation.
typedef std::shared_ptr<AnimatedResource<RSop>> ChanForm;            // Channel of SOPs.
typedef std::shared_ptr<AnimatedResource<RMesh>> ChanMesh;           // Channel of meshes.
typedef std::shared_ptr<AnimatedResource<RTexture>> ChanTexture;     // Channel of textures.
typedef std::shared_ptr<AnimatedResource<Raw<Vector3D>>> ChanHot;        // Channel of hotspots.
typedef std::shared_ptr<AnimatedResource<Raw<Vector3D>>> ChanBounds;     // Channel of bounding cylinders for collision.
typedef std::shared_ptr<AnimatedResource<Raw<real_t>>> ChanFloor;    // Channel of floor circles for attribute map..?
typedef std::shared_ptr<AnimatedResource<RTransform>> ChanTransform; // Channel of transforms.
typedef std::shared_ptr<AnimatedResource<Raw<uint8_t>>> ChanEvent;   // Channel of event states.

/////////////////////////////////////////////////////////////////////////
// This class describes the components of any 3D animation.
// To add more stuff (e.g., links for rigid bodies and such), descend
// a class from this one and add your members.  Note that you will either
// have to load these explicitly (myAnim.m_linkRocket.Load(..) or create
// a new Load() that calls the base class version and then loads your
// new members.
/////////////////////////////////////////////////////////////////////////
struct CAnim3D
{
  // Data corresponding to this animation. These are in synch such that a time is parallel with a time in any other.
  // For example, the m_sops.GetAtTime(1) SOP is used with the m_meshes.GetAtTime(1) mesh.
  ChanForm      m_psops;          // Sea of Pointses.
  ChanMesh      m_pmeshes;        // Meshes.
  ChanTexture   m_ptextures;      // Textures.
  ChanBounds    m_pbounds;        // Description of bounding cylinders.
  ChanTransform m_ptransRigid;    // Rigid body transforms.
  ChanEvent     m_pevent;         // Event states.
  ChanTransform m_ptransWeapon;   // Rigid body transforms for weapon position.


  // Get the various components of this animation from the resource names specified in the provided array of pointers to strings.
  virtual int16_t Get(const char** ppszFileNames);      // Pointer to array of pointers to filenames. These filenames should be in the order the members are listed in this class's definition.

  // Get the various components of this animation from the resource names specified in the provided array of pointers to strings.
  virtual int16_t Get(const char** ppszFileNames,       // Pointer to array of pointers to filenames. These filenames should be in the order the members are listed in this class's definition.
                      int16_t sLoopFlags);              // Looping flags to apply to all channels in this anim

  // Get the various components of this animation from the resource names specified by base name, optionally, with a rigid name.
  virtual int16_t Get(const char* pszBaseFileName,      // In:  Base string for resource filenames.
                      const char* pszRigidName,         // In:  String to add for rigid transform channel, "", or nullptr for none.
                      const char* pszEventName,         // In:  String to add for event states channel, "", or nullptr for none.
                      const char* pszWeaponTransName,   // In:  String to add for weapon transforms channel, "", or nullptr for none.
                      int16_t     sLoopFlags);          // In:  Looping flags to apply to all channels in this anim.

  // Get the various components of this animation from the resource names specified by base name, optionally, with a rigid name.
  virtual int16_t Get(const char* pszBaseName,          // In:  Base string for resource filenames.
                      const char* pszVerb,              // In:  Action name to be appended to the base
                      const char* pszRigidName,         // In:  String to add for rigid transform channel, "", or nullptr for none.
                      const char* pszEventName,         // In:  String to add for event states channel, "", or nullptr for none.
                      const char* pszWeaponTransName,   // In:  String to add for weapon transforms channel, "", or nullptr for none.
                      int16_t     sLoopFlags);          // In:  Looping flags to apply to all channels in this anim.

  // Get the various components of this animation from the resource names specified by base name, optionally, with a rigid name.
  virtual int16_t Get(const char* pszBaseName,          // In:  Base string for resource filenames.
                      int16_t     sTextureScheme,       // In:  Number of texture file to be loaded
                      const char* pszVerb,              // In:  Action name to be appended to the base
                      const char* pszRigidName,         // In:  String to add for rigid transform channel, "", or nullptr for none.
                      const char* pszEventName,         // In:  String to add for event states channel, "", or nullptr for none.
                      const char* pszWeaponTransName,   // In:  String to add for weapon transforms channel, "", or nullptr for none.
                      int16_t     sLoopFlags);          // In:  Looping flags to apply to all channels in this anim.

  // Release all resources.
  virtual void Release(void);

  // Set looping flags for this channel.
  virtual void SetLooping(int16_t sLoopFlags);          // In:  Looping flags to apply to all channels in this anim.

};

#endif // ANIM3D_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
