#ifndef TST_H
#define TST_H

#include <cmocka.h>
#include <stddef.h>

//
// test definition
//
#define TEST(t) cmocka_unit_test(t)
#define RUN(t) cmocka_run_group_tests(t, NULL, NULL)

#endif // TST_H
