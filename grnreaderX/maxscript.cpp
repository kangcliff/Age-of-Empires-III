#include <vector>
#include "maxscript.h"
#include "Granny.h"
#include "FD.h"

#pragma comment(lib, "AoE3FDLibrary.lib")

const float max_upm = 1.0f / 0.0254f;
const int upm_raw = 0x3f1d7af6;
const float max_origin[] = { 0.0f, 0.0f, 0.0f };
const float max_right[] = { 1.0f, 0.0f, 0.0f };
const float max_up[] = { 0.0f, 0.0f, 1.0f };
const float max_back[] = { 0.0f, -1.0f, 0.0f };

inline int GetTextureID(const t_FileInfo * info, const t_Texture * texture) {
	for (int i = 0; i < info->Textures_count; ++i) {
		if (texture == info->Textures[i])
			return i;
	}
	return -1;
}

void InverseMatrix(const float * data, float * invOut)
{
	float m[16], inv[16], det;
	int i;

	for (i = 0; i < 16; ++i)
		m[i] = data[i];

	inv[0] = m[5] * m[10] * m[15] -
		m[5] * m[11] * m[14] -
		m[9] * m[6] * m[15] +
		m[9] * m[7] * m[14] +
		m[13] * m[6] * m[11] -
		m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	if (det == 0)
	{
		for (i = 0; i < 16; ++i)
			invOut[i] = 0.0f;
	}

	det = 1.0f / det;

	for (i = 0; i < 16; i++)
		invOut[i] = inv[i] * det;
}

