/* DVB compliant Linux driver for dual demodulator system with
*  a Silicon Labs Si2168B DVB-T/T2/C demodulator
*  and a LG 3306A ATSC demodulator
*
* Copyright (C) 2014 PCTV Systems S.à r.l & Silicon Laboratories Inc.
*
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "dvb_frontend.h"
#include "silg.h"

static int debug = 0;
module_param_named(debug, debug, int, 0644);

#define _SI_ 0 /* index referencing Silicon Labs Si2168B demodulator */
#define _LG_ 1 /* index referencing LG3306A demodulator */

#define silg_printk(args...) \
	do { \
		if (debug) \
			printk(KERN_DEBUG "silg: " args); \
	} while (0)

struct silg_priv {
	int si_demod_enable:1;
	int lg_demod_enable:1;
	struct si2168b_config si2168b_cfg;
	struct lgdt3306a_config lgdt3306a_cfg;
	struct dvb_frontend *demod[2];
	int demod_bus_state[2]; /* flag to avoid unnecessary calls */
	struct dvb_frontend frontend;
	fe_delivery_system_t delivery_system;
	u32 dmid; /* demod id used as index */
};

/* names of the delivery systems for debugging purposes only */
static char *delsys_name(fe_delivery_system_t delsys)
{
	switch (delsys)	{
	case SYS_UNDEFINED    : {return (char*)"SYS_UNDEFINED"   ;}
	case SYS_DVBC_ANNEX_A : {return (char*)"SYS_DVBC_ANNEX_A";}
	case SYS_DVBC_ANNEX_B : {return (char*)"SYS_DVBC_ANNEX_B";}
	case SYS_DVBT         : {return (char*)"SYS_DVBT"        ;}
	case SYS_DSS          : {return (char*)"SYS_DSS"         ;}
	case SYS_DVBS         : {return (char*)"SYS_DVBS"        ;}
	case SYS_DVBS2        : {return (char*)"SYS_DVBS2"       ;}
	case SYS_DVBH         : {return (char*)"SYS_DVBH"        ;}
	case SYS_ISDBT        : {return (char*)"SYS_ISDBT"       ;}
	case SYS_ISDBS        : {return (char*)"SYS_ISDBS"       ;}
	case SYS_ISDBC        : {return (char*)"SYS_ISDBC"       ;}
	case SYS_ATSC         : {return (char*)"SYS_ATSC"        ;}
	case SYS_ATSCMH       : {return (char*)"SYS_ATSCMH"      ;}
	case SYS_DTMB         : {return (char*)"SYS_DTMB"        ;}
	case SYS_CMMB         : {return (char*)"SYS_CMMB"        ;}
	case SYS_DAB          : {return (char*)"SYS_DAB"         ;}
	case SYS_DVBT2        : {return (char*)"SYS_DVBT2"       ;}
	case SYS_TURBO        : {return (char*)"SYS_TURBO"       ;}
	case SYS_DVBC_ANNEX_C : {return (char*)"SYS_DVBC_ANNEX_C";}
    default:
    	break;
	}
	return (char*)"* UNKNOWN *";
}

static int silg_i2c_gate_ctrl(struct dvb_frontend* fe, int enable)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod = priv->demod[priv->dmid];

	return demod ? demod->ops.i2c_gate_ctrl(demod, enable) : -ENODEV;
}

/* control both demodulator ts busses to avoid bus conflicts
   modes:
   Si2168B ON,        LG3306A tri-state
   Si2168B tri-state, LG3306A ON
   Si2168B tri-state, LG3306A tri-state
 */
