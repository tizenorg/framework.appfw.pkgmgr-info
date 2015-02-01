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
#define _GNU_SOURCE
#include <dlfcn.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlschemas.h>
#include <vconf.h>
#include <glib.h>
#include <db-util.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>

#include "pkgmgr-info.h"
#include "pkgmgr_parser.h"
#include "pkgmgr_parser_internal.h"
#include "pkgmgr_parser_db.h"
#include "pkgmgr_parser_db_util.h"
#include "pkgmgr_parser_signature.h"
#include "pkgmgr_parser_plugin.h"

#include "pkgmgrinfo_debug.h"
#include "pkgmgr_parser_feature.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_PARSER"

#define MAX_QUERY_LEN		4096

sqlite3 *pkgmgr_parser_db;
sqlite3 *pkgmgr_cert_db;

#define QUERY_CREATE_TABLE_DISABLED_PACKAGE_INFO "create table if not exists disabled_package_info " \
						"(package text primary key not null, " \
						"package_type text DEFAULT 'rpm', " \
						"package_version text, " \
						"install_location text, " \
						"package_size text, " \
						"package_removable text DEFAULT 'true', " \
						"package_preload text DEFAULT 'false', " \
						"package_readonly text DEFAULT 'false', " \
						"package_update text DEFAULT 'false', " \
						"package_appsetting text DEFAULT 'false', " \
						"package_nodisplay text DEFAULT 'false', " \
						"package_system text DEFAULT 'false', " \
						"author_name text, " \
						"author_email text, " \
						"author_href text," \
						"installed_time text," \
						"installed_storage text," \
						"storeclient_id text," \
						"mainapp_id text," \
						"package_url text," \
						"root_path text," \
						"csc_path text)"

#define QUERY_CREATE_TABLE_DISABLED_PACKAGE_APP_INFO "create table if not exists disabled_package_app_info " \
						"(app_id text primary key not null, " \
						"app_component text, " \
						"app_exec text, " \
						"app_nodisplay text DEFAULT 'false', " \
						"app_type text, " \
						"app_onboot text DEFAULT 'false', " \
						"app_multiple text DEFAULT 'false', " \
						"app_autorestart text DEFAULT 'false', " \
						"app_taskmanage text DEFAULT 'false', " \
						"app_enabled text DEFAULT 'true', " \
						"app_hwacceleration text DEFAULT 'use-system-setting', " \
						"app_screenreader text DEFAULT 'use-system-setting', " \
						"app_mainapp text, " \
						"app_recentimage text, " \
						"app_launchcondition text, " \
						"app_indicatordisplay text DEFAULT 'true', " \
						"app_portraitimg text, " \
						"app_landscapeimg text, " \
						"app_guestmodevisibility text DEFAULT 'true', " \
						"app_permissiontype text DEFAULT 'normal', " \
						"app_preload text DEFAULT 'false', " \
						"app_submode text DEFAULT 'false', " \
						"app_submode_mainid text, " \
						"app_installed_storage text, " \
						"component_type text, " \
						"package text not null, " \
						"FOREIGN KEY(package) " \
						"REFERENCES package_info(package) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_DISABLED_PACKAGE_LOCALIZED_INFO "create table if not exists disabled_package_localized_info " \
							"(package text not null, " \
							"package_locale text DEFAULT 'No Locale', " \
							"package_label text, " \
							"package_icon text, " \
							"package_description text, " \
							"package_license text, " \
							"package_author, " \
							"PRIMARY KEY(package, package_locale), " \
							"FOREIGN KEY(package) " \
							"REFERENCES package_info(package) " \
							"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_DISABLED_PACKAGE_APP_LOCALIZED_INFO "create table if not exists disabled_package_app_localized_info " \
						"(app_id text not null, " \
						"app_locale text DEFAULT 'No Locale', " \
						"app_label text, " \
						"app_icon text, " \
						"package text not null, " \
						"PRIMARY KEY(app_id,app_locale) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_DISABLED_PACKAGE_APP_APP_CATEGORY "create table if not exists disabled_package_app_app_category " \
						"(app_id text not null, " \
						"category text not null, " \
						"package text not null, " \
						"PRIMARY KEY(app_id,category) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"


#define QUERY_CREATE_TABLE_DISABLED_PACKAGE_APP_APP_METADATA "create table if not exists disabled_package_app_app_metadata " \
						"(app_id text not null, " \
						"md_key text not null, " \
						"md_value text not null, " \
						"package text not null, " \
						"PRIMARY KEY(app_id, md_key, md_value) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

int __init_tables_for_wearable(void)
{
	int ret = 0;

	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_DISABLED_PACKAGE_INFO);
	if (ret == -1) {
		_LOGD("package pkg reserve info DB initialization failed\n");
		return ret;
	}

	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_DISABLED_PACKAGE_APP_INFO);
	if (ret == -1) {
		_LOGD("package pkg reserve info DB initialization failed\n");
		return ret;
	}

	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_DISABLED_PACKAGE_LOCALIZED_INFO);
	if (ret == -1) {
		_LOGD("package pkg reserve info DB initialization failed\n");
		return ret;
	}

	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_DISABLED_PACKAGE_APP_LOCALIZED_INFO);
	if (ret == -1) {
		_LOGD("package pkg reserve info DB initialization failed\n");
		return ret;
	}

	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_DISABLED_PACKAGE_APP_APP_CATEGORY);
	if (ret == -1) {
		_LOGD("package pkg reserve info DB initialization failed\n");
		return ret;
	}

	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_DISABLED_PACKAGE_APP_APP_METADATA);

	if (ret == -1) {
		_LOGD("package pkg reserve info DB initialization failed\n");
		return ret;
	}

	return ret;
}

