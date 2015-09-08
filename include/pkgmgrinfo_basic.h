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

#define DEFAULT_LOCALE		"No Locale"
#define PKG_STRING_LEN_MAX 1024
#define PKGMGR_PARSER_EMPTY_STR		""

typedef struct metadata_x {
	const char *key;
	const char *value;
	struct metadata_x *prev;
	struct metadata_x *next;
} metadata_x;

typedef struct privilege_x {
	const char *text;
	struct privilege_x *prev;
	struct privilege_x *next;
} privilege_x;

typedef struct privileges_x {
	struct privilege_x *privilege;
	struct privileges_x *prev;
	struct privileges_x *next;
} privileges_x;

typedef struct permission_x {
	const char *type;
	const char *value;
	struct permission_x *prev;
	struct permission_x *next;
} permission_x;

typedef struct icon_x {
	const char *name;
	const char *text;
	const char *lang;
	const char *section;
	const char *size;
	const char *resolution;
	struct icon_x *prev;
	struct icon_x *next;
} icon_x;

typedef struct image_x {
	const char *name;
	const char *text;
	const char *lang;
	const char *section;
	struct image_x *prev;
	struct image_x *next;
} image_x;

typedef struct allowed_x {
	const char *name;
	const char *text;
	struct allowed_x *prev;
	struct allowed_x *next;
} allowed_x;

typedef struct request_x {
	const char *text;
	struct request_x *prev;
	struct request_x *next;
} request_x;

typedef struct define_x {
	const char *path;
	struct allowed_x *allowed;
	struct request_x *request;
	struct define_x *prev;
	struct define_x *next;
} define_x;

typedef struct datashare_x {
	struct define_x *define;
	struct request_x *request;
	struct datashare_x *prev;
	struct datashare_x *next;
} datashare_x;

typedef struct description_x {
	const char *name;
	const char *text;
	const char *lang;
	struct description_x *prev;
	struct description_x *next;
} description_x;

typedef struct registry_x {
	const char *name;
	const char *text;
	struct registry_x *prev;
	struct registry_x *next;
} registry_x;

typedef struct database_x {
	const char *name;
	const char *text;
	struct database_x *prev;
	struct database_x *next;
} database_x;

typedef struct layout_x {
	const char *name;
	const char *text;
	struct layout_x *prev;
	struct layout_x *next;
} layout_x;

typedef struct label_x {
	const char *name;
	const char *text;
	const char *lang;
	struct label_x *prev;
	struct label_x *next;
} label_x;

typedef struct author_x {
	const char *email;
	const char *href;
	const char *text;
	const char *lang;
	struct author_x *prev;
	struct author_x *next;
} author_x;

typedef struct license_x {
	const char *text;
	const char *lang;
	struct license_x *prev;
	struct license_x *next;
} license_x;

typedef struct operation_x {
	const char *name;
	const char *text;
	struct operation_x *prev;
	struct operation_x *next;
} operation_x;

typedef struct uri_x {
	const char *name;
	const char *text;
	struct uri_x *prev;
	struct uri_x *next;
} uri_x;

typedef struct mime_x {
	const char *name;
	const char *text;
	struct mime_x *prev;
	struct mime_x *next;
} mime_x;

typedef struct subapp_x {
	const char *name;
	const char *text;
	struct subapp_x *prev;
	struct subapp_x *next;
} subapp_x;

typedef struct condition_x {
	const char *name;
	const char *text;
	struct condition_x *prev;
	struct condition_x *next;
} condition_x;

typedef struct notification_x {
	const char *name;
	const char *text;
	struct notification_x *prev;
	struct notification_x *next;
} notification_x;

typedef struct appsvc_x {
	const char *text;
	struct operation_x *operation;
	struct uri_x *uri;
	struct mime_x *mime;
	struct subapp_x *subapp;
	struct appsvc_x *prev;
	struct appsvc_x *next;
} appsvc_x;

typedef struct category_x{
	const char *name;
	struct category_x *prev;
	struct category_x *next;
} category_x;

