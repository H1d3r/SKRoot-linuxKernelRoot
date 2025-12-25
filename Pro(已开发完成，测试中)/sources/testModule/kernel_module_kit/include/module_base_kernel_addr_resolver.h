#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

namespace kernel_module {

enum class SymbolMatchMode : uint8_t {
    Exact,      // 完全匹配
    Prefix,     // 以 name 开头
    Suffix,     // 以 name 结尾
    Substring   // name 出现在任意位置
};
struct SymbolHit {
    uint64_t addr = 0;     // 符号地址
    char name[1024] = {0}; // 命中的符号名
};

/***************************************************************************
 * 根据符号名查找内核符号地址
 * 参数: root_key     ROOT权限密钥文本
 *       name         要查找的内核符号名称（以 '\\0' 结尾的字符串）
 *       mode         符号匹配模式
 *       out          输出参数，返回找到的符号地址
 * 返回: OK 表示成功找到符号；其他值为错误码
 ***************************************************************************/
KModErr kallsyms_lookup_name(const char* root_key, const char * name, SymbolMatchMode mode, SymbolHit & out);

inline KModErr kallsyms_lookup_name(const char* root_key, const char * name, uint64_t & out) {
    SymbolHit hit = {0};
    KModErr err = kallsyms_lookup_name(root_key, name, SymbolMatchMode::Exact, hit);
    out = hit.addr;
    return err;
}

/***************************************************************************
 * 获取sys_call_table内核地址
 * 参数: root_key                   ROOT权限密钥字符串
 *      func_start_addr             输出sys_call_table内核地址
 * 返回: OK 表示成功
 ***************************************************************************/
inline KModErr get_sys_call_table_kaddr(const char* root_key, uint64_t & kaddr) {
    return kallsyms_lookup_name(root_key, "sys_call_table", kaddr);
}

/***************************************************************************
 * 获取指定系统调用号（NR）的内核函数地址（从 sys_call_table 读取）
 *
 * 参数:
 *   root_key                ROOT 权限密钥字符串
 *   syscall_nr              系统调用号（__NR_xxx）
 *   out_syscall_func_kaddr  [输出] sys_call_table[syscall_nr] 中的函数指针地址
 *
 * 返回:
 *   OK 表示成功；其他值为错误码
 ***************************************************************************/
inline KModErr get_syscall_func_kaddr(const char* root_key, int syscall_nr, uint64_t& out_syscall_func_kaddr) {
    uint64_t table_kaddr  = 0;
    RETURN_IF_ERROR(get_sys_call_table_kaddr(root_key, table_kaddr));
    return read_kernel_mem(root_key, table_kaddr + 8 * (uint64_t)syscall_nr, &out_syscall_func_kaddr, sizeof(out_syscall_func_kaddr));
}

/***************************************************************************
 * 获取avc_denied内核地址
 * 参数: root_key                   ROOT权限密钥字符串
 *      func_entry_kaddr            输出avc_denied内核地址
 *      before_ret_can_hook_addr    输出avc_denied return前可安装HOOK的地址
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr get_avc_denied_kaddr(const char* root_key, uint64_t & func_entry_kaddr, uint64_t & before_ret_can_hook_kaddr);

}
