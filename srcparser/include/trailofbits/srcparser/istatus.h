#pragma once

#include "macros.h"

#include <sstream>
#include <string>

namespace trailofbits {
template <typename StatusCodeType>
class SRCPARSER_PUBLICSYMBOL IStatus final {
  bool success;
  StatusCodeType status_code;
  std::string status_message;

 public:
  IStatus(bool success = false,
          StatusCodeType status_code = StatusCodeType::Unknown,
          const std::string &status_message = std::string())
      : success(success),
        status_code(status_code),
        status_message(status_message) {}

  bool succeeded() const { return success; }

  StatusCodeType statusCode() const { return status_code; }

  std::string message() const { return status_message; }

  std::string toString() const {
    std::stringstream buffer;

    buffer << (success ? "Succeeded" : "Failed") << " with status code "
           << static_cast<int>(status_code);
    if (!status_message.empty()) {
      buffer << " (" << status_message << ")";
    }

    return buffer.str();
  }
};
}  // namespace trailofbits
