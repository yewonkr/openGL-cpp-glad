#version 330 core

in vec3 normal;
in vec2 texCoord;

out vec4 fragColor;

uniform vec3 lightColor;
uniform vec3 objectColor;
uniform float ambientStrength;

uniform sampler2D tex;
uniform sampler2D tex2;

void main() {

    vec3 ambient = ambientStrength * lightColor;
    // vec3 result = ambient * objectColor;
    vec3 result = objectColor;
    // fragColor = vec4(result, 1.0);
    fragColor = texture(tex, texCoord) * 0.1 + texture(tex2, texCoord) * 0.9;
}



