#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream> //03
#include <string> //03
#include <GLM/glm.hpp> // Lec 04
#include <GLM/gtc/matrix_transform.hpp> //lec 04



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


//// Lecture 04

GLfloat rotY = 0.0f;

void keyboard() {
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		rotY += 0.1f;
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		rotY -= 0.1f;

}

///////////////


int main() {
	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	//// Lecture 3 starts here

	// Cube data
	static const GLfloat points[] = {//front face, 2 triangles
		-0.5f, -0.5f, 0.5f,//0  front face
		0.5f, -0.5f, 0.5f, //3
		-0.5f, 0.5f, 0.5f, //1
		0.5f, -0.5f, 0.5f, //3
		0.5f, 0.5f, 0.5f, //2
		-0.5f, 0.5f, 0.5f, //1
		0.5f, -0.5f, 0.5f, //3 Right face
		0.5f, -0.5f, -0.5f, //7
		0.5f, 0.5f, 0.5f, //2
		0.5f, -0.5f, -0.5f, //7
		0.5f, 0.5f, -0.5f, //6
		0.5f, 0.5f, 0.5f,  //2
		-0.5f, -0.5f, -0.5f, //4 Left face
		-0.5f, -0.5f, 0.5f, //0
		-0.5f, 0.5f, -0.5f, //5
		-0.5f, -0.5f, 0.5f, //0
		-0.5f, 0.5f, 0.5f,  //1
		-0.5f, 0.5f, -0.5f //5

	};

	// Color data
	static const GLfloat colors[] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f
	};

	/////// LECTURE 05 //////////

	static const GLfloat normals[] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, // front
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, //right
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f //left
	};

	/// LECTURE 05
	GLfloat cameraPos[] = { 0.0f, 0.0f, 3.0f };
	GLfloat lightPos[] = { 0.0f, 0.0f, 3.0f };
	////////////
	

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

	/////////// LECTURE 05
	GLuint normal_vbo = 2;
	glGenBuffers(1, &normal_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, normal_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	//////////////


	glEnableVertexAttribArray(0);//pos
	glEnableVertexAttribArray(1);//colors
	/// LEC 05
	glEnableVertexAttribArray(2);//normals
	////////////

	// Load our shaders

	if (!loadShaders())
		return 1;

	////////// LECTURE 04 //////////////

	// Projection - FoV, aspect ratio, near, far
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	glm::mat4 Projection = glm::perspective(glm::radians(45.0f),
		(float)width / (float)height, 0.1f, 100.0f);

	// View matrix - Camera

	glm::mat4 View = glm::lookAt(
		glm::vec3(0, 0, 3), // camera position
		glm::vec3(0, 0, 0), //target
		glm::vec3(0, 1, 0) //up vector
	);

	// Model matrix
	glm::mat4 Model = glm::mat4(1.0f);//Identity matrix - resets your matrix

	glm::mat4 mvp;// = Projection * View * Model;

	// Handle for our mvp
	GLuint matrixMVP = glGetUniformLocation(shader_program, "MVP");
	
	////// LEC 05 - uniform variables
	GLuint matrixModel = glGetUniformLocation(shader_program, "Model");
	GLuint lightPosID = glGetUniformLocation(shader_program, "lightPos");
	GLuint cameraPosID = glGetUniformLocation(shader_program, "cameraPos");
	////////////////////////////////
	
	// GL states
	glEnable(GL_DEPTH_TEST);
	// LEC 05
	glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CCW);
	//glCullFace(GL_FRONT); //GL_BACK, GL_FRONT_AND_BACK

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shader_program);
		
		////////// Lecture 04								 X,    Y,    Z	
		Model = glm::mat4(1.0f); // reset Model

		keyboard();

		//Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, movZ));
		Model = glm::rotate(Model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
		mvp = Projection * View * Model;
		
		// Send mvp to GPU
		glUniformMatrix4fv(matrixMVP, 1, GL_FALSE, &mvp[0][0]);

		///////// LEC 05
		glUniformMatrix4fv(matrixModel, 1, GL_FALSE, &Model[0][0]);

		glUniform3fv(lightPosID, 1, &lightPos[0]);
		glUniform3fv(cameraPosID, 1, &cameraPos[0]);

		/////////////////

		glDrawArrays(GL_TRIANGLES, 0, 18);

		
		glfwSwapBuffers(window);
	}
	return 0;

}