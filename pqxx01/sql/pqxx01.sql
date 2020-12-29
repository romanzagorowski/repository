drop table if exists any_order;
drop type  if exists order_action;
drop type  if exists order_type;
drop sequence if exists order_id_seq;

create type order_action as enum ('buy', 'sell');
create type order_type as enum ('stop', 'market', 'limit');
create sequence order_id_seq start with 1;

create table any_order
(
    id      int     not null    default nextval('order_id_seq')  primary key,
    price   int     not null,
    size    int     not null,
    action  order_action not null,
    type    order_type   not null,
    sl      int not null default 0,
    tp      int not null default 0
);

