#ifndef __SI2168B_PRIV_H__
#define __SI2168B_PRIV_H__

#include "si2168b.h"

/* Uncomment the following line to activate all traces in the code */
#define SiTRACES

#define Si2168B_A40_COMPATIBLE

#define Si2168B_REF_FREQUENCY_TER 24
#define Si2168B_CLOCK_MODE_TER    Si2168B_START_CLK_CMD_CLK_MODE_CLK_CLKIO

#define TUNERTER_API
#define L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN_SETUP
#define L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP
#define L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC_SETUP

#define Si2168B_DOWNLOAD_ON_CHANGE 1
#define Si2168B_DOWNLOAD_ALWAYS    0

#define Si2168B_TERRESTRIAL 1

#define Si2168B_DVBT_MIN_LOCK_TIME    100
#define Si2168B_DVBT_MAX_LOCK_TIME   2000

#define Si2168B_CLOCK_ALWAYS_OFF 0
#define Si2168B_CLOCK_ALWAYS_ON  1
#define Si2168B_CLOCK_MANAGED    2

#define Si2168B_DVBT2_MIN_LOCK_TIME   100
#define Si2168B_DVBT2_MAX_LOCK_TIME  2000

#define Si2168B_DVBC_MIN_LOCK_TIME     20
#define Si2168B_DVBC_MAX_SEARCH_TIME 5000

#define Si2168B_COMMAND_PROTOTYPES

/* <porting> Replace  CUSTOM_PRINTF with your print-out function.*/
/* #define CUSTOM_PRINTF printf */

#define ERROR_MESSAGE_MAX_LENGH 1000
#define CHECK_FOR_ERRORS  if (L0_ErrorMessage()) printf("\n\n**************\n%s**************\n\n\n", L0_error_message);

#ifdef SiTRACES
/* Porting: Select which of the following lines will allow you to display the function names */
/* Porting: __FUNCTION__ is defined by GCC                                                   */
/* Porting: __func__     is defined by C99 (it is not defined for VisualStudio 6)            */
/* Porting: ""           will not display the function names, but will work on all platforms */
#define SiTRACE(...)        sitrace_function(__FILE__, __LINE__, __FUNCTION__ ,__VA_ARGS__)
/*#define SiTRACE(...)        SiTraceFunction(__FILE__, __LINE__, __func__     ,__VA_ARGS__)*/
/*#define SiTRACE(...)        SiTraceFunction(__FILE__, __LINE__, ""           ,__VA_ARGS__)*/
/*#define SiTRACE(...) */       /* nothing */

/* Replace 'SiTRACES_FULL' by 'SiTRACES_MINIMAL' in the following line to de-activate full features mode */
/* WARNING : the minimal features mode disables tracing functions in stdout and the extern file          */
#define SiTRACES_FULL                  1
#define SiTRACES_MINIMAL               0
#define SiTRACES_FEATURES     SiTRACES_MINIMAL

#else
#define SiTRACE(...)               /* empty */
#define SiTraceConfiguration(...)  /* empty */
#define SiTracesSuspend()          /* empty */
#define SiTracesResume()           /* empty */
#endif /* SiTRACES */

#define TRACES_PRINTF                SiTRACE
#define ALLOCATION_ERROR_MESSAGE     SiTRACE
#define TREAT_ERROR_MESSAGE          SiTRACE
#define TRACES_ERR                   SiTRACE
#define TRACES_TRACE                 SiTRACE
#define TRACES_SHOW                  SiTRACE
#define TRACES_USE                   SiTRACE

#ifdef RWTRACES
/*    #warning "register-level traces activated (RWTRACES defined)" */
#define L1_READ(ptr,  register)     L0_ReadRegisterTrace  (ptr->i2c, #register,     register##_ADDRESS, register##_OFFSET, register##_NBBIT, register##_SIGNED)
#define L1_WRITE(ptr, register, v ) L0_WriteRegisterTrace (ptr->i2c, #register, #v, register##_ADDRESS, register##_OFFSET, register##_NBBIT, register##_ALONE, v)
#else
#define L1_READ(ptr,  register)     L0_ReadRegister       (ptr->i2c,                register##_ADDRESS, register##_OFFSET, register##_NBBIT, register##_SIGNED)
#define L1_WRITE(ptr, register, v ) L0_WriteRegister      (ptr->i2c,                register##_ADDRESS, register##_OFFSET, register##_NBBIT, register##_ALONE, v)
#endif

/******************************************************************************/
/* TER Tuner FEF management options */
/******************************************************************************/
#define Si2168B_FEF_MODE_SLOW_NORMAL_AGC  0
#define Si2168B_FEF_MODE_FREEZE_PIN       1
#define Si2168B_FEF_MODE_SLOW_INITIAL_AGC 2

/******************************************************************************/
/* TER Tuner FEF management selection (possible values are defined above) */
/* NB : This selection is the 'preferred' solution.                           */
/* The code will use more compilation flags to slect the final mode based     */
/*  on what the TER tuner can actually do.                                    */
/******************************************************************************/
/*#define Si2168B_FEF_MODE    Si2168B_FEF_MODE_FREEZE_PIN*/
/*#define Si2168B_FEF_MODE    Si2168B_FEF_MODE_SLOW_NORMAL_AGC*/

/******************************************************************************/
/* Clock sources definition (allows using 'clear' names for clock sources)    */
/******************************************************************************/
typedef enum Si2168B_CLOCK_SOURCE {
	Si2168B_Xtal_clock = 0,
	Si2168B_TER_Tuner_clock,
	Si2168B_SAT_Tuner_clock
} Si2168B_CLOCK_SOURCE;

#ifndef    Si2168B_A40_COMPATIBLE
#ifndef    Si2168B_A3A_COMPATIBLE
"If you get a compilation error on these lines, it means that no Si2168B version has been selected.";
"Please define Si2168B_A40_COMPATIBLE or Si2168B_A3A_COMPATIBLE at project level!";
"Once the flags will be defined, this code will not be visible to the compiler anymore";
"Do NOT comment these lines, they are here to help, showing if there are missing project flags";
#endif /* Si2168B_A3A_COMPATIBLE */
#endif /* Si2168B_A40_COMPATIBLE */

/* Si2168B DD_BER_RESOL property definition */
#define Si2168B_DD_BER_RESOL_PROP 0x1003

#ifdef Si2168B_DD_BER_RESOL_PROP
#define Si2168B_DD_BER_RESOL_PROP_CODE 0x001003


typedef struct { /* Si2168B_DD_BER_RESOL_PROP_struct */
	u8  exp;
	u8  mant;
} Si2168B_DD_BER_RESOL_PROP_struct;

/* DD_BER_RESOL property, EXP field definition (NO TITLE)*/
#define Si2168B_DD_BER_RESOL_PROP_EXP_LSB         0
#define Si2168B_DD_BER_RESOL_PROP_EXP_MASK        0x0f
#define Si2168B_DD_BER_RESOL_PROP_EXP_DEFAULT    7
#define Si2168B_DD_BER_RESOL_PROP_EXP_EXPLO_MIN  1
#define Si2168B_DD_BER_RESOL_PROP_EXP_EXPLO_MAX  8

/* DD_BER_RESOL property, MANT field definition (NO TITLE)*/
#define Si2168B_DD_BER_RESOL_PROP_MANT_LSB         4
#define Si2168B_DD_BER_RESOL_PROP_MANT_MASK        0x0f
#define Si2168B_DD_BER_RESOL_PROP_MANT_DEFAULT    1
#define Si2168B_DD_BER_RESOL_PROP_MANT_MANTLO_MIN  1
#define Si2168B_DD_BER_RESOL_PROP_MANT_MANTLO_MAX  9

#endif /* Si2168B_DD_BER_RESOL_PROP */

/* Si2168B DD_CBER_RESOL property definition */
#define Si2168B_DD_CBER_RESOL_PROP 0x1002

#ifdef Si2168B_DD_CBER_RESOL_PROP
#define Si2168B_DD_CBER_RESOL_PROP_CODE 0x001002


typedef struct { /* Si2168B_DD_CBER_RESOL_PROP_struct */
	u8  exp;
	u8  mant;
} Si2168B_DD_CBER_RESOL_PROP_struct;

/* DD_CBER_RESOL property, EXP field definition (NO TITLE)*/
#define Si2168B_DD_CBER_RESOL_PROP_EXP_LSB         0
#define Si2168B_DD_CBER_RESOL_PROP_EXP_MASK        0x0f
#define Si2168B_DD_CBER_RESOL_PROP_EXP_DEFAULT    5
#define Si2168B_DD_CBER_RESOL_PROP_EXP_EXPLO_MIN  1
#define Si2168B_DD_CBER_RESOL_PROP_EXP_EXPLO_MAX  8

/* DD_CBER_RESOL property, MANT field definition (NO TITLE)*/
#define Si2168B_DD_CBER_RESOL_PROP_MANT_LSB         4
#define Si2168B_DD_CBER_RESOL_PROP_MANT_MASK        0x0f
#define Si2168B_DD_CBER_RESOL_PROP_MANT_DEFAULT    1
#define Si2168B_DD_CBER_RESOL_PROP_MANT_MANTLO_MIN  1
#define Si2168B_DD_CBER_RESOL_PROP_MANT_MANTLO_MAX  9

#endif /* Si2168B_DD_CBER_RESOL_PROP */

/* Si2168B DD_FER_RESOL property definition */
#define Si2168B_DD_FER_RESOL_PROP 0x100c

#ifdef Si2168B_DD_FER_RESOL_PROP
#define Si2168B_DD_FER_RESOL_PROP_CODE 0x00100c

typedef struct { /* Si2168B_DD_FER_RESOL_PROP_struct */
	u8  exp;
	u8  mant;
} Si2168B_DD_FER_RESOL_PROP_struct;

/* DD_FER_RESOL property, EXP field definition (NO TITLE)*/
#define Si2168B_DD_FER_RESOL_PROP_EXP_LSB         0
#define Si2168B_DD_FER_RESOL_PROP_EXP_MASK        0x0f
#define Si2168B_DD_FER_RESOL_PROP_EXP_DEFAULT    3
#define Si2168B_DD_FER_RESOL_PROP_EXP_EXP_MIN  1
#define Si2168B_DD_FER_RESOL_PROP_EXP_EXP_MAX  4

/* DD_FER_RESOL property, MANT field definition (NO TITLE)*/
#define Si2168B_DD_FER_RESOL_PROP_MANT_LSB         4
#define Si2168B_DD_FER_RESOL_PROP_MANT_MASK        0x0f
#define Si2168B_DD_FER_RESOL_PROP_MANT_DEFAULT    1
#define Si2168B_DD_FER_RESOL_PROP_MANT_MANT_MIN  1
#define Si2168B_DD_FER_RESOL_PROP_MANT_MANT_MAX  9

#endif /* Si2168B_DD_FER_RESOL_PROP */

/* Si2168B DD_IEN property definition */
#define Si2168B_DD_IEN_PROP 0x1006

#ifdef Si2168B_DD_IEN_PROP
#define Si2168B_DD_IEN_PROP_CODE 0x001006

typedef struct { /* Si2168B_DD_IEN_PROP_struct */
	u8  ien_bit0;
	u8  ien_bit1;
	u8  ien_bit2;
	u8  ien_bit3;
	u8  ien_bit4;
	u8  ien_bit5;
	u8  ien_bit6;
	u8  ien_bit7;
} Si2168B_DD_IEN_PROP_struct;

/* DD_IEN property, IEN_BIT0 field definition (NO TITLE)*/
#define Si2168B_DD_IEN_PROP_IEN_BIT0_LSB         0
#define Si2168B_DD_IEN_PROP_IEN_BIT0_MASK        0x01
#define Si2168B_DD_IEN_PROP_IEN_BIT0_DEFAULT    0
#define Si2168B_DD_IEN_PROP_IEN_BIT0_DISABLE  0
#define Si2168B_DD_IEN_PROP_IEN_BIT0_ENABLE   1

/* DD_IEN property, IEN_BIT1 field definition (NO TITLE)*/
#define Si2168B_DD_IEN_PROP_IEN_BIT1_LSB         1
#define Si2168B_DD_IEN_PROP_IEN_BIT1_MASK        0x01
#define Si2168B_DD_IEN_PROP_IEN_BIT1_DEFAULT    0
#define Si2168B_DD_IEN_PROP_IEN_BIT1_DISABLE  0
#define Si2168B_DD_IEN_PROP_IEN_BIT1_ENABLE   1

/* DD_IEN property, IEN_BIT2 field definition (NO TITLE)*/
#define Si2168B_DD_IEN_PROP_IEN_BIT2_LSB         2
#define Si2168B_DD_IEN_PROP_IEN_BIT2_MASK        0x01
#define Si2168B_DD_IEN_PROP_IEN_BIT2_DEFAULT    0
#define Si2168B_DD_IEN_PROP_IEN_BIT2_DISABLE  0
#define Si2168B_DD_IEN_PROP_IEN_BIT2_ENABLE   1

/* DD_IEN property, IEN_BIT3 field definition (NO TITLE)*/
#define Si2168B_DD_IEN_PROP_IEN_BIT3_LSB         3
#define Si2168B_DD_IEN_PROP_IEN_BIT3_MASK        0x01
#define Si2168B_DD_IEN_PROP_IEN_BIT3_DEFAULT    0
#define Si2168B_DD_IEN_PROP_IEN_BIT3_DISABLE  0
#define Si2168B_DD_IEN_PROP_IEN_BIT3_ENABLE   1

/* DD_IEN property, IEN_BIT4 field definition (NO TITLE)*/
#define Si2168B_DD_IEN_PROP_IEN_BIT4_LSB         4
#define Si2168B_DD_IEN_PROP_IEN_BIT4_MASK        0x01
#define Si2168B_DD_IEN_PROP_IEN_BIT4_DEFAULT    0
#define Si2168B_DD_IEN_PROP_IEN_BIT4_DISABLE  0
#define Si2168B_DD_IEN_PROP_IEN_BIT4_ENABLE   1

/* DD_IEN property, IEN_BIT5 field definition (NO TITLE)*/
#define Si2168B_DD_IEN_PROP_IEN_BIT5_LSB         5
#define Si2168B_DD_IEN_PROP_IEN_BIT5_MASK        0x01
#define Si2168B_DD_IEN_PROP_IEN_BIT5_DEFAULT    0
#define Si2168B_DD_IEN_PROP_IEN_BIT5_DISABLE  0
#define Si2168B_DD_IEN_PROP_IEN_BIT5_ENABLE   1

/* DD_IEN property, IEN_BIT6 field definition (NO TITLE)*/
#define Si2168B_DD_IEN_PROP_IEN_BIT6_LSB         6
#define Si2168B_DD_IEN_PROP_IEN_BIT6_MASK        0x01
#define Si2168B_DD_IEN_PROP_IEN_BIT6_DEFAULT    0
#define Si2168B_DD_IEN_PROP_IEN_BIT6_DISABLE  0
#define Si2168B_DD_IEN_PROP_IEN_BIT6_ENABLE   1

/* DD_IEN property, IEN_BIT7 field definition (NO TITLE)*/
#define Si2168B_DD_IEN_PROP_IEN_BIT7_LSB         7
#define Si2168B_DD_IEN_PROP_IEN_BIT7_MASK        0x01
#define Si2168B_DD_IEN_PROP_IEN_BIT7_DEFAULT    0
#define Si2168B_DD_IEN_PROP_IEN_BIT7_DISABLE  0
#define Si2168B_DD_IEN_PROP_IEN_BIT7_ENABLE   1

#endif /* Si2168B_DD_IEN_PROP */

/* Si2168B DD_IF_INPUT_FREQ property definition */
#define Si2168B_DD_IF_INPUT_FREQ_PROP 0x100b

#ifdef Si2168B_DD_IF_INPUT_FREQ_PROP
#define Si2168B_DD_IF_INPUT_FREQ_PROP_CODE 0x00100b


typedef struct { /* Si2168B_DD_IF_INPUT_FREQ_PROP_struct */
	u16 offset;
} Si2168B_DD_IF_INPUT_FREQ_PROP_struct;

/* DD_IF_INPUT_FREQ property, OFFSET field definition (NO TITLE)*/
#define Si2168B_DD_IF_INPUT_FREQ_PROP_OFFSET_LSB         0
#define Si2168B_DD_IF_INPUT_FREQ_PROP_OFFSET_MASK        0xffff
#define Si2168B_DD_IF_INPUT_FREQ_PROP_OFFSET_DEFAULT    5000
#define Si2168B_DD_IF_INPUT_FREQ_PROP_OFFSET_OFFSET_MIN  0
#define Si2168B_DD_IF_INPUT_FREQ_PROP_OFFSET_OFFSET_MAX  36000

#endif /* Si2168B_DD_IF_INPUT_FREQ_PROP */

/* Si2168B DD_INT_SENSE property definition */
#define Si2168B_DD_INT_SENSE_PROP 0x1007

#ifdef Si2168B_DD_INT_SENSE_PROP
#define Si2168B_DD_INT_SENSE_PROP_CODE 0x001007

typedef struct { /* Si2168B_DD_INT_SENSE_PROP_struct */
	u8  neg_bit0;
	u8  neg_bit1;
	u8  neg_bit2;
	u8  neg_bit3;
	u8  neg_bit4;
	u8  neg_bit5;
	u8  neg_bit6;
	u8  neg_bit7;
	u8  pos_bit0;
	u8  pos_bit1;
	u8  pos_bit2;
	u8  pos_bit3;
	u8  pos_bit4;
	u8  pos_bit5;
	u8  pos_bit6;
	u8  pos_bit7;
} Si2168B_DD_INT_SENSE_PROP_struct;

/* DD_INT_SENSE property, NEG_BIT0 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT0_LSB         0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT0_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT0_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT0_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT0_ENABLE   1

/* DD_INT_SENSE property, NEG_BIT1 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT1_LSB         1
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT1_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT1_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT1_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT1_ENABLE   1

/* DD_INT_SENSE property, NEG_BIT2 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT2_LSB         2
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT2_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT2_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT2_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT2_ENABLE   1

/* DD_INT_SENSE property, NEG_BIT3 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT3_LSB         3
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT3_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT3_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT3_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT3_ENABLE   1

/* DD_INT_SENSE property, NEG_BIT4 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT4_LSB         4
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT4_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT4_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT4_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT4_ENABLE   1

/* DD_INT_SENSE property, NEG_BIT5 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT5_LSB         5
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT5_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT5_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT5_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT5_ENABLE   1

/* DD_INT_SENSE property, NEG_BIT6 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT6_LSB         6
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT6_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT6_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT6_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT6_ENABLE   1

/* DD_INT_SENSE property, NEG_BIT7 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT7_LSB         7
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT7_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT7_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT7_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_NEG_BIT7_ENABLE   1

/* DD_INT_SENSE property, POS_BIT0 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT0_LSB         8
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT0_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT0_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT0_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT0_ENABLE   1

/* DD_INT_SENSE property, POS_BIT1 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT1_LSB         9
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT1_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT1_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT1_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT1_ENABLE   1

/* DD_INT_SENSE property, POS_BIT2 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT2_LSB         10
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT2_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT2_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT2_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT2_ENABLE   1

/* DD_INT_SENSE property, POS_BIT3 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT3_LSB         11
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT3_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT3_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT3_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT3_ENABLE   1

/* DD_INT_SENSE property, POS_BIT4 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT4_LSB         12
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT4_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT4_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT4_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT4_ENABLE   1

/* DD_INT_SENSE property, POS_BIT5 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT5_LSB         13
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT5_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT5_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT5_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT5_ENABLE   1

/* DD_INT_SENSE property, POS_BIT6 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT6_LSB         14
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT6_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT6_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT6_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT6_ENABLE   1

/* DD_INT_SENSE property, POS_BIT7 field definition (NO TITLE)*/
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT7_LSB         15
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT7_MASK        0x01
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT7_DEFAULT    0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT7_DISABLE  0
#define Si2168B_DD_INT_SENSE_PROP_POS_BIT7_ENABLE   1

#endif /* Si2168B_DD_INT_SENSE_PROP */

/* Si2168B DD_MODE property definition */
#define Si2168B_DD_MODE_PROP 0x100a

#ifdef Si2168B_DD_MODE_PROP
#define Si2168B_DD_MODE_PROP_CODE 0x00100a

typedef struct { /* Si2168B_DD_MODE_PROP_struct */
	u8  auto_detect;
	u8  bw;
	u8  invert_spectrum;
	u8  modulation;
} Si2168B_DD_MODE_PROP_struct;

/* DD_MODE property, AUTO_DETECT field definition (NO TITLE)*/
#define Si2168B_DD_MODE_PROP_AUTO_DETECT_LSB         9
#define Si2168B_DD_MODE_PROP_AUTO_DETECT_MASK        0x07
#define Si2168B_DD_MODE_PROP_AUTO_DETECT_DEFAULT    0
#define Si2168B_DD_MODE_PROP_AUTO_DETECT_NONE               0
#define Si2168B_DD_MODE_PROP_AUTO_DETECT_AUTO_DVB_T_T2      1
#define Si2168B_DD_MODE_PROP_AUTO_DETECT_AUTO_DVB_S_S2      2
#define Si2168B_DD_MODE_PROP_AUTO_DETECT_AUTO_DVB_S_S2_DSS  3

/* DD_MODE property, BW field definition (NO TITLE)*/
#define Si2168B_DD_MODE_PROP_BW_LSB         0
#define Si2168B_DD_MODE_PROP_BW_MASK        0x0f
#define Si2168B_DD_MODE_PROP_BW_DEFAULT    8
#define Si2168B_DD_MODE_PROP_BW_BW_5MHZ    5
#define Si2168B_DD_MODE_PROP_BW_BW_6MHZ    6
#define Si2168B_DD_MODE_PROP_BW_BW_7MHZ    7
#define Si2168B_DD_MODE_PROP_BW_BW_8MHZ    8
#define Si2168B_DD_MODE_PROP_BW_BW_1D7MHZ  2

/* DD_MODE property, INVERT_SPECTRUM field definition (NO TITLE)*/
#define Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_LSB         8
#define Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_MASK        0x01
#define Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_DEFAULT    0
#define Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_NORMAL    0
#define Si2168B_DD_MODE_PROP_INVERT_SPECTRUM_INVERTED  1

/* DD_MODE property, MODULATION field definition (NO TITLE)*/
#define Si2168B_DD_MODE_PROP_MODULATION_LSB         4
#define Si2168B_DD_MODE_PROP_MODULATION_MASK        0x0f
#define Si2168B_DD_MODE_PROP_MODULATION_DEFAULT    2
#define Si2168B_DD_MODE_PROP_MODULATION_MCNS         1
#define Si2168B_DD_MODE_PROP_MODULATION_DVBT         2
#define Si2168B_DD_MODE_PROP_MODULATION_DVBC         3
#define Si2168B_DD_MODE_PROP_MODULATION_DVBT2        7
#define Si2168B_DD_MODE_PROP_MODULATION_DVBS         8
#define Si2168B_DD_MODE_PROP_MODULATION_DVBS2        9
#define Si2168B_DD_MODE_PROP_MODULATION_DSS          10
#define Si2168B_DD_MODE_PROP_MODULATION_DVBC2        11
#define Si2168B_DD_MODE_PROP_MODULATION_AUTO_DETECT  15
#define Si2168B_DD_MODE_PROP_MODULATION_ANALOG     100
/* new mode definition to set SLEEP mode */
#define Si2168B_DD_MODE_PROP_MODULATION_SLEEP      0xFF

#endif /* Si2168B_DD_MODE_PROP */

/* Si2168B DD_PER_RESOL property definition */
#define Si2168B_DD_PER_RESOL_PROP 0x1004

#ifdef Si2168B_DD_PER_RESOL_PROP
#define Si2168B_DD_PER_RESOL_PROP_CODE 0x001004

typedef struct { /* Si2168B_DD_PER_RESOL_PROP_struct */
	u8  exp;
	u8  mant;
} Si2168B_DD_PER_RESOL_PROP_struct;

/* DD_PER_RESOL property, EXP field definition (NO TITLE)*/
#define Si2168B_DD_PER_RESOL_PROP_EXP_LSB         0
#define Si2168B_DD_PER_RESOL_PROP_EXP_MASK        0x0f
#define Si2168B_DD_PER_RESOL_PROP_EXP_DEFAULT    5
#define Si2168B_DD_PER_RESOL_PROP_EXP_EXPLO_MIN  1
#define Si2168B_DD_PER_RESOL_PROP_EXP_EXPLO_MAX  9

/* DD_PER_RESOL property, MANT field definition (NO TITLE)*/
#define Si2168B_DD_PER_RESOL_PROP_MANT_LSB         4
#define Si2168B_DD_PER_RESOL_PROP_MANT_MASK        0x0f
#define Si2168B_DD_PER_RESOL_PROP_MANT_DEFAULT    1
#define Si2168B_DD_PER_RESOL_PROP_MANT_MANTLO_MIN  1
#define Si2168B_DD_PER_RESOL_PROP_MANT_MANTLO_MAX  9

#endif /* Si2168B_DD_PER_RESOL_PROP */

/* Si2168B DD_RSQ_BER_THRESHOLD property definition */
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP 0x1005

#ifdef Si2168B_DD_RSQ_BER_THRESHOLD_PROP
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_CODE 0x001005

typedef struct { /* Si2168B_DD_RSQ_BER_THRESHOLD_PROP_struct */
	u8  exp;
	u8  mant;
} Si2168B_DD_RSQ_BER_THRESHOLD_PROP_struct;

/* DD_RSQ_BER_THRESHOLD property, EXP field definition (NO TITLE)*/
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_EXP_LSB         0
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_EXP_MASK        0x0f
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_EXP_DEFAULT    1
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_EXP_EXP_MIN  1
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_EXP_EXP_MAX  8

/* DD_RSQ_BER_THRESHOLD property, MANT field definition (NO TITLE)*/
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_MANT_LSB         4
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_MANT_MASK        0x0f
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_MANT_DEFAULT    10
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_MANT_MANT_MIN  0
#define Si2168B_DD_RSQ_BER_THRESHOLD_PROP_MANT_MANT_MAX  99

#endif /* Si2168B_DD_RSQ_BER_THRESHOLD_PROP */

/* Si2168B DD_SSI_SQI_PARAM property definition */
#define Si2168B_DD_SSI_SQI_PARAM_PROP 0x100f

#ifdef Si2168B_DD_SSI_SQI_PARAM_PROP
#define Si2168B_DD_SSI_SQI_PARAM_PROP_CODE 0x00100f

typedef struct { /* Si2168B_DD_SSI_SQI_PARAM_PROP_struct */
	u8  sqi_average;
} Si2168B_DD_SSI_SQI_PARAM_PROP_struct;

/* DD_SSI_SQI_PARAM property, SQI_AVERAGE field definition (NO TITLE)*/
#define Si2168B_DD_SSI_SQI_PARAM_PROP_SQI_AVERAGE_LSB         0
#define Si2168B_DD_SSI_SQI_PARAM_PROP_SQI_AVERAGE_MASK        0x1f
#define Si2168B_DD_SSI_SQI_PARAM_PROP_SQI_AVERAGE_DEFAULT    1
#define Si2168B_DD_SSI_SQI_PARAM_PROP_SQI_AVERAGE_SQI_AVERAGE_MIN  1
#define Si2168B_DD_SSI_SQI_PARAM_PROP_SQI_AVERAGE_SQI_AVERAGE_MAX  30

#endif /* Si2168B_DD_SSI_SQI_PARAM_PROP */

/* Si2168B DD_TS_FREQ property definition */
#define Si2168B_DD_TS_FREQ_PROP 0x100d

#ifdef Si2168B_DD_TS_FREQ_PROP
#define Si2168B_DD_TS_FREQ_PROP_CODE 0x00100d

typedef struct { /* Si2168B_DD_TS_FREQ_PROP_struct */
	u16 req_freq_10khz;
} Si2168B_DD_TS_FREQ_PROP_struct;

/* DD_TS_FREQ property, REQ_FREQ_10KHZ field definition (NO TITLE)*/
#define Si2168B_DD_TS_FREQ_PROP_REQ_FREQ_10KHZ_LSB         0
#define Si2168B_DD_TS_FREQ_PROP_REQ_FREQ_10KHZ_MASK        0x3fff
#define Si2168B_DD_TS_FREQ_PROP_REQ_FREQ_10KHZ_DEFAULT    720
#define Si2168B_DD_TS_FREQ_PROP_REQ_FREQ_10KHZ_REQ_FREQ_10KHZ_MIN  0
#define Si2168B_DD_TS_FREQ_PROP_REQ_FREQ_10KHZ_REQ_FREQ_10KHZ_MAX  14550

#endif /* Si2168B_DD_TS_FREQ_PROP */

/* Si2168B DD_TS_MODE property definition */
#define Si2168B_DD_TS_MODE_PROP 0x1001

#ifdef Si2168B_DD_TS_MODE_PROP
#define Si2168B_DD_TS_MODE_PROP_CODE 0x001001

typedef struct { /* Si2168B_DD_TS_MODE_PROP_struct */
	u8  clk_gapped_en;
	u8  clock;
	u8  mode;
	u8  special;
	u8  ts_err_polarity;
	u8  ts_freq_resolution;
} Si2168B_DD_TS_MODE_PROP_struct;

/* DD_TS_MODE property, CLK_GAPPED_EN field definition (NO TITLE)*/
#define Si2168B_DD_TS_MODE_PROP_CLK_GAPPED_EN_LSB         6
#define Si2168B_DD_TS_MODE_PROP_CLK_GAPPED_EN_MASK        0x01
#define Si2168B_DD_TS_MODE_PROP_CLK_GAPPED_EN_DEFAULT    0
#define Si2168B_DD_TS_MODE_PROP_CLK_GAPPED_EN_DISABLED  0
#define Si2168B_DD_TS_MODE_PROP_CLK_GAPPED_EN_ENABLED   1

/* DD_TS_MODE property, CLOCK field definition (NO TITLE)*/
#define Si2168B_DD_TS_MODE_PROP_CLOCK_LSB         4
#define Si2168B_DD_TS_MODE_PROP_CLOCK_MASK        0x03
#define Si2168B_DD_TS_MODE_PROP_CLOCK_DEFAULT    0
#define Si2168B_DD_TS_MODE_PROP_CLOCK_AUTO_FIXED  0
#define Si2168B_DD_TS_MODE_PROP_CLOCK_AUTO_ADAPT  1
#define Si2168B_DD_TS_MODE_PROP_CLOCK_MANUAL      2

/* DD_TS_MODE property, MODE field definition (NO TITLE)*/
#define Si2168B_DD_TS_MODE_PROP_MODE_LSB         0
#define Si2168B_DD_TS_MODE_PROP_MODE_MASK        0x0f
#define Si2168B_DD_TS_MODE_PROP_MODE_DEFAULT    0
#define Si2168B_DD_TS_MODE_PROP_MODE_TRISTATE  0
#define Si2168B_DD_TS_MODE_PROP_MODE_OFF       1
#define Si2168B_DD_TS_MODE_PROP_MODE_SERIAL    3
#define Si2168B_DD_TS_MODE_PROP_MODE_PARALLEL  6
#define Si2168B_DD_TS_MODE_PROP_MODE_GPIF      7

/* DD_TS_MODE property, SPECIAL field definition (NO TITLE)*/
#define Si2168B_DD_TS_MODE_PROP_SPECIAL_LSB         8
#define Si2168B_DD_TS_MODE_PROP_SPECIAL_MASK        0x03
#define Si2168B_DD_TS_MODE_PROP_SPECIAL_DEFAULT    0
#define Si2168B_DD_TS_MODE_PROP_SPECIAL_FULL_TS         0
#define Si2168B_DD_TS_MODE_PROP_SPECIAL_DATAS_TRISTATE  1

/* DD_TS_MODE property, TS_ERR_POLARITY field definition (NO TITLE)*/
#define Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_LSB         7
#define Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_MASK        0x01
#define Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_DEFAULT    0
#define Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_NOT_INVERTED  0
#define Si2168B_DD_TS_MODE_PROP_TS_ERR_POLARITY_INVERTED      1

/* DD_TS_MODE property, TS_FREQ_RESOLUTION field definition (NO TITLE)*/
#define Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_LSB         10
#define Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_MASK        0x01
#define Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_DEFAULT    0
#define Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_NORMAL  0
#define Si2168B_DD_TS_MODE_PROP_TS_FREQ_RESOLUTION_FINE    1

#endif /* Si2168B_DD_TS_MODE_PROP */

/* Si2168B DD_TS_SERIAL_DIFF property definition */
#define Si2168B_DD_TS_SERIAL_DIFF_PROP 0x1012

#ifdef Si2168B_DD_TS_SERIAL_DIFF_PROP
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_CODE 0x001012

typedef struct { /* Si2168B_DD_TS_SERIAL_DIFF_PROP_struct */
	u8  ts_clkb_on_data1;
	u8  ts_data0b_on_data2;
	u8  ts_data1_shape;
	u8  ts_data1_strength;
	u8  ts_data2_shape;
	u8  ts_data2_strength;
} Si2168B_DD_TS_SERIAL_DIFF_PROP_struct;

/* DD_TS_SERIAL_DIFF property, TS_CLKB_ON_DATA1 field definition (NO TITLE)*/
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_CLKB_ON_DATA1_LSB         12
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_CLKB_ON_DATA1_MASK        0x01
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_CLKB_ON_DATA1_DEFAULT    0
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_CLKB_ON_DATA1_DISABLE  0
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_CLKB_ON_DATA1_ENABLE   1

/* DD_TS_SERIAL_DIFF property, TS_DATA0B_ON_DATA2 field definition (NO TITLE)*/
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA0B_ON_DATA2_LSB         13
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA0B_ON_DATA2_MASK        0x01
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA0B_ON_DATA2_DEFAULT    0
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA0B_ON_DATA2_DISABLE  0
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA0B_ON_DATA2_ENABLE   1

