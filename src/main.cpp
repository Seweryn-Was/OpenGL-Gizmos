#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


#define GIZMOS_DEBUG

#include "shaderprogram.h"
#include "VertexArray.h"
#include "Base.h"
#include "openglUtil.h"
#include "Gizmo.h"
#include "Input.h"

#define PI 3.14159

uint16_t gWindowWidth = 1200, gWindowHeight = 800;

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

    
    std::vector<GLfloat> verticesCircle;
    std::vector<GLuint> indicesCircle;

    int numSegments = 100;

    float radius = 0.7f;
     
    for (int i = 0; i <= numSegments; i++) {
        float angle = (float)i / (float)numSegments * 2 *  PI;
        float x = (radius + 0.05) * cos(angle);
        float y = (radius + 0.05) * sin(angle);
        verticesCircle.push_back(x * 1.5f);
        verticesCircle.push_back(y * 1.5f);
        verticesCircle.push_back(0.0f);
    }

    for (int i = 0; i < numSegments; i++) {
        indicesCircle.push_back(i);
        indicesCircle.push_back(i + 1);
    }

    unsigned int VAOcircle, VBOcircle, EBOcircle;
    glGenVertexArrays(1, &VAOcircle);
    glGenBuffers(1, &VBOcircle);
    glGenBuffers(1, &EBOcircle);

    glBindVertexArray(VAOcircle);


    glBindBuffer(GL_ARRAY_BUFFER, VBOcircle);
    glBufferData(GL_ARRAY_BUFFER, verticesCircle.size() * sizeof(GLfloat), verticesCircle.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcircle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCircle.size() * sizeof(GLuint), indicesCircle.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float verticesbox[] = {
        // front face
        -0.5f, -0.5f,  0.5f,  // 0
         0.5f, -0.5f,  0.5f,  // 1
         0.5f,  0.5f,  0.5f,  // 2
        -0.5f,  0.5f,  0.5f,  // 3
        // back face
        -0.5f, -0.5f, -0.5f,  // 4
         0.5f, -0.5f, -0.5f,  // 5
         0.5f,  0.5f, -0.5f,  // 6
        -0.5f,  0.5f, -0.5f   // 7
    };

    //indices for the square
    unsigned int indicesbox[] = {
        0, 1, 2, 2, 3, 0,  // front face
        1, 5, 6, 6, 2, 1,  // right face
        5, 4, 7, 7, 6, 5,  // back face
        4, 0, 3, 3, 7, 4,  // left face
        3, 2, 6, 6, 7, 3,  // top face
        4, 5, 1, 1, 0, 4   // bottom face
    };


    Gizmo::VertexArray box_vao; 

    Gizmo::VertexBuffer box_vbo(&(verticesbox[0]), sizeof(verticesbox));
    Gizmo::BufferLayout box_layout({ Gizmo::BufferAttribute(Gizmo::ShaderDataType::Float3, false)});
    box_vbo.SetLayout(box_layout); 

    Gizmo::IndexBuffer box_ebo(&(indicesbox[0]), sizeof(indicesbox));

    box_vao.AddVertexBuffer(Gizmo::CreateRef<Gizmo::VertexBuffer>(box_vbo)); 
    box_vao.SetIndexBuffer(Gizmo::CreateRef<Gizmo::IndexBuffer>(box_ebo)); 

    ShaderProgram defaultShader("shaders/v_default.glsl", "shaders/f_default.glsl");
    ShaderProgram gridShader("shaders/v_grid.glsl", "shaders/f_grid.glsl");

    glm::vec3 cameraPos = glm::vec3(.0f, 0.0f, -5.0f), objPos = glm::vec3(1.0f, 0.0f, 0.0f);

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(glm::mat4(1.0f), objPos);
    float deltaTime = 0.0; 
    glfwSetTime(0.0f); 


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
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(gWindowWidth) / static_cast<float>(gWindowHeight), 0.1f, 100.0f);

        defaultShader.use();

        glUniformMatrix4fv(defaultShader.u("V"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(defaultShader.u("P"), 1, GL_FALSE, glm::value_ptr(projection));

        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(VAOcircle);
        glBindBuffer(GL_ARRAY_BUFFER, VBOcircle);
        glUniform3f(defaultShader.u("color"), 0.5, 0.5, 0.5);

        glUniformMatrix4fv(defaultShader.u("M"), 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1.0f), objPos)));

        glDrawElements(GL_LINES, indicesCircle.size(), GL_UNSIGNED_INT, 0);
        glEnable(GL_DEPTH_TEST);

        int pixelX = static_cast<int>(Input::GetMouseX());
        int pixelY = 600 - static_cast<int>(Input::GetMouseY());

        gizmo::manipulate(&view, &projection, &model);

        //draw box
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        defaultShader.use(); 

        glUniformMatrix4fv(defaultShader.u("M"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(defaultShader.u("color"), 149.0f/250.0f, 149.0f / 250.0f, 149.0f / 250.0f);
        box_vao.Bind(); 
        glDrawElements(GL_TRIANGLES, box_ebo.GetCount(), GL_UNSIGNED_INT, 0);

        gizmo::drawRotationGizmo(); 

#ifdef GIZMOS_DEBUG
        ImGui::Text("glfwGetTime() = %.3f", (float)glfwGetTime());  
        ImGui::Text("FPS: %d", (int)(1/((float)glfwGetTime() - deltaTime))); 
        deltaTime = glfwGetTime(); 

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif // GIZMOS_DEBUG

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


