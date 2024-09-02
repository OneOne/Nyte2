#pragma once

// stl
#include <stdint.h>
#include <vector>
#include <array>
// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// vulkan
#include "vulkan/vulkan_core.h"

#include "Math.h"

struct VertexBasic {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoords;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0; // binding index 
        bindingDescription.stride = sizeof(VertexBasic);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    using VertexInputAttributDescriptions = std::array<VkVertexInputAttributeDescription, 3>;
    static VertexInputAttributDescriptions getAttributeDescriptions()
    {
        VertexInputAttributDescriptions attributeDescriptions{};

        // inPosition
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // = vec3
        attributeDescriptions[0].offset = offsetof(VertexBasic, pos);

        // inColor
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // = vec3
        attributeDescriptions[1].offset = offsetof(VertexBasic, color);

        // inTexCoords
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // = vec2
        attributeDescriptions[2].offset = offsetof(VertexBasic, texCoords);

        return attributeDescriptions;
    }
};
const std::vector<VertexBasic> BasicVertices =
{
    { { -0.5f, -0.5f,  0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f} },
    { {  0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f} },
    { { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f} },
    { {  0.5f,  0.5f,  0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f} },

    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f} },
    { {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f} },
    { { -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f} },
    { {  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f} },
};
const std::vector<u16> BasicIndices = { 
    0, 1, 2, 1, 3, 2,
    4, 5, 6, 5, 7, 6
};


