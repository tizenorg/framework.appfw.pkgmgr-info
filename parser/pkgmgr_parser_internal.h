/*
 * pkgmgr-info
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





#ifndef __PKGMGR_PARSER_INTERNAL_H__
#define __PKGMGR_PARSER_INTERNAL_H__

#include "pkgmgrinfo_basic.h"

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif

void pkgmgr_parser_close_db();
void __add_preload_info(manifest_x * mfx, const char *manifest);
void __extract_data(gpointer data, GList *lbl, GList *lcn, GList *icn, GList *dcn, GList *ath,
		char **label, char **license, char **icon, char **description, char **author);
GList *__create_locale_list(GList *locale, GList *lbl, GList *lcn, GList *icn, GList *dcn, GList *ath);
int __exec_query_no_msg(char *query);
void __trimfunc(GList* trim_list);
const char *__get_str(const char *str);
int __initialize_db(sqlite3 *db_handle, char *db_query);
int __exec_query(char *query);
int __evaluate_query(sqlite3 *db_handle, char *query);
char *__zone_get_root_path(const char *zone);

#endif				/* __PKGMGR_PARSER_INTERNAL_H__ */
