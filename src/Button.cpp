/**
 * @file Button.cpp
 *
 * @brief Simple button handling
 */

#include <string.h>
#include "Button.h"

// *** DEFINES ***

#define DEBOUNCE_DELAY_MS 20
#define HOLD_DELAY_MS 600

/**
 * \brief Initializes a new button object.
 *
 * \param buttonObj   - button object to initialize
 * \param pin         - pin number
 * \param mode        - pin mode
 * \param activeLevel - pin active level (LOW/HIGH)
 *
 * \return  0 - succeed
 *         <0 - failed
 */
int buttonNew(btn_t *buttonObj, uint8_t pin, uint8_t mode, uint8_t activeLevel)
{
    // sanity checks...
    if (!buttonObj ||                                    // no object or
        ((activeLevel != LOW) && (activeLevel != HIGH))) // invalid active level?
    {                                                    // yes, error...
        return BTN_STATUS_UNKNOWN;
    }

    memset(buttonObj, 0, sizeof(*buttonObj));
    pinMode(pin, mode);
    buttonObj->pinNo = pin;
    buttonObj->pinActiveLevel = activeLevel;
    buttonObj->state = BTN_STATE_RELEASED;
    buttonObj->status = BTN_STATUS_RELEASED;
    return 0;
}

/**
 * \brief Returns the status of given button.
 *
 * \param buttonObj   - button object to initialize
 *
 * \return button status
 */
btn_status_t buttonGetStatus(btn_t *buttonObj)
{
    bool isPinActive;

    // sanity checks...
    if (!buttonObj)
        return BTN_STATUS_UNKNOWN;

    // determine button status...
    isPinActive = (digitalRead(buttonObj->pinNo) == buttonObj->pinActiveLevel);
    switch (buttonObj->state)
    {
    //---released---------------------------------------------------------------
    case BTN_STATE_RELEASED:
    {
        if (isPinActive)                           // pin active?
        {                                          // yes...
            if (buttonObj->timeDebounceStart == 0) // debouncing started?
            {                                      // no, start debounce phase...
                buttonObj->timeDebounceStart = millis();
                buttonObj->timeActiveStart = buttonObj->timeDebounceStart;
            }
            else if (millis() >= buttonObj->timeDebounceStart + DEBOUNCE_DELAY_MS) // debounce phase finished?
            {                                                                      // yes, button is pressed...
                buttonObj->timeDebounceStart = 0;
                buttonObj->state = BTN_STATE_PRESSED;
                buttonObj->status = BTN_STATUS_PRESSED;
            }
        }
        else
        {                                     // no, pin is inactive...
            buttonObj->timeDebounceStart = 0; // abort debounce phase
            buttonObj->timeActiveStart = 0;
        }
        break;
    }

    //---pressed----------------------------------------------------------------
    case BTN_STATE_PRESSED:
    {
        if (!isPinActive)                          // pin inactive?
        {                                          // yes...
            if (buttonObj->timeDebounceStart == 0) // debouncing started?
            {                                      // no, start debounce phase...
                buttonObj->timeDebounceStart = millis();
            }
            else if (millis() >= buttonObj->timeDebounceStart + DEBOUNCE_DELAY_MS) // debounce phase finished?
            {                                                                      // yes, button is released...
                buttonObj->timeDebounceStart = 0;
                buttonObj->timeActiveStart = 0;
                buttonObj->state = BTN_STATE_RELEASED;
                buttonObj->status = BTN_STATUS_RELEASED;
            }
        }
        else
        {
            buttonObj->timeDebounceStart = 0;                           // abort debounce phase
            if (millis() >= buttonObj->timeActiveStart + HOLD_DELAY_MS) // "hold" status?
            {                                                           // yes, button is pressed...
                buttonObj->status = BTN_STATUS_HOLD;
            }
        }
        break;
    }

    //---unknow state (error)---------------------------------------------------
    default:
        buttonObj->state = BTN_STATE_RELEASED;
        buttonObj->status = BTN_STATUS_RELEASED;
        break;
    }

    return buttonObj->status;
}
