#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>
#include <sstream>

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

#include "Camera.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"

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
std::string windowTitle = "Car Wong - 100781520";

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowSize = glm::ivec2(width, height);
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

//////////////////////////////////////////////////////
////////////////// NEW IN WEEK 7 /////////////////////
//////////////////////////////////////////////////////

glm::mat4 MAT4_IDENTITY = glm::mat4(1.0f);
glm::mat3 MAT3_IDENTITY = glm::mat3(1.0f);

glm::vec4 UNIT_X = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
glm::vec4 UNIT_Y = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
glm::vec4 UNIT_Z = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
glm::vec4 UNIT_W = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
glm::vec4 ZERO   = glm::vec4(0.0f);
glm::vec4 ONE    = glm::vec4(1.0f);

// Helper structure for material parameters
// to our shader
struct MaterialInfo : IResource {
	typedef std::shared_ptr<MaterialInfo> Sptr;
	// A human readable name for the material
	std::string     Name;
	// The shader that the material is using
	Shader::Sptr    Shader;

	// Material shader parameters
	Texture2D::Sptr Texture;
	float           Shininess;

	/// <summary>
	/// Handles applying this material's state to the OpenGL pipeline
	/// Will bind the shader, update material uniforms, and bind textures
	/// </summary>
	virtual void Apply() {
		// Material properties
		Shader->SetUniform("u_Material.Shininess", Shininess);

		// For textures, we pass the *slot* that the texture sure draw from
		Shader->SetUniform("u_Material.Diffuse", 0);

		// Bind the texture
		if (Texture != nullptr) {
			Texture->Bind(0);
		}
	}

	/// <summary>
	/// Loads a material from a JSON blob
	/// </summary>
	static MaterialInfo::Sptr FromJson(const nlohmann::json& data) {
		MaterialInfo::Sptr result = std::make_shared<MaterialInfo>();
		result->OverrideGUID(Guid(data["guid"]));
		result->Name = data["name"].get<std::string>();
		result->Shader  = ResourceManager::GetShader(Guid(data["shader"]));

		// material specific parameters
		result->Texture = ResourceManager::GetTexture(Guid(data["texture"]));
		result->Shininess = data["shininess"].get<float>();
		return result;
	}

	/// <summary>
	/// Converts this material into it's JSON representation for storage
	/// </summary>
	nlohmann::json ToJson() const {
		return {
			{ "guid", GetGUID().str() },
			{ "name", Name },
			{ "shader", Shader ? Shader->GetGUID().str() : "" },
			{ "texture", Texture ? Texture->GetGUID().str() : "" },
			{ "shininess", Shininess },
		};
	}
};

// Helper structure to represent an object 
// with a transform, mesh, and material
struct RenderObject {
	// Human readable name for the object
	std::string             Name;
	// Unique ID for the object
	Guid                    GUID;
	// The object's world transform
	glm::mat4               Transform;
	// The object's mesh
	VertexArrayObject::Sptr Mesh;
	// The object's material
	MaterialInfo::Sptr      Material;

	// If we want to use MeshFactory, we can populate this list
	std::vector<MeshBuilderParam> MeshBuilderParams;

	// Position of the object
	glm::vec3 Position;
	// Rotation of the object in Euler angler
	glm::vec3 Rotation;
	// The scale of the object
	glm::vec3 Scale;

	RenderObject() :
		Name("Unknown"),
		GUID(Guid::New()),
		Transform(MAT4_IDENTITY),
		Mesh(nullptr),
		Material(nullptr),
		MeshBuilderParams(std::vector<MeshBuilderParam>()),
		Position(ZERO),
		Rotation(ZERO),
		Scale(ONE) {}

	// Recalculates the Transform from the parameters (pos, rot, scale)
	void RecalcTransform() {
		Rotation = glm::fmod(Rotation, glm::vec3(360.0f)); // Wrap all our angles into the 0-360 degree range
		Transform = glm::translate(MAT4_IDENTITY, Position) * glm::mat4_cast(glm::quat(glm::radians(Rotation))) * glm::scale(MAT4_IDENTITY, Scale);
	}

	// Regenerates this object's mesh if it is using the MeshFactory
	void GenerateMesh() {
		if (MeshBuilderParams.size() > 0) {
			if (Mesh != nullptr) {
				LOG_WARN("Overriding existing mesh!");
			}
			MeshBuilder<VertexPosNormTexCol> mesh;
			for (int ix = 0; ix < MeshBuilderParams.size(); ix++) {
				MeshFactory::AddParameterized(mesh, MeshBuilderParams[ix]);
			}
			Mesh = mesh.Bake();
		}
	}

