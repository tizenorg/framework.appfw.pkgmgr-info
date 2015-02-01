/*
 * rpm-installer
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

#include <glib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

#include <pkgmgr_parser.h>
#include "pkgmgr_parser_signature.h"
#include "pkgmgrinfo_debug.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_PARSER"

/**
 * MDM API result
 * @see
 */
typedef enum _mdm_result {
	MDM_RESULT_SERVICE_NOT_ENABLED = -5,
	MDM_RESULT_ACCESS_DENIED = -4,
	MDM_RESULT_INVALID_PARAM = -3,
	MDM_RESULT_NOT_SUPPORTED = -2,
	MDM_RESULT_FAIL		 = -1,
	MDM_RESULT_SUCCESS	 = 0,
} mdm_result_t;

/**
 * MDM Policy status
 * @see
 */
typedef enum _mdm_status {
	MDM_STATUS_ERROR	= -1,

	MDM_ALLOWED		= 0,
	MDM_RESTRICTED		= 1,

	MDM_ENABLED		= 0,
	MDM_DISABLED		= 1,

	MDM_FALSE		= 0,
	MDM_TRUE		= 1,
} mdm_status_t;

typedef struct _mem_header {
	int id;					/* Internal id */
	void (* free_func)(void *block);	/* Pointer to function used to free this block of memory */
} mem_header_t;

typedef struct _mdm_data {
	mdm_result_t ret;
	mem_header_t mem_header;
	void *data;
} mdm_data_t;

#define LIBMDM_PATH "libmdm.so.1"

#define ASCII(s) (const char *)s
#define XMLCHAR(s) (const xmlChar *)s

static int __get_attribute(xmlTextReaderPtr reader, char *attribute, const char **xml_attribute)
{
	if(xml_attribute == NULL){
		_LOGE("@xml_attribute argument is NULL!!");
		return -1;
	}
	xmlChar	*attrib_val = xmlTextReaderGetAttribute(reader,XMLCHAR(attribute));
	if(attrib_val)
		*xml_attribute = ASCII(attrib_val);

	return 0;
}

static int  __get_value(xmlTextReaderPtr reader, const char **xml_value)
{
	if(xml_value == NULL){
		_LOGE("@xml_value is NULL!!");
		return -1;
	}
	xmlChar *value = xmlTextReaderValue(reader);
	if(value)
		*xml_value = ASCII(value);

	return 0;
}

static int _ri_next_child_element(xmlTextReaderPtr reader, int depth)
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

static void _ri_free_transform(transform_x *transform)
{
	if (transform == NULL)
		return;
	FREE_AND_NULL(transform->algorithm);
	FREE_AND_NULL(transform);
}

static void _ri_free_cannonicalizationmethod(cannonicalizationmethod_x *cannonicalizationmethod)
{
	if (cannonicalizationmethod == NULL)
		return;
	FREE_AND_NULL(cannonicalizationmethod->algorithm);
	FREE_AND_NULL(cannonicalizationmethod);
}

static void _ri_free_signaturemethod(signaturemethod_x *signaturemethod)
{
	if (signaturemethod == NULL)
		return;
	FREE_AND_NULL(signaturemethod->algorithm);
	FREE_AND_NULL(signaturemethod);
}

static void _ri_free_digestmethod(digestmethod_x *digestmethod)
{
	if (digestmethod == NULL)
		return;
	FREE_AND_NULL(digestmethod->algorithm);
	FREE_AND_NULL(digestmethod);
}

static void _ri_free_digestvalue(digestvalue_x *digestvalue)
{
	if (digestvalue == NULL)
		return;
	FREE_AND_NULL(digestvalue->text);
	FREE_AND_NULL(digestvalue);
}

static void _ri_free_signaturevalue(signaturevalue_x *signaturevalue)
{
	if (signaturevalue == NULL)
		return;
	FREE_AND_NULL(signaturevalue->text);
	FREE_AND_NULL(signaturevalue);
}

static void _ri_free_x509certificate(x509certificate_x *x509certificate)
{
	if (x509certificate == NULL)
		return;
	FREE_AND_NULL(x509certificate->text);
	FREE_AND_NULL(x509certificate);
}

