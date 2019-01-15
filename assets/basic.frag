precision mediump float;

uniform float time;
uniform vec2 resolution;
uniform vec2 mouse;

out vec4 fragColor;

void main(void) {
  vec2 p = gl_FragCoord.xy / resolution;

  vec3 rgb = vec3(p, 0.0);

  fragColor = vec4(rgb, 1.0);
}
