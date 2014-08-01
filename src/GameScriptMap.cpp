#include "stdafx.h"
#include "MyApplication.h"
#include "GameInfo.h"
#include "GameScriptMap.h"
#include "GameFieldWidget.h"
#include "ActCounter.h"
#include "GameFillBonus.h"
#include "Match3Loot.h"
#include "Match3Gadgets.h"
#include "card/CardWidget.h"
#include "DebugSelectLevel.h"
#include "TimeFactorWidget.h"
#include "LevelInfoManager.h"
#include "GameField.h"
#include "BlurScreen.h"
#include "Tutorial.h"
#include "CheckBoxEx.h"
#include "BoostersTransitionWidget.h"
#include "Flash/events/FlashEvents.h"
#include "Flash/bindings/FlashEngineBinding.h"
#include "TutorialFlashObjects.h"
#include "LiveCounter.h"
#include "FlashUserAvatar.h"
#include "FriendInfo.h"
#include "LoadScreenWidget.h"
#include "Splash.h"
#include "MousePosWidget.h"
#include "Core/SystemDialog.h"
#include "SwrveManager.h"

#if defined(ENGINE_TARGET_IPHONE) || defined(ENGINE_TARGET_ANDROID)
	#include "GCInterface.h"
	#include "MicroTransactions.h"
	#include "DownloadPackage.h"
	#include "ParseManager.h"
	#include "FBInterface.h"
#endif

#ifdef ENGINE_TARGET_WIN32
	#include "RyushkaSelect.h"
	#include "EditorUtils.h"
	#include "EditorWidget.h"
	#include "EditorPanel.h"
	#include "Combobox.h"
	#include "EditorFileList.h"
	#include "ClickMessageEdit.h"
	#include "EditBoxEx.h"
	#include "ScrollableSelectorWidget.h"
#endif

#if defined(ENGINE_TARGET_IPHONE) || defined(ENGINE_TARGET_ANDROID)
	void ShowHelp();
	void PostAppRequests(const std::string& friendUids, const std::string& helpType);
	void appMakePurchase(const std::string& purchaseId);
	void OpenURL(const std::string& url);
	void OpenFacebookPage();
	void InviteFriends();
	void ReportIssue();
	std::string GetDeviceLanguage();
#endif

namespace GameLua 
{
	Core::Timer timer;

	void StartTimer() {
		timer.Start();
	}

	void GetTime(std::string message) {
		float time = timer.getElapsedTime();
		Log::log.WriteMessage(Log::Severity::Info,message + utils::lexical_cast(time));
	}

	bool isWindows() {
#ifdef ENGINE_TARGET_WIN32
		return true;
#else
		return false;
#endif
	}

	int GetContentWidth() {
		return MyApplication::GAME_WIDTH;
	}

	int GetContentHeight() {
		return MyApplication::GAME_HEIGHT;
	}

	void ShowOkSystemDialog(const std::string& captionId, const std::string& textId, const std::string& okTextId) {
#ifndef ENGINE_TARGET_WIN32
		SystemDialog dlg;
		dlg.SetCaption(captionId);
		dlg.SetText(textId);
		dlg.AddButton(okTextId, NULL);
		dlg.Show();
#else
		Log::log.WriteMessage(Log::Severity::Info, "Emulation 'OkSystemDialog' : " + captionId + ", " + textId + ", " + okTextId);
#endif
	}
	
	// цена покупки - нам отдает apple
	std::string ProductPrice(const std::string& id) {
#ifdef ENGINE_TARGET_WIN32
		std::map<std::string, std::string> shopPrice;
		shopPrice["p1"] = "RUB 66";
		shopPrice["p2"] = "RUB 329";
		shopPrice["p3"] = "RUB 649";
		shopPrice["p4"] = "RUB 1,690";
		shopPrice["p5"] = "RUB 3,290";
		Assert(shopPrice.find(id) != shopPrice.end());
		return shopPrice[id];
#else
		return InApp::ProductPrice(Core::GlobalConstants::GetString("purchase_prefix")+ id);
#endif
	}
	// сколько кэша покупается покупкой id
	std::string ProductCash(const std::string& id) {
		return utils::lexical_cast(gameInfo.prices[id].cash);
	}

	bool CanMakePurchase() {
#if defined(ENGINE_TARGET_WIN32) || defined(ENTERPRISE)
		return true;
#else
		return InApp::CanPurchase();
#endif
	}
	
