#include "order.h"

#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <mutex>
#include <chrono>
#include <map>
#include <utility>
#include <cstdint>
#include <vector>
#include <memory>
#include <regex>
#include <cassert>
#include <sstream>

void create_table(pqxx::connection& c)
{
	pqxx::work w(c);

	const char* sql =
			"create table company ("
			"id int primary key,"
			"name text not null,"
			"age int not null,"
			"address varchar(50) not null,"
			"salary real);"
			;

	w.exec(sql);
	w.commit();
}

void insert_data(pqxx::connection& c)
{
	pqxx::work w(c);

	/*
	const char* sql =
			"create table company ("
			"id int primary key,"
			"name text not null,"
			"age int not null,"
			"address varchar(50) not null,"
			"salary real);"
			;
	*/
	const char* sql = "insert into company values "
			"(1, 'name1', 11, 'address1', 3.4),"
			"(2, 'name2', 12, 'address2', 6.2),"
			"(3, 'name3', 13, 'address3', 9.7)"
			;

	w.exec(sql);
	w.commit();
}

void select_data(pqxx::connection& c)
{
	pqxx::nontransaction n(c);

	const char* sql = "select * from company";

	pqxx::result r(n.exec(sql));

	for(pqxx::result::const_iterator i = r.begin(); i != r.end(); ++i)
	{
		std::cout << "id = " << i[0].as<int>() << ", ";
		std::cout << "name = '" << i[1].as<std::string>() << "', ";
		std::cout << "age = " << i[2].as<int>() << ", ";
		std::cout << "address = '" << i[3].as<std::string>() << "', ";
		std::cout << "salary = " << i[4].as<float>() << std::endl;
	}
}

void select_waiting_orders(pqxx::connection & c)
{
	pqxx::nontransaction n(c);

	const char* sql = "select order_id, price, size, action, wait_type from waiting_order";

	pqxx::result r = n.exec(sql);

	for(pqxx::result::const_iterator i = r.begin(); i != r.end(); ++i)
	{
		std::cout << "order_id=" 	<< i[0].as<int>() << ", ";
		std::cout << "price=" 		<< i[1].as<std::string>() << ", ";
		std::cout << "size=" 		<< i[2].as<std::string>() << ", ";
		std::cout << "action='" 	<< i[3].as<std::string>() << "', ";
		std::cout << "wait_type='" 	<< i[4].as<std::string>() << "'" << std::endl;
	}
}

int foo()
{
	try
	{
		pqxx::connection c("dbname=hg01");

		select_waiting_orders(c);

		c.disconnect();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;

		return 1;
	}

	return 0;
}

std::timed_mutex mutex;

//typedef std::uint64_t 	order_id_t;
typedef int	 			order_id_t;
typedef double			order_price_t;
typedef double			order_size_t;

typedef std::tuple<order_price_t, order_id_t> price_key;

enum class order_action_t { buy, sell };
enum class order_wait_type_t { limit, stop };

struct waiting_order_t
{
	order_id_t			id;
	order_price_t		price;
	order_size_t		size;
	order_action_t		action;
	order_wait_type_t	wait_type;
};

std::ostream& operator << (std::ostream& os, const waiting_order_t& wo)
{
	os << "{ ";
	os << "id=" << wo.id;
	os << ", price=" << wo.price;
	os << ", size=" << wo.size;
	os << ", action='" << (wo.action == order_action_t::buy ? "buy" : "sell") << "'";
	os << ", wait_type='" << (wo.wait_type == order_wait_type_t::limit ? "limit" : "stop") << "'";
	os << " }";
	return os;
}

waiting_order_t a[] = {
	{ 1, 1190, 1.7, order_action_t::buy, order_wait_type_t::limit }
,	{ 2, 1180, 2.3, order_action_t::buy, order_wait_type_t::limit }
,	{ 3, 1195, 0.7, order_action_t::buy, order_wait_type_t::limit }
,	{ 4, 1185, 0.4, order_action_t::buy, order_wait_type_t::limit }
,	{ 5, 1175, 0.8, order_action_t::buy, order_wait_type_t::limit }
,	{ 6, 1199, 1.1, order_action_t::buy, order_wait_type_t::limit }
,	{ 7, 1180, 1.1, order_action_t::buy, order_wait_type_t::limit }
};

