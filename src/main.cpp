#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_core.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>

using namespace std;

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

const vector<const char *> REQUIRED_VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};

const vector<const char *> REQUIRED_DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
[[maybe_unused]] const bool ENABLE_VALIDATION_LAYERS = false;
#else
[[maybe_unused]] const bool ENABLE_VALIDATION_LAYERS = true;
#endif

static vector<char> readFile(const string &filename) {
    ifstream file(filename, ios::ate | ios::binary);

    if (!file.is_open()) {
        throw runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

class Vitamin {
  public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

  private:
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice;
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    vector<VkImageView> swapChainImageViews;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vitamin", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        createSurface();
        pickAndPrintPhysicalDevices();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }
        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
        vkDestroyDevice(logicalDevice, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void createInstance() {
        VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                  .pApplicationName = "Vitamin",
                                  .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                  .pEngineName = "No Engine",
                                  .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                  .apiVersion = VK_API_VERSION_1_0};

        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        printVulkanExtensions(glfwExtensions, glfwExtensionCount);

        VkInstanceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                        .pApplicationInfo = &appInfo,
                                        .enabledExtensionCount = glfwExtensionCount,
                                        .ppEnabledExtensionNames = glfwExtensions};

        if (ENABLE_VALIDATION_LAYERS) {
            if (checkAndPrintValidationLayerSupport(REQUIRED_VALIDATION_LAYERS)) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(REQUIRED_VALIDATION_LAYERS.size());
                createInfo.ppEnabledLayerNames = REQUIRED_VALIDATION_LAYERS.data();
            } else {
                throw runtime_error("Validation layers requested, but some are not available!");
            }
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw runtime_error("Failed to create Vulkan instance!");
        }
    }

    void pickAndPrintPhysicalDevices() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw runtime_error("Failed to find any GPUs with Vulkan support!");
        }

        vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        cout << "Available GPUs:\n";

        multimap<int32_t, tuple<VkPhysicalDevice, VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures>, greater<int>> devicePreferenceMap;

        for (const auto &device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            QueueFamilyIndices queueFamilyIndices = findQueueFamilies(device);

            int32_t score = 0;

            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += 1000;
            }

            if (!deviceFeatures.geometryShader || !deviceFeatures.tessellationShader) {
                score = -10;
            }
            if (!queueFamilyIndices.isComplete()) {
                score = -20;
            }
            if (!checkDeviceExtensionSupport(device)) {
                score = -30;
            } else {
                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
                if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
                    score = -40;
                }
            }

            devicePreferenceMap.insert(make_pair(score, make_tuple(device, deviceProperties, deviceFeatures)));
        }

        bool isTopChoice = true;
        for (const auto &[score, mapValue] : devicePreferenceMap) {
            const auto &[device, props, features] = mapValue;

            if (score >= 0 && isTopChoice) {
                physicalDevice = device;
                cout << '*';
                isTopChoice = false;
            }

            cout << '\t' << score << '\t' << props.deviceID << ' ' << props.deviceName << ' ' << props.deviceType << '\n';
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw runtime_error("Failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                    .queueFamilyIndex = queueFamily,
                                                    .queueCount = 1,
                                                    .pQueuePriorities = &queuePriority};
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
                                      .pQueueCreateInfos = queueCreateInfos.data(),
                                      .enabledExtensionCount = static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size()),
                                      .ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data(),
                                      .pEnabledFeatures = &deviceFeatures};

        if (ENABLE_VALIDATION_LAYERS) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(REQUIRED_VALIDATION_LAYERS.size());
            createInfo.ppEnabledLayerNames = REQUIRED_VALIDATION_LAYERS.data();
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
            throw runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
    }

    void printVulkanExtensions(const char **requiredExtensions, uint32_t requiredExtensionCount) {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        cout << "Required Vulkan extensions:\n";

        for (uint32_t i = 0; i < requiredExtensionCount; ++i) {
            bool exists = find_if(begin(extensions), end(extensions), [&](const auto &element) {
                              return strcmp(element.extensionName, requiredExtensions[i]) == 0;
                          }) != end(extensions);

            if (exists) {
                cout << "ok";
            }

            cout << '\t' << requiredExtensions[i] << '\n';
        }

        cout << "Available Vulkan extensions:\n";

        for (const auto &extension : extensions) {
            cout << '\t' << extension.extensionName << '\n';
        }
    }

    bool checkAndPrintValidationLayerSupport(const vector<const char *> requiredValidationLayers) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        cout << "Required Vulkan validation layers:\n";

        bool isRequiredLayerMissing = false;
        for (const auto &requiredLayer : requiredValidationLayers) {
            bool exists = find_if(begin(availableLayers), end(availableLayers), [&](const auto &element) {
                              return strcmp(element.layerName, requiredLayer) == 0;
                          }) != end(availableLayers);

            if (exists) {
                cout << "ok";
            } else {
                isRequiredLayerMissing = true;
            }

            cout << '\t' << requiredLayer << '\n';
        }

        cout << "Available Vulkan validation layers:\n";

        for (const auto &availableLayer : availableLayers) {
            cout << '\t' << availableLayer.layerName << '\n';
        }

        return !isRequiredLayerMissing;
    }

    struct QueueFamilyIndices {
        optional<uint32_t> graphicsFamily;
        optional<uint32_t> presentFamily;

        bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }

            i++;
        }

        return indices;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        set<string> requiredExtensions(REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end());

        for (const auto &extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw runtime_error("Failed to create window surface!");
        }
    }

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        vector<VkSurfaceFormatKHR> formats;
        vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR> &availableFormats) {
        for (const auto &availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats.front();
    }

    VkPresentModeKHR chooseSwapPresentMode(const vector<VkPresentModeKHR> &availablePresentModes) {
        for (const auto &availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actualExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t minImageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && minImageCount > swapChainSupport.capabilities.maxImageCount) {
            minImageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                            .surface = surface,
                                            .minImageCount = minImageCount,
                                            .imageFormat = surfaceFormat.format,
                                            .imageColorSpace = surfaceFormat.colorSpace,
                                            .imageExtent = extent,
                                            .imageArrayLayers = 1,
                                            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                            .preTransform = swapChainSupport.capabilities.currentTransform,
                                            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                            .presentMode = presentMode,
                                            .clipped = VK_TRUE,
                                            .oldSwapchain = VK_NULL_HANDLE};

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw runtime_error("Failed to create a swap chain!");
        }

        uint32_t imageCount;
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);

        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = swapChainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = swapChainImageFormat,
                .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                               .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                               .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                               .a = VK_COMPONENT_SWIZZLE_IDENTITY},
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

            if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                            .stage = VK_SHADER_STAGE_VERTEX_BIT,
                                                            .module = vertShaderModule,
                                                            .pName = "main"};

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                                                            .module = fragShaderModule,
                                                            .pName = "main"};

        [[maybe_unused]] VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
    }

    VkShaderModule createShaderModule(const vector<char> &code) {
        VkShaderModuleCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                            .codeSize = code.size(),
                                            .pCode = reinterpret_cast<const uint32_t *>(code.data())};

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw runtime_error("Failed to create shader module!");
        }

        return shaderModule;
    }
};

int main() {
    Vitamin app;

    try {
        app.run();
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
