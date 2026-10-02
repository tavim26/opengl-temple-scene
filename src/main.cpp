#if defined(__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "Camera.hpp"
#include "Model3D.hpp"
#include "Shader.hpp"
#include "SkyBox.hpp"

enum class TimeOfDay { Day, Sunset, Night };

struct RainParticle {
    glm::vec3 position;
    glm::vec3 velocity;
};

// window
constexpr int windowWidth = 1920;
constexpr int windowHeight = 1080;
int framebufferWidth = 0;
int framebufferHeight = 0;
GLFWwindow* mainWindow = nullptr;

// input
bool keyStates[GLFW_KEY_LAST + 1] = {};
bool firstMouseEvent = true;
double mouseLastX = 0.0;
double mouseLastY = 0.0;
constexpr float mouseSensitivity = 0.2f;

// camera
const glm::vec3 initialCameraPosition(13.0f, 44.0f, -10.0f);
const glm::vec3 initialCameraTarget(0.0f, 0.0f, 1.0f);
const glm::vec3 animationCameraPosition(5.0f, 60.0f, -10.0f);
const glm::vec3 animationCameraTarget(0.0f, 0.0f, 0.0f);
const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
constexpr float cameraSpeed = 0.5f;

gps::Camera mainCamera(initialCameraPosition, initialCameraTarget, worldUp);
float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
bool cameraAnimationEnabled = false;

// scene
gps::Model3D mainSceneModel;
gps::Model3D carriage;
glm::vec3 carriagePosition(0.0f);
float rotationAngle = 0.0f;
constexpr float rotationStep = 1.0f;
constexpr float carriageStep = 0.4f;

// shaders and skybox
gps::Shader basicShaderProgram;
gps::Shader skyboxShaderProgram;
gps::Shader rainShaderProgram;
gps::SkyBox skyboxModel;

// matrices
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

GLint modelMatrixLoc = -1;
GLint viewMatrixLoc = -1;
GLint projectionMatrixLoc = -1;
GLint normalMatrixLoc = -1;

// lighting
TimeOfDay timeOfDay = TimeOfDay::Day;
const glm::vec3 directionalLightDir(0.0f, 1.0f, 1.0f);
glm::mat4 spotlightRotationMatrix;
const glm::vec3 spotlightPosition(-10.0f, 2.0f, -1.0f);
constexpr float spotlightConstant = 1.0f;
constexpr float spotlightLinear = 0.1f;
constexpr float spotlightQuadratic = 0.1f;

// fog
bool fogEnabled = false;
float currentFogDensity = 0.0f;
float previousFogDensity = 0.0f;
constexpr float fogDensityStep = 0.003f;
constexpr float maxFogDensity = 1.0f;

// rain
constexpr std::size_t rainParticleCount = 500000;
std::vector<RainParticle> rainParticles;
GLuint rainVAO = 0;
GLuint rainVBO = 0;
bool rainEnabled = false;


GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        default:                               error = "UNKNOWN_ERROR"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)


// ---------------------------------------------------------------
// Camera
// ---------------------------------------------------------------

// Keeps yaw/pitch consistent with the camera, so the first mouse
// movement continues from the current view instead of snapping.
void setCamera(const glm::vec3& position, const glm::vec3& target)
{
    mainCamera = gps::Camera(position, target, worldUp);

    const glm::vec3 front = glm::normalize(target - position);
    cameraPitch = glm::degrees(std::asin(front.y));
    cameraYaw = glm::degrees(std::atan2(front.z, front.x));
}


// ---------------------------------------------------------------
// Rain
// ---------------------------------------------------------------

float randomTenths(int min, int max)
{
    return static_cast<float>(std::rand() % (max - min) + min) / 10.0f;
}

void respawnRainParticle(RainParticle& particle)
{
    particle.position = glm::vec3(randomTenths(-500, 500), randomTenths(100, 1100), randomTenths(-500, 500));
}

