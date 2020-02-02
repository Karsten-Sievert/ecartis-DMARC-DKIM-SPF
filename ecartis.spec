Summary: Ecartis modular mailing list manager
Name: ecartis
Vendor: NodeRunner Software
Version: 1.0.0
Release: 1
Copyright: GPL
Group: Applications/Mail
Source: ftp://ftp.ecartis.org/pub/ecartis/%{name}-%{version}.tar.gz
URL: http://www.ecartis.org/
Prefix: /home/ecartis
Packager: Ecartis Project <bugs@ecartis.org>
Buildroot: /var/tmp/ecartis-root

%changelog
* Sun Aug 5 2001 Nils Vogels <nivo@is-root.com>
- Converted SPEC file to Ecartis

* Wed Jun 21 2000 Rachel Blackman <sparks@noderunner.net>
- Added detection of SMRSH to RPM install.
- Removed old RHCN information since RHCN is dead.

* Tue Apr  4 2000 Rachel Blackman <sparks@noderunner.net>
- Updated the spec for the template directory.

* Fri Jun  2 1999 JT Traub <jtraub@dragoncat.net>
- Updated the spec for multiple changelog files.

* Mon Mar  8 1999 JT Traub <jtraub@dragoncat.net>
- Added the changelog section to the SPEC file.

%description
Ecartis is a modular mailing list manager; all its functionality is
encapsulated in individual 'epm' (Ecartis Plugin Module) files.  This
allows new commands and functionality to be added on the fly.  Ecartis
has several useful features, including the ability to have 'flags' set on
user accounts (similar to L-soft Listserv), and a very secure remote
administration method over e-mail.

Errors to this package should be reported to bugs@ecartis.org or via the
web at http://bugs.ecartis.org/ecartis

NOTE: This package used to be named Listar, but has recently changed name
due to trademark issues.

%prep
%setup -q

%build
cd src
cp Makefile.dist Makefile
make install

%install
rm -Rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT/home/ecartis/{queue,modules,lists,scripts,templates}
install -s ecartis $RPM_BUILD_ROOT/home/ecartis
install -s modules/*.lpm $RPM_BUILD_ROOT/home/ecartis/modules
install scripts/* $RPM_BUILD_ROOT/home/ecartis/scripts
install ecartis.cfg.dist $RPM_BUILD_ROOT/home/ecartis/ecartis.cfg
install ecartis.aliases.dist $RPM_BUILD_ROOT/home/ecartis/ecartis.aliases
install banned $RPM_BUILD_ROOT/home/ecartis/banned
install spam-regexp.sample $RPM_BUILD_ROOT/home/ecartis/spam-regexp.sample
install ecartis.hlp $RPM_BUILD_ROOT/home/ecartis/ecartis.hlp
install templates/*.lsc $RPM_BUILD_ROOT/home/ecartis/templates

%pre
[ "`id -gn ecartis 2> /dev/null`" = "ecartis" ] || {
        cat <<EOF

This package requires an account for user "ecartis" with primary group
"ecartis".  We test [ "\`id -gn ecartis 2> /dev/null\`" = "ecartis" ].  Please
create the required account and try again.

On a Red Hat 5.x system you can create a suitable account like this:

        useradd -d /home/ecartis -s /bin/false ecartis

where /home/ecartis is the prefix you provided for install and defaults to
/home/ecartis.  If you wanted to install in /var/ecartis, for example, you
would make that the user's home directory, and add --prefix /var/ecartis
to the RPM command installing this package.

The ecartis user does NOT need any special permissions, it is merely to
ensure file ownership stays safe, and that Ecartis is suid to something
non-root when called by a mailserver.

Aborting.

EOF
        exit 1
}
exit 0

%post
# Detect SMRSH
if [ -e /etc/smrsh -a ! -e /etc/smrsh/ecartis ]; then
    echo "#!/bin/sh" > %{prefix}/ecartis-wrapper
    echo "%{prefix}/ecartis $@" >> %{prefix}/ecartis-wrapper
    chmod ug+rx %{prefix}/ecartis-wrapper

    echo "Your installation has been detected to have SMRSH, the SendMail"
    echo "Restricted SHell, installed.  If this is your first install, you"
    echo "will want to: "
    echo ""
    echo "1) cp %{prefix}/ecartis-wrapper /etc/smrsh/ecartis"
    echo "2) add 'listserver-bin-dir = /etc/smrsh' to ecartis.cfg"
    echo "3) change the address for Ecartis in the aliases file to be"
    echo "   /etc/smrsh/ecartis instead of /home/ecartis/ecartis"
fi

# Force the /home/ecartis directory permissions to something sane
chmod 711 %{prefix}

# Usual post-install note
cat << EOF
You will need to make some changes to your sendmail/other SMTP server
installation (such as adding alias files so that Ecartis aliases have
somewhere to live).  Very rudimentary documentation is in the installed
documentation directory in the file README.

If you have any problems, please feel free to subscribe to
the support list at ecartis-support@ecartis.org and we'll try to help you.

Simply send mail to ecartis-support-request@ecartis.org with a subject of
subscribe, a'la:

mail -s "subscribe" ecartis-support-request@ecartis.org < /dev/null

EOF

# Run upgrade
%{prefix}/ecartis -upgrade
exit 0

%clean
rm -Rf $RPM_BUILD_ROOT

%files
%defattr(-,ecartis,ecartis)
%doc README README.* NOTE LICENSE PLATFORMS src/CHANGELOG src/CHANGELOG.*
%attr(711,ecartis,ecartis)  %dir %{prefix}
%attr(4755,ecartis,ecartis) %{prefix}/ecartis
%attr(751,ecartis,ecartis)  %dir %{prefix}/lists
%attr(750,ecartis,ecartis)  %dir %{prefix}/queue
%attr(750,ecartis,ecartis)  %dir %{prefix}/scripts
%attr(750,ecartis,ecartis)  %{prefix}/scripts/*
%attr(750,ecartis,ecartis)  %dir %{prefix}/modules
%attr(750,ecartis,ecartis)  %{prefix}/modules/*
%attr(750,ecartis,ecartis)  %dir %{prefix}/templates
%config %{prefix}/ecartis.cfg
%config %{prefix}/ecartis.aliases
%config %{prefix}/banned
%{prefix}/spam-regexp.sample
%{prefix}/ecartis.hlp
%config %{prefix}/templates/*
