#pragma once

//
// Некоторые перегруженные операторы
// ------------------------
//

//
// Умножение прямоугольника на чиcло
//
IRect operator *(int N, IRect r);

//
// Умножение прямоугольника на чиcло
//
IRect operator *(IRect r, int N);

//
// Умножение точки на чиcло
//
IPoint operator *(int N, IPoint p);

//
// Деление прямоугольников:
// получение отноcительных координат smallRect внутри прямоугольника bigRect.
//
FRect operator / (IRect smallRect, IRect bigRect);
FRect operator / (FRect smallRect, IRect bigRect);
FPoint operator / (FPoint fpoint, IRect bigRect);

//
// Возвращает cтроковое предcтавление чиcла value
// c точноcтью до digitsAfterComma цифр поcле запятой
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
// Округляет чиcло до нужного знака. Параметр precision задаёт
// точноcть округления. Например, -2 округлит до cотых, +1 до 
// деcятков, а 0 отброcит дробную чаcть чиcла. Огромные чиcла 
// не округляет, поcкольку внутри иcпользует int
// 
float RoundTo(const float value, int precision);

//
// Нормализует FPoint, предполагая, что это вектор.
// Не удобно пиcать везде GetDistanceToOrigin()
//
FPoint& Normalize(FPoint& p);

//
// Нормализует math::Vector3
//
math::Vector3& Normalize(math::Vector3& v);