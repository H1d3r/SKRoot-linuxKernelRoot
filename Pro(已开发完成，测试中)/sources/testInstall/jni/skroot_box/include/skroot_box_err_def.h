#pragma once
#include <string>
#include <string_view>
#include <ostream>
#include <type_traits>
#include <sys/types.h>

#define SKBOXERR_LIST_INIT(X) \
X(OK,        = 0)        \
X(ERR_PARAM,       = -10000)

#define SKBOXERR_LIST_AUTO(X) \
X(ERR_NO_ROOT,) \
X(ERR_READ_CHILD_ERRCODE,) \
X(ERR_READ_CHILD_STRING,) \
X(ERR_READ_CHILD_INT,) \
X(ERR_READ_CHILD_UINT64,) \
X(ERR_READ_CHILD_SET_ARR,) \
X(ERR_READ_CHILD_MAP_I_S,) \
X(ERR_READ_CHILD_MAP_S_I,) \
X(ERR_READ_EOF,) \
X(ERR_NO_MEM,) \
X(ERR_EXECVE,) \
X(ERR_KILL,) \
X(ERR_APP_DIR,) \
X(ERR_FIND_CMDLINE_PROC,) \
X(ERR_EXIST_32BIT,) \
X(ERR_NOT_EXIST_ORIGINAL_FILE,) \
X(ERR_NOT_EXIST_IMPLANT_FILE,) \
X(ERR_CHMOD,) \
X(ERR_COPY_SELINUX,) \
X(ERR_LINK_SO,) \
X(ERR_CHECK_LINK_SO,) \
X(ERR_WRITE_ROOT_SERVER,) \
X(ERR_POPEN,) \
X(ERR_OPEN_FILE,) \
X(ERR_OPEN_DIR,) \
X(ERR_PID_NOT_FOUND,)

enum class SkBoxErr : ssize_t {
#define DECL_ENUM(name, assign) name assign,
    SKBOXERR_LIST_INIT(DECL_ENUM)
#undef DECL_ENUM
#define DECL_ENUM(name, assign) name,
    SKBOXERR_LIST_AUTO(DECL_ENUM)
#undef DECL_ENUM
};

constexpr bool is_ok(SkBoxErr err) noexcept {
    return err == SkBoxErr::OK;
}

constexpr bool is_failed(SkBoxErr err) noexcept {
    return err != SkBoxErr::OK;
}

#define RETURN_IF_ERROR_SKBOX(expr)                                             \
    do {                                                                  \
        static_assert(                                                   \
            std::is_same<decltype(expr), SkBoxErr>::value,                \
            "RETURN_IF_ERROR_SKBOX: expr 必须返回 SkBoxErr"                       \
        );                                                                \
        SkBoxErr _err = (expr);                                            \
        if (is_failed(_err))                                      \
            return _err;                                                  \
    } while (0)

#define BREAK_IF_ERROR_SKBOX(expr)                                              \
    do {                                                                  \
        static_assert(                                                   \
            std::is_same<decltype(expr), SkBoxErr>::value,                \
            "BREAK_IF_ERROR_SKBOX: expr 必须返回 SkBoxErr"                        \
        );                                                                \
        SkBoxErr _err = (expr);                                            \
        if (is_failed(_err))                                      \
            break;                                                        \
    } while (0)


constexpr ssize_t to_num(SkBoxErr err) noexcept {
    return static_cast<ssize_t>(err);
}

constexpr std::string_view skboxerr_name_sv(SkBoxErr e) {
    switch (e) {
#define DECL_CASE(name, assign) case SkBoxErr::name: return std::string_view(#name);
        SKBOXERR_LIST_INIT(DECL_CASE)
        SKBOXERR_LIST_AUTO(DECL_CASE)
#undef DECL_CASE
    }
    return {};
}

inline std::string to_string(SkBoxErr e) {
    if (auto sv = skboxerr_name_sv(e); !sv.empty()) return std::string(sv);
    return "Unknown(" + std::to_string(static_cast<ssize_t>(e)) + ")";
}