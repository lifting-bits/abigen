#pragma once

#include <trailofbits/srcparser/isourcecodeparser.h>
#include <trailofbits/srcparser/macros.h>

namespace trailofbits {
class SourceCodeParser final : public ISourceCodeParser {
 public:
  SourceCodeParser(const std::vector<std::string> &additional_include_dirs);
  virtual ~SourceCodeParser();

  virtual Status processFile(const std::string &path) const override;
  virtual Status processBuffer(const std::string &buffer) const override;

 private:
  struct PrivateData;
  std::unique_ptr<PrivateData> d;

  SourceCodeParser &operator=(const SourceCodeParser &other) = delete;
  SourceCodeParser(const SourceCodeParser &other) = delete;
};

SRCPARSER_PUBLICSYMBOL ISourceCodeParser::Status CreateSourceCodeParser(
    std::unique_ptr<ISourceCodeParser> &obj,
    const std::vector<std::string> &additional_include_dirs);
}  // namespace trailofbits
