#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <thread>
#include <vector>

#include <sys/syscall.h>

#include "kernel_module_kit_umbrella.h"

inline char g_root_key[256] = {0};

#define EN_MODULE_NAME "module_all_demo"

#define TEST(NUM, NAME)                                                    \
    do {                                                                   \
        int _test_idx = (NUM);                                             \
        printf("\n----- Test%d: %s -----\n", _test_idx, #NAME);            \
        KModErr err = (NAME)();                                            \
        printf("[%s] run done, return error code: %s\n", #NAME, to_string(err).c_str());     \
        printf("----- End of Test%d -----\n\n", _test_idx);                \
    } while (0)

