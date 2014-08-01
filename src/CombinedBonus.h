#pragma once

#include "GameFieldController.h"
#include "GameColor.h"

// Эдакий "менеджер каскада бонусов". На вход получает список бонусов, и согласованно их запускает,
// обеспечивая правильную синхронизацию (одновременно или по цепочке), одновременное осыпание фишек
// по окончании, и то, что каждая клетка в зоне действия бонусов будет убита лишь один раз.
class CombinedBonus : public GameFieldController
{
public:
	typedef std::vector< std::pair<Game::FieldAddress, Game::Hang> > BonusCascade;

	enum TriggerTiming
	{
		INSTANT, // все бонусы взрываются сразу же
		RANDOM,  // бонусы взрываются по очереди с небольшой задержкой
		CHAIN    // бонусы взрываются так, будто детонируют друг от друга по цепочке
	};
private:

	Game::AffectedArea _chips; // все фишки, которые будут убиты
	std::set<int> _fallColumns;
	ColorMask _colors;
	bool _endMove;

	float _totalDuration;

	void Init(BonusCascade &bonuses, TriggerTiming timing);
	void TriggerBonus(const Game::FieldAddress &fa, Game::Hang& hang, float startTime);

	void DestroyChips();
	void RunFallColumns();
public:
	/* instant: true - взрывает все бонусы одновременно,
	         false - по цепочке, начиная с первого, как будто они детонируют друг от друга
	endMove: закончить ли ход после окончания бонусов (true для бонусов с цепочки, false для остальных случаев)*/
	CombinedBonus(Game::FieldAddress cell, Game::Hang bonus, GameField *field, bool endMove);
	CombinedBonus(BonusCascade& bonuses, GameField *field, bool endMove, TriggerTiming timing);
	
	void Update(float dt);
	bool isFinish();
	void Draw();
};