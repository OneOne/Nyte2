#include "Engine.h"

// std
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <set>
#include <limits>
#include <algorithm>
#include <unordered_map>

using namespace std;

// VCR for Vulkan Check Result
#define VCR(val, msg) if(val != VK_SUCCESS) { throw std::runtime_error(msg); }

namespace Nyte
{
const vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
};

#if _DEBUG
// Check "Config/vk_layer_settings.txt" in VulkanSDK to get more information on how to configure validation layer.
const vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_KHRONOS_synchronization2"
};

// Create and Destroy messenger for validation layer callback
VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    // messageSeverity:
    //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
    //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
    //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
    //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
    //
    // messageType:
    //      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 
    //      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
    //      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
    //      
    // pCallbackData:
    //      ->pMessage
    //      ->pObjects
    //      ->objectCount
    //
    // pUserData: (void*) provided when callback is attatched
    //
    // return:
    //      VK_TRUE break function execution and return VK_ERROR_VALIDATION_FAILED_EXT
    //      VK_FALSE otherwise
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {

    }
    return VK_FALSE;
}


void Engine::init()
{
#if _DEBUG
    setupDebugMessenger();
#endif

    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPools();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSemaphoresAndFences();
}

void Engine::deinit()
{
    destroySwapchain();

    vkDestroySampler(m_logicalDevice, m_textureSampler, nullptr);
    vkDestroyImageView(m_logicalDevice, m_textureImageView, nullptr);
    vkDestroyImage(m_logicalDevice, m_textureImage, nullptr);
    vkFreeMemory(m_logicalDevice, m_textureImageDeviceMemory, nullptr);

    vkDestroyDescriptorSetLayout(m_logicalDevice, m_descriptorSetLayout, nullptr);

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
    }

    vkDestroyBuffer(m_logicalDevice, m_indexBuffer, nullptr);
    vkFreeMemory(m_logicalDevice, m_indexBufferDeviceMemory, nullptr);
    vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
    vkFreeMemory(m_logicalDevice, m_vertexBufferDeviceMemory, nullptr);
    vkDestroyCommandPool(m_logicalDevice, m_transferCommandPool, nullptr);
    vkDestroyCommandPool(m_logicalDevice, m_graphicsCommandPool, nullptr);

    vkDestroyDevice(m_logicalDevice, nullptr);

#if _DEBUG
    destroyDebugUtilsMessengerEXT(m_instance, m_callback, nullptr);
#endif

    vkDestroySurfaceKHR(m_instance, *m_surface, nullptr);
}

void Engine::drawFrame()
{
    // Acquire next available image in swapchain
    u32 imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }
    else if (acquireResult == VK_SUBOPTIMAL_KHR)
    {
        // Nothing to do. Swapchain can still be used to present image even though some settings of the window
        // do not correspond to the swapchain.
    }
    else
        VCR(acquireResult, "Failed to acquire next image in swapchain.");

    // Ensure no operation are currently done on this image
    if (m_imagesInFlightFences[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(m_logicalDevice, 1, &m_imagesInFlightFences[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Set fence with current image fence
    m_imagesInFlightFences[imageIndex] = m_inFlightFences[m_currentFrame];

    // Submit corresponding command buffer to queue
    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    updateUniformBuffer(imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores; // wait image is available
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_graphicsCommandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores; // signal image has been rendered once command buffer is executed

    // Reset in-flight fence for current frame just before submitting to graphics queue
    vkResetFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame]);

    VCR(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]), "Failed to submit command buffer to queue.");

    // Present: send swapchain image result to display
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // wait that image has been rendered before present

    VkSwapchainKHR swapchains[] = { m_swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    VkResult presentResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_framebufferResized)
    {
        m_framebufferResized = false;
        recreateSwapchain();
    }
    else
        VCR(presentResult, "Failed to present.");

    //m_currentFrame = (m_currentFrame + 1) & 1;
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    vkQueueWaitIdle(m_presentQueue);
}

void Engine::idle()
{
    vkDeviceWaitIdle(m_logicalDevice);
}

void Engine::resizeWindow(int _width, int _height)
{
    m_framebufferResized = true;
    m_windowWidth = _width;
    m_windowHeight = _height;
}


#pragma region Instance & PhysicalDevice

