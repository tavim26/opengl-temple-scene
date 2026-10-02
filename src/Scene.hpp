#ifndef Scene_hpp
#define Scene_hpp

#include "Camera.hpp"
#include "Lighting.hpp"
#include "Model3D.hpp"
#include "RainSystem.hpp"
#include "Shader.hpp"
#include "SkyBox.hpp"

#include <glm/glm.hpp>

#include <array>

namespace gps {

    class Scene
    {
    public:
        Scene();
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        void init(float aspectRatio);
        void release();

        void onKey(int key, int action);
        void onMouseMove(double xpos, double ypos);

        void processMovement();
        void update();
        void render();

    private:
        void setCamera(const glm::vec3& position, const glm::vec3& target);
        void loadSkybox();
        void handleKeyPress(int key);
        void handleKeyHeld(int key);
        void toggleTimeOfDay(TimeOfDay target);
        void toggleCameraAnimation();
        void drawModel(Model3D& model, const glm::mat4& modelMatrix);

        Camera camera;
        float cameraYaw = 0.0f;
        float cameraPitch = 0.0f;
        bool cameraAnimationEnabled = false;

        std::array<bool, 1024> keyStates{};
        bool firstMouseEvent = true;
        double mouseLastX = 0.0;
        double mouseLastY = 0.0;

        Model3D mainSceneModel;
        Model3D carriage;
        glm::vec3 carriagePosition = glm::vec3(0.0f);
        float rotationAngle = 0.0f;

        Shader basicShader;
        Shader skyboxShader;
        Shader rainShader;
        SkyBox skybox;
        Lighting lighting;
        RainSystem rain;
        bool rainEnabled = false;

        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);
        GLint modelMatrixLoc = -1;
        GLint viewMatrixLoc = -1;
        GLint normalMatrixLoc = -1;
    };

}

#endif /* Scene_hpp */