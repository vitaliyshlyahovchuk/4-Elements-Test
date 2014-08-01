#pragma once

#include <string>
#include <vector>

namespace Render {
	class Texture;
};

class IFlashDisplayObject;

// Аватарка личности - квадратное изображение (и ничего больше)
// Представляет собой объёртку Render::Texture* для возможности отложенной загрузки изображения
class UserAvatar {
public:
	UserAvatar();
	void Draw(const FPoint& position);
	void Draw(const FRect& rect);
	void SetTexture(Render::Texture* texture);
	float GetWidth() const;
	float GetHeight() const;
	Render::Texture* GetTexture();
private:
	Render::Texture* texture;
	FPoint texturePosition;
	FPoint size;
};

// Используется для хранения информации о в Parse.
struct FriendInfo {
	std::string name;        // Имя игрока
	std::string fb_id;       // Facebook Id
	int current_marker;      // Текущий маркер на котором остановился пользователь (сквозная нумерация по уровням и шлюзам)
	int extreme_level;       // Максимальный игровой уровень, на котором остановился пользователь
	std::vector<int> level_scores; // заработанные очки на пройденных уровнях
	UserAvatar* avatar;

	FriendInfo();
	~FriendInfo();

	IFlashDisplayObject* getFlashAvatar();
};