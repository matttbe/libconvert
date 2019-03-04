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

#include <check.h>
#include <stdlib.h>
#include "convert_util.h"

START_TEST(test_convert_parse_header){
	int			ret;
	struct convert_header	hdr;
	uint8_t *		buff = (uint8_t *)&hdr;
	size_t			tlvs_length;

	ret = convert_parse_header(buff, sizeof(hdr) - 1, &tlvs_length);
	ck_assert_msg(ret == -1, "Should fail: buff too short");

	ret = convert_parse_header(buff, sizeof(hdr) + 1, &tlvs_length);
	ck_assert_msg(ret == -1, "Should fail: buff too long");

	hdr.version	= CONVERT_VERSION + 1;
	ret		= convert_parse_header(buff, sizeof(hdr), &tlvs_length);
	ck_assert_msg(ret == -1, "Should fail: unsupported version");

	hdr.version		= CONVERT_VERSION;
	hdr.total_length	= 2;
	ret			= convert_parse_header(buff, sizeof(hdr),
	                                               &tlvs_length);
	/* hdr.total_length is in 32-bit words, and include the Convert Header.
	 * tlvs_length is total length of TLVs in bytes, excluding Convert
	 * Header. We thus expect tlvs_length==4, which is total length (2*4)
	 * minus the Convert Header size (4).
	 */
	ck_assert_msg(ret == 0, "Should parse a valid Convert Header");
	ck_assert_msg(tlvs_length == 4,
	              "Should set tlvs_length to the total size of TLVs in bytes");
}
END_TEST

START_TEST(test_convert_parse_tlvs){
	/* TODO */
	ck_assert(1);
}
END_TEST

START_TEST(test_convert_write){
	/* TODO */
	ck_assert(1);
}
END_TEST

Suite *
convert_util_suite(void)
{
	Suite * s;
	TCase * tc_core;

	s = suite_create("convert_util");

	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_convert_parse_header);
	tcase_add_test(tc_core, test_convert_parse_tlvs);
	tcase_add_test(tc_core, test_convert_write);

	suite_add_tcase(s, tc_core);

	return s;
}

int
main(void)
{
	int		number_failed;
	Suite *		s;
	SRunner *	sr;

	s	= convert_util_suite();
	sr	= srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}