#include "shaderprogram.h"
#include "assert.h"
#include <iostream>

#define assertm(exp, msg) assert((void(msg), exp))

//Procedure reads a file into an array of chars
char* ShaderProgram::readFile(const char* fileName) {
	int filesize;
	FILE* plik;
	char* result;

#pragma warning(suppress : 4996) //Turn off an error in Visual Studio stemming from Microsoft not adhering to standards
	plik = fopen(fileName, "rb");
	if (plik != NULL) {
		fseek(plik, 0, SEEK_END);
		filesize = ftell(plik);
		fseek(plik, 0, SEEK_SET);
		result = new char[filesize + 1];
#pragma warning(suppress : 6386) //Turn off an error in Visual Studio stemming from incorrent static code analysis
		int readsize = fread(result, 1, filesize, plik);
		result[filesize] = 0;
		fclose(plik);

		return result;
	}

	return NULL;
}


//The method reads a shader code, compiles it and returns a corresponding handle
GLuint ShaderProgram::loadShader(GLenum shaderType, const char* fileName) {
	
	GLuint shader = glCreateShader(shaderType); //Create a shader handle

	const GLchar* shaderSource = readFile(fileName);	//Read a shader source file into an array of chars

	assertm(shaderSource != NULL,  "Shader Source is NULL");

	glShaderSource(shader, 1, &shaderSource, NULL);	//Associate source code with the shader handle

	glCompileShader(shader);	//Compile source code

	std::cout << fileName <<"\n";

	delete[]shaderSource;	//Delete source code from memory (it is no longer needed)

	//Download a compilation error log and display it
	int infologLength = 0;
	int charsWritten = 0;
	char* infoLog;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 1) {
		infoLog = new char[infologLength];
		glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
		printf("[Info Log load shader]%s\n", infoLog);
		delete[]infoLog;
	}

	//Return shader handle
	return shader;
}

int ShaderProgram::addShader(GLuint shaderType, const char* ShaderFile) {

	GLuint* p_shader; 
	switch (shaderType)
	{
	case GL_VERTEX_SHADER:
		p_shader = &vertexShader;
		break; 
	case GL_FRAGMENT_SHADER: 
		p_shader = &fragmentShader;
		break; 
	case GL_GEOMETRY_SHADER:
		p_shader = &geometryShader; 
		break;
	case GL_TESS_CONTROL_SHADER:
		p_shader = &tessControlShader;
		break;
	case GL_TESS_EVALUATION_SHADER:
		p_shader = &tessEvalShader;
		break;
	default:
		return -1; 
	}

	if (*p_shader != 0) {
		glDetachShader(shaderProgram, *p_shader);
		glDeleteShader(*p_shader);
	}
	*p_shader = loadShader(shaderType, ShaderFile);
	glAttachShader(shaderProgram, *p_shader);

	return 0; 
}

int ShaderProgram::linkProgram() {

	glLinkProgram(shaderProgram);
	//Download an error log and display it
	int infologLength = 0;
	int charsWritten = 0;
	char* infoLog;

	glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 1)
	{
		infoLog = new char[infologLength];
		glGetProgramInfoLog(shaderProgram, infologLength, &charsWritten, infoLog);
		printf("%s\n", infoLog);
		delete[]infoLog;
	}
	return 0; 
}


ShaderProgram::ShaderProgram(const char* vertexShaderFile, const char* fragmentShaderFile) {

	vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderFile);	//Load vertex shader
	fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderFile);	//Load fragment shader

	shaderProgram = glCreateProgram();	//Generate shader program handle

	//Attach shaders and link shader program
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	linkProgram(); 
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept {
	*this = std::move(other);
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
	if (this != &other) {
		clean();

		shaderProgram = other.shaderProgram;
		vertexShader = other.vertexShader;
		geometryShader = other.geometryShader;
		fragmentShader = other.fragmentShader;
		tessEvalShader = other.tessEvalShader;
		tessControlShader = other.tessControlShader;

		other.shaderProgram = 0;
		other.vertexShader = 0;
		other.geometryShader = 0;
		other.fragmentShader = 0;
		other.tessEvalShader = 0;
		other.tessControlShader = 0;
	}
	return *this;
}

