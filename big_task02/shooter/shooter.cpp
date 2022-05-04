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
#include <cstdlib>
#include <iostream>

#include <common/shader.hpp>

std::vector<GLfloat> sphere_to_cartesian(GLfloat phi, GLfloat psi, GLfloat radius) {
  phi = phi * glm::pi<GLfloat>() / 180;
  psi = psi * glm::pi<GLfloat>() / 180;
  GLfloat x = radius * glm::sin(psi) * glm::cos(phi);
  GLfloat y = radius * glm::sin(psi) * glm::sin(phi);
  GLfloat z = radius * glm::cos(psi);

  return { x, y, z };
}

void insert_into_vertices(GLfloat* array, int& from_index, std::vector<GLfloat>& vertecies) {
  array[from_index++] = vertecies[0];
  array[from_index++] = vertecies[1];
  array[from_index++] = vertecies[2];
}

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix(){
  return ViewMatrix;
}
glm::mat4 getProjectionMatrix(){
  return ProjectionMatrix;
}

// Initial position : on +Z
glm::vec3 position = glm::vec3( 0, 0, 5 );
// Initial horizontal angle : toward -Z
float horizontalAngle = 3.14f;
// Initial vertical angle : none
float verticalAngle = 0.0f;
// Initial Field of View
float initialFoV = 45.0f;

float speed = 3.0f; // 3 units / second
float mouseSpeed = 0.005f;

glm::vec3 direction;


void computeMatricesFromInputs(){

  // glfwGetTime is called only once, the first time this function is called
  static double lastTime = glfwGetTime();

  // Compute time difference between current and last frame
  double currentTime = glfwGetTime();
  float deltaTime = float(currentTime - lastTime);

  // Get mouse position
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  // Reset mouse position for next frame
  glfwSetCursorPos(window, 1024/2, 768/2);

  // Compute new orientation
  horizontalAngle += mouseSpeed * float(1024/2 - xpos );
  verticalAngle   += mouseSpeed * float( 768/2 - ypos );

  // Direction : Spherical coordinates to Cartesian coordinates conversion
  direction = glm::vec3(
      cos(verticalAngle) * sin(horizontalAngle),
      sin(verticalAngle),
      cos(verticalAngle) * cos(horizontalAngle)
  );

  // Right vector
  glm::vec3 right = glm::vec3(
      sin(horizontalAngle - 3.14f/2.0f),
      0,
      cos(horizontalAngle - 3.14f/2.0f)
  );

  // Up vector
  glm::vec3 up = glm::cross( right, direction );

  // Move forward
  if (glfwGetKey( window, GLFW_KEY_UP ) == GLFW_PRESS){
    position += direction * deltaTime * speed;
  }
  // Move backward
  if (glfwGetKey( window, GLFW_KEY_DOWN ) == GLFW_PRESS){
    position -= direction * deltaTime * speed;
  }
  // Strafe right
  if (glfwGetKey( window, GLFW_KEY_RIGHT ) == GLFW_PRESS){
    position += right * deltaTime * speed;
  }
  // Strafe left
  if (glfwGetKey( window, GLFW_KEY_LEFT ) == GLFW_PRESS){
    position -= right * deltaTime * speed;
  }

  float FoV = initialFoV;// - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires setting up a callback for this. It's a bit too complicated for this beginner's tutorial, so it's disabled instead.
//  ModelMatrix = glm::translate(glm::mat4(), );
  // Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
  ProjectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 100.0f);
  // Camera matrix
  ViewMatrix       = glm::lookAt(
      position,           // Camera is here
      position+direction, // and looks here : at the same position, plus "direction"
      up                  // Head is up (set to 0,-1,0 to look upside-down)
  );

  // For the next frame, the "last time" will be "now"
  lastTime = currentTime;
}

