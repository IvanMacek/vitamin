#include "vulkan/vulkan_core.h"
#include <stdint.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iterator>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <tuple>
#include <vector>

using namespace std;

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

const std::vector<const char *> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
[[maybe_unused]] const bool ENABLE_VALIDATION_LAYERS = false;
#else
[[maybe_unused]] const bool ENABLE_VALIDATION_LAYERS = true;
#endif

class HelloTriangleApplication {
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

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vitamin", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        pickAndPrintPhysicalDevices();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
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
            if (checkAndPrintValidationLayerSupport(VALIDATION_LAYERS)) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
                createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
            } else {
                throw std::runtime_error("Validation layers requested, but some are not available!");
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
            throw std::runtime_error("Failed to find any GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
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

            if (!queueFamilyIndices.graphicsFamily.has_value()) {
                score = -1;
            }
            if (!deviceFeatures.geometryShader || !deviceFeatures.tessellationShader) {
                score = -1;
            }

            devicePreferenceMap.insert(make_pair(score, make_tuple(device, deviceProperties, deviceFeatures)));
        }

        VkPhysicalDevice selectedPhysicalDevice = VK_NULL_HANDLE;
        bool isTopChoice = true;
        for (const auto &[score, mapValue] : devicePreferenceMap) {
            const auto &[device, props, features] = mapValue;

            if (score >= 0 && isTopChoice) {
                selectedPhysicalDevice = device;
                cout << '*';
                isTopChoice = false;
            }

            cout << '\t' << score << '\t' << props.deviceID << ' ' << props.deviceName << ' ' << props.deviceType << '\n';
        }
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

    bool checkAndPrintValidationLayerSupport(const std::vector<const char *> requiredValidationLayers) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
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
        std::optional<uint32_t> graphicsFamily;
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            i++;
        }

        return indices;
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