	/// <summary>
	/// Loads a render object from a JSON blob
	/// </summary>
	static RenderObject FromJson(const nlohmann::json& data) {
		RenderObject result = RenderObject();
		result.Name = data["name"];
		result.GUID = Guid(data["guid"]);
		result.Mesh = ResourceManager::GetMesh(Guid(data["mesh"]));
		// TODO material is not in resource manager
		//objects[ix]["material"] = obj.Material->GetGUID().str();
		result.Position = ParseJsonVec3(data["position"]);
		result.Rotation = ParseJsonVec3(data["rotation"]);
		result.Scale = ParseJsonVec3(data["scale"]);
		// If we have mesh parameters, we'll use that instead of the existing mesh
		if (data.contains("mesh_params") && data["mesh_params"].is_array()) {
			std::vector<nlohmann::json> meshbuilderParams = data["mesh_params"].get<std::vector<nlohmann::json>>();
			MeshBuilder<VertexPosNormTexCol> mesh;
			for (int ix = 0; ix < meshbuilderParams.size(); ix++) {
				MeshBuilderParam p = MeshBuilderParam::FromJson(meshbuilderParams[ix]);
				result.MeshBuilderParams.push_back(p);
				MeshFactory::AddParameterized(mesh, p);
			}
			result.Mesh = mesh.Bake();
		}
		return result;
	}

	/// <summary>
	/// Converts this object into it's JSON representation for storage
	/// </summary>
	nlohmann::json ToJson() const {
		nlohmann::json result = {
			{ "name", Name },
			{ "guid", GUID.str() },
			{ "mesh", Mesh->GetGUID().str() },
			{ "material", Material->GetGUID().str() },
			{ "position", GlmToJson(Position) },
			{ "rotation", GlmToJson(Rotation) },
			{ "scale", GlmToJson(Scale) },
		};
		if (MeshBuilderParams.size() > 0) {
			std::vector<nlohmann::json> params = std::vector<nlohmann::json>();
			params.resize(MeshBuilderParams.size());
			for (int ix = 0; ix < MeshBuilderParams.size(); ix++) {
				params[ix] = MeshBuilderParams[ix].ToJson();
			}
			result["mesh_params"] = params;
		}
		return result;
	}
};

// Helper structure for our light data
struct Light {
	glm::vec3 Position;
	glm::vec3 Color;
	// Basically inverse of how far our light goes (1/dist, approx)
	float Attenuation = 1.0f / 5.0f;
	// The approximate range of our light
	float Range = 4.0f;

	/// <summary>
	/// Loads a light from a JSON blob
	/// </summary>
	static Light FromJson(const nlohmann::json& data) {
		Light result;
		result.Position = ParseJsonVec3(data["position"]);
		result.Color = ParseJsonVec3(data["color"]);
		result.Range = data["range"].get<float>();
		result.Attenuation = 1.0f / (1.0f + result.Range);
		return result;
	}

	/// <summary>
	/// Converts this object into it's JSON representation for storage
	/// </summary>
	nlohmann::json ToJson() const {
		return {
			{ "position", GlmToJson(Position) },
			{ "color", GlmToJson(Color) },
			{ "range", Range },
		};
	}

};

// Temporary structure for storing all our scene stuffs
struct Scene {
	typedef std::shared_ptr<Scene> Sptr;

	std::unordered_map<Guid, MaterialInfo::Sptr> Materials; // Really should be in resources but meh

	// Stores all the objects in our scene
	std::vector<RenderObject>  Objects;
	// Stores all the lights in our scene
	std::vector<Light>         Lights;
	// The camera for our scene
	Camera::Sptr               Camera;

	Shader::Sptr               BaseShader; // Should think of more elegant ways of handling this

	Scene() :
		Materials(std::unordered_map<Guid, MaterialInfo::Sptr>()),
		Objects(std::vector<RenderObject>()),
		Lights(std::vector<Light>()),
		Camera(nullptr),
		BaseShader(nullptr) {} 

	/// <summary>
	/// Searches all render objects in the scene and returns the first
	/// one who's name matches the one given, or nullptr if no object
	/// is found
	/// </summary>
	/// <param name="name">The name of the object to find</param>
	RenderObject* FindObjectByName(const std::string name) {
		auto it = std::find_if(Objects.begin(), Objects.end(), [&](const RenderObject& obj) {
			return obj.Name == name;
		});
		return it == Objects.end() ? nullptr : &(*it);
	}

