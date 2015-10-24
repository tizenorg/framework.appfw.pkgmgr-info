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
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlschemas.h>
#include <vconf.h>
#include <glib.h>
#include <db-util.h>
#include <vasum.h>

#include "pkgmgr_parser.h"
#include "pkgmgr_parser_internal.h"
#include "pkgmgr_parser_db.h"
#include "pkgmgr_parser_db_util.h"
#include "pkgmgr_parser_plugin.h"

#include "pkgmgrinfo_debug.h"
#include "pkgmgrinfo_zone.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_PARSER"

#define WATCH_CLOCK_CATEGORY "org.tizen.wmanager.WATCH_CLOCK"
#define MANIFEST_RW_DIRECTORY "opt/share/packages"
#define MANIFEST_RO_DIRECTORY "usr/share/packages"
#define ASCII(s) (const char *)s
#define XMLCHAR(s) (const xmlChar *)s

const char *package;

static int __ps_process_label(xmlTextReaderPtr reader, label_x *label);
static int __ps_process_privilege(xmlTextReaderPtr reader, manifest_x *mfx);
static int __ps_process_privileges(xmlTextReaderPtr reader, manifest_x *mfx);
static int __ps_process_deviceprofile(xmlTextReaderPtr reader, deviceprofile_x *deviceprofile);
static int __ps_process_allowed(xmlTextReaderPtr reader, allowed_x *allowed);
static int __ps_process_operation(xmlTextReaderPtr reader, operation_x *operation);
static int __ps_process_uri(xmlTextReaderPtr reader, uri_x *uri);
static int __ps_process_mime(xmlTextReaderPtr reader, mime_x *mime);
static int __ps_process_subapp(xmlTextReaderPtr reader, subapp_x *subapp);
static int __ps_process_condition(xmlTextReaderPtr reader, condition_x *condition);
static int __ps_process_notification(xmlTextReaderPtr reader, notification_x *notifiation);
static int __ps_process_category(xmlTextReaderPtr reader, category_x *category);
static int __ps_process_metadata(xmlTextReaderPtr reader, metadata_x *metadata);
static int __ps_process_permission(xmlTextReaderPtr reader, permission_x *permission);
static int __ps_process_compatibility(xmlTextReaderPtr reader, compatibility_x *compatibility);
#if 0
static int __ps_process_resolution(xmlTextReaderPtr reader, resolution_x *resolution);
#endif
static int __ps_process_request(xmlTextReaderPtr reader, request_x *request);
static int __ps_process_define(xmlTextReaderPtr reader, define_x *define);
static int __ps_process_appsvc(xmlTextReaderPtr reader, appsvc_x *appsvc);
static int __ps_process_launchconditions(xmlTextReaderPtr reader, launchconditions_x *launchconditions);
static int __ps_process_datashare(xmlTextReaderPtr reader, datashare_x *datashare);
static int __ps_process_icon(xmlTextReaderPtr reader, icon_x *icon);
static int __ps_process_author(xmlTextReaderPtr reader, author_x *author);
static int __ps_process_description(xmlTextReaderPtr reader, description_x *description);
#if 0
static int __ps_process_capability(xmlTextReaderPtr reader, capability_x *capability);
#endif
static int __ps_process_license(xmlTextReaderPtr reader, license_x *license);
static int __ps_process_datacontrol(xmlTextReaderPtr reader, datacontrol_x *datacontrol);
static int __ps_process_uiapplication(xmlTextReaderPtr reader, uiapplication_x *uiapplication);
static int __ps_process_font(xmlTextReaderPtr reader, font_x *font);
static int __ps_process_theme(xmlTextReaderPtr reader, theme_x *theme);
static int __ps_process_daemon(xmlTextReaderPtr reader, daemon_x *daemon);
static int __ps_process_ime(xmlTextReaderPtr reader, ime_x *ime);
static char *__pkgid_to_manifest(const char *pkgid);
static int __next_child_element(xmlTextReaderPtr reader, int depth);
static int __start_process(xmlTextReaderPtr reader, manifest_x * mfx);
static int __process_manifest(xmlTextReaderPtr reader, manifest_x * mfx);


static int __get_patch_trimmed_api_version(const char *api_version, char **trim_api_version)
{
	char *trim_version = NULL;
	char *ptr = NULL;

	trim_version = strdup(api_version);
	if (trim_version == NULL) {
		_LOGE("out of memory");
		return -1;
	}
	ptr = strchr(trim_version, '.');
	if (ptr) {
		ptr = strchr(++ptr, '.');
	}
	if (ptr) {
		if (atoi(++ptr) == 0) {
			*(--ptr) = '\0';
			_LOGD("api_version (%s -> %s)", api_version, trim_version);
		}
	}

	*trim_api_version = trim_version;

	return 0;
}

static void __save_xml_attribute(xmlTextReaderPtr reader, char *attribute, const char **xml_attribute, char *default_value)
{
	xmlChar *attrib_val = xmlTextReaderGetAttribute(reader, XMLCHAR(attribute));
	if (attrib_val) {
		*xml_attribute = ASCII(attrib_val);
	} else {
		if (default_value != NULL) {
			*xml_attribute = strdup(default_value);
		}
	}
}

static void __save_xml_lang(xmlTextReaderPtr reader, const char **xml_attribute)
{
	const xmlChar *attrib_val = xmlTextReaderConstXmlLang(reader);
	if (attrib_val != NULL) {
		*xml_attribute = strdup(ASCII(attrib_val));
	} else {
		*xml_attribute = strdup(DEFAULT_LOCALE);
	}
}

static void __save_xml_value(xmlTextReaderPtr reader, const char **xml_attribute)
{
	xmlTextReaderRead(reader);
	xmlChar *attrib_val = xmlTextReaderValue(reader);

	if (attrib_val) {
		*xml_attribute = ASCII(attrib_val);
	}
}

static void __save_xml_support_mode(xmlTextReaderPtr reader, manifest_x * mfx)
{
	xmlChar *attrib_val = xmlTextReaderGetAttribute(reader, XMLCHAR("support-mode"));
	if (attrib_val) {
		int temp_mode = 0;
		char buffer[10] = {'\0'};

		if (strstr(ASCII(attrib_val), "ultra-power-saving")) {
			temp_mode |= PMINFO_SUPPORT_MODE_ULTRA_POWER_SAVING;	// PMINFO_MODE_PROP_ULTRA_POWER_SAVING	0x00000001
		}
		if (strstr(ASCII(attrib_val), "cool-down")) {
			temp_mode |= PMINFO_SUPPORT_MODE_COOL_DOWN;	// PMINFO_MODE_PROP_COOL_DOWN			0x00000002
		}
		if (strstr(ASCII(attrib_val), "screen-reader")) {
			temp_mode |= PMINFO_SUPPORT_MODE_SCREEN_READER;	// PMINFO_MODE_PROP_SCREEN_READER		0x00000004
		}

		snprintf(buffer,sizeof(buffer), "%d", temp_mode);
		mfx->support_mode = strdup(buffer);
		xmlFree(attrib_val);
	} else {
		mfx->support_mode = strdup("0");
	}
}

static void __save_xml_support_mode_metadata(const char *value, uiapplication_x *uiapplication)
{
	if (value) {
		char buffer[10] = {'\0'};
		int temp_mode = 0;

		if (uiapplication->support_mode)
			temp_mode = atoi(uiapplication->support_mode);

		if (strstr(value, "ultra-power-saving")) {
			temp_mode |= PMINFO_SUPPORT_MODE_ULTRA_POWER_SAVING;	// PMINFO_MODE_PROP_ULTRA_POWER_SAVING	0x00000001
		} else if (strstr(value, "cool-down")) {
			temp_mode |= PMINFO_SUPPORT_MODE_COOL_DOWN;	// PMINFO_MODE_PROP_COOL_DOWN			0x00000002
		} else if (strstr(value, "screen-reader")) {
			temp_mode |= PMINFO_SUPPORT_MODE_SCREEN_READER;	// PMINFO_MODE_PROP_SCREEN_READER		0x00000004
		} else if (strstr(value, "activation-stub")) {
			temp_mode |= PMINFO_SUPPORT_MODE_ACTIVATION_STUB; // PMINFO_MODE_ACTIVATION_STUB		0x00000008
		}

		snprintf(buffer, sizeof(buffer), "%d", temp_mode);
		if(uiapplication->support_mode)
			free((void *)uiapplication->support_mode);
		uiapplication->support_mode = strdup(buffer);
	}
}

#define METADATA_KEY_EFFECTIVE_APPID "http://tizen.org/metadata/effective-appid"
#define METADATA_KEY_PREFIX_BACKGROUND_CATEGORY "http://tizen.org/metadata/background-category/"

static void __save_xml_metadata(metadata_x *metadata, uiapplication_x *uiapplication)
{
	if (metadata && metadata->key) {
		// companian mode check
		if (strcmp(metadata->key, "master_app_packagename") == 0)
			FREE_AND_STRDUP("true", uiapplication->companion_type);
		if (strcmp(metadata->key, METADATA_KEY_EFFECTIVE_APPID) == 0) {
			if (metadata->value)
				FREE_AND_STRDUP(metadata->value, uiapplication->effective_appid);
		}
		if (strncmp(metadata->key,
			METADATA_KEY_PREFIX_BACKGROUND_CATEGORY,
			strlen(METADATA_KEY_PREFIX_BACKGROUND_CATEGORY)) == 0) {
			char *background_category;

			background_category = strrchr(metadata->key, '/');
			if (background_category && strlen(background_category + 1))
				uiapplication->background_category = g_list_append(uiapplication->background_category, strdup(background_category + 1));
		}
	}
}

static void __save_xml_support_feature(const char *metadata_key, uiapplication_x *uiapplication)
{
	if (metadata_key) {
		int temp_mode = 0;
		char buffer[PKG_STRING_LEN_MAX] = {'\0'};

		if (uiapplication->support_feature) {
			temp_mode = atoi(uiapplication->support_feature);
		}
		FREE_AND_NULL(uiapplication->support_feature);

		snprintf(buffer, PKG_STRING_LEN_MAX, "%d", temp_mode);
		uiapplication->support_feature = strdup(buffer);
	}
}

static void __save_xml_support_category(const char *category_key, uiapplication_x *uiapplication)
{
	if (category_key) {
		int temp_mode = 0;
		char buffer[PKG_STRING_LEN_MAX] = {'\0'};

		if (uiapplication->support_category) {
			temp_mode = atoi(uiapplication->support_category);
		}
		FREE_AND_NULL(uiapplication->support_category);

		if (strcmp(category_key, "http://tizen.org/category/lockapp") == 0) {
			temp_mode |= PMINFO_SUPPORT_CATEGORY_LOCK_APP;
		} else if (strcmp(category_key, "http://tizen.org/category/homeapp") == 0) {
			temp_mode |= PMINFO_SUPPORT_CATEGORY_HOME_APP;
		} else if (strcmp(category_key, "http://tizen.org/category/ime") == 0) {
			temp_mode |= PMINFO_SUPPORT_CATEGORY_IME_APP;
		}

		snprintf(buffer, PKG_STRING_LEN_MAX, "%d", temp_mode);
		uiapplication->support_category = strdup(buffer);
	}
}

static void __save_xml_installed_time(manifest_x * mfx)
{
	char buf[PKG_STRING_LEN_MAX] = {'\0'};
	char *val = NULL;
	time_t current_time;
	time(&current_time);
	snprintf(buf, PKG_STRING_LEN_MAX - 1, "%d", (int)current_time);
	val = strndup(buf, PKG_STRING_LEN_MAX - 1);
	mfx->installed_time = val;
}

