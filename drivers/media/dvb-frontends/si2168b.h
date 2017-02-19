#ifndef SI2168_H
#define SI2168_H

#define DRIVER_BUILD
#define TER_TUNER_SILABS
#define HAUPPAUGE
#define HCW_10BDA
#define no_CHECK_CRC
#define no_RANGE_CHECK
#define __MPLP_TEST__
#ifndef FE_READ_STREAM_IDS
#ifdef __MPLP_TEST__
struct dvb_stream_ids {
	__u8	val[256];	/* stream ids seen on the transponder */
	__u8    num;		/* number of valid stream ids in 'val' */
	__u8    cur;		/* currently selected stream id */
};
#endif
#endif

#include <linux/kconfig.h>
#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"

extern int _sitrace; /* module parameter: SiTRACE on/off */

struct si2168b_config {
	/* the demodulator's i2c address */
	u8 demod_address;

	/* minimum delay before retuning */
	int min_delay_ms;

	u8 ts_bus_mode; /*1-serial, 2-parallel.*/
	u8 ts_clock_mode; /*0-auto, 1-manual.*/
	u8 clk_gapped_en; /*0-disabled, 1-enabled.*/
	u8 ts_par_clk_invert; /* 0-not-invert, 1-invert */
	u8 ts_par_clk_shift;

	/* TER Tuner FEF management options
	   FEF_MODE_SLOW_NORMAL_AGC  (=0)
	   FEF_MODE_FREEZE_PIN       (=1)
	   FEF_MODE_SLOW_INITIAL_AGC (=2) */
    u8 fef_mode;
    /* FEF pin connected to TER tuner AGC freeze input */
	u8 fef_pin;
	 /* GPIO state on FEF_pin when used (during FEF periods) */
	u8 fef_level;

    /* tuner i2c connection               */
	/* 0-tuner connected through Si2168B  */
	/* 1-tuner is direct accessible       */
	u8 indirect_i2c_connection;

	int (*start_ctrl)(struct dvb_frontend *fe);
};

#if IS_ENABLED(CONFIG_DVB_SI2168B)
extern struct dvb_frontend *si2168b_attach(const struct si2168b_config *config,
						struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *si2168b_attach(
		const struct si2168b_config *config, struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif
