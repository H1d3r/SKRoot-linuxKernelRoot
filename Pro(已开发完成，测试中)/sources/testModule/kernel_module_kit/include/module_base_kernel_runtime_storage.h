#pragma once
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "____DO_NOT_EDIT____/private_to_string.h"
#include "module_base_err_def.h"

namespace kernel_module {
/***************************************************************************
 * 写入内核运行时存储
 * 作用: 提供一种在内核运行期间的持久化存储，内核未重启前一直有效，重启后自动清空
 * 参数: root_key       ROOT权限密钥文本
 *      app_name       应用名称（用于区分不同存储空间）
 *      key_name       键名称
 *      value          要写入的字符串值
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr write_string_kernel_runtime_storage(
	const char   * root_key,
	const char   * app_name,
	const char   * key_name,
	const char   * value
);

/***************************************************************************
 * 写入内核运行时存储（泛型版本）
 * 作用: 泛型版本，将任意类型的 value 转换为字符串后写入存储
 * 参数: 同 write_string_kernel_runtime_storage，只是 value 类型不同
 * 返回: 同 write_string_kernel_runtime_storage
 ***************************************************************************/
template<typename T>
inline KModErr write_kernel_runtime_storage(
    const char *root_key,
    const char *app_name,
    const char *key_name,
    const T &value) {
    std::string s = detail::to_string_generic(value);
    return write_string_kernel_runtime_storage(root_key, app_name, key_name, s.c_str());
}

/***************************************************************************
 * 读取内核运行时存储
 * 参数: root_key       ROOT权限密钥文本
 *      app_name       应用名称（用于区分不同存储空间）
 *      key_name       键名称
 *      out_string     输出参数，存放读取到的字符串
 * 返回: OK 表示成功
 ***************************************************************************/
KModErr read_string_kernel_runtime_storage(
	const char   * root_key,
	const char   * app_name,
	const char   * key_name,
    std::string & out_string
);

/***************************************************************************
 * 读取内核运行时存储（泛型版本）
 * 作用: 泛型版本，从存储中读取字符串并转换为目标类型
 * 参数: 同 read_string_kernel_runtime_storage，只是 out_value 类型不同
 * 返回: 同 read_string_kernel_runtime_storage
 ***************************************************************************/
template<typename T>
inline KModErr read_kernel_runtime_storage(
    const char *root_key,
    const char *app_name,
    const char *key_name,
    T &out_value) {
    std::string s;
    RETURN_IF_ERROR_KMOD(read_string_kernel_runtime_storage(root_key, app_name, key_name, s));
    out_value = detail::from_string_generic<T>(s);
    return KModErr::OK;
}
}
