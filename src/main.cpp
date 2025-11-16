#include "Manager.h"

struct BSLightingShader_SetupGeometry
{
	static void thunk(RE::BSShader* shader, RE::BSRenderPass* pass, std::uint32_t renderFlags)
	{
		func(shader, pass, renderFlags);
		Manager::GetSingleton()->onSetupGeometry(pass);
	}
	static inline REL::Relocation<decltype(thunk)> func;

	static void Install()
	{
		REL::Relocation<std::uintptr_t> Vtbl{ RE::VTABLE_BSLightingShader[0] };
		func = Vtbl.write_vfunc(0x6, &thunk);
	}
};

#define DLLEXPORT __declspec(dllexport)

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []()
	{
		SKSE::PluginVersionData v;
		v.PluginName(Plugin::NAME);
		v.AuthorName("SkyHorizon"sv);
		v.PluginVersion(Plugin::VERSION);
		v.UsesAddressLibrary();
		v.UsesNoStructs();
		return v;
	}
();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	SKSE::Init(skse, true);

	spdlog::set_pattern("[%H:%M:%S:%e] [%l] %v"s);

	const auto manager = Manager::GetSingleton();
	manager->loadINI();

	if (manager->enableDebugLog())
	{
		spdlog::set_level(spdlog::level::trace);
		spdlog::flush_on(spdlog::level::trace);
	}
	else
	{
		spdlog::set_level(spdlog::level::info);
		spdlog::flush_on(spdlog::level::info);
	}

	SKSE::log::info("Game version: {}", skse->RuntimeVersion());

	BSLightingShader_SetupGeometry::Install();

	return true;
}
