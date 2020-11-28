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

		//--------------------------------------
		std::cout << "\n\ndemoing accesses\n" << std::endl;
		std::cout << "accessing with one [] operator, each printed row is a column" << std::endl;
		for (int col = 0; col < 4; col++)
		{
			std::cout << "translationMatrix[" << col << "]: ";

			glm::vec4 colV = translate[col];
			std::cout << colV.x << " " << colV.y << " " << colV.z << " " << colV.w << std::endl;
		}
		/*
		translation matrix using column vectors
		1 0 0 X			1 0 0 1
		0 1 0 Y		=	0 1 0 2
		0 0 1 Z			0 0 1 3
		0 0 0 1			0 0 0 1
		*/
		//output from above:
		//accessing with one[] operator, each ***printed row*** is a column
		//translationMatrix[0] : 1 0 0 0
		//translationMatrix[1] : 0 1 0 0
		//translationMatrix[2] : 0 0 1 0
		//translationMatrix[3] : 1 2 3 1

		//------------------------------------------------------

		std::cout << "\nprint 3rd column for access demonstration" << std::endl;
		std::cout << "translateMatrix[2][0]: " << translate[2][0] << std::endl;
		std::cout << "translateMatrix[2][1]: " << translate[2][1] << std::endl;
		std::cout << "translateMatrix[2][2]: " << translate[2][2] << std::endl;
		std::cout << "translateMatrix[2][3]: " << translate[2][3] << std::endl;
		//this prove that above was actually printing columns not rows
		//
		//output from above
		//print 3rd column for access demonstration
		//translateMatrix[2][0] : 0
		//translateMatrix[2][1] : 0
		//translateMatrix[2][2] : 1
		//translateMatrix[2][3] : 0

		std::cin.get();
	}
}



//int main()
//{
//	truemain();
//	return 0;
//}