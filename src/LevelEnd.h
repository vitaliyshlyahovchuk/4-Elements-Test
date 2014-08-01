#ifndef _LEVEL_END_H_
#define _LEVEL_END_H_

#include "PlayerStatistic.h"
#include "GameTask.h"

class FlashAnimation;

namespace LevelEnd
{

enum LoseLifeReason {
	OUT_OF_TIME  = 0,
	OUT_OF_MOVES = 1,
	NO_MOVES     = 2,
	TIME_BOMB    = 3,
	MONSTER_EAT_RECEIVER = 4,
	NONE = 5
};

class CompleteLevelProcess
{
	bool _loseLife;
	bool _levelComplete;
	LoseLifeReason _loseReason;

	int _additionalMovesBought;

	GameTaskController _taskManager;
public:

	CompleteLevelProcess();
	~CompleteLevelProcess();
	void InitLevel();

	void Run(bool debug_short = false); // запускаем выигрыш уровня, бонусы, эффекты, праздник, радость
	void RunLoseLife();		// показываем диалог о проигрыше уровня с предложением купить ходы/время и продолжить, либо проиграть окончательно
	void ContinuePlaying(); // выбрали продолжить
	void LoseLife();		// выбрали проиграть
	void SetLoseLifeReason(LoseLifeReason reason);
	bool NeedRunLoseLife() const;

	void Update(float dt);
	bool AcceptMessage(const Message &message);		// еcли возвращает true - дальше обрабатывать cообщение не нужно
	bool MouseDown(const IPoint &mouse_pos);
	void Draw();
	bool IsRunning() const;
};

class ActivateEnergyRecTask : public GameTask
{
public:
	ActivateEnergyRecTask();
	void Run();
	bool isFinished();
};

class CrossTask : public GameTask
{
public:
	CrossTask();
	void Run();
	bool isFinished();
};

class TriggerBonusesTask : public GameTask
{
	bool _wait_standby;
public:
	TriggerBonusesTask(bool wait_standby);
	void Update(float dt);
	void Run();
	bool isFinished();
};

class HangBonusesTask : public GameTask
{
public:
	HangBonusesTask(float duration);
	void Update(float dt);
	void Run();
	bool isFinished();
};

class ShowAnimationLevelComplete : public GameTask
{
	FlashAnimation *animation;
	bool _isFinished;
public:
	ShowAnimationLevelComplete();
	~ShowAnimationLevelComplete();
	void Update(float dt);
	void Draw();
	void Run();
	bool isFinished();
};

class ShowBonusMovesEffect : public GameTask
{
public:
	ShowBonusMovesEffect();
	void Run();
	bool isFinished();
};

class DelayTask : public GameTask
{
public:
	DelayTask(float time);
	void Update(float dt);
	void Run();
	bool isFinished();
};

class ShowLevelFailTask : public GameTask
{
	LoseLifeReason _reason;
public:
	ShowLevelFailTask(LoseLifeReason reason);
	void Run();
	bool isFinished();
};

} //namespace LevelEnd

#endif //_LEVEL_END_H_
