#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <thread>
#include "shader.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "texture.hpp"

float
Clamp(float x, float min, float max) {
    return x < min ? min : x > max ? max : x;
}

int WindowWidth = 1200;
int WindowHeight = 1200;
const float TargetFPS = 60.0f;
const std::string WindowTitle = "Phong";

struct Input {
    bool MoveLeft;
    bool MoveRight;
    bool MoveUp;
    bool MoveDown;
    bool LookLeft;
    bool LookRight;
    bool LookUp;
    bool LookDown;
};

struct EngineState {
    Input* mInput;
    Camera* mCamera;
    unsigned mShadingMode;
    bool mDrawDebugLines;
    float mDT;
};

static void
ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << std::endl;
}

static float Svetlofenjer = 0;

static void
KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    EngineState* State = (EngineState*)glfwGetWindowUserPointer(window);
    Input* UserInput = State->mInput;
    bool IsDown = action == GLFW_PRESS || action == GLFW_REPEAT;
    switch (key) {
    case GLFW_KEY_A: UserInput->MoveLeft = IsDown; break;
    case GLFW_KEY_D: UserInput->MoveRight = IsDown; break;
    case GLFW_KEY_W: UserInput->MoveUp = IsDown; break;
    case GLFW_KEY_S: UserInput->MoveDown = IsDown; break;

    case GLFW_KEY_RIGHT: UserInput->LookLeft = IsDown; break;
    case GLFW_KEY_LEFT: UserInput->LookRight = IsDown; break;
    case GLFW_KEY_UP: UserInput->LookUp = IsDown; break;
    case GLFW_KEY_DOWN: UserInput->LookDown = IsDown; break;

    case GLFW_KEY_1: Svetlofenjer = 1; break;
    case GLFW_KEY_2: Svetlofenjer = 0; break;
    case GLFW_KEY_L: {
        if (IsDown) {
            State->mDrawDebugLines ^= true; break;
        }
    } break;

    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    }
}
static void
MoveCube(GLFWwindow* context, float& x, float& y, float& z) {
    if (glfwGetKey(context, GLFW_KEY_C) == GLFW_PRESS) {
        if (y < 0) {
            y += 0.08;
        }
    }
    if (glfwGetKey(context, GLFW_KEY_V) == GLFW_PRESS) {
        if (y > -2.0) {
            y -= 0.08;
        }
    }
}

static void
FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    WindowWidth = width;
    WindowHeight = height;
    glViewport(0, 0, width, height);
}

static void
HandleInput(EngineState* state) {
    Input* UserInput = state->mInput;
    Camera* FPSCamera = state->mCamera;
    if (UserInput->MoveLeft) FPSCamera->Move(-1.0f, 0.0f, state->mDT);
    if (UserInput->MoveRight) FPSCamera->Move(1.0f, 0.0f, state->mDT);
    if (UserInput->MoveDown) FPSCamera->Move(0.0f, -1.0f, state->mDT);
    if (UserInput->MoveUp) FPSCamera->Move(0.0f, 1.0f, state->mDT);

    if (UserInput->LookLeft) FPSCamera->Rotate(1.0f, 0.0f, state->mDT);
    if (UserInput->LookRight) FPSCamera->Rotate(-1.0f, 0.0f, state->mDT);
    if (UserInput->LookDown) FPSCamera->Rotate(0.0f, -1.0f, state->mDT);
    if (UserInput->LookUp) FPSCamera->Rotate(0.0f, 1.0f, state->mDT);
}

