//------------------------------------------------------------------------------
//
// File Name:	DescriptorSetLayout.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/29/2020 
//
//------------------------------------------------------------------------------
#pragma once
#include "spirv_reflect/spirv_reflect.h"

namespace dm
{

class DescriptorPool : public IVulkanType<vk::DescriptorPool>, public IOwned<Device>
{
	DM_TYPE_VULKAN_OWNED_BODY(DescriptorPool, IOwned<Device>)
	DM_TYPE_VULKAN_OWNED_GENERIC(DescriptorPool, DescriptorPool)
};

class DescriptorSetLayout : public IVulkanType<vk::DescriptorSetLayout>, public IOwned<Device>
{
	DM_TYPE_VULKAN_OWNED_BODY(DescriptorSetLayout, IOwned<Device>)
	DM_TYPE_VULKAN_OWNED_GENERIC(DescriptorSetLayout, DescriptorSetLayout)
};

enum DescriptorSetIndex : int
{
    Invalid     = -1,

    PerDraw     = 0,
    PerPass     = 1,
    PerMaterial = 2,
    PerObject   = 3,

    Count
};

enum GlobalDescriptors : int
{
    ViewProjection = 0,
    GlobalSampler = 1,
    GlobalTextures = 2
};

struct DescriptorSetLayoutData
{
    uint32_t setNumber = -1;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    std::vector<SpvReflectDescriptorBinding> reflections;
    std::vector<std::string> names;
    std::vector<uint32_t> sizes;
};

struct IUniformStructure
{
    virtual void Update(vk::CommandBuffer commandBuffer, int imageIndex) {}
};

struct IGlobalUniformStructure : public IUniformStructure, public IOwned<Device>
{
    using UniformBase = IGlobalUniformStructure;

    void Create(Device* inOwner)
    {
        IOwned::CreateOwned(inOwner);
        dirty.resize(owner->ImageCount());
    }

    virtual void WriteToSet(vk::WriteDescriptorSet& writeSet) = 0;
    void SetDirty(bool inDirty, int imageIndex) { dirty[imageIndex] = inDirty; }
    [[nodiscard]] bool IsDirty(int imageIndex) const { return dirty[imageIndex]; }

    // Used to consolidate function calls for std::variant (perf isn't an issue, as global sets are very limited)
    void SetDirty(bool inDirty, int imageIndex, int ID) { SetDirty(inDirty, imageIndex); }
    [[nodiscard]] bool IsDirty(int imageIndex, int ID) const { return IsDirty(imageIndex); }

    ImageAsync<bool> dirty;
};

struct IIndexedUniformStructure : public IUniformStructure, public IOwned<Device>
{
    using UniformBase = IIndexedUniformStructure;

    void Create(Device* inOwner)
    {
        IOwned::CreateOwned(inOwner);
        dirty.resize(inOwner->ImageCount());
    }

    virtual void WriteToSet(vk::WriteDescriptorSet& writeSet, int imageIndex, int ID ) = 0;

    void SetDirty(bool inDirty, int imageIndex, int ID)
    {
        dirty[imageIndex][ID] = inDirty;
    }

    [[nodiscard]] bool IsDirty(int imageIndex, int ID) const
    {
        return dirty[imageIndex][ID];
    };

    ImageAsync<std::vector<bool>> dirty;
};

struct UniformBuffer : public IIndexedUniformStructure
{
    void Create(uint32_t inSize,
                int objectCount,
                Device* inOwner)
    {
        IIndexedUniformStructure::Create(inOwner);

        // Get buffer size
        elementSize = std::max(static_cast<vk::DeviceSize>(inSize), OwnerGet<PhysicalDevice>().GetMinimumUniformBufferOffset());
        vk::DeviceSize bufferSize = elementSize * objectCount;

        // Allocate buffer for each image
        buffers.resize(owner->ImageCount());
        instancedBufferInfo.resize(owner->ImageCount());

        // Assign data for each triple-buffered member
        for(auto& dirtyVec : dirty) dirtyVec.resize(objectCount, true);
        for(auto& infoVec : instancedBufferInfo) infoVec.resize(objectCount, {});
        instanced = objectCount > 1;

        // Create triple buffered version of the underlying UBO buffer
        for(auto& buffer : buffers)
        {
            buffer.CreateStaged(
                nullptr,
                bufferSize,
                vk::BufferUsageFlagBits::eUniformBuffer,
                VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY,
                false,
                true,
                inOwner
            );
        }
    }

