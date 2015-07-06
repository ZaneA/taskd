#ifndef __STORAGE_H
#define __STORAGE_H

#include <sqlite3.h>

#define _STORAGE_PERSISTENT "main"
#define _STORAGE_MEMORY     "memory"

typedef struct {
    sqlite3 *conn;
    int queries;
} storage_t;

int storage_exec_noresult(storage_t *storage, const char *format, ...);
void storage_exec_result_(storage_t *storage, void *user, int(*callback)(void*, int, char**, char**), const char *format, ...);
char* storage_exec_result(storage_t *storage, const char *format, ...);
int storage_init(storage_t *storage, const char *path);
void storage_shutdown();

#ifdef DEBUG
void storage_debug(storage_t *storage);
#endif

#endif
