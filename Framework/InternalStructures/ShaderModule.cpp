//------------------------------------------------------------------------------
//
// File Name:	ShaderModule.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/23/2020
//
//------------------------------------------------------------------------------

CUSTOM_VK_DEFINE(ShaderModule, ShaderModule, Device)

vk::PipelineShaderStageCreateInfo ShaderModule::Load(
	ShaderModule& module,
	const std::string& name,
	vk::ShaderStageFlagBits stageFlags,
	Device& owner)
{
	auto src = utils::ReadFile(std::string(ASSET_DIR) + "Shaders/" + name);

	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = src.size();
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(src.data());
	ShaderModule::Create(module, shaderInfo, owner);
	module.stage = stageFlags;

	return vk::PipelineShaderStageCreateInfo(
			{},
			stageFlags,
			module.Get(),
			"main"
		);
}



