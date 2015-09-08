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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <db-util.h>
#include <glib.h>
#include <dlfcn.h>

#include "pkgmgr_parser_internal.h"
#include "pkgmgr_parser_db.h"

#include "pkgmgrinfo_debug.h"
#include "pkgmgrinfo_type.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_PARSER"

#define MAX_QUERY_LEN		4096
#define MAX_PACKAGE_STR_SIZE 512
#define MAX_ALIAS_INI_ENTRY	(2 * MAX_PACKAGE_STR_SIZE) + 1
#define USR_APPSVC_ALIAS_INI "/usr/share/appsvc/alias.ini"
#define OPT_APPSVC_ALIAS_INI "/opt/usr/share/appsvc/alias.ini"
#define MANIFEST_DB "/opt/dbspace/.pkgmgr_parser.db"
typedef int (*sqlite_query_callback)(void *data, int ncols, char **coltxt, char **colname);

sqlite3 *pkgmgr_parser_db;
sqlite3 *pkgmgr_cert_db;


#define QUERY_CREATE_TABLE_PACKAGE_INFO "create table if not exists package_info " \
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
						"csc_path text," \
						"package_support_disable text DEFAULT 'false', " \
						"package_mother_package text DEFAULT 'false', " \
						"package_disable text DEFAULT 'false', " \
						"package_support_mode text, " \
						"package_hash text, " \
						"package_reserve1 text," \
						"package_reserve2 text," \
						"package_reserve3 text," \
						"package_reserve4 text," \
						"package_reserve5 text)"

#define QUERY_CREATE_TABLE_PACKAGE_LOCALIZED_INFO "create table if not exists package_localized_info " \
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

