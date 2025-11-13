#include "kernel_module_kit_test.h"
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include "kernel_module_kit_umbrella.h"

using namespace asmjit;
using namespace asmjit::a64;
using namespace asmjit::a64::Predicate;


KModErr Test_kallsyms_lookup_name1() {
    uint64_t result_addr = 0;
    RETURN_IF_ERROR(kernel_module::kallsyms_lookup_name(g_root_key, "kernel_halt", result_addr));
    printf("kallsyms_lookup_name1 output addr: %p\n", (void*)result_addr);
    return KModErr::OK;
}

KModErr Test_kallsyms_lookup_name2() {
    kernel_module::SymbolHit hit;
    RETURN_IF_ERROR(kernel_module::kallsyms_lookup_name(g_root_key, "kernel_resta", kernel_module::SymbolMatchMode::Prefix, hit));
    printf("kallsyms_lookup_name2 hit name:%s, addr: %p\n", hit.name, (void*)hit.addr);
    return KModErr::OK;
}

KModErr Test_get_task_mm() {
    KModErr err = KModErr::ERR_MODULE_ASM;
    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    kernel_module::export_symbol::get_current_to_reg(a, x1);
	kernel_module::export_symbol::get_task_mm(g_root_key, a, err, x1);
	RETURN_IF_ERROR(err);
    kernel_module::export_symbol::mmput(g_root_key, a, err, x0);
    RETURN_IF_ERROR(err);
    kernel_module::arm64_module_asm_func_end(a, 0x1234);
    //std::cout << print_aarch64_asm(asm_info).c_str() << std::endl;
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t result = 0;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, result));
    printf("get_task_mm+mmput result: %s\n", result == 0x1234 ? "ok" : "failed");
    return KModErr::OK;
}

KModErr Test_printk() {
    KModErr err = KModErr::ERR_MODULE_ASM;
    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x0, 1234);
    kernel_module::export_symbol::printk(g_root_key, a, err, "[!!!] my_printk:%d\n", x0);
    RETURN_IF_ERROR(err);
    kernel_module::arm64_module_asm_func_end(a, 0x4567);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t result = 0;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, result));
    printf("printk result: %s\n", result == 0x4567 ? "ok" : "failed");
    return KModErr::OK;
}

KModErr Test_copy_from_user() {
    uint64_t addr = 0;
    std::vector<uint8_t> empty(1024);
    RETURN_IF_ERROR(kernel_module::alloc_kernel_mem(g_root_key, empty, addr));
    const char * str = "abc123456789";
    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x1, addr);
    aarch64_asm_mov_x(a, x2, (uint64_t)str);
    aarch64_asm_mov_x(a, x3, 4);

    KModErr err = KModErr::ERR_MODULE_ASM;
    kernel_module::export_symbol::copy_from_user(g_root_key, a, err, x1, x2, x3);
    RETURN_IF_ERROR(err);
    kernel_module::arm64_module_asm_func_end(a, x0);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t ret;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, ret));
    if(ret == 0) {
        std::vector<uint8_t> buf(5);
        RETURN_IF_ERROR(kernel_module::read_kernel_mem(g_root_key, addr, buf.data(), buf.size()));
        buf.push_back(0);
        std::string s(reinterpret_cast<const char*>(buf.data()));
        printf("copy_from_user result: %s\n", s == "abc1" ? "ok" : "failed");
    } else {
        printf("copy_from_user result: failed\n");
    }
    return KModErr::OK;
}

KModErr Test_copy_to_user() {
    KModErr err = KModErr::ERR_MODULE_ASM;
    char str[] = "123456789";
    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x1, (uint64_t)str);
    aarch64_asm_set_x_cstr_ptr(a, x2, "abc1");
    aarch64_asm_mov_x(a, x3, 5);
    kernel_module::export_symbol::copy_to_user(g_root_key, a, err, x1, x2, x3);
    RETURN_IF_ERROR(err);
    kernel_module::arm64_module_asm_func_end(a, x0);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t r = 0;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, r));
    printf("copy_to_user result: %s\n", std::string(str) == "abc1" ? "ok" : "failed");
    return KModErr::OK;
}

KModErr Test_kmalloc1() {
    using namespace kernel_module::export_symbol;
    KModErr err = KModErr::ERR_MODULE_ASM;

    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x1, 128);
    kernel_module::export_symbol::kmalloc(g_root_key, a, err, x1, KmallocFlags::GFP_KERNEL);
    RETURN_IF_ERROR(err);
    kernel_module::arm64_module_asm_func_end(a, x0);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t ret;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, ret));
    if(ret != 0) {
        RETURN_IF_ERROR(kernel_module::write_kernel_mem(g_root_key, ret, (void*)"123456789aaa", sizeof("123456789aaa")));
        std::vector<uint8_t> buf(sizeof("123456789aaa"));
        RETURN_IF_ERROR(kernel_module::read_kernel_mem(g_root_key, ret, buf.data(), buf.size()));
        buf.push_back(0);
        std::string s(reinterpret_cast<const char*>(buf.data()));
        printf("kmalloc1 result: %s\n", s == "123456789aaa" ? "ok" : "failed");
    } else {
        printf("kmalloc1 result: failed\n");
    }
    return KModErr::OK;
}