void Engine::createInstance(std::vector<const char*> _requiredExtensions)
{
#if _DEBUG
    if (!checkValidationLayerSupport())
    {
        throw std::runtime_error("Active validation layers are not available.");
    }
#endif

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr; // no extension structure
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Retrieve needed extensions for GLFW
    u32 requiredExtensionCount = (u32)_requiredExtensions.size();
    createInfo.enabledExtensionCount = requiredExtensionCount;
    createInfo.ppEnabledExtensionNames = _requiredExtensions.data();

    // enable the validation layers (debug only)
#if !_DEBUG
    createInfo.enabledLayerCount = 0; // validation layer count
#else
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
    fillDebugMessengerCreateInfo(messengerCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&messengerCreateInfo;
#endif

    // Listing available extension
    {
        u32 extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "Available extensions:\n";
        for (const VkExtensionProperties& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }

        // Checking GLFW extensions are available
        bool allExtensionsAvailable = true;
        for (u32 i = 0; i < requiredExtensionCount; ++i)
        {
            const char* extensionStr = _requiredExtensions[i];
            auto it = std::find_if(extensions.begin(), extensions.end(),
                [&extensionStr](const VkExtensionProperties& extProperties) { return strcmp(extProperties.extensionName, extensionStr) == 0; });
            allExtensionsAvailable &= (it != extensions.end());
        }
        if (!allExtensionsAvailable)
            std::cout << "One or more required extensions are not supported.\n";
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    VCR(result, "Failed to create instance.");
}
void Engine::destroyInstance()
{
    vkDestroyInstance(m_instance, nullptr);
}

void Engine::pickPhysicalDevice()
{
    // Retrieve physical devices supporting Vulkan count
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("No GPU (physical device) supporting Vulkan found.");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const VkPhysicalDevice& device : devices)
    {
        if (isPhysicalDeviceSuitable(device)) {
            m_physicalDevice = device;
            m_msaaSamples = getMaxUsableSampleCount(device);
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("No GPU (physical device) found suitable.");

}


Nyte::SwapchainSupportDetails Engine::retrieveSwapchainSupportDetails(VkPhysicalDevice _device, VkSurfaceKHR _window)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, _window, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device, _window, &formatCount, nullptr);

    if (formatCount > 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_device, _window, &formatCount, details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device, _window, &presentModeCount, nullptr);

    if (presentModeCount > 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(_device, _window, &presentModeCount, details.presentModes.data());
    }

    return details;
}
Nyte::QueueFamilyIndices Engine::findQueueFamilies(VkPhysicalDevice _device, VkSurfaceKHR _window)
{
    QueueFamilyIndices queueIndices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());

    for (int i = 0; i < queueFamilies.size(); ++i)
    {
        // graphics
        const VkQueueFamilyProperties& queueFamily = queueFamilies[i];
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queueIndices.graphicsFamily = i;
        }

        // present
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, _window, &presentSupport);
        if (presentSupport)
            queueIndices.presentFamily = i; // probably the same as graphics

        // transfer
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            queueIndices.transferFamily = i;
        }

        if (queueIndices.isComplete())
            break;
    }

    return queueIndices;
}
VkSampleCountFlagBits Engine::getMaxUsableSampleCount(VkPhysicalDevice _physicalDevice)
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(_physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

bool Engine::isPhysicalDeviceSuitable(VkPhysicalDevice _device)
{
    //VkPhysicalDeviceProperties deviceProperties;
    //vkGetPhysicalDeviceProperties(_device, &deviceProperties);
    //
    //VkPhysicalDeviceFeatures deviceFeatures;
    //vkGetPhysicalDeviceFeatures(_device, &deviceFeatures);
    //
    //bool isSuitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    //isSuitable &= deviceFeatures.geometryShader; // support geometry shader
    //
    //return isSuitable;

    bool isSuitable = true;

    QueueFamilyIndices indices = findQueueFamilies(_device, *m_surface);
    isSuitable &= indices.isComplete();

    // Features
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(_device, &supportedFeatures);
    isSuitable &= (supportedFeatures.samplerAnisotropy == VK_TRUE);

    // Extensions
    bool physicalDeviceExtensionsSupported = checkDeviceExtensionSupport(_device);
    isSuitable &= physicalDeviceExtensionsSupported;

    bool swapchainAdequate = false;
    if (physicalDeviceExtensionsSupported) {
        SwapchainSupportDetails swapchainSupport = retrieveSwapchainSupportDetails(_device, *m_surface);
        swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }
    isSuitable &= swapchainAdequate;

    return isSuitable;
}
bool Engine::checkDeviceExtensionSupport(VkPhysicalDevice _device)
{
    u32 extensionsCount;
    vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionsCount, nullptr);

    vector<VkExtensionProperties> extensions(extensionsCount);
    vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionsCount, extensions.data());

    for (const char* deviceExtension : deviceExtensions)
    {
        auto it = find_if(extensions.begin(), extensions.end(), [&](const VkExtensionProperties& _extProperties) -> bool {
            return strcmp(_extProperties.extensionName, deviceExtension) == 0;
        });
        if (it == extensions.end())
            return false;
    }
    return true;
}

#if _DEBUG
bool Engine::checkValidationLayerSupport()
{
    // Retrieve available layer
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Check validation layers are available
    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}
void Engine::fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _createInfo)
{
    _createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    _createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    _createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    _createInfo.pfnUserCallback = debugCallback;
}
void Engine::setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    fillDebugMessengerCreateInfo(createInfo);

    VCR(createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_callback), "Enable to create the messenger.");
}
#endif

#pragma endregion Instance & PhysicalDevice

void Engine::createLogicalDevice()
{
    m_queueFamilyIndices = findQueueFamilies(m_physicalDevice, *m_surface);

    // note: queue family must be unique, cannot create twice a queue for a same index.
    set<u32> uniqueQueueFamilies = {
        m_queueFamilyIndices.graphicsFamily.value(),
        m_queueFamilyIndices.presentFamily.value(),
        m_queueFamilyIndices.transferFamily.value()
    };

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f; // priority relative to other queues (same for each one here)
    for (u32 index : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = index;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    createInfo.pEnabledFeatures = &deviceFeatures;

    VkPhysicalDeviceSynchronization2Features synchronization2Features{};
    synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2Features.synchronization2 = VK_TRUE;
    createInfo.pNext = &synchronization2Features;

    createInfo.enabledExtensionCount = (u32)deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();


#if _DEBUG
    createInfo.enabledLayerCount = (u32)validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    VCR(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice), "Failed to create the logical device.");

    vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
    vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.transferFamily.value(), 0, &m_transferQueue);
}


