#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <thread>
#include <vector>

#include <sys/syscall.h>
#include <sys/prctl.h>

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

static inline __uid_t my_getfsuid() {
    return syscall(SYS_setfsuid, (uid_t)-1);
}

static inline __gid_t my_getfsgid() {
    return syscall(SYS_setfsgid, (gid_t)-1);
}

static void print_current_uid_caps_state() {
    __uid_t ruid, euid, suid;
    if (getresuid(&ruid, &euid, &suid) != 0) {
        perror("getresuid failed");
        return;
    }
    __gid_t rgid, egid, sgid;
    if (getresgid(&rgid, &egid, &sgid) != 0) {
        perror("getresgid failed");
        return;
    }
    __uid_t fsuid = my_getfsuid();
    __gid_t fsgid = my_getfsgid();

    printf("current self process status:\n");
    printf("ruid: %d\n", ruid);
    printf("rgid: %d\n", rgid);
    printf("suid: %d\n", suid);
    printf("sgid: %d\n", sgid);
    printf("euid: %d\n", euid);
    printf("egid: %d\n", egid);
    printf("fsuid: %d\n", fsuid);
    printf("fsgid: %d\n", fsgid);

    kernel_module::caps_info caps;
    kernel_module::get_current_caps(caps);
    int seccomp = prctl(PR_GET_SECCOMP, 0, 0, 0, 0);
    int securebits = prctl(PR_GET_SECUREBITS);
    printf("capInh: 0x%016llx\n", (unsigned long long)caps.inheritable);
    printf("capPrm: 0x%016llx\n", (unsigned long long)caps.permitted);
    printf("capEff: 0x%016llx\n", (unsigned long long)caps.effective);
    printf("capBnd: 0x%016llx\n", (unsigned long long)caps.bounding);
    printf("capAmb: 0x%016llx\n", (unsigned long long)caps.ambient);
    printf("seccomp: %d\n", seccomp);
    printf("securebits: %d\n", securebits);

    FILE *fp = popen("getenforce", "r");
    if (fp) {
        char cmd[512] = {0};
        fread(cmd, 1, sizeof(cmd) - 1, fp);
        pclose(fp);
        printf("read system SELinux status:%s\n", cmd);
    }
}
