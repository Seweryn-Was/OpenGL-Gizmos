#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include "GL/glew.h"
#include "stdio.h"

class ShaderProgram {
private:
	GLuint shaderProgram;		// Shader program handle
	GLuint vertexShader;		// Vertex shader handle
	GLuint geometryShader;		// Geometry shader handle
	GLuint fragmentShader;		// Fragment shader handle 
	GLuint tessEvalShader;		// Tessellation Evaluation shader handle
	GLuint tessControlShader;	// Tessellation Control shader handle

	char* readFile(const char* fileName);						// File reading method
	GLuint loadShader(GLenum shaderType, const char* fileName); // Method reads shader source file, compiles it and returns the corresponding handle

public:
	ShaderProgram(const char* vertexShaderFile, const char* fragmentShaderFile);

	int addShader(GLuint shaderType, const char* ShaderFile); 
	int linkProgram(); 
	~ShaderProgram();
	void use();													// Turns on the shader program
	GLuint u(const char* variableName);							// Returns the slot number corresponding to the uniform variableName
	GLuint a(const char* variableName);							// Returns the slot number corresponding to the attribute variableName
};

class ComputeShaderProgram {
public: 
	ComputeShaderProgram(const char* computeShaderFile);
	~ComputeShaderProgram();

	int updateShader(const char* computeShaderFile);
	void use();
private: 
	GLuint computeShader;
	GLuint shaderProgram;
	char* readFile(const char* fileName);
	GLuint loadShader(const char* fileName);
};
#endif