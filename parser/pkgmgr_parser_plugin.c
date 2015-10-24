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
#include <glib.h>
#include <db-util.h>

#include "pkgmgr_parser_plugin.h"
#include "pkgmgr-info.h"
#include "pkgmgrinfo_debug.h"
#include "pkgmgr_parser_internal.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_PLUGIN"

#define ASCII(s) (const char *)s
#define XMLCHAR(s) (const xmlChar *)s

#define PLLUGIN_LIST "/usr/etc/package-manager/parserlib/pkgmgr_parser_plugin_list.txt"

#define TOKEN_TYPE_STR	"type="
#define TOKEN_NAME_STR	"name="
#define TOKEN_FLAG_STR	"flag="
#define TOKEN_PATH_STR	"path="

#define SEPERATOR_START		'"'
#define SEPERATOR_END		'"'

#define PKG_PARSER_CONF_PATH	"/usr/etc/package-manager/parser_path.conf"
#define TAG_PARSER_NAME	"parserlib:"

typedef struct pkgmgr_parser_plugin_info_x {
	manifest_x *mfx;
	char *pkgid;
	char *appid;
	char *filename;
	char *type;
	char *name;
	char *path;
	char *flag;
	void *lib_handle;
	ACTION_TYPE action;
	int enabled_plugin;
} pkgmgr_parser_plugin_info_x;

#define E_PKGMGR_PARSER_PLUGIN_MAX						0xFFFFFFFF

#define QUERY_CREATE_TABLE_PACKAGE_PLUGIN_INFO "create table if not exists package_plugin_info " \
	"(pkgid text primary key not null, " \
	"enabled_plugin INTEGER DEFAULT 0)"

/*db info*/
#define PKGMGR_PARSER_DB_FILE "/opt/dbspace/.pkgmgr_parser.db"
sqlite3 *pkgmgr_parser_db;

typedef struct {
	char *key;
	char *value;
} __metadata_t;

typedef struct {
	char *name;
} __category_t;

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

static char *__get_parser_plugin_path(const char *type, char *plugin_name)
{
	FILE *fp = NULL;
	char buffer[1024] = { 0 };
	char temp_path[1024] = { 0 };
	char *path = NULL;

	if (type == NULL) {
		_LOGE("invalid argument\n");
		return NULL;
	}

	fp = fopen(PKG_PARSER_CONF_PATH, "r");
	if (fp == NULL) {
		_LOGE("no matching backendlib\n");
		return NULL;
	}

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (buffer[0] == '#')
			continue;

		__str_trim(buffer);

		if ((path = strstr(buffer, plugin_name)) != NULL) {
			path = path + strlen(plugin_name);
			break;
		}

		memset(buffer, 0x00, 1024);
	}

	if (fp != NULL)
		fclose(fp);

	if (path == NULL) {
		_LOGE("no matching backendlib\n");
		return NULL;
	}

	snprintf(temp_path, sizeof(temp_path) - 1, "%slib%s.so", path, type);

	return strdup(temp_path);
}

static void __metadata_parser_clear_dir_list(GList* dir_list)
{
	GList *list = NULL;
	__metadata_t* detail = NULL;

	if (dir_list) {
		list = g_list_first(dir_list);
		while (list) {
			detail = (__metadata_t *)list->data;
			if (detail) {
				FREE_AND_NULL(detail->key);
				FREE_AND_NULL(detail->value);
				FREE_AND_NULL(detail);
			}
			list = g_list_next(list);
		}
		g_list_free(dir_list);
	}
}

static void __category_parser_clear_dir_list(GList* dir_list)
{
	GList *list = NULL;
	__category_t* detail = NULL;

	if (dir_list) {
		list = g_list_first(dir_list);
		while (list) {
			detail = (__category_t *)list->data;
			if (detail) {
				FREE_AND_NULL(detail->name);
				FREE_AND_NULL(detail);
			}
			list = g_list_next(list);
		}
		g_list_free(dir_list);
	}
}

