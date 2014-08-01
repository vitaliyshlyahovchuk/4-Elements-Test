#pragma once

#include "DynamicArray2D.h"
#include "Render/VertexBuffer.h"

//
// ����� ��� ���c���� - ��������� ���������
// ���c���� �� ������ ������������� ��c�� � �����������
// �������� ���������� ����� ����������c� c����,
// � ����� ���������� �� �����, � ����c�� �c� ��� �� �����.
//
class TextureMesh
{
private:

	DynamicArray2D<FPoint> _knotPositions;
		// �������� ���������� ����� c���� (���������)

	DynamicArray2D<FPoint> _knotShifts;
		// c����� �������� ��������� ����� c���� (��� c��c������ �c�������)

	DynamicArray2D<BYTE> _knotAlphas;
		// ����� �����

	int _columns;
		// ������c��� ����� �� ������

	int _rows;
		// ������c��� ����� �� ��c���

	float _startU;
		// ����������� ���������� "u" ����������� � ������������ ����c�� ���c����

	float _startV;
		// ����������� ���������� "v" ����������� � ������������ ����c�� ���c����

	float _endU;
		// ���c�������� ���������� "u" ����������� � ������������ ����c�� ���c����

	float _endV;
		// ���c�������� ���������� "v" ����������� � ������������ ����c�� ���c����

	Render::Texture* _texture;
		// ��������� �� ����������� ���c����
    
    VertexBufferIndexed _buff;

public:

	//
	// ������� ���c����, �� c������ ��c��� ��������� ���c���� �� ����������� � ���������,
	// � ����� ����� �������, ������ ������� ����� ������ ���� ���c���� ����� ���������c�.
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
	// �c�������� c���� ����
	//
	void SetKnotShift(int column, int row, FPoint shift) {
		_knotShifts[column][row] = shift;
	}

	//
	// �c�������� ����� ����
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
	// ���������� ������c��� c������� �����;
	// ��������� ����� �� 0 �� ����� ��c�� ������������.
	//
	int Columns() {
		return _columns;
	}

	//
	// ���������� ������c��� ����� �����;
	// ��������� ����� �� 0 �� ����� ��c�� ������������.
	//
	int Rows() {
		return _rows;
	}

	//
	// ������� c���� ���c���� �� ����� c ����� ������ ����� � ����� pos
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
	// "���c�����" ������ ���c����. ���c���� ������ ���� ������ �� �������
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
		// ��� ��������� c� c�������� cc����
};
