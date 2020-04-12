#ifndef INCLUDE_PSQL_SERIALIZATION_H_
#define INCLUDE_PSQL_SERIALIZATION_H_

#include "serialization_ctx.h"
#include "types.h"
#include <type_traits>
#include <boost/endian/conversion.hpp>

enum class serialization_tag
{
	none,
	int_,
	struct_with_fields
};

template <typename T>
constexpr bool is_struct_with_fields();

template <typename T>
constexpr serialization_tag get_serialization_tag()
{
	if (std::is_integral_v<T>) return serialization_tag::int_;
	else if (is_struct_with_fields<T>()) return serialization_tag::struct_with_fields;
	else return serialization_tag::none;
}

template <typename T, serialization_tag=get_serialization_tag<T>()>
struct serialization_traits;

template <typename T>
errc deserialize(T& output, deserialization_context& ctx) noexcept
{
	return serialization_traits<T>::deserialize_(output, ctx);
}

template <typename T>
void serialize(const T& input, serialization_context& ctx) noexcept
{
	serialization_traits<T>::serialize_(input, ctx);
}

template <typename T>
std::size_t get_size(const T& input, const serialization_context& ctx) noexcept
{
	return serialization_traits<T>::get_size_(input, ctx);
}

// Integers
template <typename T>
struct serialization_traits<T, serialization_tag::int_>
{
	static constexpr auto sz = sizeof(T);
	static errc deserialize_(T& output, deserialization_context& ctx) noexcept
	{
		if (!ctx.enough_size(sz))
		{
			return errc::incomplete_message;
		}

		memcpy(&output, ctx.first(), sz);
		boost::endian::big_to_native_inplace(output);
		ctx.advance(sz);

		return errc::ok;
	}
	static void serialize_(T input, serialization_context& ctx) noexcept
	{
		boost::endian::native_to_big_inplace(input);
		ctx.write(&input, sz);
	}
	static constexpr std::size_t get_size_(T, const serialization_context&) noexcept { return sz; }
};

// strings
inline std::string_view get_string(
	const std::uint8_t* from,
	std::size_t size
)
{
	return std::string_view(reinterpret_cast<const char*>(from), size);
}

template <>
struct serialization_traits<string_null, serialization_tag::none>
{
	static inline errc deserialize_(string_null& output, deserialization_context& ctx) noexcept
	{
		auto string_end = std::find(ctx.first(), ctx.last(), 0);
		if (string_end == ctx.last())
		{
			return errc::incomplete_message;
		}
		output.value = get_string(ctx.first(), string_end-ctx.first());
		ctx.set_first(string_end + 1); // skip the null terminator
		return errc::ok;
	}
	static inline void serialize_(string_null input, serialization_context& ctx) noexcept
	{
		ctx.write(input.value.data(), input.value.size());
		ctx.write(0); // null terminator
	}
	static inline std::size_t get_size_(string_null input, const serialization_context&) noexcept
	{
		return input.value.size() + 1;
	}
};

// string_eof
template <>
struct serialization_traits<string_eof, serialization_tag::none>
{
	static inline errc deserialize_(string_eof& output, deserialization_context& ctx) noexcept
	{
		output.value = get_string(ctx.first(), ctx.last()-ctx.first());
		ctx.set_first(ctx.last());
		return errc::ok;
	}
	static inline void serialize_(string_eof input, serialization_context& ctx) noexcept
	{
		ctx.write(input.value.data(), input.value.size());
	}
	static inline std::size_t get_size_(string_eof input, const serialization_context&) noexcept
	{
		return input.value.size();
	}
};

// string_lenenc
template <>
struct serialization_traits<string_lenenc, serialization_tag::none>
{
	static inline errc deserialize_(string_lenenc& output, deserialization_context& ctx) noexcept
	{
		std::int32_t length;
		errc err = deserialize(length, ctx);
		if (err != errc::ok)
		{
			return err;
		}
		if (!ctx.enough_size(length))
		{
			return errc::incomplete_message;
		}

		output.value = get_string(ctx.first(), length);
		ctx.advance(length);
		return errc::ok;
	}
	static inline void serialize_(string_lenenc input, serialization_context& ctx) noexcept
	{
		serialize(std::int32_t(input.value.size()), ctx);
		ctx.write(input.value.data(), input.value.size());
	}
	static inline std::size_t get_size_(string_lenenc input, const serialization_context& ctx) noexcept
	{
		return 4 + input.value.size();
	}
};

// Structs and commands (messages)
// To allow a limited way of reflection, structs should
// specialize get_struct_fields with a tuple of pointers to members,
// thus defining which fields should be (de)serialized in the struct
// and in which order
struct not_a_struct_with_fields {}; // Tag indicating a type is not a struct with fields

template <typename T>
struct get_struct_fields
{
	static constexpr not_a_struct_with_fields value {};
};

template <typename T>
constexpr bool is_struct_with_fields()
{
	return !std::is_same_v<
		std::decay_t<decltype(get_struct_fields<T>::value)>,
		not_a_struct_with_fields
	>;
}

// Helpers for structs
struct is_command_helper
{
    template <typename T>
    static constexpr std::true_type get(decltype(T::command_id)*);