static int silg_ts_bus_ctrl(struct dvb_frontend* fe, int acquire)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod;
	int ret = 0;

	silg_printk("%s() acquire=%d\n", __func__, acquire);

	if (acquire) {
		if (priv->dmid == _SI_) {
			/* turn off demod[_LG_] */
			if (priv->lg_demod_enable && priv->demod_bus_state[_LG_] != 0 ) {
				demod = priv->demod[_LG_];
				silg_printk("%s() disabling LG3306A output bus\n", __func__);
				ret = demod->ops.ts_bus_ctrl(demod, 0);
				if (ret==0) priv->demod_bus_state[_LG_] = 0;
			}
			/* turn on demod[_SI_] */
			if (priv->si_demod_enable && priv->demod_bus_state[_SI_] != 1 ) {
				demod = priv->demod[_SI_];
				silg_printk("%s() enabling Si2168B output bus\n", __func__);
				ret = demod->ops.ts_bus_ctrl(demod, 1);
				if (ret==0) priv->demod_bus_state[_SI_] = 1;
			}
		} else { /* priv->dmid != _SI_ */
			/* turn off demod[_SI_] */
			if (priv->si_demod_enable && priv->demod_bus_state[_SI_] != 0 ) {
				demod = priv->demod[_SI_];
				silg_printk("%s() disabling Si2168B output bus\n", __func__);
				ret = demod->ops.ts_bus_ctrl(demod, 0);
				if (ret==0) priv->demod_bus_state[_SI_] = 0;
			}
			/* turn on demod[_LG_] */
			if (priv->lg_demod_enable && priv->demod_bus_state[_LG_] != 1 ) {
				demod = priv->demod[_LG_];
				silg_printk("%s() enabling LG3306A output bus\n", __func__);
				ret = demod->ops.ts_bus_ctrl(demod, 1);
				if (ret==0) priv->demod_bus_state[_LG_] = 1;
			}
		}
	} else { /* !acquire */
		silg_printk("%s() setting all demod output buses to tri-state\n", __func__);
		if (priv->si_demod_enable && priv->demod_bus_state[_SI_] != 0) {
			demod = priv->demod[_SI_];
			ret = demod->ops.ts_bus_ctrl(demod, 0);
			if (ret==0) priv->demod_bus_state[_SI_] = 0;
		}
		if (ret == 0 && priv->lg_demod_enable && priv->demod_bus_state[_LG_] != 0) {
			demod = priv->demod[_LG_];
			ret = demod->ops.ts_bus_ctrl(demod, 0);
			if (ret==0) priv->demod_bus_state[_LG_] = 0;
		}
	}

	return ret;
}

static int silg_init(struct dvb_frontend *fe)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod;

	silg_printk("((((((((((((((((((((((((((((((((((((((((((((((((((((((((((\n");
	silg_printk("(((                       %s()                  )))\n", __func__);
	silg_printk("))))))))))))))))))))))))))))))))))))))))))))))))))))))))))\n");

	if (priv->si_demod_enable) {
		demod = priv->demod[_SI_];
		if (!demod) {
			silg_printk("%s(): Silicon Labs demod configured but not available\n", __func__);
			goto err;
		}
		if (fe->tuner_priv) {
			demod->tuner_priv = fe->tuner_priv;
			memcpy(&demod->ops.tuner_ops, &fe->ops.tuner_ops, sizeof(struct dvb_tuner_ops));
		}
		if (demod->ops.init(demod)) {
			silg_printk("%s(): initializing Silicon Labs demod failed\n", __func__);
			goto err;
		}
		/* reconfigure the ts bus tri-state control after init */
		switch (priv->demod_bus_state[_SI_]) {
		case 0: demod->ops.ts_bus_ctrl(demod, 0); break;
		case 1: demod->ops.ts_bus_ctrl(demod, 1); break;
		}
	}
	silg_printk("%s(): initializing Silicon Labs demod succeeded\n", __func__);

	if (priv->lg_demod_enable) {
		demod = priv->demod[_LG_];
		if (!demod) {
			silg_printk("%s(): LG demod configured but not available\n", __func__);
			goto err;
		}
		if (fe->tuner_priv) {
			demod->tuner_priv = fe->tuner_priv;
			memcpy(&demod->ops.tuner_ops, &fe->ops.tuner_ops, sizeof(struct dvb_tuner_ops));
		}
		if (demod->ops.init(demod)) {
			silg_printk("%s(): initializing LG demod failed\n", __func__);
			goto err;
		}
		/* reconfigure the ts bus tri-state control after init */
		switch (priv->demod_bus_state[_LG_]) {
		case 0: demod->ops.ts_bus_ctrl(demod, 0); break;
		case 1: demod->ops.ts_bus_ctrl(demod, 1); break;
		default:
			silg_printk("%s(): lg demod not reconfigured (demod_bus_state=%d)\n", __func__, priv->demod_bus_state[_LG_]);
			break;
		}
	}
	silg_printk("%s(): initializing LG demod succeeded\n", __func__);

	return 0;

