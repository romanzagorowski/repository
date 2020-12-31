#include "market.h"
#include "request.h"

#include <chrono>
#include <cassert>
#include <iostream>
#include <string>

//-----------------------------------------------------------------------------

static
void read_sell_limit_orders(
	pqxx::connection& c
,	std::set<any_order, sell_limit_less>& s
)
{
	const std::string sql = "select id, price, size from any_order where action = 'sell' and type = 'limit'";

	pqxx::nontransaction nt(c);
	pqxx::result r = nt.exec(sql);

	for(const auto & a : r)
	{
		order_id_t id = a[0].as<int>();
		order_action_t action = order_action_t::sell;
		order_type_t type = order_type_t::limit;
		order_price_t price = a[1].as<int>();
		order_size_t size = a[2].as<int>();

		any_order o{ id, action, type, price, size };

		s.insert(o);
	}
}

//-----------------------------------------------------------------------------

static
void read_buy_limit_orders(
	pqxx::connection& c
,	std::set<any_order, buy_limit_less>& s
)
{
	const std::string sql = "select id, price, size from any_order where action = 'buy' and type = 'limit'";

	pqxx::nontransaction nt(c);
	pqxx::result r = nt.exec(sql);

	for(const auto & a : r)
	{
		order_id_t id = a[0].as<int>();
		order_action_t action = order_action_t::buy;
		order_type_t type = order_type_t::limit;
		order_price_t price = a[1].as<int>();
		order_size_t size = a[2].as<int>();

		any_order o{ id, action, type, price, size };

		s.insert(o);
	}
}

//-----------------------------------------------------------------------------

market::market() :
	connection("dbname=pqxx01")
,	thread(&market::process_request_thread, this)
{
	read_sell_limit_orders(this->connection, this->sell_limit_orders);
	read_buy_limit_orders(this->connection, this->buy_limit_orders);
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

static
std::string format_sql_insert_market_order(order_action_t order_action, order_size_t order_size)
{
	std::stringstream ss;
	ss << "insert into any_order (action, type, price, size) values ('";
	ss << order_action_to_string(order_action) << "', '";
	ss << order_type_to_string(order_type_t::market) << "', ";
	ss << 0 << ", ";
	ss << order_size << ") returning id";

	return ss.str();
}

static
order_id_t insert_market_order(pqxx::work& w, order_action_t order_action, order_size_t order_size)
{
	const std::string sql = format_sql_insert_market_order(order_action, order_size);

	order_id_t order_id = 0;

	pqxx::result r = w.exec(sql);

	for(const auto& a : r)
	{
		order_id = a[0].as<int>();
	}

	return order_id;
}

void market::process_request_create_market_order(const create_order_request& r)
{
	pqxx::work w(this->connection);

	order_id_t order_id = insert_market_order(w, r.order_action, r.order_size);
	assert(0 != order_id);

	w.commit();
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

void market::create_market_order_request(order_action_t order_action, order_size_t order_size)
{
	create_order_request request(order_action, order_size);

	{
		std::lock_guard<std::mutex> guard(this->requests_mutex);

		this->requests.push(request);

		this->requests_mutex.unlock();

		this->condition_variable.notify_one();
	}
}

void market::create_pending_order_request(order_action_t order_action, order_type_t order_type, order_price_t order_price, order_size_t order_size)
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
		//os << o << std::endl;
		os << o.price << ", " << o.id << ", " << o.size << std::endl;
	}
}

void market::list_buy_limit_orders(std::ostream& os)
{
	std::lock_guard<std::mutex> lock(this->buy_limit_orders_mutex);

	for(const auto& o : this->buy_limit_orders)
	{
		//os << o << std::endl;
		os << o.price << ", " << o.id << ", " << o.size << std::endl;
	}
}
