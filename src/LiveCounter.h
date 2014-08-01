#pragma once
/// Счетчик жизней
/// Хранит, списывает/генерирует/добавляет жизни
class LiveCounter {
public:
	LiveCounter();
	// инициализация после смены пользователся/загрузки игры
	void load();
	// максимальное количество автоматически генерируемых жизней
	int getMaxGenerateLive() const { return MAX_GENERATE_LIVES; }
	// текущее количество жизней
	int getLiveAmount() const { return liveAmount; }
	// время до следущей генерации жизни (если достигнут максимум, то возвращается 0, но "0" может возвращаться во время перехода генерации новой жизни)
	int getTimeToNextLive() const { return timeToNextLive; }
	// обновление времени
	void update(float dt);

	/// потратить жизнь
	bool spendLive();
	/// добавить жизнь (после покупки)
	void addLive();
	/// заполнениt жизней
	void fillLives();

	/// включить/выключить уведомления о генерации жизни
	void notificationsEnable(bool value);
	/// включены или выключены уведомления?
	bool notifications() const;
	// добавляет нотификацию о генерации всех жинзей
	void addNotification();
private:
	
	int liveAmount;
	int timeToNextLive;
	
	// время генерации жизни (в секундах)
	int GENERATE_TIME;
	int MAX_GENERATE_LIVES;

	// время следующей генерации жизни
	double nextGenerateTime;
	// сохраняет переменные в хранилище
	void save();
};