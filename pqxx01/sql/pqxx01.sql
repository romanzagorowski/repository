drop table if exists waiting_order;
drop table if exists instant_order;
drop type if exists order_action;
drop type if exists order_wait_type;
drop sequence if exists order_id_seq;

create type order_action as enum ('buy', 'sell');
create type order_wait_type as enum ('limit', 'stop');
create sequence order_id_seq start with 1;

create table waiting_order
(
    id      int     not null    default nextval('order_id_seq')  primary key,
    price   int     not null,
    size    int     not null,
    action      order_action    not null,
    wait_type   order_wait_type not null
);

create table instant_order
(
    id      int     not null    default nextval('order_id_seq')  primary key,
    size    int     not null,
    action      order_action    not null
);

