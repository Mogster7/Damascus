//------------------------------------------------------------------------------
//
// File Name:	ShaderModule.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/23/2020
//
//------------------------------------------------------------------------------
namespace dm
{

vk::PipelineShaderStageCreateInfo ShaderModule::Load(const std::string& path, vk::ShaderStageFlagBits stageFlags, Device* inOwner)
{
	if (created)
	{
		owner->destroyShaderModule(VkType());
	}

	auto src = utils::ReadFile(path);

	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = src.size();
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(src.data());
	Create(shaderInfo, inOwner);
	stage = stageFlags;

	return vk::PipelineShaderStageCreateInfo(
		{},
		stageFlags,
		VkType(),
		"main"
	);

}

}