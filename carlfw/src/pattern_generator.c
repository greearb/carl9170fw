/*
 * carl9170 firmware - used by the ar9170 wireless device
 *
 * pattern generator
 *
 * Copyright 2013	Christian Lamparter <chunkeey@googlemail.com>
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

#include "carl9170.h"
#include "pattern_generator.h"
#include "fwdsc.h"
#include "timer.h"

#if defined(CONFIG_CARL9170FW_PATTERN_GENERATOR)

static struct pattern_pulse_info pattern_NO_PATTERN[0] = {  };
static struct pattern_pulse_info pattern_ONE_KHZ[] = {
	{
		.pulse_width = 1,
		.pulse_interval = 1000,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
	},
};

static struct pattern_pulse_info pattern_TEN_KHZ[] = {
	{
		.pulse_width = 1,
		.pulse_interval = 100,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
	},
};

static struct pattern_pulse_info pattern_ONE_TWO_KHZ[] = {
	{
		.pulse_width = 1,
		.pulse_interval = 1000,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
	},

	{
		.pulse_width = 10,
		.pulse_interval = 500,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
	},
};

/*
 * Data taken from:
 * <http://linuxwireless.org/en/developers/DFS>
 */

/* FCC Test Signal 1 - 1us pulse, 1428 us interval */
static struct pattern_pulse_info pattern_FCC1[] = {
	{
		.pulse_width = 1,
		.pulse_interval = 1428,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
	},
};

/* FCC Test Signal 4 - 11-20us pulse, 200-500 us interval */
static struct pattern_pulse_info pattern_FCC4[] = {
	{
		.pulse_width = 11,
		.pulse_interval = 200,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x7f01,
	},
};

/* ETSI Test Signal 1 (Fixed) - 1us Pulse, 750 us interval */
static struct pattern_pulse_info pattern_ETSIFIXED[] = {
	{
		.pulse_width = 1,
		.pulse_interval = 750,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x7f01,
	},
};

/* Users may configure this accordingly.  There are 4 patterns here,
 * so the host driver will need to fill all 4 of them accordingly
 * in order to provide expected results (some may have zero pulse_width
 * if they should be skipped, for instance.)
 */
static struct pattern_pulse_info pattern_CUSTOM[] = {
	{
		.pulse_width = 1,
		.pulse_interval = 1000,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
	},

	{
		.pulse_width = 10,
		.pulse_interval = 500,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
	},

	{
		.pulse_width = 10,
		.pulse_interval = 500,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
	},

	{
		.pulse_width = 10,
		.pulse_interval = 500,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
	},
};

#define ADD_RADAR(name) [name] = { .pulses = ARRAY_SIZE(pattern_## name), .pattern = pattern_## name }

static struct pattern_info patterns[__CARL9170FW_NUM_PATTERNS] = {
	ADD_RADAR(NO_PATTERN),
	ADD_RADAR(ONE_KHZ),
	ADD_RADAR(TEN_KHZ),
	ADD_RADAR(ONE_TWO_KHZ),
	ADD_RADAR(FCC1),
	ADD_RADAR(FCC4),
	ADD_RADAR(ETSIFIXED),
        ADD_RADAR(CUSTOM),
};

void pattern_wreg(uint32_t addr, uint32_t val)
{
	struct pattern_info *pi = &patterns[CUSTOM];
	uint32_t *pattern_addrs = (uint32_t*)(pi->pattern);
	addr = addr & ~0x80000000; /* Remove special 'pattern' bit. */
	if (addr == 0xFFFF) {
		fw.wlan.soft_pattern = val;
		/* Reset the pattern indices as well. */
		fw.wlan.pattern_last = get_clock_counter();
		fw.wlan.pattern_index = 0;
                if (fw.wlan.soft_pattern == NO_PATTERN) {
                   set(0x1C3BBC, 0); /* Disable pulse mode */
                   set(0x1C3BC0, 0); /* Clear pulse pattern */
                }
	} else if (addr == 0xFFFE) {
		pi->pulses = val;
	} else {
		/* Set a value in the pattern_CUSTOM data array. */
		pattern_addrs[addr] = val;
	}
}

void pattern_generator(void)
{
	if (fw.phy.state == CARL9170_PHY_ON) {
		if (likely(fw.wlan.soft_pattern == NO_PATTERN ||
		    fw.wlan.soft_pattern >= __CARL9170FW_NUM_PATTERNS))
			return;

		const struct pattern_info *pattern = &patterns[fw.wlan.soft_pattern];
		if (pattern->pulses >= fw.wlan.pattern_index) {
			fw.wlan.pattern_index = 0;
		}

		if (pattern->pulses > fw.wlan.pattern_index) {
			const struct pattern_pulse_info *ppi = &pattern->pattern[fw.wlan.pattern_index];
			if (is_after_usecs(fw.wlan.pattern_last, ppi->pulse_interval)) {
				fw.wlan.pattern_last = get_clock_counter();
				set(0x1C3BC0, ppi->pulse_pattern);
				set(0x1C3BBC, ppi->pulse_mode);
				/* Zero pulse-width means 'infinite', so don't delay nor
				 * should we change the pulse mode back if it is zero.
				 */
				if (ppi->pulse_width) {
					udelay(ppi->pulse_width);
					set(0x1C3BBC, 0);
				}
				fw.wlan.pattern_index++;
			}
		}
	}
}

#endif /* CONFIG_CONFIG_CARL9170FW_RADAR */
