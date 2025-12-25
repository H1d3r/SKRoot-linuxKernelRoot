

#pragma once
#include "kernel_module_kit_umbrella.h"
#include "aarch64_kernel_sym_call_helper.h"


/***************************************************************************
 * 这是 module_base_kernel_export_symbol.h 的实现部分
 ***************************************************************************/
namespace kernel_module {
namespace export_symbol {
using namespace asmjit;
using namespace asmjit::a64;
using namespace asmjit::a64::Predicate;
using CallHelper = Aarch64KernelSymCallHelper;
using NeedReturnX0 = CallHelper::NeedReturnX0;

inline void get_current(Assembler* a, GpX x) {
	struct thread_info {
		uint64_t flags;		/* low level flags */
		uint64_t addr_limit;	/* address limit */
	};
	Label label_error = a->newLabel();
	uint32_t sp_el0_id = SysReg::encode(3, 0, 4, 1, 0);
	a->mrs(x, sp_el0_id);
	if (!kernel_module::is_CONFIG_THREAD_INFO_IN_TASK()) {
		a->cbz(x, label_error);
		a->and_(x, x, Imm((uint64_t)~(0x4000 - 1)));
		a->ldr(x, ptr(x, sizeof(thread_info)));
		a->bind(label_error);
	}
}

static const std::vector<std::string> copy_from_user_symbol_names = {
	"__arch_copy_from_user",	// Android 10+ / Kernel 4.14+ (GKI)
	"__copy_from_user",			// Android 7-9 / Kernel 3.18-4.9
	"_copy_from_user",			// 老内核 fallback (通常无 LTO，可用)
	"arm64_copy_from_user",		// 极少数异类
	"copy_from_user",			// 最后倔强
};

static const std::vector<std::string> copy_to_user_symbol_names = {
	"__arch_copy_to_user",	// Android 10+ / Kernel 4.14+ (GKI)
	"__copy_to_user",		// Android 7-9 / Kernel 3.18-4.9
	"_copy_to_user",		// 老内核 fallback (通常无 LTO，可用)
	"arm64_copy_to_user",	// 极少数异类
	"copy_to_user",			// 最后倔强
};

inline void copy_from_user(const char* root_key, Assembler* a, KModErr & out_err, GpX to, GpX __user_from, GpX n) {
	CallHelper::callSymbolCandidates(root_key, a, out_err,
		copy_from_user_symbol_names,
		NeedReturnX0::Yes,
		to, __user_from, n
	);
}

inline void copy_from_user(const char* root_key, Assembler* a, KModErr & out_err, GpX to, GpX __user_from, uint64_t n) {
	IdleRegPool pool = IdleRegPool::make(to, __user_from);
	GpX xN = pool.acquireX();

	RegProtectGuard g1(a, excluding_x0(pool.getUsed()));

	aarch64_asm_mov_x(a, xN, n);
	CallHelper::callSymbolCandidates(root_key, a, out_err,
		copy_from_user_symbol_names,
		NeedReturnX0::Yes,
		to, __user_from, xN
	);
}

inline void copy_to_user(const char* root_key, Assembler* a, KModErr & out_err, GpX __user_to, GpX from, GpX n) {
	CallHelper::callSymbolCandidates(root_key, a, out_err,
		copy_to_user_symbol_names,
		NeedReturnX0::Yes,
        __user_to, from, n
    );
}

inline void copy_to_user(const char* root_key, Assembler* a, KModErr & out_err, GpX __user_to, GpX from, uint64_t n) {
	IdleRegPool pool = IdleRegPool::make(__user_to, from);
	GpX xN = pool.acquireX();

	RegProtectGuard g1(a, excluding_x0(pool.getUsed()));

	aarch64_asm_mov_x(a, xN, n);
	CallHelper::callSymbolCandidates(root_key, a, out_err,
		copy_to_user_symbol_names,
		NeedReturnX0::Yes,
        __user_to, from, xN
    );
}

inline void printk(const char* root_key, Assembler* a, KModErr & out_err, const char* fmt, const Arm64Arg* regs, int regs_count) {
	std::vector<Arm64Arg> __regs_vec(regs, regs + regs_count);
	IdleRegPool pool = IdleRegPool::makeFromVec(__regs_vec);
	GpX xFmt = pool.acquireX();
	RegProtectGuard g1(a, xFmt);
	aarch64_asm_set_x_cstr_ptr(a, xFmt, fmt);
	std::vector<Arm64Arg> new_regs = __regs_vec;
	new_regs.insert(new_regs.begin(), to_arg(xFmt));
	if(kernel_module::is_kernel_version_less("5.15.0")) {
		out_err = CallHelper::callNameAuto(root_key, a, "printk", NeedReturnX0::No, new_regs);
	} else {
		out_err = CallHelper::callNameAuto(root_key, a, "_printk", NeedReturnX0::No, new_regs);
	}
}

inline void kallsyms_on_each_symbol(const char* root_key, Assembler* a, KModErr & out_err, SymbolCb fn, GpX data) {
	out_err = KModErr::ERR_MODULE_ASM;
	std::vector<uint8_t> callback_code;
	{
		aarch64_asm_ctx asm_ctx = init_aarch64_asm();
		auto b = asm_ctx.assembler();
		aarch64_asm_bit_c(b);
		b->mov(x10, x0);
		b->mov(x11, x1);
		b->mov(x12, x2);
		b->mov(x13, x3);
		fn(b, x10, x11, x12, x13);
		b->ret(x30);
		callback_code = aarch64_asm_to_bytes(a);
		if (callback_code.size() == 0) {
			out_err = KModErr::ERR_MODULE_ASM;
			return;
		}
	}
	IdleRegPool pool = IdleRegPool::make(data);
	GpX xCallback = pool.acquireX();

	RegProtectGuard g1(a, excluding_x0(pool.getUsed()));

	aarch64_asm_set_x_data_ptr(a, xCallback, callback_code);
	std::vector<Arm64Arg> new_regs;
	new_regs.push_back(to_arg(xCallback));
	new_regs.push_back(to_arg(data));
	out_err = CallHelper::callNameAuto(root_key, a, "kallsyms_on_each_symbol", NeedReturnX0::Yes, new_regs);
}

inline void get_task_mm(const char* root_key, Assembler* a, KModErr & out_err, GpX task) {
    out_err = CallHelper::callNameAuto(root_key, a, "get_task_mm", NeedReturnX0::Yes, task);
}

inline void mmput(const char* root_key, Assembler* a, KModErr & out_err, GpX mm) {
    out_err = CallHelper::callNameAuto(root_key, a, "mmput", NeedReturnX0::No, mm);
}

inline void set_memory_ro(const char* root_key, Assembler* a, KModErr & out_err, GpX addr, GpW numpages) {
    out_err = CallHelper::callNameAuto(root_key, a, "set_memory_ro", NeedReturnX0::Yes, addr, numpages);
}

inline void set_memory_ro(const char* root_key, Assembler* a, KModErr & out_err, uint64_t addr, uint32_t numpages) {
    out_err = CallHelper::callNameAuto(root_key, a, "set_memory_ro", NeedReturnX0::Yes, addr, numpages);
}

inline void set_memory_rw(const char* root_key, Assembler* a, KModErr & out_err, GpX addr, GpW numpages) {
    out_err = CallHelper::callNameAuto(root_key, a, "set_memory_rw", NeedReturnX0::Yes, addr, numpages);
}

inline void set_memory_rw(const char* root_key, Assembler* a, KModErr & out_err, uint64_t addr, uint32_t numpages) {
    out_err = CallHelper::callNameAuto(root_key, a, "set_memory_rw", NeedReturnX0::Yes, addr, numpages);
}

inline void set_memory_nx(const char* root_key, Assembler* a, KModErr & out_err, GpX addr, GpW numpages) {
    out_err = CallHelper::callNameAuto(root_key, a, "set_memory_nx", NeedReturnX0::Yes, addr, numpages);
}

inline void set_memory_nx(const char* root_key, Assembler* a, KModErr & out_err, uint64_t addr, uint32_t numpages) {
    out_err = CallHelper::callNameAuto(root_key, a, "set_memory_nx", NeedReturnX0::Yes, addr, numpages);
}

inline void set_memory_x(const char* root_key, Assembler* a, KModErr & out_err, GpX addr, GpW numpages) {
    out_err = CallHelper::callNameAuto(root_key, a, "set_memory_x", NeedReturnX0::Yes, addr, numpages);
}

inline void set_memory_x(const char* root_key, Assembler* a, KModErr & out_err, uint64_t addr, uint32_t numpages) {
    out_err = CallHelper::callNameAuto(root_key, a, "set_memory_x", NeedReturnX0::Yes, addr, numpages);
}

inline void kmalloc(const char* root_key, Assembler* a, KModErr & out_err, GpX size, GpW flags) {
	CallHelper::callSymbolCandidates(root_key, a, out_err,
        (std::vector<std::string>{ "__kmalloc", "__kmalloc_noprof"/*Linux kernel 6.10*/ }),
		NeedReturnX0::Yes,
        size, flags
    );

}

inline void kmalloc(const char* root_key, Assembler* a, KModErr & out_err, GpX size, KmallocFlags flags) {
	IdleRegPool pool = IdleRegPool::make(size);
	GpW wGFP_FLAG = pool.acquireW();

	RegProtectGuard g1(a, excluding_x0(pool.getUsed()));

	aarch64_asm_mov_w(a, wGFP_FLAG, (uint32_t)flags);
	std::vector<Arm64Arg> new_regs;
	new_regs.push_back(to_arg(size));
	new_regs.push_back(to_arg(wGFP_FLAG));
	CallHelper::callSymbolCandidates(root_key, a, out_err,
        (std::vector<std::string>{ "__kmalloc", "__kmalloc_noprof"/*Linux kernel 6.10*/ }),
		NeedReturnX0::Yes,
        new_regs
    );
}

inline void kmalloc(const char* root_key, Assembler* a, KModErr & out_err, uint64_t size, KmallocFlags flags) {
	RegProtectGuard g1(a, x11, w12);
	aarch64_asm_mov_x(a, x11, size);
	aarch64_asm_mov_w(a, w12, (uint32_t)flags);
	std::vector<Arm64Arg> new_regs;
	new_regs.push_back(to_arg(x11));
	new_regs.push_back(to_arg(w12));
	CallHelper::callSymbolCandidates(root_key, a, out_err,
        (std::vector<std::string>{ "__kmalloc", "__kmalloc_noprof"/*Linux kernel 6.10*/ }),
		NeedReturnX0::Yes,
        new_regs
    );
}

inline KModErr kmalloc(const char* root_key, uint64_t size, KmallocFlags flags, uint64_t& out_objp) {
    out_objp = 0;
	KModErr out_err = KModErr::OK;
    aarch64_asm_ctx asm_ctx = init_aarch64_asm();
	auto a = asm_ctx.assembler();
	Label label_cycle = a->newLabel();
	arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x11, size);
    aarch64_asm_mov_w(a, w12, (uint32_t)flags);
	kmalloc(root_key, a, out_err, x11, w12);
	RETURN_IF_ERROR(out_err);
	arm64_module_asm_func_end(a, x0);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(a);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;
	RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(root_key, bytes, out_objp));
	return KModErr::OK;
}

