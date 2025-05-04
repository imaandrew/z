#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

class SourceManager {
    std::vector<char> input;
    std::optional<std::filesystem::path> path;

    SourceManager(std::filesystem::path& path, std::vector<char> input) : path(path), input(input) {};
    SourceManager(std::vector<char> input) : input(input), path(std::nullopt) {};
public:
    static SourceManager Create(std::vector<char> input) {
        return SourceManager(input);
    }

    static SourceManager Create(const char* path) {
        auto p = std::filesystem::path(path);
        if (!std::filesystem::exists(p)) {
            // err
        }

        auto file = std::ifstream(p, std::ios::ate);
        if (!file.is_open()) {
            // err
        }

        auto size = file.tellg();
        file.seekg(0);
        auto input = std::vector<char>(size);
        if (!file.read(&input[0], size)) {
            // err
        }

        return SourceManager(p, input);
    }

    inline std::optional<char> get_char(std::size_t index) const {
        if (in_bounds(index)) {
            return std::nullopt;
        }

        return input[index];
    }

    inline std::optional<std::string> get_line(std::size_t index) {
        if (in_bounds(index)) {
            return std::nullopt;
        }

        size_t start = index;
        size_t end = index;

        while (start > 0 && input[start - 1] != '\n')
            start--;

        while (!in_bounds(end) && input[end] != '\n')
            end++;

        return std::string(input.data() + start, input.data() + end);
    }

    inline bool in_bounds(std::size_t index) const {
        return index >= input.size();
    }

    inline const char* get_char_ptr(std::size_t index) const {
        if (in_bounds(index))
            return nullptr;

        return &input[index];
    }

    inline const std::string get_path() const {
        if (path) {
            return path->string();
        }

        return std::string("asdf");
    }
};