static void __save_xml_root_path(manifest_x * mfx)
{
	char root[PKG_STRING_LEN_MAX] = { '\0' };

	if (mfx->root_path == NULL) {
		if (strcmp(mfx->type,"rpm") == 0) {
			snprintf(root, PKG_STRING_LEN_MAX - 1, "/opt/share/packages/%s.xml", mfx->package);
			if (access(root, F_OK) == 0) {
				memset(root, '\0', PKG_STRING_LEN_MAX);
				snprintf(root, PKG_STRING_LEN_MAX - 1, "/opt/usr/apps/%s", mfx->package);
			} else {
				memset(root, '\0', PKG_STRING_LEN_MAX);
				snprintf(root, PKG_STRING_LEN_MAX - 1, "/usr/apps/%s", mfx->package);
			}
		} else if (strcmp(mfx->type,"tpk") == 0) {
			if (strcmp(mfx->backend_installer,"coretpk") == 0) {
				snprintf(root, PKG_STRING_LEN_MAX - 1, "/opt/share/packages/%s.xml", mfx->package);
				if (access(root, F_OK) == 0) {
					memset(root, '\0', PKG_STRING_LEN_MAX);
					snprintf(root, PKG_STRING_LEN_MAX - 1, "/opt/usr/apps/%s", mfx->package);
				} else {
					memset(root, '\0', PKG_STRING_LEN_MAX);
					snprintf(root, PKG_STRING_LEN_MAX - 1, "/usr/apps/%s", mfx->package);
				}
			}
		} else {
			snprintf(root, PKG_STRING_LEN_MAX - 1, "/opt/usr/apps/%s", mfx->package);
		}

		mfx->root_path = strdup(root);
	}
}

static void __save_xml_default_value(manifest_x * mfx)
{
	mfx->preload = strdup("False");
	mfx->removable = strdup("True");
	mfx->readonly = strdup("False");
	mfx->update = strdup("False");
	mfx->system = strdup("False");
	mfx->installed_storage= strdup("installed_internal");
	package = mfx->package;
}

static char *__zone_pkgid_to_manifest(const char *pkgid, const char *zone)
{
	char *manifest;
	int size;
	char *rootpath = NULL;

	if (pkgid == NULL) {
		_LOGE("pkgid is NULL");
		return NULL;
	}

	if (zone == NULL || strlen(zone) == 0 || strcmp(zone, ZONE_HOST) == 0) {
		size = strlen(MANIFEST_RW_DIRECTORY) + strlen(pkgid) + 11;
	} else {
		rootpath = __zone_get_root_path(zone);
		if (rootpath == NULL) {
			_LOGE("Failed to get rootpath");
			return NULL;
		}

		size = strlen(rootpath) + strlen(MANIFEST_RW_DIRECTORY) + strlen(pkgid) + 11;
	}

	manifest = malloc(size);
	if (manifest == NULL) {
		_LOGE("No memory");
		return NULL;
	}
	memset(manifest, '\0', size);

	if (zone == NULL || strlen(zone) == 0 || strcmp(zone, ZONE_HOST) == 0) {
		snprintf(manifest, size, "/%s/%s.xml", MANIFEST_RW_DIRECTORY, pkgid);
	} else {
		snprintf(manifest, size, "%s%s/%s.xml", rootpath, MANIFEST_RW_DIRECTORY, pkgid);
	}

	if (access(manifest, F_OK)) {
		snprintf(manifest, size, "/%s/%s.xml", MANIFEST_RO_DIRECTORY, pkgid);
	}

	return manifest;
}

static char *__pkgid_to_manifest(const char *pkgid)
{
	return __zone_pkgid_to_manifest(pkgid, NULL);
}

static int __next_child_element(xmlTextReaderPtr reader, int depth)
{
	int ret = xmlTextReaderRead(reader);
	int cur = xmlTextReaderDepth(reader);
	while (ret == 1) {

		switch (xmlTextReaderNodeType(reader)) {
		case XML_READER_TYPE_ELEMENT:
			if (cur == depth + 1)
				return 1;
			break;
		case XML_READER_TYPE_TEXT:
			/*text is handled by each function separately*/
			if (cur == depth + 1)
				return 0;
			break;
		case XML_READER_TYPE_END_ELEMENT:
			if (cur == depth)
				return 0;
			break;
		default:
			if (cur <= depth)
				return 0;
			break;
		}
		ret = xmlTextReaderRead(reader);
		cur = xmlTextReaderDepth(reader);
	}
	return ret;
}

static int __check_action_fota(char *const tagv[])
{
	int i = 0;
	char delims[] = "=";
	char *ret_result = NULL;
	char *tag = NULL;
	int ret = PM_PARSER_R_ERROR;

	if (tagv == NULL)
		return ret;

	for (tag = strdup(tagv[0]); tag != NULL; ) {
		ret_result = strtok(tag, delims);

		/*check tag :  fota is true */
		if (ret_result && strcmp(ret_result, "fota") == 0) {
			ret_result = strtok(NULL, delims);
			if (ret_result && strcmp(ret_result, "true") == 0) {
				ret = PM_PARSER_R_OK;
			}
		} else
			_LOGD("tag process [%s]is not defined\n", ret_result);

		FREE_AND_NULL(tag);

		/*check next value*/
		if (tagv[++i] != NULL)
			tag = strdup(tagv[i]);
		else {
			_LOGD("tag process success...%d\n" , ret);
			return ret;
		}
	}

	return ret;
}

static int __ps_process_allowed(xmlTextReaderPtr reader, allowed_x *allowed)
{
	__save_xml_value(reader, &allowed->text);
	return 0;
}

static int __ps_process_operation(xmlTextReaderPtr reader, operation_x *operation)
{
	__save_xml_attribute(reader, "name", &operation->name, NULL);
/* Text does not exist. Only attribute exists
	xmlTextReaderRead(reader);
	if (xmlTextReaderValue(reader))
		operation->text = ASCII(xmlTextReaderValue(reader));
*/
	return 0;
}

static int __ps_process_uri(xmlTextReaderPtr reader, uri_x *uri)
{
	__save_xml_attribute(reader, "name", &uri->name, NULL);
/* Text does not exist. Only attribute exists
	xmlTextReaderRead(reader);
	if (xmlTextReaderValue(reader))
		uri->text = ASCII(xmlTextReaderValue(reader));
*/
	return 0;
}

static int __ps_process_mime(xmlTextReaderPtr reader, mime_x *mime)
{
	__save_xml_attribute(reader, "name", &mime->name, NULL);
/* Text does not exist. Only attribute exists
	xmlTextReaderRead(reader);
	if (xmlTextReaderValue(reader))
		mime->text = ASCII(xmlTextReaderValue(reader));
*/
	return 0;
}

static int __ps_process_subapp(xmlTextReaderPtr reader, subapp_x *subapp)
{
	__save_xml_attribute(reader, "name", &subapp->name, NULL);
/* Text does not exist. Only attribute exists
	xmlTextReaderRead(reader);
	if (xmlTextReaderValue(reader))
		mime->text = ASCII(xmlTextReaderValue(reader));
*/
	return 0;
}

static int __ps_process_condition(xmlTextReaderPtr reader, condition_x *condition)
{
	__save_xml_attribute(reader, "name", &condition->name, NULL);
	__save_xml_value(reader, &condition->text);
	return 0;
}

static int __ps_process_notification(xmlTextReaderPtr reader, notification_x *notification)
{
	__save_xml_attribute(reader, "name", &notification->name, NULL);
	__save_xml_value(reader, &notification->text);
	return 0;
}

static int __ps_process_category(xmlTextReaderPtr reader, category_x *category)
{
	__save_xml_attribute(reader, "name", &category->name, NULL);
	return 0;
}

static int __ps_process_privilege(xmlTextReaderPtr reader, manifest_x* mfx)
{
	xmlTextReaderRead(reader);
	xmlChar *attrib_val = xmlTextReaderValue(reader);
	if(attrib_val){
		mfx->privileges = g_list_append(mfx->privileges,(gpointer)(ASCII(attrib_val)));
	}
	return 0;
}

static int __ps_process_metadata(xmlTextReaderPtr reader, metadata_x *metadata)
{
	__save_xml_attribute(reader, "key", &metadata->key, NULL);
	__save_xml_attribute(reader, "value", &metadata->value, NULL);
	return 0;
}

static int __ps_process_permission(xmlTextReaderPtr reader, permission_x *permission)
{
	__save_xml_attribute(reader, "type", &permission->type, NULL);
	__save_xml_value(reader, &permission->value);

	return 0;
}

static int __ps_process_compatibility(xmlTextReaderPtr reader, compatibility_x *compatibility)
{
	__save_xml_attribute(reader, "name", &compatibility->name, NULL);
	__save_xml_value(reader, &compatibility->text);

	return 0;
}

#if 0
static int __ps_process_resolution(xmlTextReaderPtr reader, resolution_x *resolution)
{
	__save_xml_attribute(reader, "mime-type", &resolution->mimetype, NULL);
	__save_xml_attribute(reader, "uri-scheme", &resolution->urischeme, NULL);
	return 0;
}
#endif

static int __ps_process_request(xmlTextReaderPtr reader, request_x *request)
{
	__save_xml_value(reader, &request->text);

	return 0;
}

