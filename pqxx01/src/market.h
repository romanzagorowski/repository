#pragma once

#include "any_order.h"
#include "request.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <pqxx/pqxx>
#include <set>

class market
{
private:
	pqxx::connection connection;

private:
	market(const market&) = delete;
	market(market&&) = delete;

	market& operator = (const market&) = delete;

private:
	std::queue<create_order_request> requests;
	std::mutex requests_mutex;

	std::condition_variable condition_variable;

	bool finish_flag = false;

	std::thread thread;

private:
	std::mutex			buy_limit_orders_mutex;
	std::set<any_order>	buy_limit_orders;

	std::mutex			sell_limit_orders_mutex;
	std::set<any_order> sell_limit_orders;

private:
	void process_request_thread();

private:
	void process_request(const create_order_request& request);

	void process_request_create_market_order(const create_order_request& request);
	void process_request_create_pending_order(const create_order_request& request);

public:
	market();
	~market();

public:
	void create_market_order(order_action_t action, order_size_t size);
	void create_pending_order(order_action_t action, order_type_t type, order_price_t price, order_size_t size);

	void list_sell_limit_orders(std::ostream& os);
	void list_buy_limit_orders(std::ostream& os);
};
