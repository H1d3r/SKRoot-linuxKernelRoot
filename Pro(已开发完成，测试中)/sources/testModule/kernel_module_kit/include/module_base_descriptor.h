#pragma once
#include "____DO_NOT_EDIT____/private_desc_parser.h"
#include "module_base_web_ui_server.h"

/*************************************************************************** 
 * SKRoot 模块入口函数（实现方必须提供）
 ***************************************************************************/
int skroot_module_main(const char* root_key, const char* module_private_dir);

/***************************************************************************
 * 必填元数据（不填写将导致模块无法加载）
 ***************************************************************************/
// SKRoot 模块名称（必填）
#define SKROOT_MODULE_NAME(name)            ___MOD_NAME(name)

// SKRoot 模块版本号（必填）
#define SKROOT_MODULE_VERSION(ver)          ___MOD_VERSION(ver)

// SKRoot 模块描述（必填）
#define SKROOT_MODULE_DESC(desc)            ___MOD_DESC(desc)

// SKRoot 模块作者（必填）
#define SKROOT_MODULE_AUTHOR(author)        ___MOD_AUTHOR(author)

// SKRoot 模块 UUID32（必填，32 个随机字符 [0-9a-zA-Z]）
#define SKROOT_MODULE_UUID32(str32)         ___MOD_UUID32(str32)

/***************************************************************************
 * 可选能力（按需填写）
 ***************************************************************************/
// SKRoot 模块 WebUI 处理类（可选）
#define SKROOT_MODULE_WEB_UI(WebUIHandlerClass) ___MOD_WEB_UI(WebUIHandlerClass)

// SKRoot 模块更新信息 JSON 的 URL（可选）
// 示例：SKROOT_MODULE_UPDATE_JSON("https://example.com/your_module_update.json")
#define SKROOT_MODULE_UPDATE_JSON(url)      ___MOD_UPDATE_JSON(url)
