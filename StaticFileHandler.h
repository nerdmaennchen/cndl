#pragma once

#include "Route.h"

#include <filesystem>

namespace cndl {

struct StaticFileHandler {
    StaticFileHandler(std::filesystem::path base_dir);

    OptResponse operator()(Request const& request, std::string const& resource) const;
    bool can_serve_resource(std::string const& resource) const;
private:
    std::filesystem::path base_dir;
};

}
