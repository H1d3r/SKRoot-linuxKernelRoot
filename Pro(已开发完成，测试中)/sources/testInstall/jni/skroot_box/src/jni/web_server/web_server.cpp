#include <netinet/in.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>
#include <set>
#include <mutex>
#include <filesystem>

#include "web_server.h"
#include "web_server_inline.h"
#include "civetweb-1.16/include/CivetServer.h"
#include "index_html_gz_data.h"
#include "skroot_box_umbrella.h"
#include "src/jni/common/android_open_url.h"

#define MAX_HEARTBEAT_TIME_SEC 20
constexpr const char* recommend_files[] = {"libc++_shared.so"};
char ROOT_KEY[256] = {0};
int PORT = 0;
std::atomic<bool> g_heartbeat{true};

std::string convert_2_json(const std::string & str, const std::map<std::string, std::string> & appendParam = {}) {
    std::string strJson;
    cJSON *json = cJSON_CreateObject();
    if(json) {
        cJSON_AddStringToObject(json, "content", str.c_str());
        for(const auto & param: appendParam) {
            cJSON_AddStringToObject(json, param.first.c_str(), param.second.c_str());
        }
        char *jsonString = cJSON_Print(json);
        if(jsonString) {
            strJson = jsonString;
            free(jsonString);
        }
        cJSON_Delete(json);
    }
    return strJson;
}

std::string convert_2_json_m(const std::string & str, const std::map<std::string, std::string> & appendParam = {}) {
    std::string strJson;
    cJSON *json = cJSON_CreateObject();
    if(json) {
        cJSON_AddStringToObject(json, "content", str.c_str());
        cJSON *jsonArray = cJSON_CreateArray();
        for(const auto & param: appendParam) {
            cJSON *jsonMap = cJSON_CreateObject();
            cJSON_AddStringToObject(jsonMap, param.first.c_str(), param.second.c_str());
            cJSON_AddItemToArray(jsonArray, jsonMap);
        }
        cJSON_AddItemToObject(json, "arr_map", jsonArray);

        char *jsonString = cJSON_Print(json);
        if(jsonString) {
            strJson = jsonString;
            free(jsonString);
        }
        cJSON_Delete(json);
    }
    return strJson;
}

std::string convert_2_json_v(const std::vector<std::string> &v, const std::map<std::string, std::string> & appendParam = {}) {
    std::string strJson;
    cJSON *json = cJSON_CreateObject();
    if (json) {
        cJSON *jsonArray = cJSON_CreateArray();
        for (const std::string &str : v) {
            cJSON_AddItemToArray(jsonArray, cJSON_CreateString(str.c_str()));
        }
        cJSON_AddItemToObject(json, "content", jsonArray);
        for(const auto & param: appendParam) {
            cJSON_AddStringToObject(json, param.first.c_str(), param.second.c_str());
        }
        char *jsonString = cJSON_Print(json);
        if (jsonString) {
            strJson = jsonString;
            free(jsonString);
        }

        cJSON_Delete(json);
    }
    return strJson;
}

std::string get_json_str(const std::string& json, const char* key) {
    cJSON* root = cJSON_Parse(json.c_str());
    if (!root) return {};
    cJSON* j_str = cJSON_GetObjectItem(root, key);
    std::string result = j_str ? j_str->valuestring : "";
    cJSON_Delete(root); 
    return result;
}

int get_json_int(const std::string& json, const char* key) {
    cJSON* root = cJSON_Parse(json.c_str());
    if (!root) return {};
    cJSON* j_int = cJSON_GetObjectItem(root, key);
    int n = j_int ? j_int->valueint : 0;
    cJSON_Delete(root); 
    return n;
}

std::tuple<std::string, bool> handle_index() {
	std::string gzip_html;
	gzip_html.assign(reinterpret_cast<const char*>(web_server::index_html_gz_data), web_server::index_html_gz_size);
	return { gzip_html, true };
}

std::string handle_heartbeat(const std::string & json) {
    g_heartbeat = true;
    std::string userName = get_json_str(json, "userName");
    return convert_2_json(userName);
}

std::string handle_test_root() {
    std::string test_report = skroot_box::get_root_test_report(ROOT_KEY);
    return convert_2_json(test_report);
}

std::string handle_run_root_cmd(const std::string & json) {
    std::string cmd = get_json_str(json, "cmd");
    std::string result;
    SkBoxErr err = skroot_box::run_root_cmd(ROOT_KEY, cmd.c_str(), result);

    std::stringstream sstr;
    sstr << "run_root_cmd " << to_string(err).c_str() << ", result: " << result;
    return convert_2_json(sstr.str());
}

