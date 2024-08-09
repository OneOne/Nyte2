#include "HelloTriangleApplication.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <set>
#include <limits>
#include <algorithm>
#include "FileHelper.h"

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
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSemaphoresAndFences();
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

void HelloTriangleApplication::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear color framebuffer before pass
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // no stencil == no care
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // index 0 (=> layout(location = 0) in shader)
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
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
VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code) 
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VCR(vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &shaderModule), "Failed to create shader module.");

    return shaderModule;
}
void HelloTriangleApplication::createGraphicsPipeline() 
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
    vector<octet> vertexShaderCode = FileHelper::readFile("Shaders/vertexshader.spv");
    vector<octet> fragmentShaderCode = FileHelper::readFile("Shaders/fragmentshader.spv");

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
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // Setup multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // Setup depth and stencil (enable depth test/write or stencil test/write)
    //VkPipelineDepthStencilStateCreateInfo depthAndStencil{};

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
    pipelineLayoutInfo.setLayoutCount = 0;            // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
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
    pipelineInfo.pDepthStencilState = nullptr; // Optional
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

void HelloTriangleApplication::createFramebuffers()
{
     m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

     for (u32 i = 0; i < (u32)m_swapchainImageViews.size(); i++) 
     {
         VkImageView attachments[] = { m_swapchainImageViews[i] };

         VkFramebufferCreateInfo framebufferInfo{};
         framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
         framebufferInfo.renderPass = m_renderPass;
         framebufferInfo.attachmentCount = 1;
         framebufferInfo.pAttachments = attachments;
         framebufferInfo.width = m_swapchainExtent.width;
         framebufferInfo.height = m_swapchainExtent.height;
         framebufferInfo.layers = 1;

         VCR(vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]), "Failed to create framebuffer.");
     }
}
void HelloTriangleApplication::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0; // Optional

    VCR(vkCreateCommandPool(m_logicalDevice, &poolInfo, nullptr, &m_commandPool), "Failed to create command pool.");
}
void HelloTriangleApplication::createCommandBuffers()
{
    m_commandBuffers.resize(m_swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (u32)m_commandBuffers.size();

    VCR(vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, m_commandBuffers.data()), "Failed to allocate command buffers.");

    for (u32 i = 0; i < (u32)m_commandBuffers.size(); i++) 
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        VCR(vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo), "Failed to begin command buffer.");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapchainFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapchainExtent;

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // VK_SUBPASS_CONTENTS_INLINE = render pass commands are directly included into the command buffer
        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0); // vertexCount, instanceCount, firstVertex, firstInstance

        vkCmdEndRenderPass(m_commandBuffers[i]);

        VCR(vkEndCommandBuffer(m_commandBuffers[i]), "Failed to end command buffer.");
    }
}

void HelloTriangleApplication::createSemaphoresAndFences()
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

void HelloTriangleApplication::cleanup() 
{
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
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
        drawFrame();
    }

    vkDeviceWaitIdle(m_logicalDevice);
}

void HelloTriangleApplication::drawFrame()
{
    // Acquire next available image in swapchain
    u32 imageIndex;
    vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

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

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores; // wait image is available
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];
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

    VCR(vkQueuePresentKHR(m_presentQueue, &presentInfo), "Failed to present.");
    
    vkQueueWaitIdle(m_presentQueue);

    //m_currentFrame = (m_currentFrame + 1) & 1;
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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