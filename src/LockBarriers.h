#ifndef LOCK_BARRIERS_H
#define LOCK_BARRIERS_H

#include "BaseEditorMaker.h"
#include "GameFieldController.h"
#include "GameOrder.h"

class LockBarrierBase
{
protected:
	float _killTimer;
	IPoint _pos;
	bool _destroyed;  // уничтожен ли окончательно (т.е. эффект завершился)
	bool _destroying; // запущен ли эффект окрывания и уничтожения

	//Render::Texture *_tex;
	GameField *_field;
	std::string _type;
	EffectsContainer _effectCont;
public:
	static float YOffset_LOCK;
	bool need_remove; //используется в редакторе для исключения повторов
public:
	LockBarrierBase(const std::string &type);
	virtual void InitLevel();
	virtual ~LockBarrierBase(){};

	IPoint GetPos() const;
	void SetPos(IPoint pos);

	virtual void Destroy() = 0;
	virtual bool Update(float dt);
	virtual void Draw() = 0;
	virtual void DrawUp() = 0;
	void DrawLockBase(const FPoint pos);

	void InitCell();
	void FreeCell(const bool &remove_cell);
	bool IsDestroyed() const;
	virtual bool SetOrder(Game::Order::HardPtr order) { return false; }
	virtual Game::Order::HardPtr* GetOrder() { return NULL; }
	virtual void Save(Xml::TiXmlElement *elem);
	virtual void Load(rapidxml::xml_node<> *elem);
	std::string GetType() const;

	typedef boost::shared_ptr<LockBarrierBase> HardPtr;
};

class LockBarrierWithKey
	:public LockBarrierBase
{
	//Render::Texture *_texLock;
public:
	LockBarrierWithKey();
	void Destroy();
	bool Update(float dt);
	void Draw();
	void DrawUp();

	typedef boost::shared_ptr<LockBarrierWithKey> HardPtr;
};

class LockBarrierOrder
	: public LockBarrierBase
{
	FPoint _offsetTexture;
	float _localTime;
	float _readyTimer; //Цепочка составляется и уже достаточна для убийства замка
	Game::Order::HardPtr _order;
	//Render::Texture *_texFly;
public:
	LockBarrierOrder();
	void InitLevel();
	bool Update(float dt);
	void Draw();
	void DrawUp();
	void Destroy();
	void Save(Xml::TiXmlElement *elem);
	void Load(rapidxml::xml_node<> *elem);
	bool SetOrder(Game::Order::HardPtr order);
	Game::Order::HardPtr* GetOrder();

	typedef boost::shared_ptr<LockBarrierOrder> HardPtr;
};

class LockBarriers : public Gadgets::BaseEditorMaker
{
	typedef std::vector<LockBarrierBase::HardPtr> Barriers;
	Barriers _barriers;
	Barriers _clipboard;

	LockBarrierBase *_selected;

	LockBarrierBase* BarrierOnCell(Game::FieldAddress fa) const;
public:
	LockBarriers();

	void LoadLevel(rapidxml::xml_node<> *root);
	void SaveLevel(Xml::TiXmlElement *root);

	void InitLevel();

	void Clear();

	void Update(float dt);
	void Draw();
	void DrawUp();

	bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq);

	void Editor_CutToClipboard(IRect part);
	bool Editor_PasteFromClipboard(IPoint pos);

	Game::Order::HardPtr* Editor_SelectOrder(IPoint mouse_pos, Game::Square *sq);

	//Поиск повторов по адресам, не может быть более 1 замка в клетке
	void CheckRepeat();
	bool isLocked(Game::FieldAddress fa) const;
};

/***************************************************************************************/
// Контроллеры эффектов открытия и разрушения замка

class LockBarrierUnlockEffect : public GameFieldController
{
	Render::Texture *_keyTex;
	LockBarrierBase *_barrier;
public:
	LockBarrierUnlockEffect(GameField *field, LockBarrierBase *barrier);

	void Update(float dt);
	void Draw();
	bool isFinish();
};

namespace Gadgets
{
	extern LockBarriers lockBarriers;
}

#endif //LOCK_BARRIERS_H