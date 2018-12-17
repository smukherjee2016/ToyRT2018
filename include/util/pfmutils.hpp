#pragma once

#include "common/common.hpp"

#include <fstream>
#include <sstream>
#include <vector>

struct PFMInfo {
	std::vector<glm::vec3> image;
	int w = 1;
	int h = 1;
};

PFMInfo readPFM(std::string file) {
	std::ifstream inFile(file, std::ios::binary | std::ios::in);
	if (!inFile) {
		throw std::runtime_error("Cannot open input file: " + file);
	}
	std::string type;
	int w, h;
	float sFEndianness;
	inFile >> type;
	if (type.compare("PF") != 0) {
		throw std::runtime_error("Invalid or unsupported PFM format, sorry.");
	}
	inFile >> w >> h;
	inFile >> sFEndianness;
	char dummy;
	dummy = inFile.get();
	if (dummy == '\r')
		inFile.get();
	//std::cout << type << "  " << w << "  " << h << "  " << sFEndianness << "\n";
	PFMInfo returnPFM;
	returnPFM.w = w;
	returnPFM.h = h;
	returnPFM.image.resize(w*h);
	returnPFM.image.clear();
	//Ignore scale
	if (sFEndianness < 0) { //Big endian, the usual processing 
		while (!inFile.eof()) {
			float r, g, b;
			inFile.read((char*)(&r), sizeof(float));
			inFile.read((char*)(&g), sizeof(float));
			inFile.read((char*)(&b), sizeof(float));
			returnPFM.image.emplace_back(glm::vec3(r, g, b));
			inFile.peek(); //To set EOF if we have reached EOF, to prevent an extra dummy iteration of this loop

		}

	}
	else //Little endian, process each bit separately
	{
		while (!inFile.eof()) {
			//https://stackoverflow.com/questions/19081385/c-read-big-endian-binary-float
			unsigned char temp[sizeof(float)];
			inFile.read(reinterpret_cast<char*>(temp), sizeof(float));
			unsigned char t = temp[0];
			temp[0] = temp[3];
			temp[3] = t;
			t = temp[1];
			temp[1] = temp[2];
			temp[2] = t;
			float r;
			memcpy(&r, temp, sizeof(r));
			//http://dbp-consulting.com/tutorials/StrictAliasing.html

			inFile.read(reinterpret_cast<char*>(temp), sizeof(float));
			t = temp[0];
			temp[0] = temp[3];
			temp[3] = t;
			t = temp[1];
			temp[1] = temp[2];
			temp[2] = t;
			float g;
			memcpy(&g, temp, sizeof(g));

			inFile.read(reinterpret_cast<char*>(temp), sizeof(float));
			t = temp[0];
			temp[0] = temp[3];
			temp[3] = t;
			t = temp[1];
			temp[1] = temp[2];
			temp[2] = t;
			float b;
			memcpy(&b, temp, sizeof(b));

			returnPFM.image.emplace_back(glm::vec3(r, g, b));
			inFile.peek(); //To set EOF if we have reached EOF, to prevent an extra dummy iteration of this loop

		}
		
	}
	inFile.close();
	return returnPFM;
}

void writePFM(std::string file, std::vector<Spectrum> image, int w, int h) {
	//Write Big-endian colorful image
	std::ofstream outFile(file, std::ios::out | std::ios::binary);
	float sfEndianness = -1.0f;
	outFile << "PF" << std::endl << w << " " << h << std::endl << sfEndianness << std::endl;
	for (int i = w - 1; i >= 0 ; i--) {
		for(int j = 0; j < h; j++) {
			Spectrum v = image.at(j * (w - i) + i);
			float x = static_cast<float>(v.x);
			float y = static_cast<float>(v.y);
			float z = static_cast<float>(v.z);

			outFile.write(reinterpret_cast<char*>(&x), sizeof(float));
			outFile.write(reinterpret_cast<char*>(&y), sizeof(float));
			outFile.write(reinterpret_cast<char*>(&z), sizeof(float));
		}
	}
	outFile.close();
}