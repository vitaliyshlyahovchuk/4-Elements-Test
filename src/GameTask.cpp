#include "stdafx.h"
#include "GameTask.h"

void GameTaskController::Update(float dt)
{
	for( TaskList::iterator itr = _activeTasks.begin(); itr != _activeTasks.end(); )
	{
		(*itr)->Update(dt);
		if( (*itr)->isFinished() )
			itr = _activeTasks.erase(itr);
		else
			++itr;
	}
}

void GameTaskController::Draw()
{
	for( TaskList::iterator itr = _activeTasks.begin(); itr != _activeTasks.end(); ++itr)
	{
		(*itr)->Draw();
	}
}

void GameTaskController::RunNextTask()
{
	if( !_queuedTasks.empty() )
	{
		_activeTasks.push_back(_queuedTasks.front());
		_queuedTasks.pop_front();
		if(_activeTasks.empty())
		{
			Assert(false);
		}else{
			_activeTasks.back()->Run();
		}
	}
}

void GameTaskController::QueueNextTask(GameTask::HardPtr task)
{
	task->controller = this;
	_queuedTasks.push_front(task);
}

void GameTaskController::QueueTask(GameTask::HardPtr task)
{
	task->controller = this;
	_queuedTasks.push_back(task);
}

void GameTaskController::Clear()
{
	_activeTasks.clear();
	_queuedTasks.clear();
}

void GameTaskController::AcceptMessage(const Message& msg)
{
	for( TaskList::iterator itr = _activeTasks.begin(); itr != _activeTasks.end(); ++itr)
	{
		(*itr)->AcceptMessage(msg);
	}
}

bool GameTaskController::MouseDown(IPoint mouse_pos)
{
	for( TaskList::iterator itr = _activeTasks.begin(); itr != _activeTasks.end(); ++itr)
	{
		if( (*itr)->MouseDown(mouse_pos) )
			return true;
	}
	return false;
}

bool GameTaskController::isActive() const
{
	return !_activeTasks.empty();
}