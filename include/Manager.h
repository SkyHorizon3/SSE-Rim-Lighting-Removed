#pragma once

class Manager
	: public REX::Singleton<Manager>
{
public:
	bool loadINI();
	void onSetupGeometry(RE::BSRenderPass* const pass);
	const bool enableDebugLog() const noexcept { return m_enableDebugLog; }

private:
	//bool hasAddonNode(const RE::BSGeometry* obj);
	void writeSuccessLog(const RE::BSGeometry* const obj);

	bool m_enableDebugLog{ false };
	bool m_excludeBackLighting{ true };
};