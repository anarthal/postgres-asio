#ifndef INCLUDE_PSQL_PREPARED_STATEMENT_H_
#define INCLUDE_PSQL_PREPARED_STATEMENT_H_

#include "psql/channel.h"

template <typename Stream>
class prepared_statement
{
	channel<Stream>* channel_ {};
	std::string name_;

	template <typename ForwardIterator>
	void check_num_params(ForwardIterator first, ForwardIterator last, error_code& err, error_info& info) const;
public:
	/// Default constructor.
	prepared_statement() = default;

	// Private. Do not use.
	prepared_statement(channel<Stream>& chan, std::string&& name) noexcept:
		channel_(&chan), name_(std::move(name)) {}

	bool valid() const noexcept { return channel_ != nullptr; }

	/// Executes a statement (iterator, sync with exceptions version).
	template <typename ForwardIterator>
	resultset<Stream> execute(ForwardIterator params_first, ForwardIterator params_last) const
	{
		// Bind
		channel_->write(bind_message<ForwardIterator>{
			string_null(""), // unnamed portal
			string_null(name_),
			params_first,
			params_last
		});
		channel_->write(flush_message{});
		bind_complete_message bind_complete;
		channel_->read(bind_complete);

		// Issue a describe to get metadata
		channel_->write(describe_message{
			'P',
			string_null("")
		});
		channel_->write(flush_message{});

		// We may get either 'no data' or a row_description
		std::uint8_t msg_type = 0;
		bytestring meta_buffer;
		channel_->read(meta_buffer, msg_type);
		deserialization_context ctx (boost::asio::buffer(meta_buffer));
		resultset_metadata meta;
		if (msg_type == row_description::message_type)
		{
			row_description descr;
			check_error_code(deserialize_message(descr, ctx), error_info());
			meta = make_resultset_metadata(descr, std::move(meta_buffer));
		}
		else if (msg_type != no_data_message::message_type)
		{
			throw std::runtime_error("Unknown message type");
		}

		// Execute
		channel_->write(execute_message{
			string_null("") // unnamed portal
		});
		channel_->write(sync_message{});

		return resultset<Stream>(*channel_, std::move(meta));
	}

	// TODO: close
};



#endif /* INCLUDE_PSQL_PREPARED_STATEMENT_H_ */
