//Idea to use a global header from: https://www.3dgep.com/introduction-to-opengl-and-glsl/

#define GLEW_STATIC

#include <GL\glew.h>

#ifdef WIN32
#include <GL\wglew.h>
#endif // WIN32

#include <GLFW\glfw3.h>

#include <glm\glm.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtx\transform.hpp>
#include <glm\gtx\quaternion.hpp>