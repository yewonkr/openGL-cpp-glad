#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 transform;

out vec4 vertexColor;
out vec2 texCoord;

void main() {
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
//    gl_Position = vec4(aPos, 1.0) * transform;
    vertexColor = vec4(aColor, 1.0);
    texCoord = aTexCoord;
}


#shader fragment
#version 330 core

in vec4 vertexColor;
in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D tex;
uniform sampler2D tex2;

void main() {
    fragColor = vec4(1.0, 1.0, 1.0, 1.0);    // +4
    // fragColor = vertexColor;         // +4/2
    // fragColor = texture(tex, texCoord);     // +6
    // fragColor = texture(tex, texCoord) * 0.9 + texture(tex2, texCoord) * 0.1;
}

