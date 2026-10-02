#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "Model3D.hpp"
#include "SkyBox.hpp"
#include "Shader.hpp"
#include "Camera.hpp"

// camera settings
float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
float mouseLastX = 0.0f;
float mouseLastY = 0.0f;
GLfloat cameraSpeed = 0.5f;
GLboolean keyStates[1024];

gps::Camera mainCamera(
    glm::vec3(13.0f, 44.0f, -10.0f),   // cam poz
    glm::vec3(0.0f, 0.0f, 1.0f),  // look direction
    glm::vec3(0.0f, 1.0f, 0.0f));  // UP vector



//models and shaders
gps::Model3D mainSceneModel;
gps::Shader basicShaderProgram;
GLfloat modelRotationAngle = 0.0f;

gps::Model3D carriage;
glm::vec3 carriagePosition(0.0f, 0.0f, 0.0f); 



// main window
int windowWidth = 1920;
int windowHeight = 1080;
int framebufferWidth, framebufferHeight;
GLFWwindow* mainWindow = NULL;

// matrices
glm::mat4 modelMatrix;
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;
glm::mat3 normalMatrix;

//lightning
glm::vec3 directionalLightDir;  //sursa lumina directionala
glm::vec3 directionalLightColor;

GLint modelMatrixLoc;
GLint viewMatrixLoc;
GLint projectionMatrixLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;


//spotlight settings
glm::mat4 spotlightRotationMatrix;
glm::vec3 spotlightPosition = glm::vec3(-10.0f, 2.0f, -1.0f);   //sursa de lumina spotlight
GLfloat spotlightConstant = 1.0f;
GLfloat spotlightLinear = 0.1f;
GLfloat spotlightQuadratic = 0.1f;

GLuint spotlightColorLoc;
GLuint spotlightConstantLoc;
GLuint spotlightLinearLoc;
GLuint spotlightQuadraticLoc;
GLuint spotlightPositionLoc;

GLfloat spotlightCutoff = 10.0f;
GLfloat spotlightOuterCutoff = 20.0f;


//fog settings
bool fogEnabled = false;
GLfloat currentFogDensity = 0.0f;
GLfloat previousFogDensity = 0.0f;
glm::vec4 fogColor;
GLuint fogDensityLoc;
GLuint fogColorLoc;

//skybox
gps::SkyBox skyboxModel;
gps::Shader skyboxShaderProgram;
bool useSunsetSkybox = false;
bool useNightSkybox = false;

// animation
bool cameraAnimationEnabled = false;

bool nightColor = false;
bool sunsetColor = false;

GLfloat rotationAngle;



struct RainParticle {
    glm::vec3 position;
    glm::vec3 velocity;
};


std::vector<RainParticle> rainParticles;
gps::Shader rainShaderProgram;
bool rainEnabled = false;
int numRainParticles = 10000000;


GLuint rainVAO, rainVBO;




