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

#define HASH_LEN  MD5_DIGEST_LENGTH * 2


static void __ps_free_category(category_x *category)
{
	if (category == NULL)
		return;
	FREE_AND_NULL(category->name);
	FREE_AND_NULL(category);
}

static void __ps_free_privilege(privilege_x *privilege)
{
	if (privilege == NULL)
		return;
	FREE_AND_NULL(privilege->text);
	FREE_AND_NULL(privilege);
}

static void __ps_free_privileges(privileges_x *privileges)
{
	if (privileges == NULL)
		return;
	/*Free Privilege*/
	if (privileges->privilege) {
		privilege_x *privilege = privileges->privilege;
		privilege_x *tmp = NULL;
		while(privilege != NULL) {
			tmp = privilege->next;
			__ps_free_privilege(privilege);
			privilege = tmp;
		}
	}
	FREE_AND_NULL(privileges);
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
	FREE_AND_NULL(operation);
}

static void __ps_free_uri(uri_x *uri)
{
	if (uri == NULL)
		return;
	FREE_AND_NULL(uri->text);
	FREE_AND_NULL(uri);
}

static void __ps_free_mime(mime_x *mime)
{
	if (mime == NULL)
		return;
	FREE_AND_NULL(mime->text);
	FREE_AND_NULL(mime);
}

