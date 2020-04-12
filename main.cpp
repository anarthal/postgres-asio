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

	auto result = conn.query("SELECT * FROM \"mytable\";");
	while (const row* r = result.fetch_one())
	{
		print(r->values());
	}
}
