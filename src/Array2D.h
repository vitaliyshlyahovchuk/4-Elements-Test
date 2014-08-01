#pragma once

#include "GameFieldAddress.h"

template<class T> class Array2D
{
private:
	T _default;
	//Указатель на рабочую область. В нем всего "строки*стольбцы" элементов
	T *_ptr;
	//Дефолтовая колонка если при выборе колонки выбрали несуществующую, а далее следует явный выбор строки!
	//В идеале нужно избавиться от таких непроверенных явных обращений в коде TODO (Но это нагромоздит его)
	T *_default_col;
	//Список прямых указателей на столбцы для прямого обращения без пересчета!
	T **_cols;

	int _row_count;
	int _col_count;
	//Дефолтовое смещение при обращении к элементам TODO: возможно не потребуется - время покажет
	int _offset_x;
	int _offset_y;
	
public:
	Array2D()
		:_ptr(NULL),
		_cols(NULL),
		_default_col(NULL),
		_row_count(0),
		_col_count(0)
	{}

	~Array2D()
	{
		Release();
	}

	void Release()
	{
		if(_cols)
		{
			delete [] _cols;
			_cols = NULL;
		}
		if(_ptr)
		{
			delete[] _ptr;
			_ptr = NULL;
		}
		if(_default_col)
		{
			delete[] _default_col;
			_default_col = NULL;
		}
		_row_count = 0;
		_col_count = 0;
	}

	void Init(int col_count, int row_count, const T &default_value, int offset_x, int offset_y)
	{
		Release();
		_default = default_value;
		_row_count = row_count;
		_col_count = col_count;
		_offset_x = offset_x;
		_offset_y = offset_y;
		int count = _row_count*_col_count;
		_ptr = new T[count];

		for(int i = 0; i < count; i++)
		{
			_ptr[i] = _default;
		}
	
		//Инициализируем быстрые ссылки на начало столбцов
		_cols = new T*[_col_count];
		for(int i = 0; i < _col_count; i++)
		{
			_cols[i] = &_ptr[i*_row_count];
		}
	
		//Создаем одну дефолтовую колонку и заполняем дефолтовыми значениями
		_default_col = new T[_row_count];
		for(int i = 0; i < _row_count; i++)
		{
			_default_col[i] = _default;
		}
	}
	
	T* operator [](const int &col) const
	{
		if( col < 0 || _col_count <= col)
		{
			return _default_col;
		}
		return _cols[col];
	}
		
	T& operator [](const Game::FieldAddress &address)
	{
		int col = address.GetCol() + _offset_x;
		int row = address.GetRow() + _offset_y;
		if( row < 0 || _row_count <= row)
		{
			return _default;
		}
		if( col < 0 || _col_count <= col)
		{
			return _default;
		}
		return _cols[col][row];
	}

	const T& operator [](const Game::FieldAddress &address) const
	{
		int col = address.GetCol() + _offset_x;
		int row = address.GetRow() + _offset_y;
		if( row < 0 || _row_count <= row)
		{
			return _default;
		}
		if( col < 0 || _col_count <= col)
		{
			return _default;
		}
		return _cols[col][row];
	}

	T& operator [](const IPoint &index)
	{
		int col = index.x + _offset_x;
		int row = index.y + _offset_y;
		if( row < 0 || _row_count <= row)
		{
			return _default;
		}
		if( col < 0 || _col_count <= col)
		{
			return _default;
		}
		return _cols[col][row];
	}

	const T& operator [](const IPoint &index) const
	{
		int col = index.x + _offset_x;
		int row = index.y + _offset_y;
		if( row < 0 || _row_count <= row)
		{
			return _default;
		}
		if( col < 0 || _col_count <= col)
		{
			return _default;
		}
		return _cols[col][row];
	}

	Array2D<T> & operator = (const Array2D<T> &copy)
	{
		//Init(copy._col_count, copy._row_count, copy._default, copy._offset_x, copy._offset_y);
		{// более быстрый Init
			_col_count = copy._col_count;
			_row_count = copy._row_count;
			_default = copy._default;
			_offset_x = copy._offset_x;
			_offset_y = copy._offset_y;
			
			int count = _row_count*_col_count;
			_ptr = new T[count];
			
			//Инициализируем быстрые ссылки на начало столбцов
			_cols = new T*[_col_count];
			for(int i = 0; i < _col_count; i++)
			{
				_cols[i] = &_ptr[i*_row_count];
			}
			
			//Создаем одну дефолтовую колонку и заполняем дефолтовыми значениями
			_default_col = new T[_row_count];
			for(int i = 0; i < _row_count; i++)
			{
				_default_col[i] = _default;
			}
		}
		
		memcpy(_ptr, copy._ptr, _row_count*_col_count*sizeof(T));
		return *this;
	}
	
	int Height() const
	{
		return _row_count;
	}
	
	int Width() const
	{
		return _col_count;
	}
};