using shared_waiting_order_t 	= std::shared_ptr<waiting_order_t>;
using price_id_key_t 			= std::pair<order_price_t, order_id_t>;

std::map<price_id_key_t, shared_waiting_order_t> m;

void populate_map()
{
	std::unique_lock<std::timed_mutex> lock(mutex, std::chrono::seconds(1));

	if(lock)
	{
		for(auto & x : a)
		{
			price_id_key_t k = std::make_pair(x.price, x.id);
			shared_waiting_order_t v = std::make_shared<waiting_order_t>(x);
			m.insert(std::make_pair(k, v));
		}
	}
	else
	{
		std::cout << "ERROR: Unable to lock waiting order list for writing..." << std::endl;
	}
}

void list_waiting_orders()
{
	std::unique_lock<std::timed_mutex> lock(mutex, std::chrono::seconds(1));

	if(lock)
	{
		for(auto & a : m)
		{
			std::cout << *a.second << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to lock waiting order list for reading..." << std::endl;
	}
}

//-----------------------------------------------------------------------------

std::string order_action_to_string(order_action_t order_action)
{
	return order_action_t::buy == order_action ? "buy" : "sell";
}

std::string order_wait_type_to_string(order_wait_type_t order_wait_type)
{
	return order_wait_type_t::limit == order_wait_type ? "limit" : "stop";
}

std::string format_query(order_action_t order_action, order_price_t order_price)
{
	std::stringstream ss;

	ss << "insert into instant_order (action, size) values ('"
		<< order_action_to_string(order_action) << "', "
		<< order_price
		<< ") returning id"
		;

	return ss.str();
}

order_id_t insert_instant_order(pqxx::work & w, order_action_t order_action, order_price_t order_price)
{
	std::string sql = format_query(order_action, order_price);

	pqxx::result r = w.exec(sql);

	order_id_t order_id = 0;

	for(pqxx::result::const_iterator i = r.begin(); i != r.end(); ++i)
	{
		order_id = i[0].as<int>();
	}

	return order_id;
}

order_id_t process_instant_order(pqxx::connection & c, order_action_t order_action, order_size_t order_size)
{
	order_id_t order_id = 0;

	try
	{
		pqxx::work w(c);

		order_id = insert_instant_order(w, order_action, order_size);

		w.commit();
	}
	catch(const std::exception & e)
	{
		std::cerr << "ERROR: cought std exception! what(): '" << e.what() << "'" << std::endl;
	}

	return order_id;
}

//-----------------------------------------------------------------------------

static order_id_t last_order_id = 0;

order_id_t get_next_order_id()
{
	return ++last_order_id;
}

void set_last_order_id(order_id_t order_id)
{
	last_order_id = order_id;
}

order_id_t insert_waiting_order(
	pqxx::work & w
,	order_action_t 		order_action
,	order_wait_type_t 	order_wait_type
,	order_price_t 		order_price
,	order_size_t 		order_size
)
{
	std::stringstream ss;

	ss	<< "insert into waiting_order (price, size, action, wait_type) values ("
		<< order_price << ", "
		<< order_size << ", '"
		<< order_action_to_string(order_action) << "', '"
		<< order_wait_type_to_string(order_wait_type) << "') returning id"
		;

	std::string sql = ss.str();

	pqxx::result r = w.exec(sql);

	order_id_t order_id = 0;

	for(auto i = r.begin(); i != r.end(); ++i)
	{
		order_id = i[0].as<int>();
	}

	return order_id;
}

//-----------------------------------------------------------------------------

order_id_t process_waiting_order(
	pqxx::connection&	c
,	order_action_t 		order_action
,	order_wait_type_t 	order_wait_type
,	order_price_t 		order_price
,	order_size_t 		order_size
)
{

	pqxx::work w(c);

	try
	{

		order_id_t order_id = insert_waiting_order(
			w
		,	order_action
		,	order_wait_type
		,	order_price
		,	order_size
		);

		w.commit();

		return order_id;
	}
//	catch(const pqxx::pqxx_exception & pqxx_e)
//	{
//		std::cerr << "ERROR: Caught pqxx exception!" << std::endl;
//	}
	catch(const std::exception & std_e)
	{
		std::cerr << "ERROR: Caught std exception! what(): " << std_e.what() << std::endl;
	}

	return 0;
}

void do_work()
{
	populate_map();

	//-------------------------------------------------------------------------

	pqxx::connection c("dbname=pqxx01");

	//-------------------------------------------------------------------------

	std::string s;

	while(std::getline(std::cin, s))
	{
		if("list" == s)
		{
			list_waiting_orders();
		}
		else if("exit" == s || "quit" == s || "done" == s)
		{
			break;
		}
		else
		{
			std::string p = "(buy|sell) ([1-9][0-9]*)";
			std::regex r(p);
			std::smatch m;

			bool b = std::regex_match(s, m, r);

			if(b)
			{
				std::cout << "MATCH!" << std::endl;
				std::cout << "m.str(0): '" << m.str(0) << "'" << std::endl;
				std::cout << "m.str(1): '" << m.str(1) << "'" << std::endl;
				std::cout << "m.str(2): '" << m.str(2) << "'" << std::endl;

				order_action_t 	order_action 	= "buy" == m.str(1) ? order_action_t::buy : order_action_t::sell;
				order_size_t 	order_size 		= std::stoi(m.str(2));

				order_id_t order_id = process_instant_order(c, order_action, order_size);
				assert(0 != order_id);

				std::cout << order_id << std::endl;
			}
			else
			{
				std::string p = "(buy|sell) (limit|stop) ([1-9][0-9]*) ([1-9][0-9]*)";
				std::regex r(p);
				std::smatch m;

				bool b = std::regex_match(s, m, r);

				if(b)
				{
					std::cout << "MATCH!" << std::endl;
					std::cout << "m.str(0): '" << m.str(0) << "'" << std::endl;
					std::cout << "m.str(1): '" << m.str(1) << "'" << std::endl;
					std::cout << "m.str(2): '" << m.str(2) << "'" << std::endl;
					std::cout << "m.str(3): '" << m.str(3) << "'" << std::endl;
					std::cout << "m.str(4): '" << m.str(4) << "'" << std::endl;

					order_action_t 		order_action 	= "buy"   == m.str(1) ? order_action_t::buy : order_action_t::sell;
					order_wait_type_t	order_wait_type	= "limit" == m.str(2) ? order_wait_type_t::limit : order_wait_type_t::stop;
					order_price_t 		order_price 	= std::stoi(m.str(3));
					order_size_t 		order_size 		= std::stoi(m.str(4));

					order_id_t order_id = process_waiting_order(c, order_action, order_wait_type, order_price, order_size);
					assert(0 != order_id);

					std::cout << order_id << std::endl;
				}
				else
				{
					std::cout << "MISMATCH..." << std::endl;
				}
			}
		}
	}

	std::cout << "bye!" << std::endl;
}

void do_test()
{
	std::regex_constants::syntax_option_type a[] = {
			std::regex_constants::basic,
			std::regex_constants::extended,
//			std::regex_constants::normal,
//			std::regex_constants::emacs,
			std::regex_constants::awk,
			std::regex_constants::grep,
			std::regex_constants::egrep
	};

	const char* t[] = {
			"basic",
			"extended",
//			"normal",
//			"emacs",
			"awk",
			"grep",
			"egrep"
	};

	std::string p = "(buy|sell)";
	std::string s = "sell";

	for(size_t i = 0, j = sizeof(a)/sizeof(a[0]); i < j; ++i)
	{
		std::regex r(p, a[i]);

		std::cout << t[i] << ": "<< std::boolalpha << std::regex_match(s, r) << std::endl;
	}
}

int main()
{
	std::cout << "sizeof(order): " << sizeof(order) << std::endl;

	//do_test();
	do_work();
}
