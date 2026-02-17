#pragma once

namespace z::colour {

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
inline const char* RESET = "\033[0m";
inline const char* BOLD = "\033[1m";
inline const char* DIM = "\033[2m";

inline const char* RED = "\033[31m";
inline const char* GREEN = "\033[32m";
inline const char* YELLOW = "\033[33m";
inline const char* BLUE = "\033[34m";
inline const char* MAGENTA = "\033[35m";
inline const char* CYAN = "\033[36m";
inline const char* GRAY = "\033[90m";

inline const char* BOLD_GREEN = "\033[1;32m";
inline const char* BOLD_YELLOW = "\033[1;33m";
inline const char* BOLD_CYAN = "\033[1;36m";
inline const char* BOLD_MAGENTA = "\033[1;35m";
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

inline void disable() {
    RESET = "";
    BOLD = "";
    DIM = "";
    RED = "";
    GREEN = "";
    YELLOW = "";
    BLUE = "";
    MAGENTA = "";
    CYAN = "";
    GRAY = "";
    BOLD_GREEN = "";
    BOLD_YELLOW = "";
    BOLD_CYAN = "";
    BOLD_MAGENTA = "";
}

} // namespace z::colour
