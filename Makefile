taskd: main.c storage.c storage.h taskrunner.c taskrunner.h variables.c variables.h profiles.c profiles.h plugins.c plugins.h plugin_api.h
	gcc -Wall -Wextra -Werror -std=gnu99 -pedantic -I/usr/include/lua5.1 -o taskd main.c storage.c taskrunner.c variables.c profiles.c plugins.c -lsqlite3 -llua5.1 -ldl

p_core: p_core.c plugin_api.h
	gcc -Wall -Wextra -Werror -std=gnu99 -pedantic -shared -fPIC -o plugins/p_core.so p_core.c

p_httpapi: p_httpapi.c plugin_api.h
	gcc -Wall -Wextra -Werror -std=gnu99 -pedantic -shared -fPIC -o plugins/p_httpapi.so p_httpapi.c -lmicrohttpd
