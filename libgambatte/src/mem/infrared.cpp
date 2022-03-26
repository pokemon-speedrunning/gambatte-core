//
//   Copyright (C) 2007-2010 by sinamas <sinamas at users.sourceforge.net>
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

#include "infrared.h"

namespace gambatte {

Infrared::Infrared()
: irTrigger_(true)
, thisGbIrSignal_(false)
, linkedGbIrSignal_(false)
{
}

bool Infrared::getIrSignal(WhichIrGb which) const {
	if (which == this_gb)
		return thisGbIrSignal_;
	else if (which == linked_gb)
		return linkedGbIrSignal_;

	return false;
}

void Infrared::setIrSignal(WhichIrGb which, bool signal) {
	if (which == this_gb) {
		irTrigger_ |= thisGbIrSignal_ ^ signal;
		thisGbIrSignal_ = signal;
	} else if (which == linked_gb) {
		irTrigger_ |= linkedGbIrSignal_ ^ signal;
		linkedGbIrSignal_ = signal;
	}
}

void Infrared::saveState(SaveState &ss) const {
	ss.ir.irTrigger = irTrigger_;
	ss.ir.thisGbIrSignal = thisGbIrSignal_;
	ss.ir.linkedGbIrSignal = linkedGbIrSignal_;
}

void Infrared::loadState(SaveState const &ss) {
	irTrigger_ = ss.ir.irTrigger;
	thisGbIrSignal_ = ss.ir.thisGbIrSignal;
	linkedGbIrSignal_ = ss.ir.linkedGbIrSignal;
}

SYNCFUNC(Infrared) {
	NSS(irTrigger_);
	NSS(thisGbIrSignal_);
	NSS(linkedGbIrSignal_);
}

}
