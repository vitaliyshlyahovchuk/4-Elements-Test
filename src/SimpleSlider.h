#pragma once
#include "Render.h"

//
// ���cc ����c������� - c������, ������� ����c�������c� ��� ���c���.
// ������ ���������� SimpleSlider
//
class SimpleSlider
{

public:

	/// ���c�������
	SimpleSlider(IPoint pos, int size, bool isVertical, int begunWidth, int begunHeight);

	/// ������� ����c�������� ��������� ������� (�� 0 �� 1)
	float GetFactor();

	/// �c�������� ����c�������� ��������� ������� (�� 0 �� 1)
	void SetFactor(float factor);

	/// ����c����� c������
	void Draw();

	/// ���������� ������� ����
	bool MouseDown(const IPoint &mousePos);

	/// ���������� �������� ����
	bool MouseMove(const IPoint &mousePos);

	/// ���������� ������� ����
	void MouseUp(const IPoint &mousePos);

	/// �c�������� ����� c�������
	void SetSize(int size);

	/// �c�������� ������� c�������
	void SetPosition(IPoint pos);

	/// �c�������� ������c �����c�
	void SetLabelPrefix(std::string  labelPrefix);

	/// �c�������� �����c�
	void SetLabel(std::string  label);

	/// ������� ������� �������
	IPoint GetBegunPos();

	bool GetIsMouseDown();
	bool GetIsMouseOnSlider();
	void MouseWheel(int delta);
	float GetSize();
private:

/// ������������/��������������
	bool _isVertical;

	/// ���������� ������ ������� ���� �����, ����� ������� ����������c� �������
	IPoint _pos;

	/// ����� �����
	int _size;

	/// ��c��� �������
	int _begunHeight;

	/// ������ �������
	int _begunWidth;

	/// ������� ������� �������
	int _begunPos;

	/// ������ �� ������
	bool _isMouseDown;

	/// �������c� �� ���c�� ���� ��� c��������
	bool _isMouseOnSlider;

	/// ������c �����c�
	std::string  _labelPrefix;

	/// �����c�
	std::string  _label;
	//
	// �c�� ���� �������� �� c������
	//
	bool IsMouseOnSlider(const IPoint &mousePos);

	//
	// �c�������� ������� ������� �� ����������� ����
	//
	void SetBegunPos(const IPoint &mousePos);
};