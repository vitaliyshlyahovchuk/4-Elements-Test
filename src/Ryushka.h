#pragma once
#include <boost/smart_ptr.hpp>
#include "StripEffect.h"

class GameField;

// ������� ���cc ��� �c���c��� "�����" �� ������� ����
class Ryushka : public boost::enable_shared_from_this<Ryushka>
{
	bool Contains(FPoint pos, const IRect &rect,const FPoint &rec_half);
public:
	Color _currentDepthStyle;
	void SetDepthStyle(Color depth_style);

	static int COUNT_ID;

	//c������ cc����
    typedef boost::shared_ptr<Ryushka> HardPtr;
	
	//c����� cc����
	typedef boost::weak_ptr<Ryushka> WeakPtr;

	//
	// ���c�������
	//
	Ryushka(rapidxml::xml_node<>* xe, float time_scale_);

	//
	// ��c�������
	//
	virtual ~Ryushka();

	//
	// ��������� � XML
	//
	virtual void SaveToXml(Xml::TiXmlElement* parentElem) = 0;

	//
	// ���-�� ������� �� "�������", c�������� ����� ������� ������cc� �� XML-���c����
	//
	static Ryushka::HardPtr CreateRyushkaFromXml(rapidxml::xml_node<>* xmlElement);
	
	//
	// ������� ��c���� �����
	//
	float GetScale();

	//
	// ��������c�
	//
	virtual void Reset();


	//
	// �c�������� ��c���� �� ��������� �������� c�������
	//
	void SetScaleByFactor(float factor);

	//
	// ������� ��������� �������� c������� �� �������� ��c�����
	//
	float GetFactorByScale();

	//
	// ������� ������� zLevel �����
	//
	int GetZLevel();

	//
	// �c�������� zLevel �� ������� �������
	//
	void SetZLevelByKey(int key);

	//
	// ������� ������� ��������� �����
	//
	float GetDepth();

	//
	// �c�������� ������� ��������� �����
	//
	virtual void SetDepth(float depth);

	//
	// �������� ���� �������� ����� � �����c��
	//
	float GetAngle();

	//
	// �c�������� ���� �������� ����� � �����c��
	//
	void SetAngle(float angle);

	//
	// �������� ����� �� �c� ��c
	//
	void MirrorX();

	bool GetMirrorX();

	//
	// ��c��� �����
	//
	void Draw();
	
	//
	// ��c��� �����
	//
	virtual void DrawRamka();

	//
	// �c�������� ���c����
	//
	virtual void Release();

	//
	// ��������� ���c����
	//
	virtual void Upload();

	//
	// �������� �����
	//
	virtual void Update(float dt);

	//
	// ������� �������������, ���c������� �����
	//
	virtual IRect getClientRect() = 0;

	virtual IRect getVisibleRect() = 0;

	//
	// ������� �������������, ��� ��������������
	//
	virtual IRect getEditRect();

	//
	// ������c���� ����� � ������� pos
	//
	virtual void setPosition(const IPoint &pos) = 0;

	//
	//AcceptMessage (�������������� � �������� �� ���� ����������c��)
	//
	virtual void AcceptMessage(const Message &message);


	virtual void MouseMove(const IPoint &pos){}
	virtual void MouseDown(const IPoint &pos){}
	IPoint CorrectionMove(const IPoint &pos);
	FPoint CorrectionDp(FPoint dp);

	void SetConnect(Ryushka::HardPtr ryushka);
	WeakPtr GetConnect();
	void ResetConnect();
	virtual FPoint GetDeltaPos();
	int GetId();
	void SetId();
	int GetConnectId();

	//������� �� �����
	bool _isSelected;
	bool _visible;

	//�������� �����
	Ryushka::HardPtr SelectRyushka();

	// c���� ���������
	Ryushka::HardPtr UnSelectRyushka();

	//
	// ����������� � �������� ���c���� �� �����
	//
	Render::Texture* getTexture(std::string  name);

	bool isRyushkaOnScreen();
	void UpdateVisible(float screenX,float screenY);

	void SetRadius(int r)
	{
		_radius = r;
	}

	int GetRadius()
	{
		return _radius;
	}

	void IncRadius();

	virtual void setStringData(std::string  str){}


protected:
	//GameField *_field;
	float local_time;

	float time_scale;

	//
	// ������� ����� ������� XML c� �c��� ������������ �������
	//
	Xml::TiXmlElement* CreateXmlElement(Xml::TiXmlElement* parentXml, std::string  name);

	float _depth;
	// ������� ��������� (��� ����� � "�����")

	// ������������� �����
	std::string  _name;

private:

	int _radius; // �����c ������� �� ������� ��������� �����

