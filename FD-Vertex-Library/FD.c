#include "FD.h"
#include "CRC32.h"

#define FD_EXT_VEC(v, i) (fd_float)(*((const fd_int8*)&v + i)) / 63.5f - 1.0f

fd_int32 FDGetVertexCount(const void* Source) {
	return ((const fd_header*)Source)->TotalVertexCount;
}

fd_int32 FDGetVertices(const void* Source, fd_pwngt34332_vertex* DestVertices)
{
	const fd_int8* data = (const fd_int8*)Source;
	const fd_header *header = (const fd_header*)data;

	if (header->MagicValue != FD_MAGIC)
		return 0;
	if (header->CRC32 != crc32(data + 12, header->DataSize))
		return 0;

	const fd_packet_group* groups = (const fd_packet_group*)(data + 64);
	const fd_vertex_packet* packets = (const fd_vertex_packet*)(data + 64 + header->GroupSize);
	const fd_vertex* vertices = (const fd_vertex*)(data + 64 + header->GroupSize + header->PacketSize);

	fd_int32 i = 0, p = 0;
	for (fd_int32 g = 0; g < header->GroupCount; ++g) {
		for (fd_int32 h = 0; h < groups[g].PacketCount; ++h) {
			for (fd_int32 v = 0; v < 4; ++v) {
				DestVertices[i].Position[0] = packets[p].Position[0][v];
				DestVertices[i].Position[1] = packets[p].Position[1][v];
				DestVertices[i].Position[2] = packets[p].Position[2][v];
				DestVertices[i].BoneWeights[0] = 255;
				DestVertices[i].BoneIndices[0] = groups[g].BoneIndex;
				DestVertices[i].BoneWeights[1] = 0;
				DestVertices[i].BoneIndices[1] = 0;
				DestVertices[i].BoneWeights[2] = 0;
				DestVertices[i].BoneIndices[2] = 0;
				DestVertices[i].BoneWeights[3] = 0;
				DestVertices[i].BoneIndices[3] = 0;
				DestVertices[i].Normal[0] = FD_EXT_VEC(packets[p].Normal[v], 0);
				DestVertices[i].Normal[1] = FD_EXT_VEC(packets[p].Normal[v], 1);
				DestVertices[i].Normal[2] = FD_EXT_VEC(packets[p].Normal[v], 2);
				DestVertices[i].Tangent[0] = FD_EXT_VEC(packets[p].Tangent[v], 0);
				DestVertices[i].Tangent[1] = FD_EXT_VEC(packets[p].Tangent[v], 1);
				DestVertices[i].Tangent[2] = FD_EXT_VEC(packets[p].Tangent[v], 2);
				DestVertices[i].TextureCoordinates0[0] = packets[p].TextureCoordinates0[v][0];
				DestVertices[i].TextureCoordinates0[1] = packets[p].TextureCoordinates0[v][1];

				++i;
			}
			++p;
		}

	}

	for (fd_int32 v = 0; v < header->VertexCount; ++v) {
		DestVertices[i].Position[0] = vertices[v].Position[0];
		DestVertices[i].Position[1] = vertices[v].Position[1];
		DestVertices[i].Position[2] = vertices[v].Position[2];
		DestVertices[i].BoneWeights[0] = vertices[v].BoneWeights[0];
		DestVertices[i].BoneIndices[0] = vertices[v].BoneIndices[0];
		DestVertices[i].BoneWeights[1] = vertices[v].BoneWeights[1];
		DestVertices[i].BoneIndices[1] = vertices[v].BoneIndices[1];
		DestVertices[i].BoneWeights[2] = 0;
		DestVertices[i].BoneIndices[2] = 0;
		DestVertices[i].BoneWeights[3] = 0;
		DestVertices[i].BoneIndices[3] = 0;
		DestVertices[i].Normal[0] = FD_EXT_VEC(vertices[v].Normal, 0);
		DestVertices[i].Normal[1] = FD_EXT_VEC(vertices[v].Normal, 1);
		DestVertices[i].Normal[2] = FD_EXT_VEC(vertices[v].Normal, 2);
		DestVertices[i].Tangent[0] = FD_EXT_VEC(vertices[v].Tangent, 0);
		DestVertices[i].Tangent[1] = FD_EXT_VEC(vertices[v].Tangent, 1);
		DestVertices[i].Tangent[2] = FD_EXT_VEC(vertices[v].Tangent, 2);
		DestVertices[i].TextureCoordinates0[0] = vertices[v].TextureCoordinates0[0];
		DestVertices[i].TextureCoordinates0[1] = vertices[v].TextureCoordinates0[1];

		++i;
	}

	return 1;
}