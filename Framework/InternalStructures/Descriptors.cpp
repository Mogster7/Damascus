//------------------------------------------------------------------------------
//
// File Name:	DescriptorSetLayout.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/29/2020
//
//------------------------------------------------------------------------------
namespace bk {

vk::DescriptorSetLayoutBinding DescriptorSetLayoutBinding::Create(
	vk::DescriptorType type,
	vk::ShaderStageFlags flags,
	uint32_t binding,
	uint32_t descriptorCount)
{
	vk::DescriptorSetLayoutBinding setBinding = {};
	setBinding.descriptorType = type;
	setBinding.stageFlags = flags;
	setBinding.binding = binding;
	setBinding.descriptorCount = descriptorCount;
	return setBinding;
}

}