#define QUERY_CREATE_TABLE_PACKAGE_PRIVILEGE_INFO "create table if not exists package_privilege_info " \
						"(package text not null, " \
						"privilege text not null, " \
						"PRIMARY KEY(package, privilege) " \
						"FOREIGN KEY(package) " \
						"REFERENCES package_info(package) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_INFO "create table if not exists package_app_info " \
						"(app_id text primary key not null, " \
						"app_component text, " \
						"app_exec text, " \
						"app_ambient_support text DEFAULT 'false', " \
						"app_nodisplay text DEFAULT 'false', " \
						"app_type text, " \
						"app_onboot text DEFAULT 'false', " \
						"app_multiple text DEFAULT 'false', " \
						"app_autorestart text DEFAULT 'false', " \
						"app_taskmanage text DEFAULT 'false', " \
						"app_enabled text DEFAULT 'true', " \
						"app_hwacceleration text DEFAULT 'default', " \
						"app_screenreader text DEFAULT 'use-system-setting', " \
						"app_mainapp text, " \
						"app_recentimage text, " \
						"app_launchcondition text, " \
						"app_indicatordisplay text DEFAULT 'true', " \
						"app_portraitimg text, " \
						"app_landscapeimg text, " \
						"app_effectimage_type text, " \
						"app_guestmodevisibility text DEFAULT 'true', " \
						"app_permissiontype text DEFAULT 'normal', " \
						"app_preload text DEFAULT 'false', " \
						"app_submode text DEFAULT 'false', " \
						"app_submode_mainid text, " \
						"app_installed_storage text, " \
						"app_process_pool text DEFAULT 'false', " \
						"app_multi_instance text DEFAULT 'false', " \
						"app_multi_instance_mainid text, " \
						"app_multi_window text DEFAULT 'false', " \
						"app_support_disable text DEFAULT 'false', " \
						"app_disable text DEFAULT 'false', " \
						"app_ui_gadget text DEFAULT 'false', " \
						"app_removable text DEFAULT 'false', " \
						"app_support_mode text, " \
						"app_support_feature text, " \
						"component_type text, " \
						"package text not null, " \
						"app_package_type text DEFAULT 'rpm', " \
						"app_package_system text, " \
						"app_package_installed_time text, " \
						"app_reserve1 text, " \
						"app_reserve2 text, " \
						"app_reserve3 text, " \
						"app_reserve4 text, " \
						"app_reserve5 text, " \
						"FOREIGN KEY(package) " \
						"REFERENCES package_info(package) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_LOCALIZED_INFO "create table if not exists package_app_localized_info " \
						"(app_id text not null, " \
						"app_locale text DEFAULT 'No Locale', " \
						"app_label text, " \
						"app_icon text, " \
						"PRIMARY KEY(app_id,app_locale) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_ICON_SECTION_INFO "create table if not exists package_app_icon_section_info " \
						"(app_id text not null, " \
						"app_icon text, " \
						"app_icon_section text, " \
						"app_icon_resolution text, " \
						"PRIMARY KEY(app_id,app_icon_section,app_icon_resolution) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_IMAGE_INFO "create table if not exists package_app_image_info " \
						"(app_id text not null, " \
						"app_locale text DEFAULT 'No Locale', " \
						"app_image_section text, " \
						"app_image text, " \
						"PRIMARY KEY(app_id,app_image_section) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_APP_CONTROL "create table if not exists package_app_app_control " \
						"(app_id text not null, " \
						"app_control text not null, " \
						"PRIMARY KEY(app_id,app_control) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_APP_SVC "create table if not exists package_app_app_svc " \
						"(app_id text not null, " \
						"operation text not null, " \
						"uri_scheme text, " \
						"mime_type text, " \
						"subapp_name text, " \
						"PRIMARY KEY(app_id,operation,uri_scheme,mime_type,subapp_name) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_APP_CATEGORY "create table if not exists package_app_app_category " \
						"(app_id text not null, " \
						"category text not null, " \
						"PRIMARY KEY(app_id,category) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_APP_METADATA "create table if not exists package_app_app_metadata " \
						"(app_id text not null, " \
						"md_key text not null, " \
						"md_value text not null, " \
						"PRIMARY KEY(app_id, md_key, md_value) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_APP_PERMISSION "create table if not exists package_app_app_permission " \
						"(app_id text not null, " \
						"pm_type text not null, " \
						"pm_value text not null, " \
						"PRIMARY KEY(app_id, pm_type, pm_value) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_SHARE_ALLOWED "create table if not exists package_app_share_allowed " \
						"(app_id text not null, " \
						"data_share_path text not null, " \
						"data_share_allowed text not null, " \
						"PRIMARY KEY(app_id,data_share_path,data_share_allowed) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_SHARE_REQUEST "create table if not exists package_app_share_request " \
						"(app_id text not null, " \
						"data_share_request text not null, " \
						"PRIMARY KEY(app_id,data_share_request) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_CERT_INDEX_INFO "create table if not exists package_cert_index_info " \
						"(cert_info text not null, " \
						"cert_id integer, " \
						"cert_ref_count integer, " \
						"PRIMARY KEY(cert_id)) "

#define QUERY_CREATE_TABLE_PACKAGE_CERT_INFO "create table if not exists package_cert_info " \
						"(package text not null, " \
						"author_root_cert integer, " \
						"author_im_cert integer, " \
						"author_signer_cert integer, " \
						"dist_root_cert integer, " \
						"dist_im_cert integer, " \
						"dist_signer_cert integer, " \
						"dist2_root_cert integer, " \
						"dist2_im_cert integer, " \
						"dist2_signer_cert integer, " \
						"PRIMARY KEY(package)) "

#define QUERY_CREATE_TABLE_PACKAGE_PKG_RESERVE "create table if not exists package_pkg_reserve " \
						"(pkgid text not null, " \
						"reserve1 text, " \
						"reserve2 text, " \
						"reserve3 text, " \
						"reserve4 text, " \
						"reserve5 text, " \
						"PRIMARY KEY(pkgid)) "

#define QUERY_CREATE_TABLE_PACKAGE_APP_RESERVE "create table if not exists package_app_reserve " \
						"(app_id text not null, " \
						"reserve1 text, " \
						"reserve2 text, " \
						"reserve3 text, " \
						"reserve4 text, " \
						"reserve5 text, " \
						"PRIMARY KEY(app_id)) "

#define QUERY_CREATE_TABLE_PACKAGE_PLUGIN_INFO "create table if not exists package_plugin_info " \
						"(pkgid text primary key not null, " \
						"enabled_plugin INTEGER DEFAULT 0)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_DATA_CONTROL "create table if not exists package_app_data_control " \
						"(app_id text not null, " \
						"providerid text not null, " \
						"access text not null, " \
						"type text not null, " \
						"PRIMARY KEY(app_id, providerid, access, type) " \
						"FOREIGN KEY(app_id) " \
						"REFERENCES package_app_info(app_id) " \
						"ON DELETE CASCADE)"

#define QUERY_CREATE_TABLE_PACKAGE_APP_ALIASID "create table if not exists package_app_aliasid" \
						"(app_id text primary key not null, "\
						"alias_id text not null)"

static int __insert_uiapplication_info(manifest_x *mfx);
static int __insert_uiapplication_appsvc_info(manifest_x *mfx);
static int __insert_uiapplication_appcategory_info(manifest_x *mfx);
static int __insert_uiapplication_appcontrol_info(manifest_x *mfx);
static int __insert_uiapplication_appmetadata_info(manifest_x *mfx);
static int __insert_uiapplication_share_allowed_info(manifest_x *mfx);
static int __insert_uiapplication_share_request_info(manifest_x *mfx);
static void __insert_uiapplication_locale_info(gpointer data, gpointer userdata);
static int __insert_uiapplication_datacontrol_info(manifest_x *mfx);
static void __insert_pkglocale_info(gpointer data, gpointer userdata);
static int __delete_manifest_info_from_pkgid(const char *pkgid);
static int __delete_manifest_info_from_appid(const char *appid);
static int __insert_manifest_info_in_db(manifest_x *mfx);
static int __delete_manifest_info_from_db(manifest_x *mfx);
static int __delete_appinfo_from_db(char *db_table, const char *appid);
static gint __comparefunc(gconstpointer a, gconstpointer b, gpointer userdata);
static int __pkgmgr_parser_create_db(sqlite3 **db_handle, const char *db_path);

static void __str_trim(char *input)
{
	char *trim_str = input;

	if (input == NULL)
		return;

	while (*input != 0) {
		if (!isspace(*input)) {
			*trim_str = *input;
			trim_str++;
		}
		input++;
	}

	*trim_str = 0;
	return;
}

const char *__get_str(const char *str)
{
	if (str == NULL)
	{
		return PKGMGR_PARSER_EMPTY_STR;
	}

	return str;
}

static int __pkgmgr_parser_create_db(sqlite3 **db_handle, const char *db_path)
{
	int ret = -1;
	sqlite3 *handle;
	if (access(db_path, F_OK) == 0) {
		ret =
		    db_util_open(db_path, &handle,
				 DB_UTIL_REGISTER_HOOK_METHOD);
		if (ret != SQLITE_OK) {
			_LOGD("connect db [%s] failed!\n",
			       db_path);
			return -1;
		}
		*db_handle = handle;
		return 0;
	}
	_LOGD("%s DB does not exists. Create one!!\n", db_path);

	ret =
	    db_util_open(db_path, &handle,
			 DB_UTIL_REGISTER_HOOK_METHOD);

	if (ret != SQLITE_OK) {
		_LOGD("connect db [%s] failed!\n", db_path);
		return -1;
	}
	*db_handle = handle;
	return 0;
}

int __evaluate_query(sqlite3 *db_handle, char *query)
{
	sqlite3_stmt* p_statement;
	int result;
    result = sqlite3_prepare_v2(db_handle, query, strlen(query), &p_statement, NULL);
    if (result != SQLITE_OK) {
        _LOGE("Sqlite3 error [%d] : <%s> preparing <%s> querry\n", result, sqlite3_errmsg(db_handle), query);
		return -1;
    }

	result = sqlite3_step(p_statement);
	if (result != SQLITE_DONE) {
		_LOGE("Sqlite3 error [%d] : <%s> executing <%s> statement\n", result, sqlite3_errmsg(db_handle), query);
		return -1;
	}

	result = sqlite3_finalize(p_statement);
	if (result != SQLITE_OK) {
		_LOGE("Sqlite3 error [%d] : <%s> finalizing <%s> statement\n", result, sqlite3_errmsg(db_handle), query);
		return -1;
	}
	return 0;
}

int __initialize_db(sqlite3 *db_handle, char *db_query)
{
	return (__evaluate_query(db_handle, db_query));
}

int __exec_query(char *query)
{
	return (__evaluate_query(pkgmgr_parser_db, query));
}

int __exec_query_no_msg(char *query)
{
	char *error_message = NULL;
	if (SQLITE_OK !=
	    sqlite3_exec(pkgmgr_parser_db, query, NULL, NULL, &error_message)) {
		sqlite3_free(error_message);
		return -1;
	}
	sqlite3_free(error_message);
	return 0;
}

static int __exec_db_query(sqlite3 *db, char *query, sqlite_query_callback callback, void *data)
{
	char *error_message = NULL;
	if (SQLITE_OK !=
	    sqlite3_exec(db, query, callback, data, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", query,
		       error_message);
		sqlite3_free(error_message);
		return -1;
	}
	sqlite3_free(error_message);
	return 0;
}

GList *__create_locale_list(GList *locale, label_x *lbl, license_x *lcn, icon_x *icn, description_x *dcn, author_x *ath)
{

	while(lbl != NULL)
	{
		if (lbl->lang)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)lbl->lang, __comparefunc, NULL);
		lbl = lbl->next;
	}
	while(lcn != NULL)
	{
		if (lcn->lang)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)lcn->lang, __comparefunc, NULL);
		lcn = lcn->next;
	}
	while(icn != NULL)
	{
		if (icn->lang)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)icn->lang, __comparefunc, NULL);
		icn = icn->next;
	}
	while(dcn != NULL)
	{
		if (dcn->lang)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)dcn->lang, __comparefunc, NULL);
		dcn = dcn->next;
	}
	while(ath != NULL)
	{
		if (ath->lang)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)ath->lang, __comparefunc, NULL);
		ath = ath->next;
	}
	return locale;

}

static GList *__create_icon_list(GList *locale, icon_x *icn)
{
	while(icn != NULL)
	{
		if (icn->section)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)icn->section, __comparefunc, NULL);
		icn = icn->next;
	}
	return locale;
}

static GList *__create_image_list(GList *locale, image_x *image)
{
	while(image != NULL)
	{
		if (image->section)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)image->section, __comparefunc, NULL);
		image = image->next;
	}
	return locale;
}

void __trimfunc(GList* trim_list)
{
	char *trim_data = NULL;
	char *prev = NULL;

	GList *list = NULL;
	list = g_list_first(trim_list);

	while (list) {
		trim_data = (char *)list->data;
		if (trim_data) {
			if (prev) {
				if (strcmp(trim_data, prev) == 0) {
					trim_list = g_list_remove(trim_list, trim_data);
					list = g_list_first(trim_list);
					prev = NULL;
					continue;
				} else
					prev = trim_data;
			}
			else
				prev = trim_data;
		}
		list = g_list_next(list);
	}
}

