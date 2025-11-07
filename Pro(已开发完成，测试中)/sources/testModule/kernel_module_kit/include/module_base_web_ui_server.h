#pragma once
#include <string.h>
#include <unistd.h>
#include <unordered_map>
#include "civetweb-1.16/include/CivetServer.h"
namespace kernel_module {

/***************************************************************************
 * WebUI页面 HTTP 请求处理的基类
 * 作用: 支持模块在 CivetWeb 服务器中自定义行为
 * 说明: 可通过继承此类，实现 WebUI 的动态页面逻辑或自定义接口响应
 ***************************************************************************/
class WebUIHttpHandler : public CivetHandler {
public:
    /***************************************************************************
     * WebUI 服务器启动前回调
     * 参数: root_key           ROOT权限密钥文本
     *      module_private_dir  模块私有目录（用于存放受保护的资源文件）
     *      port                WebUI 服务器监听端口
     ***************************************************************************/
    virtual void onPrepareCreate(const char* root_key, const char* module_private_dir, uint32_t port) {}

    /***************************************************************************
     * 响应处理 GET 请求
     * 参数: server   CivetWeb 服务器实例指针
     *      conn     当前 HTTP 连接对象
     * 返回: true 表示已生成完整响应（请求结束）
     *      false 表示未生成响应，由 CivetWeb 默认机制继续处理
     ***************************************************************************/
    virtual bool handleGet(CivetServer* server, struct mg_connection* conn) override { return false; }

    /***************************************************************************
     * 响应处理 POST 请求
     * 参数: server   CivetWeb 服务器实例指针
     *      conn     当前 HTTP 连接对象
     * 返回: true  表示已生成完整响应（请求结束）
     *      false 表示未生成响应，由 CivetWeb 默认机制继续处理
     ***************************************************************************/
    virtual bool handlePost(CivetServer* server, struct mg_connection* conn) override { return false; }
};

} // namespace kernel_module
