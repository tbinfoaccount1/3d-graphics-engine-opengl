#include <fstream>
#include <string>
#include <SDL3/SDL_image.h>
#include <filesystem>

#include "obj_loader.h"


Texture loadTexture(const std::string& filename)
{
	SDL_Surface* surface = IMG_Load(filename.c_str());
	if (!surface)
	{
		throw std::runtime_error("Could not load texture: '" + filename + "': " + std::string(SDL_GetError()));
	}

	SDL_Surface* convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
	SDL_DestroySurface(surface);
	if (!convertedSurface)
	{
		throw std::runtime_error("Could not convert texture: '" + filename + "': " + std::string(SDL_GetError()));
	}

	Texture texture;
	texture.width = convertedSurface->w;
	texture.height = convertedSurface->h;
	texture.pixels.resize(texture.width * texture.height);

	std::memcpy(texture.pixels.data(), convertedSurface->pixels, texture.pixels.size() * sizeof(uint32_t));
	SDL_DestroySurface(convertedSurface);
	return texture;
}

void loadMtlFile(const std::string& filename, std::vector<Material>& materials)
{
	std::filesystem::path filePath = std::filesystem::path(filename);
	std::ifstream file(filePath);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file: " + filePath.string());
	}
	std::string line;
	while (std::getline(file, line))
	{
		if (line.starts_with("newmtl "))
		{
			Material material;
			material.name = line.substr(7);
			materials.push_back(material);
		}
		else if (line.starts_with("Ka "))
		{
			float r, g, b;
			if (sscanf_s(line.c_str(), "Ka %f %f %f", &r, &g, &b) == 3)
			{
				materials.back().ambientColor = glm::vec3{ r, g, b };
			}
		}
		else if (line.starts_with("Kd "))
		{
			float r, g, b;
			if (sscanf_s(line.c_str(), "Kd %f %f %f", &r, &g, &b) == 3)
			{
				materials.back().diffuseColor = glm::vec3{ r, g, b };
			}
		}
		else if (line.starts_with("Ks "))
		{
			float r, g, b;
			if (sscanf_s(line.c_str(), "Ks %f %f %f", &r, &g, &b) == 3)
			{
				materials.back().specularColor = glm::vec3{ r, g, b };
			}
		}
		else if (line.starts_with("Ns "))
		{
			float shininess;
			if (sscanf_s(line.c_str(), "Ns %f", &shininess) == 1)
			{
				materials.back().shininess = shininess;
			}
		}
		else if (line.starts_with("d "))
		{
			float transparency;
			if (sscanf_s(line.c_str(), "d %f", &transparency) == 1)
			{
				materials.back().transparency = transparency;
			}
		}
		else if (line.starts_with("Ni "))
		{
			float opticalDensity;
			if (sscanf_s(line.c_str(), "Ni %f", &opticalDensity) == 1)
			{
				materials.back().opticalDensity = opticalDensity;
			}
		}
		else if (line.starts_with("Tf "))
		{
			float r, g, b;
			if (sscanf_s(line.c_str(), "Tf %f %f %f", &r, &g, &b) == 3)
			{
				materials.back().transmissionFilter = glm::vec3{ r, g, b };
			}
		}
		else if (line.starts_with("map_Ka "))
		{
			std::string textureFilename = line.substr(7);
			materials.back().ambientTexture = loadTexture((filePath.parent_path() / textureFilename).string());
		}
		else if (line.starts_with("map_Kd "))
		{
			std::string textureFilename = line.substr(7);
			materials.back().diffuseTexture = loadTexture((filePath.parent_path() / textureFilename).string());
		}
		else if (line.starts_with("map_Ks "))
		{
			std::string textureFilename = line.substr(7);
			materials.back().specularTexture = loadTexture((filePath.parent_path() / textureFilename).string());
		}
		else if (line.starts_with("map_Ns "))
		{
			std::string textureFilename = line.substr(7);
			materials.back().shininessTexture = loadTexture((filePath.parent_path() / textureFilename).string());
		}
		else if (line.starts_with("map_d "))
		{
			std::string textureFilename = line.substr(6);
			materials.back().transparencyTexture = loadTexture((filePath.parent_path() / textureFilename).string());
		}
		else if (line.starts_with("map_bump ") || line.starts_with("bump "))
		{
			std::string textureFilename;
			if (line.starts_with("map_bump "))
				textureFilename = line.substr(9);
			else
				textureFilename = line.substr(5);
			materials.back().bumpTexture = loadTexture((filePath.parent_path() / textureFilename).string());
		}
	}
}

ObjData loadObjFile(const std::string& filename)
{
	ObjData objData;
	std::filesystem::path filePath = std::filesystem::path(filename);
	std::ifstream file(filePath);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file: " + filePath.string());
	}

	std::string line;
	int currentMaterialIndex = -1;

	while (std::getline(file, line))
	{
		if (line.starts_with("v "))
		{
			float x, y, z;
			if (sscanf_s(line.c_str(), "v %f %f %f", &x, &y, &z) == 3)
			{
				objData.vertices.push_back(glm::vec3{ x, y, z });
			}
			else throw std::runtime_error("Unable to read vertex line: " + line);
		}
		else if (line.starts_with("vt "))
		{
			float x, y;
			if (sscanf_s(line.c_str(), "vt %f %f", &x, &y) == 2)
			{
				objData.textureCoords.push_back(glm::vec2{ x, y });
			}
			else throw std::runtime_error("Unable to read texture coordinate line: " + line);
		}
		else if (line.starts_with("vn "))
		{
			float x, y, z;
			if (sscanf_s(line.c_str(), "vn %f %f %f", &x, &y, &z) == 3)
			{
				objData.normals.push_back(glm::vec3{ x, y, z });
			}
			else throw std::runtime_error("Unable to read normal line: " + line);
		}
		else if (line.starts_with("f "))
		{
			ObjFace face;
			int v1, v2, v3, vt1, vt2, vt3, vn1, vn2, vn3;
			if (sscanf_s(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3) == 9)
			{
				face.vertexIndices.push_back(v1 - 1);
				face.vertexIndices.push_back(v2 - 1);
				face.vertexIndices.push_back(v3 - 1);
				face.textureCoordIndices.push_back(vt1 - 1);
				face.textureCoordIndices.push_back(vt2 - 1);
				face.textureCoordIndices.push_back(vt3 - 1);
				face.normalIndices.push_back(vn1 - 1);
				face.normalIndices.push_back(vn2 - 1);
				face.normalIndices.push_back(vn3 - 1);
				face.materialIndex = currentMaterialIndex;
				objData.faces.push_back(face);
			}
			else throw std::runtime_error("Unable to read face line: " + line);
		}
		else if (line.starts_with("usemtl "))
		{
			std::string materialName = line.substr(7);
			for (size_t i = 0; i < objData.materials.size(); ++i)
			{
				if (objData.materials[i].name == materialName)
				{
					currentMaterialIndex = static_cast<int>(i);
					break;
				}
			}
		}
		else if (line.starts_with("mtllib "))
		{
			std::string mtlFilename = line.substr(7);
			loadMtlFile((filePath.parent_path() / mtlFilename).string(), objData.materials);
		}
	}
	return objData;
}