static int __insert_disabled_ui_mainapp_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	int ret = -1;
	char *query = NULL;
	while(up != NULL)
	{
		query = sqlite3_mprintf("update disabled_package_app_info set app_mainapp=%Q where app_id=%Q", up->mainapp, up->appid);

		ret = __exec_query(query);
		sqlite3_free(query);
		if (ret == -1) {
			_LOGD("Package UiApp Info DB Insert Failed\n");
			return -1;
		}
		if (strcasecmp(up->mainapp, "True")==0)
			mfx->mainapp_id = strdup(up->appid);

		up = up->next;
	}

	if (mfx->mainapp_id == NULL){
		if (mfx->uiapplication && mfx->uiapplication->appid) {
			query = sqlite3_mprintf("update disabled_package_app_info set app_mainapp='true' where app_id=%Q", mfx->uiapplication->appid);
		} else {
			_LOGD("Not valid appid\n");
			return -1;
		}

		ret = __exec_query(query);
		sqlite3_free(query);
		if (ret == -1) {
			_LOGD("Package UiApp Info DB Insert Failed\n");
			return -1;
		}

		free((void *)mfx->uiapplication->mainapp);
		mfx->uiapplication->mainapp= strdup("true");
		mfx->mainapp_id = strdup(mfx->uiapplication->appid);
	}

	query = sqlite3_mprintf("update disabled_package_info set mainapp_id=%Q where package=%Q", mfx->mainapp_id, mfx->package);
	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1) {
		_LOGD("Package Info DB update Failed\n");
		return -1;
	}

	return 0;
}

