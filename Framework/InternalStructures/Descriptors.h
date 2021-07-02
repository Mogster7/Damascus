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


class DescriptorInfo : public vk::WriteDescriptorSet
{
public:

	static DescriptorInfo Create(
		uint32_t binding,
		vk::ShaderStageFlags shaderStage,
		vk::DescriptorType type,
		vk::DescriptorBufferInfo& bufferInfo,
		uint32_t descriptorCount = 1)
	{
		DescriptorInfo descInfo = {};
		descInfo.descriptorType = type;
		descInfo.descriptorCount = descriptorCount;
		descInfo.dstBinding = binding;
		descInfo.pBufferInfo = &bufferInfo;
		descInfo.shaderStage = shaderStage;
		return descInfo;
	}

	static DescriptorInfo Create(
		uint32_t binding,
		vk::ShaderStageFlags shaderStage,
		vk::DescriptorType type,
		vk::DescriptorImageInfo& imageInfo,
		uint32_t descriptorCount = 1)
	{
		DescriptorInfo descInfo = {};
		descInfo.descriptorType = type;
		descInfo.descriptorCount = descriptorCount;
		descInfo.dstBinding = binding;
		descInfo.pImageInfo = &imageInfo;
		descInfo.shaderStage = shaderStage;
		return descInfo;
	}

	static DescriptorInfo CreateAsync(
		uint32_t binding,
		vk::ShaderStageFlags shaderStage,
		vk::DescriptorType type,
		std::vector<vk::DescriptorImageInfo*>& imageInfosPerSet,
		uint32_t descriptorCount = 1)
	{
		DescriptorInfo descInfo = Create(binding, shaderStage,
										 type, *imageInfosPerSet[0], 
										 descriptorCount);
		descInfo.imageInfosPerSet = &imageInfosPerSet[0];
		descInfo.infosCount = imageInfosPerSet.size();
		return descInfo;
	}

	static DescriptorInfo CreateAsync(
		uint32_t binding,
		vk::ShaderStageFlags shaderStage,
		vk::DescriptorType type,
		std::vector<vk::DescriptorBufferInfo*>& bufferInfosPerSet,
		uint32_t descriptorCount = 1)
	{
		DescriptorInfo descInfo = Create(binding, shaderStage,
										 type, *bufferInfosPerSet[0], 
										 descriptorCount);
		descInfo.bufferInfosPerSet = &bufferInfosPerSet[0];
		descInfo.infosCount = bufferInfosPerSet.size();
		return descInfo;
	}

	vk::WriteDescriptorSet& Get()
	{
		return *this;
	}

	vk::ShaderStageFlags shaderStage;
	vk::DescriptorImageInfo * * imageInfosPerSet = nullptr;
	vk::DescriptorBufferInfo * * bufferInfosPerSet = nullptr;
	uint32_t infosCount = 0;
};

class Descriptors
{
public:
	template <uint32_t SetCount>
	void Create(
		std::array<DescriptorInfo, SetCount> infos,
		uint32_t imageCount,
		Device& owner
	)
	{
		m_owner = &owner;
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
		uint32_t i = 0;
		for (const auto& kv : typeCounts)
		{
			poolSizes[i].type = kv.first;
			poolSizes[i].descriptorCount = kv.second;
			++i;
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
		std::vector<vk::DescriptorSetLayout> setLayouts(imageCount, layout.Get());

		vk::DescriptorSetAllocateInfo allocInfo = {};
		allocInfo.descriptorPool = pool.Get();
		allocInfo.descriptorSetCount = imageCount;
		allocInfo.pSetLayouts = setLayouts.data();

		sets.resize(imageCount);
		utils::CheckVkResult(
			owner.allocateDescriptorSets(
				&allocInfo,
				sets.data()),
			"Failed to allocate descriptor sets");

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

			owner.updateDescriptorSets(
				static_cast<uint32_t>(writeSets.size()),
				writeSets.data(),
				0, nullptr
			);
		}

	}

	void Destroy()
	{
		pool.Destroy();
		layout.Destroy();
	}

	DescriptorPool pool;
	DescriptorSetLayout layout = {};
	std::vector<vk::DescriptorSet> sets = {};
private:
	Device* m_owner;
};
