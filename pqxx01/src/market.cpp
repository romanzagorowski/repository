#include "market.h"
#include "request.h"

#include <chrono>
#include <cassert>
#include <iostream>

market::market() :
	thread(&market::process_request_thread, this)
{
}

market::~market()
{
	{
		std::lock_guard<std::mutex> lock(this->mutex);
		this->finish = true;
	}

	this->condition_variable.notify_one();

	this->thread.join();
}

void market::process_request(const create_order_request& request)
{
	std::cout << "INFO: market::process_reqeust: " << request << std::endl;
}

void market::process_request_thread()
{
	std::cout << "INFO: market::process_reqeust_thread() started..." << std::endl;

	while(true)
	{
		create_order_request request = {};

		{
			std::unique_lock<std::mutex> lock(this->mutex);

			this->condition_variable.wait(lock, [&] { return this->finish || !this->request_queue.empty(); });

			if(this->finish)
			{
				std::cout << "INFO: marked::process_request_thread() received finish signal..." << std::endl;

				break;
			}

			assert(!this->request_queue.empty());

			request = this->request_queue.front();
			this->request_queue.pop();

		}	// lock gets out of scope

		this->process_request(request);
	}
}

void market::create_market_order(order_action_t order_action, order_size_t order_size)
{
	create_order_request request(order_action, order_size);

	{
		std::lock_guard<std::mutex> guard(this->mutex);

		this->request_queue.push(request);

		this->mutex.unlock();

		this->condition_variable.notify_one();
	}
}

void market::create_pending_order(order_action_t order_action, order_type_t order_type, order_price_t order_price, order_size_t order_size)
{
	create_order_request request(order_action, order_type, order_price, order_size);

	{
		std::lock_guard<std::mutex> guard(this->mutex);

		this->request_queue.push(request);

		this->mutex.unlock();

		this->condition_variable.notify_one();
	}
}
