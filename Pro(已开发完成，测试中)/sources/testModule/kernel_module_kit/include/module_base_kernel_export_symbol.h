#pragma once
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include "aarch64_asm_arg.h"
#include "kernel_module_kit_umbrella.h"

/*
 * 这里封装了对内核导出符号（exported symbols）的 API 调用函数。可后续补充。
 * 主要目的是为了在汇编代码生成（asmjit AArch64）过程中，更方便地调用内核 API，而无需处理寻址细节。
 */

namespace kernel_module {
namespace export_symbol {
using namespace asmjit::a64;
// 原型：struct task_struct *get_current(void);，获取current指针并赋值到寄存器x，需判空，因为kthread无法获取
void get_current_to_reg(Assembler* a, GpX x);

// 原型：unsigned long copy_from_user(void *to, const void __user *from, unsigned long n)，返回值为X0寄存器
void copy_from_user(const char* root_key, Assembler* a, KModErr& out_err, GpX to, GpX __user_from, GpX n);
void copy_from_user(const char* root_key, Assembler* a, KModErr& out_err, GpX to, GpX __user_from, uint64_t n);

// 原型：unsigned long copy_to_user(void __user *to, const void *from, unsigned long n)，返回值为X0寄存器
void copy_to_user(const char* root_key, Assembler* a, KModErr& out_err, GpX __user_to, GpX from, GpX n);
void copy_to_user(const char* root_key, Assembler* a, KModErr& out_err, GpX __user_to, GpX from, uint64_t n);

// 原型：void printk(const char *fmt, ...); 无返回值
template <typename... Regs>
void printk(const char* root_key, Assembler* a, KModErr& out_err, const char *fmt, Regs... regs) {
    auto orig = mk_args(std::forward<Regs>(regs)...);
    void printk(const char* root_key, Assembler* a, KModErr& out_err, const char *fmt, const Arm64Arg* regs, int regs_count);
    printk(root_key, a, out_err, fmt, orig.data(), static_cast<int>(orig.size()));
}

// 原型：int fn(void *data, const char *name, struct module *mod, unsigned long addr)，设置末尾X0返回值非0代表终止遍历。
typedef void (*SymbolCb)(
    Assembler* a, // Assembler 对象，用于生成 ARM64 指令
    GpX data,     // 用户数据寄存器，对应 void* data
    GpX name_ptr, // 符号名指针寄存器，指向内核符号字符串
    GpX mod,      // 符号所属模块(struct module*)寄存器，可能为 NULL
    GpX addr      // 符号地址寄存器
);
// 原型: int kallsyms_on_each_symbol(int (*fn)(void * data, const char * name, struct module *mod, unsigned long addr), void *data)，返回值为X0寄存器
void kallsyms_on_each_symbol(const char* root_key, Assembler* a, KModErr& out_err, SymbolCb fn, GpX data);

// 原型: struct mm_struct *get_task_mm(struct task_struct *task)，返回值为X0寄存器
void get_task_mm(const char* root_key, Assembler* a, KModErr& out_err, GpX task);

// 原型: void mmput(struct mm_struct *mm)
void mmput(const char* root_key, Assembler* a, KModErr& out_err, GpX mm);

// 原型: int set_memory_ro(unsigned long addr, int numpages)，返回值为W0寄存器
void set_memory_ro(const char* root_key, Assembler* a, KModErr& out_err, GpX addr, GpW numpages);
void set_memory_ro(const char* root_key, Assembler* a, KModErr& out_err, uint64_t addr, uint32_t numpages);

// 原型: int set_memory_rw(unsigned long addr, int numpages)，返回值为W0寄存器
void set_memory_rw(const char* root_key, Assembler* a, KModErr& out_err, GpX addr, GpW numpages);
void set_memory_rw(const char* root_key, Assembler* a, KModErr& out_err, uint64_t addr, uint32_t numpages);

// 原型: int set_memory_nx(unsigned long addr, int numpages)，返回值为W0寄存器
void set_memory_nx(const char* root_key, Assembler* a, KModErr& out_err, GpX addr, GpW numpages);
void set_memory_nx(const char* root_key, Assembler* a, KModErr& out_err, uint64_t addr, uint32_t numpages);

// 原型: int set_memory_x(unsigned long addr, int numpages)，返回值为W0寄存器
void set_memory_x(const char* root_key, Assembler* a, KModErr& out_err, GpX addr, GpW numpages);
void set_memory_x(const char* root_key, Assembler* a, KModErr& out_err, uint64_t addr, uint32_t numpages);

enum class KmallocFlags : uint32_t {
    GFP_KERNEL = 0,        // 默认的内存分配标志
    GFP_ATOMIC = 1,        // 不允许阻塞的内存分配
    GFP_HIGHUSER = 2,      // 高端用户内存
    GFP_DMA = 4,           // 支持 DMA 的内存
    GFP_USER = 8,          // 用户空间内存
    GFP_RECLAIMABLE = 16,  // 可回收的内存
    GFP_ZERO = 32          // 分配内存并清零
};
// 原型：void * kmalloc(size_t size, gfp_t flags)，返回值为X0寄存器
void kmalloc(const char* root_key, Assembler* a, KModErr& out_err, GpX size, GpW flags);
void kmalloc(const char* root_key, Assembler* a, KModErr& out_err, GpX size, KmallocFlags flags);
void kmalloc(const char* root_key, Assembler* a, KModErr& out_err, uint64_t size, KmallocFlags flags);
// kmalloc (外部封装版本)：申请内核内存，结果写入 out_objp，返回值为OK代表执行成功
KModErr kmalloc(const char* root_key, uint64_t size, KmallocFlags flags, uint64_t& out_objp);

// 原型：void kfree(const void *objp)
void kfree(const char* root_key, Assembler* a, KModErr& out_err, GpX objp);
// kfree (外部封装版本)：释放内核内存，返回值为OK代表执行成功
KModErr kfree(const char* root_key, uint64_t objp);

namespace linux_above_6_12_0 {
enum class ExecmemTypes : uint32_t {
    EXECMEM_DEFAULT = 0,
	EXECMEM_MODULE_TEXT = EXECMEM_DEFAULT,
	EXECMEM_KPROBES,
	EXECMEM_FTRACE,
	EXECMEM_BPF,
	EXECMEM_MODULE_DATA,
	EXECMEM_TYPE_MAX,
};
// 原型：void *execmem_alloc(enum execmem_type type, size_t size)，返回值为X0寄存器
void execmem_alloc(const char* root_key, Assembler* a, KModErr& out_err, GpW type, GpX size);
void execmem_alloc(const char* root_key, Assembler* a, KModErr& out_err, ExecmemTypes type, uint64_t size);
// execmem_alloc (外部封装版本)：申请内核内存，结果写入 out_ptr，返回值为OK代表执行成功
KModErr execmem_alloc(const char* root_key, ExecmemTypes type, uint64_t size, uint64_t& out_ptr);

// 原型：void execmem_free(void *ptr);
void execmem_free(const char* root_key, Assembler* a, KModErr& out_err, GpX ptr);
// execmem_free (外部封装版本)：释放内核内存，返回值为OK代表执行成功
KModErr execmem_free(const char* root_key, uint64_t ptr);
}

namespace linux_older {
// 原型：void *module_alloc(unsigned long size)，返回值为X0寄存器
void module_alloc(const char* root_key, Assembler* a, KModErr& out_err, GpX size);
void module_alloc(const char* root_key, Assembler* a, KModErr& out_err, uint64_t size);
// module_alloc (外部封装版本)：申请内核内存，结果写入 out_module_region，返回值为OK代表执行成功
KModErr module_alloc(const char* root_key, uint64_t size, uint64_t& out_module_region);

// 原型：void module_memfree(void *module_region);
void module_memfree(const char* root_key, Assembler* a, KModErr& out_err, GpX module_region);
// module_memfree (外部封装版本)：释放内核内存，返回值为OK代表执行成功
KModErr module_memfree(const char* root_key, uint64_t module_region);
}


namespace linux_above_4_9_0 {
// 原型: int access_process_vm(struct task_struct *tsk, unsigned long addr, void *buf, int len, unsigned int gup_flags)，返回值为W0寄存器
void access_process_vm(const char* root_key, Assembler* a, KModErr& out_err, GpX tsk, GpX addr, GpX buf, GpW len, GpW gup_flags);
void access_process_vm(const char* root_key, Assembler* a, KModErr& out_err, GpX tsk, GpX addr, GpX buf, uint32_t len, GpW gup_flags);
}
namespace linux_older {
// 原型: int access_process_vm(struct task_struct *tsk, unsigned long addr, void *buf, int len, int write)，返回值为W0寄存器
void access_process_vm(const char* root_key, Assembler* a, KModErr& out_err, GpX tsk, GpX addr, GpX buf, GpW len, GpW write);
void access_process_vm(const char* root_key, Assembler* a, KModErr& out_err, GpX tsk, GpX addr, GpX buf, uint32_t len, GpW write);
}

void access_process_vm(const char* root_key, Assembler* a, KModErr& out_err, uint64_t addr, uint32_t insn);

// 原型: int __kprobes aarch64_insn_write(void *addr, u32 insn)，返回值为W0寄存器
void aarch64_insn_write(const char* root_key, Assembler* a, KModErr& out_err, GpX addr, GpW insn);

// 原型: int __kprobes aarch64_insn_patch_text(void *addrs[], u32 insns[], int cnt)，返回值为W0寄存器
void aarch64_insn_patch_text(const char* root_key, Assembler* a, KModErr & out_err, GpX addrs, GpX insns, GpW cnt);

enum class LookupFlags : uint32_t {
    LOOKUP_FOLLOW = 0x0001,	/* follow links at the end */
    LOOKUP_DIRECTORY = 0x0002,	/* require a directory */
    LOOKUP_AUTOMOUNT = 0x0004,  /* force terminal automount */
};
// 原型：int kern_path(const char *name, unsigned int flags, struct path *path)，返回值为W0寄存器
void kern_path(const char* root_key, Assembler* a, KModErr& out_err, GpX name, GpW flags, GpX path);
void kern_path(const char* root_key, Assembler* a, KModErr& out_err, GpX name, LookupFlags flags, GpX path);
void kern_path(const char* root_key, Assembler* a, KModErr& out_err, GpX name, LookupFlags flags, uint64_t path_buf_addr);

} // namespace export_symbol
} // namespace kernel_module