#pragma once

#include "istatus.h"
#include "macros.h"

#include <memory>
#include <string>
#include <vector>

namespace trailofbits {
class SRCPARSER_PUBLICSYMBOL ISourceCodeParser {
 public:
  enum class StatusCode {
    OpenFailed,
    MemoryAllocationFailure,
    ParsingError,
    SucceededWithWarnings,
    Unknown
  };

  using Status = IStatus<StatusCode>;

  virtual ~ISourceCodeParser() = default;

  virtual Status processFile(const std::string &path) const = 0;
  virtual Status processBuffer(const std::string &buffer) const = 0;
};

SRCPARSER_PUBLICSYMBOL ISourceCodeParser::Status CreateSourceCodeParser(
    std::unique_ptr<ISourceCodeParser> &obj,
    const std::vector<std::string> &additional_include_dirs);
}  // namespace trailofbits
