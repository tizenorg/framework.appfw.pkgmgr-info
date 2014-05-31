/*
 *  slp-pkgmgr
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>, Sewook Park <sewook7.park@samsung.com>,
 * Jaeho Lee <jaeho81.lee@samsung.com>, Shobhit Srivastava <shobhit.s@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <tet_api.h>
#include <pkgmgr-info.h>

static void startup(void);
static void cleanup(void);

void (*tet_startup) (void) = startup;
void (*tet_cleanup) (void) = cleanup;

static void utc_ApplicationFW_pkgmgrinfo_pkginfo_get_list_func_01(void);
static void utc_ApplicationFW_pkgmgrinfo_pkginfo_get_list_func_02(void);
static int __pkg_list_cb(const pkgmgrinfo_pkginfo_h handle, void *user_data);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{utc_ApplicationFW_pkgmgrinfo_pkginfo_get_list_func_01, POSITIVE_TC_IDX},
	{utc_ApplicationFW_pkgmgrinfo_pkginfo_get_list_func_02, NEGATIVE_TC_IDX},
	{NULL, 0},
};

static void startup(void)
{
}

static void cleanup(void)
{
}

static int __pkg_list_cb(const pkgmgrinfo_pkginfo_h handle, void *user_data)
{
	return 0;
}

/**
 * @brief Positive test case of pkgmgrinfo_pkginfo_get_list()
 */
static void utc_ApplicationFW_pkgmgrinfo_pkginfo_get_list_func_01(void)
{
	int r = 0;
	r = pkgmgrinfo_pkginfo_get_list(__pkg_list_cb, NULL);
	if (r != PMINFO_R_OK) {
		tet_infoline
		    ("pkgmgrinfo_pkginfo_get_list() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of pkgmgrinfo_pkginfo_get_list()
 */
static void utc_ApplicationFW_pkgmgrinfo_pkginfo_get_list_func_02(void)
{
	int r = 0;
	r = pkgmgrinfo_pkginfo_get_list(NULL, NULL);
	if (r == PMINFO_R_OK) {
		tet_infoline
		    ("pkgmgrinfo_pkginfo_get_list() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
