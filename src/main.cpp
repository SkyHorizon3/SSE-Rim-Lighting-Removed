#include <SimpleIni.h>

bool g_enableDebugLog = false;

bool LoadINI()
{
	const auto path = std::format("Data/SKSE/Plugins/{}.ini", Plugin::NAME);
	if (!std::filesystem::exists(path))
		return false;

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(path.c_str());

	g_enableDebugLog = ini.GetBoolValue("Debug", "EnableDebugLog");
	return true;
}

struct BSLightingShader_SetupGeometry
{
	static void writeLog(const RE::BSGeometry* obj)
	{
		const auto data = obj ? obj->GetUserData() : nullptr;
		if (!data)
			return;

		const auto base = data->GetBaseObject();
		const std::string baseMessage = base ? std::format("{:08X}", base->GetFormID()) : "Unknown";
		SKSE::log::debug("Removed Rim Lighting on {:08X} with base object {}", data->GetFormID(), baseMessage);
	}

	static void thunk(RE::BSShader* shader, RE::BSRenderPass* pass, std::uint32_t flags)
	{
		func(shader, pass, flags);

		const auto prop = pass ? pass->shaderProperty : nullptr;

		if (prop && prop->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kRimLighting))
		{
			prop->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kRimLighting, false);

			if (g_enableDebugLog)
			{
				writeLog(pass->geometry);
			}
		}
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

	LoadINI();

	if (g_enableDebugLog)
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
