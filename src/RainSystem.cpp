#include "RainSystem.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>

namespace gps {

    void RainSystem::init(std::size_t particleCount)
    {
        particles.resize(particleCount);
        for (Particle& particle : particles)
        {
            particle.position = glm::vec3(randomTenths(-500, 500), randomTenths(100, 500), randomTenths(-500, 500));
            particle.velocity = glm::vec3(0.0f, -1.0f, 0.0f);
        }

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle),
                              reinterpret_cast<void*>(offsetof(Particle, position)));
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void RainSystem::update()
    {
        for (Particle& particle : particles)
        {
            particle.position += particle.velocity;
            if (particle.position.y < 0.0f)
            {
                respawn(particle);
            }
        }
    }

    void RainSystem::render(Shader& shader, const glm::mat4& view, const glm::mat4& projection)
    {
        shader.useShaderProgram();

        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "view"),
                           1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "projection"),
                           1, GL_FALSE, glm::value_ptr(projection));

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), particles.data());

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particles.size()));
        glBindVertexArray(0);
    }

    void RainSystem::release()
    {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        vbo = 0;
        vao = 0;
        particles.clear();
    }

    float RainSystem::randomTenths(int min, int max)
    {
        return static_cast<float>(std::rand() % (max - min) + min) / 10.0f;
    }

    void RainSystem::respawn(Particle& particle)
    {
        particle.position = glm::vec3(randomTenths(-500, 500), randomTenths(100, 1100), randomTenths(-500, 500));
    }

}