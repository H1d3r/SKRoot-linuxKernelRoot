#pragma once
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "skroot_box_err_def.h"

namespace skroot_box {
/***************************************************************************
 * 以ROOT身份直接执行程序（exec）
 * 参数:
 *   str_root_key  ROOT 权限密钥文本
 *   file_path     目标可执行文件的绝对路径
 * 返回: 仅在执行失败时返回负错误码；若 exec 成功，本函数不返回
 ***************************************************************************/
SkBoxErr root_exec_process(const char* str_root_key, const char *file_path);
}
