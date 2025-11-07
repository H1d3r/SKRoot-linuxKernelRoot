#pragma once
#include <set>
#include <type_traits>
#include <algorithm>
#include <cstdio>
#include "kernel_module_kit_umbrella.h"

/***************************************************************************
 * AArch64 寄存器保护守卫类
 * 作用: 自动生成“保存与恢复寄存器”的指令对 (stp / ldp)，用于保护调用现场寄存器。
 ***************************************************************************/
class RegProtectGuard {
	using GpX = asmjit::a64::GpX;
	using GpW = asmjit::a64::GpW;
public:
	/*************************************************************************
	 * 枚举 SkipX0
	 * 作用: 控制是否在保护寄存器时跳过 X0（常用于保留返回值寄存器）
	 *************************************************************************/
	enum class SkipX0 : bool { No = false, Yes = true };


	/*************************************************************************
	 * 构造函数模板 (支持任意数量 X/W 寄存器)
	 * 参数: skip  : 是否跳过 X0
	 *   	a     : asmjit::a64::Assembler 指针
	 *   	regs  : 可变参数列表（GpX/GpW）
	 * 逻辑:
	 *   	1. 生成 stp 指令序列 (push pairs)；
	 *   	2. 析构时自动生成对应 ldp 指令 (pop pairs)。
	 *************************************************************************/
	template <typename... Regs,
			typename = std::enable_if_t<(std::conjunction_v<
				std::bool_constant<std::is_same_v<std::decay_t<Regs>, GpX> ||
									std::is_same_v<std::decay_t<Regs>, GpW>>...
			>)>>
	explicit RegProtectGuard(SkipX0 skip, asmjit::a64::Assembler* a, const Regs&... regs)
		: a_(a), skip_x0_(skip == SkipX0::Yes) {
		regs_.reserve(sizeof...(regs) + 1);
		(appendOneUnique(regs), ...);
		if (regs_.empty()) return;
		if (regs_.size() & 1) regs_.push_back(xzr);
		do_push_pairs();
		pushed_ = true;
	}

	explicit RegProtectGuard(SkipX0 skip, asmjit::a64::Assembler* a, const std::set<uint32_t>& regs_id)
		: a_(a), skip_x0_(skip == SkipX0::Yes) {
		regs_.reserve(regs_id.size() + 1);
		for(auto id : regs_id) {
			appendOneUnique(asmjit::a64::x(id));
		}
		if (regs_.empty()) return;
		if (regs_.size() & 1) regs_.push_back(xzr);
		do_push_pairs();
		pushed_ = true;
	}

	// DO NOT COPY && DO NOT DELETE
	RegProtectGuard(const RegProtectGuard&) = delete;
	RegProtectGuard& operator=(const RegProtectGuard&) = delete;
	RegProtectGuard(RegProtectGuard&& other) noexcept
		: a_(other.a_), regs_(std::move(other.regs_)),
		pushed_(other.pushed_), skip_x0_(other.skip_x0_) {
	other.pushed_ = false;
	}
	RegProtectGuard& operator=(RegProtectGuard&& other) noexcept {
	if (this != &other) {
		cleanup();
		a_ = other.a_;
		regs_ = std::move(other.regs_);
		pushed_ = other.pushed_;
		skip_x0_ = other.skip_x0_;
		other.pushed_ = false;
	}
	return *this;
	}

	~RegProtectGuard() { cleanup(); }

private:
	static GpX toX(const GpX& rx) { return rx; }
	static GpX toX(const GpW& rw) { return asmjit::a64::x(rw.id()); }

	void appendOneUnique(const GpX& r) {
		const uint32_t id = r.id();
		for (auto &v : regs_) if (v.id() == id) return;
		regs_.push_back(r);
	}
	void appendOneUnique(const GpW& r) { appendOneUnique(asmjit::a64::x(r.id())); }

	inline GpX mask(GpX r) const {
		return (skip_x0_ && r.id() == 0) ? xzr : r;
	}

	void do_push_pairs() {
		for (size_t i = 0; i < regs_.size(); i += 2) {
			GpX a1 = mask(regs_[i]);
			GpX a2 = mask(regs_[i + 1]);
			if(a1 == xzr && a2 == xzr) continue;
			a_->stp(a1, a2, ptr(sp).pre(-16));
		}
	}

	void do_pop_pairs() {
		for (size_t i = regs_.size(); i > 0; i -= 2) {
			GpX a1 = mask(regs_[i - 2]);
			GpX a2 = mask(regs_[i - 1]);
			if(a1 == xzr && a2 == xzr) continue;
			a_->ldp(a1, a2, ptr(sp).post(16));
		}
	}

	void cleanup() {
		if (!pushed_ || !a_) return;
		do_pop_pairs();
		pushed_ = false;
		regs_.clear();
	}

private:
	asmjit::a64::Assembler* a_ = nullptr;
	std::vector<GpX> regs_;
	bool pushed_ = false;
	bool skip_x0_ = true;

	const GpX xzr = asmjit::a64::xzr;
	const GpX sp = asmjit::a64::sp;
};