	void MakePurchase(const std::string& id) {
#if defined(ENGINE_TARGET_WIN32) || defined(ENTERPRISE) || !defined(APPSTORE)
		// сразу же покупаем
		if (gameInfo.IsDevMode()) {
			gameInfo.buy(id);
			Core::LuaCallVoidFunction("onPurchaseComplete", id);
		} else {
			ShowOkSystemDialog("@Error", "@Purchases are unavailable", "@Ok");
			Core::LuaCallVoidFunction("onPurchaseFail", id);
		}
#else
		appMakePurchase(id);
#endif
	}

	void OpenFacebookGamePage() {
#ifdef ENGINE_TARGET_WIN32
		gameInfo.addCash(gameInfo.getConstInt("FBLikeAward"), false);
		Core::LuaDoString("marketingActions:disableLikeGameAction()");
#else
		gameInfo.setLocalBool("tryLikeGamePage", true);
		OpenFacebookPage();
#endif
	}

	bool FBIsLoggedIn() {
#ifdef ENGINE_TARGET_WIN32
		return false;
#else
		return FB::IsLoggedIn();
#endif
	}

	void gameInfoSetProperty(GameInfo& info, const std::string &name, const std::string &value) {
		info.setProperty(name, value);
	}

	int countFriends(GameInfo& info) {
		return static_cast<int>(info.friends.size());
	}

	FriendInfo* getFriendByIndex(GameInfo& info, int index) {
		Assert(index >= 0 && index < static_cast<int>(info.friends.size()));
		return info.friends[index];
	}

	IFlashDisplayObject* getAvatar(GameInfo& info) {
		return new FlashUserAvatar(info.getAvatar());
	}

	int FriendLevelScore(FriendInfo* info, int level) {
		Assert(level >= 0);
		return level >= (int)info->level_scores.size() ? 0: info->level_scores[level];
	}

	std::string GetLanguage() {
		return Core::locale.GetLanguage();
	}

	void SetLanguage(const std::string language) {
		Core::locale.UseMui(language);
		// используем setGlobalConstant только для того, чтобы сбросить кэш текстов
		// до того момента, пока в FreeType нет специальной функции
		FreeType::setGlobalConstant("selected_language", language);
	}

	// DataStoreWithRapid default-function
	bool DataStoreWithRapid_getBool(DataStoreWithRapid* store, const std::string& varName) {
		return store->getBool(varName);
	}
	int DataStoreWithRapid_getInt(DataStoreWithRapid* store, const std::string& varName) {
		return store->getInt(varName);
	}
	float DataStoreWithRapid_getFloat(DataStoreWithRapid* store, const std::string& varName) {
		return store->getFloat(varName);
	}
	double DataStoreWithRapid_getDouble(DataStoreWithRapid* store, const std::string& varName) {
		return store->getDouble(varName);
	}
	std::string DataStoreWithRapid_getString(DataStoreWithRapid* store, const std::string& varName) {
		return store->getString(varName);
	}
	IPoint DataStoreWithRapid_getPoint(DataStoreWithRapid* store, const std::string& varName) {
		return store->getPoint(varName);
	}
	// GameInfo defalut-function
	int GameInfo_getConstInt(GameInfo* info, const std::string &name) {
		return info->getConstInt(name);
	}
	bool GameInfo_getConstBool(GameInfo* info, const std::string &name) {
		return info->getConstBool(name);
	}
	std::string GameInfo_getConstString(GameInfo* info, const std::string &name) {
		return info->getConstString(name);
	}
	float GameInfo_getConstFloat(GameInfo* info, const std::string &name) {
		return info->getConstFloat(name);
	}
	int GameInfo_getLocalInt(GameInfo* info, const std::string &name) {
		return info->getLocalInt(name);
	}
	bool GameInfo_getLocalBool(GameInfo* info, const std::string &name) {
		return info->getLocalBool(name);
	}
	std::string GameInfo_getLocalString(GameInfo* info, const std::string &name) {
		return info->getLocalString(name);
	}
	float GameInfo_getLocalFloat(GameInfo* info, const std::string &name) {
		return info->getLocalFloat(name);
	}
	double GameInfo_getLocalDouble(GameInfo* info, const std::string &name) {
		return info->getLocalDouble(name);
	}

