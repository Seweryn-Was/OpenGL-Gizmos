#include <iostream>
#include <map>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>


#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define GIZMOS_DEBUG

#include "shaderprogram.h"
#include "VertexArray.h"
#include "Base.h"
#include "openglUtil.h"
#include "Gizmo.h"
#include "Input.h"
#include "Mesh.h"

#include <stb_image.h>

#include "Texture2D.h"

#define PI 3.14159f

uint16_t gWindowWidth = 1200, gWindowHeight = 800;

glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
    glm::mat4 to(1.0f);

    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;

    return to;
}

struct Transform {
    glm::vec3 position; 
    glm::quat rotation; 
    glm::vec3 scale; 

    glm::mat4 GetTransMat() const{
        return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
    }
};

Transform DecomposeMatrix(const glm::mat4& mat) {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation;
    glm::decompose(mat, scale, rotation, translation, skew, perspective);
    Transform trans = {};
    trans.position = translation;
    trans.rotation = rotation;
    trans.scale = scale;

    return trans;
}

Gizmo::Ref<Gizmo::Skeleton> gSkeleton; 
std::vector<Gizmo::Ref<Gizmo::SkinnedMesh>> gMeshes; 
std::vector<std::string> gMeshesNames; 

void BuildNodeHierarchy(int32_t parent, const aiNode* node){

    std::string nodeName(node->mName.C_Str());

    glm::mat4 localTrans = aiMatrix4x4ToGlm(node->mTransformation);

    int nodeIndex = gSkeleton->addNode(nodeName, parent, localTrans); 

    for (int i = 0; i < node->mNumChildren; i++) {
        BuildNodeHierarchy(nodeIndex, node->mChildren[i]);
    }
}

void BuildHierarchy(const aiNode* node) {
    gSkeleton = Gizmo::CreateRef<Gizmo::Skeleton>();
    BuildNodeHierarchy(-1, node);
}

int boneCtr = 0; 

void ProcessAiMesh(const aiMesh* mesh) {
    std::vector<float> vertecies; 
    std::vector<uint32_t> indecies; 
    std::vector<Gizmo::Bone> bones; 

    const int stride = 16;
    const int boneIDOffset = 8, weightOffset = 12;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        glm::vec3 normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f);
        glm::vec2 texCoords = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);

        vertecies.resize(vertecies.size() + stride);

        const int vertIndex = i * stride; 
        vertecies[vertIndex + 0] = mesh->mVertices[i].x;
        vertecies[vertIndex + 1] = mesh->mVertices[i].y;
        vertecies[vertIndex + 2] = mesh->mVertices[i].z;

        vertecies[vertIndex + 3] = normal.x;
        vertecies[vertIndex + 4] = normal.y;
        vertecies[vertIndex + 5] = normal.z;

        vertecies[vertIndex + 6] = texCoords.x;
        vertecies[vertIndex + 7] = texCoords.y;

        //Bone Id; 
        vertecies[vertIndex + 8] = -1.0f;
        vertecies[vertIndex + 9] = -1.0f;
        vertecies[vertIndex + 10]= -1.0f;
        vertecies[vertIndex + 11]= -1.0f;

        //weights
        vertecies[vertIndex + 12] = 0.0f;
        vertecies[vertIndex + 13] = 0.0f;
        vertecies[vertIndex + 14] = 0.0f;
        vertecies[vertIndex + 15] = 0.0f;
    }

    //settting Indecies
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indecies.push_back(face.mIndices[j]);
        }
    }

    //Skinning
    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        aiBone* aibone = mesh->mBones[i];
        std::string boneName(aibone->mName.C_Str());

        uint32_t index = gSkeleton->getNodeIndex(boneName); 

        uint32_t boneIndex = gSkeleton->addBone(boneName, index, aiMatrix4x4ToGlm(aibone->mOffsetMatrix)); 

        for (unsigned int j = 0; j < aibone->mNumWeights; ++j) {
            const int vertexID = aibone->mWeights[j].mVertexId;
            const float weight = aibone->mWeights[j].mWeight;

            const int base = vertexID * stride;

            for (int k = 0; k < 4; ++k) {
                const int boneSlot = base + boneIDOffset + k;
                const int weightSlot = base + weightOffset + k;

                if (vertecies[weightSlot] == 0.0f) {
                    vertecies[boneSlot] = static_cast<float>(boneIndex);
                    vertecies[weightSlot] = weight;
                    break;
                }

            }

        }
    }

    Gizmo::SkinnedMesh myMesh(vertecies, { Gizmo::SubMesh(indecies, 0) }, Gizmo::BufferLayout({
       Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float3, false),
       Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float3, false),
       Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float2, false),
       Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float4, false),
       Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float4, false)
        }), bones);

    gMeshes.push_back(Gizmo::CreateRef<Gizmo::SkinnedMesh>(myMesh));
}

