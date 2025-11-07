#include <string.h>
#include <thread>
#include <vector>
#include <sstream>
#include <filesystem>

#include "skroot_box_umbrella.h"
#include "kernel_module_kit_umbrella.h"

char ROOT_KEY[256] = {0};
constexpr const char* recommend_files[] = {"libc++_shared.so"};

static void print_sep(char c) {
    for (int i = 0; i < 32; ++i) std::putchar(c);
    std::putchar('\n');
}

static void print_desc_info(const skroot_env::module_desc& desc) {
    printf("----- SKRoot Module Meta -----\n");
    printf("Name    : %s\n", desc.name);
    printf("Version : %s\n", desc.version);
    printf("Desc    : %s\n", desc.desc);
    printf("Author  : %s\n", desc.author);
    printf("UUID    : %s\n", desc.uuid);
    bool any_feature = desc.web_ui;
    if (any_feature) {
        std::puts("Features:");
		if(desc.web_ui) std::printf("  [+] Web UI\n");
    }
    auto & sdk = desc.min_sdk_ver;
    printf("MinSDK  : %u.%u.%u\n", sdk.major, sdk.minor, sdk.patch);
    printf("------------------------------\n");
}

void test_install_skroot() {
	KModErr err = skroot_env::install_skroot_environment(ROOT_KEY);
    printf("install_skroot_environment: %s\n", to_string(err).c_str());
	if(is_ok(err)) printf("将在重启后生效\n");
}

void test_uninstall_skroot() {
	KModErr err = skroot_env::uninstall_skroot_environment(ROOT_KEY);
    printf("uninstall_skroot_environment: %s\n", to_string(err).c_str());
	if(is_ok(err)) printf("将在重启后生效\n");
}

void test_show_skroot_ver() {
	if(!skroot_env::is_installed_skroot_environment(ROOT_KEY)) {
		printf("not install skroot!\n");
		return;
	}
	skroot_env::SkrootSdkVersion ver;
	KModErr err = skroot_env::get_installed_skroot_environment_version(ROOT_KEY, ver);
    printf("get_installed_skroot_environment_version: %s, %d.%d.%d\n", to_string(err).c_str(), ver.major, ver.minor, ver.patch);
}

void test_show_skroot_log() {
	std::string log;
	KModErr err = skroot_env::read_skroot_autorun_log(ROOT_KEY, log);
    printf("%s\n", log.c_str());
    printf("read_skroot_autorun_log: %s\n", to_string(err).c_str());
}

void test_root() {
	printf("%s\n", skroot_box::get_root_test_report(ROOT_KEY));
}

void test_run_root_cmd(int argc, char* argv[]) {
	std::stringstream sstrCmd;
	for (int i = 0; i < argc; i++) {
		sstrCmd << argv[i];
		if (i != (argc - 1)) {
			sstrCmd << " ";
		}
	}
	printf("test_run_root_cmd(%s)\n", sstrCmd.str().c_str());

	std::string result;
	SkBoxErr err = skroot_box::run_root_cmd(ROOT_KEY, sstrCmd.str().c_str(), result);
	printf("test_run_root_cmd err:%s\n", to_string(err).c_str());
	printf("test_run_root_cmd result:%s\n", result.c_str());
}

void test_root_exec_process(int argc, char* argv[]) {
	std::stringstream ss;
	for (int i = 0; i < argc; i++) {
		ss << argv[i];
		if (i != (argc - 1)) {
			ss << " ";
		}
	}
	printf("test_root_exec_process(%s)\n", ss.str().c_str());

	SkBoxErr err = skroot_box::root_exec_process(ROOT_KEY, ss.str().c_str());
	printf("root_exec_process err:%s\n", to_string(err).c_str());
}

void test_add_su_list(const char* app_package_name) {
	KModErr err = skroot_env::add_su_auth_list(ROOT_KEY, app_package_name);
	printf("add_su_auth_list err: %s\n", to_string(err).c_str());
}

void test_remove_su_list(const char* app_package_name) {
	KModErr err = skroot_env::remove_su_auth_list(ROOT_KEY, app_package_name);
	printf("remove_su_auth_list err: %s\n", to_string(err).c_str());
}

void test_show_su_list() {
	std::vector<skroot_env::su_auth_item> pkgs;
    KModErr err = skroot_env::get_su_auth_list(ROOT_KEY, pkgs);
    printf("get_su_auth_list err: %s, cnt:%zd\n", to_string(err).c_str(), pkgs.size());
    for(auto & item : pkgs) {
    	printf("su app: %s\n", item.app_package_name);
    }
}

void test_clear_su_list() {
	KModErr err = skroot_env::clear_su_auth_list(ROOT_KEY);
	printf("clear_su_auth_list err: %s\n", to_string(err).c_str());
}

void test_add_module(const char* zip_file_path) {
	KModErr err = skroot_env::install_module(ROOT_KEY, zip_file_path);
	printf("install_module err: %s\n", to_string(err).c_str());
	if(is_ok(err)) printf("将在重启后生效\n");
}

