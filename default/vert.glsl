#if GL_ES
#define half2 mediump vec2
#else
#define half2 vec2
#endif

attribute vec3 aPosition;
varying vec2 surfacePosition;

uniform half2 resolution;

void main(void) {
    surfacePosition =
        0.5 * aPosition.xy * vec2((resolution.x / resolution.y), 1.0);
    gl_Position = vec4(aPosition, 1.0);
}