static void ProcessAiNode(aiNode* node, const aiScene* scene) {

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        gMeshesNames.push_back(std::string(node->mName.C_Str())); 
        ProcessAiMesh(mesh);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessAiNode(node->mChildren[i], scene);
    }
}

void processInput(GLFWwindow* window, glm::vec3 *cameraPos, glm::vec3 *cameraFront, glm::vec3 *cameraUp, float *pitch, float *yaw)
{
        const float cameraSpeed = 0.05f; // adjust accordingly
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        *cameraPos += cameraSpeed * *cameraFront * 0.1f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        *cameraPos -= cameraSpeed * *cameraFront * 0.1f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        *cameraPos -= glm::normalize(glm::cross(*cameraFront, *cameraUp)) * cameraSpeed * 0.1f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        *cameraPos += glm::normalize(glm::cross(*cameraFront, *cameraUp)) * cameraSpeed * 0.1f;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        cameraPos->y += cameraSpeed*0.1f;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        cameraPos->y -= cameraSpeed * 0.1f;
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
    {
        *pitch += cameraSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
    {
        *pitch -= cameraSpeed;
        
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
    {
        *yaw -= cameraSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
    {
        *yaw += cameraSpeed;
    }

    glm::vec3 direction;
    direction.x = cos(glm::radians(*yaw)) * cos(glm::radians(*pitch));
    direction.y = sin(glm::radians(*pitch));
    direction.z = sin(glm::radians(*yaw)) * cos(glm::radians(*pitch));
    *cameraFront = glm::normalize(direction);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(gWindowWidth, gWindowHeight, "OpenGL-Gizmos", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glClearColor(0.2f, 0.0f, 0.3f, 1.0f);
    glfwSwapInterval(0); //V-sync
   
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL Version: " << version << std::endl;

#ifdef GIZMOS_DEBUG
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
#endif // GIZMOS_DEBUG

    gizmo::init();
    Input::Init(window); 

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("C:/Users/ACER/Desktop/stormtrooper/source/StormTrooper.fbx", //C:/Users/ACER/Desktop/Nowy folder/hero.fbx "C:/Users/ACER/Desktop/stormtrooper/source/StormTrooper.fbx"
        aiProcess_GlobalScale |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights |
        aiProcess_ImproveCacheLocality);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return 0;
    }

    BuildHierarchy(scene->mRootNode); 
    ProcessAiNode(scene->mRootNode, scene); 

    std::vector<float> verticesbox = {
        //front face
        -0.02f, -0.02f,  0.02f, 0.0f, 0.0f, // 0
         0.02f, -0.02f,  0.02f,  1.0f, 0.0f,// 1
         0.02f,  0.02f,  0.02f,  1.0f, 1.0f, // 2
        -0.02f,  0.02f,  0.02f,  0.0f, 1.0f, // 3
        // back face
        -0.02f, -0.02f, -0.02f,  0.0f, 0.0f, // 4
         0.02f, -0.02f, -0.02f,  1.0f, 0.0f, // 5
         0.02f,  0.02f, -0.02f,  1.0f, 1.0f, // 6
        -0.02f,  0.02f, -0.02f,  0.0f, 1.0f,// 7
    };

    //indices for the square
    std::vector<uint32_t> indicesbox = {
        0, 1, 2, 2, 3, 0,  // front face
        1, 5, 6, 6, 2, 1,  // right face
        5, 4, 7, 7, 6, 5,  // back face
        4, 0, 3, 3, 7, 4,  // left face
        3, 2, 6, 6, 7, 3,  // top face
        4, 5, 1, 1, 0, 4   // bottom face
    };

    Gizmo::BufferLayout box_layout({ 
        Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float3, false), 
        Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float2, false)
        });

    Gizmo::StaticMesh boxMesh(verticesbox, { Gizmo::SubMesh(indicesbox, 0) }, box_layout);

    ShaderProgram defaultShader("shaders/v_default.glsl", "shaders/f_default.glsl");
    //ShaderProgram gridShader("shaders/v_grid.glsl", "shaders/f_grid.glsl");
    ShaderProgram textureShader("shaders/v_texture.glsl", "shaders/f_texture.glsl");

    Texture2D wallTexture("assets/textures/wall.jpg", 0);
    Texture2D stormTrooperBodyTexture( "C:/Users/ACER/Desktop/stormtrooper/textures/diffuse_body.png", 0);
    Texture2D stormTrooperHandTexture( "C:/Users/ACER/Desktop/stormtrooper/textures/diffuse_hands.png", 0);
    Texture2D stormTrooperHelmetTexture( "C:/Users/ACER/Desktop/stormtrooper/textures/diffuse_helmets.png", 0);

    std::unordered_map < std::string, Texture2D*> texturesMap; 
    texturesMap["body"] = &stormTrooperBodyTexture; 
    texturesMap["hand"] = &stormTrooperHandTexture;
    texturesMap["helmet"] = &stormTrooperHelmetTexture;

    glm::vec3 cameraPos = glm::vec3(.0f, 0.0f, -4.0f), objPos = glm::vec3(0.5f, 0.0f, 0.0f);
    glm::vec3 lightPos = glm::vec3(0.5f, 1.0f, 1.0f), lighColor = glm::vec3(1.0, 0.0, 0.0);

    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f, pitch = 0.0f;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(glm::mat4(1.0f), objPos);

    float deltaTime = 0.0;
    glfwSetTime(0.0f); 

    int index = 0; 

    while (!glfwWindowShouldClose(window)) {

        glClearColor(35.0f/255.0f, 35.0f / 255.0f, 35.0f / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

#ifdef GIZMOS_DEBUG
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Debug Window");
        ImGui::SliderFloat("x", &cameraPos[0], -10.0f, 10.0f);
        ImGui::SliderFloat("y", &cameraPos[1], -10.0f, 10.0f);
        ImGui::SliderFloat("z", &cameraPos[2], -10.0f, 10.0f);

        ImGui::SliderFloat3("cameraTarget", glm::value_ptr(cameraFront), -1.0f, 1.0f);
        ImGui::SliderFloat3("up", glm::value_ptr(cameraUp), -1.0f, 1.0f);
#endif // GIZMOS_DEBUG

        processInput(window, &cameraPos, &cameraFront, &cameraUp, &pitch, &yaw);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos+cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(80.0f), static_cast<float>(gWindowWidth) / static_cast<float>(gWindowHeight), 0.1f, 300.0f);

        gSkeleton->calculateGlobalTransforms();
        glm::mat4 temp = glm::mat4(1.0f);


        glm::mat4 boneGlobal = gSkeleton->getGlobalTransform(index); 
        glm::mat4 boneWorldMat = model * boneGlobal;
        glm::mat4 copy = boneGlobal;

        gizmo::manipulate(&view, &projection, &boneWorldMat, &temp);

        int parentIndex = gSkeleton->getNode(index).mParentIndex;
        glm::mat4 boneglobalTrans = glm::inverse(model) * boneWorldMat;

        
        glm::mat4 parentBoneGlobal = parentIndex == -1 ? glm::mat4(1.0f) : gSkeleton->getGlobalTransform(parentIndex);

        glm::mat4 boneNewLocalTrans = glm::inverse(parentBoneGlobal) * boneglobalTrans;

        gSkeleton->setNodeLocalTrans(index, boneNewLocalTrans); 
        gSkeleton->calculateGlobalTransforms();

        //model 
        for (int i = 0; i < gMeshes.size(); i++) {

            textureShader.use();
            gMeshes[i]->bindSubMesh(0);

            if(texturesMap[gMeshesNames[i]] != nullptr)
                texturesMap[gMeshesNames[i]]->Bind(); 

            glUniformMatrix4fv(textureShader.u("V"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(textureShader.u("P"), 1, GL_FALSE, glm::value_ptr(projection)); 
            glUniform3f(textureShader.u("color"), (GLfloat)0.2, (GLfloat)0.6, (GLfloat)0.2);

            glUniform1i(textureShader.u("myTexture"), texturesMap[gMeshesNames[i]] != nullptr ? texturesMap[gMeshesNames[i]]->getSlot() : 0);

            glUniform3f(textureShader.u("lightColor"), lighColor.x, lighColor.y, lighColor.z);
            glUniform3f(textureShader.u("lightPos"), lightPos.x, lightPos.y, lightPos.z);

            glUniformMatrix4fv(textureShader.u("M"), 1, GL_FALSE, glm::value_ptr(model));

            for (int j = 0; j < gSkeleton->getBoneCount() ; ++j) {
                std::string name = "uBoneMatrices[" + std::to_string(j) + "]";
                glUniformMatrix4fv(textureShader.u(name.c_str()), 1, GL_FALSE, glm::value_ptr(gSkeleton->getGlobalTransform(gSkeleton->getBone(j).mNodeIndex)* gSkeleton->getBone(j).mInvBindPose));
            }

            glDrawElements(GL_TRIANGLES, gMeshes[i]->getSubMesh(0).getCount(), GL_UNSIGNED_INT, 0);
        }

        int pixelX = static_cast<int>(Input::GetMouseX());
        int pixelY = 600 - static_cast<int>(Input::GetMouseY());

        //draw box as Bones transforamtions
        glDisable(GL_DEPTH_TEST); 
        defaultShader.use();
        boxMesh.bindSubMesh(0);
        glUniformMatrix4fv(defaultShader.u("V"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(defaultShader.u("P"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(defaultShader.u("color"), 149.0f/250.0f, 149.0f / 250.0f, 149.0f / 250.0f);

        for (int i = 4; i < gSkeleton->getNodeCount(); i++) {
            glm::mat4 boneGlobal = gSkeleton->getGlobalTransform(i);
            glm::mat4 trans = model * boneGlobal;
            trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5)); 

            glUniformMatrix4fv(defaultShader.u("M"), 1, GL_FALSE, glm::value_ptr(trans));
            glDrawElements(GL_TRIANGLES, boxMesh.getSubMesh(0).getCount(), GL_UNSIGNED_INT, nullptr);
        }

        //drawing light source cube
        glEnable(GL_DEPTH_TEST); 
        glUniform3f(defaultShader.u("color"), lighColor.x, lighColor.y, lighColor.z);
        glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), lightPos); 
        glUniformMatrix4fv(defaultShader.u("M"), 1, GL_FALSE, glm::value_ptr(lightModel));
        glDrawElements(GL_TRIANGLES, boxMesh.getSubMesh(0).getCount(), GL_UNSIGNED_INT, nullptr);

        gizmo::drawRotationGizmo();

#ifdef GIZMOS_DEBUG
        ImGui::Text("glfwGetTime() = %.3f", (float)glfwGetTime());  
        ImGui::Text("FPS: %d", (int)(1/((float)glfwGetTime() - deltaTime))); 

        ImGui::InputInt("index", &index, 1); 
        
        if (index < 0) index = gSkeleton->getNodeCount() - 1;
        if (index >= gSkeleton->getNodeCount()) index = 0;

        ImGui::Text(gSkeleton->getNode(index).mName.c_str());

        ImGui::InputFloat3("light Position", glm::value_ptr(lightPos)); 
        ImGui::InputFloat3("light Color", glm::value_ptr(lighColor)); 

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif // GIZMOS_DEBUG

        deltaTime = (float)glfwGetTime();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

#ifdef GIZMOS_DEBUG
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif // GIZMOS_DEBUG

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}