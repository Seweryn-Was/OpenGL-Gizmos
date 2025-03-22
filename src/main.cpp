#include <iostream>
#include "shaderprogram.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "ImGui Delta Time & FPS", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glfwSwapInterval(0); // Disable V-Sync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    float vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,  // 0
         0.5f, -0.5f,  0.5f,  // 1
         0.5f,  0.5f,  0.5f,  // 2
        -0.5f,  0.5f,  0.5f,  // 3
        // Back face
        -0.5f, -0.5f, -0.5f,  // 4
         0.5f, -0.5f, -0.5f,  // 5
         0.5f,  0.5f, -0.5f,  // 6
        -0.5f,  0.5f, -0.5f   // 7
    };

    // Indices for the square
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,  // Front face
        1, 5, 6, 6, 2, 1,  // Right face
        5, 4, 7, 7, 6, 5,  // Back face
        4, 0, 3, 3, 7, 4,  // Left face
        3, 2, 6, 6, 7, 3,  // Top face
        4, 5, 1, 1, 0, 4   // Bottom face
    };

  
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
  
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

    ShaderProgram defaultShader = ShaderProgram("shaders/v_default.glsl", "shaders/f_default.glsl"); 

    glClearColor(0.0f, 0.0f, 0.3f, 1.0f);
    float myAngle = 0.0f; 
    while (!glfwWindowShouldClose(window)) {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Debug Window");
        ImGui::Text("glfwGetTime() = %.3f", (float)glfwGetTime());
        ImGui::End();
        
        ImGui::Render();

        defaultShader.use();
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(defaultShader.u("M"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(defaultShader.u("V"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(defaultShader.u("P"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(defaultShader.u("color"), 1.0f, 0.0f, 0.0f );

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,sizeof(indices), GL_UNSIGNED_INT, 0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
