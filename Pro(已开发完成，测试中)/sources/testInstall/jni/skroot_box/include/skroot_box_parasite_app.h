#pragma once
#include <iostream>
#include <set>
#include <map>

#include "____DO_NOT_EDIT____/private_api_runtime_helper.h"


namespace skroot_box {
/***************************************************************************
 * 预检查寄生APP（仅做检查，不植入）
 * 参数:
 *   str_root_key               ROOT 权限密钥文本
 *   target_pid_cmdline         目标进程的 cmdline
 *   output_dynlib_full_path    输出：name为可寄生 so 的绝对路径，key为其状态（running / not_running）
 * 返回: OK 表示成功; 其余为错误码
 ***************************************************************************/
SkBoxErr parasite_precheck_app(const char* str_root_key, const char* target_pid_cmdline, std::map<std::string, AppDynlibStatus> &output_dynlib_full_path);


/***************************************************************************
 * 开始植入寄生APP
 * 参数:
 *   str_root_key           ROOT 权限密钥文本
 *   target_pid_cmdline     目标进程的 cmdline
 *   original_so_full_path  待植入的 so 的绝对路径
 * 返回: OK 表示成功; 其余为错误码
 ***************************************************************************/
SkBoxErr parasite_implant_app(const char* str_root_key, const char* target_pid_cmdline, const char* original_so_full_path);

/***************************************************************************
 * 杀死指定进程
 * 参数:
 *   str_root_key	ROOT 权限密钥文本
 *   pid			要终止的进程 ID
 * 返回: OK 表示成功；其余为错误码
 ***************************************************************************/
SkBoxErr kill_process(const char* str_root_key, pid_t pid);

}
