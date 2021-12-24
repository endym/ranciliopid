/******************************************************************************
 * \brief Simple button handling.
 ******************************************************************************/

#include <Arduino.h>
#include <LCDMenuLib2.h>
#include <U8g2lib.h>
#include "userConfig.h"
#include "Button.h"
#include "DebugStreamManager.h"
#include "rancilio-pid.h"



/******************************************************************************
 * DEFINES
 ******************************************************************************/

// display settings...
#define DISPLAY_FONT              u8g2_font_profont15_tf
#define DISPLAY_FONT_WIDTH        7                                             // font width 
#define DISPLAY_FONT_HEIGHT       15                                            // font heigt 
#define DISPLAY_WIDTH             SCREEN_WIDTH                                  // display width
#define DISPLAY_HEIGHT            SCREEN_HEIGHT                                 // display height
#define DISPLAY_COLS_MAX          (DISPLAY_WIDTH/DISPLAY_FONT_WIDTH)
#define DISPLAY_ROWS_MAX          (DISPLAY_HEIGHT/DISPLAY_FONT_HEIGHT)

// menu settings...
#define LCDML_DISP_COLS           DISPLAY_COLS_MAX
#define LCDML_DISP_ROWS           DISPLAY_ROWS_MAX
#define LCDML_SCROLLBAR_WIDTH     (DISPLAY_FONT_WIDTH-1)                        // scrollbar width 
#define LCDML_TEXT_INDENTING      (DISPLAY_FONT_WIDTH+2)



/******************************************************************************
 * TYPEDEFS
 ******************************************************************************/

// buttons...
typedef enum
{
  // USED AS INDEX!
  BTN_TYPE_LEFT = 0,
  BTN_TYPE_RIGHT,

  BTN_TYPE__LAST_ENUM                                                           // must be the last enum!
}btn_type_t;

// object for dynamic menu parameter handling
typedef struct
{
  sys_param_t sysParam;
  bool isEditMode;
}menu_dyn_param_t;



/******************************************************************************
 * PROTOTYPES
 ******************************************************************************/

static void lcdml_menu_display(void);
static void lcdml_menu_clear(void);
static void lcdml_menu_control(void);
static void cbMenuBack(uint8_t param);
static void cbMenuExit(uint8_t param);
static void cbMenuInformation(uint8_t param);
// static void mFunc_para(uint8_t param);
static void cbMenuDynParaBrewSetPoint(uint8_t line);
static void cbMenuDynParaSteamSetPoint(uint8_t line);



/******************************************************************************
 * VARIABLES
 ******************************************************************************/

extern DebugStreamManager debugStream;

//---buttons...-----------------------------------------------------------------
static btn_t displayButton[BTN_TYPE__LAST_ENUM];
//static btn_t displayButtonLeft;
//static btn_t displayButtonRight;

//---display...-----------------------------------------------------------------
extern LCDMenuLib2 LCDML;
#if DISPLAY == 1
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
#elif DISPLAY == 2
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
#endif

//---menu...--------------------------------------------------------------------
static menu_dyn_param_t dynMenuParam4EditMode;

LCDMenuLib2_menu LCDML_0 (255, 0, 0, NULL, NULL); // root menu element (do not change)
LCDMenuLib2 LCDML(LCDML_0, LCDML_DISP_ROWS, LCDML_DISP_COLS, lcdml_menu_display, lcdml_menu_clear, lcdml_menu_control);

