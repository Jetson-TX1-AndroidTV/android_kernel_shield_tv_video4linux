/*
 * silabs_tercab.c - Silicon Labs terrestrial/cable hybrid tuner driver
 * for the tuners Si2157 and Si2177
 *
 * (C) Copyright 2014, PCTV Systems S.à r.l
 * Henning Garbers <hgarbers@pctvsystems.com>
 *
 */

#include  <linux/delay.h>
#include <linux/videodev2.h>
#include "tuner-i2c.h"
#include "silabs_tercab_priv.h"

static int silabs_tercab_debug = 0;
module_param_named(debug, silabs_tercab_debug, int, 0644);

#ifdef SiTRACES
static int silabs_tercab_trace = 0;
module_param_named(sitrace, silabs_tercab_trace, int, 0644);
#endif

static DEFINE_MUTEX(silabs_tercab_list_mutex);
static LIST_HEAD(hybrid_tuner_instance_list);

/*---------------------------------------------------------------------*/

#define DBG_INFO 1

__attribute__((format(printf, 4, 5)))
int _silabs_tercab_printk(struct silabs_tercab_priv *state, const char *level,
		const char *func, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int rtn;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	if (state)
		rtn = printk("%s%s: [%d-%04x] %pV",
				level, func, i2c_adapter_id(state->i2c_props.adap),
				state->i2c_props.addr,
				&vaf);
	else
		rtn = printk("%s%s: %pV", level, func, &vaf);

	va_end(args);

	return rtn;
}

