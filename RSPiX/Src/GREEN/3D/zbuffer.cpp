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
#include <BLUE/System.h>
#include <ORANGE/QuickMath/FixedPoint.h>
#include "zbuffer.h"

#include <cstdlib>


// ************************ RZBuffer
void	RZBuffer::Init()
	{
	m_sW = m_sH = 0;
	m_lP = 0;
	m_pBuf = nullptr;
	}

RZBuffer::RZBuffer()
	{
	Init();
	}

RZBuffer::RZBuffer(int16_t sW,int16_t sH)
	{
	Init();
	Create(sW,sH);
	}

int16_t RZBuffer::Create(int16_t sW,int16_t sH)
	{
	if (m_pBuf)
		{
		TRACE("RZBuffer::Create: already exists!\n");
      return FAILURE;
		}

	m_sW = sW;
	m_sH = sH;
   m_lP = (sW+3)&~3; // keep for emergencies!

   size_t lSize = m_lP * m_sH * sizeof(int16_t);
	m_pBuf = (int16_t*) malloc(lSize);
	// you then may clear it, buddy!

	return SUCCESS;
	}

RZBuffer::~RZBuffer()
	{
	Destroy();
	}

int16_t RZBuffer::Destroy()
	{
	if (!m_pBuf)
		{
		TRACE("RZBuffer::Destroy: already deleted!\n");
      return FAILURE;
		}

	free(m_pBuf);

	Init();
	return SUCCESS;
	}

//----------------------------------------------
void RZBuffer::Clear(int16_t sVal)
	{
	// Do by 64-bit longs
	// 1) Create the 64-bit long
	union
		{
		uint64_t word;
		struct
			{
			int16_t p1;
			int16_t p2;
			int16_t p3;
			int16_t p4;
			};
		} BigWord;

	BigWord.p1 = BigWord.p2 = BigWord.p3 = BigWord.p4 = sVal;

	//2) Do the copy
	int32_t lWordP = m_lP >> 2; // 4 * 16 = 64
	uint64_t* pWord = (uint64_t*) m_pBuf;

	int32_t lWordLen = lWordP * m_sH;
	for (int32_t i=0;i < lWordLen; i++) *pWord++ = BigWord.word;

	}
#ifdef UNUSED_FUNCTIONS
//----------------------------------------------
// debugging stuff
void RZBuffer::TestHeight(RImage* pimDst,int16_t sDepth,
				int16_t sX,int16_t sY,int16_t sW,int16_t sH)
	{
	int16_t i,j;
	int32_t lP = pimDst->m_lPitch;

	uint8_t *pDst,*pDstLine = pimDst->m_pData + lP * sY + sX;
	int16_t *pZB,*pZLine = m_pBuf + m_lP * sY + sX;

	for (j=0;j<sH;j++,pDstLine += lP,pZLine += m_lP )
		{
		pDst = pDstLine;
		pZB = pZLine;
		for (i=0;i<sW;i++,pDst++,*pZB++)
			{
			if (*pZB < sDepth)
				*pDst = 255; // draw it!
			}
		}
	}

void RZBuffer::Dump(RImage* pimDst,int16_t sX,int16_t sY,int16_t sW,int16_t sH,uint8_t* pZCol)
	{
	int16_t i,j;
	int32_t lP = pimDst->m_lPitch;

	uint8_t *pDst,*pDstLine = pimDst->m_pData + lP * sY + sX;
	int16_t *pZLine = m_pBuf + m_lP * sY + sX;
	RFixedS16 *pZB;


	for (j=0;j<sH;j++,pDstLine += lP,pZLine += m_lP )
		{
		pDst = pDstLine;
		pZB = (RFixedS16*)pZLine;
		for (i=0;i<sW;i++,pZB++)
			{
			*pDst++ = pZCol[(*pZB).mod + 128]; // color code it!
			}
		}
	}
#endif
