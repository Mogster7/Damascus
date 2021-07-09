#include <utility>

//------------------------------------------------------------------------------
//
// File Name:	RenderingDefines.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/24/2020 
//
//------------------------------------------------------------------------------
#pragma once

constexpr std::string_view ASSET_DIR = "Assets/";
constexpr size_t MAX_FRAME_DRAWS = 2;


const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


template<class Type>
struct IOwned
{
public:
	using OwnerType = Type;

	void Create(Type* inOwner, std::function<void(void)> inDestroyFunction = [](){})
	{
		owner = inOwner;
		destroyFunction = std::move(inDestroyFunction);
		created = true;
	}

	void DestroyIfCreated()
	{
		if (created)
		{
			destroyFunction();
		}
	}

	virtual ~IOwned()
	{
		DestroyIfCreated();
	}

	template<class QueryType, bool Match = std::is_same<QueryType, OwnerType>::value>
	constexpr QueryType& OwnerGet() const
	{
		if constexpr (Match)
		{
			return *owner;
		}
		else
		{
			return owner->template OwnerGet<QueryType>();
		}
	}

	Type* owner = nullptr;
	bool created = false;
private:
	std::function<void(void)> destroyFunction;
};



#define ASSERT_VK(VkResult) utils::CheckVkResult(VkResult)

template<class Type, class CType = typename Type::CType>
struct IVulkanType : public Type
{
	using VulkanType = Type;
	using VulkanCType = CType;


	const VulkanType& VkType() const
	{
		return static_cast<const VulkanType&>(*this);
	}

	VulkanType& VkType()
	{
		return static_cast<VulkanType&>(*this);
	}

	const VulkanCType& VkCType() const
	{
		return (VulkanCType&) *this;
	}

	VulkanCType& VkCType()
	{
		return (VulkanCType&) *this;
	}

	const VulkanType* VkTypePtr() const
	{
		return reinterpret_cast<VulkanType*>(this);
	}

	VulkanType* VkTypePtr()
	{
		return reinterpret_cast<VulkanType*>(this);
	}

	const VulkanCType* VkCTypePtr() const
	{
		return reinterpret_cast<VulkanCType*>(this);
	}

	VulkanCType* VkCTypePtr()
	{
		return reinterpret_cast<VulkanCType*>(this);
	}

};

#define BK_TYPE_VULKAN_OWNED_CREATE_FULL(Type, VulkanName, EXT, CreateName, ConstructName)    \
    public:                                                                           \
        template <class ...Args>\
        void Create(const vk::CreateName##CreateInfo##EXT & createInfo,                \
            OwnerType* inOwner, Args&&... args)                                                                                \
        {                                                                                     \
            IOwned::Create(inOwner, [this]() { owner->destroy##VulkanName##EXT(VkType()); });\
            utils::CheckVkResult(owner->create##ConstructName##EXT(args... , &createInfo, nullptr, &VkType()), \
                std::string("Failed to construct ") + #VulkanName );              \
        }                                                                         \

#define BK_TYPE_VULKAN_OWNED_CREATE(Type, VulkanName) \
	BK_TYPE_VULKAN_OWNED_CREATE_FULL(Type, VulkanName, ,VulkanName, VulkanName)

#define BK_TYPE_VULKAN_OWNED_DESTROY_EXT(Type, VulkanName, EXT) \
        ~Type() { if (created) owner->destroy##VulkanName##EXT(VkType()); } \

#define BK_TYPE_VULKAN_OWNED_DESTROY(Type, VulkanName) BK_TYPE_VULKAN_OWNED_DESTROY_EXT(Type, VulkanName,)

#define BK_TYPE_VULKAN_OWNED_GENERIC(Type, VulkanName) \
	BK_TYPE_VULKAN_OWNED_CREATE(Type, VulkanName)         \

/**
 * Defines things common to each owned vulkan type
 */
#define BK_TYPE_BODY(Type) \
public:                          \


#define BK_TYPE_OWNED_BODY(Type, DerivedOwner) \
BK_TYPE_BODY(Type)\

#define BK_TYPE_VULKAN_OWNED_BODY(Type, DerivedOwner)\
BK_TYPE_OWNED_BODY(Type, DerivedOwner)\

