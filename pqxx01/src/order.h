#pragma once

#include <cstdint>
#include <string>

typedef std::int64_t order_id_t;
typedef std::int32_t order_price_t;
typedef std::int32_t order_size_t;

enum class order_type_t { stop, market, limit };
enum class order_action_t { sell, buy };

class order
{
public:
	order_id_t id;
	order_price_t price;
	order_size_t size;
	order_type_t type;
	order_action_t action;
	order_price_t sl;
	order_price_t tp;

public:
	order();
	~order();
};

std::string order_type_to_string(const order_type_t order_type);
std::string order_action_to_string(const order_action_t order_action);
