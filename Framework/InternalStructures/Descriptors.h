//------------------------------------------------------------------------------
//
// File Name:	DescriptorSetLayout.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/29/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace dm {

//BK_TYPE(DescriptorPool)
class DescriptorPool : public IVulkanType<vk::DescriptorPool>, public IOwned<Device>
{
	BK_TYPE_VULKAN_OWNED_BODY(DescriptorPool, IOwned<Device>)
	BK_TYPE_VULKAN_OWNED_GENERIC(DescriptorPool, DescriptorPool)
};

//BK_TYPE(DescriptorSetLayout)
class DescriptorSetLayout : public IVulkanType<vk::DescriptorSetLayout>, public IOwned<Device>
{
	BK_TYPE_VULKAN_OWNED_BODY(DescriptorSetLayout, IOwned<Device>)
	BK_TYPE_VULKAN_OWNED_GENERIC(DescriptorSetLayout, DescriptorSetLayout)
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

//BK_TYPE(WriteDescriptorSet)
class WriteDescriptorSet : public IVulkanType<vk::WriteDescriptorSet, VkWriteDescriptorSet>
{
public:

	static WriteDescriptorSet Create(
		uint32_t binding,
		vk::ShaderStageFlags shaderStage,
		vk::DescriptorType type,
		vk::DescriptorBufferInfo& bufferInfo,
		uint32_t descriptorCount = 1)
	{
		WriteDescriptorSet descInfo = {};
		descInfo.descriptorType = type;
		descInfo.descriptorCount = descriptorCount;
		descInfo.dstBinding = binding;
		descInfo.pBufferInfo = &bufferInfo;
		descInfo.shaderStage = shaderStage;
		return descInfo;
	}

	static WriteDescriptorSet Create(
		uint32_t binding,
		vk::ShaderStageFlags shaderStage,
		vk::DescriptorType type,
		vk::DescriptorImageInfo& imageInfo,
		uint32_t descriptorCount = 1)
	{
		WriteDescriptorSet descInfo = {};
		descInfo.descriptorType = type;
		descInfo.descriptorCount = descriptorCount;
		descInfo.dstBinding = binding;
		descInfo.pImageInfo = &imageInfo;
		descInfo.shaderStage = shaderStage;
		return descInfo;
	}

	static WriteDescriptorSet CreateAsync(
		uint32_t binding,
		vk::ShaderStageFlags shaderStage,
		vk::DescriptorType type,
		std::vector<vk::DescriptorImageInfo *>& imageInfosPerSet,
		uint32_t descriptorCount = 1)
	{
		WriteDescriptorSet descInfo = Create(binding, shaderStage,
			type, *imageInfosPerSet[0],
			descriptorCount
		);
		descInfo.imageInfosPerSet = &imageInfosPerSet[0];
		descInfo.infosCount = imageInfosPerSet.size();
		return descInfo;
	}

	static WriteDescriptorSet CreateAsync(
		uint32_t binding,
		vk::ShaderStageFlags shaderStage,
		vk::DescriptorType type,
		std::vector<vk::DescriptorBufferInfo *>& bufferInfosPerSet,
		uint32_t descriptorCount = 1)
	{
		WriteDescriptorSet descInfo = Create(binding, shaderStage,
			type, *bufferInfosPerSet[0],
			descriptorCount
		);
		descInfo.bufferInfosPerSet = &bufferInfosPerSet[0];
		descInfo.infosCount = bufferInfosPerSet.size();
		return descInfo;
	}

	vk::WriteDescriptorSet& Get()
	{
		return *this;
	}

	vk::ShaderStageFlags shaderStage;
	vk::DescriptorImageInfo **imageInfosPerSet = nullptr;
	vk::DescriptorBufferInfo **bufferInfosPerSet = nullptr;
	uint32_t infosCount = 0;
};



class Descriptors : public IOwned<Device>
{
public:
	BK_TYPE_OWNED_BODY(Descriptors, IOwned<Device>)

	template<uint32_t SetCount>
	void Create(
		std::array<WriteDescriptorSet, SetCount> infos,
		uint32_t imageCount,
		Device* inOwner
	)
	{
		IOwned::Create(inOwner);

		// Aggregate type counts for pool
		std::unordered_map<vk::DescriptorType, uint32_t> typeCounts = {};
		// Create bindings from write descriptions and aggregate type counts
		std::array<vk::DescriptorSetLayoutBinding, SetCount> bindings = {};
		for (uint32_t i = 0; i < SetCount; ++i)
		{
			bindings[i] =
				DescriptorSetLayoutBinding::Create(
					infos[i].descriptorType,
					infos[i].shaderStage,
					infos[i].dstBinding,
					infos[i].descriptorCount
				);

			typeCounts[infos[i].descriptorType] += infos[i].descriptorCount * imageCount;
		}

		// Allocate descriptor pool for our sets
		std::vector<vk::DescriptorPoolSize> poolSizes(typeCounts.size());
		uint32_t kvIndex = 0;
		for (const auto& kv : typeCounts)
		{
			poolSizes[kvIndex].type = kv.first;
			poolSizes[kvIndex].descriptorCount = kv.second;
			++kvIndex;
		}

		vk::DescriptorPoolCreateInfo poolInfo = {};
		poolInfo.maxSets = imageCount;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		pool.Create(poolInfo, owner);

		// Create descriptor set layouts
		vk::DescriptorSetLayoutCreateInfo dslInfo = {};
		dslInfo.bindingCount = SetCount;
		dslInfo.pBindings = bindings.data();
		layout.Create(dslInfo, owner);

		// Copy set layout for each image 
		std::vector<vk::DescriptorSetLayout> setLayouts(imageCount, layout.VkType());

		vk::DescriptorSetAllocateInfo allocInfo = {};
		allocInfo.descriptorPool = pool.VkType();
		allocInfo.descriptorSetCount = imageCount;
		allocInfo.pSetLayouts = setLayouts.data();

		sets.resize(imageCount);
		utils::CheckVkResult(
			owner->allocateDescriptorSets(
				&allocInfo,
				sets.data()),
			"Failed to allocate descriptor sets"
		);

		std::array<vk::WriteDescriptorSet, SetCount> writeSets = {};
		for (int i = 0; i < imageCount; ++i)
		{
			for (int j = 0; j < SetCount; ++j)
			{
				auto& info = infos[j];
				auto& writeSet = writeSets[j];
				writeSet = info.Get();
				writeSet.dstSet = sets[i];

				if (info.infosCount != 0)
				{
					if (info.imageInfosPerSet != nullptr)
						writeSet.pImageInfo = info.imageInfosPerSet[i];
					else
						writeSet.pBufferInfo = info.bufferInfosPerSet[i];
				}
			}

			owner->updateDescriptorSets(
				static_cast<uint32_t>(writeSets.size()),
				writeSets.data(),
				0, nullptr
			);
		}

	}

	~Descriptors() noexcept = default;

	DescriptorPool pool;
	DescriptorSetLayout layout ;
	std::vector<vk::DescriptorSet> sets = {};
};

}