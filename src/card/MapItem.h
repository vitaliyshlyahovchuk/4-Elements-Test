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
	// ������ ������ ���������� ������, ��� ���������� �����
	virtual bool isDrawUp() const;
	// ������������ ����������� �������������
	const IRect& getRect() const { return rect; }
	// �������� �� ������������� ���������
	virtual bool needDraw(const IRect &draw_rect) const;
	// �������� �� ������������� ���������� (������� ���������� ���� ���� ������ ������� ���������)
	virtual bool needUpdate(const IRect &update_rect) const;
	// ��������� �������� ������������ ��������
	const FPoint& getPosition() const { return position; }
	// ������� ��������� �������� ������������ ��������
	virtual void setPosition(const FPoint new_pos);
	// ������������� ��������
	const std::string& GetName() const { return name; }
	// ��� ��������
	const std::string& GetType() const { return type; }

	static bool CompareByBound(MapItem *item1, MapItem *item2);

protected:
	//��� �������
	std::string type;
	// ������������� ~ ��� �������
	std::string name;
	// � ���� ��������� ������
	Container* parent;
	// ��������� �������
	FPoint position;
	// ������������ ����������� �������������
	IRect rect;
	//������ "y" ��� ���������� � ����c����
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


// ���������� �������� �����
#ifdef CARD_EDITOR
public:
	// ��� ����������
	bool edited;
	rapidxml::xml_node<>* info;
	// �������� �������
	bool change_position;
	FPoint prev_mouse_pos;
private:
#endif

};

} // namespace