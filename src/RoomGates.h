#pragma once

#include "BaseEditorMaker.h"

class RoomGate
{
	IPoint _pos;
	float _local_time;
	Game::Order::HardPtr _order;

	ParticleEffectPtr _eff;

	bool _opened;
	int _dir;

	void StartEffect();
	void KillEffect();
	bool EnergyReady() const;
public:
	RoomGate(rapidxml::xml_node<> *xmlElem);
	RoomGate(IPoint pos, int direction);
	~RoomGate();

	void Save(Xml::TiXmlElement *xmlElem);

	void Update(float dt);
	void Draw(Render::SpriteBatch *batch);
	void DrawOrder();

	void InitCell();
	void FreeCell();

	bool Opened() const;

	IPoint GetPos() const { return _pos; }
	IPoint GetActivePos() const;
	int GetDir() const { return _dir; }
	bool IsOnScreen() const;

	Game::Order::HardPtr* GetOrder();
	bool SetOrder(Game::Order::HardPtr order);

	typedef boost::shared_ptr<RoomGate> HardPtr;
};

class RoomGates : public Gadgets::BaseEditorMaker
{
	std::vector<RoomGate::HardPtr> _gates;

	int _dir;

	Render::SpriteBatchPtr _batch;

	RoomGate* GateOnCell(Game::FieldAddress fa) const;
public:
	RoomGates();

	void LoadLevel(rapidxml::xml_node<> *root);
	void SaveLevel(Xml::TiXmlElement *root);

	void Clear();

	void Update(float dt);
	void Draw();
	void DrawEdit();

	bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_MouseWheel(int delta, Game::Square *sq);

	Game::Order::HardPtr* Editor_SelectOrder(IPoint mouse_pos, Game::Square *sq);

	bool IsGateCell(Game::FieldAddress fa) const;

	BYTE EnergyWallsInCell(Game::FieldAddress fa) const;

	void RemoveVisible();
};

namespace Gadgets
{
	extern RoomGates gates;
}