#include "Manager.h"
#include <SimpleIni.h>

bool Manager::loadINI()
{
	const auto path = std::format("Data/SKSE/Plugins/{}.ini", Plugin::NAME);
	if (!std::filesystem::exists(path))
		return false;

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(path.c_str());

	constexpr const char* section = "Settings";
	m_enableDebugLog = ini.GetBoolValue(section, "bEnableDebugLog");
	m_excludeBackLighting = ini.GetBoolValue(section, "bExcludeBackLighting");
	return true;
}

//bool Manager::hasAddonNode(const RE::BSGeometry* obj)
//{
//	if (!obj || !m_excludeAddonNodeNifs)
//		return false;
//
//	const RE::BSFadeNode* fadeNode = nullptr;
//	RE::NiNode* parent = obj->parent;
//
//	while (parent && !fadeNode)
//	{
//		fadeNode = parent->AsFadeNode();
//		parent = parent->parent;
//	}
//
//	if (fadeNode)
//	{
//		if (const auto extraData = fadeNode->GetExtraData<RE::BSXFlags>("BSX"))
//		{
//			constexpr auto mask = static_cast<std::int32_t>(RE::BSXFlags::Flag::kAddon);
//			return (extraData->value & mask) != 0;
//		}
//	}
//
//	return false;
//}

void Manager::writeSuccessLog(const RE::BSGeometry* const obj)
{
	const auto data = obj ? obj->GetUserData() : nullptr;
	if (!data)
		return;

	const auto base = data->GetBaseObject();
	const std::string baseMessage = base ? std::format("{:08X} - {}", base->GetFormID(), RE::FormTypeToString(base->GetFormType())) : "Unknown";
	SKSE::log::debug("Removed Rim Lighting on {:08X} - {} with base object {}", data->GetFormID(), RE::FormTypeToString(data->GetFormType()), baseMessage);
}

void Manager::onSetupGeometry(RE::BSRenderPass* const pass)
{
	const auto prop = pass ? pass->shaderProperty : nullptr;

	if (!prop || !prop->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kRimLighting))
		return;

	if (prop->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kBackLighting))
	{
		if (m_excludeBackLighting)
			return;

		prop->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kBackLighting, false);

	}

	prop->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kRimLighting, false);

	if (m_enableDebugLog)
	{
		writeSuccessLog(pass->geometry);
	}
}