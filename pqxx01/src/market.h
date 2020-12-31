#pragma once

#include "any_order.h"
#include "request.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <pqxx/pqxx>
#include <set>
#include <functional>

struct buy_limit_less
{
	bool operator () (const any_order& o1, const any_order& o2) const
	{
		return (o1.price > o2.price) || (o1.price == o2.price && o1.id < o2.id);
	}
};

struct sell_limit_less
{
	bool operator () (const any_order& o1, const any_order& o2) const
	{
		return (o1.price < o2.price) || (o1.price == o2.price && o1.id < o2.id);
	}
};

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
	std::mutex							buy_limit_orders_mutex;
	std::set<any_order,	buy_limit_less>	buy_limit_orders;

	std::mutex								sell_limit_orders_mutex;
	std::set<any_order, sell_limit_less>	sell_limit_orders;

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
	void create_market_order_request(order_action_t action, order_size_t size);
	void create_pending_order_request(order_action_t action, order_type_t type, order_price_t price, order_size_t size);

	void list_sell_limit_orders(std::ostream& os);
	void list_buy_limit_orders(std::ostream& os);
};