static gint __comparefunc(gconstpointer a, gconstpointer b, gpointer userdata)
{
	if (a == NULL || b == NULL)
		return 0;
	if (strcmp((char*)a, (char*)b) == 0)
		return 0;
	if (strcmp((char*)a, (char*)b) < 0)
		return -1;
	if (strcmp((char*)a, (char*)b) > 0)
		return 1;
	return 0;
}

void __extract_data(gpointer data, label_x *lbl, license_x *lcn, icon_x *icn, description_x *dcn, author_x *ath,
		char **label, char **license, char **icon, char **description, char **author)
{
	while(lbl != NULL)
	{
		if (lbl->lang) {
			if (strcmp(lbl->lang, (char *)data) == 0) {
				*label = (char*)lbl->text;
				break;
			}
		}
		lbl = lbl->next;
	}
	while(lcn != NULL)
	{
		if (lcn->lang) {
			if (strcmp(lcn->lang, (char *)data) == 0) {
				*license = (char*)lcn->text;
				break;
			}
		}
		lcn = lcn->next;
	}
	while(icn != NULL)
	{
		if (icn->section == NULL) {
			if (icn->lang) {
				if (strcmp(icn->lang, (char *)data) == 0) {
					*icon = (char*)icn->text;
					break;
				}
			}
		}
		icn = icn->next;
	}
	while(dcn != NULL)
	{
		if (dcn->lang) {
			if (strcmp(dcn->lang, (char *)data) == 0) {
				*description = (char*)dcn->text;
				break;
			}
		}
		dcn = dcn->next;
	}
	while(ath != NULL)
	{
		if (ath->lang) {
			if (strcmp(ath->lang, (char *)data) == 0) {
				*author = (char*)ath->text;
				break;
			}
		}
		ath = ath->next;
	}

}

static void __extract_icon_data(gpointer data, icon_x *icn, char **icon, char **resolution)
{
	while(icn != NULL)
	{
		if (icn->section) {
			if (strcmp(icn->section, (char *)data) == 0) {
				*icon = (char*)icn->text;
				*resolution = (char*)icn->resolution;
				break;
			}
		}
		icn = icn->next;
	}
}

static void __extract_image_data(gpointer data, image_x*image, char **lang, char **img)
{
	while(image != NULL)
	{
		if (image->section) {
			if (strcmp(image->section, (char *)data) == 0) {
				*lang = (char*)image->lang;
				*img = (char*)image->text;
				break;
			}
		}
		image = image->next;
	}
}

static void __insert_pkglocale_info(gpointer data, gpointer userdata)
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

	query = sqlite3_mprintf("insert into package_localized_info(package, package_locale, " \
		"package_label, package_icon, package_description, package_license, package_author) values " \
		"(%Q, %Q, %Q, %Q, %Q, %Q, %Q)",
		mfx->package,
		(char*)data,
		__get_str(label),
		__get_str(icon),
		__get_str(description),
		__get_str(license),
		__get_str(author));

	ret = __exec_query(query);
	if (ret == -1)
		_LOGD("Package Localized Info DB Insert failed\n");

	sqlite3_free(query);
}

static int __insert_uiapplication_datacontrol_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	datacontrol_x *dc = NULL;
	int ret = -1;
	char *query = NULL;

	while (up != NULL)
	{
		dc = up->datacontrol;
		while (dc != NULL) {
			query = sqlite3_mprintf("insert into package_app_data_control(app_id, providerid, access, type) " \
				"values(%Q, %Q, %Q, %Q)", \
				up->appid,
				dc->providerid,
				dc->access,
				dc->type);

			ret = __exec_query(query);
			sqlite3_free(query);
			if (ret == -1) {
				_LOGD("Package UiApp Data Control DB Insert Failed\n");
				return -1;
			}
			dc = dc->next;
		}
		up = up->next;
	}
	return 0;
}

static void __insert_uiapplication_locale_info(gpointer data, gpointer userdata)
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

	query = sqlite3_mprintf("insert into package_app_localized_info(app_id, app_locale, " \
		"app_label, app_icon) values " \
		"(%Q, %Q, %Q, %Q)", up->appid, (char*)data,
		__get_str(label), __get_str(icon));
	ret = __exec_query(query);
	if (ret == -1)
		_LOGD("Package UiApp Localized Info DB Insert failed\n");

	sqlite3_free(query);

	/*insert ui app locale info to pkg locale to get mainapp data */
	if (strcasecmp(up->mainapp, "true")==0) {
		char *update_query = NULL;
		update_query = sqlite3_mprintf("insert into package_localized_info(package, package_locale, " \
			"package_label, package_icon, package_description, package_license, package_author) values " \
			"(%Q, %Q, %Q, %Q, %Q, %Q, %Q)",
			up->package,
			(char*)data,
			__get_str(label),
			__get_str(icon),
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

static void __insert_uiapplication_icon_section_info(gpointer data, gpointer userdata)
{
	int ret = -1;
	char *icon = NULL;
	char *resolution = NULL;
	char * query = NULL;

	uiapplication_x *up = (uiapplication_x*)userdata;
	icon_x *icn = up->icon;

	__extract_icon_data(data, icn, &icon, &resolution);
	if (!icon && !resolution)
		return;

	query = sqlite3_mprintf("insert into package_app_icon_section_info(app_id, " \
		"app_icon, app_icon_section, app_icon_resolution) values " \
		"(%Q, %Q, %Q, %Q)", up->appid,
		icon, (char*)data, resolution);

	ret = __exec_query(query);
	if (ret == -1)
		_LOGD("Package UiApp Localized Info DB Insert failed\n");

	sqlite3_free(query);
}

static void __insert_uiapplication_image_info(gpointer data, gpointer userdata)
{
	int ret = -1;
	char *lang = NULL;
	char *img = NULL;
	char *query = NULL;

	uiapplication_x *up = (uiapplication_x*)userdata;
	image_x *image = up->image;

	__extract_image_data(data, image, &lang, &img);
	if (!lang && !img)
		return;

	query = sqlite3_mprintf("insert into package_app_image_info(app_id, app_locale, " \
		"app_image_section, app_image) values " \
		"(%Q, %Q, %Q, %Q)", up->appid, lang, (char*)data, img);

	ret = __exec_query(query);
	if (ret == -1)
		_LOGD("Package UiApp image Info DB Insert failed\n");

	sqlite3_free(query);
}

static int __insert_ui_mainapp_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	int ret = -1;
	char *query = NULL;
	while(up != NULL)
	{
		query = sqlite3_mprintf("update package_app_info set app_mainapp=%Q where app_id=%Q", up->mainapp, up->appid);

		ret = __exec_query(query);
		sqlite3_free(query);
		if (ret == -1) {
			_LOGD("Package UiApp Info DB Insert Failed\n");
			return -1;
		}
		if (strcasecmp(up->mainapp, "True") == 0) {
			FREE_AND_NULL(mfx->mainapp_id);
			mfx->mainapp_id = strdup(up->appid);
		}

		up = up->next;
	}

	if (mfx->mainapp_id == NULL){
		if (mfx->uiapplication && mfx->uiapplication->appid) {
			query = sqlite3_mprintf("update package_app_info set app_mainapp='true' where app_id=%Q", mfx->uiapplication->appid);
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

		FREE_AND_NULL(mfx->uiapplication->mainapp);
		mfx->uiapplication->mainapp= strdup("true");
		mfx->mainapp_id = strdup(mfx->uiapplication->appid);
	}

	query = sqlite3_mprintf("update package_info set mainapp_id=%Q where package=%Q", mfx->mainapp_id, mfx->package);
	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1) {
		_LOGD("Package Info DB update Failed\n");
		return -1;
	}

	return 0;
}
/* _PRODUCT_LAUNCHING_ENHANCED_
*  up->indicatordisplay, up->portraitimg, up->landscapeimg, up->guestmode_appstatus
*/
static int __insert_uiapplication_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	int ret = -1;
	char *query = NULL;
	char *type = NULL;

	if (mfx->type)
		type = strdup(mfx->type);
	else
		type = strdup("rpm");

	while(up != NULL)
	{
		query = sqlite3_mprintf("insert into package_app_info(app_id, app_component, app_exec, app_ambient_support, app_nodisplay, app_type, app_onboot, " \
			"app_multiple, app_autorestart, app_taskmanage, app_enabled, app_hwacceleration, app_screenreader, app_mainapp , app_recentimage, " \
			"app_launchcondition, app_indicatordisplay, app_portraitimg, app_landscapeimg, app_effectimage_type, app_guestmodevisibility, app_permissiontype, "\
			"app_preload, app_submode, app_submode_mainid, app_installed_storage, app_process_pool, app_multi_instance, app_multi_instance_mainid, app_multi_window, app_support_disable, "\
			"app_ui_gadget, app_removable, app_support_mode, app_support_feature, component_type, package, "\
			"app_package_type, app_package_system, app_package_installed_time"\
			") " \
			"values(%Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q)",\
			up->appid,
			"uiapp",
			up->exec,
			up->ambient_support,
			up->nodisplay,
			up->type,
			up->onboot,
			up->multiple,
			up->autorestart,
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
			__get_str(up->effectimage_type),
			up->guestmode_visibility,
			up->permission_type,
			mfx->preload,
			up->submode,
			__get_str(up->submode_mainid),
			mfx->installed_storage,
			up->process_pool,
			up->multi_instance,
			__get_str(up->multi_instance_mainid),
			up->multi_window,
			mfx->support_disable,
			up->ui_gadget,
			mfx->removable,
			mfx->support_mode,
			up->support_feature,
			up->component_type,
			mfx->package,
			type,
			mfx->system,
			mfx->installed_time
			);

		ret = __exec_query(query);
		sqlite3_free(query);
		if (ret == -1) {
			_LOGD("Package UiApp Info DB Insert Failed\n");
			FREE_AND_NULL(type);
			return -1;
		}
		up = up->next;
	}
	FREE_AND_NULL(type);
	return 0;
}

