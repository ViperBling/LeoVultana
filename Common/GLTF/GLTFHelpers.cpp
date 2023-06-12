#include "GLTFHelpers.h"

int GetFormatSize(int id)
{
    switch (id)
    {
        case 5120: return 1; //(BYTE)
        case 5121: return 1; //(UNSIGNED_BYTE)1
        case 5122: return 2; //(SHORT)2
        case 5123: return 2; //(UNSIGNED_SHORT)2
        case 5124: return 4; //(SIGNED_INT)4
        case 5125: return 4; //(UNSIGNED_INT)4
        case 5126: return 4; //(FLOAT)
        default : return 1;
    }
    return -1;
}

int GetDimensions(const std::string &str)
{
    if (str == "SCALAR")    return  1;
    else if (str == "VEC2") return  2;
    else if (str == "VEC3") return  3;
    else if (str == "VEC4") return  4;
    else if (str == "MAT4") return  4*4;
    else return  -1;
}

void SplitGLTFAttribute(std::string attribute, std::string *semanticName, uint32_t *semanticIndex)
{
    *semanticIndex = 0;

    if (isdigit(attribute.back()))
    {
        *semanticIndex = attribute.back() - '0';

        attribute.pop_back();
        if (attribute.back() == '_')
            attribute.pop_back();
    }

    *semanticName = attribute;
}

math::Vector4 GetVector(const json::array_t &accessor)
{
    if (accessor.size() == 4) return {accessor[0], accessor[1], accessor[2], accessor[3]};
    else return {accessor[0], accessor[1], accessor[2], 0};
}

math::Matrix4 GetMatrix(const json::array_t &accessor)
{
    return {
        math::Vector4(accessor[0], accessor[1], accessor[2], accessor[3]),
        math::Vector4(accessor[4], accessor[5], accessor[6], accessor[7]),
        math::Vector4(accessor[8], accessor[9], accessor[10], accessor[11]),
        math::Vector4(accessor[12], accessor[13], accessor[14], accessor[15])};
}

template <class type>
type GetElement(const json::object_t *pRoot, const char *path, type pDefault)
{
    const char *p = path;
    char token[128];
    while (true)
    {
        for (; *p != '/' && *p != 0 && *p != '['; p++);
        memcpy(token, path, p - path);
        token[p - path] = 0;

        auto it = pRoot->find(token);
        if (it == pRoot->end())
            return pDefault;

        if (*p == '[')
        {
            p++;
            int i = atoi(p);
            for (; *p != 0 && *p != ']'; p++);
            pRoot = it->second.at(i).get_ptr<const json::object_t *>();
            p++;
        }
        else
        {
            if (it->second.is_object())
                pRoot = it->second.get_ptr<const json::object_t *>();
            else
            {
                return it->second.get<type>();
            }
        }
        p++;
        path = p;
    }

    return pDefault;
}

std::string GetElementString(const json::object_t &root, const char *path, std::string pDefault)
{
    return GetElement<std::string>(&root, path, pDefault);
}

bool GetElementBoolean(const json::object_t &root, const char *path, bool defaults)
{
    return GetElement<bool>(&root, path, defaults);
}

float GetElementFloat(const json::object_t &root, const char *path, float defaults)
{
    return GetElement<float>(&root, path, defaults);
}

int GetElementInt(const json::object_t &root, const char *path, int defaults)
{
    return GetElement<int>(&root, path, defaults);
}

json::array_t GetElementJsonArray(const json::object_t &root, const char *path, json::array_t defaults)
{
    return GetElement<json::array_t>(&root, path, defaults);
}

math::Vector4 GetElementVector(json::object_t &root, const char *path, math::Vector4 defaults)
{
    if (root.find(path) != root.end() && !root[path].is_null())
    {
        return GetVector(root[path].get<json::array_t>());
    }
    else
        return defaults;
}

