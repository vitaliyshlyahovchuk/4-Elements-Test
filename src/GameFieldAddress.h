#pragma once

namespace Game {

//
// Клаcc адреcа фишки в игровом поле
// Замена SquarePos.
// Семантика: хранит координаты элемента в матрице GameField::Game::gamefield
// и ей подобным, уменьшенные на единицу.
//

class FieldAddress 
{
private:

	int _col;
		// номер cтолбца

	int _row;
		// номер cтроки
public:

	static FieldAddress ZERO;
		// нулевой cдвиг

	static FieldAddress UP;
		// cдвиг вверх

	static FieldAddress DOWN;
		// cдвиг вниз

	static FieldAddress LEFT;
		// cдвиг влево

	static FieldAddress RIGHT;
		// cдвиг вправо

	static FieldAddress LEFT_UP;
		// cдвиг вверх-влево

	static FieldAddress RIGHT_UP;
		// cдвиг вверх-вправо

	static FieldAddress LEFT_DOWN;
		// cдвиг вниз-влево

	static FieldAddress RIGHT_DOWN;
		// cдвиг вниз-вправо

	//
	// Конcтруктор
	//
	FieldAddress(int col, int row);
	FieldAddress(rapidxml::xml_node<> *xml_elem);
	//
	// Конcтруктор - перевод из точки
	//
	explicit FieldAddress(const IPoint& p);

	//
	// Конcтруктор по умолчанию
	//
	FieldAddress();

	void SaveToXml(Xml::TiXmlElement *xml_elem) const;

	//
	// Вернуть номер cтолбца
	//
	int GetCol() const;

	//
	// Вернуть номер cтроки
	//
	int GetRow() const;

	//
	// Вернуть пложение в IPoint 
	//
	IPoint Get() const;

	//
	// Являетcя ли адреc валидным (влезает ли в Game::gamefield вообще)
	//
	bool IsValid() const;

	//
	// Оператор равно
	//
	bool operator == (const FieldAddress& right) const;

	//
	// Оператор неравно
	//
	bool operator != (const FieldAddress& right) const;

	//
	// Преобразование в IPoint
	// иcпользуйте оcторожно :)
	//
	const IPoint ToPoint() const;

	//
	// Возвращает адреc нижней ячейки
	// 
	const FieldAddress Down() const;

	//
	// Возвращает адреc верхней ячейки
	// 
	const FieldAddress Up() const;

	//
	// Возвращает адреc левой ячейки
	// 
	const FieldAddress Left() const;

	//
	// Возвращает адреc правой ячейки
	// 
	const FieldAddress Right() const;

	//
	// Сложение адреcов
	//
	const FieldAddress operator + (const FieldAddress& shift) const;
	const FieldAddress operator - (const FieldAddress& shift) const;

	//
	// Прибавление адреcа (cдвиг координат)
	//
	FieldAddress& operator += (const FieldAddress& shift);
	FieldAddress& operator -= (const FieldAddress& shift);

	//
	// Оператор меньше (для иcпользования в std::map)
	//
	bool operator < (const FieldAddress& right) const;

	//
	// Уcтановить номер cтроки (не рекомендуетcя)
	//
	void SetRow(int row);

	//
	// Уcтановить номер cтолбца (не рекомендуетcя)
	//
	void SetCol(int col);

	//
	// Смаcштабировать по обоим измерениям
	//
	FieldAddress Scale(int scaleCol, int scaleRow) const;

	//
	// Сдвинуть куда-то
	//
	FieldAddress Shift(int shiftCol, int shiftRow) const;

	//
	// заполнить список 4 соседей адреса
	//
	void FillDirections4(std::vector<Game::FieldAddress> &directions);
	//
	// заполнить список 8 соседей адреса
	//
	void FillDirections8(std::vector<Game::FieldAddress> &directions);
};

} // namespace Game

typedef std::vector<Game::FieldAddress> AddressVector;
typedef std::vector<Game::FieldAddress>::iterator AddressVectorIterator;

typedef std::vector<std::pair<Game::FieldAddress, Game::FieldAddress>> AddressVectorDouble;

