/*
 * pkgmgrinfo-certinfo
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
#include <sqlite3.h>
#include <db-util.h>

#include "pkgmgrinfo_private.h"
#include "pkgmgrinfo_debug.h"
#include "pkgmgr-info.h"
#include "pkgmgrinfo_basic.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_CERT"


typedef struct _pkgmgr_instcertinfo_x {
	char *pkgid;
	char *cert_info[MAX_CERT_TYPE];	/*certificate data*/
	int is_new[MAX_CERT_TYPE];		/*whether already exist in table or not*/
	int ref_count[MAX_CERT_TYPE];		/*reference count of certificate data*/
	int cert_id[MAX_CERT_TYPE];		/*certificate ID in index table*/
} pkgmgr_instcertinfo_x;

typedef struct _pkgmgr_certindexinfo_x {
	int cert_id;
	int cert_ref_count;
} pkgmgr_certindexinfo_x;

typedef struct _pkgmgr_certinfo_x {
	char *pkgid;
	char *cert_value;
	char *cert_info[MAX_CERT_TYPE];	/*certificate info*/
	int cert_id[MAX_CERT_TYPE];		/*certificate ID in index table*/
} pkgmgr_certinfo_x;

__thread sqlite3 *cert_db = NULL;

static int __maxid_cb(void *data, int ncols, char **coltxt, char **colname)
{
	int *p = (int*)data;
	if (coltxt[0])
		*p = atoi(coltxt[0]);
	return 0;
}

static int __certinfo_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_certinfo_x *info = (pkgmgr_certinfo_x *)data;
	int i = 0;
	for(i = 0; i < ncols; i++)
	{
		if (strcmp(colname[i], "package") == 0) {
			if (coltxt[i] && info->pkgid == NULL)
				info->pkgid = strdup(coltxt[i]);
			else
				FREE_AND_NULL(info->pkgid);
		} else if (strcmp(colname[i], "author_signer_cert") == 0) {
			if (coltxt[i])
				(info->cert_id)[PMINFO_AUTHOR_SIGNER_CERT] = atoi(coltxt[i]);
			else
				(info->cert_id)[PMINFO_AUTHOR_SIGNER_CERT] = 0;
		} else if (strcmp(colname[i], "author_im_cert") == 0) {
			if (coltxt[i])
				(info->cert_id)[PMINFO_AUTHOR_INTERMEDIATE_CERT] = atoi(coltxt[i]);
			else
				(info->cert_id)[PMINFO_AUTHOR_INTERMEDIATE_CERT] = 0;
		} else if (strcmp(colname[i], "author_root_cert") == 0) {
			if (coltxt[i])
				(info->cert_id)[PMINFO_AUTHOR_ROOT_CERT] = atoi(coltxt[i]);
			else
				(info->cert_id)[PMINFO_AUTHOR_ROOT_CERT] = 0;
		} else if (strcmp(colname[i], "dist_signer_cert") == 0 ){
			if (coltxt[i])
				(info->cert_id)[PMINFO_DISTRIBUTOR_SIGNER_CERT] = atoi(coltxt[i]);
			else
				(info->cert_id)[PMINFO_DISTRIBUTOR_SIGNER_CERT] = 0;
		} else if (strcmp(colname[i], "dist_im_cert") == 0 ){
			if (coltxt[i])
				(info->cert_id)[PMINFO_DISTRIBUTOR_INTERMEDIATE_CERT] = atoi(coltxt[i]);
			else
				(info->cert_id)[PMINFO_DISTRIBUTOR_INTERMEDIATE_CERT] = 0;
		} else if (strcmp(colname[i], "dist_root_cert") == 0 ){
			if (coltxt[i])
				(info->cert_id)[PMINFO_DISTRIBUTOR_ROOT_CERT] = atoi(coltxt[i]);
			else
				(info->cert_id)[PMINFO_DISTRIBUTOR_ROOT_CERT] = 0;
		} else if (strcmp(colname[i], "dist2_signer_cert") == 0 ){
			if (coltxt[i])
				(info->cert_id)[PMINFO_DISTRIBUTOR2_SIGNER_CERT] = atoi(coltxt[i]);
			else
				(info->cert_id)[PMINFO_DISTRIBUTOR2_SIGNER_CERT] = 0;
		} else if (strcmp(colname[i], "dist2_im_cert") == 0 ){
			if (coltxt[i])
				(info->cert_id)[PMINFO_DISTRIBUTOR2_INTERMEDIATE_CERT] = atoi(coltxt[i]);
			else
				(info->cert_id)[PMINFO_DISTRIBUTOR2_INTERMEDIATE_CERT] = 0;
		} else if (strcmp(colname[i], "dist2_root_cert") == 0 ){
			if (coltxt[i])
				(info->cert_id)[PMINFO_DISTRIBUTOR2_ROOT_CERT] = atoi(coltxt[i]);
			else
				(info->cert_id)[PMINFO_DISTRIBUTOR2_ROOT_CERT] = 0;
		} else if (strcmp(colname[i], "cert_info") == 0 ){
			if (coltxt[i] && info->cert_value == NULL)
				info->cert_value = strdup(coltxt[i]);
			else
				FREE_AND_NULL(info->cert_value);
		} else
			continue;
	}
	return 0;
}