static int __insert_uiapplication_appcategory_info(manifest_x *mfx)
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
			query = sqlite3_mprintf("insert into package_app_app_category(app_id, category) " \
				"values(%Q, %Q)",\
				 up->appid, ct->name);
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

static int __insert_uiapplication_appmetadata_info(manifest_x *mfx)
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
				query = sqlite3_mprintf("insert into package_app_app_metadata(app_id, md_key, md_value) " \
					"values(%Q, %Q, %Q)",\
					 up->appid, md->key, __get_str(md->value));
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

static int __insert_uiapplication_apppermission_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	permission_x *pm = NULL;
	int ret = -1;
	char *query = NULL;
	while(up != NULL)
	{
		pm = up->permission;
		while (pm != NULL)
		{
			query = sqlite3_mprintf("insert into package_app_app_permission(app_id, pm_type, pm_value) " \
				"values(%Q, %Q, %Q)",\
				 up->appid, pm->type, pm->value);
			ret = __exec_query(query);
			sqlite3_free(query);
			if (ret == -1) {
				_LOGD("Package UiApp permission Info DB Insert Failed\n");
				return -1;
			}
			pm = pm->next;
		}
		up = up->next;
	}
	return 0;
}

static int __insert_uiapplication_appcontrol_info(manifest_x *mfx)
{
	int ret = 0;
	char *buf = NULL;
	char *buftemp = NULL;
	char *query = NULL;
	uiapplication_x *up = mfx->uiapplication;

	buf = (char *)calloc(1, BUFMAX);
	retvm_if(!buf, PMINFO_R_ERROR, "Malloc Failed\n");

	buftemp = (char *)calloc(1, BUFMAX);
	tryvm_if(!buftemp, ret = PMINFO_R_ERROR, "Malloc Failed\n");

	for(; up; up=up->next) {
		if (up->appsvc) {
			appsvc_x *asvc = NULL;
			operation_x *op = NULL;
			mime_x *mi = NULL;
			uri_x *ui = NULL;
			subapp_x *sub = NULL;
			const char *operation = NULL;
			const char *mime = NULL;
			const char *uri = NULL;
			const char *subapp = NULL;
			int i = 0;
			memset(buf, '\0', BUFMAX);
			memset(buftemp, '\0', BUFMAX);

			asvc = up->appsvc;
			while(asvc != NULL) {
				op = asvc->operation;
				while(op != NULL) {
					if (op)
						operation = op->name;
					mi = asvc->mime;

					do
					{
						if (mi)
							mime = mi->name;
						sub = asvc->subapp;
						do
						{
							if (sub)
								subapp = sub->name;
							ui = asvc->uri;
							do
							{
								if (ui)
									uri = ui->name;

								if(i++ > 0) {
									strncpy(buftemp, buf, BUFMAX);
									snprintf(buf, BUFMAX, "%s;", buftemp);
								}

								strncpy(buftemp, buf, BUFMAX);
								snprintf(buf, BUFMAX, "%s%s|%s|%s|%s", buftemp, operation?operation:"NULL", uri?uri:"NULL", mime?mime:"NULL", subapp?subapp:"NULL");
//								_LOGD("buf[%s]\n", buf);

								if (ui)
									ui = ui->next;
								uri = NULL;
							} while(ui != NULL);
							if (sub)
								sub = sub->next;
							subapp = NULL;
						}while(sub != NULL);
						if (mi)
							mi = mi->next;
						mime = NULL;
					}while(mi != NULL);
					if (op)
						op = op->next;
					operation = NULL;
				}
				asvc = asvc->next;
			}

			query= sqlite3_mprintf("insert into package_app_app_control(app_id, app_control) values(%Q, %Q)", up->appid,  buf);
			ret = __exec_query(query);
			if (ret < 0) {
				_LOGD("Package UiApp app_control DB Insert Failed\n");
			}
			sqlite3_free(query);
		}
	}

catch:
	FREE_AND_NULL(buf);
	FREE_AND_NULL(buftemp);

	return 0;
}

static int __insert_uiapplication_appsvc_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	appsvc_x *asvc = NULL;
	operation_x *op = NULL;
	mime_x *mi = NULL;
	uri_x *ui = NULL;
	subapp_x *sub = NULL;
	int ret = -1;
	char *query = NULL;
	const char *operation = NULL;
	const char *mime = NULL;
	const char *uri = NULL;
	const char *subapp = NULL;
	while(up != NULL)
	{
		asvc = up->appsvc;
		while(asvc != NULL)
		{
			op = asvc->operation;
			while(op != NULL)
			{
				if (op)
					operation = op->name;
				mi = asvc->mime;

				do
				{
					if (mi)
						mime = mi->name;
					sub = asvc->subapp;
					do
					{
						if (sub)
							subapp = sub->name;
						ui = asvc->uri;
						do
						{
							if (ui)
								uri = ui->name;
							query = sqlite3_mprintf("insert into package_app_app_svc(app_id, operation, uri_scheme, mime_type, subapp_name) " \
								"values(%Q, %Q, %Q, %Q, %Q)",\
								 up->appid,
								 operation,
								 __get_str(uri),
								 __get_str(mime),
								 __get_str(subapp));

							ret = __exec_query(query);
							sqlite3_free(query);
							if (ret == -1) {
								_LOGD("Package UiApp AppSvc DB Insert Failed\n");
								return -1;
							}
							if (ui)
								ui = ui->next;
							uri = NULL;
						} while(ui != NULL);
						if (sub)
							sub = sub->next;
						subapp = NULL;
					}while(sub != NULL);
					if (mi)
						mi = mi->next;
					mime = NULL;
				}while(mi != NULL);
				if (op)
					op = op->next;
				operation = NULL;
			}
			asvc = asvc->next;
		}
		up = up->next;
	}
	return 0;
}

