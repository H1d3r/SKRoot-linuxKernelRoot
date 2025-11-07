#pragma once
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include "../module_base_err_def.h"
namespace kernel_module {
inline KModErr alloc_kernel_mem(const char* root_key, const std::vector<uint8_t>& initial_bytes, uint64_t& out_kaddr) {
    KModErr alloc_kernel_mem_and_fill(const char* root_key, const uint8_t*buf, uint32_t size, uint64_t& out_kaddr);
    return alloc_kernel_mem_and_fill(root_key, initial_bytes.data(), initial_bytes.size(), out_kaddr);
}

inline KModErr execute_kernel_asm_func(const char* root_key, const std::vector<uint8_t>& func_bytes, uint64_t& output_result) {
    KModErr execute_kernel_asm_func_with_buf(const char* root_key, const uint8_t*shellcode, uint32_t size, uint64_t& out_kaddr);
    return execute_kernel_asm_func_with_buf(root_key, func_bytes.data(), func_bytes.size(), output_result);
}

inline KModErr read_string_kernel_runtime_storage(const char* root_key, const char* app_name, const char* key_name, std::string& out_string) {
    int cap = 4096;
    std::vector<char> buf(cap);
    for (;;) {
        KModErr read_string_kernel_runtime_storage_with_buf(const char* root_key, const char* app_name, const char* key_name, char* out_buf, int out_buf_size);
        KModErr err = read_string_kernel_runtime_storage_with_buf(root_key, app_name, key_name, buf.data(), cap);
        if (is_ok(err)) {
            out_string.assign(buf.data());
            return KModErr::OK;
        }
        if (err != KModErr::ERR_MODULE_BUFFER_TOO_SMALL) {
            return err; // 其他错误直接返回
        }

        // 缓冲不够：翻倍扩容继续
        cap <<= 1;
        buf.resize(cap);
    }
}

inline KModErr install_kernel_function_before_hook(const char* root_key, uint64_t kernel_func_addr, const std::vector<uint8_t>& my_shellcode_func) {
    KModErr install_kfunc_before_hook_with_buf(const char* root_key, uint64_t hook_kernel_addr, const void *shellcode, uint32_t shellcode_len);
    return install_kfunc_before_hook_with_buf(root_key, kernel_func_addr, my_shellcode_func.data(), my_shellcode_func.size());
}

inline KModErr install_kernel_function_after_hook(const char* root_key, uint64_t kernel_func_addr, const std::vector<uint8_t>& my_shellcode_func) {
    KModErr install_kfunc_after_hook_with_buf(const char* root_key, uint64_t hook_kernel_addr, const void *shellcode, uint32_t shellcode_len);
    return install_kfunc_after_hook_with_buf(root_key, kernel_func_addr, my_shellcode_func.data(), my_shellcode_func.size());
}
}