	void spendCash(GameInfo* info, int value, const luabind::adl::object& callback, const std::string& purchaseId) {
		Assert(luabind::type(callback) == LUA_TFUNCTION );
		info->spendCash(value, callback, purchaseId);
	}

#if defined(ENGINE_TARGET_IPHONE) || defined(ENGINE_TARGET_ANDROID)
	void FBLogIn() {
		Core::LuaCallVoidFunction("startFacebookLogin");
		FB::LogIn();
	}

	void FBLogOut() {
		Core::LuaCallVoidFunction("startFacebookLogout");
		FB::LogOut();
	}

	void ShowLogoutFacebookDialog() {
		SystemDialog dlg;
		dlg.SetCaption("MessageFacebookDoYouWantLogout");
		dlg.SetText("TextFacebookDoYouWantLogout");
		dlg.AddButton("BtnOk", FBLogOut);
		dlg.AddButton("BtnCancel", NULL);
		dlg.Show();
	}
#endif
	// освобождение памяти, занятой флешем, для тестирования
	void ReleaseMemory() {
		Flash::FlashResourceManager::instance->releaseMemory(true);
	}

	// эти функция нужна только для тестовой проверки валидности текстов.
	int GetAllTextIds(const luabind::adl::object& luaTable) {
		std::vector<Resource*> allTexts;
		Core::resourceManager.CollectResources<Render::Text>(allTexts);
		int index = 1;
		for (std::vector<Resource*>::iterator i = allTexts.begin(), end = allTexts.end(); i != end; ++i) {
			luaTable[utils::lexical_cast(index++)] = std::string((*i)->GetName());
		}
		return index;
	}

	std::string GetSourceText(const std::string& id) {
		Render::Text* text = Core::resourceManager.Find<Render::Text>(id);
		if (text) {
			return text->GetSource();
		}
		return "|" + id + "|";
	}

	IPlaybackOperation* getPlayOnceOperation() {
		class PlayOncePlaybackOperation : public IPlaybackOperation {
		public:
			virtual bool onAnimationCompleted(IFlashMovieClip* movieClip) {
				movieClip->setPlayback(false);
				return false;
			}
			virtual void onMovieDisposed(IFlashMovieClip* movieClip) {}
		};
		static PlayOncePlaybackOperation operation;
		return &operation;
	}

	bool inGateway() {
		return Core::guiManager.getLayer("CardLayer")->getWidget("CardWidget")->QueryState(Message("inGateway")).getIntegerParam() == 1;
	}

	std::string GetLevelObjectiveText() {
		return GameField::Get()->GetLevelObjectiveText();
	}

	int GetMovesToFinish() {
		return GameField::Get()->_fuuuTester.GetMovesToFinish();
	}
	std::string GetAvgPossibleMoves() {
		return Game::CountAvgPossibleMoves();
	}
	int GetMoveCount() {
		return Match3GUI::ActCounter::GetCounter() - Match3GUI::ActCounter::GetHiddenCount();
	}
	float GetGameTime() {
		return Match3GUI::TimeCounter::GetTime() - Match3GUI::TimeCounter::GetHiddenTime();
	}
	void AddUsedBoost(std::string bonusName) //добавить в список бонус используемый до уровня
	{
		GameField::Get()->boostList.AddUsedBoost(bonusName);
	}
	void UseBoostInstantly(std::string bonusName) //использовать бонус (вывызвается из lua во время игры)
	{
		GameField::Get()->boostList.UseBoostInstantly(bonusName);
	}
//	bool NeedConfirmationForBoost(std::string bonusName) //требуется ли подтверждение использования буста
//	{
//		return GameField::Get()->boostList.NeedConfirmationForBoost(bonusName);
//	}	
	bool CanStartBoostNow(std::string bonusName) //можно ли запустить этот буст сейчас (например нельзя пока камера едет)
	{
		return GameField::Get()->boostList.CanStartBoostNow(bonusName);
	}

	// playback операция для событий внутри анимации старта/конца уравня
	class AnimationPlaybackOperation : public IPlaybackOperation {
	public:
		virtual void onFrameConstructed(IFlashMovieClip* movieClip, int frame){
			PlaybackEvent event(PlaybackFrame, frame);
			EventManager::Get().dispatchTarget(movieClip->DisplayObject(), event);
		}
		virtual bool onAnimationCompleted(IFlashMovieClip* movieClip) {
			PlaybackEvent event(PlaybackEnd, movieClip->getCurrentFrame());
			EventManager::Get().dispatchTarget(movieClip->DisplayObject(), event);
			movieClip->setPlayback(false);
			return false;
		}
		// не позволяем убивать себя, т.к. будем использовать один объект
		virtual void onMovieDisposed(IFlashMovieClip* movieClip) {
		}
	};
	IPlaybackOperation* getAnimationPlaybackOperation() {
		static AnimationPlaybackOperation operation;
		return &operation;
	}

