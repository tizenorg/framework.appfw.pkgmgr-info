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
#include <ctype.h>

#include "pkgmgr_parser_internal.h"
#include "pkgmgr_parser_db.h"

#include "pkgmgrinfo_debug.h"
#include "pkgmgrinfo_type.h"
#include "pkgmgr-info.h"

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
						"package_api_version text, " \
						"package_tep_name text, " \
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
						"package_backend_installer text DEFAULT 'rpm', " \
						"package_custom_smack_label text, " \
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
						"app_companion_type text DEFAULT 'false', " \
						"app_support_mode text, " \
						"app_support_feature text, " \
						"app_support_category text, " \
						"component_type text, " \
						"package text not null, " \
						"app_tep_name text, " \
						"app_package_type text DEFAULT 'rpm', " \
						"app_package_system text, " \
						"app_package_installed_time text, " \
						"app_launch_mode text NOT NULL DEFAULT 'caller', " \
						"app_alias_appid text, " \
						"app_effective_appid text, " \
						"app_background_category INTEGER DEFAULT 0, " \
						"app_api_version text, " \
						"app_mount_install INTEGER DEFAULT 0, " \
						"app_tpk_name text, " \
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
						"(app_id text not null, "\
						"alias_id text primary key not null, "\
						"type INTEGER)"

typedef enum {
	ALIAS_APPID_TYPE_PREDEFINED = 0,
	ALIAS_APPID_TYPE_APPDEFINED = 1
} alias_appid_type;

// pkgmgr parser DB
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

int __verify_label_cb(void *data, int ncols, char **coltxt, char **colname)
{
	if (strcasecmp(coltxt[0], PKGMGR_PARSER_EMPTY_STR) == 0) {
		_LOGE("package_label is PKGMGR_PARSER_EMPTY_STR");
		return -1;
	}
	return 0;
}

int __exec_verify_label(char *query, sqlite_query_callback callback)
{
	if (SQLITE_OK != sqlite3_exec(pkgmgr_parser_db, query, callback, NULL, NULL)) {
		return -1;
	}
	return 0;
}

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

	char *query = NULL;
	query = sqlite3_mprintf("PRAGMA user_version=%d", PKGMGR_PARSER_DB_VERSION);
	ret = __evaluate_query(handle, query);
	sqlite3_free(query);

	*db_handle = handle;
	return 0;
}

int __evaluate_query(sqlite3 *db_handle, char *query)
{
	sqlite3_stmt* p_statement;
	int result;
    result = sqlite3_prepare_v2(db_handle, query, strlen(query), &p_statement, NULL);
    if (result != SQLITE_OK) {
        _LOGE("Sqlite3 error [%d] : <%s> preparing <%s> query\n", result, sqlite3_errmsg(db_handle), query);
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

GList *__create_locale_list(GList *locale, GList *lbl, GList *lcn, GList *icn, GList *dcn, GList *ath)
{

	while(lbl != NULL)
	{
		label_x* label = (label_x*)lbl->data;
		if (label && label->lang)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)label->lang, __comparefunc, NULL);
		lbl = lbl->next;
	}
	while(lcn != NULL)
	{
		license_x *lic = (license_x*)lcn->data;
		if (lic && lic->lang){
			locale = g_list_insert_sorted_with_data(locale, (gpointer)lic->lang, __comparefunc, NULL);
		}
		lcn = lcn->next;
	}
	while(icn != NULL)
	{
		icon_x* icon = (icon_x*)icn->data;
		if (icon && icon->lang){
			locale = g_list_insert_sorted_with_data(locale, (gpointer)icon->lang, __comparefunc, NULL);
		}
		icn = icn->next;
	}
	while(dcn != NULL)
	{
		description_x *desc = (description_x*)dcn->data;
		if (desc && desc->lang)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)desc->lang, __comparefunc, NULL);
		dcn = dcn->next;
	}
	while(ath != NULL)
	{
		author_x* author = (author_x*)ath->data;
		if (author && author->lang)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)author->lang, __comparefunc, NULL);
		ath = ath->next;
	}
	return locale;

}

static GList *__create_icon_list(GList *locale, GList *icn)
{
	while(icn != NULL)
	{
		icon_x *icon = (icon_x*)icn->data;
		if (icon && icon->section){
			locale = g_list_insert_sorted_with_data(locale, (gpointer)icon->section, __comparefunc, NULL);
		}
		icn = icn->next;
	}
	return locale;
}

