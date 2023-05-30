#define MAX_LIGHT_INSTANCES  80
#define MAX_SHADOW_INSTANCES 32

struct Light
{
    mat4          lightViewProj;
    mat4          lightView;

    vec3          direction;
    float         range;

    vec3          color;
    float         intensity;

    vec3          position;
    float         innerConeCos;

    float         outerConeCos;
    int           type;
    float         depthBias;
    int           shadowMapIndex;
};

const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;

struct PerFrame
{
    mat4          u_mCameraCurrViewProj;
    mat4          u_mCameraPrevViewProj;
    mat4          u_mCameraCurrViewProjInverse;

    vec4          u_CameraPos;
    float         u_IBLFactor;
    float         u_EmissiveFactor;
    vec2          u_invScreenResolution;

    vec4          u_WireframeOptions;
    float         u_LODBias;
    vec2          u_Padding;
    int           u_LightCount;
    Light         u_Lights[MAX_LIGHT_INSTANCES];
};