	BlurScreen* CreateBlurScreen() {
		return new BlurScreen();
	}

	EmptyBlurScreen* CreateEmptyScreen() {
		return new EmptyBlurScreen();
	}

	IPoint GetCurrentBoostPoint() {
		IPoint point = IPoint(0, 0);
		if (!gameInfo.currentBoostTutorial.points.empty()) {
			point = gameInfo.currentBoostTutorial.points[0];
		}
		return point;
	}

	TutorialFlashObjects::TutorialFlashAreaHighlighter* CreateAreaHighlighter(luabind::object const& LuaRects, float opacity) {
		return new TutorialFlashObjects::TutorialFlashAreaHighlighter(LuaRects, opacity);
	}

	TutorialFlashObjects::TutorialFlashArrow* CreateArrow(luabind::object const& LuaPoints) {
		return new TutorialFlashObjects::TutorialFlashArrow(LuaPoints);
	}
}

void gamePreInit(rapidxml::xml_node<>* root) {
	
#ifdef ENGINE_TARGET_WIN32
	Core::locale.SetCodepage(Xml::GetIntAttributeOrDef(root, "codepage", -1));
	gameInfo.setGlobalBool("SoftCypher", true);
#endif

	gameInfo.setGlobalBool("only_game", Xml::GetBoolAttributeOrDef(root, "only_game", false));
	gameInfo.setGlobalBool("only_card", Xml::GetBoolAttributeOrDef(root, "only_card", false));
	gameInfo.setGlobalBool("game_debug_cursor", Xml::GetBoolAttributeOrDef(root, "game_debug_cursor", false));
	gameInfo.setGlobalBool("check_texts", Xml::GetBoolAttributeOrDef(root, "check_texts", false));

	GameSettings::InitGameSettings(root);

	Flash::pushSeparateEffect(true);
}

