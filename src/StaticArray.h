#pragma once

/// Шаблон одномерного массива - аналога статического массива Си.
/// Обеспечивает:
/// 1. Зануление элементов при создании (в отличие от классического массива).
/// 2. Контроль диапазонов при индексировании (в отличие от массивов и std::vector).
/// 3. Отображение содержимого в отладчике Visual Studio (в отличие от std::vector).
/// 4. Выдачу в size() числа со знаком, так что для прохода по массиву
/// можно использовать тип int без предупреждений компилятора (в отличие от std::vector).
///
/// Использование:
/// Вместо
/// int a[10];
///
/// нужно переменную объявлять как
/// StaticArray<int, 10> a;
///
/// вот и все различия.


extern int StaticArray_NEXT_ID;

template<typename T, int SIZE>
class StaticArray {
private:
	int id;
	/// Массив, обёрнутый в этот класс.
	T _data[SIZE];

public:

	/// Конструктор - зануление элементов массива.
	StaticArray() 
		: id(StaticArray_NEXT_ID++)
	{
		for (int i = 0; i < SIZE; i++) {
			_data[i] = T();
		}
	}

	/// Конструктор - задание одинаковых значений элементов массива.
	StaticArray(T value) 
		: id(StaticArray_NEXT_ID++)
	{
		for (int i = 0; i < SIZE; i++) {
			_data[i] = value;
		}
	}

	/// Оператор индексирования массива с проверкой диапазона.
	T & operator [] (int pos) {
		if(0 <= pos && pos < SIZE)
		{
			return _data[pos];
		}else{
			Halt("Problem in StaticArray:id = " +  utils::lexical_cast(id));
		}
		return _data[pos];
	}

	/// Оператор индексирования константного массива.
	const T & operator [] (int pos) const {
		if(0 <= pos && pos < SIZE)
		{
			return _data[pos];
		}else{
			Halt("Problem in StaticArray:id = " +  utils::lexical_cast(id));
		}
		return _data[pos];
	}

	/// Функция получения размера массива в элементах.
	/// Не слишком актуальна, поскольку размер массива
	/// всегда известен в момент использования,
	/// поскольку следует из типа массива.
	int size() const {
		return SIZE;
	}
};
