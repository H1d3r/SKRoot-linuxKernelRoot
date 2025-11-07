#pragma once
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../skroot_box_err_def.h"
#include "../skroot_box_parasite_app_dynlib_status.h"
namespace skroot_box {
inline SkBoxErr run_root_cmd(const char* str_root_key, const char* cmd, std::string & out_result) {
    out_result.clear();
    thread_local std::string* tls_out = nullptr;
    auto cb = [](const char* result) {
        if (!tls_out || !result) return;
        (*tls_out) = result;
    };
    tls_out = &out_result;
    SkBoxErr run_root_cmd_with_cb(const char* str_root_key, const char* cmd, void (*cb)(const char* result));
    SkBoxErr err = run_root_cmd_with_cb(str_root_key, cmd, cb);
    tls_out = nullptr; 
    return err;

}

inline SkBoxErr parasite_precheck_app(const char* str_root_key, const char* target_pid_cmdline, std::map<std::string, skroot_box::AppDynlibStatus> &output_dynlib_full_path) {
    output_dynlib_full_path.clear();
    thread_local std::map<std::string, skroot_box::AppDynlibStatus>* tls_out = nullptr;
    auto cb = [](const char* dynlib_full_path, skroot_box::AppDynlibStatus status) {
        if (!tls_out || !dynlib_full_path) return;
        (*tls_out)[std::string(dynlib_full_path)] = status;
    };
    tls_out = &output_dynlib_full_path;
    SkBoxErr parasite_precheck_app_with_cb(const char* str_root_key, const char* target_pid_cmdline, void (*cb)(const char* dynlib_full_path, skroot_box::AppDynlibStatus status));
    SkBoxErr err = parasite_precheck_app_with_cb(str_root_key, target_pid_cmdline, cb);
    tls_out = nullptr; 
    return err;
}

inline SkBoxErr find_all_cmdline_process(const char* str_root_key, const char* target_cmdline, std::set<pid_t> & out, bool compare_full_agrc) {
    out.clear();
    thread_local std::set<pid_t>* tls_out = nullptr;
    auto cb = [](pid_t pid) {
        if (!tls_out || !pid) return;
        tls_out->insert(pid);
    };
    tls_out = &out;
    SkBoxErr find_all_cmdline_process_with_cb(const char* str_root_key, const char* target_cmdline, void (*cb)(pid_t pid), bool compare_full_agrc);
    SkBoxErr err = find_all_cmdline_process_with_cb(str_root_key, target_cmdline, cb, compare_full_agrc);
    tls_out = nullptr;
    return err;
}

inline SkBoxErr get_all_cmdline_process(const char* str_root_key, std::map<pid_t, std::string> & pid_map, bool compare_full_agrc) {
    pid_map.clear();
    thread_local std::map<pid_t, std::string>* tls_out = nullptr;
    auto cb = [](pid_t pid, const char* cmdline) {
        if (!tls_out || !pid || !cmdline) return;
        (*tls_out)[pid] = cmdline;
    };
    tls_out = &pid_map;
    SkBoxErr get_all_cmdline_process_with_cb(const char* str_root_key, void (*cb)(pid_t pid, const char* cmdline), bool compare_full_agrc);
    SkBoxErr err = get_all_cmdline_process_with_cb(str_root_key, cb, compare_full_agrc);
    tls_out = nullptr;
    return err;
}

}