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
// crawler.cpp
// Project: Crawler Test
//
// History:
//		04/19/97 MJR	Started.
//
////////////////////////////////////////////////////////////////////////////////

#include "crawler.h"

#include "realm.h"


bool CCrawler::CanWalk(	// Returns true if we can walk there, false otherwise.
               int16_t sx,	// In:  X position on attribute map.
               int16_t	sy,	// In:  Y position on attribute map.
               int16_t sz,	// In:  Z position on attribute map.
               int16_t* psH)	// Out: Terrain height at X/Z.
{
  bool bCanWalk;
  bool bCannotWalk;
  *psH = realm()->GetHeightAndNoWalk(sx, sz, &bCannotWalk);
  if (bCannotWalk == true								// Not walkable
      || (*psH - sy > m_sVertTolerance) )			// Terrain higher by m_sVertTolerance.
  {
    bCanWalk	= false;
  }
  else
  {
    bCanWalk	= true;
  }

  return bCanWalk;
}