#pragma region Swapchain
VkSurfaceFormatKHR Engine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats)
{
    for (const VkSurfaceFormatKHR& availableFormat : _availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;

        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    // By default return first format
    return _availableFormats.front();
}
VkPresentModeKHR Engine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes)
{
    // VK_PRESENT_MODE_IMMEDIATE_KHR: 
    //      Images submitted by your application are transferred to the screen right away, which may result in tearing.
    // 
    // VK_PRESENT_MODE_FIFO_KHR : 
    //      The swap chain is a queue where the display takes an image from the front of the queue when the display is 
    //      refreshed and the program inserts rendered images at the back of the queue.If the queue is full then the program 
    //      has to wait.This is most similar to vertical sync as found in modern games.The moment that the display is 
    //      refreshed is known as "vertical blank".
    // 
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR : 
    //      This mode only differs from the previous one if the application is late and the queue was empty at the last 
    //      vertical blank.Instead of waiting for the next vertical blank, the image is transferred right away when it 
    //      finally arrives.This may result in visible tearing.
    // 
    // VK_PRESENT_MODE_MAILBOX_KHR : 
    //      This is another variation of the second mode.Instead of blocking the application when the queue is full, the 
    //      images that are already queued are simply replaced with the newer ones.This mode can be used to render frames as 
    //      fast as possible while still avoiding tearing, resulting in fewer latency issues than standard vertical sync.
    //      This is commonly known as "triple buffering", although the existence of three buffers alone does not necessarily
    //      mean that the framerate is unlocked.

    for (const VkPresentModeKHR& availablePresentMode : _availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }

    // By default return FIFO mode as it is always available
    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D Engine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities)
{
    if (_capabilities.currentExtent.width != numeric_limits<u32>::max())
        return _capabilities.currentExtent;

    VkExtent2D actualExtent = { m_windowWidth, m_windowHeight };
    actualExtent.width = clamp(actualExtent.width, _capabilities.minImageExtent.width, _capabilities.maxImageExtent.width);
    actualExtent.height = clamp(actualExtent.height, _capabilities.minImageExtent.height, _capabilities.maxImageExtent.height);

    m_windowWidth = actualExtent.width;
    m_windowHeight = actualExtent.height;

    return actualExtent;
}
void Engine::createSwapchain()
{
    SwapchainSupportDetails swapchainSupport = retrieveSwapchainSupportDetails(m_physicalDevice, *m_surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    m_swapchainImageFormat = surfaceFormat.format;
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
    m_swapchainExtent = chooseSwapExtent(swapchainSupport.capabilities);

    u32 imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0) // a limit exist (nolimit = (u32)-1)
    {
        if (imageCount > swapchainSupport.capabilities.maxImageCount) // the limit is exceeded with minImageCount+1, then clamp it to maxImageCount
            imageCount = swapchainSupport.capabilities.maxImageCount;
    }


    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = *m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = m_swapchainImageFormat;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // images in swapchain are use directly as color, use TRANSFER_DST to use other images to be transfered into swap chain

    u32 queueFamilyIndices[] = { m_queueFamilyIndices.graphicsFamily.value(), m_queueFamilyIndices.presentFamily.value() };

    if (m_queueFamilyIndices.graphicsFamily != m_queueFamilyIndices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // images can be shared by sevral queues
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // images are exclusive to the present/graphics queue which offers maximum performance
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform; // allows to apply transform on images provided to the swapchain (current transform = no transform)
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // allows to composite window with other windows depending on the alpha value
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; // allows to clip pixels hidden by other windows
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VCR(vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapchain), "Failed to create the swapchain.");

    u32 swapchainImagesCount;
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &swapchainImagesCount, nullptr);
    m_swapchainImages.resize(swapchainImagesCount);
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &swapchainImagesCount, m_swapchainImages.data());

}
void Engine::createImageViews()
{
    m_swapchainImageViews.resize(m_swapchainImages.size());

    for (u32 i = 0; i < m_swapchainImages.size(); i++)
    {
        createImageView(m_swapchainImages[i], m_swapchainImageFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT, m_swapchainImageViews[i]);
    }
}
void Engine::destroySwapchain()
{
    //vkDestroyImageView(m_logicalDevice, m_normalImageView, nullptr);
    //vkDestroyImage(m_logicalDevice, m_normalImage, nullptr);
    //vkFreeMemory(m_logicalDevice, m_normalImageDeviceMemory, nullptr);

    vkDestroyImageView(m_logicalDevice, m_colorImageView, nullptr);
    vkDestroyImage(m_logicalDevice, m_colorImage, nullptr);
    vkFreeMemory(m_logicalDevice, m_colorImageDeviceMemory, nullptr);

    vkDestroyImageView(m_logicalDevice, m_depthImageView, nullptr);
    vkDestroyImage(m_logicalDevice, m_depthImage, nullptr);
    vkFreeMemory(m_logicalDevice, m_depthImageDeviceMemory, nullptr);

    if (m_transferCommandBuffers.size() > 0)
        vkFreeCommandBuffers(m_logicalDevice, m_transferCommandPool, (u32)m_transferCommandBuffers.size(), m_transferCommandBuffers.data());
    if (m_graphicsCommandBuffers.size() > 0)
        vkFreeCommandBuffers(m_logicalDevice, m_graphicsCommandPool, (u32)m_graphicsCommandBuffers.size(), m_graphicsCommandBuffers.data());

    for (u32 i = 0; i < m_swapchainImages.size(); i++)
    {
        vkDestroyBuffer(m_logicalDevice, m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_logicalDevice, m_uniformBuffersDeviceMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(m_logicalDevice, m_descriptorPool, nullptr); // also free descriptor sets allocation

    for (VkFramebuffer& framebuffer : m_swapchainFramebuffers)
    {
        vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);
    }

    vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

    for (VkImageView& imageView : m_swapchainImageViews)
    {
        vkDestroyImageView(m_logicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
}
void Engine::recreateSwapchain()
{
    // Handle framebuffer size (0,0) by waiting futher events
    // note: this can happen when the window is minimized
    //int width = 0, height = 0;
    //glfwGetFramebufferSize(m_window, &width, &height);
    //while (width == 0 || height == 0)
    //{
    //    glfwGetFramebufferSize(m_window, &width, &height);
    //    glfwWaitEvents();
    //}

    vkDeviceWaitIdle(m_logicalDevice);

    destroySwapchain();

    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}
#pragma endregion Swapchain

void Engine::createRenderPass()
{
    // Color
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = m_msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear color framebuffer before pass
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // no stencil == no care
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // index 0 (=> layout(location = 0) in shader)
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = m_msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;  // index 1
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = m_swapchainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;  // index 2
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //// Normal
    //VkAttachmentDescription normalAttachment{};
    //normalAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    //normalAttachment.samples = m_msaaSamples;
    //normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear color framebuffer before pass
    //normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // no stencil == no care
    //normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //normalAttachment.finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    //
    //VkAttachmentReference normalAttachmentRef{};
    //normalAttachmentRef.attachment = 3; // index 3 (=> layout(location = 3) in shader)
    //normalAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //
    //VkAttachmentDescription normalAttachmentResolve{};
    //normalAttachmentResolve.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    //normalAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    //normalAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //normalAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //normalAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //normalAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //normalAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //normalAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    //
    //VkAttachmentReference normalAttachmentResolveRef{};
    //normalAttachmentResolveRef.attachment = 4;  // index 4
    //normalAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;


    // Subpass
    //array<VkAttachmentReference, 2> attachmentColorReferences = { colorAttachmentRef, normalAttachmentRef };
    //array<VkAttachmentReference, 2> attachmentResolveReferences = { colorAttachmentResolveRef, normalAttachmentResolveRef };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;//(u32)attachmentColorReferences.size();
    subpass.pColorAttachments = &colorAttachmentRef; //attachmentColorReferences.data();
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef; //attachmentResolveReferences.data();

    //array<VkAttachmentDescription, 5> attachmentDescriptions = 
    array<VkAttachmentDescription, 3> attachmentDescriptions =
    {
        colorAttachment,
        depthAttachment,
        colorAttachmentResolve,
        //normalAttachment,
        //normalAttachmentResolve,
    };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = (u32)attachmentDescriptions.size();
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    // Subpass dependencies handle the transition between subpasses including before and after the subpass
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0; // subpass index
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VCR(vkCreateRenderPass(m_logicalDevice, &renderPassInfo, nullptr, &m_renderPass), "Failed to create render pass.");
}
VkShaderModule Engine::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VCR(vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &shaderModule), "Failed to create shader module.");

    return shaderModule;
}
void Engine::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = (u32)bindings.size();
    layoutInfo.pBindings = bindings.data();

    VCR(vkCreateDescriptorSetLayout(m_logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout), "Faild to create descriptor set layout.");
}
void Engine::createGraphicsPipeline()
{
    // In this function we load shaders and setup everything for the pipeline creation.
    // It's done in the following order:
    //  - Load and create shader modules
    //  - Setup shader stage
    //  - Setup vertex format
    //  - Setup input assembly
    //  - Setup viewport
    //  - Setup scissor
    //  - Setup rasterizer
    //  - Setup multisampling
    //  - Setup depth and stencil
    //  - Setup color blend 
    //  - Setup dynamic states

    // Load and create shader modules
    //vector<octet> vertexShaderCode = FileHelper::readFile("Resources/Shaders/vertexshader.spv");
    //vector<octet> fragmentShaderCode = FileHelper::readFile("Resources/Shaders/fragmentshader.spv");
    vector<octet> vertexShaderCode = FileHelper::readFile("Resources/Shaders/model_vs.spv");
    vector<octet> fragmentShaderCode = FileHelper::readFile("Resources/Shaders/model_fs.spv");

    VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    // Setup shader stage
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertexShaderModule;
    vertShaderStageInfo.pName = "main";
    vertShaderStageInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragmentShaderModule;
    fragShaderStageInfo.pName = "main";
    fragShaderStageInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Setup vertex format
    // note: at the moment defined as empty as vertices are hard-defined in shader
    VkVertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
    Vertex::VertexInputAttributDescriptions vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = (u32)vertexAttributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

    // Setup input assembly (way triangles are described)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Setup viewport (region of framebuffer where rendering operates)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swapchainExtent.width;
    viewport.height = (float)m_swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Setup scissor (region of the framebuffer that will be kept)
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Setup rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // fill, edges or points
    rasterizer.lineWidth = 1.0f; // default value (otherwise require "wideLines" extension)
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // careful, we use ccw here due to glm->vulkan conversion in projection matrix
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // Setup multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = m_msaaSamples;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // Setup depth and stencil (enable depth test/write or stencil test/write)
    VkPipelineDepthStencilStateCreateInfo depthAndStencil{};
    depthAndStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthAndStencil.depthTestEnable = VK_TRUE;
    depthAndStencil.depthWriteEnable = VK_TRUE;
    depthAndStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthAndStencil.depthBoundsTestEnable = VK_FALSE;
    depthAndStencil.minDepthBounds = 0.0f; // Optional
    depthAndStencil.maxDepthBounds = 1.0f; // Optional
    depthAndStencil.stencilTestEnable = VK_FALSE;
    depthAndStencil.front = {}; // Optional
    depthAndStencil.back = {}; // Optional

    // Setup color blend 
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    // Setup dynamic states (for viewport size, line width or blend constants update without rebuilding full pipeline)
    vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (u32)(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();


    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    VCR(vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout), "Failed to create pipeline layout.");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthAndStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0; // subpass index
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional (pipeline inheritance)
    pipelineInfo.basePipelineIndex = -1; // Optional

    VCR(vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline), "Failed to create graphics pipeline(s).");

    // Clean shader modules
    vkDestroyShaderModule(m_logicalDevice, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(m_logicalDevice, vertexShaderModule, nullptr);
}

