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

#ifndef __PKGMGR_PARSER_H__
#define __PKGMGR_PARSER_H__

/**
 * @file pkgmgr_parser.h
 * @author Sewook Park <sewook7.park@samsung.com>
 * @author Shobhit Srivastava <shobhit.s@samsung.com>
 * @version 0.1
 * @brief    This file declares API of pkgmgr_parser
 * @addtogroup		APPLICATION_FRAMEWORK
 * @{
 *
 * @defgroup		PackageManagerParser
 * @section		Header Header file to include:
 * @code
 * #include		<pkgmgr_parser.h>
 * @endcode
 *
 * @}
 */

#include <libxml/xmlreader.h>
#include "pkgmgrinfo_basic.h"
#include "pkgmgr_parser_feature.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defined path and file*/
#define DESKTOP_RW_PATH "/opt/share/applications/"
#define DESKTOP_RO_PATH "/usr/share/applications/"
#define MANIFEST_RO_PREFIX "/usr/share/packages/"
#define MANIFEST_RW_PREFIX "/opt/share/packages/"

#define SCHEMA_FILE "/usr/etc/package-manager/preload/manifest.xsd"

#define BUFMAX 1024*128

/* so path */
#define LIBAPPSVC_PATH "/usr/lib/libappsvc.so.0"
#define LIBAIL_PATH "/usr/lib/libail.so.0"

/** Integer property for mode*/
#define	PMINFO_SUPPORT_MODE_ULTRA_POWER_SAVING	0x00000001
#define	PMINFO_SUPPORT_MODE_COOL_DOWN			0x00000002
#define	PMINFO_SUPPORT_MODE_SCREEN_READER		0x00000004

#define	PMINFO_SUPPORT_FEATURE_MULTI_WINDOW			0x00000001
#define	PMINFO_SUPPORT_FEATURE_OOM_TERMINATION		0x00000002
#define	PMINFO_SUPPORT_FEATURE_LARGE_MEMORY			0x00000004

/* operation_type */
typedef enum {
	ACTION_INSTALL = 0,
	ACTION_UPGRADE,
	ACTION_UNINSTALL,
	ACTION_FOTA,
	ACTION_MAX
} ACTION_TYPE;

typedef enum {
	AIL_INSTALL = 0,
	AIL_UPDATE,
	AIL_REMOVE,
	AIL_CLEAN,
	AIL_FOTA,
	AIL_MAX
} AIL_TYPE;

/**
 * @brief API return values
 */
enum {
	PM_PARSER_R_EINVAL = -2,		/**< Invalid argument */
	PM_PARSER_R_ERROR = -1,		/**< General error */
	PM_PARSER_R_OK = 0			/**< General success */
};

/**
 * @fn char 	int pkgmgr_parser_check_mdm_policy_for_uninstallation(manifest_x * mfx)
 * @brief	This API gets the mdm policy form mdm server
 */
int pkgmgr_parser_check_mdm_policy_for_uninstallation(const char *manifest);

/**
 * @fn char *pkgmgr_parser_get_manifest_file(const char *pkgid)
 * @brief	This API gets the manifest file of the package.
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	pkgid	pointer to package ID
 * @return	manifest file path on success, NULL on failure
 * @pre		None
 * @post		Free the manifest file pointer that is returned by API
 * @code
static int get_manifest_file(const char *pkgid)
{
	char *manifest = NULL;
	manifest = pkgmgr_parser_get_manifest_file(pkgid);
	if (manifest == NULL)
		return -1;
	printf("Manifest File Path is %s\n", manifest);
	free(manifest);
	return 0;
}
 * @endcode
 */
char *pkgmgr_parser_get_manifest_file(const char *pkgid);

