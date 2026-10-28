#version 330 core

in vec2 UV;
in vec3 Normal;
in vec3 FragPos;
in vec3 Tangent;
in vec3 Bitangent;

out vec3 color;

uniform sampler2D textureSampler;
uniform sampler2D heightMapSampler;
float bumpScale = 1.5;

// Material
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float ns;

// Licht
uniform vec3 lpos;
uniform vec3 la;
uniform vec3 ld;
uniform vec3 ls;

uniform vec3 camPos; // camera position in world space

// Bereken de aangepaste normal op basis van heightmap
vec3 getBumpNormal(vec3 normal, vec2 uv, sampler2D texHeight, float scale)
{
    // Bepaal de grootte van pixel
    vec2 texelSize = 1.0 / textureSize(texHeight, 0);

    // Sample omliggende hoogtes door te interpoleren
    float heightL = texture(texHeight, uv - vec2(texelSize.x, 0.0)).r;
    float heightR = texture(texHeight, uv + vec2(texelSize.x, 0.0)).r;
    float heightD = texture(texHeight, uv - vec2(0.0, texelSize.y)).r;
    float heightU = texture(texHeight, uv + vec2(0.0, texelSize.y)).r;

    // Bereken de gradients
    float dHeightU = (heightR - heightL) * scale;
    float dHeightV = (heightU - heightD) * scale;

    vec3 tangent = Tangent;
    vec3 bitangent = Bitangent;

    tangent -= normal * dot(normal, tangent);
    float tangentLength = length(tangent);

    if (tangentLength > 0.000001)
    {
        tangent /= tangentLength;

        vec3 B = cross(normal, tangent);

        if (dot(B, bitangent) < 0.0)
        {
            B = -B;
        }

        bitangent = B;
    }
    else
    {
        bitangent = normalize(bitangent);
    }

    vec3 pixelHeight = normalize(vec3(
        dHeightU,
        dHeightV,
        1.0
    ));

    mat3 TBN = mat3(
        normalize(tangent),
        normalize(bitangent),
        normalize(normal)
    );

    return normalize(TBN * pixelHeight);
}

void main(){
    vec3 texColor = texture(textureSampler, UV).rgb;
    vec3 changedNormal = getBumpNormal(normalize(Normal), UV, heightMapSampler, bumpScale); // Debug: changedNormal = normalize(Normal);

    vec3 n = changedNormal; // normaalvector uit bump mapping
    vec3 v = normalize(camPos - FragPos); // vector van object naar camera
    vec3 l = normalize(lpos - FragPos); // vector van object naar lichtbron
    vec3 r = reflect(-l, n); // reflectie

    vec3 ia = ka * la;
    vec3 id = kd * ld * max(0.0, dot(n, l));
    
    float spec = 0.0;
    if (dot(n, l) > 0.0) {
    spec = pow(max(0.0, dot(r, v)), ns);
    }

    vec3 is = ks * ls * spec;
    
    vec3 ambient = texColor * ia;
    vec3 diffuse = texColor * id;
    vec3 specular = is;

    color = ambient + diffuse + specular;
}