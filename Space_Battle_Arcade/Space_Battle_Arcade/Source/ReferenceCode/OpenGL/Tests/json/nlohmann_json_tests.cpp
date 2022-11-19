#include<iostream>

#include "Libraries/nlohmann/json.hpp"
#include <string>
#include <fstream>
#include <filesystem>
#include <system_error>

using json = nlohmann::json;

namespace
{
	void true_main()
	{
		json j = {
			{"pi", 3.141},
			{"happy", true},
			{"name", "Neils"},
			{"nothing", nullptr },
			{"answer", {
				{ "everything", 42 }
			}},
			{"list", {1, 0, 2}},
			{"object", {
				{ "currency", "USD"},
				{"value", 42.99}
			}}
		};

		j["name"] = "matt";
		std::cout << j["name"] << std::endl;
		std::cout << j["object"]["currency"] << std::endl;

		//you can do checks too
		json subobj = j["object"];
		if (!subobj.is_null())
		{
			std::cout << "object's not null, its value is " << subobj["value"] << std::endl;
		}

		/////////////////////////////////////////////////////////////////////////////
		//serializing to/from string
		/////////////////////////////////////////////////////////////////////////////
		std::string serialization = j.dump(4);
		std::cout << serialization << std::endl;

		json read_j = json::parse(serialization);
		std::cout << read_j << std::endl;

		/////////////////////////////////////////////////////////////////////////////
		//quotes get removed when converting to string type!
		/////////////////////////////////////////////////////////////////////////////
		std::string name = read_j["name"];
		std::cout << "converting to string " << name << std::endl;

		/////////////////////////////////////////////////////////////////////////////
		// writing to disk
		/////////////////////////////////////////////////////////////////////////////
		std::string dir = "./DELETE_ME_LIBRARY_SERIALIZATION_TESTS/json/";

		std::error_code mkdir_ec;
		//std::filesystem::create_directory("./library_tests", mkdir_ec); //must manually create parent directories with create_directory
		std::filesystem::create_directories(dir, mkdir_ec); //create directories will make the paths you need along the way
		if (!mkdir_ec)	{ std::cout << "made directory" << std::endl; }
		else { std::cout << "mkdir error:" << mkdir_ec.message() << std::endl; }

		std::string file_path = dir + "test.json";
		{
			std::ofstream outFile(file_path, std::fstream::trunc);
			if (outFile.is_open())
			{
				//outFile << j; //alternatively you can just write the string it produces 
				outFile << j.dump(/*indention*/4); //use 4 indention
			} else { std::cerr << "couldn't open file for writing" << std::endl; }
		}

		/////////////////////////////////////////////////////////////////////////////
		// reading from the disk
		/////////////////////////////////////////////////////////////////////////////
		json file_j;
		{
			std::ifstream inFile(file_path);
			if (inFile.is_open())
			{
				inFile >> file_j;
			} else { std::cerr << "couldn't open file for reading" << std::endl; }
		}

		std::cout << "json read from disk directly using provided serialization" << std::endl;
		std::cout << file_j << std::endl;

		std::cin.get();
	}
}

//int main()
//{
//	true_main();
//}