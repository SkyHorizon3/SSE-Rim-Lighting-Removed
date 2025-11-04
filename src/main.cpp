#include <SimpleIni.h>

bool g_enableDebugLog = false;
bool g_excludeAddonNodeNifs = true;

bool LoadINI()
{
	const auto path = std::format("Data/SKSE/Plugins/{}.ini", Plugin::NAME);
	if (!std::filesystem::exists(path))
		return false;

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(path.c_str());

	constexpr const char* section = "Settings";
	g_enableDebugLog = ini.GetBoolValue(section, "bEnableDebugLog");
	g_excludeAddonNodeNifs = ini.GetBoolValue(section, "bExcludeAddonNodeNifs");
	return true;
}

struct BSLightingShader_SetupGeometry
{
	static bool hasAddonNode(const RE::BSGeometry* obj)
	{
		if (!obj || !g_excludeAddonNodeNifs)
			return false;

		const RE::BSFadeNode* fadeNode = nullptr;
		RE::NiNode* parent = obj->parent;

		while (parent && !fadeNode)
		{
			fadeNode = parent->AsFadeNode();
			parent = parent->parent;
		}

		if (fadeNode)
		{
			if (const auto extraData = fadeNode->GetExtraData<RE::BSXFlags>("BSX"))
			{
				constexpr auto mask = static_cast<std::int32_t>(RE::BSXFlags::Flag::kAddon);
				return (extraData->value & mask) != 0;
			}
		}

		return false;
	}

	static void writeSuccessLog(const RE::BSGeometry* obj)
	{
		const auto data = obj ? obj->GetUserData() : nullptr;
		if (!data)
			return;

		const auto base = data->GetBaseObject();
		const std::string baseMessage = base ? std::format("{:08X} - {}", base->GetFormID(), RE::FormTypeToString(base->GetFormType())) : "Unknown";
		SKSE::log::debug("Removed Rim Lighting on {:08X} - {} with base object {}", data->GetFormID(), RE::FormTypeToString(data->GetFormType()), baseMessage);
	}

	static void thunk(RE::BSShader* shader, RE::BSRenderPass* pass, std::uint32_t flags)
	{
		func(shader, pass, flags);

		const auto prop = pass ? pass->shaderProperty : nullptr;

		if (prop && prop->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kRimLighting) && !hasAddonNode(pass->geometry))
		{
			prop->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kRimLighting, false);

			if (g_enableDebugLog)
			{
				writeSuccessLog(pass->geometry);
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