static int __insert_disabled_uiapplication_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	int ret = -1;
	char *query = NULL;
	while(up != NULL)
	{
		query = sqlite3_mprintf("insert into disabled_package_app_info(app_id, app_component, app_exec, app_nodisplay, app_type, app_onboot, " \
			"app_multiple, app_autorestart, app_taskmanage, app_enabled, app_hwacceleration, app_screenreader, app_mainapp , app_recentimage, " \
			"app_launchcondition, app_indicatordisplay, app_portraitimg, app_landscapeimg, app_guestmodevisibility, app_permissiontype, "\
			"app_preload, app_submode, app_submode_mainid, app_installed_storage, component_type, package) " \
			"values(%Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q)",\
			 up->appid,
			 "uiapp",
			 up->exec,
			 up->nodisplay,
			 up->type,
			 PKGMGR_PARSER_EMPTY_STR,
			 up->multiple,
			 PKGMGR_PARSER_EMPTY_STR,
			 up->taskmanage,
			 up->enabled,
			 up->hwacceleration,
			 up->screenreader,
			 up->mainapp,
			 __get_str(up->recentimage),
			 up->launchcondition,
			 up->indicatordisplay,
			 __get_str(up->portraitimg),
			 __get_str(up->landscapeimg),
			 up->guestmode_visibility,
			 up->permission_type,
			 mfx->preload,
			 up->submode,
			 __get_str(up->submode_mainid),
			 mfx->installed_storage,
			 up->component_type,
			 mfx->package);

		ret = __exec_query(query);
		sqlite3_free(query);
		if (ret == -1) {
			_LOGD("Package UiApp Info DB Insert Failed\n");
			return -1;
		}
		up = up->next;
	}
	return 0;
}

static void __insert_disabled_uiapplication_locale_info(gpointer data, gpointer userdata)
{
	int ret = -1;
	char *label = NULL;
	char *icon = NULL;
	char *query = NULL;

	uiapplication_x *up = (uiapplication_x*)userdata;
	label_x *lbl = up->label;
	icon_x *icn = up->icon;

	__extract_data(data, lbl, NULL, icn, NULL, NULL, &label, NULL, &icon, NULL, NULL);
	if (!label && !icon)
		return;
	query = sqlite3_mprintf("insert into disabled_package_app_localized_info(app_id, app_locale, " \
		"app_label, app_icon, package) values " \
		"(%Q, %Q, %Q, %Q, %Q)", up->appid, (char*)data,
		label, icon, up->package);
	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1)
		_LOGD("Package UiApp Localized Info DB Insert failed\n");

	/*insert ui app locale info to pkg locale to get mainapp data */
	if (strcasecmp(up->mainapp, "true")==0) {
		char *update_query = NULL;
		update_query = sqlite3_mprintf("insert into disabled_package_localized_info(package, package_locale, " \
			"package_label, package_icon, package_description, package_license, package_author) values " \
			"(%Q, %Q, %Q, %Q, %Q, %Q, %Q)",
			up->package,
			(char*)data,
			label,
			icon,
			PKGMGR_PARSER_EMPTY_STR,
			PKGMGR_PARSER_EMPTY_STR,
			PKGMGR_PARSER_EMPTY_STR);

		ret = __exec_query_no_msg(update_query);
		sqlite3_free(update_query);

		if (icon != NULL) {
			update_query = sqlite3_mprintf("update package_localized_info set package_icon=%Q " \
				"where package=%Q and package_locale=%Q", icon, up->package, (char*)data);
			ret = __exec_query_no_msg(update_query);
			sqlite3_free(update_query);
		}
	}
}

static int __insert_disabled_uiapplication_appcategory_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	category_x *ct = NULL;
	int ret = -1;
	char *query = NULL;
	while(up != NULL)
	{
		ct = up->category;
		while (ct != NULL)
		{
			query = sqlite3_mprintf("insert into disabled_package_app_app_category(app_id, category, package) " \
				"values(%Q, %Q, %Q)",\
				 up->appid, ct->name, up->package);
			ret = __exec_query(query);
			sqlite3_free(query);
			if (ret == -1) {
				_LOGD("Package UiApp Category Info DB Insert Failed\n");
				return -1;
			}
			ct = ct->next;
		}
		up = up->next;
	}
	return 0;
}

