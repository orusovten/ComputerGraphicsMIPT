//
// Created by Тенгис on 22.03.2022.
//

// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <vector>
#include <iostream>

#include <common/shader.hpp>
#include <common/texture.hpp>

std::vector<GLfloat> sphereToCartesian(GLfloat phi, GLfloat psi, GLfloat radius) {
    phi = phi * glm::pi<GLfloat>() / 180;
    psi = psi * glm::pi<GLfloat>() / 180;
    GLfloat x = radius * glm::sin(psi) * glm::cos(phi);
    GLfloat y = radius * glm::sin(psi) * glm::sin(phi);
    GLfloat z = radius * glm::cos(psi);

    return { x, y, z };
}

void insertIntoVertices(GLfloat* array, int& from_index, std::vector<GLfloat>& ballVertices) {
    array[from_index++] = ballVertices[0];
    array[from_index++] = ballVertices[1];
    array[from_index++] = ballVertices[2];
}


struct Camera {
    static glm::mat4 ViewMatrix;
    static glm::mat4 ProjectionMatrix;
    // Initial position : on +Z
    static glm::vec3 position;
    // Initial horizontal angle : toward -Z
    static float horizontalAngle;
    // Initial vertical angle : none
    static float verticalAngle;
    // Initial Field of View
    static float initialFoV;

    static float speed;
    static float mouseSpeed;

    static glm::vec3 direction;

    static void computeMatricesFromInputs();
};

glm::mat4 Camera::ViewMatrix = glm::mat4();
glm::mat4 Camera::ProjectionMatrix = glm::mat4();
glm::vec3 Camera::position = glm::vec3(10, 10, 10);
float Camera::horizontalAngle = 3.14f;
float Camera::verticalAngle = 0.0f;
float Camera::initialFoV = 45.0f;
float Camera::speed = 3.0f; // 3 units / second
float Camera::mouseSpeed = 0.005f;
glm::vec3 Camera::direction = glm::vec3();

void Camera::computeMatricesFromInputs() {

    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    auto deltaTime = float(currentTime - lastTime);

    // Get mouse position
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);

    // Reset mouse position for next frame
    glfwSetCursorPos(window, 1024.f / 2, 768.f / 2);

    // Compute new orientation
    horizontalAngle += mouseSpeed * float(1024.f / 2 - xPos);
    verticalAngle += mouseSpeed * float(768.f / 2 - yPos);

    // Direction : Spherical coordinates to Cartesian coordinates conversion
    direction = glm::vec3(
            cos(verticalAngle) * sin(horizontalAngle),
            sin(verticalAngle),
            cos(verticalAngle) * cos(horizontalAngle)
    );

    // Right vector
    glm::vec3 right = glm::vec3(
            sin(horizontalAngle - 3.14f / 2.0f),
            0,
            cos(horizontalAngle - 3.14f / 2.0f)
    );

    // Up vector
    glm::vec3 up = glm::cross(right, direction);

    // Move forward
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        position += direction * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        position -= direction * deltaTime * speed;
    }
    // Strafe right
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        position += right * deltaTime * speed;
    }
    // Strafe left
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        position -= right * deltaTime * speed;
    }

    float FoV = initialFoV;// - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires setting up a callback for this. It's a bit too complicated for this beginner's tutorial, so it's disabled instead.
    //  ModelMatrix = glm::translate(glm::mat4(), );
    // Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    ProjectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 100.0f);
    // Camera matrix
    ViewMatrix = glm::lookAt(
            position,           // Camera is here
            position + direction, // and looks here : at the same position, plus "direction"
            up                  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // For the next frame, the "last time" will be "now"
    lastTime = currentTime;
}

struct Enemy {
    static const GLfloat colliderRadius;
    static const GLfloat CREATION_TIME;
    glm::vec3 position;
    GLfloat angle;
    glm::vec3 rotationAxis;
    Enemy() {
        position = {
                ((GLfloat)(std::rand() % 1000)) / 200.0f,
                ((GLfloat)(std::rand() % 1000)) / 200.0f,
                ((GLfloat)(std::rand() % 1000)) / 200.0f,
        };
        int is_long = std::rand() % 4;
        if (is_long < 3) {
            position[is_long] *= 2.f;
        }
        angle = ((GLfloat)(std::rand() % 360)) / 360 * glm::pi<GLfloat>();
        GLfloat x = ((GLfloat)(std::rand() % 1000)) / 100.0f;
        GLfloat y = (std::rand() % 1000) / 100.0f;
        GLfloat z = (std::rand() % 1000) / 100.0f;
        rotationAxis = {
                x,
                y,
                z
        };
        glm::normalize(rotationAxis);
    }
};

