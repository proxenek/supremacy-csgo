#include "includes.h"
////////////////////////////////////////////////
// https://github.com/proxenek/supremacy-csgo //
////////////////////////////////////////////////
// S U P R E M A C Y /////
// C S : G O C H E A T //
/////////////////////////

CSGO	   g_csgo{};;
WinAPI	   g_winapi{};;
Netvars	   g_netvars{};;
EntOffsets g_entoffsets{};;
Menu       g_menu{};;
Notify     g_notify{};;

bool CSGO::init( ) {
	m_done = false;

	if( m_done )
		return false;

	// wait for the game to init.
	// "serverbrowser.dll" is the last module to be loaded.
	// if it gets loaded we can be ensured that the entire game done loading.

#ifndef KOLO
	while( !m_serverbrowser_dll ) {
		m_serverbrowser_dll = PE::GetModule( HASH( "serverbrowser.dll" ) );
		if( !m_serverbrowser_dll )
			std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
	}
#endif

	// grab some modules.
	m_kernel32_dll = PE::GetModule( HASH( "kernel32.dll" ) );
	m_user32_dll   = PE::GetModule( HASH( "user32.dll" ) );
	m_shell32_dll  = PE::GetModule( HASH( "shell32.dll" ) );
	m_shlwapi_dll  = PE::GetModule( HASH( "shlwapi.dll" ) );
	m_client_dll   = PE::GetModule( HASH( "client.dll" ) );
	m_engine_dll   = PE::GetModule( HASH( "engine.dll" ) );
	m_vstdlib_dll  = PE::GetModule( HASH( "vstdlib.dll" ) );
	m_tier0_dll    = PE::GetModule( HASH( "tier0.dll" ) );

	// import winapi functions.
	g_winapi.WideCharToMultiByte = PE::GetExport( m_kernel32_dll, HASH( "WideCharToMultiByte" ) ).as< WinAPI::WideCharToMultiByte_t >( );
	g_winapi.MultiByteToWideChar = PE::GetExport( m_kernel32_dll, HASH( "MultiByteToWideChar" ) ).as< WinAPI::MultiByteToWideChar_t >( );
	g_winapi.GetTickCount        = PE::GetExport( m_kernel32_dll, HASH( "GetTickCount" ) ).as< WinAPI::GetTickCount_t >( );
	g_winapi.VirtualProtect      = PE::GetExport( m_kernel32_dll, HASH( "VirtualProtect" ) ).as< WinAPI::VirtualProtect_t >( );
	g_winapi.VirtualQuery        = PE::GetExport( m_kernel32_dll, HASH( "VirtualQuery" ) ).as< WinAPI::VirtualQuery_t >( );
	g_winapi.CreateDirectoryA    = PE::GetExport( m_kernel32_dll, HASH( "CreateDirectoryA" ) ).as< WinAPI::CreateDirectoryA_t >( );
	g_winapi.SetWindowLongA      = PE::GetExport( m_user32_dll, HASH( "SetWindowLongA" ) ).as< WinAPI::SetWindowLongA_t >( );
	g_winapi.CallWindowProcA     = PE::GetExport( m_user32_dll, HASH( "CallWindowProcA" ) ).as< WinAPI::CallWindowProcA_t >( );
	g_winapi.SHGetFolderPathA    = PE::GetExport( m_shell32_dll, HASH( "SHGetFolderPathA" ) ).as< WinAPI::SHGetFolderPathA_t >( );
	g_winapi.PathAppendA         = PE::GetExport( m_shlwapi_dll, HASH( "PathAppendA" ) ).as< WinAPI::PathAppendA_t >( );

	// run interface collection code.
	Interfaces interfaces{};

	// get interface pointers.
	m_client             = interfaces.get< CHLClient* >( HASH( "VClient" ) );
	m_cvar               = interfaces.get< ICvar* >( HASH( "VEngineCvar" ) );
	m_engine             = interfaces.get< IVEngineClient* >( HASH( "VEngineClient" ) );
	m_entlist            = interfaces.get< IClientEntityList* >( HASH( "VClientEntityList" ) );
	m_input_system       = interfaces.get< IInputSystem* >( HASH( "InputSystemVersion" ) );
	m_surface            = interfaces.get< ISurface* >( HASH( "VGUI_Surface" ) );
	m_panel              = interfaces.get< IPanel* >( HASH( "VGUI_Panel" ) );
	m_engine_vgui        = interfaces.get< IEngineVGui* >( HASH( "VEngineVGui" ) );
	m_prediction         = interfaces.get< CPrediction* >( HASH( "VClientPrediction" ) );
	m_engine_trace       = interfaces.get< IEngineTrace* >( HASH( "EngineTraceClient" ) );
	m_game_movement      = interfaces.get< CGameMovement* >( HASH( "GameMovement" ) );
	m_render_view        = interfaces.get< IVRenderView* >( HASH( "VEngineRenderView" ) );
	m_model_render       = interfaces.get< IVModelRender* >( HASH( "VEngineModel" ) );
	m_material_system    = interfaces.get< IMaterialSystem* >( HASH( "VMaterialSystem" ) );
	m_studio_render      = interfaces.get< CStudioRenderContext* >( HASH( "VStudioRender" ) );
	m_model_info         = interfaces.get< IVModelInfo* >( HASH( "VModelInfoClient" ) );
	m_debug_overlay      = interfaces.get< IVDebugOverlay* >( HASH( "VDebugOverlay" ) );
	m_phys_props         = interfaces.get< IPhysicsSurfaceProps* >( HASH( "VPhysicsSurfaceProps" ) );
	m_game_events        = interfaces.get< IGameEventManager2* >( HASH( "GAMEEVENTSMANAGER" ), 1 );
	m_match_framework    = interfaces.get< CMatchFramework* >( HASH( "MATCHFRAMEWORK_" ) );
	m_localize           = interfaces.get< ILocalize* >( HASH( "Localize_" ) );
    m_networkstringtable = interfaces.get< INetworkStringTableContainer* >( HASH( "VEngineClientStringTable" ) );
	m_sound              = interfaces.get< IEngineSound* >( HASH( "IEngineSoundClient" ) );

	// convars.
	clear                                   = m_cvar->FindVar( HASH( "clear" ) );
	toggleconsole                           = m_cvar->FindVar( HASH( "toggleconsole" ) );
	name                                    = m_cvar->FindVar( HASH( "name" ) );
	sv_maxunlag                             = m_cvar->FindVar( HASH( "sv_maxunlag" ) );
	sv_gravity                              = m_cvar->FindVar( HASH( "sv_gravity" ) );
	sv_jump_impulse                         = m_cvar->FindVar( HASH( "sv_jump_impulse" ) );
	sv_enablebunnyhopping                   = m_cvar->FindVar( HASH( "sv_enablebunnyhopping" ) );
	sv_airaccelerate                        = m_cvar->FindVar( HASH( "sv_airaccelerate" ) );
	sv_friction                             = m_cvar->FindVar( HASH( "sv_friction" ) );
	sv_stopspeed                            = m_cvar->FindVar( HASH( "sv_stopspeed" ) );
	cl_interp                               = m_cvar->FindVar( HASH( "cl_interp" ) );
	cl_interp_ratio                         = m_cvar->FindVar( HASH( "cl_interp_ratio" ) );
	cl_updaterate                           = m_cvar->FindVar( HASH( "cl_updaterate" ) );
	cl_cmdrate                              = m_cvar->FindVar( HASH( "cl_cmdrate" ) );
	cl_lagcompensation                      = m_cvar->FindVar( HASH( "cl_lagcompensation" ) );
	mp_teammates_are_enemies                = m_cvar->FindVar( HASH( "mp_teammates_are_enemies" ) );
	weapon_debug_spread_show                = m_cvar->FindVar( HASH( "weapon_debug_spread_show" ) );
	molotov_throw_detonate_time             = m_cvar->FindVar( HASH( "molotov_throw_detonate_time" ) );
	weapon_molotov_maxdetonateslope         = m_cvar->FindVar( HASH( "weapon_molotov_maxdetonateslope" ) );
	weapon_recoil_scale                     = m_cvar->FindVar( HASH( "weapon_recoil_scale" ) );
    view_recoil_tracking                    = m_cvar->FindVar( HASH( "view_recoil_tracking" ) );
	cl_fullupdate                           = m_cvar->FindVar( HASH( "cl_fullupdate" ) );
	r_DrawSpecificStaticProp                = m_cvar->FindVar( HASH( "r_DrawSpecificStaticProp" ) );
	cl_crosshair_sniper_width               = m_cvar->FindVar( HASH( "cl_crosshair_sniper_width" ) );
	hud_scaling                             = m_cvar->FindVar( HASH( "hud_scaling" ) );
    sv_clip_penetration_traces_to_players   = m_cvar->FindVar( HASH( "sv_clip_penetration_traces_to_players" ) );
    weapon_accuracy_shotgun_spread_patterns = m_cvar->FindVar( HASH( "weapon_accuracy_shotgun_spread_patterns" ) );
	net_showfragments                       = m_cvar->FindVar( HASH( "net_showfragments" ) );

	// hehe xd.
	name->m_callbacks.RemoveAll( );

	// classes by sig.
	m_move_helper		= pattern::find(m_client_dll, XOR("8B 0D ? ? ? ? 8B 46 08 68")).add(2).get< IMoveHelper* >(2);
	m_cl				= pattern::find(m_engine_dll, XOR("B9 ? ? ? ? 56 FF 50 14 8B 34 85")).add(1).get< CGlobalState* >()->m_client_state;
	m_shadow_mgr		= pattern::find(m_client_dll, XOR("A1 ? ? ? ? FF 90 ? ? ? ? 6A 00")).add(1).get().as< IClientShadowMgr* >();
	m_view_render		= pattern::find(m_client_dll, XOR("8B 0D ? ? ? ? 57 8B 01 FF 50 14 E8 ? ? ? ? 5F")).add(2).get< CViewRender* >(2);
	m_hookable_cl		= reinterpret_cast<void*>(*reinterpret_cast<uintptr_t**>(reinterpret_cast<uintptr_t>(m_cl) + 0x8));
	m_gamerules			= pattern::find(m_client_dll, XOR("A1 ? ? ? ? 8B 0D ? ? ? ? 6A 00 68 ? ? ? ? C6")).add(0x1).get< C_CSGameRules* >();
	m_beams				= pattern::find(m_client_dll, XOR("8D 43 FC B9 ? ? ? ? 50 A1")).add(0x4).get< IViewRenderBeams* >();
	m_mem_alloc			= PE::GetExport(m_tier0_dll, HASH("g_pMemAlloc")).get< IMemAlloc* >();
	m_glow				= *pattern::find(m_client_dll, XOR("0F 11 05 ? ? ? ? 83 C8 01")).add(0x3).as< CGlowObjectManager** >();

	// classes by offset from virtual.
	m_globals		= util::get_method(m_client, CHLClient::INIT).add(0x1f).get< CGlobalVarsBase* >(2);
	m_client_mode	= util::get_method(m_client, CHLClient::HUDPROCESSINPUT).add(0x5).get< IClientMode* >(2);
	m_input			= util::get_method(m_client, CHLClient::INACTIVATEMOUSE).at< CInput* >(0x1);

	// functions.
	MD5_PseudoRandom				= pattern::find(m_client_dll, XOR("55 8B EC 83 E4 F8 83 EC 70 6A")).as< MD5_PseudoRandom_t >();;
	SetAbsAngles					= pattern::find(m_client_dll, XOR("55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8 ?"));
	InvalidateBoneCache				= pattern::find(m_client_dll, XOR("80 3D ? ? ? ? ? 74 16 A1 ? ? ? ? 48 C7 81"));
	LockStudioHdr					= pattern::find(m_client_dll, XOR("55 8B EC 51 53 8B D9 56 57 8D B3 54 29 00 00 ? ? ? ? ? ?"));
	SetAbsOrigin					= pattern::find(m_client_dll, XOR("55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8 ? ?"));
	IsBreakableEntity				= pattern::find(m_client_dll, XOR("55 8B EC 51 56 8B F1 85 F6 74 68")).as< IsBreakableEntity_t >();
	SetAbsVelocity					= pattern::find(m_client_dll, XOR("55 8B EC 83 E4 F8 83 EC 0C 53 56 57 8B 7D 08 8B F1"));
	AngleMatrix						= pattern::find(m_client_dll, XOR("E8 ? ? ? ? 8B 07 89 46 0C")).rel32(0x1).as< AngleMatrix_t >();
	ComputeHitboxSurroundingBox		= pattern::find(m_client_dll, XOR("55 8B EC 83 E4 F8 81 EC 24 04 00 00 ? ? ? ? ? ?"));
	GetSequenceActivity				= pattern::find(m_client_dll, XOR("55 8B EC 53 8B 5D 08 56 8B F1 83"));
	LoadFromBuffer					= pattern::find(m_client_dll, XOR("E8 ? ? ? ? 88 44 24 0F 8B 56 FC")).rel32(0x1).as< LoadFromBuffer_t >();
	HasC4							= pattern::find(m_client_dll, XOR("56 8B F1 85 F6 74 31"));
	InvalidatePhysicsRecursive		= pattern::find(m_client_dll, XOR("55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56"));
	GetEconItemView					= pattern::find(m_client_dll, XOR("8B 81 ? ? ? ? 81 C1 ? ? ? ? FF 50 04 83 C0 40 C3")).as< GetEconItemView_t >();
	GetStaticData					= pattern::find(m_client_dll, XOR("55 8B EC 51 53 8B D9 8B 0D ? ? ? ? 56 57 8B B9")).as< GetStaticData_t >();
	SmokeCount						= pattern::find(m_client_dll, XOR("A3 ? ? ? ? 57 8B CB")).add(0x1).to();
	BeamAlloc						= pattern::find(m_client_dll, XOR("E8 ? ? ? ? 8B F0 85 F6 74 7C")).rel32< BeamAlloc_t >(0x1);
	SetupBeam						= pattern::find(m_client_dll, XOR("E8 ? ? ? ? 8B 07 33 C9")).rel32< SetupBeam_t >(0x1);
	AddListenerEntity				= pattern::find(m_client_dll, XOR("55 8B EC 8B 0D ? ? ? ? 33 C0 56 85 C9 7E 32 8B 55 08 8B 35")).as< AddListenerEntity_t >();
	GetShotgunSpread				= pattern::find(m_client_dll, XOR("E8 ? ? ? ? EB 38 83 EC 08")).rel32< GetShotgunSpread_t >(1);
	BoneAccessor					= pattern::find(m_client_dll, XOR("8D 81 ? ? ? ? 50 8D 84 24")).add(2).to< size_t >();
	AnimOverlay						= pattern::find(m_client_dll, XOR("8B 80 ? ? ? ? 8D 34 C8")).add(2).to< size_t >();
	studioHdr						= pattern::find(m_client_dll, XOR("8B 86 ? ? ? ? 89 44 24 10 85 C0")).add(2).to< size_t >();
	UTIL_TraceLine					= pattern::find(m_client_dll, XOR("55 8B EC 83 E4 F0 83 EC 7C 56 52"));
	CTraceFilterSimple_vmt			= UTIL_TraceLine.add(0x3D).to();
	LastBoneSetupTime				= InvalidateBoneCache.add(0x11).to< size_t >();
	MostRecentModelBoneCounter		= InvalidateBoneCache.add(0x1B).to< size_t >();

	// exported functions.
	RandomSeed	= PE::GetExport(m_vstdlib_dll, HASH("RandomSeed")).as< RandomSeed_t >();
	RandomInt	= PE::GetExport(m_vstdlib_dll, HASH("RandomInt")).as< RandomInt_t >();
	RandomFloat = PE::GetExport(m_vstdlib_dll, HASH("RandomFloat")).as< RandomFloat_t >();

	// prediction pointers.
	m_nPredictionRandomSeed = util::get_method(m_prediction, CPrediction::RUNCOMMAND).add(0x30).get< int* >();
	m_pPredictionPlayer = util::get_method(m_prediction, CPrediction::RUNCOMMAND).add(0x54).get< Player* >();

	// some weird tier0 stuff that prevents me from debugging properly...
#ifdef _DEBUG
	Address debugbreak = pattern::find( g_csgo.m_client_dll, XOR( "CC F3 0F 10 4D ? 0F 57 C0" ) );

	DWORD old;
	g_winapi.VirtualProtect( debugbreak, 1, PAGE_EXECUTE_READWRITE, &old );

	debugbreak.set< uint8_t >( 0x90 );

	g_winapi.VirtualProtect( debugbreak, 1, old, &old );
#endif

    // init everything else.
	g_config.init( );

    // g_netvars stores all netvar offsets into an unordered_map, EntOffsets is for the raw offset values so we don't have to access the unordered_map a bunch.
	g_netvars.init( );
    g_entoffsets.init( );

	g_listener.init( );
	render::init( );
	g_menu.init( );
	g_config.LoadHotkeys( );
	g_chams.init( );
	g_hooks.init( );

    // if we injected and we're ingame, run map load func.
	if( m_engine->IsInGame( ) ) {
		g_cl.OnMapload( );
		g_csgo.cl_fullupdate->m_callback( );
	}
	
	m_done = true;
	return true;
}

