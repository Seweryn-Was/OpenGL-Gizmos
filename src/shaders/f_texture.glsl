#version 330 core

in vec2 TexCoord;
in vec4 n;
in vec4 l;
in vec4 l2;
in vec4 v;

out vec4 FragColor;

uniform sampler2D myTexture;
uniform vec3 lightColor;
uniform vec3 lightColor2;

void main() {

   vec3 normal = normalize(n.xyz);
    vec3 viewDir = normalize(v.xyz);

    // Surface parameters
    vec3 kd = texture(myTexture, TexCoord).rgb;
    vec3 ks = vec3(0.0);
    float shininess = 10.0;
    float ambientStrength = 0.1;

    // --- Light 1 ---
    vec3 lightDir1 = normalize(l.xyz);
    vec3 reflectDir1 = reflect(-lightDir1, normal);

    vec3 ambient1 = ambientStrength * lightColor * kd;
    float diff1 = max(dot(normal, lightDir1), 0.0);
    vec3 diffuse1 = diff1 * lightColor * kd;
    float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), shininess);
    vec3 specular1 = spec1 * lightColor * ks;

    // --- Light 2 ---
    vec3 lightDir2 = normalize(l2.xyz);
    vec3 reflectDir2 = reflect(-lightDir2, normal);

    vec3 ambient2 = ambientStrength * lightColor2 * kd;
    float diff2 = max(dot(normal, lightDir2), 0.0);
    vec3 diffuse2 = diff2 * lightColor2 * kd;
    float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), shininess);
    vec3 specular2 = spec2 * lightColor2 * ks;

    vec3 result = (ambient1 + diffuse1 + specular1) + (ambient2 + diffuse2 + specular2);
    FragColor = vec4(result, 1.0);
}