	static const float SCALES[];
		// ����c����� �������� ��c����� �����

	float _scale;
		// ������� ��c����

	std::vector<std::string> _textureNames;
		// ����� ���c���, �c���������� ������, ����� ��� Upload, Release

	int _zLevel;
		// z-�������:
		// -4 c���� �������� ������,
		// +4 - c���� ������� � �����������,
		// -4 - -1 - ��c������� ��� �����,
		// +1 - +4 - ��� �����

	float _angle;
		// ���� �������� � �����c��

	bool _xMirror;
		// ����������� �� ����� �� ��c�

	//
	// ������� ��������� ��c���� �� ���������
	//
	static float GetNearestScale(float scale);

	void DepthOnDraw();
	virtual void OnDraw() = 0;

	Ryushka::WeakPtr _connect;
		// ��c���� c���� c ������

	int _id;
		// �������������

	int _id_connect;
		// ������������� id ����� ��� �c���������� c ��� c����
		// c�������� �� XML
		
		

};

class StaticImage : public Ryushka
{
protected:
	IRect _rect;
	Render::Texture *_tex;
	float u1, u2, v1, v2;
	std::string  _textureName;	


public:
//	StaticImage(Render::Texture *tex_, IPoint pos_);
	StaticImage(rapidxml::xml_node<>* xmlElement);
	virtual void OnDraw();
	virtual void Update(float dt);
	virtual void setPosition(const IPoint &pos);
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();
	virtual void SaveToXml(Xml::TiXmlElement* parentElem);
	IPoint Reduced(const IPoint& pos);
	IPoint _posCenter;
};




class RyushkaCone : public StaticImage
{
public:
	float _alpha; 
	float _angle;
	float _scale;
	float _timeShow;
	float _sRotate;

	IPoint _posScale;
	typedef enum {
		STATE_HANG,
		STATE_FALL,
		STATE_SHOW,
	} State;

	State _state; 
	FPoint _a;
	FPoint _s;
	FPoint _p;

	RyushkaCone(rapidxml::xml_node<>* xmlElement);
	virtual void OnDraw();
	virtual void Update(float dt);
	virtual void SetState(State state);


	virtual void SaveToXml(Xml::TiXmlElement* parentElem);
	virtual void MouseDown(const IPoint &pos);
	virtual void setPosition(const IPoint &pos);

};
class RyushkaList : public RyushkaCone
{
public:
	float _ampl;
	float _timerFall;
	float _f;
	float _fr;
	RyushkaList(rapidxml::xml_node<>* xmlElement);
	virtual void Update(float dt);
	virtual void MouseMove(const IPoint &pos);
	virtual void SaveToXml(Xml::TiXmlElement* parentElem);
	virtual void SetState(State state);
	float _timer;
};

class Mesh
{

public:
	typedef enum {
		BRIDGE,
		TREE,
		FLAG,
		CHAIN,
		WEB
	} State;
	Mesh(State state);
	virtual ~Mesh();
	virtual void Update(float dt);
	virtual void Draw();	

	int _rowMesh;
	int _columnMesh;
	FPoint _pStart;
	FPoint _pEnd;
	FPoint _normal;
	
	
	struct Grid{
		FPoint pos;
		float k;
		float l;
		FPoint v;
	};

	std::vector<std::vector<Grid> > _d_mesh;
	std::vector<std::vector<FPoint> > _mesh;

	Distortion *_dist;

	State _state;

	virtual void Init(const IRect& rect, const FRect& uv);
	void InitK(IPoint pStart, IPoint pEnd);
	FPoint ComputeGrid(int i, int j);
	void SetColor(Color color);

};

class Pendant
{
public:
	typedef enum{
		ONE,
		TWO
	} Type;
	Pendant(rapidxml::xml_node<> *xmlElement, Ryushka *parent, Type type = ONE);
	Pendant(IPoint pStart, IPoint pEnd,  float ampl, float  fr, float m, float c);
	void Reset();
	void MouseMove(IPoint pos, float angle, float scale);
	void Update(float dt);
	float GetDx();
	IPoint IntersectionTwoLine(IPoint pS1, IPoint pE1, IPoint pS2, IPoint pE2);
	void SaveToXml(Xml::TiXmlElement *parentElem);
	void Draw();
	IPoint _pos;
	IPoint _pStart;
	IPoint _pEnd;
	Ryushka *_parent;
	float _timer;

	Type _type;

private:
	IPoint _mousePosOld;

	float  _ampl;
	float _random_ampl;
	float _fr;
	float _m;
	float _c;
	float _speed;
	float _dx;
	float _w;
	float _countMove;
};