inline void kfree(const char* root_key, Assembler* a, KModErr & out_err, GpX objp) {
    out_err = CallHelper::callNameAuto(root_key, a, "kfree", NeedReturnX0::No, objp);
}

inline KModErr kfree(const char* root_key, uint64_t objp) {
	KModErr out_err = KModErr::OK;
    aarch64_asm_ctx asm_ctx = init_aarch64_asm();
	auto a = asm_ctx.assembler();
	Label label_cycle = a->newLabel();
	arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x11, objp);
	kfree(root_key, a, out_err, x11);
	RETURN_IF_ERROR(out_err);
	arm64_module_asm_func_end(a);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(a);
	if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t return_value = 0;
	return kernel_module::execute_kernel_asm_func(root_key, bytes, return_value);
}

namespace linux_above_6_12_0 {
inline void execmem_alloc(const char* root_key, Assembler* a, KModErr& out_err, GpW type, GpX size) {
	if(kernel_module::is_kernel_version_less("6.12.0")) {
		out_err = KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;
		return;
	}
	out_err = CallHelper::callNameAuto(root_key, a, "execmem_alloc", NeedReturnX0::Yes, type, size);
}

inline void execmem_alloc(const char* root_key, Assembler* a, KModErr& out_err, ExecmemTypes type, uint64_t size) {
	if(kernel_module::is_kernel_version_less("6.12.0")) {
		out_err = KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;
		return;
	}
	RegProtectGuard g1(a, x11, x12);
	aarch64_asm_mov_w(a, w11, (uint32_t)type);
	aarch64_asm_mov_x(a, x12, size);
	std::vector<Arm64Arg> new_regs;
	new_regs.push_back(to_arg(w11));
	new_regs.push_back(to_arg(x12));
	out_err = CallHelper::callNameAuto(root_key, a, "execmem_alloc", NeedReturnX0::Yes, new_regs);
}

inline KModErr execmem_alloc(const char* root_key, ExecmemTypes type, uint64_t size, uint64_t& out_ptr) {
	out_ptr = 0;
	KModErr out_err = KModErr::OK;
    aarch64_asm_ctx asm_ctx = init_aarch64_asm();
	auto a = asm_ctx.assembler();
	Label label_cycle = a->newLabel();
	arm64_module_asm_func_start(a);
    aarch64_asm_mov_w(a, w11, (uint32_t)type);
    aarch64_asm_mov_x(a, x12, size);
	execmem_alloc(root_key, a, out_err, w11, x12);
	RETURN_IF_ERROR(out_err);
	arm64_module_asm_func_end(a, x0);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(a);
	if (!bytes.size()) return KModErr::ERR_MODULE_ASM;
	RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(root_key, bytes, out_ptr));
	return KModErr::OK;
}

