#pragma once

// stl
#include <stdint.h>
#include <vector>
#include <array>

// vulkan
#include "vulkan/vulkan_core.h"

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "Math.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoords;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0; // binding index 
        bindingDescription.stride = sizeof(Vertex);
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
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        // inNormal
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // = vec3
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        // inTexCoords
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // = vec2
        attributeDescriptions[2].offset = offsetof(Vertex, texCoords);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& _other) const 
    {
        return pos == _other.pos && normal == _other.normal && texCoords == _other.texCoords;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ 
                    (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ 
                    (hash<glm::vec2>()(vertex.texCoords) << 1);
        }
    };
}