static int __parser_send_tag(void *lib_handle, ACTION_TYPE action, PLUGIN_PROCESS_TYPE process, const char *pkgid)
{
	int ret = -1;
	int (*plugin_install) (const char *);
	char *ac = NULL;

	if (process == PLUGIN_PRE_PROCESS) {
		switch (action) {
		case ACTION_INSTALL:
			ac = "PKGMGR_PARSER_PLUGIN_PRE_INSTALL";
			break;
		case ACTION_UPGRADE:
			ac = "PKGMGR_PARSER_PLUGIN_PRE_UPGRADE";
			break;
		case ACTION_UNINSTALL:
			ac = "PKGMGR_PARSER_PLUGIN_PRE_UNINSTALL";
			break;
		default:
			_LOGE("PRE_PROCESS : action type error[%d]\n", action);
			return -1;
		}
	} else if (process == PLUGIN_POST_PROCESS) {
		switch (action) {
		case ACTION_INSTALL:
			ac = "PKGMGR_PARSER_PLUGIN_POST_INSTALL";
			break;
		case ACTION_UPGRADE:
			ac = "PKGMGR_PARSER_PLUGIN_POST_UPGRADE";
			break;
		case ACTION_UNINSTALL:
			ac = "PKGMGR_PARSER_PLUGIN_POST_UNINSTALL";
			break;
		default:
			_LOGE("POST_PROCESS : action type error[%d]\n", action);
			return -1;
		}
	} else {
		_LOGE("process type error[%d]\n", process);
		return -1;
	}

	if ((plugin_install = dlsym(lib_handle, ac)) == NULL || dlerror() != NULL) {
		return -1;
	}

	ret = plugin_install(pkgid);
	return ret;
}

int __ps_run_parser(xmlDocPtr docPtr, const char *tag,
			   ACTION_TYPE action, const char *pkgid)
{
	char *lib_path = NULL;
	void *lib_handle = NULL;
	int (*plugin_install) (xmlDocPtr, const char *);
	int ret = -1;
	char *ac = NULL;

	switch (action) {
	case ACTION_INSTALL:
		ac = "PKGMGR_PARSER_PLUGIN_INSTALL";
		break;
	case ACTION_UPGRADE:
		ac = "PKGMGR_PARSER_PLUGIN_UPGRADE";
		break;
	case ACTION_UNINSTALL:
		ac = "PKGMGR_PARSER_PLUGIN_UNINSTALL";
		break;
	default:
		goto END;
	}

	lib_path = __get_parser_plugin_path(tag, TAG_PARSER_NAME);

	if (!lib_path) {
		goto END;
	}

	if ((lib_handle = dlopen(lib_path, RTLD_LAZY)) == NULL) {
		_LOGE("dlopen is failed lib_path[%s]\n", lib_path);
		goto END;
	}
	if ((plugin_install =
		dlsym(lib_handle, ac)) == NULL || dlerror() != NULL) {
		_LOGE("can not find symbol[%s] \n", ac);
		goto END;
	}

	ret = plugin_install(docPtr, pkgid);
	if (ret < 0)
		_LOGD("[pkgid = %s, libpath = %s plugin fail\n", pkgid, lib_path);
	else
		_LOGD("[pkgid = %s, libpath = %s plugin success\n", pkgid, lib_path);

END:
	FREE_AND_NULL(lib_path);
	if (lib_handle)
		dlclose(lib_handle);
	return ret;
}

static int __ps_get_enabled_plugin(const char *pkgid)
{
	char *query = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *tail = NULL;
	int rc = 0;
	int enabled_plugin = 0;

	query = sqlite3_mprintf("select * from package_plugin_info where pkgid LIKE %Q", pkgid);

	if (SQLITE_OK != sqlite3_prepare(pkgmgr_parser_db, query, strlen(query), &stmt, &tail)) {
		_LOGE("sqlite3_prepare error\n");
		sqlite3_free(query);
		return E_PKGMGR_PARSER_PLUGIN_MAX;
	}

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW || rc == SQLITE_DONE) {
		_LOGE("No records found");
		goto FINISH_OFF;
	}

	enabled_plugin = (int)sqlite3_column_int(stmt, 1);

//	_LOGD("enabled_plugin  ===>    %d", enabled_plugin);

	if (SQLITE_OK != sqlite3_finalize(stmt)) {
		_LOGE("error : sqlite3_finalize\n");
		goto FINISH_OFF;
	}

	sqlite3_free(query);
	return enabled_plugin;

