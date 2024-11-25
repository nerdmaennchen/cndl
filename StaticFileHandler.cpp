#include "StaticFileHandler.h"
#include "DateStrHelper.h"

#include <simplyfile/FileDescriptor.h>

#include <string_view>

#include <sys/stat.h>

#include <fmt/format.h>

namespace cndl {

namespace {

template<typename Func>
struct Finally final {
	Finally(Func && f) : _f(f) {}
	~Finally() {_f();}
private:
	Func _f;
};

template<typename Func>
Finally(Func && f) -> Finally<Func>;

}

StaticFileHandler::StaticFileHandler(std::filesystem::path bd) 
    : base_dir{std::move(bd)}
{}

OptResponse StaticFileHandler::operator()(Request const& request, std::string const& resource) const {
    auto pat = (base_dir / resource).native();
    simplyfile::FileDescriptor f{::open(pat.c_str(), O_RDONLY)};
    if (not f) {
        throw cndl::Error(404);
    }
    struct stat statbuf;
    if (fstat(f, &statbuf) != 0) {
        throw cndl::Error(500);
    }
    
    cndl::Response response;
    response.fields.emplace("cache-control", "max-age=3600, no-cache");
    response.fields.emplace("last-modified", mkdatestr(statbuf.st_mtim));
    response.fields.emplace("Accept-Ranges", "bytes");
    response.setContentTypeFromExtension(std::filesystem::path{resource}.extension().native());

    if (auto it = request.header.fields.find("if-modified-since"); it != request.header.fields.end()) {
        auto req_tm = parsedatestr(it->second);
        time_t req_time = mktime(&req_tm);
        if (req_time >= statbuf.st_mtim.tv_sec) {
            response.status_code = 304;
            return response;
        }
    }

    if (request.header.method == "HEAD") {
        response.fields.emplace("Content-Length", std::to_string(statbuf.st_size));
        return response;
    }

    std::size_t start_offset = 0;
    std::size_t end_offset   = statbuf.st_size;
    auto [it, end] = request.header.fields.equal_range("range");
    for (; it != end; ++it) {
        auto const& val = it->second;
        const std::regex reg{R"(bytes=(\d+)-(\d*))"};
        std::smatch match;
        if (std::regex_match(val, match, reg, std::regex_constants::format_first_only)) {
            start_offset = std::min<std::size_t>(statbuf.st_size, std::stoi(match[1]));
            if (match[2].length()) {
                end_offset   = std::min<std::size_t>(statbuf.st_size, std::stoi(match[2]));
            }
            if (start_offset > end_offset) {
                throw cndl::Error(400);
            }
            if (start_offset != 0 or end_offset != statbuf.st_size) {
                response.status_code = 206;
                response.fields.emplace("Content-Range", fmt::format("bytes {}-{}/{}", start_offset, end_offset - 1, statbuf.st_size));
            }
            break;
        }
    }

    response.message_body.emplace();
    response.message_body->resize(end_offset - start_offset);
    auto bytes_read = ::pread(f, response.message_body->data(), response.message_body->size(), start_offset);
    if (bytes_read != static_cast<std::int64_t>(response.message_body->size())) {
        throw cndl::Error(500);
    }
    return response;
}

bool StaticFileHandler::can_serve_resource(std::string const& resource) const {
    auto pat = (base_dir / resource).native();
    std::FILE* f = std::fopen(pat.c_str(), "r");
    if (not f) {
        return false;
    }
    std::fclose(f);
    return true;
}

}
