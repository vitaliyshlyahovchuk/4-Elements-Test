#include "stdafx.h"
#include "LevelEnd.h"
#include "EditorUtils.h"
#include "GameField.h"
#include "Match3Gadgets.h"
#include "ActCounter.h"
#include "Match3Loot.h"
#include "SwrveManager.h"
#include "Match3.h"
#include "LevelInfoManager.h"
#include "PlayerStatistic.h"
#include "Flash/bindings/FlashAnimation.h"
#include "LevelEndEffects.h"
#include "MyApplication.h"
#include "EnergyReceivers.h"
#include "GameInfo.h"

namespace LevelEnd
{

std::string NumberToStr(int num, int digits = 3)
{
	std::string str = utils::lexical_cast(num);
	Assert(static_cast<int>(str.length()) <= digits);
	std::string zeros(digits - str.length(), '0');
	return zeros + str;
}

CompleteLevelProcess::CompleteLevelProcess()
{
	InitLevel();
}

CompleteLevelProcess::~CompleteLevelProcess()
{
}

void CompleteLevelProcess::InitLevel()
{
	_taskManager.Clear();
	_loseLife = false;
	_loseReason = NONE;
	_levelComplete = false;
	_additionalMovesBought = 0;
}

void CompleteLevelProcess::SetLoseLifeReason(LoseLifeReason reason)
{
	if(_loseReason == NO_MOVES || _loseReason == TIME_BOMB || _loseReason == MONSTER_EAT_RECEIVER )
		return;
	_loseReason = reason;
}

bool CompleteLevelProcess::NeedRunLoseLife() const
{
	return _loseReason != NONE;
}
	
void CompleteLevelProcess::RunLoseLife()
{
	Log::Info("[Match3] Run Lose Life");
	GameField::Get()->BlockField();

	_taskManager.Clear();
	_taskManager.QueueTask( boost::make_shared<DelayTask>(1.5f) );
	_taskManager.QueueTask( boost::make_shared<ShowLevelFailTask>(_loseReason) );
	_taskManager.RunNextTask();

	_loseLife = true;

	if (gameInfo.getConstBool("CollectStatistics"))
	{
		if( playerStatistic.GetValue("ActiveReceivers").empty() )
			playerStatistic.SetValue("ActiveReceivers", utils::lexical_cast(Gadgets::receivers.ActiveCount()));
	}
	//считаем остававшиеся ходы до финиша
	if(_loseReason == OUT_OF_MOVES || _loseReason == OUT_OF_TIME)
	{
		GameField::Get()->_fuuuTester.CountMovesToFinish(2,5);
	}
}

void CompleteLevelProcess::ContinuePlaying()
{
	std::string gameType = Gadgets::levelSettings.getString("LevelType");
	if (gameType == "moves") {
		Match3GUI::ActCounter::SetCounter(gameInfo.getConstInt("LevelFailAdd_" + gameType));
		_additionalMovesBought++;
	} else {
		Match3GUI::TimeCounter::SetTime(static_cast<float>(gameInfo.getConstInt("LevelFailAdd" + gameType)));
	}
	GameField::Get()->UnblockField();
	_loseLife = false;
	_loseReason = NONE;
}

void CompleteLevelProcess::LoseLife()
{
	std::string string_reason;
	switch (_loseReason) {
		case NO_MOVES: string_reason = "NoMoves"; break;
		case MONSTER_EAT_RECEIVER: string_reason = "Thief"; break;
		default:
			string_reason = "EndMoves";
	}
	SwrveManager::TrackEvent("Level.FailedLevel", 
		"{\"Level\":" + utils::lexical_cast(gameInfo.getLocalInt("current_level")) + 
		",\"Reason\":\"" + string_reason + "\"}");

	if (gameInfo.getLocalInt("extreme_level") == gameInfo.getLocalInt("current_level")) {
		SwrveManager::TrackEvent("Level.Failed.level_" + NumberToStr(gameInfo.getLocalInt("current_level",0)), 
			"{\"AdditionalMoves\":" + utils::lexical_cast(_additionalMovesBought) +
			//(check_maybe_future ? (",\"NeedMatch\":" + utils::lexical_cast(GameField::Get()->_fuuuTester.GetMovesToFinish())): "") +
			",\"ActivCristall\":" + utils::lexical_cast(Gadgets::receivers.ActiveCount()) + 
			",\"BoostsCount\":" + utils::lexical_cast(gameInfo.getLocalInt("used_boosts_count")) +
			",\"Reason\":\"" + string_reason + "\""+
			"}");
		gameInfo.setLocalInt("attempt_win_level", gameInfo.getLocalInt("attempt_win_level") + 1);
	}

	if (gameInfo.getConstBool("CollectStatistics"))
	{
		playerStatistic.SetValue("Result", "Fail");
		//считаем среднее число возможных ходов
		playerStatistic.SetValue("AvgPossibleMoves", Game::CountAvgPossibleMoves());
		//так как проигрыш - считаем сколько ходов не хватило до победы
		int movesToFinish = GameField::Get()->_fuuuTester.GetMovesToFinish();
		playerStatistic.SetValue("NeededMovesWhenLose", utils::lexical_cast(movesToFinish));
		

		playerStatistic.EndCollect();
	}
}
	
void CompleteLevelProcess::Run(bool debug_short)
{
	gameInfo.getGlobalBool("GameSaved", false);

	GameField::Get()->ClearChipSeq(false);
		
	int currentLevel = gameInfo.getLocalInt("current_level", 0);

	Core::LuaCallVoidFunction("gameInterfaceBlockPauseButton", true);

	_taskManager.Clear();
	_taskManager.QueueTask( boost::make_shared<ActivateEnergyRecTask>() );
	_taskManager.QueueTask( boost::make_shared<DelayTask>(1.0f) );
	_taskManager.QueueTask( boost::make_shared<ShowAnimationLevelComplete>() );
	if(!debug_short)
	{
		_taskManager.QueueTask( boost::make_shared<TriggerBonusesTask>(true) );
		if( Match3GUI::ActCounter::GetCounter() < Match3GUI::ActCounter::INFINITE_MOVES )
		{
			_taskManager.QueueTask( boost::make_shared<HangBonusesTask>(0.5f) );
		}
	}
	_taskManager.QueueTask( boost::make_shared<DelayTask>(6.0f) );
	_taskManager.QueueTask( boost::make_shared<CrossTask>() );
	_taskManager.RunNextTask();

	IRect r = Game::visibleRect;
	Match3::SetRect(r.x, r.x + r.width - 1, r.y, r.y + r.height - 1, true);

	_loseReason = NONE;
	_levelComplete = true;

	SwrveManager::TrackEvent("Level.WinLevel", "{\"Level\":" + utils::lexical_cast(gameInfo.getLocalInt("current_level")) + "}");
	if (gameInfo.getLocalInt("extreme_level") == gameInfo.getLocalInt("current_level")) {
		SwrveManager::TrackEvent("Level.Win.level_" + NumberToStr(gameInfo.getLocalInt("current_level",0)), 
			"{\"Attempt\":" + utils::lexical_cast(gameInfo.getLocalInt("attempt_win_level") + 1) +
			",\"AdditionalMoves\":" + utils::lexical_cast(_additionalMovesBought) +
			",\"MovesLeft\":" + utils::lexical_cast(_additionalMovesBought > 0 ? 0 : Match3GUI::ActCounter::GetCounter()) + 
			",\"BoostsCount\":" + utils::lexical_cast(gameInfo.getLocalInt("used_boosts_count")) +
			",\"Scores\":" + utils::lexical_cast(Match3GUI::LootPanel::GetScore()) +
			"}");
		gameInfo.setLocalInt("attempt_win_level", 0); // сбрасываем количество попыток пройти уровень
	}

	if (gameInfo.getConstBool("CollectStatistics"))
	{
		if( playerStatistic.GetValue("ActiveReceivers").empty() )
			playerStatistic.SetValue("ActiveReceivers", utils::lexical_cast(Gadgets::receivers.ActiveCount()));

		playerStatistic.SetValue("Result", "Win");
		//считаем среднее число возможных ходов
		playerStatistic.SetValue("AvgPossibleMoves", Game::CountAvgPossibleMoves());

		playerStatistic.EndCollect();
	}
}

void CompleteLevelProcess::Update(float dt)
{
	_taskManager.Update(dt);
}

bool CompleteLevelProcess::AcceptMessage(const Message &message)
{
	if(message.is("ContinuePlaying"))
	{
		ContinuePlaying();
		return true;
	}
	else if(message.is("LoseLife"))
	{
		LoseLife();
		return true;
	}
	return false;
}

bool CompleteLevelProcess::MouseDown(const IPoint &mouse_pos)
{
	return _taskManager.MouseDown(mouse_pos);
}

void CompleteLevelProcess::Draw()
{
	_taskManager.Draw();
}

bool CompleteLevelProcess::IsRunning() const
{
	return _levelComplete || _loseLife;
}

ActivateEnergyRecTask::ActivateEnergyRecTask()
	: GameTask(1.0f)
{
}

void ActivateEnergyRecTask::Run()
{
	MM::manager.PlaySample("LevelComplete");
	MM::manager.FadeOutTrack(1);
	controller->RunNextTask();
}

bool ActivateEnergyRecTask::isFinished()
{
	return true;
}

CrossTask::CrossTask()
	: GameTask(1.0f)
{
}

void CrossTask::Run()
{
	Core::LuaCallVoidFunction("gameInterfaceBlockPauseButton", false);

	int extreme_level = gameInfo.getLocalInt("extreme_level", 0);
	int score = Match3GUI::LootPanel::GetScore();

	if(Core::guiManager.isLayerLoaded("CardLayer")) {
		if( extreme_level > 0 )
			Core::LuaCallVoidFunction("ShowGameStat", score);
	} else {
		Core::messageManager.putMessage(Message("lua:RunLevel"));
	}

	levelsInfo.setLevelScore(score);

	controller->RunNextTask();

	if( extreme_level == 0 )
		Core::messageManager.putMessage(Message("lua:StartNextLevel"));
}

bool CrossTask::isFinished()
{
	return true;
}

TriggerBonusesTask::TriggerBonusesTask(bool wait_standby)
	: GameTask(1.f)
	, _wait_standby(wait_standby)
{
	local_time = -1.0f;
}

void TriggerBonusesTask::Update(float dt)
{
	if(local_time >= 0.0f && local_time < 1.0f)
	{
		local_time += time_scale * dt;
	}
}

void TriggerBonusesTask::Run()
{
	local_time = 0.0f;
	GameField::Get()->TriggerBonusesOnScreen();

	//int moves = Match3GUI::ActCounter::GetCounter();
	//if( moves > 0 && moves < Match3GUI::ActCounter::INFINITE_MOVES ) {
	//	controller->QueueNextTask( boost::make_shared<HangBonusesTask>(1.0f) );
	//}
}

bool TriggerBonusesTask::isFinished()
{
	if( local_time >= 1.0f && (!_wait_standby || GameField::Get()->IsStandby()) ) {
		controller->RunNextTask();
		return true;
	}
	return false;
}

HangBonusesTask::HangBonusesTask(float duration)
	: GameTask(1.0f / duration)
{
	local_time = -1.0f;
}

void HangBonusesTask::Update(float dt)
{
	if(local_time >= 0.0f && local_time < 1.0f)
	{
		local_time += time_scale * dt;
		if( local_time >= 1.0f) {
			controller->RunNextTask();
		}
	}
}

void HangBonusesTask::Run()
{
	local_time = 0.0f;
	if(GameField::Get()->HangBonusesFromMoves(1) > 0) {
		//controller->QueueNextTask( boost::make_shared<TriggerBonusesTask>(false) );
		controller->QueueNextTask( boost::make_shared<HangBonusesTask>(1.0f / time_scale) );
	}
}

bool HangBonusesTask::isFinished()
{
	return (local_time >= 1.0f);
}

ShowAnimationLevelComplete::ShowAnimationLevelComplete()
	: GameTask(1.f)
	, animation(new FlashAnimation("complete_anim", "complete_anim", 30))
	, _isFinished(false)
{
	animation->GetMovieClip()->setPlaybackOperation(GameLua::getPlayOnceOperation());
}

ShowAnimationLevelComplete::~ShowAnimationLevelComplete()
{
	delete animation;
}

void ShowAnimationLevelComplete::Update(float dt)
{
	animation->Update(dt);
	if (animation->IsLastFrame())
	{
		controller->RunNextTask();
		_isFinished = true;
	}
}

void ShowAnimationLevelComplete::Draw()
{
	animation->Draw(Render::device.Width()*0.5f, Render::device.Height()*0.5f);
}

void ShowAnimationLevelComplete::Run()
{
//	int i = 1;
}

bool ShowAnimationLevelComplete::isFinished()
{
	return _isFinished;
}


ShowBonusMovesEffect::ShowBonusMovesEffect()
	: GameTask(1.0f)
{
	local_time = -1.0f;
}

void ShowBonusMovesEffect::Run()
{
	local_time = 1.0f;
	int moves = Match3GUI::ActCounter::GetCounter();
	if( moves > 0 && moves < Match3GUI::ActCounter::INFINITE_MOVES ) {
		Game::AddController(new LevelEndEffect());
	}
	controller->RunNextTask();
}

bool ShowBonusMovesEffect::isFinished()
{
	return (local_time > 0.0f);
}


DelayTask::DelayTask(float time)
	: GameTask(1.0f / time)
{
	local_time = -1.0f;
}

void DelayTask::Update(float dt)
{
	if( local_time >= 0.0f )
	{
		if( local_time < 1.0f) {
			local_time += time_scale * dt;
			if( local_time >= 1.0f )
				controller->RunNextTask();
		}
	}
}

void DelayTask::Run()
{
	local_time = 0.0f;
}

bool DelayTask::isFinished()
{
	return (local_time >= 1.0f);
}

ShowLevelFailTask::ShowLevelFailTask(LoseLifeReason reason)
	: GameTask(1.0f)
	, _reason(reason)
{
}

void ShowLevelFailTask::Run()
{
	Log::Info("[Match3] Show level fail");
	if(Core::guiManager.isLayerLoaded("CardLayer")) {
		Core::LuaCallVoidFunction("showLevelFailed", (int)_reason);
	} else {
		Core::messageManager.putMessage(Message("lua:RunLevel"));
	}
}

bool ShowLevelFailTask::isFinished()
{
	return true;
}

}//namespace LevelEnd