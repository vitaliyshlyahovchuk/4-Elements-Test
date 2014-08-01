#ifndef ONCE_ENERGY_RECEIVERS
#define ONCE_ENERGY_RECEIVERS
#include "EnergyReceiver.h"
#include "BaseEditorMaker.h"

class EnergyReceivers
	: public Gadgets::BaseEditorMaker
{
	typedef std::list<EnergyReceiver::HardPtr> Receivers;	// Спиcок указателей на приёмники энергии

	Receivers receivers;			// Спиcок вcех приёмников энергии раccтавленных cейчаc на уроне

	EnergyReceiver *_energyReceiverSelected;	// Выбранный приёмник энергии. Только в редакторе!

	void ResetForEditor();

	Receivers _clipboard;
	IPoint _lastReceiver;
public:
	EnergyReceivers();

	void InitLevel();
	void LoadLevel(rapidxml::xml_node<> *xml_level);

	EnergyReceiver* MoreFar();
	// возвращает какой-нибудь приемник, существует только для совместимости со старым кодом (стрелка
	// показывающая направление куда идти), который был рассчитан только на один приемник и один хрен
	// уже не работает и надеюсь будет либо выпилен, либо модернизирован

	void Init(GameField *field, const bool &with_editor);
	void Clear();
	EnergyReceiver::HardPtr GetReceiver(IPoint index);
	bool IsReceiverCell(Game::FieldAddress fa) const;
	bool IsReceiverCell(const IPoint &mouse_pos) const;

	void DrawDown();
	void DrawBase();
	void DrawUp();

	bool FocusNeed() const; // нужно ли сфокусировать камеру на приемнике энергии
	IPoint FocusCenter() const;

	void Update(float dt);

	void OnEndMove();

	void DrawEdit();
	void SaveLevel(Xml::TiXmlElement *xml_level);
	bool AcceptMessage(const Message &message);
	size_t ActiveCount();
	size_t TotalCount() const;
	bool IsFlying();
	// Обновляем эффекты на алтарях...
	void ReloadEffect();
	bool Editor_CheckMinigamesArea(const IRect& part);
	void Editor_MoveElements(const IRect &part, const IPoint &delta);

	bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq);
	bool Editor_MouseMove(const IPoint &mouse_pos, Game::Square *sq);
	void Editor_Reset();

	void Editor_CutToClipboard(IRect part);
	bool Editor_PasteFromClipboard(IPoint pos);

	Game::Order::HardPtr* Editor_SelectOrder(IPoint mouse_pos, Game::Square *sq);

	bool CanMoveCameraTo(FPoint pos) const;

	void GetReceiversPositions(std::vector<IPoint> &vec) const;
	bool IsLastReceiver(const Game::FieldAddress &address) const;

	EnergyReceiver *GetReceiverOnSquare(Game::FieldAddress fa) const;
};

namespace Gadgets
{
	extern EnergyReceivers receivers;
}

#endif//ONCE_ENERGY_RECEIVERS
