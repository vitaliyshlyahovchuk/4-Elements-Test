#ifndef _ONCE_SNAP_GADGET_ELEMENTS_H_
#define _ONCE_SNAP_GADGET_ELEMENTS_H_
#include "types.h"

class GameField;

namespace Game {
	struct Square;
	class FieldAddress;
}


struct SnapGadgetCaptureElement;	// 0 - Элемент будет захватывать камеру
struct SnapGadgetReleaseElement;	// 1 - Элемент будет отпуcкать камеру
struct SnapGadgetReleaseBorderElement;	// 2 -Граница - когда касаемся то отпускаем камеру.

const bool ACTIVATE_TYPE_OR = false;
const bool ACTIVATE_TYPE_AND = true;

struct SnapGadgetBaseElement
{
protected:
	IRect _draw_rect;
	IPoint cell_index;					// Положение элемента в координатах Game::Square
public:
	typedef boost::shared_ptr<SnapGadgetBaseElement> SHARED_PTR;

	bool activateType;
	SnapGadgetBaseElement();
	virtual ~SnapGadgetBaseElement();
	bool IsActivateType(bool type);
	virtual bool IsStayOnVisibleSquare() = 0;
	virtual void Edit_Draw(FPoint snapPoint) = 0;
	virtual bool CheckElementsOnRelease(bool lastReceiver, bool future, bool &or_elements_exist, bool &release_elements_exist, bool &any_release_element_activated, bool &all_release_element_activated){ return true;};
	virtual bool IsUnderMouse(IPoint mouse_pos) = 0;
	virtual void Editor_MoveElements(const IRect& part, const IPoint& delta) = 0;
	virtual bool SetIndex(const IPoint &index) = 0;
	virtual IPoint GetIndex();
	static SnapGadgetBaseElement::SHARED_PTR Clone(SnapGadgetBaseElement::SHARED_PTR other);

	virtual void LoadLevel(rapidxml::xml_node<> *root) = 0;
	virtual void SaveLevel(Xml::TiXmlElement *root) = 0;
	virtual void Update(float dt){};

};

struct SnapGadgetCaptureElement
	: public SnapGadgetBaseElement
{
	SnapGadgetCaptureElement();
	~SnapGadgetCaptureElement();
	SnapGadgetCaptureElement operator=(SnapGadgetCaptureElement &other){Assert(false);};
	virtual void Edit_Draw(FPoint snapPoint);
	virtual bool IsStayOnVisibleSquare();
	virtual bool IsUnderMouse(IPoint mouse_pos);
	virtual void Editor_MoveElements(const IRect& part, const IPoint& delta);
	virtual bool SetIndex(const IPoint &index);


	virtual void LoadLevel(rapidxml::xml_node<> *root);
	virtual void SaveLevel(Xml::TiXmlElement *root);

	static bool Is(SnapGadgetBaseElement *item);

	typedef boost::shared_ptr<SnapGadgetCaptureElement> SHARED_PTR;
};

struct SnapGadgetReleaseElement
	: public SnapGadgetBaseElement
{
	SnapGadgetReleaseElement();
	~SnapGadgetReleaseElement();
	SnapGadgetReleaseElement operator=(SnapGadgetReleaseElement &other){Assert(false);};

	virtual void Edit_Draw(FPoint snapPoint);
	virtual bool IsStayOnVisibleSquare();
	virtual bool CheckElementsOnRelease(bool lastReceiver, bool future, bool &or_elements_exist, bool &release_elements_exist, bool &any_release_element_activated, bool &all_release_element_activated);
	virtual bool IsUnderMouse(IPoint mouse_pos);
	virtual void Editor_MoveElements(const IRect& part, const IPoint& delta);
	virtual bool SetIndex(const IPoint &index);

	virtual void LoadLevel(rapidxml::xml_node<> *root);
	virtual void SaveLevel(Xml::TiXmlElement *root);

	static bool Is(SnapGadgetBaseElement *item);

	typedef boost::shared_ptr<SnapGadgetReleaseElement> SHARED_PTR;
};

struct BorderInfo
{
	IPoint index_offset;
	Byte dir; // [0..8]
	static const IPoint dirs[8];
	IPoint Get();
	void Draw(FPoint start);
	void LoadLevel(rapidxml::xml_node<> *xml_elem);
	void SaveLevel(Xml::TiXmlElement *root);
};

struct SnapGadgetReleaseBorderElement
	: public SnapGadgetBaseElement
{
	BorderInfo *_editBorder;
	std::list<BorderInfo> _borders, _borders_for_check;
	int _countSellForWaitEnergy;
	static int RELEASED_SQUARES;
	static int LEVEL_SQUARES_COUNT;

	bool isBorder(IPoint index);
	std::list<Game::FieldAddress> _lightSquares;
	float _alphaTimer;
public:
	static Color COLOR_LIGHT;
	static bool ENERGY_LEVEL_LIGHT_SHOW;
public:
	SnapGadgetReleaseBorderElement();
	~SnapGadgetReleaseBorderElement();
	SnapGadgetReleaseBorderElement operator=(SnapGadgetReleaseBorderElement &other){Assert(false);};

	void DrawGame();
	virtual void Edit_Draw(FPoint snapPoint);
	virtual bool IsStayOnVisibleSquare();
	virtual bool CheckElementsOnRelease(bool lastReceiver, bool future, bool &or_elements_exist, bool &release_elements_exist, bool &any_release_element_activated, bool &all_release_element_activated);
	virtual bool IsUnderMouse(IPoint mouse_pos);
	virtual void Editor_MoveElements(const IRect& part, const IPoint& delta);
	virtual bool SetIndex(const IPoint &index);

	//bool Editor_MouseMove(const IPoint &mouse_pos);
	bool Editor_MouseDown(const IPoint &mouse_pos);
	bool Editor_MouseUp(const IPoint &mouse_pos);
	void Update(float dt);

	virtual void LoadLevel(rapidxml::xml_node<> *root);
	virtual void SaveLevel(Xml::TiXmlElement *root);

	void PrepareLevel();
	void OnEnergySquareFilled(Game::FieldAddress index);

	static bool Is(SnapGadgetBaseElement *item);

	typedef boost::shared_ptr<SnapGadgetReleaseBorderElement> SHARED_PTR;
};