/* DD_TS_SERIAL_DIFF property, TS_DATA1_SHAPE field definition (NO TITLE)*/
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA1_SHAPE_LSB         4
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA1_SHAPE_MASK        0x03
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA1_SHAPE_DEFAULT    3
/* DD_TS_SERIAL_DIFF property, TS_DATA1_STRENGTH field definition (NO TITLE)*/
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA1_STRENGTH_LSB         0
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA1_STRENGTH_MASK        0x0f
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA1_STRENGTH_DEFAULT    15
/* DD_TS_SERIAL_DIFF property, TS_DATA2_SHAPE field definition (NO TITLE)*/
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA2_SHAPE_LSB         10
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA2_SHAPE_MASK        0x03
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA2_SHAPE_DEFAULT    3
/* DD_TS_SERIAL_DIFF property, TS_DATA2_STRENGTH field definition (NO TITLE)*/
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA2_STRENGTH_LSB         6
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA2_STRENGTH_MASK        0x0f
#define Si2168B_DD_TS_SERIAL_DIFF_PROP_TS_DATA2_STRENGTH_DEFAULT    15
#endif /* Si2168B_DD_TS_SERIAL_DIFF_PROP */

/* Si2168B DD_TS_SETUP_PAR property definition */
#define Si2168B_DD_TS_SETUP_PAR_PROP 0x1009

#ifdef Si2168B_DD_TS_SETUP_PAR_PROP
#define Si2168B_DD_TS_SETUP_PAR_PROP_CODE 0x001009

typedef struct { /* Si2168B_DD_TS_SETUP_PAR_PROP_struct */
	u8  ts_clk_invert;
	u8  ts_clk_shape;
	u8  ts_clk_shift;
	u8  ts_clk_strength;
	u8  ts_data_shape;
	u8  ts_data_strength;
} Si2168B_DD_TS_SETUP_PAR_PROP_struct;

/* DD_TS_SETUP_PAR property, TS_CLK_INVERT field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_LSB         12
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_MASK        0x01
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_DEFAULT    1
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_NOT_INVERTED  0
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_INVERT_INVERTED      1

/* DD_TS_SETUP_PAR property, TS_CLK_SHAPE field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHAPE_LSB         10
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHAPE_MASK        0x03
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHAPE_DEFAULT    1
/* DD_TS_SETUP_PAR property, TS_CLK_SHIFT field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHIFT_LSB         13
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHIFT_MASK        0x07
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_SHIFT_DEFAULT    0
/* DD_TS_SETUP_PAR property, TS_CLK_STRENGTH field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_STRENGTH_LSB         6
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_STRENGTH_MASK        0x0f
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_CLK_STRENGTH_DEFAULT    3
/* DD_TS_SETUP_PAR property, TS_DATA_SHAPE field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_SHAPE_LSB         4
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_SHAPE_MASK        0x03
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_SHAPE_DEFAULT    1
/* DD_TS_SETUP_PAR property, TS_DATA_STRENGTH field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_STRENGTH_LSB         0
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_STRENGTH_MASK        0x0f
#define Si2168B_DD_TS_SETUP_PAR_PROP_TS_DATA_STRENGTH_DEFAULT    3
#endif /* Si2168B_DD_TS_SETUP_PAR_PROP */

/* Si2168B DD_TS_SETUP_SER property definition */
#define Si2168B_DD_TS_SETUP_SER_PROP 0x1008

#ifdef Si2168B_DD_TS_SETUP_SER_PROP
#define Si2168B_DD_TS_SETUP_SER_PROP_CODE 0x001008

typedef struct { /* Si2168B_DD_TS_SETUP_SER_PROP_struct */
	u8  ts_byte_order;
	u8  ts_clk_invert;
	u8  ts_clk_shape;
	u8  ts_clk_strength;
	u8  ts_data_shape;
	u8  ts_data_strength;
	u8  ts_sync_duration;
} Si2168B_DD_TS_SETUP_SER_PROP_struct;

/* DD_TS_SETUP_SER property, TS_BYTE_ORDER field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_BYTE_ORDER_LSB         14
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_BYTE_ORDER_MASK        0x01
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_BYTE_ORDER_DEFAULT    0
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_BYTE_ORDER_MSB_FIRST  0
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_BYTE_ORDER_LSB_FIRST  1

/* DD_TS_SETUP_SER property, TS_CLK_INVERT field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_INVERT_LSB         12
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_INVERT_MASK        0x01
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_INVERT_DEFAULT    1
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_INVERT_NOT_INVERTED  0
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_INVERT_INVERTED      1

/* DD_TS_SETUP_SER property, TS_CLK_SHAPE field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_SHAPE_LSB         10
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_SHAPE_MASK        0x03
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_SHAPE_DEFAULT    3
/* DD_TS_SETUP_SER property, TS_CLK_STRENGTH field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_STRENGTH_LSB         6
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_STRENGTH_MASK        0x0f
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_CLK_STRENGTH_DEFAULT    15
/* DD_TS_SETUP_SER property, TS_DATA_SHAPE field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_DATA_SHAPE_LSB         4
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_DATA_SHAPE_MASK        0x03
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_DATA_SHAPE_DEFAULT    3
/* DD_TS_SETUP_SER property, TS_DATA_STRENGTH field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_DATA_STRENGTH_LSB         0
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_DATA_STRENGTH_MASK        0x0f
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_DATA_STRENGTH_DEFAULT    15
/* DD_TS_SETUP_SER property, TS_SYNC_DURATION field definition (NO TITLE)*/
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_SYNC_DURATION_LSB         13
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_SYNC_DURATION_MASK        0x01
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_SYNC_DURATION_DEFAULT    0
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_SYNC_DURATION_FIRST_BYTE  0
#define Si2168B_DD_TS_SETUP_SER_PROP_TS_SYNC_DURATION_FIRST_BIT   1

#endif /* Si2168B_DD_TS_SETUP_SER_PROP */

/* Si2168B DVBC_ADC_CREST_FACTOR property definition */
#define Si2168B_DVBC_ADC_CREST_FACTOR_PROP 0x1104

#ifdef Si2168B_DVBC_ADC_CREST_FACTOR_PROP
#define Si2168B_DVBC_ADC_CREST_FACTOR_PROP_CODE 0x001104

typedef struct { /* Si2168B_DVBC_ADC_CREST_FACTOR_PROP_struct */
	u8  crest_factor;
} Si2168B_DVBC_ADC_CREST_FACTOR_PROP_struct;

/* DVBC_ADC_CREST_FACTOR property, CREST_FACTOR field definition (NO TITLE)*/
#define Si2168B_DVBC_ADC_CREST_FACTOR_PROP_CREST_FACTOR_LSB         0
#define Si2168B_DVBC_ADC_CREST_FACTOR_PROP_CREST_FACTOR_MASK        0xff
#define Si2168B_DVBC_ADC_CREST_FACTOR_PROP_CREST_FACTOR_DEFAULT    112
#endif /* Si2168B_DVBC_ADC_CREST_FACTOR_PROP */

/* Si2168B DVBC_AFC_RANGE property definition */
#define Si2168B_DVBC_AFC_RANGE_PROP 0x1103

#ifdef Si2168B_DVBC_AFC_RANGE_PROP
#define Si2168B_DVBC_AFC_RANGE_PROP_CODE 0x001103

typedef struct { /* Si2168B_DVBC_AFC_RANGE_PROP_struct */
	u16 range_khz;
} Si2168B_DVBC_AFC_RANGE_PROP_struct;

/* DVBC_AFC_RANGE property, RANGE_KHZ field definition (NO TITLE)*/
#define Si2168B_DVBC_AFC_RANGE_PROP_RANGE_KHZ_LSB         0
#define Si2168B_DVBC_AFC_RANGE_PROP_RANGE_KHZ_MASK        0xffff
#define Si2168B_DVBC_AFC_RANGE_PROP_RANGE_KHZ_DEFAULT    100
#endif /* Si2168B_DVBC_AFC_RANGE_PROP */

/* Si2168B DVBC_CONSTELLATION property definition */
#define Si2168B_DVBC_CONSTELLATION_PROP 0x1101

#ifdef Si2168B_DVBC_CONSTELLATION_PROP
#define Si2168B_DVBC_CONSTELLATION_PROP_CODE 0x001101

typedef struct { /* Si2168B_DVBC_CONSTELLATION_PROP_struct */
	u8  constellation;
} Si2168B_DVBC_CONSTELLATION_PROP_struct;

/* DVBC_CONSTELLATION property, CONSTELLATION field definition (NO TITLE)*/
#define Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_LSB         0
#define Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_MASK        0x3f
#define Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_DEFAULT    0
#define Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_AUTO    0
#define Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM16   7
#define Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM32   8
#define Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM64   9
#define Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM128  10
#define Si2168B_DVBC_CONSTELLATION_PROP_CONSTELLATION_QAM256  11

#endif /* Si2168B_DVBC_CONSTELLATION_PROP */

/* Si2168B DVBC_SYMBOL_RATE property definition */
#define Si2168B_DVBC_SYMBOL_RATE_PROP 0x1102

#ifdef Si2168B_DVBC_SYMBOL_RATE_PROP
#define Si2168B_DVBC_SYMBOL_RATE_PROP_CODE 0x001102

typedef struct { /* Si2168B_DVBC_SYMBOL_RATE_PROP_struct */
	u16 rate;
} Si2168B_DVBC_SYMBOL_RATE_PROP_struct;

/* DVBC_SYMBOL_RATE property, RATE field definition (NO TITLE)*/
#define Si2168B_DVBC_SYMBOL_RATE_PROP_RATE_LSB         0
#define Si2168B_DVBC_SYMBOL_RATE_PROP_RATE_MASK        0xffff
#define Si2168B_DVBC_SYMBOL_RATE_PROP_RATE_DEFAULT    6900
#endif /* Si2168B_DVBC_SYMBOL_RATE_PROP */

/* Si2168B DVBT2_ADC_CREST_FACTOR property definition */
#define Si2168B_DVBT2_ADC_CREST_FACTOR_PROP 0x1303

#ifdef Si2168B_DVBT2_ADC_CREST_FACTOR_PROP
#define Si2168B_DVBT2_ADC_CREST_FACTOR_PROP_CODE 0x001303

typedef struct { /* Si2168B_DVBT2_ADC_CREST_FACTOR_PROP_struct */
	u8  crest_factor;
} Si2168B_DVBT2_ADC_CREST_FACTOR_PROP_struct;

/* DVBT2_ADC_CREST_FACTOR property, CREST_FACTOR field definition (NO TITLE)*/
#define Si2168B_DVBT2_ADC_CREST_FACTOR_PROP_CREST_FACTOR_LSB         0
#define Si2168B_DVBT2_ADC_CREST_FACTOR_PROP_CREST_FACTOR_MASK        0xff
#define Si2168B_DVBT2_ADC_CREST_FACTOR_PROP_CREST_FACTOR_DEFAULT    130
#endif /* Si2168B_DVBT2_ADC_CREST_FACTOR_PROP */

/* Si2168B DVBT2_AFC_RANGE property definition */
#define Si2168B_DVBT2_AFC_RANGE_PROP 0x1301

#ifdef Si2168B_DVBT2_AFC_RANGE_PROP
#define Si2168B_DVBT2_AFC_RANGE_PROP_CODE 0x001301

typedef struct { /* Si2168B_DVBT2_AFC_RANGE_PROP_struct */
	u16 range_khz;
} Si2168B_DVBT2_AFC_RANGE_PROP_struct;

/* DVBT2_AFC_RANGE property, RANGE_KHZ field definition (NO TITLE)*/
#define Si2168B_DVBT2_AFC_RANGE_PROP_RANGE_KHZ_LSB         0
#define Si2168B_DVBT2_AFC_RANGE_PROP_RANGE_KHZ_MASK        0xffff
#define Si2168B_DVBT2_AFC_RANGE_PROP_RANGE_KHZ_DEFAULT    550
#endif /* Si2168B_DVBT2_AFC_RANGE_PROP */

/* Si2168B DVBT2_FEF_TUNER property definition */
#define Si2168B_DVBT2_FEF_TUNER_PROP 0x1302

#ifdef Si2168B_DVBT2_FEF_TUNER_PROP
#define Si2168B_DVBT2_FEF_TUNER_PROP_CODE 0x001302

typedef struct { /* Si2168B_DVBT2_FEF_TUNER_PROP_struct */
	u8  tuner_delay;
	u8  tuner_freeze_time;
	u8  tuner_unfreeze_time;
} Si2168B_DVBT2_FEF_TUNER_PROP_struct;

/* DVBT2_FEF_TUNER property, TUNER_DELAY field definition (NO TITLE)*/
#define Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_DELAY_LSB         0
#define Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_DELAY_MASK        0xff
#define Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_DELAY_DEFAULT    1
/* DVBT2_FEF_TUNER property, TUNER_FREEZE_TIME field definition (NO TITLE)*/
#define Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_FREEZE_TIME_LSB         8
#define Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_FREEZE_TIME_MASK        0x0f
#define Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_FREEZE_TIME_DEFAULT    1
/* DVBT2_FEF_TUNER property, TUNER_UNFREEZE_TIME field definition (NO TITLE)*/
#define Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_UNFREEZE_TIME_LSB         12
#define Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_UNFREEZE_TIME_MASK        0x0f
#define Si2168B_DVBT2_FEF_TUNER_PROP_TUNER_UNFREEZE_TIME_DEFAULT    1
#endif /* Si2168B_DVBT2_FEF_TUNER_PROP */

/* Si2168B DVBT2_MODE property definition */
#define Si2168B_DVBT2_MODE_PROP 0x1304

#ifdef Si2168B_DVBT2_MODE_PROP
#define Si2168B_DVBT2_MODE_PROP_CODE 0x001304

typedef struct { /* Si2168B_DVBT2_MODE_PROP_struct */
	u8  lock_mode;
} Si2168B_DVBT2_MODE_PROP_struct;

/* DVBT2_MODE property, LOCK_MODE field definition (NO TITLE)*/
#define Si2168B_DVBT2_MODE_PROP_LOCK_MODE_LSB         0
#define Si2168B_DVBT2_MODE_PROP_LOCK_MODE_MASK        0x03
#define Si2168B_DVBT2_MODE_PROP_LOCK_MODE_DEFAULT    0
#define Si2168B_DVBT2_MODE_PROP_LOCK_MODE_ANY        0
#define Si2168B_DVBT2_MODE_PROP_LOCK_MODE_BASE_ONLY  1
#define Si2168B_DVBT2_MODE_PROP_LOCK_MODE_LITE_ONLY  2
#define Si2168B_DVBT2_MODE_PROP_LOCK_MODE_RESERVED   3

#endif /* Si2168B_DVBT2_MODE_PROP */

/* Si2168B DVBT_ADC_CREST_FACTOR property definition */
#define Si2168B_DVBT_ADC_CREST_FACTOR_PROP 0x1203

#ifdef Si2168B_DVBT_ADC_CREST_FACTOR_PROP
#define Si2168B_DVBT_ADC_CREST_FACTOR_PROP_CODE 0x001203

typedef struct { /* Si2168B_DVBT_ADC_CREST_FACTOR_PROP_struct */
	u8  crest_factor;
} Si2168B_DVBT_ADC_CREST_FACTOR_PROP_struct;

/* DVBT_ADC_CREST_FACTOR property, CREST_FACTOR field definition (NO TITLE)*/
#define Si2168B_DVBT_ADC_CREST_FACTOR_PROP_CREST_FACTOR_LSB         0
#define Si2168B_DVBT_ADC_CREST_FACTOR_PROP_CREST_FACTOR_MASK        0xff
#define Si2168B_DVBT_ADC_CREST_FACTOR_PROP_CREST_FACTOR_DEFAULT    130
#endif /* Si2168B_DVBT_ADC_CREST_FACTOR_PROP */

/* Si2168B DVBT_AFC_RANGE property definition */
#define Si2168B_DVBT_AFC_RANGE_PROP 0x1202

#ifdef Si2168B_DVBT_AFC_RANGE_PROP
#define Si2168B_DVBT_AFC_RANGE_PROP_CODE 0x001202

typedef struct { /* Si2168B_DVBT_AFC_RANGE_PROP_struct */
	u16 range_khz;
} Si2168B_DVBT_AFC_RANGE_PROP_struct;

/* DVBT_AFC_RANGE property, RANGE_KHZ field definition (NO TITLE)*/
#define Si2168B_DVBT_AFC_RANGE_PROP_RANGE_KHZ_LSB         0
#define Si2168B_DVBT_AFC_RANGE_PROP_RANGE_KHZ_MASK        0xffff
#define Si2168B_DVBT_AFC_RANGE_PROP_RANGE_KHZ_DEFAULT    550
#endif /* Si2168B_DVBT_AFC_RANGE_PROP */

/* Si2168B DVBT_HIERARCHY property definition */
#define Si2168B_DVBT_HIERARCHY_PROP 0x1201

#ifdef Si2168B_DVBT_HIERARCHY_PROP
#define Si2168B_DVBT_HIERARCHY_PROP_CODE 0x001201

typedef struct { /* Si2168B_DVBT_HIERARCHY_PROP_struct */
	u8  stream;
} Si2168B_DVBT_HIERARCHY_PROP_struct;

/* DVBT_HIERARCHY property, STREAM field definition (NO TITLE)*/
#define Si2168B_DVBT_HIERARCHY_PROP_STREAM_LSB         0
#define Si2168B_DVBT_HIERARCHY_PROP_STREAM_MASK        0x01
#define Si2168B_DVBT_HIERARCHY_PROP_STREAM_DEFAULT    0
#define Si2168B_DVBT_HIERARCHY_PROP_STREAM_HP  0
#define Si2168B_DVBT_HIERARCHY_PROP_STREAM_LP  1

#endif /* Si2168B_DVBT_HIERARCHY_PROP */

/* Si2168B MASTER_IEN property definition */
#define Si2168B_MASTER_IEN_PROP 0x0401

#ifdef Si2168B_MASTER_IEN_PROP
#define Si2168B_MASTER_IEN_PROP_CODE 0x000401

typedef struct { /* Si2168B_MASTER_IEN_PROP_struct */
	u8  ctsien;
	u8  ddien;
	u8  errien;
	u8  scanien;
} Si2168B_MASTER_IEN_PROP_struct;

/* MASTER_IEN property, CTSIEN field definition (NO TITLE)*/
#define Si2168B_MASTER_IEN_PROP_CTSIEN_LSB         7
#define Si2168B_MASTER_IEN_PROP_CTSIEN_MASK        0x01
#define Si2168B_MASTER_IEN_PROP_CTSIEN_DEFAULT    0
#define Si2168B_MASTER_IEN_PROP_CTSIEN_OFF  0
#define Si2168B_MASTER_IEN_PROP_CTSIEN_ON   1

/* MASTER_IEN property, DDIEN field definition (NO TITLE)*/
#define Si2168B_MASTER_IEN_PROP_DDIEN_LSB         0
#define Si2168B_MASTER_IEN_PROP_DDIEN_MASK        0x01
#define Si2168B_MASTER_IEN_PROP_DDIEN_DEFAULT    0
#define Si2168B_MASTER_IEN_PROP_DDIEN_OFF  0
#define Si2168B_MASTER_IEN_PROP_DDIEN_ON   1

/* MASTER_IEN property, ERRIEN field definition (NO TITLE)*/
#define Si2168B_MASTER_IEN_PROP_ERRIEN_LSB         6
#define Si2168B_MASTER_IEN_PROP_ERRIEN_MASK        0x01
#define Si2168B_MASTER_IEN_PROP_ERRIEN_DEFAULT    0
#define Si2168B_MASTER_IEN_PROP_ERRIEN_OFF  0
#define Si2168B_MASTER_IEN_PROP_ERRIEN_ON   1

/* MASTER_IEN property, SCANIEN field definition (NO TITLE)*/
#define Si2168B_MASTER_IEN_PROP_SCANIEN_LSB         1
#define Si2168B_MASTER_IEN_PROP_SCANIEN_MASK        0x01
#define Si2168B_MASTER_IEN_PROP_SCANIEN_DEFAULT    0
#define Si2168B_MASTER_IEN_PROP_SCANIEN_OFF  0
#define Si2168B_MASTER_IEN_PROP_SCANIEN_ON   1

#endif /* Si2168B_MASTER_IEN_PROP */

/* Si2168B MCNS_ADC_CREST_FACTOR property definition */
#define Si2168B_MCNS_ADC_CREST_FACTOR_PROP 0x1604

#ifdef Si2168B_MCNS_ADC_CREST_FACTOR_PROP
#define Si2168B_MCNS_ADC_CREST_FACTOR_PROP_CODE 0x001604

typedef struct { /* Si2168B_MCNS_ADC_CREST_FACTOR_PROP_struct */
	u8  crest_factor;
} Si2168B_MCNS_ADC_CREST_FACTOR_PROP_struct;

/* MCNS_ADC_CREST_FACTOR property, CREST_FACTOR field definition (NO TITLE)*/
#define Si2168B_MCNS_ADC_CREST_FACTOR_PROP_CREST_FACTOR_LSB         0
#define Si2168B_MCNS_ADC_CREST_FACTOR_PROP_CREST_FACTOR_MASK        0xff
#define Si2168B_MCNS_ADC_CREST_FACTOR_PROP_CREST_FACTOR_DEFAULT    112
#endif /* Si2168B_MCNS_ADC_CREST_FACTOR_PROP */

/* Si2168B MCNS_AFC_RANGE property definition */
#define Si2168B_MCNS_AFC_RANGE_PROP 0x1603

#ifdef Si2168B_MCNS_AFC_RANGE_PROP
#define Si2168B_MCNS_AFC_RANGE_PROP_CODE 0x001603

typedef struct { /* Si2168B_MCNS_AFC_RANGE_PROP_struct */
	u16 range_khz;
} Si2168B_MCNS_AFC_RANGE_PROP_struct;

/* MCNS_AFC_RANGE property, RANGE_KHZ field definition (NO TITLE)*/
#define Si2168B_MCNS_AFC_RANGE_PROP_RANGE_KHZ_LSB         0
#define Si2168B_MCNS_AFC_RANGE_PROP_RANGE_KHZ_MASK        0xffff
#define Si2168B_MCNS_AFC_RANGE_PROP_RANGE_KHZ_DEFAULT    100
#endif /* Si2168B_MCNS_AFC_RANGE_PROP */

/* Si2168B MCNS_CONSTELLATION property definition */
#define Si2168B_MCNS_CONSTELLATION_PROP 0x1601

#ifdef Si2168B_MCNS_CONSTELLATION_PROP
#define Si2168B_MCNS_CONSTELLATION_PROP_CODE 0x001601

typedef struct { /* Si2168B_MCNS_CONSTELLATION_PROP_struct */
	u8  constellation;
} Si2168B_MCNS_CONSTELLATION_PROP_struct;

/* MCNS_CONSTELLATION property, CONSTELLATION field definition (NO TITLE)*/
#define Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_LSB         0
#define Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_MASK        0x3f
#define Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_DEFAULT    0
#define Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_AUTO    0
#define Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_QAM64   9
#define Si2168B_MCNS_CONSTELLATION_PROP_CONSTELLATION_QAM256  11

#endif /* Si2168B_MCNS_CONSTELLATION_PROP */

/* Si2168B MCNS_SYMBOL_RATE property definition */
#define Si2168B_MCNS_SYMBOL_RATE_PROP 0x1602

#ifdef Si2168B_MCNS_SYMBOL_RATE_PROP
#define Si2168B_MCNS_SYMBOL_RATE_PROP_CODE 0x001602

typedef struct { /* Si2168B_MCNS_SYMBOL_RATE_PROP_struct */
	u16 rate;
} Si2168B_MCNS_SYMBOL_RATE_PROP_struct;

/* MCNS_SYMBOL_RATE property, RATE field definition (NO TITLE)*/
#define Si2168B_MCNS_SYMBOL_RATE_PROP_RATE_LSB         0
#define Si2168B_MCNS_SYMBOL_RATE_PROP_RATE_MASK        0xffff
#define Si2168B_MCNS_SYMBOL_RATE_PROP_RATE_DEFAULT    6900
#endif /* Si2168B_MCNS_SYMBOL_RATE_PROP */

/* Si2168B SCAN_FMAX property definition */
#define Si2168B_SCAN_FMAX_PROP 0x0304

#ifdef Si2168B_SCAN_FMAX_PROP
#define Si2168B_SCAN_FMAX_PROP_CODE 0x000304

typedef struct { /* Si2168B_SCAN_FMAX_PROP_struct */
	u16 scan_fmax;
} Si2168B_SCAN_FMAX_PROP_struct;

/* SCAN_FMAX property, SCAN_FMAX field definition (NO TITLE)*/
#define Si2168B_SCAN_FMAX_PROP_SCAN_FMAX_LSB         0
#define Si2168B_SCAN_FMAX_PROP_SCAN_FMAX_MASK        0xffff
#define Si2168B_SCAN_FMAX_PROP_SCAN_FMAX_DEFAULT    0
#endif /* Si2168B_SCAN_FMAX_PROP */

/* Si2168B SCAN_FMIN property definition */
#define Si2168B_SCAN_FMIN_PROP 0x0303

#ifdef Si2168B_SCAN_FMIN_PROP
#define Si2168B_SCAN_FMIN_PROP_CODE 0x000303

typedef struct { /* Si2168B_SCAN_FMIN_PROP_struct */
	u16 scan_fmin;
} Si2168B_SCAN_FMIN_PROP_struct;

/* SCAN_FMIN property, SCAN_FMIN field definition (NO TITLE)*/
#define Si2168B_SCAN_FMIN_PROP_SCAN_FMIN_LSB         0
#define Si2168B_SCAN_FMIN_PROP_SCAN_FMIN_MASK        0xffff
#define Si2168B_SCAN_FMIN_PROP_SCAN_FMIN_DEFAULT    0
#endif /* Si2168B_SCAN_FMIN_PROP */

/* Si2168B SCAN_IEN property definition */
#define Si2168B_SCAN_IEN_PROP 0x0308

#ifdef Si2168B_SCAN_IEN_PROP
#define Si2168B_SCAN_IEN_PROP_CODE 0x000308

typedef struct { /* Si2168B_SCAN_IEN_PROP_struct */
	u8  buzien;
	u8  reqien;
} Si2168B_SCAN_IEN_PROP_struct;

/* SCAN_IEN property, BUZIEN field definition (NO TITLE)*/
#define Si2168B_SCAN_IEN_PROP_BUZIEN_LSB         0
#define Si2168B_SCAN_IEN_PROP_BUZIEN_MASK        0x01
#define Si2168B_SCAN_IEN_PROP_BUZIEN_DEFAULT    0
#define Si2168B_SCAN_IEN_PROP_BUZIEN_DISABLE  0
#define Si2168B_SCAN_IEN_PROP_BUZIEN_ENABLE   1

/* SCAN_IEN property, REQIEN field definition (NO TITLE)*/
#define Si2168B_SCAN_IEN_PROP_REQIEN_LSB         1
#define Si2168B_SCAN_IEN_PROP_REQIEN_MASK        0x01
#define Si2168B_SCAN_IEN_PROP_REQIEN_DEFAULT    0
#define Si2168B_SCAN_IEN_PROP_REQIEN_DISABLE  0
#define Si2168B_SCAN_IEN_PROP_REQIEN_ENABLE   1

#endif /* Si2168B_SCAN_IEN_PROP */

/* Si2168B SCAN_INT_SENSE property definition */
#define Si2168B_SCAN_INT_SENSE_PROP 0x0307

#ifdef Si2168B_SCAN_INT_SENSE_PROP
#define Si2168B_SCAN_INT_SENSE_PROP_CODE 0x000307

typedef struct { /* Si2168B_SCAN_INT_SENSE_PROP_struct */
	u8  buznegen;
	u8  buzposen;
	u8  reqnegen;
	u8  reqposen;
} Si2168B_SCAN_INT_SENSE_PROP_struct;

/* SCAN_INT_SENSE property, BUZNEGEN field definition (NO TITLE)*/
#define Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_LSB         0
#define Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_MASK        0x01
#define Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_DEFAULT    1
#define Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_DISABLE  0
#define Si2168B_SCAN_INT_SENSE_PROP_BUZNEGEN_ENABLE   1

/* SCAN_INT_SENSE property, BUZPOSEN field definition (NO TITLE)*/
#define Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_LSB         8
#define Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_MASK        0x01
#define Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_DEFAULT    0
#define Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_DISABLE  0
#define Si2168B_SCAN_INT_SENSE_PROP_BUZPOSEN_ENABLE   1

/* SCAN_INT_SENSE property, REQNEGEN field definition (NO TITLE)*/
#define Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_LSB         1
#define Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_MASK        0x01
#define Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_DEFAULT    0
#define Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_DISABLE  0
#define Si2168B_SCAN_INT_SENSE_PROP_REQNEGEN_ENABLE   1

/* SCAN_INT_SENSE property, REQPOSEN field definition (NO TITLE)*/
#define Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_LSB         9
#define Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_MASK        0x01
#define Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_DEFAULT    1
#define Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_DISABLE  0
#define Si2168B_SCAN_INT_SENSE_PROP_REQPOSEN_ENABLE   1

#endif /* Si2168B_SCAN_INT_SENSE_PROP */

/* Si2168B SCAN_SYMB_RATE_MAX property definition */
#define Si2168B_SCAN_SYMB_RATE_MAX_PROP 0x0306

#ifdef Si2168B_SCAN_SYMB_RATE_MAX_PROP
#define Si2168B_SCAN_SYMB_RATE_MAX_PROP_CODE 0x000306

typedef struct { /* Si2168B_SCAN_SYMB_RATE_MAX_PROP_struct */
	u16 scan_symb_rate_max;
} Si2168B_SCAN_SYMB_RATE_MAX_PROP_struct;

/* SCAN_SYMB_RATE_MAX property, SCAN_SYMB_RATE_MAX field definition (NO TITLE)*/
#define Si2168B_SCAN_SYMB_RATE_MAX_PROP_SCAN_SYMB_RATE_MAX_LSB         0
#define Si2168B_SCAN_SYMB_RATE_MAX_PROP_SCAN_SYMB_RATE_MAX_MASK        0xffff
#define Si2168B_SCAN_SYMB_RATE_MAX_PROP_SCAN_SYMB_RATE_MAX_DEFAULT    0
#endif /* Si2168B_SCAN_SYMB_RATE_MAX_PROP */

/* Si2168B SCAN_SYMB_RATE_MIN property definition */
#define Si2168B_SCAN_SYMB_RATE_MIN_PROP 0x0305

#ifdef Si2168B_SCAN_SYMB_RATE_MIN_PROP
#define Si2168B_SCAN_SYMB_RATE_MIN_PROP_CODE 0x000305

typedef struct { /* Si2168B_SCAN_SYMB_RATE_MIN_PROP_struct */
	u16 scan_symb_rate_min;
} Si2168B_SCAN_SYMB_RATE_MIN_PROP_struct;

/* SCAN_SYMB_RATE_MIN property, SCAN_SYMB_RATE_MIN field definition (NO TITLE)*/
#define Si2168B_SCAN_SYMB_RATE_MIN_PROP_SCAN_SYMB_RATE_MIN_LSB         0
#define Si2168B_SCAN_SYMB_RATE_MIN_PROP_SCAN_SYMB_RATE_MIN_MASK        0xffff
#define Si2168B_SCAN_SYMB_RATE_MIN_PROP_SCAN_SYMB_RATE_MIN_DEFAULT    0
#endif /* Si2168B_SCAN_SYMB_RATE_MIN_PROP */

/* Si2168B SCAN_TER_CONFIG property definition */
#define Si2168B_SCAN_TER_CONFIG_PROP 0x0301

#ifdef Si2168B_SCAN_TER_CONFIG_PROP
#define Si2168B_SCAN_TER_CONFIG_PROP_CODE 0x000301

typedef struct { /* Si2168B_SCAN_TER_CONFIG_PROP_struct */
	u8  analog_bw;
	u8  mode;
	u8  scan_debug;
	u8  search_analog;
} Si2168B_SCAN_TER_CONFIG_PROP_struct;

/* SCAN_TER_CONFIG property, ANALOG_BW field definition (NO TITLE)*/
#define Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_LSB         2
#define Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_MASK        0x03
#define Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_DEFAULT    3
#define Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_6MHZ  1
#define Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_7MHZ  2
#define Si2168B_SCAN_TER_CONFIG_PROP_ANALOG_BW_8MHZ  3

/* SCAN_TER_CONFIG property, MODE field definition (NO TITLE)*/
#define Si2168B_SCAN_TER_CONFIG_PROP_MODE_LSB         0
#define Si2168B_SCAN_TER_CONFIG_PROP_MODE_MASK        0x03
#define Si2168B_SCAN_TER_CONFIG_PROP_MODE_DEFAULT    0
#define Si2168B_SCAN_TER_CONFIG_PROP_MODE_BLIND_SCAN    0
#define Si2168B_SCAN_TER_CONFIG_PROP_MODE_MAPPING_SCAN  1
#define Si2168B_SCAN_TER_CONFIG_PROP_MODE_BLIND_LOCK    2

/* SCAN_TER_CONFIG property, SCAN_DEBUG field definition (NO TITLE)*/
#define Si2168B_SCAN_TER_CONFIG_PROP_SCAN_DEBUG_LSB         12
#define Si2168B_SCAN_TER_CONFIG_PROP_SCAN_DEBUG_MASK        0x0f
#define Si2168B_SCAN_TER_CONFIG_PROP_SCAN_DEBUG_DEFAULT    0
/* SCAN_TER_CONFIG property, SEARCH_ANALOG field definition (NO TITLE)*/
#define Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_LSB         4
#define Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_MASK        0x01
#define Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_DEFAULT    0
#define Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_DISABLE  0
#define Si2168B_SCAN_TER_CONFIG_PROP_SEARCH_ANALOG_ENABLE   1

