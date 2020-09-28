#include "loader.h"

#include <cstdio>

namespace qrqma {

TemplateLoader defaultLoader(std::filesystem::path const& template_directory) {
    return [template_directory](std::string const& filename) {
        auto pat = (template_directory / filename).native();
        std::FILE* f = std::fopen(pat.c_str(), "r");
        if (not f) {
            throw std::runtime_error("cannot load template file \"" + filename + "\"");
        }
        std::string templ;
        while (true) {
            std::array<char, 4096> buf;
            int r = std::fread(&buf[0], sizeof(buf[0]), buf.size(), f);
            if (r == 0) {
                break;
            }
            templ += std::string(buf.data(), r);
        }
        std::fclose(f);
        return templ;
    };
}

}