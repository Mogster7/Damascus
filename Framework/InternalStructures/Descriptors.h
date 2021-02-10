//------------------------------------------------------------------------------
//
// File Name:	DescriptorSetLayout.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/29/2020 
//
//------------------------------------------------------------------------------
#pragma once

CUSTOM_VK_DECLARE_DERIVE(DescriptorPool, DescriptorPool, Device)

};

CUSTOM_VK_DECLARE_DERIVE(DescriptorSetLayout, DescriptorSetLayout, Device)

};

class DescriptorSetLayoutBinding
{
public:
	static vk::DescriptorSetLayoutBinding Create(
		vk::DescriptorType type,
		vk::ShaderStageFlags flags,
		uint32_t binding,
		uint32_t descriptorCount = 1);
};

class WriteDescriptorSet
{
public:

	static vk::WriteDescriptorSet Create(
		vk::DescriptorSet dstSet,
		vk::DescriptorType type,
		uint32_t binding,
		vk::DescriptorBufferInfo& bufferInfo,
		uint32_t descriptorCount = 1);

	static vk::WriteDescriptorSet Create(
		vk::DescriptorSet dstSet,
		vk::DescriptorType type,
		uint32_t binding,
		vk::DescriptorImageInfo& imageInfo,
		uint32_t descriptorCount = 1);
};


