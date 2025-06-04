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

std::vector<Gizmo::Ref<Gizmo::StaticMesh>> gMeshes;

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
    trans.scale = glm::vec3(1.0f);
    return trans;
}

struct NodeData {    
    std::string name;
    Transform localTrans; 
    glm::mat4 invBindPose = glm::mat4(1.0f); 
    int32_t parent = -1; 
    std::vector<int32_t> children = {};
};

std::map<std::string, int> boneNameToIndex;
std::vector<uint32_t> gpuIndexToCpuIndex(100);
std::vector<NodeData> nodeHierarchy(50); 
std::vector<glm::mat4> finalBoneMatrices(100, glm::mat4(1.0f));

int boneCurrIndex = 0; 

void BuildNodeHierarchy(int32_t parent, glm::mat4 parentGlobalTrans, const aiNode* node) {

    std::string nodeName(node->mName.C_Str());

    int32_t index;
    glm::mat4 globalTrans; 

    if (boneNameToIndex.find(nodeName) == boneNameToIndex.end()) {
        boneNameToIndex[nodeName] = boneCurrIndex;
        index = boneCurrIndex; 

        glm::mat4 localTrans = aiMatrix4x4ToGlm(node->mTransformation); 
        nodeHierarchy[boneCurrIndex].name = nodeName;
        nodeHierarchy[boneCurrIndex].parent = parent; 
        nodeHierarchy[boneCurrIndex].localTrans = DecomposeMatrix(localTrans);
        globalTrans = parentGlobalTrans * localTrans;
        nodeHierarchy[boneCurrIndex].invBindPose = glm::inverse(globalTrans);

        if (parent != -1) {
            nodeHierarchy[parent].children.push_back(index);
        }
        boneCurrIndex++;
    } else {
        index = boneNameToIndex[nodeName]; 
        globalTrans = glm::inverse(nodeHierarchy[index].invBindPose);
    }

    for (int i = 0; i < node->mNumChildren; i++) {
        BuildNodeHierarchy(boneNameToIndex[nodeName], globalTrans, node->mChildren[i]);
    }
}

uint32_t gpuBoneIndexCtr = 0; 

void ProcessMesh(aiMesh* mesh, const aiScene* scene) {

    std::vector<float> meshVert;
    std::vector<uint32_t> meshIndecies;

    const int stride = 16;
    const int boneIDOffset = 8, weightOffset = 12;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        glm::vec3 normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f);
        glm::vec2 texCoords = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);       

        meshVert.push_back(mesh->mVertices[i].x);
        meshVert.push_back(mesh->mVertices[i].y);
        meshVert.push_back(mesh->mVertices[i].z);
        
        meshVert.push_back(normal.x); 
        meshVert.push_back(normal.y);
        meshVert.push_back(normal.z); 

        meshVert.push_back(texCoords.x);
        meshVert.push_back(texCoords.y);

        //Bone Id; 
        meshVert.push_back(-1.0f);
        meshVert.push_back(-1.0f);
        meshVert.push_back(-1.0f);
        meshVert.push_back(-1.0f);

        //weights
        meshVert.push_back(0.0f);
        meshVert.push_back(0.0f);
        meshVert.push_back(0.0f);
        meshVert.push_back(0.0f);
        // Store to your vertex structure
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        //gModelIndices.push_back({});
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            meshIndecies.push_back(face.mIndices[j]);
        }
    }

    //skinning
    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        aiBone* aibone = mesh->mBones[i];
        std::string boneName(aibone->mName.C_Str());

        int cpuBoneIndex;
        if (boneNameToIndex.find(boneName) == boneNameToIndex.end()) {
            assertm(false, "Somethings Wrong didnt find Bone in Node Hierarchy");
        }
        else {
            cpuBoneIndex = boneNameToIndex[boneName];
        }

        uint32_t gpuBoneIndex = gpuBoneIndexCtr;
        for (int j = 0; j < gpuBoneIndexCtr; j++) {
            if (gpuIndexToCpuIndex[j] == cpuBoneIndex) {
                gpuBoneIndex = j; 
            }
        }

        gpuIndexToCpuIndex[gpuBoneIndexCtr] = cpuBoneIndex; 

        if(gpuBoneIndex == gpuBoneIndexCtr) gpuBoneIndexCtr++; 

        for (unsigned int j = 0; j < aibone->mNumWeights; ++j) {
            const int vertexID = aibone->mWeights[j].mVertexId;
            const float weight = aibone->mWeights[j].mWeight;

            const int base = vertexID * stride;

            for (int k = 0; k < 4; ++k) {
                const int boneSlot = base + boneIDOffset + k;
                const int weightSlot = base + weightOffset + k;

                if (meshVert[weightSlot] == 0.0f) {
                    meshVert[boneSlot] = static_cast<float>(gpuBoneIndex);
                    meshVert[weightSlot] = weight;
                    break;
                }
            }

        }
    }

    Gizmo::StaticMesh myMesh(meshVert, { Gizmo::SubMesh(meshIndecies, 0) }, Gizmo::BufferLayout({
        Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float3, false),
        Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float3, false),
        Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float2, false),
        Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float4, false),
        Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float4, false)
        }));
    gMeshes.push_back(Gizmo::CreateRef<Gizmo::StaticMesh>(myMesh));

}

