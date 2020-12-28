#pragma once

#include <cstdint>

typedef std::int64_t order_id;
typedef std::int32_t order_price;
typedef std::int32_t order_size;

enum class order_type { stop, market, limit };
enum class order_action { sell, buy };

class order
{
public:
	order_id id;
	order_price price;
	order_size size;
	order_type type;
	order_action action;
	order_price sl;
	order_price tp;

public:
	order();
	~order();
};
