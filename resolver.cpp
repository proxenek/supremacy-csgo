#include "includes.h"

Resolver g_resolver{};;

LagRecord* Resolver::FindIdealRecord(AimPlayer* data) {
	LagRecord* first_valid, * current;

	if (data->m_records.empty())
		return nullptr;

	first_valid = nullptr;

	// iterate records.
	for (const auto& it : data->m_records) {
		if (it->dormant() || it->immune() || !it->valid())
			continue;

		// get current record.
		current = it.get();

		// first record that was valid, store it for later.
		if (!first_valid)
			first_valid = current;

		// try to find a record with a shot, lby update, walking or no anti-aim.
		if (it->m_shot || it->m_mode == Modes::RESOLVE_BODY || it->m_mode == Modes::RESOLVE_WALK || it->m_mode == Modes::RESOLVE_NONE)
			return current;
	}

	// none found above, return the first valid record if possible.
	return (first_valid) ? first_valid : nullptr;
}

LagRecord* Resolver::FindLastRecord(AimPlayer* data) {
	LagRecord* current;

	if (data->m_records.empty())
		return nullptr;

	// iterate records in reverse.
	for (auto it = data->m_records.crbegin(); it != data->m_records.crend(); ++it) {
		current = it->get();

		// if this record is valid.
		// we are done since we iterated in reverse.
		if (current->valid() && !current->immune() && !current->dormant())
			return current;
	}

	return nullptr;
}

void Resolver::OnBodyUpdate(Player* player, float value) {
	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	// set data.
	data->m_old_body = data->m_body;
	data->m_body = value;
}

float Resolver::GetAwayAngle(LagRecord* record) {
	float  delta{ std::numeric_limits< float >::max() };
	vec3_t pos;
	ang_t  away;

	// other cheats predict you by their own latency.
	// they do this because, then they can put their away angle to exactly
	// where you are on the server at that moment in time.

	// the idea is that you would need to know where they 'saw' you when they created their user-command.
	// lets say you move on your client right now, this would take half of our latency to arrive at the server.
	// the delay between the server and the target client is compensated by themselves already, that is fortunate for us.

	// we have no historical origins.
	// no choice but to use the most recent one.
	//if( g_cl.m_net_pos.empty( ) ) {
	math::VectorAngles(g_cl.m_local->m_vecOrigin() - record->m_pred_origin, away);
	return away.y;
	//}

	// half of our rtt.
	// also known as the one-way delay.
	//float owd = ( g_cl.m_latency / 2.f );

	// since our origins are computed here on the client
	// we have to compensate for the delay between our client and the server
	// therefore the OWD should be subtracted from the target time.
	//float target = record->m_pred_time; //- owd;

	// iterate all.
	//for( const auto &net : g_cl.m_net_pos ) {
		// get the delta between this records time context
		// and the target time.
	//	float dt = std::abs( target - net.m_time );

		// the best origin.
	//	if( dt < delta ) {
	//		delta = dt;
	//		pos   = net.m_pos;
	//	}
	//}

	//math::VectorAngles( pos - record->m_pred_origin, away );
	//return away.y;
}

void Resolver::MatchShot(AimPlayer* data, LagRecord* record) {
	// do not attempt to do this in nospread mode.

	float shoot_time = -1.f;

	Weapon* weapon = data->m_player->GetActiveWeapon();
	if (weapon) {
		// with logging this time was always one tick behind.
		// so add one tick to the last shoot time.
		shoot_time = weapon->m_fLastShotTime() + g_csgo.m_globals->m_interval;
	}

	// this record has a shot on it.
	if (game::TIME_TO_TICKS(shoot_time) == game::TIME_TO_TICKS(record->m_sim_time)) {
		if (record->m_lag <= 2)
			record->m_shot = true;

		// more then 1 choke, cant hit pitch, apply prev pitch.
		else if (data->m_records.size() >= 2) {
			LagRecord* previous = data->m_records[1].get();

			if (previous && !previous->dormant())
				record->m_eye_angles.x = previous->m_eye_angles.x;
		}
	}
}

void Resolver::SetMode(LagRecord* record) {
	// the resolver has 3 modes to chose from.
	// these modes will vary more under the hood depending on what data we have about the player
	// and what kind of hack vs. hack we are playing (mm/nospread).

	float speed = record->m_anim_velocity.length();

	// if on ground, moving, and not fakewalking.
	if ((record->m_flags & FL_ONGROUND) && speed > 0.1f && !record->m_fake_walk)
		record->m_mode = Modes::RESOLVE_WALK;

	// if on ground, not moving or fakewalking.
	if ((record->m_flags & FL_ONGROUND) && (speed <= 0.1f || record->m_fake_walk))
		record->m_mode = Modes::RESOLVE_STAND;

	// if not on ground.
	else if (!(record->m_flags & FL_ONGROUND))
		record->m_mode = Modes::RESOLVE_AIR;
}

float GetBackwardYaw(Player* player) {
	if (!g_cl.m_processing)
		return -1.f;

	return math::CalculateAngle(g_cl.m_local->m_vecOrigin(), player->m_vecOrigin()).y;
}

float GetAngle(Player* player) {
	return math::NormalizedAngle(player->m_angEyeAngles().y);
}

float GetForwardYaw(Player* player) {
	return math::NormalizedAngle(GetBackwardYaw(player) - 180.f);
}