FINISH_OFF:
	rc = sqlite3_finalize(stmt);
	if (rc != SQLITE_OK) {
		_LOGE(" sqlite3_finalize failed - %d", rc);
	}
	sqlite3_free(query);
	return E_PKGMGR_PARSER_PLUGIN_MAX;
}

static char *__getvalue(const char* pBuf, const char* pKey)
{
	const char* p = NULL;
	const char* pStart = NULL;
	const char* pEnd = NULL;

	p = strstr(pBuf, pKey);
	if (p == NULL)
		return NULL;

	pStart = p + strlen(pKey) + 1;
	pEnd = strchr(pStart, SEPERATOR_END);
	if (pEnd == NULL)
		return NULL;

	size_t len = pEnd - pStart;
	if (len <= 0)
		return NULL;

	char *pRes = (char*)malloc(len + 1);
	if(pRes == NULL)
		return NULL;

	strncpy(pRes, pStart, len);
	pRes[len] = 0;

	return pRes;
}

static int __get_plugin_info_x(const char*buf, pkgmgr_parser_plugin_info_x *plugin_info)
{
	if (buf[0] == '#')
		return -1;

	if (strstr(buf, TOKEN_TYPE_STR) == NULL)
		return -1;

	plugin_info->type = __getvalue(buf, TOKEN_TYPE_STR);
	if (plugin_info->type == NULL)
		return -1;

	plugin_info->name = __getvalue(buf, TOKEN_NAME_STR);
	plugin_info->flag = __getvalue(buf, TOKEN_FLAG_STR);
	plugin_info->path = __getvalue(buf, TOKEN_PATH_STR);

	return 0;
}

void __clean_plugin_info(pkgmgr_parser_plugin_info_x *plugin_info)
{
	if(plugin_info == NULL)
		return;

	FREE_AND_NULL(plugin_info->filename);
	FREE_AND_NULL(plugin_info->pkgid);
	FREE_AND_NULL(plugin_info->appid);
	FREE_AND_NULL(plugin_info);
	return;
}

static void __check_enabled_plugin(pkgmgr_parser_plugin_info_x *plugin_info)
{
	int enabled_plugin = 0x0000001;

	enabled_plugin = (int)strtol( plugin_info->flag, NULL, 16 );

	_LOGD( "[%s] flag = 0x%x done[action=%d]!! \n", plugin_info->pkgid, enabled_plugin, plugin_info->action);

	plugin_info->enabled_plugin = plugin_info->enabled_plugin | enabled_plugin;
}

static void __run_metadata_parser(pkgmgr_parser_plugin_info_x *plugin_info, GList *md_list, const char *appid)
{
	int (*metadata_parser_plugin) (const char *, const char *, GList *);
	int ret = -1;
	char *ac = NULL;

	switch (plugin_info->action) {
	case ACTION_INSTALL:
		ac = "PKGMGR_MDPARSER_PLUGIN_INSTALL";
		break;
	case ACTION_UPGRADE:
		ac = "PKGMGR_MDPARSER_PLUGIN_UPGRADE";
		break;
	case ACTION_UNINSTALL:
		ac = "PKGMGR_MDPARSER_PLUGIN_UNINSTALL";
		break;
	default:
		goto END;
	}

	if ((metadata_parser_plugin =
		dlsym(plugin_info->lib_handle, ac)) == NULL || dlerror() != NULL) {
		_LOGE("can not find symbol[%s] \n",ac);
		goto END;
	}

	ret = metadata_parser_plugin(plugin_info->pkgid, appid, md_list);
	_LOGD("Plugin = %s, appid = %s, result=%d\n", plugin_info->name, appid, ret);

	__check_enabled_plugin(plugin_info);

END:
	return;
}

