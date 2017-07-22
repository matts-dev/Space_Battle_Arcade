#include"Utilities.h"
#include<fstream>
#include<sstream>

namespace Utils {

	bool convertFileToString(const std::string filePath, std::string& strRef)
	{
		std::ifstream inFileStream;
		inFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);//enable these type of exceptions
		std::stringstream strStream;

		//instead of using exceptions, we can check inFileStream.is_open(); but it is good to learn some new stuff! (though I've heard c++ community doesn't like exceptions right now)
		try {
			inFileStream.open(filePath);
			strStream << inFileStream.rdbuf();
			strRef = strStream.str();

			//clean up
			inFileStream.close();
			strStream.str(std::string()); //clear string stream
			strStream.clear(); //clear error states
		}
		catch (std::ifstream::failure e) {
			std::cerr << "failed to open file\n" << e.what() << std::endl;
			return false;
		}

		return true;
	}
}
