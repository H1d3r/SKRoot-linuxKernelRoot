#pragma once
#include <unistd.h>
#include <iostream>
namespace skroot_env {
/***************************************************************************
 * 获取ROOT权限
 * 参数: root_key ROOT权限密钥文本
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr get_root(const char* root_key);
}