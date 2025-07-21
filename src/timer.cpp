#include "timer.hpp"
#include "print.hpp"

#include <functional>
#include <chrono>
#include <set>

int Timer::nextId = 1;

bool TimerEvent::operator<(const TimerEvent &other) const {
	return time < other.time;
}

/// @brief Add a new event to the timer.
/// @param delay The delay after which the event should be triggered
/// @param callback The function to call when the event is triggered
/// @param isRecurring If true, the event will be repeated at the specified interval; otherwise, it will only be triggered once
/// @return Returns the ID of the added event
int Timer::addEvent(std::chrono::steady_clock::duration delay, std::function<void()> callback, bool isRecurring) {
	std::chrono::steady_clock::duration interval = isRecurring ? delay : std::chrono::steady_clock::duration::max();
	std::chrono::steady_clock::time_point eventTime = std::chrono::steady_clock::now() + delay;
	auto it = _events.insert(TimerEvent{interval, eventTime, callback, nextId++});
	return (it->id);
}

/// @brief Delete an event by its ID
/// @param eventId The ID of the event to delete
/// @return Returns 0 if the event was deleted successfully, -1 if the event was not found
int Timer::deleteEvent(int eventId) {
	for (auto it = _events.begin(); it != _events.end(); ++it) {
		if (it->id == eventId) {
			_events.erase(it);
			return (0);
		}
	}

	return (-1);
}

/// @brief Get the time until the next scheduled event
/// @return Returns the duration until the next event, or max() if no events are scheduled
int Timer::getNextEventTimeoutMS() const {
	if (_events.empty()) return (-1);
	auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(_events.begin()->time - now);
    return diff.count() < 0 ? 0 : static_cast<int>(diff.count());
}

/// @brief Process all events that are due
/// This function checks the current time against the scheduled event times
/// and calls the callback functions for events that are due.
void Timer::processEvents() {
	auto now = std::chrono::steady_clock::now();
	while (!_events.empty() && _events.begin()->time <= now) {
		auto event = *_events.begin();
		_events.erase(_events.begin());
		event.callback();

		if (event.interval != std::chrono::steady_clock::duration::max()) {
			event.time = std::max(event.time + event.interval, now);
			_events.insert(event);
		}
	}
}

/// @brief Clear all events from the timer
/// This function removes all scheduled events from the timer.
void Timer::clear() {
	_events.clear();
}