    void Update(vk::CommandBuffer commandBuffer, int imageIndex) override
    {
        if (buffers[imageIndex].dirty)
        {
            buffers[imageIndex].StageTransferDynamic(commandBuffer);
        }
    }

    void WriteToSet(vk::WriteDescriptorSet& writeSet, int imageIndex, int ID) override
    {
        vk::DescriptorBufferInfo* info = &buffers[imageIndex].descriptorInfo;

        if (instanced)
        {
            auto& bufferInfoVec = instancedBufferInfo[imageIndex];
            auto& bufferInfo = bufferInfoVec[ID];

            bufferInfo = *info;
            bufferInfo.range = elementSize;
            bufferInfo.offset = ID * elementSize;
            info = &bufferInfo;
        }

        writeSet.pBufferInfo = info;
    }

    vk::DeviceSize elementSize = 0;
    ImageAsync<Buffer> buffers;
    ImageAsync<std::vector<vk::DescriptorBufferInfo>> instancedBufferInfo = {};
    bool instanced = false;
};


struct UniformImage final : public IGlobalUniformStructure
{
    void Create(Device* inOwner, uint32_t arraySize)
    {
        IGlobalUniformStructure::Create(inOwner);
        imageInfo.resize(arraySize);
        currentIndex = 0;
    }

    void WriteToSet(vk::WriteDescriptorSet& writeSet) override
    {
        // @Jon TODO: Make it so that it doesn't literally copy everything to avoid null handles of bindless sets
        for (int i = 0; i < imageInfo.size(); ++i)
        {
            if (i >= currentIndex)
            {
                imageInfo[i] = imageInfo[0];
            }
        }

        writeSet.pImageInfo = imageInfo.data();
        writeSet.descriptorCount = imageInfo.size();
    }

    void WriteToSet(vk::WriteDescriptorSet& writeSet, int imageIndex, int ID)
    {
        WriteToSet(writeSet);
    }

    int PushImageInfo(vk::DescriptorImageInfo info)
    {
        imageInfo[currentIndex] = info;
        for(auto&& d : dirty) d = true;
        return currentIndex++;
    }

    void PushImageInfo(vk::DescriptorImageInfo info, int index)
    {
        imageInfo[index] = info;
        for(auto&& d : dirty) d = true;
    }

    int currentIndex = -1;
    std::vector<vk::DescriptorImageInfo> imageInfo;
};

struct UniformSampler final : public IGlobalUniformStructure
{
    // Explicitly created for now
    void Create(Device* inOwner)
    {
        IGlobalUniformStructure::Create(inOwner);

        vk::SamplerCreateInfo samplerInfo = {};
        samplerInfo.magFilter = vk::Filter::eNearest;
        samplerInfo.minFilter = vk::Filter::eNearest;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = false;
        samplerInfo.compareEnable = false;
        samplerInfo.compareOp = vk::CompareOp::eNever;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        sampler.Create(samplerInfo, inOwner);

        imageInfo.sampler = sampler.VkType();
    }

    void WriteToSet(vk::WriteDescriptorSet& writeSet) override
    {
        writeSet.pImageInfo = &imageInfo;
    }
    void WriteToSet(vk::WriteDescriptorSet& writeSet, int imageIndex, int ID) { WriteToSet(writeSet); }

    dm::Sampler sampler = {};
    vk::DescriptorImageInfo imageInfo = {};
};

struct DescriptorBinding
{
    [[nodiscard]] vk::DescriptorType GetType() const
    {
        auto type = static_cast<vk::DescriptorType>(reflection.descriptor_type);
        return type;
    }

    template <class UniformType>
    UniformType& Get()
    {
        return std::get<UniformType>(descriptor);
    }

    template <class UniformType>
    const UniformType& Get() const
    {
        return std::get<UniformType>(descriptor);
    }