void initRainParticles(int numParticles) 
{
    rainParticles.clear();
    for (int i = 0; i < numParticles; i++) 
    {

        RainParticle particle;

        particle.position = glm::vec3(
            ((rand() % 1000) - 500) / 10.0f, 
            ((rand() % 400) + 100) / 10.0f,  
            ((rand() % 1000) - 500) / 10.0f  
        );



        particle.velocity = glm::vec3(0.0f, -1.0f, 0.0f);
        rainParticles.push_back(particle);
    }

    // Actualizează bufferul
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferData(GL_ARRAY_BUFFER, numParticles * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
}



void updateRainParticles()
{
    for (auto& particle : rainParticles) 
    {
        particle.position += particle.velocity;

       
        if (particle.position.y < 0.0f) {
            particle.position.y = ((rand() % 1000) + 100) / 10.0f; 
            particle.position.x = ((rand() % 1000) - 500) / 10.0f; 
            particle.position.z = ((rand() % 1000) - 500) / 10.0f; 
        }

    }
}



void initRainBuffers() 
{
    glGenVertexArrays(1, &rainVAO);
    glGenBuffers(1, &rainVBO);

    glBindVertexArray(rainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferData(GL_ARRAY_BUFFER, rainParticles.size() * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}



void renderRain() 
{
    rainShaderProgram.useShaderProgram();

    
    GLint viewLoc = glGetUniformLocation(rainShaderProgram.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    GLint projectionLoc = glGetUniformLocation(rainShaderProgram.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rainParticles.size() * sizeof(glm::vec3), &rainParticles[0].position);

    
    glBindVertexArray(rainVAO);
    glDrawArrays(GL_POINTS, 0, rainParticles.size());
    glBindVertexArray(0);
}




// OpenGL error checking
GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }

        std::cout << error << " | " << file << " (" << line << ")" << std::endl;

    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)




void initSkybox() 
{
    std::vector<const GLchar*> faces;


    if (!nightColor and !sunsetColor) 
    {
        faces.push_back("skybox/negx.jpg");
        faces.push_back("skybox/posx.jpg");
        faces.push_back("skybox/posy.jpg");
        faces.push_back("skybox/negy.jpg");
        faces.push_back("skybox/negz.jpg");
        faces.push_back("skybox/posz.jpg");
    }
    //night
    else if (nightColor and !sunsetColor)
    {
        faces.push_back("skybox/negx_night.jpg");
        faces.push_back("skybox/posx_night.jpg");
        faces.push_back("skybox/posy_night.jpg");
        faces.push_back("skybox/negy_night.jpg");
        faces.push_back("skybox/negz_night.jpg");
        faces.push_back("skybox/posz_night.jpg");

    }
    //sunset
    else if (!nightColor and sunsetColor)
    {
        faces.push_back("skybox/negx_sunset.jpg");
        faces.push_back("skybox/posx_sunset.jpg");
        faces.push_back("skybox/posy_sunset.jpg");
        faces.push_back("skybox/negy_sunset.jpg");
        faces.push_back("skybox/negz_sunset.jpg");
        faces.push_back("skybox/posz_sunset.jpg");
    }
 
    skyboxModel.Load(faces);
}



glm::mat4 computeLightSpaceTrMatrix()
{
    glm::mat4 lightView = glm::lookAt(glm::inverseTranspose(glm::mat3(spotlightRotationMatrix)) * directionalLightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = -30.0f, far_plane = 30.0f;
    glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

    return lightSpaceTrMatrix;
}


void initModelMatrix() 
{
    modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrixLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "model");
}

void initViewMatrix() 
{
    viewMatrix = mainCamera.getViewMatrix();
    viewMatrixLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "view");
    glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
}

void initNormalMatrix() 
{
    normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * modelMatrix));
    normalMatrixLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "normalMatrix");
}

void initProjectionMatrix() 
{
    projectionMatrix = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f);
    projectionMatrixLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "projection");
    glUniformMatrix4fv(projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
}

void initDirectionalLight() 
{
    directionalLightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    lightDirLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "lightDir");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(directionalLightDir));

    if (nightColor) 
    {
        directionalLightColor = glm::vec3(0.1f, 0.1f, 0.1f);
    }
    else if (sunsetColor)
    {
        directionalLightColor = glm::vec3(1.0f, 0.459f, 0.1f);
    }
    else
    {
        directionalLightColor = glm::vec3(1.0f, 1.0f, 0.96f);
    }

    lightColorLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(directionalLightColor));
}

void initFog() 
{
    fogDensityLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "fogDensity");
    glUniform1f(fogDensityLoc, currentFogDensity);

    if (nightColor) 
    {
        fogColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (sunsetColor)
    {
        fogColor = glm::vec4(1.0f, 0.45f, 0.1f, 1.0f);
    }
    else 
    {
        fogColor = glm::vec4(1.0f, 1.0f, 0.8f, 1.0f);
    }

    fogColorLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "fogColor");
    glUniform4fv(fogColorLoc, 1, glm::value_ptr(fogColor));
}

void initSpotlight() 
{
    spotlightConstantLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "constant");
    glUniform1f(spotlightConstantLoc, spotlightConstant);

    spotlightLinearLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "linear");
    glUniform1f(spotlightLinearLoc, spotlightLinear);

    spotlightQuadraticLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "quadratic");
    glUniform1f(spotlightQuadraticLoc, spotlightQuadratic);

    spotlightPositionLoc = glGetUniformLocation(basicShaderProgram.shaderProgram, "position");
    glUniform3fv(spotlightPositionLoc, 1, glm::value_ptr(spotlightPosition));
}

void initLightSpaceMatrix() 
{
    glUniformMatrix4fv(glGetUniformLocation(basicShaderProgram.shaderProgram, "lightSpaceTrMatrix"),1,GL_FALSE,glm::value_ptr(computeLightSpaceTrMatrix()));
}

void initUniforms() 
{
    basicShaderProgram.useShaderProgram();

    initModelMatrix();
    initViewMatrix();
    initNormalMatrix();
    initProjectionMatrix();
    initDirectionalLight();
    initFog();
    initSpotlight();
    initLightSpaceMatrix();
}


