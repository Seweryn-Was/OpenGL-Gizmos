#version 330 core

in vec2 TexCoord;
in vec4 n;
in vec4 l;
in vec4 v;

out vec4 FragColor;

uniform sampler2D myTexture;
uniform vec3 lightColor;

void main() {

    vec3 normal = normalize(n.xyz);
    vec3 lightDir = normalize(l.xyz);
    vec3 viewDir = normalize(v.xyz);
    vec3 reflectDir = reflect(-lightDir, normal);

    // Surface parameters
    vec3 kd = texture(myTexture, TexCoord).rgb;
    vec3 ks = vec3(0.0, 0.0, 0.0); // White specular
    float shininess = 10.0;

    // Lighting
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor * kd;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * kd;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = spec * lightColor * ks;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
