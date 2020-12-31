#include "any_order.h"

any_order::any_order(
	order_id_t		id
,	order_action_t	action
,	order_type_t	type
,	order_price_t	price
,	order_size_t	size
)	:
	id(id)
,	action(action)
,	type(type)
,	price(price)
,	size(size)
{
}

any_order::~any_order()
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

/*
bool operator < (const any_order& o1, const any_order& o2)
{
	return (o1.price < o2.price) || (o1.price == o2.price && o1.id < o2.id);
}
*/

std::ostream& operator << (std::ostream& os, const any_order& o)
{
	os << "{ order_id: " << o.id;
	os << ", order_action: " << order_action_to_string(o.action);
	os << ", order_type: " << order_type_to_string(o.type);
	os << ", order_price: " << o.price;
	os << ", order_size: " << o.size;
	os << " }";
	return os;
}
