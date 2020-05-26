#include "SAUtilities.h"

#include<fstream>
#include<sstream>
#include "../../../../Libraries/stb_image.h"
#include <complex>
#include <vector>
#include "../Rendering/SAShader.h"
#include "../Rendering/OpenGLHelpers.h"

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

			GLuint textureID=0;
			ec(glGenTextures(1, &textureID));

			if (texture_unit >= 0)
			{
				ec(glActiveTexture(texture_unit));
			}
			ec(glBindTexture(GL_TEXTURE_2D, textureID));

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

			ec(glTexImage2D(GL_TEXTURE_2D, 0, mode, img_width, img_height, 0, dataFormat, GL_UNSIGNED_BYTE, textureData));
			ec(glGenerateMipmap(GL_TEXTURE_2D));
			//ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT)); //causes issue with materials on models
			//ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT)); //causes issue with materials on models
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
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
			GLuint tmpVAO=0, tmpVBO=0;
			ec(glBindVertexArray(0));
			ec(glGenVertexArrays(1, &tmpVAO));
			ec(glBindVertexArray(tmpVAO));

			ec(glGenBuffers(1, &tmpVBO));
			ec(glBindBuffer(GL_ARRAY_BUFFER, tmpVBO));
			ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * linesToDraw.size(), &linesToDraw[0], GL_STATIC_DRAW));
			ec(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0)));
			ec(glEnableVertexAttribArray(0));

			ec(glDrawArrays(GL_LINES, 0, linesToDraw.size()));

			ec(glDeleteVertexArrays(1, &tmpVAO));
			ec(glDeleteBuffers(1, &tmpVBO));
		}

		glm::vec3 getDifferentVector(glm::vec3 vec)
		{
			if (vec.x < vec.y && vec.x < vec.z)
			{
				vec.x = 1.f;
			}
			else if (vec.y < vec.x && vec.y < vec.z)
			{
				vec.y = 1.f;
			}
			else if (vec.z < vec.x && vec.z < vec.y)
			{
				vec.z = 1.f;
			}
			else //all are equal
			{
				if (vec.x > 0.f)
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

		glm::vec3 getDifferentNonparallelVector(const glm::vec3& vec)
		{
			using namespace glm;

			vec3 diffVec = getDifferentVector(vec);

			//this above algorithm will produce parallel vectors if given unit vectors; check for that case.
			//if we detect any special cases, tweak one of the zero components to be something non-zero
			if (bool zIsOne = (diffVec.x == 0.f && diffVec.y == 0.f))
			{
				diffVec.x = 0.5f;
			}
			else if (bool xIsOne = diffVec.y == 0.f && diffVec.z == 0.f)
			{
				diffVec.y = 0.5f;
			}
			else if (bool yIsOne = diffVec.z == 0.f && diffVec.x == 0.f)
			{
				diffVec.z = 0.5f;
			}

			return diffVec;
		}

		glm::vec3 getDifferentNonparallelVector_slowwarning(const glm::vec3& vec)
		{
			using namespace glm;
			//the getDifferentVec implementation from graphics book seems to cause issues when passing normal axis aligned vecs
			//trying this out by doing 2 rotations. Since a vector may be aligned with rotation axis, it wouldn't rotate. Thus a 
			//second rotation is used to using a different rotation axis; this means a vector must have been rotated.

			//using non-90 degree rotations so that this doesn't create orthonormal vectors, which may give unexpected
			//results when doing dot products. However, it may be useful to provide "getDifferentOrthonormalVec" that 
			//does do that behavior. It can work similar to this but with 90 degree rotations; but edge cases may need to be addressed (ie vec matches 1 axis of rtoation)

			//using static const version of quat so that calculation is only done once, the only overhead is applying the quaternion to the vector
			static const glm::quat fixedRotation = glm::angleAxis(glm::radians(33.f), vec3(1, 0, 0)) * glm::angleAxis(glm::radians(33.f), vec3(0, 1, 0));
			
			//turns out this is very expensive (involves 2 cross products, etc) and may be slower than brancher; profiling is needed here
			return fixedRotation * vec;
		}

		float getRadianAngleBetween(const glm::vec3& from_n, const glm::vec3& to_n)
		{
			//clamp required because dot can produce values outside range of [-1,1] which will cause acos to return nan.
			float cosTheta = glm::clamp(glm::dot(from_n, to_n), -1.f, 1.f);
			float rotRadians = glm::acos(cosTheta);

			return rotRadians;
		}

		float getCosBetween(const glm::vec3& from_n, const glm::vec3& to_n)
		{
			float cosTheta = glm::clamp(glm::dot(from_n, to_n), -1.f, 1.f);
			return cosTheta;
		}

		float getDegreeAngleBetween(const glm::vec3& from_n, const glm::vec3& to_n)
		{
			return (180.f / glm::pi<float>()) * getRadianAngleBetween(from_n, to_n);
		}

		glm::quat getRotationBetween(const glm::vec3& from_n, const glm::vec3& to_n)
		{
			glm::quat rot = glm::quat{ 1, 0, 0, 0 };//unit quaternion;

			float cosTheta = glm::clamp(glm::dot(from_n, to_n), -1.f, 1.f);

			bool bVectorsAre180 = Utils::float_equals(cosTheta, -1.0f);
			bool bVectorsAreSame = Utils::float_equals(cosTheta, 1.0f);

			if (!bVectorsAreSame && !bVectorsAre180)
			{
				glm::vec3 rotAxis = glm::normalize(glm::cross(from_n, to_n));
				float rotAngle_rad = glm::acos(cosTheta);
				rot = glm::angleAxis(rotAngle_rad, rotAxis);
			}
			else if (bVectorsAre180)
			{
				//if tail end and front of projectile are not the same, we need a 180 rotation around ?any? axis
				glm::vec3 temp = Utils::getDifferentVector(from_n);

				bool bTemp180 = Utils::float_equals(glm::clamp(glm::dot(from_n, temp), -1.f, 1.f), -1.0f);
				if (bTemp180) { temp += glm::vec3(1.f); }

				glm::vec3 rotAxisFor180 = glm::normalize(cross(from_n, temp));

				rot = glm::angleAxis(glm::pi<float>(), rotAxisFor180);
			}

			return rot;
		}

		glm::quat degreesVecToQuat(const glm::vec3& rotationInDegrees)
		{
			//#TODO i think you can just construct quat like : quat(radX, radY, radZ);
			glm::quat x = glm::angleAxis(glm::radians(rotationInDegrees.x), glm::vec3(1, 0, 0));
			glm::quat y = glm::angleAxis(glm::radians(rotationInDegrees.y), glm::vec3(0, 1, 0));
			glm::quat z = glm::angleAxis(glm::radians(rotationInDegrees.z), glm::vec3(0, 0, 1));
			return x * y * z;
		}

		glm::vec3 findBoxLow(const std::array<glm::vec4, 8>& localAABB)
		{
			glm::vec3 min = glm::vec3(std::numeric_limits<float>::infinity());
			for (const glm::vec4& vertex : localAABB)
			{
				min.x = vertex.x < min.x ? vertex.x : min.x;
				min.y = vertex.y < min.y ? vertex.y : min.y;
				min.z = vertex.z < min.z ? vertex.z : min.z;
			}
			return min;
		}

		glm::vec3 findBoxMax(const std::array<glm::vec4, 8>& localAABB)
		{
			glm::vec3 max = glm::vec3(-std::numeric_limits<float>::infinity());
			for (const glm::vec4& vertex : localAABB)
			{
				max.x = vertex.x > max.x ? vertex.x : max.x;
				max.y = vertex.y > max.y ? vertex.y : max.y;
				max.z = vertex.z > max.z ? vertex.z : max.z;
			}
			return max;
		}

		bool rayHitTest_FastAABB(const glm::vec3& boxLow, const glm::vec3& boxMax, const glm::vec3 rayStart, const glm::vec3 rayDir)
		{
			//FAST RAY BOX INTERSECTION
			//intersection = s + t*d;		where s is the start and d is the direction
			//for an axis aligned box, we can look at each axis individually
			//
			//intersection_x = s_x + t_x * d_x
			//intersection_y = s_y + t_y * d_y
			//intersection_z = s_z + t_z * d_z
			//
			//for each of those, we can solve for t
			//eg: (intersection_x - s_x) / d_z = t_z
			//intersection_x can be easily found since we have an axis aligned box, and there are 2 yz-planes that represent x values a ray will have to pass through
			//
			//intuitively, a ray that DOES intersect will pass through 3 planes before entering the cube; and pass through 3 planes to exit the cube.
			//the last plane it intersects when entering the cube, is the t value for the box intersection.
			//		eg ray goes through X plane, Y plane, and then Z Plane, the intersection point is the t value associated with the Z plane
			//the first plane it intersects when it leaves the box is also its exit intersection t value
			//		eg ray goes leaves Y plane, X plane, Z plane, then the intersection of the Y plane is the intersection point
			//if the object doesn't collide, then it will exit a plane before all 3 entrance places are intersected
			//		eg ray Enters X Plane, Enters Y plane, Exits X PLane, Enters Z plane, Exits Y plane, Exits Z plane; 
			//		there is no collision because it exited the x plane before it penetrated the z plane
			//it seems that, if it is within the cube, the entrance planes will all have negative t values

			////calculate bounding box dimensions; may need to nudge intersection pnt so it doesn't pick up previous box
			float fX = boxLow.x;
			float cX = boxMax.x;
			float fY = boxLow.y;
			float cY = boxMax.y;
			float fZ = boxLow.z;
			float cZ = boxMax.z;

			//use algbra to calculate T when these planes are hit; similar to above example -- looking at single components
			// pnt = s + t * d;			t = (pnt - s)/d
			//these calculations may produce infinity
			float tMaxX = (fX - rayStart.x) / rayDir.x;
			float tMinX = (cX - rayStart.x) / rayDir.x;
			float tMaxY = (fY - rayStart.y) / rayDir.y;
			float tMinY = (cY - rayStart.y) / rayDir.y;
			float tMaxZ = (fZ - rayStart.z) / rayDir.z;
			float tMinZ = (cZ - rayStart.z) / rayDir.z;

			float x_enter_t = std::min(tMinX, tMaxX);
			float x_exit_t = std::max(tMinX, tMaxX);
			float y_enter_t = std::min(tMinY, tMaxY);
			float y_exit_t = std::max(tMinY, tMaxY);
			float z_enter_t = std::min(tMinZ, tMaxZ);
			float z_exit_t = std::max(tMinZ, tMaxZ);

			float enterTs[3] = { x_enter_t, y_enter_t, z_enter_t };
			std::size_t numElements = sizeof(enterTs) / sizeof(enterTs[0]);
			std::sort(enterTs, enterTs + numElements);

			float exitTs[3] = { x_exit_t, y_exit_t, z_exit_t };
			std::sort(exitTs, exitTs + numElements);

			//handle cases where infinity is within enterT
			for (int idx = numElements - 1; idx >= 0; --idx)
			{
				if (enterTs[idx] != std::numeric_limits<float>::infinity())
				{
					//move a real value to the place where infinity was sorted
					enterTs[2] = enterTs[idx];
					break;
				}
			}

			bool intersects = enterTs[2] <= exitTs[0];
			float collisionT = enterTs[2];

			//calculate new intersection point!
			//glm::vec3 intersectPoint = start + collisionT * rayDir;

			return intersects;
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
