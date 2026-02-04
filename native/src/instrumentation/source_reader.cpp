#include "source_reader.h"
#include <fstream>
#include <vector>
#include <cstdint>

namespace NanoCoverage {

namespace {

// Helper to check if a buffer starts with a sequence
bool starts_with(const std::vector<uint8_t>& buffer, const std::vector<uint8_t>& prefix) {
    if (buffer.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (buffer[i] != prefix[i]) return false;
    }
    return true;
}

// Simple UTF-8 validation
bool is_valid_utf8(const std::string& str) {
    const unsigned char* bytes = (const unsigned char*)str.data();
    size_t len = str.size();
    size_t i = 0;
    while (i < len) {
        unsigned char c = bytes[i];
        if (c < 0x80) { // 0xxxxxxx
            i++;
        } else if ((c & 0xE0) == 0xC0) { // 110xxxxx 10xxxxxx
            if (i + 1 >= len || (bytes[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) { // 1110xxxx 10xxxxxx 10xxxxxx
            if (i + 2 >= len || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        } else if ((c & 0xF8) == 0xF0) { // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            if (i + 3 >= len || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

std::string utf16le_to_utf8(const std::vector<uint8_t>& data, size_t offset) {
    std::string res;
    res.reserve(data.size()); // Estimate
    for (size_t i = offset; i + 1 < data.size(); i += 2) {
        uint16_t c = data[i] | (data[i + 1] << 8);
        if (c < 0x80) {
            res.push_back((char)c);
        } else if (c < 0x800) {
            res.push_back((char)(0xC0 | (c >> 6)));
            res.push_back((char)(0x80 | (c & 0x3F)));
        } else {
            // Basic BMP support
            res.push_back((char)(0xE0 | (c >> 12)));
            res.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            res.push_back((char)(0x80 | (c & 0x3F)));
        }
    }
    return res;
}

std::string utf16be_to_utf8(const std::vector<uint8_t>& data, size_t offset) {
    std::string res;
    res.reserve(data.size());
    for (size_t i = offset; i + 1 < data.size(); i += 2) {
        uint16_t c = (data[i] << 8) | data[i + 1];
        if (c < 0x80) {
            res.push_back((char)c);
        } else if (c < 0x800) {
            res.push_back((char)(0xC0 | (c >> 6)));
            res.push_back((char)(0x80 | (c & 0x3F)));
        } else {
             // Basic BMP support
            res.push_back((char)(0xE0 | (c >> 12)));
            res.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            res.push_back((char)(0x80 | (c & 0x3F)));
        }
    }
    return res;
}

} // namespace

ReadTextResult SourceReader::read_text_file(const std::string& path) {
    ReadTextResult result;
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        result.ok = false;
        result.error_message = "Could not open file: " + path;
        return result;
    }

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (buffer.empty()) {
        result.ok = true;
        result.content = "";
        return result;
    }

    // Check BOMs
    // UTF-8: EF BB BF
    if (starts_with(buffer, {0xEF, 0xBB, 0xBF})) {
        result.content.assign(reinterpret_cast<const char*>(buffer.data() + 3), buffer.size() - 3);
        result.ok = true;
    } 
    // UTF-16 LE: FF FE
    else if (starts_with(buffer, {0xFF, 0xFE})) {
        result.content = utf16le_to_utf8(buffer, 2);
        result.ok = true;
    }
    // UTF-16 BE: FE FF
    else if (starts_with(buffer, {0xFE, 0xFF})) {
        result.content = utf16be_to_utf8(buffer, 2);
        result.ok = true;
    }
    else {
        // No BOM
        std::string raw(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        if (is_valid_utf8(raw)) {
            result.content = raw;
            result.ok = true;
        } else {
            result.ok = false;
            result.error_message = "File is not valid UTF-8 and has no BOM.";
        }
    }

    return result;
}

} // namespace NanoCoverage
