#pragma once

#include "vectormath/vectormath.hpp"

enum ColorSpace
{
    ColorSpace_REC709,
    ColorSpace_P3,
    ColorSpace_REC2020,
    ColorSpace_Display
};

enum ColorPrimaries
{
    ColorPrimaries_WHITE,
    ColorPrimaries_RED,
    ColorPrimaries_GREEN,
    ColorPrimaries_BLUE
};

enum ColorPrimariesCoordinates
{
    ColorPrimariesCoordinates_X,
    ColorPrimariesCoordinates_Y
};

extern float ColorSpacePrimaries[4][4][2];

void FillDisplaySpecificPrimaries(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb);

math::Matrix4 CalculateRGBToXYZMatrix(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, bool scaleLumaFlag);
math::Matrix4 CalculateXYZToRGBMatrix(float xw, float yw, float xr, float yr, float xg, float yg, float xb, float yb, bool scaleLumaFlag);

void SetupGamutMapperMatrices(ColorSpace gamutIn, ColorSpace gamutOut, math::Matrix4* inputToOutputRecMatrix);