#pragma once

#include <memory>

#include "ara/core/application.hpp"

namespace openaa::examples::hello_world {

std::unique_ptr<ara::core::Application> CreateHelloWorldApp();

} // namespace openaa::examples::hello_world
