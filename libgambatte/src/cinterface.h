//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef CINTERFACE_H
#define CINTERFACE_H

#ifdef SHLIB
	#ifdef _WIN32
		#define GBEXPORT extern "C" __declspec(dllexport)
	#elif defined(__GNUC__) && __GNUC__ >= 4
		#define GBEXPORT extern "C" __attribute__((visibility("default")))
	#else
		#define GBEXPORT extern "C"
	#endif
#else
	#define GBEXPORT extern "C"
#endif

#endif