static int __insert_uiapplication_share_request_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	datashare_x *ds = NULL;
	request_x *rq = NULL;
	int ret = -1;
	char *query = NULL;
	while(up != NULL)
	{
		ds = up->datashare;
		while(ds != NULL)
		{
			rq = ds->request;
			while(rq != NULL)
			{
				query = sqlite3_mprintf("insert into package_app_share_request(app_id, data_share_request) " \
					"values(%Q, %Q)",\
					 up->appid, rq->text);
				ret = __exec_query(query);
				sqlite3_free(query);
				if (ret == -1) {
					_LOGD("Package UiApp Share Request DB Insert Failed\n");
					return -1;
				}
				rq = rq->next;
			}
			ds = ds->next;
		}
		up = up->next;
	}
	return 0;
}

static int __insert_uiapplication_share_allowed_info(manifest_x *mfx)
{
	uiapplication_x *up = mfx->uiapplication;
	datashare_x *ds = NULL;
	define_x *df = NULL;
	allowed_x *al = NULL;
	int ret = -1;
	char *query = NULL;
	while(up != NULL)
	{
		ds = up->datashare;
		while(ds != NULL)
		{
			df = ds->define;
			while(df != NULL)
			{
				al = df->allowed;
				while(al != NULL)
				{
					query = sqlite3_mprintf("insert into package_app_share_allowed(app_id, data_share_path, data_share_allowed) " \
						"values(%Q, %Q, %Q)",\
						 up->appid, df->path, al->text);
					ret = __exec_query(query);
					sqlite3_free(query);
					if (ret == -1) {
						_LOGD("Package UiApp Share Allowed DB Insert Failed\n");
						return -1;
					}
					al = al->next;
				}
				df = df->next;
			}
			ds = ds->next;
		}
		up = up->next;
	}
	return 0;
}

static int __insert_manifest_info_in_db(manifest_x *mfx)
{
	label_x *lbl = mfx->label;
	license_x *lcn = mfx->license;
	icon_x *icn = mfx->icon;
	description_x *dcn = mfx->description;
	author_x *ath = mfx->author;
	uiapplication_x *up = mfx->uiapplication;
	uiapplication_x *up_icn = mfx->uiapplication;
	uiapplication_x *up_image = mfx->uiapplication;
	privileges_x *pvs = NULL;
	privilege_x *pv = NULL;
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
	GList *appicon = NULL;
	GList *appimage = NULL;

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
	if (mfx->root_path) {
		path = strdup(mfx->root_path);
	} else {
		if (type && strcmp(type,"rpm")==0) {
			snprintf(root, MAX_QUERY_LEN - 1, "/opt/share/packages/%s.xml", mfx->package);
			if (access(root, F_OK) == 0) {
				memset(root, '\0', MAX_QUERY_LEN);
				snprintf(root, MAX_QUERY_LEN - 1, "/opt/usr/apps/%s", mfx->package);
			} else {
				memset(root, '\0', MAX_QUERY_LEN);
				snprintf(root, MAX_QUERY_LEN - 1, "/usr/apps/%s", mfx->package);
			}
		} else {
			snprintf(root, MAX_QUERY_LEN - 1, "/opt/usr/apps/%s", mfx->package);
		}

		path = strdup(root);
	}

	query = sqlite3_mprintf("insert into package_info(package, package_type, package_version, install_location, package_size, " \
		"package_removable, package_preload, package_readonly, package_update, package_appsetting, package_nodisplay, package_system," \
		"author_name, author_email, author_href, installed_time, installed_storage, storeclient_id, mainapp_id, package_url, root_path, csc_path, package_support_disable, " \
		"package_mother_package, package_support_mode, package_reserve1, package_reserve2, package_hash) " \
		"values(%Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q)",\
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
		 __get_str(mfx->csc_path),
		 mfx->support_disable,
		 mfx->mother_package,
		 mfx->support_mode,
		 __get_str(mfx->support_reset),
		 mfx->use_reset,
		 mfx->hash
		 );

	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1) {
		_LOGD("Package Info DB Insert Failed\n");
		FREE_AND_NULL(type);
		FREE_AND_NULL(path);
		return -1;
	}

	FREE_AND_NULL(type);
	FREE_AND_NULL(path);

	/*Insert in the package_privilege_info DB*/
	pvs = mfx->privileges;
	while (pvs != NULL) {
		pv = pvs->privilege;
		while (pv != NULL) {
			query = sqlite3_mprintf("insert into package_privilege_info(package, privilege) " \
				"values(%Q, %Q)",\
				 mfx->package, pv->text);
			ret = __exec_query(query);
			sqlite3_free(query);
			if (ret == -1) {
				_LOGD("Package Privilege Info DB Insert Failed\n");
				return -1;
			}
			pv = pv->next;
		}
		pvs = pvs->next;
	}

	ret = __insert_ui_mainapp_info(mfx);
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

	/*Insert the app icon info */
	while(up_icn != NULL)
	{
		appicon = __create_icon_list(appicon, up_icn->icon);
		up_icn = up_icn->next;
	}
	/*remove duplicated data in appicon*/
	__trimfunc(appicon);

	/*Insert the image info */
	while(up_image != NULL)
	{
		appimage = __create_image_list(appimage, up_image->image);
		up_image = up_image->next;
	}
	/*remove duplicated data in appimage*/
	__trimfunc(appimage);

	/*g_list_foreach(pkglocale, __printfunc, NULL);*/
	/*_LOGD("\n");*/
	/*g_list_foreach(applocale, __printfunc, NULL);*/

	g_list_foreach(pkglocale, __insert_pkglocale_info, (gpointer)mfx);

	/*native app locale info*/
	up = mfx->uiapplication;
	while(up != NULL)
	{
		g_list_foreach(applocale, __insert_uiapplication_locale_info, (gpointer)up);
		up = up->next;
	}

	/*app icon locale info*/
	up_icn = mfx->uiapplication;
	while(up_icn != NULL)
	{
		g_list_foreach(appicon, __insert_uiapplication_icon_section_info, (gpointer)up_icn);
		up_icn = up_icn->next;
	}

	/*app image info*/
	up_image = mfx->uiapplication;
	while(up_image != NULL)
	{
		g_list_foreach(appimage, __insert_uiapplication_image_info, (gpointer)up_image);
		up_image = up_image->next;
	}

	g_list_free(pkglocale);
	pkglocale = NULL;
	g_list_free(applocale);
	applocale = NULL;
	g_list_free(appicon);
	appicon = NULL;
	g_list_free(appimage);
	appimage = NULL;


	/*Insert in the package_app_info DB*/
	ret = __insert_uiapplication_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert in the package_app_app_control DB*/
	ret = __insert_uiapplication_appcontrol_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert in the package_app_app_category DB*/
	ret = __insert_uiapplication_appcategory_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert in the package_app_app_metadata DB*/
	ret = __insert_uiapplication_appmetadata_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert in the package_app_app_permission DB*/
	ret = __insert_uiapplication_apppermission_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert in the package_app_app_svc DB*/
	ret = __insert_uiapplication_appsvc_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert in the package_app_share_allowed DB*/
	ret = __insert_uiapplication_share_allowed_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert in the package_app_share_request DB*/
	ret = __insert_uiapplication_share_request_info(mfx);
	if (ret == -1)
		return -1;

	/*Insert in the package_app_data_control DB*/
	ret = __insert_uiapplication_datacontrol_info(mfx);
	if (ret == -1)
		return -1;

	return 0;

}

