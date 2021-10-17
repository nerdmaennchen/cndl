#include "Request.h"
#include "Error.h"
#include "overloaded.h"

#include <charconv>

namespace cndl {

std::string url_unescape(std::string_view str) {
    std::string unescaped;
    std::size_t start{0};
    while (true) {
        auto pos = str.find('%', start);
        unescaped += str.substr(start, pos-start);
        if (pos == std::string_view::npos) {
            break;
        }
        if (pos > str.size()-2) {
            throw Error(400, "Bad URL escaping");
        }
        int val{};
        auto res =std::from_chars(&str[pos+1], &str[pos+3], val, 16);
        if (res.ec == std::errc::invalid_argument) {
            throw Error(400, "Bad URL escaping");
        }
        unescaped += char(val);
        start = pos+3;
    }
    return unescaped;
}

std::string to_lower(std::string str) {
    std::transform(begin(str), end(str), begin(str), ::tolower);
    return str;
}

std::string to_lower(std::string_view str) {
    return to_lower(std::string{str});
}

std::vector<FieldVal> extractFieldVals(std::string_view str, std::string_view delimiters) {
    if (str.empty()) {
        return {};
    }

    using namespace std::string_view_literals;

    auto remove_quots = [](std::string_view v) {
        if (v.size() and v[0]=='\"' and v[v.size()-1]=='\"') {
            v = v.substr(1, v.size()-2);
        }
        return v;
    };

    std::vector<FieldVal> fields;

    auto start {0U};
    auto spaces = " \t";
    while (true) {
        start = str.find_first_not_of(spaces, start);
        auto end = str.find_first_of(delimiters, start);
        auto chunk = str.substr(start, end-start);
        if (chunk.empty()) {
            break;
        }
        chunk = chunk.substr(0, chunk.find_last_not_of(spaces)+1);
        if (chunk.empty()) {
            break;
        }
        auto eq_sign = chunk.find('=');
        if (eq_sign != std::string_view::npos) {
            fields.emplace_back(std::make_pair(url_unescape(remove_quots(chunk.substr(0, eq_sign))), url_unescape(remove_quots(chunk.substr(eq_sign+1)))));
        } else {
            fields.emplace_back(url_unescape(std::string{remove_quots(chunk)}));
        }

        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    return fields;
}

Request::Header::FieldMap parse_fields(std::string_view fields) {
    Request::Header::FieldMap fields_map;
    std::string_view illegal_chars{" \t\r\n"};
    auto pos = 0U;
    while (pos < fields.size()) {
        auto const eol = fields.find("\r\n", pos);
        if (eol == std::string_view::npos) {
            break;
        }
        std::string_view line = fields.substr(pos, eol-pos);
        auto colon_idx = line.find(':');
        if (colon_idx == std::string_view::npos || colon_idx == 0) {
            throw Error(400, "invalid header field");
        }

        std::string field_name = to_lower(line.substr(0, colon_idx));
        if (field_name.find_first_of(illegal_chars) != std::string_view::npos) {
            throw Error(400, "invalid Request-Line");
        }

        auto field_val_beg = line.find_first_not_of(illegal_chars, colon_idx+1);
        if (field_val_beg == std::string_view::npos) {
            throw Error(400, "invalid Request-Line");
        }
        auto field_val_end = line.find_last_not_of(illegal_chars);
        if (field_val_end == std::string_view::npos || field_val_beg>field_val_end) {
            throw Error(400, "invalid Request-Line");
        }
        std::string field_value{line.substr(field_val_beg, field_val_end-field_val_beg+1)};

        fields_map.emplace(url_unescape(field_name), url_unescape(std::move(field_value)));
        pos = eol+2;
    }
    return fields_map;
}

Request::Header parse_header(std::string_view request) {
    using namespace std::string_view_literals;

    Request::Header header;

    auto first_line_end = request.find("\r\n"sv);
    if (first_line_end == std::string_view::npos) {
        throw Error(400, "invalid Request-Line");
    }

    auto method_end = request.find(' ');
    if (method_end == std::string_view::npos) {
        throw Error(400, "invalid Request-Line");
    }

    auto URL_end = request.find(' ', method_end+1);
    if (URL_end == std::string_view::npos) {
        throw Error(400, "invalid Request-Line");
    }

    auto raw_url = std::string_view{request.data()+method_end+1, URL_end-method_end-1};

    header.method = request.substr(0, method_end);
    header.url = url_unescape(raw_url);
    header.resource = header.url.substr(0, header.url.find('?'));
    header.version = request.substr(URL_end+1, first_line_end-URL_end-1);

    auto trailer_idx = raw_url.find('?');
    if (trailer_idx != std::string_view::npos and trailer_idx + 1 < raw_url.size()) {
        std::string_view args_part = raw_url.substr(trailer_idx+1); // drop the '?'
        args_part = args_part.substr(0, args_part.find('#'));

        auto fields = extractFieldVals(args_part, "&"sv);
        for (auto const& f: fields) {
            std::visit(detail::overloaded{
                [&](KV_Pair const& pair) {
                    header.url_args.emplace(
                        url_unescape(pair.first),
                        url_unescape(pair.second));
                },
                [&](std::string const& str) {
                    header.url_args.emplace(
                        url_unescape(str),
                        "");
                }
            }, f);
        }
    }

    header.fields = parse_fields(request.substr(first_line_end+2));

    auto cl_it = header.fields.find("content-length");
    if (cl_it != header.fields.end()) {
        std::string_view sv{cl_it->second};
        auto res =std::from_chars(sv.begin(), sv.end(), header.content_length);
        if (res.ec == std::errc::invalid_argument) {
            throw Error(400, "invalid content-length");
        }
    }

    auto cookies = header.fields.equal_range("cookie");
    while (cookies.first != cookies.second) {
        auto extracted = extractFieldVals(cookies.first->second);
        for (auto const& c : extracted) {
            if (auto kvp = std::get_if<KV_Pair>(&c); kvp) {
                header.cookies.insert_or_assign(kvp->first, kvp->second);
            }
        }
        ++cookies.first;
    }

    return header;
}


Request::Header::BodyArgMap readMultipartBody(std::string_view body, std::string_view boundary) {
    using namespace std::string_view_literals;
    Request::Header::BodyArgMap body_args;

    std::size_t start{body.find(boundary)};
    while (start != std::string_view::npos) {
        start += boundary.size();
        if (body.substr(start, 2) == "--"sv) {
            break;
        }
        if (body.substr(start, 2) != "\r\n"sv) {
            throw Error(400, "missing CRLF after boundary");
        }
        start += 2;
        auto body_end = body.find(boundary, start);
        if (body_end == std::string_view::npos || body_end < start+4) {
            break;
        }
        auto term = body.substr(body_end-4, 4);
        if (term != "\r\n--"sv) {
            throw Error(400, "invalid termination of multipart body");
        }
        auto content = body.substr(start, body_end-start-4);

        auto header_end = content.find("\r\n\r\n");
        if (header_end == std::string_view::npos) {
            throw Error(400, "missing header information in multipart body");
        }
        auto fields = parse_fields(content.substr(0, header_end+2));

        auto cd_it = fields.find("content-disposition");
        if (cd_it == fields.end()) {
            throw Error(400, "missing content-disposition header in multipart body");
        }

        auto cd_split = extractFieldVals(cd_it->second);
        auto name_it = std::find_if(begin(cd_split), end(cd_split), [](FieldVal const& fv){
            return std::holds_alternative<KV_Pair>(fv) and std::get<KV_Pair>(fv).first == "name";
        });
        if (name_it == cd_split.end()) {
            throw Error(400, "missing name field in content-disposition");
        }

        Request::Header::BodyArg ba;
        for (auto const& s : cd_split) {
            std::visit(detail::overloaded{
                [&](std::string const& s) {ba.fields.emplace(s, "");},
                [&](KV_Pair const& kv) {ba.fields.emplace(kv.first, kv.second);}
            }, s);
        }
        for (auto const& [k, v] : fields) {
            ba.fields.emplace(k, v);
        }

        auto body_content = content.substr(header_end+4);
        std::transform(begin(body_content), end(body_content), std::back_inserter(ba.content), [](char c) { return std::byte(c); });
        body_args.emplace(std::get<KV_Pair>(*name_it).second, std::move(ba));
        start = body.find(boundary, start);
    }
    return body_args;
}

}
