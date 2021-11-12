#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>
#include <sstream>
#include <typeindex>
#include <optional>
#include <string>

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

// Graphics
#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexTypes.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

//#define LOG_GL_NOTIFICATIONS

/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
	case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
	case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
#ifdef LOG_GL_NOTIFICATIONS
	case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
#endif
	default: break;
	}
}

// Stores our GLFW window in a global variable for now
GLFWwindow* window;
// The current size of our window in pixels
glm::ivec2 windowSize = glm::ivec2(800, 800);
// The title of our GLFW window
std::string windowTitle = "Car Wong - 100781520 Midterm";

// using namespace should generally be avoided, and if used, make sure it's ONLY in cpp files
using namespace Gameplay;
using namespace Gameplay::Physics;

// The scene that we will be rendering
Scene::Sptr scene = nullptr;

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowSize = glm::ivec2(width, height);
	if (windowSize.x * windowSize.y > 0) {
		scene->MainCamera->ResizeWindow(width, height);
	}
}

/// <summary>
/// Handles intializing GLFW, should be called before initGLAD, but after Logger::Init()
/// Also handles creating the GLFW window
/// </summary>
/// <returns>True if GLFW was initialized, false if otherwise</returns>
bool initGLFW() {
	// Initialize GLFW
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

	//Create a new GLFW window and make it current
	window = glfwCreateWindow(windowSize.x, windowSize.y, windowTitle.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	return true;
}

/// <summary>
/// Handles initializing GLAD and preparing our GLFW window for OpenGL calls
/// </summary>
/// <returns>True if GLAD is loaded, false if there was an error</returns>
bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

/// <summary>
/// Draws a widget for saving or loading our scene
/// </summary>
/// <param name="scene">Reference to scene pointer</param>
/// <param name="path">Reference to path string storage</param>
/// <returns>True if a new scene has been loaded</returns>
bool DrawSaveLoadImGui(Scene::Sptr& scene, std::string& path) {
	// Since we can change the internal capacity of an std::string,
	// we can do cool things like this!
	ImGui::InputText("Path", path.data(), path.capacity());

	// Draw a save button, and save when pressed
	if (ImGui::Button("Save")) {
		scene->Save(path);
	}
	ImGui::SameLine();
	// Load scene from file button
	if (ImGui::Button("Load")) {
		// Since it's a reference to a ptr, this will
		// overwrite the existing scene!
		scene = nullptr;
		scene = Scene::Load(path);

		return true;
	}
	return false;
}

/// <summary>
/// Draws some ImGui controls for the given light
/// </summary>
/// <param name="title">The title for the light's header</param>
/// <param name="light">The light to modify</param>
/// <returns>True if the parameters have changed, false if otherwise</returns>
bool DrawLightImGui(const Scene::Sptr& scene, const char* title, int ix) {
	bool isEdited = false;
	bool result = false;
	Light& light = scene->Lights[ix];
	ImGui::PushID(&light); // We can also use pointers as numbers for unique IDs
	if (ImGui::CollapsingHeader(title)) {
		isEdited |= ImGui::DragFloat3("Pos", &light.Position.x, 0.01f);
		isEdited |= ImGui::ColorEdit3("Col", &light.Color.r);
		isEdited |= ImGui::DragFloat("Range", &light.Range, 0.1f);

		result = ImGui::Button("Delete");
	}
	if (isEdited) {
		scene->SetShaderLight(ix);
	}

	ImGui::PopID();
	return result;
}



//rand()%3;

int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	float movex = 0; // head movement x coordinate
	float movey = 0; // head movement y coordinate

	float fruitx = rand() % 5 - 2.5; // fruit placement x coordinate
	float fruity = rand() % 5 - 2.5; // fruit placement y coordinate

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Initialize our ImGui helper
	ImGuiHelper::Init(window);

	// Initialize our resource manager
	ResourceManager::Init();

	// Register all our resource types so we can load them from manifest files
	ResourceManager::RegisterType<Texture2D>();
	ResourceManager::RegisterType<Material>();
	ResourceManager::RegisterType<MeshResource>();
	ResourceManager::RegisterType<Shader>();

	// Register all of our component types so we can load them from files
	ComponentManager::RegisterType<Camera>();
	ComponentManager::RegisterType<RenderComponent>();
	ComponentManager::RegisterType<RigidBody>();
	ComponentManager::RegisterType<TriggerVolume>();
	ComponentManager::RegisterType<RotatingBehaviour>();
	ComponentManager::RegisterType<JumpBehaviour>();
	ComponentManager::RegisterType<MaterialSwapBehaviour>();

	// GL states, we'll enable depth testing and backface fulling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);



	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene) {
		ResourceManager::LoadManifest("manifest.json");
		scene = Scene::Load("scene.json");
	}
	else {
		// Create our OpenGL resources
		Shader::Sptr uboShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shader.glsl" },
			{ ShaderPartType::Fragment, "shaders/frag_blinn_phong_textured.glsl" }
		});
		MeshResource::Sptr headMesh = ResourceManager::CreateAsset<MeshResource>("snakehead.obj"); // load head mesh
		Texture2D::Sptr headTexture = ResourceManager::CreateAsset<Texture2D>("textures/snakehead.png"); // load snakehead texture

		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/floor.png"); // change box texture to floor texture

		MeshResource::Sptr fruitMesh = ResourceManager::CreateAsset<MeshResource>("apple.obj"); // load fruit mesh
		Texture2D::Sptr fruitTexture = ResourceManager::CreateAsset<Texture2D>("textures/apple.png"); // load fruit texture 

		Texture2D::Sptr gfruitTexture = ResourceManager::CreateAsset<Texture2D>("textures/gapple.png"); // load gold fruit texture

		//Create an empty scene
		scene = std::make_shared<Scene>();

		// I hate this
		scene->BaseShader = uboShader;

		// Create our materials
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(); // create floor material
		{
			boxMaterial->Name = "Box";
			boxMaterial->MatShader = scene->BaseShader;
			boxMaterial->Texture = boxTexture;
			boxMaterial->Shininess = 0.0f;
		}

		Material::Sptr headMaterial = ResourceManager::CreateAsset<Material>(); // create the material for the head
		{
			headMaterial->Name = "Head";
			headMaterial->MatShader = scene->BaseShader;
			headMaterial->Texture = headTexture;
			headMaterial->Shininess = 0.0f;
		}

		Material::Sptr fruitMaterial = ResourceManager::CreateAsset<Material>(); // create the fruit material
		{
			fruitMaterial->Name = "Fruit";
			fruitMaterial->MatShader = scene->BaseShader;
			fruitMaterial->Texture = fruitTexture;
			fruitMaterial->Shininess = 10.0f;
		}

		Material::Sptr gfruitMaterial = ResourceManager::CreateAsset<Material>(); // create the gold fruit material
		{
			gfruitMaterial->Name = "Gold Fruit";
			gfruitMaterial->MatShader = scene->BaseShader;
			gfruitMaterial->Texture = gfruitTexture;
			gfruitMaterial->Shininess = 10.0f;
		}

		// Create some lights for our scene
		scene->Lights.resize(5);
		scene->Lights[0].Position = glm::vec3(2.0f, 2.0f, 3.0f); // move the lights to the corner of the board
		scene->Lights[0].Color = glm::vec3(0.5f, 0.5f, 0.5f);
		scene->Lights[0].Range = 10.0f;

		scene->Lights[1].Position = glm::vec3(2.0f, -2.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(0.5f, 0.5f, 0.5f);

		scene->Lights[2].Position = glm::vec3(-2.0f, -2.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(0.5f, 0.5f, 0.5f);

		scene->Lights[3].Position = glm::vec3(-2.0f, 2.0f, 3.0f);
		scene->Lights[3].Color = glm::vec3(0.5f, 0.5f, 0.5f);

		scene->Lights[4].Position = glm::vec3(0.0f, 0.0f, 3.0f); // create a 5th light for the center of the board
		scene->Lights[4].Color = glm::vec3(0.5f, 0.5f, 0.0f);

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();


		// Set up the scene's camera
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera"); // make the camera look over the board
		{
			camera->SetPostion(glm::vec3(0, 0, 2.5));
			camera->SetRotation(glm::vec3(0, 0, -180));

			Camera::Sptr cam = camera->Add<Camera>();

			// Make sure that the camera is set as the scene's main camera!
			scene->MainCamera = cam;
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane"); // create the floor game object
		{
			// Scale up the plane
			plane->SetScale(glm::vec3(5.0f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(boxMaterial);
		}

		GameObject::Sptr head = scene->CreateGameObject("Snake Head"); // create the snake head object 
		{
			head->SetPostion(glm::vec3(movex, movey, -0.5f));
			head->SetRotation(glm::vec3(90.0f, 0.0f, 90.0f));
			head->SetScale(glm::vec3(0.5f));

			RenderComponent::Sptr renderer = head->Add<RenderComponent>();
			renderer->SetMesh(headMesh);
			renderer->SetMaterial(headMaterial);

			RigidBody::Sptr physics = head->Add<RigidBody>(RigidBodyType::Dynamic);
			BoxCollider::Sptr headcollider = BoxCollider::Create(glm::vec3(0.8f, 0.8f, 0.8f));
			headcollider->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));
			headcollider->SetPosition(head->GetPosition());
			physics->AddCollider(headcollider);
		}

		GameObject::Sptr fruit = scene->CreateGameObject("Fruit"); // create the fruit object 
		{
			fruit->SetPostion(glm::vec3(fruitx, fruity, 0.0f));
			fruit->SetScale(glm::vec3(0.25f, 0.25f, 0.25f));
			fruit->SetRotation(glm::vec3(90.0f, 0.0f, 90.0f));

			RenderComponent::Sptr renderer = fruit->Add<RenderComponent>();
			renderer->SetMesh(fruitMesh);
			renderer->SetMaterial(fruitMaterial);

			RigidBody::Sptr physics = fruit->Add<RigidBody>(RigidBodyType::Static);
			BoxCollider::Sptr fruitcollider = BoxCollider::Create(glm::vec3(0.6f, 0.6f, 0.6f));
			fruitcollider->SetScale(glm::vec3(0.25f, 0.25f, 0.25f));
			fruitcollider->SetPosition(fruit->GetPosition());
			physics->AddCollider(fruitcollider);
		}

		GameObject::Sptr goldfruit = scene->CreateGameObject("Gold Fruit"); // create the gold fruit object
		{
			goldfruit->SetPostion(glm::vec3(5.0f, 5.0f, 0.0f));
			goldfruit->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));
			goldfruit->SetRotation(glm::vec3(90.0f, 0.0f, 90.0f));

			RenderComponent::Sptr renderer = goldfruit->Add<RenderComponent>();
			renderer->SetMesh(fruitMesh);
			renderer->SetMaterial(gfruitMaterial);

			RigidBody::Sptr physics = goldfruit->Add<RigidBody>(RigidBodyType::Static);
			BoxCollider::Sptr gfruitcollider = BoxCollider::Create(glm::vec3(0.6f, 0.6f, 0.6f));
			gfruitcollider->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));
			gfruitcollider->SetPosition(goldfruit->GetPosition());
			physics->AddCollider(gfruitcollider);
		}

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");
	}

	// Call scene awake to start up all of our components
	scene->Window = window;
	scene->Awake();

	// We'll use this to allow editing the save/load path
	// via ImGui, note the reserve to allocate extra space
	// for input!
	std::string scenePath = "scene.json";
	scenePath.reserve(256);

	bool isRotating = true;
	float rotateSpeed = 90.0f;

	// Our high-precision timer
	double lastFrame = glfwGetTime();


	BulletDebugMode physicsDebugMode = BulletDebugMode::None;
	float playbackSpeed = 1.0f;

	nlohmann::json editorSceneState;



	glm::mat4 transform = glm::mat4(1.0f);

	double thisFrame = glfwGetTime();

	// get the game objects
	GameObject::Sptr head = scene->FindObjectByName("Snake Head");
	GameObject::Sptr fruit = scene->FindObjectByName("Fruit");
	GameObject::Sptr goldfruit = scene->FindObjectByName("Gold Fruit");

	int count = 0;
	int time = 0;

	int randfruit = 1;

	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGuiHelper::StartFrame();

		// Calculate the time since our last frame (dt)
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);

		// Showcasing how to use the imGui library!
		bool isDebugWindowOpen = ImGui::Begin("Debugging");
		if (isDebugWindowOpen) {
			// Draws a button to control whether or not the game is currently playing
			static char buttonLabel[64];
			sprintf_s(buttonLabel, "%s###playmode", scene->IsPlaying ? "Exit Play Mode" : "Enter Play Mode");
			if (ImGui::Button(buttonLabel)) {
				// Save scene so it can be restored when exiting play mode
				if (!scene->IsPlaying) {
					editorSceneState = scene->ToJson();
				}

				// Toggle state
				scene->IsPlaying = !scene->IsPlaying;

				// If we've gone from playing to not playing, restore the state from before we started playing
				if (!scene->IsPlaying) {
					scene = nullptr;
					// We reload to scene from our cached state
					scene = Scene::FromJson(editorSceneState);
					// Don't forget to reset the scene's window and wake all the objects!
					scene->Window = window;
					scene->Awake();
				}
			}

			// Make a new area for the scene saving/loading
			ImGui::Separator();
			if (DrawSaveLoadImGui(scene, scenePath)) {
				// C++ strings keep internal lengths which can get annoying
				// when we edit it's underlying datastore, so recalcualte size
				scenePath.resize(strlen(scenePath.c_str()));

				// We have loaded a new scene, call awake to set
				// up all our components
				scene->Window = window;
				scene->Awake();
			}
			ImGui::Separator();
			// Draw a dropdown to select our physics debug draw mode
			if (BulletDebugDraw::DrawModeGui("Physics Debug Mode:", physicsDebugMode)) {
				scene->SetPhysicsDebugDrawMode(physicsDebugMode);
			}
			LABEL_LEFT(ImGui::SliderFloat, "Playback Speed:    ", &playbackSpeed, 0.0f, 10.0f);
			ImGui::Separator();
		}

		// Clear the color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Update our application level uniforms every frame

		// Draw some ImGui stuff for the lights
		if (isDebugWindowOpen) {
			for (int ix = 0; ix < scene->Lights.size(); ix++) {
				char buff[256];
				sprintf_s(buff, "Light %d##%d", ix, ix);
				// DrawLightImGui will return true if the light was deleted
				if (DrawLightImGui(scene, buff, ix)) {
					// Remove light from scene, restore all lighting data
					scene->Lights.erase(scene->Lights.begin() + ix);
					scene->SetupShaderAndLights();
					// Move back one element so we don't skip anything!
					ix--;
				}
			}
			// As long as we don't have max lights, draw a button
			// to add another one
			if (scene->Lights.size() < scene->MAX_LIGHTS) {
				if (ImGui::Button("Add Light")) {
					scene->Lights.push_back(Light());
					scene->SetupShaderAndLights();
				}
			}
			// Split lights from the objects in ImGui
			ImGui::Separator();
		}

		// key inputs

		if (glfwGetKey(window, GLFW_KEY_W))
		{
			movey -= 0.05f;
			head->SetRotation(glm::vec3(90.0f, 0.0f, 90.0f));
		}
		if (glfwGetKey(window, GLFW_KEY_S))
		{
			movey += 0.05f;
			head->SetRotation(glm::vec3(90.0f, 0.0f, -90.0f));
		}

		if (glfwGetKey(window, GLFW_KEY_A))
		{
			movex += 0.05f;
			head->SetRotation(glm::vec3(90.0f, 0.0f, 180.0f));
		}

		if (glfwGetKey(window, GLFW_KEY_D))
		{
			movex -= 0.05f;
			head->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
		}


		// if the head is not at the edges, make the lights white
		if (movey > 2.5) // if the head goes past the edge, move to the opposite side
		{
			movey = -2.4;
			scene->Lights[0].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[1].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[2].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[3].Color = glm::vec3(0.5f, 0.5f, 0.5f);
		}

		if (movey < -2.5)
		{
			movey = 2.4;
			scene->Lights[0].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[1].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[2].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[3].Color = glm::vec3(0.5f, 0.5f, 0.5f);
		}

		if (movex > 2.5)
		{
			movex = -2.4;
			scene->Lights[0].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[1].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[2].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[3].Color = glm::vec3(0.5f, 0.5f, 0.5f);
		}

		if (movex < -2.5)
		{
			movex = 2.4;
			scene->Lights[0].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[1].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[2].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[3].Color = glm::vec3(0.5f, 0.5f, 0.5f);
		}


		// if the head is near the boarder, make the lights red at the edge
		if (movex < -2.0)
		{
			scene->Lights[2].Color = glm::vec3(1.0f, 0.0f, 0.0f);
			scene->Lights[3].Color = glm::vec3(1.0f, 0.0f, 0.0f);
		}

		if (movex > 2.0)
		{
			scene->Lights[0].Color = glm::vec3(1.0f, 0.0f, 0.0f);
			scene->Lights[1].Color = glm::vec3(1.0f, 0.0f, 0.0f);
		}

		if (movey > 2.0)
		{
			scene->Lights[0].Color = glm::vec3(1.0f, 0.0f, 0.0f);
			scene->Lights[3].Color = glm::vec3(1.0f, 0.0f, 0.0f);
		}

		if (movey < -2.0)
		{
			scene->Lights[1].Color = glm::vec3(1.0f, 0.0f, 0.0f);
			scene->Lights[2].Color = glm::vec3(1.0f, 0.0f, 0.0f);
		}

		if (movex > -2.0 && movex < 2.0 && movey < 2.0 && movey >-2.0)
		{
			scene->Lights[0].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[1].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[2].Color = glm::vec3(0.5f, 0.5f, 0.5f);
			scene->Lights[3].Color = glm::vec3(0.5f, 0.5f, 0.5f);
		}

		head->SetPostion(glm::vec3(movex, movey, 0.0f)); // set the position of the snake head

		if (randfruit == 1) // if the fruit is the normal fruit
		{
			fruit->SetPostion(glm::vec3(fruitx, fruity, 0.0f)); // change the postion of the normal fruit
			goldfruit->SetPostion(glm::vec3(5.0f, 5.0f, 0.0f)); // put the gold fruit to the side

			if (head->GetPosition().x < fruit->GetPosition().x + 0.2
				&& head->GetPosition().x + 0.4 > fruit->GetPosition().x
				&& head->GetPosition().y < fruit->GetPosition().y + 0.2
				&& head->GetPosition().y + 0.4 > fruit->GetPosition().y) // checks collisions of the head and the fruit
			{
				// if the fruit is picked up make the lights flash green
				scene->Lights[0].Color = glm::vec3(0.0f, 1.0f, 0.0f);
				scene->Lights[1].Color = glm::vec3(0.0f, 1.0f, 0.0f);
				scene->Lights[2].Color = glm::vec3(0.0f, 1.0f, 0.0f);
				scene->Lights[3].Color = glm::vec3(0.0f, 1.0f, 0.0f);
				scene->Lights[4].Color = glm::vec3(0.0f, 1.0f, 0.0f);
				count++; // add to the score

				fruitx = rand() % 5 - 2; // randomize the next location
				fruity = rand() % 5 - 2;

				randfruit = (rand() % 2) + 1; // randomize if the normal or gold fruit is placed

				std::cout << "Score: " << count << "\n";
			}
		}

		if (randfruit == 2)
		{
			goldfruit->SetPostion(glm::vec3(fruitx, fruity, 0.0f)); // change the position of the gold fruit
			fruit->SetPostion(glm::vec3(5.0f, 5.0f, 0.0f)); // put the normal fruit to the side

			if (head->GetPosition().x < goldfruit->GetPosition().x + 0.3
				&& head->GetPosition().x + 0.4 > goldfruit->GetPosition().x
				&& head->GetPosition().y < goldfruit->GetPosition().y + 0.3
				&& head->GetPosition().y + 0.4 > goldfruit->GetPosition().y)
			{
				scene->Lights[0].Color = glm::vec3(0.0f, 1.0f, 0.0f);
				scene->Lights[1].Color = glm::vec3(0.0f, 1.0f, 0.0f);
				scene->Lights[2].Color = glm::vec3(0.0f, 1.0f, 0.0f);
				scene->Lights[3].Color = glm::vec3(0.0f, 1.0f, 0.0f);
				scene->Lights[4].Color = glm::vec3(0.0f, 1.0f, 0.0f);

				count += 2; // add two since it's the gold fruit

				fruitx = rand() % 5 - 2;
				fruity = rand() % 5 - 2;

				randfruit = (rand() % 2) + 1;

				std::cout << "Score: " << count << "\n";
			}
		}

		scene->SetupShaderAndLights();

		dt *= playbackSpeed;

		// Perform updates for all components
		scene->Update(dt);

		// Grab shorthands to the camera and shader from the scene
		Camera::Sptr camera = scene->MainCamera;

		// Cache the camera's viewprojection
		glm::mat4 viewProj = camera->GetViewProjection();
		DebugDrawer::Get().SetViewProjection(viewProj);

		// Update our worlds physics!
		scene->DoPhysics(dt);

		// Draw object GUIs
		if (isDebugWindowOpen) {
			scene->DrawAllGameObjectGUIs();
		}


		// The current material that is bound for rendering
		Material::Sptr currentMat = nullptr;
		Shader::Sptr shader = nullptr;

		// Render all our objects
		ComponentManager::Each<RenderComponent>([&](const RenderComponent::Sptr& renderable) {

			// If the material has changed, we need to bind the new shader and set up our material and frame data
			// Note: This is a good reason why we should be sorting the render components in ComponentManager
			if (renderable->GetMaterial() != currentMat) {
				currentMat = renderable->GetMaterial();
				shader = currentMat->MatShader;

				shader->Bind();
				shader->SetUniform("u_CamPos", scene->MainCamera->GetGameObject()->GetPosition());
				currentMat->Apply();
			}

			// Grab the game object so we can do some stuff with it
			GameObject* object = renderable->GetGameObject();

			// Set vertex shader parameters
			shader->SetUniformMatrix("u_ModelViewProjection", viewProj * object->GetTransform());
			shader->SetUniformMatrix("u_Model", object->GetTransform());
			shader->SetUniformMatrix("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(object->GetTransform()))));

			// Draw the object
			renderable->GetMesh()->Draw();
			});

		// End our ImGui window
		ImGui::End();

		VertexArrayObject::Unbind();

		lastFrame = thisFrame;
		ImGuiHelper::EndFrame();
		glfwSwapBuffers(window);
	}

	// Clean up the ImGui library
	ImGuiHelper::Cleanup();

	// Clean up the resource manager
	ResourceManager::Cleanup();

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}