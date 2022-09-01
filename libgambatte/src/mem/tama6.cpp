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

#include "tama6.h"
#include "../savestate.h"

#include <algorithm>

namespace gambatte {

Tama6::Tama6(Time &time)
: time_(time)
{
	
}

unsigned Tama6::readPort(unsigned const p, unsigned long const cc) {
	switch (p) {
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0E:
		case 0x0F:
		case 0x12:
		case 0x13:
		// invalid port
		default: return 0xF;
	}
}

void Tama6::writePort(unsigned const p, unsigned const data, unsigned long const cc) {
	switch (p) {
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0F:
		case 0x10:
		case 0x12:
		case 0x13:
		case 0x15:
		case 0x17:
		case 0x19:
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F:
	}
}

static inline unsigned toF(unsigned cf, unsigned zf, unsigned sf) {
	return ((cf & 0x10) >> 1) | (zf & 0xF ? 0 : 4) | (sf ? 2 : 0);
}

static inline unsigned zfFromF(unsigned f) { return ~f & 4; }
static inline unsigned cfFromF(unsigned f) { return f << 1 & 0x10; }
static inline unsigned sfFromF(unsigned f) { return f & 2; }

#define hl() ( h * 0x10u | l )

#define PC_READ(dest) do { dest = rom_[pc & 0x7FF]; pc = (pc + 1) & 0xFFF; cycleCounter += 4; } while (0)

void Tama6::runFor(unsigned long const cycles) {
	unsigned long cycleCounter = cycleCounter_;
	unsigned long const end = cycleCounter + cycles;
	unsigned char opcode;
	while (cycleCounter < end) {
		PC_READ(opcode);
		switch (opcode) {
			// nop
			case 0x00:
				break;
			// testp gf
			case 0x01:
				sf = gf;
				break;
			// clr gf
			case 0x02:
				gf = 0;
				sf = 1;
				break;
			// set gf
			case 0x03:
				sf = gf = 1;
				break;
			// testp cf
			case 0x04:
				sf = cf & 0x10;
				cf = 0x10;
				break;
			// rla
			case 0x05:
			{
				unsigned oldcf = (cf & 0x10) >> 4;
				cf = a << 1;
				a = zf = (cf | oldcf) & 0xF;
				sf = ~cf & 0x10;
				break;
			}
			// test cf
			case 0x06:
				sf = ~cf & 0x10;
				cf = 0;
				break;
			// rra
			case 0x07:
			{
				unsigned oldcf = cf & 0x10;
				cf = a << 4;
				a = zf = (a | oldcf) >> 1;
				sf = ~cf & 0x10;
				break;
			}
			// inc a
			case 0x08:
				zf = a + 1;
				a = zf & 0xF;
				sf = ~zf & 0x10;
				break;
			// dec a
			case 0x09:
				zf = a - 1;
				a = zf & 0xF;
				sf = ~zf & 0x10;
				break;
			// inc [hl]
			case 0x0A:
			{
				unsigned addr = hl() & 0x7F;
				zf = ram_[addr] + 1;
				ram_[addr] = zf & 0xF;
				sf = ~zf & 0x10;
				break;
			}
			// dec [hl]
			case 0x0B:
			{
				unsigned addr = hl() & 0x7F;
				zf = ram_[addr] - 1;
				ram_[addr] = zf & 0xF;
				sf = ~zf & 0x10;
				break;
			}
			// ld a, [hl]
			case 0x0C:
				a = zf = ram_[hl() & 0x7F];
				sf = 1;
				break;
			// xch a, [hl]
			case 0x0D:
			{
				unsigned addr = hl() & 0x7F;
				zf = ram_[addr];
				ram_[addr] = a;
				a = zf;
				sf = 1;
				break;
			}
			// testp zf
			case 0x0E:
				sf = !(zf & 0xF);
				break;
			// ld [hl], a
			case 0x0F:
				ram_[hl() & 0x7F] = a;
				sf = 1;
				break;
			// ld a, h
			case 0x10:
				a = zf = h;
				sf = 1;
				break;
			// ld a, l
			case 0x11:
				a = zf = l;
				sf = 1;
				break;
			// outb [hl]
			case 0x12:
			{
				unsigned data = rom_[0x7E0 | (cf & 0x10) | ram_[hl() & 0x7F]];
				cycleCounter += 4;
				writePort(5, data & 0xF, cycleCounter);
				writePort(6, data >> 4, cycleCounter);
				sf = 1;
				break;
			}
			// xch a,eir
			case 0x13:
			{
				unsigned temp = eir;
				eir = a;
				a = temp;
				sf = 1;
				break;
			}
			// subrc a,[hl]
			case 0x14:
				zf = ram_[hl() & 0x7F] - a - (~cf >> 4 & 1);
				sf = cf = ~zf & 0x10;
				a = zf & 0xF;
				break;
			// addc a,[hl]
			case 0x15:
				cf = zf = a + ram_[hl() & 0x7F] + (cf >> 4 & 1);
				sf = ~zf & 0x10;
				a = zf & 0xF;
				break;
			// cp [hl],a
			case 0x16:
				zf = ram_[hl() & 0x7F] - a;
				cf = ~zf & 0x10;
				sf = zf & 0xF;
				break;
			// add a,[hl]
			case 0x17:
				zf = a + ram_[hl() & 0x7F];
				sf = ~zf & 0x10;
				a = zf & 0xF;
				break;
			// inc l
			case 0x18:
				zf = l + 1;
				a = zf & 0xF;
				sf = ~zf & 0x10;
				break;
			// dec l
			case 0x19:
				zf = l - 1;
				a = zf & 0xF;
				sf = ~zf & 0x10;
				break;
			// ld [hl+], a
			case 0x1A:
				ram_[hl() & 0x7F] = a;
				zf = l + 1;
				sf = zf & 0x10;
				l = zf & 0xF;
				break;
			// ld [hl-], a
			case 0x1B:
				ram_[hl() & 0x7F] = a;
				zf = l - 1;
				sf = ~zf & 0x10;
				l = zf & 0xF;
				break;
			// invalid (???)
			case 0x1C:
				fprintf(stderr, "TAMA6 - Executed invalid opcode 0x1C\n");
				break;
			// or a, [hl]
			case 0x1D:
				a = sf = zf = a | ram_[hl() & 0x7F];
				break;
			// and a, [hl]
			case 0x1E:
				a = sf = zf = a & ram_[hl() & 0x7F];
				break;
			// xor a, [hl]
			case 0x1F:
				a = sf = zf = a ^ ram_[hl() & 0x7F];
				break;
			// call a
			case 0x20: case 0x21: case 0x22: 0x23:
			case 0x24: case 0x25: case 0x26: 0x27:
			{
				unsigned imm;
				PC_READ(imm);
				unsigned spw = ram_[0x7F] << 2;
				ram_[0x40 + spw + 0] = pc >> 0 & 0xF;
				ram_[0x40 + spw + 1] = pc >> 4 & 0xF;
				ram_[0x40 + spw + 2] = pc >> 8 & 0xF;
				ram_[0x40 + spw + 3] = 0;
				ram_[0x7F] = (ram_[0x7F] - 1) & 0xF;
				pc = (opcode & 7) << 8 | imm;
				break;
			}
			// ld hl, [x]
			case 0x28:
			{
				unsigned imm;
				PC_READ(imm);
				// "lower two bits must be 0"
				imm &= 0x7C;
				l = ram_[imm + 0] & 0xF;
				h = ram_[imm + 1] & 0xF;
				sf = 1;
				break;
			}
			// xch hl, [x]
			case 0x29:
			{
				unsigned imm;
				PC_READ(imm);
				// "lower two bits must be 0"
				imm &= 0x7C;
				unsigned temp = ram_[imm + 1] << 4 | ram_[imm];
				ram_[imm + 0] = l;
				ram_[imm + 1] = h;
				l = temp & 0xF;
				h = temp >> 4;
				sf = 1;
				break;
			}
			// ret
			case 0x2A:
			{
				ram_[0x7F] = (ram_[0x7F] + 1) & 0xF;
				unsigned spw = ram_[0x7F] << 2;
				pc = ram_[0x40 + spw + 0];
				pc |= ram_[0x40 + spw + 1] << 4;
				pc |= ram_[0x40 + spw + 2] << 8;
				break;
			}
			// reti
			case 0x2B:
			{
				ram_[0x7F] = (ram_[0x7F] + 1) & 0xF;
				unsigned spw = ram_[0x7F] << 2;
				pc = ram_[0x40 + spw + 0];
				pc |= ram_[0x40 + spw + 1] << 4;
				pc |= ram_[0x40 + spw + 2] << 8;
				unsigned f = ram_[0x40 + spw + 3];
				zf = zfFromF(f);
				cf = cfFromF(f);
				sf = sfFromF(f);
				eif = true;
				break;
			}
			// out [p], k
			case 0x2C:
			{
				unsigned imm;
				PC_READ(imm);
				writePort(imm & 0xF, imm >> 4, cycleCounter);
				sf = 1;
				break;
			}
			// ld [y], k
			case 0x2D:
			{
				unsigned imm;
				PC_READ(imm);
				ram_[imm & 0xF] = imm >> 4;
				sf = 1;
				break;
			}
			// cp k, [y]
			case 0x2E:
			{
				unsigned imm;
				PC_READ(imm);
				zf = (imm >> 4) - ram_[imm & 0xF];
				cf = ~zf & 0x10;
				sf = zf & 0xF;
				break;
			}
			// add [y], k
			case 0x2F:
			{
				unsigned imm;
				PC_READ(imm);
				zf = ram_[imm & 0xF] + (imm >> 4);
				sf = ~zf & 0x10;
				ram_[imm & 0xF] = zf & 0xF;
				break;
			}
			// xch a, h
			case 0x30:
				zf = h;
				h = a;
				a = zf;
				sf = 1;
				break;
			// xch a, l
			case 0x31:
				zf = l;
				l = a;
				a = zf;
				sf = 1;
				break;
			// ldh a, [dc+]
			case 0x32:
			{
				unsigned dc = ram_[0x7E] * 0x100u | ram_[0x7D] * 0x10u | ram_[0x7C];
				a = zf = rom_[dc & 0x7FF] >> 4;
				sf = 1;
				cycleCounter += 4;
				dc = dc + 1;
				ram_[0x7C] = dc >> 0 & 0xF;
				ram_[0x7D] = dc >> 4 & 0xF;
				ram_[0x7E] = dc >> 8 & 0xF;
				break;
			}
			// ldl a, [dc]
			case 0x33:
				a = zf = rom_[(ram_[0x7E] * 0x100u | ram_[0x7D] * 0x10u | ram_[0x7C]) & 0x7FF] & 0xF;
				sf = 1;
				cycleCounter += 4;
				break;
			// set [l]
			case 0x34:
			{
				cycleCounter += 4;
				unsigned data = readPort(4 | (l >> 2), cycleCounter - 1);
				writePort(4 | (l >> 2), data | (1 << (l & 3)), cycleCounter);
				sf = 1;
				break;
			}
			// clr [l]
			case 0x35:
			{
				cycleCounter += 4;
				unsigned data = readPort(4 | (l >> 2), cycleCounter - 1);
				writePort(4 | (l >> 2), data & ~(1 << (l & 3)), cycleCounter);
				sf = 1;
				break;
			}
			// 0x36 prefix
			case 0x36:
				PC_READ(opcode);
				switch (opcode >> 6) {
					// invalid (???)
					// probably fair assumption this is just clr il
					case 0:
						fprintf(stderr, "TAMA6 - Executed invalid opcode 0x36 %02X\n", opcode);
						break;
					// eiclr il, r
					case 1:
						eif = true;
						break;
					// diclr il, r
					case 2:
						eif = false;
						break;
					// clr il
					case 3:
						// handled after break
						break;
				}
				il = il & opcode & 0x3F;
				sf = 1;
				break;
			// test [l]
			case 0x37:
			{
				cycleCounter += 4;
				unsigned data = readPort(4 | (l >> 2), cycleCounter - 1);
				sf = ~data & (1 << (l & 3));
				break;
			}
			// 0x38 prefix
			case 0x38:
				PC_READ(opcode);
				switch (opcode >> 4) {
					// add a, k
					case 0x0:
						zf = a + (opcode & 0xF);
						sf = ~zf & 0x10;
						a = zf & 0xF;
						break;
					// subr a, k
					case 0x1:
						zf = (opcode & 0xF) - a;
						sf = ~zf & 0x10;
						a = zf & 0xF;
						break;
					// or a, k
					case 0x2:
						a = sf = zf = a | (opcode & 0xF);
						break;
					// and a, k
					case 0x3:
						a = sf = zf = a & (opcode & 0xF);
						break;
					// add [hl], k
					case 0x4:
					{
						unsigned addr = hl() & 0x7F;
						zf = ram_[addr] + (opcode & 0xF);
						sf = ~zf & 0x10;
						ram_[addr] = zf & 0xF;
						break;
					}
					// subr [hl], k
					case 0x5:
					{
						unsigned addr = hl() & 0x7F;
						zf = (opcode & 0xF) - ram_[addr];
						sf = ~zf & 0x10;
						ram_[addr] = zf & 0xF;
						break;
					}
					// or [hl], k
					case 0x6:
					{
						unsigned addr = hl() & 0x7F;
						ram_[addr] = sf = zf = ram_[addr] | (opcode & 0xF);
						break;
					}
					// and [hl], k
					case 0x7:
					{
						unsigned addr = hl() & 0x7F;
						ram_[addr] = sf = zf = ram_[addr] & (opcode & 0xF);
						break;
					}
					// add l, k
					case 0x8:
						zf = l + (opcode & 0xF);
						sf = ~zf & 0x10;
						l = zf & 0xF;
						break;
					// cp k, l
					case 0x9:
						zf = (opcode & 0xF) - l;
						sf = ~zf & 0x10;
						break;
					// add h, k
					case 0xC:
						zf = h + (opcode & 0xF);
						sf = ~zf & 0x10;
						h = zf & 0xF;
						break;
					// cp k, h
					case 0xD:
						zf = (opcode & 0xF) - h;
						sf = ~zf & 0x10;
						break;
					// invalid (???)
					default:
						fprintf(stderr, "TAMA6 - Executed invalid opcode 0x38 %02X\n", opcode);
						break;
				}
				break;
			// 0x39 prefix
			case 0x39:
				PC_READ(opcode);
				switch (opcode >> 6) {
					// set y, b
					case 0:
						ram_[opcode & 0xF] |= (1 << (opcode >> 4 & 3));
						sf = 1;
						break;
					// clr y, b
					case 1:
						ram_[opcode & 0xF] &= ~(1 << (opcode >> 4 & 3));
						sf = 1;
						break;
					// test y, b
					case 2:
						sf = ~ram_[opcode & 0xF] & (1 << (opcode >> 4 & 3));
						break;
					// testp y, b
					case 3:
						sf = ram_[opcode & 0xF] & (1 << (opcode >> 4 & 3));
						break;
				}
				break;
			// 0x3A prefix
			case 0x3A:
				PC_READ(opcode);
				switch (opcode >> 4) {
					// in a, [p]
					case 0x2:
						a = sf = zf = readPort(opcode & 0xF, cycleCounter - 1);
						break;
					// in [hl], [p]
					case 0x6:
						ram_[hl() & 0x7F] = sf = zf = readPort(opcode & 0xF, cycleCounter - 1);
						break;
					// out [p], a
					case 0x8:
					case 0xA:
						writePort((opcode >> 1 & 0x10) | (opcode & 0xF), a, cycleCounter - 1);
						sf = 1;
						break;
					// out [p], [hl]
					case 0xC:
					case 0xE:
						writePort((opcode >> 1 & 0x10) | (opcode & 0xF), ram_[hl() & 0x7F], cycleCounter - 1);
						sf = 1;
						break;
					// invalid (???)
					default:
						fprintf(stderr, "TAMA6 - Executed invalid opcode 0x3A %02X\n", opcode);
						break;
				}
				break;
			// 0x3B prefix
			case 0x3B:
				PC_READ(opcode);
				switch (opcode >> 6) {
					// set [p], b
					case 0:
					{
						unsigned data = readPort(opcode & 0xF, cycleCounter - 1);
						writePort(opcode & 0xF, data | (1 << (opcode >> 4 & 3)), cycleCounter);
						sf = 1;
						break;
					}
					// clr [p], b
					case 1:
					{
						unsigned data = readPort(opcode & 0xF, cycleCounter - 1);
						writePort(opcode & 0xF, data & ~(1 << (opcode >> 4 & 3)), cycleCounter);
						sf = 1;
						break;
					}
					// test [p], b
					case 2:
					{
						unsigned data = readPort(opcode & 0xF, cycleCounter - 1);
						sf = ~data & (1 << (opcode >> 4 & 3));
						break;
					}
					// testp [p], b
					case 3:
					{
						unsigned data = readPort(opcode & 0xF, cycleCounter - 1);
						sf = data & (1 << (opcode >> 4 & 3));
						break;
					}
				}
				break;
			// ld a, [x]
			case 0x3C:
			{
				unsigned imm;
				PC_READ(imm);
				a = zf = ram_[imm & 0x7F];
				sf = 1;
				break;
			}
			// xch a, [x]
			case 0x3D:
			{
				unsigned imm;
				PC_READ(imm);
				zf = ram_[imm & 0x7F];
				ram_[imm & 0x7F] = a;
				a = zf;
				sf = 1;
				break;
			}
			// cp [x], a
			case 0x3E:
			{
				unsigned imm;
				PC_READ(imm);
				zf = ram_[imm & 0x7F] - a;
				cf = ~zf & 0x10;
				sf = zf & 0xF;
				break;
			}
			// ld [x], a
			case 0x3F:
			{
				unsigned imm;
				PC_READ(imm);
				ram_[imm & 0x7F] = a;
				sf = 1;
				break;
			}
			// ld a, k
			case 0x40: case 0x41: case 0x42: case 0x43:
			case 0x44: case 0x45: case 0x46: case 0x47:
			case 0x48: case 0x49: case 0x4A: case 0x4B:
			case 0x4C: case 0x4D: case 0x4E: case 0x4F:
				a = zf = opcode & 0xF;
				sf = 1;
				break;
			// set [hl], b
			case 0x50: case 0x51: case 0x52: case 0x53:
				ram_[hl() & 0x7F] |= (1 << (opcode & 3));
				sf = 1;
				break;
			// clr [hl], b
			case 0x54: case 0x55: case 0x56: case 0x57:
				ram_[hl() & 0x7F] &= ~(1 << (opcode & 3));
				sf = 1;
				break;
			// test [hl], b
			case 0x58: case 0x59: case 0x5A: case 0x5B:
				sf = ~ram_[hl() & 0x7F] & (1 << (opcode & 3));
				break;
			// test a, b
			case 0x5C: case 0x5D: case 0x5E: case 0x5F:
				sf = ~a & (1 << (opcode & 3));
				break;
			// bs a
			case 0x60: case 0x61: case 0x62: case 0x63:
			case 0x64: case 0x65: case 0x66: case 0x67:
			case 0x68: case 0x69: case 0x6A: case 0x6B:
			case 0x6C: case 0x6D: case 0x6E: case 0x6F:
			{
				unsigned imm;
				PC_READ(imm);
				if (sf)
					pc = (opcode << 8 | imm) & 0xFFF;

				sf = 1;
				break;
			}
			// calls a
			case 0x70: case 0x71: case 0x72: case 0x73:
			case 0x74: case 0x75: case 0x76: case 0x77:
			case 0x78: case 0x79: case 0x7A: case 0x7B:
			case 0x7C: case 0x7D: case 0x7E: case 0x7F:
			{
				unsigned spw = ram_[0x7F] << 2;
				ram_[0x40 + spw + 0] = pc >> 0 & 0xF;
				ram_[0x40 + spw + 1] = pc >> 4 & 0xF;
				ram_[0x40 + spw + 2] = pc >> 8 & 0xF;
				ram_[0x40 + spw + 3] = 0;
				ram_[0x7F] = (ram_[0x7F] - 1) & 0xF;
				pc = (opcode & 0xF ? opcode & 0xF : 0x10) << 3 | 0x6;
				break;
			}
			// bss a
			case 0x80: case 0x81: case 0x82: case 0x83:
			case 0x84: case 0x85: case 0x86: case 0x87:
			case 0x88: case 0x89: case 0x8A: case 0x8B:
			case 0x8C: case 0x8D: case 0x8E: case 0x8F:
			case 0x90: case 0x91: case 0x92: case 0x93:
			case 0x94: case 0x95: case 0x96: case 0x97:
			case 0x98: case 0x99: case 0x9A: case 0x9B:
			case 0x9C: case 0x9D: case 0x9E: case 0x9F:
			case 0xA0: case 0xA1: case 0xA2: case 0xA3:
			case 0xA4: case 0xA5: case 0xA6: case 0xA7:
			case 0xA8: case 0xA9: case 0xAA: case 0xAB:
			case 0xAC: case 0xAD: case 0xAE: case 0xAF:
			case 0xB0: case 0xB1: case 0xB2: case 0xB3:
			case 0xB4: case 0xB5: case 0xB6: case 0xB7:
			case 0xB8: case 0xB9: case 0xBA: case 0xBB:
			case 0xBC: case 0xBD: case 0xBE: case 0xBF:
				if (sf) {
					pc &= ~0x3Fu;
					pc |= opcode & 0x3F;
				}

				sf = 1;
				break;
			// ld h, k
			case 0xC0: case 0xC1: case 0xC2: case 0xC3:
			case 0xC4: case 0xC5: case 0xC6: case 0xC7:
			case 0xC8: case 0xC9: case 0xCA: case 0xCB:
			case 0xCC: case 0xCD: case 0xCE: case 0xCF:
				h = opcode & 0xF;
				sf = 1;
				break;
			// cp k, a
			case 0xD0: case 0xD1: case 0xD2: case 0xD3:
			case 0xD4: case 0xD5: case 0xD6: case 0xD7:
			case 0xD8: case 0xD9: case 0xDA: case 0xDB:
			case 0xDC: case 0xDD: case 0xDE: case 0xDF:
				zf = (opcode & 0xF) - a;
				cf = ~zf & 0x10;
				sf = zf & 0xF;
				break;
			// ld l, k
			case 0xE0: case 0xE1: case 0xE2: case 0xE3:
			case 0xE4: case 0xE5: case 0xE6: case 0xE7:
			case 0xE8: case 0xE9: case 0xEA: case 0xEB:
			case 0xEC: case 0xED: case 0xEE: case 0xEF:
				l = opcode & 0xF;
				sf = 1;
				break;
			// ld [hl+], k
			case 0xF0: case 0xF1: case 0xF2: case 0xF3:
			case 0xF4: case 0xF5: case 0xF6: case 0xF7:
			case 0xF8: case 0xF9: case 0xFA: case 0xFB:
			case 0xFC: case 0xFD: case 0xFE: case 0xFF:
				ram_[hl() & 0x7F] = opcode & 0xF;
				zf = l + 1;
				l = sf = zf & 0xF;
				break;
		}
	}

	cycleCounter_ = cycleCounter;
}

void Tama6::updateClock(unsigned long const cc) {
	
}

unsigned long long Tama6::timeNow() const {
	
}

void Tama6::setTime(unsigned long long const dividers) {
	
}

void Tama6::setBaseTime(timeval baseTime, unsigned long const cc) {
	
}

void Tama6::getRam(unsigned char *dest, unsigned long const cc) {
	
}

void Tama6::setRam(unsigned char *src) {
	
}

void Tama6::accumulateSamples(unsigned long const cc) {
	
}

unsigned Tama6::generateSamples(short *soundBuf) {
	
}

void Tama6::resetCc(unsigned long const oldCc, unsigned long const newCc) {
	
}

void Tama6::speedChange(unsigned long const cc) {
	
}

void Tama6::setStatePtrs(SaveState &state) {
	
}

void Tama6::saveState(SaveState &state) const {
	
}

void Tama6::loadState(SaveState const &state, bool const ds) {
	
}

unsigned char Tama6::read(unsigned /*p*/, unsigned long const cc) {
	
}

void Tama6::write(unsigned /*p*/, unsigned data, unsigned long const cc) {
	
}

SYNCFUNC(Tama6) {
	
}

}
