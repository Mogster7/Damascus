//------------------------------------------------------------------------------
//
// File Name:	DescriptorSetLayout.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/29/2020
//
//------------------------------------------------------------------------------
#include "Descriptors.h"

namespace dm
{

void Descriptors::RecreateGlobalSet()
{
    if (globalSet == nullptr)
    {
        globalSet = new GlobalUniforms();
    }

    if (!globalSetDirty)
    {
        return;
    }

    // Perform merge of global sets
    vk::DescriptorSetLayoutCreateInfo createInfo = {};
    std::vector<vk::DescriptorSetLayoutBinding> mergedBindings;
    std::set<uint32_t> bindingIndices;
    uint32_t globalLayoutDataCount = globalLayoutData.size();
    for (int i = 0; i < globalLayoutDataCount; ++i)
    {
        DescriptorSetLayoutData& layoutData = globalLayoutData[i];
        for (auto& binding : layoutData.bindings)
        {
            if (bindingIndices.find(binding.binding) == bindingIndices.end())
            {
                mergedBindings.push_back(binding);
                bindingIndices.emplace(binding.binding);
            }
        }
    }
    createInfo.pBindings = mergedBindings.data();
    createInfo.bindingCount = mergedBindings.size();
    int imageCount = owner->ImageCount();

    DescriptorSetLayout* globalLayout = &globalSetData.layout;
    bool previouslyCreated = globalLayout->created;

    if (previouslyCreated)
    {
        globalLayout->Destroy();
        globalSet->Destroy();
    }

    globalLayout->Create(createInfo, owner);

    if (!previouslyCreated)
        globalSetData.CreateSets(owner, PerDraw);

    globalSet->Create<0, 1, 2>(owner);
    globalSet->SetDirtyBindings(true);
    owner->waitIdle();
    globalSetDirty = false;
}

void Descriptors::BindGlobalSet(int imageIndex,
                                vk::CommandBuffer commandBuffer,
                                vk::PipelineLayout pipelineLayout)
{
    globalSet->Bind(imageIndex, commandBuffer, pipelineLayout);
}

void Descriptors::Destroy()
{
    delete globalSet;
}

Descriptors::~Descriptors()
{
    if (created)
    {
        Destroy();
    }
}
void Descriptors::WriteUniforms()
{
    RecreateGlobalSet();
    globalSetData.WriteSets(owner);
    for(auto& [ti, pipeline] : pipelineDescriptors)
    {
        auto& setData = pipeline.setData;
        for(auto& data : setData)
        {
            data.WriteSets(owner);
        }
    }
}

void Descriptors::PipelineDescriptors::SetData::WriteSets(Device* device)
{
    int dirtyCount = GetDirtyCount();
    if (!dirtyCount) return;

    int imageCount = device->ImageCount();
    std::vector<vk::WriteDescriptorSet> writeSets;
    writeSets.reserve(dirtyCount);

    // Index into descriptor set vector
    // Write the descriptor set at each image / index (establish memory mapping)
    for (int img = 0; img < imageCount; ++img)
    {
        WriteBindingsToSet(writeSets, img);
    }

    // Set all bindings to not dirty, as they've had their memory mappings updated
    for (int img = 0; img < imageCount; ++img)
    {
        SetBindingsDirty(false, img);
    }

    // Update the memory on the GPU
    device->updateDescriptorSets(
        writeSets.size(), writeSets.data(),
        0, nullptr
    );
}
void Descriptors::PipelineDescriptors::SetData::WriteBindingsToSet(std::vector<vk::WriteDescriptorSet>& writeSets, int imageIndex)
{
    int setCount = (int)sets[0].size();
    for (auto& [bindingIndex, binding] : bindings)
    {
        for (int ID = 0; ID < setCount; ++ID)
        {
            if (!binding.IsDirty(imageIndex, ID)) continue;

            auto& writeSet = writeSets.emplace_back();
            binding.WriteToSet(writeSet, imageIndex, ID);
            writeSet.dstSet = sets[imageIndex][ID];
        }
    }
}

void Descriptors::PipelineDescriptors::SetData::SetBindingsDirty(bool dirty, int imageIndex)
{
    int setCount = (int)sets[0].size();
    for (auto& [bindingIndex, binding] : bindings)
    {
        for (int ID = 0; ID < setCount; ++ID)
        {
            binding.SetDirty(dirty, imageIndex, ID);
        }
    }
}


}
