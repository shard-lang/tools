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
#include <algorithm>
#include <iterator>

// Shard
#include "shard/utility.hpp"
#include "shard/Path.hpp"
#include "shard/String.hpp"
#include "shard/StringView.hpp"
#include "shard/tokenizer/Token.hpp"
#include "shard/tokenizer/Tokenizer.hpp"

/* ************************************************************************* */

using namespace shard;

/* ************************************************************************* */

struct ValueHelper
{
    const tokenizer::Token& token;
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

/* ************************************************************************* */

/**
 * @brief Write keyword type.
 * @param os
 * @param type
 * @return os
 */
std::ostream& operator<<(std::ostream& os, tokenizer::KeywordType type) noexcept
{
    switch (type)
    {
#define KEYWORD(name, str) case tokenizer::KeywordType::name: return os << # str ;
#include "shard/tokenizer/Token.def"
    }

    return os << "false";
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
    case tokenizer::TokenType::String:  return os << "\"" << token.getStringValue() << "\"";
    case tokenizer::TokenType::Float:   return os << token.getFloatValue();
    case tokenizer::TokenType::Char:    return os << token.getCharValue();
    case tokenizer::TokenType::Int:     return os << token.getIntValue();

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
        std::istream_iterator<char>(std::cin),
        std::istream_iterator<char>(),
        std::back_inserter(input)
    );

    return tokenizer::Tokenizer(moveValue(input));
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

    std::cout << "[\n";

    for (auto&& token : createTokenizer(argv[1]))
    {
        std::cout << "  {\n";
        std::cout << "    \"type\": " << token.getType() << ",\n";
        std::cout << "    \"keyword\": " << token.getKeywordType() << ",\n";
        std::cout << "    \"value\": " << ValueHelper{token} << ",\n";
        std::cout << "  },\n";
    }

    std::cout << "]\n";
}

/* ************************************************************************* */
