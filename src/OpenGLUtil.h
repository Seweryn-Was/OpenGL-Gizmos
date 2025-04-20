#ifndef UTIL_OPENGL_GUARD
#define UTIL_OPENGL_GUARD

#include <GL/glew.h>

GLenum glCheckError_(const char* file, int line, const char* mess);
#define glCheckError(mess) glCheckError_(__FILE__, __LINE__, mess) 

#endif //UTIL_OPENGL_GUARD