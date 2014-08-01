#ifndef __GAME_BONUS_H__
#define __GAME_BONUS_H__

#include "GameFieldAddress.h"
#include "GameFieldController.h"
#include "LevelColors.h"

namespace Game
{

BYTE RotateArrow(BYTE dir, int a);

struct ClearCellInfo
{
	float delay;
	bool kill_near;

	ClearCellInfo() : delay(0.0f), kill_near(false) {}
	ClearCellInfo(float d, bool n) : delay(d), kill_near(n) {}
};

typedef std::map<Game::FieldAddress, ClearCellInfo> AffectedArea;

class HangBonus
{
protected:
	float _localTime;
public:
	enum Type
	{
		ARROW,
		BOMB,
		LIGHTNING,
		LIGHTNING_ARROW_COMBO,
		LEVEL_END,
		LITTLE_BOMB_BOOST, //"фиктивный" бонус - чтобы сработал буст
	};
	HangBonus();

	virtual ~HangBonus() {};

	// вставляет данные о фишке в area, если фишка там уже есть, то выставляет меньшую задержку
	static void AddChip(Game::FieldAddress chip, const ClearCellInfo& info, AffectedArea &area);

	// добавляет в список chips фишки, которые будут убиты этим бонусом и в какой момент времени после детонации это произойдет
	virtual void GetAffectedChips(Game::FieldAddress from, AffectedArea &chips, float startTime) = 0;

	// запускает контроллер, проигрывающий визуальный эффект бонуса
	virtual void StartEffect(Game::FieldAddress from, float startTime) {};

	// сохранение предустановленного на фишке бонуса
	virtual void Save(Xml::TiXmlElement *xml_elem) = 0;

	// рисование бонуса навешенного на фишке
	virtual void DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch) = 0;
	virtual void DrawOverChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch){};

	// рисование бонуса в самостоятельном виде, вместо фишки
	virtual void DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch) = 0;

	virtual Type GetType() const = 0;

	virtual void Update(float dt);

	typedef boost::shared_ptr<HangBonus> HardPtr;
};

/************************************************************************************/

class ArrowBonus;
class BombBonus;
class LightningBonus;
class LightningArrowBonus;

class Hang
{
	typedef std::list<HangBonus::HardPtr> BonusList;
	BonusList _bonuses;

	void DeepCopyBonuses(const Hang& other);

	bool Arrow_Arrow(ArrowBonus *arrow1, ArrowBonus *arrow2);
	bool Bomb_Bomb(BombBonus *bomb1, BombBonus *bomb2);
	bool Light_Light(LightningBonus *l1, LightningBonus *l2);
	bool LArrow_LArrow(LightningArrowBonus *la1, LightningArrowBonus *la2);
	bool Arrow_Bomb(ArrowBonus *arrow, BombBonus *bomb);
	bool Arrow_Light(ArrowBonus *arrow, LightningBonus *light);
	bool Arrow_LArrow(ArrowBonus *arrow, LightningArrowBonus *la);
	bool Bomb_Light(BombBonus *bomb, LightningBonus *light);
	bool Bomb_LArrow(BombBonus *bomb, LightningArrowBonus *la);
	bool Light_LArrow(LightningBonus *light, LightningArrowBonus *la);
	bool AddCombination(HangBonus::HardPtr b1, HangBonus::HardPtr b2);
	void CombineBonuses();
public:
	enum TransformChip
	{
		NONE,
		CHAMELEON,
		ENERGY_BONUS
	};
	TransformChip transformChip;
	bool autoRun;
	
public:
	static const BYTE ARROW_R  = 0x01;
	static const BYTE ARROW_UR = 0x02;
	static const BYTE ARROW_U  = 0x04;
	static const BYTE ARROW_UL = 0x08;
	static const BYTE ARROW_L  = 0x10;
	static const BYTE ARROW_DL = 0x20;
	static const BYTE ARROW_D  = 0x40;
	static const BYTE ARROW_DR = 0x80;
	static const BYTE ARROW_4 = ARROW_L | ARROW_R | ARROW_U | ARROW_D;
	static const BYTE ARROW_4D = ARROW_UR | ARROW_UL | ARROW_DL | ARROW_DR;
	static const BYTE ARROW_8 = ARROW_4 | ARROW_4D;

	Hang();
	Hang(const Hang& other);
	Hang(const std::string& type, int radius, int level, IPoint arrowDir = IPoint(1,0), TransformChip transform = NONE, bool autorun = false);

	void MakeArrow(int radius, BYTE dir);
	void MakeBomb(int radius, int level);
	void MakeLightning(ColorMask colors);
	void MakeLittleBomb();
	void MakeBonus(HangBonus::HardPtr bonus);

	void GetAffectedChips(Game::FieldAddress from, AffectedArea &chips, float startTime);
	void StartEffects(Game::FieldAddress cell, float startTimes) const;
	ColorMask GetColor() const;

	void Clear();
	bool IsEmpty() const;

	void Load(rapidxml::xml_node<> *xml_elem);
	void Save(Xml::TiXmlElement *xml_elem);

	// суммирует бонусы в цепочке фишек (или в каскаде бонусов)
	// inChain - суммируем бонусы внутри одной цепочки, либо бонусы задевающие друг друга в каскаде
	// relativePos - положение бонуса hang относительно данного бонуса (в клетках)
	void Add(const Hang &hang, bool inChain, IPoint relativePos = IPoint());

	void DrawOnChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch);
	void DrawOverChip(FPoint chipPos, float localTime, Render::SpriteBatch *batch);
	void DrawOnSquare(FPoint chipPos, float localTime, Render::SpriteBatch *batch);

	HangBonus *GetBonusByType(HangBonus::Type type) const;
	void DeleteBonusType(HangBonus::Type type);
	void Update(float dt);

	Hang& operator= (const Hang &other);

	typedef boost::shared_ptr<Hang> HardPtr;
	typedef boost::weak_ptr<Hang> WeakPtr;
};

} // end of namespace

#endif