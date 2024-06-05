
// Point light

#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 transform;
uniform mat4 modelTransform;

out vec3 normal;
out vec2 texCoord;
out vec3 position;

void main() {
    gl_Position = transform * vec4(aPos, 1.0);
    normal = (transpose(inverse(modelTransform)) * vec4(aNormal, 0.0)).xyz;
    texCoord = aTexCoord;
    position = (modelTransform * vec4(aPos, 1.0)).xyz;
}


#shader fragment
#version 330 core

in vec3 normal;
in vec2 texCoord;
in vec3 position;

out vec4 fragColor;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

uniform float ambientStrength;
uniform float specularStrength;
uniform float specularShiniess;

struct Light {
    vec3 position;
    vec3 attenuation;
    vec3 direction;
    // float cutoff;
    vec2 cutoff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;

struct Material {
    vec3 ambient;

    sampler2D diffuse;
    sampler2D specular;

    float shininess;
};
uniform Material material;

void main() {
 
    vec3 texColor = texture2D(material.diffuse, texCoord).xyz;
    vec3 ambient = texColor * light.ambient;

    // vec3 lightDir = normalize(light.direction - position);
    // vec3 lightDir = normalize(-light.direction);

    float dist = length(light.position - position);
    vec3 distPoly = vec3(1.0, dist, dist*dist);
    float attenuation = 1.0 / dot(distPoly, light.attenuation);
    vec3 lightDir = (light.position - position) / dist;

    // vec3 pixelNorm = normalize(normal);
    // float diff = max(dot(pixelNorm, lightDir), 0.0);
    // vec3 diffuse = diff * texColor * light.diffuse;

    // vec3 specColor = texture2D(material.specular, texCoord).xyz;
    // vec3 viewDir = normalize(viewPos - position);
    // vec3 reflectDir = reflect(-lightDir, pixelNorm);
    // float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // vec3 specular = spec * specColor * light.specular;

    // vec3 result = ambient + diffuse + specular ;
    vec3 result = ambient;

    float theta = dot(lightDir, normalize(-light.direction));
    float intensity = clamp(
        (theta - light.cutoff[1]) / (light.cutoff[0] - light.cutoff[1]),
        0.0, 1.0);

    // if (theta > light.cutoff) {
    if (intensity > 0.0) {
        vec3 pixelNorm = normalize(normal);
        float diff = max(dot(pixelNorm, lightDir), 0.0);
        vec3 diffuse = diff * texColor * light.diffuse;

        vec3 specColor = texture2D(material.specular, texCoord).xyz;
        vec3 viewDir = normalize(viewPos - position);
        vec3 reflectDir = reflect(-lightDir, pixelNorm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = spec * specColor * light.specular;

        // result += (diffuse + specular) ;
        result += (diffuse + specular) * intensity;
    }

    result *= attenuation;

    fragColor = vec4(result, 1.0);
}