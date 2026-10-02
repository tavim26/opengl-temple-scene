#ifndef Lighting_hpp
#define Lighting_hpp

#include "GLUtils.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace gps {

    enum class TimeOfDay { Day, Sunset, Night };

    std::vector<std::string> skyboxFacePaths(TimeOfDay timeOfDay);

    class Lighting
    {
    public:
        void init(GLuint shaderProgram);

        TimeOfDay timeOfDay() const;
        void toggleTimeOfDay(TimeOfDay target);

        void toggleFog();
        void changeFogDensity(float delta);

    private:
        GLint uniform(const char* name) const;
        glm::vec3 directionalLightColor() const;
        glm::vec4 fogColor() const;
        glm::mat4 computeLightSpaceTrMatrix() const;
        void uploadTimeOfDayUniforms() const;
        void uploadFogDensity() const;

        GLuint program = 0;
        TimeOfDay currentTimeOfDay = TimeOfDay::Day;
        bool fogEnabled = false;
        float fogDensity = 0.0f;
        float previousFogDensity = 0.0f;
    };

}

#endif /* Lighting_hpp */