void updateViewAndNormalMatrix()
{
    viewMatrix = mainCamera.getViewMatrix();
    basicShaderProgram.useShaderProgram();
    glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * modelMatrix));
}

void updateModelAndNormalMatrix()
{
    modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0, 1, 0));
    normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * modelMatrix));
}

void updateCarriageNormalMatrix()
{
    glm::mat4 carriageModelMatrix = glm::translate(glm::mat4(1.0f), carriagePosition);
    normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * carriageModelMatrix));
}


void updateCarriageTransform() 
{
    glm::mat4 carriageModelMatrix = glm::mat4(1.0f);
  
    carriageModelMatrix = glm::rotate(carriageModelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    
    carriageModelMatrix = glm::translate(carriageModelMatrix, carriagePosition);
    
    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(carriageModelMatrix));
    
    normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * carriageModelMatrix));
}






void renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    skyboxModel.Draw(skyboxShaderProgram, viewMatrix, projectionMatrix);

    basicShaderProgram.useShaderProgram();

    modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    mainSceneModel.Draw(basicShaderProgram);


    updateCarriageTransform();
    carriage.Draw(basicShaderProgram);

    if (rainEnabled) {
        renderRain();
    }




}



void processKeyInputs() 
{


    if (keyStates[GLFW_KEY_O])
    {
        nightColor = !nightColor;
        sunsetColor = false;
    }
    if (keyStates[GLFW_KEY_P])
    {
        sunsetColor = !sunsetColor;
        nightColor = false;
    }

   
    // Fog controls
    if (keyStates[GLFW_KEY_Z]) 
    {

        fogEnabled = !fogEnabled;

        if (fogEnabled) 
        {
            currentFogDensity = previousFogDensity;
        }
        else 
        {
            previousFogDensity = currentFogDensity;
            currentFogDensity = 0.0f;
        }
    }

    if (keyStates[GLFW_KEY_X]) 
    {

        if (fogEnabled && currentFogDensity < 1.003f) 
        {
            currentFogDensity += 0.003f;
        }
    }

    if (keyStates[GLFW_KEY_C]) 
    {

        if (fogEnabled && currentFogDensity > 0.003f) 
        {
            currentFogDensity -= 0.003;
        }
    }

    //rain

    if (keyStates[GLFW_KEY_R])
    {
        rainEnabled = !rainEnabled;
    }
   


    // Camera animation
    if (keyStates[GLFW_KEY_K]) 
    {

        cameraAnimationEnabled = !cameraAnimationEnabled;

        if (cameraAnimationEnabled) 
        {
            gps::Camera animationCamera(
                glm::vec3(5.0f, 60.0f, -10.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f));

            mainCamera = animationCamera;
            viewMatrix = mainCamera.getViewMatrix();

            glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
            normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * modelMatrix));

           // updateCarriageTransform();
        }
    }


    // Polygon rendering modes
    if (keyStates[GLFW_KEY_1]) 
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  //mod wireframe
    }

    if (keyStates[GLFW_KEY_2]) 
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); //mod cu puncte
    }

    if (keyStates[GLFW_KEY_3]) 
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  //mod solid (fill)
    }
}










void processMovement() 
{
    if (!cameraAnimationEnabled) 
    {
        for (int key = 0; key < 1024; ++key) 
        {
            switch (key) 
            {
            case GLFW_KEY_W:

                if (keyStates[key]) 
                {
                    mainCamera.move(gps::MOVE_FORWARD, cameraSpeed);
                    updateViewAndNormalMatrix();
                    updateCarriageNormalMatrix();
                }
                break;

            case GLFW_KEY_S:

                if (keyStates[key]) 
                {
                    mainCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
                    updateViewAndNormalMatrix();
                    updateCarriageNormalMatrix();
                }
                break;

            case GLFW_KEY_A:

                if (keyStates[key]) 
                {
                    mainCamera.move(gps::MOVE_LEFT, cameraSpeed);
                    updateViewAndNormalMatrix();
                    updateCarriageNormalMatrix();
                }
                break;

            case GLFW_KEY_D:
                if (keyStates[key]) 
                {
                    mainCamera.move(gps::MOVE_RIGHT, cameraSpeed);
                    updateViewAndNormalMatrix();
                    
                }
                break;

            case GLFW_KEY_Q:

                if (keyStates[key]) 
                {
                    rotationAngle -= 1.0f;
                    updateModelAndNormalMatrix();
                    updateCarriageTransform();
                   
                }
                break;

            case GLFW_KEY_E:

                if (keyStates[key]) 
                {
                    rotationAngle += 1.0f;
                    updateModelAndNormalMatrix();
                    updateCarriageTransform();
                 
                }
                break;




               
            case GLFW_KEY_UP: 
                if (keyStates[key])
                {
                    carriagePosition.x += 0.4f;
                }
                break;
            case GLFW_KEY_DOWN: 
                if (keyStates[key])
                {
                    carriagePosition.x -= 0.4f;
                }
                break;
            case GLFW_KEY_LEFT: 
                if (keyStates[key])
                {
                    carriagePosition.z += 0.4f;
                }
                break;
            case GLFW_KEY_RIGHT: 
                if (keyStates[key])
                {
                    carriagePosition.z -= 0.4f;
                }
                break;

            default:
                break;
            }
        }
    }
    else 
    {
        rotationAngle += 1.0f;
        updateModelAndNormalMatrix();
        updateCarriageTransform();
    }
}




