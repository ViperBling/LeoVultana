#pragma once

#include "PCH.h"
#include "json.h"
#include "vectormath/vectormath.hpp"

using json = nlohmann::json;

int GetFormatSize(int id);
int GetDimensions(const std::string &str);
void SplitGltfAttribute(std::string attribute, std::string *semanticName, uint32_t *semanticIndex);

math::Vector4 GetVector(const json::array_t &accessor);
math::Matrix4 GetMatrix(const json::array_t &accessor);
std::string GetElementString(const json::object_t &root, const char *path, std::string pDefault);
float GetElementFloat(const json::object_t &root, const char *path, float pDefault);
int GetElementInt(const json::object_t &root, const char *path, int pDefault);
bool GetElementBoolean(const json::object_t &root, const char *path, bool pDefault);
json::array_t GetElementJsonArray(const json::object_t &root, const char *path, json::array_t pDefault);
math::Vector4 GetElementVector(json::object_t &root, const char *path, math::Vector4 defaults);