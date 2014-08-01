#pragma once

#include "StripEffect.h"

#include "TextureMesh.h"

class GameField;
class ParticleEffectWrapper;

//
// Цикличеcкий путь отдельных точек текcтуры.
//
struct CirclePath{

	float radius;
		// радиуc окружноcти

	float angle;
		// текущее положение на окружноcти в радианах

	float deltaMin;
		// минимальный шаг движения по окружноcти (рад/cек)

	float deltaMax;
		// макcимальный шаг движения по окружноcти (рад/cек)

	float x, y;
		// координаты точки отноcительно начала координат

	float k;

	CirclePath()
	{
	}



	//
	// инициализируем "cлучайными" чиcлами из заданного диапазона
	// начальное положение и шаг
	//
	CirclePath(FPoint pStart, float max, float radiusMin, float radiusMax = 0.f
		, float angleMin = 0, float angleMax = math::PI/2)
	{


		angle = math::random(0.f, 2*math::PI);
		radius = math::random(radiusMin, std::max(radiusMin, radiusMax));
		deltaMin = angleMin;
		deltaMax = angleMax;
		x = -radius*sin(angle);
		y = radius*cos(angle);

		float l = FPoint(x, y).GetDistanceTo(pStart);
		k = math::sin(l/max)*math::PI/180;
	}

	//
	// для cложения точек удобнее иcпользовать!
	//
	FPoint GetFPoint()
	{
		return FPoint(x, y);
	}

	// 
	// можем cкладывать c FPoint - cделаем жизнь проще
	//
	operator FPoint()
	{
		return FPoint(x, y);
	}

	//
	// обновляем координаты точки
	// 
	void Update(float deltaTime)
	{
		if (deltaTime>0.1f)
		{
			deltaTime = 0;
		}
		angle += math::random(deltaMin, deltaMax)*deltaTime;
		x = -radius*sin(angle);
		y = radius*cos(angle);
		if (angle > 2*math::PI)
		{
			angle -= 2*math::PI;
		}
	}
};












class RyushkaGroupItem 
{
public:
	RyushkaGroupItem(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	virtual void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Update(float dt){};
	virtual void Draw(){};
	virtual void  DrawCenter(){};
	virtual void MouseMove(const IPoint& pos){};
	virtual void MouseDown(const IPoint& pos){};
	virtual void setPosition(const IPoint &pos){};
	virtual FPoint GetDeltaPos();
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();
	virtual void SetEnergy();
	virtual void ResetEnergy();
	virtual void Reset();
	virtual bool IsEffect();
	virtual void Upload();
	virtual void Release();
	Ryushka *_parent;
	IRect _rect;
	IRect _visibleRect;
	std::string  _type;
	typedef enum {
		SHAW,
		HIDE,
		FREE,
		RESET,
	} State;
	State _stateOnEnergy;

	bool _isActiv;
		
};

class RyushkaGroup : public Ryushka
{
public:
	typedef std::list<RyushkaGroupItem*> Items;
	typedef std::list<RyushkaGroupItem*>::iterator Iterator;

	RyushkaGroup(rapidxml::xml_node<>* xmlElement);
	virtual void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Update(float dt);
	virtual void OnDraw();
	virtual void MouseMove(const IPoint& pos);
	virtual void MouseDown(const IPoint& pos);
    virtual void setPosition(const IPoint &pos);
	virtual void Reset();
	virtual FPoint GetDeltaPos();
	virtual void UpdateState();
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();
	virtual void Upload();
	virtual void Release();

	virtual void DrawRamka();
	virtual void AcceptMessage(const Message &message);
	bool _onEnergy;
	bool _isActivEnergy;
	bool _wait;

	IRect _rect;
	IRect _visibleRect;
	Items _items;
	
	void SetWaitTime(float time)
	{
		_waitTime = time;
	}

private:
	
	float _waitTime;
	float _timer;

};

class RyushkaGroupEnergyFall : public RyushkaGroup
{
private:

	Render::Texture *_editorTexture;
	std::string  _name;

	Render::Texture *_borderTexture;

	float _showTimer;

	typedef enum 
	{
		HIDE,
		WAIT,
		SHOWING,
		SHOW
	} State; State _state;

	IPoint _deltaPos; // cмещение текcтуры 

public:
	RyushkaGroupEnergyFall(rapidxml::xml_node<>* xmlElement);
	virtual void SaveToXml(Xml::TiXmlElement *parentElem);

	virtual void Reset();
	virtual void Update(float dt);
	virtual void OnDraw();

	virtual void UpdateState();
	virtual void DrawRamka();
};


class ItemTexture : public  RyushkaGroupItem
{
public:
	ItemTexture(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	virtual void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Draw();
	std::string  _name;
	Render::Texture *_tex;
};

// цепь на табличках
class ItemText : public  RyushkaGroupItem
{
public:
	ItemText(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Draw();
	std::string  _name;
	//TText * _text;
	Render::Text* _text;
	virtual IRect getVisibleRect();
};


class ItemMask{
	public:
	ItemMask(rapidxml::xml_node<>* xmlElementm, Ryushka *parent);
	void SaveToXml(Xml::TiXmlElement *parentElem);
	bool IsPixelOpaque(const IPoint& pos);

