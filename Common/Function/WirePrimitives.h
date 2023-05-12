#pragma once

#include "PCH.h"

void GenerateSphere(int sides, std::vector<short> &outIndices, std::vector<float> &outVertices);
void GenerateBox(std::vector<short> &outIndices, std::vector<float> &outVertices);

void GenerateSphere(int sides, std::vector<unsigned short> &outIndices, std::vector<float> &outVertices);
void GenerateBox(std::vector<unsigned short> &outIndices, std::vector<float> &outVertices);
