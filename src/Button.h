#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <Arduino.h>

// button status
typedef enum
{
    BTN_STATUS_UNKNOWN = 0,
    BTN_STATUS_RELEASED,
    BTN_STATUS_PRESSED,
    BTN_STATUS_HOLD,

    BTN_STATUS__LAST_ENUM
} btn_status_t;

// button state
typedef enum
{
    BTN_STATE_UNKNOWN = 0,
    BTN_STATE_RELEASED,
    BTN_STATE_PRESSED,

    BTN_STATE__LAST_ENUM
} btn_state_t;

// button object
typedef struct
{
    uint8_t pinNo;
    uint8_t pinActiveLevel;
    btn_state_t state;
    btn_status_t status;
    unsigned long timeActiveStart;
    unsigned long timeDebounceStart;
} btn_t;

// FUNCTIONS

int buttonNew(btn_t *buttonObj, uint8_t pin, uint8_t mode, uint8_t activeLevel);
btn_status_t buttonGetStatus(btn_t *buttonObj);

#endif // _BUTTON_H_
