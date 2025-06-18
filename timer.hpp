#pragma once

#include <functional>
#include <chrono>
#include <set>

struct TimerEvent {
	std::chrono::steady_clock::duration interval;
	std::chrono::steady_clock::time_point time;
	std::function<void()> callback;
	int	id;

	bool operator<(const TimerEvent &other) const;
};

class Timer {
private:
	std::multiset<TimerEvent> _events;
	
public:
	static int nextId;

	Timer() = default;
	Timer(const Timer &other) = default;
	Timer &operator=(const Timer &other) = default;
	~Timer() = default;

	int addEvent(std::chrono::steady_clock::duration delay, std::function<void()> callback, bool isRecurring = false);
	int deleteEvent(int eventId);
	
	int getNextEventTimeoutMS() const;
	void processEvents();
	void clear();
};