    template <class ReturnType = void, class Lambda>
    ReturnType Execute(Lambda&& lambda)
    {
        switch(GetType())
        {
            case vk::DescriptorType::eUniformBuffer:
                return lambda(Get<UniformBuffer>());
                break;
            case vk::DescriptorType::eCombinedImageSampler:
            case vk::DescriptorType::eSampledImage:
                return lambda(Get<UniformImage>());
                break;
            case vk::DescriptorType::eSampler:
                return lambda(Get<UniformSampler>());
                break;
            default:
                DM_ASSERT_MSG(false, "Executing on unsupported uniform type");
                return ReturnType();
        }
    }

    template <class ReturnType = void, class Lambda>
    ReturnType Execute(Lambda&& lambda) const
    {
        switch(GetType())
        {
            case vk::DescriptorType::eUniformBuffer:
                return lambda(Get<UniformBuffer>());
                break;
            case vk::DescriptorType::eCombinedImageSampler:
            case vk::DescriptorType::eSampledImage:
                return lambda(Get<UniformImage>());
                break;
            case vk::DescriptorType::eSampler:
                return lambda(Get<UniformSampler>());
                break;
            default:
                DM_ASSERT_MSG(false, "Executing on unsupported uniform type");
                return ReturnType();
        }
    }

    [[nodiscard]] bool IsDirty(int imageIndex, int ID = 0) const
    {
        return Execute<bool>([=](const auto& desc) { return desc.IsDirty(imageIndex, ID); });
    }

    void SetDirty(bool dirty, int imageIndex = 0, int ID = 0)
    {
        Execute([=](auto& desc) { desc.SetDirty(dirty, imageIndex, ID); });
    }

    void WriteToSet(vk::WriteDescriptorSet& writeSet, int imageIndex, int ID = 0)
    {
        writeSet.descriptorType = GetType();
        writeSet.descriptorCount = descriptorCount;
        writeSet.dstBinding = reflection.binding;
        Execute([&](auto& desc)
                {

                  desc.WriteToSet(writeSet, imageIndex, ID);
                });
    }

    void Update(vk::CommandBuffer commandBuffer, int imageIndex)
    {
        Execute([&](auto& desc){ desc.Update(commandBuffer, imageIndex); });
    }

    SpvReflectDescriptorBinding reflection = {};
    std::variant<UniformBuffer,
                 UniformSampler,
                 UniformImage> descriptor = {};
    std::string name;
    uint32_t descriptorCount = 1;
};

template <class Pipeline, DescriptorSetIndex SetIndex>
class UniformSet;
using GlobalUniforms = UniformSet<Descriptors, PerDraw>;

class Descriptors : public IOwned<Device>
{
public:
    using GlobalPipeline = Descriptors;

    struct PipelineDescriptors : public IOwned<Device>
    {
        inline static constexpr int MAX_MATERIALS = 128;
        inline static constexpr int MAX_OBJECTS = 4096;

        static int GetSetMax(DescriptorSetIndex setIndex)
        {
            switch (setIndex)
            {
                case PerMaterial:
                    return MAX_MATERIALS;
                case PerObject:
                    return MAX_OBJECTS;
                default:
                    return 1;
            }
        }

        void Create(Device* inOwner,
                    std::array<DescriptorSetLayout, DescriptorSetIndex::Count>&& inLayouts,
                    std::array<DescriptorSetLayoutData, DescriptorSetIndex::Count>&& inLayoutData)
        {
            IOwned::CreateOwned(inOwner);

            // Allocate every descriptor set ahead of time and create the memory for it
            for (int i = 0; i < DescriptorSetIndex::Count; ++i)
            {
                auto setIndex = (DescriptorSetIndex) i;
                setData[i].Create(owner,
                                  setIndex,
                                  std::move(inLayouts[i]),
                                  std::move(inLayoutData[i]));
            }
        }

        void UpdateSets(vk::CommandBuffer commandBuffer, int imageIndex)
        {
            for(auto& data : setData)
            {
                data.UpdateBindings(commandBuffer, imageIndex);
            }
        }


        struct SetData
        {
            void Create(Device* device,
                        DescriptorSetIndex setIndex,
                        DescriptorSetLayout&& inLayout,
                        DescriptorSetLayoutData&& inLayoutData)
            {
                layout = std::move(inLayout);
                layoutData = std::move(inLayoutData);

                if (!layoutData.bindings.empty())
                    CreateSets(device, setIndex);
            }

