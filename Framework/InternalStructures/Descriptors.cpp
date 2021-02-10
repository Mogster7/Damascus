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

vk::WriteDescriptorSet WriteDescriptorSet::Create(
	vk::DescriptorSet dstSet, 
	vk::DescriptorType type, 
	uint32_t binding,
	vk::DescriptorBufferInfo& bufferInfo, 
	uint32_t descriptorCount)
{
	vk::WriteDescriptorSet writeSet = {};
	writeSet.dstSet = dstSet;
	writeSet.descriptorType = type;
	writeSet.descriptorCount = descriptorCount;
	writeSet.dstBinding = binding;
	writeSet.pBufferInfo = &bufferInfo;
	return writeSet;
}

vk::WriteDescriptorSet WriteDescriptorSet::Create(
	vk::DescriptorSet dstSet, 
	vk::DescriptorType type, 
	uint32_t binding,
	vk::DescriptorImageInfo& imageInfo, 
	uint32_t descriptorCount)
{
	vk::WriteDescriptorSet writeSet = {};
	writeSet.dstSet = dstSet;
	writeSet.descriptorType = type;
	writeSet.descriptorCount = descriptorCount;
	writeSet.dstBinding = binding;
	writeSet.pImageInfo = &imageInfo;
	return writeSet;
}
