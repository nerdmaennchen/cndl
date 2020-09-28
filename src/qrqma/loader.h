#pragma once

#include <functional>
#include <string>

#include <filesystem>

namespace qrqma {

using TemplateLoader = std::function<std::string(std::string const&)>;
TemplateLoader defaultLoader(std::filesystem::path const& template_directory = {"./"});

}