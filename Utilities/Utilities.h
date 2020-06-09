//------------------------------------------------------------------------------
//
// File Name:	Utilities.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/6/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace utils
{
    std::vector<char> ReadFile(const std::string& filename);
    void CheckVkResult(vk::Result result, const std::string& error);
}






