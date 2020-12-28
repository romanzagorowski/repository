create or replace procedure sp_process_instant_order_buy(
    in  p_order_size    instant_order.size%type
)
language plpgsql
as
$$
declare
    r           record;
    position_id position.id%type;
begin
    for r in
    with x as (
        select
            id
        ,   price
        ,   size
        ,   sum(size) over (order by price asc, id asc) as sum_size
        from waiting_order
        where
               action = 'sell'
        and wait_type = 'limit'
    )
    select * from x
    where
        sum_size < p_order_size
    order by
        price asc
    ,      id asc
    loop
        raise notice 'order: % % % %', r.id, r.price, r.size, r.sum_size;

        insert into position (open_price, size, side, order_id) values (r.price, r.size, 'short', r.id) returning id into position_id;

        raise notice 'position: %', position_id;

    end loop;
end;
$$

