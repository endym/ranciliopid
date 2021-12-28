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
#define DISPLAY_FONT_TITLE        u8g2_font_profont11_tf
#define DISPLAY_FONT              u8g2_font_profont15_tf
#define DISPLAY_FONT_WIDTH        7                                             // font width 
#define DISPLAY_FONT_HEIGHT       15                                            // font heigt 
#define DISPLAY_WIDTH             SCREEN_WIDTH                                  // display width
#define DISPLAY_HEIGHT            SCREEN_HEIGHT                                 // display height
#define DISPLAY_COLS_MAX          (DISPLAY_WIDTH/DISPLAY_FONT_WIDTH)
#define DISPLAY_ROWS_MAX          (DISPLAY_HEIGHT/DISPLAY_FONT_HEIGHT)

// menu settings...
#define LCDML_TITLE_ROWS          1                                             // menu title rows
#define LCDML_SCROLLBAR_WIDTH     3                                             // scrollbar width 
#if (LCDML_SCROLLBAR_WIDTH == 0)
#define LCDML_COLS                DISPLAY_COLS_MAX                              // menu columns
#else
#define LCDML_COLS                (DISPLAY_COLS_MAX - 1)                        // menu columns
#endif
#define LCDML_ROWS                (DISPLAY_ROWS_MAX - LCDML_TITLE_ROWS)         // menu rows w/o menu title
#define LCDML_TEXT_INDENTING      (DISPLAY_FONT_WIDTH+2)



/******************************************************************************
 * TYPEDEFS
 ******************************************************************************/

//---buttons...-----------------------------------------------------------------
typedef enum
{
  // USED AS INDEX!
  BTN_TYPE_LEFT = 0,
  BTN_TYPE_RIGHT,

  BTN_TYPE__LAST_ENUM                                                           // must be the last enum!
}btn_type_t;

//---object for dynamic menu parameter handling...------------------------------
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
static void cbMenuDynParaBdThreshold(uint8_t line);
static void cbMenuDynParaBrewSetPoint(uint8_t line);
static void cbMenuDynParaBrewSwTimer(uint8_t line);
static void cbMenuDynParaSteamSetPoint(uint8_t line);
static void cbMenuDynParaPidKpStart(uint8_t line);
static void cbMenuDynParaPidTnStart(uint8_t line);
static void cbMenuDynParaPidKp(uint8_t line);
static void cbMenuDynParaPidTn(uint8_t line);
static void cbMenuDynParaPidTv(uint8_t line);
static void cbMenuDynParaPidKpBd(uint8_t line);
static void cbMenuDynParaPidTnBd(uint8_t line);
static void cbMenuDynParaPidTvBd(uint8_t line);
static void cbMenuSaveSettings(uint8_t param);



/******************************************************************************
 * VARIABLES
 ******************************************************************************/

extern DebugStreamManager debugStream;

//---buttons...-----------------------------------------------------------------
static btn_t displayButton[BTN_TYPE__LAST_ENUM];

//---display...-----------------------------------------------------------------
extern LCDMenuLib2 LCDML;
#if DISPLAY == 1
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
#elif DISPLAY == 2
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
#endif

//---menu...--------------------------------------------------------------------
static menu_dyn_param_t dynMenuParam4EditMode;
static unsigned long menuTimer;

LCDMenuLib2_menu LCDML_0 (255, 0, 0, NULL, NULL); // root menu element (do not change)
LCDMenuLib2 LCDML(LCDML_0, LCDML_ROWS, LCDML_COLS, lcdml_menu_display, lcdml_menu_clear, lcdml_menu_control);

