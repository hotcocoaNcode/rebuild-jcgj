#include "../../engineconfig.h"
#ifdef GFX_API_OPENGL41
#include <iostream>
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>
#include "gfx_gl41.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../../../stb/stb_image.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

//
// Created by Ember Lee on 3/9/24.
//
unsigned int vaoID;
glm::vec3 ambient(glm::max(AMBIENT_RED - 0.5f, 0.1f), glm::max(AMBIENT_GREEN - 0.5f, 0.1f), glm::max(AMBIENT_BLUE - 0.5f, 0.1f));

unsigned int loadCubemap(std::vector<std::string> faces) {
    stbi_set_flip_vertically_on_load(false);
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cerr << "Failed to load cubemap texture " << faces[i] << "!" << std::endl;
            stbi_image_free(data);
            textureID = 0;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int loadTexture(std::string fileName){
    stbi_set_flip_vertically_on_load(true);
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA-4+nrChannels, width, height, 0, GL_RGBA-4+nrChannels, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture " << fileName << "!" << std::endl;
        texture = 0;
    }
    stbi_image_free(data);
    return texture;
}

void initGFX(GLFWwindow** window){
    stbi_set_flip_vertically_on_load(true);

    if(!glfwInit())
    {
        throw std::runtime_error("OpenGL 4.1: Could not initialize GLFW!");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fucking macOS

    *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, nullptr, nullptr);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    if(*window == nullptr){
        throw std::runtime_error("OpenGL 4.1: Could not open window!");
    }

    glfwMakeContextCurrent(*window);

    // Set up depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Backface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Set up blending
    // UPDATE TRANSPARENCY IS EVIL AND FRAMERATE DROPS TO HELL
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifndef DO_SKYBOX
    glClearColor(AMBIENT_RED, AMBIENT_GREEN, AMBIENT_BLUE, CLEAR_ALPHA);
#endif
    // Vertex Array
    glGenVertexArrays(1, &vaoID); //reserve an ID for our VAO
    glBindVertexArray(vaoID); // bind VAO

    // set up VAO attributes
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    // Set up ImGui
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

// Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(*window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
}

unsigned int loadShader(const std::string file_path, int target){
    // Create the shader
    unsigned int shaderID = glCreateShader(target);

    // Read the shader code
    std::string shaderCode;
    std::ifstream shaderCodeStream(file_path, std::ios::in);
    if (shaderCodeStream.is_open()){
        std::stringstream sstr;
        sstr << shaderCodeStream.rdbuf();
        shaderCode = sstr.str();
        shaderCodeStream.close();
    } else {
        std::cerr << "Couldn't open \"" << file_path << "\", are you sure it exists?" << std::endl;
        return 0;
    }

    if (shaderCode.starts_with("// JE_TRANSLATE\n#version 420")){
        std::cout << "Translating " << file_path << "... (JE_TRANSLATE, Vulkan GLSL 4.2 -> OpenGL GLSL 4.1)" << std::endl;
        std::string tempShaderCode = shaderCode;
        tempShaderCode.replace(0, 28, "#version 410");
        std::vector<std::string> lines;
        std::string line;
        for (int i = 0; i < tempShaderCode.length(); i++){
            char character = tempShaderCode[i];
            if (character != '\n')
                line += character;
            else {
                lines.push_back(line);
                line = "";
            }
        }
        lines.push_back(line);
        shaderCode = "";
        for (int i = 0; i < lines.size(); i++){
            line = lines[i];
            if (line.starts_with("layout") && (line.find("uniform") != std::string::npos)){
                if ((line.find("JE_TRANSLATE") != std::string::npos)){
                    line = lines[++i];
                    do {
                        line.replace(0, 0, "uniform");
                        shaderCode.append(line + "\n");
                        line = lines[++i];
                    } while (!line.starts_with('}'));
                    continue;
                } else {
                    int layoutEnd = line.find("uniform");
                    line.replace(0, layoutEnd, "");
                }
            }
            shaderCode.append(line + "\n");
        }
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Shader
    std::cout << "Compiling " << file_path << "..." << std::endl;
    char const * sourcePointer = shaderCode.c_str();
    glShaderSource(shaderID, 1, &sourcePointer, NULL);
    glCompileShader(shaderID);

    // Check Shader
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> shaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(shaderID, InfoLogLength, NULL, &shaderErrorMessage[0]);
        printf("\e[0;33m%s\e[0m\n", &shaderErrorMessage[0]);
    }

    return shaderID;
}

unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID){
    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Link the program
    std::cout << "Linking program..." << std::endl;
    unsigned int ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    std::cout << "Testing program..." << std::endl;
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> ProgramErrorMessage(InfoLogLength+1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        // \e characters are yellow escape and reset so warnings are different color
        printf("\e[0;33m%s\e[0m\n", &ProgramErrorMessage[0]);
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    std::cout << "Success!" << std::endl;

    return ProgramID;
}

void renderFrame(GLFWwindow **window, glm::mat4 cameraMatrix, glm::vec3 camerapos, glm::vec3 cameradir, float fieldOfViewAngle, std::vector<Renderable> renderables, int w, int h, std::vector<void (*)()> imGuiCalls) {
    glm::vec3 packCamera[2] = {camerapos, cameradir};
    float scaledHeight = h * (1.0f / w);
    float scaledWidth = 1.0f;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    for (auto execute : imGuiCalls){
        execute();
    }

    glm::mat4 projectionMatrix;
    glm::mat4 mvp;

    // According to Khronos.org's wiki page, clearing both buffers
    // will always be faster even while drawing skybox.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    unsigned int currentProgram = 0;
    bool currentDepth = true;

    for (auto renderable : renderables){
        if (renderable.enabled){
            if (renderable.testDepth ^ currentDepth){
                if (renderable.testDepth)
                    glEnable(GL_DEPTH_TEST);
                else
                    glDisable(GL_DEPTH_TEST);

                currentDepth = renderable.testDepth;
            }

            if (renderable.is3d){
                projectionMatrix = glm::perspective(glm::radians(fieldOfViewAngle), (float) w / (float) h, 0.01f, 500.0f);
                mvp = projectionMatrix * cameraMatrix * renderable.objectMatrix();
            } else {
                projectionMatrix = glm::ortho(-scaledWidth,scaledWidth,-scaledHeight,scaledHeight,0.0f,100.0f); // In world coordinates
                #ifdef CAMERA_AFFECTS_2D
                mvp = projectionMatrix * cameraMatrix * renderable.objectMatrix();
                #else
                mvp = projectionMatrix * renderable.objectMatrix();
                #endif
            }

            if (renderable.shaderProgram != currentProgram){
                glUseProgram(renderable.shaderProgram);
                currentProgram = renderable.shaderProgram;
            }

            glm::mat4 packMatrices[4] = {mvp, renderable.transform, renderable.rotate, renderable.scale};

            GLint matrices = glGetUniformLocation(renderable.shaderProgram, "matrices");
            glUniformMatrix4fv(matrices, 4, GL_FALSE, &packMatrices[0][0][0]);

            GLint camera = glGetUniformLocation(renderable.shaderProgram, "cameraProperties");
            glUniform3fv(camera, 1, &packCamera[0][0]);

            GLint amb = glGetUniformLocation(renderable.shaderProgram, "ambience");
            glUniform3fv(amb, 1, &ambient[0]);

            //GLint alphaID = glGetUniformLocation(renderable.shaderProgram, "alpha");
            //glUniform1f(alphaID, renderable.alpha);

            glBindTexture(GL_TEXTURE_2D, renderable.texture);

            glBindBuffer(GL_ARRAY_BUFFER, renderable.vboID);
            glVertexAttribPointer(
                    0,                  // attribute
                    3,                  // size
                    GL_FLOAT,           // type
                    GL_FALSE,           // normalized?
                    0,                  // stride
                    (void*)0            // array buffer offset
            );

            glBindBuffer(GL_ARRAY_BUFFER, renderable.cboID);
            glVertexAttribPointer(
                    1,                                // attribute
                    3,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
            );

            glBindBuffer(GL_ARRAY_BUFFER, renderable.tboID);
            glVertexAttribPointer(
                    2,                                // attribute
                    2,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
            );

            glBindBuffer(GL_ARRAY_BUFFER, renderable.nboID);
            glVertexAttribPointer(
                    3,                                // attribute
                    3,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
            );

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderable.iboID);

            glDrawElements(
                    GL_TRIANGLES,      // mode
                    renderable.indices.size(),    // count
                    GL_UNSIGNED_INT,   // type
                    (void*)0           // element array buffer offset
            );
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(*window);
}

void deinitGFX(GLFWwindow** window){
    glfwDestroyWindow(*window);
    glfwTerminate();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
#endif //GFX_API_OPENGL33