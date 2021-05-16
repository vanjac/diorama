#pragma once
#include "common.h"

const string VERSION_DIRECTIVE = "#version 330 core\n";

const string vertShaderSrc = R"X(
// defined in mesh.h
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aSTQ;

out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec3 vSTQ;

// std140: predictable layout
layout (std140) uniform TransformBlock
{
    // avoid using vec3!
    mat4 ModelMatrix;
    mat4 NormalMatrix;  // actually mat3
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;
};

uniform vec2 TextureScale;

void main()
{
    vWorldPosition = vec3(ModelMatrix * vec4(aPosition, 1));
    vWorldNormal = normalize(mat3(NormalMatrix) * aNormal);
    vSTQ = vec3(aSTQ.st * TextureScale, aSTQ.p);

    gl_Position = ProjectionMatrix * ViewMatrix * ModelMatrix * vec4(aPosition, 1.0);
}
)X";

const string fragShaderSrc = R"X(
in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec3 vSTQ;

out vec4 fColor;

uniform sampler2D BaseTexture;
uniform vec4 BaseColor;

#if defined(COLORIZE_SHIFT) || defined(COLORIZE_TINT)
    #define COLORIZE
#endif

#ifdef COLORIZE
// https://github.com/SketchUp/sketchup-colorize-algorithm/blob/master/cpp/colorize.cpp

const float EPSILON = 1.0e-3;

vec3 RGBToHSL(vec3 color)
{
    // Convert to HSL
    float max = max(max(color.r, color.g), color.b);
    float min = min(min(color.r, color.g), color.b);

    // Lightness
    float l = (max + min) / 2.0;

    // saturation and hue
    if (max == min) {
        // Achromatic
        return vec3(-1.0, 0.0, l);  // undefined hue
    } else {
        // Chromatic

        // Saturation
        float s = max - min;
        if (l <= 0.5) {
            s /= max + min;
        } else {
            s /= 2 - max - min;
        }

        // Hue
        vec3 c = (max - color) / (max - min);

        float h;
        if (max == color.r) {
            h = c.b - c.g;            // color between yellow and magenta
        } else if (max == color.g) {
            h = 2 + c.r - c.b;        // color between cyan and yellow
        } else if (max == color.b) {
            h = 4 + c.g - c.r;        // color between magenta and cyan
        }

        h = h * 60;
        if (h < 0) {
            h += 360;
        }
        return vec3(h, s, l);
    }
}

float CalcValue(float n1, float n2, float hue) {
    hue = mod(hue, 360);
    if (hue < 60) {
        return n1 + (n2 - n1) * hue / 60;
    } else if (hue < 180) {
        return n2;
    } else if (hue < 240) {
        return n1 + (n2 - n1) * (240 - hue) / 60.0;
    } else {
        return n1;
    }
}

vec3 HSLToRGB(vec3 hsl)
{
    float h = hsl.x;
    float s = hsl.y;
    float l = hsl.z;

    // Figure out the color
    float m2;
    if (l < 0.5) {
        m2 = l * (1.0 + s);
    } else {
        m2 = l + s - (l * s);
    }
    float m1 = 2 * l - m2;

    if (abs(s) < EPSILON) {
        // Achromatic
        return vec3(l);
    } else {
        // Chromatic
        return vec3(CalcValue(m1, m2, h + 120),
                    CalcValue(m1, m2, h),
                    CalcValue(m1, m2, h - 120));
    }
}

vec3 Colorize(vec3 color, vec3 delta)
{
    // Convert to HSL
    vec3 colorHSL = RGBToHSL(color);

#ifdef COLORIZE_TINT
    // Clamp Hue
    colorHSL.x = 0;
#endif
    // Shift Hue, Saturation, Luminance
    colorHSL += delta;
    colorHSL = vec3(
        mod(colorHSL.x, 360),
        clamp(colorHSL.y, 0.0, 1.0),
        clamp(colorHSL.z, 0.01, 1.0));

    // Convert back to rgb
    if (abs(colorHSL.y) < EPSILON) {
        return vec3(colorHSL.z);  // luminance to RGB
    } else {
        return HSLToRGB(colorHSL);
    }
}

#endif  // colorize

void main(void)
{    
#ifdef DEBUG_NORMALS
    fColor = vec4((vWorldNormal + 1) / 2, 1);
    return;
#endif

#ifdef DEBUG_GAMMA
    fColor = vec4(0.05, 0.05, 0.05, 1);  // should be easily visible
#endif

    float simpleLight = max(0, dot(vWorldNormal, normalize(vec3(0.5,-1,1))))
        * 0.7 + 0.3;
    vec4 color = vec4(simpleLight);
#ifndef COLORIZE
    color *= BaseColor;
#endif
#ifdef BASE_TEXTURE
    // perspective warping with homogeneous coordinates
    vec2 uv = vec2(vSTQ.s, -vSTQ.t) / vSTQ.p;
    color *= texture(BaseTexture, uv);
#endif
#ifdef CUTOUT
    if (color.a < 0.5)
        discard;
#endif
#ifdef COLORIZE
    color.rgb = Colorize(color.rgb, BaseColor.rgb);
#endif
    fColor = color;
}
)X";