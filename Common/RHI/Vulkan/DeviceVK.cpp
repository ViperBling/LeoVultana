#include "DeviceVK.h"

using namespace LeoVultana_VK;

Device::Device() {

}

Device::~Device() {

}

void Device::OnCreate(const char *pAppName, const char *pEngineName, bool cpuValidationLayerEnabled,
                      bool gpuValidationLayerEnabled, HWND hWnd) {

}

void Device::SetEssentialInstanceExtensions(bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled,
                                            InstancePropertiesVK *pInstProp) {

}

void Device::SetEssentialDeviceExtensions(DevicePropertiesVK *pDeviceProp) {

}

void Device::OnCreateEx(VkInstance vulkanInstance, VkPhysicalDevice physicalDevice, HWND hWnd,
                        DevicePropertiesVK *pDeviceProp) {

}

void Device::OnDestroy() {

}

void Device::GetDeviceInfo(std::string *deviceName, std::string *driverVersion) {

}

void Device::CreatePipelineCache() {

}

void Device::DestroyPipelineCache() {

}

VkPipelineCache Device::GetPipelineCache() {
    return nullptr;
}

void Device::GPUFlush() {

}
