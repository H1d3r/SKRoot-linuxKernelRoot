
#include "cpu_pin_guard_auto.h"
#include "test_linux_kernel_api.h"
#include "test_string_ops.h"

KModErr Test_execute_kernel_asm_func() {
    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    kernel_module::arm64_module_asm_func_end(a, 0x12345);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;
    uint64_t result = 0;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, result));
    
    print_current_uid_caps_state();
    printf("Shellcode output result: %s, value: %p\n", result == 0x12345 ? "ok" : "failed", (void*)result);
    return KModErr::OK;
}

KModErr Test_alloc_kernel_mem() {
    uint64_t addr = 0;
    RETURN_IF_ERROR(kernel_module::alloc_kernel_mem(g_root_key, 1024, addr));
    printf("Output addr: %p\n", (void*)addr);
    return KModErr::OK;
}

KModErr Test_free_kernel_mem() {
    uint64_t addr = 0;
    RETURN_IF_ERROR(kernel_module::alloc_kernel_mem(g_root_key, 1024, addr));
    printf("Output addr: %p\n", (void*)addr);
    RETURN_IF_ERROR(kernel_module::free_kernel_mem(g_root_key, addr));
    return KModErr::OK;
}

KModErr Test_write_rw_kernel_mem() {
    uint32_t test_data[2] = {0x11223344, 0x55667788};

    uint64_t addr = 0;
    RETURN_IF_ERROR(kernel_module::alloc_kernel_mem(g_root_key, sizeof(test_data), addr));
    RETURN_IF_ERROR(kernel_module::write_kernel_mem(g_root_key, addr, &test_data, sizeof(test_data)));
    printf("Output addr: %p\n", (void*)addr);
    return KModErr::OK;
}

KModErr Test_read_kernel_mem() {
    uint32_t test_data[2] = {0xAABBCCDD, 0xEEFF1122};

    uint64_t addr = 0;
    RETURN_IF_ERROR(kernel_module::alloc_kernel_mem(g_root_key, sizeof(test_data), addr));
    RETURN_IF_ERROR(kernel_module::write_kernel_mem(g_root_key, addr, &test_data, sizeof(test_data)));
    printf("Output addr: %p\n", (void*)addr);

    std::vector<uint8_t> buf(sizeof(test_data));
    RETURN_IF_ERROR(kernel_module::read_kernel_mem(g_root_key, addr, buf.data(), buf.size()));
    printf("read_kernel_mem Buffer bytes:");
    for (size_t i = 0; i < buf.size(); ++i) {
        printf(" %02hhx", buf[i]);
    }
    printf("\n");
    return KModErr::OK;
}

KModErr Test_get_kernel_virtual_mem_start_addr() {
    uint64_t result_addr = 0;
    RETURN_IF_ERROR(kernel_module::get_kernel_virtual_mem_start_addr(g_root_key, result_addr));
    printf("Output addr: %p\n", (void*)result_addr);
    return KModErr::OK;
}

KModErr Test_write_x_kernel_mem() {
    uint64_t result_addr = 0;
    RETURN_IF_ERROR(kernel_module::kallsyms_lookup_name(g_root_key, "kernel_halt", result_addr));

    // 读取原始内存内容
    uint8_t buf[16] = {0};
    RETURN_IF_ERROR(kernel_module::read_kernel_mem(g_root_key, result_addr, buf, sizeof(buf)));
    printf("read_kernel_mem (before) Buffer before:");
    for (size_t i = 0; i < sizeof(buf); ++i) {
        printf(" %02hhx", buf[i]);
    }
    printf("\n");

    // 准备修改数据（8 字节）
    uint32_t ccmd[2] = { 0x11223344, 0x55667788 };
    RETURN_IF_ERROR(kernel_module::write_kernel_mem(g_root_key, result_addr, ccmd, sizeof(ccmd), kernel_module::KernMemProt::KMP_X));

    // 再次读取内存以验证修改效果
    memset(buf, 0, sizeof(buf));
    RETURN_IF_ERROR(kernel_module::read_kernel_mem(g_root_key, result_addr, buf, sizeof(buf)));
    printf("read_kernel_mem (after) Buffer after:");
    for (size_t i = 0; i < sizeof(buf); ++i) {
        printf(" %02hhx", buf[i]);
    }
    printf("\n");
    return KModErr::OK;
}

