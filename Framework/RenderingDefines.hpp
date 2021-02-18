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


// Tests if the class has a method by the name of Initialization
// If it does, it will execute it immediately following the constructor of our underlying Vulkan structure
template <typename T>
class HasInitialization
{
    typedef char one;
    struct two { char x[2]; };

    template <typename C> constexpr static one test(decltype(&C::Initialization));
    template <typename C> constexpr static two test(...);

public:
    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

template <typename T>
constexpr void CheckInitialization(T& obj)
{
    if constexpr (HasInitialization<T>::value)
        obj.Initialization();
}


#define CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, derived, EXT) \
    class ownerName;\
    class myName derived \
    { \
    public:\
        myName() = default;\
        ~##myName() = default;\
        void Destroy();\
        operator vk::vkName##EXT &() { return Get(); }\
        operator const vk::vkName##EXT &() const { return Get(); }\
    \
    protected:\
        ownerName* m_owner = {};\
    private:



#define CUSTOM_VK_CREATE(myName, vkName, ownerName, EXT) \
    public:\
        template <class ...Args>\
        void Create(const vk::vkName##CreateInfo##EXT & createInfo, ownerName& owner, Args&&... args)\
        {\
            m_owner = &owner; \
            utils::CheckVkResult(m_owner->create##vkName##EXT(args... , &createInfo, nullptr, &m_object), \
                std::string("Failed to construct ") + #vkName );\
            CheckInitialization<myName>(*this);\
        }\
    private:\

#define CUSTOM_VK_DERIVED_CREATE_FULL(myName, vkName, ownerName, EXT, createName, constructName) \
    public:\
        template <class ...Args>\
        static void Create(myName& obj, const vk::createName##CreateInfo##EXT & createInfo, \
            ownerName& owner, Args&&... args)\
        {\
            utils::CheckVkResult(owner.create##constructName##EXT(args... , &createInfo, nullptr, &obj), \
                std::string("Failed to construct ") + #vkName );\
            obj.m_owner = &owner; \
            CheckInitialization<myName>(obj);\
        }\
        template <class ...Args>\
        static void Create(myName*&& obj, const vk::createName##CreateInfo##EXT & createInfo, \
            ownerName& owner, Args &&... args)\
        {\
            Create(*obj, createInfo, owner, std::forward(args)...);\
        }\
    private:\

#define CUSTOM_VK_DERIVED_CREATE(myName, vkName, ownerName, EXT) CUSTOM_VK_DERIVED_CREATE_FULL(myName, vkName, ownerName, EXT, vkName, vkName)



#define MEMBER_GETTER(myName, vkName, EXT)\
public:\
    inline const vk::vkName##EXT& Get() const { return (m_object); }\
    inline vk::vkName##EXT& Get() { return (m_object); }\
private:\
    vk::vkName##EXT m_object = {};

#define DERIVED_GETTER(myName, vkName, EXT)\
public:\
    inline vk::vkName##EXT  &Get() { return *this; }\
    inline const vk::vkName##EXT & Get() const { return *this; }\
private:

#define CONCAT_COMMA(name) ,##name

#define CUSTOM_VK_DECLARE(myName, vkName, ownerName) \
    CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, , ) CUSTOM_VK_CREATE(myName, vkName, ownerName, ) MEMBER_GETTER(myName, vkName, )

#define CUSTOM_VK_DECLARE_NO_CREATE(myName, vkName, ownerName)\
    CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, , ) MEMBER_GETTER(myName, vkName, )

#define CUSTOM_VK_DECLARE_KHR(myName, vkName, ownerName) \
    CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, , KHR) CUSTOM_VK_CREATE(myName, vkName, ownerName, KHR) MEMBER_GETTER(myName, vkName, KHR)

#define CUSTOM_VK_DECLARE_DERIVE(myName, vkName, ownerName) \
    CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, : public vk::##vkName , ) CUSTOM_VK_DERIVED_CREATE(myName, vkName, ownerName, ) DERIVED_GETTER(myName, vkName, )

#define CUSTOM_VK_DECLARE_DERIVE_NO_CREATE(myName, vkName, ownerName) \
    CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, : public vk::##vkName , ) DERIVED_GETTER(myName, vkName, ) 

#define CUSTOM_VK_DECLARE_DERIVE_KHR(myName, vkName, ownerName) \
    CUSTOM_VK_DECLARE_FULL(myName, vkName, ownerName, : public vk::##vkName##KHR , KHR) CUSTOM_VK_DERIVED_CREATE(myName, vkName, ownerName, KHR) DERIVED_GETTER(myName, vkName, KHR)

#define CUSTOM_VK_DEFINE_FULL(myName, vkName, ownerName, EXT)\
    void myName::Destroy()\
    {\
        m_owner->destroy##vkName##EXT(Get());\
    }

#define CUSTOM_VK_DEFINE(myName, vkName, ownerName) CUSTOM_VK_DEFINE_FULL(myName, vkName, ownerName,)
#define CUSTOM_VK_DEFINE_KHR(myName, vkName, ownerName) CUSTOM_VK_DEFINE_FULL(myName, vkName, ownerName, KHR)


