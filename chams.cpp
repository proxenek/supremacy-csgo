#include "includes.h"
#include "studiorender.h"

Chams g_chams{ };;

void Chams::SetColor(Color col, IMaterial* mat) {
	if (mat)
		mat->ColorModulate(col);
	g_csgo.m_render_view->SetColorModulation(col);
}

void Chams::SetAlpha(float alpha, IMaterial* mat) {
	if (mat)
		mat->AlphaModulate(alpha);
	g_csgo.m_render_view->SetBlend(alpha);
}

void Chams::SetupMaterial(IMaterial* mat, Color col, bool z_flag) {
	SetColor(col);

	// mat->SetFlag( MATERIAL_VAR_HALFLAMBERT, flags );
	mat->SetFlag(MATERIAL_VAR_ZNEARER, z_flag);
	mat->SetFlag(MATERIAL_VAR_NOFOG, z_flag);
	mat->SetFlag(MATERIAL_VAR_IGNOREZ, z_flag);

	g_csgo.m_model_render->ForcedMaterialOverride(mat);
}

void Chams::init() {
	// find stupid materials.
	debugambientcube = g_csgo.m_material_system->FindMaterial(XOR("debug/debugambientcube"), nullptr);
	debugambientcube->IncrementReferenceCount();

	debugdrawflat = g_csgo.m_material_system->FindMaterial(XOR("debug/debugdrawflat"), nullptr);
	debugdrawflat->IncrementReferenceCount();
}

bool Chams::GenerateLerpedMatrix(int index, BoneArray* out) {
	LagRecord* current_record;
	AimPlayer* data;

	Player* ent = g_csgo.m_entlist->GetClientEntity< Player* >(index);
	if (!ent)
		return false;

	if (!g_aimbot.IsValidTarget(ent))
		return false;

	data = &g_aimbot.m_players[index - 1];
	if (!data || data->m_records.empty())
		return false;

	if (data->m_records.size() < 2)
		return false;

	auto* channel_info = g_csgo.m_engine->GetNetChannelInfo();
	if (!channel_info)
		return false;

	static float max_unlag = 0.2f;
	static auto sv_maxunlag = g_csgo.m_cvar->FindVar(HASH("sv_maxunlag"));
	if (sv_maxunlag) {
		max_unlag = sv_maxunlag->GetFloat();
	}

	for (auto it = data->m_records.rbegin(); it != data->m_records.rend(); it++) {
		current_record = it->get();

		bool end = it + 1 == data->m_records.rend();

		if (current_record && current_record->valid() && (!end && ((it + 1)->get()))) {
			if (current_record->m_origin.dist_to(ent->GetAbsOrigin()) < 1.f) {
				return false;
			}

			vec3_t next = end ? ent->GetAbsOrigin() : (it + 1)->get()->m_origin;
			float  time_next = end ? ent->m_flSimulationTime() : (it + 1)->get()->m_sim_time;

			float total_latency = channel_info->GetAvgLatency(0) + channel_info->GetAvgLatency(1);
			total_latency = std::clamp(total_latency, 0.f, max_unlag);

			float correct = total_latency + g_cl.m_lerp;
			float time_delta = time_next - current_record->m_sim_time;
			float add = end ? 1.f : time_delta;
			float deadtime = current_record->m_sim_time + correct + add;

			float curtime = g_csgo.m_globals->m_curtime;
			float delta = deadtime - curtime;

			float mul = 1.f / add;
			vec3_t lerp = math::Interpolate(next, current_record->m_origin, std::clamp(delta * mul, 0.f, 1.f));

			matrix3x4_t ret[128];

			std::memcpy(ret,
				current_record->m_bones,
				sizeof(ret));

			for (size_t i{ }; i < 128; ++i) {
				vec3_t matrix_delta = current_record->m_bones[i].GetOrigin() - current_record->m_origin;
				ret[i].SetOrigin(matrix_delta + lerp);
			}

			std::memcpy(out,
				ret,
				sizeof(ret));

			return true;
		}
	}

	return false;
}

void Chams::DrawChams(void* ecx, uintptr_t ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* bone) {

	AimPlayer* data;
	LagRecord* record;

	if (strstr(info.m_model->m_name, XOR("models/player")) != nullptr) {
		Player* m_entity = g_csgo.m_entlist->GetClientEntity<Player*>(info.m_index);
		if (!m_entity)
			return;

		g_csgo.m_model_render->ForcedMaterialOverride(nullptr);
		g_csgo.m_render_view->SetColorModulation(colors::white);
		g_csgo.m_render_view->SetBlend(1.f);

		if (m_entity->m_bIsLocalPlayer()) {
			if (g_menu.main.players.chams_local_scope.get() && m_entity->m_bIsScoped())
				SetAlpha(0.5f);

			else if (g_menu.main.players.chams_local.get()) {
				// override blend.
				SetAlpha(g_menu.main.players.chams_local_blend.get() / 100.f);

				// set material and color.
				SetupMaterial(debugambientcube, g_menu.main.players.chams_local_col.get(), false);
				g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(ecx, ctx, state, info, bone);
			}
		}

		bool enemy = g_cl.m_local && m_entity->enemy(g_cl.m_local);

		if (enemy && g_menu.main.players.chams_enemy_history.get()) {
			if (g_aimbot.IsValidTarget(m_entity)) {
				if (g_menu.main.players.chams_enemy_history.get()) {
					data = &g_aimbot.m_players[m_entity->index() - 1];
					if (!data->m_records.empty())
					{
						record = g_resolver.FindLastRecord(data);
						if (record) {
							// was the matrix properly setup?
							BoneArray arr[128];
							if (Chams::GenerateLerpedMatrix(m_entity->index(), arr)) {
								// override blend.
								SetAlpha(g_menu.main.players.chams_enemy_history_blend.get() / 100.f);

								// set material and color.
								SetupMaterial(debugdrawflat, g_menu.main.players.chams_enemy_history_col.get(), true);

								g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(ecx, ctx, state, info, arr);
							}
						}
					}
				}
			}
		}

		if (enemy && g_menu.main.players.chams_enemy.get(0)) {
			if (g_menu.main.players.chams_enemy.get(1)) {

				SetAlpha(g_menu.main.players.chams_enemy_blend.get() / 100.f);
				SetupMaterial(debugambientcube, g_menu.main.players.chams_enemy_invis.get(), true);

				g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(ecx, ctx, state, info, bone);
			}

			SetAlpha(g_menu.main.players.chams_enemy_blend.get() / 100.f);
			SetupMaterial(debugambientcube, g_menu.main.players.chams_enemy_vis.get(), false);

			g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(ecx, ctx, state, info, bone);
		}

		else if (!enemy && g_menu.main.players.chams_friendly.get(0)) {
			if (g_menu.main.players.chams_friendly.get(1)) {

				SetAlpha(g_menu.main.players.chams_friendly_blend.get() / 100.f);
				SetupMaterial(debugambientcube, g_menu.main.players.chams_friendly_invis.get(), true);

				g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(ecx, ctx, state, info, bone);
			}

			SetAlpha(g_menu.main.players.chams_friendly_blend.get() / 100.f);
			SetupMaterial(debugambientcube, g_menu.main.players.chams_friendly_vis.get(), false);

			g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(ecx, ctx, state, info, bone);
		}
	}
}
