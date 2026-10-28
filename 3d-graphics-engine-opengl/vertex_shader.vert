#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec3 vertexTangent;
layout(location = 4) in vec3 vertexBitangent;

out vec2 UV;
out vec3 Normal;
out vec3 FragPos; // fragment position in world space
out vec3 Tangent;
out vec3 Bitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    // Zet coordinaten om naar world space
    vec4 worldPos = model * vec4(vertexPosition, 1.0);
    FragPos = worldPos.xyz;
    UV = vertexUV;
    Tangent = normalize(mat3(model) * vertexTangent);
    Bitangent = normalize(mat3(model) * vertexBitangent);

    // Bereken normal met behulp van inverse transpose van model matrix
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * vertexNormal);

    // Positie is world space > view space > clip space
    gl_Position = projection * view * worldPos;
}