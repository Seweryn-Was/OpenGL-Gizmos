#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aBoneID;
layout (location = 4) in vec4 aBoneWeight;

uniform vec3 lightPos;
uniform vec3 lightPos2;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform mat4 uBoneMatrices[100];

out vec2 TexCoord;
out vec4 l;
out vec4 l2;
out vec4 n;
out vec4 v;

void main() {
    vec4 totalPosition = vec4(0.0f);
    mat4 boneTransform = mat4(1.0);

    for(int i = 0 ; i < 3 ; i++)
    {
        if(int(aBoneID[i]) == -1) 
            continue;
        if(int(aBoneID[i]) >=100) 
        {
            totalPosition = vec4(aPos,1.0f);
            break;
        }
        vec4 localPosition = uBoneMatrices[int(aBoneID[i])] * vec4(aPos,1.0f);
        totalPosition += localPosition * aBoneWeight[i];

        boneTransform += aBoneWeight[i] * uBoneMatrices[int(aBoneID[i])]; 
    }

    vec4 worldPos = M * totalPosition; 
    gl_Position = P * V * worldPos;

    // Light calculations
    
    l = normalize(V * vec4(lightPos, 1.0) - V * worldPos); // Light vector in view space
    l2 = normalize(V * vec4(lightPos2, 1.0) - V * worldPos); // Light vector in view space

    v = normalize(vec4(0, 0, 0, 1) - V * worldPos); // View vector in view space

    mat3 normalMatrix = transpose(inverse(mat3(V * M * mat4(mat3(boneTransform)))));
    n = vec4(normalMatrix * aNormal, 0.0); // Correct normal transform

    TexCoord = aTexCoord;
}
