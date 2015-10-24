/*
 * pkgmgrinfo-basic
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

#include <stdlib.h>
#include <string.h>

#include "pkgmgrinfo_basic.h"
#include "pkgmgrinfo_private.h"
#include <openssl/md5.h>



static void __ps_free_category(category_x *category)
{
	if (category == NULL)
		return;
	FREE_AND_NULL(category->name);
	FREE_AND_NULL(category);
}


static void __ps_free_metadata(metadata_x *metadata)
{
	if (metadata == NULL)
		return;
	FREE_AND_NULL(metadata->key);
	FREE_AND_NULL(metadata->value);
	FREE_AND_NULL(metadata);
}

static void __ps_free_permission(permission_x *permission)
{
	if (permission == NULL)
		return;
	FREE_AND_NULL(permission->type);
	FREE_AND_NULL(permission->value);
	FREE_AND_NULL(permission);
}

static void __ps_free_icon(icon_x *icon)
{
	if (icon == NULL)
		return;
	FREE_AND_NULL(icon->text);
	FREE_AND_NULL(icon->lang);
	FREE_AND_NULL(icon->name);
	FREE_AND_NULL(icon->section);
	FREE_AND_NULL(icon->size);
	FREE_AND_NULL(icon->resolution);
	FREE_AND_NULL(icon);
}

static void __ps_free_image(image_x *image)
{
	if (image == NULL)
		return;
	FREE_AND_NULL(image->text);
	FREE_AND_NULL(image->lang);
	FREE_AND_NULL(image->name);
	FREE_AND_NULL(image->section);
	FREE_AND_NULL(image);
}

static void __ps_free_operation(operation_x *operation)
{
	if (operation == NULL)
		return;
	FREE_AND_NULL(operation->text);
	FREE_AND_NULL(operation->name);
	FREE_AND_NULL(operation);
}

static void __ps_free_uri(uri_x *uri)
{
	if (uri == NULL)
		return;
	FREE_AND_NULL(uri->text);
	FREE_AND_NULL(uri->name);
	FREE_AND_NULL(uri);
}

static void __ps_free_mime(mime_x *mime)
{
	if (mime == NULL)
		return;
	FREE_AND_NULL(mime->text);
	FREE_AND_NULL(mime->name);
	FREE_AND_NULL(mime);
}

static void __ps_free_subapp(subapp_x *subapp)
{
	if (subapp == NULL)
		return;
	FREE_AND_NULL(subapp->text);
	FREE_AND_NULL(subapp->name);
	FREE_AND_NULL(subapp);
}

static void __ps_free_condition(condition_x *condition)
{
	if (condition == NULL)
		return;
	FREE_AND_NULL(condition->text);
	FREE_AND_NULL(condition->name);
	FREE_AND_NULL(condition);
}

static void __ps_free_notification(notification_x *notification)
{
	if (notification == NULL)
		return;
	FREE_AND_NULL(notification->text);
	FREE_AND_NULL(notification->name);
	FREE_AND_NULL(notification);
}

static void __ps_free_compatibility(compatibility_x *compatibility)
{
	if (compatibility == NULL)
		return;
	FREE_AND_NULL(compatibility->text);
	FREE_AND_NULL(compatibility->name);
	FREE_AND_NULL(compatibility);
}

static void __ps_free_allowed(allowed_x *allowed)
{
	if (allowed == NULL)
		return;
	FREE_AND_NULL(allowed->name);
	FREE_AND_NULL(allowed->text);
	FREE_AND_NULL(allowed);
}

static void __ps_free_request(request_x *request)
{
	if (request == NULL)
		return;
	FREE_AND_NULL(request->text);
	FREE_AND_NULL(request);
}

static void __ps_free_datacontrol(datacontrol_x *datacontrol)
{
	if (datacontrol == NULL)
		return;
	FREE_AND_NULL(datacontrol->providerid);
	FREE_AND_NULL(datacontrol->access);
	FREE_AND_NULL(datacontrol->type);
	FREE_AND_NULL(datacontrol);
}

static void __ps_free_appcontrol(appcontrol_x *appcontrol)
{
	if (appcontrol == NULL)
		return;
	FREE_AND_NULL(appcontrol->operation);
	FREE_AND_NULL(appcontrol->uri);
	FREE_AND_NULL(appcontrol->mime);
	FREE_AND_NULL(appcontrol);
}

static void __ps_free_launchconditions(launchconditions_x *launchconditions)
{
	if (launchconditions == NULL)
		return;
	FREE_AND_NULL(launchconditions->text);
	GList *cond_list = NULL;
	/*Free Condition*/
	if (launchconditions->condition) {
		cond_list = launchconditions->condition;
		condition_x *condition = NULL;
		while(cond_list != NULL) {
			condition = (condition_x*)cond_list->data;
			__ps_free_condition(condition);
			cond_list = cond_list->next;
		}
		g_list_free(launchconditions->condition);
	}
	FREE_AND_NULL(launchconditions);
}

