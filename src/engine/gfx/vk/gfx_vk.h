//
// Created by Ember Lee on 3/9/24.
//

#include "../../engineconfig.h"
#ifdef GFX_API_VK
#ifndef JOSHENGINE_GFX_VK_H
#define JOSHENGINE_GFX_VK_H

#include "../renderable.h"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// VK_SHADER_STAGE_VERTEX_BIT
#define JE_VERTEX_SHADER 0x00000001
// VK_SHADER_STAGE_FRAGMENT_BIT
#define JE_FRAGMENT_SHADER 0x00000010

#define MAX_FRAMES_IN_FLIGHT 2

struct JEPushConstants_VK {
    glm::mat4 model;
    glm::mat4 normal;
};

struct JEUniformBufferObject_VK {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 _2dProj;
    alignas(16) glm::mat4 _3dProj;
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::vec3 cameraDir;
    alignas(16) glm::vec3 sunDir;
    alignas(16) glm::vec3 sunPos;
    alignas(16) glm::vec3 ambience;
};

void initGFX(GLFWwindow** window);
void renderFrame(glm::vec3 camerapos, glm::vec3 cameradir, glm::vec3 sundir, glm::vec3 suncol, glm::vec3 ambient, glm::mat4 cameraMatrix,  glm::mat4 _2dProj, glm::mat4 _3dProj, std::vector<Renderable> renderables, std::vector<void (*)()> imGuiCalls);
void deinitGFX();
unsigned int loadTexture(std::string fileName);
unsigned int loadShader(const std::string file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, bool testDepth);
unsigned int loadCubemap(std::vector<std::string> faces);
void resizeViewport(int w, int h);
unsigned int createVBO(Renderable* r);

#endif //JOSHENGINE_GFX_VK_H
#endif //GFX_API_VK