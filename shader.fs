#version 100
precision mediump float;

varying vec4 vColor;
varying vec2 vUV;
varying float vFogFactor;

uniform bool uUseFog;
uniform vec4 uFogColor;
uniform sampler2D uTexture;

void main() {
    vec4 texColor = texture2D(uTexture, vUV);
    vec4 finalColor = texColor * vColor;

    if(uUseFog) {
        finalColor = mix(uFogColor, finalColor, vFogFactor);
    }

    gl_FragColor = finalColor;
}
