/*
 * silabs_tercab.h - header for the Silicon Laboratories terrestrial/cable
 * hybrid tuner series
 *
 * (C) Copyright 2014, PCTV Systems S.à r.l
 * Henning Garbers <hgarbers@pctvsystems.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 */

#ifndef __SILABS_TERCAB_H__
#define __SILABS_TERCAB_H__

#include <linux/i2c.h>
#include "dvb_frontend.h"

#define HVR19x5_QAM_IF 4000
#define HVR19x5_VSB_IF 3250

#define _Si2168B_CLOCK_ALWAYS_OFF 0  /* turn clock ON then never switch it off, used when the clock is provided to another part */
#define _Si2168B_CLOCK_ALWAYS_ON  1  /* never switch it on, used when the clock is not going anywhere */
#define _Si2168B_CLOCK_MANAGED    2  /* control clock state as before */

/* settings for the Silicon Labs demod driver */
#define L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN_SETUP
#define L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP
#define L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC_SETUP

/******************************************************************************/
/* Si2177 Tuner FEF management options */
/******************************************************************************/
#define Si2177_FEF_MODE_SLOW_NORMAL_AGC  0
#define Si2177_FEF_MODE_FREEZE_PIN       1
#define Si2177_FEF_MODE_SLOW_INITIAL_AGC 2

struct silabs_tercab_config {
	/* the tuner's i2c address */
	u8 tuner_address;

	/* user defined IF frequencies in KHz */
	u16 qam_if_khz;
	u16 vsb_if_khz;

	/* tuner clock_control:   how the TER clock must be controlled
       possible modes:
       CLOCK_ALWAYS_ON  (=0)
       CLOCK_ALWAYS_OFF (=1)
	   CLOCK_MANAGED    (=2) */
	u8 tuner_clock_control;

	/* enable / disable AGC */
	u8 tuner_agc_control;

	/* FEF management options
	   FEF_MODE_SLOW_NORMAL_AGC  (=0)
	   FEF_MODE_FREEZE_PIN       (=1)
	   FEF_MODE_SLOW_INITIAL_AGC (=2) */
    u8 fef_mode;

    u8  crystal_trim_xo_cap;

	/* tuner i2c connection               */
	/* 0-tuner connected through Si2168B  */
	/* 1-tuner is direct accessible       */
	u8 indirect_i2c_connection;

	struct dvb_frontend *fe;
};

#if IS_ENABLED(CONFIG_MEDIA_TUNER_SILABS_TERCAB)
extern int silabs_tercab_autodetection(struct i2c_adapter* i2c_adapter, u8 i2c_addr);

extern struct dvb_frontend *silabs_tercab_attach(struct dvb_frontend *fe,
					    struct i2c_adapter *i2c,
					    struct silabs_tercab_config *cfg);
#else
static inline int silabs_tercab_autodetection(struct i2c_adapter* i2c_adapter,
					u8 i2c_addr)
{
	printk(KERN_INFO "%s: not probed - driver disabled by Kconfig\n",
	       __func__);
	return -EINVAL;
}

static inline struct dvb_frontend *silabs_tercab_attach(struct dvb_frontend *fe,
						   struct i2c_adapter *i2c,
						   struct silabs_tercab_config *cfg)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* __SILABS_TERCAB_H__ */
