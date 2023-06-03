//
//   Copyright (C) 2023 by Pierre Wendling <pierre.wendling.4@gmail.com>
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

#ifndef GBEXPORT_H
#define GBEXPORT_H

#include "config.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#define GBEXPORT __declspec(dllexport)
#elif defined(LIBGAMBATTE_HAVE_ATTRIBUTE_VISIBILITY_DEFAULT)
#define GBEXPORT __attribute__((visibility("default")))
#else
#define GBEXPORT
#endif

#endif
