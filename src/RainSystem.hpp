#ifndef RainSystem_hpp
#define RainSystem_hpp

#include "Shader.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace gps {

    class RainSystem
    {
    public:
        void init(std::size_t particleCount);
        void update();
        void render(Shader& shader, const glm::mat4& view, const glm::mat4& projection);
        void release();

    private:
        struct Particle
        {
            glm::vec3 position;
            glm::vec3 velocity;
        };

        static float randomTenths(int min, int max);
        static void respawn(Particle& particle);

        std::vector<Particle> particles;
        GLuint vao = 0;
        GLuint vbo = 0;
    };

}

#endif /* RainSystem_hpp */