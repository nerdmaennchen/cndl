#include "base64.h"

#include <array>
#include <string>

using namespace std::string_view_literals;

namespace {

constexpr std::string_view base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/"sv;

constexpr std::byte operator"" _b(unsigned long long v){
    return std::byte(v);
}

}


namespace cndl {

std::string base64_encode(std::basic_string_view<std::byte> s) {
    std::string out;
    if (s.empty()) {
        return out;
    }

    auto min_len = (s.size()*4)/3;
    out.reserve(min_len + (4 - (min_len % 4)));

    int val=0;
    int valb=-6;

    for (auto b : s) {
        val = (val<<8) | std::to_integer<int>(b);
        valb += 8;
        while (valb>=0) {
            out.push_back(base64_chars[(val>>valb)&0x3F]);
            valb-=6;
        }
    }
    if (valb>-6) {
        out.push_back(base64_chars[((val<<8)>>(valb+8))&0x3F]);
    }
    while (out.size()%4) {
        out.push_back('=');
    } 

    return out;
}

std::vector<std::byte> base64_decode(const std::string_view in) {
    // table from '+' to 'z'
    constexpr std::array<int, 80> lookup{
        62,  255, 62,  255, 63,  52,  53, 54, 55, 56, 57, 58, 59, 60, 61, 255,
        255, 0,   255, 255, 255, 255, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
        10,  11,  12,  13,  14,  15,  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
        255, 255, 255, 255, 63,  255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36,  37,  38,  39,  40,  41,  42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

    std::vector<std::byte> out;
    out.reserve((in.size()*3)/4);

    int val = 0;
    int valb = -8;
    for (auto c : in) {
        if (c < '+' || c > 'z') {
            break;
        }
        c -= '+';
        if (lookup[c] == 255) {
            break;
        }
        val = (val << 6) + lookup[c];
        valb += 6;
        if (valb >= 0) {
            out.emplace_back(std::byte((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

}
