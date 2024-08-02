#pragma once

#include <stdint.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

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

public:
    void run();

private:
    void initWindow();
    void initVulkan();

#pragma region Instance Creation

    void createInstance();
    void pickPhysicalDevice();
    bool isPhysicalDeviceSuitable(VkPhysicalDevice _device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice _device);

    std::vector<const char*> getRequiredExtensions();

#if _DEBUG
    bool checkValidationLayerSupport();
    void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _createInfo);
    void setupDebugMessenger();
#endif

#pragma endregion Instance Creation

    void createLogicalDevice();
    void createWindowSurface();


    void cleanup();

    void mainLoop();

private:
    GLFWwindow* m_window = nullptr;
    static constexpr u32 m_WindowWidth = 1280;
    static constexpr u32 m_WindowHeight = 720;

    VkInstance m_instance;
    VkSurfaceKHR m_windowSurface;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_logicalDevice;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;


#if _DEBUG
    VkDebugUtilsMessengerEXT m_callback;
#endif
};