// LCDML_add             (id, prev_layer, new_num, lang_char_array, callback_function)
// LCDMenuLib_addAdvanced(id, prev_layer, new_num, condition, lang_char_array, callback_function, parameter (0-255), menu function type  )
LCDML_add         (0,  LCDML_0,       1, "Information", cbMenuInformation);
LCDML_addAdvanced (1,  LCDML_0,       2, NULL,   "Settings"    ,   NULL,                       0, _LCDML_TYPE_default);
LCDML_addAdvanced (2,  LCDML_0_2,     1, NULL,   "",               cbMenuDynParaBrewSetPoint,  0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (3,  LCDML_0_2,     2, NULL,   "",               cbMenuDynParaSteamSetPoint, 0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (4,  LCDML_0_2,     3, NULL,   "",               cbMenuDynParaPidKp,         0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (5,  LCDML_0_2,     4, NULL,   "",               cbMenuDynParaPidTn,         0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (6,  LCDML_0_2,     5, NULL,   "",               cbMenuDynParaPidTv,         0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (7,  LCDML_0_2,     6, NULL,   "Cold Start",     NULL,                       0, _LCDML_TYPE_default);
LCDML_addAdvanced (8,  LCDML_0_2_6,   1, NULL,   "",               cbMenuDynParaPidKpStart,    0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (9,  LCDML_0_2_6,   2, NULL,   "",               cbMenuDynParaPidTnStart,    0, _LCDML_TYPE_dynParam);
LCDML_add         (10, LCDML_0_2_6,   3, "Back", cbMenuBack);
LCDML_addAdvanced (11, LCDML_0_2,     7, NULL,   "Brew Detection", NULL,                       0, _LCDML_TYPE_default);
LCDML_addAdvanced (12, LCDML_0_2_7,   1, NULL,   "",               cbMenuDynParaBdThreshold,   0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (13, LCDML_0_2_7,   2, NULL,   "",               cbMenuDynParaBrewSwTimer,   0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (14, LCDML_0_2_7,   3, NULL,   "",               cbMenuDynParaPidKpBd,       0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (15, LCDML_0_2_7,   4, NULL,   "",               cbMenuDynParaPidTnBd,       0, _LCDML_TYPE_dynParam);
LCDML_addAdvanced (16, LCDML_0_2_7,   5, NULL,   "",               cbMenuDynParaPidTvBd,       0, _LCDML_TYPE_dynParam);
LCDML_add         (17, LCDML_0_2_7,   6, "Back", cbMenuBack);
LCDML_add         (18, LCDML_0_2,     8, "Save", cbMenuSaveSettings);
LCDML_add         (19, LCDML_0_2,     9, "Back", cbMenuBack);
LCDML_add         (20, LCDML_0,       3, "Exit", cbMenuExit);

// menu element count - last element id
// this value must be the same as the last menu element
#define _LCDML_DISP_cnt    20

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
  // content variable
  char content_text[LCDML_COLS];  // save the content text of every menu element
  // menu element object
  LCDMenuLib2_menu *tmp;
  // some limit values
  uint8_t i = LCDML.MENU_getScroll();
  uint8_t maxi = LCDML_ROWS + i;
  uint8_t n = 0;
  // scrollbar
  uint8_t n_max             = (LCDML.MENU_getChilds() >= LCDML_ROWS) ? LCDML_ROWS : (LCDML.MENU_getChilds());
  uint8_t scrollbar_max     = LCDML.MENU_getChilds();
  uint8_t scrollbar_cur_pos = LCDML.MENU_getCursorPosAbs();
  // uint8_t scroll_pos        = ((1.*n_max * LCDML_ROWS) / (scrollbar_max - 1) * scrollbar_cur_pos);
  uint8_t stringIdx;

  u8g2.clearBuffer();                                                           // clear lcd
  n = 0;
  i = LCDML.MENU_getScroll();

  // check if this element has children
  if ((tmp = LCDML.MENU_getDisplayedObj()) != NULL)
  {
    // display a menu title with the parent element name
    if (LCDML_TITLE_ROWS > 0)
    {
      u8g2.setFont(DISPLAY_FONT_TITLE);                                         // set title font
      if (LCDML.MENU_getLayer() == 0)                                           // root menu?
      {                                                                         // yes, use fixed text...
        strncpy(content_text, getMachineName((enum MACHINE)MACHINEID), sizeof(content_text)-1);
        content_text[sizeof(content_text)-1] = 0;
      }
      else
      {                                                                         // no, use parent menu name...
        LCDML_getContent(content_text, LCDML.MENU_getParentID());
      }
      u8g2.setCursor(0, 0);
      u8g2.print(content_text);
    }

    // loop to display lines
    u8g2.setFont(DISPLAY_FONT);                                                 // set menu item font
    do
    {
      // check if a menu element has a condition and if the condition be true
      if (tmp->checkCondition())
      {
        if (tmp->checkType_menu() == true)                                      // default menu type?
        {                                                                       // yes, display menu content...
          LCDML_getContent(content_text, tmp->getID());
          if (tmp->getChild(1) != NULL)                                         // has sub-menu?
          {                                                                     // yes...
            // add sub-menu indicator at last column...
            for (stringIdx=strlen(content_text); stringIdx<(sizeof(content_text)-2); stringIdx++)
              content_text[stringIdx] = ' ';
            content_text[sizeof(content_text)-2] = '>';
            content_text[sizeof(content_text)-1] = 0;
          }
          u8g2.setCursor(LCDML_TEXT_INDENTING, DISPLAY_FONT_HEIGHT * (LCDML_TITLE_ROWS + n));
          u8g2.println(content_text);
        }
        else
        {                                                                       // no, non-default menu type...
          if (tmp->checkType_dynParam())
          {
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
  u8g2.setFont(DISPLAY_FONT);                                                   // set menu item font
  u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * (LCDML_TITLE_ROWS + LCDML.MENU_getCursorPos()));
  u8g2.println("*");

  // display scrollbar when more content as rows available and with > 2
  if (scrollbar_max > n_max && LCDML_SCROLLBAR_WIDTH > 2)
  {
    // set frame for scrollbar
    u8g2.drawFrame(DISPLAY_WIDTH - LCDML_SCROLLBAR_WIDTH, 0, LCDML_SCROLLBAR_WIDTH, DISPLAY_HEIGHT-0);
    // calculate scrollbar length
    uint8_t scrollbar_block_length = scrollbar_max - n_max;
    scrollbar_block_length = (DISPLAY_HEIGHT-0) / (scrollbar_block_length + LCDML_ROWS);
    // set scrollbar
    if (scrollbar_cur_pos == 0)
    {                                                                           // top position     (min)
      u8g2.drawBox(DISPLAY_WIDTH - (LCDML_SCROLLBAR_WIDTH-1), 0+1,
                   (LCDML_SCROLLBAR_WIDTH-2), scrollbar_block_length);
    }
    else if (scrollbar_cur_pos == (scrollbar_max-1))
    {                                                                           // bottom position  (max)
      u8g2.drawBox(DISPLAY_WIDTH - (LCDML_SCROLLBAR_WIDTH-1),
                   DISPLAY_HEIGHT - scrollbar_block_length,
                   (LCDML_SCROLLBAR_WIDTH-2), scrollbar_block_length);
    }
    else
    {                                                                           // between top and bottom
      u8g2.drawBox(DISPLAY_WIDTH - (LCDML_SCROLLBAR_WIDTH-1),
                   0 + (scrollbar_block_length * scrollbar_cur_pos + 1),
                   (LCDML_SCROLLBAR_WIDTH-2), scrollbar_block_length);
    }
  }

  u8g2.sendBuffer();
}


/**************************************************************************//**
 * \brief Callback for menu "Back" function.
 * 
 * \param param - parameter value
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
 * \param param - parameter value
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
 * \param param - parameter value
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
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 0); // line 1
    u8g2.println(getFwVersion());
    #if 0
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 1); // line 2
    u8g2.println();
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 2); // line 3
    u8g2.println();
    u8g2.setCursor(0, DISPLAY_FONT_HEIGHT * 3); // line 4
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


/**************************************************************************//**
 * \brief Callback for menu "Save Settings".
 * 
 * \param param - parameter value
 ******************************************************************************/
static void cbMenuSaveSettings(uint8_t param)
{
  if (LCDML.FUNC_setup())          // ****** SETUP *********
  {
    // remmove compiler warnings when the param variable is not used:
    LCDML_UNUSED(param);

    // write settings to non-volatile storage...
    u8g2.clearBuffer();
    u8g2.setCursor(0, 0); // line 1
    if (writeSysParamsToStorage() == 0)
      u8g2.println("Saving... OK");
    else
      u8g2.println("Saving... ERR");
    u8g2.sendBuffer();
    
    // set display time of this message
    LCDML.FUNC_setLoopInterval(100);                                            // trigger loop function at 100ms interval
    menuTimer = 0;
    LCDML.TIMER_msReset(menuTimer);                                             // start message display timer
  }

  if (LCDML.FUNC_loop())           // ****** LOOP *********
  {
    if (LCDML.TIMER_ms(menuTimer, 1500))                                        // message display timer expired?
    {                                                                           // yes, back to menu...
      LCDML.FUNC_goBackToMenu();
    }
  }

  if(LCDML.FUNC_close())      // ****** STABLE END *********
  {
  }
}


#if 0
// *********************************************************************
static void mFunc_para(uint8_t param)
// *********************************************************************
{
  if(LCDML.FUNC_setup())          // ****** SETUP *********
  {
    char lineText[LCDML_COLS+1];
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
 * \param line        - screen line of this parameter
 * \param paramType   - system parameter type
 * \param sysParamObj - system parameter object
 * \param stepSize    - value to dec/add per change
 ******************************************************************************/
static void handleDynPara(uint8_t line, sys_param_type_t paramType, sys_param_t *sysParamObj, double stepSize)
{
  uint8_t curLine = LCDML.MENU_getCursorPos();
  
  debugStream.writeI("%s(): param %i at line %u", __FUNCTION__, paramType, line);
  
  // read current parameter value...
  if (getSysParam(paramType, sysParamObj) != 0)
  {
    debugStream.writeE("%s(): failed to get param %i!", __FUNCTION__, paramType);
    return;
  }
  
  // make only an action when the cursor stands on this menu item
  if (line == curLine)                                                          // cursor at this line?
  {                                                                             // yes...
    if (LCDML.BT_checkAny())                                                    // any button pressed?
    {                                                                           // yes...
      if (LCDML.BT_checkEnter())                                                // button event "enter"?
      {                                                                         // yes, switch between scroll/edit mode...
        // When enter the edit mode the button event "pressed" should change the
        // paramter value instead of jump to prev/next menu item. Therefore the
        // menu scroll function must be disabled.
        // Leaving the edit mode will re-eanble the normal button behaviour.
        if (LCDML.MENU_getScrollDisableStatus() == 0)                           // menu scrolling enabled?
        {                                                                       // yes, enter edit mode...
          // After disable of menu scroll function it is possible to work with
          // BT_checkUp and BT_checkDown in this function.
          // This function can only be called in a menu, not in a menu function.
          LCDML.MENU_disScroll();
          dynMenuParam4EditMode.sysParam = *sysParamObj;                        // create copy of current parameter value
          dynMenuParam4EditMode.isEditMode = true;
        }
        else
        {                                                                       // no, leave edit mode...
          LCDML.MENU_enScroll();                                                // re-enable the normal menu scroll function
          setSysParam(paramType, dynMenuParam4EditMode.sysParam.cur);           // set edit value as new current value
          *sysParamObj = dynMenuParam4EditMode.sysParam;                        // update callers parameter object
          dynMenuParam4EditMode.isEditMode = false;
        }
      }

      // Following button check have only an effect when menu scrolling is disabled
      if (LCDML.BT_checkUp())
      {
        if (dynMenuParam4EditMode.sysParam.cur > dynMenuParam4EditMode.sysParam.min)
          dynMenuParam4EditMode.sysParam.cur -= stepSize;
        else
          dynMenuParam4EditMode.sysParam.cur = dynMenuParam4EditMode.sysParam.max;
        *sysParamObj = dynMenuParam4EditMode.sysParam;
      }

      // Following button check have only an effect when menu scrolling is disabled
      if (LCDML.BT_checkDown())
      {
        if (dynMenuParam4EditMode.sysParam.cur < dynMenuParam4EditMode.sysParam.max)
          dynMenuParam4EditMode.sysParam.cur += stepSize;
        else
          dynMenuParam4EditMode.sysParam.cur = dynMenuParam4EditMode.sysParam.min;
        *sysParamObj = dynMenuParam4EditMode.sysParam;
      }
    }
  }

  // clear current menu line, before writing current updated value...
  if (dynMenuParam4EditMode.isEditMode && (line == curLine))                    // edit mode and cursor at edit line?
  {                                                                             // yes, invert line background...
    // clear line with inverted color...
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, (LCDML_TITLE_ROWS+line)*DISPLAY_FONT_HEIGHT, DISPLAY_WIDTH, DISPLAY_FONT_HEIGHT);
    u8g2.setDrawColor(0);
  }
  else
  {                                                                             // no, normal navigation mode...
    // clear line...
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, (LCDML_TITLE_ROWS+line)*DISPLAY_FONT_HEIGHT, DISPLAY_WIDTH, DISPLAY_FONT_HEIGHT);
    u8g2.setDrawColor(1);
    #if 0
    // clear the line manuel because clear the complete content is disabled when a external refreshed function is active
    u8g2.setCursor(LCDML_TEXT_INDENTING, LCDML_TITLE_ROWS+line);
    for(uint8_t i=0;i<LCDML_COLS-3;i++) // -3 because: 
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
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_BREW_SETPOINT, &sysParam, 0.5);
  snprintf(lineText, sizeof(lineText), "Brew Temp. %5.1f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
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
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_STEAM_SETPOINT, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "Steam Temp.  %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "PID Kp" of coldstart phase.
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaPidKpStart(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_PID_KP_START, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "PID Kp       %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "PID Tn" of coldstart phase.
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaPidTnStart(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_PID_TN_START, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "PID Tn       %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "PID Kp" of regular operation.
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaPidKp(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_PID_KP_REGULAR, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "PID Kp       %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "PID Tn" of regular operation.
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaPidTn(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_PID_TN_REGULAR, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "PID Tn       %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "PID Tv" of regular operation.
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaPidTv(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_PID_TV_REGULAR, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "PID Tv       %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "PID Kp" of brew detection phase.
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaPidKpBd(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_PID_KP_BD, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "PID Kp       %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "PID Tn" of brew detection phase.
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaPidTnBd(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_PID_TN_BD, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "PID Tn       %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "PID Tv" of brew detection phase.
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaPidTvBd(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_PID_TV_BD, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "PID Tv       %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "brew detection threshold".
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaBdThreshold(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_BD_THRESHOLD, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "Threshold    %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
  u8g2.print(lineText);
  u8g2.setDrawColor(1);
}


/**************************************************************************//**
 * \brief Callback for setting parameter "brew software timer".
 * 
 * \param line - display line of this parameter
 ******************************************************************************/
static void cbMenuDynParaBrewSwTimer(uint8_t line)
{
  static sys_param_t sysParam;
  char lineText[LCDML_COLS+1];

  handleDynPara(line, SYS_PARAM_BREW_SW_TIMER, &sysParam, 1.0);
  snprintf(lineText, sizeof(lineText), "SW Timer     %3.0f", sysParam.cur);
  u8g2.setCursor(LCDML_TEXT_INDENTING, (LCDML_TITLE_ROWS+line) * DISPLAY_FONT_HEIGHT);
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