inline void execmem_free(const char* root_key, Assembler* a, KModErr& out_err, GpX ptr) {
	if(kernel_module::is_kernel_version_less("6.12.0")) {
		out_err = KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;
		return;
	}
	out_err = CallHelper::callNameAuto(root_key, a, "execmem_free", NeedReturnX0::No, ptr);
}

inline KModErr execmem_free(const char* root_key, uint64_t ptr) {
	KModErr out_err = KModErr::OK;
    aarch64_asm_ctx asm_ctx = init_aarch64_asm();
	auto a = asm_ctx.assembler();
	Label label_cycle = a->newLabel();
	arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x11, ptr);
	export_symbol::linux_older::module_memfree(root_key, a, out_err, x11);
	RETURN_IF_ERROR(out_err);
	arm64_module_asm_func_end(a);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(a);
	if (!bytes.size()) return KModErr::ERR_MODULE_ASM;
    uint64_t return_value = 0;
	return kernel_module::execute_kernel_asm_func(root_key, bytes, return_value);
}
}

namespace linux_older {
inline void module_alloc(const char* root_key, Assembler* a, KModErr & out_err, GpX size) {
	if(!kernel_module::is_kernel_version_less("6.12.0")) {
		out_err = KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;
		return;
	}
	out_err = CallHelper::callNameAuto(root_key, a, "module_alloc", NeedReturnX0::Yes, size);
}

inline void module_alloc(const char* root_key, Assembler* a, KModErr & out_err, uint64_t size) {
	if(!kernel_module::is_kernel_version_less("6.12.0")) {
		out_err = KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;
		return;
	}
	RegProtectGuard g1(a, x11);
	aarch64_asm_mov_x(a, x11, size);
	std::vector<Arm64Arg> new_regs;
	new_regs.push_back(to_arg(x11));
	out_err = CallHelper::callNameAuto(root_key, a, "module_alloc", NeedReturnX0::Yes, new_regs);
}

inline KModErr module_alloc(const char* root_key, uint64_t size, uint64_t& out_module_region) {
    out_module_region = 0;
	KModErr out_err = KModErr::OK;
    aarch64_asm_ctx asm_ctx = init_aarch64_asm();
	auto a = asm_ctx.assembler();
	Label label_cycle = a->newLabel();
	arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x11, size);
	module_alloc(root_key, a, out_err, x11);
	RETURN_IF_ERROR(out_err);
	arm64_module_asm_func_end(a, x0);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(a);
	if (!bytes.size()) return KModErr::ERR_MODULE_ASM;
	RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(root_key, bytes, out_module_region));
	return KModErr::OK;
}