static int __insert_disabled_uiapplication_appmetadata_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	metadata_x *md = NULL;
	int ret = -1;
	char *query = NULL;
	while(up != NULL)
	{
		md = up->metadata;
		while (md != NULL)
		{
			if (md->key) {
				query = sqlite3_mprintf("insert into disabled_package_app_app_metadata(app_id, md_key, md_value, package) " \
					"values(%Q, %Q, %Q, %Q)",\
					 up->appid, md->key, md->value, up->package);
				ret = __exec_query(query);
				sqlite3_free(query);
				if (ret == -1) {
					_LOGD("Package UiApp Metadata Info DB Insert Failed\n");
					return -1;
				}
			}
			md = md->next;
		}
		up = up->next;
	}
	return 0;
}

static void __insert_disabled_pkglocale_info(gpointer data, gpointer userdata)
{
	int ret = -1;
	char *label = NULL;
	char *icon = NULL;
	char *description = NULL;
	char *license = NULL;
	char *author = NULL;
	char *query = NULL;

	manifest_x *mfx = (manifest_x *)userdata;
	label_x *lbl = mfx->label;
	license_x *lcn = mfx->license;
	icon_x *icn = mfx->icon;
	description_x *dcn = mfx->description;
	author_x *ath = mfx->author;

	__extract_data(data, lbl, lcn, icn, dcn, ath, &label, &license, &icon, &description, &author);
	if (!label && !description && !icon && !license && !author)
		return;

	query = sqlite3_mprintf("insert into disabled_package_localized_info(package, package_locale, " \
		"package_label, package_icon, package_description, package_license, package_author) values " \
		"(%Q, %Q, %Q, %Q, %Q, %Q, %Q)",
		mfx->package,
		(char*)data,
		label,
		icon,
		__get_str(description),
		__get_str(license),
		__get_str(author));

	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1)
		_LOGD("Package Localized Info DB Insert failed\n");
}

static int __insert_disabled_pkg_info_in_db(manifest_x *mfx)
{
	label_x *lbl = mfx->label;
	license_x *lcn = mfx->license;
	icon_x *icn = mfx->icon;
	description_x *dcn = mfx->description;
	author_x *ath = mfx->author;
	uiapplication_x *up = mfx->uiapplication;

	char *query = NULL;
	char root[MAX_QUERY_LEN] = { '\0' };
	int ret = -1;
	char *type = NULL;
	char *path = NULL;
	const char *auth_name = NULL;
	const char *auth_email = NULL;
	const char *auth_href = NULL;

	GList *pkglocale = NULL;
	GList *applocale = NULL;

	if (ath) {
		if (ath->text)
			auth_name = ath->text;
		if (ath->email)
			auth_email = ath->email;
		if (ath->href)
			auth_href = ath->href;
	}

	/*Insert in the package_info DB*/
	if (mfx->type)
		type = strdup(mfx->type);
	else
		type = strdup("rpm");
	/*Insert in the package_info DB*/
	if (mfx->root_path)
		path = strdup(mfx->root_path);
	else{
		if (strcmp(type,"rpm")==0)
			snprintf(root, MAX_QUERY_LEN - 1, "/usr/apps/%s", mfx->package);
		else
			snprintf(root, MAX_QUERY_LEN - 1, "/opt/usr/apps/%s", mfx->package);

		path = strdup(root);
	}
	query = sqlite3_mprintf("insert into disabled_package_info(package, package_type, package_version, install_location, package_size, " \
		"package_removable, package_preload, package_readonly, package_update, package_appsetting, package_nodisplay, package_system," \
		"author_name, author_email, author_href, installed_time, installed_storage, storeclient_id, mainapp_id, package_url, root_path, csc_path) " \
		"values(%Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q)",\
		 mfx->package,
		 type,
		 mfx->version,
		 __get_str(mfx->installlocation),
		 __get_str(mfx->package_size),
		 mfx->removable,
		 mfx->preload,
		 mfx->readonly,
		 mfx->update,
		 mfx->appsetting,
		 mfx->nodisplay_setting,
		 mfx->system,
		 __get_str(auth_name),
		 __get_str(auth_email),
		 __get_str(auth_href),
		 mfx->installed_time,
		 mfx->installed_storage,
		 __get_str(mfx->storeclient_id),
		 mfx->mainapp_id,
		 __get_str(mfx->package_url),
		 path,
		 __get_str(mfx->csc_path));

	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1) {
		_LOGD("Package Info DB Insert Failed\n");
		if (type) {
			free(type);
			type = NULL;
		}
		if (path) {
			free(path);
			path = NULL;
		}
		return -1;
	}

	if (type) {
		free(type);
		type = NULL;
	}
	if (path) {
		free(path);
		path = NULL;
	}

	ret = __insert_disabled_ui_mainapp_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert the package locale*/
	pkglocale = __create_locale_list(pkglocale, lbl, lcn, icn, dcn, ath);
	/*remove duplicated data in pkglocale*/
	__trimfunc(pkglocale);

	/*Insert the app locale info */
	while(up != NULL)
	{
		applocale = __create_locale_list(applocale, up->label, NULL, up->icon, NULL, NULL);
		up = up->next;
	}

	/*remove duplicated data in applocale*/
	__trimfunc(applocale);

	/*g_list_foreach(pkglocale, __printfunc, NULL);*/
	/*_LOGD("\n");*/
	/*g_list_foreach(applocale, __printfunc, NULL);*/

	g_list_foreach(pkglocale, __insert_disabled_pkglocale_info, (gpointer)mfx);

	/*native app locale info*/
	up = mfx->uiapplication;
	while(up != NULL)
	{
		g_list_foreach(applocale, __insert_disabled_uiapplication_locale_info, (gpointer)up);
		up = up->next;
	}

	g_list_free(pkglocale);
	pkglocale = NULL;
	g_list_free(applocale);
	applocale = NULL;

	/*Insert in the package_app_info DB*/
	ret = __insert_disabled_uiapplication_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert in the package_app_app_category DB*/
	ret = __insert_disabled_uiapplication_appcategory_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert in the package_app_app_metadata DB*/
	ret = __insert_disabled_uiapplication_appmetadata_info(mfx);
	if (ret == -1)
		return -1;

	return 0;
}

