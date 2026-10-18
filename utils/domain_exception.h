#pragma once

#include <stdexcept>
#include <string>

namespace scapms::utils {

class DomainException : public std::invalid_argument {
public:
    explicit DomainException(const std::string& message)
        : std::invalid_argument(message) {}
};

} // namespace scapms::utils
