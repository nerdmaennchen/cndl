#pragma once

#include <string>
#include <string_view>

#include <time.h>

namespace cndl {

// tm is expected to be given in UTC (e.g., from gmtime_r)
std::string mkdatestr(struct tm const& tm);
std::string mkdatestr(struct timespec const& ts);

// the returned tm is in UTC (use timegm to convert it)
struct tm parsedatestr(std::string_view str);

}