static int __update_pkg_info_for_disable(const char *pkgid, bool disable)
{
	char *query = NULL;
	int ret = -1;

	/*Update from package info*/
	query = sqlite3_mprintf("update package_info set package_disable=%Q where package=%Q", disable?"true":"false", pkgid);
	ret = __exec_query(query);
	sqlite3_free(query);
	retvm_if(ret < 0, PMINFO_R_ERROR, "__exec_query() failed.\n");

	/*Update from app info*/
	query = sqlite3_mprintf("update package_app_info set app_disable=%Q where package=%Q", disable?"true":"false", pkgid);
	ret = __exec_query(query);
	sqlite3_free(query);
	retvm_if(ret < 0, PMINFO_R_ERROR, "__exec_query() failed.\n");

	return PMINFO_R_OK;
}

static int __delete_appinfo_from_db(char *db_table, const char *appid)
{
	int ret = 0;
	char *query = sqlite3_mprintf("delete from %Q where app_id=%Q", db_table, appid);
	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1) {
		_LOGD("DB Deletion from table (%s) Failed\n", db_table);
		ret = -1;
	}

	return ret;
}

static int __delete_manifest_info_from_db(manifest_x *mfx)
{
	int ret = -1;
	uiapplication_x *up = mfx->uiapplication;

	/* Delete package info DB */
	ret = __delete_manifest_info_from_pkgid(mfx->package);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Package Info DB Delete Failed\n");

	while (up != NULL) {
		ret = __delete_manifest_info_from_appid(up->appid);
		retvm_if(ret < 0, PMINFO_R_ERROR, "App Info DB Delete Failed\n");

		up = up->next;
	}

	return 0;
}

static int __delete_appsvc_db(const char *appid)
{
	int ret = -1;
	void *lib_handle = NULL;
	int (*appsvc_operation) (const char *);

	if ((lib_handle = dlopen(LIBAPPSVC_PATH, RTLD_LAZY)) == NULL) {
		_LOGE("dlopen is failed LIBAIL_PATH[%s]\n", LIBAPPSVC_PATH);
		return ret;
	}
	if ((appsvc_operation =
		 dlsym(lib_handle, "appsvc_unset_defapp")) == NULL || dlerror() != NULL) {
		_LOGE("can not find symbol \n");
		goto END;
	}

	ret = appsvc_operation(appid);

END:
	if (lib_handle)
		dlclose(lib_handle);

	return ret;
}

static int __delete_appsvc_info_from_db(manifest_x *mfx)
{
	int ret = 0;
	uiapplication_x *uiapplication = mfx->uiapplication;

	for(; uiapplication; uiapplication=uiapplication->next) {
		ret = __delete_appsvc_db(uiapplication->appid);
		if (ret <0)
			_LOGE("can not remove_appsvc_db\n");
	}

	return ret;
}

static int __delete_manifest_info_from_pkgid(const char *pkgid)
{
	char *query = NULL;
	int ret = -1;

	/*Delete from cert table*/
	ret = pkgmgrinfo_delete_certinfo(pkgid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Cert Info  DB Delete Failed\n");

	/*Delete from Package Info DB*/
	query = sqlite3_mprintf("delete from package_info where package=%Q", pkgid);
	ret = __exec_query(query);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "Package Info DB Delete Failed\n");

	/*Delete from Package Localized Info*/
	sqlite3_free(query);
	query = sqlite3_mprintf("delete from package_localized_info where package=%Q", pkgid);
	ret = __exec_query(query);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "Package Localized Info DB Delete Failed\n");

	/*Delete from Package Privilege Info*/
	sqlite3_free(query);
	query = sqlite3_mprintf("delete from package_privilege_info where package=%Q", pkgid);
	ret = __exec_query(query);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "Package Privilege Info DB Delete Failed\n");

catch:
	sqlite3_free(query);
	return 0;
}


static  int __delete_manifest_info_from_appid(const char *appid)
{
	int ret = -1;

	ret = __delete_appinfo_from_db("package_app_info", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_localized_info", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_icon_section_info", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_image_info", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_app_svc", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_app_control", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_app_category", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_app_metadata", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_app_permission", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_share_allowed", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_share_request", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	ret = __delete_appinfo_from_db("package_app_data_control", appid);
	retvm_if(ret < 0, PMINFO_R_ERROR, "Fail to get handle");

	return 0;
}


static int __update_preload_condition_in_db()
{
	int ret = -1;
	char *query = NULL;

	query = sqlite3_mprintf("update package_info set package_preload='true'");

	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1)
		_LOGD("Package preload_condition update failed\n");

	return ret;
}