static void __run_category_parser(pkgmgr_parser_plugin_info_x *plugin_info, GList *category_list, const char *appid)
{
	int (*category_parser_plugin) (const char *, const char *, GList *);
	int ret = -1;
	char *ac = NULL;

	switch (plugin_info->action) {
	case ACTION_INSTALL:
		ac = "PKGMGR_CATEGORY_PARSER_PLUGIN_INSTALL";
		break;
	case ACTION_UPGRADE:
		ac = "PKGMGR_CATEGORY_PARSER_PLUGIN_UPGRADE";
		break;
	case ACTION_UNINSTALL:
		ac = "PKGMGR_CATEGORY_PARSER_PLUGIN_UNINSTALL";
		break;
	default:
		goto END;
	}

	if ((category_parser_plugin =
		dlsym(plugin_info->lib_handle, ac)) == NULL || dlerror() != NULL) {
		_LOGE("can not find symbol[%s] \n",ac);
		goto END;
	}

	ret = category_parser_plugin(plugin_info->pkgid, appid, category_list);
	_LOGD("Plugin = %s, appid = %s, result=%d\n", plugin_info->name, appid, ret);

	__check_enabled_plugin(plugin_info);

END:
	return;
}


static void __run_tag_parser(pkgmgr_parser_plugin_info_x *plugin_info, xmlDocPtr docPtr)
{
	int (*plugin_install) (xmlDocPtr, const char *);
	int ret = -1;
	char *ac = NULL;

	switch (plugin_info->action) {
	case ACTION_INSTALL:
		ac = "PKGMGR_PARSER_PLUGIN_INSTALL";
		break;
	case ACTION_UPGRADE:
		ac = "PKGMGR_PARSER_PLUGIN_UPGRADE";
		break;
	case ACTION_UNINSTALL:
		ac = "PKGMGR_PARSER_PLUGIN_UNINSTALL";
		break;
	default:
		goto END;
	}

	if ((plugin_install =
		dlsym(plugin_info->lib_handle, ac)) == NULL || dlerror() != NULL) {
		_LOGE("can not find symbol[%s] \n", ac);
		goto END;
	}

	ret = plugin_install(docPtr, plugin_info->pkgid);
	_LOGD("Plugin = %s, appid = %s, result=%d\n", plugin_info->name, plugin_info->pkgid, ret);

	__check_enabled_plugin(plugin_info);
END:
	return;
}

static int __run_tag_parser_prestep(pkgmgr_parser_plugin_info_x *plugin_info, xmlTextReaderPtr reader)
{
	const xmlChar *name;

	if (xmlTextReaderDepth(reader) != 1) {
		_LOGE("Node depth is not 1");
		goto END;
	}

	if (xmlTextReaderNodeType(reader) != 1) {
		_LOGE("Node type is not 1");
		goto END;
	}

	const xmlChar *value;
	name = xmlTextReaderConstName(reader);
	if (name == NULL) {
		_LOGE("name is NULL.");
	}

	value = xmlTextReaderConstValue(reader);
	if (value != NULL) {
		if (xmlStrlen(value) > 40) {
			_LOGD(" %.40s...", value);
		} else {
			_LOGD(" %s", value);
		}
	}

	name = xmlTextReaderConstName(reader);
	if (name == NULL) {
		_LOGE("name is NULL.");
	}

	xmlDocPtr docPtr = xmlTextReaderCurrentDoc(reader);
	xmlDocPtr copyDocPtr = xmlCopyDoc(docPtr, 1);
	if (copyDocPtr == NULL)
		return -1;
	xmlNode *rootElement = xmlDocGetRootElement(copyDocPtr);
	if (rootElement == NULL)
		return -1;
	xmlNode *cur_node = xmlFirstElementChild(rootElement);
	if (cur_node == NULL)
		return -1;
	xmlNode *temp = xmlTextReaderExpand(reader);
	if (temp == NULL)
		return -1;
	xmlNode *next_node = NULL;
	while(cur_node != NULL) {
		if ( (strcmp(ASCII(temp->name), ASCII(cur_node->name)) == 0) &&
			(temp->line == cur_node->line) ) {
			break;
		}
		else {
			next_node = xmlNextElementSibling(cur_node);
			xmlUnlinkNode(cur_node);
			xmlFreeNode(cur_node);
			cur_node = next_node;
		}
	}
	if (cur_node == NULL)
		return -1;
	next_node = xmlNextElementSibling(cur_node);
	if (next_node) {
		cur_node->next = NULL;
		next_node->prev = NULL;
		xmlFreeNodeList(next_node);
		xmlSetTreeDoc(cur_node, copyDocPtr);
	} else {
		xmlSetTreeDoc(cur_node, copyDocPtr);
	}

	__run_tag_parser(plugin_info, copyDocPtr);
 END:

	return 0;
}