// LCDML_add      (id, prev_layer, new_num, lang_char_array, callback_function)
LCDML_add         (0, LCDML_0,   1, "Information", cbMenuInformation);
// LCDMenuLib_addAdvanced(id, prev_layer, new_num, condition, lang_char_array, callback_function, parameter (0-255), menu function type  )
LCDML_addAdvanced (1, LCDML_0,   2, NULL,   "Settings", NULL,                       0, _LCDML_TYPE_default);
LCDML_addAdvanced (2, LCDML_0_2, 1, NULL,   "",         cbMenuDynParaBrewSetPoint,  0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (3, LCDML_0_2, 2, NULL,   "",         cbMenuDynParaSteamSetPoint, 0, _LCDML_TYPE_dynParam);
LCDML_add         (4, LCDML_0_2, 3, "Back", cbMenuBack);
LCDML_add         (5, LCDML_0,   3, "Exit", cbMenuExit);

// menu element count - last element id
// this value must be the same as the last menu element
#define _LCDML_DISP_cnt    5
// create menu
LCDML_createMenu(_LCDML_DISP_cnt);

static bool isMenuEnabled = false;
static bool isMenuExitEvent = false;



/* ******************************************************************** */
static void lcdml_menu_control(void)
/* ******************************************************************** */
{
  uint btnIdx;
  static btn_status_t currentButtonStatus;
  static btn_status_t lastButtonStatus[BTN_TYPE__LAST_ENUM] = { BTN_STATUS_RELEASED };
  
  if (LCDML.BT_setup())
  {
    // runs only once
  }

  for (btnIdx=0; btnIdx<BTN_TYPE__LAST_ENUM; btnIdx++)
  {
    currentButtonStatus = buttonGetStatus(&displayButton[btnIdx]);
    if (currentButtonStatus != lastButtonStatus[btnIdx])
    {
      switch (currentButtonStatus)
      {
        case BTN_STATUS_PRESSED:
          debugStream.writeI("%s(): button %u pressed", __FUNCTION__, btnIdx);
          // do action only on next release, because it may become "hold" status
          break;
        case BTN_STATUS_HOLD:
          debugStream.writeI("%s(): button %u hold", __FUNCTION__, btnIdx);
          LCDML.BT_enter();
          break;
        case BTN_STATUS_RELEASED:
          debugStream.writeI("%s(): button %u released", __FUNCTION__, btnIdx);
          if (lastButtonStatus[btnIdx] == BTN_STATUS_PRESSED)
          {
            switch ((btn_type_t)btnIdx)
            {
              case BTN_TYPE_LEFT:
                LCDML.BT_up();
                break;
              case BTN_TYPE_RIGHT:
                LCDML.BT_down();
                break;
              default:
                break;
            }
          }
          break;
        default:
          debugStream.writeE("%s(): button %u unknown state", __FUNCTION__, btnIdx);
          break;
      }
      lastButtonStatus[btnIdx] = currentButtonStatus;
    }
  }
}


/* ******************************************************************** */
static void lcdml_menu_clear(void)
/* ******************************************************************** */
{
  debugStream.writeI("%s()", __FUNCTION__);
}

/* ******************************************************************** */
static void lcdml_menu_display(void)
/* ******************************************************************** */
{
  u8g2.setFont(DISPLAY_FONT); // set font

  // clear lcd
  u8g2.clearBuffer();
  
  // declaration of some variables
  // ***************
  // content variable
  char content_text[LCDML_DISP_COLS];  // save the content text of every menu element
  // menu element object
  LCDMenuLib2_menu *tmp;
  // some limit values
  uint8_t i = LCDML.MENU_getScroll();
  uint8_t maxi = LCDML_DISP_ROWS + i;
  uint8_t n = 0;

   // init vars
  uint8_t n_max             = (LCDML.MENU_getChilds() >= LCDML_DISP_ROWS) ? LCDML_DISP_ROWS : (LCDML.MENU_getChilds());

  uint8_t scrollbar_min     = 0;
  uint8_t scrollbar_max     = LCDML.MENU_getChilds();
  uint8_t scrollbar_cur_pos = LCDML.MENU_getCursorPosAbs();
  uint8_t scroll_pos        = ((1.*n_max * LCDML_DISP_ROWS) / (scrollbar_max - 1) * scrollbar_cur_pos);


  n = 0;
  i = LCDML.MENU_getScroll();
  // update content
  // ***************

    // clear menu
    // ***************

  // check if this element has children
  if ((tmp = LCDML.MENU_getDisplayedObj()) != NULL)
  {
    // loop to display lines
    do
    {
      // check if a menu element has a condition and if the condition be true
      if (tmp->checkCondition())
      {
        // check the type off a menu element
        if(tmp->checkType_menu() == true)
        {
          // display normal content
          LCDML_getContent(content_text, tmp->getID());
          u8g2.setCursor(LCDML_TEXT_INDENTING, DISPLAY_FONT_HEIGHT * (n));
          u8g2.println(content_text);
        }
        else
        {
          if(tmp->checkType_dynParam()) {
            tmp->callback(n);
          }
        }
        // increment some values
        i++;
        n++;
      }
    // try to go to the next sibling and check the number of displayed rows
    } while (((tmp = tmp->getSibling(1)) != NULL) && (i < maxi));
  }

  // set cursor
  u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * (LCDML.MENU_getCursorPos()));
  u8g2.println(">");

  // ***** todo *****
  #ifdef _SCROLLBAR_TODO_
  if(_LCDML_DISP_draw_frame == 1) {
     u8g2.drawFrame(_LCDML_DISP_box_x0, _LCDML_DISP_box_y0, (_LCDML_DISP_box_x1-_LCDML_DISP_box_x0), (_LCDML_DISP_box_y1-_LCDML_DISP_box_y0));
  }

  
  // display scrollbar when more content as rows available and with > 2
  
  if (scrollbar_max > n_max && _LCDML_DISP_scrollbar_w > 2)
  {
    // set frame for scrollbar
    //u8g2.drawFrame(_LCDML_DISP_box_x1 - _LCDML_DISP_scrollbar_w, _LCDML_DISP_box_y0, _LCDML_DISP_scrollbar_w, _LCDML_DISP_box_y1-_LCDML_DISP_box_y0);
    u8g2.drawRect(DISPLAY_WIDTH - LCDML_SCROLLBAR_WIDTH, 0,LCDML_SCROLLBAR_WIDTH,DISPLAY_HEIGHT, _LCDML_ADAFRUIT_TEXT_COLOR); 

    // calculate scrollbar length
    uint8_t scrollbar_block_length = scrollbar_max - n_max;
    scrollbar_block_length = (_LCDML_DISP_box_y1-_LCDML_DISP_box_y0) / (scrollbar_block_length + LCDML_DISP_ROWS);

    //set scrollbar
    if (scrollbar_cur_pos == 0) {                                   // top position     (min)
      u8g2.drawBox(_LCDML_DISP_box_x1 - (_LCDML_DISP_scrollbar_w-1), _LCDML_DISP_box_y0 + 1                                                     , (_LCDML_DISP_scrollbar_w-2)  , scrollbar_block_length);
    }
    else if (scrollbar_cur_pos == (scrollbar_max-1)) {            // bottom position  (max)
      u8g2.drawBox(_LCDML_DISP_box_x1 - (_LCDML_DISP_scrollbar_w-1), _LCDML_DISP_box_y1 - scrollbar_block_length                                , (_LCDML_DISP_scrollbar_w-2)  , scrollbar_block_length);
    }
    else {                                                                // between top and bottom
      u8g2.drawBox(_LCDML_DISP_box_x1 - (_LCDML_DISP_scrollbar_w-1), _LCDML_DISP_box_y0 + (scrollbar_block_length * scrollbar_cur_pos + 1),(_LCDML_DISP_scrollbar_w-2)  , scrollbar_block_length);
    }
  }
  #endif

  u8g2.sendBuffer();
}