void Resolver::DetectSide(Player* player, int* side)
{
	vec3_t src3D, dst3D, forward, right, up, src, dst;
	float back_two, right_two, left_two;
	CGameTrace tr;
	CTraceFilterSimple filter;

	math::AngleVectors(ang_t(0, GetBackwardYaw(player), 0), &forward, &right, &up);

	filter.SetPassEntity(player);
	src3D = player->GetShootPosition();
	dst3D = src3D + (forward * 384); //Might want to experiment with other numbers, incase you don't know what the number does, its how far the trace will go. Lower = shorter.

	g_csgo.m_engine_trace->TraceRay(Ray(src3D, dst3D), MASK_SHOT, &filter, &tr);
	back_two = (tr.m_endpos - tr.m_startpos).length();

	g_csgo.m_engine_trace->TraceRay(Ray(src3D + right * 35, dst3D + right * 35), MASK_SHOT, &filter, &tr);
	right_two = (tr.m_endpos - tr.m_startpos).length();

	g_csgo.m_engine_trace->TraceRay(Ray(src3D - right * 35, dst3D - right * 35), MASK_SHOT, &filter, &tr);
	left_two = (tr.m_endpos - tr.m_startpos).length();

	if (left_two > right_two) {
		*side = -1;
	}
	else if (right_two > left_two) {
		*side = 1;
	}
	else
		*side = 0;
}

void Resolver::ResolveAngles(Player* player, LagRecord* record) {
	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	// mark this record if it contains a shot.
	MatchShot(data, record);

	// next up mark this record with a resolver mode that will be used.
	SetMode(record);

	// if we are in nospread mode, force all players pitches to down.
	// TODO; we should check thei actual pitch and up too, since those are the other 2 possible angles.
	// this should be somehow combined into some iteration that matches with the air angle iteration.

	int shots = data->m_missed_shots;

	if (!record->m_shot) {
		if (g_menu.main.config.mode.get() == 1)
		{
			switch (shots % 6) {
			case 0:
			
				record->m_eye_angles.x = player->m_angEyeAngles().x = 90.f;
				break;

		
			case 1:
				record->m_eye_angles.x = player->m_angEyeAngles().x = -90.f;
				break;

			default:
				break;
			}
		}
	}

	static int side{};
	DetectSide(player, &side);

	float max_rotation = player->GetMaxBodyRotation();

	// setup a starting brute angle.
	float resolve_value = 50.f;
	static float brute = 0.f;

	float eye_yaw = player->m_PlayerAnimState()->m_flEyeYaw;

	// clamp our starting brute angle, to not brute an angle, we most likely can't hit.
	if (max_rotation < resolve_value)
		resolve_value = max_rotation;

	// detect if player is using maximum desync.
	bool m_extending = record->m_layers[3].m_cycle == 0.f && record->m_layers[3].m_weight == 0.f;

	if (m_extending)
		resolve_value = max_rotation;

	bool forward = fabsf(math::NormalizedAngle(GetAngle(player) - GetForwardYaw(player))) < 90.f;

	// resolve shooting players separately.
	if (record->m_shot)
	{
		float flPseudoFireYaw = math::NormalizedAngle(math::CalculateAngle(record->m_bones[8].GetOrigin(), g_cl.m_local->m_BoneCache().m_pCachedBones[0].GetOrigin()).y);

		if (m_extending) {
			float flLeftFireYawDelta = fabsf(math::NormalizedAngle(flPseudoFireYaw - (record->m_eye_angles.y + 58.f)));
			float flRightFireYawDelta = fabsf(math::NormalizedAngle(flPseudoFireYaw - (record->m_eye_angles.y - 58.f)));

			//g_notify.add( tfm::format( XOR( "found shot record on %s: [ yaw: %i ]" ), game::GetPlayerName( record->m_player->index( ) ), int( flLeftFireYawDelta > flRightFireYawDelta ? -58.f : 58.f ) ) );

			brute = flLeftFireYawDelta > flRightFireYawDelta ? -58.f : 58.f;
		}
		else {
			float flLeftFireYawDelta = fabsf(math::NormalizedAngle(flPseudoFireYaw - (record->m_eye_angles.y + 29.f)));
			float flRightFireYawDelta = fabsf(math::NormalizedAngle(flPseudoFireYaw - (record->m_eye_angles.y - 29.f)));

			//g_notify.add( tfm::format( XOR( "found shot record on %s: [ yaw: %i ]" ), game::GetPlayerName( record->m_player->index( ) ), int( flLeftFireYawDelta > flRightFireYawDelta ? -29.f : 29.f ) ) );

			brute = flLeftFireYawDelta > flRightFireYawDelta ? -29.f : 29.f;
		}
	}
	// bruteforce player accordingly.
	else {
		float resolve_yaw = resolve_value;
		if (shots == 0) {
			brute = resolve_yaw * (forward ? -side : side);
		}
		else {
			switch (data->m_missed_shots % 3) {
			case 0:
				brute = resolve_yaw * (forward ? -side : side);
				break;
			case 1:
				brute = resolve_yaw * (forward ? side : -side);
				break;
			case 2:
				brute = 0;
				break;
			}
		}
	}

	player->m_PlayerAnimState()->m_flGoalFeetYaw = eye_yaw + brute;

	// normalize the eye angles, doesn't really matter but its clean.
	math::NormalizeAngle(player->m_PlayerAnimState()->m_flGoalFeetYaw);
	math::NormalizeAngle(record->m_eye_angles.y);

	record->m_player->m_PlayerAnimState()->m_flGoalFeetYaw = player->m_PlayerAnimState()->m_flGoalFeetYaw;
}

void Resolver::ResolvePoses(Player* player, LagRecord* record) {
	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	// only do this bs when in air.
	if (record->m_mode == Modes::RESOLVE_AIR) {
		// ang = pose min + pose val x ( pose range )

		// lean_yaw
		player->m_flPoseParameter()[2] = g_csgo.RandomInt(0, 4) * 0.25f;

		// body_yaw
		player->m_flPoseParameter()[11] = g_csgo.RandomInt(1, 3) * 0.25f;
	}
}