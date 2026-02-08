#ifndef NANO_COVERAGE_SOURCE_READER_H
#define NANO_COVERAGE_SOURCE_READER_H

#include <string>

namespace NanoCoverage {

struct ReadTextResult {
    bool ok = false;
    std::string content;
    std::string error_message;
};

class SourceReader {
   public:
    static ReadTextResult read_text_file(const std::string& path);
};

}  // namespace NanoCoverage

#endif  // NANO_COVERAGE_SOURCE_READER_H
