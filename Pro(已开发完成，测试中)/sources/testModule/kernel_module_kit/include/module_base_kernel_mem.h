#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

namespace kernel_module {
        
enum class KernMemProt: uint8_t {
    KMP_RO,  // 只读区域
    KMP_RW,  // 可读写区域
    KMP_X    // 仅执行区域
};
/***************************************************************************
 * 申请内核内存
 * 参数: root_key       ROOT权限密钥文本
 *       size           欲分配内存大小
 *       out_kaddr      返回分配到的内核地址
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr alloc_kernel_mem(
        const char* root_key,
        uint32_t size,
        uint64_t& out_kaddr
);

/***************************************************************************
 * 申请内核内存并写入初始内容
 * 参数: root_key       ROOT权限密钥文本
 *       initial_bytes  申请与其长度相同的内核区域，并写入该字节序列
 *       out_kaddr      返回分配到的内核地址
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr alloc_kernel_mem(
        const char* root_key,
        const std::vector<uint8_t>& initial_bytes,
        uint64_t& out_kaddr
);


/***************************************************************************
 * 释放内核内存
 * 参数: root_key       ROOT权限密钥文本
 *       kaddr          内核地址
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr free_kernel_mem(
        const char* root_key,
        uint64_t    kaddr
);

/***************************************************************************
 * 读取内核内存
 * 参数: root_key       ROOT权限密钥文本
 *       kernel_addr    要读取的内核地址
 *       buf            本地缓冲区指针
 *       len            读取长度（字节数）
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr read_kernel_mem(
        const char* root_key,
        uint64_t    kernel_addr,
        void*       buf,
        uint32_t    len
);

/***************************************************************************
 * 写入内核内存
 * 参数: root_key       ROOT权限密钥文本
 *       kernel_addr    目标内核地址
 *       buf            本地数据缓冲区指针
 *       len            写入长度（字节数）
 *       prot           目标区域类型：
 *                      KMP_RW — 写入可读写区域
 *                      KMP_X  — 写入仅执行区域
* 备注: 当 prot 为 KMP_X（仅执行区域）且需要保证写入原子性时，
 *       len 必须 ≤ 4 字节。
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr write_kernel_mem(
        const char*  root_key,
        uint64_t     kernel_addr,
        const void*  buf,
        uint32_t     len,
        KernMemProt  prot = KernMemProt::KMP_RW
);

/***************************************************************************
 * 填充00到内核内存
 * 参数: root_key       ROOT权限密钥文本
 *       kernel_addr    目标内核地址
 *       buf            本地数据缓冲区指针
 *       len            写入长度（字节数）
 *       prot           目标区域类型：
 *                      KMP_RW — 写入可读写区域
 *                      KMP_X  — 写入仅执行区域
* 备注: 当 prot 为 KMP_X（仅执行区域）且需要保证写入原子性时，
 *       len 必须 ≤ 4 字节。
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr fill00_kernel_mem(
        const char*  root_key,
        uint64_t     kernel_addr,
        uint32_t     len,
        KernMemProt  prot = KernMemProt::KMP_RW
);

/***************************************************************************
 * 设置内核内存区域的保护属性
 * 参数: root_key — ROOT权限密钥字符串
 *      kernel_addr  — 起始虚拟地址（页对齐）
 *      mem_size     — 要修改的内存大小（字节）
 *      prot         — 目标保护属性: MEM_PROT_RW/RO/X
 * 返回: OK 表示成功；其它值为错误码
 ***************************************************************************/
KModErr set_kernel_memory_protection(
    const char*  root_key,
    uint64_t     kernel_addr,
    uint64_t     mem_size,
    KernMemProt  prot
);


/***************************************************************************
 * 获取内核虚拟起始地址
 * 参数: root_key     — ROOT权限密钥字符串
 *      result       — 输出参数，返回内核静态代码段（.text）的起始虚拟地址
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr get_kernel_virtual_mem_start_addr(
        const char*  root_key,
        uint64_t&    result
);

/***************************************************************************
 * 获取内存页大小
 * 返回: 内存页大小(字节)
 ***************************************************************************/
uint32_t get_page_size();

} // namespace kernel_module
