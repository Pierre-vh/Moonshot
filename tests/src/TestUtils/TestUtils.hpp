////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : Utils.hpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
// This file defines the Test utilities
////------------------------------------------------------////

#pragma once

#include <string>
#include <vector>

namespace Moonshot::Tests
{
	// Reads a file line by line and puts every line into a vector. 
	// Returns true in case of success, false if the file could not be opened.
	bool readFileToVec(const std::string& filepath, std::vector<std::string>& outvec);
	bool readFileToString(const std::string& filepath, std::string& outstr);
	std::string indent(const unsigned char& size);
}