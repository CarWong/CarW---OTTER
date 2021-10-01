#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream> //03
#include <string> //03
#include <GLM/glm.hpp>//04
#include <GLM/gtc/matrix_transform.hpp> //04

GLFWwindow* window;

bool initGLFW() {
	if (glfwInit() == GLFW_FALSE) {
		std::cout << "Failed to Initialize GLFW" << std::endl;
		return false;
	}

	//Create a new GLFW window
	window = glfwCreateWindow(800, 800, "INFR1350U", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	return true;
}

bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		std::cout << "Failed to initialize Glad" << std::endl;
		return false;
	}
}


GLuint shader_program;

bool loadShaders() {
	// Read Shaders from file
	std::string vert_shader_str;
	std::ifstream vs_stream("vertex_shader.glsl", std::ios::in);
	if (vs_stream.is_open()) {
		std::string Line = "";
		while (getline(vs_stream, Line))
			vert_shader_str += "\n" + Line;
		vs_stream.close();
	}
	else {
		printf("Could not open vertex shader!!\n");
		return false;
	}
	const char* vs_str = vert_shader_str.c_str();

	std::string frag_shader_str;
	std::ifstream fs_stream("frag_shader.glsl", std::ios::in);
	if (fs_stream.is_open()) {
		std::string Line = "";
		while (getline(fs_stream, Line))
			frag_shader_str += "\n" + Line;
		fs_stream.close();
	}
	else {
		printf("Could not open fragment shader!!\n");
		return false;
	}
	const char* fs_str = frag_shader_str.c_str();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_str, NULL);
	glCompileShader(vs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_str, NULL);
	glCompileShader(fs);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, fs);
	glAttachShader(shader_program, vs);
	glLinkProgram(shader_program);

	return true;
}


// lec 4
GLfloat rotZ = 0.0f;
GLfloat movZ = 0.5f;

void keyboard()
{
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		rotZ += 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		rotZ -= 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		movZ += 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		movZ -= 0.1f;
	}
}

//


int main() {
	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	//// Lecture 3 starts here

	static const GLfloat points[] = {
		-0.5f, -0.5f, 0.5f,
		0.5f, -0.5f, 0.5f,
		0.0f, 0.5f, 0.5f
	};

	static const GLfloat colors[] = {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	};

	//VBO - Vertex buffer object
	GLuint pos_vbo = 0;
	glGenBuffers(1, &pos_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	
	GLuint color_vbo = 1;
	glGenBuffers(1, &color_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	
	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
	
	//						index, size, type, normalize?, stride, pointer
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(0);//pos
	glEnableVertexAttribArray(1);//colors

	// Load our shaders

	if (!loadShaders())
		return 1;

	//Lec 4

	// Projection - FOV, aspect ratio, near, far

	int width, height;
	glfwGetWindowSize(window, &width, &height);
	
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

	//view matrix - Camera

	glm::mat4 View = glm::lookAt(glm::vec3(0, 0, 4), // camera position
		glm::vec3(0, 0, 0), // target
		glm::vec3(0, 1, 0)); // up vector

	// Model matrix

	glm::mat4 Model = glm::mat4(1.0f); // identity matrix - resets your matirx
	glm::mat4 mvp = Projection * View * Model;

	//Handle for mvp
	GLuint matrixMVP = glGetUniformLocation(shader_program, "MVP");

	//

	// GL states
	glEnable(GL_DEPTH_TEST);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shader_program);
		
		//Lec 4

		// 1 0 0 0
		// 0 1 0 0
		// 0 0 1 0
		// 0 0 0 1

		Model = glm::mat4(1.0f); // reset model

		keyboard();

		Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, movZ));
		Model = glm::rotate(Model, glm::radians(rotZ), glm::vec3(0.0f, 0.0f, 1.0f));
		mvp = Projection * View * Model;

		// Send mvp to GPU
		glUniformMatrix4fv(matrixMVP, 1, GL_FALSE, &mvp[0][0]);
		
		//

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(window);
	}
	return 0;

}