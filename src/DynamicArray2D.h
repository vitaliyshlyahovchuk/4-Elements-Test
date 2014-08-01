#pragma once

#include "DynamicArray.h"

//
// ������ ���������� ��������c���� ��cc���, ������� ��cc��� �� ���������� �� ��cc���.
// � ��c���c��, �����cc��� ����� ���� ������� �������.
// ���c��������:
// 1. ��������� ��������� ��� c������� � ��������� �������.
// 2. �������� ���������� ��� �����c��������.
// 3. �������� � ������ ����� size() ������� ��cc���.
// 4. ����������c��� �c���������� ������ ��� ������ �� ����c�� ������c��.
//
// �c�����������:
// DynamicArray2D<int> a(10, 10);
// a[5][5] = 7;
// a.resize(15, 15);
// a[10][10] = 11;
//
// �������� ������������ ��cc���:
// DynamicArray2D<int> a(10);
// for (int i = 0; i < 10; i++) {
//    a[i].resize(i);
// }
//
template<typename T>
class DynamicArray2D {
private:

	typedef DynamicArray<T> Subtype;
		// ��� �����cc����. ��������� ��������� ��� c������� DynamicArray2D
		// ���c��������c� ���������� ��������� DynamicArray.

	Subtype *_data;
		// ��������� �� ���������� ��cc�� �������� ������.

	int _size;
		// ������� ������ ��cc��� �������� ������.

public:

	//
	// ���c������� �� ��������� - c����� ��cc�� ����� 0.
	//
	DynamicArray2D()
		: _data(NULL)
		, _size(0)
	{}

	//
	// �������� ��cc��� ������� size x size2, ������������ ���������� value.
	// �c�� value �� ������� - ��cc�� ���c�� ��������c�.
	// �c�� size2 �� ������� - ������ ������������c� � ������� �����cc���
	// ������ �c����������� ��������, � ���� �� 0.
	//
	DynamicArray2D(int size, int size2 = 0, T value = T())
		: _data(NULL)
		, _size(0)
	{
		resize(size, size2, value);
	}

	//
	// ���c������� ����������� ��� ������ �������������.
	//
	DynamicArray2D(const DynamicArray2D &copy)
		: _data(NULL)
		, _size(0)
	{
		*this = copy;
	}

	//
	// �������� ���c�������� ��� ������������.
	//
	DynamicArray2D & operator = (const DynamicArray2D &copy) {
		resize(copy._size); // c������c� ��c��� ��������� ��cc���
		for (int i = 0; i < _size; i++) {
			_data[i] = copy._data[i]; // ��������c� �����������
		}
		return *this;
	}

	//
	// �������� c������ c���������� ��cc���, �c������� ������ �������,
	// ���������� ��� ���������� value.
	// �c�� value �� ������� - ��cc�� ���c�� ��������c�.
	// �c�� size2 �� ������� - ������ ������������c� � ������� �����cc���
	// ������ �c����������� ��������, � ���� �� 0.
	//
	void resize(int size, int size2 = 0, T value = T()) {
		delete[] _data; // �� c����������� C++ NULL ������� �����
		if (size == 0) {
			_data = NULL;
		} else {
			Assert(size > 0);
			_data = new Subtype[size];
		}
		_size = size;
		for (int i = 0; i < size; i++) {
			_data[i].resize(size2, value);
		}
	}

	//
	// �������� �����c�������� ��cc��� c ��������� ���������.
	//
	Subtype & operator [] (int pos) {
		Assert(0 <= pos && pos < _size);
		return _data[pos];
	}

	//
	// �������� �����c�������� ���c�������� ��cc���.
	//
	const Subtype & operator [] (int pos) const {
		Assert(0 <= pos && pos < _size);
		return _data[pos];
	}

	//
	// ��������� ������� ��cc��� �������� ������.
	//
	int size() const {
		return _size;
	}

	//
	// �c���������� �c���������� ������.
	//
	~DynamicArray2D() {
		delete[] _data;
	}
};