static int __certindexinfo_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_certindexinfo_x *info = (pkgmgr_certindexinfo_x *)data;
	int i = 0;
	for(i = 0; i < ncols; i++) {
		if (strcmp(colname[i], "cert_id") == 0) {
			if (coltxt[i])
				info->cert_id = atoi(coltxt[i]);
			else
				info->cert_id = 0;
		} else if (strcmp(colname[i], "cert_ref_count") == 0) {
			if (coltxt[i])
				info->cert_ref_count = atoi(coltxt[i]);
			else
				info->cert_ref_count = 0;
		} else
			continue;
	}
	return 0;
}

static int __exec_certinfo_query(char *query, void *data)
{
	char *error_message = NULL;
	if (SQLITE_OK !=
	    sqlite3_exec(cert_db, query, __certinfo_cb, data, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", query,
		       error_message);
		sqlite3_free(error_message);
		return -1;
	}
	sqlite3_free(error_message);
	return 0;
}

static int __exec_certindexinfo_query(char *query, void *data)
{
	char *error_message = NULL;
	if (SQLITE_OK !=
	    sqlite3_exec(cert_db, query, __certindexinfo_cb, data, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", query,
		       error_message);
		sqlite3_free(error_message);
		return -1;
	}
	sqlite3_free(error_message);
	return 0;
}

static int __delete_certinfo(const char *pkgid)
{
	int ret = -1;
	int i = 0;
	int j = 0;
	int c = 0;
	int unique_id[MAX_CERT_TYPE] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	char *error_message = NULL;
	char query[MAX_QUERY_LEN] = {'\0'};
	pkgmgr_certinfo_x *certinfo = NULL;
	pkgmgr_certindexinfo_x *indexinfo = NULL;
	certinfo = calloc(1, sizeof(pkgmgr_certinfo_x));
	retvm_if(certinfo == NULL, PMINFO_R_ERROR, "Malloc Failed\n");
	indexinfo = calloc(1, sizeof(pkgmgr_certindexinfo_x));
	if (indexinfo == NULL) {
		_LOGE("Out of Memory!!!");
		ret = PMINFO_R_ERROR;
		goto err;
	}
	/*populate certinfo from DB*/
	char *sel_query = sqlite3_mprintf("select * from package_cert_info where package=%Q ", pkgid);
	ret = __exec_certinfo_query(sel_query, (void *)certinfo);
	sqlite3_free(sel_query);
	if (ret == -1) {
		_LOGE("Package Cert Info DB Information retrieval failed\n");
		ret = PMINFO_R_ERROR;
		goto err;
	}
	/*Update cert index table*/
	for (i = 0; i < MAX_CERT_TYPE; i++) {
		if ((certinfo->cert_id)[i]) {
			for (j = 0; j < MAX_CERT_TYPE; j++) {
				if ((certinfo->cert_id)[i] == unique_id[j]) {
					/*Ref count has already been updated. Just continue*/
					break;
				}
			}
			if (j == MAX_CERT_TYPE)
				unique_id[c++] = (certinfo->cert_id)[i];
			else
				continue;
			sqlite3_snprintf(MAX_QUERY_LEN, query, "select * from package_cert_index_info where cert_id=%d ", (certinfo->cert_id)[i]);
			ret = __exec_certindexinfo_query(query, (void *)indexinfo);
			if (ret == -1) {
				_LOGE("Cert Info DB Information retrieval failed\n");
				ret = PMINFO_R_ERROR;
				goto err;
			}
			memset(query, '\0', MAX_QUERY_LEN);
			if (indexinfo->cert_ref_count > 1) {
				/*decrease ref count*/
				sqlite3_snprintf(MAX_QUERY_LEN, query, "update package_cert_index_info set cert_ref_count=%d where cert_id=%d ",
				indexinfo->cert_ref_count - 1, (certinfo->cert_id)[i]);
			} else {
				/*delete this certificate as ref count is 1 and it will become 0*/
				sqlite3_snprintf(MAX_QUERY_LEN, query, "delete from  package_cert_index_info where cert_id=%d ", (certinfo->cert_id)[i]);
			}
		        if (SQLITE_OK !=
		            sqlite3_exec(cert_db, query, NULL, NULL, &error_message)) {
		                _LOGE("Don't execute query = %s error message = %s\n", query,
		                       error_message);
				sqlite3_free(error_message);
				ret = PMINFO_R_ERROR;
				goto err;
		        }
		}
	}
	/*Now delete the entry from db*/
	char *del_query = sqlite3_mprintf("delete from package_cert_info where package=%Q", pkgid);
    if (SQLITE_OK !=
        sqlite3_exec(cert_db, del_query, NULL, NULL, &error_message)) {
            _LOGE("Don't execute query = %s error message = %s\n", del_query,
                   error_message);
		sqlite3_free(error_message);
		sqlite3_free(del_query);
		ret = PMINFO_R_ERROR;
		goto err;
    }
	sqlite3_free(del_query);
	ret = PMINFO_R_OK;
err:
	FREE_AND_NULL(indexinfo);
	FREE_AND_NULL(certinfo->pkgid);
	for (i = 0; i < MAX_CERT_TYPE; i++) {
		FREE_AND_NULL((certinfo->cert_info)[i]);
	}
	FREE_AND_NULL(certinfo);
	return ret;
}

