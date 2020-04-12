#ifndef INCLUDE_PSQL_ERROR_H_
#define INCLUDE_PSQL_ERROR_H_

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

using boost::system::error_code;

enum class errc : int
{
	ok,
	incomplete_message,
	protocol_value_error,
	extra_bytes
};

class error_info
{
	std::string msg_;
public:
	/// Default constructor.
	error_info() = default;

	/// Initialization constructor.
	error_info(std::string&& err) noexcept: msg_(std::move(err)) {}

	/// Gets the error message.
	const std::string& message() const noexcept { return msg_; }

	/// Sets the error message.
	void set_message(std::string&& err) { msg_ = std::move(err); }

	/// Restores the object to its initial state.
	void clear() noexcept { msg_.clear(); }
};

/**
 * \relates error_info
 * \brief Compare two error_info objects.
 */
inline bool operator==(const error_info& lhs, const error_info& rhs) noexcept { return lhs.message() == rhs.message(); }

/**
 * \relates error_info
 * \brief Compare two error_info objects.
 */
inline bool operator!=(const error_info& lhs, const error_info& rhs) noexcept { return !(lhs==rhs); }

/**
 * \relates error_info
 * \brief Streams an error_info.
 */
inline std::ostream& operator<<(std::ostream& os, const error_info& v) { return os << v.message(); }

inline const char* error_to_string(errc error) noexcept
{
	switch (error)
	{
	case errc::ok: return "no error";
	case errc::incomplete_message: return "The message read was incomplete (not enough bytes to fully decode it)";
	default: return "<unknown error>";
	}
}

class psql_error_category_t : public boost::system::error_category
{
public:
	const char* name() const noexcept final override { return "mysql"; }
	std::string message(int ev) const final override
	{
		return error_to_string(static_cast<errc>(ev));
	}
};
inline psql_error_category_t psql_error_category;

inline boost::system::error_code make_error_code(errc error)
{
	return boost::system::error_code(static_cast<int>(error), psql_error_category);
}

inline void check_error_code(const error_code& code, const error_info& info)
{
	if (code)
	{
		throw boost::system::system_error(code, info.message());
	}
}

inline void conditional_clear(error_info* info) noexcept
{
	if (info)
	{
		info->clear();
	}
}

inline void conditional_assign(error_info* to, error_info&& from)
{
	if (to)
	{
		*to = std::move(from);
	}
}


#endif /* INCLUDE_PSQL_ERROR_H_ */