err:
	printk(KERN_ERR "%s(): failed\n", __func__);
	return -ENODEV;
}

static int silg_read_status(struct dvb_frontend *fe, fe_status_t *status)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod = priv->demod[priv->dmid];

	return demod ? demod->ops.read_status(demod, status) : -ENODEV;
}

static int silg_read_signal_strength(struct dvb_frontend *fe, u16 *rssi)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod = priv->demod[priv->dmid];

	return demod ? demod->ops.read_signal_strength(demod, rssi) : -ENODEV;
}

static int silg_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod = priv->demod[priv->dmid];

	return demod ? demod->ops.read_ber(demod, ber) : -ENODEV;
}

static int silg_read_snr(struct dvb_frontend *fe, u16 *cnr)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod = priv->demod[priv->dmid];

	return demod ? demod->ops.read_snr(demod, cnr) : -ENODEV;
}

static int silg_read_ucblocks(struct dvb_frontend *fe, u32 *uncorrs)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod = priv->demod[priv->dmid];

	return demod ? demod->ops.read_ucblocks(demod, uncorrs) : -ENODEV;
}

static int select_demod(struct dvb_frontend *fe, fe_delivery_system_t delsys)
{
	struct silg_priv *priv = fe->demodulator_priv;
	u32    dmid = priv->dmid;

	switch (delsys) {
	case SYS_ATSC:
	case SYS_DVBC_ANNEX_B:
		dmid = _LG_; /* LG demod */
		break;
	case SYS_DVBT:
	case SYS_DVBT2:
	case SYS_DVBC_ANNEX_A:
		dmid = _SI_; /* SiLabs demod */
		break;
	default:
		silg_printk("%s : ERROR: delivery system %s not supported\n", __func__, delsys_name(delsys));
		return -EINVAL;
	}
	silg_printk("%s(): changing delivery system from %s to %s\n", __func__,
			delsys_name(priv->delivery_system), delsys_name(delsys));

	priv->delivery_system = delsys;

	if (dmid != priv->dmid) {
		silg_printk("%s(): changing demod from %s to %s\n", __func__,
				priv->dmid ? "LG" : "SI", dmid ? "LG" : "SI");
		priv->dmid = dmid;
	}
	return 0;
}

static int silg_set_frontend(struct dvb_frontend *fe)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod;
	struct dtv_frontend_properties *p = &fe->dtv_property_cache;
	int ret = 0;

	silg_printk("%s(): FE_SET_FRONTEND delsys=%s\n", __func__, delsys_name(p->delivery_system));

	if (p->delivery_system != priv->delivery_system) {
		ret = select_demod(fe, p->delivery_system);
		if (ret) {
			return ret;
		}
	}

	demod = priv->demod[priv->dmid];
	if (!demod) {
		silg_printk("%s(): ERROR: no demod for index=%u\n", __func__, priv->dmid);
		return -ENODEV;
	}

	if (priv->demod_bus_state[priv->dmid] != 1) { /* ts bus of the demod already enabled? */
		ret = silg_ts_bus_ctrl(fe, 1); /* enable ts bus of the selected demod */
		if (ret) {
			return ret;
		}
	}

	memcpy(&demod->dtv_property_cache, &fe->dtv_property_cache, sizeof(struct dtv_frontend_properties));
	return demod->ops.set_frontend(demod);
}

