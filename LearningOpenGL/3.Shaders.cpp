#pragma once
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include<string>
#include<iostream>
#include<fstream>
#include<sstream>

#include"Utilities.h"

int main() {
	std::ifstream inFileStream;
	inFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);//enable these type of exceptions
	std::stringstream strStream;

	std::string vertexShaderSrc, fragShaderSrc;
	if (!Utils::convertFileToString("BasicVertexShader.glsl", vertexShaderSrc)) { std::cerr << "failed to load vertex shader src" << std::endl; return -1; }
	if (!Utils::convertFileToString("BasicFragShader.glsl", fragShaderSrc)) { std::cerr << "failed to load frag shader src" << std::endl; return -1; }

	std::cout << vertexShaderSrc << std::endl;
	std::cout << fragShaderSrc << std::endl;

	int x;
	std::cin >> x;

	return 0;
}