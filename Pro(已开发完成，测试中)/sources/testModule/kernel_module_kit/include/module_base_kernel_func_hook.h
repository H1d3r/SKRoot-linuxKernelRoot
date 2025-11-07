#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <errno.h>
#include "____DO_NOT_EDIT____/private_mod_api_runtime_helper.h"

namespace kernel_module {
/***************************************************************************
 * （内核函数执行前）HOOK函数入口
 * 参数： a 		ASMJIT 汇编器上下文
 ***************************************************************************/
void arm64_before_hook_start(asmjit::a64::Assembler* a);
/***************************************************************************
 * （内核函数执行前）HOOK函数结束：可选择是否跳回原函数
 * 参数： a 					ASMJIT 汇编器上下文
 *  - continue_original = true  ：恢复现场并跳回原函数（从被覆盖指令后继续执行）
 *  - continue_original = false ：直接生成 RET，不再执行原函数，返回到调用者
 ***************************************************************************/
void arm64_before_hook_end(asmjit::a64::Assembler* a, bool continue_original);

/***************************************************************************
 * 安装内核钩子（在内核函数执行前）
 * 参数: root_key			ROOT权限密钥文本
 *   	kernel_func_addr	欲安装Hook的内核函数地址
 *   	my_shellcode_func	将被跳转执行的 Hook 函数 (HOOK函数必须在开头调用 arm64_before_hook_start，结尾调用 arm64_before_hook_end，否则不合法)。
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr install_kernel_function_before_hook(
	const char*    				root_key,
	uint64_t       				kernel_func_addr,
	const std::vector<uint8_t>& my_shellcode_func
);


/***************************************************************************
 * （内核函数执行后）HOOK函数入口
 * 参数： a 		ASMJIT 汇编器上下文
 ***************************************************************************/
void arm64_after_hook_start(asmjit::a64::Assembler* a);

/***************************************************************************
 * （内核函数执行后）HOOK函数结束
 * 参数： a			ASMJIT 汇编器上下文
 ***************************************************************************/
void arm64_after_hook_end(asmjit::a64::Assembler* a);

/***************************************************************************
 * 安装内核钩子（在内核函数执行后）
 * 参数: root_key			ROOT权限密钥文本
 *   	kernel_func_addr	欲安装Hook的内核函数地址
 *   	shellcode    		将被跳转执行的 Hook 函数 (HOOK函数必须在开头调用 arm64_after_hook_start，结尾调用 arm64_after_hook_end，否则不合法)。
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr install_kernel_function_after_hook(
	const char*					root_key,
	uint64_t					kernel_func_addr,
	const std::vector<uint8_t>&	my_shellcode_func
);
}
