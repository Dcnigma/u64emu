#version 100
attribute vec3 aPosition;
attribute vec3 aNormal;
attribute vec2 aUV;

uniform mat4 uWorld;
uniform mat4 uView;
uniform mat4 uProj;

uniform vec4 uAmbientColor;
uniform vec3 uLightDir;
uniform vec4 uLightColor;

uniform bool uUseFog;
uniform vec4 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

varying vec4 vColor;
varying vec2 vUV;
varying float vFogFactor;

void main() {
    vec4 worldPos = uWorld * vec4(aPosition, 1.0);
    vec3 worldNormal = normalize(mat3(uWorld) * aNormal);

    // Lighting (ambient + directional)
    float diffuse = max(dot(worldNormal, -uLightDir), 0.0);
    vColor = uAmbientColor + uLightColor * diffuse;

    gl_Position = uProj * uView * worldPos;
    vUV = aUV;

    // Fog
    if(uUseFog) {
        float dist = length(worldPos.xyz);
        vFogFactor = clamp((uFogEnd - dist) / (uFogEnd - uFogStart), 0.0, 1.0);
    } else {
        vFogFactor = 1.0;
    }
}