	/// <summary>
	/// Loads a scene from a JSON blob
	/// </summary>
	static Scene::Sptr FromJson(const nlohmann::json& data) {
		Scene::Sptr result = std::make_shared<Scene>();
		result->BaseShader = ResourceManager::GetShader(Guid(data["default_shader"]));

		LOG_ASSERT(data["materials"].is_array(), "Materials not present in scene!");
		for (auto& material : data["materials"]) {
			MaterialInfo::Sptr mat = MaterialInfo::FromJson(material);
			result->Materials[mat->GetGUID()] = mat;
		}

		LOG_ASSERT(data["objects"].is_array(), "Objects not present in scene!");
		for (auto& object : data["objects"]) {
			RenderObject obj = RenderObject::FromJson(object);
			obj.Material = result->Materials[Guid(object["material"])];
			result->Objects.push_back(obj);
		}

		LOG_ASSERT(data["lights"].is_array(), "Lights not present in scene!");
		for (auto& light : data["lights"]) {
			result->Lights.push_back(Light::FromJson(light));
		}

		// Create and load camera config
		result->Camera = Camera::Create();
		result->Camera->SetPosition(ParseJsonVec3(data["camera"]["position"]));
		result->Camera->SetForward(ParseJsonVec3(data["camera"]["normal"]));

		return result;
	}

	/// <summary>
	/// Converts this object into it's JSON representation for storage
	/// </summary>
	nlohmann::json ToJson() const {
		nlohmann::json blob;
		// Save the default shader (really need a material class)
		blob["default_shader"] = BaseShader->GetGUID().str();

		// Save materials (TODO: this should be managed by the ResourceManager)
		std::vector<nlohmann::json> materials;
		materials.resize(Materials.size());
		int ix = 0;
		for (auto& [key, value] : Materials) {
			materials[ix] = value->ToJson();
			ix++;
		}
		blob["materials"] = materials;

		// Save renderables
		std::vector<nlohmann::json> objects;
		objects.resize(Objects.size());
		for (int ix = 0; ix < Objects.size(); ix++) {
			objects[ix] = Objects[ix].ToJson();
		}
		blob["objects"] = objects;

		// Save lights
		std::vector<nlohmann::json> lights;
		lights.resize(Lights.size());
		for (int ix = 0; ix < Lights.size(); ix++) {
			lights[ix] = Lights[ix].ToJson();
		}
		blob["lights"] = lights;

		// Save camera info
		blob["camera"] = {
			{"position", GlmToJson(Camera->GetPosition()) },
			{"normal",   GlmToJson(Camera->GetForward()) }
		};

		return blob;
	}

	/// <summary>
	/// Saves this scene to an output JSON file
	/// </summary>
	/// <param name="path">The path of the file to write to</param>
	void Save(const std::string& path) {
		// Save data to file
		FileHelpers::WriteContentsToFile(path, ToJson().dump());
		LOG_INFO("Saved scene to \"{}\"", path);
	}

	/// <summary>
	/// Loads a scene from an input JSON file
	/// </summary>
	/// <param name="path">The path of the file to read from</param>
	/// <returns>A new scene loaded from the file</returns>
	static Scene::Sptr Load(const std::string& path) {
		LOG_INFO("Loading scene from \"{}\"", path);
		std::string content = FileHelpers::ReadFile(path);
		nlohmann::json blob = nlohmann::json::parse(content);
		return FromJson(blob);
	}
};

/// <summary>
/// Handles setting the shader uniforms for our light structure in our array of lights
/// </summary>
/// <param name="shader">The pointer to the shader</param>
/// <param name="uniformName">The name of the uniform (ex: u_Lights)</param>
/// <param name="index">The index of the light to set</param>
/// <param name="light">The light data to copy over</param>
void SetShaderLight(const Shader::Sptr& shader, const std::string& uniformName, int index, const Light& light) {
	std::stringstream stream;
	stream << uniformName << "[" << index << "]";
	std::string name = stream.str();

	// Set the shader uniforms for the light
	shader->SetUniform(name + ".Position", light.Position);
	shader->SetUniform(name + ".Color",    light.Color);
	shader->SetUniform(name + ".Attenuation",  light.Attenuation);
}

