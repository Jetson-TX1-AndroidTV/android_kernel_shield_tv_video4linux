#ifndef SILG_H
#define SILG_H

#include <linux/kconfig.h>
#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"

#include "lgdt3306a.h"
#include "si2168b.h"

#define CUSTOM_TUNING_ALGO

struct silg_config {
	/* Si2168B demodulator configuration */
	int si_demod_enable:1; /* 0:no SiLabs demod 1:SiLabs demod present */

	u8 si_i2c_addr; /* the Si2168B demodulator's i2c address */
	u8 ts_bus_mode; /*1-serial, 2-parallel.*/
	u8 ts_clock_mode; /*0-auto, 1-manual.*/
	u8 clk_gapped_en; /*0-disabled, 1-enabled.*/
	u8 ts_par_clk_invert; /* 0-not-invert, 1-invert */
	u8 ts_par_clk_shift;
    u8 fef_mode; /*0-slow normal AGC, 1-freeze pin, 2-slow initial AGC*/
	u8 fef_pin; /* FEF pin connected to TER tuner AGC freeze input */
	u8 fef_level; /* GPIO state on FEF_pin when used (during FEF periods) */

	/* tuner i2c connection               */
	/* 0-tuner connected through Si2168B  */
	/* 1-tuner is direct accessible       */
	u8 indirect_i2c_connection;

	int min_delay_ms; /* minimum delay before retuning */
	int (*start_ctrl)(struct dvb_frontend *fe);

	/* LG3306A demodulator configuration */
	int lg_demod_enable:1; /* 0:no LG demod 1:LG demod present */

	u8 lg_i2c_addr; /* the LG3306A demodulator's i2c address */

	/* user defined IF frequency in KHz */
	u16 qam_if_khz;
	u16 vsb_if_khz;

	/* disable i2c repeater - 0:repeater enabled 1:repeater disabled */
	int deny_i2c_rptr:1;

	/* spectral inversion - 0:disabled 1:enabled */
	int spectral_inversion:1;

	enum lgdt3306a_mpeg_mode mpeg_mode;
	enum lgdt3306a_tp_clock_edge tpclk_edge;
	enum lgdt3306a_tp_valid_polarity tpvalid_polarity;

	int  xtalMHz;//demod clock freq in MHz; 24 or 25 supported
};

#if IS_ENABLED(CONFIG_DVB_SILG)
extern struct dvb_frontend *silg_attach(const struct silg_config *config,
						struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *silg_attach(
		const struct silg_config *config, struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif
