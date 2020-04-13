#ifndef INCLUDE_PSQL_AUTH_MD5_H_
#define INCLUDE_PSQL_AUTH_MD5_H_

#include <openssl/md5.h>
#include <string_view>
#include <string>
#include <cstdint>

namespace psql
{

inline std::string
bytes_to_hex(std::uint8_t b[16])
{
	std::string s;
	static const char *hex = "0123456789abcdef";
	for (int q = 0; q < 16; q++)
	{
		s.push_back(char(hex[(b[q] >> 4) & 0x0F]));
		s.push_back(char(hex[b[q] & 0x0F]));
	}
	return s;
}

inline std::string md5_str(
	std::string_view first,
	std::string_view second
)
{
	std::string input (first);
	input += second;
	std::uint8_t buffer [MD5_DIGEST_LENGTH];
	MD5((unsigned char*)input.data(), input.size(), buffer);
	return bytes_to_hex(buffer);
}

inline std::string auth_md5(
	std::string_view user,
	std::string_view password,
	std::string_view salt
)
{
	return "md5" + md5_str(md5_str(password, user), salt);
}

}

#endif /* INCLUDE_PSQL_AUTH_MD5_H_ */
