#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "kernel_module_kit_umbrella.h"

#include "____DO_NOT_EDIT____/private_mod_api_runtime_helper.h"
namespace kernel_module {
// arm64模块汇编函数入口：保存调用函数上下文
void arm64_module_asm_func_start(asmjit::a64::Assembler* a);

// arm64模块函数结束：恢复调用函数上下文（可设置返回值）
void arm64_module_asm_func_end(asmjit::a64::Assembler* a);
void arm64_module_asm_func_end(asmjit::a64::Assembler* a, uint64_t return_value);
void arm64_module_asm_func_end(asmjit::a64::Assembler* a, asmjit::a64::GpX x);

/***************************************************************************
 * 在内核态执行 AArch64 汇编函数
 * 参数: root_key       ROOT权限密钥文本
 *      func_bytes      待执行的内核汇编函数机器码 (必须在开头调用 arm64_module_asm_func_start，结尾调用 arm64_module_asm_func_end，否则不合法)。
 *      output_result   [输出] 函数返回的 64 位结果。仅当返回 KModErr::OK 时有效。
 *
 * 返回: OK 执行成功，output_result 已填充
 ***************************************************************************/
KModErr execute_kernel_asm_func(
                const char* root_key,
                const std::vector<uint8_t>& func_bytes,
                uint64_t& output_result
);

}