static int __delete_disabled_pkg_info_from_pkgid(const char *pkgid)
{
	char *query = NULL;
	int ret = -1;

	/*Delete from Package Info DB*/
	query = sqlite3_mprintf("delete from disabled_package_info where package=%Q", pkgid);
	ret = __exec_query(query);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "Package Info DB Delete Failed\n");

	/*Delete from Package Localized Info*/
	sqlite3_free(query);
	query = sqlite3_mprintf("delete from disabled_package_localized_info where package=%Q", pkgid);
	ret = __exec_query(query);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "Package Localized Info DB Delete Failed\n");

	/*Delete from app Info*/
	sqlite3_free(query);
	query = sqlite3_mprintf("delete from disabled_package_app_info where package=%Q", pkgid);
	ret = __exec_query(query);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "app Info DB Delete Failed\n");

	/*Delete from app Localized Info*/
	sqlite3_free(query);
	query = sqlite3_mprintf("delete from disabled_package_app_localized_info where package=%Q", pkgid);
	ret = __exec_query(query);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "app Localized Info DB Delete Failed\n");

	/*Delete from app category Info*/
	sqlite3_free(query);
	query = sqlite3_mprintf("delete from disabled_package_app_app_category where package=%Q", pkgid);
	ret = __exec_query(query);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "app category Info DB Delete Failed\n");

	/*Delete from app metadata Info*/
	sqlite3_free(query);
	query = sqlite3_mprintf("delete from disabled_package_app_app_metadata where package=%Q", pkgid);
	ret = __exec_query(query);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "app metadata Info DB Delete Failed\n");

catch:
	sqlite3_free(query);
	return 0;
}

