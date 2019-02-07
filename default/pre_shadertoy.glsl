#version 300 es

#ifdef GL_ES
precision highp float;
precision highp int;
precision mediump sampler3D;
#endif

uniform float iTime;
uniform int iFrame;
uniform vec4 iMouse;
uniform vec2 iResolution;

uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;

out vec4 outColor;

void mainImage(out vec4 fragColor, in vec2 fragCoord);

void main(void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    mainImage(color, gl_FragCoord.xy);
    color.a = 1.0;
    outColor = color;
}