static int silg_get_frontend(struct dvb_frontend *fe)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod = priv->demod[priv->dmid];
	int ret = 0;

	if (!demod)
		return -ENODEV;

	ret = demod->ops.get_frontend(demod);
	if (ret) {
		return ret;
	}
	memcpy(&fe->dtv_property_cache, &demod->dtv_property_cache, sizeof(struct dtv_frontend_properties));
	return ret;
}

static int silg_sleep(struct dvb_frontend *fe)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod;
	int ret = 0;

	if (priv->si_demod_enable) {
		demod = priv->demod[_SI_];
		if(!demod){
			silg_printk("%s : ERROR: no demod for index=%d\n", __func__, _SI_);
			return -ENODEV;
		}
		ret = demod->ops.sleep(demod);
	}
	if (priv->lg_demod_enable) {
		demod = priv->demod[_LG_];
		if(!demod){
			silg_printk("%s : ERROR: no demod for index=%d\n", __func__, _LG_);
			return -ENODEV;
		}
		ret |= demod->ops.sleep(demod);
	}
	return ret;
}

static void silg_release(struct dvb_frontend *fe)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod;

	silg_printk("%s\n", __func__);

	if (priv->si_demod_enable) {
		demod = priv->demod[_SI_];
		if(demod){
			demod->ops.release(demod);
			symbol_put_addr((void*)demod->ops.release);
		} else {
			silg_printk("%s : ERROR: no demod for index=%d\n", __func__, _SI_);
		}
	}
	if (priv->lg_demod_enable) {
		demod = priv->demod[_LG_];
		if(demod){
			demod->ops.release(demod);
			symbol_put_addr((void*)demod->ops.release);
		} else {
			silg_printk("%s : ERROR: no demod for index=%d\n", __func__, _LG_);
		}
	}
	if (priv)
		kfree(priv);
}

static int silg_get_tune_settings(struct dvb_frontend *fe,
				      struct dvb_frontend_tune_settings
					*fe_tune_settings)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod = priv->demod[priv->dmid];

	return demod ? demod->ops.get_tune_settings(demod, fe_tune_settings) : -ENODEV;
}

#ifdef CUSTOM_TUNING_ALGO
static int silg_tune(struct dvb_frontend *fe, bool re_tune, unsigned int mode_flags, unsigned int *delay, fe_status_t *status)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod;
	struct dtv_frontend_properties *p = &fe->dtv_property_cache;
	int ret = 0;

	silg_printk("%s(): delsys=%s\n", __func__, delsys_name(p->delivery_system));

	if (p->delivery_system != priv->delivery_system) {
		ret = select_demod(fe, p->delivery_system);
		if (ret) {
			return ret;
		}
	}

	demod = priv->demod[priv->dmid];
	if(!demod) {
		silg_printk("%s : ERROR: no demod for index=%u\n", __func__, priv->dmid);
		return -ENODEV;
	}

	if (priv->demod_bus_state[priv->dmid] != 1) { /* ts bus of the demod already enabled? */
		ret = silg_ts_bus_ctrl(fe, 1); /* enable ts bus of the selected demod */
		if (ret) {
			return ret;
		}
	}

	memcpy(&demod->dtv_property_cache, &fe->dtv_property_cache, sizeof(struct dtv_frontend_properties));
	return demod ? demod->ops.tune(demod, re_tune, mode_flags, delay, status) : -ENODEV;
}

static enum dvbfe_algo silg_get_frontend_algo(struct dvb_frontend *fe)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod = priv->demod[priv->dmid];

	return demod ? demod->ops.get_frontend_algo(demod) : -ENODEV;
}

