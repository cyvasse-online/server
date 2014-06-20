/* Copyright 2014 Jonas Platte
*
* This file is part of Cyvasse Online.
*
* Cyvasse Online is free software: you can redistribute it and/or modify it under the
* terms of the GNU Affero General Public License as published by the Free Software Foundation,
* either version 3 of the License, or (at your option) any later version.
*
* Cyvasse Online is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
* PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _B64_HPP_
#define _B64_HPP_

// default value
#define BUFFERSIZE 16777216
#include <b64/encode.h>
#include <b64/decode.h>
#undef BUFFERSIZE

// this function starts with _ because it's not universally usable
// (it ignores the last 1/4 of bytes of the parameter value)
template <typename IntType>
std::string _intToB64ID(IntType intVal)
{
	const int size = sizeof(IntType) / 4 * 3; // ignore the first 1/4 bytes
	const int sizeEnc = sizeof(IntType); // base64 adds 1/3 of size again

	base64::encoder enc;
	base64_init_encodestate(&enc._state);

	char* cstr = new char[sizeEnc + 1];
	enc.encode(reinterpret_cast<char*>(&intVal), size, cstr);
	cstr[sizeEnc] = '\0';

	std::string retStr(cstr);
	// it's no problem to use a '/' in the URL, but '_' looks better
	for(size_t pos = retStr.find('/'); pos != std::string::npos; pos = retStr.find('/', pos + 1))
		retStr.at(pos) = '_';

	return retStr;
}

#define int24ToB64ID(x) \
	_intToB64ID<uint32_t>(x)
#define int48ToB64ID(x) \
	_intToB64ID<uint64_t>(x)

#endif // _B64_HPP_
