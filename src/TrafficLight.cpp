#include <iostream>
#include <random>

#include <queue>
#include<future>

#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

 
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
     std::unique_lock<std::mutex> uLock(_mutex);
	// to wait for and receive new messages and pull them from the queue using move semantics. 
    _cond.wait(uLock, [this]{return !_messages.empty();});
    // The received object should then be returned by the receive function. 
	T msg = std::move(_messages.back());//message received
    _messages.pop_back();
	return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    std::lock_guard<std::mutex> uLock(_mutex);
	// as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
	_messages.push_back(std::move(msg));//message sent (adding trafficlightphase to the queue)
    _cond.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
	
	while(1)
	{
		// sleep to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
		
		auto currentPhase = _messageQueue.receive();
        if (currentPhase == TrafficLightPhase::green) return;

	}
	
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
	threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
	std::random_device rd;  
    std::mt19937 eng(rd());//generate random number
    std::uniform_int_distribution<> distribution(4, 6); // random value between 4 and 6
    double lastCycleDuration = distribution(eng); // duration of a single cycle in sec
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;

    // initialize the stop watch 
    lastUpdate = std::chrono::system_clock::now();
    while (1) 
	{
        // sleep to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch 
        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastUpdate).count();
 
        if (timeSinceLastUpdate >= lastCycleDuration) 
		{
            _currentPhase = (_currentPhase == TrafficLightPhase::red) ? TrafficLightPhase::green: TrafficLightPhase::red;
			
			
            auto message = _currentPhase;
            auto is_sent = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, &_messageQueue, std::move(message));
            is_sent.wait();

			
            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();
            lastCycleDuration = distribution(eng);

        }
    }
	
}