KModErr Test_rw_kernel_runtime_storage() {
    KModErr err = KModErr::ERR_MODULE_ASM;
    std::string str_value1;
    std::string str_value2;
    err = kernel_module::read_string_kernel_runtime_storage(g_root_key, EN_MODULE_NAME, "testKey1", str_value1);
    if (is_failed(err) && err != KModErr::ERR_MODULE_STORAGE_NO_FOUND) return err;
    err = kernel_module::read_string_kernel_runtime_storage(g_root_key, "app_name2", "testKey1", str_value2);
    if (is_failed(err) && err != KModErr::ERR_MODULE_STORAGE_NO_FOUND) return err;
    printf("read_string_kernel_storage Output string1: %s, string2: %s\n", str_value1.c_str(), str_value2.c_str());
    std::string new_value1 = std::to_string(time(NULL));
    std::string new_value2 = new_value1;
    std::reverse(new_value2.begin(), new_value2.end());
    RETURN_IF_ERROR(kernel_module::write_string_kernel_runtime_storage(g_root_key, EN_MODULE_NAME, "testKey1", new_value1.c_str()));
    RETURN_IF_ERROR(kernel_module::write_string_kernel_runtime_storage(g_root_key, "app_name2", "testKey1", new_value2.c_str()));
    printf("write_string_kernel_storage Write string1: %s, string2: %s\n", new_value1.c_str(), new_value2.c_str());
    return KModErr::OK;
}

std::vector<uint8_t> generate_filename_lookup_before_hook_bytes() {
    KModErr err = KModErr::OK;
    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_before_hook_start(a);
    kernel_module::export_symbol::printk(g_root_key, a, err, "[!!!] test ok\n");
    kernel_module::arm64_before_hook_end(a, true);
    return aarch64_asm_to_bytes(asm_info);;
}

KModErr Test_install_kernel_function_before_hook() {
    kernel_module::SymbolHit filename_lookup;
    RETURN_IF_ERROR(kernel_module::get_filename_lookup_addr(g_root_key, filename_lookup));
    printf("get_filename_lookup_addr Output addr: %p, name: %s\n", (void*)filename_lookup.addr, filename_lookup.name);
   
    // Read memory before hook
    {
        uint8_t buf[4] = {0};
        RETURN_IF_ERROR(kernel_module::read_kernel_mem(g_root_key, filename_lookup.addr, buf, sizeof(buf)));
        printf("read_kernel_mem (filename_lookup) Before before:");
        for (size_t i = 0; i < sizeof(buf); ++i) {
            printf(" %02hhx", buf[i]);
        }
        printf("\n");
    }

    std::vector<uint8_t> my_func_bytes = generate_filename_lookup_before_hook_bytes();
    KModErr err = kernel_module::install_kernel_function_before_hook(g_root_key, filename_lookup.addr, my_func_bytes);
    printf("install_kernel_function_before_hook return: %s\n", to_string(err).c_str());
    RETURN_IF_ERROR(err);
    // Read memory after hook
    {
        uint8_t buf[4] = {0};
        RETURN_IF_ERROR(kernel_module::read_kernel_mem(g_root_key, filename_lookup.addr, buf, sizeof(buf)));
        printf("read_kernel_mem (filename_lookup) After before:");
        for (size_t i = 0; i < sizeof(buf); ++i) {
            printf(" %02hhx", buf[i]);
        }
        printf("\n");
    }
    return KModErr::OK;
}

std::vector<uint8_t> generate_avc_denied_after_hook_bytes() {
    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_after_hook_start(a);
    a->mov(x0, xzr);
    kernel_module::arm64_after_hook_end(a);
    return aarch64_asm_to_bytes(asm_info);;
}

