//------------------------------------------------------------------------------
//
// File Name:	ShaderModule.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once

CUSTOM_VK_DECLARE_DERIVE(ShaderModule, ShaderModule, Device)

public:
	static vk::PipelineShaderStageCreateInfo Load(
		ShaderModule& module,
		const std::string& name,
		vk::ShaderStageFlagBits stageFlags,
		Device& owner);

	vk::ShaderStageFlagBits stage = {};
};




