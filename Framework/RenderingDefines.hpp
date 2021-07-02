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


const std::vector<const char *> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


// Tests if the class has a method by the name of Initialization
// If it does, it will execute it immediately following the constructor of our underlying Vulkan structure
template<typename T>
class HasInitialization
{
	typedef char one;
	struct two
	{
		char x[2];
	};

	template<typename C>
	constexpr static one test(decltype(&C::Initialization));

	template<typename C>
	constexpr static two test(...);

public:
	enum
	{
		value = sizeof(test<T>(0)) == sizeof(char)
	};
};

template<typename T>
constexpr void CheckInitialization(T& obj)
{
	if constexpr (HasInitialization<T>::value)
	{
		obj.Initialization();
	}
}

template<class OwnerType>
class IOwned
{
public:
	explicit IOwned(const OwnerType& owner)
			: owner(owner)
	{
	}

	void OwnerCreate(std::function<vk::Result(const OwnerType&)> CreateFunction)
	{
		utils::CheckVkResult(CreateFunction(owner), "Failed to create Vulkan type");
	}

	void OwnerDestroy(std::function<vk::Result(const OwnerType&)> DestroyFunction)
	{
		utils::CheckVkResult(DestroyFunction(owner), "Failed to destroy Vulkan type");
	}

	const OwnerType& owner;
};

#define ASSERT_VK(VkResult) utils::CheckVkResult(VkResult)

template<class VulkanType>
class IVulkanType : public VulkanType
{
public:
	VulkanType& GetBase() { return *this; }
	const VulkanType& GetBase() const { return *this; }
};


#define BK_TYPE(Type) \
class Type;           \
class S##Type : public std::shared_ptr<Type> \
{                     \
public:               \
	template <class ...Args>                   \
	static std::shared_ptr<Type> Make(Args&&... args) \
	{                    \
		return std::make_shared<Type>(args...);  \
	}                    \
};                     \
                      \
using W##Type = std::weak_ptr<Type>;         \
class U##Type : public std::unique_ptr<Type> \
{                     \
public:               \
	template <class ...Args>                   \
	static std::unique_ptr<Type> Make(Args&&... args) \
	{                    \
		return std::make_unique<Type>(args...);  \
	}                    \
};                     \

#define BK_TYPE_BODY(VulkanType, OwnerType) \

#define CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, derived, EXT) \
    class ownerName;\
    class myName derived \
    { \
    public:\
        myName() = default;\
        ~myName() = default;\
        virtual void Destroy() = 0;                                     \
                                                                        \
    inline vk::vkName##EXT  &Get() { return *this; }\
    inline const vk::vkName##EXT & Get() const { return *this; }        \
    protected:\
        ownerName* m_owner = {};                                        \
        void DestroyBase()                                                      \
        {                                                                      \
            if(m_owner)                                                          \
            {                                                                     \
                m_owner->destroy##vkName##EXT(Get()); \
                m_owner = nullptr;\
            }\
        }\
    public:\


#define CUSTOM_VK_DERIVED_CREATE_FULL(myName, vkName, ownerName, EXT, createName, constructName) \
    public:\
        template <class ...Args>\
        void Create(const vk::createName##CreateInfo##EXT & createInfo, \
            ownerName& owner, Args&&... args)\
        {\
            utils::CheckVkResult(owner.create##constructName##EXT(args... , &createInfo, nullptr, &Get()), \
                std::string("Failed to construct ") + #vkName );\
            m_owner = &owner; \
            CheckInitialization<myName>(*this);\
        }\
        template <class ...Args>\
        static void Create(myName*&& obj, const vk::createName##CreateInfo##EXT & createInfo, \
            ownerName& owner, Args &&... args)\
        {\
            obj->Create(createInfo, owner, std::forward(args)...);\
        }\
    private:\

#define CUSTOM_VK_DERIVED_CREATE(myName, vkName, ownerName, EXT) CUSTOM_VK_DERIVED_CREATE_FULL(myName, vkName, ownerName, EXT, vkName, vkName)


#define DERIVED_GETTER(myName, vkName, EXT)\
public:\
    inline vk::vkName##EXT  &Get() { return *this; }\
    inline const vk::vkName##EXT & Get() const { return *this; }\


#define CUSTOM_VK_DECLARE_DERIVE_NC(myName, vkName, ownerName) \
    CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, : public vk::vkName , ) DERIVED_GETTER(myName, vkName, )

#define CUSTOM_VK_DECLARE_DERIVE(myName, vkName, ownerName) \
    CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, : public vk::vkName , ) CUSTOM_VK_DERIVED_CREATE(myName, vkName, ownerName, ) DERIVED_GETTER(myName, vkName, )

#define CUSTOM_VK_DECLARE_DERIVE_KHR(myName, vkName, ownerName) \
    CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, : public vk::vkName##KHR , KHR) CUSTOM_VK_DERIVED_CREATE(myName, vkName, ownerName, KHR) DERIVED_GETTER(myName, vkName, KHR)

#define CUSTOM_VK_DEFINE_FULL(myName, vkName, ownerName, EXT)\
    void myName::Destroy()\
    {\
        if(m_owner)\
        {\
            m_owner->destroy##vkName##EXT(Get()); \
            m_owner = nullptr;\
        }\
    }

#define CUSTOM_VK_DEFINE(myName, vkName, ownerName) CUSTOM_VK_DEFINE_FULL(myName, vkName, ownerName,)
#define CUSTOM_VK_DEFINE_KHR(myName, vkName, ownerName) CUSTOM_VK_DEFINE_FULL(myName, vkName, ownerName, KHR)


