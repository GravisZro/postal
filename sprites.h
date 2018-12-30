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
// sprite.h
// Project: Nostril (aka Postal)
//
//	History:
//		06/27/97	JMI	Copied CSprite and derived sprite classes here from 
//							scene.h.
//
//		07/02/97	JMI	Now AddChild() resets the m_sX2,Y2 to zero when adding
//							a sprite that formerly had no parent.
//
//		08/04/97	JMI	Now initializes m_pthing to NULL.
//
//		08/18/97	JMI	Now ASSERTs that we're not in a scene list.
//
//		09/28/99	JMI	Changed the m_iter member of CSprite to a non-const iter
//							to work with VC++ 6.0.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef SPRITES_H
#define SPRITES_H


#include <set>

#include <RSPiX.h>


////////////////////////////////////////////////////////////////////////////////
// Typedefs.
////////////////////////////////////////////////////////////////////////////////

// NOTE: While I wanted this to be within the CScene namespace, putting it there
// created a circular dependancy between CScene and CSprite since they both
// needed to use this.  It's now at global scope, which I hate, but it works.
// Define a container of sprites.  A multiset allows for duplicates keys, which
// is necessary because our sorting function might think of two sprites as being
// the "same" as far as sorting goes.
class CSprite;	// Forward declaration

// This is the function object used to sort the set of sprites.
// It is a template NOT because we want to use it with other types
// (after all, how many types will have a m_sZ member?!) but because
// making it a template causes it to be "evaluated" by the compiler
// AFTER the CSprite class is fully defined!  In other words, it's a
// trick!
template <class T>
  struct SpriteLess : std::binary_function<T*, T*, bool>
		{
		bool operator()(const T* a, const T* b) const
			{
			return a->m_sPriority < b->m_sPriority;
			}
		};

  typedef std::multiset<CSprite*, SpriteLess<CSprite>> msetSprites;


