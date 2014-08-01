#pragma once

class Splash: 
	public GUI::Widget
{
	float timer;
	bool loadStarted;
	Render::Texture* texture;
public:
	Splash(const std::string& name, rapidxml::xml_node<>* xmlElement);
	virtual void Update(float dt);
	virtual void Draw();
};