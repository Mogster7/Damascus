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
	inline static glm::mat4 identity = glm::mat4(1.0f);
    float Random(float min = 0.0f, float max = 1.0f);
    std::vector<char> ReadFile(const std::string& filename);
    inline void CheckVkResult(vk::Result result, const std::string& error)
    {
        ASSERT(result == vk::Result::eSuccess, error.c_str());
    }
    void AssertVkBase(VkResult result);

	inline void PushIdentityModel(vk::CommandBuffer commandBuffer,
								  vk::PipelineLayout pipelineLayout)
	{
		commandBuffer.pushConstants(
			pipelineLayout,
			vk::ShaderStageFlagBits::eVertex,
			0, sizeof(glm::mat4), &identity
		);
	}

    template <class T>
    void VectorDestroyer(std::vector<T>& vec)
    {
        for (auto& item : vec) item.Destroy();
    }
}






