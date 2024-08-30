#pragma once

// stl
#include <stdint.h>
#include <vector>
#include <optional>
#include <array>
#include <chrono>

// glfw
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// glm
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


// class forward decl
struct GLFWwindow;

// using
using u32 = uint32_t;
using u16 = uint16_t;

class HelloTriangleApplication 
{
private:
    struct QueueFamilyIndices {
        std::optional<u32> graphicsFamily;
        std::optional<u32> presentFamily;
        std::optional<u32> computeFamily;
        std::optional<u32> transferFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
        }
    };

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;
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
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // = vec2
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            // inColor
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // = vec3
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            // inTexCoords
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // = vec2
            attributeDescriptions[2].offset = offsetof(Vertex, texCoords);

            return attributeDescriptions;
        }
    };
    const std::vector<Vertex> Vertices = 
    {
        { { -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f} },
        { {  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f} },
        { { -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f} },
        { {  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f} }
    };
    const std::vector<u16> Indices = { 0, 1, 2, 1, 3, 2 };

    struct alignas(16) UBO_ModelViewProj // Uniform buffer object
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

public:
    void run();

private:
    void initWindow();
    static void framebufferSizeChanged(GLFWwindow* window, int width, int height);

    void initVulkan();

#pragma region Instance & PhysicalDevice

    void createInstance();
    void createWindowSurface();
    void pickPhysicalDevice();
    bool isPhysicalDeviceSuitable(VkPhysicalDevice _device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice _device);
    SwapchainSupportDetails retrieveSwapchainSupportDetails(VkPhysicalDevice _device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice _device);

    std::vector<const char*> getRequiredExtensions();

#if _DEBUG
    bool checkValidationLayerSupport();
    void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _createInfo);
    void setupDebugMessenger();
#endif

#pragma endregion Instance & PhysicalDevice

    void createLogicalDevice();

#pragma region Swapchain
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities);
    void createSwapchain();
    void createImageViews();
    void destroySwapchain();
    void recreateSwapChain();
#pragma endregion Swapchain

    void createRenderPass();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createDescriptorSetLayout();
    void createGraphicsPipeline();

#pragma region Common
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool _commandPool);
    void endSingleTimeCommands(VkCommandPool _commandPool, VkQueue _queue, VkCommandBuffer _commandBuffer);
    void createBuffer(VkDeviceSize _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _properties, VkBuffer& _buffer, VkDeviceMemory& _bufferDeviceMemory);
    void createConcurrentBuffer(VkDeviceSize _size, VkBufferUsageFlags _usage, u32 _sharedQueueCount, u32* _sharedQueueIndices, VkMemoryPropertyFlags _properties, VkBuffer& _buffer, VkDeviceMemory& _bufferDeviceMemory);
    void copyBuffer(VkBuffer _srcBuffer, VkBuffer _dstBuffer, VkDeviceSize _size);
    void createImage(u32 _width, u32 _height, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags _properties, VkImage& _image, VkDeviceMemory& _imageMemory);
    void createConcurrentImage(u32 _width, u32 _height, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, u32 _sharedQueueCount, u32* _sharedQueueIndices, VkMemoryPropertyFlags _properties, VkImage& _image, VkDeviceMemory& _imageMemory);
    void transitionImageLayout(VkImage _image, VkFormat _format, VkImageLayout _oldLayout, VkImageLayout _newLayout);
    void copyBufferToImage(VkBuffer _buffer, VkImage _image, u32 _width, u32 _height);
    void createImageView(VkImage _image, VkFormat _format, VkImageView& _imageView);
#pragma endregion Common

    void createFramebuffers();
    void createCommandPools();
    
    u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();

    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    
    void createSemaphoresAndFences();


    void cleanup();

    void mainLoop();
    void drawFrame();
    void updateUniformBuffer(u32 _currentImage);

private:
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

    GLFWwindow* m_window = nullptr;
    u32 m_windowWidth = 1280;
    u32 m_windowHeight = 720;

    VkInstance m_instance;
    VkSurfaceKHR m_windowSurface;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    
    VkDevice m_logicalDevice;
    QueueFamilyIndices m_queueFamilyIndices;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkQueue m_transferQueue;

    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;

    VkRenderPass m_renderPass;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    VkCommandPool m_graphicsCommandPool;
    VkCommandPool m_transferCommandPool;
    std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
    std::vector<VkCommandBuffer> m_transferCommandBuffers;

    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferDeviceMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferDeviceMemory;
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersDeviceMemory;
    VkImage m_textureImage;
    VkDeviceMemory m_textureImageDeviceMemory;
    VkImageView m_textureImageView;
    VkSampler m_textureSampler;

    // Note: Fences synchronize c++ calls with gpu operations
    //       Semaphores synchronize gpu operations with one another
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_imagesInFlightFences;
    u32 m_currentFrame = 0;

    bool m_framebufferResized = false;

#if _DEBUG
    VkDebugUtilsMessengerEXT m_callback;
#endif
};
