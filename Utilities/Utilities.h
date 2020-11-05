//------------------------------------------------------------------------------
//
// File Name:	Utilities.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/6/2020 
//
//------------------------------------------------------------------------------
#pragma once

constexpr unsigned MAX_OBJECTS = 2;

namespace utils
{
    std::vector<char> ReadFile(const eastl::string& filename);
    void CheckVkResult(vk::Result result, const eastl::string& error);
}






