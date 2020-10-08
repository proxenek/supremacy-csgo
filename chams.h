#pragma once

class Chams {
public:
	void SetColor(Color col, IMaterial* mat = nullptr);
	void SetAlpha(float alpha, IMaterial* mat = nullptr);
	void SetupMaterial(IMaterial* mat, Color col, bool z_flag);

	void init();

	bool GenerateLerpedMatrix(int index, BoneArray* out);

	void DrawChams(void* ecx, uintptr_t ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* bone);
	IMaterial* debugambientcube;
	IMaterial* debugdrawflat;
};

extern Chams g_chams;