static void _ri_free_x509data(x509data_x *x509data)
{
	if (x509data == NULL)
		return;
	if (x509data->x509certificate) {
		x509certificate_x *x509certificate = x509data->x509certificate;
		x509certificate_x *tmp = NULL;
		while(x509certificate != NULL) {
		        tmp = x509certificate->next;
		        _ri_free_x509certificate(x509certificate);
		        x509certificate = tmp;
		}
	}
	FREE_AND_NULL(x509data);
}

static void _ri_free_keyinfo(keyinfo_x *keyinfo)
{
	if (keyinfo == NULL)
		return;
	if (keyinfo->x509data) {
		x509data_x *x509data = keyinfo->x509data;
		x509data_x *tmp = NULL;
		while(x509data != NULL) {
		        tmp = x509data->next;
		        _ri_free_x509data(x509data);
		        x509data = tmp;
		}
	}
	FREE_AND_NULL(keyinfo);
}

static void _ri_free_transforms(transforms_x *transforms)
{
	if (transforms == NULL)
		return;
	if (transforms->transform) {
		transform_x *transform = transforms->transform;
		transform_x *tmp = NULL;
		while(transform != NULL) {
		        tmp = transform->next;
		        _ri_free_transform(transform);
		        transform = tmp;
		}
	}
	FREE_AND_NULL(transforms);
}

static void _ri_free_reference(reference_x *reference)
{
	if (reference == NULL)
		return;
	if (reference->digestmethod) {
		digestmethod_x *digestmethod = reference->digestmethod;
		digestmethod_x *tmp = NULL;
		while(digestmethod != NULL) {
		        tmp = digestmethod->next;
		        _ri_free_digestmethod(digestmethod);
		        digestmethod = tmp;
		}
	}
	if (reference->digestvalue) {
		digestvalue_x *digestvalue = reference->digestvalue;
		digestvalue_x *tmp = NULL;
		while(digestvalue != NULL) {
		        tmp = digestvalue->next;
		        _ri_free_digestvalue(digestvalue);
		        digestvalue = tmp;
		}
	}
	if (reference->transforms) {
		transforms_x *transforms = reference->transforms;
		transforms_x *tmp = NULL;
		while(transforms != NULL) {
		        tmp = transforms->next;
		        _ri_free_transforms(transforms);
		        transforms = tmp;
		}
	}
	FREE_AND_NULL(reference);
}

static void _ri_free_signedinfo(signedinfo_x *signedinfo)
{
	if (signedinfo == NULL)
		return;
	if (signedinfo->cannonicalizationmethod) {
		cannonicalizationmethod_x *cannonicalizationmethod = signedinfo->cannonicalizationmethod;
		cannonicalizationmethod_x *tmp = NULL;
		while(cannonicalizationmethod != NULL) {
		        tmp = cannonicalizationmethod->next;
		        _ri_free_cannonicalizationmethod(cannonicalizationmethod);
		        cannonicalizationmethod = tmp;
		}
	}
	if (signedinfo->signaturemethod) {
		signaturemethod_x *signaturemethod = signedinfo->signaturemethod;
		signaturemethod_x *tmp = NULL;
		while(signaturemethod != NULL) {
		        tmp = signaturemethod->next;
		        _ri_free_signaturemethod(signaturemethod);
		        signaturemethod = tmp;
		}
	}
	if (signedinfo->reference) {
		reference_x *reference = signedinfo->reference;
		reference_x *tmp = NULL;
		while(reference != NULL) {
		        tmp = reference->next;
		        _ri_free_reference(reference);
		        reference = tmp;
		}
	}
	FREE_AND_NULL(signedinfo);
}

