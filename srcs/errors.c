#include "ft_ping.h"

void show_error(int code, char *msg, ...) {
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);

	if (code > 0)
		exit(code);
};