/**
 * @fn int pkgmgr_parser_parse_manifest_for_installation(const char *manifest, char *const tagv[])
 * @brief	This API parses the manifest file of the package after installation and stores the data in DB.
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	manifest	pointer to package manifest file
 * @param[in]	tagv		array of xml tags or NULL
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parse_manifest_file_for_installation(const char *manifest)
{
	int ret = 0;
	ret = pkgmgr_parser_parse_manifest_for_installation(manifest, NULL);
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_parse_manifest_for_installation(const char *manifest, char *const tagv[]);

/**
 * @fn int pkgmgr_parser_parse_manifest_for_upgrade(const char *manifest, char *const tagv[])
 * @brief	This API parses the manifest file of the package after upgrade and stores the data in DB.
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	manifest	pointer to package manifest file
 * @param[in]	tagv		array of xml tags or NULL
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parse_manifest_file_for_upgrade(const char *manifest)
{
	int ret = 0;
	ret = pkgmgr_parser_parse_manifest_for_upgrade(manifest, NULL);
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_parse_manifest_for_upgrade(const char *manifest, char *const tagv[]);

/**
 * @fn int pkgmgr_parser_parse_manifest_for_uninstallation(const char *manifest, char *const tagv[])
 * @brief	This API parses the manifest file of the package after uninstallation and deletes the data from DB.
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	manifest	pointer to package manifest file
 * @param[in]	tagv		array of xml tags or NULL
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parse_manifest_file_for_uninstallation(const char *manifest)
{
	int ret = 0;
	ret = pkgmgr_parser_parse_manifest_for_uninstallation(manifest, NULL);
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_parse_manifest_for_uninstallation(const char *manifest, char *const tagv[]);

/**
 * @fn int pkgmgr_parser_parse_manifest_for_preload()
 * @brief	This API update  preload information to DB.
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parser_parse_manifest_for_preload()
{
	int ret = 0;
	ret = pkgmgr_parser_parse_manifest_for_preload();
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_parse_manifest_for_preload();

/**
 * @fn int pkgmgr_parser_check_manifest_validation(const char *manifest)
 * @brief	This API validates the manifest file against the manifest schema.
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	manifest	pointer to package manifest file
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int validate_manifest_file(const char *manifest)
{
	int ret = 0;
	ret = pkgmgr_parser_check_manifest_validation(manifest);
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_check_manifest_validation(const char *manifest);

/**
 * @fn void pkgmgr_parser_free_manifest_xml(manifest_x *mfx)
 * @brief	This API will free the manifest pointer by recursively freeing all sub elements.
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	mfx	pointer to parsed manifest data
 * @pre		pkgmgr_parser_process_manifest_xml()
 * @post		None
 * @code
static int parse_manifest_file(const char *manifest)
{
	manifest_x *mfx = NULL
	mfx = pkgmgr_parser_process_manifest_xml(manifest);
	if (mfx == NULL)
		return -1;
	printf("Parsing Manifest Success\n");
	pkgmgr_parser_free_manifest_xml(mfx);
	return 0;
}
 * @endcode
 */
void pkgmgr_parser_free_manifest_xml(manifest_x *mfx);

/**
 * @fn manifest_x *pkgmgr_parser_process_manifest_xml(const char *manifest)
 * @brief	This API parses the manifest file and stores all the data in the manifest structure.
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	manifest	pointer to package manifest file
 * @return	manifest pointer on success, NULL on failure
 * @pre		None
 * @post		pkgmgr_parser_free_manifest_xml()
 * @code
static int parse_manifest_file(const char *manifest)
{
	manifest_x *mfx = NULL
	mfx = pkgmgr_parser_process_manifest_xml(manifest);
	if (mfx == NULL)
		return -1;
	printf("Parsing Manifest Success\n");
	pkgmgr_parser_free_manifest_xml(mfx);
	return 0;
}
 * @endcode
 */
manifest_x *pkgmgr_parser_process_manifest_xml(const char *manifest);

/**
 * @fn manifest_x *pkgmgr_parser_get_manifest_info(const char *pkigid)
 * @brief	This API gets the manifest info from DB and stores all the data in the manifest structure.
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	pkgid	package id for package
 * @return	manifest pointer on success, NULL on failure
 * @pre		None
 * @post		pkgmgr_parser_free_manifest_xml()
 * @code
static int get_manifest_info(const char *pkgid)
{
	manifest_x *mfx = NULL
	mfx = pkgmgr_parser_get_manifest_info(pkgid);
	if (mfx == NULL)
		return -1;
	printf("Parsing Manifest Success\n");
	pkgmgr_parser_free_manifest_xml(mfx);
	return 0;
}
 * @endcode
 */
manifest_x *pkgmgr_parser_get_manifest_info(const char *pkigid);

/* These APIs are intended to call parser directly */
typedef int (*ps_iter_fn) (const char *tag, int type, void *userdata);

int pkgmgr_parser_has_parser(const char *tag, int *type);
int pkgmgr_parser_get_list(ps_iter_fn iter_fn, void *data);

