/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
#include "sgbborder.h"

#include <algorithm>

namespace {

enum { in_width  = SgbBorder::out_width };
enum { in_height = SgbBorder::out_height };
enum { in_pitch  = in_width + 3 };

static void blit(gambatte::uint_least32_t *d,
                 std::ptrdiff_t const dstPitch,
                 gambatte::uint_least32_t const *s,
                 std::ptrdiff_t const srcPitch,
                 unsigned const w,
                 unsigned h)
{
	do {
		std::ptrdiff_t i = -static_cast<std::ptrdiff_t>(w);
		s += w;
		d += w;

		do {
			d[i] = s[i];
		} while (++i);

		s += srcPitch - static_cast<std::ptrdiff_t>(w);
		d += dstPitch - static_cast<std::ptrdiff_t>(w);
	} while (--h);
}

} // anon namespace

SgbBorder::SgbBorder()
: buffer_((in_height + 3UL) * in_pitch)
{
	std::fill_n(buffer_.get(), buffer_.size(), 0);
}

void * SgbBorder::inBuf() const {
	return buffer_ + in_pitch + 1;
}

std::ptrdiff_t SgbBorder::inPitch() const {
	return in_pitch;
}

void SgbBorder::draw(void *dbuffer, std::ptrdiff_t pitch) {
	::blit(static_cast<gambatte::uint_least32_t *>(dbuffer), pitch,
	       static_cast<gambatte::uint_least32_t *>(inBuf()), in_pitch, in_width, in_height);
}
