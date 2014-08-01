#ifndef __GAME_TASK_H__
#define __GAME_TASK_H__

class GameTaskController;

class GameTask
{
	friend class GameTaskController;
protected:
	float local_time;
	float time_scale;
	GameTaskController *controller;
public:
	GameTask(float timeScale)
		: time_scale(timeScale)
		, controller(NULL)
		, local_time(0.0f)
	{}

	virtual void Draw() {}
	virtual void Update(float dt) {}
	virtual void Run() = 0;
	virtual bool isFinished() = 0;
	virtual void AcceptMessage(const Message& msg) {}
	virtual bool MouseDown(IPoint mouse_pos) { return false; }

	typedef boost::shared_ptr<GameTask> HardPtr;
};

class GameTaskController
{
	typedef std::list<GameTask::HardPtr> TaskList;
	TaskList _queuedTasks;
	TaskList _activeTasks;
public:
	GameTaskController() {}

	void Update(float dt);
	void Draw();

	void RunNextTask();
	void QueueNextTask(GameTask::HardPtr task);
	void QueueTask(GameTask::HardPtr task);

	void AcceptMessage(const Message& msg);

	bool MouseDown(IPoint mouse_pos);

	void Clear();

	bool isActive() const;
};

#endif