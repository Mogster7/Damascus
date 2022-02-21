#include <utility>

//------------------------------------------------------------------------------
//
// File Name:	RenderingDefines.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/24/2020 
//
//------------------------------------------------------------------------------
#pragma once
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <set>
#include <cstdlib>
#include <thread>
#include <optional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <array>
#include <queue>
#include <algorithm>
#include <stack>
#include <tuple>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <typeindex>

namespace dm
{

struct UBOModel
{
    glm::mat4 model;
};

struct UBOColor
{
    glm::vec4 color;
};

constexpr static glm::mat4 identityMatrix{};

inline void PushIdentityModel(
    vk::CommandBuffer commandBuffer,
    vk::PipelineLayout pipelineLayout
)
{
    commandBuffer.pushConstants(
        pipelineLayout,
        vk::ShaderStageFlagBits::eVertex,
        0, sizeof(glm::mat4), &identityMatrix
    );
}

constexpr int MAX_FRAME_DRAWS = 2;

#ifdef USE_MSAA
constexpr vk::SampleCountFlagBits MSAA_SAMPLES = vk::SampleCountFlagBits::e4;
#else
constexpr vk::SampleCountFlagBits MSAA_SAMPLES = vk::SampleCountFlagBits::e1;
#endif

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef OS_Mac
    "VK_KHR_portability_subset"
#endif
};

#ifdef __aarch64__
#define DM_BREAKPOINT() asm("brk #0")
#else
#define DM_BREAKPOINT()
#endif

#ifndef NDEBUG
#   define DM_ASSERT_MSG(condition, message) \
    do { \
        if (! (condition)) {                 \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
                DM_BREAKPOINT();                                 \
            std::terminate(); \
        } \
    } while (false)

    #   define DM_ASSERT(condition) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << std::endl; \
                DM_BREAKPOINT();                                 \
            std::terminate(); \
        } \
    } while (false)

#else
#   define DM_ASSERT(condition) do { (void)(condition); } while (false)
#   define DM_ASSERT_MSG(condition, message) do { (void)(condition); } while(false)
#endif

inline void CheckVkResult(vk::Result result)
{
    DM_ASSERT_MSG(result == vk::Result::eSuccess, "Assertion failed when testing VkResult!");
}

inline void CheckVkResult(VkResult result)
{
    DM_ASSERT_MSG(result == VK_SUCCESS, "Assertion failed when testing VkResult!");
}

inline void CheckVkResult(vk::Result result, const std::string& error)
{
    DM_ASSERT_MSG(result == vk::Result::eSuccess, error.c_str());
}

inline void AssertVkBase(VkResult result)
{
    if (!(result == VK_SUCCESS)) DM_BREAKPOINT();
    assert(result == VK_SUCCESS);
}

#define DM_ASSERT_VK(VkResult) DM_ASSERT_MSG((VkResult) == vk::Result::eSuccess, "Assertion failed testing VkResult")




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

	void CreateOwned(
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
		return static_cast<VulkanType*>(this);
	}

	VulkanType* VkTypePtr()
	{
		return static_cast<VulkanType*>(this);
	}

	const VulkanCType* VkCTypePtr() const
	{
		return reinterpret_cast<VulkanCType*>(VkTypePtr());
	}

	VulkanCType* VkCTypePtr()
	{
		return reinterpret_cast<VulkanCType*>(VkTypePtr());
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
            OwnerInterface::CreateOwned(inOwner);    \
            DM_ASSERT_MSG(owner->create##ConstructName##EXT(args... , &createInfo, nullptr, &VkType()) == vk::Result::eSuccess, \
                std::string("Failed to construct ") + #VulkanName );              \
        }                                                                                      \
        void Destroy() { if (created) { owner->destroy##VulkanName##EXT(VkType()); created = false; } }\
		~Type() noexcept\
		{                                                                                            \
			Destroy();\
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
