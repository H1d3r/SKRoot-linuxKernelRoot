#include "module_web_ui_example.h"
#include "kernel_module_kit_umbrella.h"

#define CONFIG_FILE_NAME "config123.txt"

static std::string get_config_file_path(const char* module_private_dir) {
    return std::string(module_private_dir) + CONFIG_FILE_NAME;
}

// SKRoot模块入口函数
int skroot_module_main(const char* root_key, const char* module_private_dir) {
    printf("[module_web_ui_example] starting... \n");
    printf("[module_web_ui_example] root_key len=%zu\n", strlen(root_key));
    printf("[module_web_ui_example] module_private_dir=%s\n", module_private_dir);

    //读取配置文件内容
    std::string config_file_path = get_config_file_path(module_private_dir);
    std::string value;
    if(read_text_file(config_file_path.c_str(), value)) {
        printf("[module_web_ui_example] read file succeed: %s, value: %s\n", config_file_path.c_str(), value.c_str());
    } else {
        printf("[module_web_ui_example] read file failed: %s\n", config_file_path.c_str());
    }
    return 0;
}


// WebUI HTTP服务器回调函数
class MyWebHttpHandler : public kernel_module::WebUIHttpHandler { // HTTP服务器基于civetweb库
public:
    void onPrepareCreate(const char* root_key, const char* module_private_dir, uint32_t port) override {
        printf("[module_web_ui_example] MyHttpHandler root_key len=%zu\n", strlen(root_key));
        printf("[module_web_ui_example] MyHttpHandler module_private_dir=%s\n", module_private_dir);
        printf("[module_web_ui_example] MyHttpHandler port=%d\n", port);
        m_root_key = root_key;
        m_config_file_path = get_config_file_path(module_private_dir);
    }

    bool handleGet(CivetServer* server, struct mg_connection* conn) override {
        const struct mg_request_info* req_info = mg_get_request_info(conn);
        std::string path = req_info->local_uri ? req_info->local_uri : "/";
        std::string query = req_info->query_string ? req_info->query_string : "";
        printf("GET request\nPath: %s\nQuery: %s\n", path.c_str(), query.c_str());
        return false;
    }

    bool handlePost(CivetServer* server, struct mg_connection* conn) override {
        char buf[1024] = {0}; mg_read(conn, buf, sizeof(buf) - 1);
        const struct mg_request_info* req_info = mg_get_request_info(conn);
        std::string path = req_info->local_uri ? req_info->local_uri : "/";
        std::string body(buf);
        printf("POST request\nPath: %s\nBody: %s\n", path.c_str(), body.c_str());

        std::string resp;
        if(path == "/getPid") resp = std::to_string(getpid());
        else if(path == "/getUid") resp = std::to_string(getuid());
        else if(path == "/getValue") read_text_file(m_config_file_path.c_str(), resp);
        else if(path == "/setValue") resp = write_text_file(m_config_file_path.c_str(), body) ? "OK" : "FAILED";
        
        mg_printf(conn,
                  "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/plain\r\n"
                  "Connection: close\r\n\r\n%s",
                  resp.c_str());
        return true;
    }
private:
    std::string m_root_key;
    std::string m_config_file_path;
};

// SKRoot 模块名片
SKROOT_MODULE_NAME("演示模块WebUI页面");
SKROOT_MODULE_VERSION("1.0.0");
SKROOT_MODULE_DESC("演示在模块中使用WebUI页面");
SKROOT_MODULE_AUTHOR("SKRoot官方教程");
SKROOT_MODULE_UUID32("6080b19fb2db26c534af3051103f541f");
SKROOT_MODULE_WEB_UI(MyWebHttpHandler);