class Tree 
{
public:
	Tree(rapidxml::xml_node<>* xmlElement, Ryushka *parent, IPoint shift);
	~Tree();
	virtual void Update(float dt);
	virtual void OnDraw();
	virtual void MouseMove(const IPoint& pos);
	void SaveToXml(Xml::TiXmlElement *parentElem);
	void SetColor(Color color);
	
	std::string  _textureName;
	IRect _rect;
	IPoint _shift;

	Ryushka *_parent;
	Pendant *_pendant;
	Mesh  *_mesh;
	Render::Texture *_tex;
};


class RyushkaTrees : public Ryushka
{
public:
	typedef std::list<Tree*> Trees;
	typedef std::list<Tree*>::iterator Iterator;
	RyushkaTrees(rapidxml::xml_node<>* xmlElement);
	~RyushkaTrees();
	virtual void Update(float dt);
	virtual void SaveToXml(Xml::TiXmlElement *parentElem);
    virtual void setPosition(const IPoint &pos);
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();
	virtual void OnDraw();
	virtual void MouseMove(const IPoint& pos);
	void SetDepth(float depth);

private:
	Color _currentColor;
	IRect _rect;
	IPoint _editPos;
	Trees _trees;
};

class TestRyushka : public Ryushka
{
	IRect _rect;
	Render::Texture *_tex;
	float u1, u2, v1, v2;
	std::string  _textureName;

public:
	TestRyushka(rapidxml::xml_node<>* xmlElement);
	virtual void OnDraw();
	virtual void setPosition(const IPoint &pos);
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();
	virtual void SaveToXml(Xml::TiXmlElement* parentElem);
};

class EffectsStaticImage : public StaticImage
{
protected:
	EffectsContainer _effectsBefore;
	EffectsContainer _effectsAfter;
	struct EffectDesc
	{
		std::string  name;
		int x, y;
		int zOrder;
		int distance;
		ParticleEffect *eff;
	};

	std::vector<EffectDesc> _descs;

public:
	EffectsStaticImage(rapidxml::xml_node<>* xmlElement);
	virtual void OnDraw();
	virtual void Update(float dt);
	virtual void SaveToXml(Xml::TiXmlElement* parentElem);
	virtual void Upload();
	virtual void Release();
};

class EffectRyushka
	: public Ryushka
{

	struct EffectDesc
	{
		std::string  name;
		int x, y;
		int zOrder;
		int distance;
		ParticleEffect *eff;
	};

	std::vector<EffectDesc> _descs;

	IRect _rect;
	IPoint _editorRect;
	int WIDTH; 
	int HEIGHT;
protected:
	EffectsContainer _effects;
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();
	virtual IRect getEditRect();
	virtual void setPosition(const IPoint& position);
public:
	
	EffectRyushka(rapidxml::xml_node<>* xmlElement);
	virtual void OnDraw();
	virtual void Update(float dt);
	virtual void SaveToXml(Xml::TiXmlElement* parentElem);
	virtual void Upload();
	virtual void Release();
};

class RyushkaLamp
	: public StaticImage
{
	EffectsContainer _effects;
	std::string  _nameEff;
	IRect _visibleRect;
	Pendant *_pendant;
public:
	RyushkaLamp(rapidxml::xml_node<>* xmlElement);
	~RyushkaLamp();
	virtual void OnDraw();
	virtual void Update(float dt);
	virtual void SaveToXml(Xml::TiXmlElement* parentElem);
	virtual void MouseMove(const IPoint &pos);
	virtual void setPosition(const IPoint &pos);
	virtual void Upload();
	virtual void Release();
	virtual IRect getVisibleRect();
};

class TreeImage : public StaticImage
{
private:
	struct LeafDesc
	{
		std::string  texName;
		Render::Texture *tex;
		int x, y; // ��������� ������ ���c���� ����c������� ������ ���c���� ������
		float time;
		int angle; // ���c�������� ����
		int cx, cy; // ����� �������� �������� ����c������� ������ ���c���� ������
	};

	std::vector<LeafDesc> _descs;
	
public:
	TreeImage(rapidxml::xml_node<>* xmlElement);
	virtual void OnDraw();
	virtual void Update(float dt);
	virtual void SaveToXml(Xml::TiXmlElement* parentElem);
};

class FlyImage : public StaticImage
{
	float _time;
	float _speed;
	float _amplitude;

public:
	FlyImage(rapidxml::xml_node<> *xmlElement);

	virtual void OnDraw();
	virtual void Update(float dt);
	virtual void SaveToXml(Xml::TiXmlElement* parentElem);
};

class LavaImage : public StaticImage
{
	struct Lava
	{
		float _time;
		float _speed;
		float _scale;
		std::string  _texLavaName;
		Render::Texture *_texLava;
		StripEffect _strip;
		std::vector<math::Vector3> _params;
	};

