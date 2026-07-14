#include <cmocka.h>
#include <stdio.h>

/*
 * libc
 */

int __real_fclose (FILE *__stream);
int __wrap_fclose (FILE *__stream) {

	// close it anyway
	int rc = __real_fclose(__stream);

	return has_mock() ? mock_int() : rc;
}
