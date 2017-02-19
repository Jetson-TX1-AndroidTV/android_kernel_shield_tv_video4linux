/* DVB compliant Linux driver for the DVB-T/T2/C Si2168B demodulator
*
* Copyright (C) 2015 PCTV Systems S.à r.l & Silicon Laboratories Inc.
*
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <asm/div64.h>

#include "dvb_frontend.h"
#include "si2168b_priv.h"
#include "silabs_tercab.h"

static int sidebug = 0;
module_param_named(debug, sidebug, int, 0644);

int _sitrace = 0;
module_param_named(sitrace, _sitrace, int, 0644);

int mutex_on = 1;
module_param_named(mutex, mutex_on, int, 0644);

/* #define __MCNS__ */

#define siprintk(args...) \
	do { \
		if (sidebug) \
			printk(KERN_DEBUG "Si2168B: " args); \
	} while (0)

static inline void _mutex_lock(struct mutex *lock)
{
	if (mutex_on) {
		mutex_lock(lock);
	}
}

static inline void _mutex_unlock(struct mutex *lock)
{
	if (mutex_on) {
		mutex_unlock(lock);
	}
}

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

typedef enum custom_ts_mode_enum {
	SILABS_TS_TRISTATE = 0,
	SILABS_TS_SERIAL = 1,
	SILABS_TS_PARALLEL = 2,
	SILABS_TS_GPIF = 3,
	SILABS_TS_OFF = 4
} custom_ts_mode_enum;

/* Standard code values used by the top-level application               */
/* <porting> set these values to match the top-level application values */
typedef enum custom_standard_enum {
	SILABS_ANALOG = 4,
	SILABS_DVB_T = 0,
	SILABS_DVB_C = 1,
	SILABS_DVB_S = 2,
	SILABS_DVB_S2 = 3,
	SILABS_DVB_T2 = 5,
	SILABS_DSS = 6,
	SILABS_MCNS = 7,
	SILABS_DVB_C2 = 8,
	SILABS_SLEEP = 100
} custom_standard_enum;

typedef enum custom_constel_enum {
	SILABS_QAMAUTO = -1,
	SILABS_QAM16 = 0,
	SILABS_QAM32 = 1,
	SILABS_QAM64 = 2,
	SILABS_QAM128 = 3,
	SILABS_QAM256 = 4,
	SILABS_QPSK = 5,
	SILABS_8PSK = 6,
	SILABS_QAM1024 = 7,
	SILABS_QAM4096 = 8
} custom_constel_enum;

typedef enum custom_stream_enum {
	SILABS_HP = 0,
	SILABS_LP = 1
} custom_stream_enum;

typedef enum custom_t2_plp_type {
	SILABS_PLP_TYPE_COMMON = 0,
	SILABS_PLP_TYPE_DATA_TYPE1 = 1,
	SILABS_PLP_TYPE_DATA_TYPE2 = 2
} custom_t2_plp_type;

typedef enum custom_hierarchy_enum {
	SILABS_HIERARCHY_NONE = 0,
	SILABS_HIERARCHY_ALFA1 = 1,
	SILABS_HIERARCHY_ALFA2 = 2,
	SILABS_HIERARCHY_ALFA4 = 3
} custom_hierarchy_enum;

/************************************************************************************************************************
  system_time function
  Use:        current system time retrieval function
              Used to retrieve the current system time in milliseconds
  Returns:    The current system time in milliseconds
  Porting:    Needs to use the final system call
************************************************************************************************************************/
static inline u32 system_time(void)
{
  /* <porting> Replace 'clock' by whatever is necessary to return the system time in milliseconds */
  /* return (int)clock()*1000/CLOCKS_PER_SEC; */
  return (u32)jiffies_to_msecs(jiffies);
}

#ifdef SiTRACES
#define SiTRACES_BUFFER_LENGTH  100000
#define SiTRACES_NAMESIZE           30
#define SiTRACES_FUNCTION_NAMESIZE  30

#define CUSTOM_PRINTF(args...) \
	do { \
		if (_sitrace) \
			printk(KERN_INFO "si2168b: " args); \
	} while (0)

typedef enum TYPE_OF_OUTPUT {
    TRACE_NONE = 0,
    TRACE_STDOUT,
    TRACE_EXTERN_FILE,
    TRACE_MEMORY
} TYPE_OF_OUTPUT;

static TYPE_OF_OUTPUT trace_output_type;

static u8  trace_init_done        = 0;
static u8  trace_suspend          = 0;
static u8  trace_config_lines     = 0;
static u8  trace_config_functions = 0;
static u8  trace_config_time      = 0;
static u8  trace_config_verbose   = 0;
static u32 trace_linenumber       = 0;

static char trace_timer[50];
static char trace_elapsed_time[20];
static char trace_source_function[SiTRACES_FUNCTION_NAMESIZE+1];
static u8 trace_skip_info = 0;

/************************************************************************************************************************
  traceElapsedTime function
  Use:        SiTRACES time formatting function.
              It allows the user to know when the trace has been treated.
              It is used to insert the time before the trace when -time 'on'.
  Returns:    text containing the execution time in HH:MM:SS.ms format.
  Porting:    Not compiled if SiTRACES is not defined in Silabs_L0_API.h.
 ************************************************************************************************************************/
static char *get_elapsed_time(void)
{
	u32 time_elapsed, ms, sec, min, hours;

	time_elapsed = system_time();
	ms = time_elapsed % 1000;
	time_elapsed = time_elapsed/1000;
	sec = time_elapsed % 60;
	time_elapsed = time_elapsed/60;
	min = time_elapsed % 60;
	time_elapsed = time_elapsed/60;
	hours = time_elapsed % 60;
	sprintf(trace_elapsed_time, "%02u:%02u:%02u.%03u ", hours, min, sec, ms);

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
static void trace_to_stdout(char* trace)
{
	if (!trace_skip_info) {
		if (trace_config_lines) {
			CUSTOM_PRINTF("%5u "  , trace_linenumber);
		}
		if (trace_config_functions) {
			CUSTOM_PRINTF("%-30s ", trace_source_function);
		}
	}
	if (trace_config_time) {
		CUSTOM_PRINTF("%s ", get_elapsed_time());
	}
	CUSTOM_PRINTF("%s", trace);
}

/************************************************************************************************************************
  traceToDestination function
  Use:        switch the trace in the selected output mode.
  Comment:    In verbose mode, the trace is always displayed in stdout.
  Parameter:  trace, the trace string
  Returns:    void
  Porting:    Not compiled if SiTRACES is not defined in Silabs_L0_API.h.
************************************************************************************************************************/
static void trace_to_destination(char* trace)
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
	if (trace_config_lines + trace_config_functions + trace_config_time) {
		last = (int)strlen(trace)-1;
		if (trace[last] != 0x0a) {
			sprintf(trace, "%s\n", trace);
		}
	}
	trace_to_stdout(trace);
	if ((trace_config_verbose) & (trace_output_type!=TRACE_STDOUT)) {
		trace_to_stdout(trace);
	}
	if (strcmp(trace,"\n")==0) {
		trace_skip_info = 0;
	}
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
  Porting:    Not compiled if SiTRACES is not defined in Silabs_L0_API.h.
************************************************************************************************************************/
static void sitrace_function(const char *name, int number, const char *func, const char *fmt, ...)
{
	char message[850];
	const char *pfunc;
	va_list ap;

	if (trace_output_type == TRACE_NONE)
		return;

	/* print the line number in trace_linenumber */
	trace_linenumber = number;

	/*print the function name in trace_source_function */
	pfunc = func;
	sprintf(trace_source_function, "%s", "");
	if (strlen(pfunc) > SiTRACES_FUNCTION_NAMESIZE) {
		pfunc += (strlen(pfunc) - SiTRACES_FUNCTION_NAMESIZE) + 2;
		strcpy(trace_source_function, "..");
	}
	strncat(trace_source_function, pfunc, SiTRACES_FUNCTION_NAMESIZE-2);

	va_start(ap, fmt);
	vsnprintf(message, 900, fmt, ap);
	trace_to_destination(message);
	va_end(ap);
}

/************************************************************************************************************************
  SiTraceDefaultConfiguration function
  Use:        SiTRACES initialization function.
              It is called on the first call to L0_Init (only once).
              It defines the default output and inserts date and time in the default file.
  Returns:    void
  Porting:    Not compiled if SiTRACES is not defined in Silabs_L0_API.h.
************************************************************************************************************************/
static void sitrace_default_configuration(void)
{
    if (trace_init_done)
    	return;

    trace_output_type = TRACE_STDOUT;
    trace_init_done = 1;
    sprintf(trace_timer, "time");
}

static inline void sitraces_suspend(void)
{
	trace_suspend = 1;
}

static inline void sitraces_resume(void)
{
	trace_suspend = 0;
}

/***********************************************************************************************************************
  si2168b_error_text function
  Use:        Error information function
              Used to retrieve a text based on an error code
  Returns:    the error text
  Parameter:  error_code the error code.
  Porting:    Useful for application development for debug purposes.
  Porting:    May not be required for the final application, can be removed if not used.
 ***********************************************************************************************************************/
static char* si2168b_error_text(int error_code)
{
	switch (error_code) {
	case NO_Si2168B_ERROR                     : return (char *)"No Si2168B error";
	case ERROR_Si2168B_ALLOCATING_CONTEXT     : return (char *)"Error while allocating Si2168B context";
	case ERROR_Si2168B_PARAMETER_OUT_OF_RANGE : return (char *)"Si2168B parameter(s) out of range";
	case ERROR_Si2168B_SENDING_COMMAND        : return (char *)"Error while sending Si2168B command";
	case ERROR_Si2168B_CTS_TIMEOUT            : return (char *)"Si2168B CTS timeout";
	case ERROR_Si2168B_ERR                    : return (char *)"Si2168B Error (status 'err' bit 1)";
	case ERROR_Si2168B_POLLING_CTS            : return (char *)"Si2168B Error while polling CTS";
	case ERROR_Si2168B_POLLING_RESPONSE       : return (char *)"Si2168B Error while polling response";
	case ERROR_Si2168B_LOADING_FIRMWARE       : return (char *)"Si2168B Error while loading firmware";
	case ERROR_Si2168B_LOADING_BOOTBLOCK      : return (char *)"Si2168B Error while loading bootblock";
	case ERROR_Si2168B_STARTING_FIRMWARE      : return (char *)"Si2168B Error while starting firmware";
	case ERROR_Si2168B_SW_RESET               : return (char *)"Si2168B Error during software reset";
	case ERROR_Si2168B_INCOMPATIBLE_PART      : return (char *)"Si2168B Error Incompatible part";
	case ERROR_Si2168B_UNKNOWN_COMMAND        : return (char *)"Si2168B Error unknown command";
	case ERROR_Si2168B_UNKNOWN_PROPERTY       : return (char *)"Si2168B Error unknown property";
	default:
		return (char *)"Unknown Si2168B error code";
	}
}

/************************************************************************************************************************
  NAME: si2168b_trace_scan_status
  DESCRIPTION: traces the scan_status
  Parameter:  Pointer to Si2168B Context
  Returns:    void
************************************************************************************************************************/
static const char *si2168b_trace_scan_status(int scan_status)
{
	switch (scan_status) {
	case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_ANALOG_CHANNEL_FOUND  : { return "ANALOG  CHANNEL_FOUND"; break; }
	case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_DIGITAL_CHANNEL_FOUND : { return "DIGITAL CHANNEL_FOUND"; break; }
	case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_DEBUG                 : { return "DEBUG"                ; break; }
	case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_ERROR                 : { return "ERROR"                ; break; }
	case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_ENDED                 : { return "ENDED"                ; break; }
	case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_IDLE                  : { return "IDLE"                 ; break; }
	case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_SEARCHING             : { return "SEARCHING"            ; break; }
	case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_TUNE_REQUEST          : { return "TUNE_REQUEST"         ; break; }
	default                                                              : { return "Unknown!"             ; break; }
	}
}

/***********************************************************************************************************************
  si2168b_tag_text function
  Use:        Error information function
              Used to retrieve a text containing the TAG information (related to the code version)
  Returns:    the TAG text
  Porting:    May not be required for the final application, can be removed if not used.
 ***********************************************************************************************************************/
char* si2168b_tag_text(void)
{
	return (char *)"V0.2.6";
}

#else /* SiTRACES */
#define SiTRACE(...)               /* empty */
#define SiTracesSuspend()          /* empty */
#define SiTracesResume()           /* empty */
#endif /* SiTRACES */

static const struct dvb_frontend_ops si2168b_ops;

static int enable_tuner_i2c(void *p)
{
	/* nothing to do */
	return NO_Si2168B_ERROR;
}

static int disable_tuner_i2c(void *p)
{
	/* nothing to do */
	return NO_Si2168B_ERROR;
}

static int i2c_callback(void *p)
{
	/* nothing to do */
	return NO_Si2168B_ERROR;
}

/************************************************************************************************************************
  L0_ReadBytes function
  Use:        lowest layer read function
              Used to read a given number of bytes from the Layer 1 instance.
  Parameters: i2c, a pointer to the Layer 0 context.
              iI2CIndex, the index of the first byte to read.
              iNbBytes, the number of bytes to read.
              *pbtDataBuffer, a pointer to a buffer used to store the bytes.
  Returns:    the number of bytes read.
  Porting:    If a single connection mode is allowed, the entire switch can be replaced by a call to the final i2c read function
************************************************************************************************************************/
static u16 i2c_read_bytes(struct i2c_adapter *i2c_adap, u8 i2c_addr, u16 iNbBytes, u8 *pucDataBuffer)
{
	struct i2c_msg msg = {
			.addr  = i2c_addr,
			.flags = I2C_M_RD,
			.buf   = pucDataBuffer,
			.len   = iNbBytes,
	};

	if (i2c_adap == NULL) {
		SiTRACE("%s(): FATAL ERROR: i2c_adap is undefined!\n", __func__);
		return 0;
	}

	if (i2c_transfer(i2c_adap, &msg, 1) != 1) {
		SiTRACE("%s(): i2c transfer failed\n", __func__);
		return 0;
	}

	return iNbBytes;
}

/************************************************************************************************************************
  i2c_write_bytes function
  Use:        lowest layer write function
              Used to write a given number of bytes from the Layer 1 instance.
  Parameters: i2c, a pointer to the Layer 0 context.
              iNbBytes, the number of bytes to write.
              *pbtDataBuffer, a pointer to a buffer containing the bytes to write.
  Returns:    the number of bytes read.
  Porting:    If a single connection mode is allowed, the entire switch can be replaced by a call to the final i2c write function
************************************************************************************************************************/
static int i2c_write_bytes(struct i2c_adapter *i2c_adap, u8 i2c_addr, u16 iNbBytes, u8 *pucDataBuffer)
{
	struct i2c_msg msg = {
			.addr  = i2c_addr,
			.flags = 0,
			.buf   = pucDataBuffer,
			.len   = iNbBytes,
	};

	if (i2c_adap == NULL) {
		SiTRACE("%s(): FATAL ERROR: i2c_adap is undefined!\n", __func__);
		return 0;
	}

	if (iNbBytes > 64) {
		SiTRACE("%s(): numbers of bytes (%u) exceeds limit of 64\n", __func__, iNbBytes);
		return 0;
	}

	if (i2c_transfer(i2c_adap, &msg, 1) != 1) {
		SiTRACE("%s(): i2c transfer failed\n", __func__);
		return 0;
	}

	return iNbBytes;
}

/***********************************************************************************************************************
  si2168b_current_response_status function
  Use:        status checking function
              Used to fill the Si2168B_COMMON_REPLY_struct members with the ptDataBuffer byte's bits
  Comments:   The status byte definition being identical for all commands,
              using this function to fill the status structure helps reducing the code size
  Parameter: ptDataBuffer  a single byte received when reading a command's response (the first byte)
  Returns:   0 if the err bit (bit 6) is unset, 1 otherwise
 ***********************************************************************************************************************/
static u8 si2168b_current_response_status(si2168b_context *ctx, u8 databuffer)
{
	ctx->status_ddint   = ((databuffer >> 0 ) & 0x01);
	ctx->status_scanint = ((databuffer >> 1 ) & 0x01);
	ctx->status_err     = ((databuffer >> 6 ) & 0x01);
	ctx->status_cts     = ((databuffer >> 7 ) & 0x01);

	return (ctx->status_err ? ERROR_Si2168B_ERR : NO_Si2168B_ERROR);
}

/***********************************************************************************************************************
  si2168b_poll_for_response function
  Use:        command response retrieval function
              Used to retrieve the command response in the provided buffer
  Comments:   The status byte definition being identical for all commands,
              using this function to fill the status structure helps reducing the code size
              max timeout = 1000 ms

  Parameter:  nbBytes          the number of response bytes to read
  Parameter:  pByteBuffer      a buffer into which bytes will be stored
  Returns:    0 if no error, an error code otherwise
 ***********************************************************************************************************************/
static u8 si2168b_poll_for_response(si2168b_context *ctx, u16 count, u8 *buffer)
{
	u8 dbg[7];
	u32 start_time = system_time();

	do {
		if (i2c_read_bytes(ctx->i2c_adap, ctx->i2c_addr, count, buffer) != count) {
			SiTRACE("%s(): ERROR reading byte 0!\n", __func__);
			return ERROR_Si2168B_POLLING_RESPONSE;
		}
		/* return response err flag if CTS set */
		if (buffer[0] & 0x80)  {
			/* for debug purpose, read and trace 2 bytes in case the error bit is set */
			if (buffer[0] & 0x40)  {
				i2c_read_bytes(ctx->i2c_adap, ctx->i2c_addr, 7, &(dbg[0]) );
				SiTRACE("Si2168B debug bytes 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", dbg[0], dbg[1], dbg[2], dbg[3], dbg[4], dbg[5], dbg[6]);
			}
			return si2168b_current_response_status(ctx, buffer[0]);
		}
		msleep(10); /* FGR - pause a bit rather than just spinning on I2C */
	} while (system_time() - start_time < 1000); /* wait a maximum of 1000ms */

	SiTRACE("%s(): ERROR CTS Timeout!\n", __func__);
	return ERROR_Si2168B_CTS_TIMEOUT;
}

/*---------------------------------------------------*/
/* Si2168B_I2C_PASSTHROUGH COMMAND                  */
/*---------------------------------------------------*/
static u8 si2168b_i2c_passthrough(si2168b_context *ctx,
		u8 subcode,
		u8 i2c_passthru,
		u8 reserved)
{
	u8 cmdByteBuffer[3];
	u8 ret = NO_Si2168B_ERROR;

	/*ctx->rsp->i2c_passthrough.STATUS = ctx->status;*/

	SiTRACE("%s()\n", __func__);

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_I2C_PASSTHROUGH_CMD;
	cmdByteBuffer[1] = (u8) ( ( subcode      & Si2168B_I2C_PASSTHROUGH_CMD_SUBCODE_MASK      ) << Si2168B_I2C_PASSTHROUGH_CMD_SUBCODE_LSB     );
	cmdByteBuffer[2] = (u8) ( ( i2c_passthru & Si2168B_I2C_PASSTHROUGH_CMD_I2C_PASSTHRU_MASK ) << Si2168B_I2C_PASSTHROUGH_CMD_I2C_PASSTHRU_LSB|
			( reserved     & Si2168B_I2C_PASSTHROUGH_CMD_RESERVED_MASK     ) << Si2168B_I2C_PASSTHROUGH_CMD_RESERVED_LSB    );

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 3, cmdByteBuffer) != 3) {
		SiTRACE("Error writing I2C_PASSTHROUGH bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
	}

	_mutex_unlock(&ctx->lock);

	return ret;
}

/************************************************************************************************************************
  si2168b_tuner_i2c_enable function
  Use:        Tuner i2c bus connection
              Used to allow communication with the tuners
  Parameter:  *front_end, the front-end handle
************************************************************************************************************************/
static u8 si2168b_tuner_i2c_enable(Si2168B_L2_Context *front_end)
{
    return si2168b_i2c_passthrough(front_end->demod, Si2168B_I2C_PASSTHROUGH_CMD_SUBCODE_CODE,
    		Si2168B_I2C_PASSTHROUGH_CMD_I2C_PASSTHRU_CLOSE, Si2168B_I2C_PASSTHROUGH_CMD_RESERVED_RESERVED);
}

/************************************************************************************************************************
  si2168b_tuner_i2c_disable function
  Use:        Tuner i2c bus connection
              Used to disconnect i2c communication with the tuners
  Parameter:  *front_end, the front-end handle
************************************************************************************************************************/
static u8 si2168b_tuner_i2c_disable(Si2168B_L2_Context *front_end)
{
    return si2168b_i2c_passthrough(front_end->demod, Si2168B_I2C_PASSTHROUGH_CMD_SUBCODE_CODE,
    		Si2168B_I2C_PASSTHROUGH_CMD_I2C_PASSTHRU_OPEN, Si2168B_I2C_PASSTHROUGH_CMD_RESERVED_RESERVED);
}

/*---------------------------------------------------*/
/* Si2168B_SET_PROPERTY COMMAND                     */
/*---------------------------------------------------*/
static u8 si2168_set_property_cmd(si2168b_context *ctx, u16 prop, u16 data, Si2168B_SET_PROPERTY_CMD_REPLY_struct *set_property)
{
	const u8 reserved = 0;
	u8 error_code = 0;
	u8 cmdByteBuffer[6];
	u8 rspByteBuffer[4];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B SET_PROPERTY\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_SET_PROPERTY_CMD;
	cmdByteBuffer[1] = (u8) ( ( reserved & Si2168B_SET_PROPERTY_CMD_RESERVED_MASK ) << Si2168B_SET_PROPERTY_CMD_RESERVED_LSB);
	cmdByteBuffer[2] = (u8) ( ( prop     & Si2168B_SET_PROPERTY_CMD_PROP_MASK     ) << Si2168B_SET_PROPERTY_CMD_PROP_LSB    );
	cmdByteBuffer[3] = (u8) ((( prop     & Si2168B_SET_PROPERTY_CMD_PROP_MASK     ) << Si2168B_SET_PROPERTY_CMD_PROP_LSB    )>>8);
	cmdByteBuffer[4] = (u8) ( ( data     & Si2168B_SET_PROPERTY_CMD_DATA_MASK     ) << Si2168B_SET_PROPERTY_CMD_DATA_LSB    );
	cmdByteBuffer[5] = (u8) ((( data     & Si2168B_SET_PROPERTY_CMD_DATA_MASK     ) << Si2168B_SET_PROPERTY_CMD_DATA_LSB    )>>8);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 6, cmdByteBuffer) != 6) {
		SiTRACE("Error writing SET_PROPERTY bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 4, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling SET_PROPERTY response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	set_property->reserved =   (( ( (rspByteBuffer[1]  )) >> Si2168B_SET_PROPERTY_RESPONSE_RESERVED_LSB ) & Si2168B_SET_PROPERTY_RESPONSE_RESERVED_MASK );
	set_property->data     =   (( ( (rspByteBuffer[2]  ) | (rspByteBuffer[3]  << 8 )) >> Si2168B_SET_PROPERTY_RESPONSE_DATA_LSB     ) & Si2168B_SET_PROPERTY_RESPONSE_DATA_MASK     );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/***********************************************************************************************************************
  si2168b_set_property function
  Use:        property set function
              Used to call L1_SET_PROPERTY with the property Id and data provided.
  Parameter: *api     the Si2168B context
  Parameter: prop     the property Id
  Parameter: data     the property bytes
  Behavior:  This function will only download the property if required.
               Conditions to download the property are:
                - The property changes
                - The propertyWriteMode is set to Si2168B_DOWNLOAD_ALWAYS
                - The property is unknown to Si2168B_PackProperty (this may be useful for debug purpose)
  Returns:    0 if no error, an error code otherwise
 ***********************************************************************************************************************/
static u8 si2168b_set_property(si2168b_context *ctx, u16 prop_code, u16 data)
{
	u8  res;
	Si2168B_SET_PROPERTY_CMD_REPLY_struct set_property;

	res = si2168_set_property_cmd(ctx, prop_code, data, &set_property);
	SiTRACE("si2168b_set_property: Setting Property 0x%04x to 0x%04x(%d)\n", prop_code, data, data);
	if (res != NO_Si2168B_ERROR) {
		SiTRACE("ERROR: si2168b_set_property: %s 0x%04x!\n\n", si2168b_error_text(res), prop_code);
	}
	return res;
}

/************************************************************************************************************************
 si2168b_set_ts_mode function
 Use:      Transport Stream control function
 Used to switch the TS output in the desired mode
 Parameter: mode the mode to switch to,
 clock mode
 ************************************************************************************************************************/
static u8 si2168b_set_ts_mode(struct Si2168B_Priv *priv, u8 ts_bus_mode)
{
	si2168b_context *demod = priv->si_front_end->demod;

	const u8 ts_clk_invert              = priv->config->ts_par_clk_invert;
	const u8 ts_clk_shift               = priv->config->ts_par_clk_shift;
	const u8 ts_mode_clk_gapped_en      = priv->config->clk_gapped_en; /* (default 'DISABLED') */
	const u8 ts_mode_clock              = priv->config->ts_clock_mode; /* (default 'AUTO_FIXED') */
	const u8 ts_data_strength           = 3; /* (default     3) */
	const u8 ts_clk_strength            = 3; /* (default     3) */
	const u8 ts_parallel_data_shape     = 2;
	const u8 ts_parallel_clk_shape      = 2;
	const u8 ts_gpif_data_shape         = 7;
	const u8 ts_gpif_clk_shape          = 7;
	const u8 ts_mode_special            = Si2168B_DD_TS_MODE_PROP_SPECIAL_FULL_TS;              /* (default 'FULL_TS') */
	const u8 ts_mode_ts_err_polarity    = Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_NOT_INVERTED; /* (default 'NOT_INVERTED') */
	const u8 ts_mode_ts_freq_resolution = Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_NORMAL;    /* (default 'NORMAL') */

	u8  ts_mode_mode = Si2168B_DD_TS_MODE_PROP_MODE_TRISTATE; /* (default 'TRISTATE') */
	u8  ret = NO_Si2168B_ERROR;
	u16 data;

	siprintk("%s() ts_bus_mode=%u\n", __func__, ts_bus_mode);

	switch (ts_bus_mode) {
	case SILABS_TS_SERIAL:
		ts_mode_mode = Si2168B_DD_TS_MODE_PROP_MODE_SERIAL;
		break;
	case SILABS_TS_PARALLEL:
	    data = (ts_data_strength       & Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_STRENGTH_MASK) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_STRENGTH_LSB  |
	           (ts_parallel_data_shape & Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_SHAPE_MASK   ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_SHAPE_LSB  |
	           (ts_clk_strength        & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_STRENGTH_MASK ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_STRENGTH_LSB  |
	           (ts_parallel_clk_shape  & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHAPE_MASK    ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHAPE_LSB  |
	           (ts_clk_invert          & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_MASK   ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_LSB  |
	           (ts_clk_shift           & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHIFT_MASK    ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHIFT_LSB;
        ret = si2168b_set_property(demod, Si2168B_DD_TS_SETUP_PAR_PROP, data);
		ts_mode_mode = Si2168B_DD_TS_MODE_PROP_MODE_PARALLEL;
		break;
	case SILABS_TS_TRISTATE:
		ts_mode_mode = Si2168B_DD_TS_MODE_PROP_MODE_TRISTATE;
		break;
	case SILABS_TS_OFF:
		ts_mode_mode = Si2168B_DD_TS_MODE_PROP_MODE_OFF;
		break;
	case SILABS_TS_GPIF:
	    data = (ts_data_strength   & Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_STRENGTH_MASK) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_STRENGTH_LSB  |
	           (ts_gpif_data_shape & Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_SHAPE_MASK   ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_SHAPE_LSB  |
	           (ts_clk_strength    & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_STRENGTH_MASK ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_STRENGTH_LSB  |
	           (ts_gpif_clk_shape  & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHAPE_MASK    ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHAPE_LSB  |
	           (ts_clk_invert      & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_MASK   ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_LSB  |
	           (ts_clk_shift       & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHIFT_MASK    ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHIFT_LSB;
		ret = si2168b_set_property(demod, Si2168B_DD_TS_SETUP_PAR_PROP, data);
		ts_mode_mode = Si2168B_DD_TS_MODE_PROP_MODE_GPIF;
		break;
	default:
		/* return si2168b_set_ts_mode(priv, SILABS_TS_TRISTATE); */
		ts_mode_mode = Si2168B_DD_TS_MODE_PROP_MODE_TRISTATE;
		break;
	}

	if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): si2168b_set_property(Si2168B_DD_TS_SETUP_PAR_PROP) failed\n", __func__);
	}

    data = (ts_mode_mode               & Si2168B_DD_TS_MODE_PROP_MODE_MASK              ) << Si2168B_DD_TS_MODE_PROP_MODE_LSB  |
           (ts_mode_clock              & Si2168B_DD_TS_MODE_PROP_CLOCK_MASK             ) << Si2168B_DD_TS_MODE_PROP_CLOCK_LSB  |
           (ts_mode_clk_gapped_en      & Si2168B_DD_TS_MODE_PROP_CLK_GAPPED_EN_MASK     ) << Si2168B_DD_TS_MODE_PROP_CLK_GAPPED_EN_LSB  |
           (ts_mode_ts_err_polarity    & Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_MASK   ) << Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_LSB  |
           (ts_mode_special            & Si2168B_DD_TS_MODE_PROP_SPECIAL_MASK           ) << Si2168B_DD_TS_MODE_PROP_SPECIAL_LSB  |
           (ts_mode_ts_freq_resolution & Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_MASK) << Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_LSB;
    ret = si2168b_set_property(demod, Si2168B_DD_TS_MODE_PROP, data);

	if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): si2168b_set_property(Si2168B_DD_TS_MODE_PROP) failed\n", __func__);
	}
	siprintk("%s() DONE (ret=%d)\n", __func__, ret);
	return ret;
}

static int si2168b_i2c_gate_ctrl(struct dvb_frontend* fe, int enable)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;

	siprintk("%s() enable=%d\n", __func__, enable);

	if (enable) {
		if ( priv->config->indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
			if (priv->si_front_end->f_TER_tuner_enable(priv->si_front_end->callback) != NO_Si2168B_ERROR) {
				siprintk("%s(): f_TER_tuner_enable() failed\n", __func__);
				return -EIO;
			}
		} else {
			if (si2168b_tuner_i2c_enable(priv->si_front_end) != NO_Si2168B_ERROR) {
				siprintk("%s(): si2168b_tuner_i2c_enable() failed\n", __func__);
				return -EIO;
			}
		}
	} else {
		if ( priv->config->indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
			if (priv->si_front_end->f_TER_tuner_disable(priv->si_front_end->callback) != NO_Si2168B_ERROR) {
				siprintk("%s(): f_TER_tuner_disable() failed\n", __func__);
				return -EIO;
			}
		} else {
			if (si2168b_tuner_i2c_disable(priv->si_front_end) != NO_Si2168B_ERROR) {
				siprintk("%s(): si2168b_tuner_i2c_disable() failed\n", __func__);
				return -EIO;
			}
		}
	}
	siprintk("%s() I2C gate on Si2168B %sabled\n", __func__, enable ? "en" : "dis");
	return 0;
}

/************************************************************************************************************************
  si2168b_infos function
  Use:        software information function
              Used to retrieve information about the compilation
  Parameter:  front_end, a pointer to the Si2168B_L2_Context context to be initialized
  Parameter:  infoString, a text buffer to be filled with teh information. It must be initialized by the caller.
  Return:     the length of the information string
************************************************************************************************************************/
static int si2168b_infos(Si2168B_L2_Context *front_end, char *infoString_UNUSED)
{
	if (infoString_UNUSED == NULL)
		return 0;

	if (front_end == NULL) {
		SiTRACE("Si2168B front-end not initialized yet. Call si2168b_sw_init first!\n");
		return strlen(infoString_UNUSED);
	}

	SiTRACE("\n");
	SiTRACE("--------------------------------------\n");
	SiTRACE("Demod                Si2168B  at 0x%02x\n", front_end->demod->i2c_addr);
	SiTRACE("Demod                Source code %s\n", si2168b_tag_text() );
	SiTRACE("Terrestrial tuner    SiLabs\n");

	if ( front_end->demod->tuner_ter_clock_source == Si2168B_TER_Tuner_clock)
		SiTRACE("TER clock from  TER Tuner (%d MHz)\n", front_end->demod->tuner_ter_clock_freq);
	if ( front_end->demod->tuner_ter_clock_source == Si2168B_SAT_Tuner_clock)
		SiTRACE("TER clock from  SAT Tuner (%d MHz)\n", front_end->demod->tuner_ter_clock_freq);
	if ( front_end->demod->tuner_ter_clock_source == Si2168B_Xtal_clock)
		SiTRACE("TER clock from  Xtal      (%d MHz)\n", front_end->demod->tuner_ter_clock_freq);
	if ( front_end->demod->tuner_ter_clock_input == Si2168B_START_CLK_CMD_CLK_MODE_CLK_CLKIO)
		SiTRACE("TER clock input CLKIO\n");
	if ( front_end->demod->tuner_ter_clock_input == Si2168B_START_CLK_CMD_CLK_MODE_CLK_XTAL_IN)
		SiTRACE("TER clock input XTAL_IN\n");
	if ( front_end->demod->tuner_ter_clock_input == Si2168B_START_CLK_CMD_CLK_MODE_XTAL)
		SiTRACE("TER clock input XTAL\n");

	if (front_end->demod->fef_mode == Si2168B_FEF_MODE_SLOW_NORMAL_AGC)
		SiTRACE("FEF mode 'SLOW NORMAL AGC'\n");
	if (front_end->demod->fef_mode == Si2168B_FEF_MODE_SLOW_INITIAL_AGC)
		SiTRACE("FEF mode 'SLOW INITIAL AGC'\n");
	if (front_end->demod->fef_mode == Si2168B_FEF_MODE_FREEZE_PIN)
		SiTRACE("FEF mode 'FREEZE PIN'\n");
	if (front_end->demod->fef_mode != front_end->demod->fef_selection)
		SiTRACE("(CHANGED!)\n");

	SiTRACE("--------------------------------------\n");
	return strlen(infoString_UNUSED);
}

/***********************************************************************************************************************
  Si2168B_L1_API_Init function
  Use:        software initialisation function
              Used to initialize the software context
  Returns:    0 if no error
  Comments:   It should be called first and once only when starting the application
  Parameter:   **ppapi         a pointer to the api context to initialize
  Parameter:  add            the Si2168B I2C address
  Porting:    Allocation errors need to be properly managed.
  Porting:    I2C initialization needs to be adapted to use the available I2C functions
 ***********************************************************************************************************************/
static u8 si2168b_ctx_init(si2168b_context *ctx, u8 addr, struct i2c_adapter *i2c_adap, const struct si2168b_config *config)
{
	mutex_init(&ctx->lock);
	mutex_init(&ctx->ts_bus_ctrl_lock);
	ctx->i2c_addr = addr;
	ctx->i2c_adap = i2c_adap;
	ctx->address  = addr;

#ifdef SiTRACES
	if (!trace_init_done) {
		CUSTOM_PRINTF("********** SiTRACES activated *********\n");
		CUSTOM_PRINTF("Comment the '#define SiTRACES' line\n");
		CUSTOM_PRINTF("in Silabs_L0_API.h to de-activate all traces.\n");
		CUSTOM_PRINTF("***************************************\n");
		sitrace_default_configuration();
	}
#endif /* SiTRACES */

	/* Clock settings as per compilation flags                     */
	/*  For multi-frontend HW, these may be adapted later on,      */
	/*   using Si2168B_L1_API_TER_Clock and Si2168B_L1_API_SAT_Clock */
	/* ctx->dvbt2_fef_tuner_flag  = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_NOT_USED; */
	ctx->tuner_ter_clock_source  = Si2168B_TER_Tuner_clock;
	ctx->tuner_ter_clock_control = Si2168B_CLOCK_MANAGED;
	ctx->tuner_ter_clock_input   = Si2168B_CLOCK_MODE_TER;
	ctx->tuner_ter_clock_freq    = Si2168B_REF_FREQUENCY_TER;

	ctx->Si2168B_in_standby      = 0;

	ctx->dd_mode_modulation      = Si2168B_DD_MODE_PROP_MODULATION_DEFAULT;
	ctx->dd_mode_auto_detect     = Si2168B_DD_MODE_PROP_AUTO_DETECT_DEFAULT;
	ctx->dd_mode_invert_spectrum = Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_DEFAULT;
	ctx->dd_mode_bw              = Si2168B_DD_MODE_PROP_BW_DEFAULT;

	ctx->dvbt_hierarchy_stream   = Si2168B_DVBT_HIERARCHY_PROP_STREAM_DEFAULT;
	ctx->dvbc_symbol_rate        = Si2168B_DVBC_SYMBOL_RATE_PROP_RATE_DEFAULT;

	ctx->scan_fmin               = Si2168B_SCAN_FMIN_PROP_SCAN_FMIN_DEFAULT;
	ctx->scan_fmax               = Si2168B_SCAN_FMAX_PROP_SCAN_FMAX_DEFAULT;
	ctx->scan_symb_rate_min      = Si2168B_SCAN_SYMB_RATE_MIN_PROP_SCAN_SYMB_RATE_MIN_DEFAULT;
	ctx->scan_symb_rate_max      = Si2168B_SCAN_SYMB_RATE_MAX_PROP_SCAN_SYMB_RATE_MAX_DEFAULT;

	ctx->fef_mode                = config->fef_mode;
	ctx->fef_selection           = config->fef_mode;
	ctx->fef_pin                 = config->fef_pin;
	ctx->fef_level               = config->fef_level;

#ifdef L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP
	/* If the TER tuner has initial AGC speed control and it's the selected mode, activate it */
	if (ctx->fef_selection == Si2168B_FEF_MODE_SLOW_INITIAL_AGC) {
		SiTRACE("TER tuner FEF set to 'SLOW_INITIAL_AGC' mode\n");
		ctx->fef_mode = Si2168B_FEF_MODE_SLOW_INITIAL_AGC;
	}
#ifdef L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN_SETUP
	/* If the TER tuner has an AGC freeze pin and it's the selected mode, activate it */
	if (ctx->fef_selection == Si2168B_FEF_MODE_FREEZE_PIN) {
		SiTRACE("TER tuner FEF set to 'FREEZE_PIN' mode\n");
		ctx->fef_mode = Si2168B_FEF_MODE_FREEZE_PIN;
	}
#else /* L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN_SETUP */
	if (ctx->fef_selection == Si2168B_FEF_MODE_FREEZE_PIN) {
		SiTRACE("TER tuner FEF can not use 'FREEZE_PIN' mode, using 'SLOW_INITIAL_AGC' mode instead\n");
		ctx->fef_mode = Si2168B_FEF_MODE_SLOW_INITIAL_AGC;
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN_SETUP */
#else  /* L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP */
	SiTRACE("TER tuner FEF set to 'SLOW_NORMAL_AGC' mode\n");
#endif /* L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP */

	return NO_Si2168B_ERROR;
}

/************************************************************************************************************************
  si2168b_sw_init function
  Use:        software initialization function
              Used to initialize the Si2168B and tuner structures
  Behavior:   This function performs all the steps necessary to initialize the Si2168B and tuner instances
  Parameter:  front_end, a pointer to the Si2168B_L2_Context context to be initialized
  Parameter:  demodAdd, the I2C address of the demod
  Parameter:  tunerAdd, the I2C address of the tuner
  Comments:     It MUST be called first and once before using any other function.
                It can be used to build a multi-demod/multi-tuner application, if called several times from the upper layer with different pointers and addresses
                After execution, all demod and tuner functions are accessible.
************************************************************************************************************************/
static char si2168b_sw_init(struct Si2168B_Priv *priv
		, int tunerAdd_Ter
		, Si2168B_INDIRECT_I2C_FUNC TER_tuner_enable_func
		, Si2168B_INDIRECT_I2C_FUNC TER_tuner_disable_func
		, void *p_context)
{
	Si2168B_L2_Context *front_end = priv->si_front_end;
	char infoStringBuffer[1000] = { 0 };
	char *infoString;
	infoString = &(infoStringBuffer[0]);

	/* Pointers initialization */
	front_end->Si2168B_init_done   = 0;
	front_end->first_init_done     = 0;
	front_end->handshakeUsed       = 0; /* set to '0' by default for compatibility with previous versions */
	front_end->handshakeOn         = 0;
	front_end->handshakePeriod_ms  = 1000;
	front_end->TER_init_done       = 0;
	front_end->auto_detect_TER     = 1;
	front_end->f_TER_tuner_enable  = TER_tuner_enable_func;
	front_end->f_TER_tuner_disable = TER_tuner_disable_func;
	front_end->tuner_indirect_i2c_connection = priv->config->indirect_i2c_connection;

	/* Calling underlying SW initialization functions */
	si2168b_ctx_init(front_end->demod, priv->config->demod_address, priv->i2c, priv->config);
	SiTRACE("Si2168B_L2_EVB_SW_Init starting...\n");

	/* SiLabs_TER_Tuner_L1_API_Init(front_end->tuner_ter, tunerAdd_Ter, i2c_adap); */
	/* done in tuner module startup */

	front_end->callback = p_context;
#ifdef SiTRACE
	if (si2168b_infos(front_end, infoString))  {
		SiTRACE("%s\n", infoString);
	}
#endif /* SiTRACE */
	SiTRACE("Si2168B_L2_EVB_SW_Init complete\n");
	return 1;
}

/************************************************************************************************************************
  si2168b_media function
  Use:        media retrieval function
              Used to retrieve the media used by the Si2168B
************************************************************************************************************************/
static u8 si2168b_media(si2168b_context *ctx)
{
	switch (ctx->dd_mode_modulation) {
	default:
	case Si2168B_DD_MODE_PROP_MODULATION_AUTO_DETECT:
		switch (ctx->dd_mode_auto_detect) {
		default:
			break;
		case Si2168B_DD_MODE_PROP_AUTO_DETECT_AUTO_DVB_T_T2:
			return Si2168B_TERRESTRIAL;
		}
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
			return Si2168B_TERRESTRIAL;
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
		return Si2168B_TERRESTRIAL;
	case Si2168B_DD_MODE_PROP_MODULATION_DVBC:
#ifdef __MCNS__
	case Si2168B_DD_MODE_PROP_MODULATION_MCNS:
#endif /* __MCNS__ */
		return Si2168B_TERRESTRIAL;
	}
	return 0;
}

/*---------------------------------------------------*/
/* Si2168B_START_CLK COMMAND                        */
/*---------------------------------------------------*/
static u8 si2168b_start_clk(si2168b_context *ctx,
		u8  subcode,
		u8  reserved1,
		u8  tune_cap,
		u8  reserved2,
		u16 clk_mode,
		u8  reserved3,
		u8  reserved4,
		u8  start_clk)
{
	u8 cmdByteBuffer[13];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B START_CLK\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_START_CLK_CMD;
	cmdByteBuffer[1] = (u8) ( ( subcode   & Si2168B_START_CLK_CMD_SUBCODE_MASK   ) << Si2168B_START_CLK_CMD_SUBCODE_LSB  );
	cmdByteBuffer[2] = (u8) ( ( reserved1 & Si2168B_START_CLK_CMD_RESERVED1_MASK ) << Si2168B_START_CLK_CMD_RESERVED1_LSB);
	cmdByteBuffer[3] = (u8) ( ( tune_cap  & Si2168B_START_CLK_CMD_TUNE_CAP_MASK  ) << Si2168B_START_CLK_CMD_TUNE_CAP_LSB |
			( reserved2 & Si2168B_START_CLK_CMD_RESERVED2_MASK ) << Si2168B_START_CLK_CMD_RESERVED2_LSB);
	cmdByteBuffer[4] = (u8) ( ( clk_mode  & Si2168B_START_CLK_CMD_CLK_MODE_MASK  ) << Si2168B_START_CLK_CMD_CLK_MODE_LSB );
	cmdByteBuffer[5] = (u8) ((( clk_mode  & Si2168B_START_CLK_CMD_CLK_MODE_MASK  ) << Si2168B_START_CLK_CMD_CLK_MODE_LSB )>>8);
	cmdByteBuffer[6] = (u8) ( ( reserved3 & Si2168B_START_CLK_CMD_RESERVED3_MASK ) << Si2168B_START_CLK_CMD_RESERVED3_LSB);
	cmdByteBuffer[7] = (u8) ( ( reserved4 & Si2168B_START_CLK_CMD_RESERVED4_MASK ) << Si2168B_START_CLK_CMD_RESERVED4_LSB);
	cmdByteBuffer[8] = (u8)0x00;
	cmdByteBuffer[9] = (u8)0x00;
	cmdByteBuffer[10] = (u8)0x00;
	cmdByteBuffer[11] = (u8)0x00;
	cmdByteBuffer[12] = (u8) ( ( start_clk & Si2168B_START_CLK_CMD_START_CLK_MASK ) << Si2168B_START_CLK_CMD_START_CLK_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 13, cmdByteBuffer) != 13) {
		SiTRACE("Error writing START_CLK bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
	}

	_mutex_unlock(&ctx->lock);

	return ret;
}

/*---------------------------------------------------*/
/* Si2168B_POWER_UP COMMAND                         */
/*---------------------------------------------------*/
static u8 si2168b_power_up(si2168b_context *ctx,
		u8   subcode,
		u8   reset,
		u8   reserved2,
		u8   reserved4,
		u8   reserved1,
		u8   addr_mode,
		u8   reserved5,
		u8   func,
		u8   clock_freq,
		u8   ctsien,
		u8   wake_up)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[8];
	u8 rspByteBuffer[1];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B POWER_UP\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_POWER_UP_CMD;
	cmdByteBuffer[1] = (u8) ( ( subcode    & Si2168B_POWER_UP_CMD_SUBCODE_MASK    ) << Si2168B_POWER_UP_CMD_SUBCODE_LSB   );
	cmdByteBuffer[2] = (u8) ( ( reset      & Si2168B_POWER_UP_CMD_RESET_MASK      ) << Si2168B_POWER_UP_CMD_RESET_LSB     );
	cmdByteBuffer[3] = (u8) ( ( reserved2  & Si2168B_POWER_UP_CMD_RESERVED2_MASK  ) << Si2168B_POWER_UP_CMD_RESERVED2_LSB );
	cmdByteBuffer[4] = (u8) ( ( reserved4  & Si2168B_POWER_UP_CMD_RESERVED4_MASK  ) << Si2168B_POWER_UP_CMD_RESERVED4_LSB );
	cmdByteBuffer[5] = (u8) ( ( reserved1  & Si2168B_POWER_UP_CMD_RESERVED1_MASK  ) << Si2168B_POWER_UP_CMD_RESERVED1_LSB |
			( addr_mode  & Si2168B_POWER_UP_CMD_ADDR_MODE_MASK  ) << Si2168B_POWER_UP_CMD_ADDR_MODE_LSB |
			( reserved5  & Si2168B_POWER_UP_CMD_RESERVED5_MASK  ) << Si2168B_POWER_UP_CMD_RESERVED5_LSB );
	cmdByteBuffer[6] = (u8) ( ( func       & Si2168B_POWER_UP_CMD_FUNC_MASK       ) << Si2168B_POWER_UP_CMD_FUNC_LSB      |
			( clock_freq & Si2168B_POWER_UP_CMD_CLOCK_FREQ_MASK ) << Si2168B_POWER_UP_CMD_CLOCK_FREQ_LSB|
			( ctsien     & Si2168B_POWER_UP_CMD_CTSIEN_MASK     ) << Si2168B_POWER_UP_CMD_CTSIEN_LSB    );
	cmdByteBuffer[7] = (u8) ( ( wake_up    & Si2168B_POWER_UP_CMD_WAKE_UP_MASK    ) << Si2168B_POWER_UP_CMD_WAKE_UP_LSB   );

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 8, cmdByteBuffer) != 8) {
		SiTRACE("Error writing POWER_UP bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 1, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling POWER_UP response\n");
		ret = error_code;
	}

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/************************************************************************************************************************
  NAME: si2168b_wakeup
  DESCRIPTION:
  Parameter:  Pointer to Si2168B Context
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
************************************************************************************************************************/
static int si2168b_wakeup(si2168b_context *ctx)
{
	u8  start_clk_tune_cap;
	u16 start_clk_mode;
	u8  power_up_clock_freq;

	int return_code;
	int media;

	return_code = NO_Si2168B_ERROR;
	media       = si2168b_media(ctx);
	SiTRACE ("si2168b_wakeup: media %d\n", media);

	/* Clock source selection */
	switch (media) {
	default:
	case Si2168B_TERRESTRIAL:
		start_clk_mode = ctx->tuner_ter_clock_input;
		break;
	}
	if (start_clk_mode == Si2168B_START_CLK_CMD_CLK_MODE_XTAL) {
		start_clk_tune_cap = Si2168B_START_CLK_CMD_TUNE_CAP_15P6;
	} else {
		start_clk_tune_cap = Si2168B_START_CLK_CMD_TUNE_CAP_EXT_CLK;
	}
	si2168b_start_clk (ctx,
			Si2168B_START_CLK_CMD_SUBCODE_CODE,
			Si2168B_START_CLK_CMD_RESERVED1_RESERVED,
			start_clk_tune_cap,
			Si2168B_START_CLK_CMD_RESERVED2_RESERVED,
			start_clk_mode,
			Si2168B_START_CLK_CMD_RESERVED3_RESERVED,
			Si2168B_START_CLK_CMD_RESERVED4_RESERVED,
			Si2168B_START_CLK_CMD_START_CLK_START_CLK);
	/* Reference frequency selection */
	switch (media) {
	default:
	case Si2168B_TERRESTRIAL : {
		if (ctx->tuner_ter_clock_freq == 16) {
			SiTRACE("Si2168B_POWER_UP_CMD_CLOCK_FREQ_CLK_16MHZ\n");
			power_up_clock_freq = Si2168B_POWER_UP_CMD_CLOCK_FREQ_CLK_16MHZ;
		} else if (ctx->tuner_ter_clock_freq == 24) {
			SiTRACE("Si2168B_POWER_UP_CMD_CLOCK_FREQ_CLK_24MHZ\n");
			power_up_clock_freq = Si2168B_POWER_UP_CMD_CLOCK_FREQ_CLK_24MHZ;
		} else {
			SiTRACE("Si2168B_POWER_UP_CMD_CLOCK_FREQ_CLK_27MHZ\n");
			power_up_clock_freq = Si2168B_POWER_UP_CMD_CLOCK_FREQ_CLK_27MHZ;
		}
		break;
	}
	}

	return_code = si2168b_power_up(ctx,
			Si2168B_POWER_UP_CMD_SUBCODE_CODE,
			ctx->power_up_reset,
			Si2168B_POWER_UP_CMD_RESERVED2_RESERVED,
			Si2168B_POWER_UP_CMD_RESERVED4_RESERVED,
			Si2168B_POWER_UP_CMD_RESERVED1_RESERVED,
			Si2168B_POWER_UP_CMD_ADDR_MODE_CURRENT,
			Si2168B_POWER_UP_CMD_RESERVED5_RESERVED,
			ctx->power_up_func,
			power_up_clock_freq,
			Si2168B_POWER_UP_CMD_CTSIEN_DISABLE,
			Si2168B_POWER_UP_CMD_WAKE_UP_WAKE_UP);

	if (start_clk_mode == Si2168B_START_CLK_CMD_CLK_MODE_CLK_CLKIO) {
		SiTRACE ("Si2168B_START_CLK_CMD_CLK_MODE_CLK_CLKIO\n");
	} else if (start_clk_mode == Si2168B_START_CLK_CMD_CLK_MODE_CLK_XTAL_IN) {
		SiTRACE ("Si2168B_START_CLK_CMD_CLK_MODE_CLK_XTAL_IN\n");
	} else if (start_clk_mode == Si2168B_START_CLK_CMD_CLK_MODE_XTAL) {
		SiTRACE ("Si2168B_START_CLK_CMD_CLK_MODE_XTAL\n");
	}

	if (ctx->power_up_reset == Si2168B_POWER_UP_CMD_RESET_RESET) {
		SiTRACE ("Si2168B_POWER_UP_CMD_RESET_RESET\n");
	} else if (ctx->power_up_reset == Si2168B_POWER_UP_CMD_RESET_RESUME ) {
		SiTRACE ("Si2168B_POWER_UP_CMD_RESET_RESUME\n");
	}

	if (return_code != NO_Si2168B_ERROR ) {
		SiTRACE("si2168b_wakeup: POWER_UP ERROR!\n");
		/* second try with reset... */
		return_code = si2168b_power_up(ctx,
				Si2168B_POWER_UP_CMD_SUBCODE_CODE,
				Si2168B_POWER_UP_CMD_RESET_RESET,
				Si2168B_POWER_UP_CMD_RESERVED2_RESERVED,
				Si2168B_POWER_UP_CMD_RESERVED4_RESERVED,
				Si2168B_POWER_UP_CMD_RESERVED1_RESERVED,
				Si2168B_POWER_UP_CMD_ADDR_MODE_CURRENT,
				Si2168B_POWER_UP_CMD_RESERVED5_RESERVED,
				ctx->power_up_func,
				power_up_clock_freq,
				Si2168B_POWER_UP_CMD_CTSIEN_DISABLE,
				Si2168B_POWER_UP_CMD_WAKE_UP_WAKE_UP);
		if (return_code != NO_Si2168B_ERROR ) {
			SiTRACE("si2168b_wakeup: POWER_UP ERROR (2)!\n");
			return return_code;
		}
	}
#if 1
	/* After a successful POWER_UP, set values for 'resume' only */
	ctx->power_up_reset = Si2168B_POWER_UP_CMD_RESET_RESUME;
#else /* reset always */
	ctx->power_up_reset = Si2168B_POWER_UP_CMD_RESET_RESET;
#endif
	ctx->power_up_func  = Si2168B_POWER_UP_CMD_FUNC_NORMAL;

	return NO_Si2168B_ERROR;
}

static int si2168b_ts_bus_ctrl(struct dvb_frontend* fe, int acquire)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	Si2168B_L2_Context *front_end = priv->si_front_end;
	u8 ts_bus_mode = 0;
	int ret = 0;

	siprintk("%s() acquire=%d\n", __func__, acquire);

	_mutex_lock(&front_end->demod->ts_bus_ctrl_lock);

	if (front_end->demod->Si2168B_in_standby) {
		/* nothing to do */
		siprintk("%s() Si2168B is in standby mode. Nothing to do.\n", __func__);
		ret = 0;
		goto unlock_mutex;
	}

	if (acquire) {
		ts_bus_mode = priv->config->ts_bus_mode;
	} else {
		ts_bus_mode = SILABS_TS_TRISTATE;
	}
	siprintk("%s() setting the Si2168B ts bus to mode %d\n", __func__, ts_bus_mode);

    if (si2168b_set_ts_mode(priv, ts_bus_mode) != NO_Si2168B_ERROR) {
		printk(KERN_WARNING "%s(): si2168b_set_ts_mode(%u) failed\n", __func__, priv->config->ts_bus_mode);
		/* try to wake up... */
		front_end->demod->Si2168B_in_standby = 1;
		if (si2168b_wakeup(priv->si_front_end->demod) != NO_Si2168B_ERROR) {
			SiTRACE("si2168b_ts_bus_ctrl(): WAKEUP error!\n");
			ret = -EIO;
			goto unlock_mutex;
		}
		/* second try... */
		if (si2168b_set_ts_mode(priv, ts_bus_mode) != NO_Si2168B_ERROR) {
			printk(KERN_ERR "%s(): si2168b_set_ts_mode(%u) failed after wake up\n", __func__, priv->config->ts_bus_mode);
			ret = -EIO;
			goto unlock_mutex;
		}
	}

	siprintk("%s() DONE.\n", __func__);

unlock_mutex:
	_mutex_unlock(&front_end->demod->ts_bus_ctrl_lock);
	return ret;
}

/*---------------------------------------------------*/
/* Si2168B_DD_STATUS COMMAND                        */
/*---------------------------------------------------*/
static u8 si2168b_dd_status(si2168b_context *ctx, u8 intack, Si2168B_DD_STATUS_CMD_REPLY_struct *dd_status)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[8];
	u8 ret = NO_Si2168B_ERROR;

	/*SiTRACE("Si2168B DD_STATUS ");*/

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_STATUS_CMD;
	cmdByteBuffer[1] = (u8) ( ( intack & Si2168B_DD_STATUS_CMD_INTACK_MASK ) << Si2168B_DD_STATUS_CMD_INTACK_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DD_STATUS bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 8, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_STATUS response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	dd_status->pclint       = (( ( (rspByteBuffer[1] )) >> Si2168B_DD_STATUS_RESPONSE_PCLINT_LSB       ) & Si2168B_DD_STATUS_RESPONSE_PCLINT_MASK       );
	dd_status->dlint        = (( ( (rspByteBuffer[1] )) >> Si2168B_DD_STATUS_RESPONSE_DLINT_LSB        ) & Si2168B_DD_STATUS_RESPONSE_DLINT_MASK        );
	dd_status->berint       = (( ( (rspByteBuffer[1] )) >> Si2168B_DD_STATUS_RESPONSE_BERINT_LSB       ) & Si2168B_DD_STATUS_RESPONSE_BERINT_MASK       );
	dd_status->uncorint     = (( ( (rspByteBuffer[1] )) >> Si2168B_DD_STATUS_RESPONSE_UNCORINT_LSB     ) & Si2168B_DD_STATUS_RESPONSE_UNCORINT_MASK     );
	dd_status->rsqint_bit5  = (( ( (rspByteBuffer[1] )) >> Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT5_LSB  ) & Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT5_MASK  );
	dd_status->rsqint_bit6  = (( ( (rspByteBuffer[1] )) >> Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT6_LSB  ) & Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT6_MASK  );
	dd_status->rsqint_bit7  = (( ( (rspByteBuffer[1] )) >> Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT7_LSB  ) & Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT7_MASK  );
	dd_status->pcl          = (( ( (rspByteBuffer[2] )) >> Si2168B_DD_STATUS_RESPONSE_PCL_LSB          ) & Si2168B_DD_STATUS_RESPONSE_PCL_MASK          );
	dd_status->dl           = (( ( (rspByteBuffer[2] )) >> Si2168B_DD_STATUS_RESPONSE_DL_LSB           ) & Si2168B_DD_STATUS_RESPONSE_DL_MASK           );
	dd_status->ber          = (( ( (rspByteBuffer[2] )) >> Si2168B_DD_STATUS_RESPONSE_BER_LSB          ) & Si2168B_DD_STATUS_RESPONSE_BER_MASK          );
	dd_status->uncor        = (( ( (rspByteBuffer[2] )) >> Si2168B_DD_STATUS_RESPONSE_UNCOR_LSB        ) & Si2168B_DD_STATUS_RESPONSE_UNCOR_MASK        );
	dd_status->rsqstat_bit5 = (( ( (rspByteBuffer[2] )) >> Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT5_LSB ) & Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT5_MASK );
	dd_status->rsqstat_bit6 = (( ( (rspByteBuffer[2] )) >> Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT6_LSB ) & Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT6_MASK );
	dd_status->rsqstat_bit7 = (( ( (rspByteBuffer[2] )) >> Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT7_LSB ) & Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT7_MASK );
	dd_status->modulation   = (( ( (rspByteBuffer[3] )) >> Si2168B_DD_STATUS_RESPONSE_MODULATION_LSB   ) & Si2168B_DD_STATUS_RESPONSE_MODULATION_MASK   );
	dd_status->ts_bit_rate  = (( ( (rspByteBuffer[4] ) | (rspByteBuffer[5]  << 8 )) >> Si2168B_DD_STATUS_RESPONSE_TS_BIT_RATE_LSB  ) & Si2168B_DD_STATUS_RESPONSE_TS_BIT_RATE_MASK  );
	dd_status->ts_clk_freq  = (( ( (rspByteBuffer[6] ) | (rspByteBuffer[7]  << 8 )) >> Si2168B_DD_STATUS_RESPONSE_TS_CLK_FREQ_LSB  ) & Si2168B_DD_STATUS_RESPONSE_TS_CLK_FREQ_MASK  );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/*---------------------------------------------------*/
/* Si2168B_DVBT_STATUS COMMAND                      */
/*---------------------------------------------------*/
static u8 si2168b_dvbt_status(si2168b_context *ctx, u8 intack, Si2168B_DVBT_STATUS_CMD_REPLY_struct *dvbt_status)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[13];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DVBT_STATUS\n");

	_mutex_lock(&ctx->lock);
	cmdByteBuffer[0] = Si2168B_DVBT_STATUS_CMD;
	cmdByteBuffer[1] = (u8) ( ( intack & Si2168B_DVBT_STATUS_CMD_INTACK_MASK ) << Si2168B_DVBT_STATUS_CMD_INTACK_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DVBT_STATUS bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 13, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DVBT_STATUS response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	dvbt_status->pclint        =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT_STATUS_RESPONSE_PCLINT_LSB        ) & Si2168B_DVBT_STATUS_RESPONSE_PCLINT_MASK        );
	dvbt_status->dlint         =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT_STATUS_RESPONSE_DLINT_LSB         ) & Si2168B_DVBT_STATUS_RESPONSE_DLINT_MASK         );
	dvbt_status->berint        =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT_STATUS_RESPONSE_BERINT_LSB        ) & Si2168B_DVBT_STATUS_RESPONSE_BERINT_MASK        );
	dvbt_status->uncorint      =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT_STATUS_RESPONSE_UNCORINT_LSB      ) & Si2168B_DVBT_STATUS_RESPONSE_UNCORINT_MASK      );
	dvbt_status->notdvbtint    =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT_STATUS_RESPONSE_NOTDVBTINT_LSB    ) & Si2168B_DVBT_STATUS_RESPONSE_NOTDVBTINT_MASK    );
	dvbt_status->pcl           =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT_STATUS_RESPONSE_PCL_LSB           ) & Si2168B_DVBT_STATUS_RESPONSE_PCL_MASK           );
	dvbt_status->dl            =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT_STATUS_RESPONSE_DL_LSB            ) & Si2168B_DVBT_STATUS_RESPONSE_DL_MASK            );
	dvbt_status->ber           =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT_STATUS_RESPONSE_BER_LSB           ) & Si2168B_DVBT_STATUS_RESPONSE_BER_MASK           );
	dvbt_status->uncor         =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT_STATUS_RESPONSE_UNCOR_LSB         ) & Si2168B_DVBT_STATUS_RESPONSE_UNCOR_MASK         );
	dvbt_status->notdvbt       =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT_STATUS_RESPONSE_NOTDVBT_LSB       ) & Si2168B_DVBT_STATUS_RESPONSE_NOTDVBT_MASK       );
	dvbt_status->cnr           =   (( ( (rspByteBuffer[3]  )) >> Si2168B_DVBT_STATUS_RESPONSE_CNR_LSB           ) & Si2168B_DVBT_STATUS_RESPONSE_CNR_MASK           );
	dvbt_status->afc_freq      = (((( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 )) >> Si2168B_DVBT_STATUS_RESPONSE_AFC_FREQ_LSB      ) & Si2168B_DVBT_STATUS_RESPONSE_AFC_FREQ_MASK) <<Si2168B_DVBT_STATUS_RESPONSE_AFC_FREQ_SHIFT ) >>Si2168B_DVBT_STATUS_RESPONSE_AFC_FREQ_SHIFT      );
	dvbt_status->timing_offset = (((( ( (rspByteBuffer[6]  ) | (rspByteBuffer[7]  << 8 )) >> Si2168B_DVBT_STATUS_RESPONSE_TIMING_OFFSET_LSB ) & Si2168B_DVBT_STATUS_RESPONSE_TIMING_OFFSET_MASK) <<Si2168B_DVBT_STATUS_RESPONSE_TIMING_OFFSET_SHIFT ) >>Si2168B_DVBT_STATUS_RESPONSE_TIMING_OFFSET_SHIFT );
	dvbt_status->constellation =   (( ( (rspByteBuffer[8]  )) >> Si2168B_DVBT_STATUS_RESPONSE_CONSTELLATION_LSB ) & Si2168B_DVBT_STATUS_RESPONSE_CONSTELLATION_MASK );
	dvbt_status->sp_inv        =   (( ( (rspByteBuffer[8]  )) >> Si2168B_DVBT_STATUS_RESPONSE_SP_INV_LSB        ) & Si2168B_DVBT_STATUS_RESPONSE_SP_INV_MASK        );
	dvbt_status->rate_hp       =   (( ( (rspByteBuffer[9]  )) >> Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_LSB       ) & Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_MASK       );
	dvbt_status->rate_lp       =   (( ( (rspByteBuffer[9]  )) >> Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_LSB       ) & Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_MASK       );
	dvbt_status->fft_mode      =   (( ( (rspByteBuffer[10] )) >> Si2168B_DVBT_STATUS_RESPONSE_FFT_MODE_LSB      ) & Si2168B_DVBT_STATUS_RESPONSE_FFT_MODE_MASK      );
	dvbt_status->guard_int     =   (( ( (rspByteBuffer[10] )) >> Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_LSB     ) & Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_MASK     );
	dvbt_status->hierarchy     =   (( ( (rspByteBuffer[11] )) >> Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_LSB     ) & Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_MASK     );
	dvbt_status->tps_length    = (((( ( (rspByteBuffer[12] )) >> Si2168B_DVBT_STATUS_RESPONSE_TPS_LENGTH_LSB    ) & Si2168B_DVBT_STATUS_RESPONSE_TPS_LENGTH_MASK) <<Si2168B_DVBT_STATUS_RESPONSE_TPS_LENGTH_SHIFT ) >>Si2168B_DVBT_STATUS_RESPONSE_TPS_LENGTH_SHIFT    );

	unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/*---------------------------------------------------*/
/* Si2168B_DVBT2_STATUS COMMAND                     */
/*---------------------------------------------------*/
static u8 si2168b_dvbt2_status(si2168b_context *ctx, u8 intack, Si2168B_DVBT2_STATUS_CMD_REPLY_struct *dvbt2_status)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[14];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DVBT2_STATUS\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DVBT2_STATUS_CMD;
	cmdByteBuffer[1] = (u8) ( ( intack & Si2168B_DVBT2_STATUS_CMD_INTACK_MASK ) << Si2168B_DVBT2_STATUS_CMD_INTACK_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DVBT2_STATUS bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 14, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DVBT2_STATUS response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	dvbt2_status->pclint        =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_PCLINT_LSB        ) & Si2168B_DVBT2_STATUS_RESPONSE_PCLINT_MASK        );
	dvbt2_status->dlint         =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_DLINT_LSB         ) & Si2168B_DVBT2_STATUS_RESPONSE_DLINT_MASK         );
	dvbt2_status->berint        =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_BERINT_LSB        ) & Si2168B_DVBT2_STATUS_RESPONSE_BERINT_MASK        );
	dvbt2_status->uncorint      =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_UNCORINT_LSB      ) & Si2168B_DVBT2_STATUS_RESPONSE_UNCORINT_MASK      );
	dvbt2_status->notdvbt2int   =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2INT_LSB   ) & Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2INT_MASK   );
	dvbt2_status->pcl           =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_PCL_LSB           ) & Si2168B_DVBT2_STATUS_RESPONSE_PCL_MASK           );
	dvbt2_status->dl            =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_DL_LSB            ) & Si2168B_DVBT2_STATUS_RESPONSE_DL_MASK            );
	dvbt2_status->ber           =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_BER_LSB           ) & Si2168B_DVBT2_STATUS_RESPONSE_BER_MASK           );
	dvbt2_status->uncor         =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_UNCOR_LSB         ) & Si2168B_DVBT2_STATUS_RESPONSE_UNCOR_MASK         );
	dvbt2_status->notdvbt2      =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2_LSB      ) & Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2_MASK      );
	dvbt2_status->cnr           =   (( ( (rspByteBuffer[3]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_CNR_LSB           ) & Si2168B_DVBT2_STATUS_RESPONSE_CNR_MASK           );
	dvbt2_status->afc_freq      = (((( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 )) >> Si2168B_DVBT2_STATUS_RESPONSE_AFC_FREQ_LSB      ) & Si2168B_DVBT2_STATUS_RESPONSE_AFC_FREQ_MASK) <<Si2168B_DVBT2_STATUS_RESPONSE_AFC_FREQ_SHIFT ) >>Si2168B_DVBT2_STATUS_RESPONSE_AFC_FREQ_SHIFT      );
	dvbt2_status->timing_offset = (((( ( (rspByteBuffer[6]  ) | (rspByteBuffer[7]  << 8 )) >> Si2168B_DVBT2_STATUS_RESPONSE_TIMING_OFFSET_LSB ) & Si2168B_DVBT2_STATUS_RESPONSE_TIMING_OFFSET_MASK) <<Si2168B_DVBT2_STATUS_RESPONSE_TIMING_OFFSET_SHIFT ) >>Si2168B_DVBT2_STATUS_RESPONSE_TIMING_OFFSET_SHIFT );
	dvbt2_status->constellation =   (( ( (rspByteBuffer[8]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_LSB ) & Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_MASK );
	dvbt2_status->sp_inv        =   (( ( (rspByteBuffer[8]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_SP_INV_LSB        ) & Si2168B_DVBT2_STATUS_RESPONSE_SP_INV_MASK        );
	dvbt2_status->fef           =   (( ( (rspByteBuffer[8]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_FEF_LSB           ) & Si2168B_DVBT2_STATUS_RESPONSE_FEF_MASK           );
	dvbt2_status->fft_mode      =   (( ( (rspByteBuffer[9]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_LSB      ) & Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_MASK      );
	dvbt2_status->guard_int     =   (( ( (rspByteBuffer[9]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_LSB     ) & Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_MASK     );
	dvbt2_status->bw_ext        =   (( ( (rspByteBuffer[9]  )) >> Si2168B_DVBT2_STATUS_RESPONSE_BW_EXT_LSB        ) & Si2168B_DVBT2_STATUS_RESPONSE_BW_EXT_MASK        );
	dvbt2_status->num_plp       =   (( ( (rspByteBuffer[10] )) >> Si2168B_DVBT2_STATUS_RESPONSE_NUM_PLP_LSB       ) & Si2168B_DVBT2_STATUS_RESPONSE_NUM_PLP_MASK       );
	dvbt2_status->pilot_pattern =   (( ( (rspByteBuffer[11] )) >> Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_LSB ) & Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_MASK );
	dvbt2_status->tx_mode       =   (( ( (rspByteBuffer[11] )) >> Si2168B_DVBT2_STATUS_RESPONSE_TX_MODE_LSB       ) & Si2168B_DVBT2_STATUS_RESPONSE_TX_MODE_MASK       );
	dvbt2_status->rotated       =   (( ( (rspByteBuffer[11] )) >> Si2168B_DVBT2_STATUS_RESPONSE_ROTATED_LSB       ) & Si2168B_DVBT2_STATUS_RESPONSE_ROTATED_MASK       );
	dvbt2_status->short_frame   =   (( ( (rspByteBuffer[11] )) >> Si2168B_DVBT2_STATUS_RESPONSE_SHORT_FRAME_LSB   ) & Si2168B_DVBT2_STATUS_RESPONSE_SHORT_FRAME_MASK   );
	dvbt2_status->t2_mode       =   (( ( (rspByteBuffer[11] )) >> Si2168B_DVBT2_STATUS_RESPONSE_T2_MODE_LSB       ) & Si2168B_DVBT2_STATUS_RESPONSE_T2_MODE_MASK       );
	dvbt2_status->code_rate     =   (( ( (rspByteBuffer[12] )) >> Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_LSB     ) & Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_MASK     );
	dvbt2_status->t2_version    =   (( ( (rspByteBuffer[12] )) >> Si2168B_DVBT2_STATUS_RESPONSE_T2_VERSION_LSB    ) & Si2168B_DVBT2_STATUS_RESPONSE_T2_VERSION_MASK    );
	dvbt2_status->plp_id        =   (( ( (rspByteBuffer[13] )) >> Si2168B_DVBT2_STATUS_RESPONSE_PLP_ID_LSB        ) & Si2168B_DVBT2_STATUS_RESPONSE_PLP_ID_MASK        );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/*---------------------------------------------------*/
/* Si2168B_DVBC_STATUS COMMAND                      */
/*---------------------------------------------------*/
static u8 si2168b_dvbc_status(si2168b_context *ctx, u8 intack, Si2168B_DVBC_STATUS_CMD_REPLY_struct *dvbc_status)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[9];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DVBC_STATUS\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DVBC_STATUS_CMD;
	cmdByteBuffer[1] = (u8) ( ( intack & Si2168B_DVBC_STATUS_CMD_INTACK_MASK ) << Si2168B_DVBC_STATUS_CMD_INTACK_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DVBC_STATUS bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 9, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DVBC_STATUS response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	dvbc_status->pclint        =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBC_STATUS_RESPONSE_PCLINT_LSB        ) & Si2168B_DVBC_STATUS_RESPONSE_PCLINT_MASK        );
	dvbc_status->dlint         =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBC_STATUS_RESPONSE_DLINT_LSB         ) & Si2168B_DVBC_STATUS_RESPONSE_DLINT_MASK         );
	dvbc_status->berint        =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBC_STATUS_RESPONSE_BERINT_LSB        ) & Si2168B_DVBC_STATUS_RESPONSE_BERINT_MASK        );
	dvbc_status->uncorint      =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBC_STATUS_RESPONSE_UNCORINT_LSB      ) & Si2168B_DVBC_STATUS_RESPONSE_UNCORINT_MASK      );
	dvbc_status->pcl           =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBC_STATUS_RESPONSE_PCL_LSB           ) & Si2168B_DVBC_STATUS_RESPONSE_PCL_MASK           );
	dvbc_status->dl            =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBC_STATUS_RESPONSE_DL_LSB            ) & Si2168B_DVBC_STATUS_RESPONSE_DL_MASK            );
	dvbc_status->ber           =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBC_STATUS_RESPONSE_BER_LSB           ) & Si2168B_DVBC_STATUS_RESPONSE_BER_MASK           );
	dvbc_status->uncor         =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBC_STATUS_RESPONSE_UNCOR_LSB         ) & Si2168B_DVBC_STATUS_RESPONSE_UNCOR_MASK         );
	dvbc_status->cnr           =   (( ( (rspByteBuffer[3]  )) >> Si2168B_DVBC_STATUS_RESPONSE_CNR_LSB           ) & Si2168B_DVBC_STATUS_RESPONSE_CNR_MASK           );
	dvbc_status->afc_freq      = (((( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 )) >> Si2168B_DVBC_STATUS_RESPONSE_AFC_FREQ_LSB      ) & Si2168B_DVBC_STATUS_RESPONSE_AFC_FREQ_MASK) <<Si2168B_DVBC_STATUS_RESPONSE_AFC_FREQ_SHIFT ) >>Si2168B_DVBC_STATUS_RESPONSE_AFC_FREQ_SHIFT      );
	dvbc_status->timing_offset = (((( ( (rspByteBuffer[6]  ) | (rspByteBuffer[7]  << 8 )) >> Si2168B_DVBC_STATUS_RESPONSE_TIMING_OFFSET_LSB ) & Si2168B_DVBC_STATUS_RESPONSE_TIMING_OFFSET_MASK) <<Si2168B_DVBC_STATUS_RESPONSE_TIMING_OFFSET_SHIFT ) >>Si2168B_DVBC_STATUS_RESPONSE_TIMING_OFFSET_SHIFT );
	dvbc_status->constellation =   (( ( (rspByteBuffer[8]  )) >> Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_LSB ) & Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_MASK );
	dvbc_status->sp_inv        =   (( ( (rspByteBuffer[8]  )) >> Si2168B_DVBC_STATUS_RESPONSE_SP_INV_LSB        ) & Si2168B_DVBC_STATUS_RESPONSE_SP_INV_MASK        );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/* Receive Signal Strength Indicator (RSSI) */

static int si2168b_read_rssi(struct dvb_frontend *fe, u16 *rssi)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	int ret = 0;

	if (fe->ops.tuner_ops.get_rf_strength == NULL) {
		siprintk("%s(): WARNING: get_rf_strength() not available\n", __func__);
		return -ENODEV;
	}

	if ( priv->config->indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
		if (priv->si_front_end->f_TER_tuner_enable(priv->si_front_end->callback) != NO_Si2168B_ERROR) {
			siprintk("%s(): f_TER_tuner_enable() failed\n", __func__);
			return -EFAULT;
		}
	} else {
		if (si2168b_tuner_i2c_enable(priv->si_front_end) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_tuner_i2c_enable() failed\n", __func__);
			return -EFAULT;
		}
	}
	ret = fe->ops.tuner_ops.get_rf_strength(fe, rssi);

	if (ret == 0) {
		siprintk("%s(): RSSI %3d dBm\n", __func__, *rssi);
	} else {
		siprintk("%s(): get_rf_strength() failed\n", __func__);
	}

	if ( priv->config->indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
		if (priv->si_front_end->f_TER_tuner_disable(priv->si_front_end->callback) != NO_Si2168B_ERROR) {
			siprintk("%s(): f_TER_tuner_disable() failed\n", __func__);
			return -EFAULT;
		}
	} else {
		if (si2168b_tuner_i2c_disable(priv->si_front_end) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_tuner_i2c_disable() failed\n", __func__);
			return -EFAULT;
		}
	}

	return 0;
}

/* read carrier-to-noise ratio (C/N) */

static int si2168b_read_cnr(struct dvb_frontend *fe, u16 *cnr)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	si2168b_context *demod = priv->si_front_end->demod;

	Si2168B_DD_STATUS_CMD_REPLY_struct    dd_status;
	Si2168B_DVBT_STATUS_CMD_REPLY_struct  dvbt_status;
	Si2168B_DVBT2_STATUS_CMD_REPLY_struct dvbt2_status;
	Si2168B_DVBC_STATUS_CMD_REPLY_struct  dvbc_status;

	/* Call the demod global status function */
	si2168b_dd_status(demod, Si2168B_DD_STATUS_CMD_INTACK_OK, &dd_status);

	/* Call the standard-specific status function */
	switch (dd_status.modulation) {
	case Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBT:
		if (si2168b_dvbt_status(demod, Si2168B_DVBT_STATUS_CMD_INTACK_OK, &dvbt_status) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_dvbt_status() failed\n", __func__);
			return -EFAULT;
		}
		*cnr = dvbt_status.cnr;
		break;
	case Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBT2:
		if (si2168b_dvbt2_status(demod, Si2168B_DVBT2_STATUS_CMD_INTACK_OK, &dvbt2_status) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_dvbt2_status() failed\n", __func__);
			return -EFAULT;
		}
		*cnr = dvbt2_status.cnr;
		break;
	case Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBC:
		if (si2168b_dvbc_status(demod, Si2168B_DVBC_STATUS_CMD_INTACK_OK, &dvbc_status) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_dvbc_status() failed\n", __func__);
			return -EFAULT;
		}
		*cnr = dvbc_status.cnr;
		break;
	default:
		siprintk("%s(): invalid mode\n", __func__);
		return -EINVAL;
	}

	siprintk("%s(): C/N  %d dB\n", __func__, ((*cnr) / 4));

	/**cnr *= 160;*/ /* linux value adjustment */

	return 0;
}

/*---------------------------------------------------*/
/* Si2168B_DD_BER COMMAND                           */
/*---------------------------------------------------*/
static u8 si2168b_dd_ber(si2168b_context *ctx, u8 rst, Si2168B_DD_BER_CMD_REPLY_struct *dd_ber)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[3];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DD_BER\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_BER_CMD;
	cmdByteBuffer[1] = (u8) ( ( rst & Si2168B_DD_BER_CMD_RST_MASK ) << Si2168B_DD_BER_CMD_RST_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DD_BER bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 3, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_BER response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	dd_ber->exp  = (( ( (rspByteBuffer[1]  )) >> Si2168B_DD_BER_RESPONSE_EXP_LSB  ) & Si2168B_DD_BER_RESPONSE_EXP_MASK  );
	dd_ber->mant = (( ( (rspByteBuffer[2]  )) >> Si2168B_DD_BER_RESPONSE_MANT_LSB ) & Si2168B_DD_BER_RESPONSE_MANT_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/*---------------------------------------------------*/
/* Si2168B_DD_UNCOR COMMAND                         */
/*---------------------------------------------------*/
static u8 si2168b_dd_uncor(si2168b_context *ctx, u8 rst, Si2168B_DD_UNCOR_CMD_REPLY_struct *dd_uncor)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[3];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DD_UNCOR\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_UNCOR_CMD;
	cmdByteBuffer[1] = (u8) ( ( rst & Si2168B_DD_UNCOR_CMD_RST_MASK ) << Si2168B_DD_UNCOR_CMD_RST_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DD_UNCOR bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 3, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_UNCOR response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	dd_uncor->uncor_lsb =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DD_UNCOR_RESPONSE_UNCOR_LSB_LSB ) & Si2168B_DD_UNCOR_RESPONSE_UNCOR_LSB_MASK );
	dd_uncor->uncor_msb =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DD_UNCOR_RESPONSE_UNCOR_MSB_LSB ) & Si2168B_DD_UNCOR_RESPONSE_UNCOR_MSB_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

static inline int exp_10(int m)
{
	int i;
	int p = 1;

	for (i = 1; i <= m; i++) {
		p *= 10;
	}
	return p;
}

static int si2168b_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *p = &fe->dtv_property_cache;
	si2168b_context *demod = priv->si_front_end->demod;
	Si2168B_DD_BER_CMD_REPLY_struct dd_ber;

	/* Retrieving BER values */
	switch (p->delivery_system) {
	case SYS_DVBT:
	case SYS_DVBT2:
	case SYS_DVBC_ANNEX_A:
/*  case SILABS_MCNS : */
		if (si2168b_dd_ber(demod, Si2168B_DD_BER_CMD_RST_RUN, &dd_ber) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_dd_ber() failed\n", __func__);
			return -EFAULT;
		}
		/* CHECK the exponent value to know if the BER is available or not */
		if (dd_ber.exp != 0) {
			*ber = (dd_ber.mant / 10) / exp_10(dd_ber.exp);
		} else {
			*ber = 0;
		}
		siprintk("%s(): BER=%u\n", __func__, *ber);
		break;
	default:
		return 0;
		break;
	}

	return 0;
}

static int si2168b_read_uncorrs(struct dvb_frontend *fe, u32 *uncorrs)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	si2168b_context *demod = priv->si_front_end->demod;
	Si2168B_DD_UNCOR_CMD_REPLY_struct dd_uncor = { .uncor_msb=0, .uncor_lsb=0 };

	if (si2168b_dd_uncor(demod, Si2168B_DD_UNCOR_CMD_RST_RUN, &dd_uncor) != NO_Si2168B_ERROR) {
		siprintk("%s(): si2168b_dd_uncor() failed\n", __func__);
		return 0;
	}
	*uncorrs = (((u32) dd_uncor.uncor_msb) << 8) + dd_uncor.uncor_lsb;

	siprintk("%s(): uncorrs=%u\n", __func__, *uncorrs);

	return 0;
}

static int si2168b_get_stats(struct dvb_frontend *fe, fe_status_t *status)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	si2168b_context *demod = priv->si_front_end->demod;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	Si2168B_DD_STATUS_CMD_REPLY_struct dd_status;
	u16 rssi;
	u16 cnr;
	u32 ber;

	if (si2168b_dd_status(demod, Si2168B_DD_STATUS_CMD_INTACK_OK, &dd_status) != NO_Si2168B_ERROR) {
		siprintk("%s(): si2168b_dd_status() failed\n", __func__);
		return -EFAULT;
	}

	/* reset status */
	*status = 0;

	/* demod lock? */
	if (dd_status.pcl == Si2168B_DD_STATUS_RESPONSE_PCL_LOCKED) {
		*status |= FE_HAS_SIGNAL;
	}

	/* fec lock */
	if (dd_status.dl == Si2168B_DD_STATUS_RESPONSE_DL_LOCKED) {
		*status |= FE_HAS_LOCK;
		*status |= FE_HAS_VITERBI;

		/* ToDo: set the other flags correctly */
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_SYNC;
		/* if fec is locked signal, carrier and sync flags should be set too */
	}

	c->pre_bit_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	c->pre_bit_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	c->post_bit_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	c->post_bit_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;

	/* read RSSI from tuner */

	if (si2168b_read_rssi(fe, &rssi)) {
		c->strength.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	} else {
		c->strength.stat[0].scale = FE_SCALE_DECIBEL;
		c->strength.stat[0].uvalue = rssi;
	}

	if (si2168b_read_cnr(fe, &cnr)) {
		c->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	} else {
		c->cnr.stat[0].uvalue = cnr * 1/*00*/;
		c->cnr.stat[0].scale = FE_SCALE_DECIBEL;
	}

	if (dd_status.dl != Si2168B_DD_STATUS_RESPONSE_DL_LOCKED) {
		c->block_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
		c->block_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
		return 0;
	}

	/* read BER */

	if (si2168b_read_ber(fe, &ber)) {
		c->block_error.stat[0].scale = FE_SCALE_COUNTER;
		c->block_error.stat[0].uvalue = ber;
		c->block_count.stat[0].scale = FE_SCALE_COUNTER;
		c->block_count.stat[0].uvalue = 0;
	} else {
		c->block_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
		c->block_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	}

	return 0;
}

static int si2168b_read_status(struct dvb_frontend *fe, fe_status_t *status)
{
	return si2168b_get_stats(fe, status);
}

/************************************************************************************************************************
  NAME: Si2168B_L2_Channel_Seek_End
  DESCRIPTION: returns the chip back to normal use following a seek sequence
  Programming Guide Reference:    Flowchart TBD (Channel Scan flowchart)

  Parameter:  Pointer to Si2168B Context
  Returns:    0 if successful, otherwise an error.
************************************************************************************************************************/
static int si2168b_channel_seek_end(struct dvb_frontend *fe)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	Si2168B_L2_Context *front_end = priv->si_front_end;
	const u8 dd_mode_bw = Si2168B_DD_MODE_PROP_BW_DEFAULT;
	const u8 dd_mode_invert_spectrum = Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_DEFAULT;
	u8 dd_mode_modulation = Si2168B_DD_MODE_PROP_MODULATION_DEFAULT;
	u8 dd_mode_auto_detect = Si2168B_DD_MODE_PROP_AUTO_DETECT_NONE;
    u16 data;

	const u8 scan_ien_buzien = Si2168B_SCAN_IEN_PROP_BUZIEN_DISABLE ; /* (default 'DISABLE') */
	const u8 scan_ien_reqien = Si2168B_SCAN_IEN_PROP_REQIEN_DISABLE ; /* (default 'DISABLE') */
    data = (scan_ien_buzien & Si2168B_SCAN_IEN_PROP_BUZIEN_MASK) << Si2168B_SCAN_IEN_PROP_BUZIEN_LSB  |
           (scan_ien_reqien & Si2168B_SCAN_IEN_PROP_REQIEN_MASK) << Si2168B_SCAN_IEN_PROP_REQIEN_LSB ;
	si2168b_set_property(front_end->demod, Si2168B_SCAN_IEN_PROP, data);

	switch (front_end->standard) {
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
		dd_mode_modulation = Si2168B_DD_MODE_PROP_MODULATION_DVBT;
		break;
	case Si2168B_DD_MODE_PROP_MODULATION_DVBC:
		dd_mode_modulation = Si2168B_DD_MODE_PROP_MODULATION_DVBC;
		break;
#ifdef __MCNS__
		case Si2168B_DD_MODE_PROP_MODULATION_MCNS:
		dd_mode_modulation = Si2168B_DD_MODE_PROP_MODULATION_MCNS;
		break;
#endif /* __MCNS__ */
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
		dd_mode_modulation = Si2168B_DD_MODE_PROP_MODULATION_DVBT2;
		break;
	default:
		SiTRACE("UNKNOWN standard %d\n", front_end->standard);
		break;
	}

	SiTRACE("auto_detect_TER %d\n",front_end->auto_detect_TER);
	if (front_end->auto_detect_TER) {
		switch (front_end->standard)	{
		case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
		case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
			dd_mode_modulation  = Si2168B_DD_MODE_PROP_MODULATION_AUTO_DETECT;
			dd_mode_auto_detect = Si2168B_DD_MODE_PROP_AUTO_DETECT_AUTO_DVB_T_T2;
			break;
		default:
			break;
		}
	}
    data = (dd_mode_bw              & Si2168B_DD_MODE_PROP_BW_MASK             ) << Si2168B_DD_MODE_PROP_BW_LSB |
           (dd_mode_modulation      & Si2168B_DD_MODE_PROP_MODULATION_MASK     ) << Si2168B_DD_MODE_PROP_MODULATION_LSB |
           (dd_mode_invert_spectrum & Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_LSB |
           (dd_mode_auto_detect     & Si2168B_DD_MODE_PROP_AUTO_DETECT_MASK    ) << Si2168B_DD_MODE_PROP_AUTO_DETECT_LSB;
	si2168b_set_property(front_end->demod, Si2168B_DD_MODE_PROP, data);

	return 0;
}

/************************************************************************************************************************
  Si2168B_DVB_C_max_lock_ms function
  Use:        DVB-C lock time retrieval function
              Used to know how much time DVB-C lock will take in the worst case
************************************************************************************************************************/
static int si2168b_dvbc_max_lock_ms(si2168b_context *ctx, u8 constellation, u32 symbol_rate_baud)
{
	/* HG: replaced floating point operations by integer operations */
	/* calculation errors included now - but tolerated */
	u32 afc_khz = 100; /* (default 100 for DVB-C) */
	u32 swt;
	u32 swt_coeff;
	u32 lock_ms = 0;

	/* To avoid division by 0, return 5000 if SR is 0 */
	if (symbol_rate_baud < 1000) {
		return 5000; /* HG: to avoid division by 0 */
	}
	if (afc_khz*1000 > symbol_rate_baud*11/100 ) {
		afc_khz = symbol_rate_baud*11/100000;
	}
	swt = (1 + (afc_khz* (22369621 / (symbol_rate_baud/1000)) / (symbol_rate_baud/1000) ) ) / 2;
	switch (constellation) {
	case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM64:
	case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM16:
	case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM256:
		swt_coeff = 3;
		break;
	case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM128:
		swt_coeff = 5;
		break;
	case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_AUTO:
	default:
		swt_coeff = 11;
		break;
	}
	lock_ms = (720000000/symbol_rate_baud + swt*swt_coeff)+ 100;
	SiTRACE("afc_khz %3u, swt %6u, swt_coeff %u DVB_C_max_lock_ms %u\n", afc_khz, swt, swt_coeff, lock_ms);
	return lock_ms;
}

/*---------------------------------------------------*/
/* Si2168B_DVBT2_PLP_SELECT COMMAND                 */
/*---------------------------------------------------*/
static u8 si2168b_dvbt2_plp_select(si2168b_context *ctx, u8 plp_id, u8 plp_id_sel_mode)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[3];
	u8 rspByteBuffer[1];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DVBT2_PLP_SELECT ID=%u MODE=%u\n", plp_id, plp_id_sel_mode);

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DVBT2_PLP_SELECT_CMD;
	cmdByteBuffer[1] = (u8) ( ( plp_id          & Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_MASK          ) << Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_LSB         );
	cmdByteBuffer[2] = (u8) ( ( plp_id_sel_mode & Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_MASK ) << Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 3, cmdByteBuffer) != 3) {
		SiTRACE("Error writing DVBT2_PLP_SELECT bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 1, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DVBT2_PLP_SELECT response\n");
		ret = error_code;
	}

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/*---------------------------------------------------*/
/* Si2168B_DD_RESTART COMMAND                       */
/*---------------------------------------------------*/
static u8 si2168b_dd_restart(si2168b_context *ctx)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[1];
	u8 rspByteBuffer[1];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DD_RESTART\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_RESTART_CMD;

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 1, cmdByteBuffer) != 1) {
		SiTRACE("Error writing DD_RESTART bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 1, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_RESTART response\n");
		ret = error_code;
	}

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/************************************************************************************************************************
  si2168b_standard_name function
  Use:        standard text retrieval function
              Used to retrieve the standard text used by the Si2168B
  Parameter:  standard, the value of the standard
************************************************************************************************************************/
static char *si2168b_standard_name(u8 standard)
{
	switch (standard) {
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
		return (char*)"DVB-T";
#ifdef __MCNS__
	case Si2168B_DD_MODE_PROP_MODULATION_MCNS:
		return (char*)"MCNS";
#endif /* __MCNS__ */
	case Si2168B_DD_MODE_PROP_MODULATION_DVBC:
		return (char*)"DVB-C";
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
		return (char*)"DVB-T2";
	default:
		return (char*)"UNKNOWN";
	}
}

/************************************************************************************************************************
  si2168b_ter_fef function
  Use:        TER tuner FEF activation function
              Used to enable/disable the FEF mode in the terrestrial tuner
  Comments:   If the tuner is connected via the demodulator's I2C switch, enabling/disabling the i2c_passthru is required before/after tuning.
  Parameter:  *front_end, the front-end handle
  Parameter:  fef, a flag controlling the selection between FEF 'off'(0) and FEF 'on'(1)
  Returns:    1
************************************************************************************************************************/
static int si2168b_ter_fef(Si2168B_L2_Context *front_end, u8 fef)
{
	SiTRACE("si2168b_ter_fef %d\n", fef);

	if (front_end->demod->fef_mode == Si2168B_FEF_MODE_FREEZE_PIN) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_FREEZE_PIN\n");
		/* handled in tuner module now */
	}

#ifdef    L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP
	if (front_end->demod->fef_mode == Si2168B_FEF_MODE_SLOW_INITIAL_AGC) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_SLOW_INITIAL_AGC (AGC slowed down after tuning)\n");
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP */

#ifdef    L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC_SETUP
	if (front_end->demod->fef_mode == Si2168B_FEF_MODE_SLOW_NORMAL_AGC ) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_SLOW_NORMAL_AGC: AGC slowed down\n");
		/* handled in tuner module now */
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC_SETUP */

	return 1;
}

/************************************************************************************************************************
  si2168b_tune function
  Use:        tuner current frequency retrieval function
              Used to retrieve the current RF from the tuner's driver.
  Porting:    Replace the internal TUNER function calls by the final tuner's corresponding calls
  Comments:   If the tuner is connected via the demodulator's I2C switch, enabling/disabling the i2c_passthru is required before/after tuning.
  Behavior:   This function closes the Si2168B's I2C switch then tunes and finally reopens the I2C switch
  Parameter:  *front_end, the front-end handle
  Parameter:  rf, the frequency to tune at
  Returns:    rf
************************************************************************************************************************/
static u32 si2168b_set_tuner_params(struct dvb_frontend *fe, u32 rf)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	Si2168B_L2_Context *front_end = priv->si_front_end;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	SiTRACE("si2168b_set_tuner_params() frequency=%u\n", rf);

	if (front_end->tuner_indirect_i2c_connection) {  /* INDIRECT_I2C_CONNECTION? */
		/*  I2C connection will be done later on, depending on the media */
	} else {
		si2168b_tuner_i2c_enable(front_end);
	}

	if (front_end->demod->media == Si2168B_TERRESTRIAL) {
		if ( front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
			front_end->f_TER_tuner_enable(front_end->callback);
		}
		c->frequency = rf;
		if (fe->ops.tuner_ops.set_params) {
			if (fe->ops.tuner_ops.set_params(fe)) {
				SiTRACE("Terrestrial tuner set_params() error!\n");
			}
		} else {
			SiTRACE("WARNING: set_params() not available\n");
		}
		if ( front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
			front_end->f_TER_tuner_disable(front_end->callback);
		}
	}
	if ( front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
	} else {
		si2168b_tuner_i2c_disable(front_end);
	}

	return rf;
}

/************************************************************************************************************************
  Si2168B_lock_to_carrier function
  Use:      relocking function
            Used to relock on a channel for the current standard
            options:
              if freq = 0, do not tune. This is useful to test the lock time without the tuner delays.
              if freq < 0, do not tune and don't change settings. Just do a DD_RESTART. This is useful to test the relock time upom a reset.
  Parameter: standard the standard to lock to
  Parameter: freq                the frequency to lock to    (in Hz for TER, in kHz for SAT)
  Parameter: dvb_t_bandwidth_hz  the channel bandwidth in Hz (only for DVB-T and DVB-T2)
  Parameter: dvb_t_stream        the HP/LP stream            (only for DVB-T)
  Parameter: symbol_rate_bps     the symbol rate             (for DVB-C, MCNS and SAT)
  Parameter: dvb_c_constellation the DVB-C constellation     (only for DVB-C)
  Parameter: data_slice_id       the DVB-C2 data slice Id    (only for DVB-C2)
  Parameter: plp_id              the PLP Id                  (only for DVB-T2 and DVB-C2 when num_dslice  > 1)
  Parameter: T2_lock_mode        the DVB-T2 lock mode        (0='ANY', 1='T2-Base', 2='T2-Lite')
  Return:    1 if locked, 0 otherwise
************************************************************************************************************************/
static int si2168b_lock_to_carrier(struct dvb_frontend *fe
		, u8  standard
		, u32 freq
		, u32 dvb_t_bandwidth_hz
		, u8  dvb_t_stream
		, u32 symbol_rate_bps
		, u8  dvb_c_constellation
		, int plp_id
		, u8  T2_lock_mode
)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	Si2168B_L2_Context *front_end = priv->si_front_end;
	Si2168B_DD_STATUS_CMD_REPLY_struct dd_status;
	Si2168B_DD_BER_CMD_REPLY_struct dd_ber;

	int return_code;
	u32 lock_start_time;       /* lockStartTime is used to trace the time spent in si2168b_lock_to_carrier and is only set at when entering the function                       */
	u32 start_time;           /* startTime is used to measure internal durations. It is set in various places, whenever required                                                */
	u32 search_start_time = 0; /* searchStartTime is used to trace the time spent trying to lock. It is set differently from lockStartTime when returning from a handshake       */
	u32 search_delay;
	u32 handshake_delay;
	int lock;
	int new_lock;
	u32 max_lock_time_ms = 0;
	u32 min_lock_time_ms = 0;
	u16 data;
	u8  dd_mode_bw;
	u8  dd_mode_modulation;
	u8  dd_mode_invert_spectrum = Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_NORMAL;
	u8  dd_mode_auto_detect = Si2168B_DD_MODE_PROP_AUTO_DETECT_NONE;
	u8  dvbt_hierarchy_stream;
	u8  dvbt2_mode_lock_mode;
	u8  u_plp_id;
	u8  plp_id_sel_mode;
	u16 dvbc_symbol_rate_rate;
	u8  dvbc_constellation_constellation;
#ifdef __MCNS__
	u16 mcns_symbol_rate_rate;
	u8  mcns_constellation_constellation;
#endif /* __MCNS__ */
	u8  dvbt2_plp_select_plp_id;
	u8  dvbt2_plp_select_plp_id_sel_mode;
	u8  lockAbort = 0;

	lock_start_time = system_time(); /* lockStartTime is used to trace the time spent in si2168b_lock_to_carrier and is only set here */
	lock = 0;

	SiTRACE ("relock to %s at %u\n", si2168b_standard_name(standard), freq);

	if (front_end->handshakeUsed == 0) {
		new_lock = 1;
		search_start_time = lock_start_time;
	}
	if (front_end->handshakeUsed == 1) {
		if (front_end->handshakeOn == 1) {
			new_lock = 0;
			SiTRACE("lock_to_carrier_handshake : recalled after   handshake.\n");
		}
		if (front_end->handshakeOn == 0) {
			new_lock = 1;
			front_end->handshakeStart_ms = lock_start_time;
		}
		search_start_time = front_end->handshakeStart_ms;
		SiTRACE("lock_to_carrier_handshake : handshake start %d\n", front_end->handshakeStart_ms);
	}

	/* Setting max_lock_time_ms and min_lock_time_ms for locking on required standard */
	switch (standard) {
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
		max_lock_time_ms = Si2168B_DVBT_MAX_LOCK_TIME;
		min_lock_time_ms = Si2168B_DVBT_MIN_LOCK_TIME;
		if (front_end->auto_detect_TER) {
			max_lock_time_ms = Si2168B_DVBT2_MAX_LOCK_TIME;
		}
		break;
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
		max_lock_time_ms = Si2168B_DVBT2_MAX_LOCK_TIME;
		min_lock_time_ms = Si2168B_DVBT2_MIN_LOCK_TIME;
		if (front_end->auto_detect_TER) {
			min_lock_time_ms = Si2168B_DVBT_MIN_LOCK_TIME;
		}
		break;
	case Si2168B_DD_MODE_PROP_MODULATION_DVBC:
		max_lock_time_ms = si2168b_dvbc_max_lock_ms(front_end->demod, dvb_c_constellation, symbol_rate_bps);
		min_lock_time_ms = Si2168B_DVBC_MIN_LOCK_TIME;
		break;
#ifdef __MCNS__
	case Si2168B_DD_MODE_PROP_MODULATION_MCNS:
		max_lock_time_ms = si2168b_dvbc_max_lock_ms(front_end->demod, dvb_c_constellation, symbol_rate_bps);
		min_lock_time_ms = Si2168B_DVBC_MIN_LOCK_TIME;
		break;
#endif /* __MCNS__ */
	default: /* ATV */
		break;
	}

	/* change settings only if not testing the relock time upon a reset (activated if freq<0) */
	if ( (freq >= 0 ) && (new_lock == 1) ) {
		/* Setting demod for locking on required standard */
		switch (standard) {
		case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
		case Si2168B_DD_MODE_PROP_MODULATION_DVBT: {
			dd_mode_bw = dvb_t_bandwidth_hz / 1000000;
			dvbt_hierarchy_stream = dvb_t_stream;
			data = (dvbt_hierarchy_stream & Si2168B_DVBT_HIERARCHY_PROP_STREAM_MASK) << Si2168B_DVBT_HIERARCHY_PROP_STREAM_LSB;
			return_code = si2168b_set_property(front_end->demod, Si2168B_DVBT_HIERARCHY_PROP, data);
			if (dvb_t_bandwidth_hz == 1700000) {
				dd_mode_bw = Si2168B_DD_MODE_PROP_BW_BW_1D7MHZ;
			}
			if (front_end->auto_detect_TER) {
				SiTRACE("DVB-T/T2 auto detect\n");
				if (plp_id != -1) {
					plp_id_sel_mode = Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_MANUAL;
					u_plp_id = (u8)plp_id;
				} else {
					plp_id_sel_mode = Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_AUTO;
					u_plp_id = 0;
				}
				si2168b_dvbt2_plp_select(front_end->demod, u_plp_id, plp_id_sel_mode);
				dd_mode_modulation   = Si2168B_DD_MODE_PROP_MODULATION_AUTO_DETECT;
				dd_mode_auto_detect  = Si2168B_DD_MODE_PROP_AUTO_DETECT_AUTO_DVB_T_T2;
				dvbt2_mode_lock_mode = T2_lock_mode;
				data = (dvbt2_mode_lock_mode & Si2168B_DVBT2_MODE_PROP_LOCK_MODE_MASK) << Si2168B_DVBT2_MODE_PROP_LOCK_MODE_LSB ;
				si2168b_set_property(front_end->demod, Si2168B_DVBT2_MODE_PROP, data);
				SiTRACE ("T2_lock_mode %u\n", T2_lock_mode);
			} else {
				if (standard == Si2168B_DD_MODE_PROP_MODULATION_DVBT) {
					dvbt_hierarchy_stream = dvb_t_stream;
					data = (dvbt_hierarchy_stream & Si2168B_DVBT_HIERARCHY_PROP_STREAM_MASK) << Si2168B_DVBT_HIERARCHY_PROP_STREAM_LSB;
					return_code = si2168b_set_property(front_end->demod, Si2168B_DVBT_HIERARCHY_PROP, data);
				}
				if (standard == Si2168B_DD_MODE_PROP_MODULATION_DVBT2) {
					if (plp_id != -1) {
						dvbt2_plp_select_plp_id_sel_mode = Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_MANUAL;
						dvbt2_plp_select_plp_id = (u8)plp_id;
					} else {
						dvbt2_plp_select_plp_id_sel_mode = Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_AUTO;
						dvbt2_plp_select_plp_id = 0;
					}
					si2168b_dvbt2_plp_select(front_end->demod, dvbt2_plp_select_plp_id, dvbt2_plp_select_plp_id_sel_mode);
					dvbt2_mode_lock_mode = T2_lock_mode;
					data = (dvbt2_mode_lock_mode & Si2168B_DVBT2_MODE_PROP_LOCK_MODE_MASK) << Si2168B_DVBT2_MODE_PROP_LOCK_MODE_LSB ;
					si2168b_set_property(front_end->demod, Si2168B_DVBT2_MODE_PROP, data);
					SiTRACE ("T2_lock_mode %d\n", T2_lock_mode);
				}
			}
			data = (dd_mode_bw              & Si2168B_DD_MODE_PROP_BW_MASK             ) << Si2168B_DD_MODE_PROP_BW_LSB  |
					(dd_mode_modulation      & Si2168B_DD_MODE_PROP_MODULATION_MASK     ) << Si2168B_DD_MODE_PROP_MODULATION_LSB  |
					(dd_mode_invert_spectrum & Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_LSB  |
					(dd_mode_auto_detect     & Si2168B_DD_MODE_PROP_AUTO_DETECT_MASK    ) << Si2168B_DD_MODE_PROP_AUTO_DETECT_LSB ;
			si2168b_set_property(front_end->demod, Si2168B_DD_MODE_PROP, data);
			SiTRACE("bw %d Hz\n", dvb_t_bandwidth_hz);
			break;
		}
		case Si2168B_DD_MODE_PROP_MODULATION_DVBC:
			dd_mode_bw                       = 8;
			dd_mode_modulation   = Si2168B_DD_MODE_PROP_MODULATION_DVBC;
			dd_mode_auto_detect  = Si2168B_DD_MODE_PROP_AUTO_DETECT_DEFAULT;
			dvbc_symbol_rate_rate            = symbol_rate_bps / 1000;
			dvbc_constellation_constellation = dvb_c_constellation;
			data = (dd_mode_bw              & Si2168B_DD_MODE_PROP_BW_MASK             ) << Si2168B_DD_MODE_PROP_BW_LSB  |
					(dd_mode_modulation      & Si2168B_DD_MODE_PROP_MODULATION_MASK     ) << Si2168B_DD_MODE_PROP_MODULATION_LSB  |
					(dd_mode_invert_spectrum & Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_LSB  |
					(dd_mode_auto_detect     & Si2168B_DD_MODE_PROP_AUTO_DETECT_MASK    ) << Si2168B_DD_MODE_PROP_AUTO_DETECT_LSB ;
			si2168b_set_property(front_end->demod, Si2168B_DD_MODE_PROP, data);
			data = (dvbc_symbol_rate_rate & Si2168B_DVBC_SYMBOL_RATE_PROP_RATE_MASK) << Si2168B_DVBC_SYMBOL_RATE_PROP_RATE_LSB ;
			si2168b_set_property(front_end->demod, Si2168B_DVBC_SYMBOL_RATE_PROP, data);
			data = (dvbc_constellation_constellation & Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_MASK) << Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_LSB ;
			si2168b_set_property(front_end->demod, Si2168B_DVBC_CONSTELLATION_PROP, data);
			SiTRACE("sr %d bps, constel %d\n", symbol_rate_bps, dvb_c_constellation);
			break;
#ifdef __MCNS__
		case Si2168B_DD_MODE_PROP_MODULATION_MCNS:
			dd_mode_bw                       = 8;
			mcns_symbol_rate_rate            = symbol_rate_bps / 1000;
			mcns_constellation_constellation = dvb_c_constellation;
			si2168b_set_property(front_end->demod, Si2168B_DD_MODE_PROP, data);
			data = (mcns_symbol_rate_rate & Si2168B_MCNS_SYMBOL_RATE_PROP_RATE_MASK) << Si2168B_MCNS_SYMBOL_RATE_PROP_RATE_LSB ;
			si2168b_set_property(front_end->demod, Si2168B_MCNS_SYMBOL_RATE_PROP, data);
			data = (mcns_constellation_constellation & Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_MASK) << Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_LSB ;
			si2168b_set_property(front_end->demod, Si2168B_MCNS_CONSTELLATION_PROP, data);
			SiTRACE("sr %d bps, constel %d\n", mcns_symbol_rate_rate, mcns_constellation_constellation);
			break;
#endif /* __MCNS__ */
		default: /* ATV */
			SiTRACE("'%d' standard (%s) is not managed by Si2168B_lock_to_carrier\n", standard, si2168b_standard_name(standard));
			return 0;
			break;
		}

		if (lockAbort) {
			SiTRACE("si2168b_lock_to_carrier : lock aborted before tuning, after %d ms.\n", system_time() - lock_start_time );
			return 0;
		}

		/* ALlow using this function without tuning */
		if (freq != 0) {
			start_time = system_time();
			si2168b_set_tuner_params(fe, freq);
			SiTRACE("Si2168B_lock_to_carrier 'tune'  took %3d ms\n", system_time() - start_time);
		}

		start_time = system_time();
		si2168b_dd_restart(front_end->demod);
		SiTRACE("Si2168B_lock_to_carrier 'reset' took %3d ms\n", system_time() - start_time);

		/* as we will not lock in less than min_lock_time_ms, wait a while..., but check for a possible 'abort' from the application */
		start_time = system_time();
		while (system_time() - start_time < min_lock_time_ms) {
			if (lockAbort) {
				SiTRACE("si2168b_lock_to_carrier : lock aborted before checking lock status, after %d ms.\n", system_time() - lock_start_time);
				return 0;
			}
			/* Adapt here the minimal 'reaction time' of the application*/
			msleep(20);
		}
	}
	/* testing the relock time upon a reset (activated if freq<0) */
	if (freq < 0) {
		SiTRACE("Si2168B_lock_to_carrier 'only_reset'\n");
		si2168b_dd_restart(front_end->demod);
	}

	/* The actual lock check loop */
	while (1) {
		search_delay = system_time() - search_start_time;

		/* Check the status for the current modulation */

		switch (standard)
		default:
		case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
		case Si2168B_DD_MODE_PROP_MODULATION_DVBT2: {
			/* DVB-T/T2 auto detect seek loop, using si2168b_dd_status                                          */
			/* if DL LOCKED                            : demod is locked on a dd_status->modulation signal        */
			/* if DL NO_LOCK and rsqint_bit5 NO_CHANGE : demod is searching for a DVB-T/T2 signal                 */
			/* if DL NO_LOCK and rsqint_bit5 CHANGE    : demod says this is not a DVB-T/T2 signal (= 'neverlock') */
			return_code = si2168b_dd_status(front_end->demod, Si2168B_DD_STATUS_CMD_INTACK_CLEAR, &dd_status);
			if (return_code != NO_Si2168B_ERROR) {
				SiTRACE("Si2168B_lock_to_carrier: si2168b_dd_status error\n");
				goto exit_lock;
				break;
			}

			if (dd_status.dl == Si2168B_DD_STATUS_RESPONSE_DL_LOCKED) {
				/* Return 1 to signal that the Si2168B is locked on a valid DVB-T/T2 channel */
				SiTRACE("Si2168B_lock_to_carrier: locked on a %s signal\n", si2168b_standard_name(dd_status.modulation) );
				lock = 1;
				/* Make sure FEF mode is ON when locked on a T2 channel */
				if (dd_status.modulation == Si2168B_DD_MODE_PROP_MODULATION_DVBT2) {
					if (front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
						front_end->f_TER_tuner_enable(front_end->callback);
					} else {
						si2168b_tuner_i2c_enable(front_end);
					}
					SiTRACE("Si2168B_lock_to_carrier: tuner should enable FEF for DVBT2\n");
					si2168b_ter_fef(front_end, 1);
					if ( front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
						front_end->f_TER_tuner_disable(front_end->callback);
					} else {
						si2168b_tuner_i2c_disable(front_end);
					}
				}
				goto exit_lock;
			} else {
				/* SiTRACE("Si2168B_lock_to_carrier: NO LOCK\n"); */
				if (dd_status.rsqint_bit5 == Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT5_CHANGED ) {
					/* Return 0 if firmware signals 'no DVB-T/T2 channel' */
					SiTRACE ("'no DVB-T/T2 channel': not locked after %3d ms\n", search_delay);
					goto exit_lock;
				}
			}
			break;
		case Si2168B_DD_MODE_PROP_MODULATION_DVBC:
			return_code = si2168b_dd_status(front_end->demod, Si2168B_DD_STATUS_CMD_INTACK_CLEAR, &dd_status);

			if (return_code != NO_Si2168B_ERROR) {
				SiTRACE("Si2168B_lock_to_carrier: si2168b_dd_status error\n");
				goto exit_lock;
				break;
			}

			if (dd_status.dl == Si2168B_DD_STATUS_RESPONSE_DL_LOCKED) {
				/* Return 1 to signal that the Si2168B is locked on a valid SAT channel */
				SiTRACE("%s lock\n", si2168b_standard_name(dd_status.modulation));
				lock = 1;
				goto exit_lock;
			}
			break;
#ifdef __MCNS__
		case Si2168B_DD_MODE_PROP_MODULATION_MCNS:
			return_code = si2168b_dd_status(front_end->demod, Si2168B_DD_STATUS_CMD_INTACK_CLEAR, &dd_status);

			if (return_code != NO_Si2168B_ERROR) {
				SiTRACE("Si2168B_lock_to_carrier: si2168b_dd_status error\n");
				goto exit_lock;
				break;
			}

			if (dd_status.dl == Si2168B_DD_STATUS_RESPONSE_DL_LOCKED) {
				/* Return 1 to signal that the Si2168B is locked on a valid SAT channel */
				SiTRACE("%s lock\n", si2168b_standard_name(dd_status.modulation));
				lock = 1;
				goto exit_lock;
			}
			break;
#endif /* __MCNS__ */
		}

		/* timeout management (this should never happen if timeout values are correctly set) */
		search_delay = system_time() - search_start_time;
		if (search_delay >= max_lock_time_ms) {
			SiTRACE ("Si2168B_lock_to_carrier timeout(%d) after %d ms\n", max_lock_time_ms, search_delay);
			goto exit_lock;
			break;
		}

		if (front_end->handshakeUsed == 1) {
			handshake_delay = system_time() - lock_start_time;
			if (handshake_delay >= front_end->handshakePeriod_ms) {
				SiTRACE ("lock_to_carrier_handshake : handshake after %5d ms (at %10d). (search delay %6d ms)\n\n", handshake_delay, freq, search_delay);
				front_end->handshakeOn = 1;
				/* The application will check handshakeStart_ms to know whether the lock is complete or not */
				return search_delay;
			} else {
				SiTRACE ("lock_to_carrier_handshake : no handshake yet. (handshake delay %6d ms, search delay %6d ms)\n", handshake_delay, search_delay);
			}
		}

		if (lockAbort) {
			SiTRACE("si2168b_lock_to_carrier : lock aborted after %d ms.\n", system_time() - lock_start_time);
			goto exit_lock;
		}

		/* Check status every 10 ms */
		msleep(5);
	}

	exit_lock:

	front_end->handshakeOn = 0;
	search_delay = system_time() - search_start_time;

	if (lock) {
		si2168b_dd_ber(front_end->demod, Si2168B_DD_BER_CMD_RST_CLEAR, &dd_ber);
		SiTRACE ("Si2168B_lock_to_carrier 'lock'  took %3d ms\n", search_delay);
	} else {
		SiTRACE ("Si2168B_lock_to_carrier at %10d (%s) failed after %d ms\n", freq, si2168b_standard_name(dd_status.modulation), search_delay);
	}

	return lock;
}

/***********************************************************************************************************************
  si2168b_check_status function
  Use:        Status information function
              Used to retrieve the status byte
  Returns:    0 if no error
  Parameter:  error_code the error code.
 ***********************************************************************************************************************/
static u8 si2168b_check_status(si2168b_context *ctx)
{
    u8 rspByteBuffer[1];
    u8 ret;

	_mutex_lock(&ctx->lock);
    ret = si2168b_poll_for_response(ctx, 1, rspByteBuffer);
	_mutex_unlock(&ctx->lock);
    return ret;
}

/*---------------------------------------------------*/
/* Si2168B_SCAN_STATUS COMMAND                      */
/*---------------------------------------------------*/
static u8 si2168b_scan_status(si2168b_context *ctx, u8 intack, Si2168B_SCAN_STATUS_CMD_REPLY_struct *scan_status)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[11];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B SCAN_STATUS\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_SCAN_STATUS_CMD;
	cmdByteBuffer[1] = (u8) ( ( intack & Si2168B_SCAN_STATUS_CMD_INTACK_MASK ) << Si2168B_SCAN_STATUS_CMD_INTACK_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing SCAN_STATUS bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 11, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling SCAN_STATUS response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	scan_status->buzint      =   (( ( (rspByteBuffer[1]  )) >> Si2168B_SCAN_STATUS_RESPONSE_BUZINT_LSB      ) & Si2168B_SCAN_STATUS_RESPONSE_BUZINT_MASK      );
	scan_status->reqint      =   (( ( (rspByteBuffer[1]  )) >> Si2168B_SCAN_STATUS_RESPONSE_REQINT_LSB      ) & Si2168B_SCAN_STATUS_RESPONSE_REQINT_MASK      );
	scan_status->buz         =   (( ( (rspByteBuffer[2]  )) >> Si2168B_SCAN_STATUS_RESPONSE_BUZ_LSB         ) & Si2168B_SCAN_STATUS_RESPONSE_BUZ_MASK         );
	scan_status->req         =   (( ( (rspByteBuffer[2]  )) >> Si2168B_SCAN_STATUS_RESPONSE_REQ_LSB         ) & Si2168B_SCAN_STATUS_RESPONSE_REQ_MASK         );
	scan_status->scan_status =   (( ( (rspByteBuffer[3]  )) >> Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_LSB ) & Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_MASK );
	scan_status->rf_freq     =   (( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 ) | (rspByteBuffer[6]  << 16 ) | (rspByteBuffer[7]  << 24 )) >> Si2168B_SCAN_STATUS_RESPONSE_RF_FREQ_LSB     ) & Si2168B_SCAN_STATUS_RESPONSE_RF_FREQ_MASK     );
	scan_status->symb_rate   =   (( ( (rspByteBuffer[8]  ) | (rspByteBuffer[9]  << 8 )) >> Si2168B_SCAN_STATUS_RESPONSE_SYMB_RATE_LSB   ) & Si2168B_SCAN_STATUS_RESPONSE_SYMB_RATE_MASK   );
	scan_status->modulation  =   (( ( (rspByteBuffer[10] )) >> Si2168B_SCAN_STATUS_RESPONSE_MODULATION_LSB  ) & Si2168B_SCAN_STATUS_RESPONSE_MODULATION_MASK  );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

#ifdef __MCNS__
/*---------------------------------------------------*/
/* Si2168B_MCNS_STATUS COMMAND                      */
/*---------------------------------------------------*/
static u8 si2168b_mcns_status(si2168b_context *ctx, u8 intack)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[10];
	u8 ret = NO_Si2168B_ERROR;
	ctx->rsp->mcns_status.STATUS = ctx->status;

	SiTRACE("Si2168B MCNS_STATUS\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_MCNS_STATUS_CMD;
	cmdByteBuffer[1] = (u8) ( ( intack & Si2168B_MCNS_STATUS_CMD_INTACK_MASK ) << Si2168B_MCNS_STATUS_CMD_INTACK_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing MCNS_STATUS bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 10, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling MCNS_STATUS response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->mcns_status.pclint        =   (( ( (rspByteBuffer[1]  )) >> Si2168B_MCNS_STATUS_RESPONSE_PCLINT_LSB        ) & Si2168B_MCNS_STATUS_RESPONSE_PCLINT_MASK        );
	ctx->rsp->mcns_status.dlint         =   (( ( (rspByteBuffer[1]  )) >> Si2168B_MCNS_STATUS_RESPONSE_DLINT_LSB         ) & Si2168B_MCNS_STATUS_RESPONSE_DLINT_MASK         );
	ctx->rsp->mcns_status.berint        =   (( ( (rspByteBuffer[1]  )) >> Si2168B_MCNS_STATUS_RESPONSE_BERINT_LSB        ) & Si2168B_MCNS_STATUS_RESPONSE_BERINT_MASK        );
	ctx->rsp->mcns_status.uncorint      =   (( ( (rspByteBuffer[1]  )) >> Si2168B_MCNS_STATUS_RESPONSE_UNCORINT_LSB      ) & Si2168B_MCNS_STATUS_RESPONSE_UNCORINT_MASK      );
	ctx->rsp->mcns_status.pcl           =   (( ( (rspByteBuffer[2]  )) >> Si2168B_MCNS_STATUS_RESPONSE_PCL_LSB           ) & Si2168B_MCNS_STATUS_RESPONSE_PCL_MASK           );
	ctx->rsp->mcns_status.dl            =   (( ( (rspByteBuffer[2]  )) >> Si2168B_MCNS_STATUS_RESPONSE_DL_LSB            ) & Si2168B_MCNS_STATUS_RESPONSE_DL_MASK            );
	ctx->rsp->mcns_status.ber           =   (( ( (rspByteBuffer[2]  )) >> Si2168B_MCNS_STATUS_RESPONSE_BER_LSB           ) & Si2168B_MCNS_STATUS_RESPONSE_BER_MASK           );
	ctx->rsp->mcns_status.uncor         =   (( ( (rspByteBuffer[2]  )) >> Si2168B_MCNS_STATUS_RESPONSE_UNCOR_LSB         ) & Si2168B_MCNS_STATUS_RESPONSE_UNCOR_MASK         );
	ctx->rsp->mcns_status.cnr           =   (( ( (rspByteBuffer[3]  )) >> Si2168B_MCNS_STATUS_RESPONSE_CNR_LSB           ) & Si2168B_MCNS_STATUS_RESPONSE_CNR_MASK           );
	ctx->rsp->mcns_status.afc_freq      = (((( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 )) >> Si2168B_MCNS_STATUS_RESPONSE_AFC_FREQ_LSB      ) & Si2168B_MCNS_STATUS_RESPONSE_AFC_FREQ_MASK) <<Si2168B_MCNS_STATUS_RESPONSE_AFC_FREQ_SHIFT ) >>Si2168B_MCNS_STATUS_RESPONSE_AFC_FREQ_SHIFT      );
	ctx->rsp->mcns_status.timing_offset = (((( ( (rspByteBuffer[6]  ) | (rspByteBuffer[7]  << 8 )) >> Si2168B_MCNS_STATUS_RESPONSE_TIMING_OFFSET_LSB ) & Si2168B_MCNS_STATUS_RESPONSE_TIMING_OFFSET_MASK) <<Si2168B_MCNS_STATUS_RESPONSE_TIMING_OFFSET_SHIFT ) >>Si2168B_MCNS_STATUS_RESPONSE_TIMING_OFFSET_SHIFT );
	ctx->rsp->mcns_status.constellation =   (( ( (rspByteBuffer[8]  )) >> Si2168B_MCNS_STATUS_RESPONSE_CONSTELLATION_LSB ) & Si2168B_MCNS_STATUS_RESPONSE_CONSTELLATION_MASK );
	ctx->rsp->mcns_status.sp_inv        =   (( ( (rspByteBuffer[8]  )) >> Si2168B_MCNS_STATUS_RESPONSE_SP_INV_LSB        ) & Si2168B_MCNS_STATUS_RESPONSE_SP_INV_MASK        );
	ctx->rsp->mcns_status.interleaving  =   (( ( (rspByteBuffer[9]  )) >> Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_LSB  ) & Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_MASK  );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif /* __MCNS__ */

/************************************************************************************************************************
  NAME: si2168b_set_invert_spectrum
  DESCRIPTION: return the required invert_spectrum value depending on the settings:
              front_end->demod->media
              front_end->satellite_spectrum_inversion
              front_end->lnb_type
              front_end->unicable_spectrum_inversion

  Parameter:  Pointer to Si2168B Context
  Returns:    the required invert_spectrum value
************************************************************************************************************************/
static u8 si2168b_set_invert_spectrum(Si2168B_L2_Context *front_end)
{
	u8 inversion;

	if (front_end->demod->media == Si2168B_TERRESTRIAL) {
		inversion = Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_NORMAL;
	}
	return inversion;
}

/*---------------------------------------------------*/
/* Si2168B_SCAN_CTRL COMMAND                        */
/*---------------------------------------------------*/
static u8 si2168b_scan_ctrl(si2168b_context *ctx,
		u8 action,
		u32 tuned_rf_freq)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[8];
	u8 rspByteBuffer[1];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B SCAN_CTRL\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_SCAN_CTRL_CMD;
	cmdByteBuffer[1] = (u8) ( ( action        & Si2168B_SCAN_CTRL_CMD_ACTION_MASK        ) << Si2168B_SCAN_CTRL_CMD_ACTION_LSB);
	cmdByteBuffer[2] = (u8)0x00;
	cmdByteBuffer[3] = (u8)0x00;
	cmdByteBuffer[4] = (u8) ( ( tuned_rf_freq & Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_MASK ) << Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_LSB);
	cmdByteBuffer[5] = (u8) ((( tuned_rf_freq & Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_MASK ) << Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_LSB)>>8);
	cmdByteBuffer[6] = (u8) ((( tuned_rf_freq & Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_MASK ) << Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_LSB)>>16);
	cmdByteBuffer[7] = (u8) ((( tuned_rf_freq & Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_MASK ) << Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_LSB)>>24);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 8, cmdByteBuffer) != 8) {
		SiTRACE("Error writing SCAN_CTRL bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 1, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling SCAN_CTRL response\n");
		ret = error_code;
	}

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/************************************************************************************************************************
  NAME: si2168b_channel_seek_init
  DESCRIPTION: logs the seek parameters in the context structure
  Programming Guide Reference:    Flowchart TBD (Channel Scan flowchart)

  Parameter:  Pointer to Si2168B Context
  Parameter:  starting Frequency Hz
  Parameter:  ending Frequency Hz
  Parameter:  min RSSI dBm
  Parameter:  max RSSI dBm
  Parameter:  min SNR 1/2 dB
  Parameter:  max SNR 1/2 dB
  Returns:    0 if successful, otherwise an error.
************************************************************************************************************************/
static int si2168b_channel_seek_init(struct dvb_frontend *fe, Si2168B_CHANNEL_SEEK_PARAM_struct *seek_param)
{
	const u8 scan_ien_buzien = Si2168B_SCAN_IEN_PROP_BUZIEN_ENABLE; /* (default 'DISABLE') */
	const u8 scan_ien_reqien = Si2168B_SCAN_IEN_PROP_REQIEN_ENABLE; /* (default 'DISABLE') */
	const u8 scan_int_sense_reqnegen = Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_DISABLE; /* (default 'DISABLE') */
	const u8 scan_int_sense_reqposen = Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_ENABLE;  /* (default 'ENABLE') */
	const u8 scan_int_sense_buznegen = Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_ENABLE;  /* (default 'ENABLE') */
	const u8 scan_int_sense_buzposen = Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_DISABLE; /* (default 'DISABLE') */
    const u8 scan_ter_config_analog_bw = Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_8MHZ;
    const u8 scan_ter_config_search_analog = Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_DISABLE;

#ifdef ALLOW_Si2168B_BLINDSCAN_DEBUG
	const u8 scan_ter_config_scan_debug = 0x0f;
#else
	const u8 scan_ter_config_scan_debug = 0;
#endif /* ALLOW_Si2168B_BLINDSCAN_DEBUG */

	struct Si2168B_Priv *priv = fe->demodulator_priv;
	Si2168B_L2_Context *front_end = priv->si_front_end;
	Si2168B_SCAN_STATUS_CMD_REPLY_struct scan_status;
	u8  modulation = 0;
	u16 data;
	u8  dd_mode_bw = seek_param->seekBWHz / 1000000;
	u8  dd_mode_modulation = front_end->standard;
	u8  dd_mode_auto_detect = Si2168B_DD_MODE_PROP_AUTO_DETECT_NONE;
	u8  dd_mode_invert_spectrum = Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_DEFAULT;
	u8  dvbt2_mode_lock_mode;
	u8  dvbt_hierarchy_stream;
	u8  dvbc_constellation_constellation;
#ifdef __MCNS__
	u8  mcns_constellation_constellation;
#endif
	u16 scan_fmin = front_end->demod->scan_fmin;
	u16 scan_fmax = front_end->demod->scan_fmax;
	u16 scan_symb_rate_min = front_end->demod->scan_symb_rate_min;
	u16 scan_symb_rate_max = front_end->demod->scan_symb_rate_max;
    u8  scan_ter_config_mode;

	if (front_end->demod->media == Si2168B_TERRESTRIAL) {
		SiTRACE("media TERRESTRIAL\n");
		front_end->tuneUnitHz = 1;
	}
	SiTRACE ("blindscan_interaction >> (init  ) si2168b_scan_ctrl( front_end->demod, Si2168B_SCAN_CTRL_CMD_ACTION_ABORT)\n");
	si2168b_scan_ctrl(front_end->demod, Si2168B_SCAN_CTRL_CMD_ACTION_ABORT, 0);
	/* Check detection standard based on dd_mode.modulation and dd_mode.auto_detect */
	SiTRACE("dd_mode.modulation %d, dd_mode.auto_detect %d\n",  dd_mode_modulation, dd_mode_auto_detect);
	switch (dd_mode_modulation) {
	case Si2168B_DD_MODE_PROP_MODULATION_AUTO_DETECT:
		switch (dd_mode_auto_detect) {
		case Si2168B_DD_MODE_PROP_AUTO_DETECT_AUTO_DVB_T_T2:
			modulation = Si2168B_DD_MODE_PROP_MODULATION_DVBT2;
			break;
		default:
			SiTRACE("AUTO DETECT '%d' is not managed by si2168b_channel_seek_init\n", dd_mode_auto_detect);
			break;
		}
		break;
	case Si2168B_DD_MODE_PROP_MODULATION_DVBC:
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
		modulation = dd_mode_modulation;
		break;
	default:
		SiTRACE("'%d' modulation (%s) is not managed by si2168b_channel_seek_init\n", dd_mode_modulation, si2168b_standard_name(dd_mode_modulation));
		break;
	}
	SiTRACE("si2168b_channel_seek_init for %s (%d)\n", si2168b_standard_name(modulation), modulation );
	switch (modulation) {
	case Si2168B_DD_MODE_PROP_MODULATION_DVBC:
		/* Forcing BW to 8 MHz for DVB-C */
		seek_param->seekBWHz = 8000000;
		dd_mode_modulation  = Si2168B_DD_MODE_PROP_MODULATION_DVBC;
		dd_mode_auto_detect = Si2168B_DD_MODE_PROP_AUTO_DETECT_NONE;
		dvbc_constellation_constellation = Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_AUTO;
		data = (dvbc_constellation_constellation & Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_MASK) << Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_LSB;
		si2168b_set_property(front_end->demod, Si2168B_DVBC_CONSTELLATION_PROP, data);
		SiTRACE("DVB-C AFC range %d\n", 100);
		break;
#ifdef __MCNS__
	case Si2168B_DD_MODE_PROP_MODULATION_MCNS:
		/* Forcing BW to 8 MHz for MCNS */
		seekBWHz = 8000000;
		dd_mode_modulation  = Si2168B_DD_MODE_PROP_MODULATION_MCNS;
		dd_mode_auto_detect = Si2168B_DD_MODE_PROP_AUTO_DETECT_NONE;
		mcns_constellation_constellation = Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_AUTO;
		data = (mcns_constellation_constellation & Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_MASK) << Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_LSB;
		si2168b_set_property(front_end->demod, Si2168B_MCNS_CONSTELLATION_PROP, data);
		SiTRACE("MCNS AFC range %d\n", front_end->demod->prop->mcns_afc_range);
		break;
#endif /* __MCNS__ */
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
		dvbt_hierarchy_stream = Si2168B_DVBT_HIERARCHY_PROP_STREAM_LP;
		data = (dvbt_hierarchy_stream & Si2168B_DVBT_HIERARCHY_PROP_STREAM_MASK) << Si2168B_DVBT_HIERARCHY_PROP_STREAM_LSB;
		si2168b_set_property(front_end->demod, Si2168B_DVBT_HIERARCHY_PROP, data);
		dd_mode_modulation  = Si2168B_DD_MODE_PROP_MODULATION_AUTO_DETECT;
		dd_mode_auto_detect = Si2168B_DD_MODE_PROP_AUTO_DETECT_AUTO_DVB_T_T2;
		SiTRACE("DVB-T AFC range %d DVB-T2 AFC range %d\n", 550, 550);
		si2168b_dvbt2_plp_select(front_end->demod, 0, Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_AUTO);
		dvbt2_mode_lock_mode = Si2168B_DVBT2_MODE_PROP_LOCK_MODE_ANY;
		data = (dvbt2_mode_lock_mode & Si2168B_DVBT2_MODE_PROP_LOCK_MODE_MASK) << Si2168B_DVBT2_MODE_PROP_LOCK_MODE_LSB;
		si2168b_set_property(front_end->demod, Si2168B_DVBT2_MODE_PROP, data);
		break;
	default:
		SiTRACE("'%d' modulation (%s) is not managed by si2168b_channel_seek_init\n", modulation, si2168b_standard_name(modulation));
		break;
	}

	front_end->seekAbort = 0;

	SiTRACE("si2168b_channel_seek_init with %d to  %d, sawBW %d, minSR %d, maxSR %d\n", seek_param->rangeMin, seek_param->rangeMax, seek_param->seekBWHz, seek_param->minSRbps, seek_param->maxSRbps);
	SiTRACE("spectrum inversion %d\n",front_end->demod->dd_mode_invert_spectrum );
	scan_fmin = front_end->tuneUnitHz ? seek_param->rangeMin >> 16 : seek_param->rangeMin & 0x0000FFFF;
	scan_fmax = front_end->tuneUnitHz ? seek_param->rangeMax >> 16 : seek_param->rangeMax & 0x0000FFFF;
	scan_symb_rate_min = seek_param->minSRbps / 1000;
	scan_symb_rate_max = seek_param->maxSRbps / 1000;

	data = (scan_fmin & Si2168B_SCAN_FMIN_PROP_SCAN_FMIN_MASK) << Si2168B_SCAN_FMIN_PROP_SCAN_FMIN_LSB;
	si2168b_set_property(front_end->demod, Si2168B_SCAN_FMIN_PROP, data);
	data = (scan_fmax & Si2168B_SCAN_FMAX_PROP_SCAN_FMAX_MASK) << Si2168B_SCAN_FMAX_PROP_SCAN_FMAX_LSB;
	si2168b_set_property(front_end->demod, Si2168B_SCAN_FMAX_PROP, data);
	data = (scan_symb_rate_min & Si2168B_SCAN_SYMB_RATE_MIN_PROP_SCAN_SYMB_RATE_MIN_MASK) << Si2168B_SCAN_SYMB_RATE_MIN_PROP_SCAN_SYMB_RATE_MIN_LSB;
	si2168b_set_property(front_end->demod, Si2168B_SCAN_SYMB_RATE_MIN_PROP, data);
	data = (scan_symb_rate_max & Si2168B_SCAN_SYMB_RATE_MAX_PROP_SCAN_SYMB_RATE_MAX_MASK) << Si2168B_SCAN_SYMB_RATE_MAX_PROP_SCAN_SYMB_RATE_MAX_LSB;
	si2168b_set_property(front_end->demod, Si2168B_SCAN_SYMB_RATE_MAX_PROP, data);

	data = (scan_ien_buzien & Si2168B_SCAN_IEN_PROP_BUZIEN_MASK) << Si2168B_SCAN_IEN_PROP_BUZIEN_LSB |
           (scan_ien_reqien & Si2168B_SCAN_IEN_PROP_REQIEN_MASK) << Si2168B_SCAN_IEN_PROP_REQIEN_LSB;
	si2168b_set_property(front_end->demod, Si2168B_SCAN_IEN_PROP, data);

    data = (scan_int_sense_buznegen & Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_MASK) << Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_LSB |
           (scan_int_sense_reqnegen & Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_MASK) << Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_LSB |
           (scan_int_sense_buzposen & Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_MASK) << Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_LSB |
           (scan_int_sense_reqposen & Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_MASK) << Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_LSB ;
	si2168b_set_property(front_end->demod, Si2168B_SCAN_INT_SENSE_PROP, data);

	if (front_end->demod->media == Si2168B_TERRESTRIAL) {
		if ( seek_param->rangeMin == seek_param->rangeMax ) {
			scan_ter_config_mode = Si2168B_SCAN_TER_CONFIG_PROP_MODE_BLIND_LOCK;
			SiTRACE("Blindlock < %8d %8d > < %8d %8d >\n", front_end->demod->scan_fmin, front_end->demod->scan_fmax, front_end->demod->scan_symb_rate_min, front_end->demod->scan_symb_rate_max);
		} else {
			scan_ter_config_mode = Si2168B_SCAN_TER_CONFIG_PROP_MODE_BLIND_SCAN;
			SiTRACE("Blindscan < %8d %8d > < %8d %8d >\n", front_end->demod->scan_fmin, front_end->demod->scan_fmax, front_end->demod->scan_symb_rate_min, front_end->demod->scan_symb_rate_max);
		}
	    data = (scan_ter_config_mode          & Si2168B_SCAN_TER_CONFIG_PROP_MODE_MASK         ) << Si2168B_SCAN_TER_CONFIG_PROP_MODE_LSB  |
	           (scan_ter_config_analog_bw     & Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_MASK    ) << Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_LSB  |
	           (scan_ter_config_search_analog & Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_MASK) << Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_LSB |
	           (scan_ter_config_scan_debug    & Si2168B_SCAN_TER_CONFIG_PROP_SCAN_DEBUG_MASK   ) << Si2168B_SCAN_TER_CONFIG_PROP_SCAN_DEBUG_LSB ;
		si2168b_set_property(front_end->demod, Si2168B_SCAN_TER_CONFIG_PROP, data);
		if (seek_param->seekBWHz == 1700000) {
			dd_mode_bw = Si2168B_DD_MODE_PROP_BW_BW_1D7MHZ;
		} else {
			dd_mode_bw = seek_param->seekBWHz / 1000000;
		}
	}
    data = (dd_mode_bw              & Si2168B_DD_MODE_PROP_BW_MASK             ) << Si2168B_DD_MODE_PROP_BW_LSB  |
           (dd_mode_modulation      & Si2168B_DD_MODE_PROP_MODULATION_MASK     ) << Si2168B_DD_MODE_PROP_MODULATION_LSB  |
           (dd_mode_invert_spectrum & Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_LSB  |
           (dd_mode_auto_detect     & Si2168B_DD_MODE_PROP_AUTO_DETECT_MASK    ) << Si2168B_DD_MODE_PROP_AUTO_DETECT_LSB ;
	si2168b_set_property(front_end->demod, Si2168B_DD_MODE_PROP, data);

	si2168b_dd_restart(front_end->demod);

	si2168b_scan_status(front_end->demod, Si2168B_SCAN_STATUS_CMD_INTACK_OK, &scan_status);
	SiTRACE("blindscan_status leaving Seek_Init %s\n", si2168b_trace_scan_status(scan_status.scan_status) );
	/* Preparing the next call to si2168b_scan_ctrl which needs to be a 'START'*/
	front_end->demod->scan_ctrl_action = Si2168B_SCAN_CTRL_CMD_ACTION_START;
	front_end->handshakeOn = 0;
	SiTRACE("blindscan_handshake : Seek_Next will return every ~%d ms\n", front_end->handshakePeriod_ms );
	return 0;
}

/************************************************************************************************************************
  NAME: si2168b_channel_seek_next
  DESCRIPTION: Looks for the next channel, starting from the last detected channel
  Programming Guide Reference:    Flowchart TBD (Channel Scan flowchart)

  Parameter:  Pointer to Si2168B Context
  Returns:    1 if channel is found, 0 otherwise (either abort or end of range)
              Any other value represents the time spent searching (if front_end->handshakeUsed == 1)
************************************************************************************************************************/
static int si2168b_channel_seek_next(struct dvb_frontend *fe, Si2168B_CHANNEL_SEEK_PARAM_struct *seek_param, Si2168B_CHANNEL_SEEK_NEXT_REPLY_struct *channel_status)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	Si2168B_L2_Context *front_end = priv->si_front_end;
	Si2168B_DD_STATUS_CMD_REPLY_struct    dd_status = { 0 };
	Si2168B_DVBT_STATUS_CMD_REPLY_struct  dvbt_status = { 0 };
	Si2168B_DVBT2_STATUS_CMD_REPLY_struct dvbt2_status = { 0 };
	Si2168B_DVBC_STATUS_CMD_REPLY_struct  dvbc_status = { 0 };
	Si2168B_SCAN_STATUS_CMD_REPLY_struct  scan_status = { 0 };
	int return_code;
	int seek_freq;
	int seek_freq_kHz;
	s32 detected_rf;
	int channelIncrement;
	int startTime;        /* startTime is used to measure internal durations. It is set in various places, whenever required                                                       */
	int seekStartTime;    /* seekStartTime    is used to trace the time spent in si2168b_channel_seek_next and is only set when entering the function                            */
	int buzyStartTime;    /* buzyStartTime   is used to trace the time spent waiting for scan_status.buz to be different from 'BUZY'                                               */
	int timeoutStartTime; /* timeoutStartTime is used to make sure the FW is correctly responding. It is set differently from seekStartTime when returning from a handshake        */
	int searchStartTime = 0;  /* searchStartTime  is used to trace the time spent trying to detect a channel. It is set differently from seekStartTime when returning from a handshake */
	int timeoutDelay;
	int handshakeDelay;
	int searchDelay;
	int max_lock_time_ms;
	int min_lock_time_ms;
	int max_decision_time_ms;
	int blind_mode = 0;
	int skip_resume;
	int start_resume;
	u8 previous_scan_status;
	u8 jump_to_next_channel;
	si2168b_context *ctx;

	scan_status.scan_status = Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_IDLE;

	ctx = front_end->demod;

	/* Clear all return values which may not be used depending on the standard */
	channel_status->bandwidth_Hz    = 0;
	channel_status->stream          = 0;
	channel_status->symbol_rate_bps = 0;
	channel_status->constellation   = 0;
	channel_status->num_plp         = 0;
	channel_status->T2_base_lite    = 0;

	if (front_end->seekAbort) {
		SiTRACE("si2168b_channel_seek_next : previous run aborted. Please si2168b_channel_seek_init to perform a new search.\n");
		return 0;
	}

	SiTRACE("front_end->standard %d (%s)\n",front_end->standard, si2168b_standard_name(front_end->standard) );

	/* Setting max and max lock times and blind_mode flag */
	switch ( front_end->standard ) {
	/* For T/T2 detection, use the max value between Si2168B_DVBT_MAX_LOCK_TIME and Si2168B_DVBT2_MAX_LOCK_TIME */
	/* With Si2168B-A, it's Si2168B_DVBT2_MAX_LOCK_TIME                                                         */
	/* This value will be refined as soon as the standard is known, i.e. when PCL = 1                         */
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
		blind_mode = 0;
		max_lock_time_ms = Si2168B_DVBT_MAX_LOCK_TIME;
		max_lock_time_ms = Si2168B_DVBT2_MAX_LOCK_TIME;
		min_lock_time_ms = Si2168B_DVBT_MIN_LOCK_TIME;
		break;
	case Si2168B_DD_MODE_PROP_MODULATION_DVBC:
		blind_mode = 1;
		max_lock_time_ms = Si2168B_DVBC_MAX_SEARCH_TIME;
		min_lock_time_ms = Si2168B_DVBC_MIN_LOCK_TIME;
		break;
	default:
		SiTRACE("'%d' standard (%s) is not managed by si2168b_channel_seek_next\n", front_end->demod->dd_mode_modulation, si2168b_standard_name(front_end->demod->dd_mode_modulation));
		front_end->seekAbort = 1;
		return 0;
	}
	SiTRACE("blindscan : max_lock_time_ms %d\n", max_lock_time_ms);

	seekStartTime = system_time();

	if (front_end->handshakeUsed == 0) {
		start_resume = 1;
		searchStartTime = seekStartTime;
	}

	if (front_end->handshakeUsed == 1) {
		/* Skip tuner and demod settings if recalled after handshaking */
		if (front_end->handshakeOn == 1) {
			start_resume = 0;
			SiTRACE("blindscan_handshake : recalled after handshake. Skipping tuner and demod settings\n");
		}
		if (front_end->handshakeOn == 0) {
			start_resume = 1;
			if (front_end->demod->scan_ctrl_action == Si2168B_SCAN_CTRL_CMD_ACTION_START) {
				SiTRACE("blindscan_handshake : no handshake : starting.\n");
			} else {
				SiTRACE("blindscan_handshake : no handshake : resuming.\n");
			}
			front_end->handshakeStart_ms = seekStartTime;
			SiTRACE("blindscan_handshake : handshake start %d\n", front_end->handshakeStart_ms);
		}
		searchStartTime = front_end->handshakeStart_ms;
	}

	if (start_resume == 1) {
		/* Enabling FEF control for T/T2 */
		switch ( front_end->standard ) {
		case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
		case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
			if ( front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
				front_end->f_TER_tuner_enable(front_end->callback);
			} else {
				si2168b_tuner_i2c_enable(front_end);
			}
			si2168b_ter_fef(front_end,1);
			if ( front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
				front_end->f_TER_tuner_disable(front_end->callback);
			} else {
				si2168b_tuner_i2c_disable(front_end);
			}
			break;
		default:
			break;
		}
	}

	max_decision_time_ms = max_lock_time_ms;

	/* Select TER channel increment (this value will only be used for 'TER' scanning) */
	channelIncrement = seek_param->seekStepHz;

	/* Start Seeking */
	SiTRACE("si2168b_channel_seek_next rangeMin %10d, rangeMax %10d blind_mode %d\n", seek_param->rangeMin, seek_param->rangeMax, blind_mode);

	seek_freq = seek_param->rangeMin;

	if (blind_mode == 0) { /* DVB-T / DVB-T2 */
		while ( seek_freq <= seek_param->rangeMax ) {
			/* Call the si2168b_tune command to tune the frequency */
			if (si2168b_set_tuner_params(fe, seek_freq )!= seek_freq) {
				/* Manage possible tune error */
				SiTRACE("si2168b_channel_seek_next Tune error at %d, aborting (skipped)\n", seek_freq);
				front_end->seekAbort = 1;
				return 0;
			}

			timeoutStartTime = system_time();
			si2168b_dd_restart(ctx);

			/* as we will not lock in less than min_lock_time_ms, wait a while... */
			msleep(min_lock_time_ms);

			jump_to_next_channel = 0;

			while (!jump_to_next_channel) {

				if ((front_end->standard == Si2168B_DD_MODE_PROP_MODULATION_DVBT) || (front_end->standard == Si2168B_DD_MODE_PROP_MODULATION_DVBT2) ) {

					return_code = si2168b_dd_status(ctx, Si2168B_DD_STATUS_CMD_INTACK_CLEAR, &dd_status);
					if (return_code != NO_Si2168B_ERROR) {
						SiTRACE("si2168b_channel_seek_next: si2168b_dd_status error at %d, aborting\n", seek_freq);
						front_end->seekAbort = 1;
						return 0;
					}

					searchDelay = system_time() - searchStartTime;

					if ( (dd_status.dl == Si2168B_DD_STATUS_RESPONSE_DL_NO_LOCK) && (dd_status.rsqstat_bit5 == Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT5_NO_CHANGE ) ) {
						/* Check PCL to refine the max_lock_time_ms value if the standard has been detected */
						if (dd_status.pcl == Si2168B_DD_STATUS_RESPONSE_PCL_LOCKED) {
							if (dd_status.modulation == Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBT) {
								max_lock_time_ms = Si2168B_DVBT_MAX_LOCK_TIME ;
							}
						}
					}
					if ( (dd_status.dl == Si2168B_DD_STATUS_RESPONSE_DL_NO_LOCK) && (dd_status.rsqstat_bit5 == Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT5_CHANGED) ) {
						SiTRACE ("NO DVBT/T2. Jumping from  %d after %3d ms\n", seek_freq/1000000, searchDelay);
						if (seek_freq == seek_param->rangeMax) {
							seek_param->rangeMin = seek_freq;
						}
						seek_freq = seek_freq + channelIncrement;
						jump_to_next_channel = 1;
						break;
					}
					if ( (dd_status.dl == Si2168B_DD_STATUS_RESPONSE_DL_LOCKED) && (dd_status.rsqstat_bit5 == Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT5_NO_CHANGE ) ) {
						channel_status->standard = dd_status.modulation;
						if (channel_status->standard == Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBT) {
							si2168b_dvbt_status(ctx, Si2168B_DVBT_STATUS_CMD_INTACK_CLEAR, &dvbt_status);
							detected_rf = seek_freq + dvbt_status.afc_freq * 1000;
							if (dvbt_status.hierarchy == Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_NONE) {
								channel_status->stream = Si2168B_DVBT_HIERARCHY_PROP_STREAM_HP;
							} else {
								channel_status->stream = dvbt_status.hierarchy;
							}
							channel_status->bandwidth_Hz = front_end->demod->dd_mode_bw * 1000000;
							channel_status->freq         = detected_rf;
							SiTRACE ("DVB-T  lock at %d after %3d ms\n", (detected_rf)/1000000, searchDelay);
						}
						if (channel_status->standard == Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBT2) {
							if ( front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
								front_end->f_TER_tuner_enable(front_end->callback);
							} else {
								si2168b_tuner_i2c_enable(front_end);
							}
							si2168b_ter_fef(front_end,1);
							if ( front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
								front_end->f_TER_tuner_disable(front_end->callback);
							} else {
								si2168b_tuner_i2c_disable(front_end);
							}
							si2168b_dvbt2_status(ctx, Si2168B_DVBT_STATUS_CMD_INTACK_CLEAR, &dvbt2_status);
							channel_status->num_plp = dvbt2_status.num_plp;
							detected_rf = seek_freq + dvbt2_status.afc_freq * 1000;
							SiTRACE ("DVB-T2 lock at %d after %3d ms\n", detected_rf / 1000000, searchDelay);
							switch (front_end->demod->dd_mode_bw) {
							case Si2168B_DD_MODE_PROP_BW_BW_1D7MHZ : {
								channel_status->bandwidth_Hz = 1700000;
								break;
							}
							default: {
								channel_status->bandwidth_Hz = front_end->demod->dd_mode_bw * 1000000;
								break; }
							}
							channel_status->T2_base_lite = dvbt2_status.t2_mode;
							channel_status->freq         = detected_rf;
						}
						/* Set min seek_freq for next seek */
						seek_param->rangeMin = seek_freq + seek_param->seekBWHz;
						/* Return 1 to signal that the Si2168B is locked on a valid channel */
						return 1;
					}
				}

				/* timeout management (this should only trigger if the channel is very difficult, i.e. when pcl = 1 and dl = 0 until the timeout) */
				timeoutDelay = system_time() - timeoutStartTime;
				if (timeoutDelay >= max_lock_time_ms) {
					SiTRACE ("Timeout (blind_mode = 0) from  %d after %3d ms\n", seek_freq/1000000, timeoutDelay);
					seek_freq = seek_freq + channelIncrement;
					jump_to_next_channel = 1;
					break;
				}
				/* Check status every n ms */
				msleep(10);
			}
		}
	}

	if (blind_mode == 1) { /* DVB-C / DVB-S / DVB-S2 / MCNS */

		if (front_end->tuneUnitHz == 1) {
			seek_freq_kHz = seek_freq / 1000;
		} else {
			seek_freq_kHz = seek_freq;
		}

		previous_scan_status = scan_status.scan_status;
		/* Checking blindscan status before issuing a 'start' or 'resume' */
		si2168b_scan_status(front_end->demod, Si2168B_SCAN_STATUS_CMD_INTACK_OK, &scan_status);
		SiTRACE("blindscan_status      %s buz %d\n", si2168b_trace_scan_status(scan_status.scan_status), scan_status.buz);

		if (scan_status.scan_status != previous_scan_status) {
			SiTRACE ("scan_status changed from %s to %s\n", si2168b_trace_scan_status(previous_scan_status), si2168b_trace_scan_status(scan_status.scan_status));
		}

		if (start_resume) {
			/* Wait for scan_status.buz to be '0' before issuing SCAN_CTRL */
			buzyStartTime = system_time();
			while (scan_status.buz == Si2168B_SCAN_STATUS_RESPONSE_BUZ_BUSY) {
				si2168b_scan_status(front_end->demod, Si2168B_SCAN_STATUS_CMD_INTACK_OK, &scan_status);
				SiTRACE ("blindscan_interaction ?? (buzy)   si2168b_scan_status scan_status.buz %d after %d ms\n", scan_status.buz, system_time() - buzyStartTime);
				if (system_time() - buzyStartTime > 100) {
					SiTRACE ("blindscan_interaction -- (error)  si2168b_scan_status is always 'BUZY'\n");
					return 0;
				}
			}
			if (front_end->demod->scan_ctrl_action == Si2168B_SCAN_CTRL_CMD_ACTION_START) {
				SiTRACE ("blindscan_interaction >> (start ) si2168b_scan_ctrl( front_end->demod, %d, %8d) \n", front_end->demod->scan_ctrl_action, seek_freq_kHz);
			} else {
				SiTRACE ("blindscan_interaction >> (resume) si2168b_scan_ctrl( front_end->demod, %d, %8d) \n", front_end->demod->scan_ctrl_action, seek_freq_kHz);
			}
			return_code = si2168b_scan_ctrl (front_end->demod, front_end->demod->scan_ctrl_action, seek_freq_kHz);
			if (return_code != NO_Si2168B_ERROR) {
				SiTRACE ("blindscan_interaction -- (error1) si2168b_scan_ctrl %d      ERROR at %10d (%d)\n!!!!!!!!!!!!!!!!!!!!!!!\n", front_end->demod->scan_ctrl_action, seek_freq_kHz, scan_status.scan_status);
				SiTRACE("scan_status.buz %d\n", scan_status.buz);
				return 0;
			}
		}
		front_end->demod->scan_ctrl_action = Si2168B_SCAN_CTRL_CMD_ACTION_RESUME;

		startTime = system_time();
		/* as we will not lock in less than min_lock_time_ms, wait a while... */
		while (system_time() - startTime < min_lock_time_ms) {
			if (front_end->seekAbort) {
				break;
			}
			/* Adapt here the minimal 'reaction time' of the application*/
			msleep(20);
		}

		timeoutStartTime = system_time();

		/* The actual search loop... */
		while ( 1 ) {

			si2168b_check_status(front_end->demod);

			searchDelay = system_time() - searchStartTime;

			if ( (front_end->demod->status_scanint == Si2168B_STATUS_SCANINT_TRIGGERED) ) {

				/* There is an interaction with the FW, refresh the timeoutStartTime */
				timeoutStartTime = system_time();

				si2168b_scan_status(front_end->demod, Si2168B_SCAN_STATUS_CMD_INTACK_CLEAR, &scan_status);
				SiTRACE("blindscan_status      %s\n", si2168b_trace_scan_status(scan_status.scan_status) );
				skip_resume = 0;

				switch (scan_status.scan_status) {
				case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_TUNE_REQUEST          : {
					SiTRACE ("blindscan_interaction -- (tune  ) SCAN TUNE_REQUEST at %8ld kHz\n", scan_status.rf_freq);
					if (front_end->tuneUnitHz == 1) {
						seek_freq = si2168b_set_tuner_params(fe, scan_status.rf_freq*1000);
						seek_freq_kHz = seek_freq/1000;
					} else {
						seek_freq = si2168b_set_tuner_params(fe, scan_status.rf_freq);
						seek_freq_kHz = seek_freq;
					}
					channel_status->freq = seek_param->rangeMin = seek_freq;
					/* as we will not lock in less than min_lock_time_ms, wait a while... */
					msleep(min_lock_time_ms);
					break;
				}
				case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_DIGITAL_CHANNEL_FOUND : {
					channel_status->standard = scan_status.modulation;
					switch (scan_status.modulation) {
					case Si2168B_SCAN_STATUS_RESPONSE_MODULATION_DVBC : {
						channel_status->freq            = scan_status.rf_freq * 1000;
						channel_status->symbol_rate_bps = scan_status.symb_rate * 1000;
						si2168b_dvbc_status(front_end->demod, Si2168B_DVBC_STATUS_CMD_INTACK_OK, &dvbc_status);
						front_end->demod->dvbc_symbol_rate = scan_status.symb_rate;
						channel_status->constellation = dvbc_status.constellation;
						break;
					}
#ifdef __MCNS__
					case Si2168B_SCAN_STATUS_RESPONSE_MODULATION_MCNS : {
						*freq            = front_end->demod->rsp->scan_status.rf_freq * 1000;
						*symbol_rate_bps = front_end->demod->rsp->scan_status.symb_rate * 1000;
						si2168b_mcns_status(front_end->demod, Si2168B_MCNS_STATUS_CMD_INTACK_OK);
						front_end->demod->prop->mcns_symbol_rate.rate = front_end->demod->rsp->scan_status.symb_rate;
						*constellation   = front_end->demod->rsp->mcns_status.constellation;
						break;
					}
#endif /* __MCNS__ */
					default : {
						SiTRACE("si2168b_channel_seek_next DIGITAL_CHANNEL_FOUND error at %d: un-handled modulation (%d), aborting (skipped)\n", seek_freq, scan_status.modulation);
						front_end->seekAbort = 1;
						return 0;
					}
					}
					SiTRACE ("blindscan_interaction -- (locked) SCAN DIGITAL lock at %d MHz after %3d ms. modulation %3d (%s)\n", channel_status->freq/1000, searchDelay, channel_status->standard, si2168b_standard_name(channel_status->standard));
					front_end->handshakeOn = 0;
					return 1;
				}
				case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_ERROR: {
					SiTRACE ("blindscan_interaction -- (error2) SCAN error at %d after %4d ms\n", seek_freq/1000000, searchDelay);
					front_end->handshakeOn = 0;
					return 0;
				}
				case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_SEARCHING: {
					SiTRACE("SCAN Searching...\n");
					skip_resume = 1;
					break;
				}
				case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_ENDED: {
					SiTRACE ("blindscan_interaction -- (ended ) SCAN ENDED\n");
					si2168b_scan_ctrl (front_end->demod, Si2168B_SCAN_CTRL_CMD_ACTION_ABORT , 0);
					front_end->handshakeOn = 0;
					return 0;
				}
#ifdef ALLOW_Si2168B_BLINDSCAN_DEBUG
				case  Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_DEBUG                 : {
					SiTRACE ("blindscan_interaction -- (debug) SCAN DEBUG code %d\n", front_end->demod->rsp->scan_status.symb_rate);
					switch (front_end->demod->rsp->scan_status.symb_rate) {
					case 4: { /* SPECTRUM */
#ifndef DO_NOT_DRAW_SPECTRUM
						Si2168B_plot(front_end, "spectrum", 0, seek_freq);
#endif /* DO_NOT_DRAW_SPECTRUM */
						break;
					}
					case 9: { /* TRYLOCK */
#ifndef DO_NOT_TRACK_TRYLOCKS
						Si2168B_plot(front_end, "trylock", 0, seek_freq);
#endif /* DO_NOT_DRAW_SPECTRUM */
						break;
					}
					default: {}
					}
					/* There has been a debug request by the FW, refresh the timeoutStartTime */
					timeoutStartTime = system_time();
					break;
				}
#else /* ALLOW_Si2168B_BLINDSCAN_DEBUG */
				case  63                 : {
					SiTRACE("blindscan_interaction -- (warning) You probably run a DEBUG fw, so you need to define ALLOW_Si2168B_BLINDSCAN_DEBUG at project level\n");
					break;
				}
#endif /* ALLOW_Si2168B_BLINDSCAN_DEBUG */
				default : {
					SiTRACE("unknown scan_status %d\n", scan_status.scan_status);
					skip_resume = 1;
					break;
				}
				}

				if (skip_resume == 0) {
					SiTRACE ("blindscan_interaction >> (resume) si2168b_scan_ctrl( front_end->demod, %d, %8d)\n", front_end->demod->scan_ctrl_action, seek_freq_kHz);
					return_code = si2168b_scan_ctrl(front_end->demod, front_end->demod->scan_ctrl_action, seek_freq_kHz);
					if (return_code != NO_Si2168B_ERROR) {
						SiTRACE("si2168b_scan_ctrl ERROR at %d (%d)\n!!!!!!!!!!!!!!!!!!!!!!!\n", seek_freq_kHz, scan_status.scan_status);
						SiTRACE("si2168b_scan_ctrl 'RESUME' ERROR during seek loop\n");
					}
				}
			}

			/* timeout management (this should never happen if timeout values are correctly set) */
			timeoutDelay = system_time() - timeoutStartTime;
			if (system_time() - timeoutStartTime >= max_decision_time_ms) {
				SiTRACE ("Scan decision timeout (blind_mode = 1) from  %d after %d ms. Check your timeout limits!\n", seek_freq_kHz/1000, timeoutDelay);
				front_end->seekAbort   = 1;
				front_end->handshakeOn = 0;
				break;
			}

			if (front_end->handshakeUsed) {
				handshakeDelay = system_time() - seekStartTime;
				if (handshakeDelay >= front_end->handshakePeriod_ms) {
					SiTRACE ("blindscan_handshake : handshake after %5d ms (at %10d). (search delay %6d ms) %*s\n", handshakeDelay, seek_param->rangeMin, searchDelay, (searchDelay)/1000, "*");
					channel_status->freq = seek_freq;
					front_end->handshakeOn = 1;
					/* The application will check handshakeStart_ms to know whether the blindscan is ended or not */
					return searchDelay;
				} else {
					SiTRACE ("blindscan_handshake : no handshake yet. (handshake delay %6d ms, search delay %6d ms)\n", handshakeDelay, searchDelay);
				}
			}

			/* Check seekAbort flag (set in case of timeout or by the top-level application) */
			if (front_end->seekAbort) {
				/* Abort the SCAN loop to allow it to restart with the new rangeMin frequency */
				SiTRACE ("blindscan_interaction >> (abort!) si2168b_scan_ctrl(front_end->demod, Si2168B_SCAN_CTRL_CMD_ACTION_ABORT)\n");
				si2168b_scan_ctrl (front_end->demod, Si2168B_SCAN_CTRL_CMD_ACTION_ABORT , 0);
				front_end->handshakeOn = 0;
				return 0;
			}

			/* Check status every 100 ms */
			msleep(100);
		}
	}
	front_end->handshakeOn = 0;
	return 0;
}

/*---------------------------------------------------*/
/* Si2168B_POWER_DOWN COMMAND                       */
/*---------------------------------------------------*/
static u8 si2168b_power_down(si2168b_context *ctx)
{
	u8 cmdByteBuffer[1];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("si2168b_power_down\n");
	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_POWER_DOWN_CMD;

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 1, cmdByteBuffer) != 1) {
		SiTRACE("Error writing POWER_DOWN bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
	}

	_mutex_unlock(&ctx->lock);

	return ret;
}

/************************************************************************************************************************
  NAME: si2168b_standby
  DESCRIPTION:
  Parameter:  Pointer to Si2168B Context
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
************************************************************************************************************************/
static int si2168b_standby(si2168b_context *ctx)
{
	SiTRACE("si2168b_standby()\n");
	return si2168b_power_down(ctx);
}

/*---------------------------------------------------*/
/* Si2168B_PART_INFO COMMAND                        */
/*---------------------------------------------------*/
static u8 si2168b_part_info(si2168b_context *ctx, Si2168B_PART_INFO_CMD_REPLY_struct *part_info)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[1];
	u8 rspByteBuffer[13];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B PART_INFO\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_PART_INFO_CMD;

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 1, cmdByteBuffer) != 1) {
		SiTRACE("Error writing PART_INFO bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 13, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling PART_INFO response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	part_info->chiprev  =   (( ( (rspByteBuffer[1]  )) >> Si2168B_PART_INFO_RESPONSE_CHIPREV_LSB  ) & Si2168B_PART_INFO_RESPONSE_CHIPREV_MASK  );
	part_info->part     =   (( ( (rspByteBuffer[2]  )) >> Si2168B_PART_INFO_RESPONSE_PART_LSB     ) & Si2168B_PART_INFO_RESPONSE_PART_MASK     );
	part_info->pmajor   =   (( ( (rspByteBuffer[3]  )) >> Si2168B_PART_INFO_RESPONSE_PMAJOR_LSB   ) & Si2168B_PART_INFO_RESPONSE_PMAJOR_MASK   );
	part_info->pminor   =   (( ( (rspByteBuffer[4]  )) >> Si2168B_PART_INFO_RESPONSE_PMINOR_LSB   ) & Si2168B_PART_INFO_RESPONSE_PMINOR_MASK   );
	part_info->pbuild   =   (( ( (rspByteBuffer[5]  )) >> Si2168B_PART_INFO_RESPONSE_PBUILD_LSB   ) & Si2168B_PART_INFO_RESPONSE_PBUILD_MASK   );
	part_info->reserved =   (( ( (rspByteBuffer[6]  ) | (rspByteBuffer[7]  << 8 )) >> Si2168B_PART_INFO_RESPONSE_RESERVED_LSB ) & Si2168B_PART_INFO_RESPONSE_RESERVED_MASK );
	part_info->serial   =   (( ( (rspByteBuffer[8]  ) | (rspByteBuffer[9]  << 8 ) | (rspByteBuffer[10] << 16 ) | (rspByteBuffer[11] << 24 )) >> Si2168B_PART_INFO_RESPONSE_SERIAL_LSB   ) & Si2168B_PART_INFO_RESPONSE_SERIAL_MASK   );
	part_info->romid    =   (( ( (rspByteBuffer[12] )) >> Si2168B_PART_INFO_RESPONSE_ROMID_LSB    ) & Si2168B_PART_INFO_RESPONSE_ROMID_MASK    );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/***********************************************************************************************************************
  si2168b_patch function
  Use:        Patch information function
              Used to send a number of bytes to the Si2168B. Useful to download the firmware.
  Returns:    0 if no error
  Parameter:  error_code the error code.
  Porting:    Useful for application development for debug purposes.
  Porting:    May not be required for the final application, can be removed if not used.
 ***********************************************************************************************************************/
static u8 si2168b_patch(si2168b_context *ctx, int iNbBytes, u8 *pucDataBuffer)
{
	int res;
	u8 ret = NO_Si2168B_ERROR;
	u8 rspByteBuffer[1];

	SiTRACE("Si2168B Patch %d bytes\n",iNbBytes);

	_mutex_lock(&ctx->lock);

	res = i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, iNbBytes, pucDataBuffer);
	if (res!=iNbBytes) {
		SiTRACE("si2168b_patch error writing bytes: %s\n", si2168b_error_text(ERROR_Si2168B_LOADING_FIRMWARE) );
		ret = ERROR_Si2168B_LOADING_FIRMWARE;
		goto unlock_mutex;
	}

	res = si2168b_poll_for_response(ctx, 1, rspByteBuffer);
	if (res != NO_Si2168B_ERROR) {
		SiTRACE("si2168b_patch error 0x%02x polling response: %s\n", res, si2168b_error_text(res) );
		ret = ERROR_Si2168B_POLLING_RESPONSE;
	}

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/************************************************************************************************************************
  NAME: si2168b_load_firmware_16
  DESCRIPTON: Load firmware from firmware_struct array in Si2168B_Firmware_x_y_build_z.h file into Si2168B
              Requires Si2168B to be in bootloader mode after PowerUp

  Parameter:  Si2168B Context (I2C address)
  Parameter:  pointer to firmware_struct array
  Parameter:  number of lines in firmware table array (size in bytes / firmware_struct)
  Returns:    Si2168B/I2C transaction error code, NO_Si2168B_ERROR if successful
************************************************************************************************************************/
static int si2168b_load_firmware_16(si2168b_context *ctx, firmware_struct fw_table[], int nbLines)
{
	int return_code = NO_Si2168B_ERROR;
	int line;

	SiTRACE ("si2168b_load_firmware_16 starting...\n");
	SiTRACE ("si2168b_load_firmware_16 nbLines %d\n", nbLines);

	/* for each line in fw_table */
	for (line = 0; line < nbLines; line++) {
		if (fw_table[line].firmware_len > 0)  /* don't download if length is 0 , e.g. dummy firmware */
		{
			/* send firmware_len bytes (up to 16) to Si2168B */
			if ((return_code = si2168b_patch(ctx, fw_table[line].firmware_len, fw_table[line].firmware_table)) != NO_Si2168B_ERROR)
			{
				SiTRACE("si2168b_load_firmware_16 error 0x%02x patching line %d: %s\n", return_code, line, si2168b_error_text(return_code) );
				if (line == 0) {
					SiTRACE("The firmware is incompatible with the part!\n");
				}
				return ERROR_Si2168B_LOADING_FIRMWARE;
			}
			if (line==3) {
				sitraces_suspend();
			}
		}
	}
	sitraces_resume();
	SiTRACE ("si2168b_load_firmware_16 complete...\n");
	return NO_Si2168B_ERROR;
}

/*---------------------------------------------------*/
/* Si2168B_EXIT_BOOTLOADER COMMAND                  */
/*---------------------------------------------------*/
static u8 si2168b_exit_bootloader(si2168b_context *ctx,
		u8 func,
		u8 ctsien)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[1];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B EXIT_BOOTLOADER\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_EXIT_BOOTLOADER_CMD;
	cmdByteBuffer[1] = (u8) ( ( func   & Si2168B_EXIT_BOOTLOADER_CMD_FUNC_MASK   ) << Si2168B_EXIT_BOOTLOADER_CMD_FUNC_LSB  |
			( ctsien & Si2168B_EXIT_BOOTLOADER_CMD_CTSIEN_MASK ) << Si2168B_EXIT_BOOTLOADER_CMD_CTSIEN_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing EXIT_BOOTLOADER bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 1, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling EXIT_BOOTLOADER response\n");
		ret = error_code;
	}

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/************************************************************************************************************************
  NAME: si2168b_start_firmware
  DESCRIPTION: Start Si2168B firmware (put the Si2168B into run mode)
  Parameter:   Si2168B Context (I2C address)
  Parameter (passed by Reference):   ExitBootloadeer Response Status byte : tunint, atvint, dtvint, err, cts
  Returns:     I2C transaction error code, NO_Si2168B_ERROR if successful
 ************************************************************************************************************************/
static int si2168b_start_firmware(si2168b_context *ctx)
{
	int ret = NO_Si2168B_ERROR;

	ret = si2168b_exit_bootloader(ctx, Si2168B_EXIT_BOOTLOADER_CMD_FUNC_NORMAL, Si2168B_EXIT_BOOTLOADER_CMD_CTSIEN_OFF);
	if (ret != NO_Si2168B_ERROR) {
		return ERROR_Si2168B_STARTING_FIRMWARE;
	}

	return NO_Si2168B_ERROR;
}

/*---------------------------------------------------*/
/* Si2168B_GET_REV COMMAND                          */
/*---------------------------------------------------*/
static u8 si2168b_get_revision(si2168b_context *ctx, Si2168B_GET_REV_CMD_REPLY_struct *get_rev)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[1];
	u8 rspByteBuffer[10];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B GET_REV\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_GET_REV_CMD;

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 1, cmdByteBuffer) != 1) {
		SiTRACE("Error writing GET_REV bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 10, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling GET_REV response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	get_rev->pn       =   (( ( (rspByteBuffer[1]  )) >> Si2168B_GET_REV_RESPONSE_PN_LSB       ) & Si2168B_GET_REV_RESPONSE_PN_MASK       );
	get_rev->fwmajor  =   (( ( (rspByteBuffer[2]  )) >> Si2168B_GET_REV_RESPONSE_FWMAJOR_LSB  ) & Si2168B_GET_REV_RESPONSE_FWMAJOR_MASK  );
	get_rev->fwminor  =   (( ( (rspByteBuffer[3]  )) >> Si2168B_GET_REV_RESPONSE_FWMINOR_LSB  ) & Si2168B_GET_REV_RESPONSE_FWMINOR_MASK  );
	get_rev->patch    =   (( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 )) >> Si2168B_GET_REV_RESPONSE_PATCH_LSB    ) & Si2168B_GET_REV_RESPONSE_PATCH_MASK    );
	get_rev->cmpmajor =   (( ( (rspByteBuffer[6]  )) >> Si2168B_GET_REV_RESPONSE_CMPMAJOR_LSB ) & Si2168B_GET_REV_RESPONSE_CMPMAJOR_MASK );
	get_rev->cmpminor =   (( ( (rspByteBuffer[7]  )) >> Si2168B_GET_REV_RESPONSE_CMPMINOR_LSB ) & Si2168B_GET_REV_RESPONSE_CMPMINOR_MASK );
	get_rev->cmpbuild =   (( ( (rspByteBuffer[8]  )) >> Si2168B_GET_REV_RESPONSE_CMPBUILD_LSB ) & Si2168B_GET_REV_RESPONSE_CMPBUILD_MASK );
	get_rev->chiprev  =   (( ( (rspByteBuffer[9]  )) >> Si2168B_GET_REV_RESPONSE_CHIPREV_LSB  ) & Si2168B_GET_REV_RESPONSE_CHIPREV_MASK  );
	get_rev->mcm_die  =   (( ( (rspByteBuffer[9]  )) >> Si2168B_GET_REV_RESPONSE_MCM_DIE_LSB  ) & Si2168B_GET_REV_RESPONSE_MCM_DIE_MASK  );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/************************************************************************************************************************
  NAME: si2168b_power_up_with_patch
  DESCRIPTION: Send Si2168B API PowerUp Command with PowerUp to bootloader,
  Check the Chip rev and part, and ROMID are compared to expected values.
  Load the Firmware Patch then Start the Firmware.
  Programming Guide Reference:    Flowchart A.2 (POWER_UP with patch flowchart)

  Parameter:  pointer to Si2168B Context
  Returns:    Si2168B/I2C transaction error code, NO_Si2168B_ERROR if successful
************************************************************************************************************************/
static int si2168b_power_up_with_patch(si2168b_context *ctx)
{
	int return_code = NO_Si2168B_ERROR;
	int fw_loaded = 0;
	Si2168B_GET_REV_CMD_REPLY_struct get_rev;
	Si2168B_PART_INFO_CMD_REPLY_struct part_info = { 0 };

	/* Before patching, set POWER_UP values for 'RESET' and 'BOOTLOADER' */
	ctx->power_up_reset = Si2168B_POWER_UP_CMD_RESET_RESET;
	ctx->power_up_func  = Si2168B_POWER_UP_CMD_FUNC_BOOTLOADER,

	return_code = si2168b_wakeup(ctx);

	if (return_code != NO_Si2168B_ERROR) {
		SiTRACE("si2168b_power_up_with_patch: WAKEUP error!\n");
		return return_code;
	}

	ctx->Si2168B_in_standby = 0;

	/* Get the Part Info from the chip.   This command is only valid in Bootloader mode */
	if ((return_code = si2168b_part_info(ctx, &part_info)) != NO_Si2168B_ERROR) {
		SiTRACE ("si2168b_part_info error 0x%02x: %s\n", return_code, si2168b_error_text(return_code) );
		return return_code;
	}
	SiTRACE("part    Si21%02d", part_info.part   );
	if (part_info.chiprev == Si2168B_PART_INFO_RESPONSE_CHIPREV_A) {
		SiTRACE("A\n");
	} else if (part_info.chiprev == Si2168B_PART_INFO_RESPONSE_CHIPREV_B) {
		SiTRACE("B\n");
	} else {
		SiTRACE("\nchiprev %d\n", part_info.chiprev);
	}
	SiTRACE("romid   %d\n",     part_info.romid  );
	SiTRACE("pmajor  0x%02x\n", part_info.pmajor );
	SiTRACE("pminor  0x%02x\n", part_info.pminor );
	SiTRACE("pbuild  %d\n",     part_info.pbuild );
	if ((part_info.pmajor >= 0x30) & (part_info.pminor >= 0x30)) {
		SiTRACE("Full Info       'Si21%02d-%c%c%c ROM%x NVM%c_%cb%d'\n\n\n", part_info.part, part_info.chiprev + 0x40, part_info.pmajor, part_info.pminor, part_info.romid, part_info.pmajor, part_info.pminor, part_info.pbuild );
	}

	/* Check part info values and load the proper firmware */
#ifdef    Si2168B_A40_COMPATIBLE
#ifdef    Si2168B_PATCH16_4_4b7_LINES
	if (!fw_loaded) {
		SiTRACE  ("Is this part a  'Si21%2d_ROM%x_%c_%c_b%d'?\n", Si2168B_PATCH16_4_4b7_PART, Si2168B_PATCH16_4_4b7_ROM, Si2168B_PATCH16_4_4b7_PMAJOR, Si2168B_PATCH16_4_4b7_PMINOR, Si2168B_PATCH16_4_4b7_PBUILD );
		if ((ctx->rsp->part_info.romid  == Si2168B_PATCH16_4_4b7_ROM   )
				&(  (ctx->rsp->part_info.part == 64 )
						|| (ctx->rsp->part_info.part == 62 )
						|| (ctx->rsp->part_info.part == 60 )
				)
				& (ctx->rsp->part_info.pmajor == Si2168B_PATCH16_4_4b7_PMAJOR)
				& (ctx->rsp->part_info.pminor == Si2168B_PATCH16_4_4b7_PMINOR)
				& (ctx->rsp->part_info.pbuild == Si2168B_PATCH16_4_4b7_PBUILD)
		) {
			SiTRACE("Updating FW for 'Si21%2d NVM%c_%cb%d'\n", ctx->rsp->part_info.part, ctx->rsp->part_info.pmajor, ctx->rsp->part_info.pminor, ctx->rsp->part_info.pbuild );
#ifdef    Si2168B_PATCH16_4_4b7_INFOS
			SiTRACE(Si2168B_PATCH16_4_4b7_INFOS);
#endif /* Si2168B_PATCH16_4_4b7_INFOS */
			if ((return_code = si2168b_load_firmware_16(ctx, Si2168B_PATCH16_4_4b7, Si2168B_PATCH16_4_4b7_LINES)) != NO_Si2168B_ERROR) {
				SiTRACE ("si2168b_load_firmware_16 error 0x%02x: %s\n", return_code, si2168b_error_text(return_code) );
				return return_code;
			}
			fw_loaded++;
		}
	}
#endif /* Si2168B_PATCH16_4_4b7_LINES */
#ifdef    Si2168B_PATCH16_4_0b9_LINES
	if (!fw_loaded) {
		SiTRACE  ("Is this part a  'Si21%2d_ROM%x_%c_%c_b%d'?\n", Si2168B_PATCH16_4_0b9_PART, Si2168B_PATCH16_4_0b9_ROM, Si2168B_PATCH16_4_0b9_PMAJOR, Si2168B_PATCH16_4_0b9_PMINOR, Si2168B_PATCH16_4_0b9_PBUILD );
		if ((part_info.romid  == Si2168B_PATCH16_4_0b9_ROM   )
				&& (  (part_info.part == 69 )
						|| (part_info.part == 68 )
				)
				&& (part_info.pmajor == Si2168B_PATCH16_4_0b9_PMAJOR)
				&& (part_info.pminor == Si2168B_PATCH16_4_0b9_PMINOR)
				&& (part_info.pbuild == Si2168B_PATCH16_4_0b9_PBUILD)
		) {
			SiTRACE("Updating FW for 'Si21%2d NVM%c_%cb%d'\n", part_info.part, part_info.pmajor, part_info.pminor, part_info.pbuild);
#ifdef    Si2168B_PATCH16_4_0b9_INFOS
			SiTRACE(Si2168B_PATCH16_4_0b9_INFOS);
#endif /* Si2168B_PATCH16_4_0b9_INFOS */
			if ((return_code = si2168b_load_firmware_16(ctx, Si2168B_PATCH16_4_0b9, Si2168B_PATCH16_4_0b9_LINES)) != NO_Si2168B_ERROR) {
				SiTRACE ("si2168b_load_firmware_16 error 0x%02x: %s\n", return_code, si2168b_error_text(return_code) );
				return return_code;
			}
			fw_loaded++;
		}
	}
#endif /* Si2168B_PATCH16_4_0b9_LINES */
#endif /* Si2168B_A40_COMPATIBLE */
#ifdef    Si2168B_ES_COMPATIBLE
#ifdef    Si2168B_FIRMWARE_3_Ab12_LINES
	if (!fw_loaded) {
		SiTRACE  ("Is this part a  'Si21%2d_ROM%x_%c_%c_b%d'?\n", Si2168B_FIRMWARE_3_Ab12_PART, Si2168B_FIRMWARE_3_Ab12_ROM, Si2168B_FIRMWARE_3_Ab12_PMAJOR, Si2168B_FIRMWARE_3_Ab12_PMINOR, Si2168B_FIRMWARE_3_Ab12_PBUILD );
		if ((ctx->rsp->part_info.romid  == Si2168B_FIRMWARE_3_Ab12_ROM   )
				&(  (ctx->rsp->part_info.part == 69 )
						|| (ctx->rsp->part_info.part == 68 )
						|| (ctx->rsp->part_info.part == 64 )
						|| (ctx->rsp->part_info.part == 62 )
						|| (ctx->rsp->part_info.part == 60 )
						|| (ctx->rsp->part_info.part == 0  )
				)
				/*
        & (ctx->rsp->part_info.pmajor == Si2168B_FIRMWARE_3_Ab12_PMAJOR)
        & (ctx->rsp->part_info.pminor == Si2168B_FIRMWARE_3_Ab12_PMINOR)
        & (ctx->rsp->part_info.pbuild == Si2168B_FIRMWARE_3_Ab12_PBUILD)
				 */
		) {
			SiTRACE("Updating FW for 'Si21%2d NVM%c_%cb%d' (full download)\n", ctx->rsp->part_info.part, ctx->rsp->part_info.pmajor, ctx->rsp->part_info.pminor, ctx->rsp->part_info.pbuild );
#ifdef    Si2168B_FIRMWARE_3_Ab12_INFOS
			SiTRACE(Si2168B_FIRMWARE_3_Ab12_INFOS);
#endif /* Si2168B_FIRMWARE_3_Ab12_INFOS */
			if ((return_code = Si2168B_LoadFirmware(ctx, Si2168B_FIRMWARE_3_Ab12, Si2168B_FIRMWARE_3_Ab12_LINES)) != NO_Si2168B_ERROR) {
				SiTRACE ("Si2168B_LoadFirmware error 0x%02x: %s\n", return_code, si2168b_error_text(return_code) );
				return return_code;
			}
			fw_loaded++;
		}
	}
#endif /* Si2168B_FIRMWARE_3_Ab12_LINES */
#endif /* Si2168B_ES_COMPATIBLE */
#ifdef    Si2169_30_COMPATIBLE
#ifdef    Si2169_PATCH_3_0b18_LINES
	if (!fw_loaded) {
		SiTRACE  ("Is this part a  'Si21%2d_ROM%x_%c_%c_b%d'?\n", Si2169_PATCH_3_0b18_PART, Si2169_PATCH_3_0b18_ROM, Si2169_PATCH_3_0b18_PMAJOR, Si2169_PATCH_3_0b18_PMINOR, Si2169_PATCH_3_0b18_PBUILD );
		if ((ctx->rsp->part_info.romid  == Si2169_PATCH_3_0b18_ROM   )
				&((ctx->rsp->part_info.part   == 69 ) || (ctx->rsp->part_info.part == 68 ))
				& (ctx->rsp->part_info.pmajor == Si2169_PATCH_3_0b18_PMAJOR)
				& (ctx->rsp->part_info.pminor == Si2169_PATCH_3_0b18_PMINOR)
				& (ctx->rsp->part_info.pbuild == Si2169_PATCH_3_0b18_PBUILD)) {
			SiTRACE("Updating FW for 'Si21%2d_ROM%x %c_%c_b%d'\n", ctx->rsp->part_info.part, ctx->rsp->part_info.romid, ctx->rsp->part_info.pmajor, ctx->rsp->part_info.pminor, ctx->rsp->part_info.pbuild );
#ifdef    Si2169_PATCH_3_0b18_INFOS
			SiTRACE(Si2169_PATCH_3_0b18_INFOS);
#endif /* Si2169_PATCH_3_0b18_INFOS */
			if ((return_code = Si2168B_LoadFirmware(ctx, Si2169_PATCH_3_0b18, Si2169_PATCH_3_0b18_LINES)) != NO_Si2168B_ERROR) {
				SiTRACE ("Si2169_LoadFirmware error 0x%02x: %s\n", return_code, si2168b_error_text(return_code) );
				return return_code;
			}
			fw_loaded++;
		}
	}
#endif /* Si2169_PATCH_3_0b18_LINES */
#endif /* Si2169_0_COMPATIBLE */
#ifdef    Si2167B_20_COMPATIBLE
#ifdef    Si2167B_PATCH_2_0b5_LINES
	if (!fw_loaded) {
		SiTRACE  ("Is this part a  'Si21%2d_ROM%x_%c_%c_b%d'?\n", Si2167B_PATCH_2_0b5_PART, Si2167B_PATCH_2_0b5_ROM, Si2167B_PATCH_2_0b5_PMAJOR, Si2167B_PATCH_2_0b5_PMINOR, Si2167B_PATCH_2_0b5_PBUILD );
		if ((ctx->rsp->part_info.romid  == Si2167B_PATCH_2_0b5_ROM   )
				&((ctx->rsp->part_info.part   == 67 ) || (ctx->rsp->part_info.part == 66 ))
				& (ctx->rsp->part_info.pmajor == Si2167B_PATCH_2_0b5_PMAJOR)
				& (ctx->rsp->part_info.pminor == Si2167B_PATCH_2_0b5_PMINOR)
				& (ctx->rsp->part_info.pbuild == Si2167B_PATCH_2_0b5_PBUILD)) {
			SiTRACE("Updating FW for 'Si21%2d_FW_%c_%c_b%d'\n", ctx->rsp->part_info.part, ctx->rsp->part_info.pmajor, ctx->rsp->part_info.pminor, ctx->rsp->part_info.pbuild );
#ifdef    Si2167B_PATCH_2_0b5_INFOS
			SiTRACE("%s\n", Si2167B_PATCH_2_0b5_INFOS);
#endif /* Si2167B_PATCH_2_0b5_INFOS */
			if ((return_code = Si2167B_LoadFirmware(ctx, Si2167B_PATCH_2_0b5, Si2167B_PATCH_2_0b5_LINES)) != NO_Si2167B_ERROR) {
				SiTRACE ("Si2167B_LoadPatch error 0x%02x: %s\n", return_code, Si2167B_L1_API_ERROR_TEXT(return_code) );
				return return_code;
			}
			fw_loaded++;
		}
	}
#endif /* Si2167B_PATCH_2_0b5_LINES */
#ifdef    Si2167B_20_PATCH_CUSTOMER_LINES
	if (!fw_loaded) {
		SiTRACE  ("Is this part a  'Si21%2d_ROM%x_%c_%c_b%d'?\n", Si2167B_20_PATCH_CUSTOMER_PART, Si2167B_20_PATCH_CUSTOMER_ROM, Si2167B_20_PATCH_CUSTOMER_PMAJOR, Si2167B_20_PATCH_CUSTOMER_PMINOR, Si2167B_20_PATCH_CUSTOMER_PBUILD );
		if ((ctx->rsp->part_info.romid  == Si2167B_20_PATCH_CUSTOMER_ROM   )
				&((ctx->rsp->part_info.part   == 67 ) || (ctx->rsp->part_info.part == 66 ))
				& (ctx->rsp->part_info.pmajor == Si2167B_20_PATCH_CUSTOMER_PMAJOR)
				& (ctx->rsp->part_info.pminor == Si2167B_20_PATCH_CUSTOMER_PMINOR)
				& (ctx->rsp->part_info.pbuild == Si2167B_20_PATCH_CUSTOMER_PBUILD)) {
			SiTRACE("Updating FW for 'Si21%2d_ROM%x %c_%c_b%d'\n", ctx->rsp->part_info.part, ctx->rsp->part_info.romid, ctx->rsp->part_info.pmajor, ctx->rsp->part_info.pminor, ctx->rsp->part_info.pbuild );
#ifdef    Si2167B_20_PATCH_CUSTOMER_INFOS
			SiTRACE(Si2167B_20_PATCH_CUSTOMER_INFOS);
#endif /* Si2167B_20_PATCH_CUSTOMER_INFOS */
			if ((return_code = Si2167B_LoadFirmware(ctx, Si2167B_20_PATCH_CUSTOMER, Si2167B_20_PATCH_CUSTOMER_LINES)) != NO_Si2167B_ERROR) {
				SiTRACE ("Si2167B_LoadFirmware error 0x%02x: %s\n", return_code, Si2167B_L1_API_ERROR_TEXT(return_code) );
				return return_code;
			}
			fw_loaded++;
		}
	}
#endif /* Si2167B_20_PATCH_CUSTOMER_LINES */
#ifdef    Si2167B_PATCH_2_0b21_LINES
	if (!fw_loaded) {
		SiTRACE  ("Is this part a  'Si21%2d_ROM%x_%c_%c_b%d'?\n", Si2167B_PATCH_2_0b21_PART, Si2167B_PATCH_2_0b21_ROM, Si2167B_PATCH_2_0b21_PMAJOR, Si2167B_PATCH_2_0b21_PMINOR, Si2167B_PATCH_2_0b21_PBUILD );
		if ((ctx->rsp->part_info.romid  == Si2167B_PATCH_2_0b21_ROM   )
				&((ctx->rsp->part_info.part   == 67 ) || (ctx->rsp->part_info.part == 66 ))
				& (ctx->rsp->part_info.pmajor == Si2167B_PATCH_2_0b21_PMAJOR)
				& (ctx->rsp->part_info.pminor == Si2167B_PATCH_2_0b21_PMINOR)
				& (ctx->rsp->part_info.pbuild == Si2167B_PATCH_2_0b21_PBUILD)) {
			SiTRACE("Updating FW for 'Si21%2d_FW_%c_%c_b%d'\n", ctx->rsp->part_info.part, ctx->rsp->part_info.pmajor, ctx->rsp->part_info.pminor, ctx->rsp->part_info.pbuild );
#ifdef    Si2167B_PATCH_2_0b21_INFOS
			SiTRACE("%s\n", Si2167B_PATCH_2_0b21_INFOS);
#endif /* Si2167B_PATCH_2_0b21_INFOS */
			if ((return_code = Si2168B_LoadFirmware(ctx, Si2167B_PATCH_2_0b21, Si2167B_PATCH_2_0b21_LINES)) != NO_Si2168B_ERROR) {
				SiTRACE ("Si2167B_LoadPatch error 0x%02x: %s\n", return_code, si2168b_error_text(return_code) );
				return return_code;
			}
			fw_loaded++;
		}
	}
#endif /* Si2167B_PATCH_2_0b21_LINES */
#endif /* Si2167B_20_COMPATIBLE */

	if (!fw_loaded) {
		SiTRACE ("Si2168B_LoadFirmware error: NO Firmware Loaded! Possible part/code incompatibility !\n");
		return ERROR_Si2168B_LOADING_FIRMWARE;
	}

	/*Start the Firmware */
	return_code = si2168b_start_firmware(ctx);
	if (return_code != NO_Si2168B_ERROR) {
		/* Start firmware */
		SiTRACE ("si2168b_start_firmware error 0x%02x: %s\n", return_code, si2168b_error_text(return_code) );
		return return_code;
	}

	return_code = si2168b_get_revision(ctx, &get_rev);
	if (return_code == NO_Si2168B_ERROR) {
		if ((get_rev.mcm_die) != Si2168B_GET_REV_RESPONSE_MCM_DIE_SINGLE) {
			SiTRACE("Si21%2d%d-%c%c%c Die %c Part running 'FW_%c_%cb%d'\n", part_info.part
					, 2
					, part_info.chiprev + 0x40
					, part_info.pmajor
					, part_info.pminor
					, get_rev.mcm_die   + 0x40
					, get_rev.cmpmajor
					, get_rev.cmpminor
					, get_rev.cmpbuild );
		} else {
			SiTRACE("Si21%2d-%c%c%c Part running 'FW_%c_%cb%d'\n", part_info.part
					, part_info.chiprev + 0x40
					, part_info.pmajor
					, part_info.pminor
					, get_rev.cmpmajor
					, get_rev.cmpminor
					, get_rev.cmpbuild );
		}
	}

	return NO_Si2168B_ERROR;
}

/*---------------------------------------------------*/
/* Si2168B_CONFIG_PINS COMMAND                      */
/*---------------------------------------------------*/
static u8 si2168b_config_pins(si2168b_context *ctx,
		u8   gpio0_mode,
		u8   gpio0_read,
		u8   gpio1_mode,
		u8   gpio1_read,
		Si2168B_CONFIG_PINS_CMD_REPLY_struct *config_pins)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[3];
	u8 rspByteBuffer[3];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B CONFIG_PINS\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_CONFIG_PINS_CMD;
	cmdByteBuffer[1] = (u8) ( ( gpio0_mode & Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_MASK ) << Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_LSB|
			( gpio0_read & Si2168B_CONFIG_PINS_CMD_GPIO0_READ_MASK ) << Si2168B_CONFIG_PINS_CMD_GPIO0_READ_LSB);
	cmdByteBuffer[2] = (u8) ( ( gpio1_mode & Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_MASK ) << Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_LSB|
			( gpio1_read & Si2168B_CONFIG_PINS_CMD_GPIO1_READ_MASK ) << Si2168B_CONFIG_PINS_CMD_GPIO1_READ_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 3, cmdByteBuffer) != 3) {
		SiTRACE("Error writing CONFIG_PINS bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 3, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling CONFIG_PINS response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	config_pins->gpio0_mode  = (( ( (rspByteBuffer[1]  )) >> Si2168B_CONFIG_PINS_RESPONSE_GPIO0_MODE_LSB  ) & Si2168B_CONFIG_PINS_RESPONSE_GPIO0_MODE_MASK  );
	config_pins->gpio0_state = (( ( (rspByteBuffer[1]  )) >> Si2168B_CONFIG_PINS_RESPONSE_GPIO0_STATE_LSB ) & Si2168B_CONFIG_PINS_RESPONSE_GPIO0_STATE_MASK );
	config_pins->gpio1_mode  = (( ( (rspByteBuffer[2]  )) >> Si2168B_CONFIG_PINS_RESPONSE_GPIO1_MODE_LSB  ) & Si2168B_CONFIG_PINS_RESPONSE_GPIO1_MODE_MASK  );
	config_pins->gpio1_state = (( ( (rspByteBuffer[2]  )) >> Si2168B_CONFIG_PINS_RESPONSE_GPIO1_STATE_LSB ) & Si2168B_CONFIG_PINS_RESPONSE_GPIO1_STATE_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_DVBT2_TX_ID COMMAND                      */
/*---------------------------------------------------*/
static u8 si2168b_dvbt2_tx_id(si2168b_context *ctx)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[1];
	u8 rspByteBuffer[8];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	dvbt2_tx_id->STATUS = ctx->status;
#endif

	SiTRACE("Si2168B DVBT2_TX_ID\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DVBT2_TX_ID_CMD;

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 1, cmdByteBuffer) != 1) {
		SiTRACE("Error writing DVBT2_TX_ID bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 8, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DVBT2_TX_ID response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->dvbt2_tx_id.tx_id_availability =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT2_TX_ID_RESPONSE_TX_ID_AVAILABILITY_LSB ) & Si2168B_DVBT2_TX_ID_RESPONSE_TX_ID_AVAILABILITY_MASK );
	ctx->rsp->dvbt2_tx_id.cell_id            =   (( ( (rspByteBuffer[2]  ) | (rspByteBuffer[3]  << 8 )) >> Si2168B_DVBT2_TX_ID_RESPONSE_CELL_ID_LSB            ) & Si2168B_DVBT2_TX_ID_RESPONSE_CELL_ID_MASK            );
	ctx->rsp->dvbt2_tx_id.network_id         =   (( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 )) >> Si2168B_DVBT2_TX_ID_RESPONSE_NETWORK_ID_LSB         ) & Si2168B_DVBT2_TX_ID_RESPONSE_NETWORK_ID_MASK         );
	ctx->rsp->dvbt2_tx_id.t2_system_id       =   (( ( (rspByteBuffer[6]  ) | (rspByteBuffer[7]  << 8 )) >> Si2168B_DVBT2_TX_ID_RESPONSE_T2_SYSTEM_ID_LSB       ) & Si2168B_DVBT2_TX_ID_RESPONSE_T2_SYSTEM_ID_MASK       );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_SPI_PASSTHROUGH COMMAND                  */
/*---------------------------------------------------*/
static u8 si2168b_spi_passthrough(si2168b_context *ctx,
		u8   subcode,
		u8   spi_passthr_clk,
		u8   spi_passth_data)
{
	u8 cmdByteBuffer[4];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->spi_passthrough.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B SPI_PASSTHROUGH\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_SPI_PASSTHROUGH_CMD;
	cmdByteBuffer[1] = (u8) ( ( subcode         & Si2168B_SPI_PASSTHROUGH_CMD_SUBCODE_MASK         ) << Si2168B_SPI_PASSTHROUGH_CMD_SUBCODE_LSB        );
	cmdByteBuffer[2] = (u8) ( ( spi_passthr_clk & Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_MASK ) << Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_LSB);
	cmdByteBuffer[3] = (u8) ( ( spi_passth_data & Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_MASK ) << Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 4, cmdByteBuffer) != 4) {
		SiTRACE("Error writing SPI_PASSTHROUGH bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
	}

	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_DOWNLOAD_DATASET_CONTINUE COMMAND        */
/*---------------------------------------------------*/
static u8 si2168b_download_dataset_continue(si2168b_context *ctx,
		u8 data0,
		u8 data1,
		u8 data2,
		u8 data3,
		u8 data4,
		u8 data5,
		u8 data6)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[8];
	u8 rspByteBuffer[1];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->download_dataset_continue.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B DOWNLOAD_DATASET_CONTINUE\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD;
	cmdByteBuffer[1] = (u8) ( ( data0 & Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA0_MASK ) << Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA0_LSB);
	cmdByteBuffer[2] = (u8) ( ( data1 & Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA1_MASK ) << Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA1_LSB);
	cmdByteBuffer[3] = (u8) ( ( data2 & Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA2_MASK ) << Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA2_LSB);
	cmdByteBuffer[4] = (u8) ( ( data3 & Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA3_MASK ) << Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA3_LSB);
	cmdByteBuffer[5] = (u8) ( ( data4 & Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA4_MASK ) << Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA4_LSB);
	cmdByteBuffer[6] = (u8) ( ( data5 & Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA5_MASK ) << Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA5_LSB);
	cmdByteBuffer[7] = (u8) ( ( data6 & Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA6_MASK ) << Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA6_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 8, cmdByteBuffer) != 8) {
		SiTRACE("Error writing DOWNLOAD_DATASET_CONTINUE bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 1, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DOWNLOAD_DATASET_CONTINUE response\n");
		ret = error_code;
	}

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

/*---------------------------------------------------*/
/* Si2168B_DD_EXT_AGC_TER COMMAND                   */
/*---------------------------------------------------*/
static u8 si2168b_dd_ext_agc_ter(si2168b_context *ctx,
		u8   agc_1_mode,
		u8   agc_1_inv,
		u8   agc_2_mode,
		u8   agc_2_inv,
		u8   agc_1_kloop,
		u8   agc_2_kloop,
		u8   agc_1_min,
		u8   agc_2_min,
		Si2168B_DD_EXT_AGC_TER_CMD_REPLY_struct *dd_ext_agc_ter)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[6];
	u8 rspByteBuffer[3];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DD_EXT_AGC_TER\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_EXT_AGC_TER_CMD;
	cmdByteBuffer[1] = (u8) ( ( agc_1_mode  & Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MASK  ) << Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_LSB |
			( agc_1_inv   & Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_INV_MASK   ) << Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_INV_LSB  |
			( agc_2_mode  & Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_MASK  ) << Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_LSB |
			( agc_2_inv   & Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_INV_MASK   ) << Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_INV_LSB  );
	cmdByteBuffer[2] = (u8) ( ( agc_1_kloop & Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_KLOOP_MASK ) << Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_KLOOP_LSB);
	cmdByteBuffer[3] = (u8) ( ( agc_2_kloop & Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_KLOOP_MASK ) << Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_KLOOP_LSB);
	cmdByteBuffer[4] = (u8) ( ( agc_1_min   & Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MIN_MASK   ) << Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MIN_LSB  );
	cmdByteBuffer[5] = (u8) ( ( agc_2_min   & Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MIN_MASK   ) << Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MIN_LSB  );

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 6, cmdByteBuffer) != 6) {
		SiTRACE("Error writing DD_EXT_AGC_TER bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 3, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_EXT_AGC_TER response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	dd_ext_agc_ter->agc_1_level = (( ( (rspByteBuffer[1]  )) >> Si2168B_DD_EXT_AGC_TER_RESPONSE_AGC_1_LEVEL_LSB ) & Si2168B_DD_EXT_AGC_TER_RESPONSE_AGC_1_LEVEL_MASK );
	dd_ext_agc_ter->agc_2_level = (( ( (rspByteBuffer[2]  )) >> Si2168B_DD_EXT_AGC_TER_RESPONSE_AGC_2_LEVEL_LSB ) & Si2168B_DD_EXT_AGC_TER_RESPONSE_AGC_2_LEVEL_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_GET_PROPERTY COMMAND                     */
/*---------------------------------------------------*/
static u8 si2168b_get_property(si2168b_context *ctx,
		u8   reserved,
		unsigned int    prop)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[4];
	u8 rspByteBuffer[4];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->get_property.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B GET_PROPERTY\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_GET_PROPERTY_CMD;
	cmdByteBuffer[1] = (u8) ( ( reserved & Si2168B_GET_PROPERTY_CMD_RESERVED_MASK ) << Si2168B_GET_PROPERTY_CMD_RESERVED_LSB);
	cmdByteBuffer[2] = (u8) ( ( prop     & Si2168B_GET_PROPERTY_CMD_PROP_MASK     ) << Si2168B_GET_PROPERTY_CMD_PROP_LSB    );
	cmdByteBuffer[3] = (u8) ((( prop     & Si2168B_GET_PROPERTY_CMD_PROP_MASK     ) << Si2168B_GET_PROPERTY_CMD_PROP_LSB    )>>8);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 4, cmdByteBuffer) != 4) {
		SiTRACE("Error writing GET_PROPERTY bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 4, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling GET_PROPERTY response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->get_property.reserved =   (( ( (rspByteBuffer[1]  )) >> Si2168B_GET_PROPERTY_RESPONSE_RESERVED_LSB ) & Si2168B_GET_PROPERTY_RESPONSE_RESERVED_MASK );
	ctx->rsp->get_property.data     =   (( ( (rspByteBuffer[2]  ) | (rspByteBuffer[3]  << 8 )) >> Si2168B_GET_PROPERTY_RESPONSE_DATA_LSB     ) & Si2168B_GET_PROPERTY_RESPONSE_DATA_MASK     );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

/*---------------------------------------------------*/
/* Si2168B_DD_MP_DEFAULTS COMMAND                   */
/*---------------------------------------------------*/
static u8 si2168b_dd_mp_defaults(si2168b_context *ctx,
		u8 mp_a_mode,
		u8 mp_b_mode,
		u8 mp_c_mode,
		u8 mp_d_mode,
		Si2168B_DD_MP_DEFAULTS_CMD_REPLY_struct *dd_mp_defaults)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[5];
	u8 rspByteBuffer[5];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DD_MP_DEFAULTS\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_MP_DEFAULTS_CMD;
	cmdByteBuffer[1] = (u8) ( ( mp_a_mode & Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_MASK ) << Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_LSB);
	cmdByteBuffer[2] = (u8) ( ( mp_b_mode & Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_MASK ) << Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_LSB);
	cmdByteBuffer[3] = (u8) ( ( mp_c_mode & Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_MASK ) << Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_LSB);
	cmdByteBuffer[4] = (u8) ( ( mp_d_mode & Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_MASK ) << Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 5, cmdByteBuffer) != 5) {
		SiTRACE("Error writing DD_MP_DEFAULTS bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 5, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_MP_DEFAULTS response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	dd_mp_defaults->mp_a_mode = (( ( (rspByteBuffer[1]  )) >> Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_LSB ) & Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_MASK );
	dd_mp_defaults->mp_b_mode = (( ( (rspByteBuffer[2]  )) >> Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_LSB ) & Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_MASK );
	dd_mp_defaults->mp_c_mode = (( ( (rspByteBuffer[3]  )) >> Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_LSB ) & Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_MASK );
	dd_mp_defaults->mp_d_mode = (( ( (rspByteBuffer[4]  )) >> Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_LSB ) & Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_DD_FER COMMAND                           */
/*---------------------------------------------------*/
static u8 Si2168B_L1_DD_FER(si2168b_context *ctx, u8 rst)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[3];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->dd_fer.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B DD_FER\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_FER_CMD;
	cmdByteBuffer[1] = (u8) ( ( rst & Si2168B_DD_FER_CMD_RST_MASK ) << Si2168B_DD_FER_CMD_RST_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DD_FER bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 3, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_FER response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->dd_fer.exp  =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DD_FER_RESPONSE_EXP_LSB  ) & Si2168B_DD_FER_RESPONSE_EXP_MASK  );
	ctx->rsp->dd_fer.mant =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DD_FER_RESPONSE_MANT_LSB ) & Si2168B_DD_FER_RESPONSE_MANT_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_SPI_LINK COMMAND                         */
/*---------------------------------------------------*/
static u8 Si2168B_L1_SPI_LINK(si2168b_context *ctx,
		u8   subcode,
		u8   spi_pbl_key,
		u8   spi_pbl_num,
		u8   spi_conf_clk,
		u8   spi_clk_pola,
		u8   spi_conf_data,
		u8   spi_data_dir,
		u8   spi_enable)
{
	u8 cmdByteBuffer[7];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->spi_link.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B SPI_LINK\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_SPI_LINK_CMD;
	cmdByteBuffer[1] = (u8) ( ( subcode       & Si2168B_SPI_LINK_CMD_SUBCODE_MASK       ) << Si2168B_SPI_LINK_CMD_SUBCODE_LSB      );
	cmdByteBuffer[2] = (u8) ( ( spi_pbl_key   & Si2168B_SPI_LINK_CMD_SPI_PBL_KEY_MASK   ) << Si2168B_SPI_LINK_CMD_SPI_PBL_KEY_LSB  );
	cmdByteBuffer[3] = (u8) ( ( spi_pbl_num   & Si2168B_SPI_LINK_CMD_SPI_PBL_NUM_MASK   ) << Si2168B_SPI_LINK_CMD_SPI_PBL_NUM_LSB  );
	cmdByteBuffer[4] = (u8) ( ( spi_conf_clk  & Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_MASK  ) << Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_LSB |
			( spi_clk_pola  & Si2168B_SPI_LINK_CMD_SPI_CLK_POLA_MASK  ) << Si2168B_SPI_LINK_CMD_SPI_CLK_POLA_LSB );
	cmdByteBuffer[5] = (u8) ( ( spi_conf_data & Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_MASK ) << Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_LSB|
			( spi_data_dir  & Si2168B_SPI_LINK_CMD_SPI_DATA_DIR_MASK  ) << Si2168B_SPI_LINK_CMD_SPI_DATA_DIR_LSB );
	cmdByteBuffer[6] = (u8) ( ( spi_enable    & Si2168B_SPI_LINK_CMD_SPI_ENABLE_MASK    ) << Si2168B_SPI_LINK_CMD_SPI_ENABLE_LSB   );

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 7, cmdByteBuffer) != 7) {
		SiTRACE("Error writing SPI_LINK bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
	}

	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_DOWNLOAD_DATASET_START COMMAND           */
/*---------------------------------------------------*/
static u8 Si2168B_L1_DOWNLOAD_DATASET_START(si2168b_context *ctx,
		u8   dataset_id,
		u8   dataset_checksum,
		u8   data0,
		u8   data1,
		u8   data2,
		u8   data3,
		u8   data4)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[8];
	u8 rspByteBuffer[1];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->download_dataset_start.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B DOWNLOAD_DATASET_START\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DOWNLOAD_DATASET_START_CMD;
	cmdByteBuffer[1] = (u8) ( ( dataset_id       & Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_ID_MASK       ) << Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_ID_LSB      );
	cmdByteBuffer[2] = (u8) ( ( dataset_checksum & Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_CHECKSUM_MASK ) << Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_CHECKSUM_LSB);
	cmdByteBuffer[3] = (u8) ( ( data0            & Si2168B_DOWNLOAD_DATASET_START_CMD_DATA0_MASK            ) << Si2168B_DOWNLOAD_DATASET_START_CMD_DATA0_LSB           );
	cmdByteBuffer[4] = (u8) ( ( data1            & Si2168B_DOWNLOAD_DATASET_START_CMD_DATA1_MASK            ) << Si2168B_DOWNLOAD_DATASET_START_CMD_DATA1_LSB           );
	cmdByteBuffer[5] = (u8) ( ( data2            & Si2168B_DOWNLOAD_DATASET_START_CMD_DATA2_MASK            ) << Si2168B_DOWNLOAD_DATASET_START_CMD_DATA2_LSB           );
	cmdByteBuffer[6] = (u8) ( ( data3            & Si2168B_DOWNLOAD_DATASET_START_CMD_DATA3_MASK            ) << Si2168B_DOWNLOAD_DATASET_START_CMD_DATA3_LSB           );
	cmdByteBuffer[7] = (u8) ( ( data4            & Si2168B_DOWNLOAD_DATASET_START_CMD_DATA4_MASK            ) << Si2168B_DOWNLOAD_DATASET_START_CMD_DATA4_LSB           );

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 8, cmdByteBuffer) != 8) {
		SiTRACE("Error writing DOWNLOAD_DATASET_START bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 1, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DOWNLOAD_DATASET_START response\n");
		ret = error_code;
	}

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_RSSI_ADC COMMAND                         */
/*---------------------------------------------------*/
static u8 Si2168B_L1_RSSI_ADC(si2168b_context *ctx, u8 on_off)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[2];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->rssi_adc.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B RSSI_ADC\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_RSSI_ADC_CMD;
	cmdByteBuffer[1] = (u8) ( ( on_off & Si2168B_RSSI_ADC_CMD_ON_OFF_MASK ) << Si2168B_RSSI_ADC_CMD_ON_OFF_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing RSSI_ADC bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 2, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling RSSI_ADC response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->rssi_adc.level = (( ( (rspByteBuffer[1]  )) >> Si2168B_RSSI_ADC_RESPONSE_LEVEL_LSB ) & Si2168B_RSSI_ADC_RESPONSE_LEVEL_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_DD_SET_REG COMMAND                       */
/*---------------------------------------------------*/
static u8 Si2168B_L1_DD_SET_REG(si2168b_context *ctx,
		u8   reg_code_lsb,
		u8   reg_code_mid,
		u8   reg_code_msb,
		unsigned long   value)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[8];
	u8 rspByteBuffer[1];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->dd_set_reg.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B DD_SET_REG\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_SET_REG_CMD;
	cmdByteBuffer[1] = (u8) ( ( reg_code_lsb & Si2168B_DD_SET_REG_CMD_REG_CODE_LSB_MASK ) << Si2168B_DD_SET_REG_CMD_REG_CODE_LSB_LSB);
	cmdByteBuffer[2] = (u8) ( ( reg_code_mid & Si2168B_DD_SET_REG_CMD_REG_CODE_MID_MASK ) << Si2168B_DD_SET_REG_CMD_REG_CODE_MID_LSB);
	cmdByteBuffer[3] = (u8) ( ( reg_code_msb & Si2168B_DD_SET_REG_CMD_REG_CODE_MSB_MASK ) << Si2168B_DD_SET_REG_CMD_REG_CODE_MSB_LSB);
	cmdByteBuffer[4] = (u8) ( ( value        & Si2168B_DD_SET_REG_CMD_VALUE_MASK        ) << Si2168B_DD_SET_REG_CMD_VALUE_LSB       );
	cmdByteBuffer[5] = (u8) ((( value        & Si2168B_DD_SET_REG_CMD_VALUE_MASK        ) << Si2168B_DD_SET_REG_CMD_VALUE_LSB       )>>8);
	cmdByteBuffer[6] = (u8) ((( value        & Si2168B_DD_SET_REG_CMD_VALUE_MASK        ) << Si2168B_DD_SET_REG_CMD_VALUE_LSB       )>>16);
	cmdByteBuffer[7] = (u8) ((( value        & Si2168B_DD_SET_REG_CMD_VALUE_MASK        ) << Si2168B_DD_SET_REG_CMD_VALUE_LSB       )>>24);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 8, cmdByteBuffer) != 8) {
		SiTRACE("Error writing DD_SET_REG bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 1, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_SET_REG response\n");
		ret = error_code;
	}

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_DD_GET_REG COMMAND                       */
/*---------------------------------------------------*/
static u8 Si2168B_L1_DD_GET_REG(si2168b_context *ctx,
		u8   reg_code_lsb,
		u8   reg_code_mid,
		u8   reg_code_msb)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[4];
	u8 rspByteBuffer[5];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->dd_get_reg.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B DD_GET_REG\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_GET_REG_CMD;
	cmdByteBuffer[1] = (u8) ( ( reg_code_lsb & Si2168B_DD_GET_REG_CMD_REG_CODE_LSB_MASK ) << Si2168B_DD_GET_REG_CMD_REG_CODE_LSB_LSB);
	cmdByteBuffer[2] = (u8) ( ( reg_code_mid & Si2168B_DD_GET_REG_CMD_REG_CODE_MID_MASK ) << Si2168B_DD_GET_REG_CMD_REG_CODE_MID_LSB);
	cmdByteBuffer[3] = (u8) ( ( reg_code_msb & Si2168B_DD_GET_REG_CMD_REG_CODE_MSB_MASK ) << Si2168B_DD_GET_REG_CMD_REG_CODE_MSB_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 4, cmdByteBuffer) != 4) {
		SiTRACE("Error writing DD_GET_REG bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 5, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_GET_REG response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->dd_get_reg.data1 =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DD_GET_REG_RESPONSE_DATA1_LSB ) & Si2168B_DD_GET_REG_RESPONSE_DATA1_MASK );
	ctx->rsp->dd_get_reg.data2 =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DD_GET_REG_RESPONSE_DATA2_LSB ) & Si2168B_DD_GET_REG_RESPONSE_DATA2_MASK );
	ctx->rsp->dd_get_reg.data3 =   (( ( (rspByteBuffer[3]  )) >> Si2168B_DD_GET_REG_RESPONSE_DATA3_LSB ) & Si2168B_DD_GET_REG_RESPONSE_DATA3_MASK );
	ctx->rsp->dd_get_reg.data4 =   (( ( (rspByteBuffer[4]  )) >> Si2168B_DD_GET_REG_RESPONSE_DATA4_LSB ) & Si2168B_DD_GET_REG_RESPONSE_DATA4_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

/*---------------------------------------------------*/
/* Si2168B_DVBT2_PLP_INFO COMMAND                   */
/*---------------------------------------------------*/
static u8 si2168b_dvbt2_plp_info(si2168b_context *ctx, u8 plp_index, Si2168B_DVBT2_PLP_INFO_CMD_REPLY_struct *dvbt2_plp_info)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[13];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DVBT2_PLP_INFO\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DVBT2_PLP_INFO_CMD;
	cmdByteBuffer[1] = (u8) ( ( plp_index & Si2168B_DVBT2_PLP_INFO_CMD_PLP_INDEX_MASK ) << Si2168B_DVBT2_PLP_INFO_CMD_PLP_INDEX_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DVBT2_PLP_INFO bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 13, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DVBT2_PLP_INFO response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	dvbt2_plp_info->plp_id                 =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_ID_LSB                 ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_ID_MASK                 );
	dvbt2_plp_info->plp_payload_type       =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_PAYLOAD_TYPE_LSB       ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_PAYLOAD_TYPE_MASK       );
	dvbt2_plp_info->plp_type               =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_TYPE_LSB               ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_TYPE_MASK               );
	dvbt2_plp_info->first_frame_idx_msb    =   (( ( (rspByteBuffer[3]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_FRAME_IDX_MSB_LSB    ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_FRAME_IDX_MSB_MASK    );
	dvbt2_plp_info->first_rf_idx           =   (( ( (rspByteBuffer[3]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_RF_IDX_LSB           ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_RF_IDX_MASK           );
	dvbt2_plp_info->ff_flag                =   (( ( (rspByteBuffer[3]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_FF_FLAG_LSB                ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_FF_FLAG_MASK                );
	dvbt2_plp_info->plp_group_id_msb       =   (( ( (rspByteBuffer[4]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_GROUP_ID_MSB_LSB       ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_GROUP_ID_MSB_MASK       );
	dvbt2_plp_info->first_frame_idx_lsb    =   (( ( (rspByteBuffer[4]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_FRAME_IDX_LSB_LSB    ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_FRAME_IDX_LSB_MASK    );
	dvbt2_plp_info->plp_mod_msb            =   (( ( (rspByteBuffer[5]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MOD_MSB_LSB            ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MOD_MSB_MASK            );
	dvbt2_plp_info->plp_cod                =   (( ( (rspByteBuffer[5]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_COD_LSB                ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_COD_MASK                );
	dvbt2_plp_info->plp_group_id_lsb       =   (( ( (rspByteBuffer[5]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_GROUP_ID_LSB_LSB       ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_GROUP_ID_LSB_MASK       );
	dvbt2_plp_info->plp_num_blocks_max_msb =   (( ( (rspByteBuffer[6]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_NUM_BLOCKS_MAX_MSB_LSB ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_NUM_BLOCKS_MAX_MSB_MASK );
	dvbt2_plp_info->plp_fec_type           =   (( ( (rspByteBuffer[6]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_FEC_TYPE_LSB           ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_FEC_TYPE_MASK           );
	dvbt2_plp_info->plp_rot                =   (( ( (rspByteBuffer[6]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_ROT_LSB                ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_ROT_MASK                );
	dvbt2_plp_info->plp_mod_lsb            =   (( ( (rspByteBuffer[6]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MOD_LSB_LSB            ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MOD_LSB_MASK            );
	dvbt2_plp_info->frame_interval_msb     =   (( ( (rspByteBuffer[7]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_FRAME_INTERVAL_MSB_LSB     ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_FRAME_INTERVAL_MSB_MASK     );
	dvbt2_plp_info->plp_num_blocks_max_lsb =   (( ( (rspByteBuffer[7]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_NUM_BLOCKS_MAX_LSB_LSB ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_NUM_BLOCKS_MAX_LSB_MASK );
	dvbt2_plp_info->time_il_length_msb     =   (( ( (rspByteBuffer[8]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_LENGTH_MSB_LSB     ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_LENGTH_MSB_MASK     );
	dvbt2_plp_info->frame_interval_lsb     =   (( ( (rspByteBuffer[8]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_FRAME_INTERVAL_LSB_LSB     ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_FRAME_INTERVAL_LSB_MASK     );
	dvbt2_plp_info->time_il_type           =   (( ( (rspByteBuffer[9]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_TYPE_LSB           ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_TYPE_MASK           );
	dvbt2_plp_info->time_il_length_lsb     =   (( ( (rspByteBuffer[9]  )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_LENGTH_LSB_LSB     ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_LENGTH_LSB_MASK     );
	dvbt2_plp_info->reserved_1_1           =   (( ( (rspByteBuffer[10] )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_RESERVED_1_1_LSB           ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_RESERVED_1_1_MASK           );
	dvbt2_plp_info->in_band_b_flag         =   (( ( (rspByteBuffer[10] )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_IN_BAND_B_FLAG_LSB         ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_IN_BAND_B_FLAG_MASK         );
	dvbt2_plp_info->in_band_a_flag         =   (( ( (rspByteBuffer[10] )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_IN_BAND_A_FLAG_LSB         ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_IN_BAND_A_FLAG_MASK         );
	dvbt2_plp_info->static_flag            =   (( ( (rspByteBuffer[11] )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_STATIC_FLAG_LSB            ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_STATIC_FLAG_MASK            );
	dvbt2_plp_info->plp_mode               =   (( ( (rspByteBuffer[11] )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MODE_LSB               ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MODE_MASK               );
	dvbt2_plp_info->reserved_1_2           =   (( ( (rspByteBuffer[11] )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_RESERVED_1_2_LSB           ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_RESERVED_1_2_MASK           );
	dvbt2_plp_info->static_padding_flag    =   (( ( (rspByteBuffer[12] )) >> Si2168B_DVBT2_PLP_INFO_RESPONSE_STATIC_PADDING_FLAG_LSB    ) & Si2168B_DVBT2_PLP_INFO_RESPONSE_STATIC_PADDING_FLAG_MASK    );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

/*---------------------------------------------------*/
/* Si2168B_DVBT2_FEF COMMAND                        */
/*---------------------------------------------------*/
static u8 si2168b_dvbt2_fef(si2168b_context *ctx,
		u8   fef_tuner_flag,
		u8   fef_tuner_flag_inv,
		Si2168B_DVBT2_FEF_CMD_REPLY_struct *dvbt2_fef)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[12];
	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("Si2168B DVBT2_FEF\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DVBT2_FEF_CMD;
	cmdByteBuffer[1] = (u8) ( ( fef_tuner_flag     & Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MASK     ) << Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_LSB    |
			( fef_tuner_flag_inv & Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_MASK ) << Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DVBT2_FEF bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 12, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DVBT2_FEF response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	dvbt2_fef->fef_type       =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT2_FEF_RESPONSE_FEF_TYPE_LSB       ) & Si2168B_DVBT2_FEF_RESPONSE_FEF_TYPE_MASK       );
	dvbt2_fef->fef_length     =   (( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 ) | (rspByteBuffer[6]  << 16 ) | (rspByteBuffer[7]  << 24 )) >> Si2168B_DVBT2_FEF_RESPONSE_FEF_LENGTH_LSB     ) & Si2168B_DVBT2_FEF_RESPONSE_FEF_LENGTH_MASK     );
	dvbt2_fef->fef_repetition =   (( ( (rspByteBuffer[8]  ) | (rspByteBuffer[9]  << 8 ) | (rspByteBuffer[10] << 16 ) | (rspByteBuffer[11] << 24 )) >> Si2168B_DVBT2_FEF_RESPONSE_FEF_REPETITION_LSB ) & Si2168B_DVBT2_FEF_RESPONSE_FEF_REPETITION_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_DD_CBER COMMAND                          */
/*---------------------------------------------------*/
static u8 Si2168B_L1_DD_CBER(si2168b_context *ctx, u8 rst)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[3];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->dd_cber.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B DD_CBER\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_CBER_CMD;
	cmdByteBuffer[1] = (u8) ( ( rst & Si2168B_DD_CBER_CMD_RST_MASK ) << Si2168B_DD_CBER_CMD_RST_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DD_CBER bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 3, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_CBER response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->dd_cber.exp  =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DD_CBER_RESPONSE_EXP_LSB  ) & Si2168B_DD_CBER_RESPONSE_EXP_MASK  );
	ctx->rsp->dd_cber.mant =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DD_CBER_RESPONSE_MANT_LSB ) & Si2168B_DD_CBER_RESPONSE_MANT_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_CONFIG_CLKIO COMMAND                     */
/*---------------------------------------------------*/
static u8 Si2168B_L1_CONFIG_CLKIO(si2168b_context *ctx,
		u8   output,
		u8   pre_driver_str,
		u8   driver_str)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[4];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->config_clkio.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B CONFIG_CLKIO\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_CONFIG_CLKIO_CMD;
	cmdByteBuffer[1] = (u8) ( ( output         & Si2168B_CONFIG_CLKIO_CMD_OUTPUT_MASK         ) << Si2168B_CONFIG_CLKIO_CMD_OUTPUT_LSB        |
			( pre_driver_str & Si2168B_CONFIG_CLKIO_CMD_PRE_DRIVER_STR_MASK ) << Si2168B_CONFIG_CLKIO_CMD_PRE_DRIVER_STR_LSB|
			( driver_str     & Si2168B_CONFIG_CLKIO_CMD_DRIVER_STR_MASK     ) << Si2168B_CONFIG_CLKIO_CMD_DRIVER_STR_LSB    );

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing CONFIG_CLKIO bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 4, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling CONFIG_CLKIO response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->config_clkio.mode           =   (( ( (rspByteBuffer[1]  )) >> Si2168B_CONFIG_CLKIO_RESPONSE_MODE_LSB           ) & Si2168B_CONFIG_CLKIO_RESPONSE_MODE_MASK           );
	ctx->rsp->config_clkio.pre_driver_str =   (( ( (rspByteBuffer[2]  )) >> Si2168B_CONFIG_CLKIO_RESPONSE_PRE_DRIVER_STR_LSB ) & Si2168B_CONFIG_CLKIO_RESPONSE_PRE_DRIVER_STR_MASK );
	ctx->rsp->config_clkio.driver_str     =   (( ( (rspByteBuffer[3]  )) >> Si2168B_CONFIG_CLKIO_RESPONSE_DRIVER_STR_LSB     ) & Si2168B_CONFIG_CLKIO_RESPONSE_DRIVER_STR_MASK     );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_DVBT_TPS_EXTRA COMMAND                   */
/*---------------------------------------------------*/
static u8 Si2168B_L1_DVBT_TPS_EXTRA(si2168b_context *ctx)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[1];
	u8 rspByteBuffer[6];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->dvbt_tps_extra.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B DVBT_TPS_EXTRA\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DVBT_TPS_EXTRA_CMD;

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 1, cmdByteBuffer) != 1) {
		SiTRACE("Error writing DVBT_TPS_EXTRA bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 6, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DVBT_TPS_EXTRA response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->dvbt_tps_extra.lptimeslice =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPTIMESLICE_LSB ) & Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPTIMESLICE_MASK );
	ctx->rsp->dvbt_tps_extra.hptimeslice =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPTIMESLICE_LSB ) & Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPTIMESLICE_MASK );
	ctx->rsp->dvbt_tps_extra.lpmpefec    =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPMPEFEC_LSB    ) & Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPMPEFEC_MASK    );
	ctx->rsp->dvbt_tps_extra.hpmpefec    =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPMPEFEC_LSB    ) & Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPMPEFEC_MASK    );
	ctx->rsp->dvbt_tps_extra.dvbhinter   =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DVBT_TPS_EXTRA_RESPONSE_DVBHINTER_LSB   ) & Si2168B_DVBT_TPS_EXTRA_RESPONSE_DVBHINTER_MASK   );
	ctx->rsp->dvbt_tps_extra.cell_id     = (((( ( (rspByteBuffer[2]  ) | (rspByteBuffer[3]  << 8 )) >> Si2168B_DVBT_TPS_EXTRA_RESPONSE_CELL_ID_LSB     ) & Si2168B_DVBT_TPS_EXTRA_RESPONSE_CELL_ID_MASK) <<Si2168B_DVBT_TPS_EXTRA_RESPONSE_CELL_ID_SHIFT ) >>Si2168B_DVBT_TPS_EXTRA_RESPONSE_CELL_ID_SHIFT     );
	ctx->rsp->dvbt_tps_extra.tps_res1    =   (( ( (rspByteBuffer[4]  )) >> Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES1_LSB    ) & Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES1_MASK    );
	ctx->rsp->dvbt_tps_extra.tps_res2    =   (( ( (rspByteBuffer[4]  )) >> Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES2_LSB    ) & Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES2_MASK    );
	ctx->rsp->dvbt_tps_extra.tps_res3    =   (( ( (rspByteBuffer[5]  )) >> Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES3_LSB    ) & Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES3_MASK    );
	ctx->rsp->dvbt_tps_extra.tps_res4    =   (( ( (rspByteBuffer[5]  )) >> Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES4_LSB    ) & Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES4_MASK    );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_DD_PER COMMAND                           */
/*---------------------------------------------------*/
static u8 Si2168B_L1_DD_PER(si2168b_context *ctx, u8 rst)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[3];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->dd_per.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B DD_PER\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_PER_CMD;
	cmdByteBuffer[1] = (u8) ( ( rst & Si2168B_DD_PER_CMD_RST_MASK ) << Si2168B_DD_PER_CMD_RST_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DD_PER bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 3, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_PER response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->dd_per.exp  =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DD_PER_RESPONSE_EXP_LSB  ) & Si2168B_DD_PER_RESPONSE_EXP_MASK  );
	ctx->rsp->dd_per.mant =   (( ( (rspByteBuffer[2]  )) >> Si2168B_DD_PER_RESPONSE_MANT_LSB ) & Si2168B_DD_PER_RESPONSE_MANT_MASK );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

#ifdef __EXTRA_COMMANDS__
/*---------------------------------------------------*/
/* Si2168B_DD_SSI_SQI COMMAND                       */
/*---------------------------------------------------*/
static u8 Si2168B_L1_DD_SSI_SQI(si2168b_context *ctx, char tuner_rssi)
{
	u8 error_code = 0;
	u8 cmdByteBuffer[2];
	u8 rspByteBuffer[3];
	u8 ret = NO_Si2168B_ERROR;
#ifdef __COMMONREPLYOBJ__
	ctx->rsp->dd_ssi_sqi.STATUS = ctx->status;
#endif

	SiTRACE("Si2168B DD_SSI_SQI\n");

	_mutex_lock(&ctx->lock);

	cmdByteBuffer[0] = Si2168B_DD_SSI_SQI_CMD;
	cmdByteBuffer[1] = (u8) ( ( tuner_rssi & Si2168B_DD_SSI_SQI_CMD_TUNER_RSSI_MASK ) << Si2168B_DD_SSI_SQI_CMD_TUNER_RSSI_LSB);

	if (i2c_write_bytes(ctx->i2c_adap, ctx->i2c_addr, 2, cmdByteBuffer) != 2) {
		SiTRACE("Error writing DD_SSI_SQI bytes!\n");
		ret = ERROR_Si2168B_SENDING_COMMAND;
		goto unlock_mutex;
	}

	error_code = si2168b_poll_for_response(ctx, 3, rspByteBuffer);
	if (error_code) {
		SiTRACE("Error polling DD_SSI_SQI response\n");
		ret = error_code;
		goto unlock_mutex;
	}

	ctx->rsp->dd_ssi_sqi.ssi =   (( ( (rspByteBuffer[1]  )) >> Si2168B_DD_SSI_SQI_RESPONSE_SSI_LSB ) & Si2168B_DD_SSI_SQI_RESPONSE_SSI_MASK );
	ctx->rsp->dd_ssi_sqi.sqi = (((( ( (rspByteBuffer[2]  )) >> Si2168B_DD_SSI_SQI_RESPONSE_SQI_LSB ) & Si2168B_DD_SSI_SQI_RESPONSE_SQI_MASK) <<Si2168B_DD_SSI_SQI_RESPONSE_SQI_SHIFT ) >>Si2168B_DD_SSI_SQI_RESPONSE_SQI_SHIFT );

unlock_mutex:
	_mutex_unlock(&ctx->lock);

	return ret;
}
#endif

/*****************************************************************************************
 NAME: si2168b_set_common_properties
  DESCRIPTION: Setup Si2168B COMMON properties configuration
  This function will download all the COMMON configuration properties.
  The function Si2168B_storeUserProperties should be called before the first call to this function.
  Parameter:  Pointer to Si2168B Context
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
  Programming Guide Reference:    COMMON setup flowchart
******************************************************************************************/
static int si2168b_set_common_properties(si2168b_context *ctx)
{
	const u8 master_ien_ddien   = Si2168B_MASTER_IEN_PROP_DDIEN_OFF   ; /* (default 'OFF') */
	const u8 master_ien_scanien = Si2168B_MASTER_IEN_PROP_SCANIEN_OFF ; /* (default 'OFF') */
	const u8 master_ien_errien  = Si2168B_MASTER_IEN_PROP_ERRIEN_OFF  ; /* (default 'OFF') */
	const u8 master_ien_ctsien  = Si2168B_MASTER_IEN_PROP_CTSIEN_OFF  ; /* (default 'OFF') */

	u8  ret;
	u8  err = NO_Si2168B_ERROR;
	u16 data;

	SiTRACE("si2168b_set_common_properties\n");

	data = (master_ien_ddien   & Si2168B_MASTER_IEN_PROP_DDIEN_MASK  ) << Si2168B_MASTER_IEN_PROP_DDIEN_LSB |
           (master_ien_scanien & Si2168B_MASTER_IEN_PROP_SCANIEN_MASK) << Si2168B_MASTER_IEN_PROP_SCANIEN_LSB |
           (master_ien_errien  & Si2168B_MASTER_IEN_PROP_ERRIEN_MASK ) << Si2168B_MASTER_IEN_PROP_ERRIEN_LSB |
           (master_ien_ctsien  & Si2168B_MASTER_IEN_PROP_CTSIEN_MASK ) << Si2168B_MASTER_IEN_PROP_CTSIEN_LSB;

	ret = si2168b_set_property(ctx, Si2168B_MASTER_IEN_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_MASTER_IEN_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

	return err;
}

/*****************************************************************************************
 NAME: si2168b_set_dd_properties
  DESCRIPTION: Setup Si2168B DD properties configuration
  This function will download all the DD configuration properties.
  The function Si2168B_storeUserProperties should be called before the first call to this function.
  Parameter:  Pointer to Si2168B Context
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
  Programming Guide Reference:    DD setup flowchart
******************************************************************************************/
static int si2168b_set_dd_properties(si2168b_context *ctx)
{
	const u8  dd_ber_resol_exp                     = 7; /* (default     7) */
	const u8  dd_ber_resol_mant                    = 1; /* (default     1) */
	const u8  dd_cber_resol_exp                    = 5; /* (default     5) */
	const u8  dd_cber_resol_mant                   = 1; /* (default     1) */
	const u8  dd_fer_resol_exp                     = 3; /* (default     3) */
	const u8  dd_fer_resol_mant                    = 1; /* (default     1) */
    const u8  dd_ien_ien_bit0                      = Si2168B_DD_IEN_PROP_IEN_BIT0_DISABLE; /* (default 'DISABLE') */
    const u8  dd_ien_ien_bit1                      = Si2168B_DD_IEN_PROP_IEN_BIT1_DISABLE; /* (default 'DISABLE') */
    const u8  dd_ien_ien_bit2                      = Si2168B_DD_IEN_PROP_IEN_BIT2_DISABLE; /* (default 'DISABLE') */
    const u8  dd_ien_ien_bit3                      = Si2168B_DD_IEN_PROP_IEN_BIT3_DISABLE; /* (default 'DISABLE') */
    const u8  dd_ien_ien_bit4                      = Si2168B_DD_IEN_PROP_IEN_BIT4_DISABLE; /* (default 'DISABLE') */
    const u8  dd_ien_ien_bit5                      = Si2168B_DD_IEN_PROP_IEN_BIT5_DISABLE; /* (default 'DISABLE') */
    const u8  dd_ien_ien_bit6                      = Si2168B_DD_IEN_PROP_IEN_BIT6_DISABLE; /* (default 'DISABLE') */
    const u8  dd_ien_ien_bit7                      = Si2168B_DD_IEN_PROP_IEN_BIT7_DISABLE; /* (default 'DISABLE') */
	const u16 dd_if_input_freq_offset              = 5000; /* (default  5000) */
    const u8  dd_int_sense_neg_bit0                = Si2168B_DD_INT_SENSE_PROP_NEG_BIT0_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_neg_bit1                = Si2168B_DD_INT_SENSE_PROP_NEG_BIT1_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_neg_bit2                = Si2168B_DD_INT_SENSE_PROP_NEG_BIT2_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_neg_bit3                = Si2168B_DD_INT_SENSE_PROP_NEG_BIT3_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_neg_bit4                = Si2168B_DD_INT_SENSE_PROP_NEG_BIT4_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_neg_bit5                = Si2168B_DD_INT_SENSE_PROP_NEG_BIT5_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_neg_bit6                = Si2168B_DD_INT_SENSE_PROP_NEG_BIT6_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_neg_bit7                = Si2168B_DD_INT_SENSE_PROP_NEG_BIT7_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_pos_bit0                = Si2168B_DD_INT_SENSE_PROP_POS_BIT0_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_pos_bit1                = Si2168B_DD_INT_SENSE_PROP_POS_BIT1_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_pos_bit2                = Si2168B_DD_INT_SENSE_PROP_POS_BIT2_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_pos_bit3                = Si2168B_DD_INT_SENSE_PROP_POS_BIT3_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_pos_bit4                = Si2168B_DD_INT_SENSE_PROP_POS_BIT4_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_pos_bit5                = Si2168B_DD_INT_SENSE_PROP_POS_BIT5_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_pos_bit6                = Si2168B_DD_INT_SENSE_PROP_POS_BIT6_DISABLE; /* (default 'DISABLE') */
    const u8  dd_int_sense_pos_bit7                = Si2168B_DD_INT_SENSE_PROP_POS_BIT7_DISABLE; /* (default 'DISABLE') */
    const u8  dd_mode_bw                           = Si2168B_DD_MODE_PROP_BW_BW_8MHZ; /* (default 'BW_8MHZ') */
    const u8  dd_mode_modulation                   = Si2168B_DD_MODE_PROP_MODULATION_DVBT; /* (default 'DVBT') */
    const u8  dd_mode_invert_spectrum              = Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_NORMAL; /* (default 'NORMAL') */
    const u8  dd_mode_auto_detect                  = Si2168B_DD_MODE_PROP_AUTO_DETECT_NONE; /* (default 'NONE') */
    const u8  dd_per_resol_exp                     = 5; /* (default     5) */
    const u8  dd_per_resol_mant                    = 1; /* (default     1) */
    const u8  dd_rsq_ber_threshold_exp             = 1; /* (default     1) */
    const u8  dd_rsq_ber_threshold_mant            = 10; /* (default    10) */
    const u8  dd_ssi_sqi_param_sqi_average         = 1; /* (default     1) */
	const u16 dd_ts_freq_req_freq_10khz            = 720; /* (default   720) */
    const u8  dd_ts_mode_mode                      = Si2168B_DD_TS_MODE_PROP_MODE_TRISTATE; /* (default 'TRISTATE') */
    const u8  dd_ts_mode_clock                     = Si2168B_DD_TS_MODE_PROP_CLOCK_AUTO_FIXED; /* (default 'AUTO_FIXED') */
    const u8  dd_ts_mode_clk_gapped_en             = Si2168B_DD_TS_MODE_PROP_CLK_GAPPED_EN_DISABLED; /* (default 'DISABLED') */
    const u8  dd_ts_mode_ts_err_polarity           = Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_NOT_INVERTED; /* (default 'NOT_INVERTED') */
    const u8  dd_ts_mode_special                   = Si2168B_DD_TS_MODE_PROP_SPECIAL_FULL_TS; /* (default 'FULL_TS') */
    const u8  dd_ts_mode_ts_freq_resolution        = Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_NORMAL; /* (default 'NORMAL') */
    const u8  dd_ts_serial_diff_ts_data1_strength  = 15; /* (default    15) */
    const u8  dd_ts_serial_diff_ts_data1_shape     = 3; /* (default     3) */
    const u8  dd_ts_serial_diff_ts_data2_strength  = 15; /* (default    15) */
    const u8  dd_ts_serial_diff_ts_data2_shape     = 3; /* (default     3) */
    const u8  dd_ts_serial_diff_ts_clkb_on_data1   = Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_CLKB_ON_DATA1_DISABLE; /* (default 'DISABLE') */
    const u8  dd_ts_serial_diff_ts_data0b_on_data2 = Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA0B_ON_DATA2_DISABLE; /* (default 'DISABLE') */
    const u8  dd_ts_setup_par_ts_data_strength     = 3; /* (default     3) */
    const u8  dd_ts_setup_par_ts_data_shape        = 1; /* (default     1) */
    const u8  dd_ts_setup_par_ts_clk_strength      = 3; /* (default     3) */
    const u8  dd_ts_setup_par_ts_clk_shape         = 1; /* (default     1) */
    const u8  dd_ts_setup_par_ts_clk_invert        = Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_INVERTED; /* (default 'INVERTED') */
    const u8  dd_ts_setup_par_ts_clk_shift         = 0; /* (default     0) */
    const u8  dd_ts_setup_ser_ts_data_strength     = 15; /* (default    15) */
    const u8  dd_ts_setup_ser_ts_data_shape        = 3; /* (default     3) */
    const u8  dd_ts_setup_ser_ts_clk_strength      = 15; /* (default    15) */
    const u8  dd_ts_setup_ser_ts_clk_shape         = 3; /* (default     3) */
    const u8  dd_ts_setup_ser_ts_clk_invert        = Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_INVERT_INVERTED; /* (default 'INVERTED') */
    const u8  dd_ts_setup_ser_ts_sync_duration     = Si2168B_DD_TS_SETUP_SER_PROP_TS_SYNC_DURATION_FIRST_BYTE; /* (default 'FIRST_BYTE') */
    const u8  dd_ts_setup_ser_ts_byte_order        = Si2168B_DD_TS_SETUP_SER_PROP_TS_BYTE_ORDER_MSB_FIRST; /* (default 'MSB_FIRST') */

#ifdef    Si2168B_DD_DISEQC_FREQ_PROP
	dd_diseqc_freq.freq_hz                         = 22000; /* (default 22000) */
#endif /* Si2168B_DD_DISEQC_FREQ_PROP */

#ifdef    Si2168B_DD_DISEQC_PARAM_PROP
	dd_diseqc_param.sequence_mode                  = Si2168B_DD_DISEQC_PARAM_PROP_SEQUENCE_MODE_GAP ; /* (default 'GAP') */
#endif /* Si2168B_DD_DISEQC_PARAM_PROP */

	u8  ret;
	u8  err = NO_Si2168B_ERROR;
	u16 data;

	SiTRACE("si2168b_set_dd_properties\n");

    data = (dd_ber_resol_exp  & Si2168B_DD_BER_RESOL_PROP_EXP_MASK ) << Si2168B_DD_BER_RESOL_PROP_EXP_LSB |
           (dd_ber_resol_mant & Si2168B_DD_BER_RESOL_PROP_MANT_MASK) << Si2168B_DD_BER_RESOL_PROP_MANT_LSB;
	ret = si2168b_set_property(ctx, Si2168B_DD_BER_RESOL_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_BER_RESOL_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_cber_resol_exp  & Si2168B_DD_CBER_RESOL_PROP_EXP_MASK ) << Si2168B_DD_CBER_RESOL_PROP_EXP_LSB |
           (dd_cber_resol_mant & Si2168B_DD_CBER_RESOL_PROP_MANT_MASK) << Si2168B_DD_CBER_RESOL_PROP_MANT_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_CBER_RESOL_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_CBER_RESOL_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_fer_resol_exp  & Si2168B_DD_FER_RESOL_PROP_EXP_MASK ) << Si2168B_DD_FER_RESOL_PROP_EXP_LSB |
           (dd_fer_resol_mant & Si2168B_DD_FER_RESOL_PROP_MANT_MASK) << Si2168B_DD_FER_RESOL_PROP_MANT_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_FER_RESOL_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_FER_RESOL_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_ien_ien_bit0 & Si2168B_DD_IEN_PROP_IEN_BIT0_MASK) << Si2168B_DD_IEN_PROP_IEN_BIT0_LSB |
           (dd_ien_ien_bit1 & Si2168B_DD_IEN_PROP_IEN_BIT1_MASK) << Si2168B_DD_IEN_PROP_IEN_BIT1_LSB |
           (dd_ien_ien_bit2 & Si2168B_DD_IEN_PROP_IEN_BIT2_MASK) << Si2168B_DD_IEN_PROP_IEN_BIT2_LSB |
           (dd_ien_ien_bit3 & Si2168B_DD_IEN_PROP_IEN_BIT3_MASK) << Si2168B_DD_IEN_PROP_IEN_BIT3_LSB |
           (dd_ien_ien_bit4 & Si2168B_DD_IEN_PROP_IEN_BIT4_MASK) << Si2168B_DD_IEN_PROP_IEN_BIT4_LSB |
           (dd_ien_ien_bit5 & Si2168B_DD_IEN_PROP_IEN_BIT5_MASK) << Si2168B_DD_IEN_PROP_IEN_BIT5_LSB |
           (dd_ien_ien_bit6 & Si2168B_DD_IEN_PROP_IEN_BIT6_MASK) << Si2168B_DD_IEN_PROP_IEN_BIT6_LSB |
           (dd_ien_ien_bit7 & Si2168B_DD_IEN_PROP_IEN_BIT7_MASK) << Si2168B_DD_IEN_PROP_IEN_BIT7_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_IEN_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_IEN_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_if_input_freq_offset & Si2168B_DD_IF_INPUT_FREQ_PROP_OFFSET_MASK) << Si2168B_DD_IF_INPUT_FREQ_PROP_OFFSET_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_IF_INPUT_FREQ_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_IF_INPUT_FREQ_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_int_sense_neg_bit0 & Si2168B_DD_INT_SENSE_PROP_NEG_BIT0_MASK) << Si2168B_DD_INT_SENSE_PROP_NEG_BIT0_LSB |
           (dd_int_sense_neg_bit1 & Si2168B_DD_INT_SENSE_PROP_NEG_BIT1_MASK) << Si2168B_DD_INT_SENSE_PROP_NEG_BIT1_LSB |
           (dd_int_sense_neg_bit2 & Si2168B_DD_INT_SENSE_PROP_NEG_BIT2_MASK) << Si2168B_DD_INT_SENSE_PROP_NEG_BIT2_LSB |
           (dd_int_sense_neg_bit3 & Si2168B_DD_INT_SENSE_PROP_NEG_BIT3_MASK) << Si2168B_DD_INT_SENSE_PROP_NEG_BIT3_LSB |
           (dd_int_sense_neg_bit4 & Si2168B_DD_INT_SENSE_PROP_NEG_BIT4_MASK) << Si2168B_DD_INT_SENSE_PROP_NEG_BIT4_LSB |
           (dd_int_sense_neg_bit5 & Si2168B_DD_INT_SENSE_PROP_NEG_BIT5_MASK) << Si2168B_DD_INT_SENSE_PROP_NEG_BIT5_LSB |
           (dd_int_sense_neg_bit6 & Si2168B_DD_INT_SENSE_PROP_NEG_BIT6_MASK) << Si2168B_DD_INT_SENSE_PROP_NEG_BIT6_LSB |
           (dd_int_sense_neg_bit7 & Si2168B_DD_INT_SENSE_PROP_NEG_BIT7_MASK) << Si2168B_DD_INT_SENSE_PROP_NEG_BIT7_LSB |
           (dd_int_sense_pos_bit0 & Si2168B_DD_INT_SENSE_PROP_POS_BIT0_MASK) << Si2168B_DD_INT_SENSE_PROP_POS_BIT0_LSB |
           (dd_int_sense_pos_bit1 & Si2168B_DD_INT_SENSE_PROP_POS_BIT1_MASK) << Si2168B_DD_INT_SENSE_PROP_POS_BIT1_LSB |
           (dd_int_sense_pos_bit2 & Si2168B_DD_INT_SENSE_PROP_POS_BIT2_MASK) << Si2168B_DD_INT_SENSE_PROP_POS_BIT2_LSB |
           (dd_int_sense_pos_bit3 & Si2168B_DD_INT_SENSE_PROP_POS_BIT3_MASK) << Si2168B_DD_INT_SENSE_PROP_POS_BIT3_LSB |
           (dd_int_sense_pos_bit4 & Si2168B_DD_INT_SENSE_PROP_POS_BIT4_MASK) << Si2168B_DD_INT_SENSE_PROP_POS_BIT4_LSB |
           (dd_int_sense_pos_bit5 & Si2168B_DD_INT_SENSE_PROP_POS_BIT5_MASK) << Si2168B_DD_INT_SENSE_PROP_POS_BIT5_LSB |
           (dd_int_sense_pos_bit6 & Si2168B_DD_INT_SENSE_PROP_POS_BIT6_MASK) << Si2168B_DD_INT_SENSE_PROP_POS_BIT6_LSB |
           (dd_int_sense_pos_bit7 & Si2168B_DD_INT_SENSE_PROP_POS_BIT7_MASK) << Si2168B_DD_INT_SENSE_PROP_POS_BIT7_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_INT_SENSE_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_INT_SENSE_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_mode_bw              & Si2168B_DD_MODE_PROP_BW_MASK             ) << Si2168B_DD_MODE_PROP_BW_LSB |
           (dd_mode_modulation      & Si2168B_DD_MODE_PROP_MODULATION_MASK     ) << Si2168B_DD_MODE_PROP_MODULATION_LSB |
           (dd_mode_invert_spectrum & Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_LSB |
           (dd_mode_auto_detect     & Si2168B_DD_MODE_PROP_AUTO_DETECT_MASK    ) << Si2168B_DD_MODE_PROP_AUTO_DETECT_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_MODE_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_MODE_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_per_resol_exp  & Si2168B_DD_PER_RESOL_PROP_EXP_MASK ) << Si2168B_DD_PER_RESOL_PROP_EXP_LSB |
           (dd_per_resol_mant & Si2168B_DD_PER_RESOL_PROP_MANT_MASK) << Si2168B_DD_PER_RESOL_PROP_MANT_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_PER_RESOL_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_PER_RESOL_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_rsq_ber_threshold_exp  & Si2168B_DD_RSQ_BER_THRESHOLD_PROP_EXP_MASK ) << Si2168B_DD_RSQ_BER_THRESHOLD_PROP_EXP_LSB |
           (dd_rsq_ber_threshold_mant & Si2168B_DD_RSQ_BER_THRESHOLD_PROP_MANT_MASK) << Si2168B_DD_RSQ_BER_THRESHOLD_PROP_MANT_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_RSQ_BER_THRESHOLD_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_RSQ_BER_THRESHOLD_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_ssi_sqi_param_sqi_average & Si2168B_DD_SSI_SQI_PARAM_PROP_SQI_AVERAGE_MASK) << Si2168B_DD_SSI_SQI_PARAM_PROP_SQI_AVERAGE_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_SSI_SQI_PARAM_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_SSI_SQI_PARAM_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_ts_freq_req_freq_10khz & Si2168B_DD_TS_FREQ_PROP_REQ_FREQ_10KHZ_MASK) << Si2168B_DD_TS_FREQ_PROP_REQ_FREQ_10KHZ_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_TS_FREQ_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_TS_FREQ_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_ts_mode_mode            & Si2168B_DD_TS_MODE_PROP_MODE_MASK           ) << Si2168B_DD_TS_MODE_PROP_MODE_LSB |
           (dd_ts_mode_clock           & Si2168B_DD_TS_MODE_PROP_CLOCK_MASK          ) << Si2168B_DD_TS_MODE_PROP_CLOCK_LSB |
           (dd_ts_mode_clk_gapped_en   & Si2168B_DD_TS_MODE_PROP_CLK_GAPPED_EN_MASK  ) << Si2168B_DD_TS_MODE_PROP_CLK_GAPPED_EN_LSB |
           (dd_ts_mode_ts_err_polarity & Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_MASK) << Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_LSB |
           (dd_ts_mode_special            & Si2168B_DD_TS_MODE_PROP_SPECIAL_MASK     ) << Si2168B_DD_TS_MODE_PROP_SPECIAL_LSB |
           (dd_ts_mode_ts_freq_resolution & Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_MASK) << Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_TS_MODE_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_TS_MODE_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_ts_serial_diff_ts_data1_strength  & Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA1_STRENGTH_MASK ) << Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA1_STRENGTH_LSB |
           (dd_ts_serial_diff_ts_data1_shape     & Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA1_SHAPE_MASK    ) << Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA1_SHAPE_LSB |
           (dd_ts_serial_diff_ts_data2_strength  & Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA2_STRENGTH_MASK ) << Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA2_STRENGTH_LSB |
           (dd_ts_serial_diff_ts_data2_shape     & Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA2_SHAPE_MASK    ) << Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA2_SHAPE_LSB |
           (dd_ts_serial_diff_ts_clkb_on_data1   & Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_CLKB_ON_DATA1_MASK  ) << Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_CLKB_ON_DATA1_LSB |
           (dd_ts_serial_diff_ts_data0b_on_data2 & Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA0B_ON_DATA2_MASK) << Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA0B_ON_DATA2_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_TS_SERIAL_DIFF_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_TS_SERIAL_DIFF_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_ts_setup_par_ts_data_strength & Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_STRENGTH_MASK) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_STRENGTH_LSB |
           (dd_ts_setup_par_ts_data_shape    & Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_SHAPE_MASK   ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_SHAPE_LSB |
           (dd_ts_setup_par_ts_clk_strength  & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_STRENGTH_MASK ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_STRENGTH_LSB |
           (dd_ts_setup_par_ts_clk_shape     & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHAPE_MASK    ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHAPE_LSB |
           (dd_ts_setup_par_ts_clk_invert    & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_MASK   ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_LSB |
           (dd_ts_setup_par_ts_clk_shift     & Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHIFT_MASK    ) << Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHIFT_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DD_TS_SETUP_PAR_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_TS_SETUP_PAR_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dd_ts_setup_ser_ts_data_strength & Si2168B_DD_TS_SETUP_SER_PROP_TS_DATA_STRENGTH_MASK) << Si2168B_DD_TS_SETUP_SER_PROP_TS_DATA_STRENGTH_LSB  |
           (dd_ts_setup_ser_ts_data_shape    & Si2168B_DD_TS_SETUP_SER_PROP_TS_DATA_SHAPE_MASK   ) << Si2168B_DD_TS_SETUP_SER_PROP_TS_DATA_SHAPE_LSB  |
           (dd_ts_setup_ser_ts_clk_strength  & Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_STRENGTH_MASK ) << Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_STRENGTH_LSB  |
           (dd_ts_setup_ser_ts_clk_shape     & Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_SHAPE_MASK    ) << Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_SHAPE_LSB  |
           (dd_ts_setup_ser_ts_clk_invert    & Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_INVERT_MASK   ) << Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_INVERT_LSB  |
           (dd_ts_setup_ser_ts_sync_duration & Si2168B_DD_TS_SETUP_SER_PROP_TS_SYNC_DURATION_MASK) << Si2168B_DD_TS_SETUP_SER_PROP_TS_SYNC_DURATION_LSB  |
           (dd_ts_setup_ser_ts_byte_order    & Si2168B_DD_TS_SETUP_SER_PROP_TS_BYTE_ORDER_MASK   ) << Si2168B_DD_TS_SETUP_SER_PROP_TS_BYTE_ORDER_LSB ;
    ret = si2168b_set_property(ctx, Si2168B_DD_TS_SETUP_SER_PROP, data);
    pr_info("%s(): setting property Si2168B_DD_TS_SETUP_SER_PROP 0x%04X: data=0x%04X\n", __func__, Si2168B_DD_TS_SETUP_SER_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DD_TS_SETUP_SER_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    return err;
}

/*****************************************************************************************
 NAME: si2168b_set_dvbc_properties
  DESCRIPTION: Setup Si2168B DVBC properties configuration
  This function will download all the DVBC configuration properties.
  The function Si2168B_storeUserProperties should be called before the first call to this function.
  Parameter:  Pointer to Si2168B Context
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
  Programming Guide Reference:    DVBC setup flowchart
******************************************************************************************/
static int si2168b_set_dvbc_properties(si2168b_context *ctx)
{
    const u8  dvbc_adc_crest_factor_crest_factor = 112; /* (default   112) */
    const u16 dvbc_afc_range_range_khz           = 100; /* (default   100) */
    const u8  dvbc_constellation_constellation   = Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_AUTO ; /* (default 'AUTO') */
    const u16 dvbc_symbol_rate_rate              = 6900; /* (default  6900) */

	u8  ret;
	u8  err = NO_Si2168B_ERROR;
	u16 data;

	SiTRACE("si2168b_set_dvbc_properties\n");

    data = (dvbc_adc_crest_factor_crest_factor & Si2168B_DVBC_ADC_CREST_FACTOR_PROP_CREST_FACTOR_MASK) << Si2168B_DVBC_ADC_CREST_FACTOR_PROP_CREST_FACTOR_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DVBC_ADC_CREST_FACTOR_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBC_ADC_CREST_FACTOR_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dvbc_afc_range_range_khz & Si2168B_DVBC_AFC_RANGE_PROP_RANGE_KHZ_MASK) << Si2168B_DVBC_AFC_RANGE_PROP_RANGE_KHZ_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DVBC_AFC_RANGE_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBC_AFC_RANGE_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dvbc_constellation_constellation & Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_MASK) << Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DVBC_CONSTELLATION_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBC_CONSTELLATION_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dvbc_symbol_rate_rate & Si2168B_DVBC_SYMBOL_RATE_PROP_RATE_MASK) << Si2168B_DVBC_SYMBOL_RATE_PROP_RATE_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DVBC_SYMBOL_RATE_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBC_SYMBOL_RATE_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    return err;
}

/*****************************************************************************************
 NAME: si2168b_set_dvbt_properties
  DESCRIPTION: Setup Si2168B DVBT properties configuration
  This function will download all the DVBT configuration properties.
  The function Si2168B_storeUserProperties should be called before the first call to this function.
  Parameter:  Pointer to Si2168B Context
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
  Programming Guide Reference:    DVBT setup flowchart
******************************************************************************************/
static int si2168b_set_dvbt_properties(si2168b_context *ctx)
{
    const u8  dvbt_adc_crest_factor_crest_factor = 130; /* (default   130) */
    const u16 dvbt_afc_range_range_khz           = 550; /* (default   550) */
    const u8  dvbt_hierarchy_stream              = Si2168B_DVBT_HIERARCHY_PROP_STREAM_HP; /* (default 'HP') */

	u8  ret;
	u8  err = NO_Si2168B_ERROR;
	u16 data;

	SiTRACE("si2168b_set_dvbt_properties\n");

    data = (dvbt_adc_crest_factor_crest_factor & Si2168B_DVBT_ADC_CREST_FACTOR_PROP_CREST_FACTOR_MASK) << Si2168B_DVBT_ADC_CREST_FACTOR_PROP_CREST_FACTOR_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DVBT_ADC_CREST_FACTOR_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBT_ADC_CREST_FACTOR_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dvbt_afc_range_range_khz & Si2168B_DVBT_AFC_RANGE_PROP_RANGE_KHZ_MASK) << Si2168B_DVBT_AFC_RANGE_PROP_RANGE_KHZ_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DVBT_AFC_RANGE_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBT_AFC_RANGE_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dvbt_hierarchy_stream & Si2168B_DVBT_HIERARCHY_PROP_STREAM_MASK) << Si2168B_DVBT_HIERARCHY_PROP_STREAM_LSB;
    ret = si2168b_set_property(ctx, Si2168B_DVBT_HIERARCHY_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBT_HIERARCHY_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

	return err;
}

/*****************************************************************************************
 NAME: si2168b_set_dvbt2_properties
  DESCRIPTION: Setup Si2168B DVBT2 properties configuration
  This function will download all the DVBT2 configuration properties.
  The function Si2168B_storeUserProperties should be called before the first call to this function.
  Parameter:  Pointer to Si2168B Context
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
  Programming Guide Reference:    DVBT2 setup flowchart
******************************************************************************************/
static int si2168b_set_dvbt2_properties(si2168b_context *ctx)
{
    const u8  dvbt2_adc_crest_factor_crest_factor = 130; /* (default   130) */
    const u16 dvbt2_afc_range_range_khz           = 550; /* (default   550) */
    const u8  dvbt2_fef_tuner_tuner_delay         = 1; /* (default     1) */
    const u8  dvbt2_fef_tuner_tuner_freeze_time   = 1; /* (default     1) */
    const u8  dvbt2_fef_tuner_tuner_unfreeze_time = 1; /* (default     1) */
    const u8  dvbt2_mode_lock_mode                = Si2168B_DVBT2_MODE_PROP_LOCK_MODE_ANY; /* (default 'ANY') */

	u8  ret;
	u8  err = NO_Si2168B_ERROR;
	u16 data;

    SiTRACE("si2168b_set_dvbt2_properties\n");

    data = (dvbt2_adc_crest_factor_crest_factor & Si2168B_DVBT2_ADC_CREST_FACTOR_PROP_CREST_FACTOR_MASK) << Si2168B_DVBT2_ADC_CREST_FACTOR_PROP_CREST_FACTOR_LSB ;
    ret = si2168b_set_property(ctx, Si2168B_DVBT2_ADC_CREST_FACTOR_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBT2_ADC_CREST_FACTOR_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dvbt2_afc_range_range_khz & Si2168B_DVBT2_AFC_RANGE_PROP_RANGE_KHZ_MASK) << Si2168B_DVBT2_AFC_RANGE_PROP_RANGE_KHZ_LSB ;
    ret = si2168b_set_property(ctx, Si2168B_DVBT2_AFC_RANGE_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBT2_AFC_RANGE_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dvbt2_fef_tuner_tuner_delay         & Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_DELAY_MASK        ) << Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_DELAY_LSB  |
            (dvbt2_fef_tuner_tuner_freeze_time   & Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_FREEZE_TIME_MASK  ) << Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_FREEZE_TIME_LSB  |
            (dvbt2_fef_tuner_tuner_unfreeze_time & Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_UNFREEZE_TIME_MASK) << Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_UNFREEZE_TIME_LSB ;
    ret = si2168b_set_property(ctx, Si2168B_DVBT2_FEF_TUNER_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBT2_FEF_TUNER_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (dvbt2_mode_lock_mode & Si2168B_DVBT2_MODE_PROP_LOCK_MODE_MASK) << Si2168B_DVBT2_MODE_PROP_LOCK_MODE_LSB ;
    ret = si2168b_set_property(ctx, Si2168B_DVBT2_MODE_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_DVBT2_MODE_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    return err;
}

#ifdef __MCNS__
/*****************************************************************************************
 NAME: si2168b_set_mcns_properties
  DESCRIPTION: Setup Si2168B MCNS properties configuration
  This function will download all the MCNS configuration properties.
  The function Si2168B_storeUserProperties should be called before the first call to this function.
  Parameter:  Pointer to Si2168B Context
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
  Programming Guide Reference:    MCNS setup flowchart
******************************************************************************************/
static int si2168b_set_mcns_properties(si2168b_context *ctx)
{
#ifdef __MCNS__
#ifdef    Si2168B_MCNS_ADC_CREST_FACTOR_PROP
	mcns_adc_crest_factor_crest_factor                               =   112; /* (default   112) */
#endif /* Si2168B_MCNS_ADC_CREST_FACTOR_PROP */

#ifdef    Si2168B_MCNS_AFC_RANGE_PROP
	mcns_afc_range_range_khz                                         =   100; /* (default   100) */
#endif /* Si2168B_MCNS_AFC_RANGE_PROP */

#ifdef    Si2168B_MCNS_CONSTELLATION_PROP
	mcns_constellation_constellation                                 = Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_AUTO ; /* (default 'AUTO') */
#endif /* Si2168B_MCNS_CONSTELLATION_PROP */

#ifdef    Si2168B_MCNS_SYMBOL_RATE_PROP
	mcns_symbol_rate_rate                                            =  6900; /* (default  6900) */
#endif /* Si2168B_MCNS_SYMBOL_RATE_PROP */
#endif /* __MCNS__ */

	u8 ret = NO_Si2168B_ERROR;

	SiTRACE("si2168b_set_mcns_properties\n");
#ifdef    Si2168B_MCNS_ADC_CREST_FACTOR_PROP
	ret = si2168b_set_property2(ctx, Si2168B_MCNS_ADC_CREST_FACTOR_PROP_CODE);
	if (ret != NO_Si2168B_ERROR) {
		return ERROR_Si2168B_SENDING_COMMAND;
	}
#endif /* Si2168B_MCNS_ADC_CREST_FACTOR_PROP */
#ifdef    Si2168B_MCNS_AFC_RANGE_PROP
	ret = si2168b_set_property2(ctx, Si2168B_MCNS_AFC_RANGE_PROP_CODE);
	if (ret != NO_Si2168B_ERROR) {
		return ERROR_Si2168B_SENDING_COMMAND;
	}
#endif /* Si2168B_MCNS_AFC_RANGE_PROP */
#ifdef    Si2168B_MCNS_CONSTELLATION_PROP
	ret = si2168b_set_property2(ctx, Si2168B_MCNS_CONSTELLATION_PROP_CODE);
	if (ret != NO_Si2168B_ERROR) {
		return ERROR_Si2168B_SENDING_COMMAND;
	}
#endif /* Si2168B_MCNS_CONSTELLATION_PROP */
#ifdef    Si2168B_MCNS_SYMBOL_RATE_PROP
	ret = si2168b_set_property2(ctx, Si2168B_MCNS_SYMBOL_RATE_PROP_CODE);
	if (ret != NO_Si2168B_ERROR) {
		return ERROR_Si2168B_SENDING_COMMAND;
	}
#endif /* Si2168B_MCNS_SYMBOL_RATE_PROP */
	return NO_Si2168B_ERROR;
}
#endif /* __MCNS__ */

/*****************************************************************************************
 NAME: si2168b_set_scan_properties
  DESCRIPTION: Setup Si2168B SCAN properties configuration
  This function will download all the SCAN configuration properties.
  The function Si2168B_storeUserProperties should be called before the first call to this function.
  Parameter:  Pointer to Si2168B Context
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
  Programming Guide Reference:    SCAN setup flowchart
******************************************************************************************/
static int si2168b_set_scan_properties(si2168b_context *ctx)
{
	const u16 scan_fmax_scan_fmax                   = 0; /* (default     0) */
    const u16 scan_fmin_scan_fmin                   = 0; /* (default     0) */
    const u8  scan_ien_buzien                       = Si2168B_SCAN_IEN_PROP_BUZIEN_DISABLE; /* (default 'DISABLE') */
    const u8  scan_ien_reqien                       = Si2168B_SCAN_IEN_PROP_REQIEN_DISABLE; /* (default 'DISABLE') */
    const u8  scan_int_sense_buznegen               = Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_ENABLE; /* (default 'ENABLE') */
    const u8  scan_int_sense_reqnegen               = Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_DISABLE; /* (default 'DISABLE') */
    const u8  scan_int_sense_buzposen               = Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_DISABLE; /* (default 'DISABLE') */
    const u8  scan_int_sense_reqposen               = Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_ENABLE; /* (default 'ENABLE') */
	const u16 scan_symb_rate_max_scan_symb_rate_max = 0; /* (default     0) */
    const u16 scan_symb_rate_min_scan_symb_rate_min = 0; /* (default     0) */
    /* const u8  scan_ter_config_mode                  = Si2168B_SCAN_TER_CONFIG_PROP_MODE_MAPPING_SCAN; */ /* (default 'BLIND_SCAN') */
    const u8  scan_ter_config_mode                  = Si2168B_SCAN_TER_CONFIG_PROP_MODE_BLIND_SCAN; /* (default 'BLIND_SCAN') */
    const u8  scan_ter_config_analog_bw             = Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_8MHZ; /* (default '8MHZ') */
    const u8  scan_ter_config_search_analog         = Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_DISABLE; /* (default 'DISABLE') */
    const u8  scan_ter_config_scan_debug            = 0;

#ifdef Si2168B_SCAN_SAT_CONFIG_PROP
	scan_sat_config_analog_detect                                    = Si2168B_SCAN_SAT_CONFIG_PROP_ANALOG_DETECT_DISABLED ; /* (default 'DISABLED') */
	scan_sat_config_reserved1                                        =     0; /* (default     0) */
	scan_sat_config_reserved2                                        =    12; /* (default    12) */
#endif /* Si2168B_SCAN_SAT_CONFIG_PROP */
#ifdef Si2168B_SCAN_SAT_UNICABLE_BW_PROP
	scan_sat_unicable_bw_scan_sat_unicable_bw                        =     0; /* (default     0) */
#endif /* Si2168B_SCAN_SAT_UNICABLE_BW_PROP */
#ifdef Si2168B_SCAN_SAT_UNICABLE_MIN_TUNE_STEP_PROP
	scan_sat_unicable_min_tune_step_scan_sat_unicable_min_tune_step  =    50; /* (default    50) */
#endif /* Si2168B_SCAN_SAT_UNICABLE_MIN_TUNE_STEP_PROP */

	u8  ret;
	u8  err = NO_Si2168B_ERROR;
	u16 data;

	SiTRACE("si2168b_set_scan_properties\n");

    data = (scan_fmax_scan_fmax & Si2168B_SCAN_FMAX_PROP_SCAN_FMAX_MASK) << Si2168B_SCAN_FMAX_PROP_SCAN_FMAX_LSB;
    ret = si2168b_set_property(ctx, Si2168B_SCAN_FMAX_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_SCAN_FMAX_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (scan_fmin_scan_fmin & Si2168B_SCAN_FMIN_PROP_SCAN_FMIN_MASK) << Si2168B_SCAN_FMIN_PROP_SCAN_FMIN_LSB;
    ret = si2168b_set_property(ctx, Si2168B_SCAN_FMIN_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_SCAN_FMIN_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (scan_ien_buzien & Si2168B_SCAN_IEN_PROP_BUZIEN_MASK) << Si2168B_SCAN_IEN_PROP_BUZIEN_LSB |
           (scan_ien_reqien & Si2168B_SCAN_IEN_PROP_REQIEN_MASK) << Si2168B_SCAN_IEN_PROP_REQIEN_LSB;
    ret = si2168b_set_property(ctx, Si2168B_SCAN_IEN_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_SCAN_IEN_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (scan_int_sense_buznegen & Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_MASK) << Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_LSB |
           (scan_int_sense_reqnegen & Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_MASK) << Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_LSB |
           (scan_int_sense_buzposen & Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_MASK) << Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_LSB |
           (scan_int_sense_reqposen & Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_MASK) << Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_LSB;
    ret = si2168b_set_property(ctx, Si2168B_SCAN_INT_SENSE_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_SCAN_INT_SENSE_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (scan_symb_rate_max_scan_symb_rate_max & Si2168B_SCAN_SYMB_RATE_MAX_PROP_SCAN_SYMB_RATE_MAX_MASK) << Si2168B_SCAN_SYMB_RATE_MAX_PROP_SCAN_SYMB_RATE_MAX_LSB ;
    ret = si2168b_set_property(ctx, Si2168B_SCAN_SYMB_RATE_MAX_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_SCAN_SYMB_RATE_MAX_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (scan_symb_rate_min_scan_symb_rate_min & Si2168B_SCAN_SYMB_RATE_MIN_PROP_SCAN_SYMB_RATE_MIN_MASK) << Si2168B_SCAN_SYMB_RATE_MIN_PROP_SCAN_SYMB_RATE_MIN_LSB ;
    ret = si2168b_set_property(ctx, Si2168B_SCAN_SYMB_RATE_MIN_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_SCAN_SYMB_RATE_MIN_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    data = (scan_ter_config_mode          & Si2168B_SCAN_TER_CONFIG_PROP_MODE_MASK         ) << Si2168B_SCAN_TER_CONFIG_PROP_MODE_LSB  |
           (scan_ter_config_analog_bw     & Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_MASK    ) << Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_LSB  |
           (scan_ter_config_search_analog & Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_MASK) << Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_LSB |
           (scan_ter_config_scan_debug    & Si2168B_SCAN_TER_CONFIG_PROP_SCAN_DEBUG_MASK   ) << Si2168B_SCAN_TER_CONFIG_PROP_SCAN_DEBUG_LSB ;
    ret = si2168b_set_property(ctx, Si2168B_SCAN_TER_CONFIG_PROP, data);
    if (ret != NO_Si2168B_ERROR) {
		siprintk("%s(): setting property Si2168B_SCAN_TER_CONFIG_PROP failed.\n", __func__);
		err = ERROR_Si2168B_SENDING_COMMAND;
    }

    return err;
}

static int si2168b_set_all_properties(si2168b_context *ctx)
{
	si2168b_set_common_properties(ctx);
	si2168b_set_dd_properties(ctx);
	si2168b_set_dvbc_properties(ctx);
	si2168b_set_dvbt_properties(ctx);
	si2168b_set_dvbt2_properties(ctx);
#ifdef __MCNS__
	si2168b_set_mcns_properties(ctx);
#endif /* __MCNS__ */
	si2168b_set_scan_properties(ctx);
	return 0;
}

/************************************************************************************************************************
  NAME: si2168b_configure
  DESCRIPTION: Setup TER and SAT AGCs, Common Properties startup
  Parameter:  Pointer to Si2168B Context
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
************************************************************************************************************************/
static int si2168b_configure(si2168b_context *ctx)
{
	const u8 dd_ext_agc_ter_agc_1_mode = Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_NO_CHANGE;
	const u8 dd_ext_agc_ter_agc_1_inv  = Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_INV_NOT_INVERTED;
	const u8 dd_ext_agc_ter_agc_2_mode = Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_NO_CHANGE;
	const u8 dd_ext_agc_ter_agc_2_inv  = Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_INV_NOT_INVERTED;

	Si2168B_DD_MP_DEFAULTS_CMD_REPLY_struct dd_mp_defaults;
	Si2168B_DD_EXT_AGC_TER_CMD_REPLY_struct dd_ext_agc_ter;
	Si2168B_CONFIG_PINS_CMD_REPLY_struct config_pins;
	Si2168B_DVBT2_FEF_CMD_REPLY_struct dvbt2_fef;
	Si2168B_GET_REV_CMD_REPLY_struct get_rev;
	u8 dd_mp_defaults_mp_a_mode;
	u8 dd_mp_defaults_mp_b_mode;
	u8 dd_mp_defaults_mp_c_mode;
	u8 dd_mp_defaults_mp_d_mode;
	u8 dd_ext_agc_ter_agc_1_kloop;
	u8 dd_ext_agc_ter_agc_2_kloop;
	u8 dd_ext_agc_ter_agc_1_min;
	u8 dd_ext_agc_ter_agc_2_min;
	u8 config_pins_gpio0_mode;
	u8 config_pins_gpio0_read;
	u8 config_pins_gpio1_mode;
	u8 config_pins_gpio1_read;
	u8 dvbt2_fef_tuner_flag     = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_NOT_USED;
	u8 dvbt2_fef_tuner_flag_inv = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_FEF_HIGH;
	u8 fef_pin   = 0;
	u8 fef_level = 0;

	int return_code = NO_Si2168B_ERROR;

	SiTRACE("media %d\n", ctx->media);

	return_code = si2168b_get_revision(ctx, &get_rev);
	if (return_code != NO_Si2168B_ERROR) {
		SiTRACE ("si2168b_get_revision error 0x%02x: %s\n", return_code, si2168b_error_text(return_code) );
		return return_code;
	}

	/* AGC settings when not used */
	if ( get_rev.mcm_die == Si2168B_GET_REV_RESPONSE_MCM_DIE_DIE_A) {
		dd_mp_defaults_mp_a_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_DISABLE;
		dd_mp_defaults_mp_b_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_NO_CHANGE;
		dd_mp_defaults_mp_c_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_DISABLE;
		dd_mp_defaults_mp_d_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_NO_CHANGE;
	} else if ( get_rev.mcm_die == Si2168B_GET_REV_RESPONSE_MCM_DIE_DIE_B) {
		dd_mp_defaults_mp_a_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_NO_CHANGE;
		dd_mp_defaults_mp_b_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_DISABLE;
		dd_mp_defaults_mp_c_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_NO_CHANGE;
		dd_mp_defaults_mp_d_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_DISABLE;
	} else {
		dd_mp_defaults_mp_a_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_DISABLE;
		dd_mp_defaults_mp_b_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_DISABLE;
		dd_mp_defaults_mp_c_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_DISABLE;
		dd_mp_defaults_mp_d_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_DISABLE;
	}

	/************************************************************************************************************************
	  Si2168B_L2_TER_FEF_CONFIG function
	  Use:        TER tuner FEF pin selection function
	              Used to select the FEF pin connected to the terrestrial tuner
	  Parameter:  *front_end, the front-end handle
	  Parameter:  fef_mode, a flag controlling the FEF mode between SLOW_NORMAL_AGC(0), FREEZE_PIN(1)' and SLOW_INITIAL_AGC(2)
	  Parameter:  fef_pin: where the FEF signal comes from.
	              possible modes:
	                0x0: Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_NOT_USED
	                0xA: Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_A
	                0xB: Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_B
	                0xC: Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_C
	                0xD: Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_D
	  Parameter:  fef_level, a flag controlling the FEF signal level when active between 'low'(0) and 'high'(1)
	  Returns:    1
	************************************************************************************************************************/

	switch (ctx->fef_mode) {
	default:
	case Si2168B_FEF_MODE_SLOW_NORMAL_AGC:
	case Si2168B_FEF_MODE_SLOW_INITIAL_AGC:
		fef_pin   = 0x0;
		fef_level = 0;
		break;
	case Si2168B_FEF_MODE_FREEZE_PIN:
		fef_pin   = ctx->fef_pin;
		fef_level = ctx->fef_level;
		break;
	}

	switch (fef_pin) {
	default:
	case 0x0: { dvbt2_fef_tuner_flag  = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_NOT_USED; break; }
	case 0xA: { dvbt2_fef_tuner_flag  = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_A    ; break; }
	case 0xB: { dvbt2_fef_tuner_flag  = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_B    ; break; }
	case 0xC: { dvbt2_fef_tuner_flag  = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_C    ; break; }
	case 0xD: { dvbt2_fef_tuner_flag  = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_D    ; break; }
	}

	if (fef_level == 0) {
		dvbt2_fef_tuner_flag_inv = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_FEF_LOW ;
	} else {
		dvbt2_fef_tuner_flag_inv = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_FEF_HIGH;
	}

	/*  For DVB_T2, if the TER tuner has a FEF freeze input pin, drive this pin to 0 or 1 when NOT in T2 */
	/* if FEF is active high, set the pin to 0 when NOT in T2 */
	/* if FEF is active low,  set the pin to 1 when NOT in T2 */
	if (ctx->fef_mode == Si2168B_FEF_MODE_FREEZE_PIN) {
		switch (fef_pin) {
		case Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MP_A: {
			dvbt2_fef_tuner_flag = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_A;
			if (fef_level == 1) {
				dd_mp_defaults_mp_a_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_DRIVE_0;
			} else {
				dd_mp_defaults_mp_a_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_DRIVE_1;
			}
			break;
		}
		case Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MP_B: {
			dvbt2_fef_tuner_flag = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_B;
			if (fef_level == 1) {
				dd_mp_defaults_mp_b_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_DRIVE_0;
			} else {
				dd_mp_defaults_mp_b_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_DRIVE_1;
			}
			break;
		}
		case Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MP_C: {
			dvbt2_fef_tuner_flag = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_C;
			if (fef_level == 1) {
				dd_mp_defaults_mp_c_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_DRIVE_0;
			} else {
				dd_mp_defaults_mp_c_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_DRIVE_1;
			}
			break;
		}
		case Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MP_D: {
			dvbt2_fef_tuner_flag = Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_D;
			if (fef_level == 1) {
				dd_mp_defaults_mp_d_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_DRIVE_0;
			} else {
				dd_mp_defaults_mp_d_mode = Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_DRIVE_1;
			}
			break;
		}
		default: break;
		}
	}
	/* Si2168B_L1_SendCommand2(ctx, Si2168B_DVBT2_FEF_CMD_CODE); */
	si2168b_dvbt2_fef(ctx, dvbt2_fef_tuner_flag, dvbt2_fef_tuner_flag_inv, &dvbt2_fef);
	/* Si2168B_L1_SendCommand2(ctx, Si2168B_DD_MP_DEFAULTS_CMD_CODE); */
	si2168b_dd_mp_defaults(ctx, dd_mp_defaults_mp_a_mode, dd_mp_defaults_mp_b_mode, dd_mp_defaults_mp_c_mode, dd_mp_defaults_mp_d_mode, &dd_mp_defaults);

	if (ctx->media == Si2168B_TERRESTRIAL) {
		/* TER AGC pins and inversion are previously selected using Si2168B_L2_TER_AGC */
		dd_ext_agc_ter_agc_1_kloop = Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_KLOOP_MIN;
		dd_ext_agc_ter_agc_1_min   = Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MIN_MIN;

		dd_ext_agc_ter_agc_2_kloop = 18;
		dd_ext_agc_ter_agc_2_min   = Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MIN_MIN;
		/* Si2168B_L1_SendCommand2(ctx, Si2168B_DD_EXT_AGC_TER_CMD_CODE); */
		si2168b_dd_ext_agc_ter(ctx,
				dd_ext_agc_ter_agc_1_mode,
				dd_ext_agc_ter_agc_1_inv,
				dd_ext_agc_ter_agc_2_mode,
				dd_ext_agc_ter_agc_2_inv,
				dd_ext_agc_ter_agc_1_kloop,
				dd_ext_agc_ter_agc_2_kloop,
				dd_ext_agc_ter_agc_1_min,
				dd_ext_agc_ter_agc_2_min,
				&dd_ext_agc_ter);
	}

	/* LEDS MANAGEMENT */
	/* set hardware lock on LED */
	if ( get_rev.mcm_die == Si2168B_GET_REV_RESPONSE_MCM_DIE_DIE_A) {
		config_pins_gpio0_mode = Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_NO_CHANGE;
		config_pins_gpio0_read = Si2168B_CONFIG_PINS_CMD_GPIO0_READ_DO_NOT_READ;
		config_pins_gpio1_mode = Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_HW_LOCK;
		config_pins_gpio1_read = Si2168B_CONFIG_PINS_CMD_GPIO1_READ_DO_NOT_READ;
	} else if ( get_rev.mcm_die == Si2168B_GET_REV_RESPONSE_MCM_DIE_DIE_B) {
		config_pins_gpio0_mode = Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_HW_LOCK;
		config_pins_gpio0_read = Si2168B_CONFIG_PINS_CMD_GPIO0_READ_DO_NOT_READ;
		config_pins_gpio1_mode = Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_NO_CHANGE;
		config_pins_gpio1_read = Si2168B_CONFIG_PINS_CMD_GPIO1_READ_DO_NOT_READ;
	} else {
		config_pins_gpio0_mode = Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_HW_LOCK;
		config_pins_gpio0_read = Si2168B_CONFIG_PINS_CMD_GPIO0_READ_DO_NOT_READ;
		config_pins_gpio1_mode = Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_TS_ERR;
		config_pins_gpio1_read = Si2168B_CONFIG_PINS_CMD_GPIO1_READ_DO_NOT_READ;
	}
	/* Si2168B_L1_SendCommand2(ctx, Si2168B_CONFIG_PINS_CMD_CODE); */
	si2168b_config_pins(ctx, config_pins_gpio0_mode, config_pins_gpio0_read, config_pins_gpio1_mode, config_pins_gpio1_read, &config_pins);

	/* Download properties different from 'default' */
	si2168b_set_all_properties(ctx);

#ifdef    USB_Capability
	if ( ctx->rsp->get_rev.mcm_die == Si2168B_GET_REV_RESPONSE_MCM_DIE_SINGLE) {
		/* Setting GPIF clock to not_inverted to allow TS over USB transfer */
		Si2168B_L1_DD_SET_REG(ctx, 0 , 35, 1, 0);
	}
#endif /* USB_Capability */

	return return_code;
}

/************************************************************************************************************************
  NAME: si2168b_first_init
  DESCRIPTION:Reset and Initialize Si2168B
  Parameter:  Si2168B Context (I2C address)
  Returns:    I2C transaction error code, NO_Si2168B_ERROR if successful
************************************************************************************************************************/
static int si2168b_first_init(si2168b_context *ctx)
{
    int return_code;
    SiTRACE("si2168b_first_init() starting...\n");

    if ((return_code = si2168b_power_up_with_patch(ctx)) != NO_Si2168B_ERROR) {   /* PowerUp into bootloader */
        SiTRACE ("si2168b_power_up_with_patch error 0x%02x: %s\n", return_code, si2168b_error_text(return_code) );
        return return_code;
    }
    /* At this point, FW is loaded and started.  */
    si2168b_configure(ctx);
    SiTRACE("si2168b_first_init() complete...\n");
    return NO_Si2168B_ERROR;
}

/************************************************************************************************************************
  si2168b_ter_fef_setup function
  Use:        TER tuner LPF setting function
              Used to configure the FEF mode in the terrestrial tuner
  Comments:   If the tuner is connected via the demodulator's I2C switch, enabling/disabling the i2c_passthru is required before/after tuning.
  Behavior:   This function closes the Si2168B's I2C switch then sets the TER FEF mode and finally reopens the I2C switch
  Parameter:  *front_end, the front-end handle
  Parameter:  fef, a flag controlling the selection between FEF 'off'(0) and FEF 'on'(1)
  Returns:    1
************************************************************************************************************************/
static int si2168b_ter_fef_setup(Si2168B_L2_Context *front_end, int fef)
{
	SiTRACE("si2168b_ter_fef_setup %d\n",fef);

#ifdef L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN_SETUP
	if (front_end->demod->fef_mode == Si2168B_FEF_MODE_FREEZE_PIN) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_FREEZE_PIN\n");
		/* setup now in tuner module */
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN_SETUP */

#ifdef L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP
	if (front_end->demod->fef_mode == Si2168B_FEF_MODE_SLOW_INITIAL_AGC) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_SLOW_INITIAL_AGC (AGC slowed down after tuning)\n");
		/* setup now in tuner module */
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP */

#ifdef L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC_SETUP
	if (front_end->demod->fef_mode == Si2168B_FEF_MODE_SLOW_NORMAL_AGC) {
		SiTRACE("FEF mode Si2168B_FEF_MODE_SLOW_NORMAL_AGC: AGC slowed down\n");
		/* setup now in tuner module */
	}
#endif /* L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC */

	if ( front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
		front_end->f_TER_tuner_enable(front_end->callback);
	} else {
		si2168b_tuner_i2c_enable(front_end);
	}

	si2168b_ter_fef(front_end, fef);

	if ( front_end->tuner_indirect_i2c_connection ) {  /* INDIRECT_I2C_CONNECTION? */
		front_end->f_TER_tuner_disable(front_end->callback);
	} else {
		si2168b_tuner_i2c_disable(front_end);
	}

	SiTRACE("si2168b_ter_fef_setup done\n");
	return 1;
}

/************************************************************************************************************************
  si2168b_switch_to_standard function
  Use:      Standard switching function selection
            Used to switch nicely to the wanted standard, taking into account the previous state
  Parameter: new_standard the wanted standard to switch to
  Behavior: This function positions a set of flags to easily decide what needs to be done to
              switch between standards.
************************************************************************************************************************/
static int si2168b_switch_to_standard(struct dvb_frontend *fe, u8 new_standard, u8 force_full_init)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	Si2168B_L2_Context *front_end = priv->si_front_end;

	/* previous state flags */
	int dtv_demod_already_used = 0;
	int i2c_connected          = 0;
	int ter_tuner_already_used = 0;
	int ter_clock_already_used = 0;
	/* new state flags      */
	int dtv_demod_needed       = 0;
	int ter_tuner_needed       = 0;
	int ter_clock_needed       = 0;
	int dtv_demod_sleep_request= 0;
	int res;

	u8  dd_mode_bw = 8;
	u8  dd_mode_modulation  = Si2168B_DD_MODE_PROP_MODULATION_DVBT;
	u8  dd_mode_auto_detect = Si2168B_DD_MODE_PROP_AUTO_DETECT_NONE;
	u8  dd_mode_invert_spectrum = Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_NORMAL;
	u16 data;

	SiTRACE("Si2168B_switch_to_standard %d starting... (full_init=%d)\n", new_standard, force_full_init);
	SiTRACE("starting with Si2168B_init_done %d, first_init_done     %d\n", front_end->Si2168B_init_done, front_end->first_init_done);
	SiTRACE("TER flags:    TER_init_done    %d\n", front_end->TER_init_done);

	/* In this function is called for the first time, force a full init */
	if (front_end->first_init_done == 0) {
		force_full_init = 1;
	}
	/* ------------------------------------------------------------ */
	/* Set Previous Flags                                           */
	/* Setting flags representing the previous state                */
	/* NB: Any value not matching a known standard will init as ATV */
	/* Logic applied:                                               */
	/*  dtv demod was used for TERRESTRIAL and SATELLITE reception  */
	/*  ter tuner was used for TERRESTRIAL reception                */
	/*   and for SATELLITE reception if it is the SAT clock source  */
	/*  sat tuner was used for SATELLITE reception                  */
	/*   and for TERRESTRIAL reception if it is the TER clock source*/
	/* ------------------------------------------------------------ */
	switch (front_end->previous_standard) {
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
#ifdef __MCNS__
	case Si2168B_DD_MODE_PROP_MODULATION_MCNS:
#endif /* __MCNS__ */
	case Si2168B_DD_MODE_PROP_MODULATION_DVBC: {
		dtv_demod_already_used = 1;
		ter_tuner_already_used = 1;
		if ( front_end->demod->tuner_ter_clock_source == Si2168B_TER_Tuner_clock) {
			ter_clock_already_used = 1;
		}
		break;
	}
	case Si2168B_DD_MODE_PROP_MODULATION_ANALOG: {
		ter_tuner_already_used = 1;
		break;
	}
	default : /* SLEEP */   {
		siprintk("%s(): unknown previous standard %d.\n", __func__, front_end->previous_standard);
		ter_tuner_already_used = 0;
		break;
	}
	}
	/* ------------------------------------------------------------ */
	/* Set Needed Flags                                             */
	/* Setting flags representing the new state                     */
	/* Logic applied:                                               */
	/*  dtv demod is needed for TERRESTRIAL and SATELLITE reception */
	/*  ter tuner is needed for TERRESTRIAL reception               */
	/*   and for SATELLITE reception if it is the SAT clock source  */
	/*  sat tuner is needed for SATELLITE reception                 */
	/*   and for TERRESTRIAL reception if it is the TER clock source*/
	/* ------------------------------------------------------------ */
	switch (new_standard) {
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
	case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
#ifdef __MCNS__
	case Si2168B_DD_MODE_PROP_MODULATION_MCNS:
#endif /* __MCNS__ */
	case Si2168B_DD_MODE_PROP_MODULATION_DVBC: {
		dtv_demod_needed = 1;
		ter_tuner_needed = 1;
		if ( front_end->demod->tuner_ter_clock_source == Si2168B_TER_Tuner_clock) {
			ter_clock_needed = 1;
		}
		break;
	}
	case Si2168B_DD_MODE_PROP_MODULATION_ANALOG: {
		ter_tuner_needed = 1;
		break;
	}
	default : /* SLEEP */   {
		siprintk("%s(): unknown modulation %d submitted. Entering SLEEP mode now...\n", __func__, new_standard);
		ter_tuner_needed = 0;
		break;
	}
	}
	/* ------------------------------------------------------------ */
	/* For multiple front-ends: override clock_needed flags         */
	/*  to avoid switching shared clocks                            */
	/* ------------------------------------------------------------ */
	/* For multiple front-ends: overrride clock_needed flags to avoid switching shared clocks */
	if (ter_clock_needed == 0) {
		if ( front_end->demod->tuner_ter_clock_control == Si2168B_CLOCK_ALWAYS_ON ) {
			SiTRACE("forcing ter_clock_needed = 1\n");
			ter_clock_needed = 1;
		}
	} else {
		if ( front_end->demod->tuner_ter_clock_control == Si2168B_CLOCK_ALWAYS_OFF) {
			SiTRACE("forcing ter_clock_needed = 0\n");
			ter_clock_needed = 0;
		}
	}

	/* ------------------------------------------------------------ */
	/* if 'force' flag is set, set flags to trigger a full init     */
	/* This can be used to re-init the NIM after a power cycle      */
	/*  or a HW reset                                               */
	/* ------------------------------------------------------------ */
	if (force_full_init) {
		SiTRACE("Forcing full init\n");
		/* set 'init_done' flags to force full init     */
		front_end->first_init_done     = 0;
		front_end->Si2168B_init_done    = 0;
		front_end->TER_init_done       = 0;
		/* set 'already used' flags to force full init  */
		ter_tuner_already_used = 0;
		dtv_demod_already_used = 0;
	}

	/* ------------------------------------------------------------ */
	/* Request demodulator sleep if its clock will be stopped       */
	/* ------------------------------------------------------------ */
	if ((ter_tuner_already_used == 1) & (ter_tuner_needed == 0) ) {
		SiTRACE("TER tuner 1->0 \n");
	}
	if ((ter_tuner_already_used == 0) & (ter_tuner_needed == 1) ) {
		SiTRACE("TER tuner 0->1 \n");
	}
	if ((ter_clock_already_used == 1) & (ter_clock_needed == 0) ) {
		SiTRACE("TER clock 1->0 \n");
		dtv_demod_sleep_request = 1;
	}
	if ((ter_clock_already_used == 0) & (ter_clock_needed == 1) ) {
		SiTRACE("TER clock 0->1 \n");
		dtv_demod_sleep_request = 1;
	}
	/* ------------------------------------------------------------ */
	/* Request demodulator sleep if transition from '1' to '0'      */
	/* ------------------------------------------------------------ */
	if ((dtv_demod_already_used == 1) & (dtv_demod_needed == 0) ) {
		dtv_demod_sleep_request = 1;
	}
	SiTRACE("dtv_demod_already_used %d, dtv_demod_needed %d, dtv_demod_sleep_request %d\n", dtv_demod_already_used , dtv_demod_needed, dtv_demod_sleep_request);
	/* ------------------------------------------------------------ */
	/* Sleep dtv demodulator if requested                           */
	/* ------------------------------------------------------------ */
	if (dtv_demod_sleep_request == 1) {
		SiTRACE("Sleep DTV demod\n");
		/* To avoid issues with the FEF pin when switching from T2 to ANALOG, set the demodulator for DVB-T/non auto detect reception before POWER_DOWN */
		if (new_standard == Si2168B_DD_MODE_PROP_MODULATION_ANALOG) {
			if ( ( (front_end->previous_standard == Si2168B_DD_MODE_PROP_MODULATION_DVBT  )
					& (front_end->demod->dd_mode_auto_detect == Si2168B_DD_MODE_PROP_AUTO_DETECT_AUTO_DVB_T_T2) )
					| (front_end->previous_standard == Si2168B_DD_MODE_PROP_MODULATION_DVBT2 ) ) {
				dd_mode_modulation  = Si2168B_DD_MODE_PROP_MODULATION_DVBT;
				dd_mode_auto_detect = Si2168B_DD_MODE_PROP_AUTO_DETECT_NONE;
			    data = (dd_mode_bw              & Si2168B_DD_MODE_PROP_BW_MASK             ) << Si2168B_DD_MODE_PROP_BW_LSB |
			           (dd_mode_modulation      & Si2168B_DD_MODE_PROP_MODULATION_MASK     ) << Si2168B_DD_MODE_PROP_MODULATION_LSB |
			           (dd_mode_invert_spectrum & Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_LSB |
			           (dd_mode_auto_detect     & Si2168B_DD_MODE_PROP_AUTO_DETECT_MASK    ) << Si2168B_DD_MODE_PROP_AUTO_DETECT_LSB;
				si2168b_set_property(front_end->demod, Si2168B_DD_MODE_PROP, data);
				si2168b_dd_restart(front_end->demod);
			}
		}
		si2168b_standby(front_end->demod);
	}

	/* ------------------------------------------------------------ */
	/* Set media for new standard                                   */
	/* ------------------------------------------------------------ */
	front_end->demod->dd_mode_modulation = new_standard;
	front_end->demod->media = si2168b_media(front_end->demod);

	/* ------------------------------------------------------------ */
	/* Allow i2c traffic to reach the tuners                        */
	/* ------------------------------------------------------------ */
	if ( front_end->tuner_indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
		/* Connection will be done later on, depending on TER/SAT */
	} else {
		SiTRACE("Connect tuners i2c\n");
		si2168b_tuner_i2c_enable(front_end);
		i2c_connected = 1;
	}
	/* ------------------------------------------------------------ */
	/* Sleep Ter Tuner                                              */
	/* Sleep terrestrial tuner  if transition from '1' to '0'       */
	/* ------------------------------------------------------------ */
	if ((ter_tuner_already_used == 1) & (ter_tuner_needed == 0) ) {
		if ( front_end->tuner_indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
			if (i2c_connected==0) {
				SiTRACE("-- I2C -- Connect TER tuner i2c to sleep it\n");
				front_end->f_TER_tuner_enable(front_end->callback);
				i2c_connected++;
			}
		}
		SiTRACE("Sleep terrestrial tuner\n");
		if (fe->ops.tuner_ops.sleep) {
			res = fe->ops.tuner_ops.sleep(fe);
			if (res) {
				SiTRACE("Terrestrial tuner sleep error!\n");
			}
		} else {
			SiTRACE("WARNING: sleep() not available\n");
		}
	}

	/* ------------------------------------------------------------ */
	/* Wakeup Ter Tuner                                             */
	/* Wake up terrestrial tuner if transition from '0' to '1'      */
	/* ------------------------------------------------------------ */
	if ((ter_tuner_already_used == 0) & (ter_tuner_needed == 1)) {
		if ( front_end->tuner_indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
			if (i2c_connected==0) {
				SiTRACE("-- I2C -- Connect TER tuner i2c to init/wakeup it\n");
				front_end->f_TER_tuner_enable(front_end->callback);
				i2c_connected++;
			}
		}
		if (fe->ops.tuner_ops.init) {
			res = fe->ops.tuner_ops.init(fe);
			if (res) {
				SiTRACE("Terrestrial tuner init error!\n");
			}
		} else {
			SiTRACE("WARNING: init() not available\n");
		}
	}

	if ((front_end->previous_standard != new_standard) & (dtv_demod_needed == 0) & (front_end->demod->media == Si2168B_TERRESTRIAL)) {
		if (front_end->demod->media == Si2168B_TERRESTRIAL) {
#ifdef    TER_TUNER_ATV_LO_INJECTION
			TER_TUNER_ATV_LO_INJECTION(front_end->tuner_ter);
#endif /* TER_TUNER_ATV_LO_INJECTION */

		}
	}
	/* ------------------------------------------------------------ */
	/* Change Dtv Demod standard if required                        */
	/* ------------------------------------------------------------ */
	if ((front_end->previous_standard != new_standard) & (dtv_demod_needed == 1)) {
		SiTRACE("Store demod standard (%d)\n", new_standard);
		front_end->standard = new_standard;
		/* Set flag to trigger Si2168B init or re_init, to complete    */
		/*  the standard change                                       */
		dtv_demod_already_used = 0;
		if (front_end->demod->media == Si2168B_TERRESTRIAL) {
#ifdef    TER_TUNER_DTV_LO_INJECTION
			TER_TUNER_DTV_LO_INJECTION(front_end->tuner_ter);
#endif /* TER_TUNER_DTV_LO_INJECTION */
#ifdef    TER_TUNER_DTV_LIF_OUT_AMP
			/* Adjusting LIF signal for cable or terrestrial reception */
			switch (new_standard) {
			case Si2168B_DD_MODE_PROP_MODULATION_DVBT:
			case Si2168B_DD_MODE_PROP_MODULATION_DVBT2:
			{
				TER_TUNER_DTV_LIF_OUT_AMP(front_end->tuner_ter, 0);
				break;
			}
#ifdef __MCNS__
			case Si2168B_DD_MODE_PROP_MODULATION_MCNS:
#endif /* __MCNS__ */
			case Si2168B_DD_MODE_PROP_MODULATION_DVBC: {
				TER_TUNER_DTV_LIF_OUT_AMP(front_end->tuner_ter, 1);
				break;
			}
			default: break;
			}
#endif /* TER_TUNER_DTV_LIF_OUT_AMP */
		}
	}
	/* ------------------------------------------------------------ */
	/* Wakeup Dtv Demod                                             */
	/*  if it has been put in 'standby mode' and is needed          */
	/* ------------------------------------------------------------ */
	if (front_end->Si2168B_init_done) {
		SiTRACE("dtv_demod_sleep_request %d\n",dtv_demod_sleep_request);
		if ((dtv_demod_sleep_request == 1) & (dtv_demod_needed == 1) ) {
			SiTRACE("Wake UP DTV demod\n");
			if (si2168b_wakeup (front_end->demod) == NO_Si2168B_ERROR) {
				SiTRACE("Wake UP DTV demod OK\n");
				front_end->demod->Si2168B_in_standby = 0;
			} else {
				SiTRACE("Wake UP DTV demod failed!\n");
				return 0;
			}
		}
	}
	/* ------------------------------------------------------------ */
	/* Setup Dtv Demod                                              */
	/* Setup dtv demodulator if transition from '0' to '1'          */
	/* ------------------------------------------------------------ */
	if ((dtv_demod_already_used == 0) & (dtv_demod_needed == 1)) {
		/* Do the 'first init' only the first time, plus if requested  */
		/* (when 'force' flag is 1, Si2168B_init_done is set to '0')   */
		if (!front_end->Si2168B_init_done) {
			SiTRACE("Init demod\n");
			if (si2168b_first_init(front_end->demod) == NO_Si2168B_ERROR) {
				front_end->Si2168B_init_done = 1;
				SiTRACE("Demod init OK\n");
			} else {
				SiTRACE("Demod init failed!\n");
				return 0;
			}
		}
		if (front_end->demod->media == Si2168B_TERRESTRIAL) {
			SiTRACE("front_end->demod->media Si2168B_TERRESTRIAL\n");
			if (front_end->TER_init_done == 0) {
				SiTRACE("Configure demod for TER\n");
				if (si2168b_configure(front_end->demod) == NO_Si2168B_ERROR) {
					/* set dd_mode.modulation again, as it is overwritten by si2168b_configure */
					front_end->demod->dd_mode_modulation = new_standard;
					front_end->TER_init_done = 1;
				} else {
					SiTRACE("Demod TER configuration failed !\n");
					return 0;
				}
			}
			/* ------------------------------------------------------------ */
			/* Manage FEF mode in TER tuner                                 */
			/* ------------------------------------------------------------ */
			if (new_standard == Si2168B_DD_MODE_PROP_MODULATION_DVBT2) {
				si2168b_ter_fef_setup(front_end, 1);
			} else {
				si2168b_ter_fef_setup(front_end, 0);
			}
		}
		dd_mode_invert_spectrum = si2168b_set_invert_spectrum(front_end);
	    data = (dd_mode_bw              & Si2168B_DD_MODE_PROP_BW_MASK             ) << Si2168B_DD_MODE_PROP_BW_LSB |
	           (dd_mode_modulation      & Si2168B_DD_MODE_PROP_MODULATION_MASK     ) << Si2168B_DD_MODE_PROP_MODULATION_LSB |
	           (dd_mode_invert_spectrum & Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_MASK) << Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_LSB |
	           (dd_mode_auto_detect     & Si2168B_DD_MODE_PROP_AUTO_DETECT_MASK    ) << Si2168B_DD_MODE_PROP_AUTO_DETECT_LSB;
		if (si2168b_set_property(front_end->demod, Si2168B_DD_MODE_PROP, data)==0) {
			si2168b_dd_restart(front_end->demod);
		} else {
			SiTRACE("Demod restart failed !\n");
			return 0;
		}
	}

	/* ------------------------------------------------------------ */
	/* Forbid i2c traffic to reach the tuners                       */
	/* ------------------------------------------------------------ */
	if ( front_end->tuner_indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
		if (i2c_connected) {
			SiTRACE("-- I2C -- Disconnect TER tuner i2c\n");
			front_end->f_TER_tuner_disable(front_end->callback);
		}
	} else {
		if (i2c_connected) {
			SiTRACE("Disconnect tuners i2c\n");
			si2168b_tuner_i2c_disable(front_end);
		}
	}
	/* ------------------------------------------------------------ */
	/* update value of previous_standard to prepare next call       */
	/* ------------------------------------------------------------ */
	front_end->previous_standard = new_standard;
	front_end->standard          = new_standard;

	front_end->first_init_done = 1;

	SiTRACE("Si2168B_switch_to_standard complete\n");
	return 1;
}

static int si2168b_initialize(struct dvb_frontend *fe)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;

	siprintk("((((((((((((((((((((((((((((((((((((((((((((((((((((((((((\n");
	siprintk("(((                  %s()              )))\n", __func__);
	siprintk("))))))))))))))))))))))))))))))))))))))))))))))))))))))))))\n");

	if (!si2168b_switch_to_standard(fe, Si2168B_DD_MODE_PROP_MODULATION_DVBT, 1)) {
		printk(KERN_ERR "%s(): si2168b_switch_to_standard() failed.\n", __func__);
		goto err;
	}

	/* hardware related settings from base driver */
	siprintk("%s(): ts_clk_invert=%d\n", __func__, priv->config->ts_par_clk_invert);
	siprintk("%s(): ts_clk_shift=%d\n", __func__,  priv->config->ts_par_clk_shift);
	siprintk("%s(): clock=%d\n", __func__,         priv->config->ts_clock_mode);
	siprintk("%s(): clk_gapped_en=%d\n", __func__, priv->config->clk_gapped_en);
	siprintk("%s(): fef_mode=%d\n", __func__,      priv->config->fef_mode);

	if (si2168b_ts_bus_ctrl(fe, 1)) { /* set the bus mode */
		printk(KERN_ERR "%s(): si2168b_ts_bus_ctrl(1) failed\n", __func__);
		/* goto err; */
	}
	return 0;

err:
	printk(KERN_ERR "%s(): failed\n", __func__);
	return -ENODEV;
}

static int si2168b_dvbc_auto_tune(struct dvb_frontend *fe, si2168b_context *demod, struct dtv_frontend_properties *p, u32 freq, int lock_start_ms)
{
	Si2168B_DVBC_STATUS_CMD_REPLY_struct dvbc_status;
	Si2168B_CHANNEL_SEEK_PARAM_struct seek_param = {
			.rangeMin     = freq,
			.rangeMax     = freq,
			.seekBWHz     = 8000000,
			.seekStepHz   = 0,
			.minSRbps     = 870000,
			.maxSRbps     = 7500000,
			/*.minRSSIdBm   = 0,*/
			/*.maxRSSIdBm   = 0,*/
			/*.minSNRHalfdB = 0,*/
			/*.maxSNRHalfdB = 0,*/
	};
	Si2168B_CHANNEL_SEEK_NEXT_REPLY_struct channel_status;
	int locked = 0;
    int cnr = 0;

	siprintk("%s(): +++ DVB-C AUTO TUNING (QAM AND SYMBOL RATE DETECTION) +++\n", __func__);

	if (si2168b_channel_seek_init(fe, &seek_param)) {
		pr_err("si2168b_channel_seek_init() failed.\n");
	}
	si2168b_set_tuner_params(fe, freq); /* tune first */

    locked = si2168b_channel_seek_next(fe, &seek_param, &channel_status);

    if (locked == 1) {
		pr_info("%s(): Channel detected(standard %s, f=%d SR=%d constel=%d BW=%u)\n", __func__,
				si2168b_standard_name(channel_status.standard),
				channel_status.freq,
				channel_status.symbol_rate_bps,
				channel_status.constellation,
				channel_status.bandwidth_Hz);
		p->symbol_rate = channel_status.symbol_rate_bps;
		switch (channel_status.constellation) {
		case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_AUTO:   p->modulation = QAM_AUTO; break;
		case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM128: p->modulation = QAM_128;  break;
		case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM16:  p->modulation = QAM_16;   break;
		case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM256: p->modulation = QAM_256;  break;
		case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM32:  p->modulation = QAM_32;   break;
		case Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM64:  p->modulation = QAM_64;   break;
		default:
			/* invalid constellation returned */
			break;
		}
		msleep(150);

		if (si2168b_dvbc_status(demod, Si2168B_DVBC_STATUS_CMD_INTACK_OK, &dvbc_status) == NO_Si2168B_ERROR) {
			cnr = dvbc_status.cnr / 4;
		} else {
			siprintk("%s(): si2168b_dvbc_status() failed\n", __func__);
		}

		while (cnr < 1) {
			if (si2168b_dvbc_status(demod, Si2168B_DVBC_STATUS_CMD_INTACK_OK, &dvbc_status) == NO_Si2168B_ERROR) {
				cnr = dvbc_status.cnr / 4;
			} else {
				siprintk("%s(): si2168b_dvbc_status() failed\n", __func__);
			}
			if (system_time() - lock_start_ms > 1000){
				break;
			}
			msleep(150);
		}
		siprintk("%s(): C/N %u dB (%u ms)\n", __func__, cnr, (system_time() - lock_start_ms));
	} else {
		siprintk("%s(): Channel not locked (locked=%d)\n", __func__, locked);
	}
	si2168b_channel_seek_end(fe);
	return locked;
}

static int si2168b_set_frontend(struct dvb_frontend *fe)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
    Si2168B_L2_Context *front_end = priv->si_front_end;
	si2168b_context *demod = priv->si_front_end->demod;
	struct dtv_frontend_properties *p = &fe->dtv_property_cache;

	u8  standard = 0;
	u32 freq = p->frequency;
	u32 dvb_t_bandwidth_hz = p->bandwidth_hz;
	u8  dvb_t_stream = 0;
	u32 symbol_rate_bps = 0;
	u8  dvb_c_constellation = 0;
	int plp_id = (int)p->stream_id;
	u8  T2_lock_mode = 0;
	int locked = 0;
	u8  force_full_init = 0;
    int lock_start_ms;

    if (plp_id == 0)
    	plp_id = -1; /* set to auto detection */

	siprintk("%s(): FE_SET_FRONTEND f=%u inv=%d mod=%d bw=%u plp_id=%d\n", __func__, p->frequency, p->inversion, p->modulation, p->bandwidth_hz, plp_id);

	switch (p->delivery_system) {
	case SYS_DVBT:         standard = Si2168B_DD_MODE_PROP_MODULATION_DVBT; break;
	case SYS_DVBT2:	       standard = Si2168B_DD_MODE_PROP_MODULATION_DVBT2; break;
	case SYS_DVBC_ANNEX_A: standard = Si2168B_DD_MODE_PROP_MODULATION_DVBC;
		symbol_rate_bps = p->symbol_rate;
		switch (p->modulation) {
		case QAM_AUTO: dvb_c_constellation = Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_AUTO;   break;
		case QAM_128:  dvb_c_constellation = Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM128; break;
		case QAM_16:   dvb_c_constellation = Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM16;  break;
		case QAM_256:  dvb_c_constellation = Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM256; break;
		case QAM_32:   dvb_c_constellation = Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM32;  break;
		case QAM_64:   dvb_c_constellation = Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM64;  break;
		default:       dvb_c_constellation = Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_AUTO;   break;
		}
		break;
	default:
		siprintk("%s : ERROR: delivery system not supported\n", __func__);
		return -EINVAL;
	}

	if (standard != front_end->demod->dd_mode_modulation) {
		siprintk("%s(): switching from standard %s to %s\n", __func__, si2168b_standard_name(front_end->demod->dd_mode_modulation), si2168b_standard_name(standard));
		if (!si2168b_switch_to_standard(fe, standard, force_full_init)) {
			siprintk("%s(): si2168b_switch_to_standard() failed.\n", __func__);
			return -ENODEV;
		}
		priv->delivery_system = p->delivery_system;
	}

	if (standard == Si2168B_DD_MODE_PROP_MODULATION_DVBC) {
		demod->dvbc_symbol_rate = symbol_rate_bps / 1000;
	} else {
		demod->dvbc_symbol_rate = 0; /* clear old setting */
	}

	siprintk("%s(): delsys=%s  Si2168B_standard=%s  freq=%d  symbol_rate_bps=%d  dvb_c_constellation=%d\n",
			__func__, delsys_name(p->delivery_system), si2168b_standard_name(front_end->demod->dd_mode_modulation),
			freq, symbol_rate_bps, dvb_c_constellation);

	lock_start_ms = system_time();

	if (standard == Si2168B_DD_MODE_PROP_MODULATION_DVBC && symbol_rate_bps == 7501000) { /* 7501000 = AUTO */
		locked = si2168b_dvbc_auto_tune(fe, demod, p, freq, lock_start_ms);
	} else {
		locked = si2168b_lock_to_carrier(fe, standard, freq,
				dvb_t_bandwidth_hz, dvb_t_stream, symbol_rate_bps,
				dvb_c_constellation, plp_id, T2_lock_mode);
	}

	if (locked == 1) {
		siprintk("%s(): locked to carrier %dHz (duration=%dms)\n", __func__, freq, system_time()-lock_start_ms);
		msleep(100);

		if (priv->config->start_ctrl)
			priv->config->start_ctrl(fe);
	} else {
		siprintk("%s(): not locked to carrier %dHz (duration=%dms)\n", __func__, freq, system_time()-lock_start_ms);
	}

	/* After set_frontend, stats aren't available */
	p->strength.stat[0].scale = FE_SCALE_RELATIVE;
	p->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->block_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->block_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->pre_bit_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->pre_bit_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->post_bit_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->post_bit_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;

	return 0;
}

static int si2168b_get_tune_settings(struct dvb_frontend *fe,
		struct dvb_frontend_tune_settings
		*fe_tune_settings)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;

	fe_tune_settings->min_delay_ms = priv->config->min_delay_ms;
	siprintk("%s(): min delay = %dms\n", __func__, fe_tune_settings->min_delay_ms);
	return 0;
}

#if defined(FE_READ_STREAM_IDS) || defined(__MPLP_TEST__)
static int si2168b_read_stream_ids(struct dvb_frontend *fe, struct dvb_stream_ids* ids)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	si2168b_context *demod = priv->si_front_end->demod;
	Si2168B_DD_STATUS_CMD_REPLY_struct      dd_status;
	Si2168B_DVBT2_STATUS_CMD_REPLY_struct   dvbt2_status;
	Si2168B_DVBT2_PLP_INFO_CMD_REPLY_struct dvbt2_plp_info;
	int data_plp_count;
	int num_plp;
	int plp_index;
	int plp_id;
	int plp_type;
	int ret = 0;

	if (priv->delivery_system != SYS_DVBT2) {
		siprintk("%s(): delivery system is not DVB-T2. Aborting...\n", __func__);
	}

	/* check demod */
	if (si2168b_dd_status(demod, Si2168B_DD_STATUS_CMD_INTACK_OK, &dd_status) != NO_Si2168B_ERROR) {
		siprintk("%s(): si2168b_dd_status() failed\n", __func__);
		return -1;
	}

	if (dd_status.modulation != Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBT2) {
		siprintk("%s(): demod modulation is not DVB-T2. Aborting...\n", __func__);
		return -1;
	}

	if (si2168b_dvbt2_status(demod, Si2168B_DVBT2_STATUS_CMD_INTACK_OK, &dvbt2_status) != NO_Si2168B_ERROR) {
		siprintk("%s(): si2168b_dvbt2_status() failed\n", __func__);
		return -1;
	}

	SiTRACE("dvbt2_status.pcl %d\n", dvbt2_status.pcl);

	num_plp = dvbt2_status.num_plp;
	plp_id  = dvbt2_status.plp_id;

	SiTRACE("There are %d PLPs in this stream\n", num_plp);
	SiTRACE("PLP ID = %d\n", plp_id);

	/* hard limiter */
	if (num_plp > 255) {
		siprintk("%s(): WARNING: limiting found %d PLPs to 255\n", __func__, num_plp);
		num_plp = 255;
	}

	ids->num = num_plp; /* number of valid stream ids in 'val' */
	ids->cur = plp_id;  /* currently selected stream id */

	data_plp_count = 0;
	dvbt2_plp_info.plp_id = 0;
	dvbt2_plp_info.plp_type = 3;

	for (plp_index=0; plp_index<num_plp; plp_index++) {
		if (si2168b_dvbt2_plp_info(demod, plp_index, &dvbt2_plp_info) ==  NO_Si2168B_ERROR) {
			plp_id   = dvbt2_plp_info.plp_id;
			plp_type = dvbt2_plp_info.plp_type;

			ids->val[plp_index] = plp_id;

			SiTRACE("PLP index %3d: PLP ID %3d, PLP TYPE %d : ", plp_index, plp_id, plp_type);
			if (plp_type == Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_TYPE_COMMON) {
				SiTRACE("COMMON PLP at index %3d: PLP ID %3d, PLP TYPE COMMON\n", plp_index, plp_id);
			} else {
				SiTRACE("DATA   PLP at index %3d: PLP ID %3d, PLP TYPE %d\n", plp_index, plp_id, plp_type);
				data_plp_count++;
			}
		} else {
			siprintk("%s(): si2168b_dvbt2_plp_info() index %d failed\n", __func__, plp_index);
			ids->val[plp_index] = 0;
			ret = -1;
		}
	}
	SiTRACE("data_plp_count=%d\n", data_plp_count);
	return ret;
}
#endif /* FE_READ_STREAM_IDS */

static int si2168b_get_frontend(struct dvb_frontend *fe)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *p = &fe->dtv_property_cache;
	si2168b_context *demod = priv->si_front_end->demod;
	Si2168B_DD_STATUS_CMD_REPLY_struct    dd_status;
	Si2168B_DVBT_STATUS_CMD_REPLY_struct  dvbt_status;
	Si2168B_DVBT2_STATUS_CMD_REPLY_struct dvbt2_status;
	Si2168B_DVBC_STATUS_CMD_REPLY_struct  dvbc_status;

#if defined(FE_READ_STREAM_IDS) || defined(__MPLP_TEST__)
	struct dvb_stream_ids ids;
#endif

    if (priv->delivery_system == SYS_UNDEFINED)
		return -EINVAL;

	if (si2168b_dd_status(demod, Si2168B_DD_STATUS_CMD_INTACK_OK, &dd_status) != NO_Si2168B_ERROR) {
		siprintk("%s(): si2168b_dd_status() failed\n", __func__);
		return -EFAULT;
	}

    if (dd_status.dl == Si2168B_DD_STATUS_RESPONSE_DL_LOCKED) {
		siprintk("%s(): FE HAS LOCK\n", __func__);
    }

    switch (dd_status.modulation) {
	case Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBT:
		siprintk("%s() delivery system SYS_DVBT\n", __func__);
		if (si2168b_dvbt_status(demod, Si2168B_DVBT_STATUS_CMD_INTACK_OK, &dvbt_status) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_dvbt_status() failed\n", __func__);
			return -EFAULT;
		}
		switch (dvbt_status.constellation) {
		case Si2168B_DVBT_STATUS_RESPONSE_CONSTELLATION_QAM16: p->modulation = QAM_16; break;
		case Si2168B_DVBT_STATUS_RESPONSE_CONSTELLATION_QAM64: p->modulation = QAM_64; break;
		case Si2168B_DVBT_STATUS_RESPONSE_CONSTELLATION_QPSK:  p->modulation = QPSK;   break;
		default:
			siprintk("%s(): invalid DVB-T constellation returned\n", __func__);
		}
		switch (dvbt_status.sp_inv) {
		case Si2168B_DVBT_STATUS_RESPONSE_SP_INV_INVERTED: p->inversion = INVERSION_ON;	 break;
		case Si2168B_DVBT_STATUS_RESPONSE_SP_INV_NORMAL:   p->inversion = INVERSION_OFF; break;
		default:
			siprintk("%s(): invalid DVB-T sp_inv returned\n", __func__);
		}
		switch (dvbt_status.rate_hp) {
		case Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_1_2: p->code_rate_HP = FEC_1_2; break;
		case Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_2_3: p->code_rate_HP = FEC_2_3; break;
		case Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_3_4: p->code_rate_HP = FEC_3_4; break;
		case Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_5_6: p->code_rate_HP = FEC_5_6; break;
		case Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_7_8: p->code_rate_HP = FEC_7_8; break;
		default:
			siprintk("%s(): invalid DVB-T rate_hp returned\n", __func__);
		}
		switch (dvbt_status.rate_lp) {
		case Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_1_2: p->code_rate_LP = FEC_1_2; break;
		case Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_2_3: p->code_rate_LP = FEC_2_3; break;
		case Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_3_4: p->code_rate_LP = FEC_3_4; break;
		case Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_5_6: p->code_rate_LP = FEC_5_6; break;
		case Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_7_8: p->code_rate_LP = FEC_7_8; break;
		default:
			siprintk("%s(): invalid DVB-T rate_lp returned\n", __func__);
		}
		switch (dvbt_status.fft_mode) {
		case Si2168B_DVBT_STATUS_RESPONSE_FFT_MODE_2K: p->transmission_mode = TRANSMISSION_MODE_2K; break;
		case Si2168B_DVBT_STATUS_RESPONSE_FFT_MODE_4K: p->transmission_mode = TRANSMISSION_MODE_4K; break;
		case Si2168B_DVBT_STATUS_RESPONSE_FFT_MODE_8K: p->transmission_mode = TRANSMISSION_MODE_8K; break;
		default:
			siprintk("%s(): invalid DVB-T fft_mode returned\n", __func__);
		}
		switch (dvbt_status.guard_int) {
		case Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_1_16: p->guard_interval = GUARD_INTERVAL_1_16; break;
		case Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_1_32: p->guard_interval = GUARD_INTERVAL_1_32; break;
		case Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_1_4:  p->guard_interval = GUARD_INTERVAL_1_4;  break;
		case Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_1_8:  p->guard_interval = GUARD_INTERVAL_1_8;  break;
		default:
			siprintk("%s(): invalid DVB-T guard_int returned\n", __func__);
		}
		switch (dvbt_status.hierarchy) {
		case Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_ALFA1: p->hierarchy = HIERARCHY_1;    break;
		case Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_ALFA2: p->hierarchy = HIERARCHY_2;    break;
		case Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_ALFA4: p->hierarchy = HIERARCHY_4;    break;
		case Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_NONE:  p->hierarchy = HIERARCHY_NONE; break;
		default:
			siprintk("%s(): invalid DVB-T hierarchy returned\n", __func__);
		}
		p->fec_inner = FEC_NONE;
		break;
	case Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBT2:
		siprintk("%s() delivery system SYS_DVBT2\n", __func__);

		if (priv->delivery_system != SYS_DVBT2) {
			siprintk("%s(): delivery system changed from %s to SYS_DVBT2\n", __func__, delsys_name(p->delivery_system));
			p->delivery_system = SYS_DVBT2;
		}

		if (si2168b_dvbt2_status(demod, Si2168B_DVBT2_STATUS_CMD_INTACK_OK, &dvbt2_status) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_dvbt2_status() failed\n", __func__);
			return -EFAULT;
		}
		switch (dvbt2_status.constellation) {
		case Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QAM128: p->modulation = QAM_128; break;
		case Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QAM16:  p->modulation = QAM_16;  break;
		case Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QAM256: p->modulation = QAM_256; break;
		case Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QAM32:  p->modulation = QAM_32;  break;
		case Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QAM64:  p->modulation = QAM_64;  break;
		case Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QPSK:   p->modulation = QPSK;    break;
		default:
			siprintk("%s(): invalid DVB-T2 constellation returned\n", __func__);
		}
		switch (dvbt2_status.sp_inv) {
		case Si2168B_DVBT2_STATUS_RESPONSE_SP_INV_INVERTED: p->inversion = INVERSION_ON;  break;
		case Si2168B_DVBT2_STATUS_RESPONSE_SP_INV_NORMAL:   p->inversion = INVERSION_OFF; break;
		default:
			siprintk("%s(): invalid DVB-T2 sp_inv returned\n", __func__);
		}
		switch (dvbt2_status.fft_mode) {
		case Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_16K: p->transmission_mode = TRANSMISSION_MODE_16K; break;
		case Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_1K:  p->transmission_mode = TRANSMISSION_MODE_1K;  break;
		case Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_2K:  p->transmission_mode = TRANSMISSION_MODE_2K;  break;
		case Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_32K: p->transmission_mode = TRANSMISSION_MODE_32K; break;
		case Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_4K:  p->transmission_mode = TRANSMISSION_MODE_4K;  break;
		case Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_8K:  p->transmission_mode = TRANSMISSION_MODE_8K;  break;
		default:
			siprintk("%s(): invalid DVB-T2 fft_mode returned\n", __func__);
		}
		switch (dvbt2_status.guard_int) {
		case Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_19_128: p->guard_interval = GUARD_INTERVAL_19_128; break;
		case Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_19_256: p->guard_interval = GUARD_INTERVAL_19_256; break;
		case Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_1_128:  p->guard_interval = GUARD_INTERVAL_1_128;  break;
		case Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_1_16:   p->guard_interval = GUARD_INTERVAL_1_16;   break;
		case Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_1_32:   p->guard_interval = GUARD_INTERVAL_1_32;   break;
		case Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_1_4:    p->guard_interval = GUARD_INTERVAL_1_4;    break;
		case Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_1_8:    p->guard_interval = GUARD_INTERVAL_1_8;    break;
		default:
			siprintk("%s(): invalid DVB-T2 guard_int returned\n", __func__);
		}
		p->fec_inner = FEC_NONE;
		p->code_rate_HP = FEC_NONE;
		p->code_rate_LP = FEC_NONE;
		p->hierarchy = HIERARCHY_NONE;
		p->stream_id = dvbt2_status.plp_id;

#if defined(FE_READ_STREAM_IDS) || defined(__MPLP_TEST__)
		si2168b_read_stream_ids(fe, &ids);
#endif
		break;
	case Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBC:
		if (priv->delivery_system != SYS_DVBC_ANNEX_A) {
			siprintk("%s(): delivery system changed from %s to SYS_DVBC_ANNEX_A\n", __func__, delsys_name(p->delivery_system));
			p->delivery_system = SYS_DVBC_ANNEX_A;
		} else {
			siprintk("%s() delivery system SYS_DVBC_ANNEX_A\n", __func__);
		}
		if (si2168b_dvbc_status(demod, Si2168B_DVBC_STATUS_CMD_INTACK_OK, &dvbc_status) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_dvbc_status() failed\n", __func__);
			return -EFAULT;
		}
		switch (dvbc_status.constellation) {
		case Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_QAM128: p->modulation = QAM_128; break;
		case Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_QAM16:  p->modulation = QAM_16;  break;
		case Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_QAM256: p->modulation = QAM_256; break;
		case Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_QAM32:  p->modulation = QAM_32;  break;
		case Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_QAM64:  p->modulation = QAM_64;  break;
		default:
			siprintk("%s(): invalid DVB-C constellation returned\n", __func__);
		}
		switch (dvbc_status.sp_inv) {
		case Si2168B_DVBC_STATUS_RESPONSE_SP_INV_INVERTED: p->inversion = INVERSION_ON;  break;
		case Si2168B_DVBC_STATUS_RESPONSE_SP_INV_NORMAL:   p->inversion = INVERSION_OFF; break;
		default:
			siprintk("%s(): invalid DVB-C sp_inv returned\n", __func__);
		}
		p->symbol_rate = demod->dvbc_symbol_rate * 1000;
		p->fec_inner = FEC_NONE;
		break;
	default:
		siprintk("%s() delivery system not supported\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static int si2168b_sleep(struct dvb_frontend *fe)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	Si2168B_L2_Context *front_end = priv->si_front_end;

#if 1 /* FGR - BUGBUG try alternate power down */
	siprintk("%s()\n", __func__);

	if (si2168b_ts_bus_ctrl(fe, 0)) { /* set the bus mode */
		printk(KERN_ERR "%s(): si2168b_ts_bus_ctrl(0) failed\n", __func__);
	}

	if (!si2168b_switch_to_standard(fe, Si2168B_DD_MODE_PROP_MODULATION_SLEEP, 0)) {
		printk(KERN_ERR "%s(): si2168b_switch_to_standard() failed.\n", __func__);
		return -EIO;
	}

	front_end->demod->Si2168B_in_standby = 1;

#else /* FGR */
	si2168b_context *demod = priv->si_front_end->demod;

	siprintk("%s()\n", __func__);

	if (si2168b_ts_bus_ctrl(fe, 0)) { /* set the bus mode */
		printk(KERN_ERR "%s(): si2168b_ts_bus_ctrl(0) failed\n", __func__);
	}

	if ( priv->config->indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
		if (priv->si_front_end->f_TER_tuner_enable(priv->si_front_end->callback) != NO_Si2168B_ERROR) {
			siprintk("%s(): f_TER_tuner_enable() failed\n", __func__);
			return -EIO;
		}
	} else {
		if (si2168b_tuner_i2c_enable(priv->si_front_end) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_tuner_i2c_enable() failed\n", __func__);
			return -EIO;
		}
	}

	/* power down tuner */
	/* ToDo */
	/* ... */

	if ( priv->config->indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
		if (priv->si_front_end->f_TER_tuner_disable(priv->si_front_end->callback) != NO_Si2168B_ERROR) {
			siprintk("%s(): f_TER_tuner_disable() failed\n", __func__);
			return -EIO;
		}
	} else {
		if (si2168b_tuner_i2c_disable(priv->si_front_end) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_tuner_i2c_disable() failed\n", __func__);
			return -EIO;
		}
	}

	if (si2168b_power_down(demod) != NO_Si2168B_ERROR) {
		siprintk("%s(): si2168b_power_down() failed\n", __func__);
		return -EIO;
	}
	front_end->demod->Si2168B_in_standby = 1;
#endif /* FGR */

	return 0;
}

static void si2168b_release(struct dvb_frontend *fe)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;

	siprintk("%s\n", __func__);

	if (priv->si_front_end->demod)
		kfree(priv->si_front_end->demod);

	if (priv->si_front_end)
		kfree(priv->si_front_end);

	if (priv)
		kfree(priv);
}

static enum dvbfe_algo si2168b_get_frontend_algo(struct dvb_frontend *fe)
{
#if 1
	/*siprintk("%s(): DVBFE_ALGO_HW\n", __func__);*/
	return DVBFE_ALGO_HW;      /* = si2168b_tune()          */
#endif
#if 0
	siprintk("%s(): DVBFE_ALGO_SW\n", __func__);
	return DVBFE_ALGO_SW;      /* = dvb_frontend_swzigzag() */
#endif
#if 0
	siprintk("%s(): DVBFE_ALGO_CUSTOM\n", __func__);
	return DVBFE_ALGO_CUSTOM;  /* = Si2168B_Search()        */
#endif
}

static int si2168b_tune(struct dvb_frontend *fe, bool re_tune, unsigned int mode_flags, unsigned int *delay, fe_status_t *status)
{
	int rc = 0;

	siprintk("%s(): re_tune=%d  mode_flags=%u\n", __func__, (int)re_tune, mode_flags);

	if (re_tune)
		rc = si2168b_set_frontend(fe);

	if (!(mode_flags & FE_TUNE_MODE_ONESHOT))
		si2168b_read_status(fe, status);

	return rc;
}

static enum dvbfe_search si2168b_search(struct dvb_frontend *fe)
{
	struct Si2168B_Priv *priv = fe->demodulator_priv;
	/*si2168b_context *demod = priv->si_front_end->demod;*/
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret, i;
	fe_status_t status = 0;

	siprintk("%s(): delsys=%d\n", __func__,	fe->dtv_property_cache.delivery_system);

	/* switch between DVB-T and DVB-T2 when tune fails */
	if (priv->last_tune_failed) {
		if (priv->delivery_system == SYS_DVBT) {
			c->delivery_system = SYS_DVBT2;
		} else if (priv->delivery_system == SYS_DVBT2) {
			c->delivery_system = SYS_DVBT;
		}
	}

	/* set frontend */
	ret = si2168b_set_frontend(fe);
	if (ret)
		goto error;


	/* frontend lock wait loop count */
	switch (priv->delivery_system) {
	case SYS_DVBT:
	case SYS_DVBC_ANNEX_A:
		i = 2;
		break;
	case SYS_DVBT2:
		i = 4;
		break;
	case SYS_UNDEFINED:
	default:
		i = 0;
		break;
	}

	/* wait frontend lock */
	for (; i > 0; i--) {
		siprintk("%s(): loop=%d\n", __func__, i);
		msleep(50);
		ret = si2168b_read_status(fe, &status);
		if (ret)
			goto error;

		if (status & FE_HAS_LOCK)
			break;
	}

	/* check if we have a valid signal */
	if (status & FE_HAS_LOCK) {
		priv->last_tune_failed = 0;
		return DVBFE_ALGO_SEARCH_SUCCESS;
	} else {
		priv->last_tune_failed = 1;
		return DVBFE_ALGO_SEARCH_AGAIN;
	}

error:
	siprintk("%s(): failed=%d\n", __func__, ret);
	return DVBFE_ALGO_SEARCH_ERROR;
}

static const struct dvb_frontend_ops si2168b_ops = {
		.delsys = { SYS_DVBT, SYS_DVBT2, SYS_DVBC_ANNEX_A },
		.info = {
				.name = "Si2168B DVB-T/T2/C",
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
		},

		.release = si2168b_release,
		/*.release_sec,*/

		.init = si2168b_initialize,
		.sleep = si2168b_sleep,

		/*.write,*/

		/* if this is set, it overrides the default swzigzag */
		.tune = si2168b_tune,
		/* get frontend tuning algorithm from the module */
		.get_frontend_algo = si2168b_get_frontend_algo,

		/* these two are only used for the swzigzag code */
		.set_frontend = si2168b_set_frontend,
		.get_tune_settings = si2168b_get_tune_settings,
		.get_frontend = si2168b_get_frontend,

		.read_status = si2168b_read_status,
		.read_ber = si2168b_read_ber,
		.read_signal_strength = si2168b_read_rssi,
		.read_snr = si2168b_read_cnr,
		.read_ucblocks = si2168b_read_uncorrs,

		/* Sat:
		.diseqc_reset_overload,
		.diseqc_send_master_cmd,
		.diseqc_recv_slave_reply,
		.diseqc_send_burst,
		.set_tone,
		.set_voltage,
		.enable_high_lnb_voltage,
		.dishnetwork_send_legacy_command,
		*/

		.i2c_gate_ctrl = si2168b_i2c_gate_ctrl,
		.ts_bus_ctrl = si2168b_ts_bus_ctrl,
		/* .set_lna set in em28xx-dvb.c */

		/* These callbacks are for devices that implement their own
		 * tuning algorithms, rather than a simple swzigzag
		 */
		.search = si2168b_search,

		/* Allow the frontend to validate incoming properties */
		/*.set_property,*/
		/*.get_property,*/
#ifdef FE_READ_STREAM_IDS
		.read_stream_ids = si2168b_read_stream_ids,
#endif
};


struct dvb_frontend *si2168b_attach(const struct si2168b_config *config, struct i2c_adapter *i2c)
{
	struct dtv_frontend_properties *p;
	struct Si2168B_Priv *priv;

	siprintk("%s()\n", __func__);

	/* allocate memory */
	priv = kzalloc(sizeof(struct Si2168B_Priv), GFP_KERNEL);
	if (priv == NULL) {
		siprintk("%s(): kzalloc() failed.\n", __func__);
		goto error;
	}

	priv->si_front_end = kzalloc(sizeof(struct Si2168B_L2_Context), GFP_KERNEL);
	if (priv->si_front_end == NULL) {
		siprintk("%s(): kzalloc si_front_end failed.\n", __func__);
		kfree(priv);
		goto error;
	}

	priv->si_front_end->demod = kzalloc(sizeof(struct si2168b_context), GFP_KERNEL);
	if (priv->si_front_end->demod == NULL) {
		siprintk("%s(): kzalloc demod failed.\n", __func__);
		kfree(priv->si_front_end);
		kfree(priv);
		goto error;
	}

	priv->config = config;
	priv->i2c = i2c;
	priv->delivery_system = SYS_UNDEFINED;

	/* create dvb_frontend */
	memcpy(&priv->frontend.ops, &si2168b_ops, sizeof(struct dvb_frontend_ops));
	priv->frontend.demodulator_priv = priv;

	if (!si2168b_sw_init(priv,
			0,
			enable_tuner_i2c,
			disable_tuner_i2c,
			i2c_callback)){
		printk(KERN_ERR "%s(): si2168b_sw_init() failed.\n", __func__);
	}
	/* enable tuner i2c routing for tuner autodetection */
	if ( priv->config->indirect_i2c_connection ) { /* INDIRECT_I2C_CONNECTION? */
		if (priv->si_front_end->f_TER_tuner_enable(priv->si_front_end->callback) != NO_Si2168B_ERROR) {
			siprintk("%s(): f_TER_tuner_enable() failed\n", __func__);
		}
	} else {
		if (si2168b_tuner_i2c_enable(priv->si_front_end) != NO_Si2168B_ERROR) {
			siprintk("%s(): si2168b_tuner_i2c_enable() failed\n", __func__);
		}
	}
	/* tuner i2c routing is disabled automatically after first tuner access */

	/* Initialize stats */
	p = &priv->frontend.dtv_property_cache;
	p->strength.len = 1;
	p->cnr.len = 1;
	p->block_error.len = 1;
	p->block_count.len = 1;
	p->pre_bit_error.len = 1;
	p->pre_bit_count.len = 1;
	p->post_bit_error.len = 1;
	p->post_bit_count.len = 1;

	p->strength.stat[0].scale = FE_SCALE_DECIBEL;
	p->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->block_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->block_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->pre_bit_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->pre_bit_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->post_bit_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	p->post_bit_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;

	return &priv->frontend;

error:
	return NULL;
}
EXPORT_SYMBOL(si2168b_attach);

MODULE_PARM_DESC(debug, "Turn on/off frontend debugging (default:off).");
MODULE_PARM_DESC(sitrace, "Turn on/off SiTRACE messages (default:off).");
MODULE_PARM_DESC(mutex, "Turn on/off mutex (default:on).");

MODULE_DESCRIPTION("SiLabs 2168B T/T2/C DVB driver");
MODULE_AUTHOR("Source code provided by Silicon Laboratories Inc.");
MODULE_AUTHOR("Henning Garbers <hgarbers@pctvsystems.com>");
MODULE_LICENSE("PROPRIETARY AND CONFIDENTIAL");
MODULE_VERSION("2015-03-09");
