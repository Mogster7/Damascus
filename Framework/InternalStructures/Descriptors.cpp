//------------------------------------------------------------------------------
//
// File Name:	DescriptorSetLayout.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/29/2020
//
//------------------------------------------------------------------------------

CUSTOM_VK_DEFINE(DescriptorPool, DescriptorPool, Device)
CUSTOM_VK_DEFINE(DescriptorSetLayout, DescriptorSetLayout, Device)

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