const GLfloat Enemy::colliderRadius = 1.0f;
const GLfloat Enemy::CREATION_TIME = 3.0f;

void createEnemyByTime(std::vector<Enemy>& enemies) {
    static GLfloat lastTime = glfwGetTime();
    GLfloat currentTime = glfwGetTime();
    if (currentTime - lastTime >= Enemy::CREATION_TIME) {
        lastTime = currentTime;
        enemies.emplace_back();
    }
}

struct Ball {
    static const GLfloat colliderRadius;
    static const GLfloat CREATION_TIME;
    static const GLfloat speed;
    glm::vec3 position_;
    glm::vec3 direction_;
    Ball() :
            position_(Camera::position + Camera::direction * (colliderRadius * 2.f)),
            direction_(Camera::direction) {}
};

const GLfloat Ball::colliderRadius = 0.5f;
const GLfloat Ball::CREATION_TIME = 1.0f;
const GLfloat Ball::speed = 0.01f;

void createBallByKeySpaceAndTime(std::vector<Ball>& balls) {
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        static GLfloat lastTime = glfwGetTime();
        GLfloat currentTime = glfwGetTime();
        if (currentTime - lastTime >= Ball::CREATION_TIME) {
            lastTime = currentTime;
            balls.emplace_back();
        }
    }
}

bool deleteCollidedObjects(std::vector<Ball>::iterator& ball_it,
                           std::vector<Ball>& balls,
                           std::vector<Enemy>& enemies) {
    for (auto enemy_it = enemies.begin(); enemy_it != enemies.end(); ++enemy_it) {
        auto dist_coords = ball_it->position_ - enemy_it->position;
        GLfloat dist = 0.f;
        for (int i = 0; i < 3; ++i) {
            dist += dist_coords[i] * dist_coords[i];
        }
        if (glm::sqrt(dist) <= Enemy::colliderRadius + Ball::colliderRadius) {
            ball_it = balls.erase(ball_it);
            enemy_it = enemies.erase(enemy_it);
            return true;
        }
    }
    return false;
}


void generateSphere(GLfloat* array, GLfloat radius, int phi_steps, int psi_steps) {
    GLfloat phi_step = 360.f / phi_steps;
    GLfloat psi_step = 180.f / psi_steps;

    int current_index = 0;
    for (int cur_psi_step = 0; cur_psi_step < psi_steps; ++cur_psi_step) {
        GLfloat psi = cur_psi_step * psi_step;
        for (int cur_phi_step = 0; cur_phi_step < phi_steps; ++cur_phi_step) {
            GLfloat phi = cur_phi_step * phi_step;
            auto first_vertex = sphereToCartesian(phi, psi, radius);
            auto second_vertex = sphereToCartesian(phi + phi_step, psi, radius);
            auto third_vertex = sphereToCartesian(phi, psi + psi_step, radius);
            insertIntoVertices(array, current_index, first_vertex);
            insertIntoVertices(array, current_index, second_vertex);
            insertIntoVertices(array, current_index, third_vertex);

            first_vertex = sphereToCartesian(phi + phi_step, psi, radius);
            second_vertex = sphereToCartesian(phi + phi_step, psi + psi_step, radius);
            third_vertex = sphereToCartesian(phi, psi + psi_step, radius);
            insertIntoVertices(array, current_index, first_vertex);
            insertIntoVertices(array, current_index, second_vertex);
            insertIntoVertices(array, current_index, third_vertex);
        }
    }
}


