#include "includes.h"

void Hooks::DrawModelExecute(uintptr_t ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* bone) {
	// do chams.
	g_chams.init();

	if (g_csgo.m_model_render->IsForcedMaterialOverride()) {
		g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(this, ctx, state, info, bone);
		return;
	}

	if (g_csgo.m_engine->IsConnected() && g_csgo.m_engine->IsInGame()) {
		if (strstr(info.m_model->m_name, XOR("player/contactshadow")) != nullptr) {
			return;
		}

		g_chams.DrawChams(this, ctx, state, info, bone);
	}
	g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(this, ctx, state, info, bone);
	g_csgo.m_model_render->ForcedMaterialOverride(nullptr);
}