#endif /* Si2168B_SCAN_TER_CONFIG_PROP */

/* STATUS fields definition */
/* STATUS, DDINT field definition (address 0, size 1, lsb 0, unsigned)*/
#define Si2168B_STATUS_DDINT_LSB         0
#define Si2168B_STATUS_DDINT_MASK        0x01
#define Si2168B_STATUS_DDINT_NOT_TRIGGERED  0
#define Si2168B_STATUS_DDINT_TRIGGERED      1
/* STATUS, SCANINT field definition (address 0, size 1, lsb 1, unsigned)*/
#define Si2168B_STATUS_SCANINT_LSB         1
#define Si2168B_STATUS_SCANINT_MASK        0x01
#define Si2168B_STATUS_SCANINT_NOT_TRIGGERED  0
#define Si2168B_STATUS_SCANINT_TRIGGERED      1
/* STATUS, ERR field definition (address 0, size 1, lsb 6, unsigned)*/
#define Si2168B_STATUS_ERR_LSB         6
#define Si2168B_STATUS_ERR_MASK        0x01
#define Si2168B_STATUS_ERR_ERROR     1
#define Si2168B_STATUS_ERR_NO_ERROR  0
/* STATUS, CTS field definition (address 0, size 1, lsb 7, unsigned)*/
#define Si2168B_STATUS_CTS_LSB         7
#define Si2168B_STATUS_CTS_MASK        0x01
#define Si2168B_STATUS_CTS_COMPLETED  1
#define Si2168B_STATUS_CTS_WAIT       0

/* Si2168B_CONFIG_CLKIO command definition */
#define Si2168B_CONFIG_CLKIO_CMD 0x18

#ifdef Si2168B_CONFIG_CLKIO_CMD
#define Si2168B_CONFIG_CLKIO_CMD_CODE 0x010018

typedef struct { /* Si2168B_CONFIG_CLKIO_CMD_struct */
	u8  output;
	u8  pre_driver_str;
	u8  driver_str;
} Si2168B_CONFIG_CLKIO_CMD_struct;

typedef struct { /* Si2168B_CONFIG_CLKIO_CMD_REPLY_struct */
	u8  mode;
	u8  pre_driver_str;
	u8  driver_str;
} Si2168B_CONFIG_CLKIO_CMD_REPLY_struct;

/* CONFIG_CLKIO command, OUTPUT field definition (address 1,size 2, lsb 0, unsigned) */
#define Si2168B_CONFIG_CLKIO_CMD_OUTPUT_LSB         0
#define Si2168B_CONFIG_CLKIO_CMD_OUTPUT_MASK        0x03
#define Si2168B_CONFIG_CLKIO_CMD_OUTPUT_MIN         0
#define Si2168B_CONFIG_CLKIO_CMD_OUTPUT_MAX         2
#define Si2168B_CONFIG_CLKIO_CMD_OUTPUT_NO_CHANGE  0
#define Si2168B_CONFIG_CLKIO_CMD_OUTPUT_OFF        2
#define Si2168B_CONFIG_CLKIO_CMD_OUTPUT_ON         1
/* CONFIG_CLKIO command, PRE_DRIVER_STR field definition (address 1,size 2, lsb 2, unsigned) */
#define Si2168B_CONFIG_CLKIO_CMD_PRE_DRIVER_STR_LSB         2
#define Si2168B_CONFIG_CLKIO_CMD_PRE_DRIVER_STR_MASK        0x03
#define Si2168B_CONFIG_CLKIO_CMD_PRE_DRIVER_STR_MIN         0
#define Si2168B_CONFIG_CLKIO_CMD_PRE_DRIVER_STR_MAX         3
#define Si2168B_CONFIG_CLKIO_CMD_PRE_DRIVER_STR_PRE_DRIVER_MIN  0
#define Si2168B_CONFIG_CLKIO_CMD_PRE_DRIVER_STR_PRE_DRIVER_MAX  3
/* CONFIG_CLKIO command, DRIVER_STR field definition (address 1,size 4, lsb 4, unsigned) */
#define Si2168B_CONFIG_CLKIO_CMD_DRIVER_STR_LSB         4
#define Si2168B_CONFIG_CLKIO_CMD_DRIVER_STR_MASK        0x0f
#define Si2168B_CONFIG_CLKIO_CMD_DRIVER_STR_MIN         0
#define Si2168B_CONFIG_CLKIO_CMD_DRIVER_STR_MAX         15
#define Si2168B_CONFIG_CLKIO_CMD_DRIVER_STR_DRIVER_MIN  0
#define Si2168B_CONFIG_CLKIO_CMD_DRIVER_STR_DRIVER_MAX  15
/* CONFIG_CLKIO command, MODE field definition (address 1, size 2, lsb 0, unsigned)*/
#define Si2168B_CONFIG_CLKIO_RESPONSE_MODE_LSB         0
#define Si2168B_CONFIG_CLKIO_RESPONSE_MODE_MASK        0x03
#define Si2168B_CONFIG_CLKIO_RESPONSE_MODE_CLK_INPUT   2
#define Si2168B_CONFIG_CLKIO_RESPONSE_MODE_CLK_OUTPUT  1
#define Si2168B_CONFIG_CLKIO_RESPONSE_MODE_UNUSED      0
/* CONFIG_CLKIO command, PRE_DRIVER_STR field definition (address 2, size 8, lsb 0, unsigned)*/
#define Si2168B_CONFIG_CLKIO_RESPONSE_PRE_DRIVER_STR_LSB         0
#define Si2168B_CONFIG_CLKIO_RESPONSE_PRE_DRIVER_STR_MASK        0xff
/* CONFIG_CLKIO command, DRIVER_STR field definition (address 3, size 8, lsb 0, unsigned)*/
#define Si2168B_CONFIG_CLKIO_RESPONSE_DRIVER_STR_LSB         0
#define Si2168B_CONFIG_CLKIO_RESPONSE_DRIVER_STR_MASK        0xff

#endif /* Si2168B_CONFIG_CLKIO_CMD */

/* Si2168B_CONFIG_PINS command definition */
#define Si2168B_CONFIG_PINS_CMD 0x12

#ifdef Si2168B_CONFIG_PINS_CMD
#define Si2168B_CONFIG_PINS_CMD_CODE 0x010012

typedef struct { /* Si2168B_CONFIG_PINS_CMD_struct */
	u8  gpio0_mode;
	u8  gpio0_read;
	u8  gpio1_mode;
	u8  gpio1_read;
} Si2168B_CONFIG_PINS_CMD_struct;

typedef struct { /* Si2168B_CONFIG_PINS_CMD_REPLY_struct */
	u8  gpio0_mode;
	u8  gpio0_state;
	u8  gpio1_mode;
	u8  gpio1_state;
}  Si2168B_CONFIG_PINS_CMD_REPLY_struct;

/* CONFIG_PINS command, GPIO0_MODE field definition (address 1,size 7, lsb 0, unsigned) */
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_LSB         0
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_MASK        0x7f
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_MIN         0
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_MAX         8
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_DISABLE    1
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_DRIVE_0    2
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_DRIVE_1    3
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_HW_LOCK    8
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_INT_FLAG   7
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_NO_CHANGE  0
#define Si2168B_CONFIG_PINS_CMD_GPIO0_MODE_TS_ERR     4
/* CONFIG_PINS command, GPIO0_READ field definition (address 1,size 1, lsb 7, unsigned) */
#define Si2168B_CONFIG_PINS_CMD_GPIO0_READ_LSB         7
#define Si2168B_CONFIG_PINS_CMD_GPIO0_READ_MASK        0x01
#define Si2168B_CONFIG_PINS_CMD_GPIO0_READ_MIN         0
#define Si2168B_CONFIG_PINS_CMD_GPIO0_READ_MAX         1
#define Si2168B_CONFIG_PINS_CMD_GPIO0_READ_DO_NOT_READ  0
#define Si2168B_CONFIG_PINS_CMD_GPIO0_READ_READ         1
/* CONFIG_PINS command, GPIO1_MODE field definition (address 2,size 7, lsb 0, unsigned) */
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_LSB         0
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_MASK        0x7f
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_MIN         0
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_MAX         8
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_DISABLE    1
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_DRIVE_0    2
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_DRIVE_1    3
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_HW_LOCK    8
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_INT_FLAG   7
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_NO_CHANGE  0
#define Si2168B_CONFIG_PINS_CMD_GPIO1_MODE_TS_ERR     4
/* CONFIG_PINS command, GPIO1_READ field definition (address 2,size 1, lsb 7, unsigned) */
#define Si2168B_CONFIG_PINS_CMD_GPIO1_READ_LSB         7
#define Si2168B_CONFIG_PINS_CMD_GPIO1_READ_MASK        0x01
#define Si2168B_CONFIG_PINS_CMD_GPIO1_READ_MIN         0
#define Si2168B_CONFIG_PINS_CMD_GPIO1_READ_MAX         1
#define Si2168B_CONFIG_PINS_CMD_GPIO1_READ_DO_NOT_READ  0
#define Si2168B_CONFIG_PINS_CMD_GPIO1_READ_READ         1
/* CONFIG_PINS command, GPIO0_MODE field definition (address 1, size 7, lsb 0, unsigned)*/
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_MODE_LSB         0
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_MODE_MASK        0x7f
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_MODE_DISABLE   1
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_MODE_DRIVE_0   2
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_MODE_DRIVE_1   3
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_MODE_HW_LOCK   8
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_MODE_INT_FLAG  7
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_MODE_TS_ERR    4
/* CONFIG_PINS command, GPIO0_STATE field definition (address 1, size 1, lsb 7, unsigned)*/
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_STATE_LSB         7
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_STATE_MASK        0x01
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_STATE_READ_0  0
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO0_STATE_READ_1  1
/* CONFIG_PINS command, GPIO1_MODE field definition (address 2, size 7, lsb 0, unsigned)*/
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_MODE_LSB         0
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_MODE_MASK        0x7f
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_MODE_DISABLE   1
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_MODE_DRIVE_0   2
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_MODE_DRIVE_1   3
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_MODE_HW_LOCK   8
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_MODE_INT_FLAG  7
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_MODE_TS_ERR    4
/* CONFIG_PINS command, GPIO1_STATE field definition (address 2, size 1, lsb 7, unsigned)*/
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_STATE_LSB         7
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_STATE_MASK        0x01
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_STATE_READ_0  0
#define Si2168B_CONFIG_PINS_RESPONSE_GPIO1_STATE_READ_1  1

#endif /* Si2168B_CONFIG_PINS_CMD */

/* Si2168B_DD_BER command definition */
#define Si2168B_DD_BER_CMD 0x82

#ifdef Si2168B_DD_BER_CMD
#define Si2168B_DD_BER_CMD_CODE 0x010082

typedef struct { /* Si2168B_DD_BER_CMD_struct */
	u8  rst;
} Si2168B_DD_BER_CMD_struct;

typedef struct { /* Si2168B_DD_BER_CMD_REPLY_struct */
	u8  exp;
	u8  mant;
}  Si2168B_DD_BER_CMD_REPLY_struct;

/* DD_BER command, RST field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_DD_BER_CMD_RST_LSB         0
#define Si2168B_DD_BER_CMD_RST_MASK        0x01
#define Si2168B_DD_BER_CMD_RST_MIN         0
#define Si2168B_DD_BER_CMD_RST_MAX         1
#define Si2168B_DD_BER_CMD_RST_CLEAR  1
#define Si2168B_DD_BER_CMD_RST_RUN    0
/* DD_BER command, EXP field definition (address 1, size 4, lsb 0, unsigned)*/
#define Si2168B_DD_BER_RESPONSE_EXP_LSB         0
#define Si2168B_DD_BER_RESPONSE_EXP_MASK        0x0f
#define Si2168B_DD_BER_RESPONSE_EXP_EXP_MIN  0
#define Si2168B_DD_BER_RESPONSE_EXP_EXP_MAX  8
/* DD_BER command, MANT field definition (address 2, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_BER_RESPONSE_MANT_LSB         0
#define Si2168B_DD_BER_RESPONSE_MANT_MASK        0xff
#define Si2168B_DD_BER_RESPONSE_MANT_MANT_MIN  0
#define Si2168B_DD_BER_RESPONSE_MANT_MANT_MAX  99

#endif /* Si2168B_DD_BER_CMD */

/* Si2168B_DD_CBER command definition */
#define Si2168B_DD_CBER_CMD 0x81

#ifdef Si2168B_DD_CBER_CMD
#define Si2168B_DD_CBER_CMD_CODE 0x010081

typedef struct { /* Si2168B_DD_CBER_CMD_struct */
	u8  rst;
} Si2168B_DD_CBER_CMD_struct;

typedef struct { /* Si2168B_DD_CBER_CMD_REPLY_struct */
	u8  exp;
	u8  mant;
}  Si2168B_DD_CBER_CMD_REPLY_struct;

/* DD_CBER command, RST field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_DD_CBER_CMD_RST_LSB         0
#define Si2168B_DD_CBER_CMD_RST_MASK        0x01
#define Si2168B_DD_CBER_CMD_RST_MIN         0
#define Si2168B_DD_CBER_CMD_RST_MAX         1
#define Si2168B_DD_CBER_CMD_RST_CLEAR  1
#define Si2168B_DD_CBER_CMD_RST_RUN    0
/* DD_CBER command, EXP field definition (address 1, size 4, lsb 0, unsigned)*/
#define Si2168B_DD_CBER_RESPONSE_EXP_LSB         0
#define Si2168B_DD_CBER_RESPONSE_EXP_MASK        0x0f
#define Si2168B_DD_CBER_RESPONSE_EXP_EXP_MIN  0
#define Si2168B_DD_CBER_RESPONSE_EXP_EXP_MAX  8
/* DD_CBER command, MANT field definition (address 2, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_CBER_RESPONSE_MANT_LSB         0
#define Si2168B_DD_CBER_RESPONSE_MANT_MASK        0xff
#define Si2168B_DD_CBER_RESPONSE_MANT_MANT_MIN  0
#define Si2168B_DD_CBER_RESPONSE_MANT_MANT_MAX  99

#endif /* Si2168B_DD_CBER_CMD */

/* Si2168B_DD_EXT_AGC_TER command definition */
#define Si2168B_DD_EXT_AGC_TER_CMD 0x89

#ifdef Si2168B_DD_EXT_AGC_TER_CMD
#define Si2168B_DD_EXT_AGC_TER_CMD_CODE 0x010089

typedef struct { /* Si2168B_DD_EXT_AGC_TER_CMD_struct */
	u8  agc_1_mode;
	u8  agc_1_inv;
	u8  agc_2_mode;
	u8  agc_2_inv;
	u8  agc_1_kloop;
	u8  agc_2_kloop;
	u8  agc_1_min;
	u8  agc_2_min;
} Si2168B_DD_EXT_AGC_TER_CMD_struct;

typedef struct { /* Si2168B_DD_EXT_AGC_TER_CMD_REPLY_struct */
	u8  agc_1_level;
	u8  agc_2_level;
}  Si2168B_DD_EXT_AGC_TER_CMD_REPLY_struct;

/* DD_EXT_AGC_TER command, AGC_1_MODE field definition (address 1,size 3, lsb 0, unsigned) */
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_LSB         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MASK        0x07
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MIN         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MAX         5
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MP_A       2
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MP_B       3
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MP_C       4
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_MP_D       5
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_NOT_USED   1
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MODE_NO_CHANGE  0
/* DD_EXT_AGC_TER command, AGC_1_INV field definition (address 1,size 1, lsb 3, unsigned) */
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_INV_LSB         3
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_INV_MASK        0x01
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_INV_MIN         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_INV_MAX         1
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_INV_INVERTED      1
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_INV_NOT_INVERTED  0
/* DD_EXT_AGC_TER command, AGC_2_MODE field definition (address 1,size 3, lsb 4, unsigned) */
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_LSB         4
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_MASK        0x07
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_MIN         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_MAX         5
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_MP_A       2
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_MP_B       3
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_MP_C       4
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_MP_D       5
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_NOT_USED   1
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MODE_NO_CHANGE  0
/* DD_EXT_AGC_TER command, AGC_2_INV field definition (address 1,size 1, lsb 7, unsigned) */
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_INV_LSB         7
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_INV_MASK        0x01
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_INV_MIN         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_INV_MAX         1
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_INV_INVERTED      1
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_INV_NOT_INVERTED  0
/* DD_EXT_AGC_TER command, AGC_1_KLOOP field definition (address 2,size 5, lsb 0, unsigned) */
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_KLOOP_LSB         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_KLOOP_MASK        0x1f
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_KLOOP_MIN         6
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_KLOOP_MAX         20
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_KLOOP_AGC_1_KLOOP_MIN  6
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_KLOOP_AGC_1_KLOOP_MAX  20
/* DD_EXT_AGC_TER command, AGC_2_KLOOP field definition (address 3,size 5, lsb 0, unsigned) */
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_KLOOP_LSB         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_KLOOP_MASK        0x1f
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_KLOOP_MIN         6
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_KLOOP_MAX         20
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_KLOOP_AGC_2_KLOOP_MIN  6
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_KLOOP_AGC_2_KLOOP_MAX  20
/* DD_EXT_AGC_TER command, AGC_1_MIN field definition (address 4,size 8, lsb 0, unsigned) */
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MIN_LSB         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MIN_MASK        0xff
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MIN_MIN         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MIN_MAX         255
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MIN_AGC_1_MIN_MIN  0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_1_MIN_AGC_1_MIN_MAX  255
/* DD_EXT_AGC_TER command, AGC_2_MIN field definition (address 5,size 8, lsb 0, unsigned) */
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MIN_LSB         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MIN_MASK        0xff
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MIN_MIN         0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MIN_MAX         255
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MIN_AGC_2_MIN_MIN  0
#define Si2168B_DD_EXT_AGC_TER_CMD_AGC_2_MIN_AGC_2_MIN_MAX  255
/* DD_EXT_AGC_TER command, AGC_1_LEVEL field definition (address 1, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_EXT_AGC_TER_RESPONSE_AGC_1_LEVEL_LSB         0
#define Si2168B_DD_EXT_AGC_TER_RESPONSE_AGC_1_LEVEL_MASK        0xff
/* DD_EXT_AGC_TER command, AGC_2_LEVEL field definition (address 2, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_EXT_AGC_TER_RESPONSE_AGC_2_LEVEL_LSB         0
#define Si2168B_DD_EXT_AGC_TER_RESPONSE_AGC_2_LEVEL_MASK        0xff

#endif /* Si2168B_DD_EXT_AGC_TER_CMD */

/* Si2168B_DD_FER command definition */
#define Si2168B_DD_FER_CMD 0x86

#ifdef Si2168B_DD_FER_CMD
#define Si2168B_DD_FER_CMD_CODE 0x010086

typedef struct { /* Si2168B_DD_FER_CMD_struct */
	u8  rst;
} Si2168B_DD_FER_CMD_struct;

typedef struct { /* Si2168B_DD_FER_CMD_REPLY_struct */
	u8  exp;
	u8  mant;
}  Si2168B_DD_FER_CMD_REPLY_struct;

/* DD_FER command, RST field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_DD_FER_CMD_RST_LSB         0
#define Si2168B_DD_FER_CMD_RST_MASK        0x01
#define Si2168B_DD_FER_CMD_RST_MIN         0
#define Si2168B_DD_FER_CMD_RST_MAX         1
#define Si2168B_DD_FER_CMD_RST_CLEAR  1
#define Si2168B_DD_FER_CMD_RST_RUN    0
/* DD_FER command, EXP field definition (address 1, size 4, lsb 0, unsigned)*/
#define Si2168B_DD_FER_RESPONSE_EXP_LSB         0
#define Si2168B_DD_FER_RESPONSE_EXP_MASK        0x0f
#define Si2168B_DD_FER_RESPONSE_EXP_EXP_MIN  0
#define Si2168B_DD_FER_RESPONSE_EXP_EXP_MAX  8
/* DD_FER command, MANT field definition (address 2, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_FER_RESPONSE_MANT_LSB         0
#define Si2168B_DD_FER_RESPONSE_MANT_MASK        0xff
#define Si2168B_DD_FER_RESPONSE_MANT_MANT_MIN  0
#define Si2168B_DD_FER_RESPONSE_MANT_MANT_MAX  99

#endif /* Si2168B_DD_FER_CMD */

/* Si2168B_DD_GET_REG command definition */
#define Si2168B_DD_GET_REG_CMD 0x8f

#ifdef Si2168B_DD_GET_REG_CMD
#define Si2168B_DD_GET_REG_CMD_CODE 0x01008f

typedef struct { /* Si2168B_DD_GET_REG_CMD_struct */
	u8  reg_code_lsb;
	u8  reg_code_mid;
	u8  reg_code_msb;
} Si2168B_DD_GET_REG_CMD_struct;


typedef struct { /* Si2168B_DD_GET_REG_CMD_REPLY_struct */
	u8  data1;
	u8  data2;
	u8  data3;
	u8  data4;
}  Si2168B_DD_GET_REG_CMD_REPLY_struct;

/* DD_GET_REG command, REG_CODE_LSB field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_DD_GET_REG_CMD_REG_CODE_LSB_LSB         0
#define Si2168B_DD_GET_REG_CMD_REG_CODE_LSB_MASK        0xff
#define Si2168B_DD_GET_REG_CMD_REG_CODE_LSB_MIN         0
#define Si2168B_DD_GET_REG_CMD_REG_CODE_LSB_MAX         255
#define Si2168B_DD_GET_REG_CMD_REG_CODE_LSB_REG_CODE_LSB_MIN  0
#define Si2168B_DD_GET_REG_CMD_REG_CODE_LSB_REG_CODE_LSB_MAX  255
/* DD_GET_REG command, REG_CODE_MID field definition (address 2,size 8, lsb 0, unsigned) */
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MID_LSB         0
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MID_MASK        0xff
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MID_MIN         0
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MID_MAX         255
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MID_REG_CODE_MID_MIN  0
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MID_REG_CODE_MID_MAX  255
/* DD_GET_REG command, REG_CODE_MSB field definition (address 3,size 8, lsb 0, unsigned) */
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MSB_LSB         0
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MSB_MASK        0xff
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MSB_MIN         0
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MSB_MAX         255
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MSB_REG_CODE_MSB_MIN  0
#define Si2168B_DD_GET_REG_CMD_REG_CODE_MSB_REG_CODE_MSB_MAX  255
/* DD_GET_REG command, DATA1 field definition (address 1, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_GET_REG_RESPONSE_DATA1_LSB         0
#define Si2168B_DD_GET_REG_RESPONSE_DATA1_MASK        0xff
#define Si2168B_DD_GET_REG_RESPONSE_DATA1_DATA1_MIN  0
#define Si2168B_DD_GET_REG_RESPONSE_DATA1_DATA1_MAX  255
/* DD_GET_REG command, DATA2 field definition (address 2, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_GET_REG_RESPONSE_DATA2_LSB         0
#define Si2168B_DD_GET_REG_RESPONSE_DATA2_MASK        0xff
#define Si2168B_DD_GET_REG_RESPONSE_DATA2_DATA2_MIN  0
#define Si2168B_DD_GET_REG_RESPONSE_DATA2_DATA2_MAX  255
/* DD_GET_REG command, DATA3 field definition (address 3, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_GET_REG_RESPONSE_DATA3_LSB         0
#define Si2168B_DD_GET_REG_RESPONSE_DATA3_MASK        0xff
#define Si2168B_DD_GET_REG_RESPONSE_DATA3_DATA3_MIN  0
#define Si2168B_DD_GET_REG_RESPONSE_DATA3_DATA3_MAX  255
/* DD_GET_REG command, DATA4 field definition (address 4, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_GET_REG_RESPONSE_DATA4_LSB         0
#define Si2168B_DD_GET_REG_RESPONSE_DATA4_MASK        0xff
#define Si2168B_DD_GET_REG_RESPONSE_DATA4_DATA4_MIN  0
#define Si2168B_DD_GET_REG_RESPONSE_DATA4_DATA4_MAX  255

#endif /* Si2168B_DD_GET_REG_CMD */

/* Si2168B_DD_MP_DEFAULTS command definition */
#define Si2168B_DD_MP_DEFAULTS_CMD 0x88

#ifdef Si2168B_DD_MP_DEFAULTS_CMD
#define Si2168B_DD_MP_DEFAULTS_CMD_CODE 0x010088

typedef struct { /* Si2168B_DD_MP_DEFAULTS_CMD_struct */
	u8  mp_a_mode;
	u8  mp_b_mode;
	u8  mp_c_mode;
	u8  mp_d_mode;
} Si2168B_DD_MP_DEFAULTS_CMD_struct;

typedef struct { /* Si2168B_DD_MP_DEFAULTS_CMD_REPLY_struct */
	u8  mp_a_mode;
	u8  mp_b_mode;
	u8  mp_c_mode;
	u8  mp_d_mode;
}  Si2168B_DD_MP_DEFAULTS_CMD_REPLY_struct;

/* DD_MP_DEFAULTS command, MP_A_MODE field definition (address 1,size 7, lsb 0, unsigned) */
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_LSB         0
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_MASK        0x7f
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_MIN         0
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_MAX         3
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_DISABLE    1
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_DRIVE_0    2
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_DRIVE_1    3
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_A_MODE_NO_CHANGE  0
/* DD_MP_DEFAULTS command, MP_B_MODE field definition (address 2,size 7, lsb 0, unsigned) */
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_LSB         0
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_MASK        0x7f
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_MIN         0
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_MAX         3
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_DISABLE    1
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_DRIVE_0    2
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_DRIVE_1    3
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_B_MODE_NO_CHANGE  0
/* DD_MP_DEFAULTS command, MP_C_MODE field definition (address 3,size 7, lsb 0, unsigned) */
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_LSB         0
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_MASK        0x7f
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_MIN         0
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_MAX         3
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_DISABLE    1
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_DRIVE_0    2
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_DRIVE_1    3
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_C_MODE_NO_CHANGE  0
/* DD_MP_DEFAULTS command, MP_D_MODE field definition (address 4,size 7, lsb 0, unsigned) */
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_LSB         0
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_MASK        0x7f
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_MIN         0
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_MAX         3
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_DISABLE    1
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_DRIVE_0    2
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_DRIVE_1    3
#define Si2168B_DD_MP_DEFAULTS_CMD_MP_D_MODE_NO_CHANGE  0
/* DD_MP_DEFAULTS command, MP_A_MODE field definition (address 1, size 7, lsb 0, unsigned)*/
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_LSB         0
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_MASK        0x7f
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_AGC_1           3
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_AGC_1_INVERTED  4
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_AGC_2           5
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_AGC_2_INVERTED  6
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_DISABLE         0
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_DRIVE_0         1
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_DRIVE_1         2
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_FEF             7
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_A_MODE_FEF_INVERTED    8
/* DD_MP_DEFAULTS command, MP_B_MODE field definition (address 2, size 7, lsb 0, unsigned)*/
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_LSB         0
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_MASK        0x7f
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_AGC_1           3
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_AGC_1_INVERTED  4
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_AGC_2           5
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_AGC_2_INVERTED  6
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_DISABLE         0
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_DRIVE_0         1
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_DRIVE_1         2
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_FEF             7
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_B_MODE_FEF_INVERTED    8
/* DD_MP_DEFAULTS command, MP_C_MODE field definition (address 3, size 7, lsb 0, unsigned)*/
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_LSB         0
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_MASK        0x7f
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_AGC_1           3
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_AGC_1_INVERTED  4
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_AGC_2           5
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_AGC_2_INVERTED  6
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_DISABLE         0
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_DRIVE_0         1
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_DRIVE_1         2
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_FEF             7
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_C_MODE_FEF_INVERTED    8
/* DD_MP_DEFAULTS command, MP_D_MODE field definition (address 4, size 7, lsb 0, unsigned)*/
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_LSB         0
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_MASK        0x7f
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_AGC_1           3
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_AGC_1_INVERTED  4
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_AGC_2           5
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_AGC_2_INVERTED  6
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_DISABLE         0
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_DRIVE_0         1
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_DRIVE_1         2
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_FEF             7
#define Si2168B_DD_MP_DEFAULTS_RESPONSE_MP_D_MODE_FEF_INVERTED    8

#endif /* Si2168B_DD_MP_DEFAULTS_CMD */

/* Si2168B_DD_PER command definition */
#define Si2168B_DD_PER_CMD 0x83

#ifdef Si2168B_DD_PER_CMD
#define Si2168B_DD_PER_CMD_CODE 0x010083

typedef struct { /* Si2168B_DD_PER_CMD_struct */
	u8  rst;
} Si2168B_DD_PER_CMD_struct;

typedef struct { /* Si2168B_DD_PER_CMD_REPLY_struct */
	u8  exp;
	u8  mant;
}  Si2168B_DD_PER_CMD_REPLY_struct;

/* DD_PER command, RST field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_DD_PER_CMD_RST_LSB         0
#define Si2168B_DD_PER_CMD_RST_MASK        0x01
#define Si2168B_DD_PER_CMD_RST_MIN         0
#define Si2168B_DD_PER_CMD_RST_MAX         1
#define Si2168B_DD_PER_CMD_RST_CLEAR  1
#define Si2168B_DD_PER_CMD_RST_RUN    0
/* DD_PER command, EXP field definition (address 1, size 4, lsb 0, unsigned)*/
#define Si2168B_DD_PER_RESPONSE_EXP_LSB         0
#define Si2168B_DD_PER_RESPONSE_EXP_MASK        0x0f
#define Si2168B_DD_PER_RESPONSE_EXP_EXP_MIN  0
#define Si2168B_DD_PER_RESPONSE_EXP_EXP_MAX  8
/* DD_PER command, MANT field definition (address 2, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_PER_RESPONSE_MANT_LSB         0
#define Si2168B_DD_PER_RESPONSE_MANT_MASK        0xff
#define Si2168B_DD_PER_RESPONSE_MANT_MANT_MIN  0
#define Si2168B_DD_PER_RESPONSE_MANT_MANT_MAX  99

#endif /* Si2168B_DD_PER_CMD */

/* Si2168B_DD_RESTART command definition */
#define Si2168B_DD_RESTART_CMD 0x85

#ifdef Si2168B_DD_RESTART_CMD
#define Si2168B_DD_RESTART_CMD_CODE 0x010085

typedef struct { /* Si2168B_DD_RESTART_CMD_struct */
	u8  nothing;
} Si2168B_DD_RESTART_CMD_struct;

#endif /* Si2168B_DD_RESTART_CMD */

/* Si2168B_DD_SET_REG command definition */
#define Si2168B_DD_SET_REG_CMD 0x8e

#ifdef Si2168B_DD_SET_REG_CMD
#define Si2168B_DD_SET_REG_CMD_CODE 0x01008e

typedef struct { /* Si2168B_DD_SET_REG_CMD_struct */
	u8  reg_code_lsb;
	u8  reg_code_mid;
	u8  reg_code_msb;
	u32 value;
} Si2168B_DD_SET_REG_CMD_struct;

/* DD_SET_REG command, REG_CODE_LSB field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_DD_SET_REG_CMD_REG_CODE_LSB_LSB         0
#define Si2168B_DD_SET_REG_CMD_REG_CODE_LSB_MASK        0xff
#define Si2168B_DD_SET_REG_CMD_REG_CODE_LSB_MIN         0
#define Si2168B_DD_SET_REG_CMD_REG_CODE_LSB_MAX         255
#define Si2168B_DD_SET_REG_CMD_REG_CODE_LSB_REG_CODE_LSB_MIN  0
#define Si2168B_DD_SET_REG_CMD_REG_CODE_LSB_REG_CODE_LSB_MAX  255
/* DD_SET_REG command, REG_CODE_MID field definition (address 2,size 8, lsb 0, unsigned) */
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MID_LSB         0
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MID_MASK        0xff
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MID_MIN         0
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MID_MAX         255
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MID_REG_CODE_MID_MIN  0
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MID_REG_CODE_MID_MAX  255
/* DD_SET_REG command, REG_CODE_MSB field definition (address 3,size 8, lsb 0, unsigned) */
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MSB_LSB         0
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MSB_MASK        0xff
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MSB_MIN         0
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MSB_MAX         255
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MSB_REG_CODE_MSB_MIN  0
#define Si2168B_DD_SET_REG_CMD_REG_CODE_MSB_REG_CODE_MSB_MAX  255
/* DD_SET_REG command, VALUE field definition (address 4,size 32, lsb 0, unsigned) */
#define Si2168B_DD_SET_REG_CMD_VALUE_LSB         0
#define Si2168B_DD_SET_REG_CMD_VALUE_MASK        0xffffffff
#define Si2168B_DD_SET_REG_CMD_VALUE_MIN         0
#define Si2168B_DD_SET_REG_CMD_VALUE_MAX         4294967295
#define Si2168B_DD_SET_REG_CMD_VALUE_VALUE_MIN  0
#define Si2168B_DD_SET_REG_CMD_VALUE_VALUE_MAX  4294967295
#endif /* Si2168B_DD_SET_REG_CMD */

/* Si2168B_DD_SSI_SQI command definition */
#define Si2168B_DD_SSI_SQI_CMD 0x8b

#ifdef Si2168B_DD_SSI_SQI_CMD
#define Si2168B_DD_SSI_SQI_CMD_CODE 0x01008b

typedef struct { /* Si2168B_DD_SSI_SQI_CMD_struct */
	s8  tuner_rssi;
} Si2168B_DD_SSI_SQI_CMD_struct;


typedef struct { /* Si2168B_DD_SSI_SQI_CMD_REPLY_struct */
	u8  ssi;
	s8  sqi;
}  Si2168B_DD_SSI_SQI_CMD_REPLY_struct;