inline void module_memfree(const char* root_key, Assembler* a, KModErr & out_err, GpX module_region) {
	if(!kernel_module::is_kernel_version_less("6.12.0")) {
		out_err = KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;
		return;
	}
    out_err = CallHelper::callNameAuto(root_key, a, "module_memfree", NeedReturnX0::No, module_region);
}

inline KModErr module_memfree(const char* root_key, uint64_t module_region) {
	if(!kernel_module::is_kernel_version_less("6.12.0")) return KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;

	KModErr out_err = KModErr::OK;
    aarch64_asm_ctx asm_ctx = init_aarch64_asm();
	auto a = asm_ctx.assembler();
	Label label_cycle = a->newLabel();
	arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x11, module_region);
	module_memfree(root_key, a, out_err, x11);
	RETURN_IF_ERROR(out_err);
	arm64_module_asm_func_end(a);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(a);
	if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t return_value = 0;
	return kernel_module::execute_kernel_asm_func(root_key, bytes, return_value);
}
}

namespace linux_above_4_9_0 {
inline void access_process_vm(const char* root_key, Assembler* a, KModErr& out_err, GpX tsk, GpX addr, GpX buf, GpW len, GpW gup_flags) {
	if (is_kernel_version_less("4.9.0")) {
		out_err = KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;
		return;
	}
	out_err = CallHelper::callNameAuto(root_key, a, "access_process_vm", NeedReturnX0::Yes, tsk, addr, buf, len, gup_flags);
}
inline void access_process_vm(const char* root_key, Assembler* a, KModErr& out_err, GpX tsk, GpX addr, GpX buf, uint32_t len, GpW gup_flags) {
	IdleRegPool pool = IdleRegPool::make(tsk, addr, buf, gup_flags);
	GpW wLen = pool.acquireW();

	RegProtectGuard g1(a, excluding_x0(pool.getUsed()));

	aarch64_asm_mov_w(a, wLen, len);
	access_process_vm(root_key, a, out_err, tsk, addr, buf, wLen, gup_flags);
}
}