void _ri_free_signature_xml(signature_x *sigx)
{
	if (sigx == NULL)
		return;
	FREE_AND_NULL(sigx->id);
	FREE_AND_NULL(sigx->xmlns);
	if (sigx->signedinfo) {
		signedinfo_x *signedinfo = sigx->signedinfo;
		signedinfo_x *tmp = NULL;
		while(signedinfo != NULL) {
		        tmp = signedinfo->next;
		        _ri_free_signedinfo(signedinfo);
		        signedinfo = tmp;
		}
	}
	if (sigx->signaturevalue) {
		signaturevalue_x *signaturevalue = sigx->signaturevalue;
		signaturevalue_x *tmp = NULL;
		while(signaturevalue != NULL) {
		        tmp = signaturevalue->next;
		        _ri_free_signaturevalue(signaturevalue);
		        signaturevalue = tmp;
		}
	}
	if (sigx->keyinfo) {
		keyinfo_x *keyinfo = sigx->keyinfo;
		keyinfo_x *tmp = NULL;
		while(keyinfo != NULL) {
		        tmp = keyinfo->next;
		        _ri_free_keyinfo(keyinfo);
		        keyinfo = tmp;
		}
	}
	/*Object will be freed when it will be parsed in future*/
	FREE_AND_NULL(sigx);
}

static int _ri_process_digestmethod(xmlTextReaderPtr reader, digestmethod_x *digestmethod)
{
	int ret = 0;
	ret = __get_attribute(reader,"Algorithm",&digestmethod->algorithm);
	if(ret != 0){
		_LOGE("@Error in getting the attribute value");
	}else{
		_LOGD("DigestMethod Algo is %s\n",digestmethod->algorithm);
	}
	return ret;

}

static int _ri_process_digestvalue(xmlTextReaderPtr reader, digestvalue_x *digestvalue)
{
	int ret = -1;
	xmlTextReaderRead(reader);

	ret = __get_value(reader,&digestvalue->text);
	if(ret != 0){
		_LOGE("@Error in getting the value");
	}
	return ret;
}

static int _ri_process_transform(xmlTextReaderPtr reader, transform_x *transform)
{
	int ret = 0;
	ret = __get_attribute(reader,"Algorithm",&transform->algorithm);
	if(ret != 0){
		_LOGE("@Error in getting the attribute value");
	}else{
		_LOGD("Transform Algo is %s\n",transform->algorithm);
	}
	return ret;
}