Gizmo::Ref<Gizmo::StaticMesh> stormTrooperMesh;


static void ProcessNode(aiNode* node, const aiScene* scene) {

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene);
    }
}

glm::mat4 CalculateGloablTrans(int index) {
    if (nodeHierarchy[index].parent == -1) {
        return nodeHierarchy[index].localTrans.GetTransMat(); 
    }
    return CalculateGloablTrans(nodeHierarchy[index].parent) * nodeHierarchy[index].localTrans.GetTransMat();
}

void CalculateGlobalTrans(std::vector<glm::mat4>& globalTrans) {
    globalTrans.resize(gpuIndexToCpuIndex.size()); 
    for (int i = 0; i < gpuIndexToCpuIndex.size(); i++) {
        globalTrans[i] = CalculateGloablTrans(gpuIndexToCpuIndex[i]);
        globalTrans[i] = globalTrans[i] * nodeHierarchy[gpuIndexToCpuIndex[i]].invBindPose;
    }
}

std::vector<glm::vec3> boneLines;

void CollectBoneLines() {
    boneLines.clear();

    for (int i = 0; i < boneCurrIndex; ++i) {
        glm::mat4 globalTransform = CalculateGloablTrans(i);
        glm::vec3 parentPos = glm::vec3(globalTransform[3]);  // translation column

        for (int childIndex : nodeHierarchy[i].children) {
            glm::mat4 childTransform = CalculateGloablTrans(childIndex);
            glm::vec3 childPos = glm::vec3(childTransform[3]);

            boneLines.push_back(parentPos);
            boneLines.push_back(childPos);  // One line = two points
        }
    }
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
    assertm(false, "give path to a model file"); 
    const aiScene* scene = importer.ReadFile("<path to model file>",
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

    BuildNodeHierarchy(-1, glm::mat4(1.0f), scene->mRootNode); 
    ProcessNode(scene->mRootNode, scene);

    CalculateGlobalTrans(finalBoneMatrices); 

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
    ShaderProgram textureShader("shaders/v_texture.glsl", "shaders/f_texture.glsl");

    Texture2D wallTexture("assets/textures/wall.jpg", 0);

    assertm(false, "give path to texture"); 
    Texture2D stormTrooperTexture("<path to texture>", 0);

    glm::vec3 cameraPos = glm::vec3(.0f, 0.0f, -4.0f), objPos = glm::vec3(0.5f, 0.0f, 0.0f);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(glm::mat4(1.0f), objPos);

    float deltaTime = 0.0;
    glfwSetTime(0.0f); 

    int index = 0; 

    while (!glfwWindowShouldClose(window)) {

        glClearColor(35.0f/255.0f, 35.0f / 255.0f, 35.0f / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef GIZMOS_DEBUG
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Debug Window");
#endif // GIZMOS_DEBUG
        glm::mat4 view = glm::translate(glm::mat4(1.0f), cameraPos);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(gWindowWidth) / static_cast<float>(gWindowHeight), 0.1f, 300.0f);

        glm::mat4 temp = glm::mat4(1.0f);

        glm::mat4 boneGlobal = CalculateGloablTrans(gpuIndexToCpuIndex[index]);
        glm::mat4 boneWorldMat = model * boneGlobal; 
        glm::mat4 copy = boneGlobal; 

        int parentIndex = nodeHierarchy[gpuIndexToCpuIndex[index]].parent; 

        gizmo::manipulate(&view, &projection, &boneWorldMat, &temp);

        glm::mat4 parentGlobalTrans = CalculateGloablTrans(parentIndex); 
        glm::mat4 boneglobalTrans = glm::inverse(model) * boneWorldMat;
        glm::mat4 boneLocalTrans = glm::inverse(parentGlobalTrans) * boneglobalTrans; 

        glm::quat rot; 
        glm::decompose(boneLocalTrans, glm::vec3(), rot, glm::vec3(), glm::vec3(), glm::vec4());

        nodeHierarchy[gpuIndexToCpuIndex[index]].localTrans.rotation = rot; 

        CalculateGlobalTrans(finalBoneMatrices);

        //model 
        for (int i = 0; i < gMeshes.size(); i++) {

            textureShader.use();
            gMeshes[i]->bindSubMesh(0);
            stormTrooperTexture.Bind();
            glUniformMatrix4fv(textureShader.u("V"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(textureShader.u("P"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform1i(textureShader.u("index"), index); 
            glUniform3f(textureShader.u("color"), (GLfloat)0.2, (GLfloat)0.6, (GLfloat)0.2);

            glUniform1i(textureShader.u("myTexture"), 0);

            glUniformMatrix4fv(textureShader.u("M"), 1, GL_FALSE, glm::value_ptr(model));

            for (int i = 0; i < finalBoneMatrices.size(); ++i) {
                std::string name = "uBoneMatrices[" + std::to_string(i) + "]";
                glUniformMatrix4fv(textureShader.u(name.c_str()), 1, GL_FALSE, glm::value_ptr(finalBoneMatrices[i]));
            }
            glDrawElements(GL_TRIANGLES, gMeshes[i]->getSubMesh(0).getCount(), GL_UNSIGNED_INT, 0);
        }

        int pixelX = static_cast<int>(Input::GetMouseX());
        int pixelY = 600 - static_cast<int>(Input::GetMouseY());

        //draw box
        defaultShader.use();
        boxMesh.bindSubMesh(0); //box_vao.Bind();
        glUniformMatrix4fv(defaultShader.u("V"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(defaultShader.u("P"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(defaultShader.u("color"), 149.0f/250.0f, 149.0f / 250.0f, 149.0f / 250.0f);

        for (int i = 0; i < boneCurrIndex; i++) {
            glm::mat4 boneGlobal = finalBoneMatrices[i] * glm::inverse(nodeHierarchy[gpuIndexToCpuIndex[i]].invBindPose);
            glm::mat4 trans = model * boneGlobal;

            glUniform3f(defaultShader.u("color"), 149.0f / 250.0f, 149.0f / 250.0f, 149.0f / 250.0f);
            if (nodeHierarchy[gpuIndexToCpuIndex[index]].parent == gpuIndexToCpuIndex[i]) {
                glUniform3f(defaultShader.u("color"), 0.0f, 1.0f, 0.0f);
            }
            if (gpuIndexToCpuIndex[i] == gpuIndexToCpuIndex[index]) {
                glUniform3f(defaultShader.u("color"), 1.0f, 0.0f, 0.0f);
            }
            if (std::count(nodeHierarchy[gpuIndexToCpuIndex[index]].children.begin(), nodeHierarchy[gpuIndexToCpuIndex[index]].children.end(), gpuIndexToCpuIndex[i])) {
                glUniform3f(defaultShader.u("color"), 0.0f, 0.0f, 1.0f);
            }

            glUniformMatrix4fv(defaultShader.u("M"), 1, GL_FALSE, glm::value_ptr(trans));
            glDrawElements(GL_TRIANGLES, boxMesh.getSubMesh(0).getCount(), GL_UNSIGNED_INT, nullptr);
        }

        gizmo::drawRotationGizmo();

#ifdef GIZMOS_DEBUG
        ImGui::Text("glfwGetTime() = %.3f", (float)glfwGetTime());  
        ImGui::Text("FPS: %d", (int)(1/((float)glfwGetTime() - deltaTime))); 
        
        std::string boneName; 
        ImGui::InputInt("index", &index, 1); 
        for (const auto& [key, value] : boneNameToIndex)
            if (value == gpuIndexToCpuIndex[index])
                 boneName = key;

        ImGui::Text(boneName.c_str());
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