namespace linux_older {
inline void access_process_vm(const char* root_key, Assembler* a, KModErr& out_err, GpX tsk, GpX addr, GpX buf, GpW len, GpW write) {
	if (!is_kernel_version_less("4.9.0")) {
		out_err = KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;
		return;
	}
	out_err = CallHelper::callNameAuto(root_key, a, "access_process_vm", NeedReturnX0::Yes, tsk, addr, buf, len, write);
}
inline void access_process_vm(const char* root_key, Assembler* a, KModErr& out_err, GpX tsk, GpX addr, GpX buf, uint32_t len, GpW write) {
	IdleRegPool pool = IdleRegPool::make(tsk, addr, buf, write);
	GpW wLen = pool.acquireW();

	RegProtectGuard g1(a, excluding_x0(pool.getUsed()));

	aarch64_asm_mov_w(a, wLen, len);
	access_process_vm(root_key, a, out_err, tsk, addr, buf, wLen, write);
}
}

inline void aarch64_insn_write(const char* root_key, Assembler* a, KModErr & out_err, GpX addr, GpW insn) {
    out_err = CallHelper::callNameAuto(root_key, a, "aarch64_insn_write", NeedReturnX0::Yes, addr, insn);
}

inline void aarch64_insn_patch_text(const char* root_key, Assembler* a, KModErr & out_err, GpX addrs, GpX insns, GpW cnt) {
    out_err = CallHelper::callNameAuto(root_key, a, "aarch64_insn_patch_text", NeedReturnX0::Yes, addrs, insns, cnt);
}

