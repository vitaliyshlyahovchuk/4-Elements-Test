#pragma once

#if defined(_DEBUG) && defined(ENGINE_TARGET_WIN32)
    #define CARD_EDITOR
#endif

namespace Card {

class Container;

class MapItem {
public:
	MapItem();
	virtual ~MapItem() {};
	virtual void init(Container* parent, rapidxml::xml_node<>* info);
	virtual void Draw(const FPoint& shift);
	virtual void Update(float dt) = 0;
	virtual void MouseDown(const FPoint& mouse_position, bool &capture);
	virtual void MouseMove(const FPoint& mouse_position, bool &capture);
	virtual void MouseUp(const FPoint& mouse_position, bool &capture);
	virtual void AcceptMessage(const Message &message);
	// объект должен рисоваться сверху, над затенением карты
	virtual bool isDrawUp() const;
	// максимальный обрамляющий прямоугольник
	const IRect& getRect() const { return rect; }
	// Проверка на необходимость отрисовки
	virtual bool needDraw(const IRect &draw_rect) const;
	// Проверка на необходимость обновления (область обновления чуть чуть больше области рисования)
	virtual bool needUpdate(const IRect &update_rect) const;
	// положение элемента относительно родителя
	const FPoint& getPosition() const { return position; }
	// задание полжоения элемента относительно родителя
	virtual void setPosition(const FPoint new_pos);
	// идентификатор элемента
	const std::string& GetName() const { return name; }
	// тип элемента
	const std::string& GetType() const { return type; }

	static bool CompareByBound(MapItem *item1, MapItem *item2);

protected:
	//Тип объекта
	std::string type;
	// идентификатор ~ имя объекта
	std::string name;
	// к кому прицеплен объект
	Container* parent;
	// положение объекта
	FPoint position;
	// максимальный обрамляющий прямоугольник
	IRect rect;
	//Мнимый "y" для сортировки в отриcовке
	float yBound;

	static IRect unionRect(const IRect& r1, const IRect& r2) {
		IPoint min(
			math::min(r1.x, r2.x),
			math::min(r1.y, r2.y)
			);
		return IRect(min.x, min.y,
			math::max(r1.x + r1.width, r2.x + r2.width) - min.x,
			math::max(r1.y + r1.height, r2.y + r2.height) - min.y);
	}


// Встроенный редактор карты
#ifdef CARD_EDITOR
public:
	// для сохранения
	bool edited;
	rapidxml::xml_node<>* info;
	// изменяем позицию
	bool change_position;
	FPoint prev_mouse_pos;
private:
#endif

};

} // namespace