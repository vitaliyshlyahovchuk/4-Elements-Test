#include "stdafx.h"
#include "GameUpdateChecker.h"
#include "Core/SystemDialog.h"
#include "Core/Application.h"
#include "AppCommon.h"

// проверить версию и если надо запустить окошко, предлагающее обновить игру
void GameUpdateChecker::check() {

	std::string currenBundleVersion = GetAppVersion();
	
	Xml::RapidXmlDocument doc("versions.xml");
	rapidxml::xml_node<> *xe = doc.first_node()->first_node();
	bool needUpdate = false; // требуется обновления
	bool mandatory = false; // требуется принудительное обновление
	std::string updateVersion; // версия, до которой нужно обновиться
	while (xe) {
		updateVersion = Xml::GetStringAttribute(xe, "bundleVersion");
		if (updateVersion == currenBundleVersion) {
			// нашли текущую версию
			xe = xe->next_sibling();
			while (xe) {
				needUpdate = true;
				updateVersion = Xml::GetStringAttribute(xe, "bundleVersion");
				mandatory = mandatory || Xml::GetBoolAttributeOrDef(xe, "mandatory", false);
				xe = xe->next_sibling();
			}
			if (needUpdate) {
				// показываем окошко
				SystemDialog alert;
				alert.SetCaption("UpdateGameCaption");
				alert.AddButton("UpdateGameBtnUpdateText", &GameUpdateChecker::updateNowHandler);
				if (!mandatory) {
					alert.AddButton("UpdateGameBtnCancelText", &GameUpdateChecker::updateLaterHandler);
				}
				std::string textId = "UpdateGameVersionDescription" + updateVersion;
				if (!Core::resourceManager.Exists<Render::Text>(textId)) {
					textId = std::string("UpdateGameVersionDescription") + (mandatory ? "Mandatory" : "");
				}
				alert.SetText(textId);
				alert.Show();
			}
			return; // это единственно правильный выход
		}
	}
	Assert2(false, "Not found current bundle version");
}

void GameUpdateChecker::updateNowHandler() {
	utils::OpenPath(Core::GlobalConstants::GetString("AppUrl"));
	//что-то падает, не пойму из-за чего, пробовал и так, и этак
	//Core::appInstance->RunInMainThread(boost::bind(&Core::Application::ShutDown, Core::appInstance));
	//Core::appInstance->ShutDown();
}

void GameUpdateChecker::updateLaterHandler() {
	// ничего не делаем
}