inline void kern_path(const char* root_key, Assembler* a, KModErr & out_err, GpX name, GpW flags, GpX path) {
    out_err = CallHelper::callNameAuto(root_key, a, "kern_path", NeedReturnX0::Yes, name, flags, path);
}

inline void kern_path(const char* root_key, Assembler* a, KModErr & out_err, GpX name, LookupFlags flags, GpX path) {
	IdleRegPool pool = IdleRegPool::make(name, path);
	GpW wFlags = pool.acquireW();

	RegProtectGuard g1(a, excluding_x0(pool.getUsed()));

	aarch64_asm_mov_w(a, wFlags, (uint32_t)flags);
	kern_path(root_key, a, out_err, name, wFlags, path);
}

inline void kern_path(const char* root_key, Assembler* a, KModErr & out_err, GpX name, LookupFlags flags, uint64_t path_buf_addr) {
	IdleRegPool pool = IdleRegPool::make(name);
	GpW wFlags = pool.acquireW();
	GpX xPathBuf = pool.acquireX();

	RegProtectGuard g1(a, excluding_x0(pool.getUsed()));

	aarch64_asm_mov_w(a, wFlags, (uint32_t)flags);
	aarch64_asm_mov_x(a, xPathBuf, path_buf_addr);
	kern_path(root_key, a, out_err, name, wFlags, xPathBuf);
}

inline void find_get_pid(const char* root_key, Assembler* a, KModErr & out_err, GpW nr) {
    out_err = CallHelper::callNameAuto(root_key, a, "find_get_pid", NeedReturnX0::Yes, nr);
}

inline void put_pid(const char* root_key, Assembler* a, KModErr & out_err, GpX pid) {
    out_err = CallHelper::callNameAuto(root_key, a, "put_pid", NeedReturnX0::No, pid);
}

namespace linux_above_4_19_0 {
inline void pid_task(const char* root_key, Assembler* a, KModErr& out_err, GpX pid, GpW type) {
	if (is_kernel_version_less("4.19.0")) {
		out_err = KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;
		return;
	}
	out_err = CallHelper::callNameAuto(root_key, a, "pid_task", NeedReturnX0::Yes, pid, type);
}
inline void pid_task(const char* root_key, Assembler* a, KModErr& out_err, GpX pid, PidType type) {
	IdleRegPool pool = IdleRegPool::make(pid);
	GpW wType = pool.acquireW();

	RegProtectGuard g1(a, excluding_x0(pool.getUsed()));

	aarch64_asm_mov_w(a, wType, (uint32_t)type);
	pid_task(root_key, a, out_err, pid, wType);
}
}

namespace linux_older {
inline void pid_task(const char* root_key, Assembler* a, KModErr& out_err, GpX pid, GpW type) {
	if (!is_kernel_version_less("4.19.0")) {
		out_err = KModErr::ERR_MODULE_SYMBOL_NOT_MATCH_LINUX_VER;
		return;
	}
	out_err = CallHelper::callNameAuto(root_key, a, "pid_task", NeedReturnX0::Yes, pid, type);
}
inline void pid_task(const char* root_key, Assembler* a, KModErr& out_err, GpX pid, PidType type) {
	IdleRegPool pool = IdleRegPool::make(pid);
	GpW wType = pool.acquireW();

	RegProtectGuard g1(a, excluding_x0(pool.getUsed()));

	aarch64_asm_mov_w(a, wType, (uint32_t)type);
	pid_task(root_key, a, out_err, pid, wType);
}
}
} // namespace export_symbol
} // namespace kernel_module
