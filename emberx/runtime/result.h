#pragma once

#include "emberx/status.h"

#include <optional>
#include <string>
#include <variant>

namespace emberx {

struct Error {
    EmberxStatus code;
    std::string field;
    std::string message;
};

template <typename T>
using Result = std::variant<T, Error>;

// No value means validation succeeded.
using ValidationError = std::optional<Error>;

} // namespace emberx