int pkgmgr_parser_initialize_db()
{
	int ret = -1;
	/*Manifest DB*/
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_INFO);
	if (ret == -1) {
		_LOGD("package info DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_LOCALIZED_INFO);
	if (ret == -1) {
		_LOGD("package localized info DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_PRIVILEGE_INFO);
	if (ret == -1) {
		_LOGD("package app app privilege DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_INFO);
	if (ret == -1) {
		_LOGD("package app info DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_LOCALIZED_INFO);
	if (ret == -1) {
		_LOGD("package app localized info DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_ICON_SECTION_INFO);
	if (ret == -1) {
		_LOGD("package app icon localized info DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_IMAGE_INFO);
	if (ret == -1) {
		_LOGD("package app image info DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_APP_CONTROL);
	if (ret == -1) {
		_LOGD("package app app control DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_APP_CATEGORY);
	if (ret == -1) {
		_LOGD("package app app category DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_APP_METADATA);
	if (ret == -1) {
		_LOGD("package app app category DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_APP_PERMISSION);
	if (ret == -1) {
		_LOGD("package app app permission DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_APP_SVC);
	if (ret == -1) {
		_LOGD("package app app svc DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_SHARE_ALLOWED);
	if (ret == -1) {
		_LOGD("package app share allowed DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_SHARE_REQUEST);
	if (ret == -1) {
		_LOGD("package app share request DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_DATA_CONTROL);
	if (ret == -1) {
		_LOGD("package app data control DB initialization failed\n");
		return ret;
	}
	/*Cert DB*/
	ret = __initialize_db(pkgmgr_cert_db, QUERY_CREATE_TABLE_PACKAGE_CERT_INFO);
	if (ret == -1) {
		_LOGD("package cert info DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_cert_db, QUERY_CREATE_TABLE_PACKAGE_CERT_INDEX_INFO);
	if (ret == -1) {
		_LOGD("package cert index info DB initialization failed\n");
		return ret;
	}
	/*reserve DB*/
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_PKG_RESERVE);
	if (ret == -1) {
		_LOGD("package pkg reserve info DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_RESERVE);
	if (ret == -1) {
		_LOGD("package pkg reserve info DB initialization failed\n");
		return ret;
	}

	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_PLUGIN_INFO);
	if (ret == -1) {
		_LOGD("package pkg reserve info DB initialization failed\n");
		return ret;
	}
	ret = __initialize_db(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_APP_ALIASID);
	if (ret == -1) {
		_LOGD("package app aliasId DB initialization failed\n");
		return ret;
	}
#ifdef _APPFW_FEATURE_PROFILE_WEARABLE
	ret = __init_tables_for_wearable();
	if (ret == -1) {
		_LOGD("package pkg reserve info DB initialization failed\n");
		return ret;
	}
#endif

	return 0;
}

int pkgmgr_parser_check_and_create_db()
{
	int ret = -1;
	/*Manifest DB*/
	ret = __pkgmgr_parser_create_db(&pkgmgr_parser_db, PKGMGR_PARSER_DB_FILE);
	if (ret) {
		_LOGD("Manifest DB creation Failed\n");
		return -1;
	}
	/*Cert DB*/
	ret = __pkgmgr_parser_create_db(&pkgmgr_cert_db, PKGMGR_CERT_DB_FILE);
	if (ret) {
		_LOGD("Cert DB creation Failed\n");
		return -1;
	}
	return 0;
}

void pkgmgr_parser_close_db()
{
	sqlite3_close(pkgmgr_parser_db);
	sqlite3_close(pkgmgr_cert_db);
}

API int pkgmgr_parser_insert_manifest_info_in_db(manifest_x *mfx)
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
	ret = __insert_manifest_info_in_db(mfx);
	if (ret == -1) {
		_LOGD("Insert into DB failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGD("ROLLBACK is fail after insert_disabled_pkg_info_in_db\n");

		ret = -1;
		goto err;
	}
	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to commit transaction. Rollback now\n");
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

API int pkgmgr_parser_update_manifest_info_in_db(manifest_x *mfx)
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
		_LOGD("Failed to begin transaction\n");
		ret = -1;
		goto err;
	}
	_LOGD("Transaction Begin\n");
	ret = __delete_manifest_info_from_db(mfx);
	if (ret == -1) {
		_LOGD("Delete from DB failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGD("Failed to commit transaction. Rollback now\n");
		goto err;
	}
	ret = __insert_manifest_info_in_db(mfx);
	if (ret == -1) {
		_LOGD("Insert into DB failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGD("Failed to commit transaction. Rollback now\n");
		goto err;
	}

	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGD("Failed to commit transaction. Rollback now\n");

		ret = -1;
		goto err;
	}
	_LOGD("Transaction Commit and End\n");
err:
	pkgmgr_parser_close_db();
	return ret;
}

API int pkgmgr_parser_delete_manifest_info_from_db(manifest_x *mfx)
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
	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to begin transaction\n");
		ret = -1;
		goto err;
	}
	_LOGD("Transaction Begin\n");
	ret = __delete_manifest_info_from_db(mfx);
	if (ret == -1) {
		_LOGD("Delete from DB failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to commit transaction, Rollback now\n");

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

	ret = __delete_appsvc_info_from_db(mfx);
	if (ret == -1)
		_LOGD("Removing appsvc_db failed\n");
	else
		_LOGD("Removing appsvc_db Success\n");

	_LOGD("Transaction Commit and End\n");
err:
	pkgmgr_parser_close_db();
	return ret;
}

API int pkgmgr_parser_update_preload_info_in_db()
{
	int ret = 0;
	ret = pkgmgr_parser_check_and_create_db();
	if (ret == -1) {
		_LOGD("Failed to open DB\n");
		return ret;
	}
	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to begin transaction\n");
		ret = -1;
		goto err;
	}
	_LOGD("Transaction Begin\n");
	ret = __update_preload_condition_in_db();
	if (ret == -1) {
		_LOGD("__update_preload_condition_in_db failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to commit transaction, Rollback now\n");

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
	_LOGD("Transaction Commit and End\n");
err:
	pkgmgr_parser_close_db();
	return ret;
}

API int pkgmgr_parser_delete_pkgid_info_from_db(const char *pkgid)
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
	ret = __delete_manifest_info_from_pkgid(pkgid);
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

API int pkgmgr_parser_delete_appid_info_from_db(const char *appid)
{
	retvm_if(appid == NULL, PMINFO_R_ERROR, "argument supplied is NULL");

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

	_LOGD("Start to Delete appid[%s] info from db\n", appid);

	/*delete appinfo db*/
	ret = __delete_manifest_info_from_appid(appid);
	if (ret < 0)
		_LOGE("Fail to delete appid[%s] info from pkgmgr db\n", appid);

	/*delete appsvc db*/
	ret = __delete_appsvc_db(appid);
	if (ret < 0)
		_LOGE("Fail to delete appid[%s] info from appsvc db\n", appid);

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

API int pkgmgr_parser_update_enabled_pkg_info_in_db(const char *pkgid)
{
	retvm_if(pkgid == NULL, PMINFO_R_ERROR, "pkgid is NULL.");

	int ret = 0;

	_LOGD("pkgmgr_parser_update_enabled_pkg_info_in_db(%s)\n", pkgid);

	/*open db*/
	ret = pkgmgr_parser_check_and_create_db();
	retvm_if(ret < 0, PMINFO_R_ERROR, "pkgmgr_parser_check_and_create_db(%s) failed.\n", pkgid);

	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("sqlite3_exec(BEGIN EXCLUSIVE) failed.\n");
		ret = -1;
		goto err;
	}

	/*update pkg info*/
	ret = __update_pkg_info_for_disable(pkgid, false);
	if (ret == -1) {
		_LOGD("__update_pkg_info_for_disable(%s) failed.\n", pkgid);
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("sqlite3_exec(ROLLBACK) failed.\n");
		goto err;
	}

	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("sqlite3_exec(COMMIT) failed.\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("sqlite3_exec(ROLLBACK) failed.\n");

		ret = -1;
		goto err;
	}

err:
	pkgmgr_parser_close_db();
	return ret;
}

API int pkgmgr_parser_update_disabled_pkg_info_in_db(const char *pkgid)
{
	retvm_if(pkgid == NULL, PMINFO_R_ERROR, "pkgid is NULL.");

	int ret = 0;

	_LOGD("pkgmgr_parser_update_disabled_pkg_info_in_db(%s)\n", pkgid);

	/*open db*/
	ret = pkgmgr_parser_check_and_create_db();
	retvm_if(ret < 0, PMINFO_R_ERROR, "pkgmgr_parser_check_and_create_db(%s) failed.\n", pkgid);

	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("sqlite3_exec(BEGIN EXCLUSIVE) failed.\n");
		ret = -1;
		goto err;
	}

	/*update pkg info*/
	ret = __update_pkg_info_for_disable(pkgid, true);
	if (ret == -1) {
		_LOGD("__update_pkg_info_for_disable(%s) failed.\n", pkgid);
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("sqlite3_exec(ROLLBACK) failed.\n");
		goto err;
	}

	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("sqlite3_exec(COMMIT) failed.\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("sqlite3_exec(ROLLBACK) failed.\n");

		ret = -1;
		goto err;
	}

err:
	pkgmgr_parser_close_db();
	return ret;
}

int pkgmgr_parser_insert_app_aliasid_info_in_db(void)
{
	FILE *ini_file = NULL;
	char *match = NULL;
	char *tizen_id = NULL;
	char *app_id = NULL;
	int len = 0;
	char buf[ MAX_ALIAS_INI_ENTRY ] = {0};
	int ret = PM_PARSER_R_OK;
	sqlite3 *pkgmgr_db = NULL;
	char *query = NULL;

	/*Open the alias.ini file*/
	ini_file  = fopen(USR_APPSVC_ALIAS_INI,"r");
	if(ini_file){
		/*Open the pkgmgr DB*/
		ret = db_util_open_with_options(MANIFEST_DB, &pkgmgr_db, SQLITE_OPEN_READWRITE, NULL);
		if (ret != SQLITE_OK) {
			_LOGE("connect db [%s] failed!\n", MANIFEST_DB);
			fclose(ini_file);
			return PM_PARSER_R_ERROR;

		}
		/*Begin Transaction*/
		ret = sqlite3_exec(pkgmgr_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
		tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "Failed to begin transaction");

		_LOGD("Transaction Begin\n");

		/*delete all the previous entries from package_app_aliasid tables*/
		query = sqlite3_mprintf("delete from package_app_aliasid");
		ret = __exec_db_query(pkgmgr_db, query, NULL, NULL);
		sqlite3_free(query);
		tryvm_if(ret != SQLITE_OK,ret = PM_PARSER_R_ERROR,"sqlite exec failed to delete entries from package_app_aliasid!!");

		while ( fgets ( buf, sizeof(buf), ini_file ) != NULL ) /* read a line */
		{
			match = strstr(buf,"=");
			if(match == NULL)
				continue;
			/*format of alias.ini entry is 'tizen_app_id=alias_id'*/
			/*get the tizen appid id*/
			len = strlen(buf)-strlen(match);
			tizen_id = malloc(len + 1);
			tryvm_if(tizen_id == NULL, ret = PM_PARSER_R_ERROR,"Malloc failed!!");
			memset(tizen_id,'\0',len +1);
			strncpy(tizen_id,buf,len);
			tizen_id[len]='\0';
			__str_trim(tizen_id);

			/*get the corresponding alias id*/
			len = strlen(match)-1;
			app_id = malloc(len + 1);
			tryvm_if(app_id == NULL, ret = PM_PARSER_R_ERROR,"Malloc failed!!");
			memset(app_id,'\0',len + 1);
			strncpy(app_id, match + 1, len);
			app_id[len]='\0';
			__str_trim(app_id);

			/* Insert the data into DB*/
			query = sqlite3_mprintf("insert into package_app_aliasid(app_id, alias_id) values(%Q ,%Q)",tizen_id,app_id);
			ret = __exec_db_query(pkgmgr_db, query, NULL, NULL);
			sqlite3_free(query);
			tryvm_if(ret != SQLITE_OK,ret = PM_PARSER_R_ERROR, "sqlite exec failed to insert entries into package_app_aliasid table!!");

			FREE_AND_NULL(tizen_id);
			FREE_AND_NULL(app_id);
			memset(buf,'\0',MAX_ALIAS_INI_ENTRY);
		}

		/*Commit transaction*/
		ret = sqlite3_exec(pkgmgr_db, "COMMIT", NULL, NULL, NULL);
		if (ret != SQLITE_OK) {
			_LOGE("Failed to commit transaction, Rollback now\n");
			ret = sqlite3_exec(pkgmgr_db, "ROLLBACK", NULL, NULL, NULL);
			if (ret != SQLITE_OK)
				_LOGE("Failed to Rollback\n");

			ret = PM_PARSER_R_ERROR;
			goto catch;
		}
		_LOGE("Transaction Commit and End\n");
		ret =  PM_PARSER_R_OK;
	}else{
		perror(USR_APPSVC_ALIAS_INI);
		ret = -1;
	}

catch:
	if(ini_file)
		fclose ( ini_file);

	FREE_AND_NULL(tizen_id);
	FREE_AND_NULL(app_id);
	sqlite3_close(pkgmgr_db);
	return ret;


}

int pkgmgr_parser_update_app_aliasid_info_in_db(void)
{
	FILE *ini_file = NULL;
	char *match = NULL;
	char *tizen_id = NULL;
	char *app_id = NULL;
	int len = 0;
	char buf[ MAX_ALIAS_INI_ENTRY ] = {0};
	int ret = PM_PARSER_R_OK;
	sqlite3 *pkgmgr_db = NULL;
	char *query = NULL;

	/*Open the alias.ini file*/
	ini_file  = fopen(OPT_APPSVC_ALIAS_INI,"r");
	if(ini_file){
		/*Open the pkgmgr DB*/
		ret = db_util_open_with_options(MANIFEST_DB, &pkgmgr_db, SQLITE_OPEN_READWRITE, NULL);
		if (ret != SQLITE_OK) {
			_LOGE("connect db [%s] failed!\n", MANIFEST_DB);
			fclose(ini_file);
			return PM_PARSER_R_ERROR;

		}
		/*Begin Transaction*/
		ret = sqlite3_exec(pkgmgr_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
		tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "Failed to begin transaction");

		_LOGD("Transaction Begin\n");

		while ( fgets ( buf, sizeof(buf), ini_file ) != NULL ) /* read a line */
		{
			match = strstr(buf,"=");
			if(match == NULL)
				continue;
			/*format of alias.ini entry is 'tizen_app_id=alias_id'*/
			/*get the tizen appid id*/
			len = strlen(buf)-strlen(match);
			tizen_id = malloc(len + 1);
			tryvm_if(tizen_id == NULL, ret = PM_PARSER_R_ERROR,"Malloc failed!!");
			memset(tizen_id,'\0',len +1);
			strncpy(tizen_id,buf,len);
			tizen_id[len]='\0';
			__str_trim(tizen_id);

			/*get the corresponding alias id*/
			len = strlen(match)-1;
			app_id = malloc(len + 1);
			tryvm_if(app_id == NULL, ret = PM_PARSER_R_ERROR,"Malloc failed!!");
			memset(app_id,'\0',len + 1);
			strncpy(app_id, match + 1, len);
			app_id[len]='\0';
			__str_trim(app_id);

			/* Insert the data into DB*/
			query = sqlite3_mprintf("insert or replace into package_app_aliasid(app_id, alias_id) values(%Q ,%Q)",tizen_id,app_id);
			ret = __exec_db_query(pkgmgr_db, query, NULL, NULL);
			sqlite3_free(query);
			tryvm_if(ret != SQLITE_OK,ret = PM_PARSER_R_ERROR, "sqlite exec failed to insert entries into package_app_aliasid table!!");

			FREE_AND_NULL(tizen_id);
			FREE_AND_NULL(app_id);
			memset(buf,'\0',MAX_ALIAS_INI_ENTRY);
		}

		/*Commit transaction*/
		ret = sqlite3_exec(pkgmgr_db, "COMMIT", NULL, NULL, NULL);
		if (ret != SQLITE_OK) {
			_LOGE("Failed to commit transaction, Rollback now\n");
			ret = sqlite3_exec(pkgmgr_db, "ROLLBACK", NULL, NULL, NULL);
			if (ret != SQLITE_OK)
				_LOGE("Failed to Rollback\n");

			ret = PM_PARSER_R_ERROR;
			goto catch;
		}
		_LOGE("Transaction Commit and End\n");
		ret =  PM_PARSER_R_OK;
	}else{
		perror(OPT_APPSVC_ALIAS_INI);
		ret = -1;
	}

catch:
	if(ini_file)
		fclose ( ini_file);

	FREE_AND_NULL(tizen_id);
	FREE_AND_NULL(app_id);
	sqlite3_close(pkgmgr_db);
	return ret;


}

