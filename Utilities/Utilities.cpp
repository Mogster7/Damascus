//------------------------------------------------------------------------------
//
// File Name:	Utilities.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/6/2020 
//
//------------------------------------------------------------------------------
#include <string>
#include <fstream>
#include <assert.h>


namespace utils
{
    float Random(float min /* = 0.0f */, float max /* = 1.0f */)
    {
        return min + ((float)rand() / (RAND_MAX)) * (max - min);
    }

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

    void AssertVkBase(VkResult result)
    {
        assert(result == VK_SUCCESS);
    }

}