static enum dvbfe_search silg_search(struct dvb_frontend *fe)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod;
	struct dtv_frontend_properties *p = &fe->dtv_property_cache;
	int ret = 0;

	silg_printk("%s(): delsys=%s\n", __func__, delsys_name(p->delivery_system));

	if (p->delivery_system != priv->delivery_system) {
		ret = select_demod(fe, p->delivery_system);
		if (ret) {
			return ret;
		}
	}

	demod = priv->demod[priv->dmid];
	if(!demod) {
		silg_printk("%s : ERROR: no demod for index=%u\n", __func__, priv->dmid);
		return -ENODEV;
	}

	if (priv->demod_bus_state[priv->dmid] != 1) { /* ts bus of the demod already enabled? */
		ret = silg_ts_bus_ctrl(fe, 1); /* enable ts bus of the selected demod */
		if (ret) {
			return ret;
		}
	}

	memcpy(&demod->dtv_property_cache, &fe->dtv_property_cache, sizeof(struct dtv_frontend_properties));
	return demod->ops.search(demod);
}
#endif /* CUSTOM_TUNING_ALGO */

#ifdef FE_READ_STREAM_IDS
static int silg_read_stream_ids(struct dvb_frontend *fe, struct dvb_stream_ids* ids)
{
	struct silg_priv *priv = fe->demodulator_priv;
	struct dvb_frontend *demod = priv->demod[priv->dmid];

	return demod ? demod->ops.read_stream_ids(demod, ids) : -ENODEV;
}
#endif /* FE_READ_STREAM_IDS */

static const struct dvb_frontend_ops silg_ops = {
		.delsys = { SYS_DVBC_ANNEX_B, SYS_ATSC, SYS_DVBT, SYS_DVBT2, SYS_DVBC_ANNEX_A },
		.info = {
				.name = "SILG DVB-T/T2/C ATSC",
				.frequency_stepsize = 62500,
				.frequency_min = 48000000,
				.frequency_max = 870000000,
				.symbol_rate_min = 870000,
				.symbol_rate_max = 7501000,
				.caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3
				| FE_CAN_FEC_3_4 | FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8
				| FE_CAN_FEC_AUTO | FE_CAN_QPSK | FE_CAN_QAM_16 | FE_CAN_QAM_32
				| FE_CAN_QAM_64 | FE_CAN_QAM_128 | FE_CAN_QAM_256
				| FE_CAN_QAM_AUTO | FE_CAN_TRANSMISSION_MODE_AUTO
				| FE_CAN_GUARD_INTERVAL_AUTO | FE_CAN_HIERARCHY_AUTO
				| FE_CAN_MULTISTREAM | FE_CAN_2G_MODULATION | FE_CAN_MUTE_TS
				| FE_CAN_8VSB
		},

		.release = silg_release,
		/*.release_sec,*/

		.init = silg_init,
		.sleep = silg_sleep,

		/*.write,*/

		/* these two are only used for the swzigzag code */
		.set_frontend = silg_set_frontend,
		.get_tune_settings = silg_get_tune_settings,

		.get_frontend = silg_get_frontend,

		.read_status = silg_read_status,
		.read_ber = silg_read_ber,
		.read_signal_strength = silg_read_signal_strength,
		.read_snr = silg_read_snr,
		.read_ucblocks = silg_read_ucblocks,

		.i2c_gate_ctrl = silg_i2c_gate_ctrl,
		.ts_bus_ctrl = silg_ts_bus_ctrl,
		/* .set_lna, */

#ifdef CUSTOM_TUNING_ALGO
		/* if this is set, it overrides the default swzigzag */
		.tune = silg_tune,
		/* get frontend tuning algorithm from the module */
		.get_frontend_algo = silg_get_frontend_algo,

		/* These callbacks are for devices that implement their own
		 * tuning algorithms, rather than a simple swzigzag
		 */
		.search = silg_search,
#endif

		/* Allow the frontend to validate incoming properties */
		/*.set_property, */
		/*.get_property, */
#ifdef FE_READ_STREAM_IDS
		.read_stream_ids = silg_read_stream_ids,
#endif
};