static void
__process_tag_xml(pkgmgr_parser_plugin_info_x *plugin_info, xmlTextReaderPtr reader)
{
	switch (xmlTextReaderNodeType(reader)) {
	case XML_READER_TYPE_END_ELEMENT:
		{
			break;
		}
	case XML_READER_TYPE_ELEMENT:
		{
			// Elements without closing tag don't receive
			const xmlChar *elementName =
			    xmlTextReaderLocalName(reader);
			if (elementName == NULL) {
				break;
			}

			if (strcmp(plugin_info->name, ASCII(elementName)) == 0) {
				__run_tag_parser_prestep(plugin_info, reader);
			}
			if(elementName != NULL){
				xmlFree((void*)elementName);
				elementName = NULL;
			}
			break;
		}

	default:
		break;
	}
}

static void __process_tag_parser(pkgmgr_parser_plugin_info_x *plugin_info)
{
	xmlTextReaderPtr reader;
	xmlDocPtr docPtr = NULL;
	int ret = -1;

	if (access(plugin_info->filename, R_OK) != 0) {
		__run_tag_parser(plugin_info, NULL);
	} else {
		docPtr = xmlReadFile(plugin_info->filename, NULL, 0);
		reader = xmlReaderWalker(docPtr);
		if (reader != NULL) {
			ret = xmlTextReaderRead(reader);
			while (ret == 1) {
				__process_tag_xml(plugin_info, reader);
				ret = xmlTextReaderRead(reader);
			}
			xmlFreeTextReader(reader);

			if (ret != 0) {
				_LOGS("%s : failed to parse", plugin_info->filename);
			}
		} else {
			_LOGS("%s : failed to read", plugin_info->filename);
		}
	}

	if(docPtr != NULL){
		xmlFreeDoc(docPtr);
		docPtr = NULL;
	}
}

static void __process_category_parser(pkgmgr_parser_plugin_info_x *plugin_info)
{
	int tag_exist = 0;
	char buffer[1024] = { 0, };
	category_x *category = NULL;
	GList *ct = NULL;
	GList *category_list = NULL;
	__category_t *category_detail = NULL;

	if (plugin_info->mfx == NULL) {
		__run_category_parser(plugin_info, NULL, plugin_info->appid);
		return;
	}

	uiapplication_x *up = NULL;
	GList *list_up = plugin_info->mfx->uiapplication;
	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		ct = up->category;
		while (ct != NULL)
		{
			category = (category_x*)ct->data;
			//get glist of category key and value combination
			memset(buffer, 0x00, 1024);
			snprintf(buffer, 1024, "%s/", plugin_info->name);
			if ((category) && (category->name) && (strncmp(category->name, plugin_info->name, strlen(plugin_info->name)) == 0)) {
				category_detail = (__category_t*) calloc(1, sizeof(__category_t));
				if (category_detail == NULL) {
					_LOGD("Memory allocation failed\n");
					goto END;
				}

				category_detail->name = (char*) calloc(1, sizeof(char)*(strlen(category->name)+2));
				if (category_detail->name == NULL) {
					_LOGD("Memory allocation failed\n");
					FREE_AND_NULL(category_detail);
					goto END;
				}
				snprintf(category_detail->name, (strlen(category->name)+1), "%s", category->name);

				category_list = g_list_append(category_list, (gpointer)category_detail);
				tag_exist = 1;
			}
			ct = ct->next;
		}

		//send glist to parser when tags for metadata plugin parser exist.
		if (tag_exist) {
			__run_category_parser(plugin_info, category_list, up->appid);
		}

		__category_parser_clear_dir_list(category_list);
		category_list = NULL;
		tag_exist = 0;
		list_up = list_up->next;
	}
END:
	if (category_list)
		__category_parser_clear_dir_list(category_list);

	return;
}

