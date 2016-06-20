#ifndef _GRANNY_H
#define _GRANNY_H

#include "structs.h"
#include "AutoGranny.h"

#define FUNC(ret, name, args) \
	extern "C" typedef ret (__stdcall *raw_##name) args; \
	raw_##name* const name = (raw_##name*)&_##name;

struct t_Logger
{
	void (__cdecl *a) (int a, int b, char* msg);
	void (__cdecl *b) (int a, int b, char* msg);
};

struct t_File;

struct t_Vertex_PWNT3432
{
	granny_real32 Position[3];
	granny_uint8 BoneWeights[4];
	granny_uint8 BoneIndices[4];
	granny_real32 Normal[3];
	granny_real32 TextureCoordinates0[2];
};

FUNC(void, GrannyCopyMeshVertices, (t_Meshes* mesh, int vertextype, void* buffer));

FUNC(t_FileInfo*, GrannyGetFileInfo, (t_File* file));

FUNC(t_File*, GrannyReadEntireFile, (const char* filename));
FUNC(void, GrannySetLogCallback, (t_Logger* logger));
FUNC(void, GrannyComputeBasisConversion, (t_FileInfo* info, float units_per_meter,
	const float* origin, const float* right, const float* up, const float* back, float* a, float* b, float* c));
FUNC(void, GrannyTransformFile, (t_FileInfo* info, float* a, float* b, float* c, int n));

#endif
