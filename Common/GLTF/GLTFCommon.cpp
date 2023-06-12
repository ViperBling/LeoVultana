#include "GLTFCommon.h"
#include "GLTFHelpers.h"
#include "Misc.h"

bool GLTFCommon::Load(const std::string &path, const std::string &filename)
{
    Profile p("GLTFCommon::Load");

    mPath = path;

    std::ifstream f(path + filename);
    if (!f)
    {
        Trace(format("The file %s cannot be found\n", filename.c_str()));
        return false;
    }

    f >> j3;

    // Load Buffers
    //
    if (j3.find("buffers") != j3.end())
    {
        const json &buffers = j3["buffers"];
        mBuffersData.resize(buffers.size());
        for (int i = 0; i < buffers.size(); i++)
        {
            const std::string &name = buffers[i]["uri"];
            std::ifstream ff(path + name, std::ios::in | std::ios::binary);

            ff.seekg(0, ff.end);
            std::streamoff length = ff.tellg();
            ff.seekg(0, ff.beg);

            char *temp = new char[length];
            ff.read(temp, length);
            mBuffersData[i] = temp;
        }
    }

    // Load Meshes
    //
    m_pAccessors = &j3["accessors"];
    m_pBufferViews = &j3["bufferViews"];
    const json &meshes = j3["meshes"];
    mMeshes.resize(meshes.size());
    for (int i = 0; i < meshes.size(); i++)
    {
        gltfMesh *tfmesh = &mMeshes[i];
        auto &primitives = meshes[i]["primitives"];
        tfmesh->m_pPrimitives.resize(primitives.size());
        for (int j = 0; j < primitives.size(); j++)
        {
            gltfPrimitives *pPrimitive = &tfmesh->m_pPrimitives[j];

            int positionId = primitives[j]["attributes"]["POSITION"];
            const json &accessor = m_pAccessors->at(positionId);

            math::Vector4 max = GetVector(GetElementJsonArray(accessor, "max", { 0.0, 0.0, 0.0, 0.0 }));
            math::Vector4 min = GetVector(GetElementJsonArray(accessor, "min", { 0.0, 0.0, 0.0, 0.0 }));

            pPrimitive->mCenter = (min + max) * 0.5f;
            pPrimitive->mRadius = max - pPrimitive->mCenter;

            pPrimitive->mCenter = math::Vector4(pPrimitive->mCenter.getXYZ(), 1.0f); //set the W to 1 since this is a position not a direction
        }
    }

    // Load lights
    //
    if (j3.find("extensions") != j3.end())
    {
        const json &extensions = j3["extensions"];
        if (extensions.find("KHR_lights_punctual") != extensions.end())
        {
            const json &KHR_lights_punctual = extensions["KHR_lights_punctual"];
            if (KHR_lights_punctual.find("lights") != KHR_lights_punctual.end())
            {
                const json &lights = KHR_lights_punctual["lights"];
                mLights.resize(lights.size());
                for (int i = 0; i < lights.size(); i++)
                {
                    json::object_t light = lights[i];
                    mLights[i].mColor = GetElementVector(light, "color", math::Vector4(1, 1, 1, 0));
                    mLights[i].mRange = GetElementFloat(light, "range", 105);
                    mLights[i].mIntensity = GetElementFloat(light, "intensity", 1);
                    mLights[i].mInnerConeAngle = GetElementFloat(light, "spot/innerConeAngle", 0);
                    mLights[i].mOuterConeAngle = GetElementFloat(light, "spot/outerConeAngle", XM_PIDIV4);

                    std::string lightName = GetElementString(light, "name", "");

                    std::string lightType = GetElementString(light, "type", "");
                    if (lightType == "spot")
                        mLights[i].mType = gltfLight::LIGHT_SPOTLIGHT;
                    else if (lightType == "point")
                        mLights[i].mType = gltfLight::LIGHT_POINTLIGHT;
                    else if (lightType == "directional")
                        mLights[i].mType = gltfLight::LIGHT_DIRECTIONAL;

                    // Deal with shadow settings
                    if (mLights[i].mType == gltfLight::LIGHT_DIRECTIONAL || mLights[i].mType == gltfLight::LIGHT_SPOTLIGHT)
                    {
                        // Unless "NoShadow" is present in the name, the light will have a shadow
                        if (std::string::npos != lightName.find("NoShadow", 0, 8))
                            mLights[i].mShadowResolution = 0; // 0 shadow resolution means no shadow
                        else
                        {
                            // See if we have specified a resolution
                            size_t offset = lightName.find("Resolution_", 0, 11);
                            if (std::string::npos != offset)
                            {
                                // Update offset to start from after "_"
                                offset += 11;

                                // Look for the end separator
                                size_t endOffset = lightName.find("_", offset, 1);
                                if (endOffset != std::string::npos)
                                {
                                    // Try to grab the value
                                    std::string ResString = lightName.substr(offset, endOffset - offset);
                                    int32_t Resolution = -1;
                                    try {
                                        Resolution = std::stoi(ResString);
                                    }
                                    catch (std::invalid_argument)
                                    {
                                        // Wasn't a valid argument to convert to int, use default
                                    }
                                    catch (std::out_of_range)
                                    {
                                        // Value larger than an int can hold (also invalid), use default
                                    }

                                    // Check if resolution is a power of 2
                                    if (Resolution == 1 || (Resolution & (Resolution - 1)) == 0)
                                        mLights[i].mShadowResolution = (uint32_t)Resolution;
                                }
                            }

                            // See if we have specified a bias 
                            offset = lightName.find("Bias_", 0, 5);
                            if (std::string::npos != offset)
                            {
                                // Update offset to start from after "_"
                                offset += 5;

                                // Look for the end separator
                                size_t endOffset = lightName.find("_", offset, 1);
                                if (endOffset != std::string::npos)
                                {
                                    // Try to grab the value
                                    std::string BiasString = lightName.substr(offset, endOffset - offset);
                                    float Bias = (mLights[i].mType == LightType_Spot) ? (70.0f / 100000.0f) : (1000.0f / 100000.0f);

                                    try {
                                        Bias = std::stof(BiasString);
                                    }
                                    catch (std::invalid_argument)
                                    {
                                        // Wasn't a valid argument to convert to float, use default
                                    }
                                    catch (std::out_of_range)
                                    {
                                        // Value larger than a float can hold (also invalid), use default
                                    }

                                    // Set what we have 
                                    mLights[i].mBias = Bias;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Load cameras
    //
    if (j3.find("cameras") != j3.end())
    {
        const json &cameras = j3["cameras"];
        mCameras.resize(cameras.size());
        for (int i = 0; i < cameras.size(); i++)
        {
            const json &camera = cameras[i];
            gltfCamera *tfcamera = &mCameras[i];

            tfcamera->yFov = GetElementFloat(camera, "perspective/yFov", 0.1f);
            tfcamera->zNear = GetElementFloat(camera, "perspective/zNear", 0.1f);
            tfcamera->zFar = GetElementFloat(camera, "perspective/zFar", 100.0f);
            tfcamera->mNodeIndex = -1;
        }
    }

    // Load nodes
    //
    if (j3.find("nodes") != j3.end())
    {
        const json &nodes = j3["nodes"];
        mNodes.resize(nodes.size());
        for (int i = 0; i < nodes.size(); i++)
        {
            gltfNode *tfnode = &mNodes[i];

            // Read node data
            //
            json::object_t node = nodes[i];

            if (node.find("children") != node.end())
            {
                for (int c = 0; c < node["children"].size(); c++)
                {
                    int nodeID = node["children"][c];
                    tfnode->mChildren.push_back(nodeID);
                }
            }

            tfnode->meshIndex = GetElementInt(node, "mesh", -1);
            tfnode->skinIndex = GetElementInt(node, "skin", -1);

            int cameraIdx = GetElementInt(node, "camera", -1);
            if (cameraIdx >= 0)
                mCameras[cameraIdx].mNodeIndex = i;

            int lightIdx = GetElementInt(node, "extensions/KHR_lights_punctual/light", -1);
            if (lightIdx >= 0)
            {
                mLightInstances.push_back({ lightIdx, i});
            }

            tfnode->mTransform.mTranslation = GetElementVector(node, "translation", math::Vector4(0, 0, 0, 0));
            tfnode->mTransform.mScale = GetElementVector(node, "scale", math::Vector4(1, 1, 1, 0));


            if (node.find("name") != node.end())
                tfnode->mName = GetElementString(node, "name", "unnamed");

            if (node.find("rotation") != node.end())
                tfnode->mTransform.mRotation = math::Matrix4(math::Quat(GetVector(node["rotation"].get<json::array_t>())), math::Vector3(0.f, 0.f, 0.f));
            else if (node.find("matrix") != node.end())
                tfnode->mTransform.mRotation = GetMatrix(node["matrix"].get<json::array_t>());
            else
                tfnode->mTransform.mRotation = math::Matrix4::identity();
        }
    }

    // Load scenes
    //
    if (j3.find("scenes") != j3.end())
    {
        const json &scenes = j3["scenes"];
        mScenes.resize(scenes.size());
        for (int i = 0; i < scenes.size(); i++)
        {
            auto scene = scenes[i];
            for (int n = 0; n < scene["nodes"].size(); n++)
            {
                int nodeId = scene["nodes"][n];
                mScenes[i].mNodes.push_back(nodeId);
            }
        }
    }

    // Load skins
    //
    if (j3.find("skins") != j3.end())
    {
        const json &skins = j3["skins"];
        mSkins.resize(skins.size());
        for (uint32_t i = 0; i < skins.size(); i++)
        {
            GetBufferDetails(skins[i]["inverseBindMatrices"].get<int>(), &mSkins[i].mInverseBindMatrices);

            if (skins[i].find("skeleton") != skins[i].end())
                mSkins[i].m_pSkeleton = &mNodes[skins[i]["skeleton"]];

            const json &joints = skins[i]["joints"];
            for (uint32_t n = 0; n < joints.size(); n++)
            {
                mSkins[i].mJointsNodeIdx.push_back(joints[n]);
            }

        }
    }

    // Load animations
    //
    if (j3.find("animations") != j3.end())
    {
        const json &animations = j3["animations"];
        mAnimations.resize(animations.size());
        for (int i = 0; i < animations.size(); i++)
        {
            const json &channels = animations[i]["channels"];
            const json &samplers = animations[i]["samplers"];

            gltfAnimation *tfanim = &mAnimations[i];
            for (int c = 0; c < channels.size(); c++)
            {
                json::object_t channel = channels[c];
                int sampler = channel["sampler"];
                int node = GetElementInt(channel, "target/node", -1);
                std::string path = GetElementString(channel, "target/path", std::string());

                gltfChannel *tfchannel;

                auto ch = tfanim->mChannels.find(node);
                if (ch == tfanim->mChannels.end())
                {
                    tfchannel = &tfanim->mChannels[node];
                }
                else
                {
                    tfchannel = &ch->second;
                }

                gltfSampler *tfsmp = new gltfSampler();

                // Get timeline
                //
                GetBufferDetails(samplers[sampler]["input"], &tfsmp->mTime);
                assert(tfsmp->mTime.mStride == 4);

                tfanim->mDuration = std::max<float>(tfanim->mDuration, *(float*)tfsmp->mTime.Get(tfsmp->mTime.mCount - 1));

                // Get value line
                //
                GetBufferDetails(samplers[sampler]["output"], &tfsmp->mValue);

                // Index appropriately
                // 
                if (path == "translation")
                {
                    tfchannel->m_pTranslation = tfsmp;
                    assert(tfsmp->mValue.mStride == 3 * 4);
                    assert(tfsmp->mValue.mDimension == 3);
                }
                else if (path == "rotation")
                {
                    tfchannel->m_pRotation = tfsmp;
                    assert(tfsmp->mValue.mStride == 4 * 4);
                    assert(tfsmp->mValue.mDimension == 4);
                }
                else if (path == "scale")
                {
                    tfchannel->m_pScale = tfsmp;
                    assert(tfsmp->mValue.mStride == 3 * 4);
                    assert(tfsmp->mValue.mDimension == 3);
                }
            }
        }
    }

    InitTransformedData();

    return true;
}

void GLTFCommon::Unload()
{
    for (int i = 0; i < mBuffersData.size(); i++)
    {
        delete[] mBuffersData[i];
    }
    mBuffersData.clear();

    mAnimations.clear();
    mNodes.clear();
    mScenes.clear();
    mLights.clear();
    mLightInstances.clear();

    j3.clear();
}

//
// Animates the matrices (they are still in object space)
//
void GLTFCommon::SetAnimationTime(uint32_t animationIndex, float time)
{
    if (animationIndex < mAnimations.size())
    {
        gltfAnimation *anim = &mAnimations[animationIndex];

        //loop animation
        time = fmod(time, anim->mDuration);

        for (auto it = anim->mChannels.begin(); it != anim->mChannels.end(); it++)
        {
            Transform *pSourceTrans = &mNodes[it->first].mTransform;
            Transform animated;

            float frac, *pCurr, *pNext;

            // Animate translation
            //
            if (it->second.m_pTranslation != nullptr)
            {
                it->second.m_pTranslation->SampleLinear(time, &frac, &pCurr, &pNext);
                animated.mTranslation = ((1.0f - frac) * math::Vector4(pCurr[0], pCurr[1], pCurr[2], 0)) + ((frac) * math::Vector4(pNext[0], pNext[1], pNext[2], 0));
            }
            else
            {
                animated.mTranslation = pSourceTrans->mTranslation;
            }

            // Animate rotation
            //
            if (it->second.m_pRotation != nullptr)
            {
                it->second.m_pRotation->SampleLinear(time, &frac, &pCurr, &pNext);
                animated.mRotation = math::Matrix4(math::slerp(frac, math::Quat(pCurr[0], pCurr[1], pCurr[2], pCurr[3]), math::Quat(pNext[0], pNext[1], pNext[2], pNext[3])), math::Vector3(0.0f, 0.0f, 0.0f));
            }
            else
            {
                animated.mRotation = pSourceTrans->mRotation;
            }

            // Animate scale
            //
            if (it->second.m_pScale != nullptr)
            {
                it->second.m_pScale->SampleLinear(time, &frac, &pCurr, &pNext);
                animated.mScale = ((1.0f - frac) * math::Vector4(pCurr[0], pCurr[1], pCurr[2], 0)) + ((frac) * math::Vector4(pNext[0], pNext[1], pNext[2], 0));
            }
            else
            {
                animated.mScale = pSourceTrans->mScale;
            }

            mAnimatedMats[it->first] = animated.GetWorldMat();
        }
    }
}

void GLTFCommon::GetBufferDetails(int accessor, gltfAccessor *pAccessor) const
{
    const json &inAccessor = m_pAccessors->at(accessor);

    int32_t bufferViewIdx = inAccessor.value("bufferView", -1);
    assert(bufferViewIdx >= 0);
    const json &bufferView = m_pBufferViews->at(bufferViewIdx);

    int32_t bufferIdx = bufferView.value("buffer", -1);
    assert(bufferIdx >= 0);

    char *buffer = mBuffersData[bufferIdx];

    int32_t offset = bufferView.value("byteOffset", 0);

    int byteLength = bufferView["byteLength"];

    int32_t byteOffset = inAccessor.value("byteOffset", 0);
    offset += byteOffset;
    byteLength -= byteOffset;

    pAccessor->mData = &buffer[offset];
    pAccessor->mDimension = GetDimensions(inAccessor["type"]);
    pAccessor->mType = GetFormatSize(inAccessor["componentType"]);
    pAccessor->mStride = pAccessor->mDimension * pAccessor->mType;
    pAccessor->mCount = inAccessor["count"];
}

void GLTFCommon::GetAttributesAccessors(const json &gltfAttributes, std::vector<char*> *pStreamNames, std::vector<gltfAccessor> *pAccessors) const
{
    int streamIndex = 0;
    for (int s = 0; s < pStreamNames->size(); s++)
    {
        auto attr = gltfAttributes.find(pStreamNames->at(s));
        if (attr != gltfAttributes.end())
        {
            gltfAccessor accessor;
            GetBufferDetails(attr.value(), &accessor);
            pAccessors->push_back(accessor);
        }
    }
}

//
// Given a mesh find the skin it belongs to
//
int GLTFCommon::FindMeshSkinId(uint32_t meshId) const
{
    for (int i = 0; i < mNodes.size(); i++)
    {
        if (mNodes[i].meshIndex == meshId)
        {
            return mNodes[i].skinIndex;
        }
    }

    return -1;
}

//
// given a skinId return the size of the skeleton matrices (vulkan needs this to compute the offsets into the uniform buffers)
//
int GLTFCommon::GetInverseBindMatricesBufferSizeByID(int id) const
{
    if (id == -1 || (id >= mSkins.size()))
        return -1;

    return mSkins[id].mInverseBindMatrices.mCount * (4 * 4 * sizeof(float));
}

//
// Transforms a node hierarchy recursively 
//
void GLTFCommon::TransformNodes(const math::Matrix4& world, const std::vector<gltfNodeIdx> *pNodes)
{
    for (uint32_t n = 0; n < pNodes->size(); n++)
    {
        uint32_t nodeIdx = pNodes->at(n);

        math::Matrix4 m = world * mAnimatedMats[nodeIdx];
        mWorldSpaceMats[nodeIdx].Set(m);
        TransformNodes(m, &mNodes[nodeIdx].mChildren);
    }
}

//
// Initializes the GLTFCommonTransformed structure 
//
void GLTFCommon::InitTransformedData()
{
    // initializes matrix buffers to have the same dimension as the nodes
    mWorldSpaceMats.resize(mNodes.size());

    // same thing for the skinning matrices but using the size of the InverseBindMatrices
    for (uint32_t i = 0; i < mSkins.size(); i++)
    {
        mWorldSpaceSkeletonMats[i].resize(mSkins[i].mInverseBindMatrices.mCount);
    }

    // sets the animated data to the default values of the nodes
    // later on these values can be updated by the SetAnimationTime function
    mAnimatedMats.resize(mNodes.size());
    for (uint32_t i = 0; i < mNodes.size(); i++)
    {
        mAnimatedMats[i] = mNodes[i].mTransform.GetWorldMat();
    }
}

//
// Takes the animated matrices and processes the hierarchy, also computes the skinning matrix buffers. 
//
void GLTFCommon::TransformScene(int sceneIndex, const math::Matrix4& world)
{
    mWorldSpaceMats.resize(mNodes.size());

    // transform all the nodes of the scene (and make 
    //           
    std::vector<int> sceneNodes = { mScenes[sceneIndex].mNodes };
    TransformNodes(world, &sceneNodes);

    //process skeletons, takes the skinning matrices from the scene and puts them into a buffer that the vertex shader will consume
    //
    for (uint32_t i = 0; i < mSkins.size(); i++)
    {
        gltfSkins &skin = mSkins[i];

        //pick the matrices that affect the skin and multiply by the inverse of the bind      
        math::Matrix4* pM = (math::Matrix4*)skin.mInverseBindMatrices.mData;

        std::vector<Matrix2> &skinningMats = mWorldSpaceSkeletonMats[i];
        for (int j = 0; j < skin.mInverseBindMatrices.mCount; j++)
        {
            skinningMats[j].Set(mWorldSpaceMats[skin.mJointsNodeIdx[j]].GetCurrent() * pM[j]);
        }
    }
}

bool GLTFCommon::GetCamera(uint32_t cameraIdx, Camera *pCam) const
{
    if (cameraIdx < 0 || cameraIdx >= mCameras.size())
    {
        return false;
    }

    pCam->SetMatrix(mWorldSpaceMats[mCameras[cameraIdx].mNodeIndex].GetCurrent());
    pCam->SetFov(mCameras[cameraIdx].yFov, pCam->GetAspectRatio(), mCameras[cameraIdx].zNear, mCameras[cameraIdx].zFar);

    return true;
}

//
// Sets the per frame data from the GLTF, returns a pointer to it in case the user wants to override some values
// The scene needs to be animated and transformed before we can set the PerFrame data. We need those final matrices for the lights and the camera.
//
PerFrame* GLTFCommon::SetPerFrameData(const Camera& cam)
{
    Matrix2* pMats = mWorldSpaceMats.data();

    //Sets the camera
    mPerFrameData.mCameraCurrViewProj = cam.GetProjection() * cam.GetView();
    mPerFrameData.mCameraPrevViewProj = cam.GetProjection() * cam.GetPrevView();
    // more accurate calculation
    mPerFrameData.mInverseCameraCurrViewProj = math::affineInverse(cam.GetView()) * math::inverse(cam.GetProjection());
    mPerFrameData.mCameraPos = cam.GetPosition();

    mPerFrameData.mWireframeOptions = math::Vector4(0.0f, 0.0f, 0.0f, 0.0f);

    // Process lights
    mPerFrameData.mLightCount = (int32_t)mLightInstances.size();
    int32_t ShadowMapIndex = 0;
    for (int i = 0; i < mLightInstances.size(); i++)
    {
        Light* pSL = &mPerFrameData.mLights[i];

        // get light data and node trans
        const gltfLight &lightData = mLights[mLightInstances[i].mLightId];
        math::Matrix4 lightMat = pMats[mLightInstances[i].mNodeIndex].GetCurrent();

        math::Matrix4 lightView = math::affineInverse(lightMat);
        pSL->mLightView = lightView;
        if (lightData.mType == LightType_Spot)
            pSL->mLightViewProj = math::Matrix4::perspective(lightData.mOuterConeAngle * 2.0f, 1, .1f, 100.0f) * lightView;
        else if (lightData.mType == LightType_Directional)
            pSL->mLightViewProj = ComputeDirectionalLightOrthographicMatrix(lightView) * lightView;

        GetXYZ(pSL->direction, math::transpose(lightView) * math::Vector4(0.0f, 0.0f, 1.0f, 0.0f));
        GetXYZ(pSL->color, lightData.mColor);
        pSL->range = lightData.mRange;
        pSL->intensity = lightData.mIntensity;
        GetXYZ(pSL->position, lightMat.getCol3());
        pSL->outerConeCos = cosf(lightData.mOuterConeAngle);
        pSL->innerConeCos = cosf(lightData.mInnerConeAngle);
        pSL->type = lightData.mType;

        // Setup shadow information for light (if it has any)
        if (lightData.mShadowResolution && lightData.mType != LightType_Point)
        {
            pSL->shadowMapIndex = ShadowMapIndex++;
            pSL->depthBias = lightData.mBias;
        }
        else
            pSL->shadowMapIndex = -1;
    }

    return &mPerFrameData;
}


gltfNodeIdx GLTFCommon::AddNode(const gltfNode& node)
{
    mNodes.push_back(node);
    gltfNodeIdx idx = (gltfNodeIdx)(mNodes.size() - 1);
    mScenes[0].mNodes.push_back(idx);

    mAnimatedMats.push_back(node.mTransform.GetWorldMat());

    return idx;
}

int GLTFCommon::AddLight(const gltfNode& node, const gltfLight& light)
{
    int nodeID = AddNode(node);
    mLights.push_back(light);

    int lightInstanceID = (int)(mLights.size() - 1);
    mLightInstances.push_back({ lightInstanceID, (gltfNodeIdx)nodeID});

    return lightInstanceID;
}

//
// Computes the orthographic matrix for a directional light in order to cover the whole scene
//
math::Matrix4 GLTFCommon::ComputeDirectionalLightOrthographicMatrix(const math::Matrix4& mLightView) {

    AxisAlignedBoundingBox projectedBoundingBox;

    // NOTE: we consider all objects here, but the scene might not display all of them.
    for (uint32_t i = 0; i < mNodes.size(); ++i)
    {
        gltfNode* pNode = &mNodes.at(i);
        if ((pNode == nullptr) || (pNode->meshIndex < 0))
            continue;

        std::vector<gltfPrimitives>& primitives = mMeshes[pNode->meshIndex].m_pPrimitives;
        // loop through primitives
        //
        for (size_t p = 0; p < primitives.size(); ++p)
        {
            gltfPrimitives boundingBox = mMeshes[pNode->meshIndex].m_pPrimitives[p];
            projectedBoundingBox.Merge(GetAABBInGivenSpace(mLightView * mWorldSpaceMats[i].GetCurrent(), boundingBox.mCenter, boundingBox.mRadius));
        }
    }

    if (projectedBoundingBox.HasNoVolume())
    {
        // default ortho matrix that won't work in most cases
        return math::Matrix4::orthographic(-20.0, 20.0, -20.0, 20.0, 0.1f, 100.0f);
    }

    // we now have the final bounding box
    gltfPrimitives finalBoundingBox;
    finalBoundingBox.mCenter = 0.5f * (projectedBoundingBox.m_max + projectedBoundingBox.m_min);
    finalBoundingBox.mRadius = 0.5f * (projectedBoundingBox.m_max - projectedBoundingBox.m_min);

    finalBoundingBox.mCenter.setW(1.0f);
    finalBoundingBox.mRadius.setW(0.0f);

    // we want a square aspect ratio
    float spanX = finalBoundingBox.mRadius.getX();
    float spanY = finalBoundingBox.mRadius.getY();
    float maxSpan = spanX > spanY ? spanX : spanY;

    // manually create the orthographic matrix
    math::Matrix4 projectionMatrix = math::Matrix4::identity();
    projectionMatrix.setCol0(math::Vector4(
        1.0f / maxSpan, 0.0f, 0.0f, 0.0f
    ));
    projectionMatrix.setCol1(math::Vector4(
        0.0f, 1.0f / maxSpan, 0.0f, 0.0f
    ));
    projectionMatrix.setCol2(math::Vector4(
        0.0f, 0.0f, -0.5f / finalBoundingBox.mRadius.getZ(), 0.0f
    ));
    projectionMatrix.setCol3(math::Vector4(
        -finalBoundingBox.mCenter.getX() / maxSpan,
        -finalBoundingBox.mCenter.getY() / maxSpan,
        0.5f * (finalBoundingBox.mCenter.getZ() + finalBoundingBox.mRadius.getZ()) / finalBoundingBox.mRadius.getZ(),
        1.0f
    ));
    return projectionMatrix;
}