static int _ri_process_transforms(xmlTextReaderPtr reader, transforms_x *transforms)
{
	const xmlChar *node = NULL;
	int ret = 0;
	int depth = 0;
	transform_x *tmp1 = NULL;

	depth = xmlTextReaderDepth(reader);
	while ((ret = _ri_next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			// _d_msg(DEBUG_ERR, "node is NULL\n");
			return -1;
		}
		if (strcmp(ASCII(node), "Transform") == 0) {
			transform_x *transform = calloc(1, sizeof(transform_x));
			if (transform == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(transforms->transform, transform);
			ret = _ri_process_transform(reader, transform);
		} else {
			// _d_msg(DEBUG_INFO, "Invalid tag %s", ASCII(node));
			return -1;
		}
		if (ret < 0)
			return ret;
	}

	SAFE_LISTHEAD(transforms->transform, tmp1);

	return ret;
}

static int _ri_process_cannonicalizationmethod(xmlTextReaderPtr reader, cannonicalizationmethod_x *cannonicalizationmethod)
{
	int ret = 0;
	ret = __get_attribute(reader,"Algorithm",&cannonicalizationmethod->algorithm);
	if(ret != 0){
		_LOGE("@Error in getting the attribute value");
	}else{
		_LOGD("Cannonicalization-method Algo is %s\n",cannonicalizationmethod->algorithm);
	}
	return ret;
}

static int _ri_process_signaturemethod(xmlTextReaderPtr reader, signaturemethod_x *signaturemethod)
{
	int ret = 0;
	ret = __get_attribute(reader,"Algorithm",&signaturemethod->algorithm);
	if(ret != 0){
		_LOGE("@Error in getting the attribute value");
	}else{
		_LOGD("Signature-method Algo is %s\n",signaturemethod->algorithm);
	}
	return ret;
}

static int _ri_process_reference(xmlTextReaderPtr reader, reference_x *reference)
{
	const xmlChar *node = NULL;
	int ret = 0;
	int depth = 0;
	digestmethod_x *tmp1 = NULL;
	digestvalue_x *tmp2 = NULL;
	transforms_x *tmp3 = NULL;

	ret = __get_attribute(reader,"URI",&reference->uri);
	if(ret != 0){
		_LOGE("@Error in getting the attribute value");
		return ret;
	}
	_LOGD("Refrence-uri is %s\n",reference->uri);

	depth = xmlTextReaderDepth(reader);
	while ((ret = _ri_next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			// _d_msg(DEBUG_ERR, "node is NULL\n");
			return -1;
		}
		if (strcmp(ASCII(node), "DigestMethod") == 0) {
			digestmethod_x *digestmethod = calloc(1, sizeof(digestmethod_x));
			if (digestmethod == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(reference->digestmethod, digestmethod);
			ret = _ri_process_digestmethod(reader, digestmethod);
		} else if (strcmp(ASCII(node), "DigestValue") == 0) {
			digestvalue_x *digestvalue = calloc(1, sizeof(digestvalue_x));
			if (digestvalue == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(reference->digestvalue, digestvalue);
			ret = _ri_process_digestvalue(reader, digestvalue);
		} else if (strcmp(ASCII(node), "Transforms") == 0) {
			transforms_x *transforms = calloc(1, sizeof(transforms_x));
			if (transforms == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(reference->transforms, transforms);
			ret = _ri_process_transforms(reader, transforms);
		} else {
			// _d_msg(DEBUG_INFO, "Invalid tag %s", ASCII(node));
			return -1;
		}
		if (ret < 0)
			return ret;
	}

	SAFE_LISTHEAD(reference->digestmethod, tmp1);
	SAFE_LISTHEAD(reference->digestvalue, tmp2);
	SAFE_LISTHEAD(reference->transforms, tmp3);

	return ret;
}

static int _ri_process_x509certificate(xmlTextReaderPtr reader, x509certificate_x *x509certificate)
{
	int ret = -1;
	xmlTextReaderRead(reader);
	ret = __get_value(reader,&x509certificate->text);
	if(ret != 0){
		_LOGE("@Error in getting the value");
	}
	return ret;
}

static int _ri_process_x509data(xmlTextReaderPtr reader, x509data_x *x509data)
{
	const xmlChar *node = NULL;
	int ret = 0;
	int depth = 0;
	x509certificate_x *tmp1 = NULL;

	depth = xmlTextReaderDepth(reader);
	while ((ret = _ri_next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			// _d_msg(DEBUG_ERR, "node is NULL\n");
			return -1;
		}
		if (strcmp(ASCII(node), "X509Certificate") == 0) {
			x509certificate_x *x509certificate = calloc(1, sizeof(x509certificate_x));
			if (x509certificate == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(x509data->x509certificate, x509certificate);
			ret = _ri_process_x509certificate(reader, x509certificate);
		} else {
			// _d_msg(DEBUG_INFO, "Invalid tag %s", ASCII(node));
			return -1;
		}
		if (ret < 0)
			return ret;
	}

	SAFE_LISTHEAD(x509data->x509certificate, tmp1);

	return ret;
}

static int _ri_process_keyinfo(xmlTextReaderPtr reader, keyinfo_x *keyinfo)
{
	const xmlChar *node = NULL;
	int ret = 0;
	int depth = 0;
	x509data_x *tmp1 = NULL;

	depth = xmlTextReaderDepth(reader);
	while ((ret = _ri_next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			// _d_msg(DEBUG_ERR, "node is NULL\n");
			return -1;
		}
		if (strcmp(ASCII(node), "X509Data") == 0) {
			x509data_x *x509data = calloc(1, sizeof(x509data_x));
			if (x509data == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(keyinfo->x509data, x509data);
			ret = _ri_process_x509data(reader, x509data);
		} else {
			// _d_msg(DEBUG_INFO, "Invalid tag %s", ASCII(node));
			return -1;
		}
		if (ret < 0)
			return ret;
	}

	SAFE_LISTHEAD(keyinfo->x509data, tmp1);

	return ret;
}

static int _ri_process_signaturevalue(xmlTextReaderPtr reader, signaturevalue_x *signaturevalue)
{
	int ret = 0;
	xmlTextReaderRead(reader);
	ret = __get_value(reader,&signaturevalue->text);
	if(ret != 0){
		_LOGE("@Error in getting the value");
	}
	return ret;
}

static int _ri_process_signedinfo(xmlTextReaderPtr reader, signedinfo_x *signedinfo)
{
	const xmlChar *node = NULL;
	int ret = 0;
	int depth = 0;
	cannonicalizationmethod_x *tmp1 = NULL;
	signaturemethod_x *tmp2 = NULL;
	reference_x *tmp3 = NULL;

	depth = xmlTextReaderDepth(reader);
	while ((ret = _ri_next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			// _d_msg(DEBUG_ERR, "node is NULL\n");
			return -1;
		}
		if (strcmp(ASCII(node), "CanonicalizationMethod") == 0) {
			cannonicalizationmethod_x *cannonicalizationmethod = calloc(1, sizeof(cannonicalizationmethod_x));
			if (cannonicalizationmethod == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(signedinfo->cannonicalizationmethod, cannonicalizationmethod);
			ret = _ri_process_cannonicalizationmethod(reader, cannonicalizationmethod);
		} else if (strcmp(ASCII(node), "SignatureMethod") == 0) {
			signaturemethod_x *signaturemethod = calloc(1, sizeof(signaturemethod_x));
			if (signaturemethod == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(signedinfo->signaturemethod, signaturemethod);
			ret = _ri_process_signaturemethod(reader, signaturemethod);
		} else if (strcmp(ASCII(node), "Reference") == 0) {
			reference_x *reference = calloc(1, sizeof(reference_x));
			if (reference == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(signedinfo->reference, reference);
			ret = _ri_process_reference(reader, reference);
		} else {
			// _d_msg(DEBUG_INFO, "Invalid tag %s", ASCII(node));
			return -1;
		}
		if (ret < 0)
			return ret;
	}

	SAFE_LISTHEAD(signedinfo->cannonicalizationmethod, tmp1);
	SAFE_LISTHEAD(signedinfo->signaturemethod, tmp2);
	SAFE_LISTHEAD(signedinfo->reference, tmp3);

	return ret;
}

static int _ri_process_sign(xmlTextReaderPtr reader, signature_x *sigx)
{
	const xmlChar *node = NULL;
	int ret = 0;
	int depth = 0;
	signedinfo_x *tmp1 = NULL;
	signaturevalue_x *tmp2 = NULL;
	keyinfo_x *tmp3 = NULL;
	object_x *tmp4 = NULL;

	depth = xmlTextReaderDepth(reader);
	while ((ret = _ri_next_child_element(reader, depth))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			// _d_msg(DEBUG_ERR, "node is NULL\n");
			return -1;
		}
		if (strcmp(ASCII(node), "SignedInfo") == 0) {
			signedinfo_x *signedinfo = calloc(1, sizeof(signedinfo_x));
			if (signedinfo == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(sigx->signedinfo, signedinfo);
			ret = _ri_process_signedinfo(reader, signedinfo);
		} else if (strcmp(ASCII(node), "SignatureValue") == 0) {
			signaturevalue_x *signaturevalue = calloc(1, sizeof(signaturevalue_x));
			if (signaturevalue == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(sigx->signaturevalue, signaturevalue);
			ret = _ri_process_signaturevalue(reader, signaturevalue);
		} else if (strcmp(ASCII(node), "KeyInfo") == 0) {
			keyinfo_x *keyinfo = calloc(1, sizeof(keyinfo_x));
			if (keyinfo == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(sigx->keyinfo, keyinfo);
			ret = _ri_process_keyinfo(reader, keyinfo);
		} else if (strcmp(ASCII(node), "Object") == 0) {
			/*
			object_x *object = calloc(1, sizeof(object_x));
			if (object == NULL) {
				// _d_msg(DEBUG_ERR, "Calloc Failed\n");
				return -1;
			}
			LISTADD(sigx->object, object);
			ret = _ri_process_object(reader, object);
			*/
			continue;
		} else {
			// _d_msg(DEBUG_INFO, "Invalid tag %s", ASCII(node));
			return -1;
		}
		if (ret < 0)
			return ret;
	}

	SAFE_LISTHEAD(sigx->signedinfo, tmp1);
	SAFE_LISTHEAD(sigx->signaturevalue, tmp2);
	SAFE_LISTHEAD(sigx->keyinfo, tmp3);
	SAFE_LISTHEAD(sigx->object, tmp4);

	return ret;
}

static int _ri_process_signature(xmlTextReaderPtr reader, signature_x *sigx)
{
	const xmlChar *node = NULL;
	int ret = -1;

	if ((ret = _ri_next_child_element(reader, -1))) {
		node = xmlTextReaderConstName(reader);
		if (!node) {
			// _d_msg(DEBUG_ERR, "Node is null");
			return -1;
		}
		if (!strcmp(ASCII(node), "Signature")) {
			ret = __get_attribute(reader,"Id",&sigx->id);
			if(ret != 0){
				_LOGE("@Error in getting the attribute value");
				return ret;
			}
			_LOGD("sigx-id is %s\n",sigx->id);

			ret = __get_attribute(reader,"xmlns",&sigx->xmlns);
			if(ret != 0){
				_LOGE("@Error in getting the attribute value");
				return ret;
			}
			_LOGD("sigx-xmlns is %s\n",sigx->xmlns);

			ret = _ri_process_sign(reader, sigx);
		} else {
			// _d_msg(DEBUG_ERR, "No Signature element found\n");
			return -1;
		}
	}
	return ret;
}

signature_x *_ri_process_signature_xml(const char *signature_file)
{
	xmlTextReaderPtr reader;
	signature_x *sigx = NULL;

	reader = xmlReaderForFile(signature_file, NULL, 0);

	if (reader) {
		sigx = calloc(1, sizeof(signature_x));
		if (sigx) {
			if (_ri_process_signature(reader, sigx) < 0) {
				/* error in parsing. Let's display some hint where we failed */
				// _d_msg(DEBUG_ERR, "Syntax error in processing signature in the above line\n");
				_ri_free_signature_xml(sigx);
				xmlFreeTextReader(reader);
				return NULL;
			}
		} else {
			// _d_msg(DEBUG_ERR, "Calloc failed\n");
		}
		xmlFreeTextReader(reader);
	} else {
		// _d_msg(DEBUG_ERR, "Unable to create xml reader\n");
	}
	return sigx;
}

int __ps_verify_sig_and_cert(const char *sigfile, char *signature)
{
	char certval[BUFMAX] = { '\0'};
	int i = 0;
	int j= 0;
	int ret = PM_MDM_R_OK;
	signature_x *signx = NULL;
	struct keyinfo_x *keyinfo = NULL;
	struct x509data_x *x509data = NULL;

	signx = _ri_process_signature_xml(sigfile);
	retvm_if(signx == NULL, PM_MDM_R_OK, "Parsing %s failed", sigfile);

	keyinfo = signx->keyinfo;
	tryvm_if(keyinfo == NULL, ret = PM_MDM_R_OK, "Certificates missing in %s", sigfile);
	tryvm_if(keyinfo->x509data == NULL, ret = PM_MDM_R_OK, "x509data missing in %s", sigfile);
	tryvm_if(keyinfo->x509data->x509certificate == NULL, ret = PM_MDM_R_OK, "x509certificate missing in %s", sigfile);

	x509data = keyinfo->x509data;
	x509certificate_x *cert = x509data->x509certificate;
	if (cert->text != NULL) {
		for (i = 0; i <= (int)strlen(cert->text); i++) {
			if (cert->text[i] != '\n') {
				certval[j++] = cert->text[i];
			}
		}
		certval[j] = '\0';
		_LOGE("strlen[%d] cert_svc_load_buf_to_context() load %s", strlen(certval), certval);

		/*First cert is Signer certificate and check it with signature*/
		tryvm_if(strcmp(certval, signature)==0, ret = PM_MDM_R_ERROR, "signature is blacklist !!\n");
	}

catch:
	_ri_free_signature_xml(signx);
	signx = NULL;
	return ret;
}

static int __ps_check_signature_blacklist(manifest_x * mfx, char *signature)
{
	int ret = 0;
	char buff[PKG_STRING_LEN_MAX] = {'\0'};

	retvm_if(mfx == NULL, PM_MDM_R_OK, "Manifest pointer is NULL");
	retvm_if(signature == NULL, PM_MDM_R_OK, "signature pointer is NULL");

	snprintf(buff, PKG_STRING_LEN_MAX, "%s/author-signature.xml", mfx->root_path);

	if (access(buff, F_OK) == 0) {
		ret = __ps_verify_sig_and_cert(buff, signature);
		retvm_if(ret == PM_MDM_R_ERROR, PM_MDM_R_ERROR, "Failed to verify [%s]", buff);
	} else {
		_LOGE("No author-signature.xml file found[%s]\n", buff);
		return PM_MDM_R_OK;
	}

	return PM_MDM_R_OK;
}


static int __ps_check_privilege_blacklist(manifest_x * mfx, char *privlege)
{
	retvm_if(mfx == NULL, PM_MDM_R_OK, "Manifest pointer is NULL");
	retvm_if(privlege == NULL, PM_MDM_R_OK, "privlege pointer is NULL");

	privileges_x *pvs = NULL;
	privilege_x *pv = NULL;

	pvs = mfx->privileges;
	while (pvs != NULL) {
		pv = pvs->privilege;
		while (pv != NULL) {
			if (strcmp(pv->text, privlege)==0) {
				_LOGE("privilege[%s] is blacklist\n", pv->text);
				return PM_MDM_R_ERROR;
			}
			pv = pv->next;
		}
		pvs = pvs->next;
	}

	return PM_MDM_R_OK;
}

static int __ps_get_signature_blacklist(void *handle, manifest_x * mfx)
{
	retvm_if(mfx == NULL, PM_MDM_R_OK, "Manifest pointer is NULL");

	int ret = PM_MDM_R_OK;
	char *errmsg = NULL;

	mdm_data_t *(*__mdm_get_app_signature_blacklist)(void) = NULL;
	void (*__mdm_free_data)(mdm_data_t *data) = NULL;

	__mdm_get_app_signature_blacklist = dlsym(handle, "mdm_get_app_signature_blacklist");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__mdm_get_app_signature_blacklist == NULL), ret = PM_MDM_R_ERROR, "dlsym() failed. [%s]", errmsg);

	__mdm_free_data = dlsym(handle, "mdm_free_data");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__mdm_free_data == NULL), ret = PM_MDM_R_ERROR, "dlsym() failed. [%s]", errmsg);


	mdm_data_t *lp_data = __mdm_get_app_signature_blacklist();
	if (lp_data){
	   GList *blacklist = (GList *)lp_data->data;
	   while(blacklist) {
		   char *signature	= (char *)blacklist->data;
		   /*check signature and block installation*/
		   if (__ps_check_signature_blacklist(mfx, signature) == PM_MDM_R_ERROR) {
			   __mdm_free_data(lp_data);
			   return PM_MDM_R_ERROR;
		   }
		   blacklist = g_list_next(blacklist);
	   }
	   __mdm_free_data(lp_data);
	}

catch:
	return ret;
}

static int __ps_get_privilege_blacklist(void *handle, manifest_x * mfx)
{
	retvm_if(mfx == NULL, PM_MDM_R_OK, "Manifest pointer is NULL");

	int ret = PM_MDM_R_OK;
	char *errmsg = NULL;

	mdm_data_t *(*__mdm_get_app_privilege_blacklist)(void) = NULL;
	void (*__mdm_free_data)(mdm_data_t *data) = NULL;

	/*get MDM symbol*/
	__mdm_get_app_privilege_blacklist = dlsym(handle, "mdm_get_app_privilege_blacklist");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__mdm_get_app_privilege_blacklist == NULL), ret = PM_MDM_R_ERROR, "dlsym() failed. [%s]", errmsg);

	__mdm_free_data = dlsym(handle, "mdm_free_data");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__mdm_free_data == NULL), ret = PM_MDM_R_ERROR, "dlsym() failed. [%s]", errmsg);

	/*check list*/
	mdm_data_t *lp_data = __mdm_get_app_privilege_blacklist();
	if (lp_data){
	   GList *blacklist = (GList *)lp_data->data;
	   while(blacklist) {
		   char *privlege = (char *)blacklist->data;
		   /*check privilege and block installation*/
		   if (__ps_check_privilege_blacklist(mfx, privlege) == PM_MDM_R_ERROR) {
			   __mdm_free_data(lp_data);
			   return PM_MDM_R_ERROR;
		   }
		   blacklist = g_list_next(blacklist);
	   }
	   __mdm_free_data(lp_data);
	}

catch:
	return ret;
}

int __ps_check_mdm_policy(manifest_x * mfx, MDM_ACTION_TYPE action)
{
	int ret = PM_MDM_R_OK;

#ifdef TIZEN_MDM_ENABLE
	retvm_if(mfx == NULL, PM_MDM_R_OK, "Manifest pointer is NULL");

	char *errmsg = NULL;
	void *handle = NULL;

	mdm_result_t (*__mdm_get_service)(void) = NULL;
	int (*__mdm_release_service)(void) = NULL;
	mdm_status_t (*__mdm_get_application_installation_disabled)(const char *app_name) = NULL;
	mdm_status_t (*__mdm_get_application_uninstallation_disabled)(const char *app_name) = NULL;

	/*get MDM symbol*/
	handle = dlopen(LIBMDM_PATH, RTLD_LAZY | RTLD_GLOBAL);
	retvm_if(!handle, PM_MDM_R_OK, "dlopen() failed. [%s]", dlerror());

	__mdm_get_service = dlsym(handle, "mdm_get_service");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__mdm_get_service == NULL), ret = PM_MDM_R_OK, "dlsym() failed. [%s]", errmsg);

	__mdm_release_service = dlsym(handle, "mdm_release_service");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__mdm_release_service == NULL), ret = PM_MDM_R_OK, "dlsym() failed. [%s]", errmsg);

	__mdm_get_application_installation_disabled = dlsym(handle, "mdm_get_application_installation_disabled");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__mdm_get_application_installation_disabled == NULL), ret = PM_MDM_R_OK, "dlsym() failed. [%s]", errmsg);

	__mdm_get_application_uninstallation_disabled = dlsym(handle, "mdm_get_application_uninstallation_disabled");
	errmsg = dlerror();
	tryvm_if((errmsg != NULL) || (__mdm_get_application_uninstallation_disabled == NULL), ret = PM_MDM_R_OK, "dlsym() failed. [%s]", errmsg);


	/*connect MDM server*/
	ret = __mdm_get_service();
	tryvm_if(ret != MDM_RESULT_SUCCESS, ret = PM_MDM_R_OK, "can not connect mdm server");

	/*MDM server check disabled case*/
	switch (action) {
	case MDM_ACTION_INSTALL:
		ret = __mdm_get_application_installation_disabled(mfx->package);
		break;
	case MDM_ACTION_UNINSTALL:
		ret = __mdm_get_application_uninstallation_disabled(mfx->package);
		break;
	default:
		_LOGE("action type is not defined\n");
		dlclose(handle);
		return PM_MDM_R_OK;
	}
	tryvm_if(ret == MDM_DISABLED, ret = PM_MDM_R_ERROR, "pkgmgr parser[%d] is disabled by mdm", action);

#ifdef MDM_PHASE_2
	/*MDM server check privilege_blacklist*/
	ret = __ps_get_privilege_blacklist(handle, mfx);
	tryvm_if(ret == PM_MDM_R_ERROR, ret = PM_MDM_R_ERROR, "pkg[%s] has privilege blacklist", mfx->package);

	/*MDM server check signature_blacklist*/
	ret = __ps_get_signature_blacklist(handle, mfx);
	tryvm_if(ret == PM_MDM_R_ERROR, ret = PM_MDM_R_ERROR, "pkg[%s] has signature blacklist", mfx->package);
#endif

	/*release MDM server*/
	__mdm_release_service();

catch:

	dlclose(handle);
#endif

	return ret;
}

