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
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

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
glm::vec3 Camera::position = glm::vec3(-11, -11, -11);
float Camera::horizontalAngle = glm::asin(0.625f);
float Camera::verticalAngle = glm::asin(0.6f);
float Camera::initialFoV = 45.0f;
float Camera::speed = 3.0f; // 3 units / second
float Camera::mouseSpeed = 0.005f;
glm::vec3 Camera::direction = glm::vec3(-0.6f, -0.6f, -0.6f);

const size_t MAX_NUM_LIGHTS = 5;

void Camera::computeMatricesFromInputs() {

    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    auto deltaTime = float(currentTime - lastTime);

    static bool is_first_call = true;
    if (is_first_call) {
        is_first_call = false;
        glfwSetCursorPos(window, 1024.f / 2, 768.f / 2);
    }

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
                ((GLfloat)(std::rand() % 1000)) / 50.0f - 10.f,
                ((GLfloat)(std::rand() % 1000)) / 50.0f - 10.f,
                ((GLfloat)(std::rand() % 1000)) / 50.0f - 10.f,
        };
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
const GLfloat Enemy::CREATION_TIME = 2.5f;

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
    bool is_controlled_;
    explicit Ball(bool is_controlled = false) :
            position_(Camera::position + Camera::direction * (colliderRadius * 2.f)),
            direction_(Camera::direction), is_controlled_(is_controlled) {}
};

const GLfloat Ball::colliderRadius = 0.5f;
const GLfloat Ball::CREATION_TIME = 1.0f;
const GLfloat Ball::speed = 0.01f;

void createBallByKeySpaceAndTime(std::vector<Ball>& balls) {
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        static GLfloat lastTime = glfwGetTime() - Ball::CREATION_TIME;
        GLfloat currentTime = glfwGetTime();
        if (currentTime - lastTime >= Ball::CREATION_TIME) {
            lastTime = currentTime;
            balls.emplace_back();
        }
    }
}

void createControlledBallByKeyEnterAndTime(std::vector<Ball>& balls) {
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        static GLfloat lastTime = glfwGetTime() - Ball::CREATION_TIME;
        GLfloat currentTime = glfwGetTime();
        if (currentTime - lastTime >= Ball::CREATION_TIME) {
            lastTime = currentTime;
            balls.emplace_back(true);
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

void deleteExtraBalls(std::vector<Ball>& balls) {
    while (balls.size() > MAX_NUM_LIGHTS) {
        balls.erase(balls.begin());
    }
}

void getLights(vec3* lights, std::vector<Ball>& balls) {
    for (int i = 0; i < balls.size(); ++i) {
        lights[i] = balls[i].position_;
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

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");

    // Get a handle for our "MVP" uniform
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
    GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
    GLuint NumLightsID = glGetUniformLocation(programID, "num_lights");
    GLuint LightPowerID = glGetUniformLocation(programID, "LightPower");

    // Load textures
    GLuint fireballTexture = loadDDS("fireball_texture.DDS");
    GLuint enemyTexture = loadDDS("rock_texture.DDS");

    // Get a handle for our "myTextureSampler" uniform
    GLuint EnemyTextureID = glGetUniformLocation(programID, "myTextureSampler");
    GLuint BallTextureID = glGetUniformLocation(programID, "myTextureSampler");

    // Read sphere .obj file
    std::vector<glm::vec3> ball_vertices;
    std::vector<glm::vec2> ball_uvs;
    std::vector<glm::vec3> ball_normals;
    assert(loadOBJ("fireball_model.obj", ball_vertices, ball_uvs, ball_normals));

    for (int i = 0; i < ball_normals.size(); ++i) {
        ball_normals[i] *= -1;
    }

    // Read rock .obj file
    std::vector<glm::vec3> enemy_vertices;
    std::vector<glm::vec2> enemy_uvs;
    std::vector<glm::vec3> enemy_normals;
    assert(loadOBJ("rock_model.obj", enemy_vertices, enemy_uvs, enemy_normals));

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

    GLuint vertexBuffers[2];
    glGenBuffers(2, vertexBuffers);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, enemy_vertices.size() * sizeof(glm::vec3), &enemy_vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[1]);
    glBufferData(GL_ARRAY_BUFFER, ball_vertices.size() * sizeof(glm::vec3), &ball_vertices[0], GL_STATIC_DRAW);

    GLuint uvBuffers[2];
    glGenBuffers(2, uvBuffers);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, enemy_uvs.size() * sizeof(glm::vec2), &enemy_uvs[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffers[1]);
    glBufferData(GL_ARRAY_BUFFER, ball_uvs.size() * sizeof(glm::vec2), &ball_uvs[0], GL_STATIC_DRAW);

    GLuint normalBuffers[2];
    glGenBuffers(2, normalBuffers);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, enemy_normals.size() * sizeof(glm::vec3), &enemy_normals[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[1]);
    glBufferData(GL_ARRAY_BUFFER, ball_normals.size() * sizeof(glm::vec3), &ball_normals[0], GL_STATIC_DRAW);

    std::vector<Enemy> enemies;
    std::vector<Ball> balls;

    // Use our shader
    glUseProgram(programID);
    GLuint LightID = glGetUniformLocation(programID, "LightPositions_worldspace");
    vec3 lights[MAX_NUM_LIGHTS];

    do {
        createEnemyByTime(enemies);
        createBallByKeySpaceAndTime(balls);
        createControlledBallByKeyEnterAndTime(balls);
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        deleteExtraBalls(balls);
        getLights(lights, balls);

        glUniform1i(NumLightsID, balls.size());
        glUniform3fv(LightID, MAX_NUM_LIGHTS, &lights[0][0]);
        glUniform1f(LightPowerID, 50.0f);

        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, enemyTexture);
        // Set our "myTextureSampler" sampler to use Texture Unit 0
        glUniform1i(EnemyTextureID, 0);

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

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffers[0]);
        glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                2,                                // size : U+V => 2
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
        );

        // 3rd attribute buffer : Normals
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[0]);
        glVertexAttribPointer(
                2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                3,                                // size : 3
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

            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model[0][0]);
            glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View[0][0]);

            glDrawArrays(GL_TRIANGLES, 0, enemy_vertices.size());
        }
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        //// Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fireballTexture);
        //// Set our "myTextureSampler" sampler to use Texture Unit 0
        glUniform1i(BallTextureID, 0);

        glUniform1f(LightPowerID, 1.0f);

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
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffers[1]);
        glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                2,                                // size : U+V => 2
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
        );

        // 3rd attribute buffer : Normals
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[1]);
        glVertexAttribPointer(
                2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                3,                                // size : 3
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
        );

        for (auto ball_it = balls.begin(); ball_it != balls.end();) {
            if (ball_it->is_controlled_) {
                ball_it->position_ += Camera::direction * Ball::speed;
            }
            else {
                ball_it->position_ += ball_it->direction_ * Ball::speed;
            }
            if (!deleteCollidedObjects(ball_it, balls, enemies)) {
                Model = glm::translate(glm::mat4(), ball_it->position_);
                MVP = Projection * View * Model;

                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
                glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model[0][0]);
                glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View[0][0]);

                glDrawArrays(GL_TRIANGLES, 0, ball_vertices.size());
                ++ball_it;
            }
        }
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        glfwSwapBuffers(window);
        glfwPollEvents();
    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);

    // Cleanup VBO and shader
    glDeleteBuffers(2, vertexBuffers);
    glDeleteBuffers(2, uvBuffers);
    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteTextures(1, &fireballTexture);
    glDeleteTextures(1, &enemyTexture);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