KModErr Test_kfree1() {
    using namespace kernel_module::export_symbol;
    KModErr err = KModErr::ERR_MODULE_ASM;

    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x1, 128);
    kernel_module::export_symbol::kmalloc(g_root_key, a, err, x1, KmallocFlags::GFP_KERNEL);
    kernel_module::export_symbol::kfree(g_root_key, a, err, x0);
    RETURN_IF_ERROR(err);
    kernel_module::arm64_module_asm_func_end(a);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t result = 0;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, result));
    printf("kfree1 result: ok\n");
    return KModErr::OK;
}

KModErr Test_kmalloc2() {
    using namespace kernel_module::export_symbol;
    uint64_t addr = 0;
    RETURN_IF_ERROR(kernel_module::export_symbol::kmalloc(g_root_key, 1024, KmallocFlags::GFP_KERNEL, addr));
    printf("kmalloc2 addr: %p\n", (void*)addr);
    printf("kmalloc2 result: %s\n", addr ? "ok" : "failed");
    return KModErr::OK;
}

KModErr Test_kfree2() {
    using namespace kernel_module::export_symbol;
    uint64_t addr = 0;
    RETURN_IF_ERROR(kernel_module::export_symbol::kmalloc(g_root_key, 1024, KmallocFlags::GFP_KERNEL, addr));
    RETURN_IF_ERROR(kernel_module::export_symbol::kfree(g_root_key, addr));
    printf("kfree2 result: ok\n");
    return KModErr::OK;
}

KModErr Test_module_alloc1() {
    KModErr err = KModErr::ERR_MODULE_ASM;

    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x1, 128);
    kernel_module::export_symbol::linux_older::module_alloc(g_root_key, a, err, x1);
    RETURN_IF_ERROR(err);
    kernel_module::arm64_module_asm_func_end(a, x0);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t ret;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, ret));

    if(ret != 0) {
        RETURN_IF_ERROR(kernel_module::write_kernel_mem(g_root_key, ret, (void*)"123456789aaa", sizeof("123456789aaa")));
        std::vector<uint8_t> buf(sizeof("123456789aaa"));
        RETURN_IF_ERROR(kernel_module::read_kernel_mem(g_root_key, ret, buf.data(), buf.size()));
        buf.push_back(0);
        std::string s(reinterpret_cast<const char*>(buf.data()));
        printf("module_alloc1 result: %s\n", s == "123456789aaa" ? "ok" : "failed");
    } else {
        printf("module_alloc2 result: failed\n");
    }
    return KModErr::OK;
}

KModErr Test_module_memfree1() {
    KModErr err = KModErr::ERR_MODULE_ASM;
    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    aarch64_asm_mov_x(a, x1, 128);
    kernel_module::export_symbol::linux_older::module_alloc(g_root_key, a, err, x1);
    RETURN_IF_ERROR(err);
    kernel_module::export_symbol::linux_older::module_memfree(g_root_key, a, err, x0);
    RETURN_IF_ERROR(err);
    kernel_module::arm64_module_asm_func_end(a);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;
    uint64_t result = 0;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, result));
    printf("module_memfree1 result: ok\n");
    return KModErr::OK;
}

KModErr Test_module_alloc2() {
    uint64_t addr = 0;
    RETURN_IF_ERROR(kernel_module::export_symbol::linux_older::module_alloc(g_root_key, 1024, addr));
    printf("module_alloc2 addr: %p\n", (void*)addr);
    printf("module_alloc2 result: %s\n", addr ? "ok" : "failed");
    return KModErr::OK;
}

KModErr Test_module_memfree2() {
    uint64_t addr = 0;
    RETURN_IF_ERROR(kernel_module::export_symbol::linux_older::module_alloc(g_root_key, 1024, addr));
    RETURN_IF_ERROR(kernel_module::export_symbol::linux_older::module_memfree(g_root_key, addr));
    printf("module_memfree2 result: ok\n");
    return KModErr::OK;
}


static void emit_cb(Assembler* a, GpX data, GpX name_ptr, GpX mod, GpX addr) {
    KModErr err = KModErr::OK;
    kernel_module::export_symbol::printk(g_root_key, a, err, "[!!!!] kallsyms_symbol:%s, addr:%llx\n", name_ptr, addr);
    a->mov(x0, xzr); // continue next
}

