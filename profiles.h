#ifndef __PROFILES_H
#define __PROFILES_H

#define PROFILES_STATE_INACTIVE 0
#define PROFILES_STATE_ACTIVE   1

#define PROFILES_CONDITION_ALWAYS               0
#define PROFILES_CONDITION_CUSTOM               1
#define PROFILES_CONDITION_VARIABLE_CHANGED     2

#define _PROFILES_TABLE             _STORAGE_PERSISTENT ".profiles"
#define _PROFILES_INTERNAL_TABLE    _STORAGE_MEMORY ".profiles"

typedef struct {
    int dummy;
} profiles_t;

int profiles_init(profiles_t *profiles);
void profiles_shutdown();

int profiles_queue(profiles_t *profiles, int condition_type, const char *data);

#endif