static void __ps_free_subapp(subapp_x *subapp)
{
	if (subapp == NULL)
		return;
	FREE_AND_NULL(subapp->text);
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

static void __ps_free_launchconditions(launchconditions_x *launchconditions)
{
	if (launchconditions == NULL)
		return;
	FREE_AND_NULL(launchconditions->text);
	/*Free Condition*/
	if (launchconditions->condition) {
		condition_x *condition = launchconditions->condition;
		condition_x *tmp = NULL;
		while(condition != NULL) {
			tmp = condition->next;
			__ps_free_condition(condition);
			condition = tmp;
		}
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
		operation_x *operation = appsvc->operation;
		operation_x *tmp = NULL;
		while(operation != NULL) {
			tmp = operation->next;
			__ps_free_operation(operation);
			operation = tmp;
		}
	}
	/*Free Uri*/
	if (appsvc->uri) {
		uri_x *uri = appsvc->uri;
		uri_x *tmp = NULL;
		while(uri != NULL) {
			tmp = uri->next;
			__ps_free_uri(uri);
			uri = tmp;
		}
	}
	/*Free Mime*/
	if (appsvc->mime) {
		mime_x *mime = appsvc->mime;
		mime_x *tmp = NULL;
		while(mime != NULL) {
			tmp = mime->next;
			__ps_free_mime(mime);
			mime = tmp;
		}
	}
	/*Free subapp*/
	if (appsvc->subapp) {
		subapp_x *subapp = appsvc->subapp;
		subapp_x *tmp = NULL;
		while(subapp != NULL) {
			tmp = subapp->next;
			__ps_free_subapp(subapp);
			subapp = tmp;
		}
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
	/*Free Request*/
	if (define->request) {
		request_x *request = define->request;
		request_x *tmp = NULL;
		while(request != NULL) {
			tmp = request->next;
			__ps_free_request(request);
			request = tmp;
		}
	}
	/*Free Allowed*/
	if (define->allowed) {
		allowed_x *allowed = define->allowed;
		allowed_x *tmp = NULL;
		while(allowed != NULL) {
			tmp = allowed->next;
			__ps_free_allowed(allowed);
			allowed = tmp;
		}
	}
	FREE_AND_NULL(define);
}

static void __ps_free_datashare(datashare_x *datashare)
{
	if (datashare == NULL)
		return;
	/*Free Define*/
	if (datashare->define) {
		define_x *define =  datashare->define;
		define_x *tmp = NULL;
		while(define != NULL) {
			tmp = define->next;
			__ps_free_define(define);
			define = tmp;
		}
	}
	/*Free Request*/
	if (datashare->request) {
		request_x *request = datashare->request;
		request_x *tmp = NULL;
		while(request != NULL) {
			tmp = request->next;
			__ps_free_request(request);
			request = tmp;
		}
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

static void __ps_free_uiapplication(uiapplication_x *uiapplication)
{
	if (uiapplication == NULL)
		return;
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
	FREE_AND_NULL(uiapplication->support_mode);
	FREE_AND_NULL(uiapplication->support_feature);
	FREE_AND_NULL(uiapplication->satui_label);
	FREE_AND_NULL(uiapplication->package_type);
	FREE_AND_NULL(uiapplication->package_system);
	FREE_AND_NULL(uiapplication->package_installed_time);

	/*Free Label*/
	if (uiapplication->label) {
		label_x *label = uiapplication->label;
		label_x *tmp = NULL;
		while(label != NULL) {
			tmp = label->next;
			__ps_free_label(label);
			label = tmp;
		}
	}
	/*Free Icon*/
	if (uiapplication->icon) {
		icon_x *icon = uiapplication->icon;
		icon_x *tmp = NULL;
		while(icon != NULL) {
			tmp = icon->next;
			__ps_free_icon(icon);
			icon = tmp;
		}
	}
	/*Free image*/
	if (uiapplication->image) {
		image_x *image = uiapplication->image;
		image_x *tmp = NULL;
		while(image != NULL) {
			tmp = image->next;
			__ps_free_image(image);
			image = tmp;
		}
	}
	/*Free LaunchConditions*/
	if (uiapplication->launchconditions) {
		launchconditions_x *launchconditions = uiapplication->launchconditions;
		launchconditions_x *tmp = NULL;
		while(launchconditions != NULL) {
			tmp = launchconditions->next;
			__ps_free_launchconditions(launchconditions);
			launchconditions = tmp;
		}
	}
	/*Free Notification*/
	if (uiapplication->notification) {
		notification_x *notification = uiapplication->notification;
		notification_x *tmp = NULL;
		while(notification != NULL) {
			tmp = notification->next;
			__ps_free_notification(notification);
			notification = tmp;
		}
	}
	/*Free DataShare*/
	if (uiapplication->datashare) {
		datashare_x *datashare = uiapplication->datashare;
		datashare_x *tmp = NULL;
		while(datashare != NULL) {
			tmp = datashare->next;
			__ps_free_datashare(datashare);
			datashare = tmp;
		}
	}
	/*Free AppSvc*/
	if (uiapplication->appsvc) {
		appsvc_x *appsvc = uiapplication->appsvc;
		appsvc_x *tmp = NULL;
		while(appsvc != NULL) {
			tmp = appsvc->next;
			__ps_free_appsvc(appsvc);
			appsvc = tmp;
		}
	}
	/*Free Category*/
	if (uiapplication->category) {
		category_x *category = uiapplication->category;
		category_x *tmp = NULL;
		while(category != NULL) {
			tmp = category->next;
			__ps_free_category(category);
			category = tmp;
		}
	}
	/*Free Metadata*/
	if (uiapplication->metadata) {
		metadata_x *metadata = uiapplication->metadata;
		metadata_x *tmp = NULL;
		while(metadata != NULL) {
			tmp = metadata->next;
			__ps_free_metadata(metadata);
			metadata = tmp;
		}
	}
	/*Free permission*/
	if (uiapplication->permission) {
		permission_x *permission = uiapplication->permission;
		permission_x *tmp = NULL;
		while(permission != NULL) {
			tmp = permission->next;
			__ps_free_permission(permission);
			permission = tmp;
		}
	}

	/*Free datacontrol*/
	if (uiapplication->datacontrol) {
		datacontrol_x *datacontrol = uiapplication->datacontrol;
		datacontrol_x *tmp = NULL;
		while(datacontrol != NULL) {
			tmp = datacontrol->next;
			__ps_free_datacontrol(datacontrol);
			datacontrol = tmp;
		}
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
	FREE_AND_NULL(mfx->ns);
	FREE_AND_NULL(mfx->package);
	FREE_AND_NULL(mfx->version);
	FREE_AND_NULL(mfx->installlocation);
	FREE_AND_NULL(mfx->preload);
	FREE_AND_NULL(mfx->readonly);
	FREE_AND_NULL(mfx->removable);
	FREE_AND_NULL(mfx->update);
	FREE_AND_NULL(mfx->system);
	FREE_AND_NULL(mfx->hash);
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
	FREE_AND_NULL(mfx->support_reset);
	FREE_AND_NULL(mfx->use_reset);

	/*Free Icon*/
	if (mfx->icon) {
		icon_x *icon = mfx->icon;
		icon_x *tmp = NULL;
		while(icon != NULL) {
			tmp = icon->next;
			__ps_free_icon(icon);
			icon = tmp;
		}
	}
	/*Free Label*/
	if (mfx->label) {
		label_x *label = mfx->label;
		label_x *tmp = NULL;
		while(label != NULL) {
			tmp = label->next;
			__ps_free_label(label);
			label = tmp;
		}
	}
	/*Free Author*/
	if (mfx->author) {
		author_x *author = mfx->author;
		author_x *tmp = NULL;
		while(author != NULL) {
			tmp = author->next;
			__ps_free_author(author);
			author = tmp;
		}
	}
	/*Free Description*/
	if (mfx->description) {
		description_x *description = mfx->description;
		description_x *tmp = NULL;
		while(description != NULL) {
			tmp = description->next;
			__ps_free_description(description);
			description = tmp;
		}
	}
	/*Free License*/
	if (mfx->license) {
		license_x *license = mfx->license;
		license_x *tmp = NULL;
		while(license != NULL) {
			tmp = license->next;
			__ps_free_license(license);
			license = tmp;
		}
	}
	/*Free Privileges*/
	if (mfx->privileges) {
		privileges_x *privileges = mfx->privileges;
		privileges_x *tmp = NULL;
		while(privileges != NULL) {
			tmp = privileges->next;
			__ps_free_privileges(privileges);
			privileges = tmp;
		}
	}
	/*Free UiApplication*/
	if (mfx->uiapplication) {
		uiapplication_x *uiapplication = mfx->uiapplication;
		uiapplication_x *tmp = NULL;
		while(uiapplication != NULL) {
			tmp = uiapplication->next;
			__ps_free_uiapplication(uiapplication);
			uiapplication = tmp;
		}
	}
	/*Free Daemon*/
	if (mfx->daemon) {
		daemon_x *daemon = mfx->daemon;
		daemon_x *tmp = NULL;
		while(daemon != NULL) {
			tmp = daemon->next;
			__ps_free_daemon(daemon);
			daemon = tmp;
		}
	}
	/*Free Theme*/
	if (mfx->theme) {
		theme_x *theme = mfx->theme;
		theme_x *tmp = NULL;
		while(theme != NULL) {
			tmp = theme->next;
			__ps_free_theme(theme);
			theme = tmp;
		}
	}
	/*Free Font*/
	if (mfx->font) {
		font_x *font = mfx->font;
		font_x *tmp = NULL;
		while(font != NULL) {
			tmp = font->next;
			__ps_free_font(font);
			font = tmp;
		}
	}
	/*Free Ime*/
	if (mfx->ime) {
		ime_x *ime = mfx->ime;
		ime_x *tmp = NULL;
		while(ime != NULL) {
			tmp = ime->next;
			__ps_free_ime(ime);
			ime = tmp;
		}
	}
	/*Free Compatibility*/
	if (mfx->compatibility) {
		compatibility_x *compatibility = mfx->compatibility;
		compatibility_x *tmp = NULL;
		while(compatibility != NULL) {
			tmp = compatibility->next;
			__ps_free_compatibility(compatibility);
			compatibility = tmp;
		}
	}
	/*Free DeviceProfile*/
	if (mfx->deviceprofile) {
		deviceprofile_x *deviceprofile = mfx->deviceprofile;
		deviceprofile_x *tmp = NULL;
		while(deviceprofile != NULL) {
			tmp = deviceprofile->next;
			__ps_free_deviceprofile(deviceprofile);
			deviceprofile = tmp;
		}
	}
	FREE_AND_NULL(mfx);
	return;
}

API char*  pkgmgrinfo_basic_generate_hash_for_file( const char *file)
{

	unsigned char c[MD5_DIGEST_LENGTH] = {0};
	char *hash = NULL;
	char temp[3]={0};
	int index = 0;
	MD5_CTX mdContext;
	int bytes;
	unsigned char data[1024];

	FILE *inFile = fopen (file, "rb");

	if (inFile == NULL) {
		_LOGD("@Error while opening the file: %s",strerror(errno));
		return NULL;
	}

	MD5_Init (&mdContext);

	while ((bytes = fread (data, 1, 1024, inFile)) != 0)
		MD5_Update (&mdContext, data, bytes);

	MD5_Final (c,&mdContext);

	hash = (char*)malloc(HASH_LEN + 1);
	if(hash == NULL){
		_LOGE("Malloc failed!!");
		goto catch;
	}
	memset(hash,'\0',HASH_LEN + 1);

	for(index = 0; index < MD5_DIGEST_LENGTH; index++) {
		sprintf(temp,"%02x",c[index]);
		strncat(hash,temp,strlen(temp));

	}

catch:
	if(inFile)
		fclose (inFile);

	return hash;
}
