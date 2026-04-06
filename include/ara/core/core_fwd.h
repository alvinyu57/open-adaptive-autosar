#pragma once

#include <cstdint>
#include <memory>

namespace ara::core {

class ErrorDomain;
class ErrorCode;
class Exception;
class CoreErrorDomain;

enum class CoreErrc : std::int32_t;

using ErrorDomainIdType = std::uint64_t;

template <typename T, typename Allocator>
class Vector;

template <typename CharT, typename Traits, typename Allocator>
class BasicString;

} // namespace ara::core