void test_remove_module(const char* mod_uuid) {
	KModErr err = skroot_env::uninstall_module(ROOT_KEY, mod_uuid);
	printf("uninstall_module err: %s\n", to_string(err).c_str());
	if(is_ok(err)) printf("将在重启后生效\n");
}

void test_show_module_list() {
	std::vector<skroot_env::module_desc> list1;
	std::vector<skroot_env::module_desc> list2;
	KModErr err1 = skroot_env::get_all_modules_list(ROOT_KEY, list1, skroot_env::ModuleListMode::All);
	KModErr err2 = skroot_env::get_all_modules_list(ROOT_KEY, list2, skroot_env::ModuleListMode::RunningOnly);
    printf("get_all_modules_list(All) err: %s, cnt:%zd\n", to_string(err1).c_str(), list1.size());
    printf("get_all_modules_list(RunningOnly) err: %s, cnt:%zd\n", to_string(err2).c_str(), list2.size());
	if(is_ok(err1)) {
		printf("All modules list:\n");
		for(auto & m : list1) print_desc_info(m);
	}
	if(is_ok(err2)) {
		printf("Running modules list:\n");
		for(auto & m : list2) printf("name:%s, uuid:%s\n", m.name, m.uuid);
	}
}

void test_parse_module(const char* zip_file_path) {
	skroot_env::module_desc desc;
	KModErr err = skroot_env::parse_module_desc_from_zip_file(ROOT_KEY, zip_file_path, desc);
	printf("parse_module_desc_from_zip_file err: %s\n", to_string(err).c_str());
	if(is_ok(err)) print_desc_info(desc);
}

void test_open_module_web_ui(const char* mod_uuid) {
	int port = 0;
    KModErr err = skroot_env::features::web_ui::start_module_web_ui_server_async(ROOT_KEY, mod_uuid, port);
    printf("start_module_web_ui_server_async err: %s, port:%d\n", to_string(err).c_str(), port);
    while(1) {
        sleep(1);
    }
}

void test_implant_app(const char* target_pid_cmdline) {
	if (is_failed(skroot_box::get_root_proxy(ROOT_KEY))) return;

	// 1.寄生预检目标APP
	std::set<pid_t> pid_arr;
	SkBoxErr err = skroot_box::find_all_cmdline_process(ROOT_KEY, target_pid_cmdline, pid_arr);
	if (is_failed(err)) {
		printf("find_all_cmdline_process err:%s\n", to_string(err).c_str());
		return;
	}
	if (pid_arr.size() == 0) {
		printf("请先运行目标APP: %s\n", target_pid_cmdline);
		return;
	}
	std::map<std::string, skroot_box::AppDynlibStatus> so_path_list;
	err = skroot_box::parasite_precheck_app(ROOT_KEY, target_pid_cmdline, so_path_list);
	if (is_failed(err)) {
		printf("parasite_precheck_app err:%s\n", to_string(err).c_str());
		if(err == SkBoxErr::ERR_EXIST_32BIT) printf("此目标APP为32位应用，无法寄生\n");
		return;
	}
	if (!so_path_list.size()) {
		printf("无法检测到目标APP的JNI环境，目标APP暂不可被寄生；您可重新运行目标APP后重试；或将APP进行手动加固(加壳)，因为加固(加壳)APP后，APP会被产生JNI环境，方可寄生！\n");
		return;
	}
	printf("请在以下的目标APP文件列表中选择一个即将要被寄生的文件:\n");

	std::vector<std::tuple<std::string, skroot_box::AppDynlibStatus>> sort_printf;
	for (const auto& item : so_path_list) {
		if(item.second != skroot_box::AppDynlibStatus::Running) continue;
		sort_printf.push_back({item.first, item.second});
	}
	for (const auto& item : so_path_list) {
		if(item.second != skroot_box::AppDynlibStatus::NotRunning) continue;
		sort_printf.push_back({item.first, item.second});
	}

	for (const auto& item : sort_printf) {
		auto file_path = std::get<0>(item);
		auto app_so_status = std::get<1>(item);
		std::filesystem::path filePath(file_path);
		std::string file_name = filePath.filename().string();
		std::string status = app_so_status == skroot_box::AppDynlibStatus::Running ? "(正在运行)" : "(未运行)";
		if(app_so_status == skroot_box::AppDynlibStatus::Running) {
			for(auto x = 0; x < sizeof(recommend_files) / sizeof(recommend_files[0]); x++) {
				if(file_name == recommend_files[x]) status = "(推荐， 正在运行)";
			}
		}
		printf("\t%s %s\n", file_name.c_str(), status.c_str());
	}
	printf("\n");
	printf("请输入将要被寄生的文件名称: ");
	std::string user_input_so_name;
	std::getline(std::cin, user_input_so_name);
	printf("\n");
	auto it = std::find_if(so_path_list.begin(), so_path_list.end(), 
        [&](const auto& s) { return s.first.find(user_input_so_name) != std::string::npos; });
    if (it == so_path_list.end()) {
		printf("Not found: %s\n", user_input_so_name.c_str());
		return;
    }
	
	// 3.寄生植入目标APP
	err = skroot_box::parasite_implant_app(ROOT_KEY, target_pid_cmdline, it->first.c_str());
	printf("parasite_implant_app err:%s\n", to_string(err).c_str());
	if (is_failed(err)) return;

	// 4.杀光所有历史进程
	for (pid_t pid : pid_arr) skroot_box::kill_process(ROOT_KEY, pid);
}