void initRain()
{
    rainParticles.resize(rainParticleCount);
    for (RainParticle& particle : rainParticles)
    {
        particle.position = glm::vec3(randomTenths(-500, 500), randomTenths(100, 500), randomTenths(-500, 500));
        particle.velocity = glm::vec3(0.0f, -1.0f, 0.0f);
    }

    glGenVertexArrays(1, &rainVAO);
    glGenBuffers(1, &rainVBO);

    glBindVertexArray(rainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferData(GL_ARRAY_BUFFER, rainParticles.size() * sizeof(RainParticle), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RainParticle),
                          reinterpret_cast<void*>(offsetof(RainParticle, position)));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void updateRain()
{
    for (RainParticle& particle : rainParticles)
    {
        particle.position += particle.velocity;
        if (particle.position.y < 0.0f)
        {
            respawnRainParticle(particle);
        }
    }
}

void renderRain()
{
    rainShaderProgram.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(rainShaderProgram.shaderProgram, "view"),
                       1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(rainShaderProgram.shaderProgram, "projection"),
                       1, GL_FALSE, glm::value_ptr(projectionMatrix));

    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rainParticles.size() * sizeof(RainParticle), rainParticles.data());

    glBindVertexArray(rainVAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(rainParticles.size()));
    glBindVertexArray(0);
}


// ---------------------------------------------------------------
// Skybox, lighting and fog
// ---------------------------------------------------------------

void loadSkybox()
{
    std::string suffix;
    switch (timeOfDay)
    {
    case TimeOfDay::Day:    suffix = "";         break;
    case TimeOfDay::Sunset: suffix = "_sunset";  break;
    case TimeOfDay::Night:  suffix = "_night";   break;
    }

    const std::vector<std::string> faceNames = { "negx", "posx", "posy", "negy", "negz", "posz" };

    std::vector<std::string> facePaths;
    for (const std::string& name : faceNames)
    {
        facePaths.push_back("skybox/" + name + suffix + ".jpg");
    }

    std::vector<const GLchar*> faces;
    for (const std::string& path : facePaths)
    {
        faces.push_back(path.c_str());
    }

    skyboxModel.Load(faces);
}

glm::vec3 directionalLightColor()
{
    switch (timeOfDay)
    {
    case TimeOfDay::Night:  return glm::vec3(0.1f, 0.1f, 0.1f);
    case TimeOfDay::Sunset: return glm::vec3(1.0f, 0.459f, 0.1f);
    default:                return glm::vec3(1.0f, 1.0f, 0.96f);
    }
}

glm::vec4 fogColor()
{
    switch (timeOfDay)
    {
    case TimeOfDay::Night:  return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    case TimeOfDay::Sunset: return glm::vec4(1.0f, 0.45f, 0.1f, 1.0f);
    default:                return glm::vec4(1.0f, 1.0f, 0.8f, 1.0f);
    }
}

glm::mat4 computeLightSpaceTrMatrix()
{
    const glm::mat4 lightView = glm::lookAt(
        glm::inverseTranspose(glm::mat3(spotlightRotationMatrix)) * directionalLightDir,
        glm::vec3(0.0f),
        worldUp);

    const float nearPlane = -30.0f;
    const float farPlane = 30.0f;
    const glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, nearPlane, farPlane);

    return lightProjection * lightView;
}

GLint basicUniform(const char* name)
{
    return glGetUniformLocation(basicShaderProgram.shaderProgram, name);
}

void uploadLightingUniforms()
{
    basicShaderProgram.useShaderProgram();
    glUniform3fv(basicUniform("lightColor"), 1, glm::value_ptr(directionalLightColor()));
    glUniform4fv(basicUniform("fogColor"), 1, glm::value_ptr(fogColor()));
}

void uploadFogDensity()
{
    basicShaderProgram.useShaderProgram();
    glUniform1f(basicUniform("fogDensity"), currentFogDensity);
}

