#include "HelloTriangleApplication.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <set>
#include <limits>
#include <algorithm>

using namespace std;

// VCR for Vulkan Check Result
#define VCR(val, msg) if(val != VK_SUCCESS) { throw std::runtime_error(msg); }

#if _DEBUG
// Check "Config/vk_layer_settings.txt" in VulkanSDK to get more information on how to configure validation layer.
const vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Create and Destroy messenger for validation layer callback
VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugUtilsMessengerEXT * pCallback)
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
    const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
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

void HelloTriangleApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initWindow()
{
    glfwInit(); // init glfw lib
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // avoid openGL context creation
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // disable window resize (for now)

    m_window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, "Vulkan", nullptr, nullptr);
}

void HelloTriangleApplication::initVulkan() 
{
    createInstance();
#if _DEBUG
    setupDebugMessenger();
#endif
    createWindowSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
}



#pragma region Instance & PhysicalDevice

void HelloTriangleApplication::createInstance()
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
    std::vector<const char *> requiredExtensions = getRequiredExtensions();
    u32 glfwExtensionCount = static_cast<u32>(requiredExtensions.size());
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

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
        bool allGLFWExtensionsAvailable = true;
        for (u32 i = 0; i < glfwExtensionCount; ++i)
        {
            const char* glfwExtStr = requiredExtensions[i];
            auto it = std::find_if(extensions.begin(), extensions.end(),
                [&glfwExtStr](const VkExtensionProperties& extProperties) { return strcmp(extProperties.extensionName, glfwExtStr) == 0; });
            allGLFWExtensionsAvailable &= (it != extensions.end());
        }
        if(!allGLFWExtensionsAvailable)
            std::cout << "One or more GLFW extensions are not supported.\n";
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    VCR(result, "Failed to create instance.");
}

void HelloTriangleApplication::createWindowSurface()
{
    //VkWin32SurfaceCreateInfoKHR createInfo{};
    //createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    //createInfo.hwnd = glfwGetWin32Window(window);
    //createInfo.hinstance = GetModuleHandle(nullptr);

    //VCR(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface), "Failed to create the window surface.");

    VCR(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_windowSurface), "Failed to create the window surface.");
}

void HelloTriangleApplication::pickPhysicalDevice()
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
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("No GPU (physical device) found suitable.");

}

bool HelloTriangleApplication::isPhysicalDeviceSuitable(VkPhysicalDevice _device)
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

    QueueFamilyIndices indices = findQueueFamilies(_device);
    isSuitable &= indices.isComplete();

    bool physicalDeviceExtensionsSupported = checkDeviceExtensionSupport(_device);
    isSuitable &= physicalDeviceExtensionsSupported;

    bool swapchainAdequate = false;
    if (physicalDeviceExtensionsSupported) {
        SwapchainSupportDetails swapchainSupport = retrieveSwapchainSupportDetails(_device);
        swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }
    isSuitable &= swapchainAdequate;

    return isSuitable;
}
bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice _device)
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
HelloTriangleApplication::SwapchainSupportDetails HelloTriangleApplication::retrieveSwapchainSupportDetails(VkPhysicalDevice _device) 
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, m_windowSurface, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device, m_windowSurface, &formatCount, nullptr);

    if (formatCount > 0) 
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_device, m_windowSurface, &formatCount, details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device, m_windowSurface, &presentModeCount, nullptr);

    if (presentModeCount > 0) 
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(_device, m_windowSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

HelloTriangleApplication::QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice _device) 
{
    QueueFamilyIndices queueIndices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());

    for (int i=0; i<queueFamilies.size(); ++i) 
    {
        const VkQueueFamilyProperties& queueFamily = queueFamilies[i];
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
        {
            queueIndices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, m_windowSurface, &presentSupport);
        if (presentSupport)
            queueIndices.presentFamily = i; // probably the same as graphics

        if(queueIndices.isComplete())
            break;
    }

    return queueIndices;
}

vector<const char*> HelloTriangleApplication::getRequiredExtensions() 
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if _DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

#if _DEBUG
bool HelloTriangleApplication::checkValidationLayerSupport()
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
void HelloTriangleApplication::fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _createInfo) 
{
    _createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    _createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    _createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    _createInfo.pfnUserCallback = debugCallback;
}
void HelloTriangleApplication::setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    fillDebugMessengerCreateInfo(createInfo);

    VCR(createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_callback), "Enable to create the messenger.");
}
#endif

#pragma endregion Instance & PhysicalDevice

void HelloTriangleApplication::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    // note: queue family must be unique, cannot create twice a queue for a same index.
    set<u32> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
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
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = (u32)deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    

#if _DEBUG
    createInfo.enabledLayerCount = (u32)validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    VCR(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice), "Failed to create the logical device.");

    vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);
}


#pragma region Swapchain
VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats) 
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
VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes) 
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
VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities) 
{
    if (_capabilities.currentExtent.width != numeric_limits<u32>::max()) 
        return _capabilities.currentExtent;


    VkExtent2D actualExtent = { m_WindowWidth, m_WindowHeight };

    actualExtent.width = clamp(actualExtent.width, _capabilities.minImageExtent.width, _capabilities.maxImageExtent.width);
    actualExtent.height = clamp(actualExtent.height, _capabilities.minImageExtent.height, _capabilities.maxImageExtent.height);

    return actualExtent;
}
void HelloTriangleApplication::createSwapchain() 
{
    SwapchainSupportDetails swapchainSupport = retrieveSwapchainSupportDetails(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    m_swapchainImageFormat = surfaceFormat.format;
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
    m_swapchainExtent = chooseSwapExtent(swapchainSupport.capabilities);

    u32 imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0) // a limit exist (nolimit = (u32)-1)
    {
        if(imageCount > swapchainSupport.capabilities.maxImageCount) // the limit is exceeded with minImageCount+1, then clamp it to maxImageCount
            imageCount = swapchainSupport.capabilities.maxImageCount;
    }


    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = m_swapchainImageFormat;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // images in swapchain are use directly as color, use TRANSFERT_DST to use other images to be transfered into swap chain

    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    u32 queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
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
void HelloTriangleApplication::createImageViews()
{
    m_swapchainImageViews.resize(m_swapchainImages.size());

    for (u32 i = 0; i < m_swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainImageFormat;

        // Allows to alterate the color channels view
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Subrange allows to specify a subregion of the image to view
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VCR(vkCreateImageView(m_logicalDevice, &createInfo, nullptr, &m_swapchainImageViews[i]), "Failed to create image view.");
    }
}
#pragma endregion Swapchain



void HelloTriangleApplication::cleanup() 
{
    for (VkImageView& imageView : m_swapchainImageViews)
    {
        vkDestroyImageView(m_logicalDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
    vkDestroyDevice(m_logicalDevice, nullptr);

#if _DEBUG
    destroyDebugUtilsMessengerEXT(m_instance, m_callback, nullptr);
#endif

    vkDestroySurfaceKHR(m_instance, m_windowSurface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
    glfwDestroyWindow(m_window);

    glfwTerminate(); // deinit glfw lib
}

void HelloTriangleApplication::mainLoop() 
{
    while (!glfwWindowShouldClose(m_window)) 
    {
        glfwPollEvents();
    }
}



int main() 
{
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}