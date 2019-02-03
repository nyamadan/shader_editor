#ifdef GL_ES
precision mediump float;
#endif

uniform float iTime;
uniform vec2 iMouse;
uniform vec2 iResolution;

void mainImage(out vec4 fragColor, in vec2 fragCoord);

void main() { mainImage(gl_FragColor, gl_FragCoord.xy); }