/* DD_SSI_SQI command, TUNER_RSSI field definition (address 1,size 8, lsb 0, signed) */
#define Si2168B_DD_SSI_SQI_CMD_TUNER_RSSI_LSB         0
#define Si2168B_DD_SSI_SQI_CMD_TUNER_RSSI_MASK        0xff
#define Si2168B_DD_SSI_SQI_CMD_TUNER_RSSI_SHIFT       24
#define Si2168B_DD_SSI_SQI_CMD_TUNER_RSSI_MIN         -128
#define Si2168B_DD_SSI_SQI_CMD_TUNER_RSSI_MAX         127
/* DD_SSI_SQI command, SSI field definition (address 1, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_SSI_SQI_RESPONSE_SSI_LSB         0
#define Si2168B_DD_SSI_SQI_RESPONSE_SSI_MASK        0xff
#define Si2168B_DD_SSI_SQI_RESPONSE_SSI_SSI_MIN  0
#define Si2168B_DD_SSI_SQI_RESPONSE_SSI_SSI_MAX  100
/* DD_SSI_SQI command, SQI field definition (address 2, size 8, lsb 0, signed)*/
#define Si2168B_DD_SSI_SQI_RESPONSE_SQI_LSB         0
#define Si2168B_DD_SSI_SQI_RESPONSE_SQI_MASK        0xff
#define Si2168B_DD_SSI_SQI_RESPONSE_SQI_SHIFT       24
#define Si2168B_DD_SSI_SQI_RESPONSE_SQI_SQI_MIN  -1
#define Si2168B_DD_SSI_SQI_RESPONSE_SQI_SQI_MAX  100

#endif /* Si2168B_DD_SSI_SQI_CMD */

/* Si2168B_DD_STATUS command definition */
#define Si2168B_DD_STATUS_CMD 0x87

#ifdef Si2168B_DD_STATUS_CMD
#define Si2168B_DD_STATUS_CMD_CODE 0x010087

typedef struct { /* Si2168B_DD_STATUS_CMD_struct */
	u8  intack;
} Si2168B_DD_STATUS_CMD_struct;

typedef struct { /* Si2168B_DD_STATUS_CMD_REPLY_struct */
	u8  pclint;
	u8  dlint;
	u8  berint;
	u8  uncorint;
	u8  rsqint_bit5;
	u8  rsqint_bit6;
	u8  rsqint_bit7;
	u8  pcl;
	u8  dl;
	u8  ber;
	u8  uncor;
	u8  rsqstat_bit5;
	u8  rsqstat_bit6;
	u8  rsqstat_bit7;
	u8  modulation;
	u16 ts_bit_rate;
	u16 ts_clk_freq;
}  Si2168B_DD_STATUS_CMD_REPLY_struct;

/* DD_STATUS command, INTACK field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_DD_STATUS_CMD_INTACK_LSB         0
#define Si2168B_DD_STATUS_CMD_INTACK_MASK        0x01
#define Si2168B_DD_STATUS_CMD_INTACK_MIN         0
#define Si2168B_DD_STATUS_CMD_INTACK_MAX         1
#define Si2168B_DD_STATUS_CMD_INTACK_CLEAR  1
#define Si2168B_DD_STATUS_CMD_INTACK_OK     0
/* DD_STATUS command, PCLINT field definition (address 1, size 1, lsb 1, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_PCLINT_LSB         1
#define Si2168B_DD_STATUS_RESPONSE_PCLINT_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_PCLINT_CHANGED    1
#define Si2168B_DD_STATUS_RESPONSE_PCLINT_NO_CHANGE  0
/* DD_STATUS command, DLINT field definition (address 1, size 1, lsb 2, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_DLINT_LSB         2
#define Si2168B_DD_STATUS_RESPONSE_DLINT_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_DLINT_CHANGED    1
#define Si2168B_DD_STATUS_RESPONSE_DLINT_NO_CHANGE  0
/* DD_STATUS command, BERINT field definition (address 1, size 1, lsb 3, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_BERINT_LSB         3
#define Si2168B_DD_STATUS_RESPONSE_BERINT_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_BERINT_CHANGED    1
#define Si2168B_DD_STATUS_RESPONSE_BERINT_NO_CHANGE  0
/* DD_STATUS command, UNCORINT field definition (address 1, size 1, lsb 4, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_UNCORINT_LSB         4
#define Si2168B_DD_STATUS_RESPONSE_UNCORINT_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_UNCORINT_CHANGED    1
#define Si2168B_DD_STATUS_RESPONSE_UNCORINT_NO_CHANGE  0
/* DD_STATUS command, RSQINT_BIT5 field definition (address 1, size 1, lsb 5, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT5_LSB         5
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT5_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT5_CHANGED    1
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT5_NO_CHANGE  0
/* DD_STATUS command, RSQINT_BIT6 field definition (address 1, size 1, lsb 6, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT6_LSB         6
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT6_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT6_CHANGED    1
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT6_NO_CHANGE  0
/* DD_STATUS command, RSQINT_BIT7 field definition (address 1, size 1, lsb 7, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT7_LSB         7
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT7_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT7_CHANGED    1
#define Si2168B_DD_STATUS_RESPONSE_RSQINT_BIT7_NO_CHANGE  0
/* DD_STATUS command, PCL field definition (address 2, size 1, lsb 1, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_PCL_LSB         1
#define Si2168B_DD_STATUS_RESPONSE_PCL_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_PCL_LOCKED   1
#define Si2168B_DD_STATUS_RESPONSE_PCL_NO_LOCK  0
/* DD_STATUS command, DL field definition (address 2, size 1, lsb 2, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_DL_LSB         2
#define Si2168B_DD_STATUS_RESPONSE_DL_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_DL_LOCKED   1
#define Si2168B_DD_STATUS_RESPONSE_DL_NO_LOCK  0
/* DD_STATUS command, BER field definition (address 2, size 1, lsb 3, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_BER_LSB         3
#define Si2168B_DD_STATUS_RESPONSE_BER_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_BER_BER_ABOVE  1
#define Si2168B_DD_STATUS_RESPONSE_BER_BER_BELOW  0
/* DD_STATUS command, UNCOR field definition (address 2, size 1, lsb 4, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_UNCOR_LSB         4
#define Si2168B_DD_STATUS_RESPONSE_UNCOR_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_UNCOR_NO_UNCOR_FOUND  0
#define Si2168B_DD_STATUS_RESPONSE_UNCOR_UNCOR_FOUND     1
/* DD_STATUS command, RSQSTAT_BIT5 field definition (address 2, size 1, lsb 5, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT5_LSB         5
#define Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT5_MASK        0x01
#define Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT5_NO_CHANGE 0
#define Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT5_CHANGE    1
/* DD_STATUS command, RSQSTAT_BIT6 field definition (address 2, size 1, lsb 6, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT6_LSB         6
#define Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT6_MASK        0x01
/* DD_STATUS command, RSQSTAT_BIT7 field definition (address 2, size 1, lsb 7, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT7_LSB         7
#define Si2168B_DD_STATUS_RESPONSE_RSQSTAT_BIT7_MASK        0x01
/* DD_STATUS command, MODULATION field definition (address 3, size 4, lsb 0, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_MODULATION_LSB         0
#define Si2168B_DD_STATUS_RESPONSE_MODULATION_MASK        0x0f
#define Si2168B_DD_STATUS_RESPONSE_MODULATION_DSS    10
#define Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBC   3
#define Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBC2  11
#define Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBS   8
#define Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBS2  9
#define Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBT   2
#define Si2168B_DD_STATUS_RESPONSE_MODULATION_DVBT2  7
#define Si2168B_DD_STATUS_RESPONSE_MODULATION_MCNS   1
/* DD_STATUS command, TS_BIT_RATE field definition (address 4, size 16, lsb 0, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_TS_BIT_RATE_LSB         0
#define Si2168B_DD_STATUS_RESPONSE_TS_BIT_RATE_MASK        0xffff
/* DD_STATUS command, TS_CLK_FREQ field definition (address 6, size 16, lsb 0, unsigned)*/
#define Si2168B_DD_STATUS_RESPONSE_TS_CLK_FREQ_LSB         0
#define Si2168B_DD_STATUS_RESPONSE_TS_CLK_FREQ_MASK        0xffff

#endif /* Si2168B_DD_STATUS_CMD */

/* Si2168B_DD_UNCOR command definition */
#define Si2168B_DD_UNCOR_CMD 0x84

#ifdef Si2168B_DD_UNCOR_CMD
#define Si2168B_DD_UNCOR_CMD_CODE 0x010084

typedef struct { /* Si2168B_DD_UNCOR_CMD_struct */
	u8  rst;
} Si2168B_DD_UNCOR_CMD_struct;

typedef struct { /* Si2168B_DD_UNCOR_CMD_REPLY_struct */
	u8  uncor_lsb;
	u8  uncor_msb;
}  Si2168B_DD_UNCOR_CMD_REPLY_struct;

/* DD_UNCOR command, RST field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_DD_UNCOR_CMD_RST_LSB         0
#define Si2168B_DD_UNCOR_CMD_RST_MASK        0x01
#define Si2168B_DD_UNCOR_CMD_RST_MIN         0
#define Si2168B_DD_UNCOR_CMD_RST_MAX         1
#define Si2168B_DD_UNCOR_CMD_RST_CLEAR  1
#define Si2168B_DD_UNCOR_CMD_RST_RUN    0
/* DD_UNCOR command, UNCOR_LSB field definition (address 1, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_UNCOR_RESPONSE_UNCOR_LSB_LSB         0
#define Si2168B_DD_UNCOR_RESPONSE_UNCOR_LSB_MASK        0xff
/* DD_UNCOR command, UNCOR_MSB field definition (address 2, size 8, lsb 0, unsigned)*/
#define Si2168B_DD_UNCOR_RESPONSE_UNCOR_MSB_LSB         0
#define Si2168B_DD_UNCOR_RESPONSE_UNCOR_MSB_MASK        0xff

#endif /* Si2168B_DD_UNCOR_CMD */

/* Si2168B_DOWNLOAD_DATASET_CONTINUE command definition */
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD 0xb9

#ifdef Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_CODE 0x0100b9

typedef struct { /* Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_struct */
	u8  data0;
	u8  data1;
	u8  data2;
	u8  data3;
	u8  data4;
	u8  data5;
	u8  data6;
} Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_struct;

typedef struct { /* Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_REPLY_struct */
}  Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_REPLY_struct;

/* DOWNLOAD_DATASET_CONTINUE command, DATA0 field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA0_LSB         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA0_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA0_MIN         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA0_MAX         255
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA0_DATA0_MIN  0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA0_DATA0_MAX  255
/* DOWNLOAD_DATASET_CONTINUE command, DATA1 field definition (address 2,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA1_LSB         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA1_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA1_MIN         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA1_MAX         255
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA1_DATA1_MIN  0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA1_DATA1_MAX  255
/* DOWNLOAD_DATASET_CONTINUE command, DATA2 field definition (address 3,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA2_LSB         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA2_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA2_MIN         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA2_MAX         255
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA2_DATA2_MIN  0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA2_DATA2_MAX  255
/* DOWNLOAD_DATASET_CONTINUE command, DATA3 field definition (address 4,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA3_LSB         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA3_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA3_MIN         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA3_MAX         255
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA3_DATA3_MIN  0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA3_DATA3_MAX  255
/* DOWNLOAD_DATASET_CONTINUE command, DATA4 field definition (address 5,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA4_LSB         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA4_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA4_MIN         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA4_MAX         255
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA4_DATA4_MIN  0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA4_DATA4_MAX  255
/* DOWNLOAD_DATASET_CONTINUE command, DATA5 field definition (address 6,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA5_LSB         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA5_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA5_MIN         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA5_MAX         255
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA5_DATA5_MIN  0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA5_DATA5_MAX  255
/* DOWNLOAD_DATASET_CONTINUE command, DATA6 field definition (address 7,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA6_LSB         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA6_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA6_MIN         0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA6_MAX         255
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA6_DATA6_MIN  0
#define Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD_DATA6_DATA6_MAX  255
#endif /* Si2168B_DOWNLOAD_DATASET_CONTINUE_CMD */

/* Si2168B_DOWNLOAD_DATASET_START command definition */
#define Si2168B_DOWNLOAD_DATASET_START_CMD 0xb8

#ifdef Si2168B_DOWNLOAD_DATASET_START_CMD
#define Si2168B_DOWNLOAD_DATASET_START_CMD_CODE 0x0100b8

typedef struct { /* Si2168B_DOWNLOAD_DATASET_START_CMD_struct */
	u8  dataset_id;
	u8  dataset_checksum;
	u8  data0;
	u8  data1;
	u8  data2;
	u8  data3;
	u8  data4;
} Si2168B_DOWNLOAD_DATASET_START_CMD_struct;

/* DOWNLOAD_DATASET_START command, DATASET_ID field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_ID_LSB         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_ID_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_ID_MIN         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_ID_MAX         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_ID_RFU  0
/* DOWNLOAD_DATASET_START command, DATASET_CHECKSUM field definition (address 2,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_CHECKSUM_LSB         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_CHECKSUM_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_CHECKSUM_MIN         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_CHECKSUM_MAX         255
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_CHECKSUM_DATASET_CHECKSUM_MIN  0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATASET_CHECKSUM_DATASET_CHECKSUM_MAX  255
/* DOWNLOAD_DATASET_START command, DATA0 field definition (address 3,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA0_LSB         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA0_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA0_MIN         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA0_MAX         255
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA0_DATA0_MIN  0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA0_DATA0_MAX  255
/* DOWNLOAD_DATASET_START command, DATA1 field definition (address 4,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA1_LSB         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA1_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA1_MIN         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA1_MAX         255
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA1_DATA1_MIN  0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA1_DATA1_MAX  255
/* DOWNLOAD_DATASET_START command, DATA2 field definition (address 5,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA2_LSB         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA2_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA2_MIN         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA2_MAX         255
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA2_DATA2_MIN  0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA2_DATA2_MAX  255
/* DOWNLOAD_DATASET_START command, DATA3 field definition (address 6,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA3_LSB         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA3_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA3_MIN         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA3_MAX         255
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA3_DATA3_MIN  0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA3_DATA3_MAX  255
/* DOWNLOAD_DATASET_START command, DATA4 field definition (address 7,size 8, lsb 0, unsigned) */
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA4_LSB         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA4_MASK        0xff
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA4_MIN         0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA4_MAX         255
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA4_DATA4_MIN  0
#define Si2168B_DOWNLOAD_DATASET_START_CMD_DATA4_DATA4_MAX  255
#endif /* Si2168B_DOWNLOAD_DATASET_START_CMD */

/* Si2168B_DVBC_STATUS command definition */
#define Si2168B_DVBC_STATUS_CMD 0x90

#ifdef Si2168B_DVBC_STATUS_CMD
#define Si2168B_DVBC_STATUS_CMD_CODE 0x010090

typedef struct { /* Si2168B_DVBC_STATUS_CMD_struct */
	u8  intack;
} Si2168B_DVBC_STATUS_CMD_struct;

typedef struct { /* Si2168B_DVBC_STATUS_CMD_REPLY_struct */
	u8  pclint;
	u8  dlint;
	u8  berint;
	u8  uncorint;
	u8  pcl;
	u8  dl;
	u8  ber;
	u8  uncor;
	u8  cnr;
	s16 afc_freq;
	s16 timing_offset;
	u8  constellation;
	u8  sp_inv;
}  Si2168B_DVBC_STATUS_CMD_REPLY_struct;

/* DVBC_STATUS command, INTACK field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_DVBC_STATUS_CMD_INTACK_LSB         0
#define Si2168B_DVBC_STATUS_CMD_INTACK_MASK        0x01
#define Si2168B_DVBC_STATUS_CMD_INTACK_MIN         0
#define Si2168B_DVBC_STATUS_CMD_INTACK_MAX         1
#define Si2168B_DVBC_STATUS_CMD_INTACK_CLEAR  1
#define Si2168B_DVBC_STATUS_CMD_INTACK_OK     0
/* DVBC_STATUS command, PCLINT field definition (address 1, size 1, lsb 1, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_PCLINT_LSB         1
#define Si2168B_DVBC_STATUS_RESPONSE_PCLINT_MASK        0x01
#define Si2168B_DVBC_STATUS_RESPONSE_PCLINT_CHANGED    1
#define Si2168B_DVBC_STATUS_RESPONSE_PCLINT_NO_CHANGE  0
/* DVBC_STATUS command, DLINT field definition (address 1, size 1, lsb 2, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_DLINT_LSB         2
#define Si2168B_DVBC_STATUS_RESPONSE_DLINT_MASK        0x01
#define Si2168B_DVBC_STATUS_RESPONSE_DLINT_CHANGED    1
#define Si2168B_DVBC_STATUS_RESPONSE_DLINT_NO_CHANGE  0
/* DVBC_STATUS command, BERINT field definition (address 1, size 1, lsb 3, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_BERINT_LSB         3
#define Si2168B_DVBC_STATUS_RESPONSE_BERINT_MASK        0x01
#define Si2168B_DVBC_STATUS_RESPONSE_BERINT_CHANGED    1
#define Si2168B_DVBC_STATUS_RESPONSE_BERINT_NO_CHANGE  0
/* DVBC_STATUS command, UNCORINT field definition (address 1, size 1, lsb 4, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_UNCORINT_LSB         4
#define Si2168B_DVBC_STATUS_RESPONSE_UNCORINT_MASK        0x01
#define Si2168B_DVBC_STATUS_RESPONSE_UNCORINT_CHANGED    1
#define Si2168B_DVBC_STATUS_RESPONSE_UNCORINT_NO_CHANGE  0
/* DVBC_STATUS command, PCL field definition (address 2, size 1, lsb 1, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_PCL_LSB         1
#define Si2168B_DVBC_STATUS_RESPONSE_PCL_MASK        0x01
#define Si2168B_DVBC_STATUS_RESPONSE_PCL_LOCKED   1
#define Si2168B_DVBC_STATUS_RESPONSE_PCL_NO_LOCK  0
/* DVBC_STATUS command, DL field definition (address 2, size 1, lsb 2, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_DL_LSB         2
#define Si2168B_DVBC_STATUS_RESPONSE_DL_MASK        0x01
#define Si2168B_DVBC_STATUS_RESPONSE_DL_LOCKED   1
#define Si2168B_DVBC_STATUS_RESPONSE_DL_NO_LOCK  0
/* DVBC_STATUS command, BER field definition (address 2, size 1, lsb 3, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_BER_LSB         3
#define Si2168B_DVBC_STATUS_RESPONSE_BER_MASK        0x01
#define Si2168B_DVBC_STATUS_RESPONSE_BER_BER_ABOVE  1
#define Si2168B_DVBC_STATUS_RESPONSE_BER_BER_BELOW  0
/* DVBC_STATUS command, UNCOR field definition (address 2, size 1, lsb 4, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_UNCOR_LSB         4
#define Si2168B_DVBC_STATUS_RESPONSE_UNCOR_MASK        0x01
#define Si2168B_DVBC_STATUS_RESPONSE_UNCOR_NO_UNCOR_FOUND  0
#define Si2168B_DVBC_STATUS_RESPONSE_UNCOR_UNCOR_FOUND     1
/* DVBC_STATUS command, CNR field definition (address 3, size 8, lsb 0, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_CNR_LSB         0
#define Si2168B_DVBC_STATUS_RESPONSE_CNR_MASK        0xff
/* DVBC_STATUS command, AFC_FREQ field definition (address 4, size 16, lsb 0, signed)*/
#define Si2168B_DVBC_STATUS_RESPONSE_AFC_FREQ_LSB         0
#define Si2168B_DVBC_STATUS_RESPONSE_AFC_FREQ_MASK        0xffff
#define Si2168B_DVBC_STATUS_RESPONSE_AFC_FREQ_SHIFT       16
/* DVBC_STATUS command, TIMING_OFFSET field definition (address 6, size 16, lsb 0, signed)*/
#define Si2168B_DVBC_STATUS_RESPONSE_TIMING_OFFSET_LSB         0
#define Si2168B_DVBC_STATUS_RESPONSE_TIMING_OFFSET_MASK        0xffff
#define Si2168B_DVBC_STATUS_RESPONSE_TIMING_OFFSET_SHIFT       16
/* DVBC_STATUS command, CONSTELLATION field definition (address 8, size 6, lsb 0, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_LSB         0
#define Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_MASK        0x3f
#define Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_QAM128  10
#define Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_QAM16   7
#define Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_QAM256  11
#define Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_QAM32   8
#define Si2168B_DVBC_STATUS_RESPONSE_CONSTELLATION_QAM64   9
/* DVBC_STATUS command, SP_INV field definition (address 8, size 1, lsb 6, unsigned)*/
#define Si2168B_DVBC_STATUS_RESPONSE_SP_INV_LSB         6
#define Si2168B_DVBC_STATUS_RESPONSE_SP_INV_MASK        0x01
#define Si2168B_DVBC_STATUS_RESPONSE_SP_INV_INVERTED  1
#define Si2168B_DVBC_STATUS_RESPONSE_SP_INV_NORMAL    0

#endif /* Si2168B_DVBC_STATUS_CMD */

/* Si2168B_DVBT2_FEF command definition */
#define Si2168B_DVBT2_FEF_CMD 0x51

#ifdef Si2168B_DVBT2_FEF_CMD
#define Si2168B_DVBT2_FEF_CMD_CODE 0x010051

typedef struct { /* Si2168B_DVBT2_FEF_CMD_struct */
	u8  fef_tuner_flag;
	u8  fef_tuner_flag_inv;
} Si2168B_DVBT2_FEF_CMD_struct;

typedef struct { /* Si2168B_DVBT2_FEF_CMD_REPLY_struct */
	u8  fef_type;
	u32 fef_length;
	u32 fef_repetition;
}  Si2168B_DVBT2_FEF_CMD_REPLY_struct;

/* DVBT2_FEF command, FEF_TUNER_FLAG field definition (address 1,size 3, lsb 0, unsigned) */
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_LSB         0
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MASK        0x07
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MIN         0
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MAX         5
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_A       2
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_B       3
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_C       4
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_MP_D       5
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_NOT_USED   1
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_NO_CHANGE  0
/* DVBT2_FEF command, FEF_TUNER_FLAG_INV field definition (address 1,size 1, lsb 3, unsigned) */
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_LSB         3
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_MASK        0x01
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_MIN         0
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_MAX         1
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_FEF_HIGH  0
#define Si2168B_DVBT2_FEF_CMD_FEF_TUNER_FLAG_INV_FEF_LOW   1
/* DVBT2_FEF command, FEF_TYPE field definition (address 1, size 4, lsb 0, unsigned)*/
#define Si2168B_DVBT2_FEF_RESPONSE_FEF_TYPE_LSB         0
#define Si2168B_DVBT2_FEF_RESPONSE_FEF_TYPE_MASK        0x0f
/* DVBT2_FEF command, FEF_LENGTH field definition (address 4, size 32, lsb 0, unsigned)*/
#define Si2168B_DVBT2_FEF_RESPONSE_FEF_LENGTH_LSB         0
#define Si2168B_DVBT2_FEF_RESPONSE_FEF_LENGTH_MASK        0xffffffff
/* DVBT2_FEF command, FEF_REPETITION field definition (address 8, size 32, lsb 0, unsigned)*/
#define Si2168B_DVBT2_FEF_RESPONSE_FEF_REPETITION_LSB         0
#define Si2168B_DVBT2_FEF_RESPONSE_FEF_REPETITION_MASK        0xffffffff

#endif /* Si2168B_DVBT2_FEF_CMD */

/* Si2168B_DVBT2_PLP_INFO command definition */
#define Si2168B_DVBT2_PLP_INFO_CMD 0x53

#ifdef Si2168B_DVBT2_PLP_INFO_CMD
#define Si2168B_DVBT2_PLP_INFO_CMD_CODE 0x010053

typedef struct { /* Si2168B_DVBT2_PLP_INFO_CMD_struct */
	u8  plp_index;
} Si2168B_DVBT2_PLP_INFO_CMD_struct;

typedef struct { /* Si2168B_DVBT2_PLP_INFO_CMD_REPLY_struct */
	u8  plp_id;
	u8  reserved_1_1;
	u8  in_band_b_flag;
	u8  in_band_a_flag;
	u8  static_flag;
	u8  plp_mode;
	u8  reserved_1_2;
	u8  static_padding_flag;
	u8  plp_payload_type;
	u8  plp_type;
	u8  first_frame_idx_msb;
	u8  first_rf_idx;
	u8  ff_flag;
	u8  plp_group_id_msb;
	u8  first_frame_idx_lsb;
	u8  plp_mod_msb;
	u8  plp_cod;
	u8  plp_group_id_lsb;
	u8  plp_num_blocks_max_msb;
	u8  plp_fec_type;
	u8  plp_rot;
	u8  plp_mod_lsb;
	u8  frame_interval_msb;
	u8  plp_num_blocks_max_lsb;
	u8  time_il_length_msb;
	u8  frame_interval_lsb;
	u8  time_il_type;
	u8  time_il_length_lsb;
}  Si2168B_DVBT2_PLP_INFO_CMD_REPLY_struct;

/* DVBT2_PLP_INFO command, PLP_INDEX field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_DVBT2_PLP_INFO_CMD_PLP_INDEX_LSB         0
#define Si2168B_DVBT2_PLP_INFO_CMD_PLP_INDEX_MASK        0xff
#define Si2168B_DVBT2_PLP_INFO_CMD_PLP_INDEX_MIN         0
#define Si2168B_DVBT2_PLP_INFO_CMD_PLP_INDEX_MAX         255
/* DVBT2_PLP_INFO command, PLP_ID field definition (address 1, size 8, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_ID_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_ID_MASK        0xff
/* DVBT2_PLP_INFO command, RESERVED_1_1 field definition (address 10, size 6, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_RESERVED_1_1_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_RESERVED_1_1_MASK        0x3f
/* DVBT2_PLP_INFO command, IN_BAND_B_FLAG field definition (address 10, size 1, lsb 6, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_IN_BAND_B_FLAG_LSB         6
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_IN_BAND_B_FLAG_MASK        0x01
/* DVBT2_PLP_INFO command, IN_BAND_A_FLAG field definition (address 10, size 1, lsb 7, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_IN_BAND_A_FLAG_LSB         7
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_IN_BAND_A_FLAG_MASK        0x01
/* DVBT2_PLP_INFO command, STATIC_FLAG field definition (address 11, size 1, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_STATIC_FLAG_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_STATIC_FLAG_MASK        0x01
/* DVBT2_PLP_INFO command, PLP_MODE field definition (address 11, size 2, lsb 1, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MODE_LSB         1
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MODE_MASK        0x03
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MODE_HIGH_EFFICIENCY_MODE  2
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MODE_NORMAL_MODE           1
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MODE_NOT_SPECIFIED         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MODE_RESERVED              3
/* DVBT2_PLP_INFO command, RESERVED_1_2 field definition (address 11, size 5, lsb 3, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_RESERVED_1_2_LSB         3
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_RESERVED_1_2_MASK        0x1f
/* DVBT2_PLP_INFO command, STATIC_PADDING_FLAG field definition (address 12, size 1, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_STATIC_PADDING_FLAG_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_STATIC_PADDING_FLAG_MASK        0x01
/* DVBT2_PLP_INFO command, PLP_PAYLOAD_TYPE field definition (address 2, size 5, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_PAYLOAD_TYPE_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_PAYLOAD_TYPE_MASK        0x1f
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_PAYLOAD_TYPE_GCS   1
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_PAYLOAD_TYPE_GFPS  0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_PAYLOAD_TYPE_GSE   2
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_PAYLOAD_TYPE_TS    3
/* DVBT2_PLP_INFO command, PLP_TYPE field definition (address 2, size 3, lsb 5, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_TYPE_LSB         5
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_TYPE_MASK        0x07
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_TYPE_COMMON      0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_TYPE_DATA_TYPE1  1
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_TYPE_DATA_TYPE2  2
/* DVBT2_PLP_INFO command, FIRST_FRAME_IDX_MSB field definition (address 3, size 4, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_FRAME_IDX_MSB_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_FRAME_IDX_MSB_MASK        0x0f
/* DVBT2_PLP_INFO command, FIRST_RF_IDX field definition (address 3, size 3, lsb 4, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_RF_IDX_LSB         4
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_RF_IDX_MASK        0x07
/* DVBT2_PLP_INFO command, FF_FLAG field definition (address 3, size 1, lsb 7, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FF_FLAG_LSB         7
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FF_FLAG_MASK        0x01
/* DVBT2_PLP_INFO command, PLP_GROUP_ID_MSB field definition (address 4, size 4, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_GROUP_ID_MSB_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_GROUP_ID_MSB_MASK        0x0f
/* DVBT2_PLP_INFO command, FIRST_FRAME_IDX_LSB field definition (address 4, size 4, lsb 4, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_FRAME_IDX_LSB_LSB         4
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FIRST_FRAME_IDX_LSB_MASK        0x0f
/* DVBT2_PLP_INFO command, PLP_MOD_MSB field definition (address 5, size 1, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MOD_MSB_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MOD_MSB_MASK        0x01
/* DVBT2_PLP_INFO command, PLP_COD field definition (address 5, size 3, lsb 1, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_COD_LSB         1
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_COD_MASK        0x07
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_COD_1_2  0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_COD_2_3  2
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_COD_3_4  3
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_COD_3_5  1
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_COD_4_5  4
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_COD_5_6  5
/* DVBT2_PLP_INFO command, PLP_GROUP_ID_LSB field definition (address 5, size 4, lsb 4, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_GROUP_ID_LSB_LSB         4
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_GROUP_ID_LSB_MASK        0x0f
/* DVBT2_PLP_INFO command, PLP_NUM_BLOCKS_MAX_MSB field definition (address 6, size 3, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_NUM_BLOCKS_MAX_MSB_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_NUM_BLOCKS_MAX_MSB_MASK        0x07
/* DVBT2_PLP_INFO command, PLP_FEC_TYPE field definition (address 6, size 2, lsb 3, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_FEC_TYPE_LSB         3
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_FEC_TYPE_MASK        0x03
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_FEC_TYPE_16K_LDPC  0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_FEC_TYPE_64K_LDPC  1
/* DVBT2_PLP_INFO command, PLP_ROT field definition (address 6, size 1, lsb 5, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_ROT_LSB         5
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_ROT_MASK        0x01
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_ROT_NOT_ROTATED  0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_ROT_ROTATED      1
/* DVBT2_PLP_INFO command, PLP_MOD_LSB field definition (address 6, size 2, lsb 6, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MOD_LSB_LSB         6
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_MOD_LSB_MASK        0x03
/* DVBT2_PLP_INFO command, FRAME_INTERVAL_MSB field definition (address 7, size 1, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FRAME_INTERVAL_MSB_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FRAME_INTERVAL_MSB_MASK        0x01
/* DVBT2_PLP_INFO command, PLP_NUM_BLOCKS_MAX_LSB field definition (address 7, size 7, lsb 1, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_NUM_BLOCKS_MAX_LSB_LSB         1
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_PLP_NUM_BLOCKS_MAX_LSB_MASK        0x7f
/* DVBT2_PLP_INFO command, TIME_IL_LENGTH_MSB field definition (address 8, size 1, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_LENGTH_MSB_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_LENGTH_MSB_MASK        0x01
/* DVBT2_PLP_INFO command, FRAME_INTERVAL_LSB field definition (address 8, size 7, lsb 1, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FRAME_INTERVAL_LSB_LSB         1
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_FRAME_INTERVAL_LSB_MASK        0x7f
/* DVBT2_PLP_INFO command, TIME_IL_TYPE field definition (address 9, size 1, lsb 0, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_TYPE_LSB         0
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_TYPE_MASK        0x01
/* DVBT2_PLP_INFO command, TIME_IL_LENGTH_LSB field definition (address 9, size 7, lsb 1, unsigned)*/
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_LENGTH_LSB_LSB         1
#define Si2168B_DVBT2_PLP_INFO_RESPONSE_TIME_IL_LENGTH_LSB_MASK        0x7f

#endif /* Si2168B_DVBT2_PLP_INFO_CMD */

/* Si2168B_DVBT2_PLP_SELECT command definition */
#define Si2168B_DVBT2_PLP_SELECT_CMD 0x52

#ifdef Si2168B_DVBT2_PLP_SELECT_CMD
#define Si2168B_DVBT2_PLP_SELECT_CMD_CODE 0x010052

typedef struct { /* Si2168B_DVBT2_PLP_SELECT_CMD_struct */
	u8  plp_id;
	u8  plp_id_sel_mode;
} Si2168B_DVBT2_PLP_SELECT_CMD_struct;

/* DVBT2_PLP_SELECT command, PLP_ID field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_LSB         0
#define Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_MASK        0xff
#define Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_MIN         0
#define Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_MAX         255
/* DVBT2_PLP_SELECT command, PLP_ID_SEL_MODE field definition (address 2,size 1, lsb 0, unsigned) */
#define Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_LSB         0
#define Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_MASK        0x01
#define Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_MIN         0
#define Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_MAX         1
#define Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_AUTO    0
#define Si2168B_DVBT2_PLP_SELECT_CMD_PLP_ID_SEL_MODE_MANUAL  1
#endif /* Si2168B_DVBT2_PLP_SELECT_CMD */

/* Si2168B_DVBT2_STATUS command definition */
#define Si2168B_DVBT2_STATUS_CMD 0x50

#ifdef Si2168B_DVBT2_STATUS_CMD
#define Si2168B_DVBT2_STATUS_CMD_CODE 0x010050

typedef struct { /* Si2168B_DVBT2_STATUS_CMD_struct */
	u8  intack;
} Si2168B_DVBT2_STATUS_CMD_struct;

