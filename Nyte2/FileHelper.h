#pragma once

#include <string>
#include <vector>

using octet = char;

class FileHelper
{
public:
    static std::vector<octet> readFile(const std::string& filePath);
};