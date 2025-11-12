#pragma once
#include <iostream>
namespace skroot_env {
 /***************************************************************************
 * 测试 SKRoot Shellcode通道
 * 参数: root_key   ROOT权限密钥文本
 * 返回: OK 表示正常
 ***************************************************************************/
KModErr test_skroot_shellcode_channel(const char* root_key);
}
