#pragma once

/// ������ ����������� ������� - ������� ������������ ������� ��.
/// ������������:
/// 1. ��������� ��������� ��� �������� (� ������� �� ������������� �������).
/// 2. �������� ���������� ��� �������������� (� ������� �� �������� � std::vector).
/// 3. ����������� ����������� � ��������� Visual Studio (� ������� �� std::vector).
/// 4. ������ � size() ����� �� ������, ��� ��� ��� ������� �� �������
/// ����� ������������ ��� int ��� �������������� ����������� (� ������� �� std::vector).
///
/// �������������:
/// ������
/// int a[10];
///
/// ����� ���������� ��������� ���
/// StaticArray<int, 10> a;
///
/// ��� � ��� ��������.


extern int StaticArray_NEXT_ID;

template<typename T, int SIZE>
class StaticArray {
private:
	int id;
	/// ������, �������� � ���� �����.
	T _data[SIZE];

public:

	/// ����������� - ��������� ��������� �������.
	StaticArray() 
		: id(StaticArray_NEXT_ID++)
	{
		for (int i = 0; i < SIZE; i++) {
			_data[i] = T();
		}
	}

	/// ����������� - ������� ���������� �������� ��������� �������.
	StaticArray(T value) 
		: id(StaticArray_NEXT_ID++)
	{
		for (int i = 0; i < SIZE; i++) {
			_data[i] = value;
		}
	}

	/// �������� �������������� ������� � ��������� ���������.
	T & operator [] (int pos) {
		if(0 <= pos && pos < SIZE)
		{
			return _data[pos];
		}else{
			Halt("Problem in StaticArray:id = " +  utils::lexical_cast(id));
		}
		return _data[pos];
	}

	/// �������� �������������� ������������ �������.
	const T & operator [] (int pos) const {
		if(0 <= pos && pos < SIZE)
		{
			return _data[pos];
		}else{
			Halt("Problem in StaticArray:id = " +  utils::lexical_cast(id));
		}
		return _data[pos];
	}

	/// ������� ��������� ������� ������� � ���������.
	/// �� ������� ���������, ��������� ������ �������
	/// ������ �������� � ������ �������������,
	/// ��������� ������� �� ���� �������.
	int size() const {
		return SIZE;
	}
};
