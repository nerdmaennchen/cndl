#include "StaticFileHandler.h"

#include <string_view>


#include <cstdio>
#include <sys/stat.h>

namespace cndl {

namespace {
using namespace std::string_view_literals;
constexpr auto date_format_string = "%a, %d %b %Y %H:%M:%S GMT"sv;

template<typename Func>
struct Finally final {
	Finally(Func && f) : _f(f) {}
	~Finally() {_f();}
private:
	Func _f;
};

template<typename Func>
Finally(Func && f) -> Finally<Func>;


std::string mkdatestr(struct tm const& tm) {
    std::array<char, 128> date_buf;
    strftime(date_buf.data(), date_buf.size(), date_format_string.data(), &tm);
    return std::string(date_buf.data());
}

std::string mkdatestr(struct timespec const& ts) {
    struct tm tm;
    gmtime_r(&ts.tv_sec, &tm);
    return mkdatestr(tm);
}

}

StaticFileHandler::StaticFileHandler(std::filesystem::path bd) 
    : base_dir{std::move(bd)}
{}

OptResponse StaticFileHandler::operator()(Request const& request, std::string const& ressource) const {
    auto pat = (base_dir / ressource).native();
    std::FILE* f = std::fopen(pat.c_str(), "r");
    if (not f) {
        throw cndl::Error(404);
    }
    auto auto_close = Finally([=]{fclose(f);});

    struct stat statbuf;
    fstat(fileno(f), &statbuf);
    
    struct timespec now;
    clock_gettime(CLOCK_REALTIME_COARSE, &now);
    
    cndl::Response response;
    response.fields["cache-control"] = "max-age=3600, no-cache";
    response.fields["last-modified"] = mkdatestr(statbuf.st_mtim);

    response.setContentTypeFromExtension(std::filesystem::path{ressource}.extension().native());

    if (auto it = request.header.fields.find("if-modified-since"); it != request.header.fields.end()) {
        struct tm req_tm;
        strptime(it->second.c_str(), date_format_string.data(), &req_tm);
        time_t req_time = mktime(&req_tm);
        if (req_time >= statbuf.st_mtim.tv_sec) {
            response.status_code = 304;
            return response;
        }
    }
    response.message_body.resize(statbuf.st_size);
    auto bytes_read = std::fread(response.message_body.data(), 1, response.message_body.size(), f);
    if (bytes_read != response.message_body.size()) {
        throw cndl::Error(500);
    }

    return response;
}

bool StaticFileHandler::can_serve_ressource(std::string const& ressource) const {
    auto pat = (base_dir / ressource).native();
    std::FILE* f = std::fopen(pat.c_str(), "r");
    if (not f) {
        return false;
    }
    std::fclose(f);
    return true;
}

}