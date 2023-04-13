#ifndef LRA_USB_DEFINES
#define LRA_USB_DEFINES
#include <cstdint>
/*************/
/* lra_usb.h */
/*************/

typedef enum {

  LRA_USB_NONE_MODE = 0x00,

  LRA_USB_WAIT_FOR_INIT_MODE = 0x01,

  /**
   * @brief 當 usb 處於此模式，允許修改內部所有的狀態，對 Rasp
   * 的傳輸就變成一問一答，不會主動傳輸加速度等資訊給 Rasp，應該以
   * LRA_Modify_USB_Mode() 修改
   */
  LRA_USB_CRTL_MODE = 0x02,
  /**
   * @brief 當 usb 處於此模式，僅供大量傳輸內部狀態給 Rasp而不允許從 Rasp
   * 的設備修改指令，允許的 Rasp 指令僅有切換模式指令和 PWM 更新指令，應該以
   * LRA_Modify_USB_Mode() 修改
   */
  LRA_USB_DATA_MODE = 0x03,
  LRA_USB_ALL_AVAILABLE_MODE,
} LRA_USB_Mode_t;

typedef enum { LRA_FLAG_UNSET, LRA_FLAG_SET } LRA_Flag_t;

/**
 * PC: Precheck
 * OOB: Out of bounds.
 * EOP: End of package
 */
typedef enum {
  PC_ALL_PASS,
  PC_CMD_UNKNOWN,
  PC_RX_UNSET,
  PC_DATA_PTR_IS_NULL,
  PC_DATA_LEN0,
  PC_DATA_LEN_MISMATCH,
  PC_DATA_LEN_OOB,
  PC_EOP_NOT_FOUND,
  PC_MSG_NOT_FROM_OUT,
} LRA_USB_Parse_Precheck_t;

/**
 * @brief A uint16_t value, high byte is error descriptor byte,
 *        low byte is cmd_type byte (defined in LRA_USB_IN_Cmd_t). If the parser
 *        precheck doens't pass, the return value should be PR_PRECHECK_FAIL.
 *        Otherwise, the MSB of the high byte of the return value show
 *        the parser procedure encounter an error or not. The low byte
 *        is the value of rx cmd_type(see LRA_USB_IN_Cmd_t).
 * @details
 * The return value of Main_Parser can be seperated into H byte
 * (LRA_USB_Parse_State_t) 和 L byte (LRA_USB_IN_Cmd_t) & (LRA_USB_OUT_Cmd_t)
 *
 * L byte _ _ _ _____
 *        | | |   |__ BASIC_CMD_TYPE
 *        | | |__ NO REPLY bit
 *        | |__ DATA CONST bit
 *        |__ OUT bit
 *
 *
 * H byte _ _ _ _____
 *        | | |   |__ Reserved
 *        | | |__ PRECHECK FAIL bit
 *        | |__ DATA PARSE FAIL bit
 *        |__ R TX UNSET FAIL bit
 *
 *              H   L
 * 1. retval == 0 | 0, no msg should be parserd
 * 2. retval == PR_PRECHECK_FAIL | 0, precheck failed with unknown cmd_type
 * 3. retval == PR_PRECHECK_FAIL | no 0, precheck failed with specific command
 * type
 * 4. retval == PR_RETURN_MSG_TX_UNSET_FAIL | no 0, tx_data_buf or
 * tx_data_buf_len not set fail
 * 5. retval == PR_DATA_PARSE_FORMAT_FAIL | no 0, some error happened
 */
typedef enum {
  PR_RX_UNSET = (uint16_t)0,
  PR_PRECHECK_FAIL = ((1 << 3) << 8),
  PR_CURRENT_MODE_FORBIDDEN_FAIL = ((1 << 4) << 8),
  PR_RETURN_MSG_TX_UNSET_FAIL = ((1 << 5) << 8),
  PR_DATA_PARSE_CONTENT_FAIL = ((1 << 6) << 8),
  PR_INTERNAL_OPERATION_FAIL = ((1 << 7) << 8),
} LRA_USB_Parse_State_t;

/**
 * Brief description of the register
 *
 * Bit fields:
 * Bit 7: OUT - Description of OUT bit
 * Bit 6: DATA_CONST - Description of DATA_CONST bit
 * Bit 5: NO_REPLY - Description of NO_REPLY bit
 * Bit 4-0: CMD_TYPE - Description of CMD_TYPE bits
 *
 * Update this if you add new cmd_type
 */
