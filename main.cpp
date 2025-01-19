#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include "include/glm/fwd.hpp"
#include "include/shader.h"
#include "include/glm/glm.hpp"
#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtc/type_ptr.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>
#include <fstream>
#include <iterator>
#include <vector>

#define GLFW_KEY_SPACE 32
#define _USE_MATH_DEFINES

GLFWwindow* init();
void physics_update();
std::vector<glm::vec3> player_collision();
void player_movement(GLFWwindow *window, glm::vec3 wishDir);
bool is_grounded();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void set_uniform_vec3(const Shader& shader, const GLchar* name, const glm::vec3& vec);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

constexpr float g = 3.5f;
const float friction = 0.7f;
const float maxGroundSpeed = 10.0f;
const float maxAirSpeed = 1.0f;
const float acceleration = 5.0f;
// const float airAcceleration = 0.1f;

glm::vec3 playerVel;

glm::vec3 playerSize = glm::vec3(0.65f, 0.6f, 0.65f);
glm::vec3 platformScale = glm::vec3(0.4f, 0.1f, 0.4f);

glm::vec3 spawn;
float lowestPlatform;
float totalTime;

// camera init
glm::vec3 cameraPos = glm::vec3(0.0f, playerSize.y, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec3 lightPos = glm::vec3(0.0f, 10.0f, 0.0f);

float fov = 90.0f;
float pitch, yaw = -90.0f;
float lastX = SCR_WIDTH / 2.0f, lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

std::vector<glm::vec3> platformPositions;
float cube[] = {
  -1, -1, -1,  0.0f,  0.0f, -1.0f,
  1, -1, -1,  0.0f,  0.0f, -1.0f,
  1,  1, -1,  0.0f,  0.0f, -1.0f,
  1,  1, -1,  0.0f,  0.0f, -1.0f,
  -1,  1, -1,  0.0f,  0.0f, -1.0f,
  -1, -1, -1,  0.0f,  0.0f, -1.0f,

  -1, -1,  1,  0.0f,  0.0f,  1.0f,
  1, -1,  1,  0.0f,  0.0f,  1.0f,
  1,  1,  1,  0.0f,  0.0f,  1.0f,
  1,  1,  1,  0.0f,  0.0f,  1.0f,
  -1,  1,  1,  0.0f,  0.0f,  1.0f,
  -1, -1,  1,  0.0f,  0.0f,  1.0f,

  -1,  1,  1, -1.0f,  0.0f,  0.0f,
  -1,  1, -1, -1.0f,  0.0f,  0.0f,
  -1, -1, -1, -1.0f,  0.0f,  0.0f,
  -1, -1, -1, -1.0f,  0.0f,  0.0f,
  -1, -1,  1, -1.0f,  0.0f,  0.0f,
  -1,  1,  1, -1.0f,  0.0f,  0.0f,

  1,  1,  1,  1.0f,  0.0f,  0.0f,
  1,  1, -1,  1.0f,  0.0f,  0.0f,
  1, -1, -1,  1.0f,  0.0f,  0.0f,
  1, -1, -1,  1.0f,  0.0f,  0.0f,
  1, -1,  1,  1.0f,  0.0f,  0.0f,
  1,  1,  1,  1.0f,  0.0f,  0.0f,

  -1, -1, -1,  0.0f, -1.0f,  0.0f,
  1, -1, -1,  0.0f, -1.0f,  0.0f,
  1, -1,  1,  0.0f, -1.0f,  0.0f,
  1, -1,  1,  0.0f, -1.0f,  0.0f,
  -1, -1,  1,  0.0f, -1.0f,  0.0f,
  -1, -1, -1,  0.0f, -1.0f,  0.0f,

  -1,  1, -1,  0.0f,  1.0f,  0.0f,
  1,  1, -1,  0.0f,  1.0f,  0.0f,
  1,  1,  1,  0.0f,  1.0f,  0.0f,
  1,  1,  1,  0.0f,  1.0f,  0.0f,
  -1,  1,  1,  0.0f,  1.0f,  0.0f,
  -1,  1, -1,  0.0f,  1.0f,  0.0f
};



int main() {
  GLFWwindow* window = init();
  if (window == NULL) { return -1; }

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, mouse_callback);

  std::filesystem::path resourcePath = std::filesystem::current_path().parent_path() / "src/include";
  Shader shader(resourcePath / "vertex_shader.txt", resourcePath / "fragment_shader.txt");

  std::random_device rd; // Seed for random number generator
  std::mt19937 gen(rd()); // Standard mersenne_twister_engine
  std::uniform_real_distribution<float> rDis(5.0f, 20.0f);
  std::uniform_real_distribution<float> thetaLengthDis(M_PI/2, 2*M_PI);

  float dcInc = 1.0f;

  float sumC = 0.0f;
  float maxC = 300.0f;
  float dc = 8.0f;
  float r = rDis(gen);
  float theta = 0.0f;
  glm::vec3 pivot = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 pos;
  float y = 1.0f;
  int rot = 1;
  float dtheta = dc/(2.0f*r);
  spawn = (pivot + glm::vec3(r*cos(theta+rot*dtheta), y, r*sin(theta+rot*dtheta)))*platformScale + glm::vec3(0.0f, playerSize.y + 1.0f, 0.0f);
  while (sumC + dc < maxC) {
    float thetaLength = thetaLengthDis(gen);
    float startTheta = theta;
    for (theta += rot*dtheta; abs(startTheta - theta) <= thetaLength && sumC + dc <= maxC; theta += rot*dtheta) {
      pos = pivot + glm::vec3(r*cos(theta), 0, r*sin(theta));
      y -= 2.0f;
      platformPositions.push_back(pos + glm::vec3(0.0f, y, 0.0f));
      sumC += dc;
      dc += dcInc;
      dtheta = dc/(2.0f*r);
    }

    r = rDis(gen);

    pivot = pos + r*glm::normalize(pos - pivot);
    theta -= rot*((dc-dcInc)/(2.0f*r));
    theta = theta < M_PI ? theta + M_PI : theta - M_PI;
    rot = -rot;
  }

  lowestPlatform = y*platformScale.y;
  cameraPos = spawn;
  // float dtheta =  PI/16.0f;

  // const size_t object_size = 6;

  // size_t row;
  // size_t col;
  // std::string buffer;
  // std::ifstream map_file(resourcePath / "map.txt");

  // while (std::getline(map_file, buffer)) {
  //   for (col = 0; col < buffer.size() && buffer[col] != '\n'; ++col) {
  //     if (buffer[col] == '1')
  //       voxelPositions.push_back(glm::vec3(col * 2, voxelScale.y / 2.0f, row * 2));
  //   }
  //   ++row;
  // }

  // cameraPos = glm::vec3(col*voxelScale.x, 1, row*voxelScale.z);
  // lightPos = glm::vec3(col*voxelScale.x, 1, row*voxelScale.z);



  // float vertices[vertex_vec.size()];
  // copy(vertex_vec.begin(), vertex_vec.end(), vertices);

  // unsigned int indices[] = {
  //   0, 1, 3,   // first triangle
  //   1, 2, 3,    // second triangle
  //   0+4, 1+4, 3+4,   // first triangle
  //   1+4, 2+4, 3+4,    // second triangle
  //   0+4*2, 1+4*2, 3+4*2,   // first triangle
  //   1+4*2, 2+4*2, 3+4*2,    // second triangle
  //   0+4*3, 1+4*3, 3+4*3,   // first triangle
  //   1+4*3, 2+4*3, 3+4*3,    // second triangle
  // };


  unsigned int VBO, VAO/*, EBO*/;
  glGenBuffers(1, &VBO);
  // glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  // bind Vertex Array Object
  glBindVertexArray(VAO);

  // bopy vertices array in buffer for OpenGL to use
  glBindBuffer(GL_ARRAY_BUFFER, VBO); 
  glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

  // bopy indices into buffer
  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // set the vertex attributes pointers

  // how to interpret vertex data buffer
  //
  // using 0 in 1st param because that 
  // is where I set the location of the position attribute
  //
  // position: vec3
  // position values: floats, not normalized
  // start: 0
  //
  // position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // normal attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // note that this is allowed, the call to glVertexAttribPointer 
  // registered VBO as the vertex attribute's bound vertex buffer 
  // object so afterwards we can safely unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // you can unbind the VAO afterwards so other VAO calls 
  // won't accidentally modify this VAO, but this rarely happens
  //
  // VAOs requires a call to glBindVertexArray anyways so 
  // generally don't unbind VAOs (nor VBOs) when it's not necessary
  glBindVertexArray(0);

  // render loop
  while(!glfwWindowShouldClose(window)) {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    totalTime += deltaTime;

    // input
    process_input(window);
    physics_update();

    // rendering commands
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // activate shader
    shader.use();

    glm::vec3 objectColor = glm::vec3(1.0f, 0.5f, 0.31f);
    set_uniform_vec3(shader, "objectColor", objectColor);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    set_uniform_vec3(shader, "lightColor", lightColor);
    set_uniform_vec3(shader, "lightPos", lightPos);
    set_uniform_vec3(shader, "viewPos", cameraPos);

    // actually pointing in the reverse direction that we want
    // glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);

    // glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); 
    // glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
    // glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);

    int modelLoc = glGetUniformLocation(shader.ID, "model");
    int viewLoc = glGetUniformLocation(shader.ID, "view");
    int projectionLoc = glGetUniformLocation(shader.ID, "projection");
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view;
    glm::mat4 projection;
    // model = glm::rotate(model, (float)glfwGetTime() * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
    // note that we're translating the scene in the reverse direction of where we want to move
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    projection = glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(VAO);
    for (auto &pos : platformPositions)
    {
      // calculate the model matrix for each object and pass it to shader before drawing

      glm::mat4 model = glm::scale(glm::mat4(1.0f), platformScale); 
      model = glm::translate(model, pos);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

      glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // render shape
    // glBindVertexArray(VAO);
    // glDrawArrays(GL_TRIANGLES, 0, 3);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    // glDrawArrays(GL_TRIANGLES, 0, 36);


    // swaps color buffer for each pixel in GLFW window
    glfwSwapBuffers(window);
    // checks for input to update window state
    glfwPollEvents();
  }

  // de-allocate all resources once they've outlived their purpose
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  // glDeleteProgram(shaderProgram);

  // cleans/deletes all of GLFW's resources
  glfwTerminate();

  return 0;
}


GLFWwindow* init() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  // refuses to use full resolution framebuffers on
  // retina displays, if true, window dimensions are off
  // by a factor of 2
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_FALSE);
#endif


  GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                        "GameEngine", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return NULL;
  }
  glfwMakeContextCurrent(window);

  // initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return NULL;
  }

  // enables depth testing
  glEnable(GL_DEPTH_TEST);

  // tells OpenGL size of rendering window
  glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

  // tells GLFW we want the function defined to be called on window resize
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  return window;
}