	std::vector<Lava> _strips;
	
public:
	LavaImage(rapidxml::xml_node<> *xmlElement);

	virtual void OnDraw();
	virtual void Update(float dt);
	virtual void SaveToXml(Xml::TiXmlElement* parentElem);
};

class FlagImage : public StaticImage
{
	VertexBuffer _buffer;
	float _time;
	int _steps;
	float _ampl, _freq, _period;

public:
	FlagImage(rapidxml::xml_node<> *xmlElement);

	virtual void OnDraw();
	virtual void Update(float dt);
	virtual void SaveToXml(Xml::TiXmlElement *parentElem);
};

class LampImage : public StaticImage
{
	std::string  _texLightName;
	Render::Texture *_texLight;
	float _time;
	SplinePath<float> _alpha;

public:
	LampImage(rapidxml::xml_node<> *xmlElement);

	virtual void OnDraw();
	virtual void Update(float dt);
	virtual void SaveToXml(Xml::TiXmlElement *parentElem);
};

class RyushkaBridge : public Ryushka
{
public:
	RyushkaBridge(rapidxml::xml_node<>* xmlElement);
	~RyushkaBridge();
	virtual void Update(float dt);
	virtual void OnDraw();
	virtual void MouseMove(const IPoint& pos);
	virtual void SaveToXml(Xml::TiXmlElement *xmlElement);
    virtual void setPosition(const IPoint &pos);
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();

	
	std::string  _textureNameBridge;
	std::string  _textureNameBg;
	std::string  _textureNamePim;
	Render::Texture *_texBridge;
	Render::Texture *_texBg;
	Render::Texture *_texPim;

	IRect _rect;

	Pendant *_pendant;
	Mesh  *_mesh;
};

struct WebLightSettings{
	Render::Texture* texture;
	int min_alpha;
	int max_alpha;
	int start_alpha;
	int time;
	float current_alpha;
};

struct WebLight{
	Render::Texture* texture;
	Distortion* distortion;
	
	int min_alpha;
	int max_alpha;
	int start_alpha;
	int time;
	float current_alpha;
	float alpha_speed;
	float way;
};

typedef std::vector<WebLight> WebLights;
typedef std::vector<WebLightSettings> WebLightsSettings;


class MeshWithLights : public Mesh
{
public:
	MeshWithLights(State state, WebLightsSettings lights_settings);
	~MeshWithLights();
	virtual void Update(float dt);
	virtual void UpdateLights(float dt);
	void Draw();

	void Init(const IRect& rect, const FRect& uv);
	void InitK(IPoint pStart, IPoint pEnd);

	std::vector<WebLight> _lights;
};


class RyushkaWeb : public Ryushka
{
public:
	RyushkaWeb(rapidxml::xml_node<>* xmlElement);
	~RyushkaWeb();
	virtual void Update(float dt);
	virtual void OnDraw();
	virtual void MouseMove(const IPoint& pos);
	virtual void SaveToXml(Xml::TiXmlElement *xmlElement);
    virtual void setPosition(const IPoint &pos);
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();
	
	std::string  _textureNameBridge;
	Render::Texture *_texBridge;

	IRect _rect;

	Pendant *_pendant;
	MeshWithLights  *_light_mesh;
	WebLights _lights;
	WebLightsSettings _lights_settings;
};

class RyushkaLightRope : public Ryushka
{
public:
	RyushkaLightRope(rapidxml::xml_node<>* xmlElement);
	~RyushkaLightRope();
	virtual void Update(float dt);
	virtual void OnDraw();
	virtual void MouseMove(const IPoint& pos);
	virtual void SaveToXml(Xml::TiXmlElement *xmlElement);
    virtual void setPosition(const IPoint &pos);
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();
	
	std::string  _textureName;
	Render::Texture *_tex;

	IRect _rect;

	Pendant *_pendant;
	MeshWithLights  *_light_mesh;
	WebLights _lights;
	WebLightsSettings _lights_settings;
};


class RyushkaWindow : public Ryushka
{
public:
	RyushkaWindow(rapidxml::xml_node<>* xmlElement);
	virtual void Update(float dt);
	virtual void OnDraw();
	virtual void SaveToXml(Xml::TiXmlElement *xmlElement);
    virtual void setPosition(const IPoint &pos);
	virtual IRect getClientRect();
	virtual IRect getVisibleRect();
	
	std::string  _textureName;
	Render::Texture *_tex;

	std::string  _backgroundTextureName;
	Render::Texture *_backgroundTex;

	IPoint windowPos;
	int windowWidth;
	int windowHeight;

	IRect windowRect;

	IRect _rect;
};