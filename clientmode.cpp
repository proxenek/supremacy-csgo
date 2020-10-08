#include "includes.h"

bool Hooks::ShouldDrawParticles( ) {
	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawParticles_t >( IClientMode::SHOULDDRAWPARTICLES )( this );
}

bool Hooks::ShouldDrawFog( ) {
	// remove fog.
	if( g_menu.main.visuals.nofog.get( ) )
		return false;

	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawFog_t >( IClientMode::SHOULDDRAWFOG )( this );
}

void Hooks::OverrideView( CViewSetup* view ) {
	// damn son.
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	// g_grenades.think( );
	g_visuals.ThirdpersonThink( );

	if (g_cl.m_local && g_cl.m_local->alive() && g_input.GetKeyState(g_menu.main.antiaim.fakeduck.get()))
		view->m_origin.z = g_cl.m_local->GetAbsOrigin().z + 64.f;

    // call original.
	g_hooks.m_client_mode.GetOldMethod< OverrideView_t >( IClientMode::OVERRIDEVIEW )( this, view );
}

bool Hooks::CreateMove( float time, CUserCmd* cmd ) {
	Stack   stack;
	bool    ret;

	// let original run first.
	ret = g_hooks.m_client_mode.GetOldMethod< CreateMove_t >( IClientMode::CREATEMOVE )( this, time, cmd );

	// called from CInput::ExtraMouseSample -> return original.
	if( !cmd->m_command_number )
		return ret;

	// if we arrived here, called from -> CInput::CreateMove
	// call EngineClient::SetViewAngles according to what the original returns.
	if( ret )
		g_csgo.m_engine->SetViewAngles( cmd->m_view_angles );

	// random_seed isn't generated in ClientMode::CreateMove yet, we must set generate it ourselves.
	cmd->m_random_seed = g_csgo.MD5_PseudoRandom( cmd->m_command_number ) & 0x7fffffff;

	// get bSendPacket off the stack.
	g_cl.m_packet = stack.next( ).local( 0x1c ).as< bool* >( );

	// get bFinalTick off the stack.
	g_cl.m_final_packet = stack.next( ).local( 0x1b ).as< bool* >( );

	// invoke move function.
	g_cl.OnTick( cmd );

	if (g_hvh.m_should_work) {
		if (*g_cl.m_packet)
			g_cl.m_angle = cmd->m_view_angles;
		else
			g_cl.m_real_angle = cmd->m_view_angles;
	}
	else {
		g_cl.m_angle = g_cl.m_real_angle = cmd->m_view_angles;
	}

	//if (!(*g_cl.m_packet))
	//{
	//	if (g_csgo.m_cl->m_net_channel)
	//	{
	//		const auto current_choke = g_csgo.m_cl->m_net_channel->iChokedPackets;
	//		g_csgo.m_cl->m_net_channel->iChokedPackets = 0;
	//		util::get_method<void(__thiscall*)(void*, int*)>(g_csgo.m_cl->m_net_channel, 46)(g_csgo.m_cl->m_net_channel, 0);
	//		--g_csgo.m_cl->m_net_channel->iOutSequenceNr;
	//		g_csgo.m_cl->m_net_channel->iChokedPackets = current_choke;
	//	}
	//}
	//else
	//	g_cl.m_cmds.push_back(cmd->m_command_number);

	return false;
}

bool Hooks::DoPostScreenSpaceEffects( CViewSetup* setup ) {
	g_visuals.RenderGlow( );

	return g_hooks.m_client_mode.GetOldMethod< DoPostScreenSpaceEffects_t >( IClientMode::DOPOSTSPACESCREENEFFECTS )( this, setup );
}