            void CreateSets(Device* device,
                            DescriptorSetIndex setIndex)
            {
                sets.resize(device->ImageCount());

                // Sets equal to max number of the class
                int setCount = GetSetMax(setIndex);
                for (auto& set : sets)
                {
                    set.resize(setCount);
                }

                for (int i = setCount - 1; i >= 0; --i)
                    freeIDs.emplace(i);

                std::vector<vk::DescriptorSetLayout> setLayouts(setCount, layout.VkType());

                // Copy global layout for each image
                vk::DescriptorSetAllocateInfo allocateInfo = {};
                allocateInfo.pSetLayouts = setLayouts.data();
                allocateInfo.descriptorSetCount = setLayouts.size();
                allocateInfo.descriptorPool = device->DescriptorPool().VkType();

                // For each image, allocate a buffer of descriptor sets
                int imageCount = device->ImageCount();
                for (int img = 0; img < imageCount; ++img)
                {
                    DM_ASSERT_VK(device->allocateDescriptorSets(&allocateInfo, sets[img].data()));
                }
            }

            [[nodiscard]] int GetDirtyCount(int imageIndex) const
            {
                int dirtyCount = 0;
                int setCount = (int)sets[0].size();
                for(const auto& [bindingIndex, binding] : bindings)
                {
                    // Size of ImageAsync sets is equal to number of images
                    for (int ID = 0; ID < setCount; ++ID)
                    {
                        if (binding.IsDirty(imageIndex, ID))
                            ++dirtyCount;
                    }
                }
                return dirtyCount;
            }

            [[nodiscard]] int GetDirtyCount() const
            {
                int dirtyCount = 0;
                for(int img = 0; img < sets.size(); ++img)
                {
                    dirtyCount += GetDirtyCount(img);
                }
                return dirtyCount;
            }

            void UpdateBindings(vk::CommandBuffer commandBuffer, int imageIndex)
            {
                for (auto& [bindingIndex, binding] : bindings)
                {
                    binding.Update(commandBuffer, imageIndex);
                }
            }


            // Bindings per set per pipeline
            std::unordered_map<uint32_t, DescriptorBinding> bindings;

            // Set layout data per set per pipeline
            DescriptorSetLayout layout;
            DescriptorSetLayoutData layoutData;

            // Descriptor sets per set per pipeline (memory mappings)
            ImageAsync<std::vector<vk::DescriptorSet>> sets;

            // IDs returned to uniforms requested access to the pre-allocated set buffer above
            std::stack<int> freeIDs;

            int PopID()
            {
                DM_ASSERT_MSG(!freeIDs.empty(), "Too many objects constructed with unique uniform data");

                int ID = freeIDs.top();
                freeIDs.pop();

                return ID;
            }

            void PushID(int ID)
            {
                freeIDs.push(ID);
            }

            void WriteSets(Device* device);

            void WriteBindingsToSet(std::vector<vk::WriteDescriptorSet>& writeSets, int imageIndex);
            void SetBindingsDirty(bool dirty, int imageIndex);

            vk::DescriptorSet* GetSet(int imageIndex, int ID)
            {
                return &sets[imageIndex][ID];
            }
        };

        void WriteAllSets(Device* device)
        {
            for (int i = 0; i < DescriptorSetIndex::Count; ++i)
            {
                setData[i].WriteSets(device);
            }
        }

        std::array<SetData, DescriptorSetIndex::Count> setData;
        std::vector<vk::VertexInputAttributeDescription> vertexDescriptions;
    };

    void BindGlobalSet(int imageIndex, vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout);

    void Create(Device* inOwner)
    {
        IOwned::CreateOwned(inOwner);
    }

    void Destroy();
    ~Descriptors() override;
    void WriteUniforms();