// A CSprite is a base class for sprites designed to work with CScene.
//class CScene;	// Forward declaration
class CSprite
	{
	// Make CScene a friend so it can access private stuff
	friend class CScene;

	public:

		// Types of sprites (or primitives).
      typedef enum
			{
			Standard2d,
			Standard3d,
			Line2d,
			Cylinder3d
			} Type;

	public:
		int16_t m_sX2;												// Sprite's 2d x coord
		int16_t m_sY2;												// Sprite's 2d y coord
		int16_t m_sPriority;										// Sprite's priority
      int16_t m_sLayer;											// Sprite's layer


      struct flags_t
      {
        uint32_t Alpha          : 1; // Set if on alpha layer, clear otherwise
        uint32_t Opaque         : 1; // Set if on opaque layer, clear otherwise
        uint32_t Xrayee         : 1; // Set if xray target, clear otherwise
        uint32_t Hidden         : 1; // Set if hidden, clear otherwise
        uint32_t DeleteOnClear  : 1; // Set to delete sprite when layer is cleared
        uint32_t HighIntensity  : 1; // Set to use higher light intensities when
        uint32_t DeleteOnRender : 1; // After rendering object, delete it.
        uint32_t BlitOpaque     : 1; // Blit sprite opaque (currently only supported for 2D uncompressed, non-alpha objects).

        inline void clear(void) { *reinterpret_cast<uint32_t*>(this) = 0; }
      } flags;

      const char*	m_pszText;											// Point this at your text.
																		// DO NOT strcpy/cat/etc to this until
																		// you've pointed it at some memory!!!

		CSprite*	m_psprHeadChild;	// First child sprite.
		CSprite*	m_psprNext;			// Next sibling sprite.
		CSprite*	m_psprParent;		// Parent sprite.

	protected:
      Type m_type;												// Sprite's type
      bool m_InScene;
		int16_t m_sSavedLayer;										// Sprite's saved layer (used to detect changes)
		int16_t m_sSavedPriority;									// Sprite's saved priority (used to detect changes)
		msetSprites::iterator m_iter;							// Sprite's iterator into layer's container

	public:
		CSprite()
			{
        flags.clear();
        m_InScene = false;

			m_sX2		= 0;		// Any sprite's 2D dest x coord.
			m_sY2		= 0;		// Any sprite's 2D dest y coord.

			m_pszText			= nullptr;

			m_psprHeadChild	= nullptr;
			m_psprNext			= nullptr;
         m_psprParent		= nullptr;
			}

      virtual ~CSprite(void)
			{
			// If we have a parent . . .
			if (m_psprParent != nullptr)
				{
				// Get outta there!
				m_psprParent->RemoveChild(this);
				}

			// While we have children . . .
			while (m_psprHeadChild != nullptr)
				{
				// Remove the head child.
				RemoveChild(m_psprHeadChild);
				}

			// Make sure we're not in any lists.
         ASSERT(!m_InScene);
			}

		// Adds a child CSprite.
		void AddChild(					// Returns nothing.  Cannot fail.
			CSprite*	psprNewChild)	// In:  New child.
			{
			// If we already had a parent . . .
			if (psprNewChild->m_psprParent != nullptr)
				{
				psprNewChild->m_psprParent->RemoveChild(psprNewChild);
				}
			else
				{
				// Clear the positions which will now be parent
				// relative.
				// Note that if you already set a parent relative
				// position, this will override it.
				psprNewChild->m_sX2	= 0;	// Convenient placed right
				psprNewChild->m_sY2	= 0;	// up parent's *ss.
				}

			// Make new sprite the head for ease of insertion.
			// Point new child's next at current head.
			psprNewChild->m_psprNext	= m_psprHeadChild;
			// Make head the new child.
			m_psprHeadChild	= psprNewChild;

			// Let the child know who its parent is.
			psprNewChild->m_psprParent	= this;
			}

		// Removes a child CSprite3.
		void RemoveChild(					// Returns nothing.  Fails if not in list.
			CSprite*		psprRemove)		// In:  Child to remove.
			{
			CSprite*		psprCur			= m_psprHeadChild;	// Current traversal sprite.
			CSprite**	ppsprPrevNext	= &m_psprHeadChild;	// Ptr to the previous sprite's
																			// next ptr.
			while (psprCur != nullptr)
				{
				// If we found the item to remove . . .
				if (psprCur == psprRemove)
					{
					// Point the previous' next at this item's next.
					*ppsprPrevNext	= psprCur->m_psprNext;
					// Clean this item.
					psprCur->m_psprParent	= nullptr;
					psprCur->m_psprNext		= nullptr;
					// Done.
					break;
					}

				// Store this item's next ptr.
				ppsprPrevNext	= &psprCur->m_psprNext;
				// Get the next one.
				psprCur			= psprCur->m_psprNext;
				}

			if (psprCur == nullptr)
				{
				TRACE("CSprite3::RemoveChild(): No such child sprite.\n");
				}
			}

		// Get the type of sprite.
		// Read access to our protected member.
		// This will be inlined and requires no local stack.
		// It is important to make sure the types are as allocated.
		Type GetType(void)
			{
			return m_type;
			}

	};

// A CSprite2 is a 2d sprite designed to work with CScene.
// It has two optional ways of alpha'ing during blit:
// 1) m_pimAlpha, an alpha mask.
// 2) m_sAlphaLevel, an alpha value to apply to the whole image.
class CSprite2 : virtual public CSprite
	{
	public:
      RImage* m_pImage;									// Pointer to image
      RImage* m_pimAlpha;									// Alpha image pointer.
      int16_t m_sAlphaLevel;								// Constant alpha level to
																	// use if no m_pimAlpha.

   CSprite2(void) noexcept
		{
		m_pImage			= nullptr;
		m_pimAlpha		= nullptr;
      m_sAlphaLevel	= UINT8_MAX;
		m_type			= Standard2d;
		}

   virtual ~CSprite2(void) noexcept { }
																	
	};