static int ops_incomplete(struct dvb_frontend *demod)
{
	int ret = 0;
	if (!demod){
		silg_printk("ERROR: demod==NULL\n"); return -1;
	}
	if (!demod->ops.release){
		silg_printk("ERROR: ops.release missing\n"); ret = -1;
	}
	if (!demod->ops.sleep){
		silg_printk("ERROR: ops.sleep missing\n"); ret = -1;
	}
	if (!demod->ops.sleep){
		silg_printk("ERROR: ops.sleep missing\n"); ret = -1;
	}
	if (!demod->ops.set_frontend){
		silg_printk("ERROR: ops.set_frontend missing\n"); ret = -1;
	}
	if (!demod->ops.get_tune_settings){
		silg_printk("ERROR: ops.get_tune_settings missing\n"); ret = -1;
	}
	if (!demod->ops.get_frontend){
		silg_printk("ERROR: ops.get_frontend missing\n"); ret = -1;
	}
	if (!demod->ops.read_status){
		silg_printk("ERROR: ops.read_status missing\n"); ret = -1;
	}
	if (!demod->ops.read_ber){
		silg_printk("ERROR: ops.read_ber missing\n"); ret = -1;
	}
	if (!demod->ops.read_signal_strength){
		silg_printk("ERROR: ops.read_signal_strength missing\n"); ret = -1;
	}
	if (!demod->ops.read_snr){
		silg_printk("ERROR: ops.read_snr missing\n"); ret = -1;
	}
	if (!demod->ops.read_ucblocks){
		silg_printk("ERROR: ops.read_ucblocks missing\n"); ret = -1;
	}
	if (!demod->ops.i2c_gate_ctrl){
		silg_printk("ERROR: ops.i2c_gate_ctrl missing\n"); ret = -1;
	}
	if (!demod->ops.ts_bus_ctrl){
		silg_printk("ERROR: ops.ts_bus_ctrl missing\n"); ret = -1;
	}

#ifdef CUSTOM_TUNING_ALGO
	if (!demod->ops.tune){
		silg_printk("ERROR: ops.tune missing\n"); ret = -1;
	}
	if (!demod->ops.get_frontend_algo){
		silg_printk("ERROR: ops.get_frontend_algo missing\n"); ret = -1;
	}
	if (!demod->ops.search){
		silg_printk("ERROR: ops.search missing\n"); ret = -1;
	}
#endif

#ifdef FE_READ_STREAM_IDS
	if (!demod->ops.read_stream_ids){
		silg_printk("ERROR: ops.read_stream_ids missing\n"); ret = -1;
	}
#endif
	return ret;
}

struct dvb_frontend *silg_attach(const struct silg_config *config, struct i2c_adapter *i2c)
{
	struct silg_priv *priv = NULL;

	silg_printk("%s()\n", __func__);

	if (!config) {
		silg_printk("ERROR: configuration missing\n");
		goto error;
	}

	if (config->si_demod_enable == 0 && config->lg_demod_enable == 0) {
		silg_printk("%s(): no demodulator configured.\n", __func__);
		goto error;
	}

	/* allocate memory */
	priv = kzalloc(sizeof(struct silg_priv), GFP_KERNEL);
	if (priv == NULL) {
		silg_printk("%s(): kzalloc() failed.\n", __func__);
		goto error;
	}

