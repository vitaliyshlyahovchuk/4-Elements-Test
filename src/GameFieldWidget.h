#ifndef ONCE_GAME_FIELD_W
#define ONCE_GAME_FIELD_W

typedef std::list<Message> MessagesList;

class GameFieldWidget 
	: public  GUI::Widget
{
	FPoint _scale;
public:
    GameFieldWidget(const std::string& name_, rapidxml::xml_node<>* elem_);
    
    MessagesList _precreate_messages_list;
    
    virtual bool MouseDown(const IPoint& mouse_pos);
    virtual void MouseDoubleClick(const IPoint& mouse_pos);
    virtual void MouseUp(const IPoint& mouse_pos);
    virtual void MouseMove(const IPoint& mouse_pos);
    virtual void MouseWheel(int delta);
    virtual void Draw();
    virtual void Update(float);
    
    virtual void AcceptMessage(const Message& message);
    virtual Message QueryState(const Message& message) const;	
};

class GameFieldUp
	: public  GUI::Widget
{
public:
	GameFieldUp(const std::string& name_, rapidxml::xml_node<>* elem_);
	void Draw();
};

class BlackFrame
	: public  GUI::Widget
{
	bool _isIPhone5;
public:
	BlackFrame(const std::string& name_, rapidxml::xml_node<>* elem_);
	void Draw();
};

#endif