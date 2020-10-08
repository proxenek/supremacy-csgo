#include "includes.h"

HVH g_hvh{ };;

void HVH::AntiAim() {
	bool attack, attack2;

	if (!g_menu.main.antiaim.enable.get()) {
		m_should_work = false;
		return;
	}

	float amount = g_cl.m_cmd->m_buttons & IN_DUCK ? 3.f : 1.01f;

	bool press = (g_cl.m_cmd->m_buttons & (IN_LEFT) || g_cl.m_cmd->m_buttons & (IN_FORWARD) || g_cl.m_cmd->m_buttons & (IN_BACK) ||
		g_cl.m_cmd->m_buttons & (IN_RIGHT) || g_cl.m_cmd->m_buttons & (IN_MOVELEFT) || g_cl.m_cmd->m_buttons & (IN_MOVERIGHT) ||
		g_cl.m_cmd->m_buttons & (IN_JUMP));

	if (g_menu.main.antiaim.desync.get()) { // won't fuck up even if you're shooting
		if (!press && (g_cl.m_local->m_vecVelocity().length_2d() <= 0.f || std::fabs(g_cl.m_local->m_vecVelocity().z) <= 100.f))
			g_cl.m_cmd->m_side_move += g_cl.m_cmd->m_command_number % 2 ? amount : -amount;
	}

	if ((g_cl.m_local->m_fFlags() & FL_ONGROUND) && g_input.GetKeyState(g_menu.main.antiaim.fakeduck.get())) {
		m_fakeduck = true;

		g_cl.m_cmd->m_buttons |= IN_BULLRUSH;

		if (g_csgo.m_cl->m_choked_commands <= 7) {
			g_cl.m_cmd->m_buttons &= ~IN_DUCK;
		}
		// duck if we are choking more than 7 ticks.
		else {
			g_cl.m_cmd->m_buttons |= IN_DUCK;
		}
	}
	else
		m_fakeduck = false;

	attack = g_cl.m_cmd->m_buttons & IN_ATTACK;
	attack2 = g_cl.m_cmd->m_buttons & IN_ATTACK2;

	if (g_cl.IsFiring(game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase())))
		return;

	// disable conditions.
	if (g_csgo.m_gamerules->m_bFreezePeriod() || (g_cl.m_flags & FL_FROZEN) || g_cl.m_round_end || (g_cl.m_cmd->m_buttons & IN_USE)) {
		m_should_work = false;
		return;
	}

	// grenade throwing
	// CBaseCSGrenade::ItemPostFrame()
	// https://github.com/VSES/SourceEngine2007/blob/master/src_main/game/shared/cstrike/weapon_basecsgrenade.cpp#L209
	if (g_cl.m_weapon && g_cl.m_weapon->IsGrenade()
		&& (!g_cl.m_weapon->m_bPinPulled() || attack || attack2)
		&& g_cl.m_weapon->m_fThrowTime() > 0.f && g_cl.m_weapon->m_fThrowTime() < g_csgo.m_globals->m_curtime) {
		m_should_work = false;
		return;
	}

	m_should_work = true;

	if (g_menu.main.antiaim.pitch.get())
		g_cl.m_cmd->m_view_angles.x = g_menu.main.config.mode.get() == 1 ? -180.f : 89.f;

	int side = invert ? -1 : 1;
	float angle = 125.f;

	if (g_menu.main.antiaim.yaw.get()) {
		if (!g_menu.main.antiaim.desync.get())
			g_cl.m_cmd->m_view_angles.y += 180.f;
		else {
			if (*g_cl.m_packet)
				g_cl.m_cmd->m_view_angles.y += (angle + 60.f) * side;
			else
				g_cl.m_cmd->m_view_angles.y += (angle - 60.f) * side;
		}
	}
}

