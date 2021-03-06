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
////////////////////////////////////////////////////////////////////////////////
//
// types3d.h
// Project: RSPiX\Green\3d
//
// This file stores the "high level" data types (containers) needed by the
// renderer.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef TYPES3D_H
#define TYPES3D_H

#include <BLUE/System.h>
#include <ORANGE/File/file.h>
#include <ORANGE/color/colormatch.h>
#include <ORANGE/QuickMath/VectorMath.h>

////////////////////////////////////////////////////////////////////////////////
// This is currently a flat texture used with small 
// polygons, i.e., 1 color per triangle.
// It may be mapped or unmappable.
//
// Note that the engine only uses the index colors.
////////////////////////////////////////////////////////////////////////////////
class RTexture
	{
	//------------------------------------------------------------------------------
	// Types
	//------------------------------------------------------------------------------
	public:
		// These values are used as bit masks, so you can indicate it
		// has one thing, the other, neither, or both.
		enum
			{
			HasIndices = 1,
			HasColors = 2
			} HasFlags;

	//------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------
	public:
      palindex_t m_sNum;												// Number of colors in array(s)
      uint8_t*   m_pIndices;										// Array of indices
      pixel32_t*  m_pColors;										// Array of colors

	//------------------------------------------------------------------------------
	// Functions
	//------------------------------------------------------------------------------
	public:
		// Default constructor
		RTexture()
			{
			Init();
			}

		// Copy constructor
		RTexture(const RTexture& other)
			{
			Init();
			*this = other;
			}

		// Alternate constructor
		RTexture(int16_t sNum)
			{
			Init();
			Alloc(sNum);
			}

		// Destructor
		~RTexture()
			{
			Free();
			}
#ifdef UNUSED_OPERATORS
      // Assignment operator
		RTexture& operator=(const RTexture& rhs)
			{
			Free();

			m_sNum = rhs.m_sNum;

			if (rhs.m_pIndices != nullptr)
				{
				AllocIndices();
            memcpy(m_pIndices, rhs.m_pIndices, m_sNum);
				}

			if (rhs.m_pColors != nullptr)
				{
				AllocColors();
            memcpy(m_pColors, rhs.m_pColors, sizeof(pixel32_t) * m_sNum);
				}
			
			return *this;
			}
#endif
		// Overloaded == (equality) operator
		bool operator==(const RTexture& rhs) const
			{
			bool result = true;
			if (m_sNum == rhs.m_sNum)
				{
				if (m_sNum > 0)
					{
					// If both pointers are non-zero then we compare their data
					if (m_pIndices && rhs.m_pIndices)
                  result = std::memcmp(m_pIndices, rhs.m_pIndices, m_sNum) == SUCCESS;
					else
						{
						// If both pointers are not nullptr then they obviously don't match
						if ( !((m_pIndices == nullptr) && (rhs.m_pIndices == nullptr)) )
							result = false;
						}

					if (result == true)
						{
						// If both pointers are non-zero then we compare their data
						if (m_pColors && rhs.m_pColors)
                     result = std::memcmp(m_pColors, rhs.m_pColors, sizeof(pixel32_t) * m_sNum) == SUCCESS;
						else
							{
							// If both pointers are not nullptr then they obviously don't match
							if ( !((m_pColors == nullptr) && (rhs.m_pColors == nullptr)) )
								result = false;
							}
						}
					}
				}
			else
				result = false;
			return result;
			}

		// Allocate specified number of indices and colors
		void Alloc(int16_t sNum);

		// Allocate same number of indices as current number of colors
		void AllocIndices(void);

		// Allocate same number of colors as current number of indices
		void AllocColors(void);

		// Free indices and colors
		void Free(void);

		// Free indices only
		void FreeIndices(void);

		// Free colors only
		void FreeColors(void);

		// Load from file
		int16_t	Load(RFile* fp);

		// Save to file
		int16_t	Save(RFile* fp);

		// Map colors onto the specified palette.  For each color, the best
		// matching color is found in the  palette, and the associated palette
		// index is written to the array of indices.  If the array of indices
		// doesn't exist, it will be created.
		void Remap(
         palindex_t sStartIndex,
         palindex_t sNumIndex,
         channel_t* pr,
         channel_t* pg,
         channel_t* pb,
         uint32_t linc);

		// Unmap colors from the specified palette and put them into the colors
		// array.  If the array of colors doesn't exist, it will be created.
		void 
		Unmap(
         channel_t* pr,
         channel_t* pg,
         channel_t* pb,
         uint32_t lInc)
			;

		// Muddy or brighten or darken.  Applies the specified brightness value
		// to every nth color (where n == lInc).
		void
		Adjust(
			float fAdjustment,	// In:  Adjustment factor (1.0 == same, < 1 == dimmer, > 1 == brighter).
         uint32_t lInc)				// In:  Number of colors to skip.
			;

	private:
		// Init
		void Init(void)
			{
			m_sNum = 0;
			m_pIndices = nullptr;
			m_pColors = nullptr;
			}
	};


