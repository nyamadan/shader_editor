#version 310 es

#extension GL_GOOGLE_include_directive : enable

precision mediump float;

layout(location=0) out vec4 fragColor;

#include "./lib/color.glsl"

void main()
{
    fragColor = magenta();
}
