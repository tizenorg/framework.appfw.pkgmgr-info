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

static void utc_ApplicationFW_pkgmgrinfo_appinfo_filter_count_func_01(void);
static void utc_ApplicationFW_pkgmgrinfo_appinfo_filter_count_func_02(void);
static void utc_ApplicationFW_pkgmgrinfo_appinfo_filter_count_func_03(void);


enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{utc_ApplicationFW_pkgmgrinfo_appinfo_filter_count_func_01, POSITIVE_TC_IDX},
	{utc_ApplicationFW_pkgmgrinfo_appinfo_filter_count_func_02, NEGATIVE_TC_IDX},
	{utc_ApplicationFW_pkgmgrinfo_appinfo_filter_count_func_03, NEGATIVE_TC_IDX},
	{NULL, 0},
};

static void startup(void)
{
}

static void cleanup(void)
{
}

/**
 * @brief Positive test case of pkgmgrinfo_appinfo_filter_count()
 */
static void utc_ApplicationFW_pkgmgrinfo_appinfo_filter_count_func_01(void)
{
	int r = 0;
	int count = 0;
	pkgmgrinfo_appinfo_filter_h handle;
	r = pkgmgrinfo_appinfo_filter_create(&handle);
	if (r != PMINFO_R_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = pkgmgrinfo_appinfo_filter_add_bool(handle, PMINFO_APPINFO_PROP_APP_NODISPLAY, 1);
	if (r != PMINFO_R_OK) {
		pkgmgrinfo_appinfo_filter_destroy(handle);
		tet_result(TET_UNINITIATED);
		return;
	}
	r = pkgmgrinfo_appinfo_filter_add_string(handle, PMINFO_APPINFO_PROP_APP_URI, "http");
	if (r != PMINFO_R_OK) {
		pkgmgrinfo_appinfo_filter_destroy(handle);
		tet_result(TET_UNINITIATED);
		return;
	}
	r = pkgmgrinfo_appinfo_filter_count(handle, &count);
	if (r != PMINFO_R_OK) {
		tet_infoline
		    ("pkgmgrinfo_appinfo_filter_count() failed in positive test case");
		pkgmgrinfo_appinfo_filter_destroy(handle);
		tet_result(TET_FAIL);
		return;
	}
	pkgmgrinfo_appinfo_filter_destroy(handle);
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of pkgmgrinfo_appinfo_filter_count()
 */
static void utc_ApplicationFW_pkgmgrinfo_appinfo_filter_count_func_02(void)
{
	int r = 0;
	int count = 0;
	r = pkgmgrinfo_appinfo_filter_count(NULL, &count);
	if (r == PMINFO_R_OK) {
		tet_infoline
		    ("pkgmgrinfo_appinfo_filter_count() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of pkgmgrinfo_appinfo_filter_count()
 */
static void utc_ApplicationFW_pkgmgrinfo_appinfo_filter_count_func_03(void)
{
	int r = 0;
	pkgmgrinfo_appinfo_filter_h handle;
	r = pkgmgrinfo_appinfo_filter_create(&handle);
	if (r != PMINFO_R_OK) {
		tet_result(TET_UNINITIATED);
		return;
	}
	r = pkgmgrinfo_appinfo_filter_add_bool(handle, PMINFO_APPINFO_PROP_APP_NODISPLAY, 1);
	if (r != PMINFO_R_OK) {
		pkgmgrinfo_appinfo_filter_destroy(handle);
		tet_result(TET_UNINITIATED);
		return;
	}
	r = pkgmgrinfo_appinfo_filter_add_string(handle, PMINFO_APPINFO_PROP_APP_URI, "http");
	if (r != PMINFO_R_OK) {
		pkgmgrinfo_appinfo_filter_destroy(handle);
		tet_result(TET_UNINITIATED);
		return;
	}
	r = pkgmgrinfo_appinfo_filter_count(handle, NULL);
	if (r == PMINFO_R_OK) {
		tet_infoline
		    ("pkgmgrinfo_appinfo_filter_count() failed in negative test case");
		pkgmgrinfo_appinfo_filter_destroy(handle);
		tet_result(TET_FAIL);
		return;
	}
	pkgmgrinfo_appinfo_filter_destroy(handle);
	tet_result(TET_PASS);
}