// called whenever the user changes window size
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}  

bool is_grounded() {
  for (const auto& col : player_collision()) {
    if (glm::all(glm::equal(col, glm::vec3(0.0f, 1.0f, 0.0f)))) return true;
  }
  return false;
  // return cameraPos.y - playerSize.y < 0.01f;
}

void physics_update() {
  if (!is_grounded())
    playerVel.y -= g * deltaTime;
  else
    playerVel.y = std::max(playerVel.y, 0.0f);
  cameraPos += playerVel * deltaTime;
  if (cameraPos.y <= lowestPlatform - 0.5f) {
    totalTime = 0.0f;
    playerVel = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraPos = spawn;
  }
  if (cameraPos.y <= lowestPlatform + platformScale.y/2.0f + playerSize.y && is_grounded()) {
    std::cout << "Time: " << totalTime << " seconds" << std::endl;
    totalTime = 0.0f;
    playerVel = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraPos = spawn;
  }
}

std::vector<glm::vec3> player_collision() {
  std::vector<glm::vec3> collisions;
  // TODO: make voxelPositions a hashmap for positions
  // TODO: get bound of players, subtract them with the points on the bound of platform, then find the vector that orthogonal to the normal
  for (auto platform : platformPositions) {
    platform *= platformScale;
    float xMinPlat = platform.x - platformScale.x/2.0f;
    float xMaxPlat = platform.x + platformScale.x/2.0f;
    float yMinPlat = platform.y - platformScale.y/2.0f;
    float yMaxPlat = platform.y + platformScale.y/2.0f;
    float zMinPlat = platform.z - platformScale.z/2.0f;
    float zMaxPlat = platform.z + platformScale.z/2.0f;

    float xMinPlayer = cameraPos.x - playerSize.x/2.0f;
    float xMaxPlayer = cameraPos.x + playerSize.x/2.0f;
    float yMinPlayer = cameraPos.y - playerSize.y;
    float yMaxPlayer = cameraPos.y;
    float zMinPlayer = cameraPos.z - playerSize.z/2.0f;
    float zMaxPlayer = cameraPos.z + playerSize.z/2.0f;

    if (xMinPlayer <= xMaxPlat && xMaxPlayer >= xMinPlat 
      && yMinPlayer <= yMaxPlat && yMaxPlayer >= yMinPlat 
      && zMinPlayer <= zMaxPlat && zMaxPlayer >= zMinPlat) {

      const float error = 0.05f;

      if (abs(xMinPlat - xMaxPlayer) < error)
        collisions.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
      else if (abs(xMaxPlat - xMinPlayer) < error)
        collisions.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
      else if (abs(yMinPlat - yMaxPlayer) < error)
        collisions.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
      else if (abs(yMaxPlat - yMinPlayer) < error)
        collisions.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
      else if (abs(zMinPlat - zMaxPlayer) < error)
        collisions.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
      else if (abs(zMaxPlat - zMinPlayer) < error)
        collisions.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
      else
        collisions.push_back(glm::vec3(0.0f, 1.0f, 0.0f));

      // float maxIntersect = 0;
      // glm::vec3 bestNormal;
      // if (std::min(voxel.x + voxelScale.x/2.0f - cameraPos.x - playerSize.x/2.0f,
      //              cameraPos.x + playerSize.x/2.0f - voxel.x - voxelScale.x/2.0f) <
      //     std::min(voxel.z + voxelScale.z/2.0f - cameraPos.z - playerSize.z/2.0f,
      //              cameraPos.z + playerSize.z/2.0f - voxel.z - voxelScale.z/2.0f))
      //   collisions.push_back(glm::dot(cameraPos - voxel, glm::vec3(1, 0, 0)) >= 0 ? glm::vec3(1, 0, 0) : glm::vec3(-1, 0, 0));
      // else
      //   collisions.push_back(glm::dot(cameraPos - voxel, glm::vec3(0, 0, 1)) >= 0 ? glm::vec3(0, 0, 1) : glm::vec3(0, 0, -1));
       // if (voxel.x + voxelScale.x/2.0f - cameraPos.x - playerSize.z/2.0f < voxel.z + voxelScale.z/2.0f - cameraPos.z - playerSize.z/2.0f)
       //   collisions.push_back(glm::vec3(1, 0, 0));
       // else if (voxel.x - voxelScale.x/2.0f - cameraPos.x + playerSize.z/2.0f < voxel.z - voxelScale.z/2.0f - cameraPos.z + playerSize.z/2.0f)
       //   collisions.push_back(glm::vec3(-1, 0, 0));
       // else if (voxel.z + voxelScale.z/2.0f - cameraPos.z - playerSize.z/2.0f < voxel.z + voxelScale.z/2.0f - cameraPos.z - playerSize.z/2.0f)
       //   collisions.push_back(glm::dot(cameraPos - voxel, glm::vec3(0, 0, 1)) >= 0 ? glm::vec3(0, 0, 1) : glm::vec3(0, 0, -1));

      // if (cameraPos.x - playerSize.x/2.0f == platform.x + platformScale.x/2.0f)
      //   collisions.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
      // else if (cameraPos.x + playerSize.x/2.0f == platform.x - platformScale.x/2.0f)
      //   collisions.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
      // else if (cameraPos.y - playerSize.y == platform.y + platformScale.y/2.0f)
      //   collisions.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
      // else if (cameraPos.y == platform.y - platformScale.y/2.0f)
      //   collisions.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
      // else if (cameraPos.z - playerSize.z/2.0f == platform.z + platformScale.z/2.0f)
      //   collisions.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
      // else if (cameraPos.z + playerSize.z/2.0f == platform.z - platformScale.z/2.0f)
      //   collisions.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
      // glm::vec3 intersection = cameraPos - glm::vec3(0.0f, playerSize.y, 0.0f) - platform;
      // glm::vec3 bestNormal = glm::vec3(0, 0, 0);
      // std::vector<glm::vec3> normals = { glm::vec3(1, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0), 
      //   glm::vec3(0, -1, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, -1) };
      // for (auto& normal : normals) {
      //   // glm::vec3 target = voxel / platformScale + normal * 2.0f;
      //   if (glm::dot(intersection, normal) > glm::dot(intersection, bestNormal))
      //     // && std::all_of(platformPositions.begin(), platformPositions.end(), [target](const glm::vec3 &v) 
      //                    // { return glm::distance(v, target) > 0.01f; }))
      //     bestNormal = normal;
      // }
      // collisions.push_back(bestNormal);
    }
  } 
  return collisions;
}

