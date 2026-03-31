#pragma once

#include <memory>

#include "ara/exec/application.hpp"

namespace openaa::examples::hello_world {

std::unique_ptr<ara::exec::Application> CreateHelloWorldApp();

} // namespace openaa::examples::hello_world
