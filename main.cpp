#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>
#include <set>

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
    int presentFamily = -1;

    bool isComplete() {
        return graphicsFamily >= 0 && presentFamily >= 0;
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

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
    VDeleter<VkDevice> device{vkDestroyDevice};
    VDeleter<VkSurfaceKHR> surface{instance, vkDestroySurfaceKHR};

    VDeleter<VkDebugReportCallbackEXT> callback{instance, DestroyDebugReportCallbackEXT};
    
    /** Implicitly destroyed when the instance is destroyed */
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    VkQueue graphicsQueue;
    VkQueue presentQueue;


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
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
    }


     void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

        float queuePriority = 1.0f;
        for (int queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = (uint32_t) queueCreateInfos.size();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = 0;
        
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, device.replace()) != VK_SUCCESS) {
            throw runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
    }


    void pickPhysicalDevice() {

        /** First search for Vulkan enabled GPUs */
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        /** If there is none, there is no point to continue... */
        if (deviceCount == 0) {
            throw runtime_error("failed to find GPUs with Vulkan support!");
        }

        /** Get the device list */
        vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        cout << "Considering available devices (" << deviceCount << "): " << endl;
        for (const auto& dev : devices) {
            if (isDeviceSuitable(dev)) {
                physicalDevice = dev;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw runtime_error("failed to find a suitable GPU!");
        }

    }


    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface.replace()) != VK_SUCCESS) {
            throw runtime_error("failed to create window surface!");
        }
    }


    void createInstance() {
        cout << "Creating Vulkan instance..." << endl;

        /** First we want to check if validation layers are available */
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw runtime_error("validation layers requested, but not available!");
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
            throw runtime_error("failed to create instance!");
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
            throw runtime_error("failed to set up debug callback!");
        }
    }


    vector<const char*> getRequiredExtensions() {
        vector<const char*> extensions;

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

        QueueFamilyIndices indices = findQueueFamilies(device);
        return indices.isComplete();
    }


    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (queueFamily.queueCount > 0 && presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }


    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        vector<VkLayerProperties> availableLayers(layerCount);
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

        cerr << "validation layer: " << msg << endl;
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