int main(void)
{
    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(1024, 768, "Shooter", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint enemyProgramID = LoadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");

    GLuint ballProgramID = LoadShaders("TransformVertexShader2.vertexshader", "TextureFragmentShader.fragmentshader");
    // Get a handle for our "MVP" uniform
    GLuint enemyMatrixID = glGetUniformLocation(enemyProgramID, "MVP");
    GLuint ballMatrixID = glGetUniformLocation(ballProgramID, "MVP");

    // Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    // Camera matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(10, 10, 10),
            glm::vec3(0, 0, 0),
            glm::vec3(0, -1, 0)
    );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around

    // Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
    // A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
    /*static const GLfloat enemy_buffer_data[] = {
        -0.4f,-0.4f,-0.4f,      -0.4f,-0.4f, 0.4f,      -1.0f, -0.0f, 0.0f,
        -0.4f,-0.4f, 0.4f,      -0.4f, 0.4f, 0.4f,      -1.0f, -0.0f, 0.0f,
        -0.4f, 0.4f, 0.4f,      -0.4f, 0.4f, -0.4f,     -1.0f, -0.0f, 0.0f,
        -0.4f, 0.4f, -0.4f,     -0.4f, -0.4f, -0.4f,    -1.0f, -0.0f, 0.0f,
        -0.4f, 0.4f, 0.4f,      0.4f, 0.4f, 0.4f,       0.0f, 1.0f, 0.0f,
        0.4f, 0.4f, 0.4f,       0.4f, 0.4f, -0.4f,      0.0f, 1.0f, 0.0f,
        0.4f, 0.4f, -0.4f       -0.4f, 0.4f, -0.4f,     0.0f, 1.0f, 0.0f,
        -0.4f, 0.4f, -0.4f,     -0.4f, 0.4f, 0.4f,      0.0f, 1.0f, 0.0f,
        -0.4f, 0.4f, 0.4f,      0.4f, 0.4f, 0.4f,       0.0f, 0.0f, 1.0f,
        0.4f, 0.4f, 0.4f,       0.4f, -0.4f, 0.4f,      0.0f, 0.0f, 1.0f,
        0.4f, -0.4f, 0.4f,      -0.4f, -0.4f, 0.4f,     0.0f, 0.0f, 1.0f,
        -0.4f, -0.4f, 0.4f,     -0.4f, 0.4f, 0.4f,      0.0f, 0.0f, 1.0f,
        0.4f, 0.4f, 0.4f,       0.4f, -0.4f, 0.4f,      1.0f, 0.0f, 0.0f,
        0.4f, -0.4f, 0.4f,      0.4f, -0.4f, -0.4f,     1.0f, 0.0f, 0.0f,
        0.4f, -0.4f, -0.4f,     0.4f, 0.4f, -0.4f,      1.0f, 0.0f, 0.0f,
        0.4f, 0.4f, -0.4f,      0.4f, 0.4f, 0.4f,       1.0f, 0.0f, 0.0f,
        0.4f, 0.4f, -0.4f,      0.4f, -0.4f, -0.4f,     0.0f, 0.0f, -1.0f,
        0.4f, -0.4f, -0.4f,     -0.4f, -0.4f, -0.4f,    0.0f, 0.0f, -1.0f,
        -0.4f, -0.4f, -0.4f,    -0.4f, 0.4f, -0.4f,     0.0f, 0.0f, -1.0f,
        -0.4f, 0.4f, -0.4f,     0.4f, 0.4f, -0.4f,      0.0f, 0.0f, -1.0f,
        0.4f, -0.4f, -0.4f,     -0.4f, -0.4f, -0.4f,    0.0f, -1.0f, 0.0f,
        -0.4f, -0.4f, -0.4f,    -0.4f, -0.4f, 0.4f,     0.0f, -1.0f, 0.0f,
        -0.4f, -0.4f, 0.4f,     0.4f, -0.4f, 0.4f,      0.0f, -1.0f, 0.0f,
        0.4f, -0.4f, 0.4f,      0.4f, -0.4f, -0.4f,     0.0f, -1.0f, 0.0f,
    };*/
    static const GLfloat enemy_buffer_data[] = {
            -0.4f,-0.4f,-0.4f,
            -0.4f,-0.4f, 0.4f,
            -1.0f, -0.0f, 0.0f,

            -0.4f,-0.4f, 0.4f,
            -0.4f, 0.4f, 0.4f,
            -1.0f, -0.0f, 0.0f,

            -0.4f, 0.4f, 0.4f,
            -0.4f, 0.4f, -0.4f,
            -1.0f, -0.0f, 0.0f,

            -0.4f, 0.4f, -0.4f,
            -0.4f, -0.4f, -0.4f,
            -1.0f, -0.0f, 0.0f,


            -0.4f, 0.4f, 0.4f,
            0.4f, 0.4f, 0.4f,
            0.0f, 1.0f, 0.0f,

            0.4f, 0.4f, 0.4f,
            0.4f, 0.4f, -0.4f,
            0.0f, 1.0f, 0.0f,

            0.4f, 0.4f, -0.4f,
            -0.4f, 0.4f, -0.4f,
            0.0f, 1.0f, 0.0f,

            -0.4f, 0.4f, -0.4f,
            -0.4f, 0.4f, 0.4f,
            0.0f, 1.0f, 0.0f,


            -0.4f, 0.4f, 0.4f,
            0.4f, 0.4f, 0.4f,
            0.0f, 0.0f, 1.0f,

            0.4f, 0.4f, 0.4f,
            0.4f, -0.4f, 0.4f,
            0.0f, 0.0f, 1.0f,

            0.4f, -0.4f, 0.4f,
            -0.4f, -0.4f, 0.4f,
            0.0f, 0.0f, 1.0f,

            -0.4f, -0.4f, 0.4f,
            -0.4f, 0.4f, 0.4f,
            0.0f, 0.0f, 1.0f,


            0.4f, 0.4f, 0.4f,
            0.4f, -0.4f, 0.4f,
            1.0f, 0.0f, 0.0f,

            0.4f, -0.4f, 0.4f,
            0.4f, -0.4f, -0.4f,
            1.0f, 0.0f, 0.0f,

            0.4f, -0.4f, -0.4f,
            0.4f, 0.4f, -0.4f,
            1.0f, 0.0f, 0.0f,

            0.4f, 0.4f, -0.4f,
            0.4f, 0.4f, 0.4f,
            1.0f, 0.0f, 0.0f,


            0.4f, 0.4f, -0.4f,
            0.4f, -0.4f, -0.4f,
            0.0f, 0.0f, -1.0f,

            0.4f, -0.4f, -0.4f,
            -0.4f, -0.4f, -0.4f,
            0.0f, 0.0f, -1.0f,

            -0.4f, -0.4f, -0.4f,
            -0.4f, 0.4f, -0.4f,
            0.0f, 0.0f, -1.0f,

            -0.4f, 0.4f, -0.4f,
            0.4f, 0.4f, -0.4f,
            0.0f, 0.0f, -1.0f,


            0.4f, -0.4f, -0.4f,
            -0.4f, -0.4f, -0.4f,
            0.0f, -1.0f, 0.0f,

            -0.4f, -0.4f, -0.4f,
            -0.4f, -0.4f, 0.4f,
            0.0f, -1.0f, 0.0f,

            -0.4f, -0.4f, 0.4f,
            0.4f, -0.4f, 0.4f,
            0.0f, -1.0f, 0.0f,

            0.4f, -0.4f, 0.4f,
            0.4f, -0.4f, -0.4f,
            0.0f, -1.0f, 0.0f,
    };

    // One color for each vertex. They were generated randomly.
    static const GLfloat enemy_color_buffer_data[] = {
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,

            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,

            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,

            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,

            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,

            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,

            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,

            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,

            0.822f,     0.569f,     0.201f,     0.435f,     0.602f,     0.223f,     0.4f,       0.0f,       0.9f,
    };

    const int PHI_STEPS = 20;
    const int PSI_STEPS = 20;
    const GLfloat RADIUS = Ball::colliderRadius;

    GLfloat phi_step = 360.f / PHI_STEPS;
    GLfloat psi_step = 180.f / PSI_STEPS;

    GLfloat ballVertexData[PHI_STEPS * PSI_STEPS * 3 * 3 * 2];
    generateSphere(ballVertexData, RADIUS, PHI_STEPS, PSI_STEPS);

    GLuint Texture = loadDDS("fireball.DDS");
    // Get a handle for our "myTextureSampler" uniform
    GLuint TextureID = glGetUniformLocation(enemyProgramID, "myTextureSampler");

    // Generate UV-coordinates for ball
    GLfloat g_uv_buffer_data[PHI_STEPS * PSI_STEPS * 3 * 2 * 2];
    for (int i = 0; i < PHI_STEPS * PSI_STEPS; ++i) {
        g_uv_buffer_data[i * 3 * 2 * 2 + 0] = 0.0f;
        g_uv_buffer_data[i * 3 * 2 * 2 + 1] = 0.0f;

        g_uv_buffer_data[i * 3 * 2 * 2 + 2] = 0.0f;
        g_uv_buffer_data[i * 3 * 2 * 2 + 3] = 0.1f;

        g_uv_buffer_data[i * 3 * 2 * 2 + 4] = 1.0f;
        g_uv_buffer_data[i * 3 * 2 * 2 + 5] = 1.0f;


        g_uv_buffer_data[i * 3 * 2 * 2 + 6] = 0.0f;
        g_uv_buffer_data[i * 3 * 2 * 2 + 7] = 0.0f;

        g_uv_buffer_data[i * 3 * 2 * 2 + 8] = 1.0f;
        g_uv_buffer_data[i * 3 * 2 * 2 + 9] = 0.0f;

        g_uv_buffer_data[i * 3 * 2 * 2 + 10] = 1.0f;
        g_uv_buffer_data[i * 3 * 2 * 2 + 11] = 1.0f;
    }

    GLuint uvBuffer;
    glGenBuffers(1, &uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);

    GLuint vertexBuffers[2];
    glGenBuffers(2, vertexBuffers);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(enemy_buffer_data), enemy_buffer_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ballVertexData), ballVertexData, GL_STATIC_DRAW);

    GLuint colorBuffer;
    glGenBuffers(1, &colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(enemy_color_buffer_data), enemy_color_buffer_data, GL_STATIC_DRAW);

    std::vector<Enemy> enemies;
    std::vector<Ball> balls;

    do {
        createEnemyByTime(enemies);
        createBallByKeySpaceAndTime(balls);
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(enemyProgramID);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
        glVertexAttribPointer(
                0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
        );

        // 2nd attribute buffer : colors
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                3,                                // size
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
        );

        Camera::computeMatricesFromInputs();
        Projection = Camera::ProjectionMatrix;
        View = Camera::ViewMatrix;
        for (const auto& enemy : enemies)
        {
            Model = glm::translate(glm::mat4(), enemy.position);
            Model = glm::rotate(Model, enemy.angle, enemy.rotationAxis);
            // Send our transformation to the currently bound shader,
            // in the "MVP" uniform

            MVP = Projection * View * Model;
            glUniformMatrix4fv(enemyMatrixID, 1, GL_FALSE, &MVP[0][0]);
            glDrawArrays(GL_TRIANGLES, 0, 24 * 3);
        }
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        // Use our shader
        glUseProgram(ballProgramID);

        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture);
        // Set our "myTextureSampler" sampler to use Texture Unit 0
        glUniform1i(TextureID, 0);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[1]);
        glVertexAttribPointer(
                0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                2,                                // size : U+V => 2
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
        );

        for (auto ball_it = balls.begin(); ball_it != balls.end();) {
            ball_it->position_ += ball_it->direction_ * Ball::speed;
            if (!deleteCollidedObjects(ball_it, balls, enemies)) {
                Model = glm::translate(glm::mat4(), ball_it->position_);
                MVP = Projection * View * Model;
                glUniformMatrix4fv(ballMatrixID, 1, GL_FALSE, &MVP[0][0]);
                glDrawArrays(GL_TRIANGLES, 0, PHI_STEPS * PSI_STEPS * 3 * 3 * 2);
                ++ball_it;
            }
        }
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glfwSwapBuffers(window);
        glfwPollEvents();
    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);

    // Cleanup VBO and shader
    glDeleteBuffers(2, vertexBuffers);
    glDeleteBuffers(1, &colorBuffer);
    glDeleteProgram(enemyProgramID);
    glDeleteProgram(ballProgramID);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteTextures(1, &Texture);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