struct Enemy {
  glm::vec3 position;
  GLfloat angle;
  glm::vec3 rotationAxis;
  Enemy(
    const glm::vec3& position,
    GLfloat angle,
    const glm::vec3& rotationAxis) :
      position(position), angle(angle), rotationAxis(rotationAxis) {}
};

Enemy getRandomEnemy() {
  glm::vec3 position = {
      ((GLfloat)(std::rand() % 1000)) / 200.0f,
      ((GLfloat)(std::rand() % 1000)) / 200.0f,
      ((GLfloat)(std::rand() % 1000)) / 200.0f,
  };
  GLfloat angle = ((GLfloat)(std::rand() % 360)) / 360 * glm::pi<GLfloat>();
  GLfloat x = ((GLfloat)(std::rand() % 1000)) / 100.0f;
  GLfloat y = (std::rand() % 1000) / 100.0f;
  GLfloat z = (std::rand() % 1000) / 100.0f;
  GLfloat xyz = x * x + y * y + z * z;
  x /= xyz;
  y /= xyz;
  z /= xyz;
  glm::vec3 rotationAxis = {
      x,
      y,
      z
  };
  return Enemy(position, angle, rotationAxis);
}


int main( void )
{
  // Initialise GLFW
  if( !glfwInit() )
  {
    fprintf( stderr, "Failed to initialize GLFW\n" );
    getchar();
    return -1;
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Open a window and create its OpenGL context
  window = glfwCreateWindow( 1024, 768, "Shooter", NULL, NULL);
  if( window == NULL ){
    fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
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
  GLuint programID = LoadShaders( "TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader" );

  // Get a handle for our "MVP" uniform
  GLuint MatrixID = glGetUniformLocation(programID, "MVP");

  // Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
  glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
  // Camera matrix
  glm::mat4 View       = glm::lookAt(
      glm::vec3(10,10,10),
      glm::vec3(0,-1,-0),
      glm::vec3(0,0,0)
  );
  // Model matrix : an identity matrix (model will be at the origin)
  glm::mat4 Model      = glm::mat4(1.0f);
  // Our ModelViewProjection : multiplication of our 3 matrices
  glm::mat4 MVP        = Projection * View * Model; // Remember, matrix multiplication is the other way around

  // Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
  // A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
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
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,

      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,

      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,

      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,

      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,

      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,

      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,

      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,

      0.822f,  0.569f,  0.201f,
      0.435f,  0.602f,  0.223f,
      0.4f,  0.0f,  0.9f,
  };

  const int PHI_STEPS = 20;
  const int PSI_STEPS = 20;
  const GLfloat RADIUS = 1;

  GLfloat phi_step = 360 / PHI_STEPS;
  GLfloat psi_step = 180 / PSI_STEPS;

  GLfloat verticies[PHI_STEPS * PSI_STEPS * 3 * 3 * 2];
  int current_index = 0;
  for (int cur_psi_step = 0; cur_psi_step < PHI_STEPS; ++cur_psi_step) {
    GLfloat psi = cur_psi_step * psi_step;
    for (int cur_phi_step = 0; cur_phi_step < PSI_STEPS; ++cur_phi_step) {
      GLfloat phi = cur_phi_step * phi_step;
      auto first_vertex = sphere_to_cartesian(phi, psi, RADIUS);
      auto second_vertex = sphere_to_cartesian(phi + phi_step, psi, RADIUS);
      auto third_vertex = sphere_to_cartesian(phi, psi + psi_step, RADIUS);
      insert_into_vertices(verticies, current_index, first_vertex);
      insert_into_vertices(verticies, current_index, second_vertex);
      insert_into_vertices(verticies, current_index, third_vertex);

      first_vertex = sphere_to_cartesian(phi + phi_step, psi, RADIUS);
      second_vertex = sphere_to_cartesian(phi + phi_step, psi + psi_step, RADIUS);
      third_vertex = sphere_to_cartesian(phi, psi + psi_step, RADIUS);
      insert_into_vertices(verticies, current_index, first_vertex);
      insert_into_vertices(verticies, current_index, second_vertex);
      insert_into_vertices(verticies, current_index, third_vertex);
    }
  }

  GLfloat colors[PHI_STEPS * PSI_STEPS * 3 * 3 * 2];

  for (int i = 0; i < PHI_STEPS * PSI_STEPS * 3 * 2; ++i) {
    colors[i * 3] = 0.3f;
    colors[i * 3 + 1] = 0.6f;
    colors[i * 3 + 2] = 0.9f;
  }

  GLuint vertexbuffers[2];
  glGenBuffers(2, vertexbuffers);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(enemy_buffer_data), enemy_buffer_data, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verticies), verticies, GL_STATIC_DRAW);

  GLuint colorbuffers[2];
  glGenBuffers(2, colorbuffers);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffers[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(enemy_color_buffer_data), enemy_color_buffer_data, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffers[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

  std::vector<Enemy> enemies;
  GLfloat CREATE_ENEMY_INTERVAL = 3.0f;
  GLfloat time = glfwGetTime();

  do{
    GLfloat currentTime = glfwGetTime();
    if (currentTime - time >= CREATE_ENEMY_INTERVAL) {
      time = currentTime;
      enemies.push_back(getRandomEnemy());
    }
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use our shader
    glUseProgram(programID);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
//    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[0]);
//    glVertexAttribPointer(
//        0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
//        3,                  // size
//        GL_FLOAT,           // type
//        GL_FALSE,           // normalized?
//        0,                  // stride
//        (void*)0            // array buffer offset
//    );

    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[1]);
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
//    glBindBuffer(GL_ARRAY_BUFFER, colorbuffers[0]);
//    glVertexAttribPointer(
//        1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
//        3,                                // size
//        GL_FLOAT,                         // type
//        GL_FALSE,                         // normalized?
//        0,                                // stride
//        (void*)0                          // array buffer offset
//    );
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffers[1]);
    glVertexAttribPointer(
        1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        3,                                // size
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
    );
//    for (int i = 0; i < 3; ++i) {
//      std::cout << direction[i] << " ";
//    }
//    std::cout << std::endl;

//    MVP = Projection * View * Model;
//    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
//    glDrawArrays(GL_TRIANGLES, 0, 24*3);
    computeMatricesFromInputs();
    Projection = getProjectionMatrix();
    View = getViewMatrix();
//    Model = glm::mat4(1.f);
//    MVP = Projection * View * Model;
//    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

//    for (const auto& enemy : enemies)
    for (int j = 0; j < enemies.size(); ++j)
    {
      // Calculate the model matrix for each object and pass it to shader before drawing
//      if (j == 1) {
//        enemies[j].position += 0.001f * direction;
//      }
//      auto enemy = enemies[j];
//      glm::mat4 model;
//      model = glm::translate(model, enemy.position);
//      Model = glm::rotate(model, enemy.angle, enemy.rotationAxis);
//      // Send our transformation to the currently bound shader,
//      // in the "MVP" uniform
//
//      MVP = Projection * View * Model;
//      glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
//      glDrawArrays(GL_TRIANGLES, 0, 24*3);
    }

//
//    glDrawArrays(GL_TRIANGLES, 0, 24*3); // 24*3 indices starting at 0 -> 24 triangles
      MVP = Projection * View * Model;
      View[0];
      glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, PHI_STEPS * PSI_STEPS * 3 * 3 * 2);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();

  } // Check if the ESC key was pressed or the window was closed
  while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
         glfwWindowShouldClose(window) == 0 );

  // Cleanup VBO and shader
  glDeleteBuffers(2, vertexbuffers);
  glDeleteBuffers(2, colorbuffers);
  glDeleteProgram(programID);
  glDeleteVertexArrays(1, &VertexArrayID);

  // Close OpenGL window and terminate GLFW
  glfwTerminate();

  return 0;
}

