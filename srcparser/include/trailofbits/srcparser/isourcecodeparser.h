#pragma once

#include "istatus.h"
#include "macros.h"

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace trailofbits {
struct SourceCodeParserSettings final {
  bool cpp{true};
  std::size_t standard{11};
  bool enable_gnu_extensions{true};

  std::vector<std::string> cxx_system;
  std::vector<std::string> c_system;
  std::string resource_dir;

  std::vector<std::string> additional_include_folders;
};

struct StructureType final {
  std::string name;
  std::map<std::string, std::string> fields;
};

struct FunctionType final {
  std::string name;
  std::string return_type;
  std::map<std::string, std::string> parameters;
};

class SRCPARSER_PUBLICSYMBOL ISourceCodeParser {
 public:
  enum class StatusCode {
    OpenFailed,
    MemoryAllocationFailure,
    ParsingError,
    SucceededWithWarnings,
    InvalidLanguageStandard,
    Unknown
  };

  using Status = IStatus<StatusCode>;

  virtual ~ISourceCodeParser() = default;

  virtual Status processFile(
      std::list<StructureType> &structure_type_list,
      std::list<FunctionType> &function_type_list, const std::string &path,
      const SourceCodeParserSettings &settings) const = 0;

  virtual Status processBuffer(
      std::list<StructureType> &structure_type_list,
      std::list<FunctionType> &function_type_list, const std::string &buffer,
      const SourceCodeParserSettings &settings) const = 0;
};

SRCPARSER_PUBLICSYMBOL ISourceCodeParser::Status CreateSourceCodeParser(
    std::unique_ptr<ISourceCodeParser> &obj);
}  // namespace trailofbits
