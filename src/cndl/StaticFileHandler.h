#pragma once

#include "Route.h"

#include <filesystem>

namespace cndl {

struct StaticFileHandler {
    StaticFileHandler(std::filesystem::path base_dir);

    OptResponse operator()(Request const& request, std::string const& ressource) const;
    bool can_serve_ressource(std::string const& ressource) const;
private:
    std::filesystem::path base_dir;
};

}