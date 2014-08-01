#ifndef ONCE_SNAP_GADGET_CLASS
#define ONCE_SNAP_GADGET_CLASS
#include "SnapGadgetElements.h"
#include "BaseEditorMaker.h"

class SnapGadgets
	: public Gadgets::BaseEditorMaker
{
private :
	bool _isRightMouse;
	IPoint _mouseDownPos;

	float _max_distance;
	std::vector<SnapGadget*> _gadgets;	// Спиcок вcех привязок, которые еcть на уровне

	bool _drag_gadget;
	IPoint _fromPos;
	float _editTimer;
	SnapGadget *_currentEditorGadget, *_current;
	SnapGadget *_activeGameGadget;	// Активная в данный момент привязка или NULL, еcли активной нет

	GameField *_gamefield;

	//// Эти для редактора
	SnapGadget *_dragGadget;
	int _dragElementId;

	int CountActiveGadgets() const;
	//Гаджеты загружены. 
	//Используется как индикатор для проверки того что они загрузились раньше чем приемники энергии
	bool _loaded;
	IRect _editor_moveRect;

	bool Editor_ProcessClick(int &captured, SnapGadget *g);
	bool Editor_CaptureGadget(const IPoint& mouse_pos);
	// Перемещение инcтрументом для перемещения чаcти уровня...
	void Editor_MoveElements(const IRect& part, const IPoint& delta);
	void Editor_ReleaseGadget();

public :
	SnapGadgets();
	~SnapGadgets();

	void Reset();

	void UpdateActive();
	bool CheckTargetingFirst(IPoint &to);
	void Update(float dt);

	void AddGadget(SnapGadget *gadget);
	void Init(GameField *field);
	void SetActivePassed();

	const SnapGadget *GetActiveGadget() 
	{
		return (_activeGameGadget); 
	}

	virtual void Clear();
	virtual void Release();
	void DrawGame();
	virtual void DrawEdit();
	virtual bool Editor_MouseMove(const IPoint& mouse_pos, Game::Square *sq);
	virtual bool Editor_LeftMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	virtual bool Editor_RightMouseDown(const IPoint &mouse_pos, Game::Square *sq);
	virtual bool Editor_MouseUp(const IPoint &mouse_pos, Game::Square *sq);
	virtual bool Editor_MouseWheel(int delta, Game::Square *sq);
	virtual void Editor_CutToClipboard(IRect part);
	virtual bool Editor_PasteFromClipboard(IPoint pos);
	virtual void LoadLevel(rapidxml::xml_node<> *root);
	virtual void SaveLevel(Xml::TiXmlElement *root);
	virtual bool AcceptMessage(const Message &message);
	void CheckAfterLoadReceivers();
	void PrepareLevel();
	bool IsEnergyLevelFinished() const;

	SnapGadget* FindNearCaptureGadget(IPoint cell, float &finded_dist);
	SnapGadget* FindNearCaptureGadget(SnapGadget *prev_snap);

	SnapGadget* FindFirstGadget(); // ищем точку привязки, привязанную к источнику энергии
	bool IsLoaded() const;

	void FindSnap(const IPoint index, std::vector<SnapGadgetBaseElement::SHARED_PTR> &elements);
};

namespace Gadgets
{
	extern SnapGadgets snapGadgets;
}

#endif