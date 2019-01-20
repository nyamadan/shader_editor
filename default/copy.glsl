#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D backbuffer;
uniform vec2 resolution;

void main(void) {
    vec2 uv = gl_FragCoord.xy / resolution;
    vec3 color = texture2D(backbuffer, uv).rgb;
    gl_FragColor = vec4(color, 1.0);
}
