#ifndef INCLUDE_PSQL_CHANNEL_H_
#define INCLUDE_PSQL_CHANNEL_H_

#include "psql/serialization.h"
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>

template <typename AsyncStream>
class channel
{
	// TODO: static asserts for AsyncStream concept
	// TODO: actually we also require it to be SyncStream, name misleading
	AsyncStream& stream_;
 	std::array<std::uint8_t, 5> header_buffer_ {}; // for async ops
	bytestring shared_buff_; // for async ops

	error_code process_header_read(std::uint32_t& size_to_read); // reads from header_buffer_
	void process_header_write(std::uint32_t size_to_write); // writes to header_buffer_
public:
	channel(AsyncStream& stream): stream_(stream) {}

	void read(bytestring& buffer, std::uint8_t& msg_type)
	{
		boost::asio::read(stream_, boost::asio::buffer(header_buffer_));
		deserialization_context ctx (boost::asio::buffer(header_buffer_));
		auto err = deserialize(msg_type, ctx);
		assert(err == errc::ok);
		std::int32_t size = 0;
		err = deserialize(size, ctx);
		assert(err == errc::ok);
		buffer.resize(size - 4);
		boost::asio::read(stream_, boost::asio::buffer(buffer, buffer.size()));
	}


	template <typename Message>
	void read(Message& msg)
	{
		read(msg, shared_buff_);
	}

	template <typename Message>
	void read(Message& msg, bytestring& buffer)
	{
		std::uint8_t msg_type = 0;
		read(buffer, msg_type);
		if (msg_type != Message::message_type) throw std::runtime_error("Unexpected msg type");
		deserialization_context ctx (boost::asio::buffer(buffer));
		auto err = deserialize_message(msg, ctx);
		check_error_code(err, error_info());
	}

	template <typename Message>
	void write(const Message& msg, bool write_msg_type=true)
	{
		serialization_context ctx;

		std::size_t effective_size = 4 + get_size(msg, ctx);
		std::size_t buffer_size = effective_size +  (write_msg_type ? 1 : 0);

		shared_buff_.resize(buffer_size);
		ctx.set_first(shared_buff_.data());

		if (write_msg_type)
		{
			serialize(Message::message_type, ctx);
		}
		serialize(std::uint32_t(effective_size), ctx);
		serialize(msg, ctx);

		boost::asio::write(stream_, boost::asio::buffer(shared_buff_));
	}

	using stream_type = AsyncStream;
	stream_type& next_layer() { return stream_; }

	const bytestring& shared_buffer() const noexcept { return shared_buff_; }
	bytestring& shared_buffer() noexcept { return shared_buff_; }
};



#endif /* INCLUDE_PSQL_CHANNEL_H_ */
