#include "source_reader.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <vector>

using namespace NanoCoverage;
namespace fs = std::filesystem;

class SourceReaderTest : public ::testing::Test {
   protected:
    fs::path temp_file_path;

    void SetUp() override {
        // Create a unique temporary file path
        temp_file_path = fs::temp_directory_path() / ("source_reader_test_" + std::to_string(std::rand()) + ".txt");
    }

    void TearDown() override {
        if (fs::exists(temp_file_path)) {
            fs::remove(temp_file_path);
        }
    }

    void WriteFile(const std::vector<uint8_t>& data) {
        std::ofstream file(temp_file_path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
    }
};

TEST_F(SourceReaderTest, ValidUtf8NoBom) {
    std::string content = "Hello World! \xF0\x9F\x98\x80";  // includes emoji
    std::vector<uint8_t> data(content.begin(), content.end());
    WriteFile(data);

    ReadTextResult result = SourceReader::read_text_file(temp_file_path.string());
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.content, content);
}

TEST_F(SourceReaderTest, Utf8WithBom) {
    std::vector<uint8_t> data = {0xEF, 0xBB, 0xBF, 'H', 'e', 'l', 'l', 'o'};
    WriteFile(data);

    ReadTextResult result = SourceReader::read_text_file(temp_file_path.string());
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.content, "Hello");
}

TEST_F(SourceReaderTest, Utf16LeWithBom) {
    // "Hi" in UTF-16LE: H (0048), i (0069)
    std::vector<uint8_t> data = {0xFF, 0xFE, 0x48, 0x00, 0x69, 0x00};
    WriteFile(data);

    ReadTextResult result = SourceReader::read_text_file(temp_file_path.string());
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.content, "Hi");
}

TEST_F(SourceReaderTest, Utf16BeWithBom) {
    // "Hi" in UTF-16BE: H (0048), i (0069)
    std::vector<uint8_t> data = {0xFE, 0xFF, 0x00, 0x48, 0x00, 0x69};
    WriteFile(data);

    ReadTextResult result = SourceReader::read_text_file(temp_file_path.string());
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.content, "Hi");
}

TEST_F(SourceReaderTest, InvalidUtf8NoBom) {
    // Invalid UTF-8 sequence: 0xFF is never valid
    std::vector<uint8_t> data = {'H', 'e', 0xFF, 'l', 'l', 'o'};
    WriteFile(data);

    ReadTextResult result = SourceReader::read_text_file(temp_file_path.string());
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_message, "");
}

TEST_F(SourceReaderTest, EmptyFile) {
    WriteFile({});

    ReadTextResult result = SourceReader::read_text_file(temp_file_path.string());
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.content, "");
}

TEST_F(SourceReaderTest, MissingFile) {
    fs::remove(temp_file_path);
    ReadTextResult result = SourceReader::read_text_file(temp_file_path.string());
    EXPECT_FALSE(result.ok);
}