#pragma region Common
VkCommandBuffer Engine::beginSingleTimeCommands(VkCommandPool _commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}
void Engine::endSingleTimeCommands(
    VkCommandPool _commandPool,
    VkQueue _queue,
    VkCommandBuffer _commandBuffer,
    u32 _signalSemaphoreCount,
    VkSemaphore* _signalSemaphore,
    u32 _waitSemaphoreCount,
    VkSemaphore* _waitSemaphore,
    VkPipelineStageFlags* _waitStageMask)
{
    vkEndCommandBuffer(_commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffer;

    if (_signalSemaphoreCount > 0 && _signalSemaphore != nullptr)
    {
        submitInfo.signalSemaphoreCount = _signalSemaphoreCount;
        submitInfo.pSignalSemaphores = _signalSemaphore;
    }
    if (_waitSemaphoreCount > 0 && _waitSemaphore != nullptr)
    {
        submitInfo.waitSemaphoreCount = _waitSemaphoreCount;
        submitInfo.pWaitSemaphores = _waitSemaphore;
        submitInfo.pWaitDstStageMask = _waitStageMask;
    }

    vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_queue);

    vkFreeCommandBuffers(m_logicalDevice, _commandPool, 1, &_commandBuffer);
}
void Engine::createBuffer(
    VkDeviceSize _size,
    VkBufferUsageFlags _usage,
    VkMemoryPropertyFlags _properties,
    VkBuffer& _buffer,
    VkDeviceMemory& _bufferDeviceMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = _size;
    bufferInfo.usage = _usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VCR(vkCreateBuffer(m_logicalDevice, &bufferInfo, nullptr, &_buffer), "Failed to create buffer.");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_logicalDevice, _buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, _properties);

    VCR(vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &_bufferDeviceMemory), "Failed to allocate buffer device memory.");

    vkBindBufferMemory(m_logicalDevice, _buffer, _bufferDeviceMemory, 0);
}
void Engine::createConcurrentBuffer(
    VkDeviceSize _size,
    VkBufferUsageFlags _usage,
    u32 _sharedQueueCount,
    u32* _sharedQueueIndices,
    VkMemoryPropertyFlags _properties,
    VkBuffer& _buffer,
    VkDeviceMemory& _bufferDeviceMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = _size;
    bufferInfo.usage = _usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    bufferInfo.pQueueFamilyIndices = _sharedQueueIndices;
    bufferInfo.queueFamilyIndexCount = _sharedQueueCount;

    VCR(vkCreateBuffer(m_logicalDevice, &bufferInfo, nullptr, &_buffer), "Failed to create buffer.");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_logicalDevice, _buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, _properties);

    VCR(vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &_bufferDeviceMemory), "Failed to allocate buffer device memory.");

    vkBindBufferMemory(m_logicalDevice, _buffer, _bufferDeviceMemory, 0);
}
void Engine::copyBuffer(VkBuffer _srcBuffer, VkBuffer _dstBuffer, VkDeviceSize _size)
{
    VkCommandBuffer copyCommandBuffer = beginSingleTimeCommands(m_transferCommandPool);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = _size;
    vkCmdCopyBuffer(copyCommandBuffer, _srcBuffer, _dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(m_transferCommandPool, m_transferQueue, copyCommandBuffer);
}
void Engine::createImage(
    u32 _width,
    u32 _height,
    u32 _mipLevels,
    VkSampleCountFlagBits _sampleCount,
    VkFormat _format,
    VkImageTiling _tiling,
    VkImageUsageFlags _usage,
    u32 _sharedQueueCount,
    u32* _sharedQueueIndices,
    VkMemoryPropertyFlags _properties,
    VkImage& _image,
    VkDeviceMemory& _imageDeviceMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = _width;
    imageInfo.extent.height = _height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = _mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = _format;
    imageInfo.tiling = _tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // not usable by the GPU and the very first transition will discard the texels
    imageInfo.usage = _usage;
    imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    imageInfo.queueFamilyIndexCount = _sharedQueueCount;
    imageInfo.pQueueFamilyIndices = _sharedQueueIndices;
    imageInfo.samples = _sampleCount;
    imageInfo.flags = 0; // Optional

    VCR(vkCreateImage(m_logicalDevice, &imageInfo, nullptr, &_image), "Failed to create image.");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_logicalDevice, _image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, _properties);

    VCR(vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &_imageDeviceMemory), "Failed to allocate image device memory.");

    vkBindImageMemory(m_logicalDevice, _image, _imageDeviceMemory, 0);
}
void Engine::transitionImageLayoutToTransfer(VkImage _image, u32 _mipLevels)
{
    VkCommandBuffer copyCommandBuffer = beginSingleTimeCommands(m_transferCommandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = _mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        copyCommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(m_transferCommandPool, m_transferQueue, copyCommandBuffer);
}
void Engine::transitionImageLayoutToGraphics(VkImage _image, u32 _mipLevels, VkImageAspectFlags _aspectMask, VkImageLayout _newLayout, VkAccessFlags _dstAccessMask, VkPipelineStageFlags _dstStageMask)
{
    VkCommandBuffer graphicsCommandBuffer = beginSingleTimeCommands(m_graphicsCommandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = _newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _image;
    barrier.subresourceRange.aspectMask = _aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = _mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstAccessMask = _dstAccessMask;

    vkCmdPipelineBarrier(
        graphicsCommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, _dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(m_graphicsCommandPool, m_graphicsQueue, graphicsCommandBuffer);
}
void Engine::transitionImageLayoutFromTransferToGraphics(VkImage _image, u32 _mipLevels)
{
    // To handle image transition between transfer and graphics queues
    // we need to transition the image layout on transfer queue then graphic queue
    // with a semaphore to synchronize both queues
    // Also, we end with a Transfer_Dst layout to be ready to blit (mip levels generation)
    // Note: we consider old layout to be "undefined" but it should already be "transfer_dst" 
    // due to staging copy done previously
    // About transfer/graphics queues transition see: 
    //  - https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#three-dispatches-first-dispatch-writes-to-one-storage-buffer-second-dispatch-writes-to-a-different-storage-buffer-third-dispatch-reads-both
    //  - https://stackoverflow.com/questions/67993790/how-do-you-properly-transition-the-image-layout-from-transfer-optimal-to-shader

    VkCommandBuffer copyCommandBuffer = beginSingleTimeCommands(m_transferCommandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = m_textureImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_texture.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;


    // Create to semaphore to sync transfer and graphic queue
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkSemaphore transferSemaphore;
    VCR(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &transferSemaphore), "Failed to create transfer semaphore.");


    barrier.srcQueueFamilyIndex = m_queueFamilyIndices.transferFamily.value(); // (not sure how this is used by Vulkan, as we need to define a semaphore to sync both queue)
    barrier.dstQueueFamilyIndex = m_queueFamilyIndices.graphicsFamily.value();
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    // Barrier from undefined layout to transfer dst (on transfer queue)
    vkCmdPipelineBarrier(
        copyCommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // Submit on transfer queue with the semaphore (that will be signaled once the transition is done)
    endSingleTimeCommands(m_transferCommandPool, m_transferQueue, copyCommandBuffer, 1, &transferSemaphore);

    // Now, on graphics queue, we need to submit transition from "Bottom of pipe" to "Transfer Dst"
    // with the previous transfer semaphore as waiting
    VkCommandBuffer graphicsCommandBuffer = beginSingleTimeCommands(m_graphicsCommandPool);


    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    // Barrier from undefined layout to transfer dst (on graphics queue)
    vkCmdPipelineBarrier(
        graphicsCommandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // Submit on graphics queue waiting the semaphore to be signaled
    VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
    endSingleTimeCommands(m_graphicsCommandPool, m_graphicsQueue, graphicsCommandBuffer, 0, nullptr, 1, &transferSemaphore, &waitStageMask[0]);

    // Clean semaphore
    vkDestroySemaphore(m_logicalDevice, transferSemaphore, nullptr);
}
void Engine::copyBufferToImage(VkBuffer _buffer, VkImage _image, u32 _width, u32 _height)
{
    VkCommandBuffer copyCommandBuffer = beginSingleTimeCommands(m_transferCommandPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        _width,
        _height,
        1
    };

    vkCmdCopyBufferToImage(
        copyCommandBuffer,
        _buffer,
        _image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endSingleTimeCommands(m_transferCommandPool, m_transferQueue, copyCommandBuffer);
}
void Engine::createImageView(VkImage _image, VkFormat _format, u32 _mipLevels, VkImageAspectFlags _aspectMask, VkImageView& _imageView)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = _format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    viewInfo.subresourceRange.aspectMask = _aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = _mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VCR(vkCreateImageView(m_logicalDevice, &viewInfo, nullptr, &_imageView), "Failed to create image view.");
}
void Engine::generateMipmaps(VkImage _image, VkFormat _format, u32 _texWidth, u32 _texHeight, u32 _mipLevels)
{
    // Check image format support filtering
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, _format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        throw std::runtime_error("Image format does not support linear filtering.");
    }


    VkCommandBuffer graphicsCommandBuffer = beginSingleTimeCommands(m_graphicsCommandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = _image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    i32 mipWidth = _texWidth;
    i32 mipHeight = _texHeight;

    for (uint32_t i = 1; i < _mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        // Barrier from transfer dst layout to transfer src (on graphics queue)
        vkCmdPipelineBarrier(
            graphicsCommandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        // Blit mip level
        vkCmdBlitImage(
            graphicsCommandBuffer,
            _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // Barrier from transfer src layout to shader read only (on graphics queue)
        vkCmdPipelineBarrier(
            graphicsCommandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }


    barrier.subresourceRange.baseMipLevel = _mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    // Barrier from transfer dst layout to shader read only (on graphics queue)
    vkCmdPipelineBarrier(graphicsCommandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(m_graphicsCommandPool, m_graphicsQueue, graphicsCommandBuffer);
}
#pragma endregion Common

void Engine::createCommandPools()
{
    // Graphics
    VkCommandPoolCreateInfo graphicsPoolInfo{};
    graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    graphicsPoolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily.value();
    graphicsPoolInfo.flags = 0; // Optional
    VCR(vkCreateCommandPool(m_logicalDevice, &graphicsPoolInfo, nullptr, &m_graphicsCommandPool), "Failed to create command pool.");

    // Transfer
    VkCommandPoolCreateInfo transferPoolInfo{};
    transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transferPoolInfo.queueFamilyIndex = m_queueFamilyIndices.transferFamily.value();
    transferPoolInfo.flags = 0; // Optional
    VCR(vkCreateCommandPool(m_logicalDevice, &transferPoolInfo, nullptr, &m_transferCommandPool), "Failed to create command pool.");

}

void Engine::createColorResources()
{
    VkFormat colorFormat = m_swapchainImageFormat;
    u32 queueIndices[] = { m_queueFamilyIndices.transferFamily.value(), m_queueFamilyIndices.graphicsFamily.value() };

    createImage(
        m_swapchainExtent.width,
        m_swapchainExtent.height,
        1,
        m_msaaSamples,
        colorFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        2,
        &queueIndices[0],
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_colorImage,
        m_colorImageDeviceMemory);

    createImageView(m_colorImage, colorFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT, m_colorImageView);


    //// normal
    //VkFormat normalFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    //
    //createImage(
    //    m_swapchainExtent.width,
    //    m_swapchainExtent.height,
    //    1,
    //    m_msaaSamples,
    //    normalFormat,
    //    VK_IMAGE_TILING_OPTIMAL,
    //    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    //    2,
    //    &queueIndices[0],
    //    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //    m_normalImage,
    //    m_normalImageDeviceMemory);
    //
    //createImageView(m_normalImage, normalFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT, m_normalImageView);
}

VkFormat Engine::findSupportedFormat(const std::vector<VkFormat>& _candidates, VkImageTiling _tiling, VkFormatFeatureFlags _features)
{
    for (VkFormat format : _candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (_tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & _features) == _features)
        {
            return format;
        }
        else if (_tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & _features) == _features)
        {
            return format;
        }
    }

    throw std::runtime_error("No supported format found.");
}
bool Engine::hasStencilComponent(VkFormat _format)
{
    return _format == VK_FORMAT_D32_SFLOAT_S8_UINT || _format == VK_FORMAT_D24_UNORM_S8_UINT;
}
VkFormat Engine::findDepthFormat()
{
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}
void Engine::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();
    u32 queueIndices[] = { m_queueFamilyIndices.transferFamily.value(), m_queueFamilyIndices.graphicsFamily.value() };

    createImage(
        m_swapchainExtent.width,
        m_swapchainExtent.height,
        1,
        m_msaaSamples,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        2,
        &queueIndices[0],
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_depthImage,
        m_depthImageDeviceMemory);

    createImageView(m_depthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT, m_depthImageView);

    // Optional transition (as it will be done in render pass directly)
    transitionImageLayoutToGraphics(
        m_depthImage,
        1, // mipLevels
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
}

void Engine::createFramebuffers()
{
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

    for (u32 i = 0; i < (u32)m_swapchainImageViews.size(); i++)
    {
        //array<VkImageView, 4> attachments = 
        array<VkImageView, 3> attachments =
        {
            m_colorImageView,
            //m_normalImageView,
            m_depthImageView, // same  depth image view for each framebuffer as the process is sequential
            m_swapchainImageViews[i]
        };


        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = (u32)attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;

        VCR(vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]), "Failed to create framebuffer.");
    }
}

void Engine::loadModel()
{
    RawObj model;
    model.path = MODEL_PATH;

    FileHelper::loadModel(model);

    unordered_map<Vertex, u32> verticesMap{}; // <Vertex, vertexIndex>

    for (const tinyobj::shape_t& shape : model.shapes)
    {
        for (const tinyobj::index_t& index : shape.mesh.indices)
        {
            Vertex vertex{};
            vertex.pos.x = model.attrib.vertices[index.vertex_index * 3 + 0];
            vertex.pos.y = model.attrib.vertices[index.vertex_index * 3 + 1];
            vertex.pos.z = model.attrib.vertices[index.vertex_index * 3 + 2];

            vertex.normal.x = model.attrib.normals[index.normal_index * 3 + 0];
            vertex.normal.y = model.attrib.normals[index.normal_index * 3 + 1];
            vertex.normal.z = model.attrib.normals[index.normal_index * 3 + 2];

            vertex.texCoords.x = model.attrib.texcoords[index.texcoord_index * 2 + 0];
            vertex.texCoords.y = 1.0f - model.attrib.texcoords[index.texcoord_index * 2 + 1]; // obj -> vulkan tex coords convertion

            auto it = verticesMap.find(vertex);
            if (it == verticesMap.end())
            {
                u32 vertexIndex = (u32)m_vertices.size();
                verticesMap[vertex] = vertexIndex;
                m_vertices.push_back(vertex);
                m_indices.push_back(vertexIndex);
            }
            else
                m_indices.push_back(it->second);
        }
    }
}

u32 Engine::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    // memoryHeaps: physical memory (VRAM, RAM, RAM swap zone)
    // memoryTypes: logical memory
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            // memory type fit type and property filters
            return i;
        }
    }

    throw std::runtime_error("No memory type fit the given buffer.");
}
void Engine::createVertexBuffer()
{
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * m_vertices.size();

    // Create a staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferDeviceMemory;
    createBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: ensure coherence between buffer memory and RAM
        stagingBuffer,
        stagingBufferDeviceMemory);

    // Map vertices to the staging buffer
    void* data;
    vkMapMemory(m_logicalDevice, stagingBufferDeviceMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, m_vertices.data(), (size_t)vertexBufferSize);
    vkUnmapMemory(m_logicalDevice, stagingBufferDeviceMemory);

    // Create the vertex buffer (GPU only)
    u32 queueIndices[] = { m_queueFamilyIndices.transferFamily.value(), m_queueFamilyIndices.graphicsFamily.value() };
    createConcurrentBuffer( // used by graphic queue and transfer queue (therefore shared by several queues)
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        2,
        &queueIndices[0],
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // GPU only buffer
        m_vertexBuffer,
        m_vertexBufferDeviceMemory);

    // Copy staging to vertex buffer (one shot)
    copyBuffer(stagingBuffer, m_vertexBuffer, vertexBufferSize);

    vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicalDevice, stagingBufferDeviceMemory, nullptr);
}
void Engine::createIndexBuffer()
{
    VkDeviceSize indexBufferSize = sizeof(m_indices[0]) * m_indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void* data;
    vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, m_indices.data(), (size_t)indexBufferSize);
    vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

    u32 queueIndices[] = { m_queueFamilyIndices.transferFamily.value(), m_queueFamilyIndices.graphicsFamily.value() };
    createConcurrentBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        2,
        &queueIndices[0],
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_indexBuffer,
        m_indexBufferDeviceMemory);

    copyBuffer(stagingBuffer, m_indexBuffer, indexBufferSize);

    vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}
