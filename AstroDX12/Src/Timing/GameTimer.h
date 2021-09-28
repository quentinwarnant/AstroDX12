#pragma once

#include <Windows.h>

class GameTimer
{
public:
	GameTimer();

	float TotalTime() const; // In seconds
	float DeltaTime() const; // In seconds

	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	double m_secondsPerCount;
	double m_deltaTime;

	__int64 m_baseTime;
	__int64 m_pausedTime;
	__int64 m_stopTime;
	__int64 m_prevTime;
	__int64 m_currTime;

	bool m_stopped;

};