typedef struct { /* Si2168B_DVBT2_STATUS_CMD_REPLY_struct */
	u8  pclint;
	u8  dlint;
	u8  berint;
	u8  uncorint;
	u8  notdvbt2int;
	u8  num_plp;
	u8  pilot_pattern;
	u8  tx_mode;
	u8  rotated;
	u8  short_frame;
	u8  t2_mode;
	u8  code_rate;
	u8  t2_version;
	u8  plp_id;
	u8  pcl;
	u8  dl;
	u8  ber;
	u8  uncor;
	u8  notdvbt2;
	u8  cnr;
	s16 afc_freq;
	s16 timing_offset;
	u8  constellation;
	u8  sp_inv;
	u8  fef;
	u8  fft_mode;
	u8  guard_int;
	u8  bw_ext;
}  Si2168B_DVBT2_STATUS_CMD_REPLY_struct;

/* DVBT2_STATUS command, INTACK field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_DVBT2_STATUS_CMD_INTACK_LSB         0
#define Si2168B_DVBT2_STATUS_CMD_INTACK_MASK        0x01
#define Si2168B_DVBT2_STATUS_CMD_INTACK_MIN         0
#define Si2168B_DVBT2_STATUS_CMD_INTACK_MAX         1
#define Si2168B_DVBT2_STATUS_CMD_INTACK_CLEAR  1
#define Si2168B_DVBT2_STATUS_CMD_INTACK_OK     0
/* DVBT2_STATUS command, PCLINT field definition (address 1, size 1, lsb 1, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_PCLINT_LSB         1
#define Si2168B_DVBT2_STATUS_RESPONSE_PCLINT_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_PCLINT_CHANGED    1
#define Si2168B_DVBT2_STATUS_RESPONSE_PCLINT_NO_CHANGE  0
/* DVBT2_STATUS command, DLINT field definition (address 1, size 1, lsb 2, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_DLINT_LSB         2
#define Si2168B_DVBT2_STATUS_RESPONSE_DLINT_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_DLINT_CHANGED    1
#define Si2168B_DVBT2_STATUS_RESPONSE_DLINT_NO_CHANGE  0
/* DVBT2_STATUS command, BERINT field definition (address 1, size 1, lsb 3, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_BERINT_LSB         3
#define Si2168B_DVBT2_STATUS_RESPONSE_BERINT_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_BERINT_CHANGED    1
#define Si2168B_DVBT2_STATUS_RESPONSE_BERINT_NO_CHANGE  0
/* DVBT2_STATUS command, UNCORINT field definition (address 1, size 1, lsb 4, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_UNCORINT_LSB         4
#define Si2168B_DVBT2_STATUS_RESPONSE_UNCORINT_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_UNCORINT_CHANGED    1
#define Si2168B_DVBT2_STATUS_RESPONSE_UNCORINT_NO_CHANGE  0
/* DVBT2_STATUS command, NOTDVBT2INT field definition (address 1, size 1, lsb 5, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2INT_LSB         5
#define Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2INT_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2INT_CHANGED    1
#define Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2INT_NO_CHANGE  0
/* DVBT2_STATUS command, NUM_PLP field definition (address 10, size 8, lsb 0, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_NUM_PLP_LSB         0
#define Si2168B_DVBT2_STATUS_RESPONSE_NUM_PLP_MASK        0xff
/* DVBT2_STATUS command, PILOT_PATTERN field definition (address 11, size 4, lsb 0, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_LSB         0
#define Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_MASK        0x0f
#define Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_PP1  0
#define Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_PP2  1
#define Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_PP3  2
#define Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_PP4  3
#define Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_PP5  4
#define Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_PP6  5
#define Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_PP7  6
#define Si2168B_DVBT2_STATUS_RESPONSE_PILOT_PATTERN_PP8  7
/* DVBT2_STATUS command, TX_MODE field definition (address 11, size 1, lsb 4, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_TX_MODE_LSB         4
#define Si2168B_DVBT2_STATUS_RESPONSE_TX_MODE_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_TX_MODE_MISO  1
#define Si2168B_DVBT2_STATUS_RESPONSE_TX_MODE_SISO  0
/* DVBT2_STATUS command, ROTATED field definition (address 11, size 1, lsb 5, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_ROTATED_LSB         5
#define Si2168B_DVBT2_STATUS_RESPONSE_ROTATED_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_ROTATED_NORMAL   0
#define Si2168B_DVBT2_STATUS_RESPONSE_ROTATED_ROTATED  1
/* DVBT2_STATUS command, SHORT_FRAME field definition (address 11, size 1, lsb 6, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_SHORT_FRAME_LSB         6
#define Si2168B_DVBT2_STATUS_RESPONSE_SHORT_FRAME_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_SHORT_FRAME_16K_LDPC  0
#define Si2168B_DVBT2_STATUS_RESPONSE_SHORT_FRAME_64K_LDPC  1
/* DVBT2_STATUS command, T2_MODE field definition (address 11, size 1, lsb 7, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_T2_MODE_LSB         7
#define Si2168B_DVBT2_STATUS_RESPONSE_T2_MODE_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_T2_MODE_BASE  0
#define Si2168B_DVBT2_STATUS_RESPONSE_T2_MODE_LITE  1
/* DVBT2_STATUS command, CODE_RATE field definition (address 12, size 4, lsb 0, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_LSB         0
#define Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_MASK        0x0f
#define Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_1_2  1
#define Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_1_3  10
#define Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_2_3  2
#define Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_2_5  12
#define Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_3_4  3
#define Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_3_5  13
#define Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_4_5  4
#define Si2168B_DVBT2_STATUS_RESPONSE_CODE_RATE_5_6  5
/* DVBT2_STATUS command, T2_VERSION field definition (address 12, size 4, lsb 4, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_T2_VERSION_LSB         4
#define Si2168B_DVBT2_STATUS_RESPONSE_T2_VERSION_MASK        0x0f
#define Si2168B_DVBT2_STATUS_RESPONSE_T2_VERSION_1_1_1  0
#define Si2168B_DVBT2_STATUS_RESPONSE_T2_VERSION_1_2_1  1
#define Si2168B_DVBT2_STATUS_RESPONSE_T2_VERSION_1_3_1  2
/* DVBT2_STATUS command, PLP_ID field definition (address 13, size 8, lsb 0, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_PLP_ID_LSB         0
#define Si2168B_DVBT2_STATUS_RESPONSE_PLP_ID_MASK        0xff
/* DVBT2_STATUS command, PCL field definition (address 2, size 1, lsb 1, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_PCL_LSB         1
#define Si2168B_DVBT2_STATUS_RESPONSE_PCL_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_PCL_LOCKED   1
#define Si2168B_DVBT2_STATUS_RESPONSE_PCL_NO_LOCK  0
/* DVBT2_STATUS command, DL field definition (address 2, size 1, lsb 2, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_DL_LSB         2
#define Si2168B_DVBT2_STATUS_RESPONSE_DL_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_DL_LOCKED   1
#define Si2168B_DVBT2_STATUS_RESPONSE_DL_NO_LOCK  0
/* DVBT2_STATUS command, BER field definition (address 2, size 1, lsb 3, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_BER_LSB         3
#define Si2168B_DVBT2_STATUS_RESPONSE_BER_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_BER_BER_ABOVE  1
#define Si2168B_DVBT2_STATUS_RESPONSE_BER_BER_BELOW  0
/* DVBT2_STATUS command, UNCOR field definition (address 2, size 1, lsb 4, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_UNCOR_LSB         4
#define Si2168B_DVBT2_STATUS_RESPONSE_UNCOR_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_UNCOR_NO_UNCOR_FOUND  0
#define Si2168B_DVBT2_STATUS_RESPONSE_UNCOR_UNCOR_FOUND     1
/* DVBT2_STATUS command, NOTDVBT2 field definition (address 2, size 1, lsb 5, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2_LSB         5
#define Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2_DVBT2      0
#define Si2168B_DVBT2_STATUS_RESPONSE_NOTDVBT2_NOT_DVBT2  1
/* DVBT2_STATUS command, CNR field definition (address 3, size 8, lsb 0, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_CNR_LSB         0
#define Si2168B_DVBT2_STATUS_RESPONSE_CNR_MASK        0xff
/* DVBT2_STATUS command, AFC_FREQ field definition (address 4, size 16, lsb 0, signed)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_AFC_FREQ_LSB         0
#define Si2168B_DVBT2_STATUS_RESPONSE_AFC_FREQ_MASK        0xffff
#define Si2168B_DVBT2_STATUS_RESPONSE_AFC_FREQ_SHIFT       16
/* DVBT2_STATUS command, TIMING_OFFSET field definition (address 6, size 16, lsb 0, signed)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_TIMING_OFFSET_LSB         0
#define Si2168B_DVBT2_STATUS_RESPONSE_TIMING_OFFSET_MASK        0xffff
#define Si2168B_DVBT2_STATUS_RESPONSE_TIMING_OFFSET_SHIFT       16
/* DVBT2_STATUS command, CONSTELLATION field definition (address 8, size 6, lsb 0, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_LSB         0
#define Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_MASK        0x3f
#define Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QAM128  10
#define Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QAM16   7
#define Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QAM256  11
#define Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QAM32   8
#define Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QAM64   9
#define Si2168B_DVBT2_STATUS_RESPONSE_CONSTELLATION_QPSK    3
/* DVBT2_STATUS command, SP_INV field definition (address 8, size 1, lsb 6, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_SP_INV_LSB         6
#define Si2168B_DVBT2_STATUS_RESPONSE_SP_INV_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_SP_INV_INVERTED  1
#define Si2168B_DVBT2_STATUS_RESPONSE_SP_INV_NORMAL    0
/* DVBT2_STATUS command, FEF field definition (address 8, size 1, lsb 7, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_FEF_LSB         7
#define Si2168B_DVBT2_STATUS_RESPONSE_FEF_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_FEF_FEF     1
#define Si2168B_DVBT2_STATUS_RESPONSE_FEF_NO_FEF  0
/* DVBT2_STATUS command, FFT_MODE field definition (address 9, size 4, lsb 0, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_LSB         0
#define Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_MASK        0x0f
#define Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_16K  14
#define Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_1K   10
#define Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_2K   11
#define Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_32K  15
#define Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_4K   12
#define Si2168B_DVBT2_STATUS_RESPONSE_FFT_MODE_8K   13
/* DVBT2_STATUS command, GUARD_INT field definition (address 9, size 3, lsb 4, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_LSB         4
#define Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_MASK        0x07
#define Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_19_128  6
#define Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_19_256  7
#define Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_1_128   5
#define Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_1_16    2
#define Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_1_32    1
#define Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_1_4     4
#define Si2168B_DVBT2_STATUS_RESPONSE_GUARD_INT_1_8     3
/* DVBT2_STATUS command, BW_EXT field definition (address 9, size 1, lsb 7, unsigned)*/
#define Si2168B_DVBT2_STATUS_RESPONSE_BW_EXT_LSB         7
#define Si2168B_DVBT2_STATUS_RESPONSE_BW_EXT_MASK        0x01
#define Si2168B_DVBT2_STATUS_RESPONSE_BW_EXT_EXTENDED  1
#define Si2168B_DVBT2_STATUS_RESPONSE_BW_EXT_NORMAL    0

#endif /* Si2168B_DVBT2_STATUS_CMD */

/* Si2168B_DVBT2_TX_ID command definition */
#define Si2168B_DVBT2_TX_ID_CMD 0x54

#ifdef Si2168B_DVBT2_TX_ID_CMD
#define Si2168B_DVBT2_TX_ID_CMD_CODE 0x010054

typedef struct { /* Si2168B_DVBT2_TX_ID_CMD_struct */
	u8  nothing;
} Si2168B_DVBT2_TX_ID_CMD_struct;

typedef struct { /* Si2168B_DVBT2_TX_ID_CMD_REPLY_struct */
	u8  tx_id_availability;
	u16 cell_id;
	u16 network_id;
	u16 t2_system_id;
}  Si2168B_DVBT2_TX_ID_CMD_REPLY_struct;

/* DVBT2_TX_ID command, TX_ID_AVAILABILITY field definition (address 1, size 8, lsb 0, unsigned)*/
#define Si2168B_DVBT2_TX_ID_RESPONSE_TX_ID_AVAILABILITY_LSB         0
#define Si2168B_DVBT2_TX_ID_RESPONSE_TX_ID_AVAILABILITY_MASK        0xff
/* DVBT2_TX_ID command, CELL_ID field definition (address 2, size 16, lsb 0, unsigned)*/
#define Si2168B_DVBT2_TX_ID_RESPONSE_CELL_ID_LSB         0
#define Si2168B_DVBT2_TX_ID_RESPONSE_CELL_ID_MASK        0xffff
/* DVBT2_TX_ID command, NETWORK_ID field definition (address 4, size 16, lsb 0, unsigned)*/
#define Si2168B_DVBT2_TX_ID_RESPONSE_NETWORK_ID_LSB         0
#define Si2168B_DVBT2_TX_ID_RESPONSE_NETWORK_ID_MASK        0xffff
/* DVBT2_TX_ID command, T2_SYSTEM_ID field definition (address 6, size 16, lsb 0, unsigned)*/
#define Si2168B_DVBT2_TX_ID_RESPONSE_T2_SYSTEM_ID_LSB         0
#define Si2168B_DVBT2_TX_ID_RESPONSE_T2_SYSTEM_ID_MASK        0xffff

#endif /* Si2168B_DVBT2_TX_ID_CMD */

/* Si2168B_DVBT_STATUS command definition */
#define Si2168B_DVBT_STATUS_CMD 0xa0

#ifdef Si2168B_DVBT_STATUS_CMD
#define Si2168B_DVBT_STATUS_CMD_CODE 0x0100a0

typedef struct { /* Si2168B_DVBT_STATUS_CMD_struct */
	u8  intack;
} Si2168B_DVBT_STATUS_CMD_struct;

typedef struct { /* Si2168B_DVBT_STATUS_CMD_REPLY_struct */
	u8  pclint;
	u8  dlint;
	u8  berint;
	u8  uncorint;
	u8  notdvbtint;
	u8  fft_mode;
	u8  guard_int;
	u8  hierarchy;
	s8  tps_length;
	u8  pcl;
	u8  dl;
	u8  ber;
	u8  uncor;
	u8  notdvbt;
	u8  cnr;
	s16 afc_freq;
	s16 timing_offset;
	u8  constellation;
	u8  sp_inv;
	u8  rate_hp;
	u8  rate_lp;
}  Si2168B_DVBT_STATUS_CMD_REPLY_struct;

/* DVBT_STATUS command, INTACK field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_DVBT_STATUS_CMD_INTACK_LSB         0
#define Si2168B_DVBT_STATUS_CMD_INTACK_MASK        0x01
#define Si2168B_DVBT_STATUS_CMD_INTACK_MIN         0
#define Si2168B_DVBT_STATUS_CMD_INTACK_MAX         1
#define Si2168B_DVBT_STATUS_CMD_INTACK_CLEAR  1
#define Si2168B_DVBT_STATUS_CMD_INTACK_OK     0
/* DVBT_STATUS command, PCLINT field definition (address 1, size 1, lsb 1, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_PCLINT_LSB         1
#define Si2168B_DVBT_STATUS_RESPONSE_PCLINT_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_PCLINT_CHANGED    1
#define Si2168B_DVBT_STATUS_RESPONSE_PCLINT_NO_CHANGE  0
/* DVBT_STATUS command, DLINT field definition (address 1, size 1, lsb 2, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_DLINT_LSB         2
#define Si2168B_DVBT_STATUS_RESPONSE_DLINT_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_DLINT_CHANGED    1
#define Si2168B_DVBT_STATUS_RESPONSE_DLINT_NO_CHANGE  0
/* DVBT_STATUS command, BERINT field definition (address 1, size 1, lsb 3, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_BERINT_LSB         3
#define Si2168B_DVBT_STATUS_RESPONSE_BERINT_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_BERINT_CHANGED    1
#define Si2168B_DVBT_STATUS_RESPONSE_BERINT_NO_CHANGE  0
/* DVBT_STATUS command, UNCORINT field definition (address 1, size 1, lsb 4, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_UNCORINT_LSB         4
#define Si2168B_DVBT_STATUS_RESPONSE_UNCORINT_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_UNCORINT_CHANGED    1
#define Si2168B_DVBT_STATUS_RESPONSE_UNCORINT_NO_CHANGE  0
/* DVBT_STATUS command, NOTDVBTINT field definition (address 1, size 1, lsb 5, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_NOTDVBTINT_LSB         5
#define Si2168B_DVBT_STATUS_RESPONSE_NOTDVBTINT_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_NOTDVBTINT_CHANGED    1
#define Si2168B_DVBT_STATUS_RESPONSE_NOTDVBTINT_NO_CHANGE  0
/* DVBT_STATUS command, FFT_MODE field definition (address 10, size 4, lsb 0, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_FFT_MODE_LSB         0
#define Si2168B_DVBT_STATUS_RESPONSE_FFT_MODE_MASK        0x0f
#define Si2168B_DVBT_STATUS_RESPONSE_FFT_MODE_2K  11
#define Si2168B_DVBT_STATUS_RESPONSE_FFT_MODE_4K  12
#define Si2168B_DVBT_STATUS_RESPONSE_FFT_MODE_8K  13
/* DVBT_STATUS command, GUARD_INT field definition (address 10, size 3, lsb 4, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_LSB         4
#define Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_MASK        0x07
#define Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_1_16  2
#define Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_1_32  1
#define Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_1_4   4
#define Si2168B_DVBT_STATUS_RESPONSE_GUARD_INT_1_8   3
/* DVBT_STATUS command, HIERARCHY field definition (address 11, size 3, lsb 0, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_LSB         0
#define Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_MASK        0x07
#define Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_ALFA1  2
#define Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_ALFA2  3
#define Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_ALFA4  5
#define Si2168B_DVBT_STATUS_RESPONSE_HIERARCHY_NONE   1
/* DVBT_STATUS command, TPS_LENGTH field definition (address 12, size 7, lsb 0, signed)*/
#define Si2168B_DVBT_STATUS_RESPONSE_TPS_LENGTH_LSB         0
#define Si2168B_DVBT_STATUS_RESPONSE_TPS_LENGTH_MASK        0x7f
#define Si2168B_DVBT_STATUS_RESPONSE_TPS_LENGTH_SHIFT       25
/* DVBT_STATUS command, PCL field definition (address 2, size 1, lsb 1, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_PCL_LSB         1
#define Si2168B_DVBT_STATUS_RESPONSE_PCL_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_PCL_LOCKED   1
#define Si2168B_DVBT_STATUS_RESPONSE_PCL_NO_LOCK  0
/* DVBT_STATUS command, DL field definition (address 2, size 1, lsb 2, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_DL_LSB         2
#define Si2168B_DVBT_STATUS_RESPONSE_DL_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_DL_LOCKED   1
#define Si2168B_DVBT_STATUS_RESPONSE_DL_NO_LOCK  0
/* DVBT_STATUS command, BER field definition (address 2, size 1, lsb 3, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_BER_LSB         3
#define Si2168B_DVBT_STATUS_RESPONSE_BER_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_BER_BER_ABOVE  1
#define Si2168B_DVBT_STATUS_RESPONSE_BER_BER_BELOW  0
/* DVBT_STATUS command, UNCOR field definition (address 2, size 1, lsb 4, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_UNCOR_LSB         4
#define Si2168B_DVBT_STATUS_RESPONSE_UNCOR_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_UNCOR_NO_UNCOR_FOUND  0
#define Si2168B_DVBT_STATUS_RESPONSE_UNCOR_UNCOR_FOUND     1
/* DVBT_STATUS command, NOTDVBT field definition (address 2, size 1, lsb 5, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_NOTDVBT_LSB         5
#define Si2168B_DVBT_STATUS_RESPONSE_NOTDVBT_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_NOTDVBT_DVBT      0
#define Si2168B_DVBT_STATUS_RESPONSE_NOTDVBT_NOT_DVBT  1
/* DVBT_STATUS command, CNR field definition (address 3, size 8, lsb 0, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_CNR_LSB         0
#define Si2168B_DVBT_STATUS_RESPONSE_CNR_MASK        0xff
/* DVBT_STATUS command, AFC_FREQ field definition (address 4, size 16, lsb 0, signed)*/
#define Si2168B_DVBT_STATUS_RESPONSE_AFC_FREQ_LSB         0
#define Si2168B_DVBT_STATUS_RESPONSE_AFC_FREQ_MASK        0xffff
#define Si2168B_DVBT_STATUS_RESPONSE_AFC_FREQ_SHIFT       16
/* DVBT_STATUS command, TIMING_OFFSET field definition (address 6, size 16, lsb 0, signed)*/
#define Si2168B_DVBT_STATUS_RESPONSE_TIMING_OFFSET_LSB         0
#define Si2168B_DVBT_STATUS_RESPONSE_TIMING_OFFSET_MASK        0xffff
#define Si2168B_DVBT_STATUS_RESPONSE_TIMING_OFFSET_SHIFT       16
/* DVBT_STATUS command, CONSTELLATION field definition (address 8, size 6, lsb 0, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_CONSTELLATION_LSB         0
#define Si2168B_DVBT_STATUS_RESPONSE_CONSTELLATION_MASK        0x3f
#define Si2168B_DVBT_STATUS_RESPONSE_CONSTELLATION_QAM16  7
#define Si2168B_DVBT_STATUS_RESPONSE_CONSTELLATION_QAM64  9
#define Si2168B_DVBT_STATUS_RESPONSE_CONSTELLATION_QPSK   3
/* DVBT_STATUS command, SP_INV field definition (address 8, size 1, lsb 6, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_SP_INV_LSB         6
#define Si2168B_DVBT_STATUS_RESPONSE_SP_INV_MASK        0x01
#define Si2168B_DVBT_STATUS_RESPONSE_SP_INV_INVERTED  1
#define Si2168B_DVBT_STATUS_RESPONSE_SP_INV_NORMAL    0
/* DVBT_STATUS command, RATE_HP field definition (address 9, size 4, lsb 0, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_LSB         0
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_MASK        0x0f
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_1_2  1
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_2_3  2
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_3_4  3
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_5_6  5
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_HP_7_8  7
/* DVBT_STATUS command, RATE_LP field definition (address 9, size 4, lsb 4, unsigned)*/
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_LSB         4
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_MASK        0x0f
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_1_2  1
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_2_3  2
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_3_4  3
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_5_6  5
#define Si2168B_DVBT_STATUS_RESPONSE_RATE_LP_7_8  7

#endif /* Si2168B_DVBT_STATUS_CMD */

/* Si2168B_DVBT_TPS_EXTRA command definition */
#define Si2168B_DVBT_TPS_EXTRA_CMD 0xa1

#ifdef Si2168B_DVBT_TPS_EXTRA_CMD
#define Si2168B_DVBT_TPS_EXTRA_CMD_CODE 0x0100a1

typedef struct { /* Si2168B_DVBT_TPS_EXTRA_CMD_struct */
	u8  nothing;
} Si2168B_DVBT_TPS_EXTRA_CMD_struct;

typedef struct { /* Si2168B_DVBT_TPS_EXTRA_CMD_REPLY_struct */
	u8  lptimeslice;
	u8  hptimeslice;
	u8  lpmpefec;
	u8  hpmpefec;
	u8  dvbhinter;
	s16 cell_id;
	u8  tps_res1;
	u8  tps_res2;
	u8  tps_res3;
	u8  tps_res4;
}  Si2168B_DVBT_TPS_EXTRA_CMD_REPLY_struct;

/* DVBT_TPS_EXTRA command, LPTIMESLICE field definition (address 1, size 1, lsb 0, unsigned)*/
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPTIMESLICE_LSB         0
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPTIMESLICE_MASK        0x01
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPTIMESLICE_OFF  0
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPTIMESLICE_ON   1
/* DVBT_TPS_EXTRA command, HPTIMESLICE field definition (address 1, size 1, lsb 1, unsigned)*/
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPTIMESLICE_LSB         1
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPTIMESLICE_MASK        0x01
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPTIMESLICE_OFF  0
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPTIMESLICE_ON   1
/* DVBT_TPS_EXTRA command, LPMPEFEC field definition (address 1, size 1, lsb 2, unsigned)*/
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPMPEFEC_LSB         2
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPMPEFEC_MASK        0x01
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPMPEFEC_OFF  0
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_LPMPEFEC_ON   1
/* DVBT_TPS_EXTRA command, HPMPEFEC field definition (address 1, size 1, lsb 3, unsigned)*/
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPMPEFEC_LSB         3
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPMPEFEC_MASK        0x01
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPMPEFEC_OFF  0
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_HPMPEFEC_ON   1
/* DVBT_TPS_EXTRA command, DVBHINTER field definition (address 1, size 1, lsb 4, unsigned)*/
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_DVBHINTER_LSB         4
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_DVBHINTER_MASK        0x01
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_DVBHINTER_IN_DEPTH  1
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_DVBHINTER_NATIVE    0
/* DVBT_TPS_EXTRA command, CELL_ID field definition (address 2, size 16, lsb 0, signed)*/
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_CELL_ID_LSB         0
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_CELL_ID_MASK        0xffff
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_CELL_ID_SHIFT       16
/* DVBT_TPS_EXTRA command, TPS_RES1 field definition (address 4, size 4, lsb 0, unsigned)*/
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES1_LSB         0
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES1_MASK        0x0f
/* DVBT_TPS_EXTRA command, TPS_RES2 field definition (address 4, size 4, lsb 4, unsigned)*/
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES2_LSB         4
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES2_MASK        0x0f
/* DVBT_TPS_EXTRA command, TPS_RES3 field definition (address 5, size 4, lsb 0, unsigned)*/
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES3_LSB         0
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES3_MASK        0x0f
/* DVBT_TPS_EXTRA command, TPS_RES4 field definition (address 5, size 4, lsb 4, unsigned)*/
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES4_LSB         4
#define Si2168B_DVBT_TPS_EXTRA_RESPONSE_TPS_RES4_MASK        0x0f

#endif /* Si2168B_DVBT_TPS_EXTRA_CMD */

/* Si2168B_EXIT_BOOTLOADER command definition */
#define Si2168B_EXIT_BOOTLOADER_CMD 0x01

#ifdef Si2168B_EXIT_BOOTLOADER_CMD
#define Si2168B_EXIT_BOOTLOADER_CMD_CODE 0x010001

typedef struct { /* Si2168B_EXIT_BOOTLOADER_CMD_struct */
	u8  func;
	u8  ctsien;
} Si2168B_EXIT_BOOTLOADER_CMD_struct;

/* EXIT_BOOTLOADER command, FUNC field definition (address 1,size 4, lsb 0, unsigned) */
#define Si2168B_EXIT_BOOTLOADER_CMD_FUNC_LSB         0
#define Si2168B_EXIT_BOOTLOADER_CMD_FUNC_MASK        0x0f
#define Si2168B_EXIT_BOOTLOADER_CMD_FUNC_MIN         0
#define Si2168B_EXIT_BOOTLOADER_CMD_FUNC_MAX         1
#define Si2168B_EXIT_BOOTLOADER_CMD_FUNC_BOOTLOADER  0
#define Si2168B_EXIT_BOOTLOADER_CMD_FUNC_NORMAL      1
/* EXIT_BOOTLOADER command, CTSIEN field definition (address 1,size 1, lsb 7, unsigned) */
#define Si2168B_EXIT_BOOTLOADER_CMD_CTSIEN_LSB         7
#define Si2168B_EXIT_BOOTLOADER_CMD_CTSIEN_MASK        0x01
#define Si2168B_EXIT_BOOTLOADER_CMD_CTSIEN_MIN         0
#define Si2168B_EXIT_BOOTLOADER_CMD_CTSIEN_MAX         1
#define Si2168B_EXIT_BOOTLOADER_CMD_CTSIEN_OFF  0
#define Si2168B_EXIT_BOOTLOADER_CMD_CTSIEN_ON   1
#endif /* Si2168B_EXIT_BOOTLOADER_CMD */

/* Si2168B_GET_PROPERTY command definition */
#define Si2168B_GET_PROPERTY_CMD 0x15

#ifdef Si2168B_GET_PROPERTY_CMD
#define Si2168B_GET_PROPERTY_CMD_CODE 0x010015

typedef struct { /* Si2168B_GET_PROPERTY_CMD_struct */
	u8  reserved;
	u16 prop;
} Si2168B_GET_PROPERTY_CMD_struct;

typedef struct { /* Si2168B_GET_PROPERTY_CMD_REPLY_struct */
	u8  reserved;
	u16 data;
}  Si2168B_GET_PROPERTY_CMD_REPLY_struct;

/* GET_PROPERTY command, RESERVED field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_GET_PROPERTY_CMD_RESERVED_LSB         0
#define Si2168B_GET_PROPERTY_CMD_RESERVED_MASK        0xff
#define Si2168B_GET_PROPERTY_CMD_RESERVED_MIN         0
#define Si2168B_GET_PROPERTY_CMD_RESERVED_MAX         0
#define Si2168B_GET_PROPERTY_CMD_RESERVED_RESERVED_MIN  0
#define Si2168B_GET_PROPERTY_CMD_RESERVED_RESERVED_MAX  0
/* GET_PROPERTY command, PROP field definition (address 2,size 16, lsb 0, unsigned) */
#define Si2168B_GET_PROPERTY_CMD_PROP_LSB         0
#define Si2168B_GET_PROPERTY_CMD_PROP_MASK        0xffff
#define Si2168B_GET_PROPERTY_CMD_PROP_MIN         0
#define Si2168B_GET_PROPERTY_CMD_PROP_MAX         65535
#define Si2168B_GET_PROPERTY_CMD_PROP_PROP_MIN  0
#define Si2168B_GET_PROPERTY_CMD_PROP_PROP_MAX  65535
/* GET_PROPERTY command, RESERVED field definition (address 1, size 8, lsb 0, unsigned)*/
#define Si2168B_GET_PROPERTY_RESPONSE_RESERVED_LSB         0
#define Si2168B_GET_PROPERTY_RESPONSE_RESERVED_MASK        0xff
/* GET_PROPERTY command, DATA field definition (address 2, size 16, lsb 0, unsigned)*/
#define Si2168B_GET_PROPERTY_RESPONSE_DATA_LSB         0
#define Si2168B_GET_PROPERTY_RESPONSE_DATA_MASK        0xffff

#endif /* Si2168B_GET_PROPERTY_CMD */

/* Si2168B_GET_REV command definition */
#define Si2168B_GET_REV_CMD 0x11

#ifdef Si2168B_GET_REV_CMD
#define Si2168B_GET_REV_CMD_CODE 0x010011

typedef struct { /* Si2168B_GET_REV_CMD_struct */
	u8  nothing;
} Si2168B_GET_REV_CMD_struct;

typedef struct { /* Si2168B_GET_REV_CMD_REPLY_struct */
	u8  pn;
	u8  fwmajor;
	u8  fwminor;
	u16 patch;
	u8  cmpmajor;
	u8  cmpminor;
	u8  cmpbuild;
	u8  chiprev;
	u8  mcm_die;
}  Si2168B_GET_REV_CMD_REPLY_struct;

/* GET_REV command, PN field definition (address 1, size 8, lsb 0, unsigned)*/
#define Si2168B_GET_REV_RESPONSE_PN_LSB         0
#define Si2168B_GET_REV_RESPONSE_PN_MASK        0xff
/* GET_REV command, FWMAJOR field definition (address 2, size 8, lsb 0, unsigned)*/
#define Si2168B_GET_REV_RESPONSE_FWMAJOR_LSB         0
#define Si2168B_GET_REV_RESPONSE_FWMAJOR_MASK        0xff
/* GET_REV command, FWMINOR field definition (address 3, size 8, lsb 0, unsigned)*/
#define Si2168B_GET_REV_RESPONSE_FWMINOR_LSB         0
#define Si2168B_GET_REV_RESPONSE_FWMINOR_MASK        0xff
/* GET_REV command, PATCH field definition (address 4, size 16, lsb 0, unsigned)*/
#define Si2168B_GET_REV_RESPONSE_PATCH_LSB         0
#define Si2168B_GET_REV_RESPONSE_PATCH_MASK        0xffff
/* GET_REV command, CMPMAJOR field definition (address 6, size 8, lsb 0, unsigned)*/
#define Si2168B_GET_REV_RESPONSE_CMPMAJOR_LSB         0
#define Si2168B_GET_REV_RESPONSE_CMPMAJOR_MASK        0xff
/* GET_REV command, CMPMINOR field definition (address 7, size 8, lsb 0, unsigned)*/
#define Si2168B_GET_REV_RESPONSE_CMPMINOR_LSB         0
#define Si2168B_GET_REV_RESPONSE_CMPMINOR_MASK        0xff
/* GET_REV command, CMPBUILD field definition (address 8, size 8, lsb 0, unsigned)*/
#define Si2168B_GET_REV_RESPONSE_CMPBUILD_LSB         0
#define Si2168B_GET_REV_RESPONSE_CMPBUILD_MASK        0xff
#define Si2168B_GET_REV_RESPONSE_CMPBUILD_CMPBUILD_MIN  0
#define Si2168B_GET_REV_RESPONSE_CMPBUILD_CMPBUILD_MAX  255
/* GET_REV command, CHIPREV field definition (address 9, size 4, lsb 0, unsigned)*/
#define Si2168B_GET_REV_RESPONSE_CHIPREV_LSB         0
#define Si2168B_GET_REV_RESPONSE_CHIPREV_MASK        0x0f
#define Si2168B_GET_REV_RESPONSE_CHIPREV_A  1
#define Si2168B_GET_REV_RESPONSE_CHIPREV_B  2
#define Si2168B_GET_REV_RESPONSE_CHIPREV_C  2
/* GET_REV command, MCM_DIE field definition (address 9, size 4, lsb 4, unsigned)*/
#define Si2168B_GET_REV_RESPONSE_MCM_DIE_LSB         4
#define Si2168B_GET_REV_RESPONSE_MCM_DIE_MASK        0x0f
#define Si2168B_GET_REV_RESPONSE_MCM_DIE_DIE_A   1
#define Si2168B_GET_REV_RESPONSE_MCM_DIE_DIE_B   2
#define Si2168B_GET_REV_RESPONSE_MCM_DIE_SINGLE  0

#endif /* Si2168B_GET_REV_CMD */

