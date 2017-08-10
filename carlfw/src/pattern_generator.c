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
		.pulse_width_us = 1,
		.pulse_interval_us = 1000,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 20,
	},
};

static struct pattern_pulse_info pattern_TEN_KHZ[] = {
	{
		.pulse_width_us = 1,
		.pulse_interval_us = 100,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 50,
	},
};

static struct pattern_pulse_info pattern_ONE_TWO_KHZ[] = {
	{
		.pulse_width_us = 1,
		.pulse_interval_us = 1000,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 20,
	},

	{
		.pulse_width_us = 10,
		.pulse_interval_us = 500,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 15,
	},
};

/*
 * Data taken from:
 * <http://linuxwireless.org/en/developers/DFS>
 */

/* FCC Test Signal 1 - 1us pulse, 1428 us interval */
static struct pattern_pulse_info pattern_FCC1[] = {
	{
		.pulse_width_us = 1,
		.pulse_interval_us = 1428,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 18,
	},
	/* Then pause for a bit to emulate radar source sweep */
	/* 15 seconds might be more realistic, but to speed things up, let's assume 5 */
	{
		.pulse_width_us = 1,
		.pulse_interval_us = (5 * 1000000),
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 1,
	},
};

/* FCC Test Signal 4 - 11-20us pulse, 200-500 us interval */
static struct pattern_pulse_info pattern_FCC4[] = {
	{
		.pulse_width_us = 11,
		.pulse_interval_us = 200,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 12,
	},
	/* Then pause for a bit to emulate radar source sweep */
	/* 15 seconds might be more realistic, but to speed things up, let's assume 5 */
	{
		.pulse_width_us = 11,
		.pulse_interval_us = (5 * 1000000),
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 1,
	},
};

/* ETSI Test Signal 1 (Fixed) - 1us Pulse, 750 us interval */
static struct pattern_pulse_info pattern_ETSIFIXED[] = {
	{
		.pulse_width_us = 1,
		.pulse_interval_us = 750,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 18,
	},
	/* Then pause for a bit to emulate radar source sweep */
	/* 15 seconds might be more realistic, but to speed things up, let's assume 5 */
	{
		.pulse_width_us = 1,
		.pulse_interval_us = (5 * 1000000),
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 1,
	},
};

/* Users may configure this accordingly.  There are 4 patterns here,
 * so the host driver will need to fill all 4 of them accordingly
 * in order to provide expected results (and set the 'pulses' to proper
 * number in case we should skip those at the end)
 */
static struct pattern_pulse_info pattern_CUSTOM[] = {
	{
		.pulse_width_us = 1,
		.pulse_interval_us = 1000,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 10,
	},

	{
		.pulse_width_us = 1,
		.pulse_interval_us = 1000000,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 1,
	},

	{
		.pulse_width_us = 10,
		.pulse_interval_us = 500,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 10,
	},

	{
		.pulse_width_us = 10,
		.pulse_interval_us = 1000000,
		.pulse_pattern = 0xaa55,
		.pulse_mode    = 0x17f01,
		.pulse_count = 1,
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
		fw.wlan.pulse_index = 0;
		fw.wlan.in_pulse = 0;
		fw.wlan.pulse_count = 0;
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

#define MAX_UDELAY_SPIN_TICKS (100 * fw.ticks_per_usec)
void pattern_generator(void)
{
	if (fw.phy.state == CARL9170_PHY_ON) {
		if (likely(fw.wlan.soft_pattern == NO_PATTERN ||
		    fw.wlan.soft_pattern >= __CARL9170FW_NUM_PATTERNS))
			return;

		const struct pattern_info *pattern = &patterns[fw.wlan.soft_pattern];

		/* For long enough pulses, we do not want to spin here, but instead we
		 * will leave system in previous state until we are close to the timer
		 * expiration.
		 */
		if (fw.wlan.in_pulse) {
			/* We will spin the last bit of time to try to be as accurate as possible. */
			if (is_after_usecs(fw.wlan.start_pulse_ticks - MAX_UDELAY_SPIN_TICKS, fw.wlan.last_pulse_width_us)) {
				uint32_t expire_at_ticks = fw.wlan.start_pulse_ticks + (fw.wlan.last_pulse_width_us * fw.ticks_per_usec);
				uint32_t now_ticks = get_clock_counter();
				if (expire_at_ticks > now_ticks) {
					uint32_t sleep_time = expire_at_ticks - now_ticks;
					if (sleep_time > MAX_UDELAY_SPIN_TICKS)
						sleep_time = MAX_UDELAY_SPIN_TICKS;
					udelay(sleep_time / fw.ticks_per_usec);
				}
				goto pulse_done;
			}
			/* Check back a bit later */
			return;
		}
                
		if (fw.wlan.pulse_index >= pattern->pulses) {
			/* Loop back to the first pattern */
			fw.wlan.pulse_index = 0;
			fw.wlan.pulse_count = 0;
		}

		/* Make sure we have at least one pulse defined */
		if (fw.wlan.pulse_index < pattern->pulses) {
			const struct pattern_pulse_info *ppi = &pattern->pattern[fw.wlan.pulse_index];
			if (is_after_usecs(fw.wlan.pattern_last, ppi->pulse_interval_us)) {
				fw.wlan.pattern_last = get_clock_counter();

				if (ppi->pulse_width_us == 0xFFFFFFFF) {
					/* 0xFFFFFFFF pulse width means '0', backwards compat reasons. */
					goto pulse_done;
				}

				set(0x1C3BC0, ppi->pulse_pattern);
				set(0x1C3BBC, ppi->pulse_mode);

				/* Zero pulse-width means 'infinite', so don't delay nor
				 * should we change the pulse mode back if it is zero.
				 */
				if (!ppi->pulse_width_us) {
					goto infinite_pulse;
				}

				/* Seems the minimal pulse we can reliably do is about
				 * 17us.  Bail out as early as we can.
				 */
				if (ppi->pulse_width_us < 15) {
					udelay(1);  // We need some small pause or bad/no pulse is generated.
					//get(0x1C3BBC);  // We need some read or no pulse is generated.
					//get(AR9170_TIMER_REG_CLOCK_LOW); // We need some read or no pulse is generated.
					goto pulse_done;
				}

				if ((ppi->pulse_width_us * fw.ticks_per_usec) > MAX_UDELAY_SPIN_TICKS) {
					fw.wlan.in_pulse = true;
					fw.wlan.start_pulse_ticks = fw.wlan.pattern_last;
					fw.wlan.last_pulse_width_us = ppi->pulse_width_us;
					return;
				}
				udelay(ppi->pulse_width_us - 2); // small fudge to take overhead into account

pulse_done:
				set(0x1C3BBC, 0); /* Disable pulse mode */
				set(0x1C3BC0, 0); /* Clear pulse pattern */
				fw.wlan.in_pulse = false;

infinite_pulse:
				/* in case we get here from the goto statement */
				ppi = &pattern->pattern[fw.wlan.pulse_index];
				fw.wlan.pulse_count++;
				if (fw.wlan.pulse_count >= ppi->pulse_count) {
					fw.wlan.pulse_index++;
					fw.wlan.pulse_count = 0;
				}
			}
		}
	}
}

#endif /* CONFIG_CONFIG_CARL9170FW_RADAR */
