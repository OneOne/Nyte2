#include "HelloTriangleApplication.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <set>

using namespace std;

// VCR for Vulkan Check Result
#define VCR(val, msg) if(val != VK_SUCCESS) { throw std::runtime_error(msg); }

#if _DEBUG
// Check "Config/vk_layer_settings.txt" in VulkanSDK to get more information on how to configure validation layer.
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
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
}

#pragma region Instance Creation

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

    QueueFamilyIndices indices = findQueueFamilies(_device);

    return indices.isComplete();
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

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions() 
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

#pragma endregion Instance Creation

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

    createInfo.enabledExtensionCount = 0;

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

void HelloTriangleApplication::createWindowSurface()
{
    //VkWin32SurfaceCreateInfoKHR createInfo{};
    //createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    //createInfo.hwnd = glfwGetWin32Window(window);
    //createInfo.hinstance = GetModuleHandle(nullptr);

    //VCR(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface), "Failed to create the window surface.");

    VCR(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_windowSurface), "Failed to create the window surface.");
}


void HelloTriangleApplication::cleanup() 
{
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