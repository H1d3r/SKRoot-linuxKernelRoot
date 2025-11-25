#pragma once
#include <iostream>
namespace skroot_env {
/***************************************************************************
 * 测试 SKRoot Shellcode通道
 * 参数: root_key   ROOT权限密钥文本
 * 返回: OK 表示正常
***************************************************************************/
KModErr test_skroot_shellcode_channel(const char* root_key);

/***************************************************************************
 * 测试 SKRoot 自带默认模块
 * 参数: root_key   ROOT权限密钥文本
 *       name       要测试的内部模块枚举
 * 返回: OK 表示正常
***************************************************************************/
enum class DeafultModuleName: uint32_t {
    RootBridge,   // ROOT 提权模块
    SuRedirect,   // su 重定向模块
};
KModErr test_skroot_deafult_module(const char* root_key, DeafultModuleName name);

}
