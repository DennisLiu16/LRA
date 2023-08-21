/*
 * File: lra_usb_defines.cc
 * Created Date: 2023-07-06
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Saturday July 8th 2023 8:28:31 am
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include "lra_usb_defines.h"

namespace lra::usb_lib {
std::string rcws_msg_init = "MFIL-RCWS-USB init\r\n";
std::string rcws_msg_eop = "\r\n";

/* command map */
std::map<LRA_Device_Index_t, std::string> modify_rcws_device_index_map = {
    {LRA_DEVICE_MPU6500, "LRA_DEVICE_MPU6500"},
    {LRA_DEVICE_ADXL355, "LRA_DEVICE_ADXL355"},
    {LRA_DEVICE_DRV2605L_X, "LRA_DEVICE_DRV2605L_X"},
    {LRA_DEVICE_DRV2605L_Y, "LRA_DEVICE_DRV2605L_Y"},
    {LRA_DEVICE_DRV2605L_Z, "LRA_DEVICE_DRV2605L_Z"}};

std::map<LRA_Device_Index_t, std::string> reset_rcws_device_index_map = {
    {LRA_DEVICE_STM32, "LRA_DEVICE_STM32"},
    {LRA_DEVICE_MPU6500, "LRA_DEVICE_MPU6500"},
    {LRA_DEVICE_ADXL355, "LRA_DEVICE_ADXL355"},
    {LRA_DEVICE_DRV2605L_X, "LRA_DEVICE_DRV2605L_X"},
    {LRA_DEVICE_DRV2605L_Y, "LRA_DEVICE_DRV2605L_Y"},
    {LRA_DEVICE_DRV2605L_Z, "LRA_DEVICE_DRV2605L_Z"},
    {LRA_DEVICE_ALL, "LRA_DEVICE_ALL"}};

std::map<LRA_USB_Mode_t, std::string> usb_mode_map = {
    {LRA_USB_NONE_MODE, "LRA_USB_NONE_MODE"},
    {LRA_USB_WAIT_FOR_INIT_MODE, "LRA_USB_WAIT_FOR_INIT_MODE"},
    {LRA_USB_CRTL_MODE, "LRA_USB_CRTL_MODE"},
    {LRA_USB_DATA_MODE, "LRA_USB_DATA_MODE"}};

std::map<int, std::string> pwm_cmd_mode_map = {
    {RCWS_PWM_MANUAL_MODE, "RCWS_PWM_MANUAL_MODE"},
    {RCWS_PWM_FILE_MODE, "RCWS_PWM_FILE_MODE"}};

std::map<LRA_USB_IN_Cmd_t, std::string> usb_in_cmd_type_map = {
    {USB_IN_CMD_INIT, "USB_IN_CMD_INIT"},
    {USB_IN_CMD_SYS_INFO, "USB_IN_CMD_SYS_INFO"},
    {USB_IN_CMD_PARSE_ERR, "USB_IN_CMD_PARSE_ERR"},
    {USB_IN_CMD_GET_REG, "USB_IN_CMD_GET_REG"},
    {USB_IN_CMD_RESET_DEVICE, "USB_IN_CMD_RESET_DEVICE"},
    {USB_IN_CMD_SWITCH_MODE, "USB_IN_CMD_SWITCH_MODE"}};

std::map<LRA_USB_Cmd_Description_t, std::string> usb_basic_cmd_type_map = {
    {CMD_SYS_INFO, "CMD_SYS_INFO"},
    {CMD_PARSE_ERR, "CMD_PARSE_ERR"},
    {CMD_SWITCH_MODE, "CMD_SWITCH_MODE"},
    {CMD_UPDATE_REG, "CMD_UPDATE_REG"},
    {CMD_GET_REG, "CMD_GET_REG"},
    {CMD_RESET_DEVICE, "CMD_RESET_DEVICE"},
    {CMD_RUN_AUTOCALIBRATE, "CMD_RUN_AUTOCALIBRATE"},
    {CMD_UPDATE_PWM, "CMD_UPDATE_PWM"},
    {CMD_UPDATE_ACC, "CMD_UPDATE_ACC"}};

/* RCWS error map */
std::map<LRA_USB_Parse_State_t, std::string> rcws_error_state_map = {
    {PR_PRECHECK_FAIL, "PR_PRECHECK_FAIL"},
    {PR_CURRENT_MODE_FORBIDDEN_FAIL, "PR_CURRENT_MODE_FORBIDDEN_FAIL"},
    {PR_RETURN_MSG_TX_UNSET_FAIL, "PR_RETURN_MSG_TX_UNSET_FAIL"},
    {PR_DATA_PARSE_CONTENT_FAIL, "PR_DATA_PARSE_CONTENT_FAIL"},
    {PR_INTERNAL_OPERATION_FAIL, "PR_INTERNAL_OPERATION_FAIL"}};
};  // namespace lra::usb_lib