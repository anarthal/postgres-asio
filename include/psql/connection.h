#ifndef INCLUDE_PSQL_CONNECTION_H_
#define INCLUDE_PSQL_CONNECTION_H_

#include "psql/channel.h"
#include "psql/connection_params.h"
#include "psql/auth_md5.h"
#include "psql/resultset.h"
#include "psql/prepared_statement.h"
#include <stdexcept>

template <typename Stream>
class connection
{
	using channel_type = channel<Stream>;

	Stream next_layer_;
	channel_type channel_;
	int curr_stmt_num_ {0};
public:
	template <typename... Args>
	connection(Args&&... args) :
		next_layer_(std::forward<Args>(args)...),
		channel_(next_layer_)
	{
	}

	Stream& next_layer() { return next_layer_; }
	const Stream& next_layer() const { return next_layer_; }

	void handshake(const connection_params& params)
	{
		// Startup
		channel_.write(startup_message{
			196608,
			string_null("user"),
			string_null(params.username),
			string_null("database"),
			string_null(params.database)
		}, false);

		// Auth request
		authentication_request req;
		channel_.read(req);

		// Auth response
		if (req.auth_type != 5) throw std::runtime_error("Unknown auth type");
		std::string auth_res = auth_md5(params.username, params.password, req.auth_data.value);
		channel_.write(password_message{
			string_null(auth_res)
		});

		// Read until ready for query
		std::uint8_t msg_type = 0;
		while (msg_type != 0x5a)
		{
			channel_.read(channel_.shared_buffer(), msg_type);
		}
	}

	resultset<Stream> query(std::string_view query_string)
	{
		// Issue a query
		channel_.write(query_message{
			string_null(query_string)
		});

		// Row descriptions
		bytestring meta_buff;
		row_description descrs;
		channel_.read(descrs, meta_buff);
		return resultset<Stream>(channel_, make_resultset_metadata(descrs, std::move(meta_buff)));
	}

	prepared_statement<Stream> prepare_statement(std::string_view statement)
	{
		// Generate a name
		std::string name = "__psql_asio_" + std::to_string(curr_stmt_num_++);

		// Issue a Parse
		channel_.write(parse_message{
			string_null(name),
			string_null(statement)
		});
		channel_.write(flush_message{});

		// Read response
		parse_complete_message res;
		channel_.read(res);

		return prepared_statement<Stream>(channel_, std::move(name));
	}
};



#endif /* INCLUDE_PSQL_CONNECTION_H_ */
