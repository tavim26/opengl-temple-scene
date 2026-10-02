#include "GLUtils.hpp"
#include "Scene.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>

namespace {

    constexpr int windowWidth = 1920;
    constexpr int windowHeight = 1080;

    gps::Scene* sceneOf(GLFWwindow* window)
    {
        return static_cast<gps::Scene*>(glfwGetWindowUserPointer(window));
    }

    void keyboardCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            return;
        }

        sceneOf(window)->onKey(key, action);
    }

    void mouseCallback(GLFWwindow* window, double xpos, double ypos)
    {
        sceneOf(window)->onMouseMove(xpos, ypos);
    }

    GLFWwindow* createWindow()
    {
        if (!glfwInit())
        {
            std::cerr << "ERROR: could not start GLFW3\n";
            return nullptr;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "OpenGL Temple Scene", nullptr, nullptr);
        if (!window)
        {
            std::cerr << "ERROR: could not open window with GLFW3\n";
            glfwTerminate();
            return nullptr;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

#if !defined(__APPLE__)
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
        {
            std::cerr << "ERROR: could not initialize GLEW\n";
            glfwDestroyWindow(window);
            glfwTerminate();
            return nullptr;
        }
#endif

        std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
        std::cout << "OpenGL version supported " << glGetString(GL_VERSION) << std::endl;

        return window;
    }

    void initOpenGLState(GLFWwindow* window)
    {
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glViewport(0, 0, framebufferWidth, framebufferHeight);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        glEnable(GL_FRAMEBUFFER_SRGB);
    }

}

int main()
{
    GLFWwindow* window = createWindow();
    if (!window)
    {
        return EXIT_FAILURE;
    }

    initOpenGLState(window);

    // Scoped so the scene releases its OpenGL resources while the context is still alive.
    {
        gps::Scene scene;
        scene.init(static_cast<float>(windowWidth) / static_cast<float>(windowHeight));

        glfwSetWindowUserPointer(window, &scene);
        glfwSetKeyCallback(window, keyboardCallback);
        glfwSetCursorPosCallback(window, mouseCallback);

        glCheckError();

        while (!glfwWindowShouldClose(window))
        {
            scene.processMovement();
            scene.render();
            scene.update();

            glfwPollEvents();
            glfwSwapBuffers(window);
        }

        glfwSetKeyCallback(window, nullptr);
        glfwSetCursorPosCallback(window, nullptr);
        glfwSetWindowUserPointer(window, nullptr);
        scene.release();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}