static GList *__create_image_list(GList *locale, GList *image)
{
	while(image != NULL)
	{
		image_x* img = (image_x*)image->data;
		if (img && img->section)
			locale = g_list_insert_sorted_with_data(locale, (gpointer)img->section, __comparefunc, NULL);
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

void __extract_data(gpointer data, GList *lbl, GList *lcn, GList *icn, GList *dcn, GList *ath,
		char **label, char **license, char **icon, char **description, char **author)
{
	while(lbl != NULL)
	{
		label_x *tmp = (label_x*)lbl->data;
		if (tmp && tmp->lang) {
			if (strcmp(tmp->lang, (char *)data) == 0) {
				*label = (char*)tmp->text;
				break;
			}
		}
		lbl = lbl->next;
	}
	while(lcn != NULL)
	{
		license_x *lic = (license_x*)lcn->data;
		if (lic && lic->lang) {
			if (strcmp(lic->lang, (char *)data) == 0) {
				*license = (char*)lic->text;
				break;
			}
		}
		lcn = lcn->next;
	}
	while(icn != NULL)
	{
		icon_x *tmp = (icon_x*)icn->data;
		if(tmp){
			if (tmp->section == NULL) {
				if (tmp->lang) {
					if (strcmp(tmp->lang, (char *)data) == 0) {
						*icon = (char*)tmp->text;
						break;
					}
				}
			}
		}
		icn = icn->next;
	}
	while(dcn != NULL)
	{
		description_x *desc = (description_x*)dcn->data;
		if (desc && desc->lang) {
			if (strcmp(desc->lang, (char *)data) == 0) {
				*description = (char*)desc->text;
				break;
			}
		}
		dcn = dcn->next;
	}
	while(ath != NULL)
	{
		author_x* auth = (author_x*)ath->data;
		if (auth && auth->lang) {
			if (strcmp(auth->lang, (char *)data) == 0) {
				*author = (char*)auth->text;
				break;
			}
		}
		ath = ath->next;
	}

}

static void __extract_icon_data(gpointer data, GList *icn, char **icon, char **resolution)
{
	while(icn != NULL)
	{
		icon_x *tmp = (icon_x*)icn->data;
		if (tmp && tmp->section) {
			if (strcmp(tmp->section, (char *)data) == 0) {
				*icon = (char*)tmp->text;
				*resolution = (char*)tmp->resolution;
				break;
			}
		}
		icn = icn->next;
	}
}

static void __extract_image_data(gpointer data, GList *image, char **lang, char **img)
{
	while(image != NULL)
	{
		image_x* tmp = (image_x*)image->data;
		if (tmp && tmp->section) {
			if (strcmp(tmp->section, (char *)data) == 0) {
				*lang = (char*)tmp->lang;
				*img = (char*)tmp->text;
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
	GList *lbl = mfx->label;
	GList *lcn = mfx->license;
	GList *icn = mfx->icon;
	GList *dcn = mfx->description;
	GList *ath = mfx->author;

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
	uiapplication_x *up = NULL;
	GList *list_up = mfx->uiapplication;
	GList *dc = NULL;
	datacontrol_x *datacontrol = NULL;
	int ret = -1;
	char *query = NULL;

	while (list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		dc = up->datacontrol;
		while (dc != NULL) {
			datacontrol = (datacontrol_x*)dc->data;
			if(datacontrol){
				query = sqlite3_mprintf("insert into package_app_data_control(app_id, providerid, access, type) " \
					"values(%Q, %Q, %Q, %Q)", \
					up->appid,
					datacontrol->providerid,
					datacontrol->access,
					datacontrol->type);

				ret = __exec_query(query);
				sqlite3_free(query);
				if (ret == -1) {
					_LOGD("Package UiApp Data Control DB Insert Failed\n");
					return -1;
				}
			}
			dc = dc->next;
		}
		list_up = list_up->next;
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
	GList *lbl = up->label;
	GList *icn = up->icon;

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

		update_query = sqlite3_mprintf("select package_label from package_localized_info " \
			"where package=%Q and package_locale=%Q", up->package, (char*)data);
		ret = __exec_verify_label(update_query, __verify_label_cb);
		sqlite3_free(update_query);
		if (ret < 0) {
			update_query = sqlite3_mprintf("update package_localized_info set package_label=%Q " \
				"where package=%Q and package_locale=%Q", __get_str(label), up->package, (char*)data);
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
	GList *icn = up->icon;

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
	GList *image = up->image;

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
	uiapplication_x *up = NULL;
	GList *list_up = mfx->uiapplication;
	int ret = -1;
	char *query = NULL;
	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		query = sqlite3_mprintf("update package_app_info set app_mainapp=%Q where app_id=%Q", up->mainapp, up->appid);

		ret = __exec_query(query);
		sqlite3_free(query);
		if (ret == -1) {
			_LOGD("Package UiApp Info DB Insert Failed\n");
			return -1;
		}
		if (strcasecmp(up->mainapp, "True")==0) {

			if (mfx->mainapp_id != NULL)
				free(mfx->mainapp_id);

			mfx->mainapp_id = strdup(up->appid);
		}

		list_up = list_up->next;
	}

	if (mfx->mainapp_id == NULL){
		if (mfx->uiapplication && up->appid) {
			up = (uiapplication_x *)mfx->uiapplication->data;
			query = sqlite3_mprintf("update package_app_info set app_mainapp='true' where app_id=%Q", up->appid);
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

		FREE_AND_NULL(up->mainapp);
		up->mainapp= strdup("true");
		mfx->mainapp_id = strdup(up->appid);
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

static int __convert_background_category(GList *category_list)
{
	int ret = 0;
	GList *tmp_list = category_list;
	char *category_data = NULL;

	if (category_list == NULL)
		return 0;

	while (tmp_list != NULL) {
		category_data = (char *)tmp_list->data;
		if (strcmp(category_data, APP_BG_CATEGORY_MEDIA_STR) == 0) {
			ret = ret | APP_BG_CATEGORY_MEDIA_VAL;
		} else if (strcmp(category_data, APP_BG_CATEGORY_DOWNLOAD_STR) == 0) {
			ret = ret | APP_BG_CATEGORY_DOWNLOAD_VAL;
		} else if (strcmp(category_data, APP_BG_CATEGORY_BGNETWORK_STR) == 0) {
			ret = ret | APP_BG_CATEGORY_BGNETWORK_VAL;
		} else if (strcmp(category_data, APP_BG_CATEGORY_LOCATION_STR) == 0) {
			ret = ret | APP_BG_CATEGORY_LOCATION_VAL;
		} else if (strcmp(category_data, APP_BG_CATEGORY_SENSOR_STR) == 0) {
			ret = ret | APP_BG_CATEGORY_SENSOR_VAL;
		} else if (strcmp(category_data, APP_BG_CATEGORY_IOTCOMM_STR) == 0) {
			ret = ret | APP_BG_CATEGORY_IOTCOMM_VAL;
		} else if (strcmp(category_data, APP_BG_CATEGORY_SYSTEM) == 0) {
			ret = ret | APP_BG_CATEGORY_SYSTEM_VAL;
		} else {
			_LOGE("Unidentified category [%s]", category_data);
		}
		tmp_list = g_list_next(tmp_list);
	}
	return ret;
}

/* _PRODUCT_LAUNCHING_ENHANCED_
*  up->indicatordisplay, up->portraitimg, up->landscapeimg, up->guestmode_appstatus
*/
static int __insert_uiapplication_info(manifest_x *mfx)
{
	uiapplication_x *up = NULL;
	GList *list_up = mfx->uiapplication;
	int ret = -1;
	int background_value = 0;
	char *query = NULL;
	char *type = NULL;

	if (mfx->type)
		type = strdup(mfx->type);
	else
		type = strdup("rpm");

	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		background_value = __convert_background_category(up->background_category);
		if (background_value < 0) {
			_LOGE("Failed to retrieve background value[%d]", background_value);
			background_value = 0;
		}
		query = sqlite3_mprintf("insert into package_app_info(app_id, app_component, app_exec, app_nodisplay, app_type, app_onboot, " \
			"app_multiple, app_autorestart, app_taskmanage, app_enabled, app_hwacceleration, app_screenreader, app_mainapp , app_recentimage, " \
			"app_launchcondition, app_indicatordisplay, app_portraitimg, app_landscapeimg, app_effectimage_type, app_guestmodevisibility, app_permissiontype, "\
			"app_preload, app_submode, app_submode_mainid, app_installed_storage, app_process_pool, app_multi_instance, app_multi_instance_mainid, app_multi_window, app_support_disable, "\
			"app_ui_gadget, app_removable, app_companion_type, app_support_mode, app_support_feature, app_support_category, component_type, package, "\
			"app_package_type, app_package_system, app_package_installed_time, app_launch_mode, app_alias_appid, app_effective_appid, app_background_category, app_api_version"\
			") " \
			"values(%Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, '%d', %Q)",\
			up->appid,
			"uiapp",
			up->exec,
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
			up->companion_type,
			mfx->support_mode,
			up->support_feature,
			up->support_category,
			up->component_type,
			mfx->package,
			type,
			mfx->system,
			mfx->installed_time,
			up->launch_mode,
			__get_str(up->alias_appid),
			__get_str(up->effective_appid),
			background_value,
			__get_str(mfx->api_version)
			);

		ret = __exec_query(query);
		sqlite3_free(query);
		if (ret == -1) {
			_LOGD("Package UiApp Info DB Insert Failed\n");
			FREE_AND_NULL(type);
			return -1;
		}
		list_up = list_up->next;
	}
	FREE_AND_NULL(type);
	return 0;
}

static int __insert_uiapplication_appcategory_info(manifest_x *mfx)
{
	uiapplication_x *up = NULL;
	GList *list_up = mfx->uiapplication;
	category_x *category = NULL;
	GList *ct = NULL;
	int ret = -1;
	char *query = NULL;
	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		ct = up->category;
		while (ct != NULL)
		{
			category = (category_x*)ct->data;
			if(category){
				query = sqlite3_mprintf("insert into package_app_app_category(app_id, category) " \
					"values(%Q, %Q)",\
					 up->appid, category->name);
				ret = __exec_query(query);
				sqlite3_free(query);
				if (ret == -1) {
					_LOGD("Package UiApp Category Info DB Insert Failed\n");
					return -1;
				}
			}
			ct = ct->next;
		}
		list_up = list_up->next;
	}
	return 0;
}

static int __insert_uiapplication_app_aliasid_info(const char* appid, const char* alisid)
{
	int ret = -1;
	char *query = NULL;

	query = sqlite3_mprintf("insert into package_app_aliasid(app_id, alias_id, type) values(%Q ,%Q, %d)",
		appid,alisid,ALIAS_APPID_TYPE_APPDEFINED);
	if(query == NULL) {
		_LOGE("failed to mprintf query\n");
		return -1;
	}
	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1) {
		_LOGE("sqlite exec failed to insert aliasid(%s)!!\n", alisid);
		return -1;
	}

	return 0;
}

static int __insert_uiapplication_appmetadata_info(manifest_x *mfx)
{
	uiapplication_x *up = NULL;
	GList *list_up = mfx->uiapplication;
	metadata_x *metadata = NULL;
	GList *list_md = NULL;
	int ret = -1;
	char *query = NULL;

	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		list_md = up->metadata;
		while (list_md != NULL)
		{
			metadata = (metadata_x *)list_md->data;
			if (metadata && metadata->key){
				query = sqlite3_mprintf("insert into package_app_app_metadata(app_id, md_key, md_value) " \
					"values(%Q, %Q, %Q)",\
					 up->appid, metadata->key, __get_str(metadata->value));
				ret = __exec_query(query);
				sqlite3_free(query);
				if (ret == -1) {
					_LOGD("Package UiApp Metadata Info DB Insert Failed\n");
					return -1;
				}
			}
			list_md = list_md->next;
		}
		list_up = list_up->next;
	}
	return 0;
}

static int __insert_uiapplication_apppermission_info(manifest_x *mfx)
{
	uiapplication_x *up = NULL;
	GList *list_up = mfx->uiapplication;
	permission_x *perm = NULL;
	GList *list_pm = NULL;
	int ret = -1;
	char *query = NULL;
	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		list_pm = up->permission;
		while (list_pm != NULL)
		{
			perm = (permission_x*)list_pm->data;
			if(perm){
				query = sqlite3_mprintf("insert into package_app_app_permission(app_id, pm_type, pm_value) " \
					"values(%Q, %Q, %Q)",\
					 up->appid, perm->type, perm->value);
				ret = __exec_query(query);
				sqlite3_free(query);
				if (ret == -1) {
					_LOGD("Package UiApp permission Info DB Insert Failed\n");
					return -1;
				}
			}
			list_pm = list_pm->next;
		}
		list_up = list_up->next;
	}
	return 0;
}

static int __insert_uiapplication_appcontrol_info(manifest_x *mfx)
{
	int ret = 0;
	char *buf = NULL;
	char *buftemp = NULL;
	char *query = NULL;
	uiapplication_x *up = NULL;
	GList *list_up = mfx->uiapplication;
	buf = (char *)calloc(1, BUFMAX);
	retvm_if(!buf, PMINFO_R_ERROR, "Malloc Failed\n");

	buftemp = (char *)calloc(1, BUFMAX);
	tryvm_if(!buftemp, ret = PMINFO_R_ERROR, "Malloc Failed\n");

	for(; list_up; list_up=list_up->next) {
		up = (uiapplication_x *)list_up->data;
		if (up->appsvc) {
			appsvc_x *asvc = NULL;
			GList *list_asvc = NULL;
			operation_x *op = NULL;
			GList *list_op = NULL;
			mime_x *mi = NULL;
			GList *list_mime = NULL;
			uri_x *ui = NULL;
			GList *list_uri = NULL;
			subapp_x *sub = NULL;
			GList *list_subapp = NULL;
			const char *operation = NULL;
			const char *mime = NULL;
			const char *uri = NULL;
			const char *subapp = NULL;
			int i = 0;
			memset(buf, '\0', BUFMAX);
			memset(buftemp, '\0', BUFMAX);

			list_asvc = up->appsvc;
			while(list_asvc != NULL) {
				asvc = (appsvc_x *)list_asvc->data;
				list_op= asvc->operation;
				while(list_op != NULL) {
					op = (operation_x *)list_op->data;
					if (op && op->name)
						operation = op->name;
					list_mime = asvc->mime;

					do{
						if(list_mime){
							mi = (mime_x *)list_mime->data;
							if (mi && mi->name)
								mime = mi->name;
						}

						list_subapp = asvc->subapp;
						do{
							if(list_subapp){
								sub = (subapp_x *)list_subapp->data;
								if (sub && sub->name)
									subapp = sub->name;
							}

							list_uri = asvc->uri;
							do{
								if(list_uri){
									ui = (uri_x *)list_uri->data;
									if (ui && ui->name)
										uri = ui->name;
								}

								if(i++ > 0) {
									strncpy(buftemp, buf, BUFMAX);
									snprintf(buf, BUFMAX, "%s;", buftemp);
								}

								strncpy(buftemp, buf, BUFMAX);
								snprintf(buf, BUFMAX, "%s%s|%s|%s|%s", buftemp, operation ? (strlen(operation) > 0 ? operation : "NULL") : "NULL", uri ? (strlen(uri) > 0 ? uri : "NULL") : "NULL", mime ? (strlen(mime) > 0 ? mime : "NULL") : "NULL", subapp ? (strlen(subapp) > 0 ? subapp : "NULL") : "NULL");
//								_LOGD("buf[%s]\n", buf);

								if (list_uri)
									list_uri = list_uri->next;
								uri = NULL;
							}while(list_uri != NULL);
							if (list_subapp)
								list_subapp = list_subapp->next;
							subapp = NULL;
						}while(list_subapp != NULL);
						if (list_mime)
							list_mime = list_mime->next;
						mime = NULL;
					}while(list_mime != NULL);
					if (list_op)
						list_op = list_op->next;
					operation = NULL;
				}
				list_asvc = list_asvc->next;
			}

			query= sqlite3_mprintf("insert into package_app_app_control(app_id, app_control) values(%Q, %Q)", up->appid, buf);
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
	uiapplication_x *up = NULL;
	GList *list_up = mfx->uiapplication;
	appsvc_x *asvc = NULL;
	GList *list_asvc = NULL;
	operation_x *op = NULL;
	GList *list_op = NULL;
	mime_x *mi = NULL;
	GList *list_mime = NULL;
	uri_x *ui = NULL;
	GList *list_uri = NULL;
	subapp_x *sub = NULL;
	GList *list_subapp = NULL;
	int ret = -1;
	char *query = NULL;
	const char *operation = NULL;
	const char *mime = NULL;
	const char *uri = NULL;
	const char *subapp = NULL;

	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		list_asvc = up->appsvc;
		while(list_asvc != NULL)
		{
			asvc = (appsvc_x *)list_asvc->data;
			list_op = asvc->operation;
			while(list_op!= NULL)
			{
				op = (operation_x *)list_op->data;
				if(op && op->name)
					operation = op->name;

				list_mime = asvc->mime;
				do{
					if(list_mime){
						mi = (mime_x *)list_mime->data;
						if (mi)
							mime = mi->name;
					}

					list_subapp = asvc->subapp;
					do{
						if(list_subapp){
							sub = (subapp_x *)list_subapp->data;
							if (sub)
								subapp = sub->name;
						}

						list_uri= asvc->uri;
						do{
							if(list_uri){
								ui = (uri_x *)list_uri->data;
								if (ui && ui->name)
									uri = ui->name;
							}

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
							if (list_uri)
								list_uri = list_uri->next;
							uri = NULL;
						}while(list_uri != NULL);
						if (list_subapp)
							list_subapp = list_subapp->next;
						subapp = NULL;
					}while(list_subapp != NULL);
					if (list_mime)
						list_mime = list_mime->next;
					mime = NULL;
				}while(list_mime != NULL);
				if (list_op)
					list_op = list_op->next;
				operation = NULL;
			}
			list_asvc = list_asvc->next;
		}
		list_up = list_up->next;
	}
	return 0;
}

static int __insert_uiapplication_share_request_info(manifest_x *mfx)
{
	uiapplication_x *up = NULL;
	GList *list_up = mfx->uiapplication;
	GList *ds = NULL;
	datashare_x *datashare = NULL;
	GList *rq = NULL;
	request_x *req = NULL;
	int ret = -1;
	char *query = NULL;

	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		ds = up->datashare;
		while(ds != NULL)
		{
			datashare = (datashare_x*)ds->data;
			rq = datashare->request;
			while(rq != NULL)
			{
				req = (request_x*)rq->data;
				if(req){
					query = sqlite3_mprintf("insert into package_app_share_request(app_id, data_share_request) " \
						"values(%Q, %Q)",\
						 up->appid, req->text);
					ret = __exec_query(query);
					sqlite3_free(query);
					if (ret == -1) {
						_LOGD("Package UiApp Share Request DB Insert Failed\n");
						return -1;
					}
				}
				rq = rq->next;
			}
			ds = ds->next;
		}
		list_up = list_up->next;
	}
	return 0;
}

static int __insert_uiapplication_share_allowed_info(manifest_x *mfx)
{
	uiapplication_x *up = NULL;
	GList *list_up = mfx->uiapplication;
	GList *ds = NULL;
	datashare_x* datashare = NULL;
	GList *df = NULL;
	define_x* def = NULL;
	GList *al = NULL;
	allowed_x *alw = NULL;
	int ret = -1;
	char *query = NULL;

	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		ds = up->datashare;
		while(ds != NULL)
		{
			datashare = (datashare_x*)ds->data;
			df = datashare->define;
			while(df != NULL)
			{
				def = (define_x*)df->data;
				al = def->allowed;
				while(al != NULL)
				{
					alw = (allowed_x*)al->data;
					if(alw){
						query = sqlite3_mprintf("insert into package_app_share_allowed(app_id, data_share_path, data_share_allowed) " \
							"values(%Q, %Q, %Q)",\
							 up->appid, def->path, alw->text);
						ret = __exec_query(query);
						sqlite3_free(query);
						if (ret == -1) {
							_LOGD("Package UiApp Share Allowed DB Insert Failed\n");
							return -1;
						}
					}
					al = al->next;
				}
				df = df->next;
			}
			ds = ds->next;
		}
		list_up = list_up->next;
	}
	return 0;
}

static int __insert_manifest_info_in_db(manifest_x *mfx)
{
	GList *lbl = mfx->label;
	GList *lcn = mfx->license;
	GList *icn = mfx->icon;
	GList *dcn = mfx->description;
	GList *ath = mfx->author;
	uiapplication_x *up = NULL;
	uiapplication_x *up_icn = NULL;
	uiapplication_x *up_image = NULL;
	uiapplication_x *up_support_mode = NULL;
	GList *list_up = NULL;
	int temp_pkg_mode = 0;
	int temp_app_mode = 0;
	char pkg_mode[10] = {'\0'};
	GList *pvs = NULL;
	char *pv = NULL;
	char *query = NULL;
	int ret = -1;
	const char *auth_name = NULL;
	const char *auth_email = NULL;
	const char *auth_href = NULL;

	GList *pkglocale = NULL;
	GList *applocale = NULL;
	GList *appicon = NULL;
	GList *appimage = NULL;

	if (ath) {
		author_x* auth = (author_x*)ath->data;
		if(auth){
			if (auth->text)
				auth_name = auth->text;
			if (auth->email)
				auth_email = auth->email;
			if (auth->href)
				auth_href = auth->href;
		}
	}

	// support-mode ORing
	if (mfx->support_mode) {
		temp_pkg_mode = atoi(mfx->support_mode);
	}
	list_up = mfx->uiapplication;
	while (list_up != NULL) {
		up_support_mode = (uiapplication_x *)list_up->data;
		if (up_support_mode->support_mode) {
			temp_app_mode = atoi(up_support_mode->support_mode);
			temp_pkg_mode |= temp_app_mode;
		}
		list_up = list_up->next;
	}
	snprintf(pkg_mode, sizeof(pkg_mode), "%d", temp_pkg_mode);

	if(mfx->support_mode)
		free((void *)mfx->support_mode);
	mfx->support_mode = strdup(pkg_mode);

	/*Insert in the package_info DB*/
	char *backend = NULL;
	if (mfx->type)
		backend = strdup(mfx->type);
	else
		backend = strdup("rpm");

	if (backend == NULL) {
		_LOGE("Out of memory");
		return -1;
	}

	if (!mfx->backend_installer)
		mfx->backend_installer = strdup(backend);

	FREE_AND_NULL(backend);

	query = sqlite3_mprintf("insert into package_info(package, package_type, package_version, package_api_version, install_location, package_size, " \
		"package_removable, package_preload, package_readonly, package_update, package_appsetting, package_nodisplay, package_system," \
		"author_name, author_email, author_href, installed_time, installed_storage, storeclient_id, mainapp_id, package_url, root_path, csc_path, package_support_disable, " \
		"package_mother_package, package_support_mode, package_backend_installer, package_custom_smack_label) " \
		"values(%Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q, %Q)",\
		mfx->package,
		mfx->type,
		mfx->version,
		__get_str(mfx->api_version),
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
		mfx->root_path,
		__get_str(mfx->csc_path),
		mfx->support_disable,
		mfx->mother_package,
		mfx->support_mode,
		mfx->backend_installer,
		mfx->custom_smack_label);

	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1) {
		_LOGD("Package Info DB Insert Failed\n");
		return -1;
	}

	/*Insert in the package_privilege_info DB*/
	pvs = mfx->privileges;
	while (pvs != NULL) {
		pv = (char*)pvs->data;
		if (pv != NULL) {
			query = sqlite3_mprintf("insert into package_privilege_info(package, privilege) " \
				"values(%Q, %Q)",\
				 mfx->package, pv);
			ret = __exec_query(query);
			sqlite3_free(query);
			if (ret == -1) {
				_LOGD("Package Privilege Info DB Insert Failed\n");
				return -1;
			}
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
	list_up = mfx->uiapplication;
	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		applocale = __create_locale_list(applocale, up->label, NULL, up->icon, NULL, NULL);
		list_up = list_up->next;
	}

	/*remove duplicated data in applocale*/
	__trimfunc(applocale);

	/*Insert the app icon info */
	list_up = mfx->uiapplication;
	while(list_up != NULL)
	{
		up_icn = (uiapplication_x *)list_up->data;
		appicon = __create_icon_list(appicon, up_icn->icon);
		list_up = list_up->next;
	}
	/*remove duplicated data in appicon*/
	__trimfunc(appicon);

	/*Insert the image info */
	list_up = mfx->uiapplication;
	while(list_up != NULL)
	{
		up_image = (uiapplication_x *)list_up->data;
		appimage = __create_image_list(appimage, up_image->image);
		list_up = list_up->next;
	}
	/*remove duplicated data in appimage*/
	__trimfunc(appimage);

	/*g_list_foreach(pkglocale, __printfunc, NULL);*/
	/*_LOGD("\n");*/
	/*g_list_foreach(applocale, __printfunc, NULL);*/

	g_list_foreach(pkglocale, __insert_pkglocale_info, (gpointer)mfx);

	/*native app locale info*/
		list_up = mfx->uiapplication;
		while(list_up != NULL)
		{
			up = (uiapplication_x *)list_up->data;
			g_list_foreach(applocale, __insert_uiapplication_locale_info, (gpointer)up);
			list_up = list_up->next;
		}

	/*app icon locale info*/
	list_up = mfx->uiapplication;
	while(list_up != NULL)
	{
		up_icn = (uiapplication_x *)list_up->data;
		g_list_foreach(appicon, __insert_uiapplication_icon_section_info, (gpointer)up_icn);
		list_up = list_up->next;
	}

	/*app image info*/
	list_up = mfx->uiapplication;
	while(list_up != NULL)
	{
		up_image = (uiapplication_x *)list_up->data;
		g_list_foreach(appimage, __insert_uiapplication_image_info, (gpointer)up_image);
		list_up = list_up->next;
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

static int __update_app_info_for_bg_disable(const char *appid, bool disable)
{
	char *query = NULL;
	int ret = -1;

	/*Update from package info*/
	if (disable)
		query = sqlite3_mprintf("update package_app_info set app_background_category = app_background_category & ~1 where app_id=%Q", appid);
	else
		query = sqlite3_mprintf("update package_app_info set app_background_category = app_background_category | 1 where app_id=%Q", appid);
	ret = __exec_query(query);
	sqlite3_free(query);
	retvm_if(ret < 0, PMINFO_R_ERROR, "__exec_query() failed.\n");

	return PMINFO_R_OK;
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

static int __get_appid_list_cb(void *data, int ncols, char **coltxt, char **colname)
{
	if (ncols != 1) {
		_LOGE("invalid parameter!");
		return -1;
	}
	GList **tmp_list = (GList **)data;

	if (strlen(*coltxt) == 0 || strcmp(*colname, "app_id") != 0) {
		_LOGE("wrong return value, colname[%s], coltxt[%s]", *colname, *coltxt);
		return -1;
	}

	*tmp_list = g_list_append(*tmp_list, strdup(*coltxt));
	return 0;
}

static void appid_list_free(gpointer data)
{
	FREE_AND_NULL(data);
}

static int __delete_manifest_info_from_db(manifest_x *mfx)
{
	int ret = -1;
	GList *list_up = NULL;
	GList *tmp_list = NULL;
	char *query = sqlite3_mprintf("select app_id from package_app_info where package=%Q", mfx->package);

	/* Delete package info DB */
	ret = __delete_manifest_info_from_pkgid(mfx->package);
	if (ret < 0) {
		_LOGE("Package Info DB Delete Failed\n");
		goto catch;
	}

	/* Search package_app_info DB and get app_id list related with given package */
	ret = __exec_db_query(pkgmgr_parser_db, query, __get_appid_list_cb, &list_up);
	if (ret < 0) {
		_LOGE("failed to exec query");
		goto catch;
	}
	tmp_list = list_up;
	while (tmp_list != NULL) {
		ret = __delete_manifest_info_from_appid((char *)tmp_list->data);
		retvm_if(ret < 0, PMINFO_R_ERROR, "App Info DB Delete Failed\n");

		tmp_list = tmp_list->next;
	}

catch:

	if (query)
		sqlite3_free(query);

	g_list_free_full(list_up, (GDestroyNotify)appid_list_free);
	return ret;
}

static int __delete_appsvc_db(const char *appid)
{
	int ret = -1;
	void *lib_handle = NULL;
	int (*appsvc_operation) (const char *);

	if ((lib_handle = dlopen(LIBAPPSVC_PATH, RTLD_LAZY)) == NULL) {
		_LOGE("dlopen is failed LIBAPPSVC_PATH[%s]\n", LIBAPPSVC_PATH);
		return ret;
	}
	if ((appsvc_operation = dlsym(lib_handle, "appsvc_unset_defapp")) == NULL || dlerror() != NULL) {
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
	uiapplication_x *uiapplication = NULL;
	GList *list_up = mfx->uiapplication;

	for(; list_up; list_up=list_up->next) {
		uiapplication = (uiapplication_x *)list_up->data;
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

static int __delete_alias_appid_manifest_info_from_appid(const char *appid)
{
	int ret = PM_PARSER_R_OK;
	char *query = NULL;

	query = sqlite3_mprintf("delete from package_app_aliasid where app_id = %Q and type = %d",
		appid, ALIAS_APPID_TYPE_APPDEFINED);
	if(query == NULL) {
		_LOGE("failed to mprintf query\n");
		return PM_PARSER_R_ERROR;
	}

	ret = __exec_query(query);
	tryvm_if(ret != SQLITE_OK,ret = PM_PARSER_R_ERROR,"Package alias appid delete failed!!");

catch:
	if(query) sqlite3_free(query);
	return ret;
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

	ret = __delete_alias_appid_manifest_info_from_appid(appid);
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

	query = sqlite3_mprintf("PRAGMA user_version=%d", PKGMGR_PARSER_DB_VERSION);
	ret = __exec_query(query);
	sqlite3_free(query);
	if (ret == -1)
		_LOGD("Package user_version update failed\n");

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

	return 0;
}

int zone_pkgmgr_parser_check_and_create_db(const char *zone)
{
	int ret = -1;
	/*Manifest DB*/
	char db_path[PKG_STRING_LEN_MAX] = { '\0', };
	char *rootpath = NULL;


	if (zone == NULL || strcmp(zone, ZONE_HOST) == 0) {
		snprintf(db_path, PKG_STRING_LEN_MAX, "%s", PKGMGR_PARSER_DB_FILE);
	} else {
		rootpath = __zone_get_root_path(zone);
		if (rootpath == NULL) {
			_LOGE("Failed to get rootpath");
			return -1;
		}

		snprintf(db_path, PKG_STRING_LEN_MAX, "%s%s", rootpath, PKGMGR_PARSER_DB_FILE);
	}

	_LOGD("db path(%s)", db_path);
	ret = __pkgmgr_parser_create_db(&pkgmgr_parser_db, db_path);

	if (ret) {
		_LOGD("Manifest DB creation Failed\n");
		return -1;
	}
	/*Cert DB*/
	memset(db_path, '\0', PKG_STRING_LEN_MAX);

	if (zone == NULL || strcmp(zone, ZONE_HOST) == 0) {
		snprintf(db_path, PKG_STRING_LEN_MAX, "%s", PKGMGR_CERT_DB_FILE);
	} else {
		_LOGD("rootpath : %s", rootpath);
		snprintf(db_path, PKG_STRING_LEN_MAX, "%s%s", rootpath, PKGMGR_CERT_DB_FILE);
	}

	_LOGD("db path(%s)", db_path);
	ret = __pkgmgr_parser_create_db(&pkgmgr_cert_db, db_path);

	if (ret) {
		_LOGD("Cert DB creation Failed\n");
		return -1;
	}
	return 0;
}

int pkgmgr_parser_check_and_create_db()
{
	return zone_pkgmgr_parser_check_and_create_db(NULL);
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

	// pkgmgr parser DB transaction
	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to begin transaction[%d]\n", ret);
		ret = PM_PARSER_R_ERROR;
		goto err;
	}
	_LOGD("Transaction Begin\n");
	ret = __insert_manifest_info_in_db(mfx);
	if (ret == -1) {
		_LOGD("Insert into DB failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGD("ROLLBACK is fail after insert_pkg_info_in_db\n");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}
	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
		_LOGE("Failed to commit transaction. Rollback now\n");

		ret = PM_PARSER_R_ERROR;
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

	// pkgmgr parser DB transaction
	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to begin transaction\n");
		ret = PM_PARSER_R_ERROR;
		goto err;
	}
	_LOGD("Transaction Begin\n");
	ret = __delete_manifest_info_from_db(mfx);
	if (ret == -1) {
		_LOGD("Delete from DB failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to rollback transaction.\n");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}
	ret = __insert_manifest_info_in_db(mfx);
	if (ret == -1) {
		_LOGD("Insert into DB failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to rollback transaction\n");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}

	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGD("Failed to commit transaction. Rollback now\n");

		ret = PM_PARSER_R_ERROR;
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
	// pkgmgr parser DB transaction
	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to begin transaction\n");
		ret = PM_PARSER_R_ERROR;
		goto err;
	}
	_LOGD("Transaction Begin\n");
	ret = __delete_manifest_info_from_db(mfx);
	if (ret == -1) {
		_LOGD("Delete from DB failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to rollback transaction \n");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}

	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to commit transaction, Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to commit transaction, Rollback now\n");

		ret = PM_PARSER_R_ERROR;
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
		ret = PM_PARSER_R_ERROR;
		goto err;
	}
	_LOGD("Transaction Begin\n");
	ret = __update_preload_condition_in_db();
	if (ret == -1) {
		_LOGD("__update_preload_condition_in_db failed. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to ROLLBACK for update preload condition.");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}
	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to commit transaction, Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to ROLLBACK for COMMIT.");

		ret = PM_PARSER_R_ERROR;
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
		ret = PM_PARSER_R_ERROR;
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
		ret = PM_PARSER_R_ERROR;
		goto err;
	}

	// pkgmgr parser DB transaction
	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to commit transaction, Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to commit transaction, Rollback now\n");

		ret = PM_PARSER_R_ERROR;
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

	// pkgmgr parser DB transaction
	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to begin transaction\n");
		ret = PM_PARSER_R_ERROR;
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

		ret = PM_PARSER_R_ERROR;
		goto err;
	}

err:
	pkgmgr_parser_close_db();
	return ret;
}

int zone_pkgmgr_parser_update_enabled_pkg_info_in_db(const char *pkgid, const char *zone)
{
	retvm_if(pkgid == NULL, PMINFO_R_ERROR, "pkgid is NULL.");

	int ret = 0;

	_LOGD("pkgmgr_parser_update_enabled_pkg_info_in_db(%s)\n", pkgid);

	/*open db*/
	ret = zone_pkgmgr_parser_check_and_create_db(zone);
	retvm_if(ret < 0, PMINFO_R_ERROR, "pkgmgr_parser_check_and_create_db(%s) failed.\n", pkgid);

	// pkgmgr changeable DB transaction
	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("sqlite3_exec(BEGIN EXCLUSIVE) failed.\n");
		ret = PM_PARSER_R_ERROR;
		goto err;
	}

	/*update pkg info*/
	ret = __update_pkg_info_for_disable(pkgid, false);
	if (ret == -1) {
		_LOGD("__update_pkg_info_for_disable(%s) failed.\n", pkgid);
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("sqlite3_exec(ROLLBACK) failed.\n");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}

	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("sqlite3_exec(COMMIT) failed.\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("sqlite3_exec(ROLLBACK) failed.\n");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}

err:
	pkgmgr_parser_close_db();
	return ret;
}

API int pkgmgr_parser_update_enabled_pkg_info_in_db(const char *pkgid)
{
	return zone_pkgmgr_parser_update_enabled_pkg_info_in_db(pkgid, NULL);
}

int zone_pkgmgr_parser_update_app_disable_bg_operation_info_in_db(const char *appid, const char *zone, bool is_disable)
{
	retvm_if(appid == NULL, PMINFO_R_ERROR, "appid is NULL.");

	int ret = -1;
	_LOGD("zone_pkgmgr_parser_update_app_disable_bg_operation_info_in_db[%s]\n", appid);

	/*open db*/
	ret = zone_pkgmgr_parser_check_and_create_db(zone);
	retvm_if(ret < 0, PMINFO_R_ERROR, "pkgmgr_parser_check_and_create_db(%s) failed.\n", appid);

	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("sqlite3_exec(BEGIN EXCLUSIVE) failed.\n");
		ret = PM_PARSER_R_ERROR;
		goto err;
	}

	/*update app info*/
	ret = __update_app_info_for_bg_disable(appid, is_disable);
	if (ret == -1) {
		_LOGE("update_app_info_for_bg_disable[%s] failed.\n", appid);
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("sqlite3_exec(ROLLBACK) failed.\n");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}

	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("sqlite3_exec(COMMIT) failed.\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("sqlite3_exec(ROLLBACK) failed.\n");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}

err:
	pkgmgr_parser_close_db();
	return ret;
}

int zone_pkgmgr_parser_update_disabled_pkg_info_in_db(const char *pkgid, const char *zone)
{
	retvm_if(pkgid == NULL, PMINFO_R_ERROR, "pkgid is NULL.");

	int ret = 0;

	_LOGD("pkgmgr_parser_update_disabled_pkg_info_in_db(%s)\n", pkgid);

	/*open db*/
	ret = zone_pkgmgr_parser_check_and_create_db(zone);
	retvm_if(ret < 0, PMINFO_R_ERROR, "pkgmgr_parser_check_and_create_db(%s) failed.\n", pkgid);

	// pkgmgr changeable DB transaction
	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("sqlite3_exec(BEGIN EXCLUSIVE) failed.\n");
		ret = PM_PARSER_R_ERROR;
		goto err;
	}

	/*update pkg info*/
	ret = __update_pkg_info_for_disable(pkgid, true);
	if (ret == -1) {
		_LOGD("__update_pkg_info_for_disable(%s) failed.\n", pkgid);
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("sqlite3_exec(ROLLBACK) failed.\n");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}

	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("sqlite3_exec(COMMIT) failed.\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("sqlite3_exec(ROLLBACK) failed.\n");

		ret = PM_PARSER_R_ERROR;
		goto err;
	}

err:
	pkgmgr_parser_close_db();
	return ret;
}

API int pkgmgr_parser_update_disabled_pkg_info_in_db(const char *pkgid)
{
	return zone_pkgmgr_parser_update_disabled_pkg_info_in_db(pkgid, NULL);
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

		/*delete previous predefined entries from package_app_aliasid tables*/
		query = sqlite3_mprintf("delete from package_app_aliasid where type = %d", ALIAS_APPID_TYPE_PREDEFINED);
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
			query = sqlite3_mprintf("insert into package_app_aliasid(app_id, alias_id, type) values(%Q ,%Q, %d)",
			app_id, tizen_id, ALIAS_APPID_TYPE_PREDEFINED);
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
			query = sqlite3_mprintf("insert or replace into package_app_aliasid(app_id, alias_id, type) values(%Q ,%Q, %d)",
						app_id,tizen_id,ALIAS_APPID_TYPE_PREDEFINED);
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

#ifdef _APPFW_FEATURE_EXPANSION_PKG_INSTALL
int pkgmgr_parser_insert_tep_in_db(const char *pkgid, const char *tep_name)
{
	int ret = PM_PARSER_R_OK;
	sqlite3 *pkgmgr_db = NULL;
	char *query = NULL;

	retvm_if(pkgid == NULL, PMINFO_R_ERROR, "pkgid is NULL.");
	retvm_if(tep_name == NULL, PMINFO_R_ERROR, "tep path is NULL.");

	/*Open the pkgmgr DB*/
	ret = db_util_open_with_options(MANIFEST_DB, &pkgmgr_db, SQLITE_OPEN_READWRITE, NULL);
	if(ret != SQLITE_OK){
		_LOGE("connect db [%s] failed!\n", MANIFEST_DB);
		return PM_PARSER_R_ERROR;
	}

	/*Begin Transaction*/
	ret = sqlite3_exec(pkgmgr_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "Failed to begin transaction");

	_LOGD("Transaction Begin\n");

	/* Updating TEP info in "package_info" table */
	query = sqlite3_mprintf("UPDATE package_info "\
						"SET package_tep_name = %Q "\
						"WHERE package = %Q", tep_name, pkgid);

	ret = __exec_db_query(pkgmgr_db, query, NULL, NULL);
	sqlite3_free(query);
	tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "sqlite exec failed to insert entries into package_info!!");

	/* Updating TEP info in "package_app_info" table */
	query = sqlite3_mprintf("UPDATE package_app_info "\
						"SET app_tep_name = %Q "\
						"WHERE package = %Q", tep_name, pkgid);

	ret = __exec_db_query(pkgmgr_db, query, NULL, NULL);
	sqlite3_free(query);
	tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "sqlite exec failed to insert entries into package_app_info!!");


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
	_LOGD("Transaction Commit and End\n");
	ret =  PM_PARSER_R_OK;

catch:
	sqlite3_close(pkgmgr_db);
	return ret;
}

int pkgmgr_parser_update_tep_in_db(const char *pkgid, const char *tep_name)
{
	int ret = PM_PARSER_R_OK;
	sqlite3 *pkgmgr_db = NULL;
	char *query = NULL;

	retvm_if(pkgid == NULL, PMINFO_R_ERROR, "pkgid is NULL.");
	retvm_if(tep_name == NULL, PMINFO_R_ERROR, "tep name is NULL.");

	/*Open the pkgmgr DB*/
	ret = db_util_open_with_options(MANIFEST_DB, &pkgmgr_db, SQLITE_OPEN_READWRITE, NULL);
	if(ret != SQLITE_OK){
		_LOGE("connect db [%s] failed!\n", MANIFEST_DB);
		return PM_PARSER_R_ERROR;
	}

	/*Begin Transaction*/
	ret = sqlite3_exec(pkgmgr_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "Failed to begin transaction");

	_LOGD("Transaction Begin\n");

	/* Updating TEP info in "package_info" table */
	query = sqlite3_mprintf("UPDATE package_info "\
						"SET package_tep_name = %Q "\
						"WHERE package = %Q", tep_name, pkgid);

	ret = __exec_db_query(pkgmgr_db, query, NULL, NULL);
	sqlite3_free(query);
	tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "sqlite exec failed to insert entries into package_info!!");


	/* Updating TEP info in "package_app_info" table */
	query = sqlite3_mprintf("UPDATE package_app_info "\
						"SET app_tep_name = %Q "\
						"WHERE package = %Q", tep_name, pkgid);

	ret = __exec_db_query(pkgmgr_db, query, NULL, NULL);
	sqlite3_free(query);
	tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "sqlite exec failed to insert entries into package_app_info!!");

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
	_LOGD("Transaction Commit and End\n");
	ret =  PM_PARSER_R_OK;

catch:
	sqlite3_close(pkgmgr_db);
	return ret;
}

int pkgmgr_parser_delete_tep_in_db(const char *pkgid)
{

	int ret = PM_PARSER_R_OK;
	sqlite3 *pkgmgr_db = NULL;
	char *query = NULL;
	int row_exist = 0;
	sqlite3_stmt *stmt = NULL;

	retvm_if(pkgid == NULL, PMINFO_R_ERROR, "pkgid is NULL.");

	/*Open the pkgmgr DB*/
	ret = db_util_open_with_options(MANIFEST_DB, &pkgmgr_db, SQLITE_OPEN_READWRITE, NULL);
	if(ret != SQLITE_OK){
		_LOGE("connect db [%s] failed!\n", MANIFEST_DB);
		return PM_PARSER_R_ERROR;
	}

	/*Check for any existing entry having same pkgid*/
	query = sqlite3_mprintf("select count(*) from package_info "\
					"where pkgid=%Q", pkgid);
	ret = sqlite3_prepare_v2(pkgmgr_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	ret = sqlite3_step(stmt);
	if(ret == SQLITE_ROW) {
		row_exist  = sqlite3_column_int(stmt,0);
	}
	sqlite3_free(query);

	if(row_exist){

		/*Begin Transaction*/
		ret = sqlite3_exec(pkgmgr_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
		tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "Failed to begin transaction");

		_LOGD("Transaction Begin\n");

		/* Deleting TEP info in "package_info" table */
		query = sqlite3_mprintf("UPDATE package_info "\
							"SET package_tep_name = %Q "\
							"WHERE package = %Q", NULL, pkgid);

		ret = __exec_db_query(pkgmgr_db, query, NULL, NULL);
		sqlite3_free(query);

		if (ret != SQLITE_OK) {
			_LOGD("Delete from DB failed. Rollback now\n");
			ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
			if (ret != SQLITE_OK)
				_LOGE("Failed to commit transaction, Rollback now\n");

			ret = PM_PARSER_R_ERROR;
			goto catch;
		}

		/* Deleting TEP info in "package_app_info" table */
		query = sqlite3_mprintf("UPDATE package_app_info "\
							"SET app_tep_name = %Q "\
							"WHERE package = %Q", NULL, pkgid);

		ret = __exec_db_query(pkgmgr_db, query, NULL, NULL);
		sqlite3_free(query);

		if (ret != SQLITE_OK) {
			_LOGD("Delete from DB failed. Rollback now\n");
			ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
			if (ret != SQLITE_OK)
				_LOGE("Failed to commit transaction, Rollback now\n");

			ret = PM_PARSER_R_ERROR;
			goto catch;
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
		_LOGD("Transaction Commit and End\n");
		ret =  PM_PARSER_R_OK;
	}else{
		_LOGE("PKGID does not exist in the table");
		ret = PM_PARSER_R_ERROR;
	}

catch:
	sqlite3_close(pkgmgr_db);
	sqlite3_finalize(stmt);

	return ret;
}
#endif

#ifdef _APPFW_FEATURE_MOUNT_INSTALL
int pkgmgr_parser_insert_mount_install_info_in_db(const char *pkgid, bool ismount, const char *tpk_name)
{
	int ret = PM_PARSER_R_OK;
	char *query = NULL;
	int is_mnt = 0;

	retvm_if(pkgid == NULL, PMINFO_R_ERROR, "pkgid is NULL.");
	retvm_if(tpk_name == NULL, PMINFO_R_ERROR, "tpk_name is NULL.");

	ret = pkgmgr_parser_check_and_create_db();
	retvm_if(ret == -1, PM_PARSER_R_ERROR, "Failed to open DB");

	ret = pkgmgr_parser_initialize_db();
	tryvm_if(ret == -1, ret = PM_PARSER_R_ERROR, "Failed to initialize DB");

	// pkgmgr parser DB transaction
	/*Begin transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "Failed to begin transaction[%d]\n", ret);

	_LOGD("Transaction Begin\n");

	if (ismount) {
		is_mnt = 1;
	}

	/* Updating mount_install value in "package_app_info" table */
	query = sqlite3_mprintf("UPDATE package_app_info "\
						"SET app_mount_install = '%d', "\
						"app_tpk_name = %Q "\
						"WHERE package = %Q", is_mnt, tpk_name, pkgid);

	ret = __exec_query(query);
	_LOGD("Query: [%s]", query);
	sqlite3_free(query);
	tryvm_if(ret != SQLITE_OK, ret = PM_PARSER_R_ERROR, "sqlite exec failed to insert entries into package_app_info!!");


	/*Commit transaction*/
	ret = sqlite3_exec(pkgmgr_parser_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGD("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkgmgr_parser_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to commit transaction. Rollback now\n");

		ret = PM_PARSER_R_ERROR;
		goto catch;
	}
	_LOGD("Transaction Commit and End\n");
	ret =  PM_PARSER_R_OK;

catch:
	pkgmgr_parser_close_db();
	return ret;
}



#endif

