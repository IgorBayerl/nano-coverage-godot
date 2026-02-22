#include "persistence.h"

#include <cstring>  // <--- FIX: Required for strncmp
#include <fstream>
#include <iostream>

namespace godot {

namespace {

// Simple Binary Helpers
template <typename T>
void write_val(std::ofstream& out, T val) {
    out.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

void write_string(std::ofstream& out, const std::string& str) {
    uint32_t len = static_cast<uint32_t>(str.size());
    write_val(out, len);
    out.write(str.data(), len);
}

template <typename T>
bool read_val(std::ifstream& in, T& val) {
    in.read(reinterpret_cast<char*>(&val), sizeof(T));
    return in.good();
}

bool read_string(std::ifstream& in, std::string& str) {
    uint32_t len = 0;
    if (!read_val(in, len))
        return false;
    str.resize(len);
    in.read(str.data(), len);
    return in.good();
}

}  // namespace

bool Persistence::save_metadata(const std::string& path, const CoverageMetadata& metadata) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        return false;

    // Header: "COVM" (Coverage Meta)
    out.write("COVM", 4);

    uint32_t file_count = static_cast<uint32_t>(metadata.size());
    write_val(out, file_count);

    for (const auto& kv : metadata) {
        write_string(out, kv.first);  // File Path

        uint32_t line_count = static_cast<uint32_t>(kv.second.size());
        write_val(out, line_count);

        for (uint32_t line : kv.second) {
            write_val(out, line);
        }
    }
    return true;
}

bool Persistence::load_metadata(const std::string& path, CoverageMetadata& out_metadata) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
        return false;

    char magic[4];
    in.read(magic, 4);
    if (strncmp(magic, "COVM", 4) != 0)
        return false;

    uint32_t file_count = 0;
    read_val(in, file_count);

    for (uint32_t i = 0; i < file_count; ++i) {
        std::string file_path;
        if (!read_string(in, file_path))
            break;

        uint32_t line_count = 0;
        read_val(in, line_count);

        std::vector<uint32_t> lines;
        lines.reserve(line_count);
        for (uint32_t j = 0; j < line_count; ++j) {
            uint32_t line;
            read_val(in, line);
            lines.push_back(line);
        }
        out_metadata[file_path] = std::move(lines);
    }
    return true;
}

bool Persistence::append_execution_data(const std::string& path, const CoverageData& data) {
    // Open in Append + Binary mode
    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out.is_open())
        return false;

    // Session Marker: "SESS"
    out.write("SESS", 4);

    uint32_t file_count = static_cast<uint32_t>(data.size());
    write_val(out, file_count);

    for (const auto& file_kv : data) {
        write_string(out, file_kv.first);  // File Path

        const auto& lines = file_kv.second;
        uint32_t active_lines = static_cast<uint32_t>(lines.size());
        write_val(out, active_lines);

        for (const auto& line_kv : lines) {
            write_val(out, line_kv.first);   // Line Num
            write_val(out, line_kv.second);  // Hit Count
        }
    }
    return true;
}

bool Persistence::load_and_merge_execution_data(const std::string& path, CoverageData& out_data) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
        return false;

    // Loop until EOF to read all concatenated sessions
    while (in.peek() != EOF) {
        char magic[4];
        in.read(magic, 4);
        // If we fail to read magic or it's wrong, stop (or partial read)
        if (in.gcount() != 4 || strncmp(magic, "SESS", 4) != 0) {
            break;
        }

        uint32_t file_count = 0;
        read_val(in, file_count);

        for (uint32_t i = 0; i < file_count; ++i) {
            std::string file_path;
            read_string(in, file_path);

            uint32_t line_count = 0;
            read_val(in, line_count);

            for (uint32_t j = 0; j < line_count; ++j) {
                uint32_t line_num;
                uint64_t hits;
                read_val(in, line_num);
                read_val(in, hits);

                // Merge (Sum) hits
                out_data[file_path][line_num] += hits;
            }
        }
    }
    return true;
}

}  // namespace godot