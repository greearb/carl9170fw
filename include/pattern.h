/*
 * carl9170 firmware - used by the ar9170 wireless device
 *
 * Pattern pulse definitions
 *
 * Copyright 2012 Christian Lamparter <chunkeey@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __CARL9170FW_PATTERN_H
#define __CARL9170FW_PATTERN_H

#include "types.h"
#include "compiler.h"
#include "fwdesc.h"

enum PATTERN_TYPE {
	NO_PATTERN = 0,
	ONE_KHZ,
	TEN_KHZ,

	ONE_TWO_KHZ,

	FCC1,
	FCC4,

	ETSIFIXED,

        CUSTOM,

	/* keep last */
	__CARL9170FW_NUM_PATTERNS
};

struct pattern_pulse_info {
	unsigned int pulse_width;
	unsigned int pulse_interval;
	uint32_t     pulse_pattern;
	uint32_t     pulse_mode;
};

struct pattern_info {
	unsigned int pulses;
	struct pattern_pulse_info *pattern;
};

void pattern_wreg(uint32_t addr, uint32_t val);

#endif /* __CARL9170FW_PATTERN_H */