void gameScriptMap() {
	using namespace luabind;

#if defined(ENGINE_TARGET_IPHONE) || defined(ENGINE_TARGET_ANDROID)
	module(Core::luaState)
		[
			def("ShowHelp", &ShowHelp)
			,def("FacebookLogin", &GameLua::FBLogIn)
			,def("FacebookLogout", &GameLua::FBLogOut)
			,def("FacebookDeleteAppRequest", &FB::DeleteAppRequest)
			,def("FacebookInviteFriends", &InviteFriends)
			,def("PostAppRequests", &PostAppRequests)
			,def("ReportIssue", &ReportIssue)
			,def("ReinitStore", &InApp::ReinitStore)
			,def("GetDeviceLanguage", &GetDeviceLanguage)
			,def("ParseSave", &ParseManager::Save)
			,def("ShowLogoutFacebookDialog", &GameLua::ShowLogoutFacebookDialog)
		];
#endif
#ifdef ENGINE_TARGET_WIN32
	module(Core::luaState)
		[
			class_<EditorUtils::EditorLua>("Editor")
				.property("lastLoadedLevel", &EditorUtils::EditorLua::Get_lastLoadedLevel, &EditorUtils::EditorLua::Set_lastLoadedLevel)
				.property("activeEditBtn", &EditorUtils::EditorLua::Get_activeEditBtn, &EditorUtils::EditorLua::Set_activeEditBtn)
				.property("moveToolSet", &EditorUtils::EditorLua::Get_moveToolSet, &EditorUtils::EditorLua::Set_moveToolSet)
				.def("isOn", &EditorUtils::EditorLua::isOn)
				.def("OnPushEditorButton", &EditorUtils::EditorLua::OnPushEditorButton)

		];
	globals(Core::luaState)["Editor"] = &EditorUtils::editor_lua;
	
#endif

	module(Core::luaState)
		[
			class_<LiveCounter>("LiveCounter")
				.def("getLiveAmount", &LiveCounter::getLiveAmount)
				.def("getTimeToNextLive", &LiveCounter::getTimeToNextLive)
				.def("spendLive", &LiveCounter::spendLive)
				.def("fillLives", &LiveCounter::fillLives)
				.def("addLive", &LiveCounter::addLive)
				.def("getMaxGenerateLive", &LiveCounter::getMaxGenerateLive)
				.def("notifications", &LiveCounter::notifications)
				.def("notificationsEnable", &LiveCounter::notificationsEnable)
			,
			class_<DataStore>("DataStore")
				.def("getBool", &DataStore::getBool)
				.def("getInt", &DataStore::getInt)
				.def("getFloat", &DataStore::getFloat)
				.def("getDouble", &DataStore::getDouble)
				.def("getString", &DataStore::getString)
				.def("getPoint", &DataStore::getPoint)
				.def("setBool", &DataStore::setBool)
				.def("setInt", &DataStore::setInt)
				.def("setFloat", &DataStore::setFloat)
				.def("setDouble", &DataStore::setDouble)
				.def("setString", &DataStore::setString)
				.def("setPoint", &DataStore::setPoint)
			,
			class_<DataStoreWithRapid>("DataStoreWithRapid")
				.def("getBool", &DataStoreWithRapid::getBool)
				.def("getBool", &GameLua::DataStoreWithRapid_getBool)
				.def("getInt", &DataStoreWithRapid::getInt)
				.def("getInt", &GameLua::DataStoreWithRapid_getInt)
				.def("getFloat", &DataStoreWithRapid::getFloat)
				.def("getFloat", &GameLua::DataStoreWithRapid_getFloat)
				.def("getDouble", &DataStoreWithRapid::getDouble)
				.def("getDouble", &GameLua::DataStoreWithRapid_getDouble)
				.def("getString", &DataStoreWithRapid::getString)
				.def("getString", &GameLua::DataStoreWithRapid_getString)
				.def("getPoint", &DataStoreWithRapid::getPoint)
				.def("getPoint", &GameLua::DataStoreWithRapid_getPoint)

				.def("setBool", &DataStoreWithRapid::setBool)
				.def("setInt", &DataStoreWithRapid::setInt)
				.def("setFloat", &DataStoreWithRapid::setFloat)
				.def("setDouble", &DataStoreWithRapid::setDouble)
				.def("setString", &DataStoreWithRapid::setString)
				.def("setPoint", &DataStoreWithRapid::setPoint)
			,
			class_<FriendInfo>("FriendInfo")
				.def(constructor<>()) // нужен для дебажного добавления
				.def_readwrite("fb_id", &FriendInfo::fb_id)
				.def_readonly("name", &FriendInfo::name)
				.def_readonly("current_marker", &FriendInfo::current_marker)
				.def_readonly("extreme_level", &FriendInfo::extreme_level)
				.def("getLevelScore", &GameLua::FriendLevelScore)
				.def("getAvatar", &FriendInfo::getFlashAvatar)
			,
			class_<GameInfo>("GameInfo")
				.def("setMusicVolume", &GameInfo::setMusicVolume)
				.def("getMusicVolume", &GameInfo::getMusicVolume)
				.def("setSoundVolume", &GameInfo::setSoundVolume)
				.def("getSoundVolume", &GameInfo::getSoundVolume)

				.def("getLocalInt", &GameInfo::getLocalInt)
				.def("getLocalInt", &GameLua::GameInfo_getLocalInt)
				.def("getLocalBool", &GameInfo::getLocalBool)
				.def("getLocalBool", &GameLua::GameInfo_getLocalBool)
				.def("getLocalString", &GameInfo::getLocalString)
				.def("getLocalString", &GameLua::GameInfo_getLocalString)
				.def("getLocalFloat", &GameInfo::getLocalFloat)
				.def("getLocalFloat", &GameLua::GameInfo_getLocalFloat)
				.def("getLocalDouble", &GameInfo::getLocalDouble)
				.def("getLocalDouble", &GameLua::GameInfo_getLocalDouble)

				.def("setLocalString", &GameInfo::setLocalString)
				.def("setLocalInt", &GameInfo::setLocalInt)
				.def("setLocalBool", &GameInfo::setLocalBool)
				.def("setLocalFloat", &GameInfo::setLocalFloat)
				.def("setLocalDouble", &GameInfo::setLocalDouble)
			
				.def("getGlobalInt", &GameInfo::getGlobalInt)
				.def("getGlobalBool", &GameInfo::getGlobalBool)
				.def("getGlobalString", &GameInfo::getGlobalString)
				.def("getGlobalFloat", &GameInfo::getGlobalFloat)

				.def("setGlobalString", &GameInfo::setGlobalString)
				.def("setGlobalInt", &GameInfo::setGlobalInt)
				.def("setGlobalBool", &GameInfo::setGlobalBool)
				.def("setGlobalFloat", &GameInfo::setGlobalFloat)

				.def("getConstInt", &GameInfo::getConstInt)
				.def("getConstInt", &GameLua::GameInfo_getConstInt)
				.def("getConstBool", &GameInfo::getConstBool)
				.def("getConstBool", &GameLua::GameInfo_getConstBool)
				.def("getConstString", &GameInfo::getConstString)
				.def("getConstString", &GameLua::GameInfo_getConstString)
				.def("getConstFloat", &GameInfo::getConstFloat)
				.def("getConstFloat", &GameLua::GameInfo_getConstFloat)

				.def("findName", &GameInfo::localNameDefined)

				.def("getProperty", &GameInfo::getProperty)
				.def("setProperty", &GameLua::gameInfoSetProperty)
				.def("IsDevMode", &GameInfo::IsDevMode)
				.def("getLiveCounter", &GameInfo::getLiveCounter)
				.def("getFriendByUid", &GameInfo::getFriendByUid)
				.def("countFriends", &GameLua::countFriends)
				.def("getFriendByIndex", &GameLua::getFriendByIndex)
				.def("getAvatar", &GameLua::getAvatar)
				.def("getGlobalTime", &GameInfo::getGlobalTime)
				.def("getCash", &GameInfo::getCash)
				.def("spendCash", &GameLua::spendCash)
				.def("Save", &GameInfo::Save)

				.def("needBoostTutorial", &GameInfo::needBoostTutorial)
				.def("getBoostTutorialName", &GameInfo::getBoostTutorialName)
				.def("setBoostTutorialShown", &GameInfo::setBoostTutorialShown)
				.def("allowBoostTutorial", &GameInfo::allowBoostTutorial)
				.def("ResetShownBoostTutorials", &GameInfo::ResetShownBoostTutorials)
			,
			class_<LevelInfoManager>("LevelInfoManager")
				.def("setCurrentLevel", &LevelInfoManager::setCurrentLevel)
				.def("getLevelStars", &LevelInfoManager::getLevelStars)
				.def("getLevelScore", &LevelInfoManager::getLevelScore)
				.def("isTutorialLevel", &LevelInfoManager::isTutorialLevel)
				.def("isLevelStarted", &LevelInfoManager::isLevelStarted)
				.def("getCurrentLevelData" , &LevelInfoManager::getCurrentLevelData)
				.def("getStarsForScore", &LevelInfoManager::getStarsForScore)
				.def("getScoreForStar", &LevelInfoManager::getScoreForStar)
				.def("getCurrentLevelGoal", &LevelInfoManager::getCurrentLevelGoal)
			,
			class_<IPlaybackOperation>("IPlaybackOperation")
			,
			class_<PlaybackEvent, FlashEvent>("PlaybackEvent")
				.property("frame", &PlaybackEvent::getFrame)
			,class_<BlurScreen, IFlashDisplayObject>("BlurScreen")
				.def("shoot", &BlurScreen::shoot)
				.def("release", &BlurScreen::release)
				.def("addEffect", &BlurScreen::addEffect)
			,def("CreateBlurScreen", GameLua::CreateBlurScreen)

			,class_<TutorialFlashObjects::TutorialFlashAreaHighlighter, IFlashDisplayObject>("FlashAreaHighlighter")
				.def("hide", &TutorialFlashObjects::TutorialFlashAreaHighlighter::Hide)
				.def("release", &TutorialFlashObjects::TutorialFlashAreaHighlighter::release)
				.def("setshape", &TutorialFlashObjects::TutorialFlashAreaHighlighter::SetShape)
			,def("CreateAreaHighlighter", GameLua::CreateAreaHighlighter)

			,class_<EmptyBlurScreen, IFlashDisplayObject>("EmptyBlurScreen")
				.def("release", &EmptyBlurScreen::release)
			,def("CreateEmptyScreen", GameLua::CreateEmptyScreen)

			,class_<TutorialFlashObjects::TutorialFlashArrow, IFlashDisplayObject>("FlashArrow")
			,def("CreateArrow", GameLua::CreateArrow)

			,class_<Tutorial::LuaTutorial>("LuaTutorial")
				.def("SetThread", &Tutorial::LuaTutorial::SetThread)
				.def("ShowDirection", &Tutorial::LuaTutorial::ShowDirection)
				.def("HideDirection", &Tutorial::LuaTutorial::HideDirection)
				.def("ShowArrow", &Tutorial::LuaTutorial::ShowArrow)
				.def("HideArrow", &Tutorial::LuaTutorial::HideArrow)
				.def("ShowMove", &Tutorial::LuaTutorial::ShowMove)
				.def("HideMove", &Tutorial::LuaTutorial::HideMove)
				.def("ShowHighlight", &Tutorial::LuaTutorial::ShowHighlight)
				.def("HideHighlight", &Tutorial::LuaTutorial::HideHighlight)
				.def("PauseChipFall", &Tutorial::LuaTutorial::PauseChipFall)
				.def("UnpauseChipFall", &Tutorial::LuaTutorial::UnpauseChipFall)
				.def("SetEnergySpeed", &Tutorial::LuaTutorial::SetEnergySpeed)
				.def("SetHangBonusCells", &Tutorial::LuaTutorial::SetHangBonusCells)
				.def("SetHangBonusTypes", &Tutorial::LuaTutorial::SetHangBonusTypes)
				.def("StartReceivers", &Tutorial::LuaTutorial::StartReceivers)
				.def("ShowTapText", &Tutorial::LuaTutorial::ShowTapText)
				.def("HideTapText", &Tutorial::LuaTutorial::HideTapText)
				.def("BlockField", &Tutorial::LuaTutorial::BlockField)
				.def("UnblockField", &Tutorial::LuaTutorial::UnblockField)
				.def("WaitForGameStart", &Tutorial::LuaTutorial::WaitForGameStart)
				.def("WaitForMove", &Tutorial::LuaTutorial::WaitForMove)
				.def("WaitForTap", &Tutorial::LuaTutorial::WaitForTap)
				.def("WaitForReceiver", &Tutorial::LuaTutorial::WaitForReceiver)
				.def("WaitForEnergy", &Tutorial::LuaTutorial::WaitForEnergy)
				.def("WaitForBoostSelect", &Tutorial::LuaTutorial::WaitForBoostSelect)
				.def("WaitForChipSelect", &Tutorial::LuaTutorial::WaitForChipSelect)
				.def("WaitForRunBoost", &Tutorial::LuaTutorial::WaitForRunBoost)
				.def("WaitForFreeSeq", &Tutorial::LuaTutorial::WaitForFreeSeq)
				.def("IsPaused", &Tutorial::LuaTutorial::IsPaused)
				.def("SetTutorialHighlightChain", &Tutorial::LuaTutorial::SetTutorialHighlightChain)

			,def("StartTimer", &GameLua::StartTimer)
			,def("GetTime", &GameLua::GetTime)
			,def("IsWindows", &GameLua::isWindows)
			,def("GetContentWidth", &GameLua::GetContentWidth)
			,def("GetContentHeight", &GameLua::GetContentHeight)
			,def("ShowOkSystemDialog", &GameLua::ShowOkSystemDialog)
			,def("ProductPrice", &GameLua::ProductPrice)
			,def("ProductCash", &GameLua::ProductCash)
			,def("CanMakePurchase", &GameLua::CanMakePurchase)
			,def("MakePurchase", &GameLua::MakePurchase)
			,def("OpenFacebookGamePage", &GameLua::OpenFacebookGamePage)
			,def("FacebookIsLoggedIn", &GameLua::FBIsLoggedIn)
			,def("GetLanguage", &GameLua::GetLanguage)
			,def("SetLanguage", &GameLua::SetLanguage)
			,def("GetSourceText", &GameLua::GetSourceText)
			,def("ReleaseMemory", &GameLua::ReleaseMemory)
			,def("Utf8_Length", &Utf8_Length)
			,def("Utf8_Substr", &Utf8_Substr)
			,def("IsOtherAudioPlaying", &MyApplication::IsOtherAudioPlaying)
			,def("SwrveTrackEvent", &SwrveManager::TrackEvent)
			,def("GetAllTextIds", &GameLua::GetAllTextIds)
			,def("inGateway", &GameLua::inGateway)
			,def("GetLevelObjectiveText", GameLua::GetLevelObjectiveText)
			,def("GetGameTime", &GameLua::GetGameTime)
			,def("GetMoveCount", &GameLua::GetMoveCount)
			,def("GetGameScore", &Match3GUI::LootPanel::GetScore)
			,def("GetMovesToFinish", &GameLua::GetMovesToFinish)
			,def("GetAvgPossibleMoves", &GameLua::GetAvgPossibleMoves)
			,def("AddUsedBoost", &GameLua::AddUsedBoost)
			,def("UseBoostInstantly", &GameLua::UseBoostInstantly)
			,def("CanStartBoostNow", &GameLua::CanStartBoostNow)
//			,def("NeedConfirmationForBoost", &GameLua::NeedConfirmationForBoost)
			,def("GetCurrentBoostPoint", &GameLua::GetCurrentBoostPoint)
			,def("getAnimationPlaybackOperation", &GameLua::getAnimationPlaybackOperation)
			,def("getTextUnsnapOperation", &Flash::getTextUnsnapOperation)
		];

	globals(Core::luaState)["gameInfo"] = &gameInfo;
	globals(Core::luaState)["levelsInfo"] = &levelsInfo;
	globals(Core::luaState)["levelSettings"] = &Gadgets::levelSettings;
	globals(Core::luaState)["tutorial"] = &Tutorial::luaTutorial;
}