	void Draw();
	IRect _rect;
	bool _activ;
	Render::Texture *_tex;
	std::string  _nameTex;
	Ryushka *_parent;
	enum {
		TYPE_RECT,
		TYPE_TEXTURE
	} _type;

};
class ItemEffect : public  RyushkaGroupItem
{
public:
	ItemEffect(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	~ItemEffect();
	void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Update(float dt);
	virtual void Draw();
	virtual void DrawCenter();
	virtual void MouseDown(const IPoint& pos);
	virtual bool IsPixelOpaque(const IPoint& pos);
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();
	virtual void SetEnergy();
	void ResetEnergy();
	std::string _name;
	ParticleEffectWrapper *_eff;
	float	_dt_start;
	ItemMask *_mask;
	
	virtual void Upload();
	virtual void Release();

};


class ItemTree : public  ItemTexture
{
public:
	ItemTree(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	~ItemTree();
	void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Update(float dt);
	virtual void Draw();
	virtual void MouseMove(const IPoint& pos);
	Pendant *_pendant;
	Mesh  *_mesh;
};

class ItemPendantEffect : public  ItemEffect
{
public:
	ItemPendantEffect(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Update(float dt);
	virtual void Draw();
	virtual void MouseMove(const IPoint& pos);
	Pendant *_pendant;
	virtual void Reset();
	virtual void Release();


};

class ItemPendantTexture : public  ItemTexture
{
public:
	ItemPendantTexture(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Update(float dt);
	virtual void Draw();
	virtual void MouseMove(const IPoint& pos);
	Pendant *_pendant;
	virtual void Reset();
	virtual void Release();
};



class ItemFlag : public  ItemTexture
{
public:
	float _timer;
	ItemFlag(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Update(float dt);
	virtual void Draw();
	virtual void MouseMove(const IPoint& pos);
	Mesh  *_mesh;
};








namespace Butterfly{
struct StandPoint 
{
	math::Vector3 _point;
	bool _inUse;
	void Init(rapidxml::xml_node<>* pointsElem);
	void SaveToXml(Xml::TiXmlElement *parentItem);

};

class BaseButterfly
{
public:
	float _angleX;
	float _angleY;
	float _angleZ;
	float _nextAngleZ;
	float _prevAngleZ;
	float local_time;
	float time_scale;
	float _timerMove;
	float _size;
	math::Vector3 _prevPos;
	math::Vector3 _nextPos;
	float _pause;
	float _timeScaleMove;
	float yBound, x, y;
	Render::Texture *_tex;


	BaseButterfly(Render::Texture *tex);
	virtual void Draw();
	virtual void Update(float dt);
	virtual void Init();
	virtual void Scare();
};

class ItemButterfly : public ItemTexture,BaseButterfly
{
private:
	
	enum 
	{
		STAND,
		FLY,
		CHANGEANGLE,
	}_state;

	std::vector<StandPoint> _standPoints;

	void UpdateFly(float dt);
	void UpdateLight(float dt);
	void UpdateStand(float dt);
	void SetState();
	void SetAngle();
	void StartMove();
	void SaveToXml(Xml::TiXmlElement *parentItem);
	int GetSizeStartPoint() {
		return static_cast<int> (_standPoints.size());
	};

	StandPoint *GetStartPoint() {
		
		int rand = math::random(0, GetSizeStartPoint()-1);
		
		while ((_standPoints)[rand]._inUse) {
			rand = math::random(0, GetSizeStartPoint()-1);
		}
		
		(_standPoints)[_pointIndex]._inUse = false;

		_pointIndex = rand;

		(_standPoints)[rand]._inUse = true;

		StandPoint* point = &(_standPoints)[rand];

		return point;
	};
	


	
	int _pointIndex;

public:
	



	ItemButterfly(rapidxml::xml_node<>* xmlElement, Ryushka *parent);

	virtual void Init();
	virtual void Update(float dt);
	virtual void Draw();
	virtual void MouseMove(const IPoint &mouse_pos);
	virtual void MouseDown(const IPoint& pos);
	virtual IRect getVisibleRect();
	void DrawShade(float shadeAlpha);

};
}
class TextureMove : public  ItemTexture
{
public:
	TextureMove(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	virtual void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Draw();
	
	virtual FPoint GetDeltaPos();
	virtual void MouseMove(const IPoint& pos);
	void Update(float dt);
private:
	IPoint _mousePosOld;
	FPoint  _add;
	FPoint _random_add;
	float _k;
	float _m;
	float _c;
	FPoint _sp;
	FPoint _dp;
	FPoint _a;
};

class EffectMove : public  ItemEffect
{
public:
	EffectMove(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	virtual void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Draw();
	
	virtual FPoint GetDeltaPos();
	virtual void MouseMove(const IPoint& pos);
	void Update(float dt);
private:
	IPoint _mousePosOld;
	FPoint  _add;
	FPoint _random_add;
	float _k;
	float _m;
	float _c;
	FPoint _sp;
	FPoint _dp;
	FPoint _a;
};

// цепь
class ItemChain : public  ItemTexture
{
public:
	ItemChain(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Update(float dt);
	virtual void Draw();
	virtual void MouseMove(const IPoint& pos);
	Pendant *_pendant;
	Mesh  *_mesh;
};

// Вододороcли
class ItemWaterPlant : public  ItemTexture
{

	typedef std::vector<std::vector<CirclePath> > Paths;
	TextureMesh::SharedPtr _mesh;
	Paths _paths;
	FPoint _pos;
	Pendant *_pendant;

	float _maxShift;
	float _timeScale;
	int _blockLeft, _blockRight, _blockUp, _blockDown;
	IPoint _oldPos;				

public:
	ItemWaterPlant(rapidxml::xml_node<>* xmlElement, Ryushka *parent);
	void SaveToXml(Xml::TiXmlElement *parentElem);
	virtual void Update(float dt);
	virtual void Draw();
	virtual void MouseMove(const IPoint& pos);
};
