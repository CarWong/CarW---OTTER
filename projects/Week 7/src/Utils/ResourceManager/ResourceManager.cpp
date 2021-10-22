#include "Utils/ResourceManager/ResourceManager.h"

#include "Utils/ObjLoader.h"
#include "../FileHelpers.h"

std::map<Guid, Texture2D::Sptr> ResourceManager::_textures;
std::map<Guid, VertexArrayObject::Sptr> ResourceManager::_meshes;
std::map<Guid, Shader::Sptr> ResourceManager::_shaders;
nlohmann::json ResourceManager::_manifest;

void ResourceManager::Init() {
	// TODO: initialize the resource manager once it's a bit more complex
	_manifest["textures"] = std::vector<nlohmann::json>();
	_manifest["meshes"]   = std::vector<nlohmann::json>();
	_manifest["shaders"]  = std::vector<nlohmann::json>();
}

Guid ResourceManager::LoadTexture2D(const nlohmann::json& jsonData) {
	// Get the guid of the texture from the manifest
	LOG_ASSERT(jsonData["guid"].is_string(), "JSON data must specify a GUID!");
	Guid result = Guid(jsonData["guid"].get<std::string>());
	LOG_ASSERT(result.isValid(), "Loaded GUID is not a valid GUID!");

	// We need at least the file path to load in our texture
	LOG_ASSERT(jsonData["path"].is_string(), "JSON data must specify at least the file path for a texture!");
	std::string file = jsonData["path"].get<std::string>();

	// Grab some optional parameters from the JSON data
	WrapMode    horizontalWrap = jsonData["wrap_s"].is_number_integer() ? (WrapMode)jsonData["wrap_s"].get<int>() : WrapMode::ClampToEdge;
	WrapMode    verticalWrap = jsonData["wrap_t"].is_number_integer() ? (WrapMode)jsonData["wrap_t"].get<int>() : WrapMode::ClampToEdge;
	bool        forceRGBA    = JsonGet(jsonData, "has_alpha", true);

	// Create a description from the info we've loaded
	Texture2DDescription desc = Texture2DDescription();
	desc.HorizontalWrap = horizontalWrap;
	desc.VerticalWrap   = verticalWrap;

	// Load the texture and store the result in our resources
	Texture2D::Sptr texture = Texture2D::LoadFromFile(file, desc, forceRGBA);
	texture->OverrideGUID(result);
	_textures[result] = texture;

	return result;
}

Guid ResourceManager::LoadMesh(const nlohmann::json& jsonData) {
	// Get the guid of the texture from the manifest
	LOG_ASSERT(jsonData["guid"].is_string(), "JSON data must specify a GUID!");
	Guid result = Guid(jsonData["guid"].get<std::string>());
	LOG_ASSERT(result.isValid(), "Loaded GUID is not a valid GUID!");

	// We need at least the file path to load in our mesh
	LOG_ASSERT(jsonData["path"].is_string(), "JSON data must specify at least the file path for a mesh!");
	std::string file = jsonData["path"].get<std::string>();

	// Load the texture and store the result in our resources
	VertexArrayObject::Sptr mesh = ObjLoader::LoadFromFile(file);
	mesh->OverrideGUID(result);
	_meshes[result] = mesh;

	return result;
}

Guid ResourceManager::LoadShader(const nlohmann::json& jsonData) {
	// Get the guid of the texture from the manifest
	LOG_ASSERT(jsonData["guid"].is_string(), "JSON data must specify a GUID!");
	Guid result = Guid(jsonData["guid"].get<std::string>());
	LOG_ASSERT(result.isValid(), "Loaded GUID is not a valid GUID!");

	// Get the vertex shader path
	LOG_ASSERT(jsonData["vs"].is_string(), "JSON data must specify the vertex shader path for a shader!");
	std::string vs = jsonData["vs"].get<std::string>();

	// Get the fragment shader path
	LOG_ASSERT(jsonData["fs"].is_string(), "JSON data must specify the fragment shader path for a shader!");
	std::string fs = jsonData["fs"].get<std::string>();

	// Load the shader and store the result in our resources
	Shader::Sptr shader = Shader::Create();
	shader->LoadShaderPartFromFile(vs.c_str(), ShaderPartType::Vertex);
	shader->LoadShaderPartFromFile(fs.c_str(), ShaderPartType::Fragment);
	shader->Link();
	shader->OverrideGUID(result);
	_shaders[result] = shader;

	return result;
}

Guid ResourceManager::CreateTexture(const std::string& path, const Texture2DDescription& desc /*= Texture2DDescription()*/) {
	Guid result = Guid::New();
	nlohmann::json blob;
	blob["guid"] = result.str();
	blob["path"] = path;
	blob["wrap_s"] = (int)desc.HorizontalWrap;
	blob["wrap_t"] = (int)desc.HorizontalWrap;

	_manifest["textures"].push_back(blob);
	LoadTexture2D(blob);
	return result;
}

Guid ResourceManager::CreateMesh(const std::string& path) {
	Guid result = Guid::New();
	nlohmann::json blob;
	blob["guid"] = result.str();
	blob["path"] = path;

	_manifest["meshes"].push_back(blob);
	LoadMesh(blob);
	return result;
}

Guid ResourceManager::CreateShader(const std::unordered_map<ShaderPartType, std::string>& paths) {
	Guid result = Guid::New();
	nlohmann::json blob;
	blob["guid"] = result.str();
	blob["vs"] = paths.at(ShaderPartType::Vertex);
	blob["fs"] = paths.at(ShaderPartType::Fragment);

	_manifest["shaders"].push_back(blob);
	LoadShader(blob);
	return result;
}

Texture2D::Sptr ResourceManager::GetTexture(Guid id) {
	return _textures[id];
}

VertexArrayObject::Sptr ResourceManager::GetMesh(Guid id) {
	return _meshes[id];
}

Shader::Sptr ResourceManager::GetShader(Guid id) {
	return _shaders[id];
}

const nlohmann::json& ResourceManager::GetManifest() {
	return _manifest;
}

void ResourceManager::LoadManifest(const std::string& path) {
	std::string contents = FileHelpers::ReadFile(path);
	nlohmann::json blob = nlohmann::json::parse(contents);

	LOG_ASSERT(blob["textures"].is_array(), "Textures must exist and be an array!");
	LOG_ASSERT(blob["meshes"].is_array(), "Meshes must exist and be an array!");
	LOG_ASSERT(blob["shaders"].is_array(), "Shaders must exist and be an array!");

	for (auto& texBlob : blob["textures"]) {
		ResourceManager::LoadTexture2D(texBlob);
	}

	for (auto& meshBlob : blob["meshes"]) {
		ResourceManager::LoadMesh(meshBlob);
	}

	for (auto& shaderBlob : blob["shaders"]) {
		ResourceManager::LoadShader(shaderBlob);
	}
}

void ResourceManager::SaveManifest(const std::string& path) {
	FileHelpers::WriteContentsToFile(path, _manifest.dump());
}

void ResourceManager::Cleanup() {
	_textures.clear();
	_meshes.clear();
	_shaders.clear();
}