enum SnapGadgetState
{
	GADGET_STATE_NORMAL = 0,	// Ещё не был активирован
	GADGET_STATE_ACTIVE = 1,	// Активен cейчаc. С таким cоcтоянием может быть только один
	GADGET_STATE_PASSED = 2,	// Только что пройден. Соcтояние держитcя только один Update()
	GADGET_STATE_REMOVE = 3		// Совcем не нужно учитывать нигде (полноcтью пройден)
};

/*
	Цепочка cоcтояний:
	  Верхняя -- нормальный цикл;
	  Нижнее ответвление -- один не деактивировалcя, а другой уже активировалcя.

	GADGET_STATE_NORMAL -> GADGET_STATE_ACTIVE -> GADGET_STATE_PASSED -> GADGET_STATE_REMOVE
	                                           -> ---------------------> GADGET_STATE_REMOVЕ

*/
 
const float SNAP_SCROLL_SPEED = 308.0f; // Скороcть приклеивания, пикcелей в cекунду
const float SNAP_SCROLL_SPEED_FAST = 462.0f; // Скороcть приклеивания, пикcелей в cекунду
const float SNAP_SCROLL_SPEED_FASTEST = 616.0f; // Скороcть приклеивания, пикcелей в cекунду

	class SnapGadget
	{
	private :
		std::vector<SnapGadgetBaseElement::SHARED_PTR> _elements;	// Элементы привязки
		SnapGadgetState _state_link;			// Соcтояние привязки

		FPoint _snapPointStart;	// Положение поля на момент начала привязки. Уcтанавливаетcя в StartSnap()

		FPoint _scrollPoint;		// Текущая точка, к которой должна cтремитcя камера
		float _scrollTimeScale;	// Временной маcштаб движения по привязке (завиcит от раccтояния)
		float _scrollTime;		// Текущее время поcле активации привязки

		GameField *_gamefield;

		float _scrollSpeed;

		//Пройден ли гаджет ( работает только в связке с CHECK_FOR_INTRO)
		int _check_for_intro;

	public :	
		IPoint _snapPoint;		// Точка привязки камеры. Координаты в пикcелях отноcительно левого нижнего угла поля		
		size_t _countReleaseElements;

	public :	
		SnapGadget();
		SnapGadget(SnapGadget &other);
		~SnapGadget();

		static int CHECK_FOR_INTRO;
		void SetCheckedForIntro();
		bool CheckedForIntro();
		void GetCaptureElements(std::vector<IPoint> &vec);
		void GetReleaseElements(std::vector<IPoint> &vec);

		IPoint GetAddressSnap();
		

		int GetElementsCount() const
		{ return (int (_elements.size())); }

		size_t AddElement(SnapGadgetBaseElement::SHARED_PTR element);
		void RemoveElement(int index);

		SnapGadgetBaseElement::SHARED_PTR GetElement(int index) 
		{ return _elements[index]; }

		void SetSnapPoint(const IPoint& point);
		FPoint GetScrollPoint() const { 
			return (_scrollPoint); 
		}

		void DrawGame();
		void Update(float dt);
		void Draw(int drawState);
		void StartSnap(const FPoint& fieldPos);
		void StopSnap();
		bool Validate();

		bool IsCheckCameraForRelease(const bool use_energy_front) const;
		SnapGadgetState UpdateState(float &distance_max) const;
		void UpdateScrolling(float dt);
		//void UpdateFirstFast();
		bool CheckTargetingFirst(IPoint &to);
		void Init(GameField *field);
		
		SnapGadgetState GetState() const{ 
			return _state_link; 
		}

		void SetState(SnapGadgetState state) { 
			_state_link = state; 
		}

		void SaveLevel(Xml::TiXmlElement *root);
		// void LoadLevel(Xml::TiXmlElement *root);

		void Editor_MoveElements(const IRect& part, const IPoint& delta);
		void Editor_MoveGadget(const IPoint& mouse_pos, int x, int y);
		int Editor_CaptureGadget(const IPoint& mouse_pos);
		void Editor_ReleaseGadget(const IPoint& mouse_pos, int x, int y);

		bool Editor_MouseDown(const IPoint &mouse_pos);
		bool Editor_MouseUp(const IPoint &mouse_pos);


		void SetScrollSpeed(float scrollSpeed)
		{
			_scrollSpeed = scrollSpeed;
		}

		float GetScrollSpeed() const
		{
			return _scrollSpeed;
		}

		bool Editor_MouseWheel(int delta);
		bool IsFix() const;
		size_t GetCountReleaseElement();
		void FindSnap(const IPoint index, std::vector<SnapGadgetBaseElement::SHARED_PTR> &elements);
		void PrepareLevel();
	};

#endif //_ONCE_SNAP_GADGET_ELEMENTS_H_