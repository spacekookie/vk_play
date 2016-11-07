#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>

#include "vk_deleter.h"

using namespace std;


/*********************************************************************************************************/

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

/*********************************************************************************************************/

struct QueueFamilyIndices {
    int graphicsFamily = -1;

    bool isComplete() {
        return graphicsFamily >= 0;
    }
};

/*********************************************************************************************************/


class HelloTriangleApplication {
public:
    const int WIDTH = 800;
    const int HEIGHT = 600;

    const vector<const char*> validationLayers = {
        "VK_LAYER_LUNARG_standard_validation"
    };

    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif


    void run() {
        initWindow();
        initVulkan();
        mainLoop();
    }

private:
    GLFWwindow* window;
    
    /** Vk Deleters to clean up after ourselves if we fail with something */
    VDeleter<VkInstance> instance{vkDestroyInstance};
    VDeleter<VkDebugReportCallbackEXT> callback{instance, DestroyDebugReportCallbackEXT};
    
    /** Implicitly destroyed when the instance is destroyed */
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    /**
     * Stuff that's required to get a valid window on the screen.
     * This is platform dependant and for now, we will use GLFW
     * to take care of all the X11 stuff for us...
     *
     */
    void initWindow() {
        cout << "Initialising GLFW window context..." << endl;
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    /**
     * Actual Vulkan API initialisation
     */
    void initVulkan() {
        cout << "Starting Vulkan initialisation..." << endl;
        createInstance();
        setupDebugCallback();
        pickPhysicalDevice();
    }


    void pickPhysicalDevice() {

        /** First search for Vulkan enabled GPUs */
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        /** If there is none, there is no point to continue... */
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        /** Get the device list */
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        cout << "Considering available devices (" << deviceCount << "): " << endl;
        for (const auto& dev : devices) {
            if (isDeviceSuitable(dev)) {
                physicalDevice = dev;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

    }


    void createInstance() {
        cout << "Creating Vulkan instance..." << endl;

        /** First we want to check if validation layers are available */
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        /** Create a Vk application info*/
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VkPlay";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        /** Create a Vk instance info */
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        /** Get required extentions */
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        /** Setup validation layers for the creat info struct */
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        /** Finally attempt to create a Vk Instance... */
        if (vkCreateInstance(&createInfo, nullptr, instance.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }

    }


    /*****************************************************************************************************
     ************************ UTILITY SETUP FUNCTIONS THAT USUALLY DEAL WITH DATA ************************
     *****************************************************************************************************/


    void setupDebugCallback() {
        if (!enableValidationLayers) return;

        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        createInfo.pfnCallback = debugCallback;

        if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, callback.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug callback!");
        }
    }


    std::vector<const char*> getRequiredExtensions() {
        std::vector<const char*> extensions;

        unsigned int glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        cout << "Available GLFW extensions: " << endl;

        for (unsigned int i = 0; i < glfwExtensionCount; i++) {
            extensions.push_back(glfwExtensions[i]);
            cout << "\t" << glfwExtensions[i] << endl;
        }

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        return extensions;
    }


    bool isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties devProps;
        VkPhysicalDeviceFeatures devFeatures;

        vkGetPhysicalDeviceProperties(device, &devProps);
        vkGetPhysicalDeviceFeatures(device, &devFeatures);

        cout << "\t" << devProps.deviceName << endl;

        return true;
    }


    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        /** First find out the amount of queue families */
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        /** Then the exact items of queue families */
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());



        return indices;
    }




    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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


    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, 
                                                            VkDebugReportObjectTypeEXT objType,
                                                            uint64_t obj,
                                                            size_t location,
                                                            int32_t code,
                                                            const char* layerPrefix,
                                                            const char* msg,
                                                            void* userData) {

        std::cerr << "validation layer: " << msg << std::endl;
        return VK_FALSE;
    }


    /*******************************************************************************************************
     *******************************************************************************************************
     *******************************************************************************************************/

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }
};

int main() {
    HelloTriangleApplication app;
    cout << "Starting VkPlay..." << endl;

    try {
        app.run();
    } catch (const runtime_error& e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

