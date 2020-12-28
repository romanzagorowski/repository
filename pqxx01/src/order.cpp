#include "order.h"

order::order()
{
}

order::~order()
{
}

std::string order_type_to_string(const order_type_t order_type)
{
	switch(order_type)
	{
		case order_type_t::limit: return "limit";
		case order_type_t::market: return "market";
		case order_type_t::stop: return "stop";
	}
	return "";
}

std::string order_action_to_string(const order_action_t order_action)
{
	switch(order_action)
	{
		case order_action_t::buy: return "buy";
		case order_action_t::sell: return "sell";
	}
	return "";
}