void gameRegisterTypes() {

#ifdef ENGINE_TARGET_WIN32
	REGISTER_WIDGET_XML(RyushkaSelect, "RyushkaSelect");
	REGISTER_WIDGET_XML(EditorFileList, "EditorFileList");
	REGISTER_WIDGET_XML(ChipSelecter, "ChipSelecter");
	REGISTER_WIDGET_XML(EditorUtils::EditorWidget, "EditorUtils_Editor");
	REGISTER_WIDGET_XML(EditorUtils::EditorPanel, "EditorUtils_EditorPanel");
	REGISTER_WIDGET_XML(Combobox, "Combobox");
	REGISTER_WIDGET_XML(ClickMessageEdit, "ClickMessageEdit");
	REGISTER_WIDGET_XML(OrderConfigWidget, "OrderConfig");
	REGISTER_WIDGET_XML(ScrollableSelectorWidget, "ScrollableSelectorWidget");
	REGISTER_WIDGET_XML(GUI::EditBoxEx, "EditBoxEx");
#endif

	REGISTER_WIDGET_XML(LoadScreenWidget, "LoadScreenWidget");
	REGISTER_WIDGET_XML(Splash, "Splash");
	REGISTER_WIDGET_XML(MousePosWidget, "MousePosWidget");
	REGISTER_WIDGET_XML(GameFieldWidget, "GameField");
	REGISTER_WIDGET_XML(GameFieldUp, "GameFieldUp");
	REGISTER_WIDGET_XML(Card::CardWidget, "CardWidget");
	REGISTER_WIDGET_XML(DebugSelectLevel, "DebugSelectLevel");
	REGISTER_WIDGET_XML(GUI::CheckBoxEx, "CheckBoxEx");
	REGISTER_WIDGET_XML(BoostersTransitionWidget, "BoostersTransitionWidget");
	REGISTER_WIDGET_XML(GUI::TimeFactorWidget, "TimeFactorWidget");
}

