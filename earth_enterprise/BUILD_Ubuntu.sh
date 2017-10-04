# check to make sure user is root
#if [[ $(id -u) != 0 ]]
#then
#	echo "This must be run as root. Exiting..."
	#exit 1
#fi

SCRIPTNAME=$0

minmajor=14
maxmajor=16
minminor=4
FAIL=7

# distribution is Ubuntu, get rev
rev=(`lsb_release -sr | perl -pe 's/\./\ /g;'`)
major=${rev[0]}
minor=${rev[1]}


if [ "$major" -ge "$minmajor" ] && [ "$major" -le "$maxmajor" ] && [ "$minor" -ge "$minminor" ]
then
	# Ubuntu 14.04 needs git 
	sudo apt-get install git
	# install git lfs
	echo "$SCRIPTNAME: installing git-lfs..."
	echo "$SCRIPTNAME: curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | sudo bash"
	# curl doesn't come by default, so install
	sudo apt-get install curl
	curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | sudo bash
	echo "$SCRIPTNAME: sudo  apt-get install git-lfs"
	sudo apt-get install git-lfs
	# install dependencies
	echo "$SCRIPTNAME: installing dependencies..."
	#echo "$SCRIPTNAME: apt-get install gcc g++ scons automake autoconf libperl4-corelibs-perl libtool xorg-dev doxygen python-dev alien swig libgtest-dev libstdc++6 libxml2-dev gettext libxinerama-dev libxft-dev libxrandr-dev libxcursor-dev libgdbm-dev libc6 libc6-dev libmng-dev zlib1g-dev libcap-dev libpng12-0 libpng12-dev freeglut3-dev flex libx11-dev bison++ bisonc++ libjpeg-dev libjpeg8-dev python2.7 python2.7-dev python-psycopg2 libogdi3.2-dev libgif-dev libxerces-c-dev libgeos-dev libgeos++-dev libfreetype6 libfreetype6-dev python-imaging libproj-dev python-setuptools libgif-dev libxerces-c-dev libcap-dev libpq-dev openssl libxml2-utils libxmu-dev"
	sudo apt-get install gcc g++ scons automake autoconf libperl4-corelibs-perl libtool xorg-dev doxygen python-dev alien swig libgtest-dev libstdc++6 libxml2-dev gettext libxinerama-dev libxft-dev libxrandr-dev libxcursor-dev libgdbm-dev libc6 libc6-dev libmng-dev zlib1g-dev libcap-dev libpng12-0 libpng12-dev freeglut3-dev flex libx11-dev bison++ bisonc++ libjpeg-dev libjpeg8-dev python2.7 python2.7-dev python-psycopg2 libogdi3.2-dev libgif-dev libxerces-c-dev libgeos-dev libgeos++-dev libfreetype6 libfreetype6-dev python-imaging libproj-dev python-setuptools libgif-dev libxerces-c-dev libcap-dev libpq-dev openssl libxml2-utils libxmu-dev
else
	echo "BUILD_Ubuntu.sh Error: The only supported Ubuntu distributions are 14.04 and 16.04"
	exit $FAIL
fi

exit 1
