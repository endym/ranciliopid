#ifndef _RANCILIO_PID_H_
#define _RANCILIO_PID_H_

#include <stdint.h>



/******************************************************************************
 * DEFINES
 ******************************************************************************/




/******************************************************************************
 * TYPEDEFS
 ******************************************************************************/

//---system parameter...--------------------------------------------------------
typedef enum
{
  // USED AS INDEX! DO NOT CHANGE SEQUENCE! ALWAYS APPEND NEW PARAMETERS!
  SYS_PARAM_PID_KP_REGULAR = 0,                                                 // PID P part at regular operation
  SYS_PARAM_PID_TN_REGULAR,                                                     // PID I part at regular operation
  SYS_PARAM_PID_TV_REGULAR,                                                     // PID D part at regular operation
  SYS_PARAM_BREW_SETPOINT,                                                      // brew setpoint
  SYS_PARAM_BREW_TIME,                                                          // brew time
  SYS_PARAM_PRE_INFUSION_TIME,                                                  // pre-infusion time
  SYS_PARAM_PRE_INFUSION_PAUSE,                                                 // pre-infusion pause
  SYS_PARAM_PID_KP_BD,                                                          // PID P part at brew detection phase
  SYS_PARAM_PID_TN_BD,                                                          // PID I part at brew detection phase
  SYS_PARAM_PID_TV_BD,                                                          // PID D part at brew detection phase
  SYS_PARAM_BREW_SW_TIMER,                                                      // brew software timer
  SYS_PARAM_BD_THRESHOLD,                                                       // brew detection limit
  SYS_PARAM_PID_KP_START,                                                       // PID P part at cold start phase
  SYS_PARAM_PID_TN_START,                                                       // PID I part at cold start phase
  SYS_PARAM_PID_TV_START,                                                       // PID D part at cold start phase
  SYS_PARAM_STEAM_SETPOINT,                                                     // steam setpoint
      // <-- Add new parameters here and update 'sysParamInfo' accordingly!

  SYS_PARAM__LAST_ENUM                                                          // must be the last one!
}sys_param_type_t;

typedef struct
{
  double cur;                                                                   // min. value
  double min;                                                                   // min. value
  double max;                                                                   // max. value
  double def;                                                                   // default value
  const char *mqttName;                                                         // MQTT name
}sys_param_t;



/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

const char *getFwVersion(void);
const char *getMachineName(enum MACHINE id);
int getSysParam(sys_param_type_t paramType, sys_param_t *paramObj);
int readSysParamsFromStorage(void);
int setSysParam(sys_param_type_t paramType, double currentValue);
int writeSysParamsToStorage(void);


#endif // _RANCILIO_PID_H_
