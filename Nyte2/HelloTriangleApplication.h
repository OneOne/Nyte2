#pragma once

// stl
#include <stdint.h>
#include <vector>
#include <optional>
#include <array>

// glfw
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// glm
#include <glm/glm.hpp>

// class forward decl
struct GLFWwindow;

// using
using u32 = uint32_t;

class HelloTriangleApplication 
{
private:
    struct QueueFamilyIndices {
        std::optional<u32> graphicsFamily;
        std::optional<u32> presentFamily;
        std::optional<u32> computeFamily;
        std::optional<u32> transfertFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
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

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0; // binding index 
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

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

            return attributeDescriptions;
        }
    };
    const std::vector<Vertex> Vertices = 
    {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
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
    void createGraphicsPipeline();

    void createFramebuffers();
    void createCommandPool();
    u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
    void createVertexBuffer();
    void createCommandBuffers();

    void createSemaphoresAndFences();

    void cleanup();

    void mainLoop();
    void drawFrame();

private:
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

    GLFWwindow* m_window = nullptr;
    u32 m_windowWidth = 1024;
    u32 m_windowHeight = 1024;

    VkInstance m_instance;
    VkSurfaceKHR m_windowSurface;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    
    VkDevice m_logicalDevice;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;

    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    VkCommandPool m_commandPool;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferDeviceMemory;
    std::vector<VkCommandBuffer> m_commandBuffers;

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
