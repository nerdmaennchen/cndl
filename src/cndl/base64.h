#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace cndl {

std::string base64_encode(std::basic_string_view<std::byte> s);
std::vector<std::byte> base64_decode(std::string_view s);

}
