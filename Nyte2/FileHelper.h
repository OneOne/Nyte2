#pragma once

#include <string>
#include <vector>

#include "tiny_obj_loader.h"

using octet = char;

struct RawImage
{
    std::string path;
    int width;
    int height;
    int channels;
    unsigned int size;
    void* data;
};

struct RawObj
{
    std::string path;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
};

class FileHelper
{
public:
    static std::vector<octet> readFile(const std::string& filePath);
    static void loadImage(RawImage& _image);
    static void unloadImage(RawImage& _image);
    
    static void loadModel(RawObj& _model);
};