    template <class Pipeline>
    void PushData(
        std::array<DescriptorSetLayout, DescriptorSetIndex::Count>&& inLayouts,
        std::array<DescriptorSetLayoutData, DescriptorSetIndex::Count>&& inLayoutData,
        std::vector<vk::VertexInputAttributeDescription>&& inVertexDescriptions)
    {
        std::type_index ti = typeid(Pipeline);
        auto& pipeline = pipelineDescriptors[ti];

        // Get binding data from read-in shader
        for(auto& data : inLayoutData)
        {
            int bindingCount = (int)data.bindings.size();
            auto setIndex = (DescriptorSetIndex)data.setNumber;

            bool globalData = setIndex == DescriptorSetIndex::PerDraw;
            if (globalData && bindingCount > 0)
            {
                globalLayoutData.emplace_back(data);
                globalSetDirty = true;
            }

            for (int i = 0 ; i < bindingCount; ++i)
            {
                std::string name = data.names[i];
                vk::DescriptorSetLayoutBinding& layoutBinding = data.bindings[i];
                uint32_t bindingIndex = layoutBinding.binding;
                const SpvReflectDescriptorBinding& reflection = data.reflections[i];

                std::unordered_map<uint32_t, DescriptorBinding>* bindings;
                if (globalData)
                    bindings = &GetBindings<GlobalPipeline>(setIndex);
                else
                    bindings = &GetBindings<Pipeline>(setIndex);

                    // Get bindings map for this set
                auto it = bindings->find(bindingIndex);

                // If it exists already, perform some validation and continue
                if (it != bindings->end())
                {
                    // @Jon TODO: Validate names being different across pipelines

                    DM_ASSERT_MSG(it->second.reflection.descriptor_type == reflection.descriptor_type,
                                  "Attempting to use the same binding with different types across pipelines, not allowed");

                    continue;
                }

                DescriptorBinding& binding = bindings->emplace(bindingIndex, DescriptorBinding()).first->second;
                binding.reflection = reflection;
                binding.name = name;
                binding.descriptorCount = layoutBinding.descriptorCount;

                switch (layoutBinding.descriptorType)
                {
                    case vk::DescriptorType::eUniformBuffer:
                        // Get the buffer from the emplaced object
                        std::get<UniformBuffer>(binding.descriptor).Create(
                            reflection.block.size,
                            PipelineDescriptors::GetSetMax(setIndex),
                            owner);
                        break;
                    case vk::DescriptorType::eSampledImage:
                    case vk::DescriptorType::eCombinedImageSampler:
                        binding.descriptor = UniformImage();
                        binding.Get<UniformImage>().Create(owner, binding.descriptorCount);
                        break;
                    case vk::DescriptorType::eSampler:
                        binding.descriptor = UniformSampler();
                        binding.Get<UniformSampler>().Create(owner);
                        break;
                    default:
                        DM_ASSERT_MSG(false, "Attempting to create a non-supported uniform in the pipeline creation phase");
                        break;
                }
            }
        }

        pipeline.Create(owner, std::move(inLayouts), std::move(inLayoutData));
        pipeline.vertexDescriptions = std::move(inVertexDescriptions);
    }

    void RecreateGlobalSet();

    template <class Pipeline>
    void UploadUniforms(vk::CommandBuffer commandBuffer, int imageIndex)
    {
        for (int si = 1; si < DescriptorSetIndex::Count; ++si)
        {
            auto& setData = GetSetData<Pipeline>((DescriptorSetIndex)si);
            setData.UpdateBindings(commandBuffer, imageIndex);
        }
    }

    template <class Pipeline>
    bool PipelineExists()
    {
        if constexpr (std::is_same<Pipeline, GlobalPipeline>::value)
            return true;

        return pipelineDescriptors.find(typeid(Pipeline)) != pipelineDescriptors.end();
    }

    template <class Pipeline, DescriptorSetIndex SetIndex>
    UniformBuffer& GetUniformBuffer(const std::string& name)
    {
        std::type_index ti = typeid(Pipeline);
        DM_ASSERT_MSG(PipelineExists<Pipeline>(),
                      "Attempting to get non-existent pipeline when looking for uniform buffer");

        auto& bindings = GetSetData<Pipeline>(SetIndex).bindings;

        for(auto& kv : bindings)
        {
            if (kv.second.name == name)
            {
                return std::get<UniformBuffer>(kv.second.descriptor);
            }
        }
        DM_ASSERT_MSG(false, "Requesting invalid uniform buffer");
        exit(1);
    }


