#include "stdafx.h"
#include "AppSettings.h"
#include "ParseManager.h"
#include "GameInfo.h"
#include "AppCommon.h"

void AppSettings::CheckReset()
{
	if (Core::GlobalVars::GetBool("reset_game", false)) {
		Core::GlobalVars::SetBool("reset_game", false);
		Core::GlobalVars::Sync();

		FacebookSilentLogout();
		
		ParseManager::Reset();
		gameInfo.LoadFromFile(true);
	}
}

bool AppSettings::getBool(const std::string name)
{
	return Core::GlobalVars::GetBool(name, false);
}