typedef enum {
  CMD_IS_OUT = 1 << 7,

  CMD_DATA_LEN_CONST = 1 << 6,

  CMD_NR = 1 << 5,

  /* CMD mask, for cmd_type tuning */
  BASIC_CMD_MASK = 0b11111,
  NO_OUT_CMD_MASK = (uint8_t) ~(1 << 7),

  /* CMD_TYPE */
  CMD_UNKNOWN_L = 0,
  /*------------------All available----------------------*/
  CMD_SYS_INFO = CMD_UNKNOWN_L + 1,
  CMD_PARSE_ERR,
  CMD_SWITCH_MODE,
  /*------------------WAIT_FOR_INIT----------------------*/
  CMD_INIT,
  /*-----------------------CRTL--------------------------*/
  CMD_UPDATE_REG,
  CMD_GET_REG,
  CMD_RESET_DEVICE,
  CMD_RUN_AUTOCALIBRATE,
  /*-----------------------DATA--------------------------*/
  CMD_UPDATE_ARG,
  CMD_UPDATE_ACC,

  // DO NOT TOUCH IT!
  CMD_UNKNOWN_H
} LRA_USB_Cmd_Description_t;

/**
 * @brief it's a uint8_t number
 *
 * @warning update LRA_USB_Parse_Precheck() and LRA_USB_Main_Parser() once you
 * add a new cmd. Use this enum only at LRA_USB_Msg_Generate()
 *
 * Update this if you add new cmd_type
 */
typedef enum {
  /*------------------All available----------------------*/
  USB_IN_CMD_SYS_INFO = CMD_NR | CMD_SYS_INFO,
  USB_IN_CMD_PARSE_ERR = CMD_NR | CMD_PARSE_ERR,
  USB_IN_CMD_SWITCH_MODE = CMD_DATA_LEN_CONST | CMD_SWITCH_MODE,
  /*------------------WAIT_FOR_INIT----------------------*/
  USB_IN_CMD_INIT = CMD_DATA_LEN_CONST | CMD_INIT,
  /*-----------------------CRTL--------------------------*/
  USB_IN_CMD_UPDATE_REG = CMD_UPDATE_REG,
  USB_IN_CMD_GET_REG = CMD_GET_REG,
  USB_IN_CMD_RESET_DEVICE = CMD_DATA_LEN_CONST | CMD_RESET_DEVICE,
  USB_IN_CMD_RUN_AUTOCALIBRATE = CMD_RUN_AUTOCALIBRATE,
  /*-----------------------CRTL--------------------------*/
  USB_IN_CMD_UPDATE_ARG = CMD_UPDATE_ARG,
  USB_IN_CMD_UPDATE_ACC = CMD_NR | CMD_UPDATE_ACC,
} LRA_USB_IN_Cmd_t;

/**
 * Update this if you add new cmd_type
 *
 */
typedef enum {
  /*------------------All available----------------------*/
  USB_OUT_CMD_SYS_INFO = USB_IN_CMD_SYS_INFO | CMD_IS_OUT,
  USB_OUT_CMD_PARSE_ERR = USB_IN_CMD_PARSE_ERR | CMD_IS_OUT,
  USB_OUT_CMD_SWITCH_MODE = USB_IN_CMD_SWITCH_MODE | CMD_IS_OUT,
  /*------------------WAIT_FOR_INIT----------------------*/
  USB_OUT_CMD_INIT = USB_IN_CMD_INIT | CMD_IS_OUT,
  /*-----------------------CRTL--------------------------*/
  USB_OUT_CMD_UPDATE_REG = USB_IN_CMD_UPDATE_REG | CMD_IS_OUT,
  USB_OUT_CMD_GET_REG = USB_IN_CMD_GET_REG | CMD_IS_OUT,
  USB_OUT_CMD_RESET_DEVICE = USB_IN_CMD_RESET_DEVICE | CMD_IS_OUT,
  USB_OUT_CMD_RUN_AUTOCALIBRATE = USB_IN_CMD_RUN_AUTOCALIBRATE | CMD_IS_OUT,
  /*-----------------------CRTL--------------------------*/
  USB_OUT_CMD_UPDATE_ARG = USB_IN_CMD_UPDATE_ARG | CMD_IS_OUT,
  USB_OUT_CMD_UPDATE_ACC = USB_IN_CMD_UPDATE_ACC | CMD_IS_OUT,
} LRA_USB_OUT_Cmd_t;

/**************/
/* lra_main.h */
/**************/
typedef enum {
  LRA_DEVICE_STM32,
  LRA_DEVICE_MPU6500,
  LRA_DEVICE_ADXL355,
  LRA_DEVICE_DRV2605L_X,
  LRA_DEVICE_DRV2605L_Y,
  LRA_DEVICE_DRV2605L_Z,
  LRA_DEVICE_ALL,
  LRA_DEVICE_INVALID
} LRA_Device_Index_t;

extern const char rcws_msg_init[];
extern const char rcws_msg_eop[];

#endif