API int pkgmgrinfo_pkginfo_create_certinfo(pkgmgrinfo_certinfo_h *handle)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_certinfo_x *certinfo = NULL;
	certinfo = calloc(1, sizeof(pkgmgr_certinfo_x));
	retvm_if(certinfo == NULL, PMINFO_R_ERROR, "Malloc Failed\n");
	*handle = (void *)certinfo;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_load_certinfo(const char *pkgid, pkgmgrinfo_certinfo_h handle)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "package ID is NULL\n");
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Certinfo handle is NULL\n");
	pkgmgr_certinfo_x *certinfo = NULL;
	int ret = PMINFO_R_OK;
	char query[MAX_QUERY_LEN] = {'\0'};
	int i = 0;

	/*Open db.*/
	ret = _pminfo_db_open_with_options(CERT_DB, &cert_db, SQLITE_OPEN_READONLY);
	if (ret != SQLITE_OK) {
		_LOGE("connect db [%s] failed!\n", CERT_DB);
		return PMINFO_R_ERROR;
	}

	certinfo = (pkgmgr_certinfo_x *)handle;
	/*populate certinfo from DB*/
	sqlite3_snprintf(MAX_QUERY_LEN, query, "select * from package_cert_info where package=%Q ", pkgid);
	ret = __exec_certinfo_query(query, (void *)certinfo);
	if (ret == -1) {
		_LOGE("Package Cert Info DB Information retrieval failed\n");
		ret = PMINFO_R_ERROR;
		goto err;
	}

	if (certinfo->pkgid == NULL) {
		_LOGE("Package not found in DB\n");
		ret = PMINFO_R_ERROR;
		goto err;
	}

	for (i = 0; i < MAX_CERT_TYPE; i++) {
		memset(query, '\0', MAX_QUERY_LEN);
		sqlite3_snprintf(MAX_QUERY_LEN, query, "select cert_info from package_cert_index_info where cert_id=%d ", (certinfo->cert_id)[i]);
		ret = __exec_certinfo_query(query, (void *)certinfo);
		if (ret == -1) {
			_LOGE("Cert Info DB Information retrieval failed\n");
			ret = PMINFO_R_ERROR;
			goto err;
		}
		if (certinfo->cert_value) {
			(certinfo->cert_info)[i] = strdup(certinfo->cert_value);
			FREE_AND_NULL(certinfo->cert_value);
		}
	}