static void __process_metadata_parser(pkgmgr_parser_plugin_info_x *plugin_info)
{
	int tag_exist = 0;
//	char buffer[1024] = { 0, }; Not in use anywhere
	metadata_x *metadata = NULL;
	GList *md = NULL;
	GList *md_list = NULL;
	__metadata_t *md_detail = NULL;

	if (plugin_info->mfx == NULL) {
		__run_metadata_parser(plugin_info, NULL, plugin_info->appid);
		return;
	}

	uiapplication_x *up = NULL;
	GList *list_up = plugin_info->mfx->uiapplication;

	while(list_up != NULL)
	{
		up = (uiapplication_x *)list_up->data;
		md = up->metadata;
		while (md != NULL)
		{
			metadata = (metadata_x *)md->data;
			//get glist of metadata key and value combination
//			memset(buffer, 0x00, 1024);
//			snprintf(buffer, 1024, "%s/", plugin_info->name);
			if ((metadata && metadata->key && metadata->value) && (strncmp(metadata->key, plugin_info->name, strlen(plugin_info->name)) == 0)) {
				md_detail = (__metadata_t*) calloc(1, sizeof(__metadata_t));
				if (md_detail == NULL) {
					_LOGD("Memory allocation failed\n");
					goto END;
				}

				md_detail->key = (char*) calloc(1, sizeof(char)*(strlen(metadata->key)+2));
				if (md_detail->key == NULL) {
					_LOGD("Memory allocation failed\n");
					FREE_AND_NULL(md_detail);
					goto END;
				}
				snprintf(md_detail->key, (strlen(metadata->key)+1), "%s", metadata->key);

				md_detail->value = (char*) calloc(1, sizeof(char)*(strlen(metadata->value)+2));
				if (md_detail->value == NULL) {
					_LOGD("Memory allocation failed\n");
					FREE_AND_NULL(md_detail->key);
					FREE_AND_NULL(md_detail);
					goto END;
				}
				snprintf(md_detail->value, (strlen(metadata->value)+1), "%s", metadata->value);

				md_list = g_list_append(md_list, (gpointer)md_detail);
				tag_exist = 1;
			}
			md = md->next;
		}

		//send glist to parser when tags for metadata plugin parser exist.
		if (tag_exist) {
			__run_metadata_parser(plugin_info, md_list, up->appid);
		}

		__metadata_parser_clear_dir_list(md_list);
		md_list = NULL;
		tag_exist = 0;
		list_up = list_up->next;
	}

END:
	if(md_list)
		__metadata_parser_clear_dir_list(md_list);
	metadata = NULL;
	return;
}

void __process_plugin_db(pkgmgr_parser_plugin_info_x *plugin_info)
{
	char *query  = NULL;
	char *error_message = NULL;

	query = sqlite3_mprintf("delete from package_plugin_info where pkgid LIKE %Q", plugin_info->pkgid);
	if (SQLITE_OK == sqlite3_exec(pkgmgr_parser_db, query, NULL, NULL, &error_message)) {
		_LOGS("pkgid [%s] plugin[0x%x] deleted", plugin_info->pkgid, plugin_info->enabled_plugin);
	}

	sqlite3_free(query);

	if (plugin_info->action == ACTION_UNINSTALL)
		return;

	query = sqlite3_mprintf("insert into package_plugin_info(pkgid,enabled_plugin) values (%Q,'%d')", plugin_info->pkgid, plugin_info->enabled_plugin);
	if (SQLITE_OK != sqlite3_exec(pkgmgr_parser_db, query, NULL, NULL, &error_message)) {
		_LOGE("Don't execute query = %s, error message = %s\n", query, error_message);
		sqlite3_free(query);
		return;
	}

	sqlite3_free(query);
	_LOGS("pkgid [%s] plugin[0x%x] inserted", plugin_info->pkgid, plugin_info->enabled_plugin);

	return;
}

