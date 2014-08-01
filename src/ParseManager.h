#ifndef PARSEMANAGER_H
#define PARSEMANAGER_H

#include "GameInfo.h"

namespace ParseManager {

	// Инициализация Parse.
	void Init(const std::string& appId, const std::string& clientKey);
	// В обновлении имеется цикл проверки доступности парса
	void Update(float dt);
	// загружает сохранку
	void StartLoad();
	// от фейсбука получены uid пользователя (и его имя)
	void RegisterFacebookID(const std::string& fbId, const std::string& fbName);
	// сохранить информацию в базу
	void Save(bool withMoney = false);
	// загрузить информацию о всех имеющихся друзьях (на момент вызова социальная информация о друзьях должна быть известна)
	void LoadFriendsInfo();
	// обновить деньги значениями с сервера
	void RefreshCash(bool spendCash);
	// удалить сохранку (дабажный сброс)
	void Reset();

} // end of namespace

#endif // PARSEMANAGER_H