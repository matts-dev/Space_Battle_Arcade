#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stdio.h>
#include <assimp/matrix4x4.h>

namespace
{
	aiMatrix4x4 toAiMat4(glm::mat4 glmMat4)
	{
		aiMatrix4x4 result;

		//row 1
		result.a1 = glmMat4[0][0];
		result.a2 = glmMat4[1][0];
		result.a3 = glmMat4[2][0];
		result.a4 = glmMat4[3][0];

		//row 2
		result.b1 = glmMat4[0][1];
		result.b2 = glmMat4[1][1];
		result.b3 = glmMat4[2][1];
		result.b4 = glmMat4[3][1];

		//row 3
		result.c1 = glmMat4[0][2];
		result.c2 = glmMat4[1][2];
		result.c3 = glmMat4[2][2];
		result.c4 = glmMat4[3][2];

		//row 4
		result.d1 = glmMat4[0][3];
		result.d2 = glmMat4[1][3];
		result.d3 = glmMat4[2][3];
		result.d4 = glmMat4[3][3];

		return result;
	}

	glm::mat4 toGlmMat4(aiMatrix4x4 aiMat4)
	{
		glm::mat4 result;
		//ai row 0
		result[0][0] = aiMat4.a1;
		result[1][0] = aiMat4.a2;
		result[2][0] = aiMat4.a3;
		result[3][0] = aiMat4.a4;

		//ai row 1
		result[0][1] = aiMat4.b1;
		result[1][1] = aiMat4.b2;
		result[2][1] = aiMat4.b3;
		result[3][1] = aiMat4.b4;

		//ai row 2
		result[0][2] = aiMat4.c1;
		result[1][2] = aiMat4.c2;
		result[2][2] = aiMat4.c3;
		result[3][2] = aiMat4.c4;

		//ai row 3
		result[0][3] = aiMat4.d1;
		result[1][3] = aiMat4.d2;
		result[2][3] = aiMat4.d3;
		result[3][3] = aiMat4.d4;

		return result;
	}

	void truemain()
	{
		glm::vec3 transVecSrc(3, 4, 5);
		glm::mat4 translate(1.f);
		/*
		translation matrix using column vectors
		1 0 0 X			1 0 0 1
		0 1 0 Y		=	0 1 0 2
		0 0 1 Z			0 0 1 3
		0 0 0 1			0 0 0 1
		*/
		translate = glm::translate(translate, transVecSrc);
		std::cout << "translation vector " << transVecSrc.x << " " << transVecSrc.y << " " << transVecSrc.z << std::endl;

		auto readTransFromGlm = [](glm::mat4& mat) {
			std::cout << "glm" << std::endl;
			std::cout << "\t" << mat[3][0] << std::endl;
			std::cout << "\t" << mat[3][1] << std::endl;
			std::cout << "\t" << mat[3][2] << std::endl;
			std::cout << "\t" << mat[3][3] << std::endl;
			std::cout << std::endl;
		};
		readTransFromGlm(translate);

		aiMatrix4x4 convertToAiMat4 = toAiMat4(translate);
		std::cout << "ai" << std::endl;
		std::cout << "\t" << convertToAiMat4.a4 << std::endl;
		std::cout << "\t" << convertToAiMat4.b4 << std::endl;
		std::cout << "\t" << convertToAiMat4.c4 << std::endl;
		std::cout << "\t" << convertToAiMat4.d4 << std::endl;
		std::cout << std::endl;

		glm::mat4 backConvert = toGlmMat4(convertToAiMat4);
		readTransFromGlm(backConvert);

		/* output
		
		translation vector 3 4 5
		glm
				3
				4
				5
				1

		ai
				3
				4
				5
				1

		glm
				3
				4
				5
				1
		
		*/

		std::cin.get();
	}
}



//int main()
//{
//	truemain();
//	return 0;
//}