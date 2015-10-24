Name:       pkgmgr-info
Summary:    Packager Manager infomation api for package
Version:    0.0.230
Release:    1
Group:      Application Framework/Package Management
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
BuildRequires:	cmake
BuildRequires:	pkgconfig(dlog)
BuildRequires:	pkgconfig(vconf)
BuildRequires:	pkgconfig(sqlite3)
BuildRequires:	pkgconfig(db-util)
BuildRequires:	pkgconfig(libxml-2.0)
BuildRequires:	pkgconfig(dbus-1)
BuildRequires:	pkgconfig(dbus-glib-1)
BuildRequires:	pkgconfig(openssl)
BuildRequires:	pkgconfig(bundle)
BuildRequires:	pkgconfig(vasum)

%define appfw_feature_expansion_pkg_install 1
%define appfw_feature_delta_update 1
%define appfw_feature_mount_install 0

%description
Packager Manager infomation api for packaging

%package devel
Summary:    Packager Manager infomation api (devel)
Group:		Development/Libraries
Requires:   %{name} = %{version}-%{release}
%description devel
Packager Manager infomation api (devel)

%package parser
Summary:    Library for manifest parser
Group:      Application Framework/Package Management
Requires:   %{name} = %{version}-%{release}

%description parser
Library for manifest parser

%package parser-devel
Summary:    Dev package for libpkgmgr-parser
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description parser-devel
Dev package for libpkgmgr-parser


%prep
%setup -q

%build

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif

%if "%{?tizen_profile_name}" == "wearable"
export CFLAGS="$CFLAGS -D_APPFW_FEATURE_PROFILE_WEARABLE"
%endif

%if 0%{?appfw_feature_expansion_pkg_install}
_EXPANSION_PKG_INSTALL=ON
%else
_EXPANSION_PKG_INSTALL=OFF
%endif

%if 0%{?appfw_feature_delta_update}
_DELTA_UPDATE=ON
%else
_DELTA_UPDATE=OFF
%endif

%if 0%{?appfw_feature_mount_install}
_MOUNT_INSTALL=ON
%else
_MOUNT_INSTALL=OFF
%endif


%cmake . -D_APPFW_FEATURE_EXPANSION_PKG_INSTALL:BOOL=${_EXPANSION_PKG_INSTALL} \
		-D_APPFW_FEATURE_DELTA_UPDATE:BOOL=${_DELTA_UPDATE} \
		-DTIZEN_FULL_VERSION=%{tizen_full_version} \
		-D_APPFW_FEATURE_MOUNT_INSTALL:BOOL=${_MOUNT_INSTALL}

make %{?jobs:-j%jobs}

%install
%make_install

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}
cp LICENSE %{buildroot}/usr/share/license/%{name}-parser

%post
mkdir -p /opt/usr/apps/tmp
chown 5100:5100 /opt/usr/apps/tmp
chmod 771 /opt/usr/apps/tmp
chsmack -a '*' /opt/usr/apps/tmp
chsmack -t /opt/usr/apps/tmp

touch /opt/usr/apps/tmp/pkgmgr_tmp.txt

chsmack -a 'pkgmgr::db' /opt/usr/apps/tmp/pkgmgr_tmp.txt

mkdir /usr/etc/package-manager
chsmack -a '_' /usr/etc/package-manager

%postun


%files
%manifest pkgmgr-info.manifest
%defattr(-,root,root,-)
%{_libdir}/libpkgmgr-info.so.*
/usr/share/license/%{name}
%attr(0755,root,root) /opt/etc/dump.d/module.d/dump_pkgmgr.sh

%files devel
%defattr(-,root,root,-)
%{_includedir}/pkgmgr-info.h
%{_includedir}/pkgmgrinfo_resource.h
%{_includedir}/pkgmgrinfo_basic.h
%{_includedir}/pkgmgrinfo_type.h
%{_includedir}/pkgmgrinfo_zone.h
%{_libdir}/pkgconfig/pkgmgr-info.pc
%{_libdir}/libpkgmgr-info.so

%files parser
%manifest pkgmgr-parser.manifest
%defattr(-,root,root,-)
%{_libdir}/libpkgmgr_parser.so.*
%{_prefix}/etc/package-manager/preload/manifest.xsd
%{_prefix}/etc/package-manager/preload/xml.xsd
%{_prefix}/etc/package-manager/preload/res.xsd
%{_prefix}/etc/package-manager/parser_path.conf
%{_prefix}/etc/package-manager/parserlib/pkgmgr_parser_plugin_list.txt
/usr/share/license/%{name}-parser
%if 0%{?appfw_feature_delta_update}
%{_prefix}/etc/package-manager/preload/delta_info.xsd
%endif

%files parser-devel
%defattr(-,root,root,-)
%{_includedir}/pkgmgr/pkgmgr_parser.h
%{_includedir}/pkgmgr/pkgmgr_parser_db.h
%{_includedir}/pkgmgr/pkgmgr_parser_resource.h
%{_libdir}/pkgconfig/pkgmgr-parser.pc
%{_libdir}/libpkgmgr_parser.so
