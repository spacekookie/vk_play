//
// My first Vulkan triangle example :)
//  License of the repository applies!
//

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <iostream>

// Our own util includes
#include "VDeleter.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 5

/** Wrapper class around vulkan code */
class HelloTriangleApplication {
public:
    void run() {
        initVulkan();
        mainLoop();
    }

private:
    void initVulkan() {

    }

    void mainLoop() {

    }
};

/** Main function to start wrapper application */
int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