#define silabs_tercab_printk(st, lvl, fmt, arg...)			\
		_silabs_tercab_printk(st, lvl, __func__, fmt, ##arg)

#define si2158_dprintk(st, lvl, fmt, arg...)			\
		do {								\
			if (silabs_tercab_debug & lvl)				\
			silabs_tercab_printk(st, KERN_DEBUG, fmt, ##arg);		\
		} while (0)

#define silabs_tercab_info(fmt, arg...) silabs_tercab_printk(priv, KERN_INFO, fmt, ##arg)
#define silabs_tercab_warn(fmt, arg...) silabs_tercab_printk(priv, KERN_WARNING, fmt, ##arg)
#define silabs_tercab_err(fmt, arg...)  silabs_tercab_printk(priv, KERN_ERR, fmt, ##arg)
#define silabs_tercab_dbg(fmt, arg...)  si2158_dprintk(priv, DBG_INFO, fmt, ##arg)

#define silabs_tercab_fail(ret)							     \
		({									     \
			int __ret;							     \
			__ret = (ret != NO_SILABS_TERCAB_ERROR);						     \
			if (__ret)							     \
			silabs_tercab_printk(priv, KERN_ERR,				     \
					"error %d on line %d\n", ret, __LINE__);	     \
					__ret;								     \
		})

#ifdef SiTRACES
#define SiTRACES_BUFFER_LENGTH  100000
#define SiTRACES_NAMESIZE           30
#define SiTRACES_FUNCTION_NAMESIZE  30

#define CUSTOM_PRINTF(args...) \
		do { \
			if (silabs_tercab_trace) \
			printk(KERN_INFO "Silabs tuner: " args); \
		} while (0)

typedef enum TYPE_OF_OUTPUT {
	TRACE_NONE = 0,
	TRACE_STDOUT,
	TRACE_EXTERN_FILE,
	TRACE_MEMORY
} TYPE_OF_OUTPUT;

static TYPE_OF_OUTPUT trace_output_type;

static char trace_timer[50];

static u8 trace_init_done         = 0;
static u8 trace_suspend           = 0;
static u8 trace_skip_info         = 0;
static u8 trace_config_lines      = 0;
static u8 trace_config_files      = 0;
static u8 trace_config_functions  = 0;
static u8 trace_config_time       = 0;
static int trace_linenumber       = 0;
static int trace_count            = 0;

static char trace_timer[50];
static char trace_elapsed_time[20];
static char trace_source_file[100];
static char trace_source_function[SiTRACES_FUNCTION_NAMESIZE+1];

static void SiTraceFunction(const char *name, int trace_linenumber, const char *func, const char *fmt, ...);
#define SiTRACE(...) SiTraceFunction(__FILE__, __LINE__, __FUNCTION__ ,__VA_ARGS__)

/************************************************************************************************************************
      traceElapsedTime function
      Use:        SiTRACES time formatting function.
                  It allows the user to know when the trace has been treated.
                  It is used to insert the time before the trace when -time 'on'.
      Returns:    text containing the execution time in HH:MM:SS.ms format.
      Porting:    Not compiled if SiTRACES is not defined in Silabs_L0_ctx.h.
************************************************************************************************************************/
static char *traceElapsedTime(void)
{
	unsigned int timeElapsed, ms, sec, min, hours;
	timeElapsed = jiffies_to_msecs(jiffies);
	ms=timeElapsed%1000;
	timeElapsed=timeElapsed/1000;
	sec=timeElapsed%60;
	timeElapsed=timeElapsed/60;
	min=timeElapsed%60;
	timeElapsed=timeElapsed/60;
	hours=timeElapsed%60;
	sprintf(trace_elapsed_time,"%02d:%02d:%02d.%03d ",hours,min,sec,ms);
	return trace_elapsed_time;
}

/************************************************************************************************************************
      traceToStdout function
      Use:        SiTRACES stdout display function.
                  It displays the current trace in the command window.
                  It adds file name, line number,function name and time if selected.
      Parameter:  trace
      Returns:    void
************************************************************************************************************************/
static void traceToStdout(char* trace)
{
	if (!trace_skip_info) {
		if (trace_config_files    ) { CUSTOM_PRINTF("%-40s ", trace_source_file    ); }
		if (trace_config_lines    ) { CUSTOM_PRINTF("%5d "  , trace_linenumber     ); }
		if (trace_config_functions) { CUSTOM_PRINTF("%-30s ", trace_source_function); }
	}
	if (trace_config_time     ) { CUSTOM_PRINTF("%s ",    traceElapsedTime()   ); }
	CUSTOM_PRINTF("%s",     trace);
}

/************************************************************************************************************************
  traceToDestination function
  Use:        switch the trace in the selected output mode.
  Comment:    In verbose mode, the trace is always displayed in stdout.
  Parameter:  trace, the trace string
  Returns:    void
  Porting:    Not compiled if SiTRACES is not defined in Silabs_L0_ctx.h.
************************************************************************************************************************/
static void  traceToDestination(char* trace)
{
	int last;
	if (trace_suspend) {
		return;
	}
	/* If trace is a single CrLf, do not print the file/line/function info           */
	if (strcmp(trace,"\n")==0) {
		trace_skip_info = 1;
	}
	/* If file/line/function info printed, make sure there is a CrLf after each line */
	if (trace_config_files + trace_config_lines + trace_config_functions + trace_config_time) {
		last = (int)strlen(trace)-1;
		if (trace[last] != 0x0a) {
			sprintf(trace, "%s\n", trace);
		}
	}
	traceToStdout(trace);
	if (strcmp(trace,"\n")==0) {
		trace_skip_info = 0;
	}
	trace_count++;
}

/************************************************************************************************************************
  SiTraceFunction function
  Use:        SiTRACES trace formatting function.
              It formats the trace message with file name and line number and time if selected
              then saves it to the trace output.
  Parameter:  name    the file name where the trace is written.
  Parameter:  number  the line number where the trace is written.
  Parameter:  fmt     string content of trace message. Others arguments are sent thanks to the ellipse.
  Returns:    void
  Porting:    Not compiled if SiTRACES is not defined in Silabs_L0_ctx.h.
************************************************************************************************************************/
static void SiTraceFunction(const char *name, int number, const char *func, const char *fmt, ...)
{
	char        message[850];
	const char *pname;
	const char *pfunc;
	va_list     ap;
	/* print the line number in trace_linenumber */
	trace_linenumber = number;
	pname=name;

	/* print the file name in trace_source_file */
	if(strlen(pname)>SiTRACES_NAMESIZE) {
		pname+=strlen(pname)-SiTRACES_NAMESIZE;
	}
	strncpy(trace_source_file,pname,SiTRACES_NAMESIZE);

	/*print the function name in trace_source_function */
	pfunc=func;
	sprintf(trace_source_function,"%s","");
	if(strlen(pfunc)>SiTRACES_FUNCTION_NAMESIZE) {
		pfunc+=(strlen(pfunc)-SiTRACES_FUNCTION_NAMESIZE)+2;
		strcpy(trace_source_function,"..");
	}
	strncat(trace_source_function,pfunc,SiTRACES_FUNCTION_NAMESIZE-2);

	va_start(ap, fmt);
	vsnprintf(message,900,fmt,ap);
	traceToDestination(message);
	va_end(ap);
	return;
}

/************************************************************************************************************************
  SiTraceDefaultConfiguration function
  Use:        SiTRACES initialization function.
              It is called on the first call to L0_Init (only once).
              It defines the default output and inserts date and time in the default file.
  Returns:    void
  Porting:    Not compiled if SiTRACES is not defined in Silabs_L0_ctx.h.
************************************************************************************************************************/
static void SiTraceDefaultConfiguration(void)
{
	if (trace_init_done) return;
	trace_output_type=TRACE_STDOUT;
	trace_init_done=1;
	sprintf(trace_timer, "time");
}

/************************************************************************************************************************
  silabs_tercab_infos function
  Use:        software information function
              Used to retrieve information about the compilation
  Parameter:  front_end, a pointer to the Si2158_L2_Context context to be initialized
  Parameter:  infoString, a text buffer to be filled with teh information. It must be initialized by the caller.
  Return:     the length of the information string
 ************************************************************************************************************************/
static int silabs_tercab_infos(struct silabs_tercab_priv *priv, char *infoString_UNUSED)
{
	if (infoString_UNUSED == NULL)
		return 0;
	if (!priv) {
		SiTRACE("Si2158 front-end not initialized yet. Call silabs_tercab_sw_init first!\n");
		return strlen(infoString_UNUSED);
	}

	SiTRACE("\n");
	SiTRACE("--------------------------------------\n");
	SiTRACE("Terrestrial/Cable Tuner Si21%u at 0x%02x\n", priv->tuner.part, priv->tuner.i2c_addr);
	SiTRACE("--------------------------------------\n");
	return strlen(infoString_UNUSED);
}

static inline void SiTracesSuspend(void)
{
	trace_suspend = 1;
}
static inline void SiTracesResume(void)
{
	trace_suspend = 0;
}
#else /* SiTRACES */
#define SiTRACE(...)               /* empty */
#define SiTracesSuspend()          /* empty */
#define SiTracesResume()           /* empty */
#endif /* SiTRACES */

/************************************************************************************************************************
  i2c_read_bytes function

  Parameters: iNbBytes, the number of bytes to read.
              *pbtDataBuffer, a pointer to a buffer used to store the bytes.
  Returns:    the number of bytes read.
************************************************************************************************************************/
static u16 i2c_read_bytes(struct i2c_adapter *i2c_adap, u8 i2c_addr, u16 iNbBytes, u8 *pucDataBuffer)
{
	struct silabs_tercab_priv *priv = NULL; /* for debug output only */
	u16 nbReadBytes = 0;
	struct i2c_msg msg = {
			.addr  = i2c_addr,
			.flags = I2C_M_RD,
			.buf   = pucDataBuffer,
			.len   = iNbBytes,
	};

	if (i2c_adap == NULL) {
		silabs_tercab_err("%s(): FATAL ERROR: i2c_adap is undefined!\n", __func__);
		return 0;
	}

	if (i2c_transfer(i2c_adap, &msg, 1) == 1) {
		nbReadBytes = iNbBytes;
	} else {
		silabs_tercab_err("%s(): i2c transfer failed\n", __func__);
	}

	return nbReadBytes;
}

/************************************************************************************************************************
  i2c_write_bytes function

  Parameters: iNbBytes, the number of bytes to write.
              *pbtDataBuffer, a pointer to a buffer containing the bytes to write.
  Returns:    the number of written bytes.
************************************************************************************************************************/
static u16 i2c_write_bytes(struct i2c_adapter *i2c_adap, u8 i2c_addr, u16 iNbBytes, u8 *pucDataBuffer)
{
	u16 nbWrittenBytes = 0;
	int write_error = 0;
	struct i2c_msg msg = {
			.addr  = i2c_addr,
			.flags = 0,
			.buf   = pucDataBuffer,
			.len   = iNbBytes
	};
	struct silabs_tercab_priv *priv = NULL; /* for debug output only */

	if (i2c_adap == NULL) {
		silabs_tercab_err("%s(): FATAL ERROR: i2c_adap is undefined!\n", __func__);
		return 0;
	}

	nbWrittenBytes = 0;
	write_error    = 0;
	if (iNbBytes <= 64) {
		if (i2c_transfer(i2c_adap, &msg, 1) == 1) {
			nbWrittenBytes = iNbBytes;
		} else {
			silabs_tercab_err("%s(): i2c transfer failed\n", __func__);
			write_error++;
		}
	} else {
		silabs_tercab_err("%s(): numbers of bytes exceeds limit of 64\n", __func__);
		write_error++;
	}

	if (write_error) return 0;
	return nbWrittenBytes;
}

static u8 silabs_tercab_poll_cts(struct i2c_adapter *i2c_adap, u8 i2c_addr)
{
	u8 rspByteBuffer[1];
	struct silabs_tercab_priv *priv = NULL; /* for debug output only */
	unsigned int start_time = jiffies_to_msecs(jiffies);

	do {
		if (i2c_read_bytes(i2c_adap, i2c_addr, 1, rspByteBuffer) != 1) {
			silabs_tercab_err("%s() ERROR reading byte 0!\n", __func__);
			return ERROR_SILABS_TERCAB_POLLING_CTS;
		}
		/* return OK if CTS set */
		if (rspByteBuffer[0] & 0x80) {
			return NO_SILABS_TERCAB_ERROR;
		}
		msleep(10); /* FGR - pause a bit rather than just spinning on I2C */
	} while (jiffies_to_msecs(jiffies) - start_time <1000);/* wait a maximum of 1000ms */

	silabs_tercab_err("%s() ERROR CTS Timeout!\n", __func__);
	return ERROR_SILABS_TERCAB_CTS_TIMEOUT;
}

static u8 silabs_tercab_poll_response(struct i2c_adapter *i2c_adap, u8 i2c_addr, u16 nbBytes, u8 *pByteBuffer, silabs_tercab_status *status)
{
	struct silabs_tercab_priv *priv = NULL; /* for debug output only */
	unsigned int start_time = jiffies_to_msecs(jiffies);

	do {
		if (i2c_read_bytes(i2c_adap, i2c_addr, nbBytes, pByteBuffer) != nbBytes) {
			silabs_tercab_err("%s() ERROR reading byte 0!\n", __func__);
			return ERROR_SILABS_TERCAB_POLLING_RESPONSE;
		}
		/* return response err flag if CTS set */
		if (pByteBuffer[0] & 0x80)  {
			status->tunint = (pByteBuffer[0] >> 0) & 0x01;
			status->atvint = (pByteBuffer[0] >> 1) & 0x01;
			status->dtvint = (pByteBuffer[0] >> 2) & 0x01;
			status->err    = (pByteBuffer[0] >> 6) & 0x01;
			status->cts    = (pByteBuffer[0] >> 7) & 0x01;
			if (status->err) {
				silabs_tercab_info("ERROR flag is on!\n");
			}
			return (status->err ? ERROR_SILABS_TERCAB_ERR : NO_SILABS_TERCAB_ERROR);
		}
		msleep(10); /* pause a bit rather than just spinning on I2C */
	} while (jiffies_to_msecs(jiffies) - start_time <1000); /* wait a maximum of 1000ms */

	silabs_tercab_err("%s() ERROR CTS Timeout!\n", __func__);
	return ERROR_SILABS_TERCAB_CTS_TIMEOUT;
}

/***********************************************************************************************************************
  silabs_tercab_check_status function
  Use:        Status information function
              Used to retrieve the status byte
  Returns:    0 if no error
  Parameter:  error_code the error code.
 ***********************************************************************************************************************/
static u8 silabs_tercab_check_status(silabs_tercab_context *ctx)
{
	u8 rspByteBuffer[1];
	return silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, 1, rspByteBuffer, &ctx->status);
}

/***********************************************************************************************************************
  silabs_tercab_set_property function
  Use:        property set function
              Used to call L1_SET_PROPERTY with the property Id and data provided.
  Parameter: *ctx     the Si2158 context
  Parameter: prop     the property Id
  Parameter: data     the property bytes
  Returns:    0 if no error, an error code otherwise
 ***********************************************************************************************************************/
static u8 silabs_tercab_set_property(silabs_tercab_context *ctx, u16 prop, u16 data)
{
	u8 error_code = 0;
	u8 reserved = 0;
	u8 cmdByteBuffer[6];
	u8 rspByteBuffer[4];

	SiTRACE("%s(resv=0x%x prop=0x%x data=0x%x\n", __func__, reserved, prop, data);

	cmdByteBuffer[0] = Si2158_SET_PROPERTY_CMD;
	cmdByteBuffer[1] = (u8) ( ( reserved & Si2158_SET_PROPERTY_CMD_RESERVED_MASK ) << Si2158_SET_PROPERTY_CMD_RESERVED_LSB);
	cmdByteBuffer[2] = (u8) ( ( prop     & Si2158_SET_PROPERTY_CMD_PROP_MASK     ) << Si2158_SET_PROPERTY_CMD_PROP_LSB    );
	cmdByteBuffer[3] = (u8) ((( prop     & Si2158_SET_PROPERTY_CMD_PROP_MASK     ) << Si2158_SET_PROPERTY_CMD_PROP_LSB    ) >> 8);
	cmdByteBuffer[4] = (u8) ( ( data     & Si2158_SET_PROPERTY_CMD_DATA_MASK     ) << Si2158_SET_PROPERTY_CMD_DATA_LSB    );
	cmdByteBuffer[5] = (u8) ((( data     & Si2158_SET_PROPERTY_CMD_DATA_MASK     ) << Si2158_SET_PROPERTY_CMD_DATA_LSB    ) >> 8);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 6, cmdByteBuffer) != 6) {
		SiTRACE("Error writing SET_PROPERTY bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, 4, rspByteBuffer, &ctx->status);
	if (error_code) {
		SiTRACE("Error polling SET_PROPERTY response\n");
		return error_code;
	}

	return NO_SILABS_TERCAB_ERROR;
}

static u8 silabs_tercab_power_up(struct i2c_adapter *i2c_adap,
		u8   i2c_addr,
		u8   subcode,
		u8   clock_mode,
		u8   en_xout,
		u8   pd_ldo,
		u8   reserved2,
		u8   reserved3,
		u8   reserved4,
		u8   reserved5,
		u8   reserved6,
		u8   reserved7,
		u8   reset,
		u8   clock_freq,
		u8   reserved8,
		u8   func,
#ifndef __SI2158__
		u8   reserved9,
#endif
		u8   ctsien,
		u8   wake_up,
		silabs_tercab_status *status)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[15];
	u8 rspByteBuffer[1];
	struct silabs_tercab_priv *priv = NULL; /* for debug output only */
	//ctx->rsp.power_up.STATUS = ctx->status;

	silabs_tercab_info("%s(): clock mode=%u  en_xout=%u\n", __func__, clock_mode, en_xout);

	cmdByteBuffer[0]  = Si2158_POWER_UP_CMD;
	cmdByteBuffer[1]  = (u8) ( ( subcode    & Si2158_POWER_UP_CMD_SUBCODE_MASK    ) << Si2158_POWER_UP_CMD_SUBCODE_LSB   );
	cmdByteBuffer[2]  = (u8) ( ( clock_mode & Si2158_POWER_UP_CMD_CLOCK_MODE_MASK ) << Si2158_POWER_UP_CMD_CLOCK_MODE_LSB|
							   ( en_xout    & Si2158_POWER_UP_CMD_EN_XOUT_MASK    ) << Si2158_POWER_UP_CMD_EN_XOUT_LSB   );
	cmdByteBuffer[3]  = (u8) ( ( pd_ldo     & Si2158_POWER_UP_CMD_PD_LDO_MASK     ) << Si2158_POWER_UP_CMD_PD_LDO_LSB    );
	cmdByteBuffer[4]  = (u8) ( ( reserved2  & Si2158_POWER_UP_CMD_RESERVED2_MASK  ) << Si2158_POWER_UP_CMD_RESERVED2_LSB );
	cmdByteBuffer[5]  = (u8) ( ( reserved3  & Si2158_POWER_UP_CMD_RESERVED3_MASK  ) << Si2158_POWER_UP_CMD_RESERVED3_LSB );
	cmdByteBuffer[6]  = (u8) ( ( reserved4  & Si2158_POWER_UP_CMD_RESERVED4_MASK  ) << Si2158_POWER_UP_CMD_RESERVED4_LSB );
	cmdByteBuffer[7]  = (u8) ( ( reserved5  & Si2158_POWER_UP_CMD_RESERVED5_MASK  ) << Si2158_POWER_UP_CMD_RESERVED5_LSB );
	cmdByteBuffer[8]  = (u8) ( ( reserved6  & Si2158_POWER_UP_CMD_RESERVED6_MASK  ) << Si2158_POWER_UP_CMD_RESERVED6_LSB );
	cmdByteBuffer[9]  = (u8) ( ( reserved7  & Si2158_POWER_UP_CMD_RESERVED7_MASK  ) << Si2158_POWER_UP_CMD_RESERVED7_LSB );
	cmdByteBuffer[10] = (u8) ( ( reset      & Si2158_POWER_UP_CMD_RESET_MASK      ) << Si2158_POWER_UP_CMD_RESET_LSB     );
	cmdByteBuffer[11] = (u8) ( ( clock_freq & Si2158_POWER_UP_CMD_CLOCK_FREQ_MASK ) << Si2158_POWER_UP_CMD_CLOCK_FREQ_LSB);
	cmdByteBuffer[12] = (u8) ( ( reserved8  & Si2158_POWER_UP_CMD_RESERVED8_MASK  ) << Si2158_POWER_UP_CMD_RESERVED8_LSB );
	cmdByteBuffer[13] = (u8) ( ( func       & Si2158_POWER_UP_CMD_FUNC_MASK       ) << Si2158_POWER_UP_CMD_FUNC_LSB      |
							   ( reserved9  & Si2157_POWER_UP_CMD_RESERVED9_MASK  ) << Si2157_POWER_UP_CMD_RESERVED9_LSB |
							   ( ctsien     & Si2158_POWER_UP_CMD_CTSIEN_MASK     ) << Si2158_POWER_UP_CMD_CTSIEN_LSB    );
	cmdByteBuffer[14] = (u8) ( ( wake_up    & Si2158_POWER_UP_CMD_WAKE_UP_MASK    ) << Si2158_POWER_UP_CMD_WAKE_UP_LSB   );

	if (i2c_write_bytes(i2c_adap, i2c_addr, 15, cmdByteBuffer) != 15) {
		silabs_tercab_err("Error writing POWER_UP bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	msleep(10);

	error_code = silabs_tercab_poll_response(i2c_adap, i2c_addr, 1, rspByteBuffer, status);
	if (error_code) {
		silabs_tercab_err("Error polling POWER_UP response\n");
		return error_code;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/*---------------------------------------------------*/
/* Si2158_PART_INFO COMMAND                        */
/*---------------------------------------------------*/
static u8 si2158_part_info(struct i2c_adapter *i2c_adap, u8 i2c_addr, struct part_info *part_info)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[1];
	u8 rspByteBuffer[13];
	silabs_tercab_status status;
	struct silabs_tercab_priv *priv = NULL; /* for debug output only */
	//ctx->rsp.part_info.STATUS = ctx->status;

	silabs_tercab_info("%s()\n", __func__);

	cmdByteBuffer[0] = Si2158_PART_INFO_CMD;

	if (i2c_write_bytes(i2c_adap, i2c_addr, 1, cmdByteBuffer) != 1) {
		silabs_tercab_err("Error writing PART_INFO bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = silabs_tercab_poll_response(i2c_adap, i2c_addr, 13, rspByteBuffer, &status);
	if (error_code) {
		silabs_tercab_err("Error polling PART_INFO response\n");
		return error_code;
	}

	part_info->chiprev  = rspByteBuffer[1] & 0x0f;
	part_info->part     = rspByteBuffer[2];
	part_info->pmajor   = rspByteBuffer[3];
	part_info->pminor   = rspByteBuffer[4];
	part_info->pbuild   = rspByteBuffer[5];
	part_info->reserved = (( ( rspByteBuffer[6] | (rspByteBuffer[7]  << 8 )) >> 0 ) & 0xffff );
	part_info->serial   = (( ( rspByteBuffer[8] | (rspByteBuffer[9]  << 8 ) | (rspByteBuffer[10] << 16 ) | (rspByteBuffer[11] << 24 )) >> 0 ) & 0xffffffff );
	part_info->romid    = rspByteBuffer[12];

	return NO_SILABS_TERCAB_ERROR;
}

/***********************************************************************************************************************
  silabs_tercab_error_text function
  Use:        Error information function
              Used to retrieve a text based on an error code
  Returns:    the error text
  Parameter:  error_code the error code.
  Porting:    Useful for application development for debug purposes.
  Porting:    May not be required for the final application, can be removed if not used.
 ***********************************************************************************************************************/
static char* silabs_tercab_error_text(int error_code)
{
	switch (error_code) {
	case NO_SILABS_TERCAB_ERROR                     : return (char *)"No error";
	case ERROR_SILABS_TERCAB_ALLOCATING_CONTEXT     : return (char *)"Error while allocating Si2158 context";
	case ERROR_SILABS_TERCAB_PARAMETER_OUT_OF_RANGE : return (char *)"parameter(s) out of range";
	case ERROR_SILABS_TERCAB_SENDING_COMMAND        : return (char *)"Error while sending Si2158 command";
	case ERROR_SILABS_TERCAB_CTS_TIMEOUT            : return (char *)"CTS timeout";
	case ERROR_SILABS_TERCAB_ERR                    : return (char *)"Error (status 'err' bit 1)";
	case ERROR_SILABS_TERCAB_POLLING_CTS            : return (char *)"Error while polling CTS";
	case ERROR_SILABS_TERCAB_POLLING_RESPONSE       : return (char *)"Error while polling response";
	case ERROR_SILABS_TERCAB_LOADING_FIRMWARE       : return (char *)"Error while loading firmware";
	case ERROR_SILABS_TERCAB_LOADING_BOOTBLOCK      : return (char *)"Error while loading bootblock";
	case ERROR_SILABS_TERCAB_STARTING_FIRMWARE      : return (char *)"Error while starting firmware";
	case ERROR_SILABS_TERCAB_SW_RESET               : return (char *)"Error during software reset";
	case ERROR_SILABS_TERCAB_INCOMPATIBLE_PART      : return (char *)"Error Incompatible part";
#ifndef __SI2158__
	case ERROR_Si2157_UNKNOWN_COMMAND               : return (char *)"Error unknown command";
	case ERROR_Si2157_UNKNOWN_PROPERTY              : return (char *)"Error unknown property";
#endif
	/* _specific_error_text_string_insertion_start */
	case ERROR_SILABS_TERCAB_TUNINT_TIMEOUT         : return (char *)"Error TUNINT Timeout";
	case ERROR_SILABS_TERCAB_xTVINT_TIMEOUT         : return (char *)"Error xTVINT Timeout";
	case ERROR_SILABS_TERCAB_CRC_CHECK_ERROR        : return (char *)"Error CRC Check Error";
	/* _specific_error_text_string_insertion_point */
	default                                         : return (char *)"Unknown silabs_tercab error code";
	}
}

/***********************************************************************************************************************
  silabs_tercab_firmware_patch function
  Use:        Patch information function
              Used to send a number of bytes to the Si2158. Useful to download the firmware.
  Returns:    0 if no error
  Parameter:  error_code the error code.
  Porting:    Useful for application development for debug purposes.
  Porting:    May not be required for the final application, can be removed if not used.
 ***********************************************************************************************************************/
static u8 silabs_tercab_firmware_patch(silabs_tercab_context *ctx, u16 iNbBytes, u8 *pucDataBuffer)
{
	int res;
	u8 rspByteBuffer[1];

	SiTRACE("Si2158 Patch %d bytes\n",iNbBytes);

	res = i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, iNbBytes, pucDataBuffer);
	if (res!=iNbBytes) {
		SiTRACE("silabs_tercab_firmware_patch error writing bytes: %s\n", silabs_tercab_error_text(ERROR_SILABS_TERCAB_LOADING_FIRMWARE) );
		return ERROR_SILABS_TERCAB_LOADING_FIRMWARE;
	}

	res = silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, 1, rspByteBuffer, &ctx->status);
	if (res != NO_SILABS_TERCAB_ERROR) {
		SiTRACE("silabs_tercab_firmware_patch error 0x%02x polling response: %s\n", res, silabs_tercab_error_text(res) );
		return ERROR_SILABS_TERCAB_POLLING_RESPONSE;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/************************************************************************************************************************
  NAME: silabs_tercab_load_firmware_16
  DESCRIPTION: Load firmware from firmware_struct array in Si2158_Firmware_x_y_build_z.h file into Si2158
              Requires Si2158 to be in bootloader mode after PowerUp
  Programming Guide Reference:    Flowchart A.3 (Download FW PATCH flowchart)

  Parameter:  Si2158 Context (I2C address)
  Parameter:  pointer to firmware_struct array
  Parameter:  number of lines in firmware table array (size in bytes / firmware_struct)
  Returns:    Si2158/I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 ************************************************************************************************************************/
static int silabs_tercab_load_firmware_16(silabs_tercab_context *ctx, firmware_struct fw_table[], int nbLines)
{
	int return_code;
	int line;
	return_code = NO_SILABS_TERCAB_ERROR;

	SiTRACE ("silabs_tercab_load_firmware_16 nbLines=%d\n", nbLines);

	/* for each line in fw_table */
	for (line = 0; line < nbLines; line++)
	{
		if (fw_table[line].firmware_len > 0)  /* don't download if length is 0 , e.g. dummy firmware */
		{
			/* send firmware_len bytes (up to 16) to Si2158 */
			if ((return_code = silabs_tercab_firmware_patch(ctx, fw_table[line].firmware_len, fw_table[line].firmware_table)) != NO_SILABS_TERCAB_ERROR)
			{
				SiTRACE("silabs_tercab_load_firmware_16 error 0x%02x patching line %d: %s\n", return_code, line, silabs_tercab_error_text(return_code) );
				if (line == 0) {
					SiTRACE("The firmware is incompatible with the part!\n");
				}
				return ERROR_SILABS_TERCAB_LOADING_FIRMWARE;
			}
			if (line==3) {
				SiTracesSuspend();
			}
		}
	}
	SiTracesResume();
	SiTRACE ("silabs_tercab_load_firmware_16 complete...\n");
	return NO_SILABS_TERCAB_ERROR;
}

/************************************************************************************************************************
  NAME: silabs_tercab_load_firmware
  DESCRIPTON: Load firmware from FIRMWARE_TABLE array in Si2158_Firmware_x_y_build_z.h file into Si2158
              Requires Si2158 to be in bootloader mode after PowerUp
  Programming Guide Reference:    Flowchart A.3 (Download FW PATCH flowchart)

  Parameter:  Si2158 Context (I2C address)
  Parameter:  pointer to firmware table array
  Parameter:  number of lines in firmware table array (size in bytes / BYTES_PER_LINE)
  Returns:    Si2158/I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 ************************************************************************************************************************/
static int silabs_tercab_load_firmware(silabs_tercab_context *ctx, u8 fw_table[], int nbLines)
{
	int return_code;
	int line;
	return_code = NO_SILABS_TERCAB_ERROR;

	SiTRACE ("silabs_tercab_load_firmware nbLines=%d\n", nbLines);

	/* for each line in fw_table */
	for (line = 0; line < nbLines; line++)
	{
		/* send Si2158_BYTES_PER_LINE fw bytes to Si2158 */
		if ((return_code = silabs_tercab_firmware_patch(ctx, Si2158_BYTES_PER_LINE, fw_table + Si2158_BYTES_PER_LINE*line)) != NO_SILABS_TERCAB_ERROR)
		{
			SiTRACE("silabs_tercab_load_firmware error 0x%02x patching line %d: %s\n", return_code, line, silabs_tercab_error_text(return_code) );
			if (line == 0) {
				SiTRACE("The firmware is incompatible with the part!\n");
			}
			return ERROR_SILABS_TERCAB_LOADING_FIRMWARE;
		}
		if (line==3) {
			SiTracesSuspend();
		}
	}
	SiTracesResume();
	SiTRACE ("silabs_tercab_load_firmware complete...\n");
	return NO_SILABS_TERCAB_ERROR;
}

/*---------------------------------------------------*/
/* Si2158_EXIT_BOOTLOADER COMMAND                  */
/*---------------------------------------------------*/
static u8 silabs_tercab_exit_bootloader(silabs_tercab_context *ctx,
		u8   func,
		u8   ctsien)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[1];
	//ctx->rsp.exit_bootloader.STATUS = ctx->status;

	SiTRACE("%s()\n", __func__);

	cmdByteBuffer[0] = Si2158_EXIT_BOOTLOADER_CMD;
	cmdByteBuffer[1] = (u8) ( ( func   & Si2158_EXIT_BOOTLOADER_CMD_FUNC_MASK   ) << Si2158_EXIT_BOOTLOADER_CMD_FUNC_LSB  |
			( ctsien & Si2158_EXIT_BOOTLOADER_CMD_CTSIEN_MASK ) << Si2158_EXIT_BOOTLOADER_CMD_CTSIEN_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing EXIT_BOOTLOADER bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, 1, rspByteBuffer, &ctx->status);
	if (error_code) {
		SiTRACE("Error polling EXIT_BOOTLOADER response\n");
		return error_code;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/************************************************************************************************************************
  NAME: silabs_tercab_start_firmware
  DESCRIPTION: Start Si2158 firmware (put the Si2158 into run mode)
  Parameter:   Si2158 Context (I2C address)
  Parameter (passed by Reference):   ExitBootloadeer Response Status byte : tunint, atvint, dtvint, err, cts
  Returns:     I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 ************************************************************************************************************************/
static int silabs_tercab_start_firmware(silabs_tercab_context *ctx)
{
	if (silabs_tercab_exit_bootloader(ctx, Si2158_EXIT_BOOTLOADER_CMD_FUNC_TUNER, Si2158_EXIT_BOOTLOADER_CMD_CTSIEN_OFF) != NO_SILABS_TERCAB_ERROR)	{
		return ERROR_SILABS_TERCAB_STARTING_FIRMWARE;
	}
	return NO_SILABS_TERCAB_ERROR;
}

/************************************************************************************************************************
  NAME: silabs_tercab_power_up_with_patch
  DESCRIPTION: Send Si2158 ctx PowerUp Command with PowerUp to bootloader,
  Check the Chip rev and part, and ROMID are compared to expected values.
  Load the Firmware Patch then Start the Firmware.
  Programming Guide Reference:    Flowchart A.2 (POWER_UP with patch flowchart)

  Parameter:  pointer to Si2158 Context
  Returns:    Si2158/I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 ************************************************************************************************************************/
static int silabs_tercab_power_up_with_patch(struct silabs_tercab_priv *priv)
{
	silabs_tercab_context *ctx = &priv->tuner;
	struct part_info part_info;
	int return_code = NO_SILABS_TERCAB_ERROR;
	u16 docrc = 0;

	/* always wait for CTS prior to POWER_UP command */
	if ((return_code = silabs_tercab_poll_cts(ctx->i2c_adap, ctx->i2c_addr)) != NO_SILABS_TERCAB_ERROR) {
		SiTRACE ("Si2158_pollForCTS error 0x%02x\n", return_code);
		return return_code;
	}

	if ((return_code = silabs_tercab_power_up(ctx->i2c_adap, ctx->i2c_addr,
			Si2158_POWER_UP_CMD_SUBCODE_CODE,
			Si2158_POWER_UP_CMD_CLOCK_MODE_XTAL,
			Si2158_POWER_UP_CMD_EN_XOUT_EN_XOUT,
			Si2158_POWER_UP_CMD_PD_LDO_LDO_POWER_UP,
			Si2158_POWER_UP_CMD_RESERVED2_RESERVED,
			Si2158_POWER_UP_CMD_RESERVED3_RESERVED,
			Si2158_POWER_UP_CMD_RESERVED4_RESERVED,
			Si2158_POWER_UP_CMD_RESERVED5_RESERVED,
			Si2158_POWER_UP_CMD_RESERVED6_RESERVED,
			Si2158_POWER_UP_CMD_RESERVED7_RESERVED,
			Si2158_POWER_UP_CMD_RESET_RESET,
			Si2158_POWER_UP_CMD_CLOCK_FREQ_CLK_24MHZ,
			Si2158_POWER_UP_CMD_RESERVED8_RESERVED,
			Si2158_POWER_UP_CMD_FUNC_BOOTLOADER,
#ifndef __SI2158__
			Si2157_POWER_UP_CMD_RESERVED9_RESERVED,
#endif
			Si2158_POWER_UP_CMD_CTSIEN_DISABLE,
			Si2158_POWER_UP_CMD_WAKE_UP_WAKE_UP,
			&ctx->status
	)) != NO_SILABS_TERCAB_ERROR) {
		/* _power_up_call_insertion_point */

		SiTRACE ("silabs_tercab power up error 0x%02x: %s\n", return_code, silabs_tercab_error_text(return_code) );
		return return_code;
	}

	/* Get the Part Info from the chip.   This command is only valid in Bootloader mode */
	if ((return_code = si2158_part_info(priv->i2c_props.adap, priv->i2c_props.addr, &part_info)) != NO_SILABS_TERCAB_ERROR) {
		SiTRACE ("silabs_tercab part info error 0x%02x: %s\n", return_code, silabs_tercab_error_text(return_code) );
		return return_code;
	}
	ctx->rsp.part_info.chiprev  = part_info.chiprev;
	ctx->rsp.part_info.part     = part_info.part;
	ctx->rsp.part_info.pmajor   = part_info.pmajor;
	ctx->rsp.part_info.pbuild   = part_info.pbuild;
	ctx->rsp.part_info.romid    = part_info.romid;
	ctx->rsp.part_info.reserved = part_info.reserved;
	ctx->rsp.part_info.serial   = part_info.serial;

#ifdef SiTRACES
	SiTRACE("chiprev %d\n",        ctx->rsp.part_info.chiprev);
	SiTRACE("part    Si21%d\n",    ctx->rsp.part_info.part   );
	SiTRACE("pmajor  %d\n",        ctx->rsp.part_info.pmajor );
	if (ctx->rsp.part_info.pmajor >= 0x30) {
		SiTRACE("pmajor '%c'\n",       ctx->rsp.part_info.pmajor );
	}
	SiTRACE("pminor  %d\n",        ctx->rsp.part_info.pminor );
	if (ctx->rsp.part_info.pminor >= 0x30) {
		SiTRACE("pminor '%c'\n",       ctx->rsp.part_info.pminor );
	}
#ifndef __SI2158__
	SiTRACE("pbuild %d\n",         ctx->rsp.part_info.pbuild );
#endif
	SiTRACE("romid %3d/0x%02x\n",  ctx->rsp.part_info.romid,  ctx->rsp.part_info.romid );
#endif /* SiTRACES */

	if (ctx->rsp.part_info.part == 77) {
#ifdef PART_INTEGRITY_CHECKS
		/* Check the Chip revision, part and ROMID against expected values and report an error if incompatible */
		if (ctx->rsp.part_info.chiprev != ctx->chiprev) {
			SiTRACE ("INCOMPATIBLE PART error chiprev %d (expected %d)\n", ctx->rsp.part_info.chiprev, ctx->chiprev);
			return_code = ERROR_Si2177_INCOMPATIBLE_PART;
		}
		/* Part Number is represented as the last 2 digits */
		if (ctx->rsp.part_info.part != ctx->part) {
			SiTRACE ("INCOMPATIBLE PART error part   %d (expected %d)\n", ctx->rsp.part_info.part, ctx->part);
			return_code = ERROR_Si2177_INCOMPATIBLE_PART;
		}
		/* Part Major Version in ASCII */
		if (ctx->rsp.part_info.pmajor != ctx->partMajorVersion) {
			SiTRACE ("INCOMPATIBLE PART error pmajor %d (expected %d)\n", ctx->rsp.part_info.pmajor, ctx->partMajorVersion);
			return_code = ERROR_Si2177_INCOMPATIBLE_PART;
		}
		/* Part Minor Version in ASCII */
		if (ctx->rsp.part_info.pminor != ctx->partMinorVersion) {
			SiTRACE ("INCOMPATIBLE PART error pminor %d (expected %d)\n", ctx->rsp.part_info.pminor, ctx->partMinorVersion);
			return_code = ERROR_Si2177_INCOMPATIBLE_PART;
		}
		/* ROMID in byte representation */
		if (ctx->rsp.part_info.romid != ctx->partRomid) {
			SiTRACE ("INCOMPATIBLE PART error romid %3d (expected %2d)\n", ctx->rsp.part_info.romid, ctx->partRomid);
			return_code = ERROR_Si2177_INCOMPATIBLE_PART;
		}
		if (return_code != NO_SILABS_TERCAB_ERROR) return return_code;
#endif /* PART_INTEGRITY_CHECKS */
		if (ctx->rsp.part_info.romid == 0x50) {
			if ((return_code = silabs_tercab_load_firmware_16(ctx, Si2177_FW_3_0bx, FIRMWARE_LINES_3_0bx)) != NO_SILABS_TERCAB_ERROR) {
				SiTRACE ("silabs_tercab_load_firmware_16 error 0x%02x: %s\n", return_code, silabs_tercab_error_text(return_code) );
				return return_code;
			}
		} else {
			SiTRACE ("Invalid ROMID error 0x%02x: ROMID=%02x\n", ERROR_SILABS_TERCAB_INCOMPATIBLE_PART, ctx->rsp.part_info.romid );
			return ERROR_SILABS_TERCAB_INCOMPATIBLE_PART;
		}
	}

	docrc = RAM_CRC_2_1b9;//FGR Si2158

#ifndef __SI2158__
	if (ctx->rsp.part_info.romid == 0x50) {
		docrc = 0;//FGR - 21x7 have no CRC command
		SiTRACE ("silabs_tercab_load_firmware found Si21x7\n");

		/* Only load the Firmware if we are NOT a Si2157-A30 part*/
		if ((ctx->rsp.part_info.pmajor == '3') && (ctx->rsp.part_info.pminor == '0') && (ctx->rsp.part_info.pbuild == 5)) {
			SiTRACE ("No firmware to download for Si21x7-A30. Loading from NVM only\n" );
		} else {
			if ((return_code = silabs_tercab_load_firmware_16(ctx, Si2157_FW_3_0b5, FIRMWARE_LINES_3_0b5)) != NO_SILABS_TERCAB_ERROR) {
				SiTRACE ("silabs_tercab_load_firmware error 0x%02x: %s\n", return_code, silabs_tercab_error_text(return_code) );
				return return_code;
			}
		}
	} else
#endif
		/* Load the Firmware based on ROMID */
		if (ctx->rsp.part_info.romid == 0x32) {
			/* Load the Firmware */
			if ((return_code = silabs_tercab_load_firmware(ctx, Si2158_FW_0_Eb15, FIRMWARE_LINES_0_Eb15)) != NO_SILABS_TERCAB_ERROR) {
				SiTRACE ("silabs_tercab_load_firmware error 0x%02x: %s\n", return_code, silabs_tercab_error_text(return_code) );
				return return_code;
			}
		} else if (ctx->rsp.part_info.romid == 0x33) { /* if Si2158-A20 part load firmware patch (currently a dummy patch , 20bx) */
			/* This dummy patch (20bx) will skip the loadFirmware function and boot from NVM only.
           When a new patch is available for the Si2158-A20, include the patch file and replace the firmware array and size in the function below */
			if ((return_code = silabs_tercab_load_firmware_16(ctx, Si2158_FW_2_1b9, FIRMWARE_LINES_2_1b9)) != NO_SILABS_TERCAB_ERROR) {
				SiTRACE ("silabs_tercab_load_firmware_16 error 0x%02x: %s\n", return_code, silabs_tercab_error_text(return_code) );
				return return_code;
			}
		} else {
			SiTRACE ("INCOMPATIBLE PART error ROMID 0x%02x\n", ctx->rsp.part_info.romid);
			return ERROR_SILABS_TERCAB_INCOMPATIBLE_PART;
		}
#ifdef RAM_CRC_CHECK
	if (docrc) {
		/* Check the RAM_CRC value and compare with the expected value */
		/* If they match then move on,  otherwise indicate error */
		/* This check is bypassed by default to speed boot time. */

		if ((return_code = Si2158_L1_RAM_CRC(ctx)) != NO_SILABS_TERCAB_ERROR) {
			SiTRACE ("Si2158_L1_RAM_CRC error 0x%02x: %s\n", return_code, silabs_tercab_error_text(return_code) );
			return return_code;
		}
		SiTRACE("RAM CRC = 0x%X\n",ctx->rsp.ram_crc.crc);
		if (ctx->rsp.ram_crc.crc != docrc) {
			return_code = ERROR_SILABS_TERCAB_CRC_CHECK_ERROR;
			SiTRACE ("RAM_CRC (0x%X) does not match expected (0x%X) error 0x%02x: %s\n",ctx->rsp.ram_crc.crc,docrc,return_code, silabs_tercab_error_text(return_code) );
			return return_code;
		}
	}
#endif /* RAM_CRC_CHECK */

	/* Start the Firmware */
	return_code = silabs_tercab_start_firmware(ctx);
	if (return_code != NO_SILABS_TERCAB_ERROR) {
		SiTRACE("silabs_tercab_start_firmware error 0x%02x: %s\n", return_code, silabs_tercab_error_text(return_code));
		return return_code;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/*---------------------------------------------------*/
/* Si2158_CONFIG_PINS COMMAND                      */
/*---------------------------------------------------*/
static u8 silabs_tercab_config_pins(silabs_tercab_context *ctx,
		u8   gpio1_mode,
		u8   gpio1_read,
		u8   gpio2_mode,
		u8   gpio2_read,
		u8   reserved1,
		u8   reserved2,
		u8   reserved3)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[6];
	u8 rspByteBuffer[6];

	SiTRACE("%s()\n", __func__);

	cmdByteBuffer[0] = Si2158_CONFIG_PINS_CMD;
	cmdByteBuffer[1] = (u8) ( ( gpio1_mode & Si2158_CONFIG_PINS_CMD_GPIO1_MODE_MASK ) << Si2158_CONFIG_PINS_CMD_GPIO1_MODE_LSB|
							  ( gpio1_read & Si2158_CONFIG_PINS_CMD_GPIO1_READ_MASK ) << Si2158_CONFIG_PINS_CMD_GPIO1_READ_LSB);
	cmdByteBuffer[2] = (u8) ( ( gpio2_mode & Si2158_CONFIG_PINS_CMD_GPIO2_MODE_MASK ) << Si2158_CONFIG_PINS_CMD_GPIO2_MODE_LSB|
							  ( gpio2_read & Si2158_CONFIG_PINS_CMD_GPIO2_READ_MASK ) << Si2158_CONFIG_PINS_CMD_GPIO2_READ_LSB);
	cmdByteBuffer[3] = (u8) ( ( reserved1  & Si2158_CONFIG_PINS_CMD_RESERVED1_MASK  ) << Si2158_CONFIG_PINS_CMD_RESERVED1_LSB );
	cmdByteBuffer[4] = (u8) ( ( reserved2  & Si2158_CONFIG_PINS_CMD_RESERVED2_MASK  ) << Si2158_CONFIG_PINS_CMD_RESERVED2_LSB );
	cmdByteBuffer[5] = (u8) ( ( reserved3  & Si2158_CONFIG_PINS_CMD_RESERVED3_MASK  ) << Si2158_CONFIG_PINS_CMD_RESERVED3_LSB );

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 6, cmdByteBuffer) != 6) {
		SiTRACE("Error writing CONFIG_PINS bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, 6, rspByteBuffer, &ctx->status);
	if (error_code) {
		SiTRACE("Error polling CONFIG_PINS response\n");
		return error_code;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/***********************************************************************************************************************
  silabs_tercab_ctx_init function
  Use:        software initialisation function
              Used to initialize the software context
  Returns:    0 if no error
  Comments:   It should be called first and once only when starting the application
  Parameter:   **ppctx         a pointer to the ctx context to initialize
  Parameter:  add            the Si2158 I2C address
  Porting:    Allocation errors need to be properly managed.
  Porting:    I2C initialization needs to be adapted to use the available I2C functions
 ***********************************************************************************************************************/
static u8 silabs_tercab_ctx_init(silabs_tercab_context *ctx, u8 addr, struct i2c_adapter *i2c_adap)
{
	ctx->i2c_addr = addr;
	ctx->i2c_adap = i2c_adap;

#ifdef SiTRACES
	if (!trace_init_done) {
		CUSTOM_PRINTF("********** SiTRACES activated *********\n");
		CUSTOM_PRINTF("Comment the '#define SiTRACES' line\n");
		CUSTOM_PRINTF("in silabs_tercab_priv.h\n");
		CUSTOM_PRINTF("to deactivate all traces.\n");
		CUSTOM_PRINTF("***************************************\n");
		SiTraceDefaultConfiguration();
	}
#endif /* SiTRACES */

	return NO_SILABS_TERCAB_ERROR;
}

/************************************************************************************************************************
  silabs_tercab_sw_init function
  Use:        software initialization function
              Used to initialize the Si2158 and tuner structures
  Behavior:   This function performs all the steps necessary to initialize the Si2158 tuner instances
  Parameter:  front_end, a pointer to the Si2158_L2_Context context to be initialized
  Parameter:  tunerAdd, the I2C address of the tuner
  Comments:     It MUST be called first and once before using any other function.
                It can be used to build a multi-demod/multi-tuner application, if called several times from the upper layer with different pointers and addresses
                After execution, all demod and tuner functions are accessible.
 ************************************************************************************************************************/
static char silabs_tercab_sw_init(struct silabs_tercab_priv *priv
		, int tunerAdd
		, void *p_context
		, struct i2c_adapter *i2c_adap
)
{
#ifdef    SiTRACES
	char infoStringBuffer[1000] = { 0 };
	char *infoString;
	infoString = &(infoStringBuffer[0]);
#endif /* SiTRACE */

	/* Pointers initialization */
	priv->silabs_tercab_init_done = 0;
	priv->firmware_started = 0;
	/* Calling underlying SW initialization functions */
	SiTRACE("silabs_tercab_ctx_init starting...\n");
	silabs_tercab_ctx_init(&priv->tuner, tunerAdd, i2c_adap);
#ifdef    SiTRACES
	if (silabs_tercab_infos(priv, infoString)) {
		SiTRACE("%s\n", infoString);
	}
#endif /* SiTRACE */
	SiTRACE("silabs_tercab_ctx_init complete\n");
	return 1;
}

/*****************************************************************************************
NAME: silabs_tercab_download_atv_properties
 DESCRIPTION: Setup Si2158 ATV properties configuration
 This function will download all the ATV configuration properties.
 The function SetupATVDefaults() should be called before the first call to this function.
 Parameter:  Pointer to Si2158 Context
 Returns:    I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 Programming Guide Reference:    ATV setup flowchart
 ******************************************************************************************/
static int silabs_tercab_download_atv_properties(silabs_tercab_context *ctx)
{
	const u16 atv_afc_range_khz                   = 1000; /* (default  1000) */
	const u8  atv_af_out_mute                     = Si2177_ATV_AF_OUT_PROP_MUTE_NORMAL; /* Si2177 (default 'NORMAL') */
	const u8  atv_af_out_volume                   = 0; /* Si2177 (default     0) */
	const u8  atv_agc_speed_if_agc_speed          = Si2158_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO; /* (default 'AUTO') */
	const u8  atv_agc_speed_low_rssi_if_agc_speed = Si2158_ATV_AGC_SPEED_LOW_RSSI_PROP_IF_AGC_SPEED_158; /* (default '158') */
	const s8  atv_agc_speed_low_rssi_thld         = -128; /* (default  -128) */
	const u8  atv_artificial_snow_gain            = Si2158_ATV_ARTIFICIAL_SNOW_PROP_GAIN_AUTO; /* (default 'AUTO') */
	const s8  atv_artificial_snow_offset          = 0; /* (default     0) */
	const u8  atv_artificial_snow_period          = Si2177_ATV_ARTIFICIAL_SNOW_PROP_PERIOD_LONG; /* (default 'LONG') */
	const u8  atv_artificial_snow_sound           = Si2177_ATV_ARTIFICIAL_SNOW_PROP_SOUND_MUTE; /* (default 'MUTE') */
	const u8  atv_audio_mode_audio_sys            = Si2177_ATV_AUDIO_MODE_PROP_AUDIO_SYS_DEFAULT; /* Si2177 (default 'DEFAULT') */
	const u8  atv_audio_mode_chan_bw              = Si2177_ATV_AUDIO_MODE_PROP_CHAN_BW_DEFAULT; /* Si2177 (default 'DEFAULT') */
	const u8  atv_audio_mode_demod_mode           = Si2177_ATV_AUDIO_MODE_PROP_DEMOD_MODE_SIF; /* Si2177 (default 'SIF') */
	const u8  atv_config_if_port_atv_agc_source   = Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_AGC_SOURCE_AGC1_3DB; /* Si2158 (default 'INTERNAL') */
	const u8  atv_config_if_port_atv_out_type     = Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_LIF_DIFF_IF1; /* (default 'LIF_DIFF_IF1') */
	const u8  atv_cvbs_out_amp                    = 200; /* Si2177 (default   200) */
	const u8  atv_cvbs_out_offset                 = 25; /* Si2177 (default    25) */
	const u8  atv_cvbs_out_fine_amp               = 100; /* Si2177 (default   100) */
	const s8  atv_cvbs_out_fine_offset            = 0; /* Si2177 (default     0) */
	const u8  atv_hsync_out_gpio_sel              = Si2177_ATV_HSYNC_OUT_PROP_GPIO_SEL_NONE ; /* (default 'NONE') */
	const s8  atv_hsync_out_offset                = 0; /* Si2177 (default     0) */
	const u8  atv_hsync_out_width                 = 42; /* Si2177 (default    42) */
	const u8  atv_ext_agc_max_10mv                = 200; /* (default   200) */
	const u8  atv_ext_agc_min_10mv                = 50; /* (default    50) */
	const u8  atv_ien_chlien                      = Si2158_ATV_IEN_PROP_CHLIEN_ENABLE;           /* (default 'ENABLE') */
	const u8  atv_ien_pclien                      = Si2158_ATV_IEN_PROP_PCLIEN_DISABLE;          /* (default 'DISABLE') */
	const u8  atv_ien_dlien                       = Si2177_ATV_IEN_PROP_DLIEN_DISABLE;           /* (default 'DISABLE') */
	const u8  atv_ien_snrlien                     = Si2177_ATV_IEN_PROP_SNRLIEN_DISABLE;         /* (default 'DISABLE') */
	const u8  atv_ien_snrhien                     = Si2177_ATV_IEN_PROP_SNRHIEN_DISABLE;         /* (default 'DISABLE') */
	const u8  atv_int_sense_chlnegen              = Si2158_ATV_INT_SENSE_PROP_CHLNEGEN_DISABLE;  /* (default 'DISABLE') */
	const u8  atv_int_sense_pclnegen              = Si2158_ATV_INT_SENSE_PROP_PCLNEGEN_DISABLE;  /* (default 'DISABLE') */
	const u8  atv_int_sense_chlposen              = Si2158_ATV_INT_SENSE_PROP_CHLPOSEN_ENABLE ;  /* (default 'ENABLE') */
	const u8  atv_int_sense_pclposen              = Si2158_ATV_INT_SENSE_PROP_PCLPOSEN_ENABLE;   /* (default 'ENABLE') */
	const u8  atv_int_sense_dlnegen               = Si2177_ATV_INT_SENSE_PROP_DLNEGEN_DISABLE;   /* (default 'DISABLE') */
	const u8  atv_int_sense_snrlnegen             = Si2177_ATV_INT_SENSE_PROP_SNRLNEGEN_DISABLE; /* (default 'DISABLE') */
	const u8  atv_int_sense_snrhnegen             = Si2177_ATV_INT_SENSE_PROP_SNRHNEGEN_DISABLE; /* (default 'DISABLE') */
	const u8  atv_int_sense_dlposen               = Si2177_ATV_INT_SENSE_PROP_DLPOSEN_ENABLE;    /* (default 'ENABLE') */
	const u8  atv_int_sense_snrlposen             = Si2177_ATV_INT_SENSE_PROP_SNRLPOSEN_ENABLE;  /* (default 'ENABLE') */
	const u8  atv_int_sense_snrhposen             = Si2177_ATV_INT_SENSE_PROP_SNRHPOSEN_ENABLE;  /* (default 'ENABLE') */
	const u16 atv_lif_freq_offset                 = 4000; /* (default  5000) */
	const u8  atv_lif_out_amp                     = 100; /* (default   100) */
	const u8  atv_lif_out_offset                  = 148; /* (default   148) */
	const u8  atv_pga_target_override_enable      = Si2158_ATV_PGA_TARGET_PROP_OVERRIDE_ENABLE_DISABLE; /* (default 'DISABLE') */
	const s8  atv_pga_target_pga_target           = 0; /* (default     0) */
	const u8  atv_rf_top_atv_rf_top               = Si2158_ATV_RF_TOP_PROP_ATV_RF_TOP_AUTO; /* (default 'AUTO') */
	const s8  atv_rsq_rssi_threshold_hi           = 0; /* (default     0) */
	const s8  atv_rsq_rssi_threshold_lo           = -70; /* (default   -70) */
	const u8  atv_rsq_snr_threshold_hi            = 45; /* Si2177 (default    45) */
	const u8  atv_rsq_snr_threshold_lo            = 25; /* Si2177 (default    25) */
	const u8  atv_sif_out_amp                     = 60; /* Si2177 (default    60) */
	const u8  atv_sif_out_offset                  = 135; /* Si2177 (default   135) */
	const s8  atv_sound_agc_limit_max_gain        = 84; /* Si2177 (default    84) */
	const s8  atv_sound_agc_limit_min_gain        = -84; /* Si2177 (default   -84) */
	const u8  atv_sound_agc_speed_other_systems   = 4; /* Si2177 (default     4) */
	const u8  atv_sound_agc_speed_system_l        = 5; /* Si2177 (default     5) */
	const s8  atv_video_equalizer_slope           = 0; /* Si2177 (default     0) */
	const u8  atv_video_mode_color                = Si2158_ATV_VIDEO_MODE_PROP_COLOR_PAL_NTSC; /* (default 'PAL_NTSC') */
	const u8  atv_video_mode_invert_spectrum      = Si2158_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_INVERTED; /* (default 'INVERTED') */
	const u8  atv_video_mode_video_sys            = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M; /* (default 'B') */
	const u8  atv_vsnr_cap_atv_vsnr_cap           = 0; /* (default     0) */

	u16 si2158_atv_afc_range__prop_data;
	u16 si2177_atv_af_out__prop_data;
	u16 si2158_atv_agc_speed__prop_data;
	u16 si2158_atv_agc_speed_low_rssi__prop_data;
	u16 si2177_atv_artificial_snow__prop_data;
	u16 si2158_atv_artificial_snow__prop_data;
	u16 si2177_atv_audio_mode__prop_data;
	u16 si2158_atv_config_if_port__prop_data;
	u16 si2177_atv_config_if_port__prop_data;
	u16 si2158_atv_ext_agc__prop_data;
	u16 si2177_atv_cvbs_out__prop_data;
	u16 si2177_atv_cvbs_out_fine__prop_data;
	u16 si2177_atv_hsync_out__prop_data;
	u16 si2158_atv_ien__prop_data;
	u16 si2177_atv_ien__prop_data;
	u16 si2158_atv_int_sense__prop_data;
	u16 si2177_atv_int_sense__prop_data;
	u16 si2158_atv_lif_freq__prop_data;
	u16 si2158_atv_lif_out__prop_data;
	u16 si2158_atv_pga_target__prop_data;
	u16 si2158_atv_rf_top__prop_data;
	u16 si2158_atv_rsq_rssi_threshold__prop_data;
	u16 si2177_atv_rsq_snr_threshold__prop_data;
	u16 si2177_atv_sif_out__prop_data;
	u16 si2177_atv_sound_agc_limit__prop_data;
	u16 si2177_atv_sound_agc_speed__prop_data;
	u16 si2177_atv_video_equalizer__prop_data;
	u16 si2158_atv_video_mode__prop_data;
	u16 si2177_atv_video_mode__prop_data;
	u16 si2158_atv_vsnr_cap__prop_data;

	SiTRACE("%s()\n", __func__);

	si2158_atv_afc_range__prop_data = (atv_afc_range_khz & Si2158_ATV_AFC_RANGE_PROP_RANGE_KHZ_MASK) << Si2158_ATV_AFC_RANGE_PROP_RANGE_KHZ_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_ATV_AFC_RANGE_PROP_CODE, si2158_atv_afc_range__prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	if (ctx->part == 77) {
		si2177_atv_af_out__prop_data = (atv_af_out_volume & Si2177_ATV_AF_OUT_PROP_VOLUME_MASK) << Si2177_ATV_AF_OUT_PROP_VOLUME_LSB  |
			   (atv_af_out_mute   & Si2177_ATV_AF_OUT_PROP_MUTE_MASK  ) << Si2177_ATV_AF_OUT_PROP_MUTE_LSB ;
		if (silabs_tercab_set_property(ctx, Si2177_ATV_AF_OUT_PROP_CODE, si2177_atv_af_out__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
	}

	si2158_atv_agc_speed__prop_data = (atv_agc_speed_if_agc_speed & Si2158_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK) << Si2158_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_ATV_AGC_SPEED_PROP_CODE, si2158_atv_agc_speed__prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_atv_agc_speed_low_rssi__prop_data = (atv_agc_speed_low_rssi_if_agc_speed & Si2158_ATV_AGC_SPEED_LOW_RSSI_PROP_IF_AGC_SPEED_MASK) << Si2158_ATV_AGC_SPEED_LOW_RSSI_PROP_IF_AGC_SPEED_LSB  |
		   (atv_agc_speed_low_rssi_thld         & Si2158_ATV_AGC_SPEED_LOW_RSSI_PROP_THLD_MASK        ) << Si2158_ATV_AGC_SPEED_LOW_RSSI_PROP_THLD_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_ATV_AGC_SPEED_LOW_RSSI_PROP_CODE, si2158_atv_agc_speed_low_rssi__prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	if (ctx->part == 77) {
		si2177_atv_artificial_snow__prop_data = (atv_artificial_snow_gain   & Si2158_ATV_ARTIFICIAL_SNOW_PROP_GAIN_MASK  ) << Si2158_ATV_ARTIFICIAL_SNOW_PROP_GAIN_LSB  |
				(atv_artificial_snow_sound  & Si2177_ATV_ARTIFICIAL_SNOW_PROP_SOUND_MASK ) << Si2177_ATV_ARTIFICIAL_SNOW_PROP_SOUND_LSB  |
				(atv_artificial_snow_period & Si2177_ATV_ARTIFICIAL_SNOW_PROP_PERIOD_MASK) << Si2177_ATV_ARTIFICIAL_SNOW_PROP_PERIOD_LSB  |
				(atv_artificial_snow_offset & Si2158_ATV_ARTIFICIAL_SNOW_PROP_OFFSET_MASK) << Si2158_ATV_ARTIFICIAL_SNOW_PROP_OFFSET_LSB ;
		if (silabs_tercab_set_property(ctx, Si2158_ATV_ARTIFICIAL_SNOW_PROP_CODE, si2177_atv_artificial_snow__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
	} else {
		si2158_atv_artificial_snow__prop_data = (atv_artificial_snow_gain   & Si2158_ATV_ARTIFICIAL_SNOW_PROP_GAIN_MASK  ) << Si2158_ATV_ARTIFICIAL_SNOW_PROP_GAIN_LSB  |
			   (atv_artificial_snow_offset & Si2158_ATV_ARTIFICIAL_SNOW_PROP_OFFSET_MASK) << Si2158_ATV_ARTIFICIAL_SNOW_PROP_OFFSET_LSB ;

		if (silabs_tercab_set_property(ctx, Si2158_ATV_ARTIFICIAL_SNOW_PROP_CODE, si2158_atv_artificial_snow__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
	}

	if (ctx->part == 77) {
		si2177_atv_audio_mode__prop_data = (atv_audio_mode_audio_sys  & Si2177_ATV_AUDIO_MODE_PROP_AUDIO_SYS_MASK ) << Si2177_ATV_AUDIO_MODE_PROP_AUDIO_SYS_LSB  |
			   (atv_audio_mode_demod_mode & Si2177_ATV_AUDIO_MODE_PROP_DEMOD_MODE_MASK) << Si2177_ATV_AUDIO_MODE_PROP_DEMOD_MODE_LSB  |
			   (atv_audio_mode_chan_bw    & Si2177_ATV_AUDIO_MODE_PROP_CHAN_BW_MASK   ) << Si2177_ATV_AUDIO_MODE_PROP_CHAN_BW_LSB ;
		if (silabs_tercab_set_property(ctx, Si2177_ATV_AUDIO_MODE_PROP_CODE, si2177_atv_audio_mode__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
	}

	switch (ctx->part) {
	case 57:
		si2158_atv_config_if_port__prop_data = (atv_config_if_port_atv_out_type   & Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_MASK  ) << Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_LSB  |
		       (atv_config_if_port_atv_agc_source & Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_AGC_SOURCE_MASK) << Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_AGC_SOURCE_LSB ;
		if (silabs_tercab_set_property(ctx, Si2158_ATV_CONFIG_IF_PORT_PROP_CODE, si2158_atv_config_if_port__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
		break;
	case 77: /* Si2177 has no ATV_AGC_SOURCE */
		si2177_atv_config_if_port__prop_data = (atv_config_if_port_atv_out_type & Si2177_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_MASK) << Si2177_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_LSB ;
		if (silabs_tercab_set_property(ctx, Si2158_ATV_CONFIG_IF_PORT_PROP_CODE, si2177_atv_config_if_port__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
		break;
	default:
		pr_warn("%s(): unsupported tuner model Si21%02u\n", __func__, ctx->part);
		break;
	}

	si2158_atv_ext_agc__prop_data = (atv_ext_agc_min_10mv & Si2158_ATV_EXT_AGC_PROP_MIN_10MV_MASK) << Si2158_ATV_EXT_AGC_PROP_MIN_10MV_LSB  |
		   (atv_ext_agc_max_10mv & Si2158_ATV_EXT_AGC_PROP_MAX_10MV_MASK) << Si2158_ATV_EXT_AGC_PROP_MAX_10MV_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_ATV_EXT_AGC_PROP_CODE, si2158_atv_ext_agc__prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	if (ctx->part == 77) {
		si2177_atv_cvbs_out__prop_data = (atv_cvbs_out_offset & Si2177_ATV_CVBS_OUT_PROP_OFFSET_MASK) << Si2177_ATV_CVBS_OUT_PROP_OFFSET_LSB  |
			   (atv_cvbs_out_amp    & Si2177_ATV_CVBS_OUT_PROP_AMP_MASK   ) << Si2177_ATV_CVBS_OUT_PROP_AMP_LSB ;
		if (silabs_tercab_set_property(ctx, Si2177_ATV_CVBS_OUT_PROP_CODE, si2177_atv_cvbs_out__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}

		si2177_atv_cvbs_out_fine__prop_data = (atv_cvbs_out_fine_offset & Si2177_ATV_CVBS_OUT_FINE_PROP_OFFSET_MASK) << Si2177_ATV_CVBS_OUT_FINE_PROP_OFFSET_LSB  |
			   (atv_cvbs_out_fine_amp    & Si2177_ATV_CVBS_OUT_FINE_PROP_AMP_MASK   ) << Si2177_ATV_CVBS_OUT_FINE_PROP_AMP_LSB ;
		if (silabs_tercab_set_property(ctx, Si2177_ATV_CVBS_OUT_FINE_PROP_CODE, si2177_atv_cvbs_out_fine__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}

		si2177_atv_hsync_out__prop_data = (atv_hsync_out_gpio_sel & Si2177_ATV_HSYNC_OUT_PROP_GPIO_SEL_MASK) << Si2177_ATV_HSYNC_OUT_PROP_GPIO_SEL_LSB  |
	           (atv_hsync_out_width    & Si2177_ATV_HSYNC_OUT_PROP_WIDTH_MASK   ) << Si2177_ATV_HSYNC_OUT_PROP_WIDTH_LSB  |
	           (atv_hsync_out_offset   & Si2177_ATV_HSYNC_OUT_PROP_OFFSET_MASK  ) << Si2177_ATV_HSYNC_OUT_PROP_OFFSET_LSB ;
		if (silabs_tercab_set_property(ctx, Si2177_ATV_HSYNC_OUT_PROP_CODE, si2177_atv_hsync_out__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
	}

	switch (ctx->part) {
	case 57:
		si2158_atv_ien__prop_data = (atv_ien_chlien & Si2158_ATV_IEN_PROP_CHLIEN_MASK) << Si2158_ATV_IEN_PROP_CHLIEN_LSB  |
		       (atv_ien_pclien & Si2158_ATV_IEN_PROP_PCLIEN_MASK) << Si2158_ATV_IEN_PROP_PCLIEN_LSB ;
		if (silabs_tercab_set_property(ctx, Si2158_ATV_IEN_PROP_CODE, si2158_atv_ien__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
		break;
	case 77:
		si2177_atv_ien__prop_data = (atv_ien_chlien  & Si2158_ATV_IEN_PROP_CHLIEN_MASK ) << Si2158_ATV_IEN_PROP_CHLIEN_LSB  |
		       (atv_ien_pclien  & Si2158_ATV_IEN_PROP_PCLIEN_MASK ) << Si2158_ATV_IEN_PROP_PCLIEN_LSB  |
		       (atv_ien_dlien   & Si2177_ATV_IEN_PROP_DLIEN_MASK  ) << Si2177_ATV_IEN_PROP_DLIEN_LSB  |
		       (atv_ien_snrlien & Si2177_ATV_IEN_PROP_SNRLIEN_MASK) << Si2177_ATV_IEN_PROP_SNRLIEN_LSB  |
		       (atv_ien_snrhien & Si2177_ATV_IEN_PROP_SNRHIEN_MASK) << Si2177_ATV_IEN_PROP_SNRHIEN_LSB ;
		if (silabs_tercab_set_property(ctx, Si2158_ATV_IEN_PROP_CODE, si2177_atv_ien__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
		break;
	default:
		pr_warn("%s(): unsupported tuner model Si21%02u\n", __func__, ctx->part);
		break;
	}

	switch (ctx->part) {
	case 57:
		si2158_atv_int_sense__prop_data = (atv_int_sense_chlnegen & Si2158_ATV_INT_SENSE_PROP_CHLNEGEN_MASK) << Si2158_ATV_INT_SENSE_PROP_CHLNEGEN_LSB  |
		       (atv_int_sense_pclnegen & Si2158_ATV_INT_SENSE_PROP_PCLNEGEN_MASK) << Si2158_ATV_INT_SENSE_PROP_PCLNEGEN_LSB  |
		       (atv_int_sense_chlposen & Si2158_ATV_INT_SENSE_PROP_CHLPOSEN_MASK) << Si2158_ATV_INT_SENSE_PROP_CHLPOSEN_LSB  |
		       (atv_int_sense_pclposen & Si2158_ATV_INT_SENSE_PROP_PCLPOSEN_MASK) << Si2158_ATV_INT_SENSE_PROP_PCLPOSEN_LSB ;
		if (silabs_tercab_set_property(ctx, Si2158_ATV_INT_SENSE_PROP_CODE, si2158_atv_int_sense__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
		break;
	case 77:
		si2177_atv_int_sense__prop_data = (atv_int_sense_chlnegen  & Si2158_ATV_INT_SENSE_PROP_CHLNEGEN_MASK ) << Si2158_ATV_INT_SENSE_PROP_CHLNEGEN_LSB  |
		       (atv_int_sense_pclnegen  & Si2158_ATV_INT_SENSE_PROP_PCLNEGEN_MASK ) << Si2158_ATV_INT_SENSE_PROP_PCLNEGEN_LSB  |
		       (atv_int_sense_dlnegen   & Si2177_ATV_INT_SENSE_PROP_DLNEGEN_MASK  ) << Si2177_ATV_INT_SENSE_PROP_DLNEGEN_LSB  |
		       (atv_int_sense_snrlnegen & Si2177_ATV_INT_SENSE_PROP_SNRLNEGEN_MASK) << Si2177_ATV_INT_SENSE_PROP_SNRLNEGEN_LSB  |
		       (atv_int_sense_snrhnegen & Si2177_ATV_INT_SENSE_PROP_SNRHNEGEN_MASK) << Si2177_ATV_INT_SENSE_PROP_SNRHNEGEN_LSB  |
		       (atv_int_sense_chlposen  & Si2158_ATV_INT_SENSE_PROP_CHLPOSEN_MASK ) << Si2158_ATV_INT_SENSE_PROP_CHLPOSEN_LSB  |
		       (atv_int_sense_pclposen  & Si2158_ATV_INT_SENSE_PROP_PCLPOSEN_MASK ) << Si2158_ATV_INT_SENSE_PROP_PCLPOSEN_LSB  |
		       (atv_int_sense_dlposen   & Si2177_ATV_INT_SENSE_PROP_DLPOSEN_MASK  ) << Si2177_ATV_INT_SENSE_PROP_DLPOSEN_LSB  |
		       (atv_int_sense_snrlposen & Si2177_ATV_INT_SENSE_PROP_SNRLPOSEN_MASK) << Si2177_ATV_INT_SENSE_PROP_SNRLPOSEN_LSB  |
		       (atv_int_sense_snrhposen & Si2177_ATV_INT_SENSE_PROP_SNRHPOSEN_MASK) << Si2177_ATV_INT_SENSE_PROP_SNRHPOSEN_LSB ;
		if (silabs_tercab_set_property(ctx, Si2158_ATV_INT_SENSE_PROP_CODE, si2177_atv_int_sense__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
		break;
	default:
		pr_warn("%s(): unsupported tuner model Si21%02u\n", __func__, ctx->part);
		break;
	}

	si2158_atv_lif_freq__prop_data = (atv_lif_freq_offset & Si2158_ATV_LIF_FREQ_PROP_OFFSET_MASK) << Si2158_ATV_LIF_FREQ_PROP_OFFSET_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_ATV_LIF_FREQ_PROP_CODE, si2158_atv_lif_freq__prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_atv_lif_out__prop_data = (atv_lif_out_offset & Si2158_ATV_LIF_OUT_PROP_OFFSET_MASK) << Si2158_ATV_LIF_OUT_PROP_OFFSET_LSB  |
		   (atv_lif_out_amp    & Si2158_ATV_LIF_OUT_PROP_AMP_MASK   ) << Si2158_ATV_LIF_OUT_PROP_AMP_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_ATV_LIF_OUT_PROP_CODE, si2158_atv_lif_out__prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_atv_pga_target__prop_data = (atv_pga_target_pga_target      & Si2158_ATV_PGA_TARGET_PROP_PGA_TARGET_MASK     ) << Si2158_ATV_PGA_TARGET_PROP_PGA_TARGET_LSB  |
		   (atv_pga_target_override_enable & Si2158_ATV_PGA_TARGET_PROP_OVERRIDE_ENABLE_MASK) << Si2158_ATV_PGA_TARGET_PROP_OVERRIDE_ENABLE_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_ATV_PGA_TARGET_PROP_CODE, si2158_atv_pga_target__prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_atv_rf_top__prop_data = (atv_rf_top_atv_rf_top & Si2158_ATV_RF_TOP_PROP_ATV_RF_TOP_MASK) << Si2158_ATV_RF_TOP_PROP_ATV_RF_TOP_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_ATV_RF_TOP_PROP_CODE, si2158_atv_rf_top__prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_atv_rsq_rssi_threshold__prop_data = (atv_rsq_rssi_threshold_lo & Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_MASK) << Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_LSB  |
		   (atv_rsq_rssi_threshold_hi & Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_MASK) << Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_CODE, si2158_atv_rsq_rssi_threshold__prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	if (ctx->part == 77) {
		si2177_atv_rsq_snr_threshold__prop_data = (atv_rsq_snr_threshold_lo & Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_LO_MASK) << Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_LO_LSB  |
			   (atv_rsq_snr_threshold_hi & Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_HI_MASK) << Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_HI_LSB ;
		if (silabs_tercab_set_property(ctx, Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_CODE, si2177_atv_rsq_snr_threshold__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}

		si2177_atv_sif_out__prop_data = (atv_sif_out_offset & Si2177_ATV_SIF_OUT_PROP_OFFSET_MASK) << Si2177_ATV_SIF_OUT_PROP_OFFSET_LSB  |
			   (atv_sif_out_amp    & Si2177_ATV_SIF_OUT_PROP_AMP_MASK   ) << Si2177_ATV_SIF_OUT_PROP_AMP_LSB ;
		if (silabs_tercab_set_property(ctx, Si2177_ATV_SIF_OUT_PROP_CODE, si2177_atv_sif_out__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}

		si2177_atv_sound_agc_limit__prop_data = (atv_sound_agc_limit_max_gain & Si2177_ATV_SOUND_AGC_LIMIT_PROP_MAX_GAIN_MASK) << Si2177_ATV_SOUND_AGC_LIMIT_PROP_MAX_GAIN_LSB  |
			   (atv_sound_agc_limit_min_gain & Si2177_ATV_SOUND_AGC_LIMIT_PROP_MIN_GAIN_MASK) << Si2177_ATV_SOUND_AGC_LIMIT_PROP_MIN_GAIN_LSB ;
		if (silabs_tercab_set_property(ctx, Si2177_ATV_SOUND_AGC_LIMIT_PROP_CODE, si2177_atv_sound_agc_limit__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}

		si2177_atv_sound_agc_speed__prop_data = (atv_sound_agc_speed_system_l      & Si2177_ATV_SOUND_AGC_SPEED_PROP_SYSTEM_L_MASK     ) << Si2177_ATV_SOUND_AGC_SPEED_PROP_SYSTEM_L_LSB  |
			   (atv_sound_agc_speed_other_systems & Si2177_ATV_SOUND_AGC_SPEED_PROP_OTHER_SYSTEMS_MASK) << Si2177_ATV_SOUND_AGC_SPEED_PROP_OTHER_SYSTEMS_LSB ;
		if (silabs_tercab_set_property(ctx, Si2177_ATV_SOUND_AGC_SPEED_PROP_CODE, si2177_atv_sound_agc_speed__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}

		si2177_atv_video_equalizer__prop_data = (atv_video_equalizer_slope & Si2177_ATV_VIDEO_EQUALIZER_PROP_SLOPE_MASK) << Si2177_ATV_VIDEO_EQUALIZER_PROP_SLOPE_LSB ;
		if (silabs_tercab_set_property(ctx, Si2177_ATV_VIDEO_EQUALIZER_PROP_CODE, si2177_atv_video_equalizer__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
	}

	switch (ctx->part) {
	case 57:
		si2158_atv_video_mode__prop_data = (atv_video_mode_video_sys       & Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_MASK      ) << Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LSB  |
		       (atv_video_mode_color           & Si2158_ATV_VIDEO_MODE_PROP_COLOR_MASK          ) << Si2158_ATV_VIDEO_MODE_PROP_COLOR_LSB  |
		       (atv_video_mode_invert_spectrum & Si2158_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2158_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_LSB ;
		if (silabs_tercab_set_property(ctx, Si2158_ATV_VIDEO_MODE_PROP_CODE, si2158_atv_video_mode__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
		break;
	case 77:
		si2177_atv_video_mode__prop_data = (atv_video_mode_video_sys       & Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_MASK    ) << Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LSB  |
		       (atv_video_mode_color           & Si2158_ATV_VIDEO_MODE_PROP_COLOR_MASK        ) << Si2158_ATV_VIDEO_MODE_PROP_COLOR_LSB  |
		       (atv_video_mode_invert_spectrum & Si2177_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_MASK) << Si2177_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_LSB ;
		if (silabs_tercab_set_property(ctx, Si2158_ATV_VIDEO_MODE_PROP_CODE, si2177_atv_video_mode__prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
		break;
	default:
		pr_warn("%s(): unsupported tuner model Si21%02u\n", __func__, ctx->part);
		break;
	}

	si2158_atv_vsnr_cap__prop_data = (atv_vsnr_cap_atv_vsnr_cap & Si2158_ATV_VSNR_CAP_PROP_ATV_VSNR_CAP_MASK) << Si2158_ATV_VSNR_CAP_PROP_ATV_VSNR_CAP_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_ATV_VSNR_CAP_PROP_CODE, si2158_atv_vsnr_cap__prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/*****************************************************************************************
NAME: silabs_tercap_download_common_properties
 DESCRIPTION: Setup Si2158 COMMON properties configuration
 This function will download all the COMMON configuration properties.
 The function SetupCOMMONDefaults() should be called before the first call to this function.
 Parameter:  Pointer to Si2158 Context
 Returns:    I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 Programming Guide Reference:    COMMON setup flowchart
 ******************************************************************************************/
static int silabs_tercap_download_common_properties(silabs_tercab_context *ctx)
{
	const u8 crystal_trim_xo_cap = Si2158_CRYSTAL_TRIM_PROP_XO_CAP_6P7PF ; /* (default '8') */
	const u8 master_ien_atvien   = Si2158_MASTER_IEN_PROP_ATVIEN_OFF ; /* (default 'OFF') */
	const u8 master_ien_ctsien   = Si2158_MASTER_IEN_PROP_CTSIEN_OFF ; /* (default 'OFF') */
	const u8 master_ien_dtvien   = Si2158_MASTER_IEN_PROP_DTVIEN_OFF ; /* (default 'OFF') */
	const u8 master_ien_errien   = Si2158_MASTER_IEN_PROP_ERRIEN_OFF ; /* (default 'OFF') */
	const u8 master_ien_tunien   = Si2158_MASTER_IEN_PROP_TUNIEN_OFF ; /* (default 'OFF') */
	const u8 xout_amp            = Si2158_XOUT_PROP_AMP_HIGH ; /* (default 'HIGH') */

	u16 si2158_crystal_trim_prop_data;
	u16 si2158_master_ien_prop_data;
	u16 si2158_xout_prop_data;

	SiTRACE("%s()\n", __func__);

	si2158_crystal_trim_prop_data = (crystal_trim_xo_cap & Si2158_CRYSTAL_TRIM_PROP_XO_CAP_MASK) << Si2158_CRYSTAL_TRIM_PROP_XO_CAP_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_CRYSTAL_TRIM_PROP_CODE, si2158_crystal_trim_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_master_ien_prop_data = (master_ien_tunien & Si2158_MASTER_IEN_PROP_TUNIEN_MASK) << Si2158_MASTER_IEN_PROP_TUNIEN_LSB  |
		   (master_ien_atvien & Si2158_MASTER_IEN_PROP_ATVIEN_MASK) << Si2158_MASTER_IEN_PROP_ATVIEN_LSB  |
		   (master_ien_dtvien & Si2158_MASTER_IEN_PROP_DTVIEN_MASK) << Si2158_MASTER_IEN_PROP_DTVIEN_LSB  |
		   (master_ien_errien & Si2158_MASTER_IEN_PROP_ERRIEN_MASK) << Si2158_MASTER_IEN_PROP_ERRIEN_LSB  |
		   (master_ien_ctsien & Si2158_MASTER_IEN_PROP_CTSIEN_MASK) << Si2158_MASTER_IEN_PROP_CTSIEN_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_MASTER_IEN_PROP_CODE, si2158_master_ien_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_xout_prop_data = (xout_amp & Si2158_XOUT_PROP_AMP_MASK) << Si2158_XOUT_PROP_AMP_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_XOUT_PROP_CODE, si2158_xout_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/*****************************************************************************************
NAME: silabs_tercap_download_dtv_properties
 DESCRIPTION: Setup Si2158 DTV properties configuration
 This function will download all the DTV configuration properties.
 The function SetupDTVDefaults() should be called before the first call to this function.
 Parameter:  Pointer to Si2158 Context
 Returns:    I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 Programming Guide Reference:    DTV setup flowchart
 ******************************************************************************************/
static int silabs_tercap_download_dtv_properties(silabs_tercab_context *ctx)
{
	const u8  dtv_agc_freeze_input_level          = Si2158_DTV_AGC_FREEZE_INPUT_PROP_LEVEL_LOW  ; /* (default 'LOW') */
	const u8  dtv_agc_freeze_input_pin            = Si2158_DTV_AGC_FREEZE_INPUT_PROP_PIN_NONE   ; /* (default 'NONE') */
	const u8  dtv_agc_speed_agc_decim             = Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF     ; /* (default 'OFF') */
	const u8  dtv_agc_speed_if_agc_speed          = Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO ; /* (default 'AUTO') */
	const u8  dtv_config_if_port_dtv_agc_source   = Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_AGC_SOURCE_AGC2_3DB ; /* (default 'INTERNAL') */
	const u8  dtv_config_if_port_dtv_out_type     = Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_OUT_TYPE_LIF_IF2   ; /* (default 'LIF_IF2') */
	const u8  dtv_ext_agc_max_10mv                =   200; /* (default   200) */
	const u8  dtv_ext_agc_min_10mv                =    50; /* (default    50) */
	const u8  dtv_filter_select_filter            = Si2158_DTV_FILTER_SELECT_PROP_FILTER_CUSTOM1 ; /* (default 'CUSTOM1') */
	const u8  dtv_ien_chlien                      = Si2158_DTV_IEN_PROP_CHLIEN_ENABLE ; /* (default 'ENABLE') */
	const u8  dtv_initial_agc_speed_agc_decim     = Si2158_DTV_INITIAL_AGC_SPEED_PROP_AGC_DECIM_OFF     ; /* (default 'OFF') */
	const u8  dtv_initial_agc_speed_if_agc_speed  = Si2158_DTV_INITIAL_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO ; /* (default 'AUTO') */
	const u16 dtv_initial_agc_speed_period        =     0; /* (default     0) */
	const u8  dtv_internal_zif_atsc               = Si2158_DTV_INTERNAL_ZIF_PROP_ATSC_LIF   ; /* (default 'LIF') */
	const u8  dtv_internal_zif_dtmb               = Si2158_DTV_INTERNAL_ZIF_PROP_DTMB_LIF   ; /* (default 'LIF') */
	const u8  dtv_internal_zif_dvbc               = Si2158_DTV_INTERNAL_ZIF_PROP_DVBC_LIF   ; /* (default 'LIF') */
	const u8  dtv_internal_zif_dvbt               = Si2158_DTV_INTERNAL_ZIF_PROP_DVBT_LIF   ; /* (default 'LIF') */
	const u8  dtv_internal_zif_isdbc              = Si2158_DTV_INTERNAL_ZIF_PROP_ISDBC_LIF  ; /* (default 'LIF') */
	const u8  dtv_internal_zif_isdbt              = Si2158_DTV_INTERNAL_ZIF_PROP_ISDBT_LIF  ; /* (default 'LIF') */
	const u8  dtv_internal_zif_qam_us             = Si2158_DTV_INTERNAL_ZIF_PROP_QAM_US_LIF ; /* (default 'LIF') */
	const u8  dtv_int_sense_chlnegen              = Si2158_DTV_INT_SENSE_PROP_CHLNEGEN_DISABLE ; /* (default 'DISABLE') */
	const u8  dtv_int_sense_chlposen              = Si2158_DTV_INT_SENSE_PROP_CHLPOSEN_ENABLE  ; /* (default 'ENABLE') */
	const u16 dtv_lif_freq_offset                 =  4000; /* (default  5000) */
	const u8  dtv_lif_out_amp                     =    27; /* (default    27) */
	const u8  dtv_lif_out_offset                  =   148; /* (default   148) */
	const u8  dtv_mode_bw                         = Si2158_DTV_MODE_PROP_BW_BW_6MHZ              ; /* (default 'BW_8MHZ') */
	const u8  dtv_mode_invert_spectrum            = Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_NORMAL  ; /* (default 'NORMAL') */
	const u8  dtv_mode_modulation                 = Si2158_DTV_MODE_PROP_MODULATION_ATSC         ; /* (default 'DVBT') */
	const s8  dtv_pga_limits_max                  =    -1; /* (default    -1) */
	const s8  dtv_pga_limits_min                  =    -1; /* (default    -1) */
	const u8  dtv_pga_target_override_enable      = Si2158_DTV_PGA_TARGET_PROP_OVERRIDE_ENABLE_DISABLE ; /* (default 'DISABLE') */
	const s8  dtv_pga_target_pga_target           =     0; /* (default     0) */
	const u8  dtv_rf_top_dtv_rf_top               = Si2158_DTV_RF_TOP_PROP_DTV_RF_TOP_AUTO ; /* (default 'AUTO') */
	const s8  dtv_rsq_rssi_threshold_hi           =     0; /* (default     0) */
	const s8  dtv_rsq_rssi_threshold_lo           =   -80; /* (default   -80) */
	const u8  dtv_zif_dc_canceller_bw_bandwidth   = Si2158_DTV_ZIF_DC_CANCELLER_BW_PROP_BANDWIDTH_DEFAULT ; /* (default 'DEFAULT') */

	u16 si2158_dtv_agc_freeze_input_prop_data;
	u16 si2158_dtv_agc_speed_prop_data;
	u16 si2158_dtv_config_if_port_prop_data;
	u16 si2158_dtv_ext_agc_prop_data;
	u16 si2158_dtv_filter_select_prop_data;
	u16 si2158_dtv_ien_prop_data;
	u16 si2158_dtv_initial_agc_speed_prop_data;
	u16 si2158_dtv_initial_agc_speed_period_prop_data;
	u16 si2158_dtv_internal_zif_prop_data;
	u16 si2158_dtv_int_sense_prop_data;
	u16 si2158_dtv_lif_freq_prop_data;
	u16 si2158_dtv_lif_out_prop_data;
	u16 si2158_dtv_mode_prop_data;
	u16 si2158_dtv_pga_limits_prop_data;
	u16 si2158_dtv_pga_target_prop_data;
	u16 si2158_dtv_rf_top_prop_data;
	u16 si2158_dtv_rsq_rssi_threshold_prop_data;
	u16 si2158_dtv_zif_dc_canceller_bw_prop_data;

	SiTRACE("%s()\n", __func__);

	si2158_dtv_agc_freeze_input_prop_data = (dtv_agc_freeze_input_level & Si2158_DTV_AGC_FREEZE_INPUT_PROP_LEVEL_MASK) << Si2158_DTV_AGC_FREEZE_INPUT_PROP_LEVEL_LSB  |
		   (dtv_agc_freeze_input_pin   & Si2158_DTV_AGC_FREEZE_INPUT_PROP_PIN_MASK  ) << Si2158_DTV_AGC_FREEZE_INPUT_PROP_PIN_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_AGC_FREEZE_INPUT_PROP_CODE, si2158_dtv_agc_freeze_input_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_agc_speed_prop_data = (dtv_agc_speed_if_agc_speed & Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK) << Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB  |
		   (dtv_agc_speed_agc_decim    & Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_MASK   ) << Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_AGC_SPEED_PROP_CODE, si2158_dtv_agc_speed_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_config_if_port_prop_data = (dtv_config_if_port_dtv_out_type   & Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_OUT_TYPE_MASK  ) << Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_OUT_TYPE_LSB  |
		   (dtv_config_if_port_dtv_agc_source & Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_AGC_SOURCE_MASK) << Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_AGC_SOURCE_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_CONFIG_IF_PORT_PROP_CODE, si2158_dtv_config_if_port_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_ext_agc_prop_data = (dtv_ext_agc_min_10mv & Si2158_DTV_EXT_AGC_PROP_MIN_10MV_MASK) << Si2158_DTV_EXT_AGC_PROP_MIN_10MV_LSB  |
		   (dtv_ext_agc_max_10mv & Si2158_DTV_EXT_AGC_PROP_MAX_10MV_MASK) << Si2158_DTV_EXT_AGC_PROP_MAX_10MV_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_EXT_AGC_PROP_CODE, si2158_dtv_ext_agc_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	switch (ctx->part) {
	case 58:
		si2158_dtv_filter_select_prop_data = (dtv_filter_select_filter & Si2158_DTV_FILTER_SELECT_PROP_FILTER_MASK) << Si2158_DTV_FILTER_SELECT_PROP_FILTER_LSB ;
		if (silabs_tercab_set_property(ctx, Si2158_DTV_FILTER_SELECT_PROP_CODE, si2158_dtv_filter_select_prop_data) != NO_SILABS_TERCAB_ERROR) {
			return ERROR_SILABS_TERCAB_SENDING_COMMAND;
		}
		break;
	default:
		break;
	}

	si2158_dtv_ien_prop_data = (dtv_ien_chlien & Si2158_DTV_IEN_PROP_CHLIEN_MASK) << Si2158_DTV_IEN_PROP_CHLIEN_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_IEN_PROP_CODE, si2158_dtv_ien_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_initial_agc_speed_prop_data = (dtv_initial_agc_speed_if_agc_speed & Si2158_DTV_INITIAL_AGC_SPEED_PROP_IF_AGC_SPEED_MASK) << Si2158_DTV_INITIAL_AGC_SPEED_PROP_IF_AGC_SPEED_LSB  |
		   (dtv_initial_agc_speed_agc_decim    & Si2158_DTV_INITIAL_AGC_SPEED_PROP_AGC_DECIM_MASK   ) << Si2158_DTV_INITIAL_AGC_SPEED_PROP_AGC_DECIM_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_INITIAL_AGC_SPEED_PROP_CODE, si2158_dtv_initial_agc_speed_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_initial_agc_speed_period_prop_data = (dtv_initial_agc_speed_period & Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_PERIOD_MASK) << Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_PERIOD_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_CODE, si2158_dtv_initial_agc_speed_period_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_internal_zif_prop_data = (dtv_internal_zif_atsc   & Si2158_DTV_INTERNAL_ZIF_PROP_ATSC_MASK  ) << Si2158_DTV_INTERNAL_ZIF_PROP_ATSC_LSB  |
		   (dtv_internal_zif_qam_us & Si2158_DTV_INTERNAL_ZIF_PROP_QAM_US_MASK) << Si2158_DTV_INTERNAL_ZIF_PROP_QAM_US_LSB  |
		   (dtv_internal_zif_dvbt   & Si2158_DTV_INTERNAL_ZIF_PROP_DVBT_MASK  ) << Si2158_DTV_INTERNAL_ZIF_PROP_DVBT_LSB  |
		   (dtv_internal_zif_dvbc   & Si2158_DTV_INTERNAL_ZIF_PROP_DVBC_MASK  ) << Si2158_DTV_INTERNAL_ZIF_PROP_DVBC_LSB  |
		   (dtv_internal_zif_isdbt  & Si2158_DTV_INTERNAL_ZIF_PROP_ISDBT_MASK ) << Si2158_DTV_INTERNAL_ZIF_PROP_ISDBT_LSB  |
		   (dtv_internal_zif_isdbc  & Si2158_DTV_INTERNAL_ZIF_PROP_ISDBC_MASK ) << Si2158_DTV_INTERNAL_ZIF_PROP_ISDBC_LSB  |
		   (dtv_internal_zif_dtmb   & Si2158_DTV_INTERNAL_ZIF_PROP_DTMB_MASK  ) << Si2158_DTV_INTERNAL_ZIF_PROP_DTMB_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_INTERNAL_ZIF_PROP_CODE, si2158_dtv_internal_zif_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_int_sense_prop_data = (dtv_int_sense_chlnegen & Si2158_DTV_INT_SENSE_PROP_CHLNEGEN_MASK) << Si2158_DTV_INT_SENSE_PROP_CHLNEGEN_LSB  |
		   (dtv_int_sense_chlposen & Si2158_DTV_INT_SENSE_PROP_CHLPOSEN_MASK) << Si2158_DTV_INT_SENSE_PROP_CHLPOSEN_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_INT_SENSE_PROP_CODE, si2158_dtv_int_sense_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_lif_freq_prop_data = (dtv_lif_freq_offset & Si2158_DTV_LIF_FREQ_PROP_OFFSET_MASK) << Si2158_DTV_LIF_FREQ_PROP_OFFSET_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_LIF_FREQ_PROP_CODE, si2158_dtv_lif_freq_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_lif_out_prop_data = (dtv_lif_out_offset & Si2158_DTV_LIF_OUT_PROP_OFFSET_MASK) << Si2158_DTV_LIF_OUT_PROP_OFFSET_LSB  |
		   (dtv_lif_out_amp    & Si2158_DTV_LIF_OUT_PROP_AMP_MASK   ) << Si2158_DTV_LIF_OUT_PROP_AMP_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_LIF_OUT_PROP_CODE, si2158_dtv_lif_out_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_mode_prop_data = (dtv_mode_bw              & Si2158_DTV_MODE_PROP_BW_MASK             ) << Si2158_DTV_MODE_PROP_BW_LSB  |
		   (dtv_mode_modulation      & Si2158_DTV_MODE_PROP_MODULATION_MASK     ) << Si2158_DTV_MODE_PROP_MODULATION_LSB  |
		   (dtv_mode_invert_spectrum & Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_MODE_PROP_CODE, si2158_dtv_mode_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_pga_limits_prop_data = (dtv_pga_limits_min & Si2158_DTV_PGA_LIMITS_PROP_MIN_MASK) << Si2158_DTV_PGA_LIMITS_PROP_MIN_LSB  |
		   (dtv_pga_limits_max & Si2158_DTV_PGA_LIMITS_PROP_MAX_MASK) << Si2158_DTV_PGA_LIMITS_PROP_MAX_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_PGA_LIMITS_PROP_CODE, si2158_dtv_pga_limits_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_pga_target_prop_data = (dtv_pga_target_pga_target      & Si2158_DTV_PGA_TARGET_PROP_PGA_TARGET_MASK     ) << Si2158_DTV_PGA_TARGET_PROP_PGA_TARGET_LSB  |
		   (dtv_pga_target_override_enable & Si2158_DTV_PGA_TARGET_PROP_OVERRIDE_ENABLE_MASK) << Si2158_DTV_PGA_TARGET_PROP_OVERRIDE_ENABLE_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_PGA_TARGET_PROP_CODE, si2158_dtv_pga_target_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_rf_top_prop_data = (dtv_rf_top_dtv_rf_top & Si2158_DTV_RF_TOP_PROP_DTV_RF_TOP_MASK) << Si2158_DTV_RF_TOP_PROP_DTV_RF_TOP_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_RF_TOP_PROP_CODE, si2158_dtv_rf_top_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_rsq_rssi_threshold_prop_data = (dtv_rsq_rssi_threshold_lo & Si2158_DTV_RSQ_RSSI_THRESHOLD_PROP_LO_MASK) << Si2158_DTV_RSQ_RSSI_THRESHOLD_PROP_LO_LSB  |
		   (dtv_rsq_rssi_threshold_hi & Si2158_DTV_RSQ_RSSI_THRESHOLD_PROP_HI_MASK) << Si2158_DTV_RSQ_RSSI_THRESHOLD_PROP_HI_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_RSQ_RSSI_THRESHOLD_PROP_CODE, si2158_dtv_rsq_rssi_threshold_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_dtv_zif_dc_canceller_bw_prop_data = (dtv_zif_dc_canceller_bw_bandwidth & Si2158_DTV_ZIF_DC_CANCELLER_BW_PROP_BANDWIDTH_MASK) << Si2158_DTV_ZIF_DC_CANCELLER_BW_PROP_BANDWIDTH_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_ZIF_DC_CANCELLER_BW_PROP_CODE, si2158_dtv_zif_dc_canceller_bw_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/*****************************************************************************************
NAME: silabs_tercap_download_tuner_properties
 DESCRIPTION: Setup Si2158 TUNER properties configuration
 This function will download all the TUNER configuration properties.
 The function SetupTUNERDefaults() should be called before the first call to this function.
 Parameter:  Pointer to Si2158 Context
 Returns:    I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 Programming Guide Reference:    TUNER setup flowchart
 ******************************************************************************************/
static int silabs_tercap_download_tuner_properties(silabs_tercab_context *ctx)
{
	const s16 tuner_blocked_vco_vco_code = 0x8000; /* (default 0x8000) */
	const u8  tuner_ien_rssihien         = Si2158_TUNER_IEN_PROP_RSSIHIEN_DISABLE;          /* (default 'DISABLE') */
	const u8  tuner_ien_rssilien         = Si2158_TUNER_IEN_PROP_RSSILIEN_DISABLE;          /* (default 'DISABLE') */
	const u8  tuner_ien_tcien            = Si2158_TUNER_IEN_PROP_TCIEN_ENABLE;              /* (default 'ENABLE') */
	const u8  tuner_int_sense_tcnegen    = Si2158_TUNER_INT_SENSE_PROP_TCNEGEN_DISABLE;     /* (default 'DISABLE') */
	const u8  tuner_int_sense_rssilnegen = Si2158_TUNER_INT_SENSE_PROP_RSSILNEGEN_DISABLE;  /* (default 'DISABLE') */
	const u8  tuner_int_sense_rssihnegen = Si2158_TUNER_INT_SENSE_PROP_RSSIHNEGEN_DISABLE;  /* (default 'DISABLE') */
	const u8  tuner_int_sense_tcposen    = Si2158_TUNER_INT_SENSE_PROP_TCPOSEN_ENABLE;      /* (default 'ENABLE') */
	const u8  tuner_int_sense_rssilposen = Si2158_TUNER_INT_SENSE_PROP_RSSILPOSEN_ENABLE;   /* (default 'ENABLE') */
	const u8  tuner_int_sense_rssihposen = Si2158_TUNER_INT_SENSE_PROP_RSSIHPOSEN_ENABLE;   /* (default 'ENABLE') */
	const u8  tuner_lo_injection_band_1  = Si2158_TUNER_LO_INJECTION_PROP_BAND_1_HIGH_SIDE; /* (default 'HIGH_SIDE') */
	const u8  tuner_lo_injection_band_2  = Si2158_TUNER_LO_INJECTION_PROP_BAND_2_LOW_SIDE;  /* (default 'LOW_SIDE') */
	const u8  tuner_lo_injection_band_3  = Si2158_TUNER_LO_INJECTION_PROP_BAND_3_LOW_SIDE;  /* (default 'LOW_SIDE') */
	const u8  tuner_return_loss_config   = Si2158_TUNER_RETURN_LOSS_PROP_CONFIG_127;        /* (default '127') */
	const u8  tuner_return_loss_mode     = Si2158_TUNER_RETURN_LOSS_PROP_MODE_TERRESTRIAL;  /* (default 'TERRESTRIAL') */

	u16 si2158_tuner_blocked_vco_prop_data;
	u16 si2158_tuner_ien_prop_data;
	u16 si2158_tuner_int_sense_prop_data;
	u16 si2158_tuner_lo_injection_prop_data;
	u16 si2158_tuner_return_loss_prop_data;

	SiTRACE("%s()\n", __func__);

	si2158_tuner_blocked_vco_prop_data = (tuner_blocked_vco_vco_code & Si2158_TUNER_BLOCKED_VCO_PROP_VCO_CODE_MASK) << Si2158_TUNER_BLOCKED_VCO_PROP_VCO_CODE_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_TUNER_BLOCKED_VCO_PROP_CODE, si2158_tuner_blocked_vco_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_tuner_ien_prop_data = (tuner_ien_tcien    & Si2158_TUNER_IEN_PROP_TCIEN_MASK   ) << Si2158_TUNER_IEN_PROP_TCIEN_LSB  |
		   (tuner_ien_rssilien & Si2158_TUNER_IEN_PROP_RSSILIEN_MASK) << Si2158_TUNER_IEN_PROP_RSSILIEN_LSB  |
		   (tuner_ien_rssihien & Si2158_TUNER_IEN_PROP_RSSIHIEN_MASK) << Si2158_TUNER_IEN_PROP_RSSIHIEN_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_TUNER_IEN_PROP_CODE, si2158_tuner_ien_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_tuner_int_sense_prop_data = (tuner_int_sense_tcnegen    & Si2158_TUNER_INT_SENSE_PROP_TCNEGEN_MASK   ) << Si2158_TUNER_INT_SENSE_PROP_TCNEGEN_LSB  |
		   (tuner_int_sense_rssilnegen & Si2158_TUNER_INT_SENSE_PROP_RSSILNEGEN_MASK) << Si2158_TUNER_INT_SENSE_PROP_RSSILNEGEN_LSB  |
		   (tuner_int_sense_rssihnegen & Si2158_TUNER_INT_SENSE_PROP_RSSIHNEGEN_MASK) << Si2158_TUNER_INT_SENSE_PROP_RSSIHNEGEN_LSB  |
		   (tuner_int_sense_tcposen    & Si2158_TUNER_INT_SENSE_PROP_TCPOSEN_MASK   ) << Si2158_TUNER_INT_SENSE_PROP_TCPOSEN_LSB  |
		   (tuner_int_sense_rssilposen & Si2158_TUNER_INT_SENSE_PROP_RSSILPOSEN_MASK) << Si2158_TUNER_INT_SENSE_PROP_RSSILPOSEN_LSB  |
		   (tuner_int_sense_rssihposen & Si2158_TUNER_INT_SENSE_PROP_RSSIHPOSEN_MASK) << Si2158_TUNER_INT_SENSE_PROP_RSSIHPOSEN_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_TUNER_INT_SENSE_PROP_CODE, si2158_tuner_int_sense_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_tuner_lo_injection_prop_data = (tuner_lo_injection_band_1 & Si2158_TUNER_LO_INJECTION_PROP_BAND_1_MASK) << Si2158_TUNER_LO_INJECTION_PROP_BAND_1_LSB  |
		   (tuner_lo_injection_band_2 & Si2158_TUNER_LO_INJECTION_PROP_BAND_2_MASK) << Si2158_TUNER_LO_INJECTION_PROP_BAND_2_LSB  |
		   (tuner_lo_injection_band_3 & Si2158_TUNER_LO_INJECTION_PROP_BAND_3_MASK) << Si2158_TUNER_LO_INJECTION_PROP_BAND_3_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_TUNER_LO_INJECTION_PROP_CODE, si2158_tuner_lo_injection_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	si2158_tuner_return_loss_prop_data = (tuner_return_loss_config & Si2158_TUNER_RETURN_LOSS_PROP_CONFIG_MASK) << Si2158_TUNER_RETURN_LOSS_PROP_CONFIG_LSB  |
		   (tuner_return_loss_mode   & Si2158_TUNER_RETURN_LOSS_PROP_MODE_MASK  ) << Si2158_TUNER_RETURN_LOSS_PROP_MODE_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_TUNER_RETURN_LOSS_PROP_CODE, si2158_tuner_return_loss_prop_data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	return NO_SILABS_TERCAB_ERROR;
}

static int silabs_tercap_download_all_properties(silabs_tercab_context *ctx)
{
	struct silabs_tercab_priv *priv = NULL; /* for debug output only */
	int err = NO_SILABS_TERCAB_ERROR;
	int ret = silabs_tercab_download_atv_properties(ctx);
	if (ret) {
		err = ret;
		silabs_tercab_err("ERROR: download all ATV properties failed=0x%x\n", err);
	}
	ret = silabs_tercap_download_common_properties(ctx);
	if (ret) {
		err = ret;
		silabs_tercab_err("ERROR: download all COMMON properties failed=0x%x\n", err);
	}
	ret = silabs_tercap_download_dtv_properties(ctx);
	if (ret) {
		err = ret;
		silabs_tercab_err("ERROR: download all DTV properties failed=0x%x\n", err);
	}
	ret = silabs_tercap_download_tuner_properties(ctx);
	if (ret) {
		err = ret;
		silabs_tercab_err("ERROR: download all TUNER properties failed=0x%x\n", err);
	}
	return err;
}

/************************************************************************************************************************
  NAME: silabs_tercab_configure
  DESCRIPTION: Setup Si2158 video filters, GPIOs/clocks, Common Properties startup, etc.
  Parameter:  Pointer to Si2158 Context
  Returns:    I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 ************************************************************************************************************************/
static int silabs_tercab_configure(silabs_tercab_context *ctx)
{
	/* Set All Properties startup configuration */
	return silabs_tercap_download_all_properties(ctx);
}

/************************************************************************************************************************
  NAME: silabs_tercab_init
  DESCRIPTION:Reset and Initialize Si2158
  Parameter:  Si2158 Context (I2C address)
  Returns:    I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 ************************************************************************************************************************/
static int silabs_tercab_init(struct silabs_tercab_priv *priv)
{
	silabs_tercab_context *ctx = &priv->tuner;
	int return_code;
	SiTRACE("silabs_tercab_init starting...\n");

	/* PowerUp into bootloader */
	return_code = silabs_tercab_power_up_with_patch(priv);
	if (return_code != NO_SILABS_TERCAB_ERROR) {
		SiTRACE ("silabs_tercab_power_up_with_patch error 0x%02x: %s\n", return_code, silabs_tercab_error_text(return_code) );
		return return_code;
	}
	/* At this point, FW is loaded and started.  */
	silabs_tercab_configure(ctx);
	SiTRACE("silabs_tercab_init complete...\n");
	return NO_SILABS_TERCAB_ERROR;
}

/*---------------------------------------------------*/
/* Si2158_CONFIG_CLOCKS COMMAND                    */
/*---------------------------------------------------*/
static u8 silabs_tercab_config_clocks(silabs_tercab_context *ctx,
		u8   subcode,
		u8   clock_mode,
		u8   en_xout)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[3];
	u8 rspByteBuffer[1];

	SiTRACE("%s()\n", __func__);

	cmdByteBuffer[0] = Si2158_CONFIG_CLOCKS_CMD;
	cmdByteBuffer[1] = (u8) ( ( subcode    & Si2158_CONFIG_CLOCKS_CMD_SUBCODE_MASK    ) << Si2158_CONFIG_CLOCKS_CMD_SUBCODE_LSB   );
	cmdByteBuffer[2] = (u8) ( ( clock_mode & Si2158_CONFIG_CLOCKS_CMD_CLOCK_MODE_MASK ) << Si2158_CONFIG_CLOCKS_CMD_CLOCK_MODE_LSB|
			                  ( en_xout    & Si2158_CONFIG_CLOCKS_CMD_EN_XOUT_MASK    ) << Si2158_CONFIG_CLOCKS_CMD_EN_XOUT_LSB   );

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 3, cmdByteBuffer) != 3) {
		SiTRACE("Error writing CONFIG_CLOCKS bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, 1, rspByteBuffer, &ctx->status);
	if (error_code) {
		SiTRACE("Error polling CONFIG_CLOCKS response\n");
		return error_code;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/************************************************************************************************************************
  NAME: silabs_tercab_xout_on
  Parameter:  Pointer to Si2158 Context (I2C address)
  Returns:    I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 ************************************************************************************************************************/
static int silabs_tercab_xout_on(silabs_tercab_context *ctx)
{
	SiTRACE("Turning Xout ON\n");

	return silabs_tercab_config_clocks(ctx,
				Si2158_CONFIG_CLOCKS_CMD_SUBCODE_CODE,
				Si2158_CONFIG_CLOCKS_CMD_CLOCK_MODE_XTAL,
				Si2158_CONFIG_CLOCKS_CMD_EN_XOUT_EN_XOUT);
}

/************************************************************************************************************************
  NAME: silabs_tercab_xout_off
  Parameter:  Pointer to Si2158 Context (I2C address)
  Returns:    I2C transaction error code, NO_SILABS_TERCAB_ERROR if successful
 ************************************************************************************************************************/
static int silabs_tercab_xout_off(silabs_tercab_context *ctx)
{
	SiTRACE("Turning Xout OFF\n");

	return silabs_tercab_config_clocks(ctx,
			Si2158_CONFIG_CLOCKS_CMD_SUBCODE_CODE,
			Si2158_CONFIG_CLOCKS_CMD_CLOCK_MODE_XTAL,
			Si2158_CONFIG_CLOCKS_CMD_EN_XOUT_DIS_XOUT);
}

static int silabs_tercab_initialize(struct dvb_frontend *fe)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	int ter_clock_needed = 1;
	int res = 0;
	int ret = 0;
	u16 data;

	mutex_lock(&priv->lock);

	/* Do a full init of the Ter Tuner only if it has not been already done */
	if (priv->silabs_tercab_init_done==0) {
		silabs_tercab_info("%s()\n", __func__);
		SiTRACE("Init terrestrial tuner @ 0x%02X\n", priv->tuner.i2c_addr);
		if ((res= silabs_tercab_init(priv)) != NO_SILABS_TERCAB_ERROR) {
			silabs_tercab_err("silabs_tercab_init() failed.\n");
			SiTRACE("Terrestrial tuner HW init error: 0x%02x : %s\n",res, silabs_tercab_error_text(res) );
			ret = -ENODEV;
			goto fail;
		}
		priv->tuner.part = priv->tuner.rsp.part_info.part;
		priv->tuner.chiprev = priv->tuner.rsp.part_info.chiprev;
		silabs_tercab_info("Silicon Labs tuner Si21%u rev. %u detected\n", priv->tuner.part, priv->tuner.chiprev);
		if (priv->agc_control) {
			data = (Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_OUT_TYPE_LIF_IF1 & Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_OUT_TYPE_MASK  ) << Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_OUT_TYPE_LSB  |
				   (Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_AGC_SOURCE_INTERNAL & Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_AGC_SOURCE_MASK) << Si2158_DTV_CONFIG_IF_PORT_PROP_DTV_AGC_SOURCE_LSB ;
			silabs_tercab_set_property(&priv->tuner, Si2158_DTV_CONFIG_IF_PORT_PROP_CODE, data);
		}
		data = (priv->crystal_trim_xo_cap & Si2158_CRYSTAL_TRIM_PROP_XO_CAP_MASK) << Si2158_CRYSTAL_TRIM_PROP_XO_CAP_LSB;
		if (silabs_tercab_set_property(&priv->tuner, Si2158_CRYSTAL_TRIM_PROP_CODE, data) != NO_SILABS_TERCAB_ERROR) {
			ret = -EIO;
			goto fail;
		}
	} else {
		SiTRACE("Wakeup terrestrial tuner\n");
		if ((res= silabs_tercab_poll_cts(priv->i2c_props.adap, priv->i2c_props.addr)) != NO_SILABS_TERCAB_ERROR) {
			silabs_tercab_err("tuner Si2158 wake up failed.\n");
			SiTRACE("Terrestrial tuner wake up error: 0x%02x : %s\n",res, silabs_tercab_error_text(res) );
			ret = -ENODEV;
			goto fail;
		}
	}

	/* ------------------------------------------------------------ */
	/* If the terrestrial tuner's clock is required, activate it    */
	/* ------------------------------------------------------------ */
	SiTRACE("ter_clock_needed %d\n",ter_clock_needed);
	if (ter_clock_needed) {
		SiTRACE("Turn terrestrial tuner clock on\n");
		if (priv->clock_control != 0 /* Si2168B_CLOCK_ALWAYS_OFF */) {
			SiTRACE("Terrestrial tuner CLOCK ON\n");
			if ((res = silabs_tercab_xout_on(&priv->tuner) ) != 0) {
				SiTRACE("Terrestrial tuner CLOCK ON error: 0x%02x : %s\n",res, silabs_tercab_error_text(res) );
				ret = -ENODEV;
				goto fail;
			}
		}
	}

fail:
	mutex_unlock(&priv->lock);

	if (ret==0) {
		priv->silabs_tercab_init_done = 1;
		silabs_tercab_dbg("initializing tuner succeeded\n");
	} else {
		priv->silabs_tercab_init_done = 0;
	}

	return ret;
}

#if 0
/*---------------------------------------------------*/
/* Si2158_STANDBY COMMAND                          */
/*---------------------------------------------------*/
static u8 silabs_tercab_standby(silabs_tercab_context *ctx,	u8 type)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[1];
	ctx->rsp.standby.STATUS = ctx->status;

	SiTRACE("%s()\n", __func__);

	cmdByteBuffer[0] = Si2158_STANDBY_CMD;
	cmdByteBuffer[1] = (u8) ( ( type & Si2158_STANDBY_CMD_TYPE_MASK ) << Si2158_STANDBY_CMD_TYPE_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing STANDBY bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = Si2158_pollForResponse(ctx, 1, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling STANDBY response\n");
		return error_code;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/*---------------------------------------------------*/
/* Si2158_POWER_DOWN COMMAND                       */
/*---------------------------------------------------*/
static u8 silabs_tercab_power_down(silabs_tercab_context *ctx)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[1];
	u8 rspByteBuffer[1];
	ctx->rsp.power_down.STATUS = ctx->status;

	SiTRACE("%s()\n", __func__);

	cmdByteBuffer[0] = Si2158_POWER_DOWN_CMD;

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 1, cmdByteBuffer) != 1) {
		SiTRACE("Error writing POWER_DOWN bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = Si2158_pollForResponse(ctx, 1, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling POWER_DOWN response\n");
		return error_code;
	}

	return NO_SILABS_TERCAB_ERROR;
}
#endif

static int silabs_tercab_sleep(struct dvb_frontend *fe)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	int res;
	int ret = 0;

	silabs_tercab_dbg("\n");

	mutex_lock(&priv->lock);

	if (priv->clock_control != 1 /* Si2168B_CLOCK_ALWAYS_ON */) {
		SiTRACE("Terrestrial tuner clock OFF\n");
		res = silabs_tercab_xout_off(&priv->tuner);
		if (res) {
			SiTRACE("Terrestrial tuner CLOCK OFF error: 0x%02x : %s\n",res, silabs_tercab_error_text(res) );
			ret = -EPERM;
		}
	}
#if 1
#if 0
#define  Si2158_STANDBY_CMD_TYPE_MIN 0
	priv->silabs_tercab_init_done = 0;
	priv->firmware_started = 0;
	SiTRACE("Terrestrial tuner STANDBY\n");
	if ((res= silabs_tercab_standby(&priv->tuner, Si2158_STANDBY_CMD_TYPE_MIN)) !=0 ) {
		SiTRACE("Terrestrial tuner Standby error: 0x%02x : %s\n",res, silabs_tercab_error_text(res) );
		ret = -EPERM;
	};
#else
	silabs_tercab_info("Standby ignored\n");
#endif
#else
	priv->silabs_tercab_init_done = 0;
	priv->firmware_started = 0;
	if (silabs_tercab_power_down(&priv->tuner) != NO_SILABS_TERCAB_ERROR) {
		silabs_tercab_err("silabs_tercab_power_down() failed\n");
		ret = -EPERM;
	}
#endif

	mutex_unlock(&priv->lock);

	return ret;
}

/************************************************************************************************************************
  silabs_tercab_fef function
  Use:        TER tuner FEF activation function
              Used to enable/disable the FEF mode in the terrestrial tuner
  Comments:   If the tuner is connected via the demodulator's I2C switch, enabling/disabling the i2c_passthru is required before/after tuning.
  Parameter:  *front_end, the front-end handle
  Parameter:  fef, a flag controlling the selection between FEF 'off'(0) and FEF 'on'(1)
  Returns:    1
 ************************************************************************************************************************/
static int silabs_tercab_fef(struct dvb_frontend *fe, int fef)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	u8  agc_freeze_input_pin;
	u8  agc_speed_agc_decim;
	u8  agc_speed_if_agc_speed;
	u16 si2158_dtv_agc_freeze_input__prop_data;
	u16 si2158_dtv_agc_speed__prop_data;

	SiTRACE("%s(): FEF MODE=%d FEF=%d\n", __func__, priv->fef_mode, fef);

	if (priv->fef_mode == Si2177_FEF_MODE_FREEZE_PIN) {
		SiTRACE("FEF mode Si2177_FEF_MODE_FREEZE_PIN\n");
		if (fef == 0) {
			agc_freeze_input_pin = Si2158_DTV_AGC_FREEZE_INPUT_PROP_PIN_NONE;
		} else {
			agc_freeze_input_pin = Si2158_DTV_AGC_FREEZE_INPUT_PROP_PIN_GPIO1;
		}
		si2158_dtv_agc_freeze_input__prop_data = (Si2158_DTV_AGC_FREEZE_INPUT_PROP_LEVEL_HIGH & Si2158_DTV_AGC_FREEZE_INPUT_PROP_LEVEL_MASK) << Si2158_DTV_AGC_FREEZE_INPUT_PROP_LEVEL_LSB  |
			   (agc_freeze_input_pin   & Si2158_DTV_AGC_FREEZE_INPUT_PROP_PIN_MASK  ) << Si2158_DTV_AGC_FREEZE_INPUT_PROP_PIN_LSB ;
		silabs_tercab_set_property(&priv->tuner,  Si2158_DTV_AGC_FREEZE_INPUT_PROP_CODE, si2158_dtv_agc_freeze_input__prop_data);
	}

#ifdef L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP
	if (priv->fef_mode == Si2177_FEF_MODE_SLOW_INITIAL_AGC) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_SLOW_INITIAL_AGC (AGC slowed down after tuning)\n");
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP */

#ifdef L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC_SETUP
	if (priv->fef_mode == Si2177_FEF_MODE_SLOW_NORMAL_AGC) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_SLOW_NORMAL_AGC: AGC slowed down\n");
		if (fef == 0) {
			agc_speed_agc_decim    = Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF;
			agc_speed_if_agc_speed = Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
		} else {
			agc_speed_if_agc_speed = Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_39;
			agc_speed_agc_decim    = Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_4;
		}
		si2158_dtv_agc_speed__prop_data = (agc_speed_if_agc_speed & Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK) << Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB  |
			   (agc_speed_agc_decim    & Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_MASK   ) << Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_DTV_AGC_SPEED_PROP_CODE, si2158_dtv_agc_speed__prop_data);
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC_SETUP */
	SiTRACE("silabs_tercab_fef done\n");
	return 1;
}

/************************************************************************************************************************
  silabs_tercab_fef_setup function
  Use:        TER tuner LPF setting function
              Used to configure the FEF mode in the terrestrial tuner
  Comments:   If the tuner is connected via the demodulator's I2C switch, enabling/disabling the i2c_passthru is required before/after tuning.
  Behavior:   This function closes the Si2168B's I2C switch then sets the TER FEF mode and finally reopens the I2C switch
  Parameter:  *front_end, the front-end handle
  Parameter:  fef, a flag controlling the selection between FEF 'off'(0) and FEF 'on'(1)
  Returns:    1
 ************************************************************************************************************************/
static int silabs_tercab_fef_setup(struct dvb_frontend *fe, int fef)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	u8  initial_agc_speed_agc_decim;
	u8  initial_agc_speed_if_agc_speed;
	u16 initial_agc_speed_period;
	u8  agc_speed_agc_decim;
	u8  agc_speed_if_agc_speed;
	u16 initial_agc_speed_period_prop;
	u16 agc_speed_prop;

	SiTRACE("%s(): FEF=%d\n", __func__, fef);

#ifdef L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN_SETUP
	if (priv->fef_mode == Si2177_FEF_MODE_FREEZE_PIN) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_FREEZE_PIN\n");
		initial_agc_speed_agc_decim    = Si2158_DTV_INITIAL_AGC_SPEED_PROP_AGC_DECIM_OFF;
		initial_agc_speed_if_agc_speed = Si2158_DTV_INITIAL_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
		initial_agc_speed_period       = 0;
		agc_speed_agc_decim            = Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF;
		agc_speed_if_agc_speed         = Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
		//Si2158_L1_SendCommand2(&priv->tuner, Si2158_CONFIG_PINS_CMD_CODE);
		silabs_tercab_config_pins (&priv->tuner,
				Si2158_CONFIG_PINS_CMD_GPIO1_MODE_DISABLE,
				Si2158_CONFIG_PINS_CMD_GPIO1_READ_DO_NOT_READ,
				Si2158_CONFIG_PINS_CMD_GPIO2_MODE_DISABLE,
				Si2158_CONFIG_PINS_CMD_GPIO2_READ_DO_NOT_READ,
				Si2158_CONFIG_PINS_CMD_RESERVED1_RESERVED,
				Si2158_CONFIG_PINS_CMD_RESERVED2_RESERVED,
				Si2158_CONFIG_PINS_CMD_RESERVED3_RESERVED);

		initial_agc_speed_period_prop = (initial_agc_speed_period & Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_PERIOD_MASK) << Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_PERIOD_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_CODE, initial_agc_speed_period_prop);

		agc_speed_prop = (agc_speed_if_agc_speed & Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK) << Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB  |
			   (agc_speed_agc_decim & Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_MASK) << Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_DTV_AGC_SPEED_PROP_CODE, agc_speed_prop);
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN_SETUP */

#ifdef L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP
	if (priv->fef_mode == Si2177_FEF_MODE_SLOW_INITIAL_AGC) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_SLOW_INITIAL_AGC (AGC slowed down after tuning)\n");
		if (fef == 0) {
			initial_agc_speed_agc_decim    = Si2158_DTV_INITIAL_AGC_SPEED_PROP_AGC_DECIM_OFF;
		    initial_agc_speed_if_agc_speed = Si2158_DTV_INITIAL_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
		    initial_agc_speed_period       = 0;
		    agc_speed_agc_decim            = Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF;
		    agc_speed_if_agc_speed         = Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
		} else {
			initial_agc_speed_agc_decim    = Si2158_DTV_INITIAL_AGC_SPEED_PROP_AGC_DECIM_OFF;
			initial_agc_speed_if_agc_speed = Si2158_DTV_INITIAL_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
			initial_agc_speed_period       = 240;
			agc_speed_agc_decim            = Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_4;
			agc_speed_if_agc_speed         = Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_39;
		}
		initial_agc_speed_period_prop = (initial_agc_speed_period & Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_PERIOD_MASK) << Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_PERIOD_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_CODE, initial_agc_speed_period_prop);

		agc_speed_prop = (agc_speed_if_agc_speed & Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK) << Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB  |
			   (agc_speed_agc_decim    & Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_MASK   ) << Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_DTV_AGC_SPEED_PROP_CODE, agc_speed_prop);
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP */

#ifdef L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC_SETUP
	if (priv->fef_mode == Si2177_FEF_MODE_SLOW_NORMAL_AGC ) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_SLOW_NORMAL_AGC: AGC slowed down\n");
		initial_agc_speed_period = 0;
		if (fef == 0) {
			agc_speed_agc_decim    = Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF;
			agc_speed_if_agc_speed = Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
		} else {
			agc_speed_if_agc_speed = Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_39;
			agc_speed_agc_decim    = Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_4;
		}
		initial_agc_speed_period_prop = (initial_agc_speed_period & Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_PERIOD_MASK) << Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_PERIOD_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_CODE, initial_agc_speed_period_prop);

		agc_speed_prop = (agc_speed_if_agc_speed & Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK) << Si2158_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB  |
			   (agc_speed_agc_decim & Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_MASK) << Si2158_DTV_AGC_SPEED_PROP_AGC_DECIM_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_DTV_AGC_SPEED_PROP_CODE, agc_speed_prop);
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC */

	silabs_tercab_fef(fe, fef);

	SiTRACE("%s done\n", __func__);
	return 1;
}

/*---------------------------------------------------*/
/* Si2158_TUNER_TUNE_FREQ COMMAND                  */
/*---------------------------------------------------*/
static u8 silabs_tercab_tune_freq(silabs_tercab_context *ctx, u8  mode, u32 freq)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[8];
	u8 rspByteBuffer[1];

	SiTRACE("%s()\n", __func__);

	cmdByteBuffer[0] = Si2158_TUNER_TUNE_FREQ_CMD;
	cmdByteBuffer[1] = (u8) ( ( mode & Si2158_TUNER_TUNE_FREQ_CMD_MODE_MASK ) << Si2158_TUNER_TUNE_FREQ_CMD_MODE_LSB);
	cmdByteBuffer[2] = (u8) 0x00;
	cmdByteBuffer[3] = (u8) 0x00;
	cmdByteBuffer[4] = (u8) ( ( freq & Si2158_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << Si2158_TUNER_TUNE_FREQ_CMD_FREQ_LSB);
	cmdByteBuffer[5] = (u8) ((( freq & Si2158_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << Si2158_TUNER_TUNE_FREQ_CMD_FREQ_LSB) >> 8);
	cmdByteBuffer[6] = (u8) ((( freq & Si2158_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << Si2158_TUNER_TUNE_FREQ_CMD_FREQ_LSB) >> 16);
	cmdByteBuffer[7] = (u8) ((( freq & Si2158_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << Si2158_TUNER_TUNE_FREQ_CMD_FREQ_LSB) >> 24);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 8, cmdByteBuffer) != 8) {
		SiTRACE("Error writing TUNER_TUNE_FREQ bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, 1, rspByteBuffer, &ctx->status);
	if (error_code) {
		SiTRACE("Error polling TUNER_TUNE_FREQ response\n");
		return error_code;
	}

	return NO_SILABS_TERCAB_ERROR;
}

/************************************************************************************************************************
 NAME: Si2158_Tune
 DESCRIPTIION: Tune Si2158 in specified mode (ATV/DTV) at center frequency, wait for TUNINT and xTVINT with timeout

 Parameter:  Pointer to Si2158 Context (I2C address)
 Parameter:  Mode (ATV or DTV) use Si2158_TUNER_TUNE_FREQ_CMD_MODE_ATV or Si2158_TUNER_TUNE_FREQ_CMD_MODE_DTV constants
 Parameter:  frequency (Hz) as a u32 integer
 Returns:    0 if channel found.  A nonzero value means either an error occurred or channel not locked.
 Programming Guide Reference:    Flowchart A.7 (Tune flowchart)
 ************************************************************************************************************************/
static int silabs_tercab_tune(silabs_tercab_context *ctx, u8 mode, u32 freq)
{
	unsigned int start_time  = 0;
	int return_code = 0;
	unsigned int timeout = 40;//FGR allow for PC clock granularity

	if (silabs_tercab_tune_freq(ctx, mode, freq) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	start_time = jiffies_to_msecs(jiffies);

	/* wait for TUNINT, timeout is 36ms */
	while ( (jiffies_to_msecs(jiffies) - start_time) < timeout )
	{
		if ((return_code = silabs_tercab_check_status(ctx)) != NO_SILABS_TERCAB_ERROR)
			return return_code;
		if (ctx->status.tunint)
			break;
        msleep(5);
	}
	if (!ctx->status.tunint) {
		SiTRACE("Timeout waiting for TUNINT\n");
		return ERROR_SILABS_TERCAB_TUNINT_TIMEOUT;
	}

	/* wait for xTVINT, timeout is 150ms for ATVINT and 10 ms for DTVINT */
	start_time = jiffies_to_msecs(jiffies);
	timeout    = ((mode==Si2158_TUNER_TUNE_FREQ_CMD_MODE_ATV) ? 300 : 100);//FGR allow for PC clock granularity
	while ( (jiffies_to_msecs(jiffies) - start_time) < timeout )
	{
		if ((return_code = silabs_tercab_check_status(ctx)) != NO_SILABS_TERCAB_ERROR)
			return return_code;
		if (mode == Si2158_TUNER_TUNE_FREQ_CMD_MODE_ATV) {
			if (ctx->status.atvint)
				break;
		} else {
			if (ctx->status.dtvint)
				break;
		}
        msleep(5);
	}

	if (mode == Si2158_TUNER_TUNE_FREQ_CMD_MODE_ATV) {
		if (ctx->status.atvint) {
			SiTRACE("ATV Tune Successful\n");
			return NO_SILABS_TERCAB_ERROR;
		} else {
			SiTRACE("Timeout waiting for ATVINT\n");
		}
	} else {
		if (ctx->status.dtvint) {
			SiTRACE("DTV Tune Successful\n");
			return NO_SILABS_TERCAB_ERROR;
		} else {
			SiTRACE("Timeout waiting for DTVINT\n");
		}
	}

	return ERROR_SILABS_TERCAB_xTVINT_TIMEOUT;
}

/************************************************************************************************************************
 NAME: silabs_tercab_dtv_tune
 DESCRIPTION: Update DTV_MODE and tune DTV mode at center frequency
 Parameter:  Pointer to Si2158 Context (I2C address)
 Parameter:  frequency (Hz)
 Parameter:  bandwidth , 6,7 or 8 MHz
 Parameter:  modulation,  e.g. use constant Si2158_DTV_MODE_PROP_MODULATION_DVBT for DVBT mode
 Parameter:  rsp - commandResp structure to returns tune status info.
 Returns:    I2C transaction error code, 0 if successful
 Programming Guide Reference:    Flowchart A.7 (Tune flowchart)
 ************************************************************************************************************************/
static int silabs_tercab_dtv_tune(silabs_tercab_context *ctx, u32 freq, u8 bw, u8 modulation, u8 invert_spectrum)
{
	u16 data;

	/* update DTV_MODE_PROP property */
	data = (bw              & Si2158_DTV_MODE_PROP_BW_MASK             ) << Si2158_DTV_MODE_PROP_BW_LSB  |
		   (modulation      & Si2158_DTV_MODE_PROP_MODULATION_MASK     ) << Si2158_DTV_MODE_PROP_MODULATION_LSB  |
		   (invert_spectrum & Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_LSB ;
	if (silabs_tercab_set_property(ctx, Si2158_DTV_MODE_PROP_CODE, data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	return silabs_tercab_tune(ctx, Si2158_TUNER_TUNE_FREQ_CMD_MODE_DTV, freq);
}

/* ------------------------------------------------------------------ */
#if 0
/*---------------------------------------------------*/
/* Si2177_GET_PROPERTY COMMAND                     */
/*---------------------------------------------------*/
static u8 silabs_tercab_get_property(silabs_tercab_context *ctx, u16 prop, u16 *data)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[4];
	u8 rspByteBuffer[4];
	u8 reserved = 0;

	SiTRACE("Si2177 GET_PROPERTY\n");

	cmdByteBuffer[0] = Si2158_GET_PROPERTY_CMD;
	cmdByteBuffer[1] = (u8) ( ( reserved & Si2158_GET_PROPERTY_CMD_RESERVED_MASK ) << Si2158_GET_PROPERTY_CMD_RESERVED_LSB);
	cmdByteBuffer[2] = (u8) ( ( prop     & Si2158_GET_PROPERTY_CMD_PROP_MASK     ) << Si2158_GET_PROPERTY_CMD_PROP_LSB    );
	cmdByteBuffer[3] = (u8) ((( prop     & Si2158_GET_PROPERTY_CMD_PROP_MASK     ) << Si2158_GET_PROPERTY_CMD_PROP_LSB    )>>8);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 4, cmdByteBuffer) != 4) {
		SiTRACE("Error writing GET_PROPERTY bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, 4, rspByteBuffer, &ctx->status);
	if (error_code) {
		SiTRACE("Error polling SET_PROPERTY response\n");
		return error_code;
	}

	reserved =   (( ( (rspByteBuffer[1]  )) >> Si2158_GET_PROPERTY_RESPONSE_RESERVED_LSB ) & Si2158_GET_PROPERTY_RESPONSE_RESERVED_MASK );
	*data    =   (( ( (rspByteBuffer[2]  ) | (rspByteBuffer[3]  << 8 )) >> Si2158_GET_PROPERTY_RESPONSE_DATA_LSB     ) & Si2158_GET_PROPERTY_RESPONSE_DATA_MASK     );

	return NO_SILABS_TERCAB_ERROR;
}
#endif

static int silabs_tercab_set_params(struct dvb_frontend *fe)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	u32 freq          = c->frequency;
	u8 bw             = c->bandwidth_hz/1000000;
	u8 modulation     = Si2158_DTV_MODE_PROP_MODULATION_DEFAULT;
	u8 invert_signal  = Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_DEFAULT;
	u16 if_freq       = Si2158_DTV_LIF_FREQ_PROP_OFFSET_DEFAULT;
	u8 lif_out_offset = 148; /* (default   148) */
	u8 lif_out_amp    = Si2158_DTV_LIF_OUT_PROP_AMP_DEFAULT;
	int res, ret=0;
	u16 data;

	silabs_tercab_dbg("delivery system=%u\n", (u32)c->delivery_system);
	silabs_tercab_dbg("frequency=%u\n", c->frequency);
	silabs_tercab_dbg("modulation=%u\n", (u32)c->modulation);
	silabs_tercab_dbg("inversion=%u\n", (u32)c->inversion);
	silabs_tercab_dbg("bandwidth=%u (%u MHz)\n", c->bandwidth_hz, bw);

	priv->mode = SILABS_TERCAB_DIGITAL;

	switch(c->inversion) {
	case INVERSION_OFF:  invert_signal = Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_NORMAL;   break;
	case INVERSION_ON:   invert_signal = Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_INVERTED; break;
	case INVERSION_AUTO:
		silabs_tercab_dbg("inversion AUTO not supported!\n");
		/* return -EINVAL; */
		/* fall through */
	default:
		break;
	}

	switch (c->delivery_system) {
	case SYS_ATSC:
		invert_signal = Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_INVERTED;
		modulation = Si2158_DTV_MODE_PROP_MODULATION_ATSC;
		if_freq = priv->vsb_if_khz;
		bw = 6; /* always use 6 MHz bandwidth */
		break;
	case SYS_ISDBT:
	case SYS_DVBT:
	case SYS_DVBT2:
		modulation = Si2158_DTV_MODE_PROP_MODULATION_DVBT;
		lif_out_amp = 32;
		break;
	case SYS_DVBC_ANNEX_B:
		invert_signal = Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_NORMAL;
		modulation = Si2158_DTV_MODE_PROP_MODULATION_QAM_US;
		if_freq = priv->qam_if_khz;
		bw = 6; /* always use 6 MHz bandwidth */
		break;
	case SYS_DVBC_ANNEX_A:
	case SYS_DVBC_ANNEX_C:
		modulation = Si2158_DTV_MODE_PROP_MODULATION_DVBC;
		lif_out_amp = 43;
		//invert_signal = Si2158_DTV_MODE_PROP_INVERT_SPECTRUM_INVERTED;
		bw = 8; /* always use 8 MHz bandwidth */
		break;
	default:
		silabs_tercab_warn("modulation type not supported!\n");
		return -EINVAL;
	}

	/* When tuning digital, the analog demod must be tri-stated */
	if (fe->ops.analog_ops.standby)
		fe->ops.analog_ops.standby(fe);

	mutex_lock(&priv->lock);

	SiTRACE("Wakeup terrestrial tuner\n");
	res= silabs_tercab_poll_cts(priv->i2c_props.adap, priv->i2c_props.addr);
	if (res != NO_SILABS_TERCAB_ERROR) {
		silabs_tercab_err("tuner Si2177 wake up failed.\n");
		SiTRACE("Terrestrial tuner wake up error: 0x%02x : %s\n",res, silabs_tercab_error_text(res) );
		ret = -EIO;
		goto fail;
	}

	switch (priv->tuner.part) {
	case 57:
		if (!priv->firmware_started) {
			if (silabs_tercab_power_up_with_patch(priv) == NO_SILABS_TERCAB_ERROR) {
				priv->firmware_started = 1;
			} else {
				silabs_tercab_err("%s(): ERROR: Si2177_PowerUpWithPatch() failed\n", __func__);
			}
		}
		break;
	case 77:
	default:
		break;
	}

	/* ------------------------------------------------------------ */
	/* Manage FEF mode in TER tuner                                 */
	/* ------------------------------------------------------------ */
	if (c->delivery_system == SYS_DVBT2) {
		silabs_tercab_fef_setup(fe, 1);
	} else {
		silabs_tercab_fef_setup(fe, 0);
	}

	data = (lif_out_offset & Si2158_DTV_LIF_OUT_PROP_OFFSET_MASK) << Si2158_DTV_LIF_OUT_PROP_OFFSET_LSB  |
		   (lif_out_amp    & Si2158_DTV_LIF_OUT_PROP_AMP_MASK   ) << Si2158_DTV_LIF_OUT_PROP_AMP_LSB ;
	silabs_tercab_set_property(&priv->tuner, Si2158_DTV_LIF_OUT_PROP_CODE, data);

	//Set IF Center freq
	data = (if_freq & Si2158_DTV_LIF_FREQ_PROP_OFFSET_MASK) << Si2158_DTV_LIF_FREQ_PROP_OFFSET_LSB ;
	silabs_tercab_set_property(&priv->tuner, Si2158_DTV_LIF_FREQ_PROP_CODE, data);

	silabs_tercab_dbg("silabs_tercab_dtv_tune( , %u, %u, %u, %u) (IF=%u kHz)\n", freq, bw, modulation, invert_signal, if_freq);
	res = silabs_tercab_dtv_tune(&priv->tuner, freq, bw, modulation, invert_signal);
	msleep(85);
	if (c->delivery_system == SYS_DVBT2) {
		silabs_tercab_fef(fe, 1);
	}

	if (res != NO_SILABS_TERCAB_ERROR) {
		silabs_tercab_err("silabs_tercab_dtv_tune() failed.\n");
		SiTRACE("silabs_tercab_dtv_tune() failed. Error: 0x%02x : %s\n", res, silabs_tercab_error_text(res));
		ret = -EIO;
		goto fail;
	}

	priv->frequency    = freq;
	priv->if_frequency = 1000 * if_freq;
	priv->bandwidth    = 1000000 * bw;

fail:
	mutex_unlock(&priv->lock);
	return ret;
}

/************************************************************************************************************************
 NAME: silabs_tercab_atv_tune
 DESCRIPTION:Update ATV_VIDEO_MODE and tune ATV mode at center frequency
 Parameter:  Pointer to Si2158 Context (I2C address)
 Parameter:  frequency (Hz)
 Parameter:  Video system , e.g. use constant Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M for system M
 Parameter:  transport,  e.g. use constant Si2158_ATV_VIDEO_MODE_PROP_TRANS_TERRESTRIAL for terrestrial
 Parameter:  color , e.g. use constant Si2158_ATV_VIDEO_MODE_PROP_COLOR_PAL_NTSC for PAL or NTSC
 Parameter:  invert_signal, 0= normal, 1= inverted
 Parameter:  rsp - commandResp structure to returns tune status info.
 Returns:    I2C transaction error code, 0 if successful
 Programming Guide Reference:    Flowchart A.7 (Tune flowchart)
 ************************************************************************************************************************/
static int silabs_tercab_atv_tune(silabs_tercab_context *ctx, u32 freq, u8 video_sys, u8 color, u8 invert_spectrum)
{
	Si2158_ATV_VIDEO_MODE_PROP_struct atv_video_mode;
	u16 data;

	/* update ATV_VIDEO_MODE property */
	atv_video_mode.video_sys       = video_sys;
	atv_video_mode.color           = color;
	atv_video_mode.invert_spectrum = invert_spectrum;
	switch (ctx->part) {
	case 57:
		data = (atv_video_mode.video_sys       & Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_MASK      ) << Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LSB  |
		       (atv_video_mode.color           & Si2158_ATV_VIDEO_MODE_PROP_COLOR_MASK          ) << Si2158_ATV_VIDEO_MODE_PROP_COLOR_LSB  |
		       (atv_video_mode.invert_spectrum & Si2158_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2158_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_LSB;
		break;
	case 77:
		data = (atv_video_mode.video_sys       & Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_MASK    ) << Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LSB  |
		       (atv_video_mode.color           & Si2158_ATV_VIDEO_MODE_PROP_COLOR_MASK        ) << Si2158_ATV_VIDEO_MODE_PROP_COLOR_LSB  |
		       (atv_video_mode.invert_spectrum & Si2177_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_MASK) << Si2177_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_LSB;
		break;
	default:
		pr_warn("%s(): unsupported tuner model Si21%02u\n", __func__, ctx->part);
		return ERROR_SILABS_TERCAB_INCOMPATIBLE_PART;
	}
	if (silabs_tercab_set_property(ctx, Si2158_ATV_VIDEO_MODE_PROP_CODE, data) != NO_SILABS_TERCAB_ERROR) {
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	return silabs_tercab_tune(ctx, Si2158_TUNER_TUNE_FREQ_CMD_MODE_ATV, freq);
}

/*---------------------------------------------------*/
/* Si2158_ATV_STATUS COMMAND                       */
/*---------------------------------------------------*/
static u8 silabs_tercab_atv_status(struct silabs_tercab_priv *priv, u8 intack)
{
	silabs_tercab_context *ctx = &priv->tuner;
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[12]; /* Si2177 */

	ctx->rsp.atv_status.STATUS = &ctx->status;

	SiTRACE("%s()\n", __func__);

	cmdByteBuffer[0] = Si2158_ATV_STATUS_CMD;
	cmdByteBuffer[1] = (u8) ( ( intack & Si2158_ATV_STATUS_CMD_INTACK_MASK ) << Si2158_ATV_STATUS_CMD_INTACK_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing ATV_STATUS bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, sizeof(rspByteBuffer), rspByteBuffer, &ctx->status);
	if (error_code) {
		SiTRACE("Error polling ATV_STATUS response\n");
		return error_code;
	}

	ctx->rsp.atv_status.chlint    = (rspByteBuffer[1] >> Si2158_ATV_STATUS_RESPONSE_CHLINT_LSB)    & Si2158_ATV_STATUS_RESPONSE_CHLINT_MASK;
	ctx->rsp.atv_status.pclint    = (rspByteBuffer[1] >> Si2158_ATV_STATUS_RESPONSE_PCLINT_LSB)    & Si2158_ATV_STATUS_RESPONSE_PCLINT_MASK;
	ctx->rsp.atv_status.chl       = (rspByteBuffer[2] >> Si2158_ATV_STATUS_RESPONSE_CHL_LSB)       & Si2158_ATV_STATUS_RESPONSE_CHL_MASK;
	ctx->rsp.atv_status.pcl       = (rspByteBuffer[2] >> Si2158_ATV_STATUS_RESPONSE_PCL_LSB)       & Si2158_ATV_STATUS_RESPONSE_PCL_MASK;
	ctx->rsp.atv_status.afc_freq  = (((((rspByteBuffer[4] | (rspByteBuffer[5] << 8)) >> Si2158_ATV_STATUS_RESPONSE_AFC_FREQ_LSB) & Si2158_ATV_STATUS_RESPONSE_AFC_FREQ_MASK) << Si2158_ATV_STATUS_RESPONSE_AFC_FREQ_SHIFT) >> Si2158_ATV_STATUS_RESPONSE_AFC_FREQ_SHIFT);
	ctx->rsp.atv_status.video_sys = (rspByteBuffer[8] >> Si2158_ATV_STATUS_RESPONSE_VIDEO_SYS_LSB) & Si2158_ATV_STATUS_RESPONSE_VIDEO_SYS_MASK;
	ctx->rsp.atv_status.color     = (rspByteBuffer[8] >> Si2158_ATV_STATUS_RESPONSE_COLOR_LSB)     & Si2158_ATV_STATUS_RESPONSE_COLOR_MASK;

	switch (priv->tuner.part) {
	case 57:
		break;
	case 77:
		ctx->rsp.atv_status.dlint            = (rspByteBuffer[1]  >> Si2177_ATV_STATUS_RESPONSE_DLINT_LSB    ) & Si2177_ATV_STATUS_RESPONSE_DLINT_MASK;
		ctx->rsp.atv_status.snrlint          = (rspByteBuffer[1]  >> Si2177_ATV_STATUS_RESPONSE_SNRLINT_LSB  ) & Si2177_ATV_STATUS_RESPONSE_SNRLINT_MASK;
		ctx->rsp.atv_status.snrhint          = (rspByteBuffer[1]  >> Si2177_ATV_STATUS_RESPONSE_SNRHINT_LSB  ) & Si2177_ATV_STATUS_RESPONSE_SNRHINT_MASK;
		ctx->rsp.atv_status.dl               = (rspByteBuffer[2]  >> Si2177_ATV_STATUS_RESPONSE_DL_LSB       ) & Si2177_ATV_STATUS_RESPONSE_DL_MASK;
		ctx->rsp.atv_status.snrl             = (rspByteBuffer[2]  >> Si2177_ATV_STATUS_RESPONSE_SNRL_LSB     ) & Si2177_ATV_STATUS_RESPONSE_SNRL_MASK;
		ctx->rsp.atv_status.snrh             = (rspByteBuffer[2]  >> Si2177_ATV_STATUS_RESPONSE_SNRH_LSB     ) & Si2177_ATV_STATUS_RESPONSE_SNRH_MASK;
		ctx->rsp.atv_status.video_snr        = (rspByteBuffer[3]  >> Si2177_ATV_STATUS_RESPONSE_VIDEO_SNR_LSB) & Si2177_ATV_STATUS_RESPONSE_VIDEO_SNR_MASK;
		ctx->rsp.atv_status.video_sc_spacing = (((((rspByteBuffer[6] | (rspByteBuffer[7] << 8)) >> Si2177_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_LSB) & Si2177_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_MASK) << Si2177_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_SHIFT) >> Si2177_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_SHIFT);
		ctx->rsp.atv_status.lines            = (rspByteBuffer[8]  >> Si2177_ATV_STATUS_RESPONSE_LINES_LSB           ) & Si2177_ATV_STATUS_RESPONSE_LINES_MASK;
		ctx->rsp.atv_status.audio_sys        = (rspByteBuffer[9]  >> Si2177_ATV_STATUS_RESPONSE_AUDIO_SYS_LSB       ) & Si2177_ATV_STATUS_RESPONSE_AUDIO_SYS_MASK;
		ctx->rsp.atv_status.audio_demod_mode = (rspByteBuffer[9]  >> Si2177_ATV_STATUS_RESPONSE_AUDIO_DEMOD_MODE_LSB) & Si2177_ATV_STATUS_RESPONSE_AUDIO_DEMOD_MODE_MASK;
		ctx->rsp.atv_status.audio_chan_bw    = (rspByteBuffer[10] >> Si2177_ATV_STATUS_RESPONSE_AUDIO_CHAN_BW_LSB   ) & Si2177_ATV_STATUS_RESPONSE_AUDIO_CHAN_BW_MASK;
		ctx->rsp.atv_status.sound_level      = ((((rspByteBuffer[11] >> Si2177_ATV_STATUS_RESPONSE_SOUND_LEVEL_LSB ) & Si2177_ATV_STATUS_RESPONSE_SOUND_LEVEL_MASK) << Si2177_ATV_STATUS_RESPONSE_SOUND_LEVEL_SHIFT) >> Si2177_ATV_STATUS_RESPONSE_SOUND_LEVEL_SHIFT);
		break;
	default:
		break;
	}

	return NO_SILABS_TERCAB_ERROR;
}

static int silabs_tercab_set_analog_params(struct dvb_frontend *fe, struct analog_parameters *params)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	int res, ret     = 0;
	u32 freq         = params->frequency * 125 * ((params->mode == V4L2_TUNER_RADIO) ? 1 : 1000) / 2;
	u32 centerfreq   = 0;
	u32 if_frequency = 5400000;
	u32 bw           = 8000000;
	u8 video_sys     = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DEFAULT;
	u8 color         = Si2158_ATV_VIDEO_MODE_PROP_COLOR_DEFAULT;
	u8 invert_signal = Si2158_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_DEFAULT;

	const u16 atv_afc_range_khz    =  1000; //1500;
	const u8  tuner_ien_tcien     =  Si2158_TUNER_IEN_PROP_TCIEN_ENABLE;
	const u8  tuner_ien_rssilien  =  Si2158_TUNER_IEN_PROP_RSSILIEN_DISABLE;
	const u8  tuner_ien_rssihien  =  Si2158_TUNER_IEN_PROP_RSSIHIEN_DISABLE;
	const u8  atv_agc_speed_if_agc_speed = Si2158_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
	const u8  atv_lif_out_amp = 100; /* (default    100) */
	const u8  atv_lif_out_offset = 148; /* (default   148) */
	const s8  atv_rsq_rssi_threshold_hi = 0;
	const s8  atv_rsq_rssi_threshold_lo = -70;
	const u8  atv_ien_chlien    =  Si2158_ATV_IEN_PROP_CHLIEN_ENABLE;
	const u8  atv_ien_pclien    =  Si2158_ATV_IEN_PROP_PCLIEN_DISABLE;
	const u8  atv_config_if_port_atv_agc_source = Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_AGC_SOURCE_AGC2_3DB;
	const u8  atv_config_if_port_atv_out_type_58 = Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_LIF_DIFF_IF1;

	/* Si2177 */
	const u8  atv_config_if_port_atv_out_type_77 = Si2177_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_CVBS_IF2B_SOUND_IF2A;
	const u8  atv_sif_out_amp = 60; /* 77 (default 60) */
	const u8  atv_sif_out_offset = 135;  /* 77 (default 135) */
	const u8  atv_cvbs_out_amp = 200; /* 77 (default 200) */
	const u8  atv_cvbs_out_offset = 25;  /* 77 (default 25) */
	const u8  atv_cvbs_out_fine_amp = 100; /* 77 (default 100) */
	const s8  atv_cvbs_out_fine_offset = 0;  /* 77 (default 0) */
	const s8  atv_video_equalizer_slope = 0;  /* 77 (default 0) */
	const u8  atv_rsq_snr_threshold_hi = Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_HI_DEFAULT;// 77 45 maxSNRHalfdB;
	const u8  atv_rsq_snr_threshold_lo = Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_LO_DEFAULT;// 77 25 minSNRHalfdB;
	const u8  atv_ien_dlien     =  Si2177_ATV_IEN_PROP_DLIEN_DISABLE;
	const u8  atv_ien_snrhien   =  Si2177_ATV_IEN_PROP_SNRHIEN_DISABLE;
	const u8  atv_ien_snrlien   =  Si2177_ATV_IEN_PROP_SNRLIEN_DISABLE;
	const u8  atv_audio_mode_audio_sys  = Si2177_ATV_AUDIO_MODE_PROP_AUDIO_SYS_DEFAULT;
	const u8  atv_audio_mode_chan_bw    = Si2177_ATV_AUDIO_MODE_PROP_CHAN_BW_DEFAULT;
	const u8  atv_audio_mode_demod_mode = Si2177_ATV_AUDIO_MODE_PROP_DEMOD_MODE_SIF;
	const u8  atv_ext_agc_min_10mv = 50; // 58 (default  50)
	const u8  atv_ext_agc_max_10mv = 200; // 58 (default 200)

	u16 lif_freq_offset;

	u16 si2158_atv_agc_speed_prop_data;
	u16 si2158_atv_lif_out_prop_data;
	u16 si2158_atv_lif_freq_prop_data;
	u16 si2158_atv_config_if_port_prop_data;
	u16 si2177_atv_config_if_port_prop_data;
	u16 si2177_atv_sif_out_prop_data;
	u16 si2177_atv_cvbs_out_prop_data;
	u16 si2177_atv_cvbs_out_fine_prop_data;
	u16 si2177_atv_video_equalizer_prop_data;
	u16 si2158_atv_ext_agc_prop_data;
	u16 si2158_atv_rsq_rssi_threshold_prop_data;
	u16 si2177_atv_rsq_snr_threshold_prop_data;
	u16 si2158_tuner_ien__prop_data;
	u16 si2158_atv_ien_prop_data;
	u16 si2177_atv_ien_prop_data;
	u16 si2177_atv_audio_mode_prop_data;
	u16 si2158_atv_afc_range_prop_data;

	silabs_tercab_dbg("freq = %d, video_sys = %u, color = %u\n", freq, video_sys, color);

	priv->mode = SILABS_TERCAB_ANALOG;

	if (params->mode == V4L2_TUNER_RADIO) {
		video_sys = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DEFAULT;
		silabs_tercab_warn("radio mode is not supported\n");
	} else if (params->std & (V4L2_STD_MN | V4L2_STD_NTSC_443)) {
		if_frequency = 5400000;  /*5.4MHz	*/
		bw = 6000000;
		video_sys = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M;
		silabs_tercab_info("using video_sys Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M\n");
	} else if (params->std & V4L2_STD_B) {
		if_frequency = 6000000;  /*6.0MHz	*/
		bw = 7000000;
		video_sys = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_B;
		silabs_tercab_info("using video_sys Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_B\n");
	} else if (params->std & (V4L2_STD_PAL_DK | V4L2_STD_SECAM_DK)){
		if_frequency = 6900000;  /*6.9MHz	*/
		bw = 8000000;
		video_sys = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DK;
	} else if (params->std & V4L2_STD_GH){
		if_frequency = 7100000;  /*7.1MHz	*/
		bw = 8000000;
		video_sys = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_GH;
		silabs_tercab_info("using video_sys Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_GH\n");
	} else if (params->std & V4L2_STD_PAL_I){
		if_frequency = 7250000;  /*7.25MHz	*/
		bw = 8000000;
		video_sys = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_I;
	} else if (params->std & V4L2_STD_SECAM_L){
		if_frequency = 6900000;  /*6.9MHz	*/
		bw = 8000000;
		video_sys = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_L;
		color = Si2158_ATV_VIDEO_MODE_PROP_COLOR_SECAM;
	} else if (params->std & V4L2_STD_SECAM_LC){
		if_frequency = 1250000;  /*1.25MHz	*/
		bw = 7000000;
		video_sys = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LP; /* ToDo: lc == lp? */
		color = Si2158_ATV_VIDEO_MODE_PROP_COLOR_SECAM;
	} else {
		video_sys = Si2158_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DEFAULT;
		silabs_tercab_warn("unsupported video system\n");
	}

	mutex_lock(&priv->lock);

	SiTRACE("Wakeup terrestrial tuner\n");
	res = silabs_tercab_poll_cts(priv->i2c_props.adap, priv->i2c_props.addr);
	if (res != NO_SILABS_TERCAB_ERROR) {
		silabs_tercab_err("tuner Si2158 wake up failed.\n");
		SiTRACE("Terrestrial tuner wake up error: 0x%02x : %s\n", res, silabs_tercab_error_text(res));
		ret = -EIO;
		goto fail;
	}

	//Calc IF Center for given Video Carrier
	//uIF = if_frequency + 1250000 - (bw/2);
	//invert_signal = Si2177_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_INVERTED;

	//Set IF output level
	//dwValue = 100; //m_Config.uIFLEVEL;

	switch (priv->tuner.part) {
	case 57:
		silabs_tercab_info("%s(): initializing tuner type Si21%02u\n", __func__, priv->tuner.part);

		if (!priv->firmware_started) {
			if (silabs_tercab_power_up_with_patch(priv) == NO_SILABS_TERCAB_ERROR) {
				priv->firmware_started = 1;
			} else {
				silabs_tercab_err("%s(): ERROR: Si2177_PowerUpWithPatch() failed\n", __func__);
			}
		}
		invert_signal = Si2158_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_DEFAULT;

		si2158_atv_agc_speed_prop_data = (atv_agc_speed_if_agc_speed & Si2158_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK) << Si2158_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_ATV_AGC_SPEED_PROP_CODE, si2158_atv_agc_speed_prop_data); /* windows 0x0 */

		si2158_atv_lif_out_prop_data = (atv_lif_out_offset & Si2158_ATV_LIF_OUT_PROP_OFFSET_MASK) << Si2158_ATV_LIF_OUT_PROP_OFFSET_LSB  |
			   (atv_lif_out_amp    & Si2158_ATV_LIF_OUT_PROP_AMP_MASK   ) << Si2158_ATV_LIF_OUT_PROP_AMP_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_ATV_LIF_OUT_PROP_CODE, si2158_atv_lif_out_prop_data); /* windows 0x6494 */

		//Set IF Center freq
		lif_freq_offset = (u16)((if_frequency + 1250000 - (bw/2))/1000);
		silabs_tercab_info("atv_lif_freq.offset=%u uBW=%u video_sys=%u\n", lif_freq_offset, bw, video_sys);
		si2158_atv_lif_freq_prop_data = (lif_freq_offset & Si2158_ATV_LIF_FREQ_PROP_OFFSET_MASK) << Si2158_ATV_LIF_FREQ_PROP_OFFSET_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_ATV_LIF_FREQ_PROP_CODE, si2158_atv_lif_freq_prop_data); /* windows 0xe42 */

		si2158_atv_config_if_port_prop_data = (atv_config_if_port_atv_out_type_58 & Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_MASK  ) << Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_LSB  |
			   (atv_config_if_port_atv_agc_source & Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_AGC_SOURCE_MASK) << Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_AGC_SOURCE_LSB ;

		//atv_agc_source isn't really supported on si2177, but lets set it to a known value
		//atv_config_if_port_atv_agc_source = (u8)Si2158_ATV_CONFIG_IF_PORT_PROP_ATV_AGC_SOURCE_INTERNAL;

		silabs_tercab_set_property(&priv->tuner, Si2158_ATV_CONFIG_IF_PORT_PROP_CODE, si2158_atv_config_if_port_prop_data); /* windows 0x208 */
		break;

	case 77:
		silabs_tercab_info("%s(): initializing tuner type Si21%02u\n", __func__, priv->tuner.part);

		invert_signal = Si2177_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_DEFAULT;

		si2177_atv_config_if_port_prop_data = (atv_config_if_port_atv_out_type_77 & Si2177_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_MASK) << Si2177_ATV_CONFIG_IF_PORT_PROP_ATV_OUT_TYPE_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2158_ATV_CONFIG_IF_PORT_PROP_CODE, si2177_atv_config_if_port_prop_data);

		si2177_atv_sif_out_prop_data = (atv_sif_out_offset & Si2177_ATV_SIF_OUT_PROP_OFFSET_MASK) << Si2177_ATV_SIF_OUT_PROP_OFFSET_LSB  |
			   (atv_sif_out_amp    & Si2177_ATV_SIF_OUT_PROP_AMP_MASK   ) << Si2177_ATV_SIF_OUT_PROP_AMP_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2177_ATV_SIF_OUT_PROP_CODE, si2177_atv_sif_out_prop_data);

		si2177_atv_cvbs_out_prop_data = (atv_cvbs_out_offset & Si2177_ATV_CVBS_OUT_PROP_OFFSET_MASK) << Si2177_ATV_CVBS_OUT_PROP_OFFSET_LSB  |
			   (atv_cvbs_out_amp    & Si2177_ATV_CVBS_OUT_PROP_AMP_MASK   ) << Si2177_ATV_CVBS_OUT_PROP_AMP_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2177_ATV_CVBS_OUT_PROP_CODE, si2177_atv_cvbs_out_prop_data);

		si2177_atv_cvbs_out_fine_prop_data = (atv_cvbs_out_fine_offset & Si2177_ATV_CVBS_OUT_FINE_PROP_OFFSET_MASK) << Si2177_ATV_CVBS_OUT_FINE_PROP_OFFSET_LSB  |
			   (atv_cvbs_out_fine_amp    & Si2177_ATV_CVBS_OUT_FINE_PROP_AMP_MASK   ) << Si2177_ATV_CVBS_OUT_FINE_PROP_AMP_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2177_ATV_CVBS_OUT_FINE_PROP_CODE, si2177_atv_cvbs_out_fine_prop_data);

		si2177_atv_video_equalizer_prop_data = (atv_video_equalizer_slope & Si2177_ATV_VIDEO_EQUALIZER_PROP_SLOPE_MASK) << Si2177_ATV_VIDEO_EQUALIZER_PROP_SLOPE_LSB ;
		silabs_tercab_set_property(&priv->tuner, Si2177_ATV_VIDEO_EQUALIZER_PROP_CODE, si2177_atv_video_equalizer_prop_data);
		break;

	default:
		silabs_tercab_warn("%s(): unsupported tuner model Si21%02u\n", __func__, priv->tuner.part);
		break;
	}

	if (priv->tuner.part != 77) {
		/* setup external AGC settings */
		si2158_atv_ext_agc_prop_data = (atv_ext_agc_min_10mv & Si2158_ATV_EXT_AGC_PROP_MIN_10MV_MASK) << Si2158_ATV_EXT_AGC_PROP_MIN_10MV_LSB  |
		   (atv_ext_agc_max_10mv & Si2158_ATV_EXT_AGC_PROP_MAX_10MV_MASK) << Si2158_ATV_EXT_AGC_PROP_MAX_10MV_LSB ;
		silabs_tercab_set_property (&priv->tuner, Si2158_ATV_EXT_AGC_PROP_CODE, si2158_atv_ext_agc_prop_data); /* windows 0xc832 */
	}

	/* configure the RSQ / RSSI threshold properties */
	si2158_atv_rsq_rssi_threshold_prop_data = (atv_rsq_rssi_threshold_lo & Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_MASK) << Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_LSB  |
		   (atv_rsq_rssi_threshold_hi & Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_MASK) << Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_LSB ;

	if (silabs_tercab_set_property(&priv->tuner, Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_CODE, si2158_atv_rsq_rssi_threshold_prop_data) != NO_SILABS_TERCAB_ERROR) {
		silabs_tercab_err("%s() silabs_tercab_set_property(Si2158_ATV_RSQ_RSSI_THRESHOLD_PROP_CODE) failed.\n", __func__);
		ret = -EIO;
		goto fail;
	}

	if (priv->tuner.part == 77) { //Si2177
		/* configure the RSQ / SNR threshold properties */
		si2177_atv_rsq_snr_threshold_prop_data = (atv_rsq_snr_threshold_lo & Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_LO_MASK) << Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_LO_LSB  |
			   (atv_rsq_snr_threshold_hi & Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_HI_MASK) << Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_HI_LSB ;
		if (silabs_tercab_set_property(&priv->tuner, Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_CODE, si2177_atv_rsq_snr_threshold_prop_data) != NO_SILABS_TERCAB_ERROR) {
			silabs_tercab_warn("%s() silabs_tercab_set_property(Si2177_ATV_RSQ_SNR_THRESHOLD_PROP_CODE) failed.\n", __func__);
		}
	}

	/* setup IEN properties to enable TUNINT on TC  */
	si2158_tuner_ien__prop_data = (tuner_ien_tcien    & Si2158_TUNER_IEN_PROP_TCIEN_MASK   ) << Si2158_TUNER_IEN_PROP_TCIEN_LSB  |
		   (tuner_ien_rssilien & Si2158_TUNER_IEN_PROP_RSSILIEN_MASK) << Si2158_TUNER_IEN_PROP_RSSILIEN_LSB  |
		   (tuner_ien_rssihien & Si2158_TUNER_IEN_PROP_RSSIHIEN_MASK) << Si2158_TUNER_IEN_PROP_RSSIHIEN_LSB ;
	if (silabs_tercab_set_property(&priv->tuner, Si2158_TUNER_IEN_PROP_CODE, si2158_tuner_ien__prop_data) != NO_SILABS_TERCAB_ERROR) {
		silabs_tercab_warn("%s() silabs_tercab_set_property(Si2158_TUNER_IEN_PROP_CODE) failed.\n", __func__);
	}

	/* setup IEN properties to enable ATVINT on CHL  */
	if (priv->tuner.part == 77) { //Si2177
		si2177_atv_ien_prop_data = (atv_ien_chlien  & Si2158_ATV_IEN_PROP_CHLIEN_MASK ) << Si2158_ATV_IEN_PROP_CHLIEN_LSB  |
			   (atv_ien_pclien  & Si2158_ATV_IEN_PROP_PCLIEN_MASK ) << Si2158_ATV_IEN_PROP_PCLIEN_LSB  |
			   (atv_ien_dlien   & Si2177_ATV_IEN_PROP_DLIEN_MASK  ) << Si2177_ATV_IEN_PROP_DLIEN_LSB  |
			   (atv_ien_snrlien & Si2177_ATV_IEN_PROP_SNRLIEN_MASK) << Si2177_ATV_IEN_PROP_SNRLIEN_LSB  |
			   (atv_ien_snrhien & Si2177_ATV_IEN_PROP_SNRHIEN_MASK) << Si2177_ATV_IEN_PROP_SNRHIEN_LSB ;
		if (silabs_tercab_set_property(&priv->tuner, Si2158_ATV_IEN_PROP_CODE, si2177_atv_ien_prop_data) != NO_SILABS_TERCAB_ERROR) { /* windows 0x1 */
			silabs_tercab_warn("%s() silabs_tercab_set_property(Si2158_ATV_IEN_PROP_CODE) failed.\n", __func__);
		}
	} else {
		si2158_atv_ien_prop_data = (atv_ien_chlien  & Si2158_ATV_IEN_PROP_CHLIEN_MASK ) << Si2158_ATV_IEN_PROP_CHLIEN_LSB  |
			   (atv_ien_pclien  & Si2158_ATV_IEN_PROP_PCLIEN_MASK ) << Si2158_ATV_IEN_PROP_PCLIEN_LSB ;
		if (silabs_tercab_set_property(&priv->tuner, Si2158_ATV_IEN_PROP_CODE, si2158_atv_ien_prop_data) != NO_SILABS_TERCAB_ERROR) { /* windows 0x1 */
			silabs_tercab_warn("%s() silabs_tercab_set_property(Si2158_ATV_IEN_PROP_CODE) failed.\n", __func__);
		}
	}
	if (priv->tuner.part == 77) { //Si2177
		/* setup ATV audio property for wide SIF scanning*/
		si2177_atv_audio_mode_prop_data = (atv_audio_mode_audio_sys  & Si2177_ATV_AUDIO_MODE_PROP_AUDIO_SYS_MASK ) << Si2177_ATV_AUDIO_MODE_PROP_AUDIO_SYS_LSB  |
			   (atv_audio_mode_demod_mode & Si2177_ATV_AUDIO_MODE_PROP_DEMOD_MODE_MASK) << Si2177_ATV_AUDIO_MODE_PROP_DEMOD_MODE_LSB  |
			   (atv_audio_mode_chan_bw    & Si2177_ATV_AUDIO_MODE_PROP_CHAN_BW_MASK   ) << Si2177_ATV_AUDIO_MODE_PROP_CHAN_BW_LSB ;
		if (silabs_tercab_set_property(&priv->tuner, Si2177_ATV_AUDIO_MODE_PROP_CODE, si2177_atv_audio_mode_prop_data) != NO_SILABS_TERCAB_ERROR) {
			silabs_tercab_warn("%s() silabs_tercab_set_property(Si2177_ATV_AUDIO_MODE_PROP_CODE) failed.\n", __func__);
		}
	}
	/* setup AFC acquisition range property to 1.5MHz for scanning */
	si2158_atv_afc_range_prop_data = (atv_afc_range_khz & Si2158_ATV_AFC_RANGE_PROP_RANGE_KHZ_MASK) << Si2158_ATV_AFC_RANGE_PROP_RANGE_KHZ_LSB ;
	if (silabs_tercab_set_property(&priv->tuner, Si2158_ATV_AFC_RANGE_PROP_CODE, si2158_atv_afc_range_prop_data) != NO_SILABS_TERCAB_ERROR) {
		silabs_tercab_warn("%s() silabs_tercab_set_property(Si2158_ATV_AFC_RANGE_PROP) failed.\n", __func__);
	}

	centerfreq = freq - 1250000 + (bw/2);

	//tune to channel center freq
	silabs_tercab_dbg("tuning to center frequency %u, video system %u, color %u, invert %u\n", centerfreq, video_sys, color, invert_signal);

	res = silabs_tercab_atv_tune(&priv->tuner, centerfreq, video_sys, color, invert_signal);
	if (res == NO_SILABS_TERCAB_ERROR) {
		/* Get ATV status */
		if (silabs_tercab_atv_status (priv, Si2158_ATV_STATUS_CMD_INTACK_OK)) {
			silabs_tercab_err("%s() silabs_tercab_atv_status() failed.\n", __func__);
			ret = -EIO;
			goto fail;
		}
	} else {
		silabs_tercab_dbg("silabs_tercab_atv_tune(fe, %u, %u, %u, %u) failed (err=%d %s).\n",
				centerfreq, video_sys, color, invert_signal, res, silabs_tercab_error_text(res));
		ret = -EIO;
		goto fail;
	}
	msleep(85);

	priv->frequency = freq;
	priv->if_frequency = if_frequency;
	priv->bandwidth = bw;
	ret = 0;

fail:
	mutex_unlock(&priv->lock);

	return ret;
}

static int silabs_tercab_release(struct dvb_frontend *fe)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;

	silabs_tercab_dbg("\n");

	mutex_lock(&silabs_tercab_list_mutex);

	if (priv)
		hybrid_tuner_release_state(priv);

	mutex_unlock(&silabs_tercab_list_mutex);

	fe->tuner_priv = NULL;

	return 0;
}

static int silabs_tercab_get_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	silabs_tercab_dbg("frequency=%u\n", priv->frequency);
	*frequency = priv->frequency;
	return 0;
}

static int silabs_tercab_get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	silabs_tercab_dbg("bandwidth=%u\n", priv->bandwidth);
	*bandwidth = priv->bandwidth;
	return 0;
}

static int silabs_tercab_get_if_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	silabs_tercab_dbg("if_frequency=%u\n", priv->if_frequency);
	*frequency = (u32)priv->if_frequency;
	return 0;
}

/*---------------------------------------------------*/
/* Si2158_DTV_STATUS COMMAND                       */
/*---------------------------------------------------*/
static u8 silabs_tercab_dtv_status(silabs_tercab_context *ctx, u8 intack)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[4];
	ctx->rsp.dtv_status.STATUS = &ctx->status;

	SiTRACE("%s()\n", __func__);

	cmdByteBuffer[0] = Si2158_DTV_STATUS_CMD;
	cmdByteBuffer[1] = (u8)((intack & Si2158_DTV_STATUS_CMD_INTACK_MASK) << Si2158_DTV_STATUS_CMD_INTACK_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DTV_STATUS bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, 4, rspByteBuffer, &ctx->status);
	if (error_code) {
		SiTRACE("Error polling DTV_STATUS response\n");
		return error_code;
	}

	ctx->rsp.dtv_status.chlint     = (rspByteBuffer[1] >> Si2158_DTV_STATUS_RESPONSE_CHLINT_LSB    ) & Si2158_DTV_STATUS_RESPONSE_CHLINT_MASK;
	ctx->rsp.dtv_status.chl        = (rspByteBuffer[2] >> Si2158_DTV_STATUS_RESPONSE_CHL_LSB       ) & Si2158_DTV_STATUS_RESPONSE_CHL_MASK;
	ctx->rsp.dtv_status.bw         = (rspByteBuffer[3] >> Si2158_DTV_STATUS_RESPONSE_BW_LSB        ) & Si2158_DTV_STATUS_RESPONSE_BW_MASK;
	ctx->rsp.dtv_status.modulation = (rspByteBuffer[3] >> Si2158_DTV_STATUS_RESPONSE_MODULATION_LSB) & Si2158_DTV_STATUS_RESPONSE_MODULATION_MASK;

	return NO_SILABS_TERCAB_ERROR;
}

static int silabs_tercab_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	u8 res;
	u8 ATV_Sync_Lock;
	u8 ATV_Master_Lock;

	*status = 0;

	if (priv->mode == SILABS_TERCAB_ANALOG) {
		mutex_lock(&priv->lock);
		res = silabs_tercab_atv_status(priv, Si2158_ATV_STATUS_CMD_INTACK_OK);
		mutex_unlock(&priv->lock);
		if (res != NO_SILABS_TERCAB_ERROR) {
			silabs_tercab_err("Si2177_L1_ATV_STATUS() failed (err=%d)\n", res);
			return -EIO;
		}
		ATV_Sync_Lock   = priv->tuner.rsp.atv_status.pcl;
		ATV_Master_Lock = priv->tuner.rsp.atv_status.dl;

		silabs_tercab_dbg("ATV_Sync_Lock=%u  ATV_Master_Lock=%u  Channel=%u\n", ATV_Sync_Lock, ATV_Master_Lock, priv->tuner.rsp.atv_status.chl);

		if (ATV_Sync_Lock && ATV_Master_Lock) {
			*status = TUNER_STATUS_LOCKED;
		}
	} else {
		mutex_lock(&priv->lock);
		res = silabs_tercab_dtv_status(&priv->tuner, Si2158_DTV_STATUS_CMD_INTACK_OK);
		mutex_unlock(&priv->lock);
		if (res != NO_SILABS_TERCAB_ERROR) {
			silabs_tercab_err("Si2177_L1_DTV_STATUS() failed (err=%d)\n", res);
			return -EIO;
		}
		silabs_tercab_dbg("Channel=%u\n", priv->tuner.rsp.dtv_status.chl);
		if (priv->tuner.rsp.dtv_status.chl) { /* ToDo: is this a lock indicator? */
			*status = TUNER_STATUS_LOCKED;
		}
	}

#if 0
	if (tuner_stereo(priv->type, tuner_status))
		*status |= TUNER_STATUS_STEREO;
#endif

	silabs_tercab_dbg("Status=%d\n", *status);

	return 0;
}

/*---------------------------------------------------*/
/* Si2158_TUNER_STATUS COMMAND                     */
/*---------------------------------------------------*/
static u8 silabs_tercab_tuner_status(silabs_tercab_context *ctx, u8 intack)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[12];
	ctx->rsp.tuner_status.STATUS = &ctx->status;

	SiTRACE("%s()\n", __func__);

	cmdByteBuffer[0] = Si2158_TUNER_STATUS_CMD;
	cmdByteBuffer[1] = (u8) ( ( intack & Si2158_TUNER_STATUS_CMD_INTACK_MASK ) << Si2158_TUNER_STATUS_CMD_INTACK_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing TUNER_STATUS bytes!\n");
		return ERROR_SILABS_TERCAB_SENDING_COMMAND;
	}

	error_code = silabs_tercab_poll_response(ctx->i2c_adap, ctx->i2c_addr, 12, rspByteBuffer, &ctx->status);
	if (error_code) {
		SiTRACE("Error polling TUNER_STATUS response\n");
		return error_code;
	}

	ctx->rsp.tuner_status.tcint    =   (( ( (rspByteBuffer[1]  )) >> Si2158_TUNER_STATUS_RESPONSE_TCINT_LSB    ) & Si2158_TUNER_STATUS_RESPONSE_TCINT_MASK    );
	ctx->rsp.tuner_status.rssilint =   (( ( (rspByteBuffer[1]  )) >> Si2158_TUNER_STATUS_RESPONSE_RSSILINT_LSB ) & Si2158_TUNER_STATUS_RESPONSE_RSSILINT_MASK );
	ctx->rsp.tuner_status.rssihint =   (( ( (rspByteBuffer[1]  )) >> Si2158_TUNER_STATUS_RESPONSE_RSSIHINT_LSB ) & Si2158_TUNER_STATUS_RESPONSE_RSSIHINT_MASK );
	ctx->rsp.tuner_status.tc       =   (( ( (rspByteBuffer[2]  )) >> Si2158_TUNER_STATUS_RESPONSE_TC_LSB       ) & Si2158_TUNER_STATUS_RESPONSE_TC_MASK       );
	ctx->rsp.tuner_status.rssil    =   (( ( (rspByteBuffer[2]  )) >> Si2158_TUNER_STATUS_RESPONSE_RSSIL_LSB    ) & Si2158_TUNER_STATUS_RESPONSE_RSSIL_MASK    );
	ctx->rsp.tuner_status.rssih    =   (( ( (rspByteBuffer[2]  )) >> Si2158_TUNER_STATUS_RESPONSE_RSSIH_LSB    ) & Si2158_TUNER_STATUS_RESPONSE_RSSIH_MASK    );
	ctx->rsp.tuner_status.rssi     = (((( ( (rspByteBuffer[3]  )) >> Si2158_TUNER_STATUS_RESPONSE_RSSI_LSB     ) & Si2158_TUNER_STATUS_RESPONSE_RSSI_MASK) << Si2158_TUNER_STATUS_RESPONSE_RSSI_SHIFT ) >> Si2158_TUNER_STATUS_RESPONSE_RSSI_SHIFT);
	ctx->rsp.tuner_status.freq     =   (( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 ) | (rspByteBuffer[6]  << 16 ) | (rspByteBuffer[7]  << 24 )) >> Si2158_TUNER_STATUS_RESPONSE_FREQ_LSB     ) & Si2158_TUNER_STATUS_RESPONSE_FREQ_MASK);
	ctx->rsp.tuner_status.mode     =   (( ( (rspByteBuffer[8]  )) >> Si2158_TUNER_STATUS_RESPONSE_MODE_LSB     ) & Si2158_TUNER_STATUS_RESPONSE_MODE_MASK     );
	ctx->rsp.tuner_status.vco_code = (((( ( (rspByteBuffer[10] ) | (rspByteBuffer[11] << 8 )) >> Si2158_TUNER_STATUS_RESPONSE_VCO_CODE_LSB ) & Si2158_TUNER_STATUS_RESPONSE_VCO_CODE_MASK) << Si2158_TUNER_STATUS_RESPONSE_VCO_CODE_SHIFT) >> Si2158_TUNER_STATUS_RESPONSE_VCO_CODE_SHIFT);

	return NO_SILABS_TERCAB_ERROR;
}

static int silabs_tercab_get_signal(struct dvb_frontend *fe, u16 *strength)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	int rc = 0;
	char rssi = 0;
	u8 res;
	u32 freq;
	u8 mode;
	u8 ATV_Sync_Lock;
	u8 ATV_Master_Lock;
	u8 chl;

	mutex_lock(&priv->lock);
	res = silabs_tercab_tuner_status(&priv->tuner, Si2158_TUNER_STATUS_CMD_INTACK_OK);
	mutex_unlock(&priv->lock);
	if (res != NO_SILABS_TERCAB_ERROR) {
		silabs_tercab_err("Si2177_L1_TUNER_STATUS() failed (err=%d)\n", res);
		return -EIO;
	}

	//vco_code =  priv->tuner.rsp.tuner_status.vco_code;
	//tc       =  priv->tuner.rsp.tuner_status.tc;
	//rssil    =  priv->tuner.rsp.tuner_status.rssil;
	//rssih    =  priv->tuner.rsp.tuner_status.rssih;
	rssi     =  priv->tuner.rsp.tuner_status.rssi;
	freq     =  priv->tuner.rsp.tuner_status.freq;
	mode     =  priv->tuner.rsp.tuner_status.mode;

	silabs_tercab_dbg("freq=%u  mode=%u\n", freq, mode);

	if (priv->mode == SILABS_TERCAB_ANALOG) {
		mutex_lock(&priv->lock);
		res = silabs_tercab_atv_status(priv, Si2158_ATV_STATUS_CMD_INTACK_OK);
		mutex_unlock(&priv->lock);
		if (res != NO_SILABS_TERCAB_ERROR) {
			silabs_tercab_err("Si2177_L1_ATV_STATUS() failed (err=%d)\n", res);
			return -EIO;
		}
		ATV_Sync_Lock    = priv->tuner.rsp.atv_status.pcl;
		ATV_Master_Lock  = priv->tuner.rsp.atv_status.dl;

		silabs_tercab_dbg("chlint           = %d\n", priv->tuner.rsp.atv_status.chlint);
		silabs_tercab_dbg("pclint           = %d\n", priv->tuner.rsp.atv_status.pclint);
		silabs_tercab_dbg("dlint            = %d\n", priv->tuner.rsp.atv_status.dlint);
		silabs_tercab_dbg("snrlint          = %d\n", priv->tuner.rsp.atv_status.snrlint);
		silabs_tercab_dbg("snrhint          = %d\n", priv->tuner.rsp.atv_status.snrhint);
		silabs_tercab_dbg("chl              = %d\n", priv->tuner.rsp.atv_status.chl);
		silabs_tercab_dbg("snrl             = %d\n", priv->tuner.rsp.atv_status.snrl);
		silabs_tercab_dbg("snrh             = %d\n", priv->tuner.rsp.atv_status.snrh);
		silabs_tercab_dbg("video_snr        = %d\n", priv->tuner.rsp.atv_status.video_snr);
		silabs_tercab_dbg("afc_freq         = %d\n", priv->tuner.rsp.atv_status.afc_freq);
		silabs_tercab_dbg("video_sc_spacing = %d\n", priv->tuner.rsp.atv_status.video_sc_spacing);
		silabs_tercab_dbg("video_sys        = %d\n", priv->tuner.rsp.atv_status.video_sys);
		silabs_tercab_dbg("lines            = %d\n", priv->tuner.rsp.atv_status.lines);
		silabs_tercab_dbg("audio_sys        = %d\n", priv->tuner.rsp.atv_status.audio_sys);
		silabs_tercab_dbg("audio_demod_mode = %d\n", priv->tuner.rsp.atv_status.audio_demod_mode);
		silabs_tercab_dbg("audio_chan_bw    = %d\n", priv->tuner.rsp.atv_status.audio_chan_bw);
		silabs_tercab_dbg("sound_level      = %d\n", priv->tuner.rsp.atv_status.sound_level);

		silabs_tercab_dbg("ATV_Sync_Lock=%u  ATV_Master_Lock=%u\n", ATV_Sync_Lock, ATV_Master_Lock);

		if (priv->tuner.rsp.atv_status.chl && priv->tuner.rsp.atv_status.pcl) {
			silabs_tercab_dbg("RSSI analog: %ddBm\n", rssi);
			*strength = (rssi > -50) ? (u16)rssi + 120 : 0;
		} else {
			silabs_tercab_dbg("RSSI analog: no signal\n");
			*strength = 0;
		}
	} else {
		mutex_lock(&priv->lock);
		res = silabs_tercab_dtv_status(&priv->tuner, Si2158_DTV_STATUS_CMD_INTACK_OK);
		mutex_unlock(&priv->lock);
		if (res != NO_SILABS_TERCAB_ERROR) {
			silabs_tercab_err("Si2177_L1_DTV_STATUS() failed (err=%d)\n", res);
			return -EIO;
		}
		chl                =  priv->tuner.rsp.dtv_status.chl;
		//bw                 =  priv->tuner.rsp.dtv_status.bw;
		//modulation         =  priv->tuner.rsp.dtv_status.modulation;
		silabs_tercab_dbg("RSSI digital: %ddBm (channel=%u)\n", rssi, chl);
		*strength = (rssi > -50) ? (u16)rssi + 120 : 0;
	}
	return rc;
}

static int silabs_tercab_setup_configuration(struct dvb_frontend *fe,
		struct silabs_tercab_config *cfg)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;

	silabs_tercab_dbg("\n");

	priv->config = cfg;

	priv->qam_if_khz              = (cfg) ? cfg->qam_if_khz : 4000;
	priv->vsb_if_khz              = (cfg) ? cfg->vsb_if_khz : 3250;
	priv->clock_control           = (cfg) ? cfg->tuner_clock_control : _Si2168B_CLOCK_MANAGED;
	priv->agc_control             = (cfg) ? cfg->tuner_agc_control : 1;
	priv->fef_mode                = (cfg) ? cfg->fef_mode : Si2177_FEF_MODE_SLOW_NORMAL_AGC;
	priv->crystal_trim_xo_cap     = (cfg) ? cfg->crystal_trim_xo_cap : Si2158_CRYSTAL_TRIM_PROP_XO_CAP_DEFAULT;
	priv->indirect_i2c_connection = (cfg) ? cfg->indirect_i2c_connection : 1;
	return 0;
}

static int silabs_tercab_set_config(struct dvb_frontend *fe, void *priv_cfg)
{
	struct silabs_tercab_priv *priv = fe->tuner_priv;
	struct silabs_tercab_config *cfg = (struct silabs_tercab_config *) priv_cfg;

	silabs_tercab_dbg("\n");

	silabs_tercab_setup_configuration(fe, cfg);

	return 0;
}

int silabs_tercab_autodetection(struct i2c_adapter* i2c_adapter, u8 i2c_addr)
{
	u8 power_up_clock_mode = 0; /* clock mode xtal */
	u8 power_up_en_xout = 0;    /* disable xout */
	struct silabs_tercab_priv *priv = NULL; /* for debug output only */
	struct part_info part_info;
	silabs_tercab_status status;
	int return_code = 0;

	silabs_tercab_info("%s(): i2c addr=0x%02X  clock mode=%u  en_xou=%u\n", __func__, i2c_addr, power_up_clock_mode, power_up_en_xout);

	/* always wait for CTS prior to POWER_UP command */
	if ((return_code = silabs_tercab_poll_cts(i2c_adapter, i2c_addr)) != 0) {
		silabs_tercab_err("silabs_tercab_poll_cts error 0x%02x\n", return_code);
		return -ENODEV;
	}

	if ((return_code = silabs_tercab_power_up(i2c_adapter, i2c_addr, Si2158_POWER_UP_CMD_SUBCODE_CODE,
			power_up_clock_mode,
			power_up_en_xout,
			Si2158_POWER_UP_CMD_PD_LDO_LDO_POWER_UP,
			Si2158_POWER_UP_CMD_RESERVED2_RESERVED,
			Si2158_POWER_UP_CMD_RESERVED3_RESERVED,
			Si2158_POWER_UP_CMD_RESERVED4_RESERVED,
			Si2158_POWER_UP_CMD_RESERVED5_RESERVED,
			Si2158_POWER_UP_CMD_RESERVED6_RESERVED,
			Si2158_POWER_UP_CMD_RESERVED7_RESERVED,
			Si2158_POWER_UP_CMD_RESET_RESET,
			Si2158_POWER_UP_CMD_CLOCK_FREQ_CLK_24MHZ,
			Si2158_POWER_UP_CMD_RESERVED8_RESERVED,
			Si2158_POWER_UP_CMD_FUNC_BOOTLOADER,
#ifndef __SI2158__
			Si2157_POWER_UP_CMD_RESERVED9_RESERVED,
#endif
			Si2158_POWER_UP_CMD_CTSIEN_DISABLE,
			Si2158_POWER_UP_CMD_WAKE_UP_WAKE_UP,
			&status)) != 0)
	{
		silabs_tercab_err("silabs_tercab_power_up() failed (err 0x%02x).\n", return_code);
		return -ENODEV;
	}

	/* Get the Part Info from the chip.   This command is only valid in Bootloader mode */
	if ((return_code = si2158_part_info(i2c_adapter, i2c_addr, &part_info)) != 0) {
		silabs_tercab_err("si2158_part_info() failed (err 0x%02x).\n", return_code);
		return -ENODEV;
	}
	switch (part_info.part) {
	case 57:
	case 77:
		/* supported */
		break;
	default:
		silabs_tercab_info("unsupported Silicon Labs tuner (part %d)\n", part_info.part);
		return -ENODEV;
	}

	silabs_tercab_info("detected Silicon Labs tuner Si21%d (Rev. %d)\n", part_info.part, part_info.chiprev);
	silabs_tercab_info("pmajor  %d\n",        part_info.pmajor );
	if (part_info.pmajor >= 0x30) {
		silabs_tercab_info("pmajor '%c'\n",   part_info.pmajor );
	}
	silabs_tercab_info("pminor  %d\n",        part_info.pminor );
	if (part_info.pminor >= 0x30) {
		silabs_tercab_info("pminor '%c'\n",   part_info.pminor );
	}
	silabs_tercab_info("pbuild %d\n",         part_info.pbuild );
	silabs_tercab_info("romid %3d/0x%02x\n",  part_info.romid,  part_info.romid );

	return 0; /* Silicon Labs tuner detected */
}
EXPORT_SYMBOL(silabs_tercab_autodetection);

static const struct dvb_tuner_ops si2158_tuner_ops = {
		.info = {
				.name = "Silicon Labs terrestrial/cable tuner",
				.frequency_min  =  45000000,
				.frequency_max  = 864000000,
				.frequency_step =     62500
		},
		/* int (*release)(struct dvb_frontend *fe); */
		.release           = silabs_tercab_release,
		/* int (*init)(struct dvb_frontend *fe); */
		.init              = silabs_tercab_initialize,
		/* int (*sleep)(struct dvb_frontend *fe); */
		.sleep             = silabs_tercab_sleep,

		/** This is for simple PLLs - set all parameters in one go. */
		/* int (*set_params)(struct dvb_frontend *fe); */
		.set_params        = silabs_tercab_set_params,
		/* int (*set_analog_params)(struct dvb_frontend *fe, struct analog_parameters *p); */
		.set_analog_params = silabs_tercab_set_analog_params,

		/** This is support for demods like the mt352 - fills out the supplied buffer with what to write. */
		/* int (*calc_regs)(struct dvb_frontend *fe, u8 *buf, int buf_len); */

		/** This is to allow setting tuner-specific configs */
		/* int (*set_config)(struct dvb_frontend *fe, void *priv_cfg); */
		.set_config        = silabs_tercab_set_config,

		/* int (*get_frequency)(struct dvb_frontend *fe, u32 *frequency); */
		.get_frequency     = silabs_tercab_get_frequency,
		/* int (*get_bandwidth)(struct dvb_frontend *fe, u32 *bandwidth); */
		.get_bandwidth     = silabs_tercab_get_bandwidth,
		/* int (*get_if_frequency)(struct dvb_frontend *fe, u32 *frequency); */
		.get_if_frequency  = silabs_tercab_get_if_frequency,

		/* int (*get_status)(struct dvb_frontend *fe, u32 *status); */
		.get_status        = silabs_tercab_get_status,
		/* int (*get_rf_strength)(struct dvb_frontend *fe, u16 *strength); */
		.get_rf_strength   = silabs_tercab_get_signal,
		/* int (*get_afc)(struct dvb_frontend *fe, s32 *afc); */

		/** These are provided separately from set_params in order to facilitate silicon
		 * tuners which require sophisticated tuning loops, controlling each parameter separately. */
		/* int (*set_frequency)(struct dvb_frontend *fe, u32 frequency); */
		/* int (*set_bandwidth)(struct dvb_frontend *fe, u32 bandwidth); */

		/*
		 * These are provided separately from set_params in order to facilitate silicon
		 * tuners which require sophisticated tuning loops, controlling each parameter separately.
		 */
		/* int (*set_state)(struct dvb_frontend *fe, enum tuner_param param, struct tuner_state *state); */
		/* int (*get_state)(struct dvb_frontend *fe, enum tuner_param param, struct tuner_state *state); */
};

struct dvb_frontend *silabs_tercab_attach(struct dvb_frontend *fe,
		struct i2c_adapter *i2c,
		struct silabs_tercab_config *cfg)
{
	struct silabs_tercab_priv *priv = NULL;
	struct part_info part_info;
	int instance, ret;

	mutex_lock(&silabs_tercab_list_mutex);

	if (!cfg) {
		silabs_tercab_err("no configuration submitted\n");
		goto fail;
	}

	instance = hybrid_tuner_request_state(struct silabs_tercab_priv, priv,
			hybrid_tuner_instance_list,
			i2c, cfg->tuner_address, "silabs_tercab");

	switch (instance) {
	case 0:
		goto fail;
	case 1:
		/* new tuner instance */
		silabs_tercab_info("%s(): new instance for tuner @0x%02x\n", __func__, cfg->tuner_address);
		fe->tuner_priv = priv;

		silabs_tercab_setup_configuration(fe, cfg);

		mutex_init(&priv->lock);

		break;
	default:
		/* existing tuner instance */
		fe->tuner_priv = priv;

		/* allow dvb driver to override configuration settings */
		if (cfg) {
			silabs_tercab_info("%s(0x%02X): dvb driver submitted configuration\n", __func__, cfg->tuner_address);
			priv->clock_control       = cfg->tuner_clock_control;
			priv->agc_control         = cfg->tuner_agc_control;
			priv->fef_mode            = cfg->fef_mode;
			priv->crystal_trim_xo_cap = cfg->crystal_trim_xo_cap;
		} else {
			silabs_tercab_info("%s(0x%02X): default configuration\n", __func__, priv->tuner.i2c_addr);
		}
		break;
	}

	if (!priv->indirect_i2c_connection && fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1); /* enable i2c gate */

	if (silabs_tercab_autodetection(priv->i2c_props.adap, cfg->tuner_address)) {
		silabs_tercab_err("autodetection failed.\n");
		goto fail;
	}

	/* Get the Part Info from the chip.   This command is only valid in Bootloader mode */
	if (si2158_part_info(priv->i2c_props.adap, cfg->tuner_address, &part_info)) {
		silabs_tercab_err("si2158_part_info() failed \n");
		goto fail;
	}
	priv->tuner.part    = part_info.part;
	priv->tuner.chiprev = part_info.chiprev;

	silabs_tercab_info("%s(): Silicon Labs tuner Si21%02d rev. %d @0x%02x\n", __func__, priv->tuner.part, priv->tuner.chiprev, cfg->tuner_address);

	if (!silabs_tercab_sw_init(priv, priv->i2c_props.addr, NULL, priv->i2c_props.adap)) {
		silabs_tercab_err("silabs_tercab_sw_init() failed.\n");
		goto fail;
	}

	if (silabs_tercab_initialize(fe)) {
		silabs_tercab_err("silabs_tercab_initialize() failed.\n");
		goto fail;
	}

	/* enter standby mode, with required output features enabled */
	ret = silabs_tercab_sleep(fe);
	silabs_tercab_fail(ret);

	if (!priv->indirect_i2c_connection && fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0); /* disable i2c gate */

	mutex_unlock(&silabs_tercab_list_mutex);

	memcpy(&fe->ops.tuner_ops, &si2158_tuner_ops, sizeof(struct dvb_tuner_ops));

	return fe;

fail:
	mutex_unlock(&silabs_tercab_list_mutex);

	if (!priv->indirect_i2c_connection && fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0); /* disable i2c gate */

	silabs_tercab_release(fe);
	return NULL;
}
EXPORT_SYMBOL(silabs_tercab_attach);

static int silabs_tercab_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct silabs_tercab_config *cfg = client->dev.platform_data;
	struct silabs_tercab_priv *priv = NULL;
	struct dvb_frontend *fe;

	printk(KERN_INFO "%s: attaching Silicon Labs tuner...\n", __func__);

	fe = silabs_tercab_attach(cfg->fe, client->adapter, cfg);

	if (fe == NULL) {
		printk(KERN_ERR "%s: attaching Silicon Labs tuner failed\n", __func__);
		dev_dbg(&client->dev, "%s: failed\n", __func__);
		return -ENODEV;
	}
	priv = fe->tuner_priv;
	priv->client = client;
	priv->fe = cfg->fe;

	i2c_set_clientdata(client, fe->tuner_priv);

	dev_info(&priv->client->dev,
			"%s: Silicon Labs Tuner successfully attached\n",
			KBUILD_MODNAME);

	return 0;
}

static int silabs_tercab_remove(struct i2c_client *client)
{
	struct silabs_tercab_priv *priv = i2c_get_clientdata(client);
	struct dvb_frontend *fe = priv->fe;

	dev_dbg(&client->dev, "%s:\n", __func__);

	memset(&fe->ops.tuner_ops, 0, sizeof(struct dvb_tuner_ops));
	silabs_tercab_release(fe);
	return 0;
}

static const struct i2c_device_id silabs_tercab_id[] = {
		{"silabs_tercab", 0},
		{}
};
MODULE_DEVICE_TABLE(i2c, silabs_tercab_id);

static struct i2c_driver silabs_tercab_driver = {
		.driver = {
				.owner	= THIS_MODULE,
				.name	= "silabs_tercab",
		},
		.probe		= silabs_tercab_probe,
		.remove		= silabs_tercab_remove,
		.id_table	= silabs_tercab_id,
};

module_i2c_driver(silabs_tercab_driver);

MODULE_DESCRIPTION("Silicon Labs terrestrial/cable hybrid tuner driver");
MODULE_AUTHOR("Source code provided by Silicon Laboratories Inc.");
MODULE_AUTHOR("Henning Garbers <hgarbers@pctvsystems.com>");
MODULE_LICENSE("PROPRIETARY AND CONFIDENTIAL");
MODULE_VERSION("2015-02-05");

/*
 * Overrides for Emacs so that we follow Linus's tabbing style.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