KModErr Test_install_kernel_function_after_hook() {
    uint64_t avc_denied_addr = 0, ret_addr = 0;
    RETURN_IF_ERROR(kernel_module::get_avc_denied_addr(g_root_key, avc_denied_addr, ret_addr));
    printf("get_avc_denied_addr Output start addr: %p, ret addr: %p\n", (void*)avc_denied_addr, (void*)ret_addr);
   
    // Read memory before hook
    {
        uint8_t buf[4] = {0};
        RETURN_IF_ERROR(kernel_module::read_kernel_mem(g_root_key, avc_denied_addr, buf, sizeof(buf)));
        printf("read_kernel_mem (avc_denied) Before before:");
        for (size_t i = 0; i < sizeof(buf); ++i) {
            printf(" %02hhx", buf[i]);
        }
        printf("\n");
    }

    std::vector<uint8_t> my_func_bytes = generate_avc_denied_after_hook_bytes();
    KModErr err = kernel_module::install_kernel_function_after_hook(g_root_key, avc_denied_addr, my_func_bytes);
    printf("install_kernel_function_after_hook return: %s\n", to_string(err).c_str());
    RETURN_IF_ERROR(err);
    // Read memory after hook
    {
        uint8_t buf[4] = {0};
        RETURN_IF_ERROR(kernel_module::read_kernel_mem(g_root_key, avc_denied_addr, buf, sizeof(buf)));
        printf("read_kernel_mem (avc_denied) After before:");
        for (size_t i = 0; i < sizeof(buf); ++i) {
            printf(" %02hhx", buf[i]);
        }
        printf("\n");
    }
    return KModErr::OK;
}

KModErr Test_get_task_struct_pid_offset() {
    uint32_t result_offset = 0;
    RETURN_IF_ERROR(kernel_module::get_task_struct_pid_offset(g_root_key, result_offset));
    printf("Output offset: 0x%x\n", result_offset);
    return KModErr::OK;
}

KModErr Test_get_task_struct_real_parent_offset() {
    uint32_t result_offset = 0;
    RETURN_IF_ERROR(kernel_module::get_task_struct_real_parent_offset(g_root_key, result_offset));
    printf("Output offset: 0x%x\n", result_offset);
    return KModErr::OK;
}

KModErr Test_get_task_struct_comm_offset() {
    uint32_t result_offset = 0;
    RETURN_IF_ERROR(kernel_module::get_task_struct_comm_offset(g_root_key, result_offset));
    printf("Output offset: 0x%x\n", result_offset);
    return KModErr::OK;
}

KModErr Test_get_task_struct_real_cred_offset() {
    uint32_t result_offset = 0;
    RETURN_IF_ERROR(kernel_module::get_task_struct_real_cred_offset(g_root_key, result_offset));
    printf("Output offset: 0x%x\n", result_offset);
    return KModErr::OK;
}

KModErr Test_get_cred_uid_offset() {
    uint32_t result_offset = 0;
    RETURN_IF_ERROR(kernel_module::get_cred_uid_offset(g_root_key, result_offset));
    printf("Output offset: 0x%x\n", result_offset);
    return KModErr::OK;
}

KModErr Test_get_mm_struct_arg_offset() {
    uint32_t arg_start_offset = 0, arg_end_offset = 0;
    RETURN_IF_ERROR(kernel_module::get_mm_struct_arg_offset(g_root_key, arg_start_offset, arg_end_offset));
    printf("Output arg_start offset: 0x%x\n", arg_start_offset);
    printf("Output arg_end offset: 0x%x\n", arg_end_offset);
    return KModErr::OK;
}

KModErr Test_get_mm_struct_env_offset() {
    uint32_t env_start_offset = 0, env_end_offset = 0;
    RETURN_IF_ERROR(kernel_module::get_mm_struct_env_offset(g_root_key, env_start_offset, env_end_offset));
    printf("Output env_start offset: 0x%x\n", env_start_offset);
    printf("Output env_end offset: 0x%x\n", env_end_offset);
    return KModErr::OK;
}