typedef struct launchconditions_x {
	const char *text;
	struct condition_x *condition;
	struct launchconditions_x *prev;
	struct launchconditions_x *next;
} launchconditions_x;

typedef struct compatibility_x {
	const char *name;
	const char *text;
	struct compatibility_x *prev;
	struct compatibility_x *next;
}compatibility_x;

typedef struct deviceprofile_x {
	const char *name;
	const char *text;
	struct deviceprofile_x *prev;
	struct deviceprofile_x *next;
}deviceprofile_x;

typedef struct resolution_x {
	const char *mimetype;
	const char *urischeme;
	struct resolution_x *prev;
	struct resolution_x *next;
} resolution_x;

typedef struct capability_x {
	const char *operationid;
	const char *access;
	struct resolution_x *resolution;
	struct capability_x *prev;
	struct capability_x *next;
} capability_x;

typedef struct datacontrol_x {
	const char *providerid;
	const char *access;
	const char *type;
	struct datacontrol_x *prev;
	struct datacontrol_x *next;
} datacontrol_x;

typedef struct uiapplication_x {
	const char *appid;
	const char *exec;
	const char *ambient_support;
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
	const char *support_mode;
	const char *support_feature;
	const char *satui_label;
	const char *package_type;
	const char *package_system;
	const char *package_installed_time;
	struct label_x *label;
	struct icon_x *icon;
	struct image_x *image;
	struct appsvc_x *appsvc;
	struct category_x *category;
	struct metadata_x *metadata;
	struct permission_x *permission;
	struct launchconditions_x *launchconditions;
	struct notification_x *notification;
	struct datashare_x *datashare;
	struct datacontrol_x *datacontrol;
	struct uiapplication_x *prev;
	struct uiapplication_x *next;
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
	struct label_x *label;
	struct icon_x *icon;
	struct appsvc_x *appsvc;
	struct category_x *category;
	struct metadata_x *metadata;
	struct permission_x *permission;
	struct datacontrol_x *datacontrol;
	struct launchconditions_x *launchconditions;
	struct notification_x *notification;
	struct datashare_x *datashare;
	struct serviceapplication_x *prev;
	struct serviceapplication_x *next;
} serviceapplication_x;

typedef struct daemon_x {
	const char *name;
	const char *text;
	struct daemon_x *prev;
	struct daemon_x *next;
} daemon_x;

typedef struct theme_x {
	const char *name;
	const char *text;
	struct theme_x *prev;
	struct theme_x *next;
} theme_x;

typedef struct font_x {
	const char *name;
	const char *text;
	struct font_x *prev;
	struct font_x *next;
} font_x;

typedef struct ime_x {
	const char *name;
	const char *text;
	struct ime_x *prev;
	struct ime_x *next;
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
	const char *support_mode;		/**< package support mode*/
	const char *support_reset;		/**< package support reset*/
	const char *use_reset;		/**< package use reset*/
	const char *groupid;		/**< package groupid*/
	const char *hash;			/**<manifest file's hash */
	struct icon_x *icon;		/**< package icon*/
	struct label_x *label;		/**< package label*/
	struct author_x *author;		/**< package author*/
	struct description_x *description;		/**< package description*/
	struct license_x *license;		/**< package license*/
	struct privileges_x *privileges;	/**< package privileges*/
	struct uiapplication_x *uiapplication;		/**< package's ui application*/
	struct serviceapplication_x *serviceapplication;		/**< package's service application*/
	struct daemon_x *daemon;		/**< package daemon*/
	struct theme_x *theme;		/**< package theme*/
	struct font_x *font;		/**< package font*/
	struct ime_x *ime;		/**< package ime*/
	struct compatibility_x *compatibility;		/**< package compatibility*/
	struct deviceprofile_x *deviceprofile;		/**< package device profile*/
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

char* pkgmgrinfo_basic_generate_hash_for_file(const char* file);

#endif  /* __PKGMGRINFO_BASIC_H__ */
