//
//   Copyright (C) 2008 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef BESS_H
#define BESS_H

#include <fstream>

namespace gambatte {

struct SaveState;

class Bess {
public:
	static bool loadState(SaveState &state, char const *stateBuf, std::size_t size, int mode);
	static bool loadState(SaveState &state, std::string const &filename, int mode);

private:
	static int loadBlock(SaveState &state, std::istringstream &file, int mode);
	Bess();
};

}

#endif
