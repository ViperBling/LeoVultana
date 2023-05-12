//
// Created by Administrator on 2023/5/12.
//

#include "WirePrimitives.h"
#include "Camera.h"

void GenerateSphere(int sides, std::vector<unsigned short> &outIndices, std::vector<float> &outVertices)
{
    int i = 0;

    outIndices.clear();
    outVertices.clear();

    for (int roll = 0; roll < sides; roll++)
    {
        for (int pitch = 0; pitch < sides; pitch++)
        {
            outIndices.push_back(i);
            outIndices.push_back(i + 1);
            outIndices.push_back(i);
            outIndices.push_back(i + 2);
            i += 3;

            math::Vector4 v1 = PolarToVector((float)(roll    ) * (2.0f * XM_PI) / (float)sides, (float)(pitch    ) * (2.0f * XM_PI) / (float)sides);
            math::Vector4 v2 = PolarToVector((float)(roll + 1) * (2.0f * XM_PI) / (float)sides, (float)(pitch    ) * (2.0f * XM_PI) / (float)sides);
            math::Vector4 v3 = PolarToVector((float)(roll    ) * (2.0f * XM_PI) / (float)sides, (float)(pitch + 1) * (2.0f * XM_PI) / (float)sides);

            outVertices.push_back(v1.getX()); outVertices.push_back(v1.getY()); outVertices.push_back(v1.getZ());
            outVertices.push_back(v2.getX()); outVertices.push_back(v2.getY()); outVertices.push_back(v2.getZ());
            outVertices.push_back(v3.getX()); outVertices.push_back(v3.getY()); outVertices.push_back(v3.getZ());
        }
    }
}

void GenerateBox(std::vector<unsigned short> &outIndices, std::vector<float> &outVertices)
{
    outIndices.clear();
    outVertices.clear();

    std::vector<unsigned short> indices =
        {
            0,1, 1,2, 2,3, 3,0,
            4,5, 5,6, 6,7, 7,4,
            0,4,
            1,5,
            2,6,
            3,7
        };

    std::vector<float> vertices =
        {
            -1,  -1,   1,
            1,  -1,   1,
            1,   1,   1,
            -1,   1,   1,
            -1,  -1,  -1,
            1,  -1,  -1,
            1,   1,  -1,
            -1,   1,  -1,
        };

    outIndices = indices;
    outVertices = vertices;
}
