#include <jni.h>
#include <string>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <thread>
#include <filesystem>
#include <sys/capability.h>

#include "skroot_box/include/skroot_box_umbrella.h"
#include "kernel_module_kit/include/kernel_module_kit_umbrella.h"
#include "urlEncodeUtils.h"
#include "cJSON.h"
using namespace std;

static string jstringToStr(JNIEnv* env, jstring jstring1) {
    const char *str1 = env->GetStringUTFChars(jstring1, 0);
    string s = str1;
    env->ReleaseStringUTFChars(jstring1, str1);
    return s;
}

static string urlEncodeToStr(string str) {
    size_t len = str.length();
    size_t max_encoded_len = 3 * len + 1;
    shared_ptr<char> spData(new (std::nothrow) char[max_encoded_len], std::default_delete<char[]>());
    memset(spData.get(), 0, max_encoded_len);
    url_encode(const_cast<char*>(str.c_str()), spData.get());
    return spData.get();
}

static cJSON * suAuthToJsonObj(skroot_env::su_auth_item & auth) {
    cJSON *item = cJSON_CreateObject();
    std::string encodeAppName = urlEncodeToStr(auth.app_package_name);
    cJSON_AddStringToObject(item, "app_package_name", encodeAppName.c_str());
    return item;
}

static cJSON * moduleDescToJsonObj(skroot_env::module_desc & desc) {
    cJSON *item = cJSON_CreateObject();
    std::string encodeName = urlEncodeToStr(desc.name);
    std::string encodeVer = urlEncodeToStr(desc.version);
    std::string encodeDesc = urlEncodeToStr(desc.desc);
    std::string encodeAuthor = urlEncodeToStr(desc.author);
    std::string encodeUuid = urlEncodeToStr(desc.uuid);
    std::stringstream ss;
    ss << desc.min_sdk_ver.major << "." << desc.min_sdk_ver.minor << "." << desc.min_sdk_ver.patch;
    std::string encodeMiniSDK = urlEncodeToStr(ss.str().c_str());
    cJSON_AddStringToObject(item, "name", encodeName.c_str());
    cJSON_AddStringToObject(item, "ver", encodeVer.c_str());
    cJSON_AddStringToObject(item, "desc", encodeDesc.c_str());
    cJSON_AddStringToObject(item, "author", encodeAuthor.c_str());
    cJSON_AddStringToObject(item, "uuid", encodeUuid.c_str());
    cJSON_AddBoolToObject(item, "web_ui", desc.web_ui);
    cJSON_AddStringToObject(item, "min_sdk_ver", encodeMiniSDK.c_str());
    return item;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_installSkrootEnv(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey) {
    string strRootKey = jstringToStr(env, rootKey);

    KModErr err = skroot_env::install_skroot_environment(strRootKey.c_str());
    std::stringstream sstr;
    sstr << "install_skroot_environment: " << to_string(err).c_str();
	if(is_ok(err)) sstr  << "，将在重启后生效";
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_uninstallSkrootEnv(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey) {
    string strRootKey = jstringToStr(env, rootKey);

    KModErr err = skroot_env::uninstall_skroot_environment(strRootKey.c_str());
    std::stringstream sstr;
    sstr << "uninstall_skroot_environment: " << to_string(err).c_str();
	if(is_ok(err)) sstr  << "，将在重启后生效";
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_getInstalledSkrootEnvVersion(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey) {
    string strRootKey = jstringToStr(env, rootKey);

    if(!skroot_env::is_installed_skroot_environment(strRootKey.c_str())) return env->NewStringUTF("");

    skroot_env::SkrootSdkVersion ver;
    KModErr err = skroot_env::get_installed_skroot_environment_version(strRootKey.c_str(), ver);
    if(is_failed(err)) return env->NewStringUTF(to_string(err).c_str());

    std::stringstream sstr;
    sstr << ver.major << "." << ver.minor << "." << ver.patch;
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_getSdkSkrootEnvVersion(
        JNIEnv* env,
        jclass /* this */) {
    skroot_env::SkrootSdkVersion ver = skroot_env::get_sdk_version();
    std::stringstream sstr;
    sstr << ver.major << "." << ver.minor << "." << ver.patch;
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_readSkrootLogs(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey) {
    string strRootKey = jstringToStr(env, rootKey);

    std::string log;
    KModErr err = skroot_env::read_skroot_autorun_log(strRootKey.c_str(), log);
    if(is_failed(err)) log = to_string(err);
    return env->NewStringUTF(log.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_setSkrootLogEnable(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jboolean enable) {
    string strRootKey = jstringToStr(env, rootKey);

    KModErr err = skroot_env::set_skroot_log_enable(strRootKey.c_str(), enable);

    std::stringstream sstr;
    sstr << "set_skroot_log_enable: " << to_string(err).c_str();
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_isEnableSkrootLog(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey) {
    string strRootKey = jstringToStr(env, rootKey);
    return skroot_env::is_enable_skroot_log(strRootKey.c_str()) ? JNI_TRUE : JNI_FALSE;
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_addSuAuth(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jstring appPackageName) {
    string strRootKey = jstringToStr(env, rootKey);
    string strAppPackageName = jstringToStr(env, appPackageName);

    KModErr err = skroot_env::add_su_auth_list(strRootKey.c_str(), strAppPackageName.c_str());
    std::stringstream sstr;
    sstr << "add_su_auth_list: " << to_string(err).c_str();
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_removeSuAuth(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jstring modUuid) {
    string strRootKey = jstringToStr(env, rootKey);
    string strModUuid = jstringToStr(env, modUuid);

    KModErr err = skroot_env::remove_su_auth_list(strRootKey.c_str(), strModUuid.c_str());
    std::stringstream sstr;
    sstr << "remove_su_auth_list: " << to_string(err).c_str();
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_getSuAuthList(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey) {
    string strRootKey = jstringToStr(env, rootKey);

    std::stringstream ss;
    std::vector<skroot_env::su_auth_item> pkgs;
    KModErr err = skroot_env::get_su_auth_list(strRootKey.c_str(), pkgs);
    if(is_failed(err)) {
        ss << "get_su_auth_list: " << to_string(err).c_str();
        return env->NewStringUTF(ss.str().c_str());
    }

    cJSON *root = cJSON_CreateArray();
    for (auto & iter : pkgs) {
        cJSON *item = suAuthToJsonObj(iter);
        cJSON_AddItemToArray(root, item);
    }
    ss << cJSON_Print(root);
    cJSON_Delete(root);
    return env->NewStringUTF(ss.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_clearSuAuthList(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey) {
    string strRootKey = jstringToStr(env, rootKey);

    KModErr err = skroot_env::clear_su_auth_list(strRootKey.c_str());
    std::stringstream sstr;
    sstr << "clear_su_auth_list: " << to_string(err).c_str();
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_installSkrootModule(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jstring zipFilePath) {
    string strRootKey = jstringToStr(env, rootKey);
    string strZipFilePath = jstringToStr(env, zipFilePath);

    KModErr err = skroot_env::install_module(strRootKey.c_str(), strZipFilePath.c_str());
    std::stringstream sstr;
    sstr << "install_module: " << to_string(err).c_str();
    if(is_ok(err)) sstr  << "，将在重启后生效";
    return env->NewStringUTF(sstr.str().c_str());
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_uninstallSkrootModule(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jstring modUuid) {
    string strRootKey = jstringToStr(env, rootKey);
    string strModUuid = jstringToStr(env, modUuid);

    KModErr err = skroot_env::uninstall_module(strRootKey.c_str(), strModUuid.c_str());
    std::stringstream sstr;
    sstr << "uninstall_module: " << to_string(err).c_str();
    if(is_ok(err)) sstr  << "，将在重启后生效";
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_getSkrootModuleList(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jboolean runningOnly) {
    string strRootKey = jstringToStr(env, rootKey);

    std::stringstream ss;
    std::vector<skroot_env::module_desc> list;
    KModErr err = skroot_env::get_all_modules_list(strRootKey.c_str(), list, runningOnly ? skroot_env::ModuleListMode::RunningOnly : skroot_env::ModuleListMode::All);
    if(is_failed(err)) {
        ss << "get_all_modules_list: " << to_string(err).c_str();
        return env->NewStringUTF(ss.str().c_str());
    }

    cJSON *root = cJSON_CreateArray();
    for (auto & iter : list) {
        cJSON *item = moduleDescToJsonObj(iter);
        cJSON_AddItemToArray(root, item);
    }
    ss << cJSON_Print(root);
    cJSON_Delete(root);
    return env->NewStringUTF(ss.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_parseSkrootModuleDesc(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jstring zipFilePath) {
    string strRootKey = jstringToStr(env, rootKey);
    string strZipFilePath = jstringToStr(env, zipFilePath);

    std::stringstream ss;
    skroot_env::module_desc desc;
    KModErr err = skroot_env::parse_module_desc_from_zip_file(strRootKey.c_str(), strZipFilePath.c_str(), desc);
    if(is_ok(err)) {
        cJSON *root = moduleDescToJsonObj(desc);
        ss << cJSON_Print(root);
        cJSON_Delete(root);
    } else {
        ss << "parse_module_desc_from_zip_file: " << to_string(err).c_str();
    }
    return env->NewStringUTF(ss.str().c_str());

}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_openSkrootModuleWebUI(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jstring modUuid) {
    string strRootKey = jstringToStr(env, rootKey);
    string strModUuid = jstringToStr(env, modUuid);

    int port = 0;
    KModErr err = skroot_env::features::web_ui::start_module_web_ui_server_async(strRootKey.c_str(), strModUuid.c_str(), port);
    std::stringstream sstr;
    sstr << "start_module_web_ui_server_async: " << to_string(err).c_str() << ", port:" << port;
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_testRoot(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey) {
    string strRootKey = jstringToStr(env, rootKey);

    std::string result = skroot_box::get_root_test_report(strRootKey.c_str());
    return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_runRootCmd(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jstring cmd) {
    string strRootKey = jstringToStr(env, rootKey);
    string strCmd = jstringToStr(env, cmd);

    string result;
    SkBoxErr err = skroot_box::run_root_cmd(strRootKey.c_str(), strCmd.c_str(), result);
    stringstream sstr;
    sstr << "run_root_cmd " << to_string(err).c_str() << ", result:" << result;
    return env->NewStringUTF(sstr.str().c_str());
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_rootExecProcessCmd(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jstring cmd) {
    string strRootKey = jstringToStr(env, rootKey);
    string strCmd = jstringToStr(env, cmd);

    SkBoxErr err = skroot_box::root_exec_process(strRootKey.c_str(), strCmd.c_str());
    stringstream sstr;
    sstr << "root_exec_process " << to_string(err).c_str();
    return env->NewStringUTF(sstr.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_getAllCmdlineProcess(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey) {
    string strRootKey = jstringToStr(env, rootKey);

    std::stringstream ss;
    std::map<pid_t, std::string> pid_map;
    SkBoxErr err = skroot_box::get_all_cmdline_process(strRootKey.c_str(), pid_map);
    if (is_failed(err)) {
        ss << "get_all_cmdline_process " << to_string(err).c_str() << std::endl;
        return env->NewStringUTF(ss.str().c_str());
    }
    cJSON *root = cJSON_CreateArray();
    for (auto & iter : pid_map) {
        cJSON *item = cJSON_CreateObject();
        size_t len = iter.second.length();
        size_t max_encoded_len = 3 * len + 1;
        shared_ptr<char> spData(new (std::nothrow) char[max_encoded_len], std::default_delete<char[]>());
        memset(spData.get(), 0, max_encoded_len);
        url_encode(const_cast<char*>(iter.second.c_str()), spData.get());
        cJSON_AddNumberToObject(item, "pid",  iter.first);
        cJSON_AddStringToObject(item, "name", spData.get());
        cJSON_AddItemToArray(root, item);
    }
    ss << cJSON_Print(root);
    cJSON_Delete(root);
    return env->NewStringUTF(ss.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_parasitePrecheckApp(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jstring targetProcessCmdline) {
    string strRootKey = jstringToStr(env, rootKey);
    string strTargetProcessCmdline = jstringToStr(env, targetProcessCmdline);

    stringstream sstr;
    std::set<pid_t> test_pid;
    SkBoxErr err = skroot_box::find_all_cmdline_process(strRootKey.c_str(), strTargetProcessCmdline.c_str(), test_pid);
    if (is_failed(err)) {
        sstr << "find_all_cmdline_process " << to_string(err).c_str() << ", cnt:"<< test_pid.size() << std::endl;
        return env->NewStringUTF(sstr.str().c_str());
    }
    if (test_pid.size() == 0) {
        sstr << "目标进程不存在" << std::endl;
        return env->NewStringUTF(sstr.str().c_str());
    }

    std::map<std::string, skroot_box::AppDynlibStatus> dynlibPathList;
    err = skroot_box::parasite_precheck_app(strRootKey.c_str(), strTargetProcessCmdline.c_str(), dynlibPathList);
    if (is_failed(err)) {
        sstr << "parasite_precheck_app ret val:" << to_string(err).c_str() << std::endl;
        if(err == SkBoxErr::ERR_EXIST_32BIT) sstr << "此目标APP为32位应用，无法寄生" << std::endl;
        return env->NewStringUTF(sstr.str().c_str());
    }

    if (!dynlibPathList.size()) {
        sstr << "无法检测到目标APP的JNI环境，目标APP暂不可被寄生；您可重新运行目标APP后重试；或将APP进行手动加固(加壳)，因为加固(加壳)APP后，APP会被产生JNI环境，方可寄生！" << std::endl;
        return env->NewStringUTF(sstr.str().c_str());
    }

    cJSON *root = cJSON_CreateArray();
    for (auto & iter : dynlibPathList) {
        cJSON *item = cJSON_CreateObject();
        size_t len = iter.first.length();
        size_t max_encoded_len = 3 * len + 1;
        std::shared_ptr<char> spData(new (std::nothrow) char[max_encoded_len], std::default_delete<char[]>());
        memset(spData.get(), 0, max_encoded_len);
        url_encode(const_cast<char*>(iter.first.c_str()), spData.get());
        cJSON_AddStringToObject(item, "name", spData.get());
        cJSON_AddNumberToObject(item, "status",  iter.second);
        cJSON_AddItemToArray(root, item);
    }
    sstr << cJSON_Print(root);
    cJSON_Delete(root);
    return env->NewStringUTF(sstr.str().c_str());

}

extern "C" JNIEXPORT jstring JNICALL
Java_com_linux_permissionmanager_bridge_NativeBridge_parasiteImplantApp(
        JNIEnv* env,
        jclass /* this */,
        jstring rootKey,
        jstring targetProcessCmdline,
        jstring targetSoFullPath) {
    string strRootKey = jstringToStr(env, rootKey);
    string strTargetProcessCmdline = jstringToStr(env, targetProcessCmdline);
    string strTargetSoFullPath = jstringToStr(env, targetSoFullPath);

    stringstream sstr;
    SkBoxErr err = skroot_box::parasite_implant_app(strRootKey.c_str(), strTargetProcessCmdline.c_str(), strTargetSoFullPath.c_str());
    if (is_failed(err)) {
        sstr << "parasite_implant_app " << to_string(err).c_str() << std::endl;
        return env->NewStringUTF(sstr.str().c_str());
    }
    sstr << "parasite_implant_app done.";
    return env->NewStringUTF(sstr.str().c_str());

}

