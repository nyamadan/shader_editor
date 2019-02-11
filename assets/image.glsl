#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D iChannel0;
uniform sampler2D backbuffer;
uniform vec2 resolution;

void main(void) {
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    vec3 ch0 = texture2D(iChannel0, uv).rgb;
    vec3 prev = texture2D(backbuffer, uv).rgb;
    vec3 color = mix(prev, ch0, 0.1);
    gl_FragColor = vec4(color, 1.0);
}
