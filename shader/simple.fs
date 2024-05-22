
#version 330 core

in vec4 vertexColor;
in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D tex;
uniform sampler2D tex2;

void main() {
    // fragColor = vec4(1.0, 1.0, 1.0, 1.0);    // +4
    // fragColor = vertexColor;         // +4/2
    // fragColor = texture(tex, texCoord);     // +6
    fragColor = texture(tex, texCoord) * 0.9 + texture(tex2, texCoord) * 0.1;
}