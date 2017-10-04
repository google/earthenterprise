# check to make sure user is root
#if [[ $(id -u) != 0 ]]
#then
#	echo "This must be run as root. Exiting..."
#	exit 1
#fi

FAIL=6
SUCCESS=8
wgeturl="http://people.centos.org/tru/devtools-2/devtools-2.repo"
wgetloc="/etc/yum.repos.d/devtools-2.repo"
CENTOS="CentOS"
RHEL="RHEL"
SCRIPTNAME=$0
MINVER=6
MAXVER=7

# get contents of this file
contents=(`cat /etc/redhat-release`)
rev=(`echo ${contents[3]} | perl -pe 's/\./\ /g;'`)
dist=${contents[0]}

RHEL_CentOS_7()
{
	sudo yum install ant bzip2 doxygen gcc-c++ patch python-argparse python-setuptools swig tar
	sudo yum install -y scons perl-Perl4-CoreLibs xorg-x11-server-devel python-devel 
	sudo yum perl-Alien-Packages openssl-devel libxml2-devel libXinerama-devel libXft-devel 
	sudo yum install libXrandr-devel libXcursor-devel gdbm-devel libmng-devel libcap-devel libpng12-devel 
	sudo yum install libXmu-devel freeglut-devel zlib-devel libX11-devel bison-devel openjpeg-devel
	sudo yum install openjpeg2-devel geos-devel proj-devel ogdi-devel giflib-devel xerces-c xerces-c-devel cmake rpm-build rsync
	sudo yum install -y gtest-devel
}

install_git()
{
	sudo yum install -y git
	sudo yum install curl
	curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | sudo bash
	sudo yum install git-lfs
}

do_centos()
{
	if [ $1 -ge $MINVER ] && [ $1 -le $MAXVER ]
	then
		
		if [ $1 -eq $MINVER ]
		then
			echo "centos6"
		else
			sudo yum install epel-release
			install_git
			RHEL_CentOS_7
		fi
	else
		return $FAIL
	fi
}

do_rhel()
{
	if [ $1 -ge $MINVER ] && [ $1 -le $MAXVER ]
	then
		
		if [ $1 -eq $MINVER ]
		then
			echo "rhel6"
		else
			sudo yum install https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
			install_git
		fi
	else
		return $FAIL
	fi
}

#case $version in
#	"CentOS6") ;;&
#	"CentOS7") echo "CentOS";;
#	"RHEL6"  ) ;;&
#	"RHEL7"	 ) echo "rhel";;
#	*	 ) echo "BUILD_RHEL_CentOS.sh Error: version not supported"; 
#		   exit $FAIL;;
#esac

if [ "$dist" == "$CENTOS" ]
then
	do_centos ${rev[0]}
elif [ "$dist" == "$RHEL" ]
then
	do_rhel ${rev[0]}
else
	echo "$SCRIPTNAME Error: $dist not supported"
	exit $FAIL
fi

exit $SUCCESS
