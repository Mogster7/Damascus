//------------------------------------------------------------------------------
//
// File Name:	Utilities.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/6/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace dm::utils {

float Random(float min = 0.0f, float max = 1.0f, float offset = 0.0f);

int RandomInt(int min = 0, int max = 1, int offset = 0);

std::vector<char> ReadFile(const std::string& filename);


}