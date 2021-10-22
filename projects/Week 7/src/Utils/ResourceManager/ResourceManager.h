#pragma once

#include <json.hpp>
#include <unordered_map>

#include "Graphics/Texture2D.h";
#include "Graphics/VertexArrayObject.h";
#include "Graphics/Shader.h";

#include "Utils/GUID.hpp"

/// <summary>
/// Utility class for managing and loading resources from JSON
/// manifest files
/// </summary>
class ResourceManager {
public:
	/// <summary>
	/// Initializes the resource manager and performs any first-time
	/// setup required
	/// </summary>
	static void Init();

	/// <summary>
	/// Loads a new 2D texture from the given JSON manifest data and returns it's GUID
	/// </summary>
	/// <param name="jsonData">The JSON object containing the texture's information</param>
	/// <returns>The texture's GUID</returns>
	static Guid LoadTexture2D(const nlohmann::json& jsonData);
	/// <summary>
	/// Loads a new mesh from the given JSON manifest data and returns it's GUID
	/// </summary>
	/// <param name="jsonData">The JSON object containing the mesh's information</param>
	/// <returns>The mesh's GUID</returns>
	static Guid LoadMesh(const nlohmann::json& jsonData);
	/// <summary>
	/// Loads a new shader from the given JSON manifest data and returns it's GUID
	/// </summary>
	/// <param name="jsonData">The JSON object containing the shader's information</param>
	/// <returns>The shader's GUID</returns>
	static Guid LoadShader(const nlohmann::json& jsonData);

	/// <summary>
	/// Creates a manifest entry for a texture with the given parameters
	/// </summary>
	/// <param name="path">The relative path of the image to load</param>
	/// <param name="desc">An optional texture desctiption to use for the image</param>
	/// <returns>A JSON blob that can be appended to a manifest</returns>
	static Guid CreateTexture(const std::string& path, const Texture2DDescription& desc = Texture2DDescription());
	/// <summary>
	/// Creates a manifest entry for a mesh with the given parameters
	/// </summary>
	/// <param name="path">The relative path of the mesh file to load (.obj file)</param>
	/// <returns>A JSON blob that can be appended to a manifest</returns>
	static Guid CreateMesh(const std::string& path);
	/// <summary>
	/// Creates a manifest entry for a shader with the given parameters
	/// </summary>
	/// <param name="paths">The paths and corresponding ShaderPartTypes for the program (note: only VS and FS are currently supported)</param>
	/// <returns>A JSON blob that can be appended to a manifest</returns>
	static Guid CreateShader(const std::unordered_map<ShaderPartType, std::string>& paths);
	
	/// <summary>
	/// Gets the texture with the given GUID, or nullptr if it has not been loaded
	/// </summary>
	/// <param name="id">The GUID of the texture to fetch</param>
	static Texture2D::Sptr GetTexture(Guid id);
	/// <summary>
	/// Gets the mesh with the given GUID, or nullptr if it has not been loaded
	/// </summary>
	/// <param name="id">The GUID of the mesh to fetch</param>
	static VertexArrayObject::Sptr GetMesh(Guid id);
	/// <summary>
	/// Gets the shader with the given GUID, or nullptr if it has not been loaded
	/// </summary>
	/// <param name="id">The GUID of the shader to fetch</param>
	static Shader::Sptr GetShader(Guid id);

	/// <summary>
	/// Gets the current JSON manifest
	/// </summary>
	static const nlohmann::json& GetManifest();
	/// <summary>
	/// Loads a manifest file into the resource manager
	/// </summary>
	/// <param name="path">The path to the JSON manifest file</param>
	static void LoadManifest(const std::string& path);
	/// <summary>
	/// Saves the manifest to the given JSON file
	/// </summary>
	/// <param name="path">The path to the file to output</param>
	static void SaveManifest(const std::string& path);

	/// <summary>
	/// Releases all resources held by the resource manager
	/// </summary>
	static void Cleanup();

protected:
	static std::map<Guid, Texture2D::Sptr> _textures;
	static std::map<Guid, VertexArrayObject::Sptr> _meshes;
	static std::map<Guid, Shader::Sptr> _shaders;

	static nlohmann::json _manifest;
};