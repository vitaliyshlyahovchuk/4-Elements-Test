#pragma once

#include "GameFieldAddress.h"
#include "GameFieldController.h"
#include "GameColor.h"
#include "StripEffect.h"
#include "CommonContainers.h"
#include "SplineBezier.h"
#include "Flash/bindings/FlashAnimation.h"
#include "Game.h"

namespace Game {
struct Square;
}

class GameField;

class StartFieldMover : public GameFieldController
{
	float _fullLenght;
	bool _started;
	float _timer;
	FPoint pos_prev;
	float last_time, collect_dist;

	std::vector<math::Vector3> _path;


	SplineBezier prev_spline;
	SplinePath<FPoint> _splinePath;
	FPoint _prevGradient;
	float _startValue;
	float _timeFast;

public:
	StartFieldMover();
	void Draw();
	void Update(float dt);
	bool isFinish();
	bool MouseUp(const IPoint &mouse_pos);
	FPoint GetPos(float t);
	FPoint GetPos();
	void Start();
};


class SequenceDestroyer : public GameFieldController
{
	AddressVector _chipSeq;
	AddressVector _chipSeqCopy;
	//StripEffect _strip;
	//float _stripTime;
	//float _stripTimeScale;
	//Render::Texture *_energyTex;
	int _lengthBonus;
	bool _boom;

	bool _scoreAdded;
	int _chipNum;
    
	float _pauseBeforeFalling;
	float _pauseTime;
	bool _isOnPauseBeforeFalling;
	bool _isAnythingDestroyed;

	Game::Hang _seqHang;
public:

	SequenceDestroyer(const AddressVector& chipSeq_, GameField *gamefield_);
	~SequenceDestroyer();

	void Init();

	void Update(float dt);
	
	virtual void Draw();
	virtual bool isFinish();
};


class FieldMover : public GameFieldController
{
	math::Vector3 _lastDest;

	struct Path
	{
		math::Vector3 key1, key2;
		float t;
		float t_scale;

		Path(math::Vector3 from, math::Vector3 to, const float &time_scale)
			: key1(from)
			, key2(to)
			, t(0.f)
		{
			t_scale = time_scale;
		};

		math::Vector3 getValue()
		{
			float t1 = math::clamp(0.0f, 1.0f, 0.5f-0.5f*cosf(math::PI*t));
			return math::lerp(key1, key2, t1);
		}
		math::Vector3 getValue(float add_time)
		{
			float tt = math::clamp(0.f, 1.f, t + add_time*t_scale);
			float t1 = math::clamp(0.0f, 1.0f, 0.5f-0.5f*cosf(math::PI*tt));
			return math::lerp(key1, key2, t1);
		}
	};
	struct PTime
	{
		float t;
		float ts;
		PTime(const float &time, const float &scale)
			: t(time)
			, ts(scale)
		{
		};
	};
	std::vector<Path> _paths;
	std::vector<PTime> _pathTimes;

	bool _finished;

	float _stopingTimer;
	float _pauseWhileSquareFlyHiding;

public:
	FieldMover(GameField* gamefield, float time_scale_);	
	~FieldMover();

	void Update(float dt);
	virtual bool isFinish();
};

class ChipPiecesFly : public GameFieldController
{
	struct SqPiece
	{
		float x, y;
		float dx, dy, d2y, angle, dangle;
		float u1, u2, v1, v2;
		float local_time;
		Color color;
		float alpha;
	};

	std::string  _texName;

	std::vector<SqPiece> _pieces;
	Render::Texture *_tex;
public:
	ChipPiecesFly(math::Vector3 pos, Render::Texture *tex_, Color color_, float startTime = -1, float kV = 1.f, int col_count = 4, int row_count=4, int width = 1, int height = 1);

	void Update(float dt);
	virtual bool isFinish();
	virtual void Draw();
};

class ReshuffleChipFly : public GameFieldController
{
	Game::ChipColor &_chip;

	SplinePath<FPoint> _splinePath;
public:
	ReshuffleChipFly(Game::ChipColor &chip, Game::Square *dest);

	void Update(float dt);
	bool isFinish();
};

class DelaySendMessage : public GameFieldController
{
	std::string  _layerName;
	std::string  _widgetName;
	Message _message;

public:
	DelaySendMessage(std::string  layer, std::string  widget,const Message &message, float delay, GameField *gamefield_);
	DelaySendMessage(std::string  layer, std::string  widget, std::string  message, float delay, GameField *gamefield_);

	void Update(float dt);
	virtual bool isFinish();
};

// полет огонька из цепочки в накапливаемый бонус
class FlyFillBonusEffect : public GameFieldController
{
	FPoint start, end;
	ParticleEffect *_trailEffect;
public:
	FlyFillBonusEffect(GameField *gamefield, FPoint from);
	void Update(float dt);
	void DrawAbsolute();
	bool isFinish();
};

// анимация огоньков на конце цепочки, полет огоньков в накапливаемый бонус
class FlyFiresEffect : public GameFieldController
{
	ParticleEffectPtr _fires[3];
	EffectsContainer _effCont;
	int _count;
	FPoint _pos;
	FPoint _dest;
	float _moveTimer, _moveTime;
	bool _fillBonus;
public:
	FlyFiresEffect(GameField *gamefield, FPoint pos);
	void Update(float dt);
	void DrawAbsolute();
	bool isFinish();

	void FlyToPos(FPoint pos, float timer = 0.3f);
	void SetFires(int fires);
	void FlyToFillBonus(); // отправить огоньки к накапливаемому бонусу, после чего контроллер завершится
};

// циферки полученных очков над убитой фишкой
class AddScoreEffect : public GameFieldController
{
	int _score;
	FPoint _pos;
	float _scale;
	Color _color;
	ParticleEffectPtr _eff;
public:
	AddScoreEffect(FPoint pos, int score, float scale = 1.0f, Color col = Color::WHITE, float duration = 1.0f, float delay = 0.0f, const std::string &effName = "");
	void Update(float dt);
	void Draw();
	bool isFinish();
};

//при рождении монстрика эффект съедания фишки
class ChipEatenRemover : public GameFieldController
{
private:
	FPoint _pos; //позиция
	FRect _uv;   //координаты текстуры
	float _sizeCoeff;    //коэффициент масштаба
	float _dsize;
	float _dx;
	float _dy;
public:
	ChipEatenRemover(Game::Square* sq, float timeToEat);
	void Update(float dt);
	void Draw();
	bool isFinish();
};


class FlashAnimationPlayer : public GameFieldController
{
public:
	enum DrawType
	{
		DRAW_UP,
		DRAW_ABSOLUTE,
	};
private:
	FlashAnimation *_animation;
	FPoint _pos;
	bool _mirror;
	DrawType _draw_type;
public:
	FlashAnimationPlayer(const Game::AnimRes &res,  FPoint pos, DrawType draw_type = DRAW_UP);
	void Update(float dt);
	void Draw();
	void DrawAbsolute();
	bool isFinish();
};