void ShaderProgram::clean() {
	//Detach shaders from program
	if (vertexShader != 0) glDetachShader(shaderProgram, vertexShader);
	if (geometryShader != 0) glDetachShader(shaderProgram, geometryShader);
	if (tessControlShader != 0) glDetachShader(shaderProgram, tessControlShader);
	if (tessEvalShader != 0) glDetachShader(shaderProgram, tessEvalShader);
	if (fragmentShader != 0) glDetachShader(shaderProgram, fragmentShader);

	//Delete shaders
	if (vertexShader != 0) glDeleteShader(vertexShader);
	if (geometryShader != 0) glDeleteShader(geometryShader);
	if (tessControlShader != 0) glDeleteShader(tessControlShader);
	if (tessEvalShader != 0) glDeleteShader(tessEvalShader);
	if (fragmentShader != 0) glDeleteShader(fragmentShader);

	//Delete program
	if (shaderProgram != 0) glDeleteProgram(shaderProgram);
}

ShaderProgram::~ShaderProgram() { clean(); }

//Make the shader program active
void ShaderProgram::use() {
	glUseProgram(shaderProgram);
}

//Get the slot number corresponding to the uniform variableName
GLuint ShaderProgram::u(const char* variableName) {
	return glGetUniformLocation(shaderProgram, variableName);
}

//Get the slot number corresponding to the attribute variableName
GLuint ShaderProgram::a(const char* variableName) {
	return glGetAttribLocation(shaderProgram, variableName);
}


ComputeShaderProgram::ComputeShaderProgram(const char* computeShaderFile) {

	computeShader = loadShader(computeShaderFile);	//Load compute shader

	shaderProgram = glCreateProgram();	//Generate shader program handle

	//Attach shaders and link shader program
	glAttachShader(shaderProgram, computeShader);
	glLinkProgram(shaderProgram);

	int infologLength = 0;
	int charsWritten = 0;
	char* infoLog;

	glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 1)
	{
		infoLog = new char[infologLength];
		glGetProgramInfoLog(shaderProgram, infologLength, &charsWritten, infoLog);
		printf("%s\n", infoLog);
		delete[]infoLog;
	}
}
ComputeShaderProgram::~ComputeShaderProgram() {
	glDetachShader(shaderProgram, computeShader);
	glDeleteShader(computeShader);
	glDeleteProgram(shaderProgram);
}

int ComputeShaderProgram::updateShader(const char* computeShaderFile) {

	glDetachShader(shaderProgram, computeShader);
	glDeleteShader(computeShader);

	computeShader = loadShader( computeShaderFile);
	glAttachShader(shaderProgram, computeShader);
	glLinkProgram(shaderProgram);

	int infologLength = 0;
	int charsWritten = 0;
	char* infoLog;

	glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 1)
	{
		infoLog = new char[infologLength];
		glGetProgramInfoLog(shaderProgram, infologLength, &charsWritten, infoLog);
		printf("%s\n", infoLog);
		delete[]infoLog;
	}

	return 0; 
}

void ComputeShaderProgram::use() {
	glUseProgram(shaderProgram);
}

char* ComputeShaderProgram::readFile(const char* fileName) {
	int filesize;
	FILE* plik;
	char* result;

#pragma warning(suppress : 4996) //Turn off an error in Visual Studio stemming from Microsoft not adhering to standards
	plik = fopen(fileName, "rb");
	if (plik != NULL) {
		fseek(plik, 0, SEEK_END);
		filesize = ftell(plik);
		fseek(plik, 0, SEEK_SET);
		result = new char[filesize + 1];
#pragma warning(suppress : 6386) //Turn off an error in Visual Studio stemming from incorrent static code analysis
		int readsize = fread(result, 1, filesize, plik);
		result[filesize] = 0;
		fclose(plik);

		return result;
	}

	return NULL;
}

GLuint ComputeShaderProgram::loadShader( const char* fileName) {

	GLuint shader = glCreateShader(GL_COMPUTE_SHADER); //Create a shader handle

	const GLchar* shaderSource = readFile(fileName);	//Read a shader source file into an array of chars

	glShaderSource(shader, 1, &shaderSource, NULL);	//Associate source code with the shader handle

	glCompileShader(shader);	//Compile source code

	delete[]shaderSource;	//Delete source code from memory (it is no longer needed)

	//Download a compilation error log and display it
	int infologLength = 0;
	int charsWritten = 0;
	char* infoLog;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 1) {
		infoLog = new char[infologLength];
		glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
		printf("[Info Log]%s\n", infoLog);
		delete[]infoLog;
	}

	//Return shader handle
	return shader;
}