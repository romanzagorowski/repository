#include "request.h"

std::ostream& operator << (std::ostream& os, const create_order_request& r)
{
	os << "{ order_action: " << order_action_to_string(r.order_action);
	os << ", order_type: " << order_type_to_string(r.order_type);
	os << ", order_price: " << r.order_price;
	os << ", order_size: " << r.order_size << " }";
	return os;
}
