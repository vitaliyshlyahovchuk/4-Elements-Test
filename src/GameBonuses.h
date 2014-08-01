#ifndef __GAME_BONUSES_H__
#define __GAME_BONUSES_H__

#include "GameBonus.h"

namespace Game
{

void InitHangBonusTextures();

/************************************************************************************/

class ArrowBonus : public HangBonus
{
	int _level;
	int _radius;
	BYTE _dirs;

	// для анимации смены направления стрелки
	float _angleOffset;

	struct Arrow
	{
		int x, y;
		int radius[8];

		Arrow(int r, int x_, int y_)
			: x(x_)
			, y(y_)
		{
			for(int i = 0; i < 8; ++i) radius[i] = r;
		}
	};
	//8-ми элементный
	std::vector<float> _offsetsArrow;
	int _color_id;

	std::vector<Arrow> _arrows;

	void GetArrowAffectedChips(Arrow &arrow, int radius, Game::FieldAddress from, AffectedArea &chips, float startTime);
	void StartArrowEffect(Arrow &arrow, IPoint pos, class ArrowBonusController *c);

	friend class Hang;
	void DrawArrows(FPoint chipPos, float localTime, Render::SpriteBatch *batch, bool front);
	void PushMatrix(FPoint chipPos);
	void PopMatrix();
public:
	static float DELAY_FOR_CHIP_AFTER_ARROW;
	static float DELAY_FOR_RUN_ARROW_BONUS;

public:
	static void InitGame(rapidxml::xml_node<> *xml_info);
	ArrowBonus(BYTE dir, int radius, int level);
	void GetAffectedChips(Game::FieldAddress from, AffectedArea &chips, float startTime);
	void StartEffect(Game::FieldAddress from, float startTime);

	void Save(Xml::TiXmlElement *xml_elem);

	void Update(float dt) override;

	void DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch);
	void DrawOverChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch);

	void DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch);

	Type GetType() const { return ARROW; }

	int GetRadius() const { return _radius; }
	BYTE GetDirections() const { return _dirs; }
	void SetRadius(int radius) { _radius = radius; }
	void SetDirections(BYTE dirs) { _dirs = dirs; }
	
	void StartRotateAnim(float fromAngle);
};

/************************************************************************************/

class BombBonus : public HangBonus
{
	int _radius;
	int _level;

	friend class Hang;
public:
	BombBonus(int radius, int level);
	void GetAffectedChips(Game::FieldAddress from, AffectedArea &chips, float startTime);
	void StartEffect(Game::FieldAddress from, float startTime);

	void DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch);
	void DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch);

	void Save(Xml::TiXmlElement *xml_elem);

	Type GetType() const { return BOMB; }

	int GetRadius() const { return _radius; }
};

class BombBonusController : public GameFieldController
{
	IPoint _center;
	int _level;
	int _radius;
public:
	BombBonusController(int level, int radius, IPoint center, float startTime);
	void Update(float dt);
	void Draw();
	bool isFinish();
};

/************************************************************************************/

class GameLightningController;

class LightningBonus : public HangBonus
{
	ColorMask _colors;
	std::vector<GameLightningController*> _controllers;

	friend class Hang;
public:
	LightningBonus(ColorMask colors);
	LightningBonus(const LightningBonus &other);
	~LightningBonus();
	void GetAffectedChips(Game::FieldAddress from, AffectedArea &chips, float startTime);
	void StartEffect(Game::FieldAddress from, float startTime);
	void SetColors(ColorMask colors) { _colors = colors; }

	void Save(Xml::TiXmlElement *xml_elem) {}

	LightningBonus & operator= (const LightningBonus &other);

	void DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch);
	void DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch);

	Type GetType() const { return LIGHTNING; }
};

/************************************************************************************/

class LightningArrowBonus : public HangBonus
{
	LightningBonus _lightning;
	int _radius;
	BYTE _dirs;

	AffectedArea _lightningChips;
	std::map<Game::FieldAddress, BYTE> _arrows;

	friend class Hang;
public:
	LightningArrowBonus(ColorMask colors, BYTE dirs, int radius);

	void GetAffectedChips(Game::FieldAddress from, AffectedArea &chips, float startTime);
	void StartEffect(Game::FieldAddress from, float startTime);

	void Save(Xml::TiXmlElement *xml_elem) {}
	void DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch) {}
	void DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch) {}

	Type GetType() const { return LIGHTNING_ARROW_COMBO; }
};

class LightningArrowController : public GameFieldController
{
	std::map<Game::FieldAddress, BYTE> _arrows;
	int _radius;
	AffectedArea _chips;
public:
	LightningArrowController(const AffectedArea &chips, const std::map<Game::FieldAddress, BYTE> &arrows, int radius, float startTime);
	void Update(float dt);
	void Draw();
	bool isFinish();
};

//маленькая бомба - для соответствующего буста
class LittleBombBonus : public HangBonus
{

	friend class Hang;
public:
	LittleBombBonus();
	void GetAffectedChips(Game::FieldAddress from, AffectedArea &chips, float startTime);

	void DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch);
	void DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch);

	void Save(Xml::TiXmlElement *xml_elem);

	Type GetType() const { return LITTLE_BOMB_BOOST; }
};


} // end of namespace

#endif