#include <filesystem>
#include "Parameter.h"

namespace sargp
{

struct File final : std::filesystem::path {
    using std::filesystem::path::path;
};

struct Directory final : std::filesystem::path {
    using std::filesystem::path::path;
};

}