static void __process_each_plugin(pkgmgr_parser_plugin_info_x *plugin_info)
{
	int ret = 0;

	plugin_info->lib_handle = dlopen(plugin_info->path, RTLD_LAZY);
	if (plugin_info->lib_handle == NULL) return;

	ret = __parser_send_tag(plugin_info->lib_handle, plugin_info->action, PLUGIN_PRE_PROCESS, plugin_info->pkgid);
	_LOGS("PLUGIN_PRE_PROCESS : [%s] result=[%d]\n", plugin_info->name, ret);

	if (strcmp(plugin_info->type,PKGMGR_PARSER_PLUGIN_TAG) == 0) {
		__process_tag_parser(plugin_info);
	} else if (strcmp(plugin_info->type,PKGMGR_PARSER_PLUGIN_METADATA) == 0) {
		__process_metadata_parser(plugin_info);
	} else if (strcmp(plugin_info->type,PKGMGR_PARSER_PLUGIN_CATEGORY) == 0) {
		__process_category_parser(plugin_info);
	}

	ret =__parser_send_tag(plugin_info->lib_handle, plugin_info->action, PLUGIN_POST_PROCESS, plugin_info->pkgid);
	_LOGS("PLUGIN_POST_PROCESS : [%s] result=[%d]\n", plugin_info->name, ret);

	dlclose(plugin_info->lib_handle);
}

void __process_all_plugins(pkgmgr_parser_plugin_info_x *plugin_info)
{
	int ret = 0;
	FILE *fp = NULL;
	char buf[PKG_STRING_LEN_MAX] = {0};

	fp = fopen(PLLUGIN_LIST, "r");
	retm_if(fp == NULL, "Fail get : %s", PLLUGIN_LIST);

	while (fgets(buf, PKG_STRING_LEN_MAX, fp) != NULL) {
		__str_trim(buf);

		ret = __get_plugin_info_x(buf, plugin_info);
		if (ret < 0)
			continue;

		__process_each_plugin(plugin_info);

		memset(buf, 0x00, PKG_STRING_LEN_MAX);
		FREE_AND_NULL(plugin_info->type);
		FREE_AND_NULL(plugin_info->name);
		FREE_AND_NULL(plugin_info->flag);
		FREE_AND_NULL(plugin_info->path);
	}

	if (fp != NULL)
		fclose(fp);

	return;
}


int _zone_pkgmgr_parser_plugin_open_db(const char *zone)
{
	char *error_message = NULL;
	int ret;
	FILE * fp = NULL;
	char db_path[PKG_STRING_LEN_MAX] = {'\0',};

	if (zone == NULL || strlen(zone) == 0 || strcmp(zone, ZONE_HOST) == 0) {
		snprintf(db_path, PKG_STRING_LEN_MAX, "%s", PKGMGR_PARSER_DB_FILE);
	} else {
		char *rootpath = NULL;
		rootpath = __zone_get_root_path(zone);
		if (rootpath == NULL) {
			_LOGE("Failed to get rootpath");
			return -1;
		}

		snprintf(db_path, PKG_STRING_LEN_MAX, "%s%s", rootpath, PKGMGR_PARSER_DB_FILE);
	}
	_LOGD("db path(%s)", db_path);

	fp = fopen(db_path, "r");
	if (fp != NULL) {
		fclose(fp);
		ret = db_util_open(db_path, &pkgmgr_parser_db, DB_UTIL_REGISTER_HOOK_METHOD);
		if (ret != SQLITE_OK) {
			_LOGE("package info DB initialization failed\n");
			return -1;
		}
		return 0;
	}

	ret = db_util_open(db_path, &pkgmgr_parser_db, DB_UTIL_REGISTER_HOOK_METHOD);
	if (ret != SQLITE_OK) {
		_LOGE("package info DB initialization failed\n");
		return -1;
	}

	if (SQLITE_OK != sqlite3_exec(pkgmgr_parser_db, QUERY_CREATE_TABLE_PACKAGE_PLUGIN_INFO, NULL, NULL, &error_message)) {
		_LOGE("Don't execute query = %s, error message = %s\n", QUERY_CREATE_TABLE_PACKAGE_PLUGIN_INFO, error_message);
		return -1;
	}

	_LOGE("db_initialize_done\n");
	return 0;
}

int _pkgmgr_parser_plugin_open_db()
{
	return _zone_pkgmgr_parser_plugin_open_db(NULL);
}

void _pkgmgr_parser_plugin_close_db()
{
	sqlite3_close(pkgmgr_parser_db);
}

