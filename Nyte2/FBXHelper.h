#pragma once

// stl
#include <vector>
#include <string>
#include <array>

// openFBX
#include "ofbx.h"

// vulkan
#include "vulkan/vulkan_core.h"

#include "Common.h"


struct FBXVertex
{
    using Vec3 = ofbx::Vec3;
    using Vec2 = ofbx::Vec2;

    ofbx::Vec3 position;
    ofbx::Vec3 normal;
    //ofbx::Vec3 tangent;
    ofbx::Vec2 uv;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0; // binding index 
        bindingDescription.stride = sizeof(FBXVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    using FBXVertexInputAttributDescriptions = std::array<VkVertexInputAttributeDescription, 3>;
    static FBXVertexInputAttributDescriptions getAttributeDescriptions()
    {
        FBXVertexInputAttributDescriptions attributeDescriptions{};

        // inPosition
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // = vec3
        attributeDescriptions[0].offset = offsetof(FBXVertex, position);

        // inNormal
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // = vec3
        attributeDescriptions[1].offset = offsetof(FBXVertex, normal);

        //// inTangent
        //attributeDescriptions[2].binding = 0;
        //attributeDescriptions[2].location = 2;
        //attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT; // = vec3
        //attributeDescriptions[2].offset = offsetof(FBXVertex, tangent);

        // inUV
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 2;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT; // = vec2
        attributeDescriptions[3].offset = offsetof(FBXVertex, uv);

        return attributeDescriptions;
    }

    bool operator== (const FBXVertex& _other)
    {
        return
            position.x == _other.position.x
            && position.y == _other.position.y
            && position.z == _other.position.z

            && normal.x == _other.normal.x
            && normal.y == _other.normal.y
            && normal.z == _other.normal.z

            //&& tangent.x == _other.tangent.x
            //&& tangent.y == _other.tangent.y
            //&& tangent.z == _other.tangent.z

            && uv.x == _other.uv.x
            && uv.y == _other.uv.y;
    }
};

struct FBXMesh
{
    std::vector<FBXVertex> m_vertices;
    std::vector<u32> m_indices;

    int materialIndex = -1;
};
struct FBXMaterial
{
    std::string diffuse;
    std::string normal;
    std::string specular;
    std::string glossiness;
};
struct FBXScene
{
    std::string filePath;
    std::vector<FBXMesh> meshes;
    //std::vector<FBXMaterial> materials;
};

class FBXHelper
{
public:
    static void loadFBX(FBXScene& _fbx);

    static std::string getCachePath(FBXScene& _fbx)
    {
        return _fbx.filePath + ".cache";
    }
};

