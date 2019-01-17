/**
 * Copyright(c) 2019, Tessares S.A.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and / or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *        SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *        OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

#include "convert_util.h"

#define CONVERT_PADDING 4

#define CONVERT_TO_BYTES(v) (v * CONVERT_PADDING)
#define BYTES_TO_CONVERT(v) (v / CONVERT_PADDING)

/* The TLVs in the converter headers are padded to align to 4
 * bytes. These macros are helper functions to compute the expected
 * padded length. Eg. returns 4 for 3, 8 for 5, etc.
 */
#define __ALIGN(x, a) (((x) + (a - 1)) & ~(a - 1))
#define CONVERT_ALIGN(bytes) __ALIGN(bytes, CONVERT_PADDING)

int
convert_parse_header(const uint8_t *buff, size_t buff_len, size_t *tlv_length)
{
	struct convert_header *hdr = (struct convert_header *)buff;

	if (buff_len != sizeof(*hdr))
		return -1;

	/* only support a single version */
	if (hdr->version != CONVERT_VERSION)
		return -1;

	*tlv_length = CONVERT_TO_BYTES(hdr->total_length) - sizeof(*hdr);
	return 0;
}

int
convert_parse_tlvs(const uint8_t *buff, size_t buff_len,
                   struct convert_opts *opts)
{
	/* reset the flags */
	opts->flags = 0;

	while (buff_len > 0) {
		struct convert_tlv *	tlv = (struct convert_tlv *)buff;
		size_t			tlv_len;

		if (buff_len < CONVERT_ALIGN(sizeof(*tlv)))
			return -1;

		tlv_len = CONVERT_TO_BYTES(tlv->length);

		if (buff_len < tlv_len)
			return -1;

		switch (tlv->type) {
		case CONVERT_ERROR: {
			struct convert_error *error =
			        (struct convert_error *)buff;

			if (buff_len < CONVERT_ALIGN(sizeof(*error)))
				return -1;

			opts->flags		|= CONVERT_F_ERROR;
			opts->error_code	= error->error_code;
			/* TODO support handling the field: value */

			break;
		}
		case CONVERT_CONNECT: {
			struct convert_connect *connect =
			        (struct convert_connect *)buff;

			if (buff_len < CONVERT_ALIGN(sizeof(*connect)))
				return -1;

			/* TODO support the options. */
			if (CONVERT_TO_BYTES(connect->length) !=
			    CONVERT_ALIGN(sizeof(*connect)))
				return -1;

			opts->flags		|= CONVERT_F_CONNECT;
			opts->remote_addr	= connect->remote_addr;
			opts->remote_port	= connect->remote_port;

			break;
		}
		/* TODO support other TLVs. */
		default:
			return -1;
		}

		buff		+= tlv_len;
		buff_len	-= tlv_len;
	}
	return 0;
}

static ssize_t
_convert_write_tlv_not_supp(uint8_t *buff, size_t buff_len,
                            struct convert_opts *opts)
{
	return -1;
}

static ssize_t
_convert_write_tlv_connect(uint8_t *buff, size_t buff_len,
                           struct convert_opts *opts)
{
	struct convert_connect *connect = (struct convert_connect *)buff;
	size_t			length	=
	        CONVERT_ALIGN(sizeof(*connect));

	if (buff_len < length)
		return -1;

	connect->remote_addr	= opts->remote_addr;
	connect->remote_port	= opts->remote_port;

	return length;
}

static ssize_t
_convert_write_tlv_error(uint8_t *buff, size_t buff_len,
                         struct convert_opts *opts)
{
	struct convert_error *	error	= (struct convert_error *)buff;
	size_t			length	= CONVERT_ALIGN(sizeof(*error));

	if (buff_len < length)
		return -1;

	error->error_code = opts->error_code;

	return length;
}

static struct {
	uint32_t	flag;
	uint8_t		type;
	ssize_t		(*cb)(uint8_t *buff, size_t buff_len,
	                      struct convert_opts *opts);
} _converter_tlvs[_CONVERT_F_MAX] = {
	[_CONVERT_F_INFO] =		 {
		.flag	= CONVERT_F_INFO,
		.type	= CONVERT_INFO,
		.cb	= _convert_write_tlv_not_supp,
	},
	[_CONVERT_F_CONNECT] =		 {
		.flag	= CONVERT_F_CONNECT,
		.type	= CONVERT_CONNECT,
		.cb	= _convert_write_tlv_connect,
	},
	[_CONVERT_F_EXTENDED_TCP_HDR] =	 {
		.flag	= CONVERT_F_EXTENDED_TCP_HDR,
		.type	= CONVERT_EXTENDED_TCP_HDR,
		.cb	= _convert_write_tlv_not_supp,
	},
	[_CONVERT_F_SUPPORTED_TCP_EXT] = {
		.flag	= CONVERT_F_SUPPORTED_TCP_EXT,
		.type	= CONVERT_SUPPORTED_TCP_EXT,
		.cb	= _convert_write_tlv_not_supp,
	},
	[_CONVERT_F_COOKIE] =		 {
		.flag	= CONVERT_F_COOKIE,
		.type	= CONVERT_COOKIE,
		.cb	= _convert_write_tlv_not_supp,
	},
	[_CONVERT_F_ERROR] =		 {
		.flag	= CONVERT_F_ERROR,
		.type	= CONVERT_ERROR,
		.cb	= _convert_write_tlv_error,
	},
};


ssize_t
_convert_write_tlvs(uint8_t *buff, size_t buff_len, struct convert_opts *opts)
{
	ssize_t len = 0;
	int	i;

	for (i = 0; i < _CONVERT_F_MAX; ++i) {
		struct convert_tlv *	tlv = (struct convert_tlv *)buff;
		ssize_t			ret;

		if (!(_converter_tlvs[i].flag & opts->flags))
			continue;

		ret = _converter_tlvs[i].cb(buff, buff_len, opts);
		if (ret < 0)
			return ret;

		tlv->type	= _converter_tlvs[i].type;
		tlv->length	= BYTES_TO_CONVERT(ret);

		len		+= ret;
		buff		+= ret;
		buff_len	-= ret;

		opts->flags &= ~(_converter_tlvs[i].flag);
	}

	return len;
}

ssize_t
convert_write(uint8_t *buff, size_t buff_len, struct convert_opts *opts)
{
	struct convert_header * hdr	= (struct convert_header *)buff;
	size_t			length	= sizeof(*hdr);
	ssize_t			ret;

	memset(buff, 0, buff_len);

	if (buff_len < length)
		return -1;

	hdr->version = CONVERT_VERSION;

	/* iterate over the opts->flags */
	ret = _convert_write_tlvs(buff + length, buff_len - length, opts);
	if (ret < 0)
		return -1;

	length			+= (size_t)ret;
	hdr->total_length	= BYTES_TO_CONVERT(length);

	return length;
}