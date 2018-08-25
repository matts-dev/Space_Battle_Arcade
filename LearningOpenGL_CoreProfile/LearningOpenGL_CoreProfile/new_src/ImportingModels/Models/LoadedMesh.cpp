#include "LoadedMesh.h"


LoadedMesh::LoadedMesh(std::vector<Vertex> vertices, std::vector<MaterialTexture> textures, std::vector<unsigned int> indices) : vertices(vertices), textures(textures), indices(indices)
{
	setupMesh();
}

LoadedMesh::~LoadedMesh()
{

}
