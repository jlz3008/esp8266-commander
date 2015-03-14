#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"

#include "env.h"

struct env_element defaultenv[] =
{
   { "hostname", "TN1" },
   { "name", "jlz3008" },
};



//------------------------------------------------------------
void ICACHE_FLASH_ATTR user_init()
{
#define  CONFIG_ENV_OFFSET 0x7f000
#define  CONFIG_ENV_LEN    0x1000


    env_init(CONFIG_ENV_OFFSET, CONFIG_ENV_LEN,defaultenv,(sizeof(defaultenv)/sizeof(defaultenv[0])));
    console_init(32);

}