static void __ps_free_appsvc(appsvc_x *appsvc)
{
	if (appsvc == NULL)
		return;
	FREE_AND_NULL(appsvc->text);

	/*Free Operation*/
	if (appsvc->operation) {
		GList *list_op = appsvc->operation;
		operation_x *operation = NULL;

		while(list_op!= NULL) {
			operation = (operation_x *)list_op->data;
			__ps_free_operation(operation);
			list_op = list_op->next;
		}
		g_list_free(appsvc->operation);
		list_op = NULL;
		operation = NULL;
	}

	/*Free Uri*/
	if (appsvc->uri) {
		GList *list_uri = appsvc->uri;
		uri_x *uri = NULL;
		while(list_uri != NULL) {
			uri = (uri_x *)list_uri->data;
			__ps_free_uri(uri);
			list_uri = list_uri->next;
		}
		g_list_free(appsvc->uri);
		list_uri = NULL;
		uri = NULL;
	}

	/*Free Mime*/
	if (appsvc->mime) {
		GList *list_mime = appsvc->mime;
		mime_x *mime = NULL;
		while(list_mime != NULL) {
			mime = (mime_x *)list_mime->data;
			__ps_free_mime(mime);
			list_mime = list_mime->next;
		}
		g_list_free(appsvc->mime);
		list_mime = NULL;
		mime = NULL;
	}

	/*Free subapp*/
	if (appsvc->subapp) {
		GList *list_subapp = appsvc->subapp;
		subapp_x *subapp = NULL;
		while(list_subapp != NULL) {
			subapp = (subapp_x *)list_subapp->data;
			__ps_free_subapp(subapp);
			list_subapp = list_subapp->next;
		}
		g_list_free(appsvc->subapp);
		list_subapp = NULL;
		subapp = NULL;
	}

	FREE_AND_NULL(appsvc);
}

static void __ps_free_deviceprofile(deviceprofile_x *deviceprofile)
{
	return;
}

static void __ps_free_define(define_x *define)
{
	if (define == NULL)
		return;
	FREE_AND_NULL(define->path);
	GList *tmp = NULL;
	/*Free Request*/
	if (define->request) {
		tmp = define->request;
		request_x *request = NULL;
		while(tmp != NULL) {
			request = (request_x*)tmp->data;
			__ps_free_request(request);
			tmp = tmp->next;
		}
		g_list_free(define->request);
	}
	/*Free Allowed*/
	if (define->allowed) {
		tmp = define->allowed;
		allowed_x *allowed = NULL;
		while(tmp != NULL) {
			allowed = (allowed_x*)tmp->data;
			__ps_free_allowed(allowed);
			tmp = tmp->next;
		}
		g_list_free(define->allowed);
	}
	FREE_AND_NULL(define);
}

static void __ps_free_datashare(datashare_x *datashare)
{
	if (datashare == NULL)
		return;
	/*Free Define*/
	GList *tmp = NULL;
	if (datashare->define) {
		tmp = datashare->define;
		define_x *define =  NULL;
		while(tmp != NULL) {
			define = (define_x*)tmp->data;
			__ps_free_define(define);
			tmp = tmp->next;
		}
		g_list_free(datashare->define);
	}
	/*Free Request*/
	if (datashare->request) {
		tmp = datashare->request;
		request_x *request = NULL;
		while(tmp != NULL) {
			request = (request_x*)tmp->data;
			__ps_free_request(request);
			tmp = tmp->next;
		}
		g_list_free(datashare->request);
	}
	FREE_AND_NULL(datashare);
}

