#ifndef INCLUDE_PSQL_BYTESTRING_H_
#define INCLUDE_PSQL_BYTESTRING_H_

#include <vector>
#include <cstdint>
#include <type_traits>
#include <string_view>

template <typename T>
struct value_holder
{
	using value_type = T;
	static_assert(std::is_nothrow_default_constructible_v<T>);

	value_type value;

	value_holder() noexcept: value{} {};

	template <typename U>
	explicit constexpr value_holder(U&& u)
		noexcept(std::is_nothrow_constructible_v<T, decltype(u)>):
		value(std::forward<U>(u)) {}

	constexpr bool operator==(const value_holder<T>& rhs) const { return value == rhs.value; }
	constexpr bool operator!=(const value_holder<T>& rhs) const { return value != rhs.value; }
};

using bytestring = std::vector<std::uint8_t>;

struct string_null : value_holder<std::string_view> { using value_holder::value_holder; };
struct string_eof : value_holder<std::string_view> { using value_holder::value_holder; };
struct string_lenenc : value_holder<std::string_view> { using value_holder::value_holder; };


#endif /* INCLUDE_PSQL_BYTESTRING_H_ */
