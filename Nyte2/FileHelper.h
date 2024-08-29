#pragma once

#include <string>
#include <vector>

using octet = char;

struct RawImage
{
    const char* path;
    int width;
    int height;
    int channels;
    unsigned int size;
    void* data;
};

class FileHelper
{
public:
    static std::vector<octet> readFile(const std::string& filePath);
    static void loadImage(RawImage& _image);
    static void unloadImage(RawImage& _image);
};