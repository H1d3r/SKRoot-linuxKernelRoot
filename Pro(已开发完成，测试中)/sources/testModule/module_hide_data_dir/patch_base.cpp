#include "patch_base.h"
using namespace asmjit;
using namespace asmjit::a64;
using namespace asmjit::a64::Predicate;

PatchBase::PatchBase(const char* root_key) : m_root_key(root_key) {}

PatchBase::PatchBase(const PatchBase& other) : m_root_key(other.m_root_key) {}

PatchBase::~PatchBase() {}

std::string PatchBase::get_root_key() {
	return m_root_key;
}

KModErr PatchBase::patch_kernel_before_hook(uint64_t kaddr, const Assembler* a) {
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(a);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;
	RETURN_IF_ERROR(kernel_module::install_kernel_function_before_hook(m_root_key.c_str(), kaddr, bytes));
	return KModErr::OK;
}

KModErr PatchBase::patch_kernel_after_hook(uint64_t kaddr, const Assembler* a) {
	std::vector<uint8_t> bytes = aarch64_asm_to_bytes(a);
    if (!bytes.size()) return KModErr::ERR_MODULE_ASM;
	RETURN_IF_ERROR(kernel_module::install_kernel_function_after_hook(m_root_key.c_str(), kaddr, bytes));
	return KModErr::OK;
}