void _zone_pkgmgr_parser_plugin_process_plugin(manifest_x *mfx, const char *filename, ACTION_TYPE action, const char *zone)
{
	int ret = 0;

	pkgmgr_parser_plugin_info_x *plugin_info = NULL;

	retm_if(mfx == NULL, "manifest pointer is NULL\n");
	retm_if(filename == NULL, "filename pointer is NULL\n");

	/*initialize plugin_info_x*/
	plugin_info = malloc(sizeof(pkgmgr_parser_plugin_info_x));
	retm_if(plugin_info == NULL, "malloc fail");

	memset(plugin_info, '\0', sizeof(pkgmgr_parser_plugin_info_x));

	/*initialize pkgmgr_paser db*/
	ret = _zone_pkgmgr_parser_plugin_open_db(zone);
	if (ret < 0)
		_LOGE("_pkgmgr_parser_plugin_open_db failed\n");

	/*save plugin_info_x*/
	plugin_info->mfx = mfx;
	plugin_info->pkgid = strdup(mfx->package);
	plugin_info->filename = strdup(filename);
	plugin_info->action = action;

	/*get plugin list from list-file, and process each plugin*/
	__process_all_plugins(plugin_info);

	/*update finished info to pkgmgr_parser db*/
	__process_plugin_db(plugin_info);

	/*close pkgmgr_paser db*/
	_pkgmgr_parser_plugin_close_db();

	/*clean plugin_info_x*/
	__clean_plugin_info(plugin_info);
}

void _pkgmgr_parser_plugin_process_plugin(manifest_x *mfx, const char *filename, ACTION_TYPE action)
{
	_zone_pkgmgr_parser_plugin_process_plugin(mfx, filename, action, NULL);
}

void _pkgmgr_parser_plugin_uninstall_plugin(const char *plugin_type, const char *pkgid, const char *appid)
{
	int ret = 0;
	FILE *fp = NULL;
	char buf[PKG_STRING_LEN_MAX] = {0};

	int plugin_flag = 0;
	int enabled_plugin = 0;
	pkgmgr_parser_plugin_info_x *plugin_info = NULL;

	retm_if(plugin_type == NULL, "plugin_type is null");
	retm_if(pkgid == NULL, "pkgid is null");
	retm_if(appid == NULL, "appid is null");

	/*initialize plugin_info_x*/
	plugin_info = malloc(sizeof(pkgmgr_parser_plugin_info_x));
	retm_if(plugin_info == NULL, "malloc fail");

	memset(plugin_info, '\0', sizeof(pkgmgr_parser_plugin_info_x));

	/*save plugin_info_x*/
	enabled_plugin = __ps_get_enabled_plugin(pkgid);
	plugin_info->pkgid = strdup(pkgid);
	plugin_info->appid = strdup(appid);
	plugin_info->action = ACTION_UNINSTALL;

	/*get plugin list from list-file*/
	fp = fopen(PLLUGIN_LIST, "r");

	if (fp == NULL) {
		_LOGE("Fail get : %s", PLLUGIN_LIST);
		__clean_plugin_info(plugin_info);
		return;
	}

	while (fgets(buf, PKG_STRING_LEN_MAX, fp) != NULL) {
		__str_trim(buf);

		ret = __get_plugin_info_x(buf, plugin_info);
		if (ret < 0)
			continue;

		/*process uninstallation for given plugin type*/
		if (strcmp(plugin_info->type, plugin_type) == 0) {

			plugin_flag = (int)strtol(plugin_info->flag, NULL, 16);
			
			/*if there is a plugin that is saved in db, it need to uninstall*/
			if (enabled_plugin & plugin_flag) {
				__process_each_plugin(plugin_info);
			}
		}

		memset(buf, 0x00, PKG_STRING_LEN_MAX);
		FREE_AND_NULL(plugin_info->type);
		FREE_AND_NULL(plugin_info->name);
		FREE_AND_NULL(plugin_info->flag);
		FREE_AND_NULL(plugin_info->path);
	}

	if (fp != NULL)
		fclose(fp);

	/*clean plugin_info_x*/
	__clean_plugin_info(plugin_info);

	return;
}