static void __ps_free_label(label_x *label)
{
	if (label == NULL)
		return;
	FREE_AND_NULL(label->name);
	FREE_AND_NULL(label->text);
	FREE_AND_NULL(label->lang);
	FREE_AND_NULL(label);
}

static void __ps_free_author(author_x *author)
{
	if (author == NULL)
		return;
	FREE_AND_NULL(author->email);
	FREE_AND_NULL(author->text);
	FREE_AND_NULL(author->href);
	FREE_AND_NULL(author->lang);
	FREE_AND_NULL(author);
}

static void __ps_free_description(description_x *description)
{
	if (description == NULL)
		return;
	FREE_AND_NULL(description->name);
	FREE_AND_NULL(description->text);
	FREE_AND_NULL(description->lang);
	FREE_AND_NULL(description);
}

static void __ps_free_license(license_x *license)
{
	if (license == NULL)
		return;
	FREE_AND_NULL(license->text);
	FREE_AND_NULL(license->lang);
	FREE_AND_NULL(license);
}

void _pkgmgrinfo_basic_free_uiapplication_x(uiapplication_x *uiapplication)
{
	if (uiapplication == NULL)
		return;
	GList *tmp = NULL;
	FREE_AND_NULL(uiapplication->exec);
	FREE_AND_NULL(uiapplication->appid);
	FREE_AND_NULL(uiapplication->nodisplay);
	FREE_AND_NULL(uiapplication->multiple);
	FREE_AND_NULL(uiapplication->type);
	FREE_AND_NULL(uiapplication->categories);
	FREE_AND_NULL(uiapplication->extraid);
	FREE_AND_NULL(uiapplication->taskmanage);
	FREE_AND_NULL(uiapplication->enabled);
	FREE_AND_NULL(uiapplication->hwacceleration);
	FREE_AND_NULL(uiapplication->screenreader);
	FREE_AND_NULL(uiapplication->mainapp);
	FREE_AND_NULL(uiapplication->recentimage);
	FREE_AND_NULL(uiapplication->package);
	FREE_AND_NULL(uiapplication->launchcondition);
	FREE_AND_NULL(uiapplication->multi_instance);
	FREE_AND_NULL(uiapplication->multi_instance_mainid);
	FREE_AND_NULL(uiapplication->multi_window);
	FREE_AND_NULL(uiapplication->support_disable);
	FREE_AND_NULL(uiapplication->ui_gadget);
	FREE_AND_NULL(uiapplication->removable);
	FREE_AND_NULL(uiapplication->companion_type);
	FREE_AND_NULL(uiapplication->support_mode);
	FREE_AND_NULL(uiapplication->support_feature);
	FREE_AND_NULL(uiapplication->support_category);
	FREE_AND_NULL(uiapplication->satui_label);
	FREE_AND_NULL(uiapplication->package_type);
	FREE_AND_NULL(uiapplication->package_system);
	FREE_AND_NULL(uiapplication->package_installed_time);
	FREE_AND_NULL(uiapplication->launch_mode);
	FREE_AND_NULL(uiapplication->alias_appid);
	FREE_AND_NULL(uiapplication->effective_appid);
	FREE_AND_NULL(uiapplication->api_version);
#ifdef _APPFW_FEATURE_EXPANSION_PKG_INSTALL
	FREE_AND_NULL(uiapplication->tep_name);
#endif
#ifdef _APPFW_FEATURE_MOUNT_INSTALL
	FREE_AND_NULL(uiapplication->tpk_name);
#endif

	/*Free Label*/
	if (uiapplication->label) {
		tmp = uiapplication->label;
		while(tmp != NULL){
			label_x *label = (label_x*)tmp->data;
			__ps_free_label(label);
			tmp = tmp->next;
		}
		g_list_free(uiapplication->label);
	}
	/*Free Icon*/
	if (uiapplication->icon) {
		tmp = uiapplication->icon;
		while(tmp != NULL){
			icon_x* icon = (icon_x*)tmp->data;
			__ps_free_icon(icon);
			tmp = tmp->next;
		}
		g_list_free(uiapplication->icon);
	}
	/*Free image*/
	if (uiapplication->image) {
		tmp = uiapplication->image;
		while(tmp != NULL){
			image_x *image = (image_x*)tmp->data;
			__ps_free_image(image);
			tmp = tmp->next;
		}
		g_list_free(uiapplication->image);
	}
	/*Free LaunchConditions*/
	if (uiapplication->launchconditions) {
		tmp = uiapplication->launchconditions;
		while(tmp != NULL) {
			launchconditions_x *launchconditions = (launchconditions_x*)tmp->data;
			__ps_free_launchconditions(launchconditions);
			tmp = tmp->next;
		}
		g_list_free(uiapplication->launchconditions);
	}
	/*Free Notification*/
	if (uiapplication->notification) {
		tmp = uiapplication->notification;
		notification_x *notification = NULL;
		while(tmp != NULL) {
			notification = (notification_x*)tmp->data;
			__ps_free_notification(notification);
			tmp = tmp->next;
		}
		g_list_free(uiapplication->notification);
	}
	/*Free DataShare*/
	if (uiapplication->datashare) {
		tmp = uiapplication->datashare;
		datashare_x *datashare = NULL;
		while(tmp != NULL) {
			datashare = (datashare_x*)tmp->data;
			__ps_free_datashare(datashare);
			tmp = tmp->next;
		}
		g_list_free(uiapplication->datashare);
	}

	/*Free AppSvc*/
	if (uiapplication->appsvc) {
		GList *list_asvc = uiapplication->appsvc;
		appsvc_x *appsvc = NULL;
		while(list_asvc != NULL) {
			appsvc = (appsvc_x *)list_asvc->data;
			__ps_free_appsvc(appsvc);
			list_asvc = list_asvc->next;
		}
		g_list_free(uiapplication->appsvc);
		list_asvc = NULL;
		appsvc = NULL;
	}

	/*Free Category*/
	if (uiapplication->category) {
		tmp  = uiapplication->category;
		category_x *category = NULL;
		while(tmp != NULL) {
			category = (category_x*)tmp->data;
			__ps_free_category(category);
			tmp = tmp->next;
		}
		g_list_free(uiapplication->category);
	}
	/*Free Metadata*/
	if (uiapplication->metadata) {
		GList *list_md = uiapplication->metadata;
		metadata_x *metadata = NULL;
		while(list_md != NULL) {
			metadata = (metadata_x *)list_md->data;
			__ps_free_metadata(metadata);
			list_md = list_md->next;
		}
		g_list_free(uiapplication->metadata);
		list_md = NULL;
		metadata = NULL;
	}

	/*Free permission*/
	if (uiapplication->permission) {
		tmp = uiapplication->permission;
		permission_x *permission = NULL;
		while(tmp != NULL) {
			permission = (permission_x*)tmp->data;
			__ps_free_permission(permission);
			tmp = tmp->next;
		}
		g_list_free(uiapplication->permission);
	}

	/*Free datacontrol*/
	if (uiapplication->datacontrol) {
		tmp = uiapplication->datacontrol;
		while(tmp != NULL) {
			datacontrol_x *datacontrol = (datacontrol_x*)tmp->data;
			__ps_free_datacontrol(datacontrol);
			tmp = tmp->next;
		}
		g_list_free(uiapplication->datacontrol);
	}

	/*Free app background category*/
	if (uiapplication->background_category) {
		tmp = uiapplication->background_category;
		char *category_tmp = NULL;
		while (tmp != NULL) {
			category_tmp = (char *)tmp->data;
			if (category_tmp != NULL) {
				free(category_tmp);
				category_tmp = NULL;
			}
			tmp = g_list_next(tmp);
		}
		g_list_free(uiapplication->background_category);
	}

	/*Free app control*/
	if (uiapplication->appcontrol) {
		tmp = uiapplication->appcontrol;
		while (tmp != NULL) {
			appcontrol_x *appcontrol = (appcontrol_x *)tmp->data;
			__ps_free_appcontrol(appcontrol);
			tmp = tmp->next;
		}
		g_list_free(uiapplication->appcontrol);
	}

	/* _PRODUCT_LAUNCHING_ENHANCED_ START */
	FREE_AND_NULL(uiapplication->indicatordisplay);
	FREE_AND_NULL(uiapplication->portraitimg);
	FREE_AND_NULL(uiapplication->landscapeimg);
	FREE_AND_NULL(uiapplication->effectimage_type);
	/* _PRODUCT_LAUNCHING_ENHANCED_ END */
	FREE_AND_NULL(uiapplication->guestmode_visibility);
	FREE_AND_NULL(uiapplication->app_component);
	FREE_AND_NULL(uiapplication->permission_type);
	FREE_AND_NULL(uiapplication->component_type);
	FREE_AND_NULL(uiapplication->preload);
	FREE_AND_NULL(uiapplication->submode);
	FREE_AND_NULL(uiapplication->submode_mainid);
	FREE_AND_NULL(uiapplication->installed_storage);
	FREE_AND_NULL(uiapplication->process_pool);
	FREE_AND_NULL(uiapplication->autorestart);
	FREE_AND_NULL(uiapplication->onboot);

	FREE_AND_NULL(uiapplication);
}

