#pragma once

#include <string>

#define HASH_SEED 2166136261

size_t Hash(const void *ptr, size_t size, size_t result = HASH_SEED);
size_t HashString(const char *str, size_t result = HASH_SEED);
size_t HashString(const std::string &str, size_t result = HASH_SEED);
size_t HashInt(const int type, size_t result = HASH_SEED);
size_t HashFloat(const float type, size_t result = HASH_SEED);
size_t HashPtr(const void *type, size_t result = HASH_SEED);