    template <class Pipeline = GlobalPipeline>
    PipelineDescriptors::SetData& GetSetData(DescriptorSetIndex setIndex)
    {
        if constexpr (std::is_same<Pipeline, GlobalPipeline>::value)
        {
            DM_ASSERT_MSG(setIndex == PerDraw,
                          "Attempting to access global type with set index that isn't Per Draw (not valid)");
            return globalSetData;
        }
        else
        {
            std::type_index ti = typeid(Pipeline);
            DM_ASSERT_MSG(setIndex != PerDraw,
                          "Attempting to get global binding (set index 0) with an actual pipeline type (must be type == Descriptors)");

            DM_ASSERT_MSG(PipelineExists<Pipeline>(),
                          "Attempting to get non-existent pipeline when looking for binding");

            return pipelineDescriptors[ti].setData[setIndex];
        }
    }


    template <class Pipeline = GlobalPipeline>
    std::unordered_map<uint32_t, DescriptorBinding>& GetBindings(DescriptorSetIndex setIndex)
    {
        // Return global set bindings
        if constexpr (std::is_same<Pipeline, GlobalPipeline>::value)
            return globalSetData.bindings;

        return GetSetData<Pipeline>(setIndex).bindings;
    };


    template <class Pipeline = GlobalPipeline>
    DescriptorBinding& GetBinding(DescriptorSetIndex set, uint32_t binding)
    {
        auto& bindings = GetBindings<Pipeline>(set);
        DM_ASSERT_MSG(bindings.find(binding) != bindings.end(),
                      "Attempting to get non-existent binding from pipeline");

        return bindings[binding];
    }

    template <class Pipeline>
    std::vector<vk::VertexInputAttributeDescription>& GetVertexAttributeDescriptions()
    {
        std::type_index ti = typeid(Pipeline);
        DM_ASSERT_MSG( PipelineExists<Pipeline>() &&
                      !pipelineDescriptors[ti].vertexDescriptions.empty(),
                      "Attempting to get non-existent vertex input attribute description");

        return pipelineDescriptors[ti].vertexDescriptions;
    }

    template <class Pipeline>
    DescriptorSetLayout* GetLayout(DescriptorSetIndex set)
    {
        auto& layout = GetSetData<Pipeline>(set).layout;
        if (!layout.created)
            return nullptr;

        return &layout;
    }

    template <class Pipeline>
    DescriptorSetLayoutData* GetLayoutData(DescriptorSetIndex set)
    {
        auto& layoutData = GetSetData<Pipeline>(set).layoutData;
        if (layoutData.bindings.empty())
            return nullptr;

        return &layoutData;
    }

    template <class Pipeline>
    PipelineDescriptors& GetPipelineDescriptors()
    {
        DM_ASSERT(PipelineExists<Pipeline>());
        return pipelineDescriptors[typeid(Pipeline)];
    }

    // @Jon TODO: Make way to free allocated descriptor sets, currently not set to be a "freeing paradigm"
    // so no risk of memory leaks, in case any of you nerds are reading this

    std::unordered_map<std::type_index, PipelineDescriptors> pipelineDescriptors;
    std::vector<DescriptorSetLayoutData> globalLayoutData;

    GlobalUniforms* globalSet;
    PipelineDescriptors::SetData globalSetData;
    bool globalSetDirty = false;
};

template <class Pipeline, DescriptorSetIndex SetIndex>
class UniformSet : public IOwned<Device>
{
public:
    struct BindingReference final
    {
        uint32_t index;
        DescriptorBinding& binding;
    };

    template <uint32_t... bindings>
    void Create(Device* inOwner)
    {
        IOwned::CreateOwned(inOwner);

        setData = &owner->Descriptors().GetSetData<Pipeline>(SetIndex);
        DM_ASSERT_MSG(setData, "Set data for pipeline descriptor set should be valid at this stage");
        (RegisterBinding(bindings), ...);

        Descriptors& descriptors = owner->Descriptors();

        descriptorID = setData->PopID();
    }

    template <uint32_t... bindings>
    void AddBindings()
    {
        (RegisterBinding(bindings), ...);
    }

    void Destroy()
    {
        if (created)
        {
            setData->PushID(descriptorID);
        }
    }

    ~UniformSet() override
    {
       Destroy();
    }