/* Si2168B_I2C_PASSTHROUGH command definition */
#define Si2168B_I2C_PASSTHROUGH_CMD 0xc0

#ifdef Si2168B_I2C_PASSTHROUGH_CMD
#define Si2168B_I2C_PASSTHROUGH_CMD_CODE 0x0100c0

typedef struct { /* Si2168B_I2C_PASSTHROUGH_CMD_struct */
	u8  subcode;
	u8  i2c_passthru;
	u8  reserved;
} Si2168B_I2C_PASSTHROUGH_CMD_struct;

/* I2C_PASSTHROUGH command, SUBCODE field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_I2C_PASSTHROUGH_CMD_SUBCODE_LSB         0
#define Si2168B_I2C_PASSTHROUGH_CMD_SUBCODE_MASK        0xff
#define Si2168B_I2C_PASSTHROUGH_CMD_SUBCODE_MIN         13
#define Si2168B_I2C_PASSTHROUGH_CMD_SUBCODE_MAX         13
#define Si2168B_I2C_PASSTHROUGH_CMD_SUBCODE_CODE  13
/* I2C_PASSTHROUGH command, I2C_PASSTHRU field definition (address 2,size 1, lsb 0, unsigned) */
#define Si2168B_I2C_PASSTHROUGH_CMD_I2C_PASSTHRU_LSB         0
#define Si2168B_I2C_PASSTHROUGH_CMD_I2C_PASSTHRU_MASK        0x01
#define Si2168B_I2C_PASSTHROUGH_CMD_I2C_PASSTHRU_MIN         0
#define Si2168B_I2C_PASSTHROUGH_CMD_I2C_PASSTHRU_MAX         1
#define Si2168B_I2C_PASSTHROUGH_CMD_I2C_PASSTHRU_CLOSE  1
#define Si2168B_I2C_PASSTHROUGH_CMD_I2C_PASSTHRU_OPEN   0
/* I2C_PASSTHROUGH command, RESERVED field definition (address 2,size 7, lsb 1, unsigned) */
#define Si2168B_I2C_PASSTHROUGH_CMD_RESERVED_LSB         1
#define Si2168B_I2C_PASSTHROUGH_CMD_RESERVED_MASK        0x7f
#define Si2168B_I2C_PASSTHROUGH_CMD_RESERVED_MIN         0
#define Si2168B_I2C_PASSTHROUGH_CMD_RESERVED_MAX         0
#define Si2168B_I2C_PASSTHROUGH_CMD_RESERVED_RESERVED  0
#endif /* Si2168B_I2C_PASSTHROUGH_CMD */

/* Si2168B_MCNS_STATUS command definition */
#define Si2168B_MCNS_STATUS_CMD 0x98

#ifdef Si2168B_MCNS_STATUS_CMD
#define Si2168B_MCNS_STATUS_CMD_CODE 0x010098

typedef struct { /* Si2168B_MCNS_STATUS_CMD_struct */
	u8  intack;
} Si2168B_MCNS_STATUS_CMD_struct;

typedef struct { /* Si2168B_MCNS_STATUS_CMD_REPLY_struct */
	u8  pclint;
	u8  dlint;
	u8  berint;
	u8  uncorint;
	u8  pcl;
	u8  dl;
	u8  ber;
	u8  uncor;
	u8  cnr;
	s16 afc_freq;
	s16 timing_offset;
	u8  constellation;
	u8  sp_inv;
	u8  interleaving;
}  Si2168B_MCNS_STATUS_CMD_REPLY_struct;

/* MCNS_STATUS command, INTACK field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_MCNS_STATUS_CMD_INTACK_LSB         0
#define Si2168B_MCNS_STATUS_CMD_INTACK_MASK        0x01
#define Si2168B_MCNS_STATUS_CMD_INTACK_MIN         0
#define Si2168B_MCNS_STATUS_CMD_INTACK_MAX         1
#define Si2168B_MCNS_STATUS_CMD_INTACK_CLEAR  1
#define Si2168B_MCNS_STATUS_CMD_INTACK_OK     0
/* MCNS_STATUS command, PCLINT field definition (address 1, size 1, lsb 1, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_PCLINT_LSB         1
#define Si2168B_MCNS_STATUS_RESPONSE_PCLINT_MASK        0x01
#define Si2168B_MCNS_STATUS_RESPONSE_PCLINT_CHANGED    1
#define Si2168B_MCNS_STATUS_RESPONSE_PCLINT_NO_CHANGE  0
/* MCNS_STATUS command, DLINT field definition (address 1, size 1, lsb 2, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_DLINT_LSB         2
#define Si2168B_MCNS_STATUS_RESPONSE_DLINT_MASK        0x01
#define Si2168B_MCNS_STATUS_RESPONSE_DLINT_CHANGED    1
#define Si2168B_MCNS_STATUS_RESPONSE_DLINT_NO_CHANGE  0
/* MCNS_STATUS command, BERINT field definition (address 1, size 1, lsb 3, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_BERINT_LSB         3
#define Si2168B_MCNS_STATUS_RESPONSE_BERINT_MASK        0x01
#define Si2168B_MCNS_STATUS_RESPONSE_BERINT_CHANGED    1
#define Si2168B_MCNS_STATUS_RESPONSE_BERINT_NO_CHANGE  0
/* MCNS_STATUS command, UNCORINT field definition (address 1, size 1, lsb 4, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_UNCORINT_LSB         4
#define Si2168B_MCNS_STATUS_RESPONSE_UNCORINT_MASK        0x01
#define Si2168B_MCNS_STATUS_RESPONSE_UNCORINT_CHANGED    1
#define Si2168B_MCNS_STATUS_RESPONSE_UNCORINT_NO_CHANGE  0
/* MCNS_STATUS command, PCL field definition (address 2, size 1, lsb 1, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_PCL_LSB         1
#define Si2168B_MCNS_STATUS_RESPONSE_PCL_MASK        0x01
#define Si2168B_MCNS_STATUS_RESPONSE_PCL_LOCKED   1
#define Si2168B_MCNS_STATUS_RESPONSE_PCL_NO_LOCK  0
/* MCNS_STATUS command, DL field definition (address 2, size 1, lsb 2, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_DL_LSB         2
#define Si2168B_MCNS_STATUS_RESPONSE_DL_MASK        0x01
#define Si2168B_MCNS_STATUS_RESPONSE_DL_LOCKED   1
#define Si2168B_MCNS_STATUS_RESPONSE_DL_NO_LOCK  0
/* MCNS_STATUS command, BER field definition (address 2, size 1, lsb 3, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_BER_LSB         3
#define Si2168B_MCNS_STATUS_RESPONSE_BER_MASK        0x01
#define Si2168B_MCNS_STATUS_RESPONSE_BER_BER_ABOVE  1
#define Si2168B_MCNS_STATUS_RESPONSE_BER_BER_BELOW  0
/* MCNS_STATUS command, UNCOR field definition (address 2, size 1, lsb 4, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_UNCOR_LSB         4
#define Si2168B_MCNS_STATUS_RESPONSE_UNCOR_MASK        0x01
#define Si2168B_MCNS_STATUS_RESPONSE_UNCOR_NO_UNCOR_FOUND  0
#define Si2168B_MCNS_STATUS_RESPONSE_UNCOR_UNCOR_FOUND     1
/* MCNS_STATUS command, CNR field definition (address 3, size 8, lsb 0, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_CNR_LSB         0
#define Si2168B_MCNS_STATUS_RESPONSE_CNR_MASK        0xff
/* MCNS_STATUS command, AFC_FREQ field definition (address 4, size 16, lsb 0, signed)*/
#define Si2168B_MCNS_STATUS_RESPONSE_AFC_FREQ_LSB         0
#define Si2168B_MCNS_STATUS_RESPONSE_AFC_FREQ_MASK        0xffff
#define Si2168B_MCNS_STATUS_RESPONSE_AFC_FREQ_SHIFT       16
/* MCNS_STATUS command, TIMING_OFFSET field definition (address 6, size 16, lsb 0, signed)*/
#define Si2168B_MCNS_STATUS_RESPONSE_TIMING_OFFSET_LSB         0
#define Si2168B_MCNS_STATUS_RESPONSE_TIMING_OFFSET_MASK        0xffff
#define Si2168B_MCNS_STATUS_RESPONSE_TIMING_OFFSET_SHIFT       16
/* MCNS_STATUS command, CONSTELLATION field definition (address 8, size 6, lsb 0, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_CONSTELLATION_LSB         0
#define Si2168B_MCNS_STATUS_RESPONSE_CONSTELLATION_MASK        0x3f
#define Si2168B_MCNS_STATUS_RESPONSE_CONSTELLATION_QAM256  11
#define Si2168B_MCNS_STATUS_RESPONSE_CONSTELLATION_QAM64   9
/* MCNS_STATUS command, SP_INV field definition (address 8, size 1, lsb 6, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_SP_INV_LSB         6
#define Si2168B_MCNS_STATUS_RESPONSE_SP_INV_MASK        0x01
#define Si2168B_MCNS_STATUS_RESPONSE_SP_INV_INVERTED  1
#define Si2168B_MCNS_STATUS_RESPONSE_SP_INV_NORMAL    0
/* MCNS_STATUS command, INTERLEAVING field definition (address 9, size 4, lsb 0, unsigned)*/
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_LSB         0
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_MASK        0x0f
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_0__128_1      0
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_10__128_6     10
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_11__RESERVED  11
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_12__128_7     12
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_13__RESERVED  13
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_14__128_8     14
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_15__RESERVED  15
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_1__128_1      1
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_2__128_2      2
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_3__64_2       3
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_4__128_3      4
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_5__32_4       5
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_6__128_4      6
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_7__16_8       7
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_8__128_5      8
#define Si2168B_MCNS_STATUS_RESPONSE_INTERLEAVING_9__8_16       9

#endif /* Si2168B_MCNS_STATUS_CMD */

/* Si2168B_PART_INFO command definition */
#define Si2168B_PART_INFO_CMD 0x02

#ifdef Si2168B_PART_INFO_CMD
#define Si2168B_PART_INFO_CMD_CODE 0x010002

typedef struct { /* Si2168B_PART_INFO_CMD_struct */
	u8  nothing;
} Si2168B_PART_INFO_CMD_struct;

typedef struct { /* Si2168B_PART_INFO_CMD_REPLY_struct */
	u8  chiprev;
	u8  romid;
	u8  part;
	u8  pmajor;
	u8  pminor;
	u8  pbuild;
	u16 reserved;
	u32 serial;
}  Si2168B_PART_INFO_CMD_REPLY_struct;

/* PART_INFO command, CHIPREV field definition (address 1, size 4, lsb 0, unsigned)*/
#define Si2168B_PART_INFO_RESPONSE_CHIPREV_LSB         0
#define Si2168B_PART_INFO_RESPONSE_CHIPREV_MASK        0x0f
#define Si2168B_PART_INFO_RESPONSE_CHIPREV_A  1
#define Si2168B_PART_INFO_RESPONSE_CHIPREV_B  2
/* PART_INFO command, ROMID field definition (address 12, size 8, lsb 0, unsigned)*/
#define Si2168B_PART_INFO_RESPONSE_ROMID_LSB         0
#define Si2168B_PART_INFO_RESPONSE_ROMID_MASK        0xff
/* PART_INFO command, PART field definition (address 2, size 8, lsb 0, unsigned)*/
#define Si2168B_PART_INFO_RESPONSE_PART_LSB         0
#define Si2168B_PART_INFO_RESPONSE_PART_MASK        0xff
/* PART_INFO command, PMAJOR field definition (address 3, size 8, lsb 0, unsigned)*/
#define Si2168B_PART_INFO_RESPONSE_PMAJOR_LSB         0
#define Si2168B_PART_INFO_RESPONSE_PMAJOR_MASK        0xff
/* PART_INFO command, PMINOR field definition (address 4, size 8, lsb 0, unsigned)*/
#define Si2168B_PART_INFO_RESPONSE_PMINOR_LSB         0
#define Si2168B_PART_INFO_RESPONSE_PMINOR_MASK        0xff
/* PART_INFO command, PBUILD field definition (address 5, size 8, lsb 0, unsigned)*/
#define Si2168B_PART_INFO_RESPONSE_PBUILD_LSB         0
#define Si2168B_PART_INFO_RESPONSE_PBUILD_MASK        0xff
/* PART_INFO command, RESERVED field definition (address 6, size 16, lsb 0, unsigned)*/
#define Si2168B_PART_INFO_RESPONSE_RESERVED_LSB         0
#define Si2168B_PART_INFO_RESPONSE_RESERVED_MASK        0xffff
/* PART_INFO command, SERIAL field definition (address 8, size 32, lsb 0, unsigned)*/
#define Si2168B_PART_INFO_RESPONSE_SERIAL_LSB         0
#define Si2168B_PART_INFO_RESPONSE_SERIAL_MASK        0xffffffff

#endif /* Si2168B_PART_INFO_CMD */

/* Si2168B_POWER_DOWN command definition */
#define Si2168B_POWER_DOWN_CMD 0x13

#ifdef Si2168B_POWER_DOWN_CMD
#define Si2168B_POWER_DOWN_CMD_CODE 0x010013

typedef struct { /* Si2168B_POWER_DOWN_CMD_struct */
	u8  nothing;
} Si2168B_POWER_DOWN_CMD_struct;

#endif /* Si2168B_POWER_DOWN_CMD */

/* Si2168B_POWER_UP command definition */
#define Si2168B_POWER_UP_CMD 0xc0

#ifdef Si2168B_POWER_UP_CMD
#define Si2168B_POWER_UP_CMD_CODE 0x0200c0

typedef struct { /* Si2168B_POWER_UP_CMD_struct */
	u8  subcode;
	u8  reset;
	u8  reserved2;
	u8  reserved4;
	u8  reserved1;
	u8  addr_mode;
	u8  reserved5;
	u8  func;
	u8  clock_freq;
	u8  ctsien;
	u8  wake_up;
} Si2168B_POWER_UP_CMD_struct;

/* POWER_UP command, SUBCODE field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_POWER_UP_CMD_SUBCODE_LSB         0
#define Si2168B_POWER_UP_CMD_SUBCODE_MASK        0xff
#define Si2168B_POWER_UP_CMD_SUBCODE_MIN         6
#define Si2168B_POWER_UP_CMD_SUBCODE_MAX         6
#define Si2168B_POWER_UP_CMD_SUBCODE_CODE  6
/* POWER_UP command, RESET field definition (address 2,size 8, lsb 0, unsigned) */
#define Si2168B_POWER_UP_CMD_RESET_LSB         0
#define Si2168B_POWER_UP_CMD_RESET_MASK        0xff
#define Si2168B_POWER_UP_CMD_RESET_MIN         1
#define Si2168B_POWER_UP_CMD_RESET_MAX         8
#define Si2168B_POWER_UP_CMD_RESET_RESET   1
#define Si2168B_POWER_UP_CMD_RESET_RESUME  8
/* POWER_UP command, RESERVED2 field definition (address 3,size 8, lsb 0, unsigned) */
#define Si2168B_POWER_UP_CMD_RESERVED2_LSB         0
#define Si2168B_POWER_UP_CMD_RESERVED2_MASK        0xff
#define Si2168B_POWER_UP_CMD_RESERVED2_MIN         15
#define Si2168B_POWER_UP_CMD_RESERVED2_MAX         15
#define Si2168B_POWER_UP_CMD_RESERVED2_RESERVED  15
/* POWER_UP command, RESERVED4 field definition (address 4,size 8, lsb 0, unsigned) */
#define Si2168B_POWER_UP_CMD_RESERVED4_LSB         0
#define Si2168B_POWER_UP_CMD_RESERVED4_MASK        0xff
#define Si2168B_POWER_UP_CMD_RESERVED4_MIN         0
#define Si2168B_POWER_UP_CMD_RESERVED4_MAX         0
#define Si2168B_POWER_UP_CMD_RESERVED4_RESERVED  0
/* POWER_UP command, RESERVED1 field definition (address 5,size 4, lsb 0, unsigned) */
#define Si2168B_POWER_UP_CMD_RESERVED1_LSB         0
#define Si2168B_POWER_UP_CMD_RESERVED1_MASK        0x0f
#define Si2168B_POWER_UP_CMD_RESERVED1_MIN         0
#define Si2168B_POWER_UP_CMD_RESERVED1_MAX         0
#define Si2168B_POWER_UP_CMD_RESERVED1_RESERVED  0
/* POWER_UP command, ADDR_MODE field definition (address 5,size 1, lsb 4, unsigned) */
#define Si2168B_POWER_UP_CMD_ADDR_MODE_LSB         4
#define Si2168B_POWER_UP_CMD_ADDR_MODE_MASK        0x01
#define Si2168B_POWER_UP_CMD_ADDR_MODE_MIN         0
#define Si2168B_POWER_UP_CMD_ADDR_MODE_MAX         1
#define Si2168B_POWER_UP_CMD_ADDR_MODE_CAPTURE  1
#define Si2168B_POWER_UP_CMD_ADDR_MODE_CURRENT  0
/* POWER_UP command, RESERVED5 field definition (address 5,size 1, lsb 5, unsigned) */
#define Si2168B_POWER_UP_CMD_RESERVED5_LSB         5
#define Si2168B_POWER_UP_CMD_RESERVED5_MASK        0x01
#define Si2168B_POWER_UP_CMD_RESERVED5_MIN         1
#define Si2168B_POWER_UP_CMD_RESERVED5_MAX         1
#define Si2168B_POWER_UP_CMD_RESERVED5_RESERVED  1
/* POWER_UP command, FUNC field definition (address 6,size 4, lsb 0, unsigned) */
#define Si2168B_POWER_UP_CMD_FUNC_LSB         0
#define Si2168B_POWER_UP_CMD_FUNC_MASK        0x0f
#define Si2168B_POWER_UP_CMD_FUNC_MIN         0
#define Si2168B_POWER_UP_CMD_FUNC_MAX         1
#define Si2168B_POWER_UP_CMD_FUNC_BOOTLOADER  0
#define Si2168B_POWER_UP_CMD_FUNC_NORMAL      1
/* POWER_UP command, CLOCK_FREQ field definition (address 6,size 3, lsb 4, unsigned) */
#define Si2168B_POWER_UP_CMD_CLOCK_FREQ_LSB         4
#define Si2168B_POWER_UP_CMD_CLOCK_FREQ_MASK        0x07
#define Si2168B_POWER_UP_CMD_CLOCK_FREQ_MIN         0
#define Si2168B_POWER_UP_CMD_CLOCK_FREQ_MAX         4
#define Si2168B_POWER_UP_CMD_CLOCK_FREQ_CLK_16MHZ  0
#define Si2168B_POWER_UP_CMD_CLOCK_FREQ_CLK_24MHZ  2
#define Si2168B_POWER_UP_CMD_CLOCK_FREQ_CLK_27MHZ  3
/* POWER_UP command, CTSIEN field definition (address 6,size 1, lsb 7, unsigned) */
#define Si2168B_POWER_UP_CMD_CTSIEN_LSB         7
#define Si2168B_POWER_UP_CMD_CTSIEN_MASK        0x01
#define Si2168B_POWER_UP_CMD_CTSIEN_MIN         0
#define Si2168B_POWER_UP_CMD_CTSIEN_MAX         1
#define Si2168B_POWER_UP_CMD_CTSIEN_DISABLE  0
#define Si2168B_POWER_UP_CMD_CTSIEN_ENABLE   1
/* POWER_UP command, WAKE_UP field definition (address 7,size 1, lsb 0, unsigned) */
#define Si2168B_POWER_UP_CMD_WAKE_UP_LSB         0
#define Si2168B_POWER_UP_CMD_WAKE_UP_MASK        0x01
#define Si2168B_POWER_UP_CMD_WAKE_UP_MIN         1
#define Si2168B_POWER_UP_CMD_WAKE_UP_MAX         1
#define Si2168B_POWER_UP_CMD_WAKE_UP_WAKE_UP  1
#endif /* Si2168B_POWER_UP_CMD */

/* Si2168B_RSSI_ADC command definition */
#define Si2168B_RSSI_ADC_CMD 0x17

#ifdef Si2168B_RSSI_ADC_CMD
#define Si2168B_RSSI_ADC_CMD_CODE 0x010017

typedef struct { /* Si2168B_RSSI_ADC_CMD_struct */
	u8  on_off;
} Si2168B_RSSI_ADC_CMD_struct;

typedef struct { /* Si2168B_RSSI_ADC_CMD_REPLY_struct */
	u8  level;
}  Si2168B_RSSI_ADC_CMD_REPLY_struct;

/* RSSI_ADC command, ON_OFF field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_RSSI_ADC_CMD_ON_OFF_LSB         0
#define Si2168B_RSSI_ADC_CMD_ON_OFF_MASK        0x01
#define Si2168B_RSSI_ADC_CMD_ON_OFF_MIN         0
#define Si2168B_RSSI_ADC_CMD_ON_OFF_MAX         1
#define Si2168B_RSSI_ADC_CMD_ON_OFF_OFF  0
#define Si2168B_RSSI_ADC_CMD_ON_OFF_ON   1
/* RSSI_ADC command, LEVEL field definition (address 1, size 8, lsb 0, unsigned)*/
#define Si2168B_RSSI_ADC_RESPONSE_LEVEL_LSB         0
#define Si2168B_RSSI_ADC_RESPONSE_LEVEL_MASK        0xff

#endif /* Si2168B_RSSI_ADC_CMD */

/* Si2168B_SCAN_CTRL command definition */
#define Si2168B_SCAN_CTRL_CMD 0x31

#ifdef Si2168B_SCAN_CTRL_CMD
#define Si2168B_SCAN_CTRL_CMD_CODE 0x010031

typedef struct { /* Si2168B_SCAN_CTRL_CMD_struct */
	u8  action;
	u32 tuned_rf_freq;
} Si2168B_SCAN_CTRL_CMD_struct;

/* SCAN_CTRL command, ACTION field definition (address 1,size 4, lsb 0, unsigned) */
#define Si2168B_SCAN_CTRL_CMD_ACTION_LSB         0
#define Si2168B_SCAN_CTRL_CMD_ACTION_MASK        0x0f
#define Si2168B_SCAN_CTRL_CMD_ACTION_MIN         1
#define Si2168B_SCAN_CTRL_CMD_ACTION_MAX         3
#define Si2168B_SCAN_CTRL_CMD_ACTION_ABORT   3
#define Si2168B_SCAN_CTRL_CMD_ACTION_RESUME  2
#define Si2168B_SCAN_CTRL_CMD_ACTION_START   1
/* SCAN_CTRL command, TUNED_RF_FREQ field definition (address 4,size 32, lsb 0, unsigned) */
#define Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_LSB         0
#define Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_MASK        0xffffffff
#define Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_MIN         0
#define Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_MAX         4294967
#define Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_TUNED_RF_FREQ_MIN  0
#define Si2168B_SCAN_CTRL_CMD_TUNED_RF_FREQ_TUNED_RF_FREQ_MAX  4294967
#endif /* Si2168B_SCAN_CTRL_CMD */

/* Si2168B_SCAN_STATUS command definition */
#define Si2168B_SCAN_STATUS_CMD 0x30

#ifdef Si2168B_SCAN_STATUS_CMD
#define Si2168B_SCAN_STATUS_CMD_CODE 0x010030

typedef struct { /* Si2168B_SCAN_STATUS_CMD_struct */
	u8  intack;
} Si2168B_SCAN_STATUS_CMD_struct;

typedef struct { /* Si2168B_SCAN_STATUS_CMD_REPLY_struct */
	u8  buzint;
	u8  reqint;
	u8  modulation;
	u8  buz;
	u8  req;
	u8  scan_status;
	u32 rf_freq;
	u16 symb_rate;
}  Si2168B_SCAN_STATUS_CMD_REPLY_struct;

/* SCAN_STATUS command, INTACK field definition (address 1,size 1, lsb 0, unsigned) */
#define Si2168B_SCAN_STATUS_CMD_INTACK_LSB         0
#define Si2168B_SCAN_STATUS_CMD_INTACK_MASK        0x01
#define Si2168B_SCAN_STATUS_CMD_INTACK_MIN         0
#define Si2168B_SCAN_STATUS_CMD_INTACK_MAX         1
#define Si2168B_SCAN_STATUS_CMD_INTACK_CLEAR  1
#define Si2168B_SCAN_STATUS_CMD_INTACK_OK     0
/* SCAN_STATUS command, BUZINT field definition (address 1, size 1, lsb 0, unsigned)*/
#define Si2168B_SCAN_STATUS_RESPONSE_BUZINT_LSB         0
#define Si2168B_SCAN_STATUS_RESPONSE_BUZINT_MASK        0x01
#define Si2168B_SCAN_STATUS_RESPONSE_BUZINT_CHANGED    1
#define Si2168B_SCAN_STATUS_RESPONSE_BUZINT_NO_CHANGE  0
/* SCAN_STATUS command, REQINT field definition (address 1, size 1, lsb 1, unsigned)*/
#define Si2168B_SCAN_STATUS_RESPONSE_REQINT_LSB         1
#define Si2168B_SCAN_STATUS_RESPONSE_REQINT_MASK        0x01
#define Si2168B_SCAN_STATUS_RESPONSE_REQINT_CHANGED    1
#define Si2168B_SCAN_STATUS_RESPONSE_REQINT_NO_CHANGE  0
/* SCAN_STATUS command, MODULATION field definition (address 10, size 4, lsb 0, unsigned)*/
#define Si2168B_SCAN_STATUS_RESPONSE_MODULATION_LSB         0
#define Si2168B_SCAN_STATUS_RESPONSE_MODULATION_MASK        0x0f
#define Si2168B_SCAN_STATUS_RESPONSE_MODULATION_DSS    10
#define Si2168B_SCAN_STATUS_RESPONSE_MODULATION_DVBC   3
#define Si2168B_SCAN_STATUS_RESPONSE_MODULATION_DVBC2  11
#define Si2168B_SCAN_STATUS_RESPONSE_MODULATION_DVBS   8
#define Si2168B_SCAN_STATUS_RESPONSE_MODULATION_DVBS2  9
#define Si2168B_SCAN_STATUS_RESPONSE_MODULATION_DVBT   2
#define Si2168B_SCAN_STATUS_RESPONSE_MODULATION_DVBT2  7
#define Si2168B_SCAN_STATUS_RESPONSE_MODULATION_MCNS   1
/* SCAN_STATUS command, BUZ field definition (address 2, size 1, lsb 0, unsigned)*/
#define Si2168B_SCAN_STATUS_RESPONSE_BUZ_LSB         0
#define Si2168B_SCAN_STATUS_RESPONSE_BUZ_MASK        0x01
#define Si2168B_SCAN_STATUS_RESPONSE_BUZ_BUSY  1
#define Si2168B_SCAN_STATUS_RESPONSE_BUZ_CTS   0
/* SCAN_STATUS command, REQ field definition (address 2, size 1, lsb 1, unsigned)*/
#define Si2168B_SCAN_STATUS_RESPONSE_REQ_LSB         1
#define Si2168B_SCAN_STATUS_RESPONSE_REQ_MASK        0x01
#define Si2168B_SCAN_STATUS_RESPONSE_REQ_NO_REQUEST  0
#define Si2168B_SCAN_STATUS_RESPONSE_REQ_REQUEST     1
/* SCAN_STATUS command, SCAN_STATUS field definition (address 3, size 6, lsb 0, unsigned)*/
#define Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_LSB         0
#define Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_MASK        0x3f
#define Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_ANALOG_CHANNEL_FOUND   6
#define Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_DEBUG                  63
#define Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_DIGITAL_CHANNEL_FOUND  5
#define Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_ENDED                  2
#define Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_ERROR                  3
#define Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_IDLE                   0
#define Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_SEARCHING              1
#define Si2168B_SCAN_STATUS_RESPONSE_SCAN_STATUS_TUNE_REQUEST           4
/* SCAN_STATUS command, RF_FREQ field definition (address 4, size 32, lsb 0, unsigned)*/
#define Si2168B_SCAN_STATUS_RESPONSE_RF_FREQ_LSB         0
#define Si2168B_SCAN_STATUS_RESPONSE_RF_FREQ_MASK        0xffffffff
/* SCAN_STATUS command, SYMB_RATE field definition (address 8, size 16, lsb 0, unsigned)*/
#define Si2168B_SCAN_STATUS_RESPONSE_SYMB_RATE_LSB         0
#define Si2168B_SCAN_STATUS_RESPONSE_SYMB_RATE_MASK        0xffff

#endif /* Si2168B_SCAN_STATUS_CMD */

/* Si2168B_SET_PROPERTY command definition */
#define Si2168B_SET_PROPERTY_CMD 0x14

#ifdef Si2168B_SET_PROPERTY_CMD
#define Si2168B_SET_PROPERTY_CMD_CODE 0x010014

typedef struct { /* Si2168B_SET_PROPERTY_CMD_struct */
	u8  reserved;
	u16 prop;
	u16 data;
} Si2168B_SET_PROPERTY_CMD_struct;

typedef struct { /* Si2168B_SET_PROPERTY_CMD_REPLY_struct */
	u8  reserved;
	u16 data;
} Si2168B_SET_PROPERTY_CMD_REPLY_struct;

/* SET_PROPERTY command, RESERVED field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_SET_PROPERTY_CMD_RESERVED_LSB         0
#define Si2168B_SET_PROPERTY_CMD_RESERVED_MASK        0xff
#define Si2168B_SET_PROPERTY_CMD_RESERVED_MIN         0
#define Si2168B_SET_PROPERTY_CMD_RESERVED_MAX         255
/* SET_PROPERTY command, PROP field definition (address 2,size 16, lsb 0, unsigned) */
#define Si2168B_SET_PROPERTY_CMD_PROP_LSB         0
#define Si2168B_SET_PROPERTY_CMD_PROP_MASK        0xffff
#define Si2168B_SET_PROPERTY_CMD_PROP_MIN         0
#define Si2168B_SET_PROPERTY_CMD_PROP_MAX         65535
#define Si2168B_SET_PROPERTY_CMD_PROP_PROP_MIN  0
#define Si2168B_SET_PROPERTY_CMD_PROP_PROP_MAX  65535
/* SET_PROPERTY command, DATA field definition (address 4,size 16, lsb 0, unsigned) */
#define Si2168B_SET_PROPERTY_CMD_DATA_LSB         0
#define Si2168B_SET_PROPERTY_CMD_DATA_MASK        0xffff
#define Si2168B_SET_PROPERTY_CMD_DATA_MIN         0
#define Si2168B_SET_PROPERTY_CMD_DATA_MAX         65535
#define Si2168B_SET_PROPERTY_CMD_DATA_DATA_MIN  0
#define Si2168B_SET_PROPERTY_CMD_DATA_DATA_MAX  65535
/* SET_PROPERTY command, RESERVED field definition (address 1, size 8, lsb 0, unsigned)*/
#define Si2168B_SET_PROPERTY_RESPONSE_RESERVED_LSB         0
#define Si2168B_SET_PROPERTY_RESPONSE_RESERVED_MASK        0xff
#define Si2168B_SET_PROPERTY_RESPONSE_RESERVED_RESERVED_MIN  0
#define Si2168B_SET_PROPERTY_RESPONSE_RESERVED_RESERVED_MAX  0
/* SET_PROPERTY command, DATA field definition (address 2, size 16, lsb 0, unsigned)*/
#define Si2168B_SET_PROPERTY_RESPONSE_DATA_LSB         0
#define Si2168B_SET_PROPERTY_RESPONSE_DATA_MASK        0xffff

#endif /* Si2168B_SET_PROPERTY_CMD */

/* Si2168B_SPI_LINK command definition */
#define Si2168B_SPI_LINK_CMD 0xc0

#ifdef Si2168B_SPI_LINK_CMD
#define Si2168B_SPI_LINK_CMD_CODE 0x0400c0

typedef struct { /* Si2168B_SPI_LINK_CMD_struct */
	u8  subcode;
	u8  spi_pbl_key;
	u8  spi_pbl_num;
	u8  spi_conf_clk;
	u8  spi_clk_pola;
	u8  spi_conf_data;
	u8  spi_data_dir;
	u8  spi_enable;
} Si2168B_SPI_LINK_CMD_struct;

