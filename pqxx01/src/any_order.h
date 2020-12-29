#pragma once

#include <cstdint>
#include <string>
#include <ostream>

typedef std::int64_t order_id_t;
typedef std::int32_t order_price_t;
typedef std::int32_t order_size_t;

enum class order_type_t { stop, market, limit };
enum class order_action_t { sell, buy };

class any_order
{
public:
	order_id_t		id;
	order_action_t	action;
	order_type_t	type;
	order_price_t	price;
	order_size_t	size;

public:
	explicit any_order(order_id_t, order_action_t, order_type_t, order_price_t, order_size_t);
	~any_order();
};

std::string order_type_to_string(const order_type_t order_type);
std::string order_action_to_string(const order_action_t order_action);

bool operator < (const any_order& o1, const any_order& o2);

std::ostream& operator << (std::ostream& os, const any_order& o);
