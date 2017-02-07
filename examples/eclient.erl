-module(eclient).
-export([test/3, client/3]).

% name of chunks, allowed Characters: 0..9, A..Z, a..z, ._ (64)
-define(ALLOWED_CHARS, [46|lists:seq(48, 57)] ++ lists:seq(65, 90) ++ [95|lists:seq(97, 122)]).
-define(LEN_AC, erlang:length(?ALLOWED_CHARS)).

% how to compile and run in Erlang shell
% c(eclient).
% timer:tc(eclient, test, ["localhost", 8000, 1000]).
% -define(MAX_LENGTH, 200).
-define(SLEEP_TIME, 100000).
-define(CONN_SLEEP_TIME, 1).

test(_, _, 0) ->
	ok;
test(Server, Port, NumCon) ->
	timer:sleep(?CONN_SLEEP_TIME),
	erlang:spawn(?MODULE, client, [Server, Port, NumCon]),
	test(Server, Port, NumCon-1).

client(Server, Port, Id) ->
	case gen_tcp:connect(Server, Port, [{packet, raw}, {active, false}, binary]) of
		{ok, Sock} ->
			% send 4 byte port
			gen_tcp:send(Sock, binary:bin_to_list(<<Id:32>>)),
			% send 32 bytes
			gen_tcp:send(Sock, gen_rand_str(32)),
			% receive 32 bytes
			{ok, _} = gen_tcp:recv(Sock, 32),
			% send actual data
			{A1,A2,A3} = erlang:now(),
          	random:seed(A1, A2, A3),
			Len = random:uniform(90000)+10000,
			% loop(Sock, random:uniform(90000)+10000, Id, 0);
			send(Sock, Len, Id, 0),
			io:format("sent ~p size of data from process ~p~n", [Len, Id]),
			gen_tcp:close(Sock);
		{error, Reason} ->
			timer:sleep(1000),
			io:format("didn't connect to id ~p for reason:~p~n", [Id, Reason])
	end.

% loop(Sock, AmountData, I, J) when AmountData =< ?MAX_LENGTH ->
% 	send(Sock, AmountData, I, J),
% 	gen_tcp:close(Sock);
% loop(Sock, AmountData, I, J) ->
% 	send(Sock, ?MAX_LENGTH, I, J),
% 	loop(Sock, AmountData-?MAX_LENGTH, I, J+1).

send(Sock, Len, I, J) ->
	timer:sleep(?SLEEP_TIME),
	NewLen = Len + 12,
	gen_tcp:send(Sock, binary:bin_to_list(<<NewLen:32>>)),
	gen_tcp:send(Sock, binary:bin_to_list(<<I:32>>)),
	gen_tcp:send(Sock, binary:bin_to_list(<<J:32>>)),
	Data = gen_rand_str(Len),
	CheckSum = erlang:crc32(Data),
	gen_tcp:send(Sock, binary:bin_to_list(<<CheckSum:32>>)),
	gen_tcp:send(Sock, Data).

% generate random strings of given length
gen_rand_str(Len) ->
    gen_rand_str(Len, []).
gen_rand_str(0, Acc) ->
    Acc;
gen_rand_str(Len, Acc) ->
    Char = lists:nth(random:uniform(?LEN_AC), ?ALLOWED_CHARS),
    gen_rand_str(Len-1, [Char|Acc]).
