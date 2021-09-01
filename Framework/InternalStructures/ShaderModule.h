//------------------------------------------------------------------------------
//
// File Name:	ShaderModule.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm {

//DM_TYPE(ShaderModule)
class ShaderModule : public IVulkanType<vk::ShaderModule>, public IOwned<Device>
{
public:
DM_TYPE_VULKAN_OWNED_BODY(ShaderModule, IOwned<Device>)

DM_TYPE_VULKAN_OWNED_GENERIC(ShaderModule, ShaderModule)


	vk::PipelineShaderStageCreateInfo Load(
		const std::string& path,
		vk::ShaderStageFlagBits stageFlags,
		Device* inOwner
	);

	vk::ShaderStageFlagBits stage = {};
};


}