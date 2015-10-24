/*
 * pkgmgrinfo-basic
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


#ifndef __PKGMGRINFO_BASIC_H__
#define __PKGMGRINFO_BASIC_H__

#include <glib.h>

#define DEFAULT_LOCALE		"No Locale"
#define PKG_STRING_LEN_MAX 1024
#define PKGMGR_PARSER_EMPTY_STR		""

#define ZONE_HOST "host"

typedef struct metadata_x {
	const char *key;
	const char *value;
} metadata_x;

typedef struct permission_x {
	const char *type;
	const char *value;
} permission_x;

typedef struct icon_x {
	const char *name;
	const char *text;
	const char *lang;
	const char *section;
	const char *size;
	const char *resolution;
} icon_x;

typedef struct image_x {
	const char *name;
	const char *text;
	const char *lang;
	const char *section;
} image_x;

typedef struct allowed_x {
	const char *name;
	const char *text;
} allowed_x;

typedef struct request_x {
	const char *text;
} request_x;

typedef struct define_x {
	const char *path;
	GList *allowed;
	GList *request;
} define_x;

typedef struct datashare_x {
	GList *define;
	GList *request;
} datashare_x;

typedef struct description_x {
	const char *name;
	const char *text;
	const char *lang;
} description_x;

typedef struct registry_x {
	const char *name;
	const char *text;
} registry_x;

typedef struct database_x {
	const char *name;
	const char *text;
} database_x;

typedef struct layout_x {
	const char *name;
	const char *text;
} layout_x;

typedef struct label_x {
	const char *name;
	const char *text;
	const char *lang;
} label_x;

typedef struct author_x {
	const char *email;
	const char *href;
	const char *text;
	const char *lang;
} author_x;

typedef struct license_x {
	const char *text;
	const char *lang;
} license_x;

typedef struct operation_x {
	const char *name;
	const char *text;
} operation_x;

typedef struct uri_x {
	const char *name;
	const char *text;
} uri_x;

typedef struct mime_x {
	const char *name;
	const char *text;
} mime_x;

typedef struct subapp_x {
	const char *name;
	const char *text;
} subapp_x;

typedef struct condition_x {
	const char *name;
	const char *text;
} condition_x;

typedef struct notification_x {
	const char *name;
	const char *text;
} notification_x;

typedef struct appsvc_x {
	const char *text;
	GList *operation;
	GList *uri;
	GList *mime;
	GList *subapp;
} appsvc_x;

typedef struct category_x {
	const char *name;
} category_x;

typedef struct launchconditions_x {
	const char *text;
	GList *condition;
} launchconditions_x;

typedef struct compatibility_x {
	const char *name;
	const char *text;
} compatibility_x;

typedef struct deviceprofile_x {
	const char *name;
	const char *text;
} deviceprofile_x;

typedef struct resolution_x {
	const char *mimetype;
	const char *urischeme;
} resolution_x;

typedef struct capability_x {
	const char *operationid;
	const char *access;
	GList *resolution;
} capability_x;

typedef struct datacontrol_x {
	const char *providerid;
	const char *access;
	const char *type;
} datacontrol_x;

typedef struct appcontrol_x {
	const char *operation;
	const char *uri;
	const char *mime;
} appcontrol_x;

typedef struct uiapplication_x {
	const char *appid;
	const char *exec;
	const char *nodisplay;
	const char *multiple;
	const char *taskmanage;
	const char *enabled;
	const char *type;
	const char *categories;
	const char *extraid;
	const char *hwacceleration;
	const char *screenreader;
	const char *mainapp;
	const char *package;
	const char *recentimage;
	const char *launchcondition;
	const char *indicatordisplay;
	const char *portraitimg;
	const char *landscapeimg;
	const char *effectimage_type;
	const char *guestmode_visibility;
	const char *app_component;
	const char *permission_type;
	const char *component_type;
	const char *preload;
	const char *submode;
	const char *submode_mainid;
	const char *installed_storage;
	const char *process_pool;
	const char *autorestart;
	const char *onboot;
	const char *multi_instance;
	const char *multi_instance_mainid;
	const char *multi_window;
	const char *support_disable;
	const char *ui_gadget;
	const char *removable;
	const char *companion_type;
	const char *support_mode;
	const char *support_feature;
	const char *support_category;
	const char *satui_label;
	const char *package_type;
	const char *package_system;
	const char *package_installed_time;
	const char *launch_mode;
	const char *alias_appid;
	const char *effective_appid;
	const char *api_version;
#ifdef _APPFW_FEATURE_EXPANSION_PKG_INSTALL
	const char *tep_name;
#endif
	GList *label;
	GList *icon;
	GList *image;
	GList *appsvc;
	GList *category;
	GList *metadata;
	GList *permission;
	GList *launchconditions;
	GList *notification;
	GList *datashare;
	GList *datacontrol;
	GList *background_category;
	GList *appcontrol;
#ifdef _APPFW_FEATURE_MOUNT_INSTALL
	int ismount;
	const char *tpk_name;
#endif
} uiapplication_x;

typedef struct serviceapplication_x {
	const char *appid;
	const char *exec;
	const char *onboot;
	const char *autorestart;
	const char *enabled;
	const char *type;
	const char *package;
	const char *permission_type;
	GList *label;
	GList *icon;
	GList *appsvc;
	GList *category;
	GList *metadata;
	GList *permission;
	GList *datacontrol;
	GList *launchconditions;
	GList *notification;
	GList *datashare;
	GList *background_category;
} serviceapplication_x;

typedef struct daemon_x {
	const char *name;
	const char *text;
} daemon_x;

typedef struct theme_x {
	const char *name;
	const char *text;
} theme_x;

typedef struct font_x {
	const char *name;
	const char *text;
} font_x;

typedef struct ime_x {
	const char *name;
	const char *text;
} ime_x;

typedef struct manifest_x {
	const char *package;		/**< package name*/
	const char *version;		/**< package version*/
	const char *installlocation;		/**< package install location*/
	const char *ns;		/**<name space*/
	const char *removable;		/**< package removable flag*/
	const char *preload;		/**< package preload flag*/
	const char *readonly;		/**< package readonly flag*/
	const char *update;			/**< package update flag*/
	const char *appsetting;		/**< package app setting flag*/
	const char *system;		/**< package system flag*/
	const char *type;		/**< package type*/
	const char *package_size;		/**< package size for external installation*/
	const char *package_total_size;		/**< package size for total*/
	const char *package_data_size;		/**< package size for data*/
	const char *installed_time;		/**< installed time after finishing of installation*/
	const char *installed_storage;		/**< package currently installed storage*/
	const char *storeclient_id;		/**< id of store client for installed package*/
	const char *mainapp_id;		/**< app id of main application*/
	const char *package_url;		/**< app id of main application*/
	const char *root_path;		/**< package root path*/
	const char *csc_path;		/**< package csc path*/
	const char *nodisplay_setting;		/**< package no display setting menu*/
	const char *support_disable;		/**< package support disable flag*/
	const char *mother_package;		/**< package is mother package*/
	const char *api_version;		/**< minimum version of API package using*/
