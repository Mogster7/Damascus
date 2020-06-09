//------------------------------------------------------------------------------
//
// File Name:	Utilities.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/6/2020 
//
//------------------------------------------------------------------------------

namespace utils
{
    std::vector<char> ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

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
        (result == vk::Result::eSuccess) ? NULL : throw std::runtime_error(error.c_str());
    }

}