void Engine::createUniformBuffers()
{
    VkDeviceSize mvpBufferSize = sizeof(UBO_ModelViewProj);

    m_uniformBuffers.resize(m_swapchainImages.size());
    m_uniformBuffersDeviceMemory.resize(m_swapchainImages.size());

    for (u32 i = 0; i < m_swapchainImages.size(); i++)
    {
        createBuffer(
            mvpBufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBuffers[i],
            m_uniformBuffersDeviceMemory[i]);
    }
}
void Engine::createTextureImage()
{
    m_texture.path = TEXTURE_PATH;
    FileHelper::loadImage(m_texture);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(
        m_texture.size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void* data;
    vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, m_texture.size, 0, &data);
    memcpy(data, m_texture.data, (u32)m_texture.size);
    vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

    FileHelper::unloadImage(m_texture);

    u32 queueIndices[] = { m_queueFamilyIndices.transferFamily.value(), m_queueFamilyIndices.graphicsFamily.value() };

    createImage(
        (u32)m_texture.width,
        (u32)m_texture.height,
        (u32)m_texture.mipLevels,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL, // optimal pixel order for access (otherwise, it would be linear = pixels ordered row per row)
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // transfer src/dst for mips blit
        2,
        &queueIndices[0],
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_textureImage,
        m_textureImageDeviceMemory);

    transitionImageLayoutToTransfer(m_textureImage, m_texture.mipLevels);

    copyBufferToImage(stagingBuffer, m_textureImage, (u32)m_texture.width, (u32)m_texture.height);

    transitionImageLayoutFromTransferToGraphics(m_textureImage, m_texture.mipLevels);

    generateMipmaps(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, (u32)m_texture.width, (u32)m_texture.height, (u32)m_texture.mipLevels);

    vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}