static void
DrawFloor(unsigned vao, const Shader& shader, unsigned diffuse, unsigned specular) {
    glUseProgram(shader.GetId());
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuse);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specular);
    float Size = 4.0f;
    for (int i = -2; i < 4; ++i) {
        for (int j = -2; j < 4; ++j) {
            glm::mat4 Model(1.0f);
            Model = glm::translate(Model, glm::vec3(i * Size, -2.0f, j * Size));
            Model = glm::scale(Model, glm::vec3(Size, 0.1f, Size));
            shader.SetModel(Model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

float GetRadians(float angle) {
    return 3.14 * angle / 180;
}


int main() {
    GLFWwindow* Window = 0;
    if (!glfwInit()) {
        std::cerr << "Failed to init glfw" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window = glfwCreateWindow(WindowWidth, WindowHeight, WindowTitle.c_str(), 0, 0);
    if (!Window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(Window);

    GLenum GlewError = glewInit();
    if (GlewError != GLEW_OK) {
        std::cerr << "Failed to init glew: " << glewGetErrorString(GlewError) << std::endl;
        glfwTerminate();
        return -1;
    }

    EngineState State = { 0 };
    Camera FPSCamera;
    Input UserInput = { 0 };
    State.mCamera = &FPSCamera;
    State.mInput = &UserInput;
    glfwSetWindowUserPointer(Window, &State);

    glfwSetErrorCallback(ErrorCallback);
    glfwSetFramebufferSizeCallback(Window, FramebufferSizeCallback);
    glfwSetKeyCallback(Window, KeyCallback);

    glViewport(0.0f, 0.0f, WindowWidth, WindowHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    unsigned CubeDiffuseTexture = Texture::LoadImageToTexture("res/container_diffuse.png");
    unsigned CubeSpecularTexture = Texture::LoadImageToTexture("res/container_specular.png");
    unsigned WaterDiffuseTexture = Texture::LoadImageToTexture("res/water.jpg");
    unsigned WaterSpecularTexture = Texture::LoadImageToTexture("res/water-diff.jpg");
    unsigned TentTexture = Texture::LoadImageToTexture("res/tent.jpg");
    unsigned CubeTexture = Texture::LoadImageToTexture("res/cube.jpg");
    unsigned FloorDiffuseTexture = Texture::LoadImageToTexture("res/ice.jpg");
    unsigned FloorSpecularTexture = Texture::LoadImageToTexture("res/ice-diff.jpg");


    std::vector<float> CubeVertices = { //Vec dato 
       // X     Y     Z     NX    NY    NZ    U     V    FRONT SIDE
       -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // L D
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // R D
       -0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // L U
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // R D
        0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // R U
       -0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // L U
                                                       // LEFT SIDE
       -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // L D
       -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
       -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
       -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
       -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // R U
       -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
                                                       // RIGHT SIDE
        0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // L D
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
        0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
        0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // R U
        0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
                                                       // BOTTOM SIDE
       -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // L D
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // R D
       -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // L U
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // R D
        0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // R U
       -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // L U
                                                       // TOP SIDE
       -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // L D
        0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // R D
       -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // L U
        0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // R D
        0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // R U
       -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // L U
                                                       // BACK SIDE
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // L D
       -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // R D
        0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // L U
       -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // R D
       -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // R U
        0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // L U
    };

    unsigned CubeVAO;
    glGenVertexArrays(1, &CubeVAO);
    glBindVertexArray(CubeVAO);
    unsigned CubeVBO;
    glGenBuffers(1, &CubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, CubeVertices.size() * sizeof(float), CubeVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    Model Fox("res/low-poly-fox/low-poly-fox.obj");
    if (!Fox.Load()) {
        std::cerr << "Failed to load fox\n";
        glfwTerminate();
        return -1;
    }
    Shader ColorShader("shaders/color.vert", "shaders/color.frag");

    Shader PhongShaderMaterialTexture("shaders/basic.vert", "shaders/phong_material_texture.frag");
    glUseProgram(PhongShaderMaterialTexture.GetId());

    //Ambijentalno svetlo/Direkciono
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Direction", glm::vec3(1.0f, -1.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Ka", glm::vec3(0.7f, 0.7f, 0.7f));
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Kd", glm::vec3(0.05f, 0.05f, 0.05f));
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Ks", glm::vec3(0.8f));

    //Svetlofenjer
    PhongShaderMaterialTexture.SetUniform3f("uPointLight.Position", glm::vec3(0.3f, 0.5f, -3.3f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLight.Ka", glm::vec3(0.5f * Svetlofenjer, 0.5f * Svetlofenjer, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLight.Kd", glm::vec3(0.5f * Svetlofenjer, 0.5f * Svetlofenjer, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLight.Ks", glm::vec3(1.0f * Svetlofenjer));
    PhongShaderMaterialTexture.SetUniform1f("uPointLight.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uPointLight.Kl", 0.092f);
    PhongShaderMaterialTexture.SetUniform1f("uPointLight.Kq", 0.032f);

    //plavo
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight2.Position", glm::vec3(5.7f, -1.0f, 0.6f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight2.Direction", glm::vec3(0.0f, -0.5f, -1.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight2.Ka", glm::vec3(0.0f, 0.0f, 1.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight2.Kd", glm::vec3(0.0f, 0.0f, 1.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight2.Ks", glm::vec3(1.0f));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight2.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight2.Kl", 0.092f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight2.Kq", 0.032f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight2.InnerCutOff", glm::cos(glm::radians(12.0f))); // kad pomeras kocku pojacava se/opada
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight2.OuterCutOff", glm::cos(glm::radians(12.0f)));

    //crvena
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight3.Position", glm::vec3(5.70f, -1.0f, 1.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight3.Direction", glm::vec3(0.0f, -0.5f, 1.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight3.Ka", glm::vec3(1.0f, 0.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight3.Kd", glm::vec3(1.0f, 0.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight3.Ks", glm::vec3(1.0f));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight3.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight3.Kl", 0.092f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight3.Kq", 0.032f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight3.InnerCutOff", glm::cos(glm::radians(12.0f))); // kad pomeras kocku pojacava se/opada
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight3.OuterCutOff", glm::cos(glm::radians(12.0f)));

    //nandzarasta
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight4.Position", glm::vec3(5.7f, -0.8f, 0.8f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight4.Direction", glm::vec3(0.0f, 1.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight4.Ka", glm::vec3(1.0f, 0.5f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight4.Kd", glm::vec3(1.0f, 0.5f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight4.Ks", glm::vec3(1.0f, 0.5f, 0.0f));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight4.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight4.Kl", 0.092f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight4.Kq", 0.032f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight4.InnerCutOff", glm::cos(glm::radians(12.0f))); // kad pomeras kocku pojacava se/opada
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight4.OuterCutOff", glm::cos(glm::radians(12.0f)));


    //zutiOdsjaj
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Position", glm::vec3(5.9f, -1.0f, 0.8));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Direction", glm::vec3(1.0f, -0.5f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Ka", glm::vec3(1.0f, 1.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Kd", glm::vec3(1.0f, 1.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Ks", glm::vec3(1.0f));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kl", 0.092f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kq", 0.032f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.InnerCutOff", glm::cos(glm::radians(12.0f))); // kad pomeras kocku pojacava se/opada
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.OuterCutOff", glm::cos(glm::radians(12.0f)));


    //ljubicasta
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight5.Position", glm::vec3(5.7f, -1.2f, 0.8f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight5.Direction", glm::vec3(0.0f, -1.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight5.Ka", glm::vec3(0.5f, 0.0f, 0.5f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight5.Kd", glm::vec3(0.5f, 0.0f, 0.5f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight5.Ks", glm::vec3(0.5f, 0.0f, 0.5f));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight5.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight5.Kl", 0.092f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight5.Kq", 0.032f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight5.InnerCutOff", glm::cos(glm::radians(12.0f))); // kad pomeras kocku pojacava se/opada
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight5.OuterCutOff", glm::cos(glm::radians(12.0f)));

    //zelena
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight1.Position", glm::vec3(5.5f, -1.0f, 0.8f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight1.Direction", glm::vec3(-1.0f, -0.5f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight1.Ka", glm::vec3(0.0f, 1.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight1.Kd", glm::vec3(0.0f, 1.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight1.Ks", glm::vec3(1.0f));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight1.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight1.Kl", 0.092f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight1.Kq", 0.032f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight1.InnerCutOff", glm::cos(glm::radians(12.0f))); // kad pomeras kocku pojacava se/opada
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight1.OuterCutOff", glm::cos(glm::radians(12.0f)));

    // Diminishes the light's diffuse component by half, tinting it slightly red
    PhongShaderMaterialTexture.SetUniform1i("uMaterial.Kd", 0);
    // Makes the object really shiny
    PhongShaderMaterialTexture.SetUniform1i("uMaterial.Ks", 1);
    PhongShaderMaterialTexture.SetUniform1f("uMaterial.Shininess", 128.0f);
    glUseProgram(0);

    glm::mat4 Projection = glm::perspective(45.0f, WindowWidth / (float)WindowHeight, 0.1f, 100.0f);
    glm::mat4 View = glm::lookAt(FPSCamera.GetPosition(), FPSCamera.GetTarget(), FPSCamera.GetUp());
    glm::mat4 ModelMatrix(1.0f);

    // Current angle around Y axis, with regards to XZ plane at which the point light is situated at
    float Angle = 0.0f;
    // Distance of point light from center of rotation
    float Distance = 5.0f;
    float TargetFrameTime = 1.0f / TargetFPS;
    float StartTime = glfwGetTime();
    float EndTime = glfwGetTime();
    glClearColor(0.3f, 0.7f, 1.0f, 0.0f);

    Shader* CurrentShader = &PhongShaderMaterialTexture;
    float x = 0, y = 0, z = 0;
    while (!glfwWindowShouldClose(Window)) {
        glfwPollEvents();
        HandleInput(&State);

        glUseProgram(CurrentShader->GetId());
        PhongShaderMaterialTexture.SetUniform3f("uPointLight.Kd", glm::vec3(0.5f * Svetlofenjer, 0.5f * Svetlofenjer, 0.0f));
        PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Position", glm::vec3(5.5f, -1.0f + y, 0.8f));
        PhongShaderMaterialTexture.SetUniform3f("uSpotlight1.Position", glm::vec3(5.9f, -1.0f + y, 0.8));
        PhongShaderMaterialTexture.SetUniform3f("uSpotlight2.Position", glm::vec3(5.7f, -1.0f + y, 0.6f));
        PhongShaderMaterialTexture.SetUniform3f("uSpotlight3.Position", glm::vec3(5.70f, -1.0f + y, 1.0f));
        PhongShaderMaterialTexture.SetUniform3f("uSpotlight4.Position", glm::vec3(5.7f, -0.8f + y, 0.8f));
        PhongShaderMaterialTexture.SetUniform3f("uSpotlight5.Position", glm::vec3(5.7f, -1.2f + y, 0.8f));
        glUseProgram(0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // NOTE(Jovan): In case of window resize, update projection. Bit bad for performance to do it every iteration.
        // If laggy, remove this line
        Projection = glm::perspective(45.0f, WindowWidth / (float)WindowHeight, 0.1f, 100.0f);
        View = glm::lookAt(FPSCamera.GetPosition(), FPSCamera.GetTarget(), FPSCamera.GetUp());
        StartTime = glfwGetTime();
        glUseProgram(CurrentShader->GetId());
        CurrentShader->SetProjection(Projection);
        CurrentShader->SetView(View);
        CurrentShader->SetUniform3f("uViewPos", FPSCamera.GetPosition());

        Angle += State.mDT;
        MoveCube(Window, x, y, z);
        glm::mat4 identity(1.0f);

        // NOTE(Jovan): Set cube specular and diffuse textures
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(6.0, -2.65, 1.0));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f));
        CurrentShader->SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, WaterDiffuseTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, WaterSpecularTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(5.7, -1.0 + y, 0.8));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f));
        CurrentShader->SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, CubeTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        //leftSide tent
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::rotate(identity, GetRadians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, GetRadians(-45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-4.4, -2.2, 0.6));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(3.0f, 6.5f, 0.05f));
        CurrentShader->SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TentTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        //rightSide tent
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::rotate(identity, GetRadians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, GetRadians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-4.4, -2.6, -1.0));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(3.0f, 6.5f, 0.05f));
        CurrentShader->SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TentTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        //backSide tent
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::rotate(identity, GetRadians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::rotate(ModelMatrix, GetRadians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-1.2, -1.6, -5.9));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(4.5f, 4.5f, 0.05f));
        CurrentShader->SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TentTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        //stick
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::rotate(identity, GetRadians(-45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::rotate(ModelMatrix, GetRadians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(4.4, 1.9, -1.2));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 3.0f, 0.1f));
        CurrentShader->SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, CubeDiffuseTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        // NOTE(Jovan): Models have their textures automatically loaded and set (if existent)
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::rotate(identity, GetRadians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(2.1, -1.5, -2.4));
        CurrentShader->SetModel(ModelMatrix);
        Fox.Render();

        DrawFloor(CubeVAO, *CurrentShader, FloorDiffuseTexture, FloorSpecularTexture);

        glUseProgram(ColorShader.GetId());
        ColorShader.SetProjection(Projection);
        ColorShader.SetView(View);
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 1.0f, -2.0f));
        ColorShader.SetModel(ModelMatrix);


        //plavo
        ModelMatrix = glm::translate(identity, glm::vec3(5.7f, -1.0f + y, 0.6f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.25f));
        ColorShader.SetModel(ModelMatrix);
        glBindVertexArray(CubeVAO);
        ColorShader.SetUniform3f("uColor", glm::vec3(0.0f, 0.0f, 1.0f));
        glDrawArrays(GL_TRIANGLES, 0, 36);


        //zuto
        ModelMatrix = glm::translate(identity, glm::vec3(5.9f, -1.0f + y, 0.8));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.25f));
        ColorShader.SetModel(ModelMatrix);
        glBindVertexArray(CubeVAO);
        ColorShader.SetUniform3f("uColor", glm::vec3(1.0f, 1.0f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        //crveno
        ModelMatrix = glm::translate(identity, glm::vec3(5.70f, -1.0f + y, 1.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.25f));
        ColorShader.SetModel(ModelMatrix);
        glBindVertexArray(CubeVAO);
        ColorShader.SetUniform3f("uColor", glm::vec3(1.0, 0.0f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        //zeleno
        ModelMatrix = glm::translate(identity, glm::vec3(5.5f, -1.0f + y, 0.8f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.25f));
        ColorShader.SetModel(ModelMatrix);
        glBindVertexArray(CubeVAO);
        ColorShader.SetUniform3f("uColor", glm::vec3(0.0f, 1.0f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 0, 36);


        //nandzarasta
        ModelMatrix = glm::translate(identity, glm::vec3(5.7f, -0.8f + y, 0.8f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.25f));
        ColorShader.SetModel(ModelMatrix);
        glBindVertexArray(CubeVAO);
        ColorShader.SetUniform3f("uColor", glm::vec3(1.0f, 0.5f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 0, 36);


        ////ljubicasto
        ModelMatrix = glm::translate(identity, glm::vec3(5.7f, -1.2f + y, 0.8f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.25f));
        ColorShader.SetModel(ModelMatrix);
        glBindVertexArray(CubeVAO);
        ColorShader.SetUniform3f("uColor", glm::vec3(0.5f, 0.0f, 0.5f));
        glDrawArrays(GL_TRIANGLES, 0, 36);



        //Svetlofenjer
        ModelMatrix = glm::translate(identity, glm::vec3(0.3f, 0.5f, -3.3f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.25f));
        ColorShader.SetModel(ModelMatrix);
        glBindVertexArray(CubeVAO);
        ColorShader.SetUniform3f("uColor", glm::vec3(1.0f, 1.0f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);
        glUseProgram(0);
        glfwSwapBuffers(Window);

        // NOTE(Jovan): Time management
        EndTime = glfwGetTime();
        float WorkTime = EndTime - StartTime;
        if (WorkTime < TargetFrameTime) {
            int DeltaMS = (int)((TargetFrameTime - WorkTime) * 1000.0f);
            std::this_thread::sleep_for(std::chrono::milliseconds(DeltaMS));
            EndTime = glfwGetTime();
        }
        State.mDT = EndTime - StartTime;
    }

    glfwTerminate();
    return 0;

}
