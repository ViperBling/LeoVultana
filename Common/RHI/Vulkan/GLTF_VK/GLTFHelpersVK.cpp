#include "RHI/Vulkan/VKCommon/PCHVK.h"
#include "GLTFHelpersVK.h"

namespace LeoVultana_VK
{
    VkFormat GetFormat(const std::string &str, int id)
    {
        if (str == "SCALAR")
        {
            switch (id)
            {
                case 5120: return VK_FORMAT_R8_SINT;            // (BYTE)
                case 5121: return VK_FORMAT_R8_UINT;            // (UNSIGNED_BYTE)1
                case 5122: return VK_FORMAT_R16_SINT;           // (SHORT)2
                case 5123: return VK_FORMAT_R16_UINT;           // (UNSIGNED_SHORT)2
                case 5124: return VK_FORMAT_R32_SINT;           // (SIGNED_INT)4
                case 5125: return VK_FORMAT_R32_UINT;           // (UNSIGNED_INT)4
                case 5126: return VK_FORMAT_R32_SFLOAT;         // (FLOAT)
            }
        }
        else if (str == "VEC2")
        {
            switch (id)
            {
                case 5120: return VK_FORMAT_R8G8_SINT;          // (BYTE)
                case 5121: return VK_FORMAT_R8G8_UINT;          // (UNSIGNED_BYTE)1
                case 5122: return VK_FORMAT_R16G16_SINT;        // (SHORT)2
                case 5123: return VK_FORMAT_R16G16_UINT;        // (UNSIGNED_SHORT)2
                case 5124: return VK_FORMAT_R32G32_SINT;        // (SIGNED_INT)4
                case 5125: return VK_FORMAT_R32G32_UINT;        // (UNSIGNED_INT)4
                case 5126: return VK_FORMAT_R32G32_SFLOAT;      // (FLOAT)
            }
        }
        else if (str == "VEC3")
        {
            switch (id)
            {
                case 5120: return VK_FORMAT_UNDEFINED;          // (BYTE)
                case 5121: return VK_FORMAT_UNDEFINED;          // (UNSIGNED_BYTE)1
                case 5122: return VK_FORMAT_UNDEFINED;          // (SHORT)2
                case 5123: return VK_FORMAT_UNDEFINED;          // (UNSIGNED_SHORT)2
                case 5124: return VK_FORMAT_R32G32B32_SINT;     // (SIGNED_INT)4
                case 5125: return VK_FORMAT_R32G32B32_UINT;     // (UNSIGNED_INT)4
                case 5126: return VK_FORMAT_R32G32B32_SFLOAT;   // (FLOAT)
            }
        }
        else if (str == "VEC4")
        {
            switch (id)
            {
                case 5120: return VK_FORMAT_R8G8B8A8_SINT;      // (BYTE)
                case 5121: return VK_FORMAT_R8G8B8A8_UINT;      // (UNSIGNED_BYTE)1
                case 5122: return VK_FORMAT_R16G16B16A16_SINT;  // (SHORT)2
                case 5123: return VK_FORMAT_R16G16B16A16_UINT;  // (UNSIGNED_SHORT)2
                case 5124: return VK_FORMAT_R32G32B32A32_SINT;  // (SIGNED_INT)4
                case 5125: return VK_FORMAT_R32G32B32A32_UINT;  // (UNSIGNED_INT)4
                case 5126: return VK_FORMAT_R32G32B32A32_SFLOAT; // (FLOAT)
            }
        }

        return VK_FORMAT_UNDEFINED;
    }

    uint32_t SizeOfFormat(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R8_SINT:             return 1;           // (BYTE)
            case VK_FORMAT_R8_UINT:             return 1;           // (UNSIGNED_BYTE)1
            case VK_FORMAT_R16_SINT:            return 2;           // (SHORT)2
            case VK_FORMAT_R16_UINT:            return 2;           // (UNSIGNED_SHORT)2
            case VK_FORMAT_R32_SINT:            return 4;           // (SIGNED_INT)4
            case VK_FORMAT_R32_UINT:            return 4;           // (UNSIGNED_INT)4
            case VK_FORMAT_R32_SFLOAT:          return 4;           // (FLOAT)

            case VK_FORMAT_R8G8_SINT:           return 2 * 1;       // (BYTE)
            case VK_FORMAT_R8G8_UINT:           return 2 * 1;       // (UNSIGNED_BYTE)1
            case VK_FORMAT_R16G16_SINT:         return 2 * 2;       // (SHORT)2
            case VK_FORMAT_R16G16_UINT:         return 2 * 2;       // (UNSIGNED_SHORT)2
            case VK_FORMAT_R32G32_SINT:         return 2 * 4;       // (SIGNED_INT)4
            case VK_FORMAT_R32G32_UINT:         return 2 * 4;       // (UNSIGNED_INT)4
            case VK_FORMAT_R32G32_SFLOAT:       return 2 * 4;       // (FLOAT)

            case VK_FORMAT_UNDEFINED:           return 0;           //  (BYTE) (UNSIGNED_BYTE) (SHORT) (UNSIGNED_SHORT)
            case VK_FORMAT_R32G32B32_SINT:      return 3 * 4;       //  (SIGNED_INT)4
            case VK_FORMAT_R32G32B32_UINT:      return 3 * 4;       //  (UNSIGNED_INT)4
            case VK_FORMAT_R32G32B32_SFLOAT:    return 3 * 4;       //  (FLOAT)

            case VK_FORMAT_R8G8B8A8_SINT:       return 4 * 1;       //  (BYTE)
            case VK_FORMAT_R8G8B8A8_UINT:       return 4 * 1;       //  (UNSIGNED_BYTE)1
            case VK_FORMAT_R16G16B16A16_SINT:   return 4 * 2;       //  (SHORT)2
            case VK_FORMAT_R16G16B16A16_UINT:   return 4 * 2;       //  (UNSIGNED_SHORT)2
            case VK_FORMAT_R32G32B32A32_SINT:   return 4 * 4;       //  (SIGNED_INT)4
            case VK_FORMAT_R32G32B32A32_UINT:   return 4 * 4;       //  (UNSIGNED_INT)4
            case VK_FORMAT_R32G32B32A32_SFLOAT: return 4 * 4;       //  (FLOAT)
        }
        return 0;
    }
}

