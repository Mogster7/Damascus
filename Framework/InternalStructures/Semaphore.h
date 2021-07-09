//------------------------------------------------------------------------------
//
// File Name:	Semaphore.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/26/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace bk {

//BK_TYPE(Semaphore)
class Semaphore : public IVulkanType<vk::Semaphore>, public IOwned<Device>
{
BK_TYPE_VULKAN_OWNED_BODY(Semaphore, IOwned<Device>)

BK_TYPE_VULKAN_OWNED_GENERIC(Semaphore, Semaphore)
};

}


