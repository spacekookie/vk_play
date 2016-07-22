//
// Created by spacekookie on 22/07/16.
//

#ifndef VULKAN_PLAY_VDELETER_H
#define VULKAN_PLAY_VDELETER_H

#include <vulkan/vulkan.h>
#include <functional>

using std::function

template <typename T>
class VDeleter {
public:
    VDeleter() : VDeleter([](T _) {}) {}

    VDeleter(function<void(T, VkAllocationCallbacks*)> deletef) {
        object = VK_NULL_HANDLE;
        this->deleter = [=](T obj) { deletef(obj, nullptr); };
    }

    VDeleter(const VDeleter<VkInstance>& instance, function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
        object = VK_NULL_HANDLE;
        this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
    }

    VDeleter(const VDeleter<VkDevice>& device, function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
        object = VK_NULL_HANDLE;
        this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
    }

    ~VDeleter() {
        cleanup();
    }

    T* operator &() {
        cleanup();
        return &object;
    }

    operator T() const {
        return object;
    }

private:
    T object;
    function<void(T)> deleter;

    void cleanup() {
        if (object != VK_NULL_HANDLE) {
            deleter(object);
        }
        object = VK_NULL_HANDLE;
    }
};



#endif //VULKAN_PLAY_VDELETER_H
