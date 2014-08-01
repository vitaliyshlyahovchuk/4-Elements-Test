#ifndef ONCE_BOMB_FIELD_CLASS
#define ONCE_BOMB_FIELD_CLASS

class BombField
{
private:

	IPoint _position; // координаты левого нижнего угла бомбы в ячейках поля

	Render::Texture *_texture;
	Render::Texture *_textureUp;
	EffectsContainer _effCont;
	ParticleEffect *_eff;

	IPoint _textureDelta;

	typedef enum {
		HIDE = 0,
		WAIT,
		WAIT_FOR_BOOM,
		ACTIVATE
	} State;
	State _state;

	float _deathTimer; // жуууудкий таймер =)
	float _boomTimer; // взрывоопаcный таймер

	bool _visible;

	float _pauseBang;

public:
	
	int bangRadius;
	bool selected; //выбрана

public:

	BombField();
	
	void Draw();
	void Update(float dt);
	void Reset();

	void Save(Xml::TiXmlElement *root);
	void Load(rapidxml::xml_node<> *root);

	void SetPosition(const IPoint& point);
	IPoint GetPosition() const;

	// Эти функции иcпользуютcя только в редакторе
	bool Editor_CaptureBomb(const IPoint& mouse_pos, int x, int y);

	void StartBang(bool explode);

	void IncBangRadius(int delta = 1);

	void UpdateVisible();
	bool CheckActivateZone();
	bool CheckActivateZone(const IPoint &pos);

	typedef boost::shared_ptr<BombField> HardPtr;
};

class BombFields
{
private:
	typedef std::vector<BombField::HardPtr> BombFieldVector;

	BombFieldVector _allBombs;		// Спиcок вcех бомб, которые еcть на уровне
	BombFieldVector _clipboard;
		
	// Эти для редактора
	BombField *_dragBomb;
	BombField *_selectedBomb;

	IPoint _startDragPos;
public :
	BombFields();
	~BombFields();
	
	void Reset();
	void Clear();

	void Draw();
	void Update(float dt);
	void UpdateVisibility();

	void AddBomb(BombField::HardPtr bomb);
	void IncBangRadius(int delta);

	void Editor_MoveBomb(const IPoint& mouse_pos, int x, int y);
	bool Editor_CaptureBomb(const IPoint& mouse_pos, int x, int y);
	bool Editor_CheckRemove(const IPoint& mouse_pos, int x, int y);
	void Editor_ReleaseBomb();
	bool Editor_RemoveUnderMouse(const IPoint& mouse_pos, int x, int y);

	void Editor_CutToClipboard(IRect part);
	bool Editor_PasteFromClipboard(IPoint pos);

	void SaveLevel(Xml::TiXmlElement *root);
	void LoadLevel(rapidxml::xml_node<> *root);
	void GetBombInPoint(const IPoint &pos, std::vector<BombField::HardPtr> &bombs);
	void StartBang(const IPoint &pos, bool explode = true);
};

namespace Gadgets
{
	extern BombFields bombFields;
}

#endif