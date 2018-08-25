#include "Model.h"

Model::Model(const char* path)
{
	loadModel(path);
}

Model::~Model()
{

}

void Model::draw(Shader& shader)
{
	for (unsigned int i = 0; i < meshes.size(); ++i)
	{
		meshes[i].draw(shader);
	}
}
