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
#ifndef __PKGMGR_PARSER_PLUGIN_H_
#define __PKGMGR_PARSER_PLUGIN_H_

#include "pkgmgr_parser.h"

#ifdef __cplusplus
	 extern "C" {
#endif				/* __cplusplus */

typedef void* pkgmgr_parser_plugin_h;

/* plugin process_type */
typedef enum {
	PLUGIN_PRE_PROCESS = 0,
	PLUGIN_POST_PROCESS
} PLUGIN_PROCESS_TYPE;

#define PKGMGR_PARSER_PLUGIN_TAG		"tag"
#define PKGMGR_PARSER_PLUGIN_METADATA	"metadata"
#define PKGMGR_PARSER_PLUGIN_CATEGORY	"category"

int __ps_run_parser(xmlDocPtr docPtr, const char *tag, ACTION_TYPE action, const char *pkgid);

int _pkgmgr_parser_plugin_open_db();
void _pkgmgr_parser_plugin_close_db();
void _pkgmgr_parser_plugin_process_plugin(manifest_x *mfx, const char *filename, ACTION_TYPE action);
void _pkgmgr_parser_plugin_uninstall_plugin(const char *plugin_type, const char *pkgid, const char *appid);

#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif				/* __PKGMGR_PARSER_PLUGIN_H_ */