API int pkgmgr_parser_insert_disabled_pkg_info_in_db(manifest_x *mfx)
{
	if (mfx == NULL) {
		_LOGD("manifest pointer is NULL\n");
		return -1;
	}
	int ret = 0;
	ret = pkgmgr_parser_check_and_create_db();
	if (ret == -1) {
		_LOGD("Failed to open DB\n");
		return ret;
	}
	ret = pkgmgr_parser_initialize_db();
	if (ret == -1)
		goto err;
	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to begin transaction[%d]\n", ret);
		ret = -1;
		goto err;
	}
	_LOGD("Transaction Begin\n");
	ret = __insert_disabled_pkg_info_in_db(mfx);
	if (ret == -1) {
		_LOGE("Insert into DB failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("ROLLBACK is fail after insert_disabled_pkg_info_in_db\n");

		ret = -1;
		goto err;
	}
	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		_LOGE("Failed to commit transaction. Rollback now\n");

		ret = -1;
		goto err;
	}
	_LOGD("Transaction Commit and End\n");
err:
	pkgmgr_parser_close_db();
	return ret;
}

API int pkgmgr_parser_delete_disabled_pkgid_info_from_db(const char *pkgid)
{
	retvm_if(pkgid == NULL, PMINFO_R_ERROR, "argument supplied is NULL");

	int ret = 0;

	/*open db*/
	ret = pkgmgr_parser_check_and_create_db();
	retvm_if(ret < 0, PMINFO_R_ERROR, "Failed to open DB\n");

	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to begin transaction\n");
		ret = -1;
		goto err;
	}

	_LOGD("Start to Delete pkgid[%s] info from db\n", pkgid);

	/*delete pkg info*/
	ret = __delete_disabled_pkg_info_from_pkgid(pkgid);
	if (ret == -1) {
		_LOGD("Delete from DB failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to begin transaction\n");
		goto err;
	}

	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to commit transaction, Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to commit transaction, Rollback now\n");

		ret = -1;
		goto err;
	}

err:
	pkgmgr_parser_close_db();
	return ret;
}

API int pkgmgr_parser_insert_disabled_pkg(const char *pkgid, char *const tagv[])
{
	retvm_if(pkgid == NULL, PM_PARSER_R_ERROR, "argument supplied is NULL");

	char *manifest = NULL;
	manifest_x *mfx = NULL;
	int ret = -1;

	_LOGD("parsing manifest for installation: %s\n", pkgid);

	manifest = pkgmgr_parser_get_manifest_file(pkgid);
	if (manifest == NULL) {
		_LOGE("can not get the manifest.xml\n");
		return -1;
	}

	xmlInitParser();
	mfx = pkgmgr_parser_process_manifest_xml(manifest);
	retvm_if(mfx == NULL, PM_PARSER_R_ERROR, "argument supplied is NULL");

	_LOGD("Parsing Finished\n");

	__add_preload_info(mfx, manifest);

	_LOGD("Added preload infomation\n");

	FREE_AND_NULL(manifest);

	ret = pkgmgr_parser_insert_disabled_pkg_info_in_db(mfx);
	retvm_if(ret == PM_PARSER_R_ERROR, PM_PARSER_R_ERROR, "DB Insert failed");

	_LOGD("DB Insert Success\n");

	pkgmgr_parser_free_manifest_xml(mfx);
	_LOGD("Free Done\n");
	xmlCleanupParser();

	return PM_PARSER_R_OK;
}

API int pkgmgr_parser_delete_disabled_pkg(const char *pkgid, char *const tagv[])
{
	retvm_if(pkgid == NULL, PM_PARSER_R_ERROR, "argument supplied is NULL");

	int ret = -1;

	_LOGD("Start uninstall for pkgid : delete pkgid[%s]\n", pkgid);

	/* delete pkgmgr db */
	ret = pkgmgr_parser_delete_disabled_pkgid_info_from_db(pkgid);
	if (ret == -1)
		_LOGD("DB pkgid info Delete failed\n");
	else
		_LOGD("DB pkgid info Delete Success\n");

	_LOGD("Finish : uninstall for pkgid\n");

	return PM_PARSER_R_OK;
}
