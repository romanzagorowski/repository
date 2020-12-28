#pragma once

#include "order.h"
#include "request.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

class market
{
	market(const market&) = delete;
	market(market&&) = delete;

	market& operator = (const market&) = delete;

private:
	std::queue<create_order_request> request_queue;
	std::mutex mutex;

	std::condition_variable condition_variable;

	bool finish = false;

	std::thread thread;

private:
	void process_request_thread();

private:
	void process_request(const create_order_request& request);

public:
	market();
	~market();

public:
	void create_market_order(order_action_t action, order_size_t size);
	void create_pending_order(order_action_t action, order_type_t type, order_price_t price, order_size_t size);
};