/* SPI_LINK command, SUBCODE field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_SPI_LINK_CMD_SUBCODE_LSB         0
#define Si2168B_SPI_LINK_CMD_SUBCODE_MASK        0xff
#define Si2168B_SPI_LINK_CMD_SUBCODE_MIN         56
#define Si2168B_SPI_LINK_CMD_SUBCODE_MAX         56
#define Si2168B_SPI_LINK_CMD_SUBCODE_CODE  56
/* SPI_LINK command, SPI_PBL_KEY field definition (address 2,size 8, lsb 0, unsigned) */
#define Si2168B_SPI_LINK_CMD_SPI_PBL_KEY_LSB         0
#define Si2168B_SPI_LINK_CMD_SPI_PBL_KEY_MASK        0xff
#define Si2168B_SPI_LINK_CMD_SPI_PBL_KEY_MIN         0
#define Si2168B_SPI_LINK_CMD_SPI_PBL_KEY_MAX         255
/* SPI_LINK command, SPI_PBL_NUM field definition (address 3,size 4, lsb 0, unsigned) */
#define Si2168B_SPI_LINK_CMD_SPI_PBL_NUM_LSB         0
#define Si2168B_SPI_LINK_CMD_SPI_PBL_NUM_MASK        0x0f
#define Si2168B_SPI_LINK_CMD_SPI_PBL_NUM_MIN         0
#define Si2168B_SPI_LINK_CMD_SPI_PBL_NUM_MAX         15
/* SPI_LINK command, SPI_CONF_CLK field definition (address 4,size 4, lsb 0, unsigned) */
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_LSB         0
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_MASK        0x0f
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_MIN         0
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_MAX         9
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_DISABLE     0
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_DISEQC_CMD  9
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_DISEQC_IN   7
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_DISEQC_OUT  8
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_GPIO0       5
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_GPIO1       6
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_MP_A        1
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_MP_B        2
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_MP_C        3
#define Si2168B_SPI_LINK_CMD_SPI_CONF_CLK_MP_D        4
/* SPI_LINK command, SPI_CLK_POLA field definition (address 4,size 1, lsb 4, unsigned) */
#define Si2168B_SPI_LINK_CMD_SPI_CLK_POLA_LSB         4
#define Si2168B_SPI_LINK_CMD_SPI_CLK_POLA_MASK        0x01
#define Si2168B_SPI_LINK_CMD_SPI_CLK_POLA_MIN         0
#define Si2168B_SPI_LINK_CMD_SPI_CLK_POLA_MAX         1
#define Si2168B_SPI_LINK_CMD_SPI_CLK_POLA_FALLING  1
#define Si2168B_SPI_LINK_CMD_SPI_CLK_POLA_RISING   0
/* SPI_LINK command, SPI_CONF_DATA field definition (address 5,size 4, lsb 0, unsigned) */
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_LSB         0
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_MASK        0x0f
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_MIN         0
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_MAX         9
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_DISABLE     0
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_DISEQC_CMD  9
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_DISEQC_IN   7
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_DISEQC_OUT  8
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_GPIO0       5
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_GPIO1       6
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_MP_A        1
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_MP_B        2
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_MP_C        3
#define Si2168B_SPI_LINK_CMD_SPI_CONF_DATA_MP_D        4
/* SPI_LINK command, SPI_DATA_DIR field definition (address 5,size 1, lsb 4, unsigned) */
#define Si2168B_SPI_LINK_CMD_SPI_DATA_DIR_LSB         4
#define Si2168B_SPI_LINK_CMD_SPI_DATA_DIR_MASK        0x01
#define Si2168B_SPI_LINK_CMD_SPI_DATA_DIR_MIN         0
#define Si2168B_SPI_LINK_CMD_SPI_DATA_DIR_MAX         1
#define Si2168B_SPI_LINK_CMD_SPI_DATA_DIR_LSB_FIRST  1
#define Si2168B_SPI_LINK_CMD_SPI_DATA_DIR_MSB_FIRST  0
/* SPI_LINK command, SPI_ENABLE field definition (address 6,size 1, lsb 0, unsigned) */
#define Si2168B_SPI_LINK_CMD_SPI_ENABLE_LSB         0
#define Si2168B_SPI_LINK_CMD_SPI_ENABLE_MASK        0x01
#define Si2168B_SPI_LINK_CMD_SPI_ENABLE_MIN         0
#define Si2168B_SPI_LINK_CMD_SPI_ENABLE_MAX         1
#define Si2168B_SPI_LINK_CMD_SPI_ENABLE_DISABLE  0
#define Si2168B_SPI_LINK_CMD_SPI_ENABLE_ENABLE   1
#endif /* Si2168B_SPI_LINK_CMD */

/* Si2168B_SPI_PASSTHROUGH command definition */
#define Si2168B_SPI_PASSTHROUGH_CMD 0xc0

#ifdef Si2168B_SPI_PASSTHROUGH_CMD
#define Si2168B_SPI_PASSTHROUGH_CMD_CODE 0x0500c0

typedef struct { /* Si2168B_SPI_PASSTHROUGH_CMD_struct */
	u8  subcode;
	u8  spi_passthr_clk;
	u8  spi_passth_data;
} Si2168B_SPI_PASSTHROUGH_CMD_struct;

/* SPI_PASSTHROUGH command, SUBCODE field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_SPI_PASSTHROUGH_CMD_SUBCODE_LSB         0
#define Si2168B_SPI_PASSTHROUGH_CMD_SUBCODE_MASK        0xff
#define Si2168B_SPI_PASSTHROUGH_CMD_SUBCODE_MIN         64
#define Si2168B_SPI_PASSTHROUGH_CMD_SUBCODE_MAX         64
#define Si2168B_SPI_PASSTHROUGH_CMD_SUBCODE_CODE  64
/* SPI_PASSTHROUGH command, SPI_PASSTHR_CLK field definition (address 2,size 4, lsb 0, unsigned) */
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_LSB         0
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_MASK        0x0f
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_MIN         0
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_MAX         10
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_DISABLE     0
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_DISEQC_CMD  9
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_DISEQC_IN   7
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_DISEQC_OUT  8
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_GPIO0       5
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_GPIO1       6
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_MP_A        1
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_MP_B        2
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_MP_C        3
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_MP_D        4
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTHR_CLK_SCL_MAST    10
/* SPI_PASSTHROUGH command, SPI_PASSTH_DATA field definition (address 3,size 4, lsb 0, unsigned) */
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_LSB         0
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_MASK        0x0f
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_MIN         0
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_MAX         10
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_DISABLE     0
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_DISEQC_CMD  9
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_DISEQC_IN   7
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_DISEQC_OUT  8
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_GPIO0       5
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_GPIO1       6
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_MP_A        1
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_MP_B        2
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_MP_C        3
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_MP_D        4
#define Si2168B_SPI_PASSTHROUGH_CMD_SPI_PASSTH_DATA_SDA_MAST    10
#endif /* Si2168B_SPI_PASSTHROUGH_CMD */

/* Si2168B_START_CLK command definition */
#define Si2168B_START_CLK_CMD 0xc0

#ifdef Si2168B_START_CLK_CMD
#define Si2168B_START_CLK_CMD_CODE 0x0300c0

typedef struct { /* Si2168B_START_CLK_CMD_struct */
	u8  subcode;
	u8  reserved1;
	u8  tune_cap;
	u8  reserved2;
	u16 clk_mode;
	u8  reserved3;
	u8  reserved4;
	u8  start_clk;
} Si2168B_START_CLK_CMD_struct;

/* START_CLK command, SUBCODE field definition (address 1,size 8, lsb 0, unsigned) */
#define Si2168B_START_CLK_CMD_SUBCODE_LSB         0
#define Si2168B_START_CLK_CMD_SUBCODE_MASK        0xff
#define Si2168B_START_CLK_CMD_SUBCODE_MIN         18
#define Si2168B_START_CLK_CMD_SUBCODE_MAX         18
#define Si2168B_START_CLK_CMD_SUBCODE_CODE  18
/* START_CLK command, RESERVED1 field definition (address 2,size 8, lsb 0, unsigned) */
#define Si2168B_START_CLK_CMD_RESERVED1_LSB         0
#define Si2168B_START_CLK_CMD_RESERVED1_MASK        0xff
#define Si2168B_START_CLK_CMD_RESERVED1_MIN         0
#define Si2168B_START_CLK_CMD_RESERVED1_MAX         0
#define Si2168B_START_CLK_CMD_RESERVED1_RESERVED  0
/* START_CLK command, TUNE_CAP field definition (address 3,size 4, lsb 0, unsigned) */
#define Si2168B_START_CLK_CMD_TUNE_CAP_LSB         0
#define Si2168B_START_CLK_CMD_TUNE_CAP_MASK        0x0f
#define Si2168B_START_CLK_CMD_TUNE_CAP_MIN         0
#define Si2168B_START_CLK_CMD_TUNE_CAP_MAX         15
#define Si2168B_START_CLK_CMD_TUNE_CAP_10P4     8
#define Si2168B_START_CLK_CMD_TUNE_CAP_11P7     9
#define Si2168B_START_CLK_CMD_TUNE_CAP_13P0     10
#define Si2168B_START_CLK_CMD_TUNE_CAP_14P3     11
#define Si2168B_START_CLK_CMD_TUNE_CAP_15P6     12
#define Si2168B_START_CLK_CMD_TUNE_CAP_16P9     13
#define Si2168B_START_CLK_CMD_TUNE_CAP_18P2     14
#define Si2168B_START_CLK_CMD_TUNE_CAP_19P5     15
#define Si2168B_START_CLK_CMD_TUNE_CAP_1P3      1
#define Si2168B_START_CLK_CMD_TUNE_CAP_2P6      2
#define Si2168B_START_CLK_CMD_TUNE_CAP_3P9      3
#define Si2168B_START_CLK_CMD_TUNE_CAP_5P2      4
#define Si2168B_START_CLK_CMD_TUNE_CAP_6P5      5
#define Si2168B_START_CLK_CMD_TUNE_CAP_7P8      6
#define Si2168B_START_CLK_CMD_TUNE_CAP_9P1      7
#define Si2168B_START_CLK_CMD_TUNE_CAP_EXT_CLK  0
/* START_CLK command, RESERVED2 field definition (address 3,size 4, lsb 4, unsigned) */
#define Si2168B_START_CLK_CMD_RESERVED2_LSB         4
#define Si2168B_START_CLK_CMD_RESERVED2_MASK        0x0f
#define Si2168B_START_CLK_CMD_RESERVED2_MIN         0
#define Si2168B_START_CLK_CMD_RESERVED2_MAX         0
#define Si2168B_START_CLK_CMD_RESERVED2_RESERVED  0
/* START_CLK command, CLK_MODE field definition (address 4,size 12, lsb 0, unsigned) */
#define Si2168B_START_CLK_CMD_CLK_MODE_LSB         0
#define Si2168B_START_CLK_CMD_CLK_MODE_MASK        0xfff
#define Si2168B_START_CLK_CMD_CLK_MODE_MIN         575
#define Si2168B_START_CLK_CMD_CLK_MODE_MAX         3328
#define Si2168B_START_CLK_CMD_CLK_MODE_CLK_CLKIO    3328
#define Si2168B_START_CLK_CMD_CLK_MODE_CLK_XTAL_IN  1536
#define Si2168B_START_CLK_CMD_CLK_MODE_XTAL         575
/* START_CLK command, RESERVED3 field definition (address 6,size 8, lsb 0, unsigned) */
#define Si2168B_START_CLK_CMD_RESERVED3_LSB         0
#define Si2168B_START_CLK_CMD_RESERVED3_MASK        0xff
#define Si2168B_START_CLK_CMD_RESERVED3_MIN         22
#define Si2168B_START_CLK_CMD_RESERVED3_MAX         22
#define Si2168B_START_CLK_CMD_RESERVED3_RESERVED  22
/* START_CLK command, RESERVED4 field definition (address 7,size 1, lsb 0, unsigned) */
#define Si2168B_START_CLK_CMD_RESERVED4_LSB         0
#define Si2168B_START_CLK_CMD_RESERVED4_MASK        0x01
#define Si2168B_START_CLK_CMD_RESERVED4_MIN         0
#define Si2168B_START_CLK_CMD_RESERVED4_MAX         0
#define Si2168B_START_CLK_CMD_RESERVED4_RESERVED  0
/* START_CLK command, START_CLK field definition (address 12,size 1, lsb 0, unsigned) */
#define Si2168B_START_CLK_CMD_START_CLK_LSB         0
#define Si2168B_START_CLK_CMD_START_CLK_MASK        0x01
#define Si2168B_START_CLK_CMD_START_CLK_MIN         0
#define Si2168B_START_CLK_CMD_START_CLK_MAX         0
#define Si2168B_START_CLK_CMD_START_CLK_START_CLK  0
#endif /* Si2168B_START_CLK_CMD */

/* readback values (detected channel properties) */
typedef struct { /* Si2168B_CHANNEL_SEEK_NEXT_REPLY_struct */
    s32 freq;
    u32 bandwidth_Hz;
    u32 symbol_rate_bps;
    u8  stream;
	u8  standard;
    u8  constellation;
    u8  num_plp;
    u8  T2_base_lite;
} Si2168B_CHANNEL_SEEK_NEXT_REPLY_struct;

typedef struct {
	u32  rangeMin;
	u32  rangeMax;
	u32  seekBWHz;
	u32  seekStepHz;
	u32  minSRbps;
	u32  maxSRbps;
	/*int  minRSSIdBm;*/
	/*int  maxRSSIdBm;*/
	/*int  minSNRHalfdB;*/
	/*int  maxSNRHalfdB;*/
} Si2168B_CHANNEL_SEEK_PARAM_struct;

#define Si2168B_GET_COMMAND_STRINGS

#define NO_Si2168B_ERROR                     0x00
#define ERROR_Si2168B_PARAMETER_OUT_OF_RANGE 0x01
#define ERROR_Si2168B_ALLOCATING_CONTEXT     0x02
#define ERROR_Si2168B_SENDING_COMMAND        0x03
#define ERROR_Si2168B_CTS_TIMEOUT            0x04
#define ERROR_Si2168B_ERR                    0x05
#define ERROR_Si2168B_POLLING_CTS            0x06
#define ERROR_Si2168B_POLLING_RESPONSE       0x07
#define ERROR_Si2168B_LOADING_FIRMWARE       0x08
#define ERROR_Si2168B_LOADING_BOOTBLOCK      0x09
#define ERROR_Si2168B_STARTING_FIRMWARE      0x0a
#define ERROR_Si2168B_SW_RESET               0x0b
#define ERROR_Si2168B_INCOMPATIBLE_PART      0x0c
#define ERROR_Si2168B_DISEQC_BUS_NOT_READY   0x0d
#define ERROR_Si2168B_UNKNOWN_COMMAND        0xf0
#define ERROR_Si2168B_UNKNOWN_PROPERTY       0xf1

typedef int  (*Si2168B_INDIRECT_I2C_FUNC)  (void*);

typedef struct si2168b_context {
	struct i2c_adapter *i2c_adap;
	u8  address;
	u8  i2c_addr;
	u8  power_up_reset;
	u8  power_up_func;
	u8  scan_ctrl_action;
	u8  dd_mode_modulation;
	u8  dd_mode_auto_detect;
	u8  dd_mode_invert_spectrum;
	u8  dd_mode_bw;
	u8  dvbt_hierarchy_stream;
	u16 dvbc_symbol_rate;
	u16 scan_fmin;
	u16 scan_fmax;
	u16 scan_symb_rate_min;
	u16 scan_symb_rate_max;
	u8  status_ddint;
	u8  status_scanint;
	u8  status_err;
	u8  status_cts;
	u8  media;

	/* _additional_struct_members_point */
	Si2168B_CLOCK_SOURCE tuner_ter_clock_source;
	u8  tuner_ter_clock_freq;
	u16 tuner_ter_clock_input;
	u8  tuner_ter_clock_control;
	u8  fef_selection;
	u8  fef_mode;
	u8  fef_pin;   /* FEF pin connected to TER tuner AGC freeze input      */
	u8  fef_level; /* GPIO state on FEF_pin when used (during FEF periods) */
	u8  Si2168B_in_standby;
	struct mutex lock;
	struct mutex ts_bus_ctrl_lock;
} si2168b_context;

typedef struct Si2168B_L2_Context {
	si2168b_context *demod;
	Si2168B_INDIRECT_I2C_FUNC  f_TER_tuner_enable;
	Si2168B_INDIRECT_I2C_FUNC  f_TER_tuner_disable;
	void *callback;
	u8  standard;
	u8  previous_standard;
	u8  first_init_done;
	u8  Si2168B_init_done;
	u8  TER_init_done;
	u8  tuner_indirect_i2c_connection;
	u8  auto_detect_TER;
	u8  tuneUnitHz;
	u8  seekAbort;
	u8  handshakeUsed;
	u32 handshakeStart_ms;
	u32 handshakePeriod_ms;
	u8  handshakeOn;
} Si2168B_L2_Context;

/* firmware_struct needs to be declared to allow loading the FW in 16 bytes mode */
#ifndef __FIRMWARE_STRUCT__
#define __FIRMWARE_STRUCT__
typedef struct firmware_struct {
	u8 firmware_len;
	u8 firmware_table[16];
} firmware_struct;
#endif /* __FIRMWARE_STRUCT__ */

struct Si2168B_Priv {
	struct i2c_adapter *i2c;
	const struct si2168b_config *config;
	struct dvb_frontend frontend;
	Si2168B_L2_Context *si_front_end;
	fe_delivery_system_t delivery_system;
	bool last_tune_failed; /* for switch between T and T2 tune */
};

#if defined(SiTRACES)
static void sitrace_default_configuration(void);
static void sitrace_function(const char *name, int trace_linenumber, const char *func, const char *fmt, ...);
#ifdef DRIVER_BUILD
#define SiTraceConfiguration(...) /* empty */
#else
char * SiTraceConfiguration(const char *config);
#endif
static void sitraces_suspend(void);
static void sitraces_resume(void);
#else
#define SiTraceDefaultConfiguration(...) /* empty */
#define SiTracesSuspend()                /* empty */
#define SiTracesResume()                 /* empty */
#define sitraces_suspend()               /* empty */
#define sitraces_resume()                /* empty */
#endif /* SiTRACES */

#ifndef __FIRMWARE_STRUCT__
#define __FIRMWARE_STRUCT__
typedef struct  {
	u8 firmware_len;
	u8 firmware_table[16];
} firmware_struct;
#endif /* __FIRMWARE_STRUCT__ */

#define _Si2168B_PATCH16_4_0b9_TABLE_H_

#define Si2168B_PATCH16_4_0b9_PART    68
#define Si2168B_PATCH16_4_0b9_ROM      1
#define Si2168B_PATCH16_4_0b9_PMAJOR  '4'
#define Si2168B_PATCH16_4_0b9_PMINOR  '0'
#define Si2168B_PATCH16_4_0b9_PBUILD   2

#ifndef __FIRMWARE_STRUCT__
#define __FIRMWARE_STRUCT__
typedef struct firmware_struct {
	u8 firmware_len;
	u8 firmware_table[16];
} firmware_struct;
#endif /* __FIRMWARE_STRUCT__ */

