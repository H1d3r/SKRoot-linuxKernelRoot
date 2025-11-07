#pragma once
#include <cassert>
#include <set>
#include <type_traits>
#include <algorithm>
#include <cstdio>
#include "aarch64_asm_arg.h"
#include "kernel_module_kit_umbrella.h"

/***************************************************************************
 * AArch64 空闲通用寄存器池
 * 作用: 管理一组“可临时占用”的 AArch64 通用寄存器 (Xn / Wn)，用于 JIT 生成。
 *   - 支持从显式寄存器集合或从调用参数向量 (std::vector<Arm64Arg>) 初始化，
 *     自动标记已被使用的寄存器，避免冲突。
 ***************************************************************************/
class IdleRegPool {
	using GpX = asmjit::a64::GpX;
	using GpW = asmjit::a64::GpW;
public:
  template <typename... Regs,
      typename = std::enable_if_t<(std::conjunction_v<
      std::bool_constant<std::is_same_v<std::decay_t<Regs>, GpX> || std::is_same_v<std::decay_t<Regs>, GpW>>...
      >)>>
  static IdleRegPool Make(Regs... regs) {
    IdleRegPool p;
    p.init();
    (p.useOne(regs), ...);
    return p;
  }
  
  static IdleRegPool MakeFromVec(const std::vector<Arm64Arg>& vec) {
    IdleRegPool p;
    p.init();
    for (auto& a : vec) {
      if (a.kind == Arm64ArgKind::XReg) p.useOne(a.x);
      else if (a.kind == Arm64ArgKind::WReg) p.useOne(a.w);
    }
    return p;
  }

  GpX acquireX() {
    auto it = nextFree();
    if (it == free_.end()) printf("IdleRegPoo pool is empty!\n");
    uint32_t idx = *it;
    free_.erase(idx);
    used_.insert(idx);
    return asmjit::a64::x(idx);
  }

  GpW acquireW() {
    auto it = nextFree();
    if (it == free_.end()) printf("IdleRegPool pool is empty!\n");
    uint32_t idx = *it;
    free_.erase(idx);
    used_.insert(idx);
    return asmjit::a64::w(idx);
  }

  void release(GpX x) {
    free_.insert(x.id());
    used_.erase(x.id());
  }

  void release(GpW w) {
    free_.insert(w.id());
    used_.erase(w.id());
  }

  std::set<uint32_t> get_used() { return used_; }
private:
  void init() {
    for (uint32_t i = 0; i < 29; ++i) {
      if(i == 18) { continue; } // Platform Register
      free_.insert(i);
    }
    used_.clear();
  }

  void useOne(const GpX& rx) {
    used_.insert(rx.id());
    free_.erase(rx.id());
  }

  void useOne(const GpW& rw) {
    used_.insert(rw.id());
    free_.erase(rw.id());
  }

  std::set<uint32_t>::iterator nextFree() {
    return free_.begin();
  }

private:
  std::set<uint32_t> free_;
  std::set<uint32_t> used_;
};