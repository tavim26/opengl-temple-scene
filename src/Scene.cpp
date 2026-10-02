#include "Scene.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <string>
#include <vector>

namespace gps {

    namespace {
        const glm::vec3 initialCameraPosition(13.0f, 44.0f, -10.0f);
        const glm::vec3 initialCameraTarget(0.0f, 0.0f, 1.0f);
        const glm::vec3 animationCameraPosition(5.0f, 60.0f, -10.0f);
        const glm::vec3 animationCameraTarget(0.0f, 0.0f, 0.0f);
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

        constexpr float cameraSpeed = 0.5f;
        constexpr float mouseSensitivity = 0.2f;
        constexpr float rotationStep = 1.0f;
        constexpr float carriageStep = 0.4f;
        constexpr float fogDensityStep = 0.003f;
        constexpr std::size_t rainParticleCount = 500000;
    }

    Scene::Scene()
        : camera(initialCameraPosition, initialCameraTarget, worldUp)
    {
    }

    void Scene::init(float aspectRatio)
    {
        setCamera(initialCameraPosition, initialCameraTarget);

        mainSceneModel.LoadModel("assets/scene.obj", "assets/");
        carriage.LoadModel("carriage/carriage.obj", "carriage/");
        rain.init(rainParticleCount);
        loadSkybox();

        basicShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
        skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
        rainShader.loadShader("shaders/rain.vert", "shaders/rain.frag");

        basicShader.useShaderProgram();
        modelMatrixLoc = glGetUniformLocation(basicShader.shaderProgram, "model");
        viewMatrixLoc = glGetUniformLocation(basicShader.shaderProgram, "view");
        normalMatrixLoc = glGetUniformLocation(basicShader.shaderProgram, "normalMatrix");

        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);
        glUniformMatrix4fv(glGetUniformLocation(basicShader.shaderProgram, "projection"),
                           1, GL_FALSE, glm::value_ptr(projectionMatrix));

        lighting.init(basicShader.shaderProgram);
    }

    void Scene::release()
    {
        rain.release();
    }

    void Scene::onKey(int key, int action)
    {
        if (key < 0 || key >= static_cast<int>(keyStates.size()))
        {
            return;
        }

        if (action == GLFW_PRESS)
        {
            keyStates[key] = true;
            handleKeyPress(key);
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

    void Scene::onMouseMove(double xpos, double ypos)
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

        camera.rotate(cameraPitch, cameraYaw);
    }

    void Scene::processMovement()
    {
        if (cameraAnimationEnabled)
        {
            rotationAngle += rotationStep;
            return;
        }

        if (keyStates[GLFW_KEY_W]) camera.move(MOVE_FORWARD, cameraSpeed);
        if (keyStates[GLFW_KEY_S]) camera.move(MOVE_BACKWARD, cameraSpeed);
        if (keyStates[GLFW_KEY_A]) camera.move(MOVE_LEFT, cameraSpeed);
        if (keyStates[GLFW_KEY_D]) camera.move(MOVE_RIGHT, cameraSpeed);

        if (keyStates[GLFW_KEY_Q]) rotationAngle -= rotationStep;
        if (keyStates[GLFW_KEY_E]) rotationAngle += rotationStep;

        if (keyStates[GLFW_KEY_UP])    carriagePosition.x += carriageStep;
        if (keyStates[GLFW_KEY_DOWN])  carriagePosition.x -= carriageStep;
        if (keyStates[GLFW_KEY_LEFT])  carriagePosition.z += carriageStep;
        if (keyStates[GLFW_KEY_RIGHT]) carriagePosition.z -= carriageStep;
    }

    void Scene::update()
    {
        if (rainEnabled)
        {
            rain.update();
        }
    }

    void Scene::render()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        viewMatrix = camera.getViewMatrix();

        skybox.Draw(skyboxShader, viewMatrix, projectionMatrix);

        basicShader.useShaderProgram();
        glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

        const glm::mat4 sceneMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), worldUp);
        drawModel(mainSceneModel, sceneMatrix);
        drawModel(carriage, glm::translate(sceneMatrix, carriagePosition));

        if (rainEnabled)
        {
            rain.render(rainShader, viewMatrix, projectionMatrix);
        }
    }

    // Keeps yaw/pitch consistent with the camera, so the first mouse
    // movement continues from the current view instead of snapping.
    void Scene::setCamera(const glm::vec3& position, const glm::vec3& target)
    {
        camera = Camera(position, target, worldUp);

        const glm::vec3 front = glm::normalize(target - position);
        cameraPitch = glm::degrees(std::asin(front.y));
        cameraYaw = glm::degrees(std::atan2(front.z, front.x));
    }

    void Scene::loadSkybox()
    {
        const std::vector<std::string> paths = skyboxFacePaths(lighting.timeOfDay());

        std::vector<const GLchar*> faces;
        for (const std::string& path : paths)
        {
            faces.push_back(path.c_str());
        }

        skybox.Load(faces);
    }

    void Scene::handleKeyPress(int key)
    {
        switch (key)
        {
        case GLFW_KEY_O: toggleTimeOfDay(TimeOfDay::Night);  break;
        case GLFW_KEY_P: toggleTimeOfDay(TimeOfDay::Sunset); break;

        case GLFW_KEY_Z: lighting.toggleFog(); break;
        case GLFW_KEY_R: rainEnabled = !rainEnabled; break;
        case GLFW_KEY_K: toggleCameraAnimation(); break;

        case GLFW_KEY_1: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  break;
        case GLFW_KEY_2: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
        case GLFW_KEY_3: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  break;

        default: break;
        }
    }

    void Scene::handleKeyHeld(int key)
    {
        switch (key)
        {
        case GLFW_KEY_X: lighting.changeFogDensity(fogDensityStep);  break;
        case GLFW_KEY_C: lighting.changeFogDensity(-fogDensityStep); break;
        default: break;
        }
    }

    void Scene::toggleTimeOfDay(TimeOfDay target)
    {
        lighting.toggleTimeOfDay(target);
        loadSkybox();
    }

    void Scene::toggleCameraAnimation()
    {
        cameraAnimationEnabled = !cameraAnimationEnabled;

        if (cameraAnimationEnabled)
        {
            setCamera(animationCameraPosition, animationCameraTarget);
        }
    }

    void Scene::drawModel(Model3D& model, const glm::mat4& modelMatrix)
    {
        const glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(viewMatrix * modelMatrix));

        glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        model.Draw(basicShader);
    }

}