/**
 * @fn int pkgmgr_parser_run_parser_for_installation(xmlDocPtr docPtr, const char *tag, const char *pkgid)
 * @brief	This API calls the parser directly by supplying the xml docptr. It is used during package installation
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	docPtr	XML doxument pointer
 * @param[in]	tag		the xml tag corresponding to the parser that will parse the docPtr
 * @param[in]	pkgid		the package id
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parse_docptr_for_installation(xmlDocPtr docPtr)
{
	int ret = 0;
	ret = pkgmgr_parser_run_parser_for_installation(docPtr, "theme", "com.samsung.test");
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_run_parser_for_installation(xmlDocPtr docPtr, const char *tag, const char *pkgid);

/**
 * @fn int pkgmgr_parser_run_parser_for_upgrade(xmlDocPtr docPtr, const char *tag, const char *pkgid)
 * @brief	This API calls the parser directly by supplying the xml docptr. It is used during package upgrade
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	docPtr	XML doxument pointer
 * @param[in]	tag		the xml tag corresponding to the parser that will parse the docPtr
 * @param[in]	pkgid		the package id
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parse_docptr_for_upgrade(xmlDocPtr docPtr)
{
	int ret = 0;
	ret = pkgmgr_parser_run_parser_for_upgrade(docPtr, "theme", "com.samsung.test");
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_run_parser_for_upgrade(xmlDocPtr docPtr, const char *tag, const char *pkgid);

/**
 * @fn int pkgmgr_parser_run_parser_for_uninstallation(xmlDocPtr docPtr, const char *tag, const char *pkgid)
 * @brief	This API calls the parser directly by supplying the xml docptr. It is used during package uninstallation
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	docPtr	XML doxument pointer
 * @param[in]	tag		the xml tag corresponding to the parser that will parse the docPtr
 * @param[in]	pkgid		the package id
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parse_docptr_for_uninstallation(xmlDocPtr docPtr)
{
	int ret = 0;
	ret = pkgmgr_parser_run_parser_for_uninstallation(docPtr, "theme", "com.samsung.test");
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_run_parser_for_uninstallation(xmlDocPtr docPtr, const char *tag, const char *pkgid);



/**
 * @fn int pkgmgr_parser_create_desktop_file(manifest_x *mfx)
 * @brief	This API generates the application desktop file
 *
 * @par		This API is for package-manager installer backends.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	mfx	manifest pointer
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		pkgmgr_parser_process_manifest_xml()
 * @post	pkgmgr_parser_free_manifest_xml()
 * @code
static int create_desktop_file(char *manifest)
{
	int ret = 0;
	manifest_x *mfx = NULL;
	mfx = pkgmgr_parser_process_manifest_xml(manifest);
	ret = pkgmgr_parser_create_desktop_file(mfx);
	if (ret)
		return -1;
	pkgmgr_parser_free_manifest_xml(mfx);
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_create_desktop_file(manifest_x *mfx);

/**
 * @fn int pkgmgr_parser_enable_pkg(const char *pkgid, char *const tagv[])
 * @brief	This API updates the data of the enabled package in DB.
 *
 * @par		This API is for package-manager.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	pkgid	pointer to pkgid
 * @param[in]	tagv		array of xml tags or NULL
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parser_disabled_pkg_for_installation(const char *pkgid)
{
	int ret = 0;
	ret = pkgmgr_parser_enable_pkg(pkgid, NULL);
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_enable_pkg(const char *pkgid, char *const tagv[]);

/**
 * @fn int pkgmgr_parser_disable_pkg(const char *pkgid, char *const tagv[])
 * @brief	This API updates the data of the disabled package in DB.
 *
 * @par		This API is for package-manager.
 * @par Sync (or) Async : Synchronous API
 *
 * @param[in]	pkgid	pointer to pkgid
 * @param[in]	tagv		array of xml tags or NULL
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_EINVAL	invalid argument
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parse_disabled_pkg_file_for_uninstallation(const char *pkgid)
{
	int ret = 0;
	ret = pkgmgr_parser_disable_pkg(pkgid, NULL);
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_disable_pkg(const char *pkgid, char *const tagv[]);

/**
 * @fn int pkgmgr_parser_insert_app_aliasid(void)
 * @brief	This API updates the app aliasid table.
 *
 * @par		This API is for pkg_initdb and pkg_fota.
 * @par Sync (or) Async : Synchronous API
 *
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parse_insert_app_aliasid(void)
{
	int ret = 0;
	ret = pkgmgr_parser_insert_app_aliasid();
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_insert_app_aliasid(void );

/**
 * @fn int pkgmgr_parser_update_app_aliasid(void)
 * @brief	This API updates the app aliasid table.
 *
 * @par		This API is for csc.
 * @par Sync (or) Async : Synchronous API
 *
 * @return	0 if success, error code(<0) if fail
 * @retval	PMINFO_R_OK	success
 * @retval	PMINFO_R_ERROR	internal error
 * @pre		None
 * @post		None
 * @code
static int parse_update_app_aliasid(void)
{
	int ret = 0;
	ret = pkgmgr_parser_update_app_aliasid();
	if (ret)
		return -1;
	return 0;
}
 * @endcode
 */
int pkgmgr_parser_update_app_aliasid(void );

/** @} */
#ifdef __cplusplus
}
#endif
#endif				/* __PKGMGR_PARSER_H__ */