firmware_struct Si2168B_PATCH16_4_0b9[] = {
		{  8 , { 0x05,0x00,0xF6,0x30,0x56,0x40,0x00,0x00 } },
		{  8 , { 0x0C,0x58,0x70,0xA0,0x93,0x5E,0x38,0x95 } },
		{  3 , { 0x4A,0x4F,0xFA } },
		{  8 , { 0x0C,0xD2,0xAA,0x09,0x35,0xDD,0xF3,0x87 } },
		{  6 , { 0x4D,0x8E,0x5B,0xAC,0x28,0xB5 } },
		{  8 , { 0x0C,0x53,0x6F,0x17,0xD9,0x7C,0x82,0xBD } },
		{  3 , { 0x4A,0x55,0xF4 } },
		{  8 , { 0x0C,0xC4,0x62,0x78,0x18,0x46,0x92,0xCF } },
		{ 12 , { 0x3B,0xC5,0x82,0xE0,0xD9,0x60,0x03,0xDC,0x4C,0x1B,0xA0,0xC7 } },
		{  8 , { 0x08,0xEE,0x78,0xEE,0xD4,0xCF,0x22,0xAA } },
		{  8 , { 0x0C,0x03,0xCD,0x8E,0x92,0xD6,0x73,0xA6 } },
		{  3 , { 0x42,0x10,0xAA } },
		{  8 , { 0x0C,0x16,0xB2,0xD3,0x2D,0xC0,0xBB,0xF5 } },
		{  3 , { 0x4A,0x95,0x66 } },
		{  8 , { 0x0C,0xEC,0xA1,0x54,0xC2,0xA2,0x1E,0xCD } },
		{ 16 , { 0x3F,0x4F,0xF5,0x82,0x11,0x5D,0xD9,0x99,0x0D,0xD1,0xE7,0xBC,0xCA,0x9C,0x50,0x52 } },
		{  3 , { 0x4A,0x2A,0xBC } },
		{  8 , { 0x09,0x65,0x97,0x63,0x35,0xFB,0xDC,0xEF } },
		{  8 , { 0x09,0x2E,0xC0,0x2B,0xC4,0xBF,0x17,0x6E } },
		{  8 , { 0x08,0x27,0x3D,0x6F,0x7A,0x38,0x99,0x91 } },
		{  9 , { 0x38,0xE2,0xAC,0x75,0xDC,0x28,0x6A,0x7D,0x63 } },
		{  8 , { 0x08,0xEA,0xA3,0xAF,0xC6,0x01,0xE7,0x58 } },
		{  5 , { 0x44,0x97,0x01,0x98,0xBC } },
		{  8 , { 0x09,0x50,0x28,0x28,0xFA,0x1E,0x0B,0xC5 } },
		{ 13 , { 0x34,0x30,0xA5,0x54,0x05,0x07,0xE4,0x44,0x1C,0xEE,0x15,0xD4,0x87 } },
		{  8 , { 0x08,0xC2,0xF6,0x3D,0x59,0xF4,0x0D,0xC0 } },
		{  4 , { 0x43,0xD5,0x47,0x3C } },
		{  8 , { 0x09,0x40,0xC7,0xBF,0x5E,0x22,0x74,0x99 } },
		{ 16 , { 0x3F,0xFC,0xF1,0x57,0x95,0x67,0x26,0xA7,0x38,0x7A,0xB4,0x9B,0xFD,0xA5,0x29,0xB9 } },
		{ 16 , { 0x3F,0x71,0x73,0x43,0xB4,0x27,0x6D,0xAB,0x98,0xC9,0x95,0xD6,0x8F,0x77,0xE6,0xDF } },
		{ 12 , { 0x3B,0x84,0xC5,0x5B,0x75,0x7F,0x80,0x6C,0xED,0x09,0x65,0xE8 } },
		{  8 , { 0x09,0x32,0xE6,0x0D,0x2D,0x20,0x13,0x94 } },
		{ 16 , { 0x3F,0x4B,0xC8,0xAC,0x22,0x45,0x44,0x2F,0x99,0xA7,0x2E,0x1F,0xD9,0xDE,0xF4,0x76 } },
		{ 16 , { 0x37,0x9B,0x3F,0xAA,0x3A,0x33,0x16,0xA4,0xD5,0xDF,0xD9,0xBA,0x5C,0x3A,0x39,0x2B } },
		{  8 , { 0x09,0xE0,0xAD,0xDE,0x8A,0x6B,0x2F,0xDB } },
		{ 16 , { 0x37,0xC5,0xE2,0xAC,0x74,0x5D,0x77,0xAC,0x2E,0x79,0x2B,0x65,0x66,0xCA,0xDE,0xB2 } },
		{ 16 , { 0x37,0xEF,0x9C,0x99,0x03,0x34,0xF7,0xEF,0xEE,0x71,0xD1,0x71,0xCE,0x0B,0xF3,0x23 } },
		{ 16 , { 0x3F,0xCE,0xAC,0xF4,0x52,0x29,0x1D,0xFD,0xA8,0xF9,0xDC,0xCA,0x5B,0x71,0xF8,0x7B } },
		{ 16 , { 0x3F,0x20,0xB1,0xB9,0xA8,0xBA,0x4E,0xC3,0x43,0xBF,0xD4,0x37,0xE1,0x21,0xED,0x4E } },
		{ 16 , { 0x37,0x02,0xE2,0x06,0x37,0x01,0x19,0x03,0x46,0x9E,0x25,0xC2,0xAF,0xA5,0x0C,0x05 } },
		{ 16 , { 0x3F,0x24,0xFB,0x02,0x2F,0x87,0x72,0x72,0x78,0x33,0x20,0x06,0xC4,0x92,0x11,0x52 } },
		{ 16 , { 0x3F,0x12,0x59,0xDB,0xB6,0xA0,0xF6,0x5F,0x8D,0xF5,0x5D,0x42,0x6F,0x0B,0x85,0xFF } },
		{ 16 , { 0x37,0x9D,0x7F,0xC0,0x52,0xA4,0xB1,0xA7,0xC6,0x15,0x78,0xDF,0xFA,0x1D,0x25,0xEA } },
		{ 13 , { 0x3C,0x4C,0xFA,0x1F,0x73,0x44,0x7B,0xEA,0x30,0xBD,0xD6,0xB6,0xCF } },
		{  8 , { 0x0C,0x64,0x3E,0xB6,0x62,0xE2,0x4F,0x90 } },
		{ 16 , { 0x3F,0x04,0xA9,0xCF,0xC3,0x9C,0x93,0xFE,0x68,0x91,0xD3,0x30,0x5F,0x5C,0x34,0x4A } },
		{ 10 , { 0x39,0xDB,0xD8,0x30,0x2F,0xB5,0x7A,0xFE,0x1A,0x37 } },
		{  8 , { 0x0C,0x5A,0x55,0x79,0xD2,0x70,0xCC,0x14 } },
		{  3 , { 0x4A,0x61,0xD8 } },
		{  8 , { 0x08,0xBF,0x36,0xE1,0x0F,0x76,0x0E,0x97 } },
		{  8 , { 0x09,0xBF,0x03,0x26,0xB6,0x79,0x25,0x4C } },
		{  8 , { 0x09,0x23,0xE6,0x93,0x4E,0xC1,0x1D,0xE6 } },
		{  8 , { 0x0C,0x65,0x84,0xE0,0x20,0xAB,0x9A,0xA7 } },
		{  5 , { 0x44,0xF2,0xCE,0x5B,0x51 } },
		{  8 , { 0x0C,0xF6,0x12,0xDA,0x80,0x37,0x21,0x29 } },
		{  5 , { 0x44,0x4F,0x25,0x61,0xA1 } },
		{  8 , { 0x0C,0x98,0xC3,0x69,0x6C,0x0A,0xBD,0x02 } },
		{  9 , { 0x30,0x6D,0xA6,0x9C,0xE7,0x35,0xE1,0xF8,0xB2 } },
		{  8 , { 0x0C,0x74,0xA9,0x08,0x36,0xAC,0x71,0xD7 } },
		{  5 , { 0x44,0x3E,0x84,0x38,0x27 } },
		{  8 , { 0x0C,0x1A,0x98,0xB0,0x3D,0x03,0x53,0xD6 } },
		{  5 , { 0x44,0xE6,0x56,0xFF,0x61 } },
		{  8 , { 0x0C,0x8F,0xCC,0xCA,0xB6,0x41,0x85,0xDE } },
		{  5 , { 0x4C,0xB1,0x93,0xE7,0x2A } },
		{  8 , { 0x0C,0x0A,0xBD,0xB6,0xED,0xD5,0xC0,0x5B } },
		{  5 , { 0x4C,0x2B,0x36,0x8D,0xA2 } },
		{  8 , { 0x0C,0x7A,0x22,0x9D,0x40,0x0F,0x83,0xE9 } },
		{  5 , { 0x4C,0xE0,0x9D,0xC6,0xE1 } },
		{  8 , { 0x0C,0xFA,0x6E,0x54,0x74,0x21,0xBF,0x33 } },
		{  5 , { 0x44,0xB1,0xE5,0xC4,0xCC } },
		{  8 , { 0x0C,0xEF,0xFB,0xE5,0x37,0x7E,0xB8,0xC0 } },
		{ 16 , { 0x3F,0xF5,0x51,0x23,0xE2,0x63,0x8A,0x44,0xF2,0xD1,0x48,0xAA,0x8E,0x32,0x3F,0x24 } },
		{  2 , { 0x49,0x54 } },
		{  8 , { 0x0C,0xBF,0x04,0xB0,0xDA,0xEF,0xD3,0x63 } },
		{  5 , { 0x4C,0xEF,0xE1,0x6F,0xD0 } },
		{  8 , { 0x0C,0x23,0xC4,0xF2,0xB0,0x9F,0x5D,0xF8 } },
		{  5 , { 0x44,0x76,0x8F,0x57,0x79 } },
		{  8 , { 0x0C,0x11,0xE4,0xE8,0x77,0x54,0x91,0x24 } },
		{  5 , { 0x4C,0x94,0x2F,0x21,0xF6 } },
		{  8 , { 0x0C,0x4A,0xBE,0xCC,0xE2,0xC5,0x58,0xCF } },
		{  5 , { 0x4C,0x79,0xEE,0x3E,0xE6 } },
		{  8 , { 0x0C,0xE0,0xD0,0x42,0xAA,0xFB,0xBB,0xAC } },
		{  5 , { 0x44,0xC4,0x86,0x6D,0x84 } },
		{  8 , { 0x0C,0x04,0x16,0x66,0x22,0xDA,0x5E,0x1C } },
		{  5 , { 0x44,0xE2,0xE2,0x56,0x4D } },
		{  8 , { 0x0C,0x2D,0x68,0x26,0xE7,0xAA,0x22,0x0A } },
		{  5 , { 0x44,0x10,0x7F,0x20,0xF6 } },
		{  8 , { 0x0C,0x60,0x61,0xD4,0xE4,0xD1,0xBF,0x01 } },
		{  5 , { 0x4C,0x58,0x91,0xA2,0xF2 } },
		{  8 , { 0x0C,0xA9,0xD1,0x46,0x0A,0x23,0xBB,0xB8 } },
		{  9 , { 0x38,0x84,0xBF,0x5A,0xAB,0x0D,0x95,0x87,0x60 } },
		{  8 , { 0x0C,0x75,0xE8,0x5D,0x32,0x86,0x20,0x8D } },
		{  5 , { 0x4C,0xD0,0x18,0xDA,0x26 } },
		{  8 , { 0x0C,0xF1,0xE0,0x5E,0xF7,0x77,0xCE,0xFF } },
		{  5 , { 0x4C,0x78,0x75,0x06,0xBF } },
		{  8 , { 0x0C,0x18,0x5A,0x27,0xE7,0x2A,0x50,0x94 } },
		{  5 , { 0x4C,0xD0,0xAC,0x59,0xDD } },
		{  8 , { 0x0C,0x92,0xB2,0xFD,0xB9,0xE0,0x7A,0x15 } },
		{  5 , { 0x4C,0x6A,0x3C,0x2B,0xD3 } },
		{  8 , { 0x0C,0x2D,0xF3,0x5C,0x73,0xD2,0x3C,0x0F } },
		{  5 , { 0x4C,0xFA,0xE2,0xA5,0x9D } },
		{  8 , { 0x0C,0xFE,0xCD,0x58,0x12,0x14,0xA9,0x19 } },
		{  5 , { 0x4C,0x97,0x9A,0xF9,0xA9 } },
		{  8 , { 0x0C,0x4F,0x48,0xAC,0xF1,0x7B,0x52,0xE6 } },
		{ 13 , { 0x3C,0x1B,0x90,0xB6,0xD1,0x38,0x35,0xE8,0x29,0x04,0x18,0xD3,0x16 } },
		{  8 , { 0x0C,0xBD,0x2C,0x0F,0xFC,0x2B,0xFF,0xD9 } },
		{  5 , { 0x4C,0xEE,0x32,0x98,0x52 } },
		{  8 , { 0x0C,0xF5,0x9E,0x18,0x04,0xBE,0x0E,0xCD } },
		{ 13 , { 0x3C,0x29,0xC5,0x08,0x14,0x7B,0x01,0x05,0xAB,0x1C,0xE1,0x07,0xF6 } },
		{  8 , { 0x0C,0x97,0xA0,0xE5,0x29,0x15,0xD1,0x32 } },
		{  5 , { 0x44,0x30,0x41,0x18,0xA3 } },
		{  8 , { 0x0C,0xAC,0x97,0xBE,0xAE,0x5F,0xFE,0x0B } },
		{  5 , { 0x4C,0x60,0x20,0xB8,0x04 } },
		{  8 , { 0x0C,0xA9,0x9F,0x96,0x2E,0xF7,0x11,0x05 } },
		{  5 , { 0x44,0x1B,0x00,0x99,0xE5 } },
		{  8 , { 0x0C,0x92,0x29,0x74,0x3B,0x1C,0x50,0x7C } },
		{ 16 , { 0x37,0x0F,0xE9,0x22,0x33,0x2A,0x96,0x93,0x9E,0x99,0x71,0xA4,0x5D,0xA2,0xC5,0x4A } },
		{  2 , { 0x49,0xB4 } },
		{  8 , { 0x08,0x95,0x6D,0x10,0x64,0x62,0xF2,0x68 } },
		{ 16 , { 0x3F,0x1F,0xE6,0x74,0x05,0x4E,0x58,0xE0,0x8E,0x17,0x31,0x18,0x3F,0xB7,0xEC,0x68 } },
		{  2 , { 0x49,0x96 } },
		{  8 , { 0x09,0x61,0xC0,0x9C,0x35,0x49,0x44,0x82 } },
		{  8 , { 0x08,0x1C,0x39,0x93,0xC8,0xBD,0xF1,0x87 } },
		{ 16 , { 0x3F,0x3A,0x78,0xF2,0x62,0x23,0x18,0x11,0x13,0xEB,0xDF,0x66,0x9A,0x2D,0x40,0x5E } },
		{ 16 , { 0x37,0x91,0x61,0x2B,0x76,0xF7,0x15,0xF9,0xCD,0x82,0xB7,0x97,0xB4,0xF5,0x5D,0x7A } },
		{ 16 , { 0x37,0xED,0x0A,0x9F,0xF1,0x60,0xAA,0xF4,0x30,0x65,0xC0,0xDF,0x73,0x56,0x9D,0xE2 } },
		{  4 , { 0x4B,0x6B,0xB3,0x23 } },
		{  8 , { 0x09,0xFB,0x68,0xD7,0x90,0x6D,0x20,0xA2 } },
		{  5 , { 0x44,0x7C,0x22,0xF3,0x48 } },
		{  8 , { 0x09,0x47,0x06,0x1B,0x86,0x03,0xAC,0x02 } },
		{ 16 , { 0x37,0x06,0xF1,0xB0,0xF6,0xC1,0x21,0x94,0xB7,0xA4,0xDB,0x07,0x70,0x65,0x70,0xB7 } },
		{ 16 , { 0x37,0xFE,0x60,0x67,0x04,0x50,0x0D,0xC4,0x4D,0x1F,0x3F,0xCD,0xE2,0x0D,0x2A,0xF4 } },
		{ 11 , { 0x3A,0x98,0x70,0x26,0x0F,0x02,0x44,0xD9,0x5B,0x7A,0x25 } },
		{  8 , { 0x09,0x70,0x85,0x33,0x5C,0xB0,0x38,0x76 } },
		{ 16 , { 0x37,0xB4,0x46,0x17,0xC8,0x3B,0x40,0x06,0xF6,0x09,0xC1,0x50,0xFC,0x88,0xF8,0x78 } },
		{  6 , { 0x45,0x58,0x72,0x01,0x68,0x55 } },
		{  8 , { 0x08,0xC3,0x6B,0x35,0x0B,0x23,0x96,0x68 } },
		{ 13 , { 0x34,0xFE,0x7E,0xC9,0x63,0xAE,0x53,0xDB,0x84,0x39,0xE5,0x49,0xC4 } },
		{  8 , { 0x09,0x78,0xD6,0x0A,0x49,0x2E,0xA9,0x12 } },
		{ 16 , { 0x3F,0x52,0x76,0xCD,0x56,0x9C,0xB5,0xD1,0x28,0xA0,0x76,0x38,0x86,0x0E,0x6F,0xEE } },
		{ 16 , { 0x3F,0x27,0xFF,0x9D,0xD2,0xAF,0xAB,0x85,0xAB,0x99,0x66,0x59,0x4E,0x65,0x22,0xDF } },
		{ 16 , { 0x3F,0x6A,0x5F,0x3F,0x88,0xB1,0x12,0x3B,0x98,0x87,0xCD,0x33,0x17,0x58,0xAA,0x94 } },
		{  4 , { 0x43,0xC3,0xD6,0xE6 } },
		{  8 , { 0x08,0x66,0x32,0xC6,0xE7,0x19,0x82,0x94 } },
		{  5 , { 0x44,0x13,0x97,0x1A,0x8C } },
		{ 16 , { 0x37,0x7B,0x0A,0xC5,0x48,0xA8,0xAF,0x6C,0xA1,0x85,0x76,0x54,0x67,0xE8,0x36,0xDE } },
		{ 16 , { 0x3F,0x45,0x8A,0xE8,0x61,0xB7,0xEE,0xB9,0xD1,0x45,0x51,0x79,0xDF,0x92,0xD9,0x9E } },
		{ 15 , { 0x3E,0xF5,0x2B,0x36,0xF7,0xF6,0xD7,0x19,0x01,0xAF,0x24,0x2F,0xBA,0xCD,0xA7 } },
		{  9 , { 0x30,0x91,0xE3,0x38,0x34,0xF6,0x6E,0xF0,0x92 } },
		{  8 , { 0x09,0x7A,0xD7,0x88,0x59,0x78,0xF9,0x59 } },
		{  5 , { 0x4C,0x57,0xFB,0xBF,0xAC } },
		{  8 , { 0x09,0x4B,0x26,0xCE,0x3B,0x4B,0xEE,0x40 } },
		{ 16 , { 0x3F,0xF0,0xB2,0x05,0xAC,0x79,0x46,0xE6,0x7D,0xBA,0xB1,0x03,0xCF,0x1E,0x9B,0x64 } },
		{ 16 , { 0x3F,0x78,0x48,0xC1,0xE6,0xB1,0xD6,0x82,0x74,0x76,0x42,0x58,0x60,0xE5,0x8B,0x88 } },
		{ 16 , { 0x37,0x11,0x9D,0xE5,0x9C,0xEE,0x0D,0xC3,0x67,0x6F,0x5D,0x3D,0x1F,0x23,0x13,0x7D } },
		{  8 , { 0x4F,0x87,0x20,0x35,0xEF,0xA2,0x48,0xCA } },
		{  8 , { 0x09,0xA6,0x14,0x0A,0xF5,0xD2,0xFE,0xAE } },
		{ 13 , { 0x3C,0x5A,0x4D,0xCE,0x7B,0x72,0x54,0x2D,0xF5,0x6A,0xC6,0xC7,0x2B } },
		{  8 , { 0x08,0xF3,0xD9,0xF6,0x53,0x35,0xDE,0xA5 } },
		{  5 , { 0x44,0xED,0xD5,0x5C,0x8C } },
		{  8 , { 0x09,0xDB,0xB8,0x6B,0x17,0x9A,0xC1,0x11 } },
		{  9 , { 0x38,0x5C,0x74,0x5A,0x22,0x89,0x60,0x5B,0xAA } },
		{  8 , { 0x08,0x5E,0x13,0x0B,0x04,0xD3,0xFC,0xFF } },
		{ 13 , { 0x34,0x68,0x58,0xCC,0x67,0x2B,0x87,0xE3,0x8D,0xEF,0x87,0x1F,0x32 } },
		{  8 , { 0x09,0x21,0x6F,0xBD,0x66,0x70,0xD8,0xC1 } },
		{ 16 , { 0x3F,0x7E,0xC0,0xEF,0x53,0x59,0x30,0x8C,0xE3,0x18,0x28,0x80,0x21,0x7F,0x60,0x64 } },
		{ 16 , { 0x3F,0xFC,0x48,0x57,0x25,0x34,0xD4,0xA8,0xA1,0x43,0x24,0x27,0x2D,0x56,0xFE,0x01 } },
		{  3 , { 0x4A,0x57,0x84 } },
		{  8 , { 0x08,0xAF,0x8D,0xAF,0x3F,0xB1,0x32,0x44 } },
		{ 13 , { 0x34,0xA6,0x91,0xDE,0x72,0x1D,0xA2,0x93,0x46,0xC3,0x4E,0x17,0x38 } },
		{  8 , { 0x09,0x26,0x42,0x44,0x9D,0x31,0x64,0xB1 } },
		{ 16 , { 0x37,0xA9,0x53,0x9E,0x8D,0x66,0xD6,0x00,0xAC,0xEF,0x05,0x82,0xE8,0x8E,0xD7,0x2E } },
		{ 16 , { 0x3F,0xCF,0xD3,0x4D,0xFA,0x81,0x57,0xC2,0xF3,0x50,0x24,0x18,0x94,0xED,0xE6,0x6C } },
		{ 16 , { 0x3F,0x80,0xE7,0x7C,0x16,0xE1,0x26,0xB7,0x33,0x5F,0x4B,0x9E,0xF4,0x05,0x48,0x44 } },
		{ 16 , { 0x3F,0x44,0xEF,0x9D,0xF9,0x40,0xE5,0xA6,0x73,0xA5,0x76,0xF3,0x0E,0x15,0x66,0x2A } },
		{ 16 , { 0x37,0xF0,0xF9,0x7C,0xE2,0x35,0x47,0x57,0xB5,0x0C,0xE5,0x0D,0x4B,0xE0,0x4F,0x1B } },
		{ 14 , { 0x3D,0x24,0x58,0x97,0x02,0x2F,0xA2,0x4A,0x59,0xA9,0xD7,0xDE,0x9C,0x9E } },
		{  8 , { 0x08,0x9A,0xCD,0xEF,0x62,0xCF,0xB6,0x61 } },
		{ 16 , { 0x37,0xFE,0x79,0xFB,0x0A,0x3E,0x19,0x70,0x03,0x94,0x5E,0x6E,0x63,0x71,0xE3,0x06 } },
		{ 14 , { 0x35,0x54,0x2D,0x0F,0x07,0xBB,0x46,0x9E,0x34,0xF8,0xBA,0x87,0x0B,0xA6 } },
		{  8 , { 0x09,0x47,0x0B,0x2F,0x06,0xF3,0x6E,0x1D } },
		{ 16 , { 0x37,0x9F,0x27,0xCA,0x16,0xAC,0x24,0x71,0x8E,0xB2,0xD2,0x35,0x09,0x8C,0xAD,0x81 } },
		{ 16 , { 0x3F,0x82,0xF9,0xDA,0x35,0xEF,0x0C,0x7C,0x2C,0x11,0xE1,0x43,0x8A,0x29,0x42,0x05 } },
		{ 11 , { 0x32,0xA5,0x72,0x9C,0x34,0x7F,0x0D,0xC6,0xCF,0xF2,0x1D } },
		{  8 , { 0x08,0xED,0x1F,0x44,0x44,0x2C,0x17,0x10 } },
		{  8 , { 0x08,0xBD,0x19,0x2E,0xC6,0x50,0x2F,0xEE } },
		{  8 , { 0x09,0x1F,0x38,0x17,0x95,0x2C,0xE6,0x27 } },
		{ 16 , { 0x37,0x4F,0x69,0x25,0x5E,0xC9,0xC0,0xF4,0xAB,0xA2,0x22,0xCB,0x6F,0xEA,0xE0,0x5A } },
		{ 16 , { 0x37,0x2D,0xCD,0x85,0xCE,0x41,0x68,0xD9,0x0D,0x5C,0x7A,0x6A,0x02,0x56,0xAF,0xEB } },
		{ 16 , { 0x37,0xF4,0x10,0x4F,0x26,0x9C,0x8B,0x60,0x9F,0x68,0x3F,0xD6,0x93,0x6D,0x44,0xC6 } },
		{ 12 , { 0x33,0x00,0xAC,0x78,0xE9,0xFA,0xC3,0x23,0x0E,0x08,0x9B,0xC9 } },
		{  8 , { 0x09,0x35,0x6A,0x8B,0xB1,0xC4,0xFD,0x82 } },
		{  5 , { 0x4C,0x56,0x39,0x5C,0x71 } },
		{  8 , { 0x08,0x4C,0x31,0xB8,0x3C,0x35,0x55,0x0E } },
		{  8 , { 0x08,0x52,0xB6,0x48,0xCA,0x40,0xF8,0xBA } },
		{ 16 , { 0x37,0x85,0x77,0x83,0x57,0x26,0x9E,0xEC,0x2D,0x81,0x3A,0x47,0x64,0x49,0x4E,0x47 } },
		{ 16 , { 0x37,0x70,0xB5,0x66,0x86,0xC6,0x4C,0x24,0x6A,0x68,0x08,0x8F,0xBE,0xAE,0x4E,0xC4 } },
		{ 16 , { 0x3F,0x98,0xD2,0x93,0xA9,0x12,0x4D,0x50,0x0B,0x81,0x66,0x76,0xA8,0x52,0x06,0xC6 } },
		{  4 , { 0x43,0x85,0xDC,0x56 } },
		{  8 , { 0x09,0xD7,0x87,0x39,0x2E,0xDD,0xB5,0xB6 } },
		{  5 , { 0x44,0x93,0x97,0x06,0x9C } },
		{  8 , { 0x09,0x73,0x13,0x82,0xF4,0x86,0x61,0xBF } },
		{ 16 , { 0x37,0xEE,0xAB,0xB5,0x6D,0x7E,0x15,0x5E,0x8F,0x08,0xBB,0xA5,0x8F,0x02,0xBA,0xDF } },
		{ 16 , { 0x37,0x9F,0xA6,0x5E,0x6D,0x3B,0x07,0xA1,0x0B,0x00,0xE4,0x59,0xCF,0x95,0x02,0x17 } },
		{ 16 , { 0x3F,0x4A,0x51,0xF9,0xE5,0xC7,0xEA,0xCA,0x17,0xBB,0xEC,0xE3,0x72,0xB8,0x38,0xD3 } },
		{ 16 , { 0x3F,0x3C,0x5F,0x81,0xAD,0xA7,0xC2,0x6B,0x9E,0x58,0x64,0xB5,0x1E,0xDF,0xBC,0x9D } },
		{ 16 , { 0x3F,0x1A,0x62,0x2B,0x7E,0x4E,0xD0,0x13,0x2A,0xEE,0xFA,0xBD,0x65,0xF6,0x66,0x61 } },
		{ 16 , { 0x37,0x98,0xBE,0xD4,0xEA,0xED,0x1B,0x7B,0x04,0xC4,0x32,0x52,0x4E,0x33,0x1A,0x8A } },
		{ 16 , { 0x3F,0x3E,0x8C,0x16,0xDE,0xC6,0x6E,0xBE,0x50,0x36,0x72,0xF6,0x26,0xE6,0x51,0xDE } },
		{ 16 , { 0x37,0xE5,0x60,0x86,0xC3,0xBE,0xC0,0x0F,0xBF,0x05,0x1F,0xED,0x23,0xC3,0x1D,0x13 } },
		{ 16 , { 0x3F,0x91,0xFF,0xB3,0x9A,0x12,0x8F,0x04,0x4D,0x23,0x48,0xAD,0x0E,0x46,0xA4,0xBB } },
		{ 16 , { 0x37,0x87,0xF1,0x65,0xE2,0xE8,0x29,0xDA,0x4B,0x1B,0x06,0x7C,0xF6,0xCF,0x14,0xD9 } },
		{ 16 , { 0x37,0x9D,0x62,0x38,0x13,0xFD,0x8D,0x8D,0xA7,0xB9,0xDE,0x06,0xCE,0xFF,0xE1,0xB5 } },
		{ 16 , { 0x37,0x23,0xFB,0x05,0xD1,0x48,0x3C,0x22,0x92,0x48,0x4F,0x08,0x0B,0x5D,0x0C,0x1B } },
		{  9 , { 0x38,0x49,0xC0,0x76,0x17,0xEB,0x3C,0x04,0xDC } },
		{  8 , { 0x08,0x29,0xB6,0x14,0xBF,0x69,0x79,0x13 } },
		{ 16 , { 0x3F,0xB8,0x0A,0x0A,0x97,0xCC,0xD5,0x0C,0x82,0x49,0x3A,0x81,0x5A,0x3E,0x25,0x84 } },
		{  6 , { 0x45,0x96,0x0B,0xA8,0x89,0xE6 } },
		{  8 , { 0x08,0x6F,0x77,0xA1,0xD4,0x50,0x7F,0xDB } },
		{  9 , { 0x38,0xAA,0xE7,0x85,0xFF,0x84,0x56,0xF4,0x3E } },
		{  8 , { 0x08,0x2A,0x70,0xD6,0x03,0x5C,0x4B,0xA5 } },
		{ 16 , { 0x3F,0x82,0xBC,0x35,0xA6,0xF7,0x6A,0x91,0x47,0x93,0x11,0x28,0xE4,0xE8,0x9E,0xA3 } },
		{  2 , { 0x49,0x07 } },
		{  8 , { 0x09,0xAD,0x07,0x5A,0xF2,0xAF,0x56,0x37 } },
		{ 16 , { 0x37,0xAF,0xE4,0x5E,0x58,0xF8,0x0A,0xDE,0x26,0x03,0x1B,0x5C,0x1C,0x81,0xF3,0xB0 } },
		{ 16 , { 0x3F,0xD7,0x45,0xCE,0xB0,0xBF,0xC2,0x55,0x28,0xA2,0xF8,0xB2,0x35,0x6E,0xE1,0x35 } },
		{ 15 , { 0x3E,0x17,0x2C,0x12,0x65,0x14,0x35,0xA6,0xA6,0x7A,0x8A,0xC3,0x9E,0x87,0x3C } },
		{  8 , { 0x08,0xC8,0x85,0x2D,0xC7,0xCC,0xF3,0xED } },
		{ 13 , { 0x34,0xC4,0xC3,0x32,0x18,0xA5,0x53,0x6B,0xD3,0x75,0x40,0x3D,0x44 } },
		{  8 , { 0x09,0xA7,0xDC,0xA4,0x12,0xEE,0xE3,0x8A } },
		{  9 , { 0x30,0x8E,0x7D,0x3B,0x11,0x55,0x02,0x49,0xC0 } },
		{  8 , { 0x08,0xE9,0x20,0xCC,0xB1,0x89,0xA2,0xF4 } },
		{ 16 , { 0x3F,0x8C,0x8E,0xCC,0xA2,0x89,0x74,0x0D,0x43,0x07,0x3E,0x3A,0xD4,0x9E,0xFF,0x92 } },
		{  6 , { 0x4D,0xAB,0x50,0x1C,0xEF,0x26 } },
		{  8 , { 0x09,0xA1,0xF6,0x31,0xB1,0x21,0x5B,0x18 } },
		{  8 , { 0x08,0x82,0xAF,0xF6,0x75,0x9E,0xF5,0xE5 } },
		{  5 , { 0x44,0xB0,0x28,0xA9,0x5A } },
		{  8 , { 0x08,0xF9,0xE1,0xBF,0x17,0x87,0x46,0x52 } },
		{  9 , { 0x38,0x92,0x8E,0x4C,0x3A,0x62,0x95,0x43,0xB6 } },
		{  8 , { 0x09,0x73,0xCC,0x09,0xEC,0x39,0x6B,0xF1 } },
		{ 16 , { 0x37,0x30,0x72,0x00,0x70,0x8D,0x5A,0x92,0x0B,0x60,0x73,0xFC,0x1D,0xCE,0xB9,0x44 } },
		{ 16 , { 0x3F,0x1D,0x5E,0x80,0x26,0x5C,0x3E,0x96,0xCF,0x07,0x8F,0x82,0x80,0xDF,0x25,0xD1 } },
		{ 16 , { 0x3F,0xCC,0x57,0xB9,0x5F,0xCA,0xAB,0x45,0xB1,0x0E,0xC7,0x3B,0x62,0x3A,0x30,0x01 } },
		{ 12 , { 0x33,0x32,0x0A,0xA4,0xBE,0xBE,0x99,0xD7,0xDC,0x95,0x88,0xFB } },
		{  8 , { 0x09,0xED,0x55,0xE1,0x3C,0x18,0x46,0x38 } },
		{ 16 , { 0x37,0x05,0xC4,0x09,0x1F,0xFA,0x8C,0x16,0xB1,0x7A,0x9F,0x89,0x0C,0x83,0x60,0xC1 } },
		{ 16 , { 0x3F,0xCA,0xCD,0x6E,0xEB,0xBD,0x49,0x97,0x1E,0x35,0x12,0x88,0x5C,0xE6,0xF7,0x98 } },
		{ 16 , { 0x3F,0xE8,0x2B,0x09,0x4D,0x20,0xFC,0x5D,0xE2,0xCA,0xF2,0x49,0xA5,0x9D,0x31,0x8A } },
		{ 16 , { 0x37,0x8A,0xFA,0x1F,0x45,0x41,0xD7,0x58,0x12,0x19,0x85,0x00,0x44,0xB2,0x25,0x65 } },
		{ 16 , { 0x37,0xEA,0xCE,0xAE,0x51,0xAB,0xC9,0x70,0x4D,0x62,0x90,0x29,0xDC,0x1D,0x09,0xCB } },
		{ 16 , { 0x37,0xF9,0x1F,0x54,0x3F,0x6D,0x3F,0x22,0x31,0x72,0x0C,0xC8,0x5E,0x4E,0x91,0x34 } },
		{ 16 , { 0x37,0x4E,0x62,0xC7,0x66,0xBB,0xD0,0xE2,0xB9,0x78,0x61,0x91,0x8C,0xA8,0x5E,0x17 } },
		{  8 , { 0x4F,0xC4,0xC9,0x92,0x59,0xEE,0x5D,0x5B } },
		{  8 , { 0x09,0xCA,0xF6,0xD7,0xCD,0xD5,0x3D,0x43 } },
		{ 16 , { 0x37,0x52,0xF3,0xD4,0x4E,0x2D,0xE6,0xB0,0xBD,0xDC,0x4C,0x35,0x68,0x16,0x03,0x93 } },
		{ 16 , { 0x3F,0xDD,0xA8,0x12,0xD5,0x79,0x45,0x67,0x26,0x0C,0x51,0x79,0x0B,0xA2,0x54,0x2F } },
		{ 16 , { 0x3F,0xA7,0x56,0x10,0x6F,0x24,0x28,0xE4,0x00,0x5A,0x49,0xD0,0xB9,0x2A,0x06,0x1F } },
		{ 16 , { 0x3F,0x79,0x0E,0x2E,0x83,0x73,0xA0,0x83,0xDA,0xD0,0x10,0x7B,0x83,0xFC,0x67,0x6F } },
		{  5 , { 0x44,0xCF,0xA9,0x50,0x91 } },
		{  8 , { 0x09,0xA6,0xFB,0x86,0xC8,0x29,0x89,0x28 } },
		{ 13 , { 0x34,0xDC,0x56,0x07,0x57,0x9F,0xC3,0xFB,0x41,0xEB,0xE6,0x4D,0x34 } },
		{  8 , { 0x09,0x9D,0x4C,0xA5,0x30,0x55,0x5D,0xD9 } },
		{ 13 , { 0x3C,0x4B,0x6F,0x76,0x77,0xC0,0xF8,0x43,0xCF,0x4D,0x1A,0x2E,0x30 } },
		{  8 , { 0x08,0xE3,0x0A,0x9D,0x98,0xE8,0x25,0xCF } },
		{ 13 , { 0x3C,0xB9,0x8E,0x93,0xFD,0x5C,0xF3,0xB9,0xB9,0xD9,0x0E,0x17,0xC8 } },
		{  8 , { 0x08,0xE5,0xDC,0xB4,0xB5,0x0F,0x98,0x17 } },
		{ 16 , { 0x3F,0xBB,0x57,0x3C,0x39,0xBD,0x08,0x95,0x38,0x50,0xEE,0x51,0xB5,0xC1,0xB7,0xC7 } },
		{ 16 , { 0x3F,0xF7,0x40,0xEC,0x32,0x6A,0xDA,0x10,0x53,0xFE,0x61,0x7F,0x83,0xC7,0xE8,0xC1 } },
		{ 16 , { 0x3F,0xEB,0xB3,0x78,0x08,0xDC,0xA8,0x60,0x81,0xC0,0x8C,0xDF,0xD8,0x5A,0xC8,0xF7 } },
		{ 12 , { 0x33,0xC8,0xFD,0x05,0xEB,0x76,0xEC,0x8E,0x5F,0x25,0xF7,0x6D } },
		{  8 , { 0x08,0x70,0x26,0xB9,0x7F,0xFA,0x22,0xEB } },
		{  9 , { 0x30,0x6C,0xDA,0xCD,0xA7,0x86,0x50,0xFD,0x26 } },
		{  8 , { 0x08,0x08,0xE0,0xE6,0x21,0xE8,0xCC,0x8C } },
		{ 16 , { 0x37,0x90,0x95,0x4A,0x38,0x84,0x29,0x10,0xF5,0x0C,0x08,0xD3,0xD8,0x2F,0x82,0xE9 } },
		{ 16 , { 0x3F,0x1D,0x05,0x60,0xBD,0x0F,0x8C,0x27,0xD9,0xB6,0xC6,0x8B,0x6F,0x28,0x1C,0x13 } },
		{ 15 , { 0x3E,0x37,0xB5,0x9C,0x38,0xD3,0xA1,0x12,0xEC,0xBA,0xF8,0x70,0x53,0xCF,0x16 } },
		{  8 , { 0x09,0x48,0x57,0x7A,0x00,0xE5,0x9C,0xDA } },
		{ 13 , { 0x34,0x9D,0xDC,0xEE,0x29,0x17,0x13,0x9D,0xE8,0x9E,0xBB,0x00,0xB0 } },
		{  8 , { 0x08,0xA7,0x36,0xE1,0x6B,0x48,0xD0,0x34 } },
		{ 13 , { 0x34,0xE7,0xDA,0x19,0xD3,0x7E,0x72,0xFE,0x90,0xAE,0xFD,0xEB,0x10 } },
		{  9 , { 0x38,0x70,0x17,0xCB,0x60,0xD3,0x2D,0x8C,0xEE } },
		{ 13 , { 0x3C,0x9F,0xEF,0xD3,0x42,0x6D,0x9A,0x4C,0x2E,0x5F,0x71,0x5C,0xDB } },
		{  8 , { 0x09,0x9F,0xA6,0x25,0x1A,0xF9,0x8E,0x5C } },
		{ 16 , { 0x37,0x86,0x9D,0xE9,0xB0,0x5B,0xFF,0xED,0x54,0x35,0x35,0xD6,0x53,0x83,0xE5,0x27 } },
		{ 16 , { 0x3F,0x49,0x92,0x79,0x9A,0xFB,0x31,0x3D,0x52,0x27,0xE4,0x03,0x82,0x29,0x2F,0xAE } },
		{ 11 , { 0x3A,0x67,0xBE,0x11,0x4D,0xD7,0x54,0x69,0x39,0x4C,0xA5 } },
		{  8 , { 0x08,0x23,0x53,0xF8,0xAD,0x1A,0x16,0x32 } },
		{ 16 , { 0x37,0xF1,0x60,0x33,0x96,0x65,0x9C,0xFA,0x0D,0xFE,0xB2,0x8B,0x23,0xC1,0x8E,0x71 } },
		{ 16 , { 0x37,0xD9,0x47,0xF3,0x29,0x7D,0x5F,0x45,0xDF,0x1C,0x06,0x48,0x9F,0x46,0x0E,0x92 } },
		{ 15 , { 0x3E,0xCF,0x0C,0xA9,0xEC,0xC7,0x04,0x32,0xE1,0x45,0xC6,0x4D,0x41,0xF8,0x4A } },
		{  8 , { 0x09,0x07,0x12,0x20,0x49,0x55,0x4D,0x7F } },
		{  8 , { 0x09,0xBE,0xB6,0x6E,0x4F,0x4B,0x6C,0x81 } },
		{ 16 , { 0x37,0xCD,0x51,0x38,0x9B,0x79,0x7C,0x51,0x03,0x5A,0x36,0xDF,0x82,0x19,0x26,0x72 } },
		{ 16 , { 0x37,0x33,0x0E,0xC1,0xE0,0xDD,0xE4,0x55,0x2A,0xD2,0xDE,0x6E,0x80,0x61,0x61,0x23 } },
		{  3 , { 0x42,0x6B,0x73 } },
		{  8 , { 0x08,0x46,0x0D,0x86,0x64,0xF1,0x19,0x6B } },
		{  5 , { 0x44,0xC1,0x08,0xE4,0x0C } },
		{  8 , { 0x09,0x01,0x11,0x85,0x50,0xA6,0x93,0xFA } },
		{  9 , { 0x30,0xDB,0xA2,0xC0,0x32,0x71,0x01,0x0E,0x74 } },
		{  8 , { 0x08,0x88,0x1D,0x53,0x31,0x2D,0xC7,0xDB } },
		{  5 , { 0x44,0xB7,0x51,0x3C,0xA1 } },
		{  8 , { 0x08,0xA6,0x93,0xF0,0xCD,0x5C,0x3B,0x7B } },
		{ 16 , { 0x3F,0x17,0x48,0x96,0x2D,0x0A,0xC3,0x14,0x70,0xD6,0xEC,0x24,0x71,0xDB,0xDB,0x82 } },
		{  2 , { 0x49,0x21 } },
		{  8 , { 0x09,0x57,0xDB,0xE1,0x20,0xFB,0x35,0x57 } },
		{  9 , { 0x30,0x5C,0xA6,0xAD,0x48,0xB4,0x01,0xBB,0xAA } },
		{  8 , { 0x08,0x24,0xC6,0xCA,0x88,0x05,0xEB,0x0D } },
		{  8 , { 0x09,0xFB,0x99,0x0D,0x52,0xC8,0x71,0x4B } },
		{  8 , { 0x09,0x98,0x83,0x4B,0x9C,0x41,0x93,0x5C } },
		{  8 , { 0x09,0xC0,0xA7,0x87,0xA8,0x67,0x51,0x2E } },
		{  8 , { 0x09,0x94,0x08,0xC0,0x40,0x10,0x05,0xF7 } },
		{ 16 , { 0x37,0xE7,0x85,0xBC,0x24,0xF5,0x61,0x3C,0xBE,0x31,0xC1,0x83,0xB7,0x79,0x7B,0x43 } },
		{ 14 , { 0x3D,0x23,0xB3,0x28,0x54,0x4C,0x71,0x5C,0x0C,0x0D,0x21,0x54,0x8D,0x0C } },
		{  8 , { 0x08,0xD2,0x3A,0x3D,0x3E,0xC0,0xF8,0xB9 } },
		{  5 , { 0x4C,0x67,0x23,0xFD,0x70 } },
		{  8 , { 0x08,0xC5,0x5F,0x78,0xAD,0x9A,0xF8,0x67 } },
		{ 13 , { 0x34,0x4F,0x69,0x06,0x88,0x5D,0x7C,0xDA,0x73,0xA6,0x17,0xEA,0x2C } },
		{  8 , { 0x08,0xDF,0xB2,0x29,0x5C,0x3B,0x23,0x37 } },
		{ 16 , { 0x3F,0x4C,0x6B,0xB5,0x32,0xB7,0x53,0x99,0xA8,0x80,0x84,0xE8,0xE9,0x71,0x64,0x53 } },
		{ 16 , { 0x3F,0x71,0xDB,0x81,0xBE,0x53,0xA7,0xF2,0x57,0xF1,0x54,0xBA,0x14,0x1C,0x42,0x10 } },
		{ 16 , { 0x3F,0x59,0x82,0xFE,0xCE,0xBE,0x25,0x1E,0x29,0x28,0x99,0x74,0x42,0xBA,0x9D,0x14 } },
		{ 12 , { 0x33,0x06,0xB7,0x70,0xF5,0x3A,0x89,0x4F,0x9C,0x0B,0xAE,0xA8 } },
		{  8 , { 0x08,0xD9,0x9A,0x8A,0x8C,0x3C,0x6A,0xB5 } },
		{  8 , { 0x09,0x34,0x15,0x18,0xBE,0xC4,0x27,0x0A } },
		{  9 , { 0x38,0x7A,0x10,0x24,0x6F,0x03,0x1C,0xB8,0xF2 } },
		{  8 , { 0x09,0x49,0x91,0xB8,0x48,0x5E,0x8D,0xEF } },
		{ 16 , { 0x37,0xDB,0xB7,0x21,0xA1,0x45,0x9A,0xC3,0x91,0x09,0xE5,0xD4,0xC4,0x67,0x81,0xE9 } },
		{ 16 , { 0x37,0x54,0xE4,0x9E,0x5E,0x2D,0x85,0xF0,0x89,0x6C,0xFC,0xBF,0x26,0x4E,0xC1,0xD2 } },
		{  7 , { 0x46,0x94,0xC0,0x9A,0xCC,0x58,0x8D } },
		{  8 , { 0x09,0x2F,0x2B,0xD6,0xC3,0xCC,0x48,0x83 } },
		{  9 , { 0x38,0x96,0x61,0xE9,0x98,0xB3,0x44,0x12,0xE6 } },
		{  8 , { 0x08,0x82,0xE4,0x8D,0x30,0xDA,0xDE,0x5B } },
		{  9 , { 0x30,0x4F,0xE9,0x03,0xFF,0x9B,0x20,0x0B,0xD8 } },
		{  8 , { 0x09,0x81,0x4B,0x22,0xD3,0xE2,0xA5,0x2E } },
		{ 16 , { 0x37,0xDC,0xF7,0x8D,0xFA,0xC8,0xBE,0x3D,0x7D,0xBC,0x2E,0x62,0x20,0x17,0x87,0x1B } },
		{  2 , { 0x41,0x64 } },
		{  8 , { 0x08,0xE8,0x9F,0xDE,0x19,0xCD,0x78,0xEE } },
		{  9 , { 0x38,0x42,0xAA,0xBE,0x44,0xAC,0xFA,0x33,0x78 } },
		{  8 , { 0x08,0xD7,0x97,0x0C,0x6C,0xF2,0xEC,0xE7 } },
		{  5 , { 0x44,0x3E,0x3D,0x8F,0x96 } },
		{  8 , { 0x09,0x36,0x0E,0xA2,0xB8,0xF8,0x01,0x1C } },
		{  8 , { 0x09,0xA3,0xA3,0xD2,0x53,0x5F,0xD4,0xD0 } },
		{ 16 , { 0x3F,0xFA,0xD0,0xBF,0xD6,0x21,0xB4,0x67,0xA0,0xCC,0x51,0xDC,0xB5,0x7D,0xFD,0xBE } },
		{  2 , { 0x49,0x20 } },
		{  8 , { 0x08,0xC1,0xE2,0x4D,0xF4,0x56,0xAD,0xAD } },
		{  8 , { 0x08,0x12,0xDF,0x04,0xA3,0x1B,0xC2,0x81 } },
		{ 16 , { 0x3F,0x83,0xD3,0x97,0x48,0xDA,0xE9,0x2C,0x2C,0xE4,0x24,0x5B,0x95,0x4F,0x9E,0x12 } },
		{  6 , { 0x45,0x39,0x9F,0x86,0xF8,0x31 } },
		{  8 , { 0x09,0x23,0x5A,0xF5,0xB1,0x91,0x9D,0x82 } },
		{  9 , { 0x38,0x3D,0xBD,0xED,0x9C,0xC7,0x98,0x22,0x0E } },
		{  8 , { 0x08,0xE1,0x7B,0x7B,0xB5,0x9B,0xFA,0xFC } },
		{  5 , { 0x4C,0x01,0x2D,0x76,0x1C } },
		{  8 , { 0x08,0xFC,0x48,0x5D,0x0A,0x61,0x17,0xD7 } },
		{  8 , { 0x08,0x6E,0xB0,0x81,0x3C,0xC9,0x2D,0x77 } },
		{  5 , { 0x44,0xDA,0x56,0x75,0xB0 } },
		{  8 , { 0x08,0x4F,0xDF,0x0C,0xC0,0x6F,0x32,0xBA } },
		{  5 , { 0x4C,0xB5,0x3F,0xB7,0xDC } },
		{  8 , { 0x08,0x06,0xF3,0xAD,0x9A,0x4B,0x73,0x1F } },
		{  8 , { 0x0C,0x8E,0x0E,0x1C,0x6A,0x87,0x89,0x05 } },
		{  9 , { 0x38,0xF4,0x5C,0x97,0x8B,0x78,0x77,0x75,0xAD } },
		{  8 , { 0x0C,0xB1,0x6E,0x1E,0x67,0x04,0x01,0x6E } },
};

#define Si2168B_PATCH16_4_0b9_LINES (sizeof(Si2168B_PATCH16_4_0b9)/(sizeof(firmware_struct)))

#endif /* __SI2168B_PRIV_H__ */
