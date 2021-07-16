//------------------------------------------------------------------------------
//
// File Name:	Utilities.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/6/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm::utils {
struct UBOModel
{
	glm::mat4 model;
};

struct UBOColor
{
	glm::vec4 color;
};


inline static glm::mat4 identity = glm::mat4(1.0f);

float Random(float min = 0.0f, float max = 1.0f, float offset = 0.0f);

int RandomInt(int min = 0, int max = 1, int offset = 0);

std::vector<char> ReadFile(const std::string& filename);


inline void CheckVkResult(vk::Result result)
{
	ASSERT(result == vk::Result::eSuccess, "Assertion failed when testing VkResult!");
}

inline void CheckVkResult(VkResult result)
{
	ASSERT(result == VK_SUCCESS, "Assertion failed when testing VkResult!");
}

inline void CheckVkResult(vk::Result result, const std::string& error)
{
	ASSERT(result == vk::Result::eSuccess, error.c_str());
}

void AssertVkBase(VkResult result);

inline void PushIdentityModel(
	vk::CommandBuffer commandBuffer,
	vk::PipelineLayout pipelineLayout
)
{
	commandBuffer.pushConstants(
		pipelineLayout,
		vk::ShaderStageFlagBits::eVertex,
		0, sizeof(glm::mat4), &identity
	);
}

}