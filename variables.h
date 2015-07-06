#ifndef __VARIABLES_H
#define __VARIABLES_H

#define _VARIABLES_TABLE _STORAGE_MEMORY ".variables"

typedef struct {
    int num_vars;
} variables_t;

int variables_init(variables_t *variables);
void variables_shutdown();

char* variables_get(const char *key);
void variables_set(const char *key, const char *format, ...);

#endif
