//
// Created by zhangfuwen on 2022/1/25.
//

#ifndef AUDIO_IME_COMMON_H
#define AUDIO_IME_COMMON_H
#include "common_log.h"
#include "configor/json.hpp"
#include <filesystem>
#include <fstream>
#include <glib/gi18n.h>
#include <locale.h>

struct CandidateAttr {
    bool _isPinyin;
    explicit CandidateAttr(bool isPinyin = false)
        : _isPinyin(isPinyin) {}
};

#define CONF_SECTION "engine/ibus_fun"
#define CONF_NAME_ID "access_id"         // no captal letter allowed
#define CONF_NAME_SECRET "access_secret" // no captal letter allowed
#define CONF_NAME_WUBI "wubi_table"
#define CONF_NAME_PINYIN "pinyin_enable"
#define CONF_NAME_SPEECH "speech_enable"
#define CONF_NAME_ORIENTATION "orientation"
#define CONF_NAME_FAST_INPUT_ENABLED "fast_input_enabled"
#define CONF_NAME_FAST_INPUT_RELOAD "fast_input_reload"

#define GETTEXT_PACKAGE "messages"

static inline std::string get_ibus_fun_user_data_dir() {
    std::string ret = getenv("HOME");
    ret += "/.config/ibus/fun/";
    return ret;
}

static inline std::map<std::string, std::string> load_fast_input_config(const std::string &filename = "") {
    std::map<std::string, std::string> m;
    FUN_INFO("loading file %s", filename.c_str());

    std::string file_path;
    if (filename.empty()) {
        auto user_dir = get_ibus_fun_user_data_dir();
        file_path = user_dir + "fast_input.json";
    } else {
        file_path = filename;
    }
    if (!std::filesystem::is_regular_file(file_path)) {
        FUN_ERROR("can't open irregular file %s", file_path.c_str());
        return m;
    }

    FUN_INFO("loading file %s", file_path.c_str());
    std::ifstream ifs(file_path);
    configor::json j;
    try {
        ifs >> j;
    } catch (std::exception &e) {
        FUN_ERROR("failed to read configuration file, %s", e.what());
    }
    FUN_INFO("json size %d", j.size());

    for (auto it = j.begin(); it != j.end(); it++) {
        m[it.key()] = it.value().as_string();
        FUN_INFO("json read record %s %s", it.key().c_str(), it.value().as_string().c_str());
    }
    return m;
}

static inline std::string exec(const char *cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// trim from start (in place)
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return s;
}

// trim from end (in place)
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

// trim from both ends (in place)
static inline std::string &trim(std::string &s) {
    ltrim(s);
    rtrim(s);
    return s;
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

#endif // AUDIO_IME_COMMON_H
