//------------------------------------------------------------------------------
//
// File Name:	Semaphore.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/26/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace dm
{

class Semaphore : public IVulkanType<vk::Semaphore>, public IOwned<Device>
{
DM_TYPE_VULKAN_OWNED_BODY(Semaphore, IOwned<Device>)

DM_TYPE_VULKAN_OWNED_GENERIC(Semaphore, Semaphore)
};

}


