/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GS.h"

#ifdef _WIN32
#include <d3d11_1.h>
#endif

class GSUtil
{
public:
	static void Init();

	static GS_PRIM_CLASS GetPrimClass(uint32 prim);
	static int GetVertexCount(uint32 prim);
	static int GetClassVertexCount(uint32 primclass);

	static const uint32* HasSharedBitsPtr(uint32 dpsm);
	static bool HasSharedBits(uint32 spsm, const uint32* ptr);
	static bool HasSharedBits(uint32 spsm, uint32 dpsm);
	static bool HasSharedBits(uint32 sbp, uint32 spsm, uint32 dbp, uint32 dpsm);
	static bool HasCompatibleBits(uint32 spsm, uint32 dpsm);

	static CRCHackLevel GetRecommendedCRCHackLevel(GSRendererType type);
};

const char* psm_str(int psm);