/**************************************************************************//**
 * \brief Callback for menu "Back" function.
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuBack(uint8_t param)
{
  if(LCDML.FUNC_setup())          // ****** SETUP *********
  {
    // remmove compiler warnings when the param variable is not used:
    LCDML_UNUSED(param);

    // end function and go an layer back
    LCDML.FUNC_goBackToMenu(1);      // leave this function and go a layer back
  }
}


/**************************************************************************//**
 * \brief Callback for menu "Exit" function.
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuExit(uint8_t param)
{
  if(LCDML.FUNC_setup())          // ****** SETUP *********
  {
    // remmove compiler warnings when the param variable is not used:
    LCDML_UNUSED(param);

    // end function and go back to root menu
    debugStream.writeI("%s(): exit display menu", __FUNCTION__);
    // LCDML.MENU_goRoot();
    isMenuExitEvent = true;
  }
}


/**************************************************************************//**
 * \brief Callback for menu "Information".
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuInformation(uint8_t param)
{
  if(LCDML.FUNC_setup())          // ****** SETUP *********
  {
    // remmove compiler warnings when the param variable is not used:
    LCDML_UNUSED(param);

    // setup function
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.clearBuffer();
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 0); // line 0
    u8g2.println(getFwVersion());
    #if 0
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 1); // line 1
    u8g2.println();
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 2); // line 2
    u8g2.println();
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 3); // line 3
    u8g2.println();
    #endif
    u8g2.sendBuffer();
    u8g2.setFont(DISPLAY_FONT);
  }

  if (LCDML.FUNC_loop())           // ****** LOOP *********
  {
    // loop function, can be run in a loop when LCDML_DISP_triggerMenu(xx) is set
    // the quit button works in every DISP function without any checks; it starts the loop_end function
    if(LCDML.BT_checkAny()) // check if any button is pressed (enter, up, down, left, right)
    {
      // LCDML_goToMenu stops a running menu function and goes to the menu
      LCDML.FUNC_goBackToMenu();
    }
  }

  if(LCDML.FUNC_close())      // ****** STABLE END *********
  {
    // you can here reset some global vars or do nothing
  }
}


#if 0
// *********************************************************************
static void mFunc_para(uint8_t param)
// *********************************************************************
{
  if(LCDML.FUNC_setup())          // ****** SETUP *********
  {
    char lineText[LCDML_DISP_COLS+1];
    sprintf (lineText, "parameter: %d", param);
    
    // setup function
    u8g2.clearBuffer();
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 0); // line 0
    u8g2.println(lineText);
    #if 0
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 1); // line 1
    u8g2.println(F("press any key"));
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 2); // line 2
    u8g2.println(F("to leave it"));
    #endif
    u8g2.sendBuffer();

    LCDML.FUNC_setLoopInterval(100);  // starts a trigger event for the loop function every 100 milliseconds
  }

  if (LCDML.FUNC_loop())               // ****** LOOP *********
  {
    // For example
    switch (param)
    {
      case PARA_BREW_TEMP:
        // do something
        debugStream.writeI("%s(): param %u", __FUNCTION__, param);
        break;

      case PARA_STEAM_TEMP:
        // do something
        break;

      default:
        // do nothing
        break;
    }


    if (LCDML.BT_checkAny()) // check if any button is pressed (enter, up, down, left, right)
    {
      LCDML.FUNC_goBackToMenu();  // leave this function
    }
  }

  if(LCDML.FUNC_close())      // ****** STABLE END *********
  {
    // you can here reset some global vars or do nothing
  }
}
#endif


/**************************************************************************//**
 * \brief Handles the dynamic menu parameter.
 * 
 * \param paramType   - system parameter type
 * \param sysParamObj - system parameter object
 * \param line        - screen line of this parameter
 ******************************************************************************/