int main(int argc, char* argv[]) {
	printf(
		"=======================================================\n"
		"本工具名称: SKRoot(Pro) - Linux内核级完美隐藏ROOT演示\n"
		"用法总览:\n"
		"testInstall <command> [args...]\n"
		"---------------------- 基础环境 ----------------------\n");

	printf("%-29s %s\n", "install", "安装 SKRoot 环境");
	printf("%-29s %s\n","uninstall", "卸载 SKRoot 环境");
	printf("%-29s %s\n","show-ver", "查看已安装 SKRoot 版本");
	printf("%-29s %s\n\n","show-log", "查看 SKRoot 开机日志");

	printf("------------------ 权限测试与执行 ------------------\n");
	printf("%-29s %s\n", "test", "测试 ROOT 权限");
	printf("%-29s %s\n", "cmd <command>", "执行 ROOT 命令");
	printf("%-29s %s\n\n","exec <file-path>", "以 ROOT 身份直接执行程序");

	printf("------------------ SU 授权列表管理 ------------------\n");
	printf("%-29s %s\n", "su-list add <package>", "将 APP 加入 SU 授权列表");
	printf("%-29s %s\n", "su-list remove <package>", "将 APP 移出 SU 授权列表");
	printf("%-29s %s\n", "su-list show", "显示 SU 授权列表");
	printf("%-29s %s\n\n","su-list clear", "清空 SU 授权列表");

	printf("---------------------- 模块管理 ----------------------\n");
	printf("%-29s %s\n", "module add <zip_file_path>", "添加 SKRoot 模块");
	printf("%-29s %s\n", "module remove <mod_uuid>", "移除 SKRoot 模块");
	printf("%-29s %s\n","module list", "显示 SKRoot 模块列表");
	printf("%-29s %s\n\n","module desc <zip_file_path>", "解析 SKRoot 模块的描述信息");
	printf("%-29s %s\n\n","module webui <mod_uuid>", "打开 SKRoot 模块 WebUI 页面");

	printf("---------------------- 静态寄生 ----------------------\n");
	printf("%-29s %s\n\n","implant <process-name>", "寄生到目标 APP");

	printf("-------------------------------------------------------\n"
		"本工具特点：\n"
		"新一代SKRoot，跟面具完全不同思路，摆脱面具被检测的弱点，完美隐藏root功能，兼容安卓APP直接JNI稳定调用。\n"
		"如需帮助，请使用对应的命令，或者查看上面的菜单。\n\n");
	++argv;
	--argc;
	if (argc < 1) {
		std::cout << "error param." << std::endl;
		return 0;
	}

	//TODO: 在此修改你的Root key值。
	strncpy(ROOT_KEY, "V11s8uCBHbfCSbYki0T2wR793AYkbe4T6Md29cU52q7KXxrX", sizeof(ROOT_KEY) - 1);

	std::map<std::string, std::function<void()>> command_map = {
		{"install", []() { test_install_skroot(); }},
		{"uninstall", []() { test_uninstall_skroot(); }},
		{"show-ver", []() { test_show_skroot_ver(); }},
		{"show-log", []() { test_show_skroot_log(); }},
		{"test", []() { test_root(); }},
		{"cmd", [argc, argv]() { test_run_root_cmd(argc - 1, argv + 1); }},
		{"exec", [argc, argv]() { test_root_exec_process(argc - 1, argv + 1); }},
		{"su-list", [argv]() {
			const std::string sub = argv[1];
			if(sub == "add") test_add_su_list(argv[2]);
			else if(sub == "remove") test_remove_su_list(argv[2]);
			else if(sub == "show") test_show_su_list();
			else if(sub == "clear") test_clear_su_list();
		}},
		{"module", [argv]() {
			const std::string sub = argv[1];
			if(sub == "add") test_add_module(argv[2]);
			else if(sub == "remove") test_remove_module(argv[2]);
			else if(sub == "list") test_show_module_list();
			else if(sub == "desc") test_parse_module(argv[2]);
			else if(sub == "webui") test_open_module_web_ui(argv[2]);
		}},
		{"implant", [argv]() { test_implant_app(argv[1]); }}
	};

	std::string cmd = argv[0];
	if (command_map.find(cmd) != command_map.end()) {
		command_map[cmd]();
	} else {
		std::cout << "unknown command." << std::endl;
		return 1;
	}
	return 0;
}