static void __ps_free_font(font_x *font)
{
	if (font == NULL)
		return;
	FREE_AND_NULL(font->name);
	FREE_AND_NULL(font->text);
	FREE_AND_NULL(font);
}

static void __ps_free_theme(theme_x *theme)
{
	if (theme == NULL)
		return;
	FREE_AND_NULL(theme->name);
	FREE_AND_NULL(theme->text);
	FREE_AND_NULL(theme);
}

static void __ps_free_daemon(daemon_x *daemon)
{
	if (daemon == NULL)
		return;
	FREE_AND_NULL(daemon->name);
	FREE_AND_NULL(daemon->text);
	FREE_AND_NULL(daemon);
}

static void __ps_free_ime(ime_x *ime)
{
	if (ime == NULL)
		return;
	FREE_AND_NULL(ime->name);
	FREE_AND_NULL(ime->text);
	FREE_AND_NULL(ime);
}

API void _pkgmgrinfo_basic_free_manifest_x(manifest_x *mfx)
{
	if (mfx == NULL)
		return;
	GList *tmp = NULL;
	FREE_AND_NULL(mfx->ns);
	FREE_AND_NULL(mfx->package);
	FREE_AND_NULL(mfx->version);
	FREE_AND_NULL(mfx->api_version);
	FREE_AND_NULL(mfx->installlocation);
	FREE_AND_NULL(mfx->preload);
	FREE_AND_NULL(mfx->readonly);
	FREE_AND_NULL(mfx->removable);
	FREE_AND_NULL(mfx->update);
	FREE_AND_NULL(mfx->system);
	FREE_AND_NULL(mfx->backend_installer);
	FREE_AND_NULL(mfx->custom_smack_label);
	FREE_AND_NULL(mfx->type);
	FREE_AND_NULL(mfx->package_size);
	FREE_AND_NULL(mfx->package_total_size);
	FREE_AND_NULL(mfx->package_data_size);
	FREE_AND_NULL(mfx->installed_time);
	FREE_AND_NULL(mfx->installed_storage);
	FREE_AND_NULL(mfx->storeclient_id);
	FREE_AND_NULL(mfx->mainapp_id);
	FREE_AND_NULL(mfx->package_url);
	FREE_AND_NULL(mfx->root_path);
	FREE_AND_NULL(mfx->csc_path);
	FREE_AND_NULL(mfx->appsetting);
	FREE_AND_NULL(mfx->nodisplay_setting);
	FREE_AND_NULL(mfx->support_disable);
	FREE_AND_NULL(mfx->mother_package);
	FREE_AND_NULL(mfx->support_mode);
	FREE_AND_NULL(mfx->groupid);
#ifdef _APPFW_FEATURE_EXPANSION_PKG_INSTALL
	FREE_AND_NULL(mfx->tep_name);
#endif

	/*Free Icon*/
	if (mfx->icon) {
		tmp = mfx->icon;
		while(tmp != NULL){
			icon_x *icon = (icon_x*)tmp->data;
			__ps_free_icon(icon);
			tmp = tmp->next;
		}
		g_list_free(mfx->icon);
	}
	/*Free Label*/
	if (mfx->label) {
		tmp = mfx->label;
		while(tmp != NULL){
			label_x *label = (label_x*)tmp->data;
			__ps_free_label(label);
			tmp = tmp->next;
		}
		g_list_free(mfx->label);
	}
	/*Free Author*/
	if (mfx->author) {
		tmp = mfx->author;
		while(tmp != NULL){
			author_x *author = (author_x*)tmp->data;
			__ps_free_author(author);
			tmp = tmp->next;
		}
		g_list_free(mfx->author);
	}
	/*Free Description*/
	if (mfx->description) {
		tmp = mfx->description;
		while(tmp != NULL){
			description_x *description = (description_x*)tmp->data;
			__ps_free_description(description);
			tmp = tmp->next;
		}
		g_list_free(mfx->description);
	}
	/*Free License*/
	if (mfx->license) {
		tmp = mfx->license;
		while(tmp != NULL){
			license_x *license = (license_x*)tmp->data;
			__ps_free_license(license);
			tmp = tmp->next;
		}
		g_list_free(mfx->license);
	}
	/*Free Privileges*/
	if (mfx->privileges) {
		tmp = mfx->privileges;
		while(tmp != NULL){
			char* priv  = (char*) tmp->data;
			if(priv){
				FREE_AND_NULL(priv);
			}
			tmp = tmp->next;
		}
		g_list_free(mfx->privileges);
	}

	/*Free UiApplication*/
	if (mfx->uiapplication) {
		uiapplication_x *uiapplication = NULL;
		GList *list_up = mfx->uiapplication;

		while(list_up != NULL) {
			uiapplication = (uiapplication_x *)list_up->data;
			_pkgmgrinfo_basic_free_uiapplication_x(uiapplication);
			list_up = list_up->next;
		}
		g_list_free(mfx->uiapplication);
		list_up = NULL;
		uiapplication = NULL;
	}

	/*Free Daemon*/
	if (mfx->daemon) {
		tmp = mfx->daemon;
		while(tmp != NULL){
			daemon_x *daemon = (daemon_x*)tmp->data;
			__ps_free_daemon(daemon);
			tmp = tmp->next;
		}
		g_list_free(mfx->daemon);
	}
	/*Free Theme*/
	if (mfx->theme) {
		tmp = mfx->theme;
		while(tmp != NULL){
			theme_x* theme = (theme_x*)tmp->data;
			__ps_free_theme(theme);
			tmp = tmp->next;
		}
		g_list_free(mfx->theme);
	}
	/*Free Font*/
	if (mfx->font) {
		tmp = mfx->font;
		while(tmp != NULL){
			font_x *font = (font_x*)tmp->data;
			__ps_free_font(font);
			tmp = tmp->next;
		}
		g_list_free(mfx->font);
	}
	/*Free Ime*/
	if (mfx->ime) {
		tmp = mfx->ime;
		while(tmp != NULL){
			ime_x *ime = (ime_x*)tmp->data;
			__ps_free_ime(ime);
			tmp = tmp->next;
		}
		g_list_free(mfx->ime);
	}
	/*Free Compatibility*/
	if (mfx->compatibility) {
		tmp = mfx->compatibility;
		while(tmp != NULL){
			compatibility_x *comp = (compatibility_x*)tmp->data;
			__ps_free_compatibility(comp);
			tmp = tmp->next;
		}
		g_list_free(mfx->compatibility);
	}
	/*Free DeviceProfile*/
	if (mfx->deviceprofile) {
		tmp = mfx->deviceprofile;
		while(tmp != NULL){
			deviceprofile_x* profile = (deviceprofile_x*)tmp->data;
			__ps_free_deviceprofile(profile);
			tmp = tmp->next;
		}
		g_list_free(mfx->deviceprofile);
	}
	FREE_AND_NULL(mfx);
	return;
}