KModErr Test_set_current_caps() {
    kernel_module::caps_info caps;
    KModErr err = kernel_module::get_current_caps(caps);
    if(err == KModErr::OK) {
        printf("Inheritable: 0x%016llx\n", (unsigned long long)caps.inheritable);
        printf("Permitted  : 0x%016llx\n", (unsigned long long)caps.permitted);
        printf("Effective  : 0x%016llx\n", (unsigned long long)caps.effective);
        printf("Bounding   : 0x%016llx\n", (unsigned long long)caps.bounding);
        printf("Ambient    : 0x%016llx\n", (unsigned long long)caps.ambient);
    }
    printf("get_current_caps return:%s\n", to_string(err).c_str());
    RETURN_IF_ERROR(err);
    caps.inheritable = 0x2FFFFFFFFF;
    caps.permitted = 0x2FFFFFFFFF;
    caps.effective = 0x2FFFFFFFFF;
    caps.bounding = 0x2FFFFFFFFF;
    caps.ambient = 0x2FFFFFFFFF;
    err = kernel_module::set_current_caps(g_root_key, caps);
    printf("set_current_caps return:%s\n", to_string(err).c_str());
    RETURN_IF_ERROR(err);
    err = kernel_module::get_current_caps(caps);
    if(err == KModErr::OK) {
        printf("Inheritable: 0x%016llx\n", (unsigned long long)caps.inheritable);
        printf("Permitted  : 0x%016llx\n", (unsigned long long)caps.permitted);
        printf("Effective  : 0x%016llx\n", (unsigned long long)caps.effective);
        printf("Bounding   : 0x%016llx\n", (unsigned long long)caps.bounding);
        printf("Ambient    : 0x%016llx\n", (unsigned long long)caps.ambient);
    }
    return KModErr::OK;
}

