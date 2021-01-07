//------------------------------------------------------------------------------
//
// File Name:	Utilities.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/6/2020 
//
//------------------------------------------------------------------------------
#include <string>
#include <fstream>


namespace utils
{
    std::vector<char> ReadFile(const std::string& filename)
    {
        std::string convertedName = filename.c_str();
        std::ifstream file(convertedName, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    void CheckVkResult(vk::Result result, const std::string& error)
    {
        ASSERT(result == vk::Result::eSuccess, error.c_str());
    }

}



