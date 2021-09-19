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
#include <cstring>

namespace {

enum { in_width  = SgbBorder::out_width };
enum { in_height = SgbBorder::out_height };
enum { in_pitch  = in_width + 3 };

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
	gambatte::uint_least32_t *outBuf_ = (gambatte::uint_least32_t *)dbuffer;
	gambatte::uint_least32_t *inBuf_ = (gambatte::uint_least32_t *)inBuf();
	for (unsigned j = 0; j < in_width; j++) {
		for (unsigned i = 0; i < in_height; i++)
			outBuf_[j * pitch + i] = inBuf_[j * in_pitch + i];
	}
}
