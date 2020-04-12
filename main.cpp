#include <iostream>
#include "psql/messages.h"
#include "psql/serialization.h"
#include "psql/channel.h"
#include "psql/messages.h"
#include "psql/auth_md5.h"
#include "psql/deserialize_row.h"
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

void print(const std::vector<std::vector<value>>& rows)
{
	for (const auto& r: rows)
		print(r);
}

int main(int argc, char **argv) {
	net::io_context ctx;
	tcp::socket sock (ctx);
	channel<tcp::socket> chan (sock);
	boost::asio::ip::tcp::endpoint ep (boost::asio::ip::address_v4::loopback(), 5432);

	sock.connect(ep);

	// Startup
	chan.write(startup_message{
		196608,
		string_null("user"),
		string_null("postgres"),
		string_null("database"),
		string_null("awesome")
	}, false);

	// Auth request
	authentication_request req;
	chan.read(req);

	// Auth response
	if (req.auth_type != 5) throw runtime_error("Unknown auth type");
	std::string auth_res = auth_md5("postgres", "postgres", req.auth_data.value);
	chan.write(password_message{
		string_null(auth_res)
	});

	// Read until ready for query
	std::uint8_t msg_type = 0;
	while (msg_type != 0x5a)
	{
		chan.read(chan.shared_buffer(), msg_type);
	}

	std::cout << "Ready for query" << std::endl;

	// Issue a query
	chan.write(query_message{
		string_null("SELECT * FROM \"mytable\";")
	});

	// Row descriptions
	row_description descrs;
	chan.read(descrs);

	// Rows
	std::vector<bytestring> row_buffers;
	std::vector<std::vector<value>> rows;
	while (true)
	{
		bytestring row_buffer;
		chan.read(row_buffer, msg_type);
		if (msg_type == std::uint8_t('C'))
		{
			// command completion
			break;
		}
		rows.push_back(deserialize_row(descrs.rows, row_buffer));
		row_buffers.push_back(std::move(row_buffer));
	};
	print(rows);


}
