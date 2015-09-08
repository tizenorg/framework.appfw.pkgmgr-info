/*
 * pkgmgrinfo_feature
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

#ifndef __PKGMGRINFO_FEATURE_H__
#define __PKGMGRINFO_FEATURE_H__

#ifdef __cplusplus
extern "C" {
#endif

int pkgmgrinfo_appinfo_get_disabled_list(pkgmgrinfo_pkginfo_h handle, pkgmgrinfo_app_component component,
		pkgmgrinfo_app_list_cb app_func, void *user_data);
int pkgmgrinfo_appinfo_get_disabled_appinfo(const char *appid, pkgmgrinfo_appinfo_h *handle);
int pkgmgrinfo_pkginfo_get_disabled_list(pkgmgrinfo_pkg_list_cb pkg_list_cb, void *user_data);
int pkgmgrinfo_pkginfo_get_disabled_pkginfo(const char *pkgid, pkgmgrinfo_pkginfo_h *handle);

#ifdef __cplusplus
}
#endif
#endif  /* __PKGPMGRINFO_FEATURE_H__ */
