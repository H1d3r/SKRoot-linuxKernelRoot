#pragma once
#include "____DO_NOT_EDIT____/private_desc_parser.h"
#include "module_base_web_ui_server.h"

// SKRoot模块入口函数
int skroot_module_main(const char* root_key, const char* module_private_dir);

// SKRoot模块名称（必填)
#define SKROOT_MODULE_NAME(name) ___MOD_NAME(name)

// SKRoot模块版本号（必填)
#define SKROOT_MODULE_VERSION(ver) ___MOD_VERSION(ver)

// SKRoot模块描述（必填)
#define SKROOT_MODULE_DESC(desc) ___MOD_DESC(desc)

// SKRoot模块作者（必填）
#define SKROOT_MODULE_AUTHOR(author)  ___MOD_AUTHOR(author)

// SKRoot模块UUID32（必填，32个随机字符 [0-9a-zA-Z]）
#define SKROOT_MODULE_UUID32(str32) ___MOD_UUID32(str32)

// SKRoot模块WebUI页面（可选）
#define SKROOT_MODULE_WEB_UI(WebUIHttpHandlerClass) ___MOD_WEB_UI(WebUIHttpHandlerClass)