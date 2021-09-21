/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#include "vfilterinfo.h"
#include "vfilters/sgbborder.h"
#include "vfilters/catrom2x.h"
#include "vfilters/catrom3x.h"
#include "vfilters/kreed2xsai.h"
#include "vfilters/maxsthq2x.h"
#include "vfilters/maxsthq3x.h"

static VideoLink * createNone() { return 0; }

template<class T>
static VideoLink * createT() { return new T; }

#define VFINFO(handle, Type) { handle, Type::out_width, Type::out_height, createT<Type> }

static VfilterInfo const vfinfos[] = {
	{ "None", VfilterInfo::in_width, VfilterInfo::in_height, createNone },
#ifdef SHOW_PLATFORM_SGB
	VFINFO("SGB Border", SgbBorder),
#endif
	VFINFO("Bicubic Catmull-Rom spline 2x", Catrom2x),
	VFINFO("Bicubic Catmull-Rom spline 3x", Catrom3x),
	VFINFO("Kreed's 2xSaI", Kreed2xSaI),
	VFINFO("MaxSt's hq2x", MaxStHq2x),
	VFINFO("MaxSt's hq3x", MaxStHq3x),
};

std::size_t VfilterInfo::numVfilters() {
	return sizeof vfinfos / sizeof vfinfos[0];
}

VfilterInfo const & VfilterInfo::get(std::size_t n) {
	return vfinfos[n];
}
