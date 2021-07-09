//------------------------------------------------------------------------------
//
// File Name:	ShaderModule.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/23/2020
//
//------------------------------------------------------------------------------
namespace bk {


vk::PipelineShaderStageCreateInfo ShaderModule::Load(const std::string& name, vk::ShaderStageFlagBits stageFlags, Device* inOwner)
{
	DestroyIfCreated();

	auto src = utils::ReadFile(std::string(ASSET_DIR) + "Shaders/" + name);

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