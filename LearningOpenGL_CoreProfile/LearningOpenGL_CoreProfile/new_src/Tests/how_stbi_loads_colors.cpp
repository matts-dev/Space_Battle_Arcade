#include "..\..\Libraries\stb_image.h"
#include <iostream>


namespace
{
	void true_main()
	{
		int img_width, img_height, img_nrChannels;
		unsigned char* textureData = stbi_load("Textures/red_texture.png", &img_width, &img_height, &img_nrChannels, 0);
		if (!textureData)
		{
			std::cerr << "failed to load texture" << std::endl;
		}
		else
		{
			std::cout << "starting byte print of first 16 bytes of texture" << std::endl;
			for (int byte = 0; byte < 16; ++byte)
			{
				std::cout << (int) textureData[byte] << std::endl;
			}
		}
		std::cout << "done" << std::endl;
	}
}

//int main()
//{
//	true_main();
//}