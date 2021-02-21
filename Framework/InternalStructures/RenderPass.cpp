//------------------------------------------------------------------------------
//
// File Name:	RenderPass.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/23/2020
//
//------------------------------------------------------------------------------

CUSTOM_VK_DEFINE(RenderPass, RenderPass, Device)


void RenderPass::Create(const vk::RenderPassCreateInfo& info,
			Device& owner,
			vk::Extent2D extent,
			const std::vector<vk::ClearValue>& clearValues)
{
	m_owner = &owner;
	this->extent = extent;
	this->clearValues = clearValues;
	owner.createRenderPass(&info, nullptr, &m_object);
}

