#pragma once

//
// ������ ����������� ��cc��� - ������� ��������c���� ��cc��� ��.
// ���c��������:
// 1. ��������� ��������� ��� c������� � ��������� �������.
// 2. �������� ���������� ��� �����c��������.
// 3. �������� � ������ ����� size() ������� ��cc���.
// 4. ����������c��� �c���������� ������ ��� ������ �� ����c�� ������c��.
//
// �c�����������:
// ���c��
// int *a = new int[10];
// a[5] = 7;
// delete[] a;
// a = new int[5];
// a[4] = 5;
// delete[] a;
//
// ����� ��c���
// DynamicArray<int> a(10);
// a[5] = 7;
// a.resize(5);
// a[4] = 5;
//
template<typename T>
class DynamicArray {
private:

	T *_data;
		// ��������� �� ��cc��, �������� � ���� ���cc.

	int _size;
		// ������� ������ ��cc���.

public:

	//
	// ���c������� �� ��������� - c����� ��cc�� ����� 0.
	//
	DynamicArray()
		: _data(NULL)
		, _size(0)
	{}

	//
	// �������� ��cc��� ����� size, ������������ ���������� value.
	// �c�� value �� ������� - ��cc�� ���c�� ��������c�.
	//
	DynamicArray(int size, T value = T())
		: _data(NULL)
		, _size(0)
	{
		resize(size, value);
	}

	//
	// ���c������� ����������� ��� ������ �������������.
	//
	DynamicArray(const DynamicArray &copy)
		: _data(NULL)
		, _size(0)
	{
		*this = copy;
	}

	//
	// �������� ���c�������� ��� ������������.
	//
	DynamicArray & operator = (const DynamicArray &copy) {
		resize(copy._size);
		for (int i = 0; i < _size; i++) {
			_data[i] = copy._data[i];
		}
		return *this;
	}

	//
	// �������� c������ c���������� ��cc���, �c������� ������ �������,
	// ���������� ��� ���������� value.
	// �c�� value �� ������� - ��cc�� ���c�� ��������c�.
	//
	void resize(int size, T value = T()) {
		delete[] _data; // �� c����������� C++ NULL ������� �����
		if (size == 0) {
			_data = NULL;
		} else {
			Assert(size > 0);
			_data = new T[size];
		}
		_size = size;
		for (int i = 0; i < size; i++) {
			_data[i] = value;
		}
	}

	//
	// �������� �����c�������� ��cc��� c ��������� ���������.
	//
	T & operator [] (int pos) {
		Assert(0 <= pos && pos < _size);
		return _data[pos];
	}

	//
	// �������� �����c�������� ���c�������� ��cc���.
	//
	const T & operator [] (int pos) const {
		Assert(0 <= pos && pos < _size);
		return _data[pos];
	}

	//
	// ��������� ������� ��cc���.
	//
	int size() const {
		return _size;
	}

	//
	// �c���������� �c���������� ������.
	//
	~DynamicArray() {
		delete[] _data;
	}
};
