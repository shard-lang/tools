/* ************************************************************************* */
/* This file is part of Shard.                                               */
/*                                                                           */
/* Shard is free software: you can redistribute it and/or modify             */
/* it under the terms of the GNU Affero General Public License as            */
/* published by the Free Software Foundation.                                */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU Affero General Public License for more details.                       */
/*                                                                           */
/* You should have received a copy of the GNU Affero General Public License  */
/* along with this program. If not, see <http://www.gnu.org/licenses/>.      */
/* ************************************************************************* */

// C++
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <ios>

// Shard
#include "shard/utility.hpp"
#include "shard/Path.hpp"
#include "shard/String.hpp"
#include "shard/StringView.hpp"
#include "shard/tokenizer/Token.hpp"
#include "shard/tokenizer/Tokenizer.hpp"
#include "shard/tokenizer/TokenizerException.hpp"

/* ************************************************************************* */

using namespace shard;

/* ************************************************************************* */

namespace {

/* ************************************************************************* */

struct ValueHelper
{
    const tokenizer::Token& token;
};

/* ************************************************************************* */

struct CharHelper
{
    const tokenizer::Token::CharType& ch;
};

/* ************************************************************************* */

struct StringHelper
{
    const String& str;
};

/* ************************************************************************* */

/**
 * @brief Write token type.
 * @param os
 * @param type
 * @return os
 */
std::ostream& operator<<(std::ostream& os, tokenizer::TokenType type) noexcept
{
    switch (type)
    {
#define TOKEN(name) case tokenizer::TokenType::name: return os << "\"" # name "\"";
#include "shard/tokenizer/Token.def"
    }

    return os;
}

/* ************************************************************************ */

/// Convert table between leading zeroes to number of UTF-8 bytes.
constexpr int __utf32_clz_to_len[32] = {
    7,
    6, 6, 6, 6, 6,
    5, 5, 5, 5, 5,
    4, 4, 4, 4, 4,
    3, 3, 3, 3, 3,
    2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1
};

/* ************************************************************************ */

/// Initial UTF-8 mark depending on sequence length.
constexpr uint8_t __utf8_mark[7] = {
    0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc
};

/* ************************************************************************ */

/**
 * @brief Obtain required number of UTF-8 bytes.
 * @param c UNICODE codepoint.
 * @return Required number of bytes.
 */
inline int utf8clen(uint32_t c) noexcept
{
    // Use number of leading zeroes as index to table.
    return __utf32_clz_to_len[__builtin_clz(c | 1)];
}

/* ************************************************************************* */

/**
 * @brief Write token character.
 */
std::ostream& operator<<(std::ostream& os, const CharHelper& chr) noexcept
{
    char buf[8];
    uint32_t cp = chr.ch;

    // Escape sequences
    switch (cp)
    {
    case '\0': return os << "\"\\0\"";
    case '\a': return os << "\"\\a\"";
    case '\b': return os << "\"\\b\"";
    case '\t': return os << "\"\\t\"";
    case '\n': return os << "\"\\n\"";
    case '\v': return os << "\"\\v\"";
    case '\f': return os << "\"\\f\"";
    case '\r': return os << "\"\\r\"";
    case '\e': return os << "\"\\e\"";
    case '\\': return os << "\"\\\\\"";
    case '"':  return os << "\"\\\"\"";
    }

    // Non-printable characters
    if (cp < ' ')
        return os << static_cast<int>(cp);

    // Number of required bytes
    const int count = utf8clen(cp);

    switch (count)
    {
      case 7: return os;
      case 6: buf[5] = (cp | 0x80) & 0xbf; cp >>= 6;
      case 5: buf[4] = (cp | 0x80) & 0xbf; cp >>= 6;
      case 4: buf[3] = (cp | 0x80) & 0xbf; cp >>= 6;
      case 3: buf[2] = (cp | 0x80) & 0xbf; cp >>= 6;
      case 2: buf[1] = (cp | 0x80) & 0xbf; cp >>= 6;
      case 1: buf[0] = (cp | __utf8_mark[count]);
    }

    return (os << '\"').write(buf, count) << '\"';
}

/* ************************************************************************* */

/**
 * @brief Write token string.
 */
std::ostream& operator<<(std::ostream& os, const StringHelper& str) noexcept
{
    os << '\"';

    for (auto c : str.str)
    {
        // Escape sequences
        switch (c)
        {
        case '\0': os << "\\0"; break;
        case '\a': os << "\\a"; break;
        case '\b': os << "\\b"; break;
        case '\t': os << "\\t"; break;
        case '\n': os << "\\n"; break;
        case '\v': os << "\\v"; break;
        case '\f': os << "\\f"; break;
        case '\r': os << "\\r"; break;
        case '\e': os << "\\e"; break;
        case '\\': os << "\\\\"; break;
        case '"':  os << "\\\""; break;
        default:   os << c; break;
        }
    }

    return os << '\"';
}

/* ************************************************************************* */

/**
 * @brief Write token value.
 * @param os
 * @param value
 * @return os
 */
std::ostream& operator<<(std::ostream& os, const ValueHelper& value) noexcept
{
    const auto& token = value.token;

    switch (token.getType())
    {
    case tokenizer::TokenType::Identifier:
    case tokenizer::TokenType::String:  return os << StringHelper{token.getStringValue()};
    case tokenizer::TokenType::Float:   return os << token.getFloatValue();
    case tokenizer::TokenType::Int:     return os << token.getIntValue();
    case tokenizer::TokenType::Char:    return os << CharHelper{token.getCharValue()};

    case tokenizer::TokenType::Keyword:
    {
        switch (token.getKeywordType())
        {
#define KEYWORD(name, str) case tokenizer::KeywordType::name: return os << # str ;
#include "shard/tokenizer/Token.def"
        }
        break;
    }

#define PUNCTUATOR(name, str) case tokenizer::TokenType::name: return os << # str;
#include "shard/tokenizer/Token.def"
    default: return os << "false";
    }

    return os;
}

/* ************************************************************************* */

/**
 * @brief Create tokenizer from file name.
 * @param filename File name.
 * @return Tokenizer.
 */
tokenizer::Tokenizer createTokenizer(StringView filename)
{
    if (filename != "-")
        return tokenizer::Tokenizer(Path(String(filename)));

    String input;
    input.reserve(4096);

    // Copy standard input to string
    std::copy(
        std::istream_iterator<char>(std::cin >> std::noskipws),
        std::istream_iterator<char>(),
        std::back_inserter(input)
    );

    return tokenizer::Tokenizer(moveValue(input));
}

/* ************************************************************************* */

}

/* ************************************************************************* */

/**
 * @brief Entry function.
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "no input file" << std::endl;
        return -1;
    }

    try
    {
        std::stringstream oss;

        oss << "[\n";

        for (auto&& token : createTokenizer(argv[1]))
        {
            oss << "  {\n";
            oss << "    \"type\": " << token.getType() << ",\n";
            oss << "    \"value\": " << ValueHelper{token} << "\n";
            oss << "  },\n";
        }

        oss << "]\n";

        std::copy(
            std::istream_iterator<char>(oss >> std::noskipws),
            std::istream_iterator<char>(),
            std::ostream_iterator<char>(std::cout)
        );
    }
    catch (const tokenizer::TokenizerException& e)
    {
        std::cerr << "\033[31mERROR\033[0m: " << e.what() << "" << std::endl;
        return -1;
    }
}

/* ************************************************************************* */
