#pragma once
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <memory>

#define ASMJIT_STATIC
#include "asmjit2/core.h"
#include "asmjit2/a64.h"

/***************************************************************************
 * AArch64 / AsmJit 辅助工具集
 * 作用：
 *   - 对 AsmJit 的基础能力做一层轻量封装，便于在任意环境下快速生成
 *     AArch64 指令序列、转换为字节流，并补充若干常用的“裸指令编码”。
 ***************************************************************************/

/*************************************************************************
 * AsmJit 错误处理器：将错误输出到 stderr
 *************************************************************************/
class MyAsmJitErrorHandler : public asmjit::ErrorHandler {
public:
	void handleError(asmjit::Error err, const char* message, asmjit::BaseEmitter* origin) override {
		std::cerr << "[发生错误] " << asmjit::DebugUtils::errorAsString(err) << ": " << message << std::endl;
		isErr = true;
	}

	// 查询是否曾经发生过错误。
	bool isError() { return isErr; }
private:
	bool isErr = false;
};

/*************************************************************************
 * AArch64 汇编上下文
 *   - assembler() : 获取 AArch64 Assembler 指针
 *   - has_error() : 查询是否发生过 AsmJit 错误
 *************************************************************************/
struct aarch64_asm_ctx {
public:
	asmjit::a64::Assembler* assembler() noexcept { return a_.get(); }
	bool has_error() const noexcept { return err_ && err_->isError(); }
private:
	std::unique_ptr<asmjit::StringLogger>      logger_;
	std::unique_ptr<asmjit::CodeHolder>        codeHolder_;
	std::unique_ptr<MyAsmJitErrorHandler>      err_;
	std::unique_ptr<asmjit::a64::Assembler>    a_;
	friend aarch64_asm_ctx init_aarch64_asm();
};

/*************************************************************************
 * 初始化一个独立的 AArch64 汇编环境
 *************************************************************************/
inline aarch64_asm_ctx init_aarch64_asm() {
	aarch64_asm_ctx ctx;
	asmjit::Environment env(asmjit::Arch::kAArch64);
	ctx.codeHolder_ = std::make_unique<asmjit::CodeHolder>();
	ctx.codeHolder_->init(env);
	ctx.logger_ = std::make_unique<asmjit::StringLogger>();
	ctx.logger_->addFlags(asmjit::FormatFlags::kNone);
	ctx.codeHolder_->setLogger(ctx.logger_.get());
	ctx.err_ = std::make_unique<MyAsmJitErrorHandler>();
	ctx.a_ = std::make_unique<asmjit::a64::Assembler>(ctx.codeHolder_.get());
	ctx.a_->setErrorHandler(ctx.err_.get());
	return ctx;
}

/*************************************************************************
 * 将 AArch64 汇编机器码导出为字节数组
 *************************************************************************/
inline std::vector<uint8_t> aarch64_asm_to_bytes(const asmjit::a64::Assembler* a) {
	if (!a) return {};
	auto* err_handler = a->errorHandler();
	if (!err_handler) return {};
	auto* my_err_handler = static_cast<MyAsmJitErrorHandler*>(err_handler);
	if (my_err_handler->isError()) return {};
	asmjit::CodeHolder* code = a->code();
	if (!code) return {};
	asmjit::Section* sec = code->sectionById(0);
	if (!sec) return {};
	const asmjit::CodeBuffer& buf = sec->buffer();
	return { buf.data(), buf.data() + buf.size() };
}

/*************************************************************************
 * 生成一条 B 指令（相对 PC 跳转）
 * 参数：a       : Assembler 指针；
 * 		b_value : 以字节为单位的跳转偏移（从当前 PC 起算），必须为 4 的倍数，且偏移范围必须在 ±128MB 之内。
 * 返回：true 表示成功; false 表示失败。
 *************************************************************************/
