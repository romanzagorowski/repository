#pragma once

#include "order.h"

#include <ostream>

struct create_order_request
{
	order_action_t order_action;
	order_type_t   order_type;
	order_price_t  order_price;
	order_size_t   order_size;

	create_order_request() = default;

	explicit create_order_request(order_action_t order_action, order_size_t order_size) :
		order_action(order_action)
	,	order_type(order_type_t::market)
	,	order_price(0)
	,	order_size(order_size)
	{
	}

	explicit create_order_request(order_action_t order_action, order_type_t order_type, order_price_t order_price, order_size_t order_size) :
		order_action(order_action)
	,	order_type(order_type)
	,	order_price(order_price)
	,	order_size(order_size)
	{
	}
};

std::ostream& operator << (std::ostream& os, const create_order_request& r);
