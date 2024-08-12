#define TINYOBJLOADER_IMPLEMENTATION // For tiny_obj_loader
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "tiny_obj_loader.h"
#include <vector>
#include <iostream>

// Vertex shader source code
const char* vertexShaderSource = R"glsl(
#version 120
attribute vec3 aPos;
uniform mat4 transform;
void main() {
    gl_Position = transform * vec4(aPos, 1.0);
}
)glsl";

// Fragment shader source code
const char* fragmentShaderSource = R"glsl(
#version 120
void main() {
    gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Set the color to green
}
)glsl";

// Define transformation variables
glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
float rotationAngleZ = 0.0f; // in degrees for Z
float rotationAngleY = 0.0f; // in degrees for Y
float scaleFactor = 6.0f; // Scale factor

std::vector<float> vertices;
std::vector<unsigned int> indices;

void loadOBJ(const std::string& path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str());
    if (!ret) {
        std::cerr << "Failed to load/parse .obj file!" << std::endl;
        return;
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
            vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
            vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);

            indices.push_back(indices.size());
        }
    }
}

// Process input to update transformations
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float d = 0.01f; // Adjust the value as needed

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        translation.y += d; // Move up
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        translation.y -= d; // Move down
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        translation.x -= d; // Move left
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        translation.x += d; // Move right

    float s = 0.01f; // Adjust the value as needed
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        scaleFactor += s; // Scale up
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        scaleFactor -= s; // Scale down

    // Z axis
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        rotationAngleZ += 30.0f; // Rotate counterclockwise
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        rotationAngleZ -= 30.0f; // Rotate clockwise

    // Y axis
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        rotationAngleY += 30.0f; // Rotate counterclockwise
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        rotationAngleY -= 30.0f; // Rotate clockwise
}

// Compile a shader and check for errors
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

// Create a shader program and link shaders
GLuint createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Set up vertex buffer and vertex array objects
void setupBuffers(GLuint &VAO, GLuint &VBO, GLuint &EBO) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Update transformation matrix in the shader program
void updateTransform(GLuint shaderProgram) {
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, translation);
    transform = glm::rotate(transform, glm::radians(rotationAngleY), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotationAngleZ), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, glm::vec3(scaleFactor, scaleFactor, scaleFactor));

    GLuint transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
}

// Main rendering loop
void run() {
    if (!glfwInit())
        return;

    GLFWwindow* window = glfwCreateWindow(640, 480, "Travel Mug Wireframe", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Error initializing GLEW" << std::endl;
        return;
    }

    loadOBJ("mug.obj");

    GLuint shaderProgram = createShaderProgram();
    GLuint VAO, VBO, EBO;
    setupBuffers(VAO, VBO, EBO);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        updateTransform(shaderProgram);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe mode
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
}

int main() {
    run();
    return 0;
}
