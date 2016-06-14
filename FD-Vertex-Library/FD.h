#ifndef _FD_H
#define _FD_H

#include "CRC32.h"

typedef unsigned char fd_int8;
typedef unsigned short fd_int16;
typedef unsigned int fd_int32;
typedef float fd_float;

typedef struct fd_header fd_header;
typedef struct fd_packet_group fd_packet_group;
typedef struct fd_vertex_packet fd_vertex_packet;
typedef struct fd_vertex fd_vertex;
typedef struct fd_pwngt34332_vertex fd_pwngt34332_vertex;

#define FD_MAGIC 0xfdabcd01

struct fd_header {
	fd_int32 MagicValue;
	fd_int32 DataSize;
	fd_int32 CRC32;
	fd_int32 TotalVertexCount;
	fd_int32 GroupCount;
	fd_int32 GroupSize;
	fd_int32 PacketCount;
	fd_int32 PacketSize;
	fd_int32 VertexCount; 
	fd_int32 VertexSize;
	fd_int32 PacketVertexCount; // sum of vertex count of all vertex packets, usually equals to VertexPacketCount * 4
	fd_int32 LargestBoneIndex; // largest bone index found in the vertices
	fd_int32 IncludeTangentVectors; // bool flag which specifies if the data originally contains tangent vectors
};

// Group of vertex packets weighed to the same bone
struct fd_packet_group {
	fd_int16 PacketCount;
	fd_int16 FirstVertexIndex;
	fd_int32 BoneIndex;
};

// Packet of 4 vertices
struct fd_vertex_packet {
	fd_float Position[3][4];
	fd_int32 Normal[4];
	fd_int32 Tangent[4];
	fd_float TextureCoordinates0[4][2];
};

// Vertex weighed to 2 bones
struct fd_vertex {
	fd_float Position[3];
	fd_int8 BoneWeights[2];
	fd_int8 BoneIndices[2];
	fd_int32 Normal;
	fd_int32 Tangent;
	fd_float TextureCoordinates0[2];
};

struct fd_pwngt34332_vertex {
	fd_float Position[3];
	fd_int8 BoneWeights[4];
	fd_int8 BoneIndices[4];
	fd_float Normal[3];
	fd_float Tangent[3];
	fd_float TextureCoordinates0[2];
};

#ifdef __cplusplus
extern "C" {
#endif

fd_int32 FDGetVertexCount(const void* Source);

fd_int32 FDGetVertices(const void* Source, fd_pwngt34332_vertex* DestVertices);

#ifdef __cplusplus
}
#endif

#endif