// A CSpriteLine2d is a 2D line designed to work with CScene.
class CSpriteLine2d : virtual public CSprite
	{
	public:
		int16_t		m_sX2End;	// 2D end point for line.
		int16_t		m_sY2End;	// 2D end point for line.
		uint8_t			m_u8Color;	// Color for line segment.

   CSpriteLine2d(void) noexcept
		{
		m_type			= Line2d;
		}

   virtual ~CSpriteLine2d(void) noexcept { }
	};

// A CSpriteCylinder3d is a cylinder designed to work with CScene.
class CSpriteCylinder3d : virtual public CSprite
	{
	public:
		int16_t		m_sRadius;	// Radius of cylinder
		int16_t		m_sHeight;	// Height of cylinder.
		uint8_t			m_u8Color;	// Color for line segment.

   CSpriteCylinder3d(void) noexcept
		{
		m_type			= Cylinder3d;
		}

    virtual ~CSpriteCylinder3d(void) noexcept { }
	};

// A CSprite3 is a 3d sprite designed to work with CScene.
class CSprite3 : virtual public CSprite
	{
	public:
      CSprite3(void) noexcept
			{
			Init();
			}
      virtual ~CSprite3(void) noexcept { }

	public:
		void Init(void)
			{
			m_pmesh			= nullptr;	// Mesh.                  
			m_psop			= nullptr;	// Sea Of Points.         
			m_ptrans			= nullptr;	// Transform.             
			m_ptex			= nullptr;	// Texture.
			m_psphere		= nullptr;	// Bounding sphere.

			m_sRadius		= 0;		// Radius of collision circle.
			m_sCenX			= 0;		// Location of collision circle.
			m_sCenY			= 0;		// Location of collision circle.
			m_sBrightness	= 0;		// Brightness (start in the middle).

			m_sDirectRenderX		= 0;	// Location to Render() directly to composite buffer.
			m_sDirectRenderY		= 0;	// Location to Render() directly to composite buffer.
			m_sDirectRenderZ		= 0;	// The same z in either case.
			m_sIndirectRenderX	= 0;	// Location to Render() indirectly to clip buffer.
			m_sIndirectRenderY	= 0;	// Location to Render() indirectly to clip buffer.
			m_sRenderOffX			= 0;	// Offset to Render() to account for bounding
												// sphere.                                   
			m_sRenderOffY			= 0;	// Offset to Render() to account for bounding
												// sphere.                                   
										
			m_type			= Standard3d;
			}

	public:
		RMesh*		m_pmesh;		// Mesh.
		RSop*			m_psop;		// Sea Of Points.
		RTransform*	m_ptrans;	// Transform.
		RTexture*	m_ptex;		// Texture.
		Vector3D*			m_psphere;	// Bounding sphere def.

		int16_t			m_sRadius;	// Radius for collision (with buildings for 
										// X Ray).  Autoset by Render3D().
		int16_t			m_sCenX;		// Position of center of collision circle.
										// Autoset by Render3D().
		int16_t			m_sCenY;		// Position of center of collision circle.
										// Autoset by Render3D().

		int16_t			m_sBrightness;	// Indicates the brightness with which the
											// object will be fogged (0 .. 255).

		int16_t			m_sDirectRenderX;		// Location to Render() directly to composite buffer.
		int16_t			m_sDirectRenderY;		// Location to Render() directly to composite buffer.
		int16_t			m_sDirectRenderZ;		// Location to render in either case
		int16_t			m_sIndirectRenderX;	// Location to Render() indirectly to clip buffer.
		int16_t			m_sIndirectRenderY;	// Location to Render() indirectly to clip buffer.
		int16_t			m_sRenderOffX;	// Offset to Render() to account for bounding
											// sphere.
		int16_t			m_sRenderOffY;	// Offset to Render() to account for bounding
											// sphere.
		bool			m_bIndirect;	// Indicates an indirect Render() (into the
											// clip buffer).
	};


#endif	// SPRITES_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
