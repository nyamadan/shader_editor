#if GL_ES
#define HALF mediump
#else
#define HALF
#endif

attribute vec3 aPosition;
varying vec2 surfacePosition;

uniform HALF vec2 resolution;

void main(void) {
    surfacePosition =
        0.5 * aPosition.xy * vec2((resolution.x / resolution.y), 1.0);
    gl_Position = vec4(aPosition, 1.0);
}
