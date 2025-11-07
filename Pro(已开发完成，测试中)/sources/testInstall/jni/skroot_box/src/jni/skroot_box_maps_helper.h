#pragma once
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <set>
#include <filesystem>

namespace skroot_box {
static std::set<std::string> get_all_so_paths(pid_t pid) {
    char line[1024] = { 0 };
    std::set<std::string> so_paths;
    char filename[32] = { 0 };

    if (pid < 0) {
        snprintf(filename, sizeof(filename), "/proc/self/maps");
    } else {
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    }

    FILE* fp = fopen(filename, "r");
    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, ".so")) {
                char* start = strstr(line, "/");
                if (start) {
                    char* end = strchr(start, '\n');
                    if (end) {
                        *end = '\0';
                        so_paths.insert(std::string(start));
                    }
                }
            }
        }
        fclose(fp);
    }

    return so_paths;
}

static std::string get_app_directory(const char* package_name) {
	if(!package_name || strlen(package_name) == 0) { return {}; }
 	char line[4096] = { 0 };
    char filename[1024] = { 0 };
    snprintf(filename, sizeof(filename), "pm path %s", package_name);
	FILE * fp = popen(filename, "r");
    if (fp) {
        fread(line, 1, sizeof(line), fp);
        pclose(fp);
    }
	std::string app_path = line;
	auto start = app_path.find("/");
	if(start != std::string::npos) {
		app_path = app_path.substr(start);
	}
	auto end = app_path.find_last_of("/");
	if(end != std::string::npos) {
		app_path = app_path.substr(0, end);
	}
    return app_path;
}
}