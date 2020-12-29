#include "market.h"
#include "request.h"

#include <chrono>
#include <cassert>
#include <iostream>
#include <string>

market::market() :
	connection("dbname=pqxx01")
,	thread(&market::process_request_thread, this)
{
}

market::~market()
{
	{
		std::lock_guard<std::mutex> lock(this->requests_mutex);
		this->finish_flag = true;
	}

	this->condition_variable.notify_one();

	this->thread.join();
}

void market::process_request_create_market_order(const create_order_request& request)
{
	throw "Not implemented yet";
}

//order_id_t insert_instant_order(pqxx::work & w, order_action_t order_action, order_price_t order_price)
//{
//	std::string sql = format_query(order_action, order_price);
//
//	pqxx::result r = w.exec(sql);
//
//	order_id_t order_id = 0;
//
//	for(pqxx::result::const_iterator i = r.begin(); i != r.end(); ++i)
//	{
//		order_id = i[0].as<int>();
//	}
//
//	return order_id;
//}

static
std::string format_query(
	order_action_t 	order_action
,	order_type_t	order_type
,	order_price_t	order_price
,	order_size_t 	order_size
)
{
	std::stringstream ss;

	ss << "insert into any_order (action, type, price, size) values ('"
			<< order_action_to_string(order_action) << "', '"
			<< order_type_to_string(order_type) << "', "
			<< order_price << ", "
			<< order_size
			<< ") returning id"
		;

	return ss.str();
}

static
order_id_t insert_pending_order(
	pqxx::connection& connection
,	order_action_t 	order_action
,	order_type_t	order_type
,	order_price_t	order_price
,	order_size_t	order_size
)
{
	pqxx::work w(connection);

	std::string sql = format_query(order_action, order_type, order_price, order_size);

	pqxx::result r = w.exec(sql);
	w.commit();

	order_id_t order_id = 0;

	for(pqxx::result::const_iterator i = r.begin(); i != r.end(); ++i)
	{
		order_id = i[0].as<int>();
	}

	return order_id;
}

void market::process_request_create_pending_order(const create_order_request& r)
{
	order_id_t order_id = insert_pending_order(
		this->connection
	,	r.order_action
	,	r.order_type
	,	r.order_price
	,	r.order_size
	);
	assert(0 != order_id);

	any_order order(order_id, r.order_action, r.order_type, r.order_price, r.order_size);

	if(order.type == order_type_t::limit)
	{
		if(order.action == order_action_t::buy)
		{
			std::lock_guard<std::mutex> lock(this->buy_limit_orders_mutex);
			this->buy_limit_orders.insert(order);
		}
		else	// order.action == order_action_t::sell
		{
			std::lock_guard<std::mutex> lock(this->sell_limit_orders_mutex);
			this->sell_limit_orders.insert(order);
		}
	}
}

void market::process_request(const create_order_request& request)
{
	std::cout << "INFO: market::process_reqeust: " << request << std::endl;

	if(request.order_type == order_type_t::market)
	{
		this->process_request_create_market_order(request);
	}
	else
	{
		this->process_request_create_pending_order(request);
	}
}

void market::process_request_thread()
{
	std::cout << "INFO: market::process_reqeust_thread() started..." << std::endl;

	while(true)
	{
		create_order_request request = {};

		{
			std::unique_lock<std::mutex> lock(this->requests_mutex);

			this->condition_variable.wait(lock, [&] { return this->finish_flag || !this->requests.empty(); });

			if(this->finish_flag)
			{
				std::cout << "INFO: marked::process_request_thread() received finish_flag signal..." << std::endl;

				break;
			}

			assert(!this->requests.empty());

			request = this->requests.front();
			this->requests.pop();

		}	// lock gets out of scope

		this->process_request(request);
	}
}

void market::create_market_order(order_action_t order_action, order_size_t order_size)
{
	create_order_request request(order_action, order_size);

	{
		std::lock_guard<std::mutex> guard(this->requests_mutex);

		this->requests.push(request);

		this->requests_mutex.unlock();

		this->condition_variable.notify_one();
	}
}

void market::create_pending_order(order_action_t order_action, order_type_t order_type, order_price_t order_price, order_size_t order_size)
{
	create_order_request request(order_action, order_type, order_price, order_size);

	{
		std::lock_guard<std::mutex> guard(this->requests_mutex);

		this->requests.push(request);

		this->requests_mutex.unlock();

		this->condition_variable.notify_one();
	}
}

void market::list_sell_limit_orders(std::ostream& os)
{
	std::lock_guard<std::mutex> lock(this->sell_limit_orders_mutex);

	for(const auto& o : this->sell_limit_orders)
	{
		os << o << std::endl;
	}
}

void market::list_buy_limit_orders(std::ostream& os)
{
	std::lock_guard<std::mutex> lock(this->buy_limit_orders_mutex);

	for(const auto& o : this->buy_limit_orders)
	{
		os << o << std::endl;
	}
}