void initUniforms()
{
    basicShaderProgram.useShaderProgram();

    modelMatrixLoc = basicUniform("model");
    viewMatrixLoc = basicUniform("view");
    projectionMatrixLoc = basicUniform("projection");
    normalMatrixLoc = basicUniform("normalMatrix");

    projectionMatrix = glm::perspective(glm::radians(45.0f),
                                        static_cast<float>(windowWidth) / static_cast<float>(windowHeight),
                                        0.1f, 1000.0f);
    glUniformMatrix4fv(projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    glUniform3fv(basicUniform("lightDir"), 1, glm::value_ptr(directionalLightDir));

    glUniform1f(basicUniform("constant"), spotlightConstant);
    glUniform1f(basicUniform("linear"), spotlightLinear);
    glUniform1f(basicUniform("quadratic"), spotlightQuadratic);
    glUniform3fv(basicUniform("position"), 1, glm::value_ptr(spotlightPosition));

    glUniformMatrix4fv(basicUniform("lightSpaceTrMatrix"), 1, GL_FALSE,
                       glm::value_ptr(computeLightSpaceTrMatrix()));

    uploadLightingUniforms();
    uploadFogDensity();
}


// ---------------------------------------------------------------
// Settings changed from the keyboard
// ---------------------------------------------------------------

void setTimeOfDay(TimeOfDay newTimeOfDay)
{
    timeOfDay = newTimeOfDay;
    loadSkybox();
    uploadLightingUniforms();
}

void toggleTimeOfDay(TimeOfDay target)
{
    setTimeOfDay(timeOfDay == target ? TimeOfDay::Day : target);
}

void toggleFog()
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

    uploadFogDensity();
}

void changeFogDensity(float delta)
{
    if (!fogEnabled)
    {
        return;
    }

    currentFogDensity = glm::clamp(currentFogDensity + delta, 0.0f, maxFogDensity);
    uploadFogDensity();
}

void toggleCameraAnimation()
{
    cameraAnimationEnabled = !cameraAnimationEnabled;

    if (cameraAnimationEnabled)
    {
        setCamera(animationCameraPosition, animationCameraTarget);
    }
}


// ---------------------------------------------------------------
// Input
// ---------------------------------------------------------------

void handleKeyPress(GLFWwindow* window, int key)
{
    switch (key)
    {
    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;

    case GLFW_KEY_O: toggleTimeOfDay(TimeOfDay::Night);  break;
    case GLFW_KEY_P: toggleTimeOfDay(TimeOfDay::Sunset); break;

    case GLFW_KEY_Z: toggleFog(); break;
    case GLFW_KEY_R: rainEnabled = !rainEnabled; break;
    case GLFW_KEY_K: toggleCameraAnimation(); break;

    case GLFW_KEY_1: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  break;
    case GLFW_KEY_2: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
    case GLFW_KEY_3: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  break;

    default: break;
    }
}

void handleKeyHeld(int key)
{
    switch (key)
    {
    case GLFW_KEY_X: changeFogDensity(fogDensityStep);  break;
    case GLFW_KEY_C: changeFogDensity(-fogDensityStep); break;
    default: break;
    }
}

void keyboardCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (key < 0 || key > GLFW_KEY_LAST)
    {
        return;
    }

    if (action == GLFW_PRESS)
    {
        keyStates[key] = true;
        handleKeyPress(window, key);
    }
    else if (action == GLFW_RELEASE)
    {
        keyStates[key] = false;
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        handleKeyHeld(key);
    }
}

