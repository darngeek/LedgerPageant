/*
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

#pragma once

#include <string>
#include <vector>

static bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

static std::string get_base64_chars() {
    static std::string val = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    return val;
}

static std::vector<uint8_t> to_bytes(std::string input) {
    std::vector<uint8_t> out(input.size());
    std::copy(input.begin(), input.end(), out.begin());
    input.clear();

    return out;
}

static std::string base64_encode(const std::string& raw_string) {
    std::vector<uint8_t> tmp = to_bytes(raw_string);
    const uint8_t* bytes_to_encode = tmp.data();
    //const uint8_t* bytes_to_encode = (uint8_t*)raw_string.data();
    size_t in_len = raw_string.size();

    std::string ret;
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xFCu) >> 2u;
            char_array_4[1] = ((char_array_3[0] & 0x03u) << 4u) + ((char_array_3[1] & 0xF0u) >> 4u);
            char_array_4[2] = ((char_array_3[1] & 0x0fu) << 2u) + ((char_array_3[2] & 0xc0u) >> 6u);
            char_array_4[3] = char_array_3[2] & 0x3Fu;

            for (i = 0; (i < 4); i++)
                ret += get_base64_chars()[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfcu) >> 2u;
        char_array_4[1] = ((char_array_3[0] & 0x03u) << 4u) + ((char_array_3[1] & 0xf0u) >> 4u);
        char_array_4[2] = ((char_array_3[1] & 0x0fu) << 2u) + ((char_array_3[2] & 0xc0u) >> 6u);
        char_array_4[3] = char_array_3[2] & 0x3fu;

        for (j = 0; (j < i + 1); j++)
            ret += get_base64_chars()[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

static std::string base64_decode(const std::string& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = (unsigned char)get_base64_chars().find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2u) + ((char_array_4[1] & 0x30u) >> 4u);
            char_array_3[1] = ((char_array_4[1] & 0xfu) << 4u) + ((char_array_4[2] & 0x3cu) >> 2u);
            char_array_3[2] = ((char_array_4[2] & 0x3u) << 6u) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = (unsigned char)get_base64_chars().find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2u) + ((char_array_4[1] & 0x30u) >> 4u);
        char_array_3[1] = ((char_array_4[1] & 0xfu) << 4u) + ((char_array_4[2] & 0x3cu) >> 2u);
        char_array_3[2] = ((char_array_4[2] & 0x3u) << 6u) + char_array_4[3];

        for (j = 0; (j < i - 1); j++)
            ret += char_array_3[j];
    }

    return ret;
}