#ifdef _APPFW_FEATURE_EXPANSION_PKG_INSTALL
	const char *tep_name;
#endif
	const char *support_mode;		/**< package support mode*/
	const char *backend_installer;		/**< package backend installer*/
	const char *custom_smack_label;		/**< package custom smack label*/
	const char *groupid;		/**< package groupid*/
	GList *icon;		/**< package icon*/
	GList *label;		/**< package label*/
	GList *author;		/**< package author*/
	GList *description;		/**< package description*/
	GList *license;		/**< package license*/
	GList *privileges;	/**< package privileges*/
	GList *uiapplication;		/**< package's ui application*/
	GList *serviceapplication;		/**< package's service application*/
	GList *daemon;		/**< package daemon*/
	GList *theme;		/**< package theme*/
	GList *font;		/**< package font*/
	GList *ime;		/**< package ime*/
	GList *compatibility;		/**< package compatibility*/
	GList *deviceprofile;		/**< package device profile*/
} manifest_x;

/**
 * @brief List definitions.
 * All lists are doubly-linked, the last element is stored to list pointer,
 * which means that lists must be looped using the prev pointer, or by
 * calling LISTHEAD first to go to start in order to use the next pointer.
 */

 /**
 * @brief Convinience Macro to add node in list
 */

#define LISTADD(list, node)			\
    do {					\
	(node)->prev = (list);			\
	if (list) (node)->next = (list)->next;	\
	else (node)->next = NULL;		\
	if (list) (list)->next = (node);	\
	(list) = (node);			\
    } while (0);

 /**
 * @brief Convinience Macro to add one node to another node
 */
#define NODEADD(node1, node2)					\
    do {							\
	(node2)->prev = (node1);				\
	(node2)->next = (node1)->next;				\
	if ((node1)->next) (node1)->next->prev = (node2);	\
	(node1)->next = (node2);				\
    } while (0);

 /**
 * @brief Convinience Macro to concatenate two lists
 */
#define LISTCAT(list, first, last)		\
    if ((first) && (last)) {			\
	(first)->prev = (list);			\
	(list) = (last);			\
    }

 /**
 * @brief Convinience Macro to delete node from list
 */
#define LISTDEL(list, node)					\
    do {							\
	if ((node)->prev) (node)->prev->next = (node)->next;	\
	if ((node)->next) (node)->next->prev = (node)->prev;	\
	if (!((node)->prev) && !((node)->next)) (list) = NULL;	\
    } while (0);

 /**
 * @brief Convinience Macro to get list head
 */
#define LISTHEAD(list, node)					\
    for ((node) = (list); (node)->prev; (node) = (node)->prev)

#define SAFE_LISTHEAD(list, node)	 do { \
	if (list) { \
		for ((node) = (list); (node)->prev; (node) = (node)->prev); \
		list = node; \
	} \
} while (0)

 /**
 * @brief Convinience Macro to get list tail
 */
#define LISTTAIL(list, node)					\
    for ((node) = (list); (node)->next; (node) = (node)->next)

#define FREE_AND_STRDUP(from, to) do { \
		if (to) free((void *)to); \
		if (from) to = strdup(from); \
	} while (0)

 /**
 * @brief Convinience Macro to free pointer
 */
#define FREE_AND_NULL(ptr) do { \
		if (ptr) { \
			free((void *)ptr); \
			ptr = NULL; \
		} \
	} while (0)

void _pkgmgrinfo_basic_free_manifest_x(manifest_x *mfx);
void _pkgmgrinfo_basic_free_uiapplication_x(uiapplication_x *uiapplication);


#endif  /* __PKGMGRINFO_BASIC_H__ */