int main(int argc, char *argv[]) {
    printf("Kernel Module Kit test starting...\n");
    if (argc >= 2) {
        strncpy(g_root_key, argv[1], sizeof(g_root_key) - 1);
    } else {
        //TODO: 在此修改你的Root key值。
        strncpy(g_root_key, "KQDrJKBQvFyUS4cWS3SGkiUmAFnFblNwLJN3Hhvffjs5z93z", sizeof(g_root_key) - 1);
    }
    CpuPinGuardAuto cpu_lock;

    int idx = 1;
    // 单元测试：内核模块基础能力
    TEST(idx++, Test_execute_kernel_asm_func);               // 执行shellcode并获取返回值
    TEST(idx++, Test_alloc_kernel_mem);                      // 申请内核内存
    TEST(idx++, Test_free_kernel_mem);                       // 释放内核内存
    TEST(idx++, Test_write_rw_kernel_mem);                   // 写入内核内存(可读写区域)
    TEST(idx++, Test_read_kernel_mem);                       // 读取内核内存
    TEST(idx++, Test_get_kernel_virtual_mem_start_addr);     // 获取内核静态代码段（.text）起始虚拟地址
    TEST(idx++, Test_write_x_kernel_mem);                    // 写入内核内存(仅执行区域)
    TEST(idx++, Test_rw_kernel_runtime_storage);             // 读取、写入内核运行时存储
    TEST(idx++, Test_install_kernel_function_before_hook);   // 安装内核钩子（在内核函数执行前）
    TEST(idx++, Test_install_kernel_function_after_hook);    // 安装内核钩子（在内核函数执行后）
    TEST(idx++, Test_get_task_struct_pid_offset);            // 获取 task_struct 结构体中 pid 字段的偏移量
    TEST(idx++, Test_get_task_struct_real_parent_offset);    // 获取 task_struct 结构体中 real_parent 字段的偏移量
    TEST(idx++, Test_get_task_struct_comm_offset);           // 获取 task_struct 结构体中 comm 字段的偏移量
    TEST(idx++, Test_get_task_struct_real_cred_offset);      // 获取 task_struct 结构体中 real_cred 字段的偏移量
    TEST(idx++, Test_get_cred_uid_offset);                   // 获取 cred 结构体中 uid 字段的偏移量
    TEST(idx++, Test_get_mm_struct_arg_offset);              // 获取 mm_struct 结构体中 arg_start\arg_end 字段的偏移量
    TEST(idx++, Test_get_mm_struct_env_offset);              // 获取 mm_struct 结构体中 env_start\env_end 字段的偏移量
    TEST(idx++, Test_set_current_caps);                      // 设置进程能力集

    // 单元测试: Linux内核API调用
    TEST(idx++, Test_kallsyms_lookup_name1);                 // 调用内核API：kallsyms_lookup_name
    TEST(idx++, Test_kallsyms_lookup_name2);
    TEST(idx++, Test_get_task_mm);                           // 调用内核API：get_task_mm、mmput
    TEST(idx++, Test_printk);                                // 调用内核API：printk
    TEST(idx++, Test_copy_from_user);                        // 调用内核API：copy_from_user
    TEST(idx++, Test_copy_to_user);                          // 调用内核API：copy_to_user
    TEST(idx++, Test_kmalloc1);                              // 调用内核API：kmalloc
    TEST(idx++, Test_kfree1);                                // 调用内核API：kfree
    TEST(idx++, Test_kmalloc2);
    TEST(idx++, Test_kfree2);
    TEST(idx++, Test_module_alloc1);                         // 调用内核API：module_alloc
    TEST(idx++, Test_module_memfree1);                       // 调用内核API：module_memfree
    TEST(idx++, Test_module_alloc2);
    TEST(idx++, Test_module_memfree2);
    TEST(idx++, Test_kallsyms_on_each_symbol1);              // 调用内核API：kallsyms_on_each_symbol
    TEST(idx++, Test_kallsyms_on_each_symbol2);
    TEST(idx++, Test_kern_path);                             // 调用内核API：kern_path

    // 单元测试：内核字符串与内存操作
    TEST(idx++, Test_kstrlen);
    TEST(idx++, Test_kstrcmp1);
    TEST(idx++, Test_kstrcmp2);
    TEST(idx++, Test_kstrcmp3);
    TEST(idx++, Test_kstrncmp1);
    TEST(idx++, Test_kstrncmp2);
    TEST(idx++, Test_kstrncmp3);
    TEST(idx++, Test_kstrncmp4);
    TEST(idx++, Test_kstrncmp5);
    TEST(idx++, Test_kstrncmp6);
    TEST(idx++, Test_kstrcpy);
    TEST(idx++, Test_kstrstr1);
    TEST(idx++, Test_kstrstr2);
    TEST(idx++, Test_kstrstr3);
    TEST(idx++, Test_kstrchr1);
    TEST(idx++, Test_kstrchr2);
    TEST(idx++, Test_kmemset1);
    TEST(idx++, Test_kmemset2);
    TEST(idx++, Test_kmemcmp1);
    TEST(idx++, Test_kmemcmp2);
    TEST(idx++, Test_kmemcmp3);
    TEST(idx++, Test_kmemcpy);
    TEST(idx++, Test_kmemmem1);
    TEST(idx++, Test_kmemmem2);
    TEST(idx++, Test_kmemmem3);
    TEST(idx++, Test_kmemmem4);
    TEST(idx++, Test_kmemmem5);
    TEST(idx++, Test_kmemmem6);
    TEST(idx++, Test_kmemmem7);
    TEST(idx++, Test_kmemchr1);
    TEST(idx++, Test_kmemchr2);
    TEST(idx++, Test_kmemchr3);
    TEST(idx++, Test_kmemrchr1);
    TEST(idx++, Test_kmemrchr2);
    TEST(idx++, Test_kmemrchr3);
    TEST(idx++, Test_kstartswith1);
    TEST(idx++, Test_kstartswith2);
    TEST(idx++, Test_kendswith1);
    TEST(idx++, Test_kendswith2);

    return 0;
}