err:
	sqlite3_close(cert_db);
	return ret;
}

API int pkgmgrinfo_pkginfo_get_cert_value(pkgmgrinfo_certinfo_h handle, pkgmgrinfo_cert_type cert_type, const char **cert_value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	retvm_if(cert_value == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	retvm_if(cert_type < PMINFO_AUTHOR_ROOT_CERT, PMINFO_R_EINVAL, "Invalid certificate type\n");
	retvm_if(cert_type > PMINFO_DISTRIBUTOR2_SIGNER_CERT, PMINFO_R_EINVAL, "Invalid certificate type\n");
	pkgmgr_certinfo_x *certinfo = NULL;
	certinfo = (pkgmgr_certinfo_x *)handle;
	if ((certinfo->cert_info)[cert_type])
		*cert_value = (certinfo->cert_info)[cert_type];
	else
		*cert_value = NULL;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_destroy_certinfo(pkgmgrinfo_certinfo_h handle)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	int i = 0;
	pkgmgr_certinfo_x *certinfo = NULL;
	certinfo = (pkgmgr_certinfo_x *)handle;
	FREE_AND_NULL(certinfo->pkgid);
	for (i = 0; i < MAX_CERT_TYPE; i++) {
		FREE_AND_NULL((certinfo->cert_info)[i]);
	}
	FREE_AND_NULL(certinfo);
	return PMINFO_R_OK;
}

API int pkgmgrinfo_create_certinfo_set_handle(pkgmgrinfo_instcertinfo_h *handle)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_instcertinfo_x *certinfo = NULL;
	certinfo = calloc(1, sizeof(pkgmgr_instcertinfo_x));
	retvm_if(certinfo == NULL, PMINFO_R_ERROR, "Malloc Failed\n");
	*handle = (void *)certinfo;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_cert_value(pkgmgrinfo_instcertinfo_h handle, pkgmgrinfo_instcert_type cert_type, char *cert_value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	retvm_if(cert_value == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	retvm_if(cert_type < PMINFO_SET_AUTHOR_ROOT_CERT, PMINFO_R_EINVAL, "Invalid certificate type\n");
	retvm_if(cert_type > PMINFO_SET_DISTRIBUTOR2_SIGNER_CERT, PMINFO_R_EINVAL, "Invalid certificate type\n");
	pkgmgr_instcertinfo_x *certinfo = NULL;
	certinfo = (pkgmgr_instcertinfo_x *)handle;
	(certinfo->cert_info)[cert_type] = strdup(cert_value);
	return PMINFO_R_OK;
}

API int pkgmgrinfo_save_certinfo(const char *pkgid, pkgmgrinfo_instcertinfo_h handle)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "package ID is NULL\n");
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Certinfo handle is NULL\n");
	char *error_message = NULL;
	char query[MAX_QUERY_LEN] = {'\0'};
	char *vquery = NULL;
	int len = 0;
	int i = 0;
	int j = 0;
	int c = 0;
	int unique_id[MAX_CERT_TYPE] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	int newid = 0;
	int is_new = 0;
	int exist = -1;
	int ret = -1;
	int maxid = 0;
	int flag = 0;
	pkgmgr_instcertinfo_x *info = (pkgmgr_instcertinfo_x *)handle;
	pkgmgr_certindexinfo_x *indexinfo = NULL;
	indexinfo = calloc(1, sizeof(pkgmgr_certindexinfo_x));
	if (indexinfo == NULL) {
		_LOGE("Out of Memory!!!");
		return PMINFO_R_ERROR;
	}
	info->pkgid = strdup(pkgid);

	/*Open db.*/
	ret = db_util_open_with_options(CERT_DB, &cert_db, SQLITE_OPEN_READWRITE, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("connect db [%s] failed!\n", CERT_DB);
		ret = PMINFO_R_ERROR;
		goto err;
	}
	/*Begin Transaction*/
	ret = sqlite3_exec(cert_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to begin transaction\n");
		ret = PMINFO_R_ERROR;
		goto err;
	}
	_LOGE("Transaction Begin\n");
	/*Check if request is to insert/update*/
	char *cert_query = sqlite3_mprintf("select exists(select * from package_cert_info where package=%Q)", pkgid);
	if (SQLITE_OK !=
	    sqlite3_exec(cert_db, cert_query, _pkgmgrinfo_validate_cb, (void *)&exist, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", cert_query,
		       error_message);
		sqlite3_free(cert_query);
		sqlite3_free(error_message);
		ret = PMINFO_R_ERROR;
		goto err;
	}
	sqlite3_free(cert_query);

	if (exist) {
		/*Update request.
		We cant just issue update query directly. We need to manage index table also.
		Hence it is better to delete and insert again in case of update*/
		ret = __delete_certinfo(pkgid);
		if (ret < 0)
			_LOGE("Certificate Deletion Failed\n");
	}
	for (i = 0; i < MAX_CERT_TYPE; i++) {
		if ((info->cert_info)[i]) {
			for (j = 0; j < i; j++) {
				if ( (info->cert_info)[j]) {
					if (strcmp((info->cert_info)[i], (info->cert_info)[j]) == 0) {
						(info->cert_id)[i] = (info->cert_id)[j];
						(info->is_new)[i] = 0;
						(info->ref_count)[i] = (info->ref_count)[j];
						break;
					}
				}
			}
			if (j < i)
				continue;
			sqlite3_snprintf(MAX_QUERY_LEN, query, "select * from package_cert_index_info " \
				"where cert_info=%Q",(info->cert_info)[i]);
			ret = __exec_certindexinfo_query(query, (void *)indexinfo);
			if (ret == -1) {
				_LOGE("Cert Info DB Information retrieval failed\n");
				ret = PMINFO_R_ERROR;
				goto err;
			}
			if (indexinfo->cert_id == 0) {
				/*New certificate. Get newid*/
				memset(query, '\0', MAX_QUERY_LEN);
				sqlite3_snprintf(MAX_QUERY_LEN, query, "select MAX(cert_id) from package_cert_index_info ");
				if (SQLITE_OK !=
				    sqlite3_exec(cert_db, query, __maxid_cb, (void *)&newid, &error_message)) {
					_LOGE("Don't execute query = %s error message = %s\n", query,
					       error_message);
					sqlite3_free(error_message);
					ret = PMINFO_R_ERROR;
					goto err;
				}
				newid = newid + 1;
				if (flag == 0) {
					maxid = newid;
					flag = 1;
				}
				indexinfo->cert_id = maxid;
				indexinfo->cert_ref_count = 1;
				is_new = 1;
				maxid = maxid + 1;
			}
			(info->cert_id)[i] = indexinfo->cert_id;
			(info->is_new)[i] = is_new;
			(info->ref_count)[i] = indexinfo->cert_ref_count;
			_LOGE("Id:Count = %d %d\n", indexinfo->cert_id, indexinfo->cert_ref_count);
			indexinfo->cert_id = 0;
			indexinfo->cert_ref_count = 0;
			is_new = 0;
		}
	}
	len = MAX_QUERY_LEN;
	for (i = 0; i < MAX_CERT_TYPE; i++) {
		if ((info->cert_info)[i])
			len+= strlen((info->cert_info)[i]);
	}
	vquery = (char *)calloc(1, len);
	if (!vquery) {
		_LOGE("malloc failed");
		ret = PMINFO_R_ERROR;
		goto err;
	}
	/*insert*/
	sqlite3_snprintf(len, vquery,
                 "insert into package_cert_info(package, author_root_cert, author_im_cert, author_signer_cert, dist_root_cert, " \
                "dist_im_cert, dist_signer_cert, dist2_root_cert, dist2_im_cert, dist2_signer_cert) " \
                "values(%Q, %d, %d, %d, %d, %d, %d, %d, %d, %d)",\
                 info->pkgid,(info->cert_id)[PMINFO_SET_AUTHOR_ROOT_CERT],(info->cert_id)[PMINFO_SET_AUTHOR_INTERMEDIATE_CERT],
		(info->cert_id)[PMINFO_SET_AUTHOR_SIGNER_CERT], (info->cert_id)[PMINFO_SET_DISTRIBUTOR_ROOT_CERT],
		(info->cert_id)[PMINFO_SET_DISTRIBUTOR_INTERMEDIATE_CERT], (info->cert_id)[PMINFO_SET_DISTRIBUTOR_SIGNER_CERT],
		(info->cert_id)[PMINFO_SET_DISTRIBUTOR2_ROOT_CERT],(info->cert_id)[PMINFO_SET_DISTRIBUTOR2_INTERMEDIATE_CERT],
		(info->cert_id)[PMINFO_SET_DISTRIBUTOR2_SIGNER_CERT]);
        if (SQLITE_OK !=
            sqlite3_exec(cert_db, vquery, NULL, NULL, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", vquery,
		       error_message);
		sqlite3_free(error_message);
		ret = PMINFO_R_ERROR;
		goto err;
        }
	/*Update index table info*/
	/*If cert_id exists and is repeated for current package, ref count should only be increased once*/
	for (i = 0; i < MAX_CERT_TYPE; i++) {
		if ((info->cert_info)[i]) {
			memset(vquery, '\0', len);
			if ((info->is_new)[i]) {
				sqlite3_snprintf(len, vquery, "insert into package_cert_index_info(cert_info, cert_id, cert_ref_count) " \
				"values(%Q, '%d', '%d') ", (info->cert_info)[i], (info->cert_id)[i], 1);
				unique_id[c++] = (info->cert_id)[i];
			} else {
				/*Update*/
				for (j = 0; j < MAX_CERT_TYPE; j++) {
					if ((info->cert_id)[i] == unique_id[j]) {
						/*Ref count has already been increased. Just continue*/
						break;
					}
				}
				if (j == MAX_CERT_TYPE)
					unique_id[c++] = (info->cert_id)[i];
				else
					continue;
				sqlite3_snprintf(len, vquery, "update package_cert_index_info set cert_ref_count=%d " \
				"where cert_id=%d",  (info->ref_count)[i] + 1, (info->cert_id)[i]);
			}
		        if (SQLITE_OK !=
		            sqlite3_exec(cert_db, vquery, NULL, NULL, &error_message)) {
				_LOGE("Don't execute query = %s error message = %s\n", vquery,
				       error_message);
				sqlite3_free(error_message);
				ret = PMINFO_R_ERROR;
				goto err;
		        }
		}
	}
	/*Commit transaction*/
	ret = sqlite3_exec(cert_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to commit transaction, Rollback now\n");
		ret = sqlite3_exec(cert_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to commit transaction, Rollback now\n");

		ret = PMINFO_R_ERROR;
		goto err;
	}
	_LOGE("Transaction Commit and End\n");
	ret =  PMINFO_R_OK;
err:
	sqlite3_close(cert_db);
	FREE_AND_NULL(vquery);
	FREE_AND_NULL(indexinfo);
	return ret;
}

API int pkgmgrinfo_destroy_certinfo_set_handle(pkgmgrinfo_instcertinfo_h handle)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	int i = 0;
	pkgmgr_instcertinfo_x *certinfo = NULL;
	certinfo = (pkgmgr_instcertinfo_x *)handle;
	FREE_AND_NULL(certinfo->pkgid);
	for (i = 0; i < MAX_CERT_TYPE; i++) {
		FREE_AND_NULL((certinfo->cert_info)[i]);
	}
	FREE_AND_NULL(certinfo);
	return PMINFO_R_OK;
}

API int pkgmgrinfo_delete_certinfo(const char *pkgid)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	int ret = -1;
	/*Open db.*/
	ret = _pminfo_db_open_with_options(CERT_DB, &cert_db, SQLITE_OPEN_READWRITE);
	if (ret != SQLITE_OK) {
		_LOGE("connect db [%s] failed!\n", CERT_DB);
		ret = PMINFO_R_ERROR;
		goto err;
	}
	/*Begin Transaction*/
	ret = sqlite3_exec(cert_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to begin transaction\n");
		ret = PMINFO_R_ERROR;
		goto err;
	}
	_LOGE("Transaction Begin\n");
	ret = __delete_certinfo(pkgid);
	if (ret < 0) {
		_LOGE("Certificate Deletion Failed\n");
	} else {
		_LOGE("Certificate Deletion Success\n");
	}
	/*Commit transaction*/
	ret = sqlite3_exec(cert_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to commit transaction, Rollback now\n");
		ret = sqlite3_exec(cert_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Failed to commit transaction, Rollback now\n");

		ret = PMINFO_R_ERROR;
		goto err;
	}
	_LOGE("Transaction Commit and End\n");
	ret = PMINFO_R_OK;
err:
	sqlite3_close(cert_db);
	return ret;
}