void Engine::createTextureImageView()
{
    createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, (u32)m_texture.mipLevels, VK_IMAGE_ASPECT_COLOR_BIT, m_textureImageView);
}

void Engine::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = (float)m_texture.mipLevels;
    samplerInfo.mipLodBias = 0.0f;

    VCR(vkCreateSampler(m_logicalDevice, &samplerInfo, nullptr, &m_textureSampler), "Failed to create sampler.")
}

void Engine::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(m_swapchainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(m_swapchainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = (u32)poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = (u32)m_swapchainImages.size();

    VCR(vkCreateDescriptorPool(m_logicalDevice, &poolInfo, nullptr, &m_descriptorPool), "Failed to create descriptor pool.");
}
void Engine::createDescriptorSets()
{
    vector<VkDescriptorSetLayout> layouts(m_swapchainImages.size(), m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = (u32)m_swapchainImages.size();
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(m_swapchainImages.size());

    VCR(vkAllocateDescriptorSets(m_logicalDevice, &allocInfo, m_descriptorSets.data()), "Failed to create descriptor sets.");

    for (u32 i = 0; i < m_swapchainImages.size(); i++)
    {

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UBO_ModelViewProj);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;

        array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pImageInfo = nullptr; // Optional
        descriptorWrites[0].pTexelBufferView = nullptr; // Optional

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;
        descriptorWrites[1].pBufferInfo = nullptr; // Optional
        descriptorWrites[1].pTexelBufferView = nullptr; // Optional

        vkUpdateDescriptorSets(m_logicalDevice, (u32)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}
void Engine::createCommandBuffers()
{
    m_graphicsCommandBuffers.resize(m_swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_graphicsCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (u32)m_graphicsCommandBuffers.size();

    VCR(vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, m_graphicsCommandBuffers.data()), "Failed to allocate command buffers.");

    for (u32 i = 0; i < (u32)m_graphicsCommandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        VCR(vkBeginCommandBuffer(m_graphicsCommandBuffers[i], &beginInfo), "Failed to begin command buffer.");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapchainFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapchainExtent;

        array<VkClearValue, 2> clearValues;
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} }; // color
        clearValues[1].depthStencil.depth = 1.0f; // depth
        clearValues[1].depthStencil.stencil = 0;

        renderPassInfo.clearValueCount = (u32)clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_graphicsCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // VK_SUBPASS_CONTENTS_INLINE = render pass commands are directly included into the command buffer
        vkCmdBindPipeline(m_graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        // bind vertex buffer
        VkBuffer vertexBuffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_graphicsCommandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(m_graphicsCommandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(m_graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);

        vkCmdDrawIndexed(m_graphicsCommandBuffers[i], (u32)m_indices.size(), 1, 0, 0, 0); // indexCount, instanceCount, firstIndex, vertexOffset, firstInstance

        vkCmdEndRenderPass(m_graphicsCommandBuffers[i]);

        VCR(vkEndCommandBuffer(m_graphicsCommandBuffers[i]), "Failed to end command buffer.");
    }
}

void Engine::createSemaphoresAndFences()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlightFences.resize(m_swapchainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VCR(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]), "Failed to create semaphore.");
        VCR(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]), "Failed to create semaphore.");
        VCR(vkCreateFence(m_logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i]), "Failed to create fence.");
    }
}


void Engine::updateUniformBuffer(u32 _currentImage)
{
    static chrono::time_point startTime = chrono::high_resolution_clock::now();

    chrono::time_point currentTime = chrono::high_resolution_clock::now();
    float time = chrono::duration<float, chrono::seconds::period>(currentTime - startTime).count();

    UBO_ModelViewProj ubo_MVP{};
    ubo_MVP.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo_MVP.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo_MVP.proj = glm::perspective(glm::radians(45.0f), m_swapchainExtent.width / (float)m_swapchainExtent.height, 0.1f, 10.0f);

    ubo_MVP.proj[1][1] *= -1; // convert OpenGL coords to Vulkan coords

    void* data;
    vkMapMemory(m_logicalDevice, m_uniformBuffersDeviceMemory[_currentImage], 0, sizeof(ubo_MVP), 0, &data);
    memcpy(data, &ubo_MVP, sizeof(ubo_MVP));
    vkUnmapMemory(m_logicalDevice, m_uniformBuffersDeviceMemory[_currentImage]);
}

} // namespace Nyte