#pragma once

#include <memory>

#include "openaa/core/application.hpp"

namespace openaa::examples::hello_world {

std::unique_ptr<openaa::core::Application> CreateHelloWorldApp();

} // namespace openaa::examples::hello_world