	priv->si_demod_enable = config->si_demod_enable;
	if (priv->si_demod_enable) {
		priv->si2168b_cfg.demod_address = config->si_i2c_addr;
		priv->si2168b_cfg.min_delay_ms = config->min_delay_ms;
		priv->si2168b_cfg.ts_bus_mode = config->ts_bus_mode;
		priv->si2168b_cfg.ts_clock_mode = config->ts_clock_mode;
		priv->si2168b_cfg.clk_gapped_en = config->clk_gapped_en;
		priv->si2168b_cfg.ts_par_clk_invert = config->ts_par_clk_invert;
		priv->si2168b_cfg.ts_par_clk_shift = config->ts_par_clk_shift;
		priv->si2168b_cfg.fef_mode = config->fef_mode;
		priv->si2168b_cfg.fef_pin = config->fef_pin;
		priv->si2168b_cfg.fef_level = config->fef_level;
		priv->si2168b_cfg.indirect_i2c_connection = config->indirect_i2c_connection;
		priv->si2168b_cfg.start_ctrl = config->start_ctrl;

		priv->demod[_SI_] = dvb_attach(si2168b_attach, &priv->si2168b_cfg, i2c);
		if (!priv->demod[_SI_]) {
			silg_printk("%s(): ERROR: attaching module si2168b failed.\n", __func__);
			kfree(priv);
			goto error;
		}
		if (ops_incomplete(priv->demod[_SI_])) {
			silg_printk("%s(): ERROR: interface of si2168b is incomplete.\n", __func__);
			kfree(priv);
			goto error;
		}
		priv->demod_bus_state[_SI_] = -1; /* undefined */
		pr_info("%s(): attached si2168b\n", __func__);
	}

	priv->lg_demod_enable = config->si_demod_enable;
	if (priv->lg_demod_enable) {
		priv->lgdt3306a_cfg.i2c_addr           = config->lg_i2c_addr;
		priv->lgdt3306a_cfg.mpeg_mode          = config->mpeg_mode;
		priv->lgdt3306a_cfg.tpclk_edge         = config->tpclk_edge;
		priv->lgdt3306a_cfg.tpvalid_polarity   = config->tpvalid_polarity;
		priv->lgdt3306a_cfg.deny_i2c_rptr      = config->deny_i2c_rptr;
		priv->lgdt3306a_cfg.spectral_inversion = config->spectral_inversion;
		priv->lgdt3306a_cfg.qam_if_khz         = config->qam_if_khz;
		priv->lgdt3306a_cfg.vsb_if_khz         = config->vsb_if_khz;
		priv->lgdt3306a_cfg.xtalMHz            = config->xtalMHz;
		priv->demod[_LG_] = dvb_attach(lgdt3306a_attach, &priv->lgdt3306a_cfg, i2c);
		if (!priv->demod[_LG_]) {
			silg_printk("%s(): ERROR: attaching module lgdt3306a failed.\n", __func__);
			kfree(priv);
			goto error;
		}
		if (ops_incomplete(priv->demod[_LG_])) {
			silg_printk("%s(): ERROR: interface of lgdt3306a is incomplete.\n", __func__);
			kfree(priv);
			goto error;
		}
		priv->demod_bus_state[_LG_] = -1; /* undefined */
		pr_info("%s(): attached lgdt3306a\n", __func__);
	}

	priv->dmid = (priv->si_demod_enable) ? _SI_ : _LG_; /* set default demod */
	priv->delivery_system = SYS_UNDEFINED;

	/* create dvb_frontend */
	memcpy(&priv->frontend.ops, &silg_ops, sizeof(struct dvb_frontend_ops));
	priv->frontend.demodulator_priv = priv;

	return &priv->frontend;

error:
	return NULL;
}
EXPORT_SYMBOL(silg_attach);

MODULE_PARM_DESC(debug, "Turn on/off frontend debugging (default:off).");

MODULE_DESCRIPTION("SILG SiLabs + LG Dual Demodulator Driver");
MODULE_AUTHOR("Henning Garbers <hgarbers@pctvsystems.com>");
/* MODULE_LICENSE("Proprietary"); */
/* GPL discussion for silg not finished. Set to GPL for internal usage only. */
/* The module uses GPL functions and is rejected by the kernel build if the */
/* license is set to 'Proprietary'. */
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
