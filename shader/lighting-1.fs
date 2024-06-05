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
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    // vec3 attenuation;
    vec3 direction;
    // // float cutoff;
    // vec2 cutoff;
};
uniform Light light;

struct Material {
    vec3 ambient;
    // vec3 diffuse;
    sampler2D diffuse;
    sampler2D specular;
    // vec3 specular;
    float shininess;
};
uniform Material material;

void main() {
   
    // vec3 ambient = ambientStrength * lightColor;
    // vec3 lightDir = normalize(lightPos - position);
    // vec3 pixelNorm = normalize(normal);
    // vec3 diffuse = (max(dot(pixelNorm, lightDir), 0.0) * lightColor);

    // // vec3 result = (ambient + diffuse) * objectColor ;

    // vec3 viewDir = normalize(viewPos - position);
    // vec3 reflectDir = reflect(-lightDir, pixelNorm);
    // float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularShiniess);
    // vec3 specular = spec * lightColor * specularStrength;

    // vec3 result = (ambient + diffuse + specular) * objectColor ; 

    // vec3 ambient = material.ambient * light.ambient;
    vec3 texColor = texture2D(material.diffuse, texCoord).xyz;
    vec3 ambient = texColor * light.ambient;

    vec3 lightDir = normalize(light.direction - position);
    // float dist = length(light.position - position);
    // vec3 distPoly = vec3(1.0, dist, dist*dist);
    // float attenuation = 1.0 / dot(distPoly, light.attenuation);
    // vec3 lightDir = (light.position - position) / dist;
    vec3 pixelNorm = normalize(normal);
    float diff = max(dot(pixelNorm, lightDir), 0.0);
    // vec3 diffuse = diff * material.diffuse * light.diffuse;
    vec3 diffuse = diff * texColor * light.diffuse;

    vec3 specColor = texture2D(material.specular, texCoord).xyz;
    vec3 viewDir = normalize(viewPos - position);
    vec3 reflectDir = reflect(-lightDir, pixelNorm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // vec3 specular = spec * material.specular * light.specular;
    vec3 specular = spec * specColor * light.specular;

    vec3 result = ambient + diffuse + specular ;
    // vec3 result = (ambient + diffuse + specular) * attenuation ;

    // vec3 result = ambient;
    // float theta = dot(lightDir, normalize(-light.direction));
    // float intensity = clamp(
    //     (theta - light.cutoff[1]) / (light.cutoff[0] - light.cutoff[1]),
    //     0.0, 1.0);

    // // if (theta > light.cutoff) {
    // if (intensity > 0.0) {
    //     vec3 pixelNorm = normalize(normal);
    //     float diff = max(dot(pixelNorm, lightDir), 0.0);
    //     vec3 diffuse = diff * texColor * light.diffuse;

    //     vec3 specColor = texture2D(material.specular, texCoord).xyz;
    //     vec3 viewDir = normalize(viewPos - position);
    //     vec3 reflectDir = reflect(-lightDir, pixelNorm);
    //     float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //     vec3 specular = spec * specColor * light.specular;

    //     // result += (diffuse + specular) ;
    //     result += (diffuse + specular) * intensity;
    // }

    // result *= attenuation;

    fragColor = vec4(result, 1.0);
}