void HVH::SendPacket() {
	// if not the last packet this shit wont get sent anyway.
	// fix rest of hack by forcing to false.
	if (!*g_cl.m_final_packet)
		*g_cl.m_packet = false;

	if (g_aimbot.CanDT()) {
		*g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= 1;
	}
	else {
		// fake-lag enabled.
		if (g_menu.main.antiaim.lag_enable.get() && !g_csgo.m_gamerules->m_bFreezePeriod() && !(g_cl.m_flags & FL_FROZEN)) {
			// limit of lag.
			int limit = std::min((int)g_menu.main.antiaim.lag_limit.get(), g_cl.m_max_lag);

			// indicates wether to lag or not.
			bool active{ };

			// get current origin.
			vec3_t cur = g_cl.m_local->m_vecOrigin();

			// get prevoius origin.
			vec3_t prev = g_cl.m_net_pos.empty() ? g_cl.m_local->m_vecOrigin() : g_cl.m_net_pos.front().m_pos;

			// delta between the current origin and the last sent origin.
			float delta = (cur - prev).length_sqr();

			auto activation = g_menu.main.antiaim.lag_active.GetActiveIndices();
			for (auto it = activation.begin(); it != activation.end(); it++) {

				// move.
				if (*it == 0 && delta > 0.1f && g_cl.m_speed > 0.1f) {
					active = true;
					break;
				}

				// air.
				else if (*it == 1 && ((g_cl.m_buttons & IN_JUMP) || !(g_cl.m_flags & FL_ONGROUND))) {
					active = true;
					break;
				}

				// crouch.
				else if (*it == 2 && g_cl.m_local->m_bDucking()) {
					active = true;
					break;
				}
			}

			if (active) {
				int mode = g_menu.main.antiaim.lag_mode.get();

				// max.
				if (mode == 0)
					*g_cl.m_packet = false;

				// break.
				else if (mode == 1 && delta <= 4096.f)
					*g_cl.m_packet = false;

				// random.
				else if (mode == 2) {
					// compute new factor.
					if (g_cl.m_lag >= m_random_lag)
						m_random_lag = g_csgo.RandomInt(2, limit);

					// factor not met, keep choking.
					else *g_cl.m_packet = false;
				}

				// break step.
				else if (mode == 3) {
					// normal break.
					if (m_step_switch) {
						if (delta <= 4096.f)
							*g_cl.m_packet = false;
					}

					// max.
					else *g_cl.m_packet = false;
				}

				if (g_cl.m_lag >= limit)
					*g_cl.m_packet = true;
			}
		}

		if (!g_menu.main.antiaim.lag_land.get()) {
			vec3_t                start = g_cl.m_local->m_vecOrigin(), end = start, vel = g_cl.m_local->m_vecVelocity();
			CTraceFilterWorldOnly filter;
			CGameTrace            trace;

			// gravity.
			vel.z -= (g_csgo.sv_gravity->GetFloat() * g_csgo.m_globals->m_interval);

			// extrapolate.
			end += (vel * g_csgo.m_globals->m_interval);

			// move down.
			end.z -= 2.f;

			g_csgo.m_engine_trace->TraceRay(Ray(start, end), MASK_SOLID, &filter, &trace);

			// check if landed.
			if (trace.m_fraction != 1.f && trace.m_plane.m_normal.z > 0.7f && !(g_cl.m_flags & FL_ONGROUND))
				*g_cl.m_packet = true;
		}

		// force fake-lag to 14 when fakelagging.
		if (m_fakeduck) {
			*g_cl.m_packet = false;
		}

		// do not lag while shooting.
		if (!m_fakeduck && g_cl.m_old_shot)
			*g_cl.m_packet = true;
	}

	// we somehow reached the maximum amount of lag.
	// we cannot lag anymore and we also cannot shoot anymore since we cant silent aim.
	if (g_cl.m_lag >= g_cl.m_max_lag) {
		// set bSendPacket to true.
		*g_cl.m_packet = true;

		// disable firing, since we cannot choke the last packet.
		g_cl.m_weapon_fire = false;
	}
}