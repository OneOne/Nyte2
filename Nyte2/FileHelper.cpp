#include "FileHelper.h"

#include <fstream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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

void FileHelper::loadImage(RawImage& _image) 
{
    stbi_uc* pixels = stbi_load(_image.path.c_str(), &_image.width, &_image.height, &_image.channels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("Failed to load image.");
    }

    _image.size = _image.width * _image.height * 4;
    _image.data = (void*)pixels;
}

void FileHelper::unloadImage(RawImage& _image)
{
    stbi_image_free((stbi_uc*)_image.data);
    _image.data = nullptr;
}

void FileHelper::loadModel(RawObj& _model)
{
    std::string warn, err;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    const char* filename = _model.path.c_str();
    
    if (!tinyobj::LoadObj(&_model.attrib, &_model.shapes, &_model.materials, &warn, &err, _model.path.c_str()))
    {
        throw std::runtime_error(warn + err);
    }
}
