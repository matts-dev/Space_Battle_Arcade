#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <iostream>
#include <stdio.h>

namespace
{



	void truemain()
	{

		glm::mat4 translate(1.f);
		/*
		translation matrix using column vectors
			1 0 0 X			1 0 0 1
			0 1 0 Y		=	0 1 0 2
			0 0 1 Z			0 0 1 3
			0 0 0 1			0 0 0 1
		*/
		translate = glm::translate(translate, glm::vec3(1, 2, 3));

		std::cout << "iterate over glm matrix[i][j]" << std::endl;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				printf("%3.1f ", translate[i][j]);
			}
			printf("\n");
		}
		printf("\n");
		/*
		output: (with my comments appended)
			1.0 0.0 0.0 0.0  //column 1 (leftmost)
			0.0 1.0 0.0 0.0  //column 2 (middleleft)
			0.0 0.0 1.0 0.0  //column 3 (middleright)
			1.0 2.0 3.0 1.0  //column 4 (rightmost)
		*/

		float* itr = glm::value_ptr(translate);
		for (int i = 0; i < 16; ++i, itr += 1)
		{
			if (i % 4== 0 && i != 0)
				printf("\n");
			printf("%3.1f ", *itr);
		}
		/*
		Valueptr Output: (with my commends added)
			1.0 0.0 0.0 0.0 //column 1 (left)
			0.0 1.0 0.0 0.0 //column 2 (middle left)
			0.0 0.0 1.0 0.0 //column 3 (middle right)
			1.0 2.0 3.0 1.0 //column 4 (right)
		*/

		std::cin.get();
	}
}



//int main()
//{
//	truemain();
//	return 0;
//}