std::string handle_root_exec_process(const std::string & json) {
    std::string path = get_json_str(json, "path");
    SkBoxErr err = skroot_box::root_exec_process(ROOT_KEY, path.c_str());

    std::stringstream sstr;
    sstr << "root_exec_processs " << to_string(err).c_str();
    return convert_2_json(sstr.str());
}

std::string handle_get_app_list(const std::string & json) {
    bool isShowSystemApp = !!get_json_int(json, "showSystemApp");
    bool isShowThirtyApp = !!get_json_int(json, "showThirdApp");
    bool isShowRunningAPP = !!get_json_int(json, "showRunningApp");

    std::vector<std::string> packageNames;
    std::string cmd;
    if(isShowSystemApp && isShowThirtyApp) cmd = "pm list packages";
    else if(isShowSystemApp) cmd = "pm list packages -s";
    else if(isShowThirtyApp) cmd = "pm list packages -3";

    std::string packages;
    SkBoxErr err = skroot_box::run_root_cmd(ROOT_KEY, cmd.c_str(), packages);
    if (is_failed(err)) return convert_2_json_v(packageNames);

    std::map<pid_t, std::string> pid_map;
    if(isShowRunningAPP) {
        err = skroot_box::get_all_cmdline_process(ROOT_KEY, pid_map);
        if (is_failed(err)) return convert_2_json_v(packageNames);        
    }    
    // remove "package:" flag
    std::istringstream iss(packages);
    std::string line;
    while (getline(iss, line)) {
        size_t pos = line.find("package:");
        if (pos != std::string::npos) {
            line.erase(pos, std::string("package:").length());
        }
        if(isShowRunningAPP) {
            bool isFound = false;
            for(auto & item : pid_map) {
                if(item.second.find(line) == std::string::npos) continue;
                isFound = true;
                break;
            }
            if(!isFound) continue;
        }
        packageNames.push_back(line);
    }
    return convert_2_json_v(packageNames);
}

std::string handle_unknow_type() {
    return convert_2_json("unknow command type.");
}

class MyHttpHandler : public CivetHandler {
public:
    bool handleGet(CivetServer* server, struct mg_connection* conn) override {
        const struct mg_request_info* req_info = mg_get_request_info(conn);
        std::string path = req_info->local_uri ? req_info->local_uri : "/";

        writeToLog("Get request:" + path);
        std::string body;
        bool use_gzip = false;
        if(path == "/") {
            // home page
            std::tie(body, use_gzip) = handle_index();
        }
        mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "%s"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n\r\n",
            use_gzip ? "Content-Encoding: gzip\r\n" : "", body.size());
        mg_write(conn, body.data(), body.size());
        return true;
    }

    bool handlePost(CivetServer* server, struct mg_connection* conn) override {
        char buf[1024];
        int len = mg_read(conn, buf, sizeof(buf) - 1);
        buf[len > 0 ? len : 0] = '\0';

        const struct mg_request_info* req_info = mg_get_request_info(conn);
        std::string path = req_info->local_uri ? req_info->local_uri : "/";
        std::string body(buf);

        writeToLog("POST request:" + path);
        writeToLog("POST body:" + body);

        std::string resp;
        if(path == "/heartbeat") resp = handle_heartbeat(body);
        else if(path == "/testInstall") resp = handle_test_root();
        else if(path == "/runRootCmd") resp = handle_run_root_cmd(body);
        else if(path == "/rootExecProc") resp = handle_root_exec_process(body);
        else if(path == "/getAppList") resp = handle_get_app_list(body);
        mg_printf(conn,
                  "HTTP/1.1 200 OK\r\n"
                  "Content-Type: application/json\r\n"
                  "Connection: close\r\n\r\n%s",
                  resp.c_str());
        return true;
    }
private:
    std::string m_root_key;
    std::string m_config_file_path;
};


int main(int argc, char* argv[]) {
    srand(time(NULL));
    PORT = rand() % 40001 + 20000;
    strncpy(ROOT_KEY, const_cast<char*>(static_inline_web_server_root_key), sizeof(ROOT_KEY) - 1);

    writeToLog("web_server enter");
    if (is_failed(skroot_box::get_root_proxy(ROOT_KEY))) {
        writeToLog("web_server root error");
		return 0;
	}

    std::string str_port = std::to_string(PORT);
    const char* options[] = {
        "listening_ports", str_port.c_str(),
        "num_threads", "1",
        NULL
    };

    CivetServer server(options);

    MyHttpHandler handler;
    server.addHandler("/", handler); // /代表所有路径
    
    sleep(1);
    std::string url = "http://127.0.0.1:" + std::to_string(PORT);
    android_open_url(url);

    while (g_heartbeat) {
        g_heartbeat = false;
        usleep(1000 * 1000 * (MAX_HEARTBEAT_TIME_SEC / 2));
    }
    server.close();
	_exit(0);
    return 0;
}