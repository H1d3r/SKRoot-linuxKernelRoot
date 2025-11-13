#pragma once
#include <string.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <tuple>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "cJSON.h"

static void writeToLog(const std::string & message) {
    printf("%s\n", message.c_str());
    // std::ofstream logFile("/sdcard/web_server.log", std::ios::app);
    // if (!logFile) {
    //     std::cerr << "Error opening file" << std::endl;
    //     return;
    // }
    // logFile << message << std::endl;
    // logFile.close();
    // std::cout << message << std::endl;
}