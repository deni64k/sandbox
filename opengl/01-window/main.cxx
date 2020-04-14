// http://ogldev.atspace.co.uk/www/tutorial01/tutorial01.html

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <GL/glew.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
// #define __gl_h_  // prevents glut.h from including old gl.h header file
// #include <OpenGL/gl.h>
// #include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// struct Vector3f
// {
//     float x;
//     float y;
//     float z;

//     Vector3f() {}

//     Vector3f(float _x, float _y, float _z)
//     {
//         x = _x;
//         y = _y;
//         z = _z;
//     }
    
//     Vector3f(const float* pFloat)
//     {
//         x = pFloat[0];
//         y = pFloat[0];
//         z = pFloat[0];
//     }
    
//     Vector3f(float f)
//     {
//         x = y = z = f;
//     }

//     Vector3f& operator+=(const Vector3f& r)
//     {
//         x += r.x;
//         y += r.y;
//         z += r.z;

//         return *this;
//     }

//     Vector3f& operator-=(const Vector3f& r)
//     {
//         x -= r.x;
//         y -= r.y;
//         z -= r.z;

//         return *this;
//     }

//     Vector3f& operator*=(float f)
//     {
//         x *= f;
//         y *= f;
//         z *= f;

//         return *this;
//     }

//     operator const float*() const
//     {
//         return &(x);
//     }
// };

GLuint VAO;
GLuint VBO;

typedef GLfloat Vector3f[3];
// Vector3f vertices[3] = {
//   Vector3f(0.1f,  0.1f, 0.0f),
//   Vector3f(0.9f, -0.9f, 0.0f),
//   Vector3f(0.0f,  0.9f, 0.0f)
// };
Vector3f vertices[3] = {
  {0.1f,  0.1f, 0.0f},
  {0.9f, -0.9f, 0.0f},
  {0.0f,  0.9f, 0.0f}
};

void createVertexBuffer() {
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), (GLfloat *)(vertices), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void renderScene() {
  glClear(GL_COLOR_BUFFER_BIT);

  glEnableVertexAttribArray(0);
  // glColor3f(1.0f, 1.0, 1.0f);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glDisableVertexAttribArray(0);

  // glBegin(GL_TRIANGLES);
  // for (int i = 0; i < 3; i++) {
  //   glColor3f(vertices[i][0], vertices[i][1], vertices[i][2]);
  //   glVertex3f(vertices[i][0] + 0.05, vertices[i][1], vertices[i][2]);
  // }
  // glEnd();
  
  // glBegin(GL_TRIANGLES);
  // glColor3f(   1.0,  0.0,  0.0);
  // glVertex3f(  0.0, -0.8,  0.5);
  // glColor3f(   0.0,  1.0,  0.0);
  // glVertex3f(  0.0,  0.2,  0.5);
  // glColor3f(   0.0,  0.0,  1.0);
  // glVertex3f(  0.5,  0.8, -0.5);
  // glEnd();
}

void addShader(GLuint shaderProgram, const char* shaderSource, GLenum shaderType) {
  GLuint shaderObj = glCreateShader(shaderType);

  if (shaderObj == 0) {
    fprintf(stderr, "error creating shader type %d\n", shaderType);
    exit(1);
  }

  const GLchar* p[1] = { shaderSource };
  GLint lens[1] = { static_cast<GLint>(strlen(shaderSource)) };
  glShaderSource(shaderObj, 1, p, lens);
  glCompileShader(shaderObj);
  GLint success;
  glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLchar log[1024];
    glGetShaderInfoLog(shaderObj, 1024, NULL, log);
    fprintf(stderr, "error compiling shader type %d: '%s'\n", shaderType, log);
    exit(1);
  }

  glAttachShader(shaderProgram, shaderObj);
}

void compileShaders() {
  GLuint shaderProgram = glCreateProgram();
  if (shaderProgram == 0) {
    fprintf(stderr, "error creating shader program\n");
    exit(1);
  }

  char const *vs = R"EOF(
#version 330

layout (location = 0) in vec3 Position;

void main()
{
  gl_Position = vec4(0.5 * Position.x, 0.5 * Position.y, Position.z, 1.0);
}
)EOF";
  char const *fs = R"EOF(
#version 330

out vec4 FragColor;

void main()
{
  FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
)EOF";

  addShader(shaderProgram, vs, GL_VERTEX_SHADER);
  addShader(shaderProgram, fs, GL_FRAGMENT_SHADER);

  GLint success = 0;
  GLchar errorLog[1024] = {0};

  glLinkProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (success == 0) {
    glGetProgramInfoLog(shaderProgram, sizeof(errorLog), NULL, errorLog);
    fprintf(stderr, "error linking shader program:\n%s\n", errorLog);
    exit(1);
  }

  glValidateProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &success);
  if (success == 0) {
    glGetProgramInfoLog(shaderProgram, sizeof(errorLog), NULL, errorLog);
    fprintf(stderr, "invalid shader program:\n%s\n", errorLog);
    exit(1);
  }

  glUseProgram(shaderProgram);
}

int main(int argc, char **argv) {
  GLFWwindow* window;

  if (!glfwInit())
    return 1;

  glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
  if (!window) {
    fprintf(stderr, "could not create a window\n");
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);

  GLenum res = glewInit();
  if (res != GLEW_OK) {
    fprintf(stderr, "could not initialize GLEW: %s\n", glewGetErrorString(res));
    return 1;
  }

  createVertexBuffer();

  compileShaders();

  while (!glfwWindowShouldClose(window)) {
    renderScene();

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}