void SimpleCypher(char * data, size_t size)
{
	const static std::string cypher =
		"mqgViZNzuJ1c6acRNYnkiBw4f7OVO4djIl0oqa4teegXJ9i9l70IelftYbC0SpTrZDhEohlYe9TKdYsvjIn4QELzn4IOQcy8rgKI3dcb"
		"vGGsNw2FAAlidmCssWBf2PP9qehZMqnp0yruIvyPNQyOBpcu4ec6HlLEZdAKhgyvH2eJu8fjcEZy0wTCsLCfcNEdECUfSapml3taslea"
		"NDQ8X03QnFxXQxQYnvWasApQBNMIGGEb0NmtZMDN1OMPmSVQ0v8ad69egEv7So4YkgdGMf5HQGX4SatZy8AENOD2X2aXAgILn7aVv1g4"
		"Wq4UdILyquhTDbl5zSFK8R8l5bC7raRbj4SgxpPs8p8S06BjKJnqYeXtw5qiNR6b2W9diU6yXYCJaowJ8q1a4Snqf9U1JVL3B1rm6wgh"
		"5Ojp5YQCXEcp7Y3MEN4Z1gjSO3sHXtLWQqEhNpd1Ef5WdZc2NZ7Yf3WNnBMC96WHjDenloKOu6XvYL7VGYUwz1euVHWzuzzNfJLZmb8O";
	size_t csz = cypher.size();
	size_t k = 0;
	for (size_t i = 0; i < size; ++i, ++k) {
		if (k >= csz) { k = 0; }
		data[i] ^= cypher[k];
	}
}