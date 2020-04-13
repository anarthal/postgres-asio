#include <iostream>
#include "psql/connection.h"
#include <boost/asio/ip/tcp.hpp>

using namespace std;
using boost::asio::ip::tcp;
namespace net = boost::asio;

void print(const value& v)
{
	std::visit([](auto typed_v) {
		std::cout << typed_v;
	}, v);
}

void print(const std::vector<value>& values)
{
	std::cout << "{ ";
	for (const auto& v: values)
	{
		print(v);
		std::cout << ", ";
	}
	std::cout << " }\n";
}

int main()
{
	// Connection
	net::io_context ctx;
	connection<tcp::socket> conn (ctx);
	boost::asio::ip::tcp::endpoint ep (boost::asio::ip::address_v4::loopback(), 5432);

	// Handshake
	conn.next_layer().connect(ep);
	conn.handshake(connection_params{
		"postgres",
		"postgres",
		"awesome"
	});

	// Query (resultset without fields)
	auto result = conn.query("UPDATE mytable SET f1 = 100 WHERE f2 = 'adios'");
	while (const row* r = result.fetch_one())
	{
		print(r->values());
	}
	std::cout << "UPDATE complete\n\n";

	// Query (resultset with fields)
	result = conn.query("SELECT * FROM \"mytable\";");
	while (const row* r = result.fetch_one())
	{
		print(r->values());
	}
	std::cout << "SELECT complete\n\n";

	// Prepared statement (resultset without fields)
	std::array<value, 2> update_values { value(150), value("adios") };
	auto stmt = conn.prepare_statement("UPDATE mytable SET f1 = $1 WHERE f2 = $2");
	result = stmt.execute(std::begin(update_values), std::end(update_values));
	while (const row* r = result.fetch_one())
	{
		print(r->values());
	}
	stmt.close();
	std::cout << "Prepared UPDATE complete\n\n";


	std::array<value, 2> select_values { value("hola"), value("quetal") };
	stmt = conn.prepare_statement("SELECT * FROM mytable WHERE f2 IN ($1, $2)");
	result = stmt.execute(std::begin(select_values), std::end(select_values));
	while (const row* r = result.fetch_one())
	{
		print(r->values());
	}
	stmt.close();
	std::cout << "Prepared SELECT complete\n\n";
}
