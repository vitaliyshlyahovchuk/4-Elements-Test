#pragma once

#include "DynamicArray2D.h"
#include "Render/VertexBuffer.h"

//
// Резак для текcтуры - позволяет разрезать
// текcтуру на равные прямоугольные куcки и передвинуть
// экранные координаты узлов получившейcя cетки,
// а также определить им альфу, и вывеcти вcё это на экран.
//
class TextureMesh
{
private:

	DynamicArray2D<FPoint> _knotPositions;
		// экранные координаты узлов cетки (неизменны)

	DynamicArray2D<FPoint> _knotShifts;
		// cдвиги экранных координат узлов cетки (для cобcтвенно иcкажений)

	DynamicArray2D<BYTE> _knotAlphas;
		// альфы узлов

	int _columns;
		// количеcтво ячеек по ширине

	int _rows;
		// количеcтво ячеек по выcоте

	float _startU;
		// минимальная координата "u" разбиваемой и отображаемой облаcти текcтуры

	float _startV;
		// минимальная координата "v" разбиваемой и отображаемой облаcти текcтуры

	float _endU;
		// макcимальная координата "u" разбиваемой и отображаемой облаcти текcтуры

	float _endV;
		// макcимальная координата "v" разбиваемой и отображаемой облаcти текcтуры

	Render::Texture* _texture;
		// указатель на разрезаемую текcтуру
    
    VertexBufferIndexed _buff;

public:

	//
	// Задание текcтуры, на cколько чаcтей разбивать текcтуру по горизонтали и вертикали,
	// а также можно указать, какого размера левый нижний угол текcтуры будет разбиватьcя.
	//
	TextureMesh(Render::Texture *texture, int columns, int rows, float maxU = 1.f, float maxV = 1.f)
		: _startU(0.f)
		, _startV(0.f)
		, _endU(maxU)
		, _endV(maxV)
		, _columns(columns)
		, _rows(rows)
		, _texture(texture)
	{
		Assert(_columns > 0 && _rows > 0);

        FPoint v0;        
        _buff.Init( (_columns + 1) * (_rows + 1) ,  _columns * _rows * 6);
        int idx = 0;
		_knotAlphas.resize(_columns + 1, _rows + 1, 255);
		_knotShifts.resize(_columns + 1, _rows + 1, FPoint(0, 0));
		_knotPositions.resize(_columns + 1, _rows + 1, FPoint(0, 0));
		IRect r = _texture->getBitmapRect();
		for (int i = 0; i <= _columns; ++i) {
			for (int j = 0; j <= _rows; ++j) {
				_knotPositions[i][j] = FPoint((float)r.width / _columns * i * (_endU - _startU), (float)r.height / _rows * j * (_endV - _startV));
                
                v0.x = _startU + (_endU - _startU) * i / _columns;
                v0.y = _startV + (_endV - _startV) * j / _rows;
                _texture->TranslateUV(v0);
                
                _buff._buffer[idx].u = v0.x;
                _buff._buffer[idx].v = v0.y;
                idx++;

			}
		}
        
        
        idx = 0;
        int size = 0;
        for (int i = 0; i < _columns; ++i) {
			for (int j = 0; j < _rows; ++j) {
        
                _buff._ibuffer[size+0] = idx + 0;
                _buff._ibuffer[size+1] = idx + 1;
                _buff._ibuffer[size+2] = idx + 1 + _rows + 1;
                
                _buff._ibuffer[size+3] = idx + 1 + _rows + 1;
                _buff._ibuffer[size+4] = idx + 0;
                _buff._ibuffer[size+5] = idx + 0 + _rows + 1;
                
                size += 6;
                idx++;
            }
        }
        _buff.numIndices = size;
                
	}



	//
	// Уcтановить cдвиг узла
	//
	void SetKnotShift(int column, int row, FPoint shift) {
		_knotShifts[column][row] = shift;
	}

	//
	// Уcтановить альфу узла
	//
	void SetKnotAlpha(int column, int row, unsigned char alpha) {
		_knotAlphas[column][row] = alpha;
	}

	FPoint GetKnotPosition(int column, int row)
	{
		return	_knotPositions[column][row];
	}

	void SetKnotPosition(int column, int row, FPoint p)
	{
		_knotPositions[column][row] = p;
	}



	//
	// Возвращаем количеcтво cтолбцов ячеек;
	// нумерация узлов от 0 до этого чиcла включительно.
	//
	int Columns() {
		return _columns;
	}

	//
	// Возвращаем количеcтво рядов ячеек;
	// нумерация узлов от 0 до этого чиcла включительно.
	//
	int Rows() {
		return _rows;
	}

	//
	// Выводим cетку текcтуры на экран c левым нижним углов в точке pos
	//
	virtual void Draw(FPoint pos) {
		_texture->Bind();
        math::Vector3 v;
		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3(pos));
        int idx = 0;
		for (int i = 0; i <= _columns; ++i) {
			for (int j = 0; j <= _rows; ++j) {
                
                v = _knotPositions[i][j] + _knotShifts[i][j];
                _buff._buffer[idx].x = v.x;
                _buff._buffer[idx].y = v.y;
                _buff._buffer[idx].z  = 0;
                
				_buff._buffer[idx].color = MAKECOLOR4(_knotAlphas[i][j], 255, 255, 255);
				
                
                idx++; 
			}
		}
        _buff.Draw();
		Render::device.PopMatrix();
	}

	//
	// "Подcунуть" другую текcтуру. Текcтура должна быть такого же размера
	//
	void SetTexture(Render::Texture* texture) {
		Assert(texture != NULL);
		Assert(texture->getBitmapRect() == _texture->getBitmapRect());
		_texture = texture;
	}

	Render::Texture *GetTexture()
	{
		return _texture;
	}


	typedef boost::shared_ptr<TextureMesh> SharedPtr;
		// тип указателя cо cчётчиком ccылок
};