void process_input(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  const float jumpForce = 1.3f;
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && is_grounded())
    playerVel.y = jumpForce;

  const float cameraSpeed = 0.8f * deltaTime; // adjust accordingly

  glm::vec3 wishDir = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 dir = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    wishDir += dir;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    wishDir += -dir;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    wishDir += -glm::normalize(glm::cross(dir, cameraUp));
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    wishDir += glm::normalize(glm::cross(dir, cameraUp));
  if (wishDir != glm::vec3(0.0f, 0.0f, 0.0f))
    wishDir = glm::normalize(wishDir);

  player_movement(window, wishDir);
  // cameraPos += wishdir * cameraSpeed;
}

void player_movement(GLFWwindow *window, glm::vec3 wishDir) {
  if (is_grounded() && glfwGetKey(window, GLFW_KEY_SPACE) != GLFW_PRESS) {
    playerVel.x *= friction;
    playerVel.z *= friction;

    float maxAccel = maxGroundSpeed * acceleration;

    float currentSpeed = glm::dot(glm::vec3(playerVel.x, 0, playerVel.z), wishDir);
    float addSpeed = maxGroundSpeed - currentSpeed > maxAccel * deltaTime ? maxAccel * deltaTime 
      : (maxGroundSpeed - currentSpeed < 0 ? 0 : maxGroundSpeed - currentSpeed);
    playerVel += addSpeed * wishDir;
  } else {
    float maxAccel = maxGroundSpeed * acceleration;
    float currentSpeed = glm::dot(glm::vec3(playerVel.x, 0, playerVel.z), wishDir);
    // clamps value
    float addSpeed = maxAirSpeed - currentSpeed > maxAccel * deltaTime ? maxAccel * deltaTime 
      : (maxAirSpeed - currentSpeed < 0 ? 0 : maxAirSpeed - currentSpeed);
    playerVel += addSpeed * wishDir;
    playerVel.x *= std::pow(0.99f, std::pow(glm::length(playerVel)/20.0f, 1.1f));
    playerVel.z *= std::pow(0.99f, std::pow(glm::length(playerVel)/20.0f, 1.1f));
  }

  std::vector<glm::vec3> collisions = player_collision();
  for (auto& normal : collisions) {
    // glm::vec3 u = glm::normalize(glm::vec3(col.x - cameraPos.x, 0.0f, col.z - cameraPos.z));
    glm::vec3 u = -normal;
    if (glm::dot(playerVel, u) > 0)
      playerVel -= glm::dot(playerVel, u)*u;
  }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  // prevents jump when mouse first enters
  if (firstMouse) // initially set to true
  {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
  lastX = xpos;
  lastY = ypos;

  const float sensitivity = 0.3f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  yaw += xoffset;
  pitch += yoffset;  
  if(pitch > 89.0f)
    pitch =  89.0f;
  if(pitch < -89.0f)
    pitch = -89.0f;

  glm::vec3 direction;
  direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  direction.y = sin(glm::radians(pitch));
  direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  cameraFront = glm::normalize(direction);
}

void set_uniform_vec3(const Shader& shader, const GLchar* name, const glm::vec3& vec) {
  int loc = glGetUniformLocation(shader.ID, name);
  glUniform3f(loc, vec.x, vec.y, vec.z);
}
