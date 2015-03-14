/*
 * stolen from https://github.com/nekromant/esp8266-frankenstein
 * Adapted by jlz3008
 */
#ifndef _ENV_H_
#define _ENV_H_


struct env_element {
    char* key;
    char* value;
};

void env_init(uint32_t flashaddr, uint32_t envsize, struct env_element *def,unsigned deflen);
int env_insert(const char* key, const char *value);
const char* env_get(const char* key);
void env_save(void);
void env_dump(void);
void env_reset(void);
int env_delete(const char* key);

#endif // _ENV_H_
