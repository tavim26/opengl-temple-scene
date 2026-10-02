#include "Lighting.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gps {

    namespace {
        const glm::vec3 directionalLightDir(0.0f, 1.0f, 1.0f);
        const glm::vec3 spotlightPosition(-10.0f, 2.0f, -1.0f);
        constexpr float spotlightConstant = 1.0f;
        constexpr float spotlightLinear = 0.1f;
        constexpr float spotlightQuadratic = 0.1f;
        constexpr float maxFogDensity = 1.0f;
    }

    std::vector<std::string> skyboxFacePaths(TimeOfDay timeOfDay)
    {
        std::string suffix;
        switch (timeOfDay)
        {
        case TimeOfDay::Day:    suffix = "";        break;
        case TimeOfDay::Sunset: suffix = "_sunset"; break;
        case TimeOfDay::Night:  suffix = "_night";  break;
        }

        std::vector<std::string> paths;
        for (const char* face : { "negx", "posx", "posy", "negy", "negz", "posz" })
        {
            paths.push_back(std::string("skybox/") + face + suffix + ".jpg");
        }
        return paths;
    }

    void Lighting::init(GLuint shaderProgram)
    {
        program = shaderProgram;
        glUseProgram(program);

        glUniform3fv(uniform("lightDir"), 1, glm::value_ptr(directionalLightDir));

        glUniform1f(uniform("constant"), spotlightConstant);
        glUniform1f(uniform("linear"), spotlightLinear);
        glUniform1f(uniform("quadratic"), spotlightQuadratic);
        glUniform3fv(uniform("position"), 1, glm::value_ptr(spotlightPosition));

        glUniformMatrix4fv(uniform("lightSpaceTrMatrix"), 1, GL_FALSE,
                           glm::value_ptr(computeLightSpaceTrMatrix()));

        uploadTimeOfDayUniforms();
        uploadFogDensity();
    }

    TimeOfDay Lighting::timeOfDay() const
    {
        return currentTimeOfDay;
    }

    void Lighting::toggleTimeOfDay(TimeOfDay target)
    {
        currentTimeOfDay = (currentTimeOfDay == target) ? TimeOfDay::Day : target;
        uploadTimeOfDayUniforms();
    }

    void Lighting::toggleFog()
    {
        fogEnabled = !fogEnabled;

        if (fogEnabled)
        {
            fogDensity = previousFogDensity;
        }
        else
        {
            previousFogDensity = fogDensity;
            fogDensity = 0.0f;
        }

        uploadFogDensity();
    }

    void Lighting::changeFogDensity(float delta)
    {
        if (!fogEnabled)
        {
            return;
        }

        fogDensity = glm::clamp(fogDensity + delta, 0.0f, maxFogDensity);
        uploadFogDensity();
    }

    GLint Lighting::uniform(const char* name) const
    {
        return glGetUniformLocation(program, name);
    }

    glm::vec3 Lighting::directionalLightColor() const
    {
        switch (currentTimeOfDay)
        {
        case TimeOfDay::Night:  return glm::vec3(0.1f, 0.1f, 0.1f);
        case TimeOfDay::Sunset: return glm::vec3(1.0f, 0.459f, 0.1f);
        default:                return glm::vec3(1.0f, 1.0f, 0.96f);
        }
    }

    glm::vec4 Lighting::fogColor() const
    {
        switch (currentTimeOfDay)
        {
        case TimeOfDay::Night:  return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        case TimeOfDay::Sunset: return glm::vec4(1.0f, 0.45f, 0.1f, 1.0f);
        default:                return glm::vec4(1.0f, 1.0f, 0.8f, 1.0f);
        }
    }

    glm::mat4 Lighting::computeLightSpaceTrMatrix() const
    {
        const glm::mat4 lightView = glm::lookAt(directionalLightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, -30.0f, 30.0f);
        return lightProjection * lightView;
    }

    void Lighting::uploadTimeOfDayUniforms() const
    {
        glUseProgram(program);
        glUniform3fv(uniform("lightColor"), 1, glm::value_ptr(directionalLightColor()));
        glUniform4fv(uniform("fogColor"), 1, glm::value_ptr(fogColor()));
    }

    void Lighting::uploadFogDensity() const
    {
        glUseProgram(program);
        glUniform1f(uniform("fogDensity"), fogDensity);
    }

}