// Callback for keyboard
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }


    if (key >= 0 && key < 1024)
    {

        if (action == GLFW_PRESS)
        {
            keyStates[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            keyStates[key] = false;
        }
    }

    processKeyInputs();
}


//Callback function for mouse
void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!cameraAnimationEnabled) 
    {
        
        double xOffset = xpos - mouseLastX;
        double yOffset = mouseLastY - ypos;
        mouseLastX = xpos;
        mouseLastY = ypos;

  
        double mouseSensitivity = 0.2f;
        xOffset *= mouseSensitivity;
        yOffset *= mouseSensitivity;

    
        cameraYaw += xOffset;
        cameraPitch += yOffset;

 
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;

    
        mainCamera.rotate(cameraPitch, cameraYaw);
        viewMatrix = mainCamera.getViewMatrix();
        glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

        
        normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * modelMatrix));
    }
}


// Initialize OpenGL window
bool initOpenGLWindow()
{
    if (!glfwInit())
    {
        std::cerr << "ERROR: could not start GLFW3\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    mainWindow = glfwCreateWindow(windowWidth, windowHeight, "My Scene", NULL, NULL);
    if (!mainWindow)
    {
        std::cerr << "ERROR: could not open window with GLFW3\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(mainWindow);
    glfwSwapInterval(1);
    glfwSetKeyCallback(mainWindow, keyboardCallback);
    glfwSetCursorPosCallback(mainWindow, mouseCallback);

#if !defined(__APPLE__)
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported " << version << std::endl;

    glfwGetFramebufferSize(mainWindow, &framebufferWidth, &framebufferHeight);

    return true;
}


// Initialize OpenGL state
void initOpenGLState()
{
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_FRAMEBUFFER_SRGB);
}



// Initialize scene objects
void initObjects()
{
    mainSceneModel.LoadModel("assets/scene.obj", "assets/");

    carriage.LoadModel("carriage/carriage.obj", "carriage/");


    initRainParticles(1000000); 
    initRainBuffers();

}

// Initialize shaders
void initShaders()
{
    basicShaderProgram.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
    basicShaderProgram.useShaderProgram();

    skyboxShaderProgram.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShaderProgram.useShaderProgram();

    rainShaderProgram.loadShader("shaders/rain.vert", "shaders/rain.frag");
    rainShaderProgram.useShaderProgram();
}



void cleanup() 
{

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glfwDestroyWindow(mainWindow);
    glfwTerminate();
}


void helper()
{
    if (keyStates[GLFW_KEY_Z] || keyStates[GLFW_KEY_X] || keyStates[GLFW_KEY_C] || keyStates[GLFW_KEY_O] || keyStates[GLFW_KEY_P] || keyStates[GLFW_KEY_R])
    {

        initUniforms();
        initSkybox();
        renderScene();
    }
}



int main(int argc, const char* argv[]) 
{

    try {

        initOpenGLWindow();
    }
    catch (const std::exception& e) {

        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initObjects();
    initSkybox();
    initShaders();
    initUniforms();
    currentFogDensity = 0.0f;

    glfwSetKeyCallback(mainWindow, keyboardCallback);
    glfwSetCursorPosCallback(mainWindow, mouseCallback);

    glCheckError();
    
    while (!glfwWindowShouldClose(mainWindow))
    {
        processMovement();
        renderScene();

        helper();

        if (rainEnabled) 
        {
            updateRainParticles();
        }


        glfwPollEvents();
        glfwSwapBuffers(mainWindow);

    }

    cleanup();

}