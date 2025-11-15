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

KModErr kallsyms_lookup_name(
        const char* root_key,
        const char * name,
        SymbolMatchMode mode,
        SymbolHit   & out
);
inline KModErr kallsyms_lookup_name(
        const char* root_key,
        const char * name,
        uint64_t   & out
) {
    SymbolHit hit = {0};
    KModErr err = kallsyms_lookup_name(root_key, name, SymbolMatchMode::Exact, hit);
    out = hit.addr;
    return err;
}

/***************************************************************************
 * 获取avc_denied函数地址
 * 参数: root_key                   ROOT权限密钥字符串
 *      func_start_addr             输出avc_denied起始虚拟地址
 *      before_ret_can_hook_addr    输出avc_denied return前可安装HOOK的地址
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr get_avc_denied_addr(
    const char* root_key,
    uint64_t&   func_start_addr,
    uint64_t&   before_ret_can_hook_addr
);

}
