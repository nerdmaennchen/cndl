#pragma once

#include <string>
#include <string_view>

#include <time.h>

namespace cndl {

std::string mkdatestr(struct tm const& tm);
std::string mkdatestr(struct timespec const& ts);

struct tm parsedatestr(std::string_view str);

}