bool game::IsBreakable(Entity* ent) {
	bool        ret;
	ClientClass* cc;
	const char* name;
	char* takedmg, old_takedmg;

	static size_t m_takedamage_offset{ *(size_t*)((uintptr_t)g_csgo.IsBreakableEntity + 0x26) };

	// skip null ents and the world ent.
	if (!ent || ent->index() == 0)
		return false;

	// get m_takedamage and save old m_takedamage.
	takedmg = (char*)((uintptr_t)ent + m_takedamage_offset);
	old_takedmg = *takedmg;

	// get clientclass.
	cc = ent->GetClientClass();

	if (cc) {
		// get clientclass network name.
		name = cc->m_pNetworkName;

		// CBreakableSurface, CBaseDoor, ...
		if (name[1] != 'F'
			|| name[4] != 'c'
			|| name[5] != 'B'
			|| name[9] != 'h') {
			*takedmg = DAMAGE_YES;
		}
	}

	ret = g_csgo.IsBreakableEntity(ent);
	*takedmg = old_takedmg;

	return ret;
}

void game::UTIL_ClipTraceToPlayers(const vec3_t& start, const vec3_t& end, uint32_t mask, ITraceFilter* filter, CGameTrace* tr, float range) {
	static auto func = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? 83 C4 14 8A 56 37")).rel32< uintptr_t >(0x1);
	if (!func)
		return;

	__asm {
		mov  ecx, start
		mov	 edx, end
		push range
		push tr
		push filter
		push mask
		call func
		add	 esp, 16
	}
}