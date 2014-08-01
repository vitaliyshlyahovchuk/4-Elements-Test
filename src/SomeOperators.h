#pragma once

//
// ��������� ������������� ���������
// ------------------------
//

//
// ��������� �������������� �� ��c��
//
IRect operator *(int N, IRect r);

//
// ��������� �������������� �� ��c��
//
IRect operator *(IRect r, int N);

//
// ��������� ����� �� ��c��
//
IPoint operator *(int N, IPoint p);

//
// ������� ���������������:
// ��������� ����c�������� ��������� smallRect ������ �������������� bigRect.
//
FRect operator / (IRect smallRect, IRect bigRect);
FRect operator / (FRect smallRect, IRect bigRect);
FPoint operator / (FPoint fpoint, IRect bigRect);

//
// ���������� c�������� ����c�������� ��c�� value
// c �����c��� �� digitsAfterComma ���� ��c�� �������
//
static std::string FloatToString(float value, int digitsAfterComma = 2) {
	const int SIZE = 32;
	char buf[SIZE];
	char format[SIZE];
	sprintf_s(format, SIZE, "%%0.%df", digitsAfterComma);
	sprintf_s(buf, SIZE, format, value);
	return buf;
}

//
// ��������� ��c�� �� ������� �����. �������� precision �����
// �����c�� ����������. ��������, -2 �������� �� c����, +1 �� 
// ��c�����, � 0 �����c�� ������� ��c�� ��c��. �������� ��c�� 
// �� ���������, ��c������ ������ �c�������� int
// 
float RoundTo(const float value, int precision);

//
// ����������� FPoint, �����������, ��� ��� ������.
// �� ������ ��c��� ����� GetDistanceToOrigin()
//
FPoint& Normalize(FPoint& p);

//
// ����������� math::Vector3
//
math::Vector3& Normalize(math::Vector3& v);