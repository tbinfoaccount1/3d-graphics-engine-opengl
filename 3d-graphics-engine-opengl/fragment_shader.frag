#version 330 core

in vec2 UV;
in vec3 Normal;
in vec3 FragPos;

out vec3 color;

uniform sampler2D textureSampler;

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

void main(){
    vec3 texColor = texture(textureSampler, UV).rgb;

    vec3 n = normalize(Normal); // normaalvector
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