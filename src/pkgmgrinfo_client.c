/*
 * pkgmgrinfo-client
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>, Junsuk Oh <junsuk77.oh@samsung.com>,
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
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "pkgmgrinfo_private.h"
#include "pkgmgrinfo_debug.h"
#include "pkgmgr-info.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_CLIENT"

#define SERVICE_NAME "org.tizen.system.deviced"
#define PATH_NAME "/Org/Tizen/System/DeviceD/Mmc"
#define INTERFACE_NAME "org.tizen.system.deviced.Mmc"
#define METHOD_NAME "RequestMountApp2ext"

static int __get_pkg_location(const char *pkgid)
{
	retvm_if(pkgid == NULL, PMINFO_R_OK, "pkginfo handle is NULL");

	FILE *fp = NULL;
	char pkg_mmc_path[FILENAME_MAX] = { 0, };
	snprintf(pkg_mmc_path, FILENAME_MAX, "%s%s", PKG_SD_PATH, pkgid);

	/*check whether application is in external memory or not */
	fp = fopen(pkg_mmc_path, "r");
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
		return PMINFO_EXTERNAL_STORAGE;
	}

	return PMINFO_INTERNAL_STORAGE;
}

/* pkgmgrinfo client start*/
API pkgmgrinfo_client *pkgmgrinfo_client_new(pkgmgrinfo_client_type ctype)
{
	char *errmsg = NULL;
	void *pc = NULL;
	void *handle = NULL;
	pkgmgrinfo_client *(*__pkgmgr_client_new)(pkgmgrinfo_client_type ctype) = NULL;

	handle = dlopen("libpkgmgr-client.so.0", RTLD_LAZY | RTLD_GLOBAL);
	retvm_if(!handle, NULL, "dlopen() failed. [%s]", dlerror());

	__pkgmgr_client_new = dlsym(handle, "pkgmgr_client_new");
	retvm_if(__pkgmgr_client_new == NULL, NULL, "__pkgmgr_client_new() failed");

	errmsg = dlerror();
	retvm_if(errmsg != NULL, NULL, "dlsym() failed. [%s]", errmsg);

	pc = __pkgmgr_client_new(ctype);

//catch:
//	dlclose(handle);
	return (pkgmgrinfo_client *) pc;
}

API int pkgmgrinfo_client_set_status_type(pkgmgrinfo_client *pc, int status_type)
{
	int ret = 0;
	char *errmsg = NULL;
	void *handle = NULL;
	int (*__pkgmgr_client_set_status_type)(pkgmgrinfo_client *pc, int status_type) = NULL;

	handle = dlopen("libpkgmgr-client.so.0", RTLD_LAZY | RTLD_GLOBAL);
	retvm_if(!handle, PMINFO_R_ERROR, "dlopen() failed. [%s]", dlerror());

	__pkgmgr_client_set_status_type = dlsym(handle, "pkgmgr_client_set_status_type");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__pkgmgr_client_set_status_type == NULL), ret = PMINFO_R_ERROR, "dlsym() failed. [%s]", errmsg);

	ret = __pkgmgr_client_set_status_type(pc, status_type);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "pkgmgr_client_new failed.");

catch:
//	dlclose(handle);
	return ret;
}

API int pkgmgrinfo_client_listen_status(pkgmgrinfo_client *pc, pkgmgrinfo_handler event_cb, void *data)
{
	int ret = 0;
	char *errmsg = NULL;
	void *handle = NULL;
	int (*__pkgmgr_client_listen_status)(pkgmgrinfo_client *pc, pkgmgrinfo_handler event_cb, void *data) = NULL;

	handle = dlopen("libpkgmgr-client.so.0", RTLD_LAZY | RTLD_GLOBAL);
	retvm_if(!handle, PMINFO_R_ERROR, "dlopen() failed. [%s]", dlerror());

	__pkgmgr_client_listen_status = dlsym(handle, "pkgmgr_client_listen_status");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__pkgmgr_client_listen_status == NULL), ret = PMINFO_R_ERROR, "dlsym() failed. [%s]", errmsg);

	ret = __pkgmgr_client_listen_status(pc, event_cb, data);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "pkgmgr_client_new failed.");

catch:
//	dlclose(handle);
	return ret;
}

API int pkgmgrinfo_client_listen_status_with_zone(pkgmgrinfo_client *pc, pkgmgrinfo_handler_zone event_cb, void *data)
{
	int ret = 0;
	char *errmsg = NULL;
	void *handle = NULL;
	int (*__pkgmgr_client_listen_status_with_zone)(pkgmgrinfo_client *pc, pkgmgrinfo_handler_zone event_cb, void *data) = NULL;

	handle = dlopen("libpkgmgr-client.so.0", RTLD_LAZY | RTLD_GLOBAL);
	retvm_if(!handle, PMINFO_R_ERROR, "dlopen() failed. [%s]", dlerror());

	__pkgmgr_client_listen_status_with_zone = dlsym(handle, "pkgmgr_client_listen_status_with_zone");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__pkgmgr_client_listen_status_with_zone == NULL), ret = PMINFO_R_ERROR, "dlsym() failed. [%s]", errmsg);

	ret = __pkgmgr_client_listen_status_with_zone(pc, event_cb, data);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "pkgmgr_client_new failed.");

catch:
//	dlclose(handle);
	return ret;
}

API int pkgmgrinfo_client_free(pkgmgrinfo_client *pc)
{
	int ret = 0;
	char *errmsg = NULL;
	void *handle = NULL;
	int (*__pkgmgr_client_free)(pkgmgrinfo_client *pc) = NULL;

	handle = dlopen("libpkgmgr-client.so.0", RTLD_LAZY | RTLD_GLOBAL);
	retvm_if(!handle, PMINFO_R_ERROR, "dlopen() failed. [%s]", dlerror());

	__pkgmgr_client_free = dlsym(handle, "pkgmgr_client_free");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__pkgmgr_client_free == NULL), ret = PMINFO_R_ERROR, "dlsym() failed. [%s]", errmsg);

	ret = __pkgmgr_client_free(pc);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "pkgmgr_client_new failed.");

catch:
//	dlclose(handle);
	return ret;
}

API int pkgmgrinfo_client_request_enable_external_pkg(char *pkgid)
{
	int ret = 0;
	DBusConnection *bus;
	DBusMessage *message;
	DBusMessage *reply;

	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "pkgid is NULL\n");

	if(__get_pkg_location(pkgid) != PMINFO_EXTERNAL_STORAGE)
		return PMINFO_R_OK;

	bus = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	retvm_if(bus == NULL, PMINFO_R_EINVAL, "dbus_bus_get() failed.");

	message = dbus_message_new_method_call (SERVICE_NAME, PATH_NAME, INTERFACE_NAME, METHOD_NAME);
	retvm_if(message == NULL, PMINFO_R_EINVAL, "dbus_message_new_method_call() failed.");

	dbus_message_append_args(message, DBUS_TYPE_STRING, &pkgid, DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block(bus, message, -1, NULL);
	retvm_if(!reply, ret = PMINFO_R_EINVAL, "connection_send dbus fail");

	dbus_connection_flush(bus);
	dbus_message_unref(message);
	dbus_message_unref(reply);

	return PMINFO_R_OK;
}

/* pkgmgrinfo client end*/

