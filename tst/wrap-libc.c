#include <cmocka.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <wayland-util.h>
#include <yaml.h>

/*
 * libc
 */

int __real_fclose (FILE *__stream);
int __wrap_fclose (FILE *__stream) {
	if (has_mock())
		return mock_int();
	else
		return __real_fclose(__stream);
}
