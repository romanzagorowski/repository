create or replace procedure sp_process_instant_order(
    in  p_order_size    instant_order.size%type
,   in  p_order_action  instant_order.action%type
)
language plpgsql
as
$$
begin
    if p_order_action = 'buy' then
        call sp_process_instant_order_buy(size);
    else
        call sp_process_instant_order_sell(size);
    end;
end;
$$