void mouseCallback(GLFWwindow* /*window*/, double xpos, double ypos)
{
    if (firstMouseEvent)
    {
        mouseLastX = xpos;
        mouseLastY = ypos;
        firstMouseEvent = false;
    }

    const double xOffset = xpos - mouseLastX;
    const double yOffset = mouseLastY - ypos;
    mouseLastX = xpos;
    mouseLastY = ypos;

    if (cameraAnimationEnabled)
    {
        return;
    }

    cameraYaw += static_cast<float>(xOffset) * mouseSensitivity;
    cameraPitch = glm::clamp(cameraPitch + static_cast<float>(yOffset) * mouseSensitivity, -89.0f, 89.0f);

    mainCamera.rotate(cameraPitch, cameraYaw);
}

void processMovement()
{
    if (cameraAnimationEnabled)
    {
        rotationAngle += rotationStep;
        return;
    }

    if (keyStates[GLFW_KEY_W]) mainCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    if (keyStates[GLFW_KEY_S]) mainCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    if (keyStates[GLFW_KEY_A]) mainCamera.move(gps::MOVE_LEFT, cameraSpeed);
    if (keyStates[GLFW_KEY_D]) mainCamera.move(gps::MOVE_RIGHT, cameraSpeed);

    if (keyStates[GLFW_KEY_Q]) rotationAngle -= rotationStep;
    if (keyStates[GLFW_KEY_E]) rotationAngle += rotationStep;

    if (keyStates[GLFW_KEY_UP])    carriagePosition.x += carriageStep;
    if (keyStates[GLFW_KEY_DOWN])  carriagePosition.x -= carriageStep;
    if (keyStates[GLFW_KEY_LEFT])  carriagePosition.z += carriageStep;
    if (keyStates[GLFW_KEY_RIGHT]) carriagePosition.z -= carriageStep;
}


// ---------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------

void drawModel(gps::Model3D& model, const glm::mat4& modelMatrix)
{
    const glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * modelMatrix));

    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    model.Draw(basicShaderProgram);
}

void renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    viewMatrix = mainCamera.getViewMatrix();

    skyboxModel.Draw(skyboxShaderProgram, viewMatrix, projectionMatrix);

    basicShaderProgram.useShaderProgram();
    glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    const glm::mat4 sceneMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), worldUp);
    drawModel(mainSceneModel, sceneMatrix);
    drawModel(carriage, glm::translate(sceneMatrix, carriagePosition));

    if (rainEnabled)
    {
        renderRain();
    }
}


// ---------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------

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

    mainWindow = glfwCreateWindow(windowWidth, windowHeight, "OpenGL Temple Scene", nullptr, nullptr);
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
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "ERROR: could not initialize GLEW\n";
        glfwDestroyWindow(mainWindow);
        glfwTerminate();
        return false;
    }
#endif

    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL version supported " << glGetString(GL_VERSION) << std::endl;

    glfwGetFramebufferSize(mainWindow, &framebufferWidth, &framebufferHeight);

    return true;
}

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

void initObjects()
{
    mainSceneModel.LoadModel("assets/scene.obj", "assets/");
    carriage.LoadModel("carriage/carriage.obj", "carriage/");
    initRain();
}

void initShaders()
{
    basicShaderProgram.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
    skyboxShaderProgram.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    rainShaderProgram.loadShader("shaders/rain.vert", "shaders/rain.frag");
}

void cleanup()
{
    glDeleteBuffers(1, &rainVBO);
    glDeleteVertexArrays(1, &rainVAO);

    glfwDestroyWindow(mainWindow);
    glfwTerminate();
}


int main()
{
    if (!initOpenGLWindow())
    {
        return EXIT_FAILURE;
    }

    setCamera(initialCameraPosition, initialCameraTarget);

    initOpenGLState();
    initObjects();
    loadSkybox();
    initShaders();
    initUniforms();

    glCheckError();

    while (!glfwWindowShouldClose(mainWindow))
    {
        processMovement();
        renderScene();

        if (rainEnabled)
        {
            updateRain();
        }

        glfwPollEvents();
        glfwSwapBuffers(mainWindow);
    }

    cleanup();
    return EXIT_SUCCESS;
}