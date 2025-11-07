#include <iostream>
#include "kernel_module_kit_umbrella.h"

using namespace asmjit;
using namespace asmjit::a64;
using namespace asmjit::a64::Predicate;

char g_root_key[256] = {0};

#define TEST_RETURN_VALUE 0x12345

KModErr run_kernel_shellcode(uint64_t & result) {
    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    
    //TODO: 从这里开始输入你的aarch64 asm指令

    kernel_module::arm64_module_asm_func_end(a, TEST_RETURN_VALUE);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;
    RETURN_IF_ERROR_KMOD(kernel_module::execute_kernel_asm_func(g_root_key, bytes, result));
    return KModErr::OK;
}

/* 
 * SKRoot模块入口函数（会在zygote64进程启动之前被执行。）
 * 自动入参： root_key          为 ROOT 密钥文本。
 * 		module_private_dir     为模块私有目录（受隐藏保护；需隐藏的文件请放此处）。
 */
int skroot_module_main(const char* root_key, const char* module_private_dir) {
    printf("[module_example] starting... \n");
    printf("[module_example] root_key len=%zu\n", strlen(root_key));
    printf("[module_example] module_private_dir=%s\n", module_private_dir);

    strncpy(g_root_key, root_key, sizeof(g_root_key) - 1);

    // 演示执行内核shellcode。
    uint64_t result = 0;
    printf("run_kernel_shellcode err: %zd\n", run_kernel_shellcode(result));
    printf(result == TEST_RETURN_VALUE ? "ok\n" : "failed\n");
    return (int)result;
}

// SKRoot 模块名片
SKROOT_MODULE_NAME("演示模块名称");
SKROOT_MODULE_VERSION("1.0.0");
SKROOT_MODULE_DESC("演示模块描述");
SKROOT_MODULE_AUTHOR("演示作者名字");
SKROOT_MODULE_UUID32("3608c9af28db4dcfc05c32bbc584753e");