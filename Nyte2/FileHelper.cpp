#include "FileHelper.h"

#include <fstream>
#include <stdexcept>

std::vector<octet> FileHelper::readFile(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::ate | std::ios::binary); // open as binary, placing cursor at the end

    if (!file.is_open()) 
    {
        throw std::runtime_error(std::string{ "Failed to open file: " } + filePath);
    }

    size_t fileSize = (size_t)file.tellg(); // retrieve cursor position (here the end and therefore file size)
    std::vector<octet> buffer(fileSize);

    file.seekg(0); // move cursor to start
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}