#pragma once
#include "Spacing2D.h"

//
// ������.
// �c������ ������� �� TooltipWindow:
//  * �� �c���� ����c� AQ3Tooltip �� Call of Atlantis (aka Atlantis Quest 3);
//  * ��� ����������������;
//  * �������� ����c���� ��������� c�� (�c������� ���������� ����);
//  * �� ��������� ��������� c����� c�� (��c��������c� ����� XML);
//  * ��c������� ��c��������c� ��������������� ����� XML ("+" � ������c�����c��);
//  * ������� ���c���� �� ����������� ������ ���� c������� ������;
//  * � ����c��� ���c���� ���� �����c� �c� ���c����, � �� � ��c��;
//  * ����� ��c����� ���� ��� c���� (��c��������c� ����� XML);
//  * ��������c� ��������� �� �����������, �c�� �� ������� � �����.
//
class Tooltip
{

public:
	
	//
	// ���c������� - ������ ���c���� XML
	//
	Tooltip(Xml::TiXmlElement* xe = NULL);
	void Init(Xml::TiXmlElement* xe);
	Tooltip(rapidxml::xml_node<>* xe);
	void Init(rapidxml::xml_node<>* xe);

	//
	// �������� ���c� (���c� �����c� �� ��c��c-���������)
	// �����c� �������� �� ��������� (�� XML)
	//
	void Show(std::string idText);

	//
	// �������� ���c� c ��c������ ��������� - �c���������� ������ � ������� c������
	//
	void Show(std::string idText, float delay);

	//
	// �������� ���c� c ��c������ ��������� � ������������ �������� ������ 
	// (show_time ����� ���������c�� ���� ������ c ������ �����������c���, 0 -- �� ������������);
	//
	void Show(std::string idText, float delay, float show_time);
	void Show(Render::Text* text, float delay, float show_time);

	//
	// ������ c����� ������� ������
	//
	void HideSmoothly();

	//
	// ����� c����� ������� ������
	//
	void HideNow();

	//
	// �������� ������
	//
	void Update(float dt);

	//
	// ����c����� ������ � ������� ����
	//
	virtual void Draw();

	//
	// ��c���c� �� ������
	//
	bool IsVisible();

	//
	// ��c���c� �� ������ c ������ �����������c���
	//
	bool IsFullyVisible();

	//
	// ��������c� �� ������
	//
	bool IsHiding();

	//
	// ����� ������ ������� ���� ���������� � ��� �c�����
	//
	bool ShowTimeIsExpired();

	//
	// �������� Id ��c������� ����������� ���c��
	//
	std::string GetLastTextId();

	//
	// �c�������� ��������� ����� ������� 
	// (������ ��� ��������, �� c�������� �� �����)
	//
	void SetPosExplicitly(IPoint p);

	//
	// �������� ��������, �������� � XML
	//
	float GetOriginalDelay();

	//
	// ������� ������ ������ ����
	//
	int GetFullWidth();

	void FixMousePos();

	void SetHideOnUntap(bool b) { _hideOnUntap = b; }
    
    void SetTopAndBottomIndent(int _top, int _bottom);

protected:

	float _localTime;

	float _pulsAmplitudeBack;//��������� ���� - �������� �������� (������ ���� ����� 0.1f)

	//���������� �������� ������ (����� �������� ������� �� FreeType)
	IPoint _textInnerOffset;

	bool _alignX;

	IPoint SHIFT_LEFT;
		// ������� ������ ������ ������� (����c������� ��������� ���� ��� �����, ��������� ����)

	IPoint SHIFT_RIGHT;
		// ������� ������ ������� ������� (����c������� ��������� ���� ��� �����, ��������� ����)

	IPoint SHIFT_SHADOW;
		// c���� ����
		// �����c�������, �.�. c����� ���������������� �� XML

	float APPEAR_TIME;
		// ����� ������ ������� (� ������ � ����� c������)
		// �����c�������, �.�. c����� ���������������� �� XML

	float DELAY_TIME;
		// ����� ��������
		// �����c�������, �.�. c����� ���������������� �� XML

	float DELAY_TIME_ORIGINAL;
		// ������������ ����� ��������

	float SHOW_TIME;
		// ����� ������, 0.0f -- �� ��������������

	const int BORDER_TOP;
		// ������� �������, �� ������� ������ �� ����� ����� ��������

	const int BORDER_RIGHT;
		// ������ �������, �� ������� ������ �� ����� ����� ��������

	float _appearTimer;
		// c������ ������� ��� ��������c�

	typedef enum {
		
		STATE_HIDDEN,
			// ������ �� ����������c� � �� c�������c� :)

		STATE_DELAYING,
			// ������ �������� ������ ��� ������

		STATE_SHOWING,
			// ������ ������ ����������c� c ���������c���

		STATE_SHOW,
			// ������ ����������c� c ������ �����������c���

		STATE_HIDING,
			// ������ ������ c�������c� c ���������c���

	} State; State _state;
		// c�c������ �������

	Render::Text* _text;
		// ���c� ������

	Spacing2D _fixed;
		// ��c���� ���c��������� ���� ���c����
		// (�� �c�� c���� �� _fixed.left ������ �� ��c��������c�, c����� �� _fixed.right - ����, � �.�.

	Render::Texture* _texture;
		// ���c���� ����

	Spacing2D _indent;
		// ��c��� ���c�� ������ ���c���� (����� ���c� �� ������� �� ������� �������)

	IRect _targetRect;
		// ������������� ������ (� �������� �����������)

	SplinePath<float> _scaleSpline;
		// c����� ��������� �������

	SplinePath<float> _alphaSpline;
		// ��c���� �����

	float _delayTimer;
		// ������ ��������

	float _showTimer;
		// ������ ������, ���������c� ������ ��� SHOW_TIME > 0

	std::string _lastTextId;
		// Id ��c������� ����������� ���c��

	bool _doDrawShadow;
		// ��c����� �� ����

	Render::Texture* _shadowTexture;
		// ���c���� ����

	bool _followMouse;
		// ���������� ������ � �����

	bool _hideOnUntap; // Used in touch mode. When true, hide the tooltip if not touched.
		// If the tooltip should appear on click, set this to false.

	IPoint _pos;
		// ��������� �� ������, �c��������c� ������ ��� _followMouse == false

	FPoint _scalePos;
		// ��������� ����� �������������� ������� � ����c�������� �����������

	//
	// ������� ������� ����� �����������
	//
	float GetAlpha();

	//
	// ������� ������� ��c���� �����������
	//
	float GetScale();

	//
	// ������� ������� ����
	// 
	IPoint GetMousePos();

	//
	// ����c����� ��� ���� (��� c����������)
	// ���c���� ���� ��� ���� ��c��������c� ��� �����
	// � ���c���� ������ ���� ������ ����� Bind
	//
	virtual void DrawBackground();
};
