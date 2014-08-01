#pragma once
#include "Container.h"

class RenderTargetHolder;

namespace Card {

class UpdateLevelItem;
class LevelMarker;
class Gateway;
class CardAvatarFriends;
class CardUserAvatar;
class Shine;

class Map : public Container
{
public:
	Map();
	~Map();
	void loadDescription(rapidxml::xml_node<>* objectsXml, rapidxml::xml_node<>* shineXml, bool debugLoad = false);
	std::vector<MapItem*>& getObjects() { return children; };
	UpdateLevelItem* getUpdateLevelItem(int level_marker);
	CardAvatarFriends* getAvatarFriends(int level_marker);
	void updateUserAvatar();
	void runUserAvatar();
	void clearAvatarFriends();
	void UpdateShine(float dt);
	void DrawShine(RenderTargetHolder* target, float shift);
	void DrawShineEffect(float shift);

	virtual void removeChildren();

	void correct(int &index);
	void incrementHistory();
	void debugDecrementHistory();
	void initMarkers();
	bool isLoaded() const;
	bool inGateway();
	// запустить эффекты звезд на уровнях
	void runStarEffects();
	// запускает эффект открытия эпизода
	void openEpisod();
	void debugTryOpenEpisod();
	void removeFrozenAnimations();
private:
	CardUserAvatar* userAvatar;
	std::vector<UpdateLevelItem*> markers;
	std::vector<Gateway*> gate_markers;
	std::list<LevelMarker*> level_markers;
	std::vector<CardAvatarFriends*> avatar_friends;
	// подсвечивает эпизоды 
	Shine* shine;
private:
	void initEpisodShine();
};

} // namespase