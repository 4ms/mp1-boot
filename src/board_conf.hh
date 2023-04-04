#pragma once

// You must select your board:
// -DBOARD_CONF_DK2 or #define BOARD_CONF_DK2 to select the MP153 Discovery Board;
// -DBOARD_CONF_OSD32 or #define BOARD_CONF_OSD32 to select the OSD32 Board;
// -DBOARD_CONF_PATH="path/file.hh" or #define BOARD_CONF_PATH "path/file.hh" to use a custom board conf file.
// Passing a compile flag -DBOARD_CONF_XXX is the preferred method, but you may also place the #define before any #include "board_conf.hh"

#define HEADER_STRING_I(s) #s
#define HEADER_STRING(s) HEADER_STRING_I(s)

#if defined(BOARD_CONF_PATH)
#include HEADER_STRING(BOARD_CONF_PATH)

#elif defined(BOARD_CONF_OSD32)
#include "board_conf/osd32brk_conf.hh"

#elif defined(BOARD_CONF_DK2)
#include "board_conf/stm32disco_conf.hh"

#else
#include "board_conf/stm32disco_conf.hh"
#warning                                                                                                                 \
	"No board was selected, defaulting to STM32 MP157 Discovery board. See src/board_conf.hh" 
#endif