int OutputMAXScript(std::stringstream& ss, const char * filename) {
	t_File * file;
	t_FileInfo * info;

	std::vector<fd_pwngt34332_vertex *> vertices_ptrs;
	t_ArtToolInfo * art_info;
	float a[3], b[9], c[9];

	file = (*GrannyReadEntireFile)(filename);
	if (!file)
		return MS_FILE_ERROR;

	info = (*GrannyGetFileInfo)(file);
	if (!info)
		return MS_INFO_ERROR;

	for (int i = 0; i < info->Meshes_count; ++i) {
		t_Mesh * mesh = info->Meshes[i];
		t_VertexData * vertex_data = mesh->PrimaryVertexData;
		fd_pwngt34332_vertex * vertices;
		int vertices_count;
		if (strcmp(vertex_data->Vertex_type->Name, "FD") == 0) {
			vertices_count = FDGetVertexCount(vertex_data->Vertices);
			vertices = new fd_pwngt34332_vertex[vertices_count];
			vertices_ptrs.push_back(vertices);

			if (!FDGetVertices(vertex_data->Vertices, vertices))
				return MS_FD_ERROR;

			vertex_data->Vertices = vertices;
			vertex_data->Vertex_type = (t_Type*)GrannyPWNGT34332VertexType;
			vertex_data->Vertices_count = vertices_count;
		}
	}

	art_info = info->ArtToolInfo;
	const float * upm = (const float *)&upm_raw;
	(*GrannyComputeBasisConversion)(info, *upm,
		max_origin, max_right, max_up, max_back,
		a, b, c);
	(*GrannyTransformFile)(info, a, b, c, 2);

	for (int i = 0; i < info->Textures_count; ++i)
		ss << "t \"" << info->Textures[i]->FromFileName << "\"\n";

	for (int i = 0; i < info->Materials_count; ++i) {
		t_Material * material = info->Materials[i];
		if (!material->Maps_count)
			continue;
		ss << "mat \"" << material->Name << "\"\n";
		for (int m = 0; m < material->Maps_count; ++m) {
			t_Maps& map = material->Maps[m];
			if (strcmp(map.Usage, "Diffuse Color") == 0)
				ss << "td " << GetTextureID(info, map.Map->Texture) + 1 << "\n";
			else if (strcmp(map.Usage, "Opacity") == 0)
				ss << "tp " << GetTextureID(info, map.Map->Texture) + 1 << "\n";
			else if (strcmp(map.Usage, "Bump") == 0)
				ss << "tb " << GetTextureID(info, map.Map->Texture) + 1 << "\n";
		}
	}

	for (int m = 0; m < info->Models_count; ++m) {
		t_Models * model = info->Models[m];
		t_Skeletons * skeleton = model->Skeleton;

		ss << "md \"" << model->Name << "\"\n";
		for (int i = 0; i < skeleton->Bones_count; ++i) {
			t_Bones& bone = skeleton->Bones[i];
			float mat[16];
			InverseMatrix(bone.InverseWorldTransform, mat);

			ss << "b \"" << bone.Name << "\" " << bone.ParentIndex + 1 << " "
				<< mat[0] << " " << mat[1] << " " << mat[2] << " "
				<< mat[4] << " " << mat[5] << " " << mat[6] << " "
				<< mat[8] << " " << mat[9] << " " << mat[10] << " "
				<< mat[12] << " " << mat[13] << " " << mat[14] << "\n";
		}

		for (int i = 0; i < model->MeshBindings_count; ++i) {
			t_Mesh * mesh = model->MeshBindings[i].Mesh;
			t_VertexData * vertex_data = mesh->PrimaryVertexData;
			t_Vertex_PWNT3432 * vertices;
			int vertices_count;
			const bool vertex_weights = !(*_GrannyDataTypesAreEqual)((int)vertex_data->Vertex_type, GrannyPNT332VertexType) &&
				!(*_GrannyDataTypesAreEqual)((int)vertex_data->Vertex_type, GrannyPNGT3332VertexType);

			ss << "m \"" << mesh->Name << "\"\n";

			vertices_count = vertex_data->Vertices_count;
			vertices = new t_Vertex_PWNT3432[vertices_count];
			(*GrannyCopyMeshVertices)(mesh, GrannyPWNT3432VertexType, vertices);

			for (int v = 0; v < vertices_count; ++v) {
				t_Vertex_PWNT3432& vertex = vertices[v];
				ss << "v " << vertex.Position[0] << " " << vertex.Position[1] << " " << vertex.Position[2] << "\n"
					<< "vn " << vertex.Normal[0] << " " << vertex.Normal[1] << " " << vertex.Normal[2] << "\n"
					<< "vt " << vertex.TextureCoordinates0[0] << " " << 1 - vertex.TextureCoordinates0[1] << "\n";

				if (vertex_weights)
					ss << "vw "
					<< (double)vertex.BoneWeights[0] / 255.0f << " "
					<< (double)vertex.BoneWeights[1] / 255.0f << " "
					<< (double)vertex.BoneWeights[2] / 255.0f << " "
					<< (double)vertex.BoneWeights[3] / 255.0f << " "
					<< vertex.BoneIndices[0] + 1 << " "
					<< vertex.BoneIndices[1] + 1 << " "
					<< vertex.BoneIndices[2] + 1 << " "
					<< vertex.BoneIndices[3] + 1 << "\n";
			}

			delete[] vertices;

			if (mesh->PrimaryTopology->Indices16_count)
			{
				t_Indices16 *indices16 = mesh->PrimaryTopology->Indices16;
				for (int g = 0; g < mesh->PrimaryTopology->Groups_count; ++g)
				{
					t_Groups& group = mesh->PrimaryTopology->Groups[g];
					ss << "fg " << group.MaterialIndex + 1 << "\n";
					for (int t = group.TriFirst; t < group.TriFirst + group.TriCount; ++t)
						ss << "f " << indices16[t * 3 + 0].Int16 + 1 << " " <<
						indices16[t * 3 + 1].Int16 + 1 << " " <<
						indices16[t * 3 + 2].Int16 + 1 << "\n";

				}
			}
			else
			{
				t_Indices *indices = mesh->PrimaryTopology->Indices;
				for (int g = 0; g < mesh->PrimaryTopology->Groups_count; ++g)
				{
					t_Groups& group = mesh->PrimaryTopology->Groups[g];
					ss << "fg " << group.MaterialIndex + 1 << "\n";
					for (int t = group.TriFirst; t < group.TriFirst + group.TriCount; ++t)
						ss << "f " << indices[t * 3 + 0].Int32 + 1 << " " <<
						indices[t * 3 + 1].Int32 + 1 << " " <<
						indices[t * 3 + 2].Int32 + 1 << "\n";
				}
			}

			for (int mb = 0; mb < mesh->MaterialBindings_count; ++mb)
			{
				int id = 0;
				for (int m = 0; m < info->Materials_count; ++m)
				{
					t_Material * material = info->Materials[m];
					if (!material->Maps_count)
						continue;

					if (mesh->MaterialBindings[mb].Material == material)
					{
						ss << "mm " << id + 1 << "\n";
						break;
					}

					++id;
				}
			}

			for (int b = 0; b < mesh->BoneBindings_count; ++b)
				ss << "mb \"" << mesh->BoneBindings[b].BoneName << "\"\n";
		}
	}

	for (int a = 0; a < info->Animations_count; ++a) {
		t_Animations * anim = info->Animations[a];
		float frames = anim->Duration / anim->TimeStep,
			fps = 1.0f / anim->TimeStep;

		ss << "a " << frames << " "
			<< fps << "\n";

		for (int t = 0; t < anim->TrackGroups_count; ++t) {
			t_TrackGroups * group = anim->TrackGroups[t];
			ss << "cg \"" << group->Name << "\"\n";
			for (int i = 0; i < group->TransformTracks_count; ++i) {
				t_TransformTracks& track = group->TransformTracks[i];
				ss << "c \"" << track.Name << "\"\n";
				for (int k = 0; k < track.PositionCurve.Knots_count; ++k)
					ss << "kp " << track.PositionCurve.Knots[k].Real32 * fps << " "
					<< track.PositionCurve.Controls[3 * k + 0].Real32 << " "
					<< track.PositionCurve.Controls[3 * k + 1].Real32 << " "
					<< track.PositionCurve.Controls[3 * k + 2].Real32 << "\n";
				for (int k = 0; k < track.OrientationCurve.Knots_count; ++k)
					ss << "kr " << track.OrientationCurve.Knots[k].Real32 * fps << " "
					<< track.OrientationCurve.Controls[4 * k + 0].Real32 << " "
					<< track.OrientationCurve.Controls[4 * k + 1].Real32 << " "
					<< track.OrientationCurve.Controls[4 * k + 2].Real32 << " "
					<< track.OrientationCurve.Controls[4 * k + 3].Real32 << "\n";
				for (int k = 0; k < track.ScaleShearCurve.Knots_count; ++k)
					ss << "ks " << track.ScaleShearCurve.Knots[k].Real32 * fps << " "
					<< track.ScaleShearCurve.Controls[9 * k + 0].Real32 << " "
					<< track.ScaleShearCurve.Controls[9 * k + 4].Real32 << " "
					<< track.ScaleShearCurve.Controls[9 * k + 8].Real32 << "\n";
			}
		}
	}

	for (size_t i = 0; i < vertices_ptrs.size(); ++i)
		delete[] vertices_ptrs[i];
	(*_GrannyFreeFile)((int)file);

	return MS_OK;
}
