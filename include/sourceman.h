#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class SourceManager {
    std::vector<char> input;
    std::optional<std::filesystem::path> path;

    SourceManager(std::filesystem::path& path, std::vector<char> input)
        : input(std::move(input)), path(path) {};
    explicit SourceManager(std::vector<char> input)
        : input(std::move(input)), path(std::nullopt) {};

public:
    static SourceManager Create(std::vector<char> input) {
        return SourceManager(std::move(input));
    }

    static SourceManager Create(const char* path_) {
        auto path = std::filesystem::path(path_);
        if (!std::filesystem::exists(path)) {
            // err
        }

        auto file = std::ifstream(path, std::ios::ate);
        if (!file.is_open()) {
            // err
        }

        auto size = file.tellg();
        file.seekg(0);
        auto input = std::vector<char>(size);
        if (!file.read(input.data(), size)) {
            // err
        }

        return SourceManager(path, input);
    }

    [[nodiscard]] std::optional<char> get_char(const std::size_t index) const {
        if (in_bounds(index)) {
            return std::nullopt;
        }

        return input[index];
    }

    [[nodiscard]] std::optional<std::string>
    get_line(const std::size_t index) const {
        if (in_bounds(index)) {
            return std::nullopt;
        }

        size_t start = index;
        size_t end = index;

        while (start > 0 && input[start - 1] != '\n')
            start--;

        while (!in_bounds(end) && input[end] != '\n')
            end++;

        return std::string(&input[start], end - start);
    }

    [[nodiscard]] bool in_bounds(const std::size_t index) const {
        return index >= input.size();
    }

    [[nodiscard]] const char* get_char_ptr(const std::size_t index) const {
        if (in_bounds(index))
            return nullptr;

        return &input[index];
    }

    [[nodiscard]] std::string get_path() const {
        if (path) {
            return path->string();
        }

        return std::string("asdf");
    }
};
