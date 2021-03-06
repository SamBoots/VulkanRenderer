#pragma once
#include "../../Structs/BufferData.h"

struct MeshData
{
	void DeleteBuffers(VkDevice& r_Device);

	BufferData<Vertex> vertices;
	BufferData<uint32_t> indices;
};