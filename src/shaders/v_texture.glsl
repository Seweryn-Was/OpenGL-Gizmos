#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aBoneID;
layout (location = 4) in vec4 aBoneWeight;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform mat4 uBoneMatrices[100];

out vec2 TexCoord;
out vec4 l;
out vec4 n;
out vec4 v;

void main() {
    // Skinning
    mat4 boneTransform = uBoneMatrices[int(aBoneID.x)];

    vec4 skinnedPos = vec4(0.0);
    for (int i = 0; i < 4; i++) {
        if(aBoneWeight[i] == 0.0){ 
            break; 
        }
        skinnedPos += aBoneWeight[i] * (uBoneMatrices[int(aBoneID[i])] * vec4(aPos, 1.0));
    }


    vec4 worldPos = M * skinnedPos; //* boneTransform * vec4(aPos, 1.0);
    gl_Position = P * V * worldPos;

    // Light calculations
    vec4 lightPos = vec4(0, 0, 0, 1); // World space light position
    l = normalize(V * lightPos - V * worldPos); // Light vector in view space
    v = normalize(vec4(0, 0, 0, 1) - V * worldPos); // View vector in view space

    mat3 normalMatrix = transpose(inverse(mat3(V * M * mat4(mat3(boneTransform)))));
    n = vec4(normalMatrix * aNormal, 0.0); // Correct normal transform

    TexCoord = aTexCoord;
}
