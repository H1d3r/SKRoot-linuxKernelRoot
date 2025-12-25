#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "kernel_module_kit_umbrella.h"

/*
 * 内核环境下的基础字符串与内存操作算法，
 * 提供 strlen、strcmp、strstr、memcmp、memcpy 等常见函数实现。
 */
namespace kernel_module {
namespace string_ops {
using namespace asmjit::a64;

/* strlen：计算以 '\0' 结尾的字符串长度；X0 返回长度 */
void kstrlen(Assembler* a, GpX str);
/* strlen (外部封装版本)：计算内核地址处字符串的长度，结果写入 out_len，返回值为OK代表执行成功 */
KModErr kstrlen(const char* root_key, uint64_t kmem, uint64_t& out_len);

/* strcmp：文本字符串比较；若相等X0为0 （与标准 strcmp 一致）*/
void kstrcmp(Assembler* a, GpX str1, GpX str2);

/* strncmp：比较最多 n 个字符；若相等X0为0 （与标准 strncmp 一致） */
void kstrncmp(Assembler* a, GpX str1, GpX str2, GpX n);

/* kstrcpy：将以 '\0' 结尾的字符串从 src 复制到 dst（包含终止符）；X0 返回 dst（与标准 strcpy 一致）
   注意：仅适用于非重叠区域！ */
void kstrcpy(Assembler* a, GpX dst, GpX src);

/* kstrncpy：最多复制 n 个字节从 src 到 dst；若 src 不足 n 字节则用 '\0' 填充； X0 返回 dst（与标准 strncpy 一致）
   注意：仅适用于非重叠区域！ */
void kstrncpy(Assembler* a, GpX dst, GpX src, GpX n);
void kstrncpy(Assembler* a, GpX dst, GpX src, uint64_t n);

/* strstr：在 haystack 中查找 needle；若找到X0为匹配位置指针，否则为0 （与标准 strstr 一致）*/
void kstrstr(Assembler* a, GpX haystack, GpX needle);

/* strchr：在 str 中查找首次出现的字符 c；若找到X0为匹配位置指针，否则为0 （与标准 strchr 一致）*/
void kstrchr(Assembler* a, GpX str, GpW c/*低8位生效*/);
void kstrchr(Assembler* a, GpX str, uint8_t c);

/* memset：将前 n 字节设置为字节值 c；X0 返回 dst（与标准 memset 一致）*/
void kmemset(Assembler* a, GpX dst, GpW c/*低8位生效*/, GpX n);
void kmemset(Assembler* a, GpX dst, uint8_t c, GpX n);
void kmemset(Assembler* a, GpX dst, uint8_t c, uint64_t n);

/* memcmp：按字节比较两块内存的前 n 字节；若相等X0为0 （与标准 memcmp 一致） */
void kmemcmp(Assembler* a, GpX buf1, GpX buf2, GpX n);

/* memcpy：从 src 复制 n 字节到 dst；X0 返回 dst（与标准 memcpy 一致）
	注意：仅适用于非重叠区域！ */
void kmemcpy(Assembler* a, GpX dst, GpX src, GpX n);

/* memmem：在 buf 的前 n 字节中查找子串 needle（长度 needle_n）；若找到X0为匹配位置指针，否则为0 （与 GNU memmem 一致）*/
void kmemmem(Assembler* a, GpX buf, GpX n, GpX needle, GpX needle_n);

/* memchr：在前 n 字节内查找字节值 c；若找到X0为匹配位置指针，否则为0 （与标准 memchr 一致）*/
void kmemchr(Assembler* a, GpX buf, GpW c/*低8位生效*/, GpX n);

/* memrchr：反向查找（从 buf+n-1 向前）；若找到X0为匹配位置指针，否则为0 （与标准 memrchr 一致） */
void kmemrchr(Assembler* a, GpX buf, GpW c /*低8位*/, GpX n);

/* 前缀判断：判断 str 是否以 prefix 开头，若匹配X0为1，否则为0*/
void kstartswith(Assembler* a, GpX str, GpX prefix);

/* 后缀判断：判断 str 是否以 suffix 结尾，若匹配X0为1，否则为0*/
void kendswith(Assembler* a, GpX str, GpX suffix);

} // namespace string_ops
} // namespace kernel_module