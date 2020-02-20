#pragma once

#include <functional>
#include <string>

namespace qrqma {

using TemplateLoader = std::function<std::string(std::string const&)>;
TemplateLoader defaultLoader();

}