#pragma once

#include "GameFieldController.h"
#include "GameFieldAddress.h"
#include "GameBonus.h"

class GameField;

// полет бонуса из счетчика ходов на поле в конце игры
class LevelEndFlyBonus : public GameFieldController
{
	ParticleEffectPtr _trail;
	ParticleEffectPtr _endEffect;
	Game::Hang _bonus;
	Game::FieldAddress _dest;
	FPoint fromPanel;
	SplinePath<FPoint> _path;
public:
	LevelEndFlyBonus(GameField *gamefield, Game::FieldAddress to, const Game::Hang& bonus, float delay);
	~LevelEndFlyBonus();
	void Update(float dt);
	void DrawAbsolute();
	bool isFinish();
};

class LevelEndBonus : public Game::HangBonus
{
	float _delay;
	int _radius;

	friend class Game::Hang;
public:
	LevelEndBonus(float delay, int radius);
	void GetAffectedChips(Game::FieldAddress from, Game::AffectedArea &chips, float startTime);
	void StartEffect(Game::FieldAddress from, float startTime);

	void DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch) {}
	void DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch) {}
	void Save(Xml::TiXmlElement *xml_elem) {}

	Type GetType() const { return LEVEL_END; }
};

class LevelEndBonusController : public GameFieldController
{
	IPoint _center;
public:
	LevelEndBonusController(IPoint center, float startTime);
	void Update(float dt);
	void Draw();
	bool isFinish();
};

class LevelEndEffect : public GameFieldController
{
	ParticleEffectPtr _effect;
	SplinePath<FPoint> _path;

	TimedSplinePath _pathX, _pathY;
public:
	LevelEndEffect();
	~LevelEndEffect();
	void Update(float dt);
	void Draw();
	bool isFinish();
};