    void Bind(
        int imageIndex,
        vk::CommandBuffer commandBuffer,
        vk::PipelineLayout pipelineLayout)
    {
        DM_ASSERT_MSG(descriptorID != -1, "Uniforms not initialized correctly");

        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            pipelineLayout,
            SetIndex, 1,
            setData->GetSet(imageIndex, descriptorID),
            0, nullptr
        );
    }

    [[nodiscard]] DescriptorBinding& GetBinding(uint32_t index) const
    {
        auto it = std::find_if(bindingReferences.begin(), bindingReferences.end(),
                               [index](const BindingReference& bindingRef)
                               {
                                 return bindingRef.index == index;
                               });

        DM_ASSERT_MSG(it != bindingReferences.end(),
            "Attempting to get non-existent binding");

        return it->binding;
    }

    [[nodiscard]] const BindingReference& GetBindingStorage(uint32_t index) const
    {
        DM_ASSERT_MSG(std::find_if(bindingReferences.begin(), bindingReferences.end(),
                                   [index](const BindingReference& binding)
                                   {
                                     return binding.bindingIndex == index;
                                   }) != bindingReferences.end(),
                      "Attempting to get non-existent binding");

        return bindingReferences[index];
    }

    template <class UboType>
    void SetUniformBufferData(UboType& ubo, uint32_t index)
    {
        int imageIndex = owner->ImageIndex();
        auto& binding = GetBinding(index);

        auto& ub = binding.template Get<UniformBuffer>();
        char* data = static_cast<char*>(ub.buffers[imageIndex].GetMappedData());
        std::memcpy(data + descriptorID * ub.elementSize, (char*)&ubo, sizeof(UboType));
        ub.buffers[imageIndex].dirty = true;
    }

    int GetDirtyCount()
    {
        int dirtyCount = 0;
        int imageIndex = owner->ImageIndex();
        for(const BindingReference& bindingRef : bindingReferences)
        {
            if (bindingRef.binding.IsDirty(imageIndex, descriptorID))
                ++dirtyCount;
        }
        return dirtyCount;
    }

    void WriteSet()
    {
        int dirtyCount = GetDirtyCount();
        if (dirtyCount == 0)
        {
            return;
        }

        Descriptors& descriptors = owner->Descriptors();
        int imageCount = owner->ImageCount();
        int imageIndex = owner->ImageIndex();

        std::vector<vk::WriteDescriptorSet> writeSets;
        writeSets.reserve(imageCount * dirtyCount);

        for (const BindingReference &bindingRef : bindingReferences)
        {
            DescriptorBinding &binding = bindingRef.binding;
            // Not dirty, continue
            if (!binding.IsDirty(imageIndex, descriptorID))
                continue;

            vk::WriteDescriptorSet &writeSet = writeSets.emplace_back();

            // Get buffer at image index's descriptor info
            binding.WriteToSet(writeSet, imageIndex, descriptorID);
            writeSet.dstSet = *setData->GetSet(imageIndex, descriptorID);
        }

        for (const BindingReference &bindingRef : bindingReferences)
        {
            bindingRef.binding.SetDirty(false, imageIndex, descriptorID);
        }

        owner->updateDescriptorSets(
            writeSets.size(), writeSets.data(),
            0, nullptr
        );
    }

    void SetDirtyBindings(bool dirty)
    {
        for (int i = 0; i < owner->ImageCount(); ++i)
        {
            for (const BindingReference& bindingRef : bindingReferences)
            {
                bindingRef.binding.SetDirty(dirty, i, descriptorID);
            }
        }
    }

#ifndef NDEBUG
    template <uint32_t... Bindings>
    void TestBindings()
    {
        DM_ASSERT_MSG(((std::find(bindingReferences.begin(), bindingReferences.end(), Bindings) != bindingReferences.end()) && ...),
            "Providing a different set of bindings after storing an initial set, invalid use");
    }
#endif

    int descriptorID = -1;

private:
    void RegisterBinding(uint32_t bindingIndex)
    {
        DescriptorBinding& binding = owner->Descriptors().GetBinding<Pipeline>(SetIndex, bindingIndex);
        bindingReferences.emplace_back(BindingReference{ bindingIndex, binding });
    }

    std::vector<BindingReference> bindingReferences;
    Descriptors::PipelineDescriptors::SetData* setData = nullptr;
};

}