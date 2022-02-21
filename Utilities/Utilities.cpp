//------------------------------------------------------------------------------
//
// File Name:	Utilities.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/6/2020 
//
//------------------------------------------------------------------------------
#include <string>
#include <fstream>
#include <cassert>
#include <vector>

namespace dm::utils
{
    float Random(float min /* = 0.0f */, float max /* = 1.0f */, float offset)
    {
        return min + ((float)rand() / ((float)RAND_MAX)) * (max - min) + offset;
    }

	int RandomInt(int min /*= 0*/, int max /*= 1*/, int offset /*= 0*/)
	{
        return (int)(Random(min, max-1, (float)offset));
	}

	std::vector<char> ReadFile(const std::string& filename)
    {
        const std::string& convertedName = filename;
        std::ifstream file(convertedName, std::ios::ate | std::ios::binary);

        assert(file.is_open());

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }


}



