#pragma once
#include <iostream>
#include <fstream>

static bool read_text_file(const char* path, std::string& out) {
    if (!path) return false;
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if (!f) return false;

    f.seekg(0, std::ios::end);
    std::streamsize n = f.tellg();
    if (n < 0) return false;

    out.resize(static_cast<size_t>(n));
    f.seekg(0, std::ios::beg);
    if (!f.read(out.data(), n)) return false;
    return true;
}

static bool write_text_file(const char* target_path, std::string_view text) {
    if (!target_path) return false;
    std::ofstream file(target_path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) return false;
    file << text;
    return file.good();
}