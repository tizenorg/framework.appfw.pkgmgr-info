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


#ifndef __PKGMGR_PARSER_FEATURE_H__
#define __PKGMGR_PARSER_FEATURE_H__

#ifdef __cplusplus
extern "C" {
#endif

int __init_tables_for_wearable(void);
int pkgmgr_parser_insert_disabled_pkg(const char *pkgid, char *const tagv[]);
int pkgmgr_parser_delete_disabled_pkg(const char *pkgid, char *const tagv[]);
int pkgmgr_parser_insert_disabled_pkg_info_in_db(manifest_x *mfx);
int pkgmgr_parser_delete_disabled_pkgid_info_from_db(const char *pkgid);

#ifdef __cplusplus
}
#endif
#endif /* __PKGPMGR_PARSER_FEATURE_H__ */
