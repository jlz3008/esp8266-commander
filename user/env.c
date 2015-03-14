/*
 * stolen from https://github.com/nekromant/esp8266-frankenstein
 * Adapted by jlz3008
 *
 * Dirty and inefficient key=value storage
 */

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

#include "microrl.h"
#include "console.h"

#include "env.h"

struct environment {
	uint16_t crc;
	uint16_t occupied; 
	char data[];
} __attribute__ ((packed));

static struct environment *current_env;
static uint16_t current_env_size;
static uint16_t current_env_end;
static uint32_t current_env_flash_addr; 

struct env_element *env_defaultenv;
unsigned            env_defaultenv_len;

/* [CRC, LEN]| KEY 0x0, VALUE, 0x0, KEY 0x0, VALUE, 0x0 ... 0xFF */


uint16_t ICACHE_FLASH_ATTR crc16(const unsigned char *buf, int sz)
{
        uint16_t crc = 0;
        while (--sz >= 0)
        {
                int i;
                crc ^= (uint16_t) *buf++ << 8;
                for (i = 0; i < 8; i++)
                        if (crc & 0x8000)
                                crc = crc << 1 ^ 0x1021;
                        else
                                crc <<= 1;
        }
        return crc;
}

//-------------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR request_default_environment(void)
{
    int i;
    for (i=0; i<env_defaultenv_len; i++)
        env_insert(env_defaultenv[i].key, env_defaultenv[i].value);
}

//-------------------------------------------------------------------------------
struct env_element ICACHE_FLASH_ATTR env_next(char **ptr)
{
	struct env_element ret;
	ret.key = NULL;
	
	if (**ptr == 0xFF)
		goto bailout; /* at end */

	ret.key = *ptr;
	*ptr+= strlen(*ptr) + 1;
	ret.value = *ptr;
	*ptr+=strlen(*ptr) + 1;
 
bailout:
	return ret;
}
//-------------------------------------------------------------------------------
int ICACHE_FLASH_ATTR env_delete(const char* key)
{

	char *ptr = current_env->data;
	struct env_element e;
	do {
		e = env_next(&ptr);
		if (e.key && (strcmp(e.key, key)==0)) {  
			char* to = e.key;
			e = env_next(&ptr);				
			char* from = e.key;
			if (!from)
				from = &current_env->data[current_env->occupied]; 
			int delta = (from - to); 
			while (*from != 0xff) 
				*to++ = *from++;
			*to = 0xff;
			current_env->occupied-=delta;
			return 0;
		}
	} while (e.key);	
	return 0;
}
//-------------------------------------------------------------------------------
int ICACHE_FLASH_ATTR env_insert(const char* key, const char *value)
{
	int klen = strlen(key) + 1;
	int vlen = strlen(value) + 1;
	if (klen + vlen >= (current_env_size - current_env->occupied))
		return -1; /* No space */
	env_delete(key);
	memcpy(&current_env->data[current_env->occupied], key, klen); 
	current_env->occupied+=klen;
	memcpy(&current_env->data[current_env->occupied], value, vlen); 
	current_env->occupied+=vlen;
	current_env->data[current_env->occupied]=0xff;
	return 0;
}
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR env_save(void)
{
#ifndef CONFIG_ENV_NOWRITE
	current_env->crc = crc16((const unsigned char*)&current_env->occupied, current_env_size + sizeof(uint16_t) );
	spi_flash_erase_sector(current_env_flash_addr / SPI_FLASH_SEC_SIZE);
	spi_flash_write(current_env_flash_addr, (uint32*)current_env, 
			current_env_size + sizeof(uint16_t));
#else
	console_printf("Saving environment to flash disabled by config\n");
	console_printf("Recompile with CONFIG_ENV_NOWRITE=n\n");
#endif	
}
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR env_reset(void)
{
	current_env->occupied=0;
	current_env_end=0;
	current_env->data[0]=0xFF;
	request_default_environment();
	env_save();	
}
//-------------------------------------------------------------------------------
const ICACHE_FLASH_ATTR char* env_get(const char* key)
{
	char *ptr = current_env->data;
	struct env_element e;
	do {
		e = env_next(&ptr);
		if (e.key && (strcmp(key, e.key)==0)) 
			return e.value;
	} while (e.key);
	return NULL;
}
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR env_dump(void)
{
	char *ptr = current_env->data;
	struct env_element e;
	do {
		e = env_next(&ptr);
		if (e.key) 
            console_printf("%-10s = %s\n", e.key, e.value);
	} while (e.key);
	console_printf("=== %d/%d bytes used ===\n", 
		       current_env->occupied, 
		       current_env_size);
}
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR env_init(uint32_t flashaddr, uint32_t envsize, struct env_element *def,unsigned deflen)
{
    env_defaultenv = def;
    env_defaultenv_len = deflen;
    current_env =  (struct environment *)os_malloc(envsize);
	current_env_flash_addr = flashaddr;
	current_env_size=envsize - sizeof(struct environment);

    os_printf("env: Environment @ %p size %d bytes (%d real) \n",
		       (void*)flashaddr, (int)envsize, (int)current_env_size);

	spi_flash_read(flashaddr, (uint32*)current_env, envsize);
	uint16_t crc = crc16((const unsigned char*)&current_env->occupied, envsize - sizeof(uint16_t));
	if (crc != current_env->crc) { 
        os_printf("env: Bad CRC (%x vs %x) using defaults\n", crc, current_env->crc);
		env_reset();
	}
	env_dump();
}
