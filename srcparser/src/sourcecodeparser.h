#pragma once

#include <trailofbits/srcparser/isourcecodeparser.h>
#include <trailofbits/srcparser/macros.h>

#include <clang/Frontend/CompilerInstance.h>

namespace trailofbits {
class SourceCodeParser final : public ISourceCodeParser {
 public:
  SourceCodeParser();
  virtual ~SourceCodeParser();

  virtual Status processFile(std::list<StructureType> &structure_type_list, std::list<FunctionType> &function_type_list,
      const std::string &path,
      const SourceCodeParserSettings &settings) const override;

  virtual Status processBuffer(std::list<StructureType> &structure_type_list, std::list<FunctionType> &function_type_list,
      const std::string &buffer,
      const SourceCodeParserSettings &settings) const override;

 private:
  Status createCompilerInstance(
      std::unique_ptr<clang::CompilerInstance> &compiler,
      const SourceCodeParserSettings &settings) const;

  struct PrivateData;
  std::unique_ptr<PrivateData> d;

  SourceCodeParser &operator=(const SourceCodeParser &other) = delete;
  SourceCodeParser(const SourceCodeParser &other) = delete;
};

SRCPARSER_PUBLICSYMBOL ISourceCodeParser::Status CreateSourceCodeParser(
    std::unique_ptr<ISourceCodeParser> &obj);
}  // namespace trailofbits
