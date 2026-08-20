#include "carrot/hash.hpp"
#include "picosha2.h"

std::string Hash::sha256(const std::string& data)
{
    return picosha2::hash256_hex_string(data);
}