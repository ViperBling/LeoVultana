#ifdef ID_SKINNING_MATRICES

struct Matrix2
{
    mat4 m_Current;
    mat4 m_Previous;
};

layout (std140, binding = ID_SKINNING_MATRICES) uniform perSkeleton
{
    Matrix2 u_ModelMatrix[200];
} mPerSkeleton;

mat4 GetSkinningMatrix(vec4 Weights, uvec4 Joints)
{
    mat4 skinningMatrix =
        Weights.x * mPerSkeleton.u_ModelMatrix[Joints.x].m_Current +
        Weights.y * mPerSkeleton.u_ModelMatrix[Joints.y].m_Current +
        Weights.z * mPerSkeleton.u_ModelMatrix[Joints.z].m_Current +
        Weights.w * mPerSkeleton.u_ModelMatrix[Joints.w].m_Current;
    return skinningMatrix;
}
#endif