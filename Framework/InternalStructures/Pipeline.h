//------------------------------------------------------------------------------
//
// File Name:	Pipeline.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace dm
{

class Renderer;

//DM_TYPE(PipelineLayout)
class PipelineLayout : public IVulkanType<vk::PipelineLayout>, public IOwned<Device>
{
DM_TYPE_VULKAN_OWNED_BODY(PipelineLayout, IOwned < Device >)

DM_TYPE_VULKAN_OWNED_GENERIC(PipelineLayout, PipelineLayout)
};

class IGraphicsPipeline : public IVulkanType<vk::Pipeline>, public IOwned<Device>
{
protected:
    Renderer& Renderer();

DM_TYPE_VULKAN_OWNED_BODY(IGraphicsPipeline, IOwned < Device >);

    virtual void Load() = 0;
    virtual void Create(Device* inOwner) = 0;
    virtual void OnRecreateSwapchain() = 0;
    virtual void WriteUniformSets() = 0;
    virtual void Update(float dt) = 0;
    virtual vk::CommandBuffer* Record() = 0;

    vk::CommandBuffer* GetCommandBufferPtr();
    vk::CommandBuffer Begin();
    void End();

    template <size_t AttachmentCount>
    void Create(
        const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo,
        const vk::RenderPassCreateInfo& renderPassCreateInfo,
        const vk::PipelineLayoutCreateInfo& pipelineLayoutCreateInfo,
        vk::GraphicsPipelineCreateInfo& graphicsPipelineCreateInfo,
        const vk::SemaphoreCreateInfo& semaphoreCreateInfo,
        vk::FramebufferCreateInfo& frameBufferCreateInfo,
        const vk::Extent2D& extent,
        const std::vector<vk::ClearValue>& clearValues,
        const ImageAsync<std::array<vk::ImageView, AttachmentCount>>& frameBufferAttachments,
        size_t imageViewCount,
        CommandPool* commandPool,
        Device* inOwner
    )
    {
        IOwned<Device>::CreateOwned(inOwner);
        drawBuffers.Create(commandBufferAllocateInfo, commandPool);
        frameBuffers.resize(owner->ImageCount());

        for(auto& semaphore : semaphores)
            semaphore.Create(semaphoreCreateInfo, inOwner);


        renderPass.Create(renderPassCreateInfo, extent, clearValues, inOwner);
        pipelineLayout.Create(pipelineLayoutCreateInfo, inOwner);
        graphicsPipelineCreateInfo.renderPass = renderPass.VkType();                            // Render pass description the pipeline is compatible with
        graphicsPipelineCreateInfo.layout = pipelineLayout.VkType();

        // Create FrameBuffers
        frameBufferCreateInfo.renderPass = renderPass.VkType();
        for(size_t i = 0; i < imageViewCount; ++i)
        {
            // List of attachments 1 to 1 with render pass
            frameBufferCreateInfo.pAttachments = frameBufferAttachments[i].data();
            frameBuffers[i].Create(frameBufferCreateInfo, inOwner);
        }

        DM_ASSERT_VK(owner->createGraphicsPipelines(vk::PipelineCache(), 1, &graphicsPipelineCreateInfo, nullptr, &VkType()));
    }

    template <class PipelineType>
    void ReadShader(const std::vector<std::string>& modulePaths)
    {
        std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions;
        // Create an array of layout data per set, of which there are 4
        std::array<DescriptorSetLayoutData, DescriptorSetIndex::Count> setLayoutData;
        auto findBinding = [&setLayoutData](uint32_t setNumber, uint32_t bindingIndex) mutable {
            auto& bindings = setLayoutData[setNumber].bindings;
            return std::find_if(bindings.begin(), bindings.end(),
                                [bindingIndex](auto& bindingData) {
                                    return bindingData.binding == bindingIndex;
                                });
        };

        Descriptors& descriptors = owner->Descriptors();

        for (auto& path : modulePaths)
        {
            std::vector<char> source = utils::ReadFile(path);

            // Construct spir-v shader module from the source
            spv_reflect::ShaderModule shaderModule(source.size(), source.data());
            assert(shaderModule.GetResult() == SPV_REFLECT_RESULT_SUCCESS);

            SpvReflectShaderStageFlagBits stageFlags = shaderModule.GetShaderStage();
            uint32_t count = 0;
            auto result = shaderModule.EnumerateDescriptorSets(&count, nullptr);
            assert(result == SPV_REFLECT_RESULT_SUCCESS);

            std::vector<SpvReflectDescriptorSet*> sets(count);
            result = shaderModule.EnumerateDescriptorSets(&count, sets.data());
            assert(result == SPV_REFLECT_RESULT_SUCCESS);

            // If vertex shader, gather vertex pipeline data
            if (stageFlags == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
            {
                // Enumerate vertices
                result = shaderModule.EnumerateInputVariables(&count, nullptr);
                assert(result == SPV_REFLECT_RESULT_SUCCESS);

                std::vector<SpvReflectInterfaceVariable*> inputVariables(count);
                result = shaderModule.EnumerateInputVariables(&count, inputVariables.data());
                assert(result == SPV_REFLECT_RESULT_SUCCESS);

                // Sort input vertices by location
                std::sort(inputVariables.begin(), inputVariables.end(),
                          [](const auto* a, const auto* b) {
                              return a->location < b->location;
                          });

                // @Jon TODO: Support instanced vertex bindings
                // Get vertex descriptions, updating offsets while iterating
                uint32_t offset = 0;
                int instancedBinding = 1;
                for (int i = 0; i < count; ++i)
                {
                    auto* var = inputVariables[i];
                    const char * instancedPrefixLocation = strstr(var->name, "i_");
                    auto& desc = vertexAttributeDescriptions.emplace_back();

                    bool instanced = instancedPrefixLocation != nullptr;

                    // Binding 1+ if instanced, else binding 0
                    desc.binding = instanced ? instancedBinding++ : 0;
                    desc.location = var->location;
                    desc.format = static_cast<vk::Format>(var->format);

                    // Offset is previous vector's size if normal vector
                    // Else 0, as we pack our instanced vertex buffers fully
                    desc.offset = instanced ? 0 : offset;

                    // Get the size of this vector, add it to the existing offset
                    if (!instanced)
                    {
                        uint32_t size = var->numeric.vector.component_count * sizeof(float);
                        DM_ASSERT_MSG(size, "Only vectors of floats are supported as input attributes at this time for non-instanced variables");

                        offset += instanced ? 0 : size;
                    }
                }
            }

            // Magically acquire each descriptor set in the shader source (copied from spir-v docs)
            for (auto& set : sets)
            {
                const SpvReflectDescriptorSet& reflectedSet = *set;
                DescriptorSetLayoutData& layoutData = setLayoutData[reflectedSet.set];
                layoutData.sizes.reserve(reflectedSet.binding_count);
                layoutData.names.reserve(reflectedSet.binding_count);
                layoutData.reflections.reserve(reflectedSet.binding_count);
                for (uint32_t i_binding = 0; i_binding < reflectedSet.binding_count; ++i_binding)
                {
                    const SpvReflectDescriptorBinding& reflectedBinding = *(reflectedSet.bindings[i_binding]);
                    auto it = findBinding(reflectedSet.set, reflectedBinding.binding);

                    // If binding already exists, mask the stage flag as an additional shader stage containing the binding
                    if (it != layoutData.bindings.end())
                    {
                        it->stageFlags |= static_cast<vk::ShaderStageFlagBits>(stageFlags);
                        continue;
                    }

                    // emplace and acquire it to start recording reflected data
                    vk::DescriptorSetLayoutBinding& layoutDataBinding = layoutData.bindings.emplace_back();
                    layoutDataBinding.binding = reflectedBinding.binding;
                    layoutDataBinding.descriptorType = static_cast<vk::DescriptorType>(reflectedBinding.descriptor_type);
                    layoutDataBinding.descriptorCount = 1;
                    for (uint32_t i_dim = 0; i_dim < reflectedBinding.array.dims_count; ++i_dim)
                    {
                        layoutDataBinding.descriptorCount *= reflectedBinding.array.dims[i_dim];
                    }

                    layoutDataBinding.stageFlags = static_cast<vk::ShaderStageFlagBits>(stageFlags);

                    // Get the name of the reflected binding
                    layoutData.names.emplace_back(reflectedBinding.name);
                    layoutData.sizes.emplace_back(
                        (layoutDataBinding.descriptorType == vk::DescriptorType::eUniformBuffer) ? reflectedBinding.block.size : 0);
                    layoutData.reflections.emplace_back(reflectedBinding);
                }
                layoutData.setNumber = reflectedSet.set;
            }
        }

        std::array<DescriptorSetLayout, DescriptorSetIndex::Count> setLayouts;
        for (int i = 0; i < DescriptorSetIndex::Count; ++i)
        {
            auto& layoutData = setLayoutData[i];
            uint32_t size = layoutData.bindings.size();
            vk::DescriptorSetLayoutCreateInfo createInfo = {};
            createInfo.bindingCount = size;
            createInfo.pBindings = layoutData.bindings.data();
            setLayouts[i].Create(createInfo, owner);
        }

        descriptors.PushData<PipelineType>(
            std::move(setLayouts),
            std::move(setLayoutData),
            std::move(vertexAttributeDescriptions));
    }

    void Destroy();
    ~IGraphicsPipeline() override { Destroy(); }

    RenderPass renderPass = {};
    PipelineLayout pipelineLayout = {};

    ImageAsync<FrameBuffer> frameBuffers = {};
    FrameAsync<Semaphore> semaphores = {};
    CommandBufferVector drawBuffers = {};
    vk::PushConstantRange pushConstantRange = {};
    vk::PipelineStageFlags stageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
};

}