    template <typename T>
    static constexpr std::false_type get(...);
};

template <typename T>
struct is_command : decltype(is_command_helper::get<T>(nullptr))
{
};

template <std::size_t index, typename T>
errc deserialize_struct(
	[[maybe_unused]] T& output,
	[[maybe_unused]] deserialization_context& ctx
) noexcept
{
	constexpr auto fields = get_struct_fields<T>::value;
	if constexpr (index == std::tuple_size<decltype(fields)>::value)
	{
		return errc::ok;
	}
	else
	{
		constexpr auto pmem = std::get<index>(fields);
		errc err = deserialize(output.*pmem, ctx);
		if (err != errc::ok)
		{
			return err;
		}
		else
		{
			return deserialize_struct<index+1>(output, ctx);
		}
	}
}

template <std::size_t index, typename T>
void serialize_struct(
	[[maybe_unused]] const T& value,
	[[maybe_unused]] serialization_context& ctx
) noexcept
{
	constexpr auto fields = get_struct_fields<T>::value;
	if constexpr (index < std::tuple_size<decltype(fields)>::value)
	{
		auto pmem = std::get<index>(fields);
		serialize(value.*pmem, ctx);
		serialize_struct<index+1>(value, ctx);
	}
}

template <std::size_t index, typename T>
std::size_t get_size_struct(
	[[maybe_unused]] const T& input,
	[[maybe_unused]] const serialization_context& ctx
) noexcept
{
	constexpr auto fields = get_struct_fields<T>::value;
	if constexpr (index == std::tuple_size<decltype(fields)>::value)
	{
		return 0;
	}
	else
	{
		constexpr auto pmem = std::get<index>(fields);
		return get_size_struct<index+1>(input, ctx) +
		       get_size(input.*pmem, ctx);
	}
}

// Helpers for (de)serialize_fields
template <typename FirstType>
errc deserialize_fields_helper(deserialization_context& ctx, FirstType& field) noexcept
{
	return deserialize(field, ctx);
}

template <typename FirstType, typename... Types>
errc deserialize_fields_helper(
	deserialization_context& ctx,
	FirstType& field,
	Types&... fields_tail
) noexcept
{
	errc err = deserialize(field, ctx);
	if (err == errc::ok)
	{
		err = deserialize_fields_helper(ctx, fields_tail...);
	}
	return err;
}

template <typename FirstType>
void serialize_fields_helper(serialization_context& ctx, const FirstType& field) noexcept
{
	serialize(field, ctx);
}

template <typename FirstType, typename... Types>
void serialize_fields_helper(
	serialization_context& ctx,
	const FirstType& field,
	const Types&... fields_tail
)
{
	serialize(field, ctx);
	serialize_fields_helper(ctx, fields_tail...);
}

template <typename T>
struct serialization_traits<T, serialization_tag::struct_with_fields>
{
	static errc deserialize_(T& output, deserialization_context& ctx) noexcept
	{
		return deserialize_struct<0>(output, ctx);
	}
	static void serialize_(const T& input, serialization_context& ctx) noexcept
	{
		if constexpr (is_command<T>::value)
		{
			serialize(T::command_id, ctx);
		}
		serialize_struct<0>(input, ctx);
	}
	static std::size_t get_size_(const T& input, const serialization_context& ctx) noexcept
	{
		std::size_t res = is_command<T>::value ? 1 : 0;
		res += get_size_struct<0>(input, ctx);
		return res;
	}
};

// Use these to make all messages implement all methods, leaving the not required
// ones with a default implementation. While this is not strictly required,
// it simplifies the testing infrastructure
template <typename T>
struct noop_serialize
{
	static inline std::size_t get_size_(const T&, const serialization_context&) noexcept { return 0; }
	static inline void serialize_(const T&, serialization_context&) noexcept {}
};

template <typename T>
struct noop_deserialize
{
	static inline errc deserialize_(T&, deserialization_context&) noexcept { return errc::ok; }
};

// Helper to serialize top-level messages
template <typename Serializable, typename Allocator>
void serialize_message(
	const Serializable& input,
	bytestring& buffer
)
{
	serialization_context ctx;
	std::size_t size = get_size(input, ctx);
	buffer.resize(size);
	ctx.set_first(buffer.data());
	serialize(input, ctx);
	assert(ctx.first() == buffer.data() + buffer.size());
}

template <typename Deserializable>
error_code deserialize_message(
	Deserializable& output,
	deserialization_context& ctx
)
{
	auto err = deserialize(output, ctx);
	if (err != errc::ok) return make_error_code(err);
	if (!ctx.empty()) return make_error_code(errc::extra_bytes);
	return error_code();
}

// Helpers for (de) serializing a set of fields
/**template <typename... Types>
errc deserialize_fields(deserialization_context& ctx, Types&... fields) noexcept;

template <typename... Types>
void serialize_fields(serialization_context& ctx, const Types&... fields) noexcept;

inline std::pair<error_code, std::uint8_t> deserialize_message_type(
	deserialization_context& ctx
);*/



#endif /* INCLUDE_PSQL_SERIALIZATION_H_ */