static bool aarch64_asm_b(asmjit::a64::Assembler* a, int32_t b_value) {
	if (b_value % 4 != 0) {
		std::cout << "[发生错误] The B instruction offset must be a multiple of 4" << std::endl;
		return false;
	}
	int64_t imm26 = b_value >> 2;
	if (imm26 < -(1LL << 25) || imm26 >= (1LL << 25)) {
		std::cout << "[发生错误] B instruction offset exceeds ± 128MB range" << std::endl;
		return false;
	}
	int32_t offset = b_value >> 2;
	uint32_t instr = 0x14000000 | (offset & 0x03FFFFFF);
	a->embed((const uint8_t*)&instr, sizeof(instr));
	return true;
}

/*************************************************************************
 * 生成一条 BL 指令（相对 PC 跳转）
 * 参数：a       : Assembler 指针；
 * 		bl_value : 以字节为单位的跳转偏移（从当前 PC 起算），必须为 4 的倍数，且偏移范围必须在 ±128MB 之内。
 * 返回：true 表示成功; false 表示失败。
 *************************************************************************/
static bool aarch64_asm_bl_raw(asmjit::a64::Assembler* a, int32_t bl_value) {
	if (bl_value % 4 != 0) {
		std::cout << "[发生错误] The BL instruction offset must be a multiple of 4" << std::endl;
		return false;
	}
	int64_t imm26 = bl_value >> 2;
	if (imm26 < -(1LL << 25) || imm26 >= (1LL << 25)) {
		std::cout << "[发生错误] BL instruction offset exceeds ±128MB range" << std::endl;
		return false;
	}
	int32_t offset26 = bl_value >> 2;
	uint32_t instr = 0x94000000u | (static_cast<uint32_t>(offset26) & 0x03FFFFFFu);
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * 安全版本的 BL（在调用前后保护 x29 / x30，避免破坏调用者栈帧)。
 * 序列：
 *   stp x29, x30, [sp, #-16]!
 *   bl  label
 *   ldp x29, x30, [sp], #16
 ************************************************************************/
static void aarch64_asm_safe_bl(asmjit::a64::Assembler* a, asmjit::Label label) {
	a->stp(asmjit::a64::x29, asmjit::a64::x30, ptr(asmjit::a64::sp).pre(-16));
	a->bl(label);
	a->ldp(asmjit::a64::x29, asmjit::a64::x30, ptr(asmjit::a64::sp).post(16));
}

/*************************************************************************
 * 安全版本的 BLR（在调用前后保护 x29 / x30，避免破坏调用者栈帧)。
 * 序列：
 *   stp x29, x30, [sp, #-16]!
 *   blr x
 *   ldp x29, x30, [sp], #16
 ************************************************************************/
static void aarch64_asm_safe_blr(asmjit::a64::Assembler* a, asmjit::a64::GpX x) {
	a->stp(asmjit::a64::x29, asmjit::a64::x30, ptr(asmjit::a64::sp).pre(-16));
	a->blr(x);
	a->ldp(asmjit::a64::x29, asmjit::a64::x30, ptr(asmjit::a64::sp).post(16));
}

/*************************************************************************
 * 手工生成一条 ADR 指令
 * 通过在临时 CodeHolder 中构造一小段代码，计算并拷贝出 ADR 指令编码，再 embed 到目标 Assembler 中。
 * 参数：a       : Assembler 指针
 * 		x         : 目标 X 寄存器
 * 		adr_value : 字节偏移（以 4 对齐），范围 ±1MB。
 * 注意：这里采用 “临时汇编 + 抽取指令” 的方式，实际效果是等价于在当前 PC 附近生成一条 ADR x, label。
 ************************************************************************/
static bool aarch64_asm_adr_x(asmjit::a64::Assembler* a, asmjit::a64::GpX x, int32_t adr_value) {
	if (!a) return false;
	if (adr_value % 4 != 0) {
		std::cout << "[发生错误] The ADR instruction offset must be a multiple of 4" << std::endl;
		return false;
	}
	// ADR 立即数范围：±1MB（字节范围）
	if (adr_value < -(1 << 20) || adr_value >((1 << 20) - 1)) {
		std::cout << "[发生错误] ADR instruction offset exceeds ± 1MB range" << std::endl;
		return false;
	}
	aarch64_asm_ctx asm_ctx = init_aarch64_asm();
	asmjit::a64::Assembler* a2 = asm_ctx.assembler();
	if (!a2) return false;
	asmjit::Label label_start = a2->newLabel();
	const int32_t size = std::abs(adr_value);
	std::vector<uint8_t> filler(static_cast<size_t>(size), 0x00);
	if (adr_value > 0) {
		a2->adr(x, label_start);
		if (size) a2->embed(filler.data(), filler.size());
		a2->bind(label_start);
		a2->nop();
	} else {
		a2->bind(label_start);
		if (size) a2->embed(filler.data(), filler.size());
		a2->adr(x, label_start);
	}
	auto bytes = aarch64_asm_to_bytes(a2);
	if (bytes.empty()) return false;
	const size_t adr_off = (adr_value < 0) ? static_cast<size_t>(size) : 0;
	if (bytes.size() < adr_off + 4) return false;
	a->embed(bytes.data() + adr_off, 4);
	return true;
}

/*************************************************************************
 * 将一个 64bit 常量赋值 X 寄存器：
 * 参数：a		: Assembler 指针
 * 		x         : 目标 X 寄存器
 * 		value	 : 64bit 常量
 ************************************************************************/
static void aarch64_asm_mov_x(asmjit::a64::Assembler* a, asmjit::a64::GpX x, uint64_t value) {
	bool isInitialized = false;
	for (int idx = 0; idx < 4; ++idx) {
		uint16_t imm16 = (value >> (idx * 16)) & 0xFFFF;
		if (imm16 == 0)
			continue;

		if (!isInitialized) {
			a->movz(x, imm16, idx * 16);
			isInitialized = true;
		}
		else {
			a->movk(x, imm16, idx * 16);
		}
	}
	if (!isInitialized) {
		a->movz(x, 0, 0);
	}
}

/*************************************************************************
 * 将一个 32bit 常量赋值到 W 寄存器
 * 参数：a		: Assembler 指针；
 * 		x        : 目标 W 寄存器
 * 		value	 : 32bit 常量
 ************************************************************************/
static void aarch64_asm_mov_w(asmjit::a64::Assembler* a, asmjit::a64::GpW w, uint32_t value) {
	bool isInitialized = false;
	for (int idx = 0; idx < 2; ++idx) {
		uint16_t imm16 = (value >> (idx * 16)) & 0xFFFF;
		if (imm16 == 0)
			continue;

		if (!isInitialized) {
			a->movz(w, imm16, idx * 16);
			isInitialized = true;
		}
		else {
			a->movk(w, imm16, idx * 16);
		}
	}
	if (!isInitialized) {
		a->movz(w, 0, 0);
	}
}

/*************************************************************************
 * 在当前代码后方嵌入一块数据，并让 x 指向它
 * 参数：a		: Assembler 指针
 * 		x		: 目标 X 寄存器
 * 		data	: 数据集
 ************************************************************************/
static void aarch64_asm_set_x_data_ptr(asmjit::a64::Assembler* a, asmjit::a64::GpX x, const std::vector<uint8_t>& data) {
	auto align_up4 = [](size_t v) -> size_t {
		return (v + 3) & ~size_t(3);
		};

	const size_t n_raw = data.size();
	const size_t n_padded = align_up4(n_raw);

	std::vector<uint8_t> buf(n_padded, 0u);
	if (!data.empty()) {
		memcpy(buf.data(), data.data(), data.size());
	}

	asmjit::Label label_entry = a->newLabel();
	a->b(label_entry);
	int off_addr_start = a->offset();
	a->embed(buf.data(), buf.size());
	a->bind(label_entry);

	const int off = off_addr_start - a->offset();
	aarch64_asm_adr_x(a, x, off);
}

/*************************************************************************
 * 在代码中嵌入一个 C 字符串常量，并让 x 指向它
 * 参数：a		: Assembler 指针
 * 		x		: 目标 X 寄存器
 * 		data	: 字符串常量
 ************************************************************************/
static void aarch64_asm_set_x_cstr_ptr(asmjit::a64::Assembler* a, asmjit::a64::GpX x, const std::string& str) {
	std::vector<uint8_t> buf(str.size() + 1, 0u);
	if (!str.empty()) {
		memcpy(buf.data(), str.c_str(), str.size());
	}
	aarch64_asm_set_x_data_ptr(a, x, buf);
}

/*************************************************************************
 * PACIA Xn
 ************************************************************************/
static bool aarch64_asm_pacia(asmjit::a64::Assembler* a, const asmjit::a64::GpX& reg) {
	uint32_t reg_n = reg.id();
	if (reg_n > 31) {
		std::cout << "[发生错误] Xn 寄存器编号超出范围: " << reg_n << std::endl;
		return false;
	}
	uint32_t instr = 0xDAC103E0 | reg_n;
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * PACIASP
 ************************************************************************/
static bool aarch64_asm_paciasp(asmjit::a64::Assembler* a) {
	uint32_t instr = 0xD503233F;
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * AUTIASP
 ************************************************************************/
static bool aarch64_asm_autiasp(asmjit::a64::Assembler* a) {
	uint32_t instr = 0xD50323BF;
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * BTI C
 ************************************************************************/
static bool aarch64_asm_bit_c(asmjit::a64::Assembler* a) {
	uint32_t instr = 0xD503245F;
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * BTI J
 ************************************************************************/
static bool aarch64_asm_bit_j(asmjit::a64::Assembler* a) {
	uint32_t instr = 0xD503249F;
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * BTI JC
 ************************************************************************/
static bool aarch64_asm_bit_jc(asmjit::a64::Assembler* a) {
	uint32_t instr = 0xD50324DF;
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * 生成一条 MRS 指令，将 CTR_EL0 读入指定 X 寄存器。
 ************************************************************************/
static bool aarch64_asm_mrs_ctr_el0(asmjit::a64::Assembler* a, const asmjit::a64::GpX& reg) {
	uint32_t reg_n = reg.id();
	if (reg_n > 31) {
		std::cout << "[发生错误] Xn 寄存器编号超出范围: " << reg_n << std::endl;
		return false;
	}
	uint32_t ctr_el0 = asmjit::a64::Predicate::SysReg::encode(3, 3, 0, 0, 1);
	a->mrs(reg, ctr_el0);
	return true;
}

/*************************************************************************
 * 生成一条 DC CVAC, Xn 指令
 ************************************************************************/
static bool aarch64_asm_dc_cvac(asmjit::a64::Assembler* a, const asmjit::a64::GpX& reg) {
	uint32_t reg_n = reg.id();
	if (reg_n > 31) {
		std::cout << "[发生错误] Xn 寄存器编号超出范围: " << reg_n << std::endl;
		return false;
	}
	uint32_t instr = 0xD50B7A20u | reg_n;
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * IC IALLU
 *************************************************************************/
static bool aarch64_asm_ic_iallu(asmjit::a64::Assembler* a) {
	uint32_t instr = 0xD508751F;
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * DSB ISH
 ************************************************************************/
static bool aarch64_asm_dsb_ish(asmjit::a64::Assembler* a) {
	uint32_t instr = 0xD5033B9F;
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * ISB
 ************************************************************************/
static bool aarch64_asm_isb(asmjit::a64::Assembler* a) {
	uint32_t instr = 0xD5033FDF;
	a->embed(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
	return true;
}

/*************************************************************************
 * 打印 AsmJit 生成的 AArch64 汇编文本（大写）
 ************************************************************************/
static std::string print_aarch64_asm(asmjit::a64::Assembler* a) {
	if (!a) return {};
	asmjit::CodeHolder* code = a->code();
	if (!code) return {};
	asmjit::Logger* logger = code->logger();
	if (!logger) return {};
	auto* str_logger = static_cast<asmjit::StringLogger*>(logger);
	std::string text = str_logger->data();
	transform(text.begin(), text.end(), text.begin(), ::toupper);
	return text;
}
