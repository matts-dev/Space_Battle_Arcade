
#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../../../../Algorithms/SeparatingAxisTheorem/SATComponent.h"
#include "../../Tools/ModelLoading/SAModel.h"
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SAGameEntity.h"
#include "../../Tools/SACollisionHelpers.h"

namespace
{
	using namespace SA;
	using TriangleCCW = SAT::DynamicTriangleMeshShape::TriangleProcessor::TriangleCCW;
	using TriangleProcessor = SAT::DynamicTriangleMeshShape::TriangleProcessor;

	int trueMain()
	{
		//glfw just to get a valid opengl context for testing model
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		GLFWwindow* window = glfwCreateWindow(800, 600, "OpenglContext", nullptr, nullptr);
		if (!window)
		{
			std::cerr << "failed to create window" << std::endl;
			return -1;
		}
		glfwMakeContextCurrent(window);
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cerr << "failed to initialize glad with processes " << std::endl;
			return -2;
		}

		while(!glfwWindowShouldClose(window))
		{
			
			glfwSwapBuffers(window);
			glfwPollEvents();

			//test if leak on assimp importer
			const char* modelPath = "./GameData/mods/SpaceArcade/Assets/Models3D/Carrier/SGCarrier.coll.boxbetweenboosters.obj";
#define TEST_ASSIMP 0
#if TEST_ASSIMP
			{
				Assimp::Importer importer; //when this dtor is hit, it cleans up the memory.
				const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices); //smooth normals required for bob model, adding aiProcessJointIdenticavertices to better match tutorial
				if (!scene)
				{
					std::cerr << "failed to load scene" << std::endl;
				}
			}	//memory freed when leaving scope
#endif //TEST_ASSIMP

#define TEST_MODEL3D 1
#if TEST_MODEL3D
			{ //scope so that dtor cleans up smart pointers
				sp<Model3D> newModel = nullptr;
				try
				{
					newModel = new_sp<Model3D>(modelPath);
				}
				catch (...)
				{
					//log(__FUNCTION__, LogLevel::LOG, "Failed to load collision model");
					std::cerr << "Failed to load collision model" << std::endl;
				}
			}
#endif //TEST_MODEL3D
		}

		return 0;
	}
}

//int main()
//{
//	int result = trueMain();
//	return result;
//}