static void handleDynPara(sys_param_type_t paramType, sys_param_t *sysParamObj, uint8_t line)
{
  uint8_t curLine = LCDML.MENU_getCursorPos();
  
  debugStream.writeI("%s(): param %i at line %u", __FUNCTION__, paramType, line);
  
  if (getSysParam(paramType, sysParamObj) != 0)
  {
    debugStream.writeE("%s(): failed to get param %i!", __FUNCTION__, paramType);
    return;
  }
  
  #if 0
  if (!dynMenuParam4EditMode.isEditMode)                                                 // not edit mode?
  {                                                                             // yes...
    if (getSysParam(paramType, &dynMenuParam4EditMode.sysParam) != 0)
    {
      debugStream.writeE("%s(): failed to get param %i!", __FUNCTION__, paramType);
      return;
    }
  }
  #endif

  // check if this function is active (cursor stands on this line)
  if (line == curLine)
  {
    // make only an action when the cursor stands on this menu item
    //check Button
    if (LCDML.BT_checkAny())
    {
      if (LCDML.BT_checkEnter())
      {
        // this function checks returns the scroll disable status (0 = menu scrolling enabled, 1 = menu scrolling disabled)
        if (LCDML.MENU_getScrollDisableStatus() == 0)
        {
          // disable the menu scroll function to catch the cursor on this point
          // now it is possible to work with BT_checkUp and BT_checkDown in this function
          // this function can only be called in a menu, not in a menu function
          LCDML.MENU_disScroll();
          dynMenuParam4EditMode.sysParam = *sysParamObj;
          dynMenuParam4EditMode.isEditMode = true;
        }
        else
        {
          // enable the normal menu scroll function
          LCDML.MENU_enScroll();
          setSysParam(paramType, dynMenuParam4EditMode.sysParam.cur);
          *sysParamObj = dynMenuParam4EditMode.sysParam;
          dynMenuParam4EditMode.isEditMode = false;
        }
      }

      // This check have only an effect when MENU_disScroll is set
      if (LCDML.BT_checkUp())
      {
        if (dynMenuParam4EditMode.sysParam.cur > dynMenuParam4EditMode.sysParam.min)
          dynMenuParam4EditMode.sysParam.cur -= 0.5;
        else
          dynMenuParam4EditMode.sysParam.cur = dynMenuParam4EditMode.sysParam.max;
        *sysParamObj = dynMenuParam4EditMode.sysParam;
      }

      // This check have only an effect when MENU_disScroll is set
      if (LCDML.BT_checkDown())
      {
        if (dynMenuParam4EditMode.sysParam.cur < dynMenuParam4EditMode.sysParam.max)
          dynMenuParam4EditMode.sysParam.cur += 0.5;
        else
          dynMenuParam4EditMode.sysParam.cur = dynMenuParam4EditMode.sysParam.min;
        *sysParamObj = dynMenuParam4EditMode.sysParam;
      }
    }
  }

  if (dynMenuParam4EditMode.isEditMode && (line == curLine))                    // edit mode and edit line?
  {                                                                             // yes, invert line background...
    // clear line with inverted color...
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, line*DISPLAY_FONT_HEIGHT, DISPLAY_WIDTH, DISPLAY_FONT_HEIGHT);
    u8g2.setDrawColor(0);
  }
  else
  {                                                                             // no, normal navigation mode...
    // clear line...
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, line*DISPLAY_FONT_HEIGHT, DISPLAY_WIDTH, DISPLAY_FONT_HEIGHT);
    u8g2.setDrawColor(1);
    #if 0
    // clear the line manuel because clear the complete content is disabled when a external refreshed function is active
    u8g2.setCursor(LCDML_TEXT_INDENTING, line);
    for(uint8_t i=0;i<LCDML_DISP_COLS-3;i++) // -3 because: 
                                              // -1 for counter from 0 to x 
                                              // -1 for cursor position
                                              // -1 for scrollbar on the end
    {
      u8g2.print(F(" "));
    }
    #endif
  }
}