////////////////////////////////////////////////////////////////////////////////
// A mesh is basically an array of triangles, where each triangle consists of 3
// indices that refer to RP3d's stored in a separate CPointArray3d object.
////////////////////////////////////////////////////////////////////////////////
class RMesh
	{
	//------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------
	public:
		int16_t m_sNum;												// Number of triangles in array (3 elements per triangle!)
		uint16_t* m_pArray;												// Array of indices (3 per triangle!)

	//------------------------------------------------------------------------------
	// Functions
	//------------------------------------------------------------------------------
	public:
		// Default constructor
		RMesh(void)
			{
			Init();
			}

		// Copy constructor
		RMesh(const RMesh& other)
			{
			Init();
			*this = other;
			}

		// Alternate constructor
		RMesh(int16_t sNum)
			{
			Init();
			Alloc(sNum);
			}

		// Destructor
		~RMesh()
			{
			Free();
			}
#ifdef UNUSED_OPERATORS
		// Assignment operator
		RMesh& operator=(const RMesh& rhs)
			{
			Free();

			m_sNum = rhs.m_sNum;

			if (rhs.m_pArray != nullptr)
				{
				Alloc(rhs.m_sNum);
            memcpy(m_pArray, rhs.m_pArray, sizeof(uint16_t) * m_sNum * 3);
				}

			return *this;
			}
#endif
		// Overloaded == (equality) operator
		bool operator==(const RMesh& rhs) const
			{
			bool result = true;
			if (m_sNum == rhs.m_sNum)
				{
				if (m_sNum > 0)
					{
					// If both pointers are non-zero then we compare their data
					if (m_pArray && rhs.m_pArray)
                  result = std::memcmp(m_pArray, rhs.m_pArray, sizeof(uint16_t) * m_sNum * 3) == SUCCESS;
					else
						{
						// If both pointers are not nullptr then they obviously don't match
						if ( !((m_pArray == nullptr) && (rhs.m_pArray == nullptr)) )
							result = false;
						}
					}
				}
			else
				result = false;
			return result;
         }

		// Allocate specified number of triangles
		void Alloc(int16_t sNum);

		// Free triangles
		void Free(void);

		// Load from file
		int16_t	Load(RFile* fp);
		
		// Save to file
		int16_t	Save(RFile* fp);

	protected:
		// Init
		void Init(void)
			{
			m_sNum = 0;
			m_pArray = nullptr;
			}
	};


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class RSop
	{
	//------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------
	public:
      uint32_t m_lNum;												// Number of points in array (only 65536 currently accessible)
		RP3d*	m_pArray;											// Array of points

	//------------------------------------------------------------------------------
	// Functions
	//------------------------------------------------------------------------------
	public:
		// Default constructor
		RSop()
			{
			Init();
			}

		// Copy constructor
		RSop(const RSop& other)
			{
			Init();
			*this = other;
			}

		// Alternate constructor
      RSop(size_t lNum)
			{
			Init();
			Alloc(lNum);
			}

		// Destructor
		~RSop()
			{
			Free();
			}
#ifdef UNUSED_OPERATORS
		// Assignment operator
		RSop& operator=(const RSop& rhs)
			{
			Free();

			m_lNum = rhs.m_lNum;

			if (rhs.m_pArray != nullptr)
				{
				Alloc(rhs.m_lNum);
            memcpy(m_pArray, rhs.m_pArray, sizeof(RP3d) * m_lNum);
				}

			return *this;
			}
#endif
		// Overloaded == (equality) operator
		bool operator==(const RSop& rhs) const
			{
			bool result = true;
			if (m_lNum == rhs.m_lNum)
				{
				if (m_lNum > 0)
					{
					// If both pointers are non-zero then we compare their data
					if (m_pArray && rhs.m_pArray)
                  result = std::memcmp(m_pArray, rhs.m_pArray, sizeof(RP3d) * m_lNum) == SUCCESS;
					else
						{
						// If both pointers are not nullptr then they obviously don't match
						if ( !((m_pArray == nullptr) && (rhs.m_pArray == nullptr)) )
							result = false;
						}
					}
				}
			else
				result = false;
			return result;
         }

		// Allocate specified number of points
      void Alloc(size_t lNum);
		
		// Free points
		void Free(void);

		// Load from file
		int16_t	Load(RFile* fp);
		
		// Save to file
		int16_t	Save(RFile* fp);

	protected:
		// Init
		void Init(void)
			{
			m_lNum = 0;
			m_pArray = nullptr;
			}
	};


#endif // TYPES3D_H
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
