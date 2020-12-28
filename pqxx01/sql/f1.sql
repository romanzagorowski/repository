-- test if function can return 2 resultsets...

create or replace function f1()
returns setof refcursor as
$$
declare
    c1 refcursor;
    c2 refcursor;

begin
    open c1 for select * from instant_order;
    return next c1;

    open c2 for select * from waiting_order;
    return next c2;

end;
$$
language plpgsql VOLATILE;