/**************************************************************************//**
 * \brief Callback for setting parameter "brew setpoint".
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaBrewSetPoint(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_DISP_COLS+1];

  handleDynPara(SYS_PARAM_BREW_SETPOINT, &sysParam, line);
  snprintf(lineText, sizeof(lineText), "Brew Temp:   %3.1f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, line * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "steam setpoint".
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaSteamSetPoint(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_DISP_COLS+1];

  handleDynPara(SYS_PARAM_STEAM_SETPOINT, &sysParam, line);
  snprintf(lineText, sizeof(lineText), "Steam Temp: %3.1f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, line * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Checks if all buttons are released.
 * 
 * \return  true - all buttons are released
 *         false - at least one button is pressed
 ******************************************************************************/
static bool areAllButtonsReleased(void)
{
  uint btnIdx;

  for (btnIdx=0; btnIdx<BTN_TYPE__LAST_ENUM; btnIdx++)
  {
    if (buttonGetStatus(&displayButton[btnIdx]) != BTN_STATUS_RELEASED)
      return false;
  }

  return true;
}



/**************************************************************************//**
 * \brief Display menu setup routine.
 ******************************************************************************/
void displaymenuSetup(void)
{
  // initialize display buttons...
  debugStream.writeI("%s(): init buttons", __FUNCTION__);
  buttonNew(&displayButton[BTN_TYPE_LEFT], DISPLAY_BTN_LEFT_PIN_NO,
            DISPLAY_BTN_LEFT_PIN_MODE, DISPLAY_BTN_LEFT_PIN_ACTIVE_LEVEL);
  buttonNew(&displayButton[BTN_TYPE_RIGHT], DISPLAY_BTN_RIGHT_PIN_NO,
            DISPLAY_BTN_RIGHT_PIN_MODE, DISPLAY_BTN_RIGHT_PIN_ACTIVE_LEVEL);

  // menu
  debugStream.writeI("%s(): init menu", __FUNCTION__);
  dynMenuParam4EditMode.isEditMode = false;
  LCDML_setup(_LCDML_DISP_cnt);
}



/**************************************************************************//**
 * \brief Display menu loop routine.
 ******************************************************************************/
void displaymenuLoop(void)
{
  // debugStream.writeI("%s()", __FUNCTION__);
  LCDML.loop();
}



/**************************************************************************//**
 * \brief Checks if the display menu shall be enabled.
 * 
 * \return true  - display menu activation event occurred
 * \return false - display menu activation event not occurred
 ******************************************************************************/
bool displaymenuIsEnableEvent(void)
{
  static bool buttonPressed = false;

  if (!isMenuEnabled)                                                           // menu disabled?
  {                                                                             // yes, check for button click...
    if (!areAllButtonsReleased())                                               // any button pressed?
    {                                                                           // yes...
      buttonPressed = true;
    }
    else if (buttonPressed)                                                     // all button released, was any pressed?
    {                                                                           // yes, enable menu...
      debugStream.writeI("%s(): enable menu", __FUNCTION__);
      buttonPressed = false;
      isMenuEnabled = true;
      LCDML.init(_LCDML_DISP_cnt);
      LCDML.MENU_enRollover();
    }
    else
      buttonPressed = false;
  }
  else if (isMenuExitEvent && areAllButtonsReleased())                          // menu is enabled, menu exit event and all buttons released?
  {                                                                             // yes, disable menu...
    debugStream.writeI("%s(): disable menu", __FUNCTION__);
    isMenuExitEvent = false;
    isMenuEnabled = false;
  }
  
  return isMenuEnabled;  
}
