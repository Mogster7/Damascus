#include <utility>

//------------------------------------------------------------------------------
//
// File Name:	RenderingDefines.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/24/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace dm {

constexpr size_t MAX_FRAME_DRAWS = 2;

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


template<class Type>
struct IOwned
{
public:
	using OwnerType = Type;
	using OwnerInterface = IOwned<OwnerType>;

	IOwned() = default;

	IOwned(const IOwned& other) = delete;

	virtual IOwned& operator=(const IOwned& other) = delete;

	virtual IOwned& operator=(IOwned&& other) noexcept
	{
		assert(this != &other);

		created = other.created;
		other.created = false;

		owner = other.owner;
		other.owner = nullptr;

		return *this;
	}

	IOwned(IOwned&& other) noexcept
	{
		*this = std::move(other);
	}

	explicit operator bool()
	{
		return created;
	}

	explicit IOwned(
		OwnerType* inOwner
	)
		: owner(inOwner)
		, created(true)
	{
	}

	void Create(
		OwnerType* inOwner
	)
	{
		owner = inOwner;
		created = true;
	}

	virtual ~IOwned() noexcept
	{
		owner = nullptr;
		created = false;
	};

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
};


#define DM_ASSERT_VK(VkResult) dm::utils::CheckVkResult(VkResult)

template<class Type, class CType = typename Type::CType>
struct IVulkanType : public Type
{
	using VulkanType = Type;
	using VulkanCType = CType;
	using VulkanInterface = IVulkanType<Type, CType>;


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

	explicit operator bool()
	{
		return VkCTypePtr() != nullptr;
	}
};

#define DM_TYPE_VULKAN_OWNED_GENERIC_FULL(Type, VulkanName, EXT, CreateName, ConstructName)    \
    public:                                                                                    \
        Type(Type&& other) noexcept = default;                                                          \
        Type& operator=(Type&& other) noexcept = default;\
        template <class ...Args>\
        void Create(const vk::CreateName##CreateInfo##EXT & createInfo,                \
            OwnerType* inOwner, Args&&... args)                                               \
        {                                                                                     \
            OwnerInterface::Create(inOwner);    \
            utils::CheckVkResult(owner->create##ConstructName##EXT(args... , &createInfo, nullptr, &VkType()), \
                std::string("Failed to construct ") + #VulkanName );              \
        }\
		~Type() noexcept\
		{                                                                                            \
			if (created) owner->destroy##VulkanName##EXT(VkType());\
		}
#define DM_TYPE_VULKAN_OWNED_GENERIC(Type, VulkanName) \
    DM_TYPE_VULKAN_OWNED_GENERIC_FULL(Type, VulkanName, ,VulkanName, VulkanName)         \

/**
 * Defines things common to each owned vulkan type
 */
#define DM_TYPE_BODY(Type) \
public:                    \

#define DM_TYPE_OWNED_BODY(Type, DerivedOwner) \
DM_TYPE_BODY(Type)                             \
	Type() = default;                                                                          \
	Type(const Type& other) = delete;\
	virtual Type& operator=(const Type& other) = delete;                                       \


#define DM_TYPE_VULKAN_OWNED_BODY(Type, DerivedOwner)\
DM_TYPE_OWNED_BODY(Type, DerivedOwner)               \
    explicit operator bool()                         \
    { return VulkanInterface::operator bool() && OwnerInterface::operator bool(); }

}