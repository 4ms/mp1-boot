// Uncomment one of these to select your board:
// Or add your own at the end

// #include "osd32brk_conf.hh"
// namespace Board = OSD32BRK;

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
