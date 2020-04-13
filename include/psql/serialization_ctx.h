#ifndef INCLUDE_PSQL_SERIALIZATION_CTX_H_
#define INCLUDE_PSQL_SERIALIZATION_CTX_H_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <boost/asio/buffer.hpp>
#include "psql/error.h"

namespace psql
{

class deserialization_context
{
	const std::uint8_t* first_;
	const std::uint8_t* last_;
public:
	deserialization_context(const std::uint8_t* first, const std::uint8_t* last) noexcept:
		first_(first), last_(last) { assert(last_ >= first_); };
	deserialization_context(boost::asio::const_buffer buff) noexcept:
		deserialization_context(
				static_cast<const std::uint8_t*>(buff.data()),
				static_cast<const std::uint8_t*>(buff.data()) + buff.size()
		) {};
	const std::uint8_t* first() const noexcept { return first_; }
	const std::uint8_t* last() const noexcept { return last_; }
	void set_first(const std::uint8_t* new_first) noexcept { first_ = new_first; assert(last_ >= first_); }
	void advance(std::size_t sz) noexcept { first_ += sz; assert(last_ >= first_); }
	void rewind(std::size_t sz) noexcept { first_ -= sz; }
	std::size_t size() const noexcept { return last_ - first_; }
	bool empty() const noexcept { return last_ == first_; }
	bool enough_size(std::size_t required_size) const noexcept { return size() >= required_size; }
	errc copy(void* to, std::size_t sz) noexcept
	{
		if (!enough_size(sz)) return errc::incomplete_message;
		memcpy(to, first_, sz);
		advance(sz);
		return errc::ok;
	}
};

class serialization_context
{
	std::uint8_t* first_;
public:
	serialization_context(std::uint8_t* first = nullptr) noexcept:
		first_(first) {};
	std::uint8_t* first() const noexcept { return first_; }
	void set_first(std::uint8_t* new_first) noexcept { first_ = new_first; }
	void set_first(boost::asio::mutable_buffer buff) noexcept { first_ = static_cast<std::uint8_t*>(buff.data()); }
	void advance(std::size_t size) noexcept { first_ += size; }
	void write(const void* buffer, std::size_t size) noexcept { memcpy(first_, buffer, size); advance(size); }
	void write(std::uint8_t elm) noexcept { *first_ = elm; ++first_; }
};

}

#endif /* INCLUDE_PSQL_SERIALIZATION_CTX_H_ */
