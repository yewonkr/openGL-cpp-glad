#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 transform;

out vec4 vertexColor;
out vec2 texCoord;

void main() {

    gl_Position = vec4(aPos, 1.0) * transform;
    vertexColor = vec4(aColor, 1.0);
    texCoord = aTexCoord;
}


#shader fragment
#version 330 core

in vec3 normal;
in vec2 texCoord;

out vec4 fragColor;

uniform vec3 lightColor;
uniform vec3 objectColor;
uniform float ambientStrength;

void main() {

    vec3 ambient = ambientStrength * lightColor;
    // vec3 result = ambient * objectColor;
    vec3 result = objectColor;
    fragColor = vec4(result, 1.0);
}