/// <summary>
/// Creates the shader and sets up all the lights
/// </summary>
void SetupShaderAndLights(const Shader::Sptr& shader, Light* lights, int numLights) {
	// Global light params
	shader->SetUniform("u_AmbientCol", glm::vec3(0.1f));

	// Send the lights to our shader
	shader->SetUniform("u_NumLights", numLights);
	for (int ix = 0; ix < numLights; ix++) {
		SetShaderLight(shader, "u_Lights", ix, lights[ix]);
	}
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
bool DrawLightImGui(const char* title, Light& light) {
	bool result = false;
	ImGui::PushID(&light); // We can also use pointers as numbers for unique IDs
	if (ImGui::CollapsingHeader(title)) {
		result |= ImGui::DragFloat3("Pos", &light.Position.x, 0.01f);
		result |= ImGui::ColorEdit3("Col", &light.Color.r);
		result |= ImGui::DragFloat("Range", &light.Range, 0.1f);
	}
	ImGui::PopID();
	if (result) {
		light.Attenuation = 1.0f / (light.Range + 1.0f);
	}
	return result;
}

//////////////////////////////////////////////////////
////////////////// END OF NEW ////////////////////////
//////////////////////////////////////////////////////

int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

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

	// GL states, we'll enable depth testing and backface fulling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	// The scene that we will be rendering
	Scene::Sptr scene = nullptr;

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene) {
		ResourceManager::LoadManifest("manifest.json");
		scene = Scene::Load("scene.json");
	} 
	else {
		// Create our OpenGL resources
		Guid defaultShader = ResourceManager::CreateShader({
			{ ShaderPartType::Vertex, "shaders/vertex_shader.glsl" },
			{ ShaderPartType::Fragment, "shaders/frag_blinn_phong_textured.glsl" }
		});
		Guid monkeyMesh = ResourceManager::CreateMesh("Monkey.obj");
		Guid boxTexture = ResourceManager::CreateTexture("textures/box-diffuse.png");
		Guid monkeyTex  = ResourceManager::CreateTexture("textures/monkey-uvMap.png");
		Guid anotherTex = ResourceManager::CreateTexture("textures/fish.png");

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");

		// Create an empty scene
		scene = std::make_shared<Scene>();

		// I hate this
		scene->BaseShader = ResourceManager::GetShader(defaultShader);

		// Create our materials
		MaterialInfo::Sptr boxMaterial = std::make_shared<MaterialInfo>();
		boxMaterial->Shader = scene->BaseShader;
		boxMaterial->Texture = ResourceManager::GetTexture(boxTexture);
		boxMaterial->Shininess = 8.0f;
		scene->Materials[boxMaterial->GetGUID()] = boxMaterial;

		MaterialInfo::Sptr monkeyMaterial = std::make_shared<MaterialInfo>();
		monkeyMaterial->Shader = scene->BaseShader;
		monkeyMaterial->Texture = ResourceManager::GetTexture(monkeyTex);
		monkeyMaterial->Shininess = 1.0f;
		scene->Materials[monkeyMaterial->GetGUID()] = monkeyMaterial;

		MaterialInfo::Sptr anotherMaterial = std::make_shared<MaterialInfo>();
		anotherMaterial->Shader = scene->BaseShader;
		anotherMaterial->Texture = ResourceManager::GetTexture(anotherTex);
		anotherMaterial->Shininess = 0.5f;
		scene->Materials[anotherMaterial->GetGUID()] = anotherMaterial;

		// Create some lights for our scene
		scene->Lights.resize(3);
		scene->Lights[0].Position = glm::vec3(0.0f, 1.0f, 3.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 0.0f, 0.0f);

		scene->Lights[1].Position = glm::vec3(1.0f, 0.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(1.0f, 0.0f, 0.0f);

		scene->Lights[2].Position = glm::vec3(0.0f, 1.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(1.0f, 0.0f, 0.0f);

		// Set up the scene's camera
		scene->Camera = Camera::Create();
		scene->Camera->SetPosition(glm::vec3(0, 4, 4));
		scene->Camera->LookAt(glm::vec3(0.0f));

		// Set up all our sample objects
		RenderObject plane = RenderObject();
		plane.MeshBuilderParams.push_back(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(10.0f)));
		plane.GenerateMesh();
		plane.Name = "Plane";
		plane.Material = boxMaterial;
		scene->Objects.push_back(plane);

		RenderObject square = RenderObject();
		square.MeshBuilderParams.push_back(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(0.5f)));
		square.GenerateMesh();
		square.Position = glm::vec3(0.0f, 0.0f, 2.0f);
		square.Name = "Square";
		square.Material = boxMaterial;
		scene->Objects.push_back(square);

		RenderObject monkey1 = RenderObject();
		monkey1.Position = glm::vec3(1.5f, 0.0f, 1.0f);
		monkey1.Mesh     = ResourceManager::GetMesh(monkeyMesh);
		monkey1.Material = monkeyMaterial;
		monkey1.Name = "Monkey 1";
		scene->Objects.push_back(monkey1);

		RenderObject monkey2 = RenderObject();
		monkey2.Position = glm::vec3(-1.5f, 0.0f, 1.0f);
		monkey2.Mesh     = ResourceManager::GetMesh(monkeyMesh);
		monkey2.Material = monkeyMaterial;
		monkey2.Rotation.z = 180.0f;
		monkey2.Name = "Monkey 2";
		scene->Objects.push_back(monkey2);

		RenderObject monkey3 = RenderObject();
		monkey3.Position = glm::vec3(0.0f, 0.0f, 2.0f);
		monkey3.Mesh = ResourceManager::GetMesh(monkeyMesh);
		monkey3.Material = anotherMaterial;
		monkey3.Rotation.z = 180.0f;
		monkey3.Name = "Oooga Booga";
		scene->Objects.push_back(monkey3);

		// Save the scene to a JSON file
		scene->Save("scene.json");
	}

	// Post-load setup
	SetupShaderAndLights(scene->BaseShader, scene->Lights.data(), scene->Lights.size());

	RenderObject* monkey1 = scene->FindObjectByName("Monkey 1");
	RenderObject* monkey2 = scene->FindObjectByName("Monkey 2");
	RenderObject* monkey3 = scene->FindObjectByName("Oooga Booga");

	// We'll use this to allow editing the save/load path
	// via ImGui, note the reserve to allocate extra space
	// for input!
	std::string scenePath = "scene.json";
	scenePath.reserve(256);

	bool isRotating = true;

	// Our high-precision timer
	double lastFrame = glfwGetTime();

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
			// Make a checkbox for the monkey rotation
			ImGui::Checkbox("Rotating", &isRotating);

			// Make a new area for the scene saving/loading
			ImGui::Separator();
			if (DrawSaveLoadImGui(scene, scenePath)) {
				// Re-initialize lights, as they may have moved around
				SetupShaderAndLights(scene->BaseShader, scene->Lights.data(), scene->Lights.size());

				// Re-fetch the monkeys so we can do a behaviour for them
				monkey1 = scene->FindObjectByName("Monkey 1");
				monkey2 = scene->FindObjectByName("Monkey 2");
				monkey3 = scene->FindObjectByName("Oooga Booga");
			}
			ImGui::Separator();
		}

		// Rotate our models around the z axis at 90 deg per second
		if (isRotating) {
			monkey1->Rotation += glm::vec3(0.0f, 0.0f, dt * 90.0f);
			monkey2->Rotation -= glm::vec3(0.0f, 0.0f, dt * 90.0f);
			monkey3->Rotation += glm::vec3(0.0f, dt * 90.0f, dt * 90.0f);
		}

		// Clear the color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Grab shorthands to the camera and shader from the scene
		Shader::Sptr shader = scene->BaseShader;
		Camera::Sptr camera = scene->Camera;

		// Bind our shader for use
		shader->Bind();

		// Update our application level uniforms every frame
		shader->SetUniform("u_CamPos", scene->Camera->GetPosition());

		// Draw some ImGui stuff for the lights
		if (isDebugWindowOpen) {
			for (int ix = 0; ix < scene->Lights.size(); ix++) {
				char buff[256];
				sprintf_s(buff, "Light %d##%d", ix, ix);
				if (DrawLightImGui(buff, scene->Lights[ix])) {
					SetShaderLight(shader, "u_Lights", ix, scene->Lights[ix]);
				}
			}
			// Split lights from the objects in ImGui
			ImGui::Separator();
		}


		// Render all our objects
		for (int ix = 0; ix < scene->Objects.size(); ix++) {
			RenderObject* object = &scene->Objects[ix];

			// Update the object's transform for rendering
			object->RecalcTransform();

			// Set vertex shader parameters
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * object->Transform);
			shader->SetUniformMatrix("u_Model", object->Transform);
			shader->SetUniformMatrix("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(object->Transform))));

			// Apply this object's material
			object->Material->Apply();

			// Draw the object
			object->Mesh->Draw();

			// If our debug window is open, then let's draw some info for our objects!
			if (isDebugWindowOpen) {
				// All these elements will go into the last opened window
				if (ImGui::CollapsingHeader(object->Name.c_str())) {
					ImGui::PushID(ix); // Push a new ImGui ID scope for this object
					ImGui::DragFloat3("Position", &object->Position.x, 0.01f);
					ImGui::DragFloat3("Rotation", &object->Rotation.x, 1.0f);
					ImGui::DragFloat3("Scale",    &object->Scale.x, 0.01f, 0.0f);
					ImGui::PopID(); // Pop the ImGui ID scope for the object
				}
			}
		}

		// If our debug window is open, notify that we no longer will render new
		// elements to it
		if (isDebugWindowOpen) {
			ImGui::End();
		}

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