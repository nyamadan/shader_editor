#version 310 es

precision highp float;

struct HitResult {
    mediump int m;
    float d;
};

layout(location = 0) uniform vec2 resolution;
layout(location = 1) uniform mat4 mat_mv_t;
layout(location = 2) uniform mat4 mat_mv_it;

layout(location = 0) out vec4 fragColor;

const int MATERIAL_EMPTY = 0;
const int MATERIAL_DIFFUSE = 1;

float sdBox(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

HitResult opU(HitResult r1, HitResult r2) { return (r1.d < r2.d) ? r1 : r2; }

HitResult map(in vec3 pos) {
    HitResult res = HitResult(MATERIAL_EMPTY, 1e+10);
    res = opU(res, HitResult(MATERIAL_DIFFUSE, sdBox(pos, vec3(0.5))));
    return res;
}

vec3 getRay(vec2 p, mat3 m, float fov) {
    float f = 1.0 / tan(radians(0.5 * fov));
    return normalize(m * vec3(p.x, p.y, -f));
}

// http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
vec3 calcNormal(in vec3 pos) {
    vec2 e = vec2(1.0, -1.0) * 0.5773 * 0.0005;
    return normalize(e.xyy * map(pos + e.xyy).d + e.yyx * map(pos + e.yyx).d +
                     e.yxy * map(pos + e.yxy).d + e.xxx * map(pos + e.xxx).d);
}

void main() {
    const int MAX_STEP = 100;
    const float EPS = 1e-6;
    const float FOV = 30.0;

    vec2 p = (2.0 * gl_FragCoord.xy - resolution.xy) /
             min(resolution.x, resolution.y);

    vec3 ro = vec3(mat_mv_it[0].w, mat_mv_it[1].w, mat_mv_it[2].w);
    vec3 rd = getRay(p, mat3(mat_mv_t), FOV);
    vec3 color = vec3(0.0);

    float depth = 0.0;
    for (int i = 1; i <= MAX_STEP; i++) {
        vec3 pos = depth * rd + ro;

        HitResult res = map(pos);

        if (res.d >= EPS) {
            depth += res.d;
            continue;
        }

        if (res.m == MATERIAL_DIFFUSE) {
            vec3 nor = calcNormal(pos);
            vec3 lightDir = normalize(vec3(1.0, 2.0, 3.0));
            float diff = max(dot(lightDir, nor), 0.0);
            color = vec3(0.5 * diff * diff + 0.5);
            break;
        }
    }

    fragColor = vec4(color, 1.0);
}
