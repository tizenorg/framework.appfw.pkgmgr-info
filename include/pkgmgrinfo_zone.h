#ifndef __PKGMGRINFO_ZONE_H__
#define __PKGMGRINFO_ZONE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* APIs for zone-feature */


/**
 * @brief Set the zone name so that other APIs will be applied
 *
 * @remark This API is only available in host side
 * @param[in] zone The new zone name. If it is NULL, it means host zone.
 * @param[out] old_zone The previous zone name. Caller should provide buffer for old_zone.
 * @param[in] len Length of the old_zone buffer. At most len bytes will be copied to the old_zone
 * @return 0 if success, negative value(<0) if fail.
 *
 */
int pkgmgrinfo_pkginfo_set_zone(const char *zone, char *old_zone, int len);

int zone_pkgmgr_parser_enable_pkg(const char *pkgid, char *const tagv[], const char *zone);

int zone_pkgmgr_parser_disable_pkg(const char *pkgid, char *const tagv[], const char *zone);

int zone_pkgmgr_parser_set_app_background_operation(const char *appid, bool is_disable, const char *zone);

#ifdef __cplusplus
}
#endif
#endif /* __PKGPMGRINFO_ZONE_H__ */
