#include "SAUtilities.h"

#include<fstream>
#include<sstream>
#include "..\..\..\..\Libraries\stb_image.h"
#include <complex>
#include <vector>
#include "..\Rendering\SAShader.h"

namespace SA
{
	namespace Utils
	{
		bool readFileToString(const std::string filePath, std::string& strRef)
		{
			std::ifstream inFileStream;
			inFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);//enable these type of exceptions
			std::stringstream strStream;

			//instead of using exceptions, inFileStream.is_open(); can be used
			try
			{
				inFileStream.open(filePath);
				strStream << inFileStream.rdbuf();
				strRef = strStream.str();

				//clean up
				inFileStream.close();
				strStream.str(std::string()); //clear string stream
				strStream.clear(); //clear error states
			}
			catch (std::ifstream::failure e)
			{
				std::cerr << "failed to open file\n" << e.what() << std::endl;
				return false;
			}

			return true;
		}

		GLFWwindow* initWindow(const int width, const int height)
		{
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

			GLFWwindow* window = glfwCreateWindow(width, height, "OpenGL Window", nullptr, nullptr); 
			if (!window)
			{
				glfwTerminate();
				return nullptr;
			}

			glfwMakeContextCurrent(window);

			if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress)))
			{
				std::cerr << "Failed to set up GLAD" << std::endl;
				glfwTerminate();
				return nullptr;
			}

			return window;
		}

		GLuint loadTextureToOpengl(const char* relative_filepath, int texture_unit /*= -1*/, bool useGammaCorrection /*= false*/)
		{
			int img_width, img_height, img_nrChannels;
			unsigned char* textureData = stbi_load(relative_filepath, &img_width, &img_height, &img_nrChannels, 0);
			if (!textureData)
			{
				std::cerr << "failed to load texture" << std::endl;
				exit(-1);
			}

			GLuint textureID;
			glGenTextures(1, &textureID);

			if (texture_unit >= 0)
			{
				glActiveTexture(texture_unit);
			}
			glBindTexture(GL_TEXTURE_2D, textureID);

			int mode = -1;
			int dataFormat = -1;
			if (img_nrChannels == 3)
			{
				mode = useGammaCorrection ? GL_SRGB : GL_RGB;
				dataFormat = GL_RGB;
			}
			else if (img_nrChannels == 4)
			{
				mode = useGammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
				dataFormat = GL_RGBA;
			}
			else if (img_nrChannels == 1)
			{
				mode = GL_RED;
				dataFormat = GL_RED;
			}
			else
			{
				std::cerr << "unsupported image format for texture at " << relative_filepath << " there are " << img_nrChannels << "channels" << std::endl;
				exit(-1);
			}

			glTexImage2D(GL_TEXTURE_2D, 0, mode, img_width, img_height, 0, dataFormat, GL_UNSIGNED_BYTE, textureData);
			glGenerateMipmap(GL_TEXTURE_2D);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); //causes issue with materials on models
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT); //causes issue with materials on models
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			stbi_image_free(textureData);

			return textureID;
		}

		void renderDebugWireCube(SA::Shader& debugShader, const glm::vec3 color, const glm::mat4& model, const glm::mat4& view, const glm::mat4& perspective)
		{
			//would be much faster if we just render a unit cube VBO and transform it in polygonmode lines, 
			//this should be regarded as less-than-ideal immediate_mode-like method but a quick and dirty way to test things

			glm::vec4 FBR(0.5f, -0.5f, 0.5f,		1.f);
			glm::vec4 FTR(0.5f, 0.5f, 0.5f,			1.f);
			glm::vec4 FTL(-0.5f, 0.5f, 0.5f,		1.f);
			glm::vec4 FBL(-0.5f, -0.5f, 0.5f,		1.f);
			glm::vec4 BBR(0.5f, -0.5f, -0.5f,		1.f);
			glm::vec4 BTR(0.5f, 0.5f, -0.5f,		1.f);
			glm::vec4 BTL(-0.5f, 0.5f, -0.5f,		1.f);
			glm::vec4 BBL(-0.5f, -0.5f, -0.5f,		1.f);			

			std::vector<glm::vec4> linesToDraw;
			
			linesToDraw.push_back(FBR);
			linesToDraw.push_back(FTR);
			linesToDraw.push_back(FBR);
			linesToDraw.push_back(FBL);
			linesToDraw.push_back(FTR);
			linesToDraw.push_back(FTL);
			linesToDraw.push_back(FBL);
			linesToDraw.push_back(FTL);

			linesToDraw.push_back(BBR);
			linesToDraw.push_back(BTR);
			linesToDraw.push_back(BBR);
			linesToDraw.push_back(BBL);
			linesToDraw.push_back(BTR);
			linesToDraw.push_back(BTL);
			linesToDraw.push_back(BBL);
			linesToDraw.push_back(BTL);

			linesToDraw.push_back(FBR);
			linesToDraw.push_back(BBR);
			linesToDraw.push_back(FTR);
			linesToDraw.push_back(BTR);
			linesToDraw.push_back(FTL);
			linesToDraw.push_back(BTL);
			linesToDraw.push_back(FBL);
			linesToDraw.push_back(BBL);
			
			/* This method is extremely slow and non-performant; use only for debugging purposes */
			debugShader.use();
			debugShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			debugShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			debugShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(perspective));
			debugShader.setUniform3f("color", color);

			//basically immediate mode, should be very bad performance
			GLuint tmpVAO, tmpVBO;
			glGenVertexArrays(1, &tmpVAO);
			glBindVertexArray(tmpVAO);

			glGenBuffers(1, &tmpVBO);
			glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * linesToDraw.size(), &linesToDraw[0], GL_STATIC_DRAW);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
			glEnableVertexAttribArray(0);

			glDrawArrays(GL_LINES, 0, linesToDraw.size());

			glDeleteVertexArrays(1, &tmpVAO);
			glDeleteBuffers(1, &tmpVBO);
		}

		glm::vec3 getDifferentVector(glm::vec3 vec)
		{
			if (vec.x < vec.y && vec.z)
			{
				vec.x = 1.f;
			}
			else if (vec.y < vec.x && vec.z)
			{
				vec.y = 1.f;
			}
			else if (vec.z < vec.x && vec.y)
			{
				vec.z = 1.f;
			}
			else //all are equal
			{
				if (vec.x == 1.f)
				{
					vec.x = -1.f;
				}
				else
				{
					vec.x = 1.f;
				}
			}
			return vec;
		}

		glm::quat getRotationBetween(const glm::vec3& from_n, const glm::vec3& to_n)
		{
			glm::quat rot; //unit quaternion;

			float cosTheta = glm::dot(from_n, to_n);

			bool bVectorsAre180 = Utils::float_equals(cosTheta, -1.0f);
			bool bVectorsAreSame = Utils::float_equals(cosTheta, 1.0f) && !bVectorsAre180; //TODO, don't want to change this mid-refactor; but second check seems unecessary and impossible

			if (!bVectorsAreSame)
			{
				glm::vec3 rotAxis = glm::normalize(glm::cross(from_n, to_n)); //theoretically, I don't think I need to normalize if both normal; but generally I normalize the result of xproduct
				float rotDegreesRadians = glm::acos(cosTheta);
				rot = glm::angleAxis(rotDegreesRadians, rotAxis);
			}
			else if (bVectorsAre180)
			{
				//if tail end and front of projectile are not the same, we need a 180 rotation around ?any? axis
				glm::vec3 temp = Utils::getDifferentVector(from_n);
				glm::vec3 rotAxisFor180 = glm::normalize(cross(from_n, temp));

				rot = glm::angleAxis(glm::pi<float>(), rotAxisFor180);
			}

			return rot;
		}

		const float cubeVerticesWithUVs[] = {
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
			0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
			-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

			-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
		};

		//unit create cube (that matches the size of the collision cube)
		const float unitCubeVertices_Position_Normal[] = {
			//x     y       z      _____normal_xyz___
			-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
			0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
			-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

			-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
			0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
			-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

			-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
			-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
			-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

			0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
			0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
			0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
			0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
			0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
			0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

			-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
			0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
			0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
			0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

			-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
			-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
			-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
		};

		const float quadVerticesWithUVs[] = {
			//x y z					//u v		
			 0.5f,  0.5f, 0.0f,      1.0f, 1.0f,   
			-0.5f,  0.5f, 0.0f,      0.0f, 1.0f,
			 0.5f, -0.5f, 0.0f,      1.0f, 0.0f,   

			-0.5f, -0.5f, 0.0f,      0.0f, 0.0f,
			0.5f, -0.5f, 0.0f,      1.0f, 0.0f,
			-0.5f,  0.5f, 0.0f,      0.0f, 1.0f
		};

	}
}
