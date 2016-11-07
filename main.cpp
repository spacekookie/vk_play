#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>

#include "vk_deleter.h"

class HelloTriangleApplication {
public:
    const int WIDTH = 800;
    const int HEIGHT = 600;

    const std::vector<const char*> validationLayers = {
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
    VDeleter<VkInstance> instance {vkDestroyInstance};

    /**
     * Stuff that's required to get a valid window on the screen.
     * This is platform dependant and for now, we will use GLFW
     * to take care of all the X11 stuff for us...
     *
     */
    void initWindow() {
        std::cout << "Initialising GLFW window context..." << std::endl;
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    /**
     * Actual Vulkan API initialisation
     */
    void initVulkan() {
        std::cout << "Starting Vulkan initialisation..." << std::endl;
        createInstance();
    }

    void createInstance() {
        std::cout << "Creating Vulkan instance..." << std::endl;

        /** First we want to check if validation layers are available */
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        /** Create a Vk application info*/
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VK Play";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        /** Create a Vk instance info */
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        /** Prepare GLFW context for Vk drawing */
        unsigned int glfwExtensionCount = 0;
        const char** glfwExtensions;

        /** Get the draw extention for the instance */
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        /** Assign the data for us to use */
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        /** Probe for extentions count */
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        /** Fill a vector with available extentions */
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        /** Print available extentions */
        std::cout << "Available Vk Extensions:" << std::endl;
        for (const auto& extension : extensions) {
            std::cout << "\t> " << extension.extensionName << std::endl;
        }

        /** Attempt to create a new VKInstance */
        if (vkCreateInstance(&createInfo, nullptr, instance.replace()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
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


    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }
};

int main() {
    HelloTriangleApplication app;
    std::cout << "Starting VkPlay..." << std::endl;

    try {
        app.run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}