static int __ps_process_define(xmlTextReaderPtr reader, define_x *define)
{
	const xmlChar *node;
	int ret = -1;
	int depth = -1;

	__save_xml_attribute(reader, "path", &define->path, NULL);

	depth = xmlTextReaderDepth(reader);
	while ((ret = __next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			_LOGD("xmlTextReaderConstName value is NULL\n");
			return -1;
		}

		if (!strcmp(ASCII(node), "allowed")) {
			allowed_x *allowed= malloc(sizeof(allowed_x));
			if (allowed == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(allowed, '\0', sizeof(allowed_x));
			define->allowed = g_list_append(define->allowed, allowed);
			ret = __ps_process_allowed(reader, allowed);
		} else if (!strcmp(ASCII(node), "request")) {
			request_x *request = malloc(sizeof(request_x));
			if (request == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(request, '\0', sizeof(request_x));
			define->request = g_list_append(define->request, request);
			ret = __ps_process_request(reader, request);
		} else
			return -1;
		if (ret < 0) {
			_LOGD("Processing define failed\n");
			return ret;
		}
	}


	return ret;
}

static int __ps_process_appsvc(xmlTextReaderPtr reader, appsvc_x *appsvc)
{
	const xmlChar *node;
	int ret = -1;
	int depth = -1;

	depth = xmlTextReaderDepth(reader);
	while ((ret = __next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			_LOGD("xmlTextReaderConstName value is NULL\n");
			return -1;
		}

		if (!strcmp(ASCII(node), "operation")) {
			operation_x *operation = malloc(sizeof(operation_x));
			if (operation == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(operation, '\0', sizeof(operation_x));
			appsvc->operation = g_list_append(appsvc->operation, operation);
			ret = __ps_process_operation(reader, operation);
			_LOGD("operation processing\n");
		} else if (!strcmp(ASCII(node), "uri")) {
			uri_x *uri= malloc(sizeof(uri_x));
			if (uri == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(uri, '\0', sizeof(uri_x));
			appsvc->uri = g_list_append(appsvc->uri, uri);
			ret = __ps_process_uri(reader, uri);
			_LOGD("uri processing\n");
		} else if (!strcmp(ASCII(node), "mime")) {
			mime_x *mime = malloc(sizeof(mime_x));
			if (mime == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(mime, '\0', sizeof(mime_x));
			appsvc->mime = g_list_append(appsvc->mime, mime);
			ret = __ps_process_mime(reader, mime);
			_LOGD("mime processing\n");
		} else if (!strcmp(ASCII(node), "subapp")) {
			subapp_x *subapp = malloc(sizeof(subapp_x));
			if (subapp == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(subapp, '\0', sizeof(subapp_x));
			appsvc->subapp = g_list_append(appsvc->subapp, subapp);
			ret = __ps_process_subapp(reader, subapp);
			_LOGD("subapp processing\n");
		} else
			continue;
		if (ret < 0) {
			_LOGD("Processing appsvc failed\n");
			return ret;
		}
	}

	__save_xml_value(reader, &appsvc->text);

	return ret;
}


static int __ps_process_privileges(xmlTextReaderPtr reader, manifest_x* mfx)
{
	const xmlChar *node;
	int ret = -1;
	int depth = -1;

	depth = xmlTextReaderDepth(reader);
	while ((ret = __next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			_LOGD("xmlTextReaderConstName value is NULL\n");
			return -1;
		}

		if (strcmp(ASCII(node), "privilege") == 0) {
			ret = __ps_process_privilege(reader, mfx);
		} else
			return -1;
		if (ret < 0) {
			_LOGD("Processing privileges failed\n");
			return ret;
		}
	}
	return ret;
}

static int __ps_process_launchconditions(xmlTextReaderPtr reader, launchconditions_x *launchconditions)
{
	const xmlChar *node;
	int ret = -1;
	int depth = -1;

	depth = xmlTextReaderDepth(reader);
	while ((ret = __next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			_LOGD("xmlTextReaderConstName value is NULL\n");
			return -1;
		}

		if (strcmp(ASCII(node), "condition") == 0) {
			condition_x *condition = malloc(sizeof(condition_x));
			if (condition == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(condition, '\0', sizeof(condition_x));
			launchconditions->condition = g_list_append(launchconditions->condition, condition);
			ret = __ps_process_condition(reader, condition);
		} else
			return -1;
		if (ret < 0) {
			_LOGD("Processing launchconditions failed\n");
			return ret;
		}
	}

	__save_xml_value(reader, &launchconditions->text);

	return ret;
}

static int __ps_process_datashare(xmlTextReaderPtr reader, datashare_x *datashare)
{
	const xmlChar *node;
	int ret = -1;
	int depth = -1;
	depth = xmlTextReaderDepth(reader);
	while ((ret = __next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			_LOGD("xmlTextReaderConstName value is NULL\n");
			return -1;
		}

		if (!strcmp(ASCII(node), "define")) {
			define_x *define= malloc(sizeof(define_x));
			if (define == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(define, '\0', sizeof(define_x));
			datashare->define = g_list_append(datashare->define, define);
			ret = __ps_process_define(reader, define);
		} else if (!strcmp(ASCII(node), "request")) {
			request_x *request= malloc(sizeof(request_x));
			if (request == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(request, '\0', sizeof(request_x));
			datashare->request = g_list_append(datashare->request, request);
			ret = __ps_process_request(reader, request);
		} else
			return -1;
		if (ret < 0) {
			_LOGD("Processing data-share failed\n");
			return ret;
		}
	}


	return ret;
}

static char*
__get_icon_with_path(const char* icon)
{
	if (!icon)
		return NULL;

	if (index(icon, '/') == NULL) {
		char* theme = NULL;
		char* icon_with_path = NULL;
		int len;

		if (!package)
			return NULL;

		theme = strdup("default");
		if(theme == NULL){
			_LOGE("@malloc failed!!!");
			return NULL;
		}

		len = (0x01 << 7) + strlen(icon) + strlen(package) + strlen(theme);
		icon_with_path = malloc(len);
		if(icon_with_path == NULL) {
			_LOGD("(icon_with_path == NULL) return\n");
			FREE_AND_NULL(theme);
			return NULL;
		}

		memset(icon_with_path, 0, len);
		snprintf(icon_with_path, len, "/opt/usr/apps/%s/shared/res/icons/default/small/%s", package, icon);
		do {
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/opt/usr/apps/%s/shared/res/%s", package, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/opt/share/icons/%s/small/%s", theme, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/usr/share/icons/%s/small/%s", theme, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len,"/opt/share/icons/default/small/%s", icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/usr/share/icons/default/small/%s", icon);
			if (access(icon_with_path, R_OK) == 0) break;

			/* icon path is going to be moved intto the app directory */
			snprintf(icon_with_path, len, "/usr/apps/%s/shared/res/%s", package, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/usr/apps/%s/shared/res/icons/default/small/%s", package, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/opt/apps/%s/res/icons/%s/small/%s", package, theme, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/usr/apps/%s/res/icons/%s/small/%s", package, theme, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/opt/apps/%s/res/icons/default/small/%s", package, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/usr/apps/%s/res/icons/default/small/%s", package, icon);
			if (access(icon_with_path, R_OK) == 0) break;
			snprintf(icon_with_path, len, "/usr/ug/res/images/%s/%s", package, icon);
			if (access(icon_with_path, R_OK) == 0) break;
		} while (0);

		FREE_AND_NULL(theme);

		/* If icon is not exist, return NULL */
		if (access(icon_with_path, R_OK) != 0) {
			_LOGE("Icon is not exist");
			FREE_AND_NULL(icon_with_path);
			return NULL;
		} else {
			_LOGD("Icon path : %s ---> %s", icon, icon_with_path);
			return icon_with_path;
		}
	} else {
		char* confirmed_icon = NULL;

		confirmed_icon = strdup(icon);
		if (!confirmed_icon)
			return NULL;
		return confirmed_icon;
	}
}

static void __ps_process_tag(manifest_x * mfx, char *const tagv[])
{
	int i = 0;
	char delims[] = "=";
	char *ret_result = NULL;
	char *tag = NULL;

	if (tagv == NULL)
		return;

	for (tag = strdup(tagv[0]); tag != NULL; ) {
		ret_result = strtok(tag, delims);

		/*check tag :  preload */
		if (ret_result && strcmp(ret_result, "preload") == 0) {
			ret_result = strtok(NULL, delims);
			if (ret_result && strcmp(ret_result, "true") == 0) {
				FREE_AND_NULL(mfx->preload);
				mfx->preload = strdup("true");
			} else if (ret_result && strcmp(ret_result, "false") == 0) {
				FREE_AND_NULL(mfx->preload);
				mfx->preload = strdup("false");
			}
			/*check tag :  removable*/
		} else if (ret_result && strcmp(ret_result, "removable") == 0) {
			ret_result = strtok(NULL, delims);
			if (ret_result && strcmp(ret_result, "true") == 0){
				FREE_AND_NULL(mfx->removable);
				mfx->removable = strdup("true");
			} else if (ret_result && strcmp(ret_result, "false") == 0) {
				FREE_AND_NULL(mfx->removable);
				mfx->removable = strdup("false");
			}
			/*check tag :  not matched*/
		} else
			_LOGD("tag process [%s]is not defined\n", ret_result);

		free(tag);

		/*check next value*/
		if (tagv[++i] != NULL)
			tag = strdup(tagv[i]);
		else {
			/*update tag :	system*/
			if (mfx->preload && mfx->removable) {
				if ((strcmp(mfx->preload, "true")==0) && (strcmp(mfx->removable, "false")==0)){
					FREE_AND_NULL(mfx->system);
					mfx->system = strdup("true");
				}
			}
			_LOGD("tag process success...\n");
			return;
		}
	}
}

static int __ps_process_icon(xmlTextReaderPtr reader, icon_x *icon)
{
	__save_xml_attribute(reader, "name", &icon->name, NULL);
	__save_xml_attribute(reader, "section", &icon->section, NULL);
	__save_xml_attribute(reader, "size", &icon->size, NULL);
	__save_xml_attribute(reader, "resolution", &icon->resolution, NULL);
	__save_xml_lang(reader, &icon->lang);

	xmlTextReaderRead(reader);
	const char *text  = ASCII(xmlTextReaderValue(reader));
	if(text) {
		icon->text = (const char *)__get_icon_with_path(text);
		FREE_AND_NULL(text);
	}

	return 0;
}

static int __ps_process_image(xmlTextReaderPtr reader, image_x *image)
{
	__save_xml_attribute(reader, "name", &image->name, NULL);
	__save_xml_attribute(reader, "section", &image->section, NULL);
	__save_xml_lang(reader, &image->lang);
	__save_xml_value(reader, &image->text);

	return 0;
}

static int __ps_process_label(xmlTextReaderPtr reader, label_x *label)
{
	__save_xml_attribute(reader, "name", &label->name, NULL);
	__save_xml_lang(reader, &label->lang);
	__save_xml_value(reader, &label->text);

/*	_LOGD("lable name %s\n", label->name);
	_LOGD("lable lang %s\n", label->lang);
	_LOGD("lable text %s\n", label->text);
*/
	return 0;

}

static int __ps_process_author(xmlTextReaderPtr reader, author_x *author)
{
	__save_xml_attribute(reader, "email", &author->email, NULL);
	__save_xml_attribute(reader, "href", &author->href, NULL);
	__save_xml_lang(reader, &author->lang);
	__save_xml_value(reader, &author->text);

	return 0;
}

static int __ps_process_description(xmlTextReaderPtr reader, description_x *description)
{
	__save_xml_lang(reader, &description->lang);
	__save_xml_value(reader, &description->text);

	return 0;
}

static int __ps_process_license(xmlTextReaderPtr reader, license_x *license)
{
	__save_xml_lang(reader, &license->lang);
	__save_xml_value(reader, &license->text);

	return 0;
}

#if 0
//Function is not used anywhere
static int __ps_process_capability(xmlTextReaderPtr reader, capability_x *capability)
{
	const xmlChar *node;
	int ret = -1;
	int depth = -1;

	__save_xml_attribute(reader, "operation-id", &capability->operationid, NULL);

	depth = xmlTextReaderDepth(reader);
	while ((ret = __next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			_LOGD("xmlTextReaderConstName value is NULL\n");
			return -1;
		}

		if (!strcmp(ASCII(node), "resolution")) {
			resolution_x *resolution = malloc(sizeof(resolution_x));
			if (resolution == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(resolution, '\0', sizeof(resolution_x));
			capability->resolution = g_list_append(capability->resolution, resolution);
			ret = __ps_process_resolution(reader, resolution);
		} else
			return -1;
		if (ret < 0) {
			_LOGD("Processing capability failed\n");
			return ret;
		}
	}

	return ret;
}
#endif

static int __ps_process_datacontrol(xmlTextReaderPtr reader, datacontrol_x *datacontrol)
{
	__save_xml_attribute(reader, "providerid", &datacontrol->providerid, NULL);
	__save_xml_attribute(reader, "access", &datacontrol->access, NULL);
	__save_xml_attribute(reader, "type", &datacontrol->type, NULL);

	return 0;
}

static int __ps_process_uiapplication(xmlTextReaderPtr reader, uiapplication_x *uiapplication)
{
	const xmlChar *node;
	int ret = -1;
	int depth = -1;

	__save_xml_attribute(reader, "appid", &uiapplication->appid, NULL);
	retvm_if(uiapplication->appid == NULL, PM_PARSER_R_ERROR, "appid cant be NULL, appid field is mandatory\n");
	__save_xml_attribute(reader, "exec", &uiapplication->exec, NULL);
	__save_xml_attribute(reader, "nodisplay", &uiapplication->nodisplay, "false");
	__save_xml_attribute(reader, "multiple", &uiapplication->multiple, "false");
	__save_xml_attribute(reader, "type", &uiapplication->type, NULL);
	__save_xml_attribute(reader, "categories", &uiapplication->categories, NULL);
	__save_xml_attribute(reader, "extraid", &uiapplication->extraid, NULL);
	__save_xml_attribute(reader, "taskmanage", &uiapplication->taskmanage, "true");
	__save_xml_attribute(reader, "enabled", &uiapplication->enabled, "true");
	__save_xml_attribute(reader, "hw-acceleration", &uiapplication->hwacceleration, "default");
	__save_xml_attribute(reader, "screen-reader", &uiapplication->screenreader, "use-system-setting");
	__save_xml_attribute(reader, "mainapp", &uiapplication->mainapp, "false");
	__save_xml_attribute(reader, "recentimage", &uiapplication->recentimage, "false");
	__save_xml_attribute(reader, "launchcondition", &uiapplication->launchcondition, "false");
	__save_xml_attribute(reader, "indicatordisplay", &uiapplication->indicatordisplay, "true");
	__save_xml_attribute(reader, "portrait-effectimage", &uiapplication->portraitimg, NULL);
	__save_xml_attribute(reader, "landscape-effectimage", &uiapplication->landscapeimg, NULL);
	__save_xml_attribute(reader, "effectimage-type", &uiapplication->effectimage_type, "image");
	__save_xml_attribute(reader, "guestmode-visibility", &uiapplication->guestmode_visibility, "true");
	__save_xml_attribute(reader, "permission-type", &uiapplication->permission_type, "normal");
	__save_xml_attribute(reader, "component-type", &uiapplication->component_type, "uiapp");
	/*component_type has "svcapp" or "uiapp", if it is not, parsing manifest is fail*/
	retvm_if(((strcmp(uiapplication->component_type, "svcapp") != 0) && (strcmp(uiapplication->component_type, "uiapp") != 0) && (strcmp(uiapplication->component_type, "widgetapp") != 0)), PM_PARSER_R_ERROR, "invalid component_type[%s]\n", uiapplication->component_type);
	__save_xml_attribute(reader, "submode", &uiapplication->submode, "false");
	__save_xml_attribute(reader, "submode-mainid", &uiapplication->submode_mainid, NULL);
	__save_xml_attribute(reader, "process-pool", &uiapplication->process_pool, "false");
	__save_xml_attribute(reader, "auto-restart", &uiapplication->autorestart, "false");
	__save_xml_attribute(reader, "on-boot", &uiapplication->onboot, "false");
	__save_xml_attribute(reader, "multi-instance", &uiapplication->multi_instance, "false");
	__save_xml_attribute(reader, "multi-instance-mainid", &uiapplication->multi_instance_mainid, NULL);
	__save_xml_attribute(reader, "ui-gadget", &uiapplication->ui_gadget, "false");
	__save_xml_attribute(reader, "launch_mode", &uiapplication->launch_mode, "single");

	//set defaut value
	uiapplication->multi_window = strdup("false");
	uiapplication->companion_type = strdup("false");
	uiapplication->package= strdup(package);
	uiapplication->support_feature = strdup("0");
	uiapplication->support_category = strdup("0");
	uiapplication->support_mode = strdup("0");

	/*hw-acceleration values are changed from use-GL/not-use-GL/use-system-setting to on/off/default*/
	if (strcmp(uiapplication->hwacceleration, "use-GL") == 0) {
		free((void *)uiapplication->hwacceleration);
		uiapplication->hwacceleration = strdup("on");
	} else if (strcmp(uiapplication->hwacceleration, "not-use-GL") == 0) {
		free((void *)uiapplication->hwacceleration);
		uiapplication->hwacceleration = strdup("off");
	} else if (strcmp(uiapplication->hwacceleration, "use-system-setting") == 0) {
		free((void *)uiapplication->hwacceleration);
		uiapplication->hwacceleration = strdup("default");
	}

	depth = xmlTextReaderDepth(reader);
	while ((ret = __next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			_LOGD("xmlTextReaderConstName value is NULL\n");
			return -1;
		}
		if (!strcmp(ASCII(node), "label")) {
			label_x *label = malloc(sizeof(label_x));
			if (label == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(label, '\0', sizeof(label_x));
			ret = __ps_process_label(reader, label);
			uiapplication->label = g_list_append(uiapplication->label, label);
		} else if (!strcmp(ASCII(node), "icon")) {
			icon_x *icon = malloc(sizeof(icon_x));
			if (icon == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(icon, '\0', sizeof(icon_x));
			uiapplication->icon = g_list_append(uiapplication->icon, icon);
			ret = __ps_process_icon(reader, icon);
		} else if (!strcmp(ASCII(node), "image")) {
			image_x *image = malloc(sizeof(image_x));
			if (image == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(image, '\0', sizeof(image_x));
			uiapplication->image = g_list_append(uiapplication->image, image);
			ret = __ps_process_image(reader, image);
		} else if (!strcmp(ASCII(node), "category")) {
			category_x *category = malloc(sizeof(category_x));
			if (category == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(category, '\0', sizeof(category_x));
			uiapplication->category= g_list_append(uiapplication->category, category);
			ret = __ps_process_category(reader, category);
			__save_xml_support_category(category->name, uiapplication);
		} else if (!strcmp(ASCII(node), "metadata")) {
			metadata_x *metadata = malloc(sizeof(metadata_x));
			if (metadata == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(metadata, '\0', sizeof(metadata_x));
			uiapplication->metadata = g_list_append(uiapplication->metadata, metadata);
			ret = __ps_process_metadata(reader, metadata);

			__save_xml_metadata(metadata, uiapplication);
			__save_xml_support_feature(metadata->key, uiapplication);
		} else if (!strcmp(ASCII(node), "permission")) {
			permission_x *permission = malloc(sizeof(permission_x));
			if (permission == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(permission, '\0', sizeof(permission_x));
			uiapplication->permission = g_list_append(uiapplication->permission, permission);
			ret = __ps_process_permission(reader, permission);
		} else if (!strcmp(ASCII(node), "application-service")) {
			appsvc_x *appsvc = malloc(sizeof(appsvc_x));
			if (appsvc == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(appsvc, '\0', sizeof(appsvc_x));
			uiapplication->appsvc = g_list_append(uiapplication->appsvc, appsvc);
			ret = __ps_process_appsvc(reader, appsvc);
		} else if (!strcmp(ASCII(node), "app-control")) {
			appsvc_x *appsvc = malloc(sizeof(appsvc_x));
			if (appsvc == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(appsvc, '\0', sizeof(appsvc_x));
			uiapplication->appsvc = g_list_append(uiapplication->appsvc, appsvc);
			ret = __ps_process_appsvc(reader, appsvc);
		} else if (!strcmp(ASCII(node), "data-share")) {
			datashare_x *datashare = malloc(sizeof(datashare_x));
			if (datashare == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(datashare, '\0', sizeof(datashare_x));
			uiapplication->datashare = g_list_append(uiapplication->datashare, datashare);
			ret = __ps_process_datashare(reader, datashare);
		} else if (!strcmp(ASCII(node), "launch-conditions")) {
			launchconditions_x *launchconditions = malloc(sizeof(launchconditions_x));
			if (launchconditions == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(launchconditions, '\0', sizeof(launchconditions_x));
			uiapplication->launchconditions = g_list_append(uiapplication->launchconditions, launchconditions);
			ret = __ps_process_launchconditions(reader, launchconditions);
		} else if (!strcmp(ASCII(node), "notification")) {
			notification_x *notification = malloc(sizeof(notification_x));
			if (notification == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(notification, '\0', sizeof(notification_x));
			uiapplication->notification = g_list_append(uiapplication->notification, notification);
			ret = __ps_process_notification(reader, notification);
		} else if (!strcmp(ASCII(node), "datacontrol")) {
			datacontrol_x *datacontrol = malloc(sizeof(datacontrol_x));
			if (datacontrol == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(datacontrol, '\0', sizeof(datacontrol_x));
			uiapplication->datacontrol = g_list_append(uiapplication->datacontrol, datacontrol);
			ret = __ps_process_datacontrol(reader, datacontrol);
		} else if (!strcmp(ASCII(node), "background-category")) {
			const char *background_category = NULL;
			__save_xml_attribute(reader, "value", &background_category, NULL);
			if (background_category != NULL)
				uiapplication->background_category = g_list_append(uiapplication->background_category, (gpointer)background_category);
		} else if (!strcmp(ASCII(node), "support-size")) {
			/*temporary workaround code for widget-application. */
			FREE_AND_STRDUP("true", uiapplication->nodisplay);
			FREE_AND_STRDUP("false", uiapplication->multiple);
			FREE_AND_STRDUP("capp", uiapplication->type);
			FREE_AND_STRDUP("false", uiapplication->taskmanage);
			FREE_AND_STRDUP("false", uiapplication->indicatordisplay);
			FREE_AND_STRDUP("widgetapp", uiapplication->component_type);
		} else
			_LOGD("not handling node[%s]", ASCII(node));

		if (ret < 0) {
			_LOGD("Processing uiapplication failed\n");
			return ret;
		}
	}

	return ret;
}

static int __ps_process_deviceprofile(xmlTextReaderPtr reader, deviceprofile_x *deviceprofile)
{
	/*TODO: once policy is set*/
	__save_xml_attribute(reader, "name", &deviceprofile->name, NULL);
	return 0;
}

static int __ps_process_font(xmlTextReaderPtr reader, font_x *font)
{
	/*TODO: once policy is set*/
	return 0;
}

static int __ps_process_theme(xmlTextReaderPtr reader, theme_x *theme)
{
	/*TODO: once policy is set*/
	return 0;
}

static int __ps_process_daemon(xmlTextReaderPtr reader, daemon_x *daemon)
{
	/*TODO: once policy is set*/
	return 0;
}

static int __ps_process_ime(xmlTextReaderPtr reader, ime_x *ime)
{
	/*TODO: once policy is set*/
	return 0;
}

static int __start_process(xmlTextReaderPtr reader, manifest_x * mfx)
{
	_LOGD("__start_process\n");
	const xmlChar *node;
	int ret = -1;
	int depth = -1;

	depth = xmlTextReaderDepth(reader);
	while ((ret = __next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			_LOGD("xmlTextReaderConstName value is NULL\n");
			return -1;
		}

		if (!strcmp(ASCII(node), "label")) {
			label_x *label = malloc(sizeof(label_x));
			if (label == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(label, '\0', sizeof(label_x));
			mfx->label = g_list_append(mfx->label,label);
			ret = __ps_process_label(reader, label);
		} else if (!strcmp(ASCII(node), "author")) {
			author_x *author = malloc(sizeof(author_x));
			if (author == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(author, '\0', sizeof(author_x));
			mfx->author = g_list_append(mfx->author, author);
			ret = __ps_process_author(reader, author);
		} else if (!strcmp(ASCII(node), "description")) {
			description_x *description = malloc(sizeof(description_x));
			if (description == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(description, '\0', sizeof(description_x));
			mfx->description = g_list_append(mfx->description, description);
			ret = __ps_process_description(reader, description);
		} else if (!strcmp(ASCII(node), "license")) {
			license_x *license = malloc(sizeof(license_x));
			if (license == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(license, '\0', sizeof(license_x));
			ret = __ps_process_license(reader, license);
			mfx->license = g_list_append(mfx->license, license);
		} else if (!strcmp(ASCII(node), "privileges")) {
			ret = __ps_process_privileges(reader, mfx);
		} else if (!strcmp(ASCII(node), "ui-application")) {
			uiapplication_x *uiapplication = malloc(sizeof(uiapplication_x));
			if (uiapplication == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(uiapplication, '\0', sizeof(uiapplication_x));
			mfx->uiapplication = g_list_append(mfx->uiapplication, uiapplication);
			ret = __ps_process_uiapplication(reader, uiapplication);
		} else if (!strcmp(ASCII(node), "service-application")) {
			uiapplication_x *uiapplication = malloc(sizeof(uiapplication_x));
			if (uiapplication == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(uiapplication, '\0', sizeof(uiapplication_x));
			mfx->uiapplication = g_list_append(mfx->uiapplication, uiapplication);
			ret = __ps_process_uiapplication(reader, uiapplication);
			FREE_AND_STRDUP("svcapp", uiapplication->component_type);
		} else if (!strcmp(ASCII(node), "widget-application")) {
			_LOGD("handling widget-application");
			uiapplication_x *uiapplication = malloc(sizeof(uiapplication_x));
			if (uiapplication == NULL) {
				_LOGE("Malloc Failed\n");
				return -1;
			}
			memset(uiapplication, '\0', sizeof(uiapplication_x));
			mfx->uiapplication = g_list_append(mfx->uiapplication, uiapplication);
			ret = __ps_process_uiapplication(reader, uiapplication);
		} else if (!strcmp(ASCII(node), "watch-application")) {
			uiapplication_x *uiapplication = malloc(sizeof(uiapplication_x));
			if (uiapplication == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(uiapplication, '\0', sizeof(uiapplication_x));
			mfx->uiapplication = g_list_append(mfx->uiapplication, uiapplication);
			ret = __ps_process_uiapplication(reader, uiapplication);
			FREE_AND_STRDUP("watchapp", uiapplication->component_type);
			FREE_AND_STRDUP("true", uiapplication->nodisplay);
			FREE_AND_STRDUP("false", uiapplication->taskmanage);
			if (uiapplication->type == NULL)
				FREE_AND_STRDUP("capp", uiapplication->type);
			category_x *category = malloc(sizeof(category_x));
			if (category == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(category, '\0', sizeof(category_x));
			uiapplication->category = g_list_append(uiapplication->category, category);
			category->name = strdup(WATCH_CLOCK_CATEGORY);
		} else if (!strcmp(ASCII(node), "daemon")) {
			daemon_x *daemon = malloc(sizeof(daemon_x));
			if (daemon == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(daemon, '\0', sizeof(daemon_x));
			ret = __ps_process_daemon(reader, daemon);
			mfx->daemon = g_list_append(mfx->daemon,daemon);
		} else if (!strcmp(ASCII(node), "theme")) {
			theme_x *theme = malloc(sizeof(theme_x));
			if (theme == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(theme, '\0', sizeof(theme_x));
			ret = __ps_process_theme(reader, theme);
			mfx->theme = g_list_append(mfx->theme,theme);
		} else if (!strcmp(ASCII(node), "font")) {
			font_x *font = malloc(sizeof(font_x));
			if (font == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(font, '\0', sizeof(font_x));
			ret = __ps_process_font(reader, font);
			mfx->font = g_list_append(mfx->font,font);
		} else if (!strcmp(ASCII(node), "ime")) {
			ime_x *ime = malloc(sizeof(ime_x));
			if (ime == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(ime, '\0', sizeof(ime_x));
			ret = __ps_process_ime(reader, ime);
			mfx->ime = g_list_append(mfx->ime,ime);
		} else if (!strcmp(ASCII(node), "icon")) {
			icon_x *icon = malloc(sizeof(icon_x));
			if (icon == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(icon, '\0', sizeof(icon_x));
			mfx->icon = g_list_append(mfx->icon, icon);
			ret = __ps_process_icon(reader, icon);
		} else if (!strcmp(ASCII(node), "profile")) {
			deviceprofile_x *deviceprofile = malloc(sizeof(deviceprofile_x));
			if (deviceprofile == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(deviceprofile, '\0', sizeof(deviceprofile_x));
			ret = __ps_process_deviceprofile(reader, deviceprofile);
			mfx->deviceprofile = g_list_append(mfx->deviceprofile,deviceprofile);
		} else if (!strcmp(ASCII(node), "compatibility")) {
			compatibility_x *compatibility = malloc(sizeof(compatibility_x));
			if (compatibility == NULL) {
				_LOGD("Malloc Failed\n");
				return -1;
			}
			memset(compatibility, '\0', sizeof(compatibility_x));
			ret = __ps_process_compatibility(reader, compatibility);
			mfx->compatibility = g_list_append(mfx->compatibility,compatibility);
		} else if (!strcmp(ASCII(node), "shortcut-list")) {
			continue;
		} else if (!strcmp(ASCII(node), "livebox")) {	/**< @todo will be deprecated soon */
			continue;
		} else if (!strcmp(ASCII(node), "widget")) {
			continue;
		} else if (!strcmp(ASCII(node), "account")) {
			continue;
		} else if (!strcmp(ASCII(node), "notifications")) {
			continue;
		} else if (!strcmp(ASCII(node), "ime")) {
			continue;
		} else if (!strcmp(ASCII(node), "watchface")) {
			continue;
		} else
			continue;

		if (ret < 0) {
			_LOGD("Processing manifest failed\n");
			return ret;
		}
	}

	return ret;
}

static int __process_manifest(xmlTextReaderPtr reader, manifest_x * mfx)
{
	const xmlChar *node;
	int ret = -1;
	char* default_version = NULL;

	if ((ret = __next_child_element(reader, -1))) {
		node = xmlTextReaderConstName(reader);
		retvm_if(!node, PM_PARSER_R_ERROR, "xmlTextReaderConstName value is NULL\n");

		if (!strcmp(ASCII(node), "manifest")) {
			if (__get_patch_trimmed_api_version(TIZEN_FULL_VERSION, &default_version) < 0) {
				_LOGE("failed to get patch-trimmed api-version");
				return -1;
			}

			__save_xml_attribute(reader, "xmlns", &mfx->ns, NULL);
			__save_xml_attribute(reader, "package", &mfx->package, NULL);
			tryvm_if(mfx->package == NULL, PM_PARSER_R_ERROR, "package cant be NULL, package field is mandatory\n");
			__save_xml_attribute(reader, "version", &mfx->version, NULL);
			__save_xml_attribute(reader, "size", &mfx->package_size, NULL);
			__save_xml_attribute(reader, "install-location", &mfx->installlocation, "internal-only");
			__save_xml_attribute(reader, "type", &mfx->type, "rpm");
			__save_xml_attribute(reader, "root_path", &mfx->root_path, NULL);
			__save_xml_attribute(reader, "csc_path", &mfx->csc_path, NULL);
			__save_xml_attribute(reader, "appsetting", &mfx->appsetting, "false");
			__save_xml_attribute(reader, "storeclient-id", &mfx->storeclient_id, NULL);
			__save_xml_attribute(reader, "nodisplay-setting", &mfx->nodisplay_setting, "false");
			__save_xml_attribute(reader, "url", &mfx->package_url, NULL);
			__save_xml_attribute(reader, "support-disable", &mfx->support_disable, "false");
			__save_xml_attribute(reader, "mother-package", &mfx->mother_package, "false");
			__save_xml_attribute(reader, "api-version", &mfx->api_version, default_version);
			__save_xml_attribute(reader, "backend-installer", &mfx->backend_installer, NULL);
			__save_xml_attribute(reader, "smack-label", &mfx->custom_smack_label, NULL);

			__save_xml_support_mode(reader, mfx);
			__save_xml_installed_time(mfx);
			__save_xml_root_path(mfx);
			__save_xml_default_value(mfx);

			FREE_AND_NULL(default_version);
			ret = __start_process(reader, mfx);
		} else {
			_LOGD("No Manifest element found\n");
			return -1;
		}
	}
	return ret;

catch:
	FREE_AND_NULL(default_version);
	return ret;
}


static char* __convert_to_system_locale(const char *mlocale)
{
	if (mlocale == NULL)
		return NULL;
	char *locale = NULL;
	locale = (char *)calloc(1, 6);
	if (!locale) {
		_LOGE("Malloc Failed\n");
		return NULL;
	}

	strncpy(locale, mlocale, 2);
	strncat(locale, "_", 1);
	locale[3] = toupper(mlocale[3]);
	locale[4] = toupper(mlocale[4]);
	return locale;
}

/* desktop shoud be generated automatically based on manifest */
/* Currently removable, taskmanage, etc fields are not considerd. it will be decided soon.*/
static int __ps_make_nativeapp_desktop(manifest_x * mfx, const char *manifest, ACTION_TYPE action)
{
        FILE* file = NULL;
        int fd = 0;
        char filepath[PKG_STRING_LEN_MAX] = "";
        char *buf = NULL;
	char *buftemp = NULL;
	char *locale = NULL;
	GList *ui_label = NULL;
	label_x* label = NULL;
	uiapplication_x *up = NULL;
	GList *list_up = NULL;

	buf = (char *)calloc(1, BUFMAX);
	if (!buf) {
		_LOGE("Malloc Failed\n");
		return -1;
	}

	buftemp = (char *)calloc(1, BUFMAX);
	if (!buftemp) {
		_LOGE("Malloc Failed\n");
		FREE_AND_NULL(buf);
		return -1;
	}


	for(list_up = mfx->uiapplication; list_up; list_up = list_up->next) {
		up = (uiapplication_x *)list_up->data;
		if(mfx->readonly && !strcasecmp(mfx->readonly, "True"))
			snprintf(filepath, sizeof(filepath),"%s%s.desktop", DESKTOP_RO_PATH, up->appid);
		else
			snprintf(filepath, sizeof(filepath),"%s%s.desktop", DESKTOP_RW_PATH, up->appid);

		/* skip if desktop exists
		if (access(filepath, R_OK) == 0)
			continue;
		*/

	        file = fopen(filepath, "w");
	        if(file == NULL)
	        {
	            _LOGS("Can't open %s", filepath);
	            FREE_AND_NULL(buf);
	            FREE_AND_NULL(buftemp);
	            return -1;
	        }

	        snprintf(buf, BUFMAX, "[Desktop Entry]\n");
	        fwrite(buf, 1, strlen(buf), file);
		ui_label = up->label;
		for( ; ui_label ; ui_label = ui_label->next) {
			label = (label_x*)ui_label->data;
			if(label){
				if(!strcmp(label->lang, DEFAULT_LOCALE)) {
					snprintf(buf, BUFMAX, "Name=%s\n",label->text);
				} else {
					locale = __convert_to_system_locale(label->lang);
					snprintf(buf, BUFMAX, "Name[%s]=%s\n", locale,
					label->text);
					FREE_AND_NULL(locale);
				}
			}
	        	fwrite(buf, 1, strlen(buf), file);
		}

/*
		else if(mfx->label && mfx->label->text) {
			snprintf(buf, BUFMAX, "Name=%s\n", mfx->label->text);
	        	fwrite(buf, 1, strlen(buf), file);
		} else {
			snprintf(buf, BUFMAX, "Name=%s\n", mfx->package);
			fwrite(buf, 1, strlen(buf), file);
		}
*/


	        snprintf(buf, BUFMAX, "Type=Application\n");
	        fwrite(buf, 1, strlen(buf), file);

		if(up->exec) {
		        snprintf(buf, BUFMAX, "Exec=%s\n", up->exec);
		        fwrite(buf, 1, strlen(buf), file);
		}

		if(up->icon && (char*)((icon_x*)up->icon->data)->text) {
		        snprintf(buf, BUFMAX, "Icon=%s\n", (char*)((icon_x*)up->icon->data)->text);
		        fwrite(buf, 1, strlen(buf), file);
		} else if(mfx->icon && (char*)((icon_x*)mfx->icon->data)->text) {
		        snprintf(buf, BUFMAX, "Icon=%s\n", (char*)((icon_x*)mfx->icon->data)->text);
		        fwrite(buf, 1, strlen(buf), file);
		}

		// MIME types
		if(up && up->appsvc) {
			appsvc_x *asvc = NULL;
			GList *list_asvc = up->appsvc;
			mime_x *mi = NULL;
			GList *list_mime = NULL;
			const char *mime = NULL;
			const char *mime_delim = "; ";
			int mime_count = 0;

			strncpy(buf, "MimeType=", BUFMAX-1);
			while (list_asvc) {
				asvc = (appsvc_x *)list_asvc->data;
				list_mime = asvc->mime;
				while (list_mime) {
					mime_count++;
					mi = (mime_x *)list_mime->data;
					if(mi && mi->name) {
						mime = mi->name;
						_LOGD("MIME type: %s\n", mime);
						strncat(buf, mime, BUFMAX-strlen(buf)-1);
					}
					if(list_mime->next) {
						strncat(buf, mime_delim, BUFMAX-strlen(buf)-1);
					}

					list_mime = list_mime->next;
					mime = NULL;
				}
				list_asvc = list_asvc->next;
			}
			_LOGD("MIME types: buf[%s]\n", buf);
			_LOGD("MIME count: %d\n", mime_count);
			if(mime_count)
				fwrite(buf, 1, strlen(buf), file);
		}

		if(mfx->version) {
		        snprintf(buf, BUFMAX, "Version=%s\n", mfx->version);
		        fwrite(buf, 1, strlen(buf), file);
		}

		if(up->nodisplay) {
			snprintf(buf, BUFMAX, "NoDisplay=%s\n", up->nodisplay);
			fwrite(buf, 1, strlen(buf), file);
		}

		if(up->categories) {
			snprintf(buf, BUFMAX, "Categories=%s\n", up->categories);
			fwrite(buf, 1, strlen(buf), file);
		}

		if(up->taskmanage && !strcasecmp(up->taskmanage, "False")) {
		        snprintf(buf, BUFMAX, "X-TIZEN-TaskManage=False\n");
		        fwrite(buf, 1, strlen(buf), file);
		}

		if(up->enabled && !strcasecmp(up->enabled, "False")) {
		        snprintf(buf, BUFMAX, "X-TIZEN-Enabled=False\n");
		        fwrite(buf, 1, strlen(buf), file);
		}

		if(up->hwacceleration) {
			snprintf(buf, BUFMAX, "Hw-Acceleration=%s\n", up->hwacceleration);
			fwrite(buf, 1, strlen(buf), file);
		}

		if(up->multiple && !strcasecmp(up->multiple, "True")) {
			snprintf(buf, BUFMAX, "X-TIZEN-Multiple=True\n");
			fwrite(buf, 1, strlen(buf), file);
		}

		if(up->extraid) {
			snprintf(buf, BUFMAX, "X-TIZEN-PackageID=%s\n", up->extraid);
			fwrite(buf, 1, strlen(buf), file);
		}

		if(mfx->removable && !strcasecmp(mfx->removable, "False")) {
			snprintf(buf, BUFMAX, "X-TIZEN-Removable=False\n");
			fwrite(buf, 1, strlen(buf), file);
		}

		if(mfx->type) {
			snprintf(buf, BUFMAX, "X-TIZEN-PackageType=%s\n", mfx->type);
			fwrite(buf, 1, strlen(buf), file);
		}

		if(up->submode && !strcasecmp(up->submode, "True")) {
			snprintf(buf, BUFMAX, "X-TIZEN-Submode=%s\n", up->submode);
			fwrite(buf, 1, strlen(buf), file);
			snprintf(buf, BUFMAX, "X-TIZEN-SubmodeMainid=%s\n", up->submode_mainid);
			fwrite(buf, 1, strlen(buf), file);
		}

		snprintf(buf, BUFMAX, "X-TIZEN-PkgID=%s\n", mfx->package);
		fwrite(buf, 1, strlen(buf), file);


		snprintf(buf, BUFMAX, "X-TIZEN-InstalledStorage=%s\n", mfx->installed_storage);
		fwrite(buf, 1, strlen(buf), file);

//		snprintf(buf, BUFMAX, "X-TIZEN-PackageType=rpm\n");
//		fwrite(buf, 1, strlen(buf), file);

		if(up->appsvc) {
			snprintf(buf, BUFMAX, "X-TIZEN-Svc=");
			_LOGD("buf[%s]\n", buf);

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

			list_asvc = up->appsvc;
			while(list_asvc != NULL) {
				asvc = (appsvc_x *)list_asvc->data;
				list_op = asvc->operation;
				while(list_op != NULL) {
					op = (operation_x *)list_op->data;
					if (op && op->name)
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

							list_uri = asvc->uri;
							do{
								if(list_uri){
									ui = (uri_x *)list_uri->data;
									if (ui)
										uri = ui->name;
								}

								if(i++ > 0) {
									strncpy(buftemp, buf, BUFMAX);
									snprintf(buf, BUFMAX, "%s;", buftemp);
								}

								strncpy(buftemp, buf, BUFMAX);
								snprintf(buf, BUFMAX, "%s%s|%s|%s|%s", buftemp, operation?operation:"NULL", uri?uri:"NULL", mime?mime:"NULL", subapp?subapp:"NULL");
								_LOGD("buf[%s]\n", buf);

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

			fwrite(buf, 1, strlen(buf), file);

//			strncpy(buftemp, buf, BUFMAX);
//			snprintf(buf, BUFMAX, "%s\n", buftemp);
//			fwrite(buf, 1, strlen(buf), file);
		}

		fflush(file);
	        fd = fileno(file);
	        fsync(fd);
	        fclose(file);
	}

	FREE_AND_NULL(buf);
	FREE_AND_NULL(buftemp);

	return 0;
}

static int __ps_remove_nativeapp_desktop(manifest_x *mfx)
{
	char filepath[PKG_STRING_LEN_MAX] = "";
	int ret = 0;
	uiapplication_x *uiapplication = NULL;
	GList *list_up = mfx->uiapplication;

	for(; list_up; list_up = list_up->next) {
		uiapplication = (uiapplication_x *)list_up->data;
	        snprintf(filepath, sizeof(filepath),"%s%s.desktop", DESKTOP_RW_PATH, uiapplication->appid);

		ret = remove(filepath);
		if (ret <0)
			return -1;
	}

        return 0;
}

void __add_preload_info(manifest_x * mfx, const char *manifest)
{
	if(strstr(manifest, MANIFEST_RO_PREFIX)) {
		FREE_AND_NULL(mfx->readonly);
		mfx->readonly = strdup("True");

		FREE_AND_NULL(mfx->preload);
		mfx->preload = strdup("True");

		FREE_AND_NULL(mfx->removable);
		mfx->removable = strdup("False");

		FREE_AND_NULL(mfx->system);
		mfx->system = strdup("True");
	}
}

static int __check_preload_updated(manifest_x * mfx, const char *manifest)
{
	char filepath[PKG_STRING_LEN_MAX] = "";
	int ret = 0;
	uiapplication_x *uiapplication = NULL;
	GList *list_up = g_list_first(mfx->uiapplication);

	if(strstr(manifest, MANIFEST_RO_PREFIX)) {
		/* if preload app is updated, then remove previous desktop file on RW*/
		for(; list_up; list_up = list_up->next) {
			uiapplication = (uiapplication_x *)list_up->data;
			snprintf(filepath, sizeof(filepath),"%s%s.desktop", DESKTOP_RW_PATH, uiapplication->appid);
			ret = remove(filepath);
			if (ret <0)
				return -1;
		}
	} else {
		/* if downloaded app is updated, then update tag set true*/
		FREE_AND_NULL(mfx->update);
		mfx->update = strdup("true");
	}

	return 0;
}

static int __zone_get_appid_list(const char *pkgid, void *user_data, const char *zone)
{
	int col = 0;
	int cols = 0;
	int ret = 0;
	char *appid = NULL;

	sqlite3_stmt *stmt = NULL;
	sqlite3 *pkgmgr_parser_db;

	char *query = sqlite3_mprintf("select app_id from package_app_info where package=%Q", pkgid);


	char db_path[PKG_STRING_LEN_MAX] = {'\0',};

	if (zone == NULL || strcmp(zone, ZONE_HOST) == 0) {
		snprintf(db_path, PKG_STRING_LEN_MAX, "%s", PKGMGR_PARSER_DB_FILE);
	} else {
		char *rootpath = NULL;
		rootpath = __zone_get_root_path(zone);
		if (rootpath == NULL) {
			_LOGE("Failed to get rootpath");
			sqlite3_free(query);
			return -1;
		}

		snprintf(db_path, PKG_STRING_LEN_MAX, "%s%s", rootpath, PKGMGR_PARSER_DB_FILE);
	}

	_LOGD("db path(%s)", db_path);
	ret = db_util_open(db_path, &pkgmgr_parser_db, 0);

	if (ret != SQLITE_OK) {
		_LOGE("open fail\n");
		sqlite3_free(query);
		return -1;
	}
	ret = sqlite3_prepare_v2(pkgmgr_parser_db, query, strlen(query), &stmt, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("prepare_v2 fail\n");
		sqlite3_close(pkgmgr_parser_db);
		sqlite3_free(query);
		return -1;
	}

	cols = sqlite3_column_count(stmt);
	while(1)
	{
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {
			for(col = 0; col < cols; col++)
			{
				appid = (char*)sqlite3_column_text(stmt, col);
				_LOGS("success find appid[%s]\n", appid);
				(*(GList**)user_data) = g_list_append((*(GList**)user_data), g_strdup(appid));
			}
			ret = 0;
		} else {
			break;
		}
	}
	sqlite3_finalize(stmt);
	sqlite3_close(pkgmgr_parser_db);
	sqlite3_free(query);
	return ret;
}

static int __get_appid_list(const char *pkgid, void *user_data)
{
	return __zone_get_appid_list(pkgid, user_data, NULL);
}

char *__zone_get_root_path(const char *zone)
{
	vsm_context_h context = NULL;
	vsm_zone_h handle = NULL;
	char *rootpath = NULL;

	context = vsm_create_context();
	if (context == NULL) {
		_LOGE("Failed to create vsm context");
		return NULL;
	}

	handle = vsm_lookup_zone_by_name(context, zone);
	if (handle == NULL) {
		_LOGE("Failed to lookup zone by name");
		return NULL;
	}

	rootpath = (char *)vsm_get_zone_rootpath(handle);
	if (rootpath == NULL) {
		_LOGE("Failed to get rootpath");
		return NULL;
	}

	_LOGD("rootpath : %s", rootpath);

	return rootpath;
}

void __zone_call_uninstall_plugin(const char *pkgid, GList *appid_list, const char *zone)
{
	char *appid = NULL;
	GList *tmp_list;

	if (pkgid == NULL) {
		_LOGD("pkgid is null\n");
		return;
	}

	tmp_list = appid_list;
	_zone_pkgmgr_parser_plugin_open_db(zone);

	_pkgmgr_parser_plugin_uninstall_plugin(PKGMGR_PARSER_PLUGIN_TAG, pkgid, pkgid);

	while (tmp_list) {
		appid = (char *)tmp_list->data;
		if (appid) {
			_pkgmgr_parser_plugin_uninstall_plugin(PKGMGR_PARSER_PLUGIN_METADATA, pkgid, appid);
			_pkgmgr_parser_plugin_uninstall_plugin(PKGMGR_PARSER_PLUGIN_CATEGORY, pkgid, appid);
		}
		tmp_list = g_list_next(tmp_list);
	}

	_pkgmgr_parser_plugin_close_db();
}

void __call_uninstall_plugin(const char *pkgid, GList *appid_list)
{
	__zone_call_uninstall_plugin(pkgid, appid_list, NULL);
}

static int __delete_pkgid_info_for_uninstallation(const char *pkgid)
{
	retvm_if(pkgid == NULL, PM_PARSER_R_ERROR, "argument supplied is NULL");

	int ret = -1;
	char *appid = NULL;
	GList *appid_list = NULL;
	GList *tmp = NULL;
	char filepath[PKG_STRING_LEN_MAX] = "";

	_LOGD("Start uninstall for pkgid : delete pkgid[%s]\n", pkgid);

	/*get appid list*/
	ret = __get_appid_list(pkgid, &appid_list);
	retvm_if(ret < 0, PM_PARSER_R_ERROR, "Fail to get app_list");

	_LOGD("call plugin uninstall\n");

	/*call plugin uninstall*/
	__call_uninstall_plugin(pkgid, appid_list);

	/* delete pkgmgr db */
	ret = pkgmgr_parser_delete_pkgid_info_from_db(pkgid);
	if (ret == -1)
		_LOGD("DB pkgid info Delete failed\n");
	else
		_LOGD("DB pkgid info Delete Success\n");

	/* delete desktop file*/
	tmp = appid_list;
	while (tmp) {
		appid = (char *)tmp->data;
		if (appid) {
			_LOGD("Delete appid[%s] info from db\n", appid);

			ret = pkgmgr_parser_delete_appid_info_from_db(appid);
			if (ret == -1)
				_LOGD("DB appid info Delete failed\n");
			else
				_LOGD("DB appid info Delete Success\n");

			snprintf(filepath, sizeof(filepath),"%s%s.desktop", DESKTOP_RW_PATH, appid);
			if (access(filepath, R_OK) == 0) {
				ret = remove(filepath);
				if (ret < 0)
					_LOGD("remove failed\n");
			}
			g_free(tmp->data);
		}
		tmp  = g_list_next(tmp);
	}

	g_list_free(appid_list);

	_LOGD("Finish : uninstall for pkgid\n");

	return PM_PARSER_R_OK;
}

static int __pkgmgr_db_check_update_condition(manifest_x *mfx)
{
	int ret = -1;
	char *preload = NULL;
	char *system = NULL;
	char *csc_path = NULL;
	char *installed_storage = NULL;
	sqlite3_stmt *stmt = NULL;
	sqlite3 *pkgmgr_parser_db;
	char *query = NULL;

	ret = _pkgmgr_db_open(PKGMGR_PARSER_DB_FILE, &pkgmgr_parser_db);
	retvm_if(ret != PM_PARSER_R_OK, PM_PARSER_R_ERROR, "_pkgmgr_db_open failed\n");

	query = sqlite3_mprintf("select package_preload from package_info where package=%Q", mfx->package);

	ret = _pkgmgr_db_prepare(pkgmgr_parser_db, query, &stmt);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_prepare failed[%s]\n", query);

	ret = _pkgmgr_db_step(stmt);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_step failed\n");

	ret = _pkgmgr_db_get_str(stmt, 0, &preload);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_get_str failed\n");

	if (strcasecmp(preload, "true") == 0) {
		FREE_AND_NULL(mfx->preload);
		mfx->preload = strdup("true");
	}

	_pkgmgr_db_finalize(stmt);
	sqlite3_free(query);

	query = sqlite3_mprintf("select package_system from package_info where package=%Q", mfx->package);

	ret = _pkgmgr_db_prepare(pkgmgr_parser_db, query, &stmt);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_prepare failed[%s]\n", query);

	ret = _pkgmgr_db_step(stmt);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_step failed\n");

	ret = _pkgmgr_db_get_str(stmt, 0, &system);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_get_str failed\n");

	if (strcasecmp(system, "true") == 0) {
		FREE_AND_NULL(mfx->system);
		mfx->system = strdup("true");
	}

	_pkgmgr_db_finalize(stmt);
	sqlite3_free(query);

	query = sqlite3_mprintf("select csc_path from package_info where package=%Q", mfx->package);

	ret = _pkgmgr_db_prepare(pkgmgr_parser_db, query, &stmt);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_prepare failed[%s]\n", query);

	ret = _pkgmgr_db_step(stmt);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_step failed\n");

	ret = _pkgmgr_db_get_str(stmt, 0, &csc_path);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_get_str failed\n");

	if (csc_path != NULL) {
		if (mfx->csc_path)
			FREE_AND_NULL(mfx->csc_path);
		mfx->csc_path = strdup(csc_path);
	}

	_pkgmgr_db_finalize(stmt);
	sqlite3_free(query);

	query = sqlite3_mprintf("select installed_storage from package_info where package=%Q", mfx->package);

	ret = _pkgmgr_db_prepare(pkgmgr_parser_db, query, &stmt);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_prepare failed[%s]\n", query);

	ret = _pkgmgr_db_step(stmt);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_step failed\n");

	ret = _pkgmgr_db_get_str(stmt, 0, &installed_storage);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_get_str failed\n");

	if (strcasecmp(installed_storage, "installed_external") == 0) {
		FREE_AND_NULL(mfx->installed_storage);
		mfx->installed_storage = strdup("installed_external");
	}

	ret = PM_PARSER_R_OK;

catch:

	sqlite3_free(query);
	_pkgmgr_db_finalize(stmt);
	_pkgmgr_db_close(pkgmgr_parser_db);

	return ret;
}

static int __pkgmgr_db_check_disable_condition(manifest_x *mfx)
{
	int ret = -1;
	char *package_disable = NULL;
	sqlite3_stmt *stmt = NULL;
	sqlite3 *pkgmgr_parser_db;
	char *query = NULL;

	ret = _pkgmgr_db_open(PKGMGR_PARSER_DB_FILE, &pkgmgr_parser_db);
	retvm_if(ret != PM_PARSER_R_OK, PM_PARSER_R_ERROR, "_pkgmgr_db_open failed\n");

	query = sqlite3_mprintf("select package_disable from package_info where package=%Q", mfx->package);

	ret = _pkgmgr_db_prepare(pkgmgr_parser_db, query, &stmt);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_prepare failed[%s]\n", query);

	ret = _pkgmgr_db_step(stmt);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_step failed\n");

	ret = _pkgmgr_db_get_str(stmt, 0, &package_disable);
	tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "_pkgmgr_db_get_str failed\n");

	if (strcasecmp(package_disable, "true") == 0) {
		ret = 1;
	} else {
		ret = -1;
	}

catch:

	sqlite3_free(query);
	_pkgmgr_db_finalize(stmt);
	_pkgmgr_db_close(pkgmgr_parser_db);

	return ret;
}

API int pkgmgr_parser_create_desktop_file(manifest_x *mfx)
{
        int ret = 0;
	if (mfx == NULL) {
		_LOGD("Manifest pointer is NULL\n");
		return -1;
	}
        ret = __ps_make_nativeapp_desktop(mfx, NULL, ACTION_INSTALL);
        if (ret == -1)
                _LOGD("Creating desktop file failed\n");
        else
                _LOGD("Creating desktop file Success\n");
        return ret;
}

API void pkgmgr_parser_free_manifest_xml(manifest_x *mfx)
{
	_pkgmgrinfo_basic_free_manifest_x(mfx);
}

API manifest_x *pkgmgr_parser_process_manifest_xml(const char *manifest)
{
	_LOGD("parsing start\n");
	xmlTextReaderPtr reader;
	manifest_x *mfx = NULL;

	reader = xmlReaderForFile(manifest, NULL, 0);
	if (reader) {
		mfx = malloc(sizeof(manifest_x));
		if (mfx) {
			memset(mfx, '\0', sizeof(manifest_x));
			if (__process_manifest(reader, mfx) < 0) {
				_LOGD("Parsing Failed\n");
				pkgmgr_parser_free_manifest_xml(mfx);
				mfx = NULL;
			} else
				_LOGD("Parsing Success\n");
		} else {
			_LOGD("Memory allocation error\n");
		}
		xmlFreeTextReader(reader);
	} else {
		_LOGD("Unable to create xml reader\n");
	}
	return mfx;
}

/* These APIs are intended to call parser directly */

API int pkgmgr_parser_parse_manifest_for_installation(const char *manifest, char *const tagv[])
{
	retvm_if(manifest == NULL, PM_PARSER_R_ERROR, "argument supplied is NULL");
	_LOGD("parsing manifest for installation: %s\n", manifest);

	manifest_x *mfx = NULL;
	int ret = -1;

	xmlInitParser();
	mfx = pkgmgr_parser_process_manifest_xml(manifest);
	retvm_if(mfx == NULL, PM_PARSER_R_ERROR, "argument supplied is NULL");

	_LOGD("Parsing Finished\n");

	__add_preload_info(mfx, manifest);

	_LOGD("Added preload infomation\n");

	__ps_process_tag(mfx, tagv);

	ret = pkgmgr_parser_insert_manifest_info_in_db(mfx);
	if (ret < 0) {
		ret = __delete_pkgid_info_for_uninstallation(mfx->package);
		trym_if(ret == PM_PARSER_R_ERROR, "DB Delete failed");
		ret = pkgmgr_parser_insert_manifest_info_in_db(mfx);
		trym_if(ret == PM_PARSER_R_ERROR, "DB Insert failed");
	}

	_LOGD("DB Insert Success\n");

	_pkgmgr_parser_plugin_process_plugin(mfx, manifest, ACTION_INSTALL);

	ret = __check_action_fota(tagv);
	tryvm_if(ret == PM_PARSER_R_OK, ret = PM_PARSER_R_OK, "fota install called, dont need desktop generation\n");

	ret = __ps_make_nativeapp_desktop(mfx, NULL, ACTION_INSTALL);
	if (ret == -1)
		_LOGD("Creating desktop file failed\n");
	else
		_LOGD("Creating desktop file Success\n");

	ret = PM_PARSER_R_OK;

catch:
	pkgmgr_parser_free_manifest_xml(mfx);
	_LOGD("Free Done\n");
	xmlCleanupParser();

	return ret;
}

API int pkgmgr_parser_parse_manifest_for_upgrade(const char *manifest, char *const tagv[])
{
	retvm_if(manifest == NULL, PM_PARSER_R_ERROR, "argument supplied is NULL");
	_LOGD("parsing manifest for upgradation: %s\n", manifest);

	manifest_x *mfx = NULL;
	int ret = -1;
	int package_disabled = 0;

	xmlInitParser();
	mfx = pkgmgr_parser_process_manifest_xml(manifest);
	retvm_if(mfx == NULL, PM_PARSER_R_ERROR, "argument supplied is NULL");

	_LOGD("Parsing Finished\n");

	__add_preload_info(mfx, manifest);
	_LOGD("Added preload infomation\n");
	__check_preload_updated(mfx, manifest);

	if (__pkgmgr_db_check_disable_condition(mfx) == 1) {
		_LOGD("pkgid[%s] is disabed\n", mfx->package);
		package_disabled = 1;
	}

	ret = __pkgmgr_db_check_update_condition(mfx);
	if (ret < 0)
		_LOGD("__pkgmgr_db_check_update_condition failed\n");

	ret = pkgmgr_parser_update_manifest_info_in_db(mfx);
	trym_if(ret == PM_PARSER_R_ERROR, "DB Insert failed");
	_LOGD("DB Update Success\n");

	if (package_disabled == 1) {
		ret = pkgmgr_parser_disable_pkg(mfx->package, NULL);
		tryvm_if(ret != PM_PARSER_R_OK, ret = PM_PARSER_R_ERROR, "disable_pkg failed[%s]\n", mfx->package);
	}

	_pkgmgr_parser_plugin_process_plugin(mfx, manifest, ACTION_UPGRADE);

	ret = __check_action_fota(tagv);
	tryvm_if(ret == PM_PARSER_R_OK, ret = PM_PARSER_R_OK, "fota install called, dont need desktop generation\n");

	ret = __ps_make_nativeapp_desktop(mfx, manifest, ACTION_UPGRADE);
	if (ret == -1)
		_LOGD("Creating desktop file failed\n");
	else
		_LOGD("Creating desktop file Success\n");

	ret = PM_PARSER_R_OK;

catch:
	pkgmgr_parser_free_manifest_xml(mfx);
	_LOGD("Free Done\n");
	xmlCleanupParser();

	return ret;
}

API int pkgmgr_parser_parse_manifest_for_uninstallation(const char *manifest, char *const tagv[])
{
	retvm_if(manifest == NULL, PM_PARSER_R_ERROR, "argument supplied is NULL");
	int ret = -1;
	manifest_x *mfx = NULL;

	if (!strstr(manifest, ".xml")) {
		_LOGD("manifest is pkgid[%s]\n", manifest);
		ret = __delete_pkgid_info_for_uninstallation(manifest);
		if (ret <0)
			_LOGD("__delete_manifest_for_fota failed\n");

		return PM_PARSER_R_OK;
	}

	_LOGD("parsing manifest for uninstallation: %s\n", manifest);

	xmlInitParser();
	mfx = pkgmgr_parser_process_manifest_xml(manifest);
	retvm_if(mfx == NULL, PM_PARSER_R_ERROR, "argument supplied is NULL");

	_LOGD("Parsing Finished\n");

	__add_preload_info(mfx, manifest);
	_LOGD("Added preload infomation\n");

	_pkgmgr_parser_plugin_process_plugin(mfx, manifest, ACTION_UNINSTALL);

	ret = pkgmgr_parser_delete_manifest_info_from_db(mfx);
	if (ret == -1)
		_LOGD("DB Delete failed\n");
	else
		_LOGD("DB Delete Success\n");

	ret = __ps_remove_nativeapp_desktop(mfx);
	if (ret == -1)
		_LOGD("Removing desktop file failed\n");
	else
		_LOGD("Removing desktop file Success\n");

	ret = PM_PARSER_R_OK;

	pkgmgr_parser_free_manifest_xml(mfx);
	_LOGD("Free Done\n");
	xmlCleanupParser();

	return ret;
}

API int pkgmgr_parser_parse_manifest_for_preload()
{
	return pkgmgr_parser_update_preload_info_in_db();
}

API char *pkgmgr_parser_get_manifest_file(const char *pkgid)
{
	return __pkgid_to_manifest(pkgid);
}

API int pkgmgr_parser_run_parser_for_installation(xmlDocPtr docPtr, const char *tag, const char *pkgid)
{
	return __ps_run_parser(docPtr, tag, ACTION_INSTALL, pkgid);
}

API int pkgmgr_parser_run_parser_for_upgrade(xmlDocPtr docPtr, const char *tag, const char *pkgid)
{
	return __ps_run_parser(docPtr, tag, ACTION_UPGRADE, pkgid);
}

API int pkgmgr_parser_run_parser_for_uninstallation(xmlDocPtr docPtr, const char *tag, const char *pkgid)
{
	return __ps_run_parser(docPtr, tag, ACTION_UNINSTALL, pkgid);
}

API int pkgmgr_parser_check_manifest_validation(const char *manifest)
{
	if (manifest == NULL) {
		_LOGE("manifest file is NULL\n");
		return PM_PARSER_R_EINVAL;
	}
	int ret = PM_PARSER_R_OK;
	xmlSchemaParserCtxtPtr ctx = NULL;
	xmlSchemaValidCtxtPtr vctx = NULL;
	xmlSchemaPtr xschema = NULL;
	xmlDocPtr doc = NULL;
#ifdef _APPFW_FEATURE_DELTA_UPDATE
	if (strstr(manifest, DELTA_INFO_XML)) {
		ctx = xmlSchemaNewParserCtxt(DELTA_INFO_SCHEMA_FILE);
	} else {
		ctx = xmlSchemaNewParserCtxt(SCHEMA_FILE);
	}
#else
	ctx = xmlSchemaNewParserCtxt(SCHEMA_FILE);
#endif
	if (ctx == NULL) {
		_LOGE("xmlSchemaNewParserCtxt() Failed\n");
		return PM_PARSER_R_ERROR;
	}
	xschema = xmlSchemaParse(ctx);
	if (xschema == NULL) {
		_LOGE("xmlSchemaParse() Failed\n");
		ret = PM_PARSER_R_ERROR;
		goto cleanup;
	}

	vctx = xmlSchemaNewValidCtxt(xschema);
	if (vctx == NULL) {
		_LOGE("xmlSchemaNewValidCtxt() Failed\n");
		ret = PM_PARSER_R_ERROR;
		goto cleanup;
	}
	xmlSchemaSetValidErrors(vctx, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);

	doc = xmlParseFile(manifest);
	if (doc == NULL) {
		_LOGE("xmlParseFile() for %s failed\n", manifest);
		ret = PM_PARSER_R_ERROR;
		goto cleanup;
	}

	ret = xmlSchemaValidateDoc(vctx, doc);
	if (ret != 0) {
		_LOGE("xmlSchemaValidateDoc() for %s failed\n", manifest);
		ret = PM_PARSER_R_ERROR;
		goto cleanup;
	} else {
		_LOGE("Manifest is Valid\n");
		ret = PM_PARSER_R_OK;
		goto cleanup;
	}

cleanup:
	if (doc != NULL)
		xmlFreeDoc(doc);

	if (vctx != NULL)
		xmlSchemaFreeValidCtxt(vctx);

	if (ctx != NULL)
		xmlSchemaFreeParserCtxt(ctx);

	if (xschema != NULL)
		xmlSchemaFree(xschema);

	return ret;
}

API int zone_pkgmgr_parser_enable_pkg(const char *pkgid, char *const tagv[], const char *zone)
{
	retvm_if(pkgid == NULL, PM_PARSER_R_ERROR, "pkgid is NULL.");

	int ret = -1;
	char *manifest = NULL;
	manifest_x *mfx = NULL;

	ret = zone_pkgmgr_parser_update_enabled_pkg_info_in_db(pkgid, zone);
	if (ret == -1)
		_LOGD("pkgmgr_parser_update_enabled_pkg_info_in_db(%s) failed.\n", pkgid);
	else
		_LOGD("pkgmgr_parser_update_enabled_pkg_info_in_db(%s) succeed.\n", pkgid);

	manifest = __zone_pkgid_to_manifest(pkgid, zone);
	retvm_if(manifest == NULL, PM_PARSER_R_ERROR, "manifest of pkgid(%s) is NULL.", pkgid);

	xmlInitParser();
	mfx = pkgmgr_parser_process_manifest_xml(manifest);
	tryvm_if(mfx == NULL, ret = PM_PARSER_R_ERROR, "process_manifest_xml(%s) failed\n", pkgid);

	_zone_pkgmgr_parser_plugin_process_plugin(mfx, manifest, ACTION_INSTALL, zone);

	ret = PM_PARSER_R_OK;

catch:
	if (mfx)
		pkgmgr_parser_free_manifest_xml(mfx);

	if (manifest)
		free(manifest);

	xmlCleanupParser();

	return ret;
}

API int pkgmgr_parser_enable_pkg(const char *pkgid, char *const tagv[])
{
	return zone_pkgmgr_parser_enable_pkg(pkgid, tagv, NULL);
}

API int pkgmgr_parser_set_app_background_operation(const char *appid, bool is_disable)
{
	return zone_pkgmgr_parser_set_app_background_operation(appid, is_disable, NULL);
}

API int zone_pkgmgr_parser_set_app_background_operation(const char *appid, bool is_disable, const char *zone)
{
	int ret = -1;
	retvm_if(appid == NULL, PM_PARSER_R_ERROR, "appid is NULL.");

	ret = zone_pkgmgr_parser_update_app_disable_bg_operation_info_in_db(appid, zone, is_disable);
	if (ret == -1) {
		_LOGE("pkgmgr_parser_update_app_disabled_bg_operation_info_in_db[%s] failed.\n", appid);
		return ret;
	}
	else
		_LOGD("pkgmgr_parser_update_app_disabled_bg_operation_info_in_db[%s] succeed.\n", appid);

	return PM_PARSER_R_OK;
}

API int zone_pkgmgr_parser_disable_pkg(const char *pkgid, char *const tagv[], const char *zone)
{
	retvm_if(pkgid == NULL, PM_PARSER_R_ERROR, "pkgid is NULL.");

	int ret = -1;
	GList *appid_list = NULL;

	/*get appid list*/
	ret = __zone_get_appid_list(pkgid, &appid_list, zone);
	retvm_if(ret < 0, PM_PARSER_R_ERROR, "Fail to get app_list");

	_LOGD("call plugin uninstall\n");

	/*call plugin uninstall*/
	__zone_call_uninstall_plugin(pkgid, appid_list, zone);

	ret = zone_pkgmgr_parser_update_disabled_pkg_info_in_db(pkgid, zone);
	if (ret == -1)
		_LOGD("pkgmgr_parser_update_disabled_pkg_info_in_db(%s) failed.\n", pkgid);
	else
		_LOGD("pkgmgr_parser_update_disabled_pkg_info_in_db(%s) succeed.\n", pkgid);

	g_list_free(appid_list);

	return PM_PARSER_R_OK;
}

API int pkgmgr_parser_disable_pkg(const char *pkgid, char *const tagv[])
{
	return zone_pkgmgr_parser_disable_pkg(pkgid, tagv, NULL);
}

/* update aliasid entry in package-app-aliasid */
API int pkgmgr_parser_insert_app_aliasid(void )
{
	return pkgmgr_parser_insert_app_aliasid_info_in_db();
}

API int pkgmgr_parser_update_app_aliasid(void )
{
	return pkgmgr_parser_update_app_aliasid_info_in_db();
}

#ifdef _APPFW_FEATURE_EXPANSION_PKG_INSTALL
API int pkgmgr_parser_insert_tep(const char *pkgid, const char *tep_name)
{
	return pkgmgr_parser_insert_tep_in_db(pkgid, tep_name);
}

API int pkgmgr_parser_update_tep(const char *pkgid, const char *tep_name)
{
	return pkgmgr_parser_update_tep_in_db(pkgid, tep_name);
}

API int pkgmgr_parser_delete_tep(const char *pkgid)
{
	return pkgmgr_parser_delete_tep_in_db(pkgid);
}
#endif

#ifdef _APPFW_FEATURE_MOUNT_INSTALL
API int pkgmgr_parser_insert_mount_install_info(const char *pkgid, bool ismount, const char *tpk_name)
{
	return pkgmgr_parser_insert_mount_install_info_in_db(pkgid, ismount, tpk_name);
}
#endif