KModErr Test_kallsyms_on_each_symbol1() {
    KModErr err = KModErr::OK;
    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    kernel_module::export_symbol::kmalloc(g_root_key, a, err, 1, kernel_module::export_symbol::KmallocFlags::GFP_KERNEL);
    RETURN_IF_ERROR(err);
    a->mov(x11, x0);
    a->strb(wzr, ptr(x11));

    kernel_module::export_symbol::kallsyms_on_each_symbol(g_root_key, a, err, emit_cb, x11);
    RETURN_IF_ERROR(err);
    kernel_module::export_symbol::kfree(g_root_key, a, err, x11);
    RETURN_IF_ERROR(err);
    kernel_module::arm64_module_asm_func_end(a, x0);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;
    uint64_t result = 0;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, result));
    printf("kallsyms_on_each_symbol1 result: ok\n");
    return KModErr::OK;
}


struct TLSKsymState2 {
	const char* symbol_name_buf = nullptr;
};

static thread_local TLSKsymState2 g_tls_ksym2;
static void emit_cb_tls2(Assembler* a, GpX data, GpX name_ptr, GpX mod, GpX addr) {
    KModErr err = KModErr::OK;
    IdleRegPool pool = IdleRegPool::Make(data, name_ptr, mod, addr);
    GpX xMyKeyName = pool.acquireX();
    GpX xCopyTo = pool.acquireX();
    GpX xResult = pool.acquireX();
    GpX xLen = pool.acquireX();
    GpW wFlag = pool.acquireW();

    RegProtectGuard g1(RegProtectGuard::SkipX0::Yes, a, pool.get_used());

    Label L_end = a->newLabel();
    Label L_continue_next = a->newLabel();

    a->mov(xResult, Imm(1)); // default stop
    a->ldrb(wFlag, ptr(data));
    a->cbnz(wFlag, L_end);
    
    aarch64_asm_set_x_cstr_ptr(a, xMyKeyName, "kernel_resta");
    kernel_module::string_ops::kstartswith(a, name_ptr, xMyKeyName);
    a->cbz(x0, L_continue_next);
    
    kernel_module::string_ops::kstrlen(a, name_ptr);
    a->mov(xLen, x0);
    aarch64_asm_mov_x(a, xCopyTo, (uint64_t)g_tls_ksym2.symbol_name_buf);
    kernel_module::export_symbol::copy_to_user(g_root_key, a, err, xCopyTo, name_ptr, xLen);

    a->mov(wFlag, Imm(1));
    a->strb(wFlag, ptr(data));
    a->b(L_end);

    a->bind(L_continue_next);
    a->mov(xResult, xzr);
    
    a->bind(L_end);
    a->mov(x0, xResult);
}

KModErr Test_kallsyms_on_each_symbol2() {
    char symbol_name_buf[1024] = {0};
    KModErr err = KModErr::OK;

    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);
    kernel_module::export_symbol::kmalloc(g_root_key, a, err, 1, kernel_module::export_symbol::KmallocFlags::GFP_KERNEL);
    RETURN_IF_ERROR(err);
    a->mov(x11, x0);
    a->strb(wzr, ptr(x11));

    memset(&g_tls_ksym2, 0, sizeof(g_tls_ksym2));
    g_tls_ksym2.symbol_name_buf = symbol_name_buf;
    kernel_module::export_symbol::kallsyms_on_each_symbol(g_root_key, a, err, emit_cb_tls2, x11);

    RETURN_IF_ERROR(err);
    kernel_module::export_symbol::kfree(g_root_key, a, err, x11);
    RETURN_IF_ERROR(err);
    kernel_module::arm64_module_asm_func_end(a, x0);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t r;
    err = kernel_module::execute_kernel_asm_func(g_root_key, bytes, r);
    RETURN_IF_ERROR(err);

    std::string_view hay{symbol_name_buf};
    std::string_view needle{"kernel_resta"};
    bool ok = hay.size() >= needle.size() && hay.compare(0, needle.size(), needle) == 0;
    printf("kallsyms_on_each_symbol2 name: %s\n", std::string(symbol_name_buf).c_str());
    printf("kallsyms_on_each_symbol2 result: %s\n", ok ? "ok" : "failed");
    return KModErr::OK;
}

KModErr Test_kern_path() {
    using LookupFlags = kernel_module::export_symbol::LookupFlags;
    uint64_t path_buf_addr = 0;
    RETURN_IF_ERROR(kernel_module::export_symbol::linux_older::module_alloc(g_root_key, 0x1000, path_buf_addr));

    aarch64_asm_info asm_info = init_aarch64_asm();
    auto a = asm_info.a.get();
    kernel_module::arm64_module_asm_func_start(a);

    aarch64_asm_set_x_cstr_ptr(a, x0, "/data/bhuvsUdRKoCLH3OB/init");

    KModErr err = KModErr::ERR_MODULE_ASM;
    kernel_module::export_symbol::kern_path(g_root_key, a, err, x0, LookupFlags::LOOKUP_FOLLOW, path_buf_addr);
    RETURN_IF_ERROR(err);

    kernel_module::arm64_module_asm_func_end(a, x0);
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(asm_info);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;

    uint64_t result = 0;
    RETURN_IF_ERROR(kernel_module::execute_kernel_asm_func(g_root_key, bytes, result));
    printf("kern_path output result: %p\n", (void*)result);
    return KModErr::OK;
}