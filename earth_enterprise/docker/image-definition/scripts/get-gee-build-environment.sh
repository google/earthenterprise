#! /bin/bash

# This scripts installs packages, downloads and buils software used to
# download the Open GEE source code and build it.
#   It runs on RHEL 6, RHEL 7, Cent OS 6, Cent OS 7,
# Ubuntu 14.04, Ubuntu 16.04.
#   This script does not share code with other shell scripts so that it can be
# distributed as a single file that can be used to obtain a build environment
# and source code for Open GEE.

SELF_NAME=$(basename "$0")

MIN_GCC_VERSION="4.8"

# Where to build packages (RPMs) that are not provided with the distribution 
# we're running on (when necessary):
: ${PACKAGE_BUILD_DIR:=""}

# Whether to delete the package build directory before exiting:
: ${CLEAN_PACKAGE_BUILD_DIR:="yes"}

# Where we download packages (RPMs) that are not provided in repositories
# (Yum):
: ${PACKAGE_DOWNLOAD_DIR:=""}

# Whether to delete the package download directory before exiting:
: ${CLEAN_PACKAGE_DOWNLOAD_DIR:="yes"}

# Whether to assume answer of yes to all questions (used when installing
# packages).  Set to true to run without user interaction.
: ${ASSUME_YES:=""}

# Open GEE repository URL for cloning:
: ${CLONE_URL:="https://github.com/google/earthenterprise.git"}

# Git branch to clone:
: ${CLONE_BRANCH:=""}

# Whether to clone the Open GEE repository:
: ${CLONE_REPOSITORY:="yes"}


# Check for a known package manager:
if type apt-get >/dev/null 2>&1; then
    PACKAGE_MANAGER="apt-get"
elif type yum >/dev/null 2>&1; then
    PACKAGE_MANAGER="yum"
else
    echo "No known package manager found!  (The known are \`apt-get\` and \`yum\`.)" >&2
    exit 1
fi

# Check if we're on RHEL:
if type subscription-manager >/dev/null 2>&1; then
    ARE_WE_ON_RHEL="yes"
else
    ARE_WE_ON_RHEL=""
fi

# Extract RHEL version numbers:
if [ "$PACKAGE_MANAGER" == "yum" ]; then
    RED_HAT_VERSION_STRING=$(cut -d ":" -f 5 /etc/system-release-cpe)
    RED_HAT_VERSION=$(echo "$RED_HAT_VERSION_STRING" | grep -e "[0-9.]*" -o)
    RED_HAT_VERSION_MAJOR=$(echo "$RED_HAT_VERSION" | cut -d "." -f 1)
fi


is_string_value_false()
{
    case $(echo "$1" | tr '[:upper:]' '[:lower:]') in
        false|off|no|0)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

# Tests whether a version represented by a version string is greater than, or
# equal to a given version, also represented as a string:
is_version_ge()
{
    local TESTED_VERSION="$1"
    local MIN_VERSION="$2"

    [ "$(printf "${MIN_VERSION}\n${TESTED_VERSION}" | sort -V | head -n 1)" == "$MIN_VERSION" ]
}

make_package_build_dir()
{
    if [ -n "$PACKAGE_BUILD_DIR" ]; then
        mkdir -p "$PACKAGE_BUILD_DIR" || return 1
    else
        PACKAGE_BUILD_DIR=$(mktemp --tmpdir --directory "opengee-pkgbuild.XXXXXX")
    fi
}

clean_package_build_dir()
{
    if [ -n "$PACKAGE_BUILD_DIR" ]; then
        # Minimize `rm -Rf` damage by only removing known directories:
        rm -Rf "$PACKAGE_BUILD_DIR/gtest-devtoolset-rpm"
        rmdir "$PACKAGE_BUILD_DIR"
    fi
}

make_package_download_dir()
{
    if [ -n "$PACKAGE_DOWNLOAD_DIR" ]; then
        mkdir -p "$PACKAGE_DOWNLOAD_DIR" || return 1
    else
        PACKAGE_DOWNLOAD_DIR=$(mktemp --tmpdir --directory "opengee-pkgdownload.XXXXXX")
    fi
}

clean_package_download_dir()
{
    if [ -n "$PACKAGE_DOWNLOAD_DIR" ]; then
        echo rm -Rf "$PACKAGE_DOWNLOAD_DIR"
    fi
}

install_wget()
{
    if ! type wget >/dev/null 2>&1; then
        "$PACKAGE_MANAGER" install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER wget
    fi
}

# Installs the EPEL Yum repository or RHEL, or Cent OS:
install_epel()
{
    if [ -n "$ARE_WE_ON_RHEL" ]; then
        make_package_download_dir && \
        install_wget && \
        (   cd "$PACKAGE_DOWNLOAD_DIR" && \
            wget https://dl.fedoraproject.org/pub/epel/epel-release-latest-${RED_HAT_VERSION_MAJOR}.noarch.rpm && \
            yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER \
                "epel-release-latest-${RED_HAT_VERSION_MAJOR}.noarch.rpm"
        )
    else
        # On Cent OS:
        yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER epel-release
    fi
}

# Installs the devtoolset-2 toolchain on RHEL 6 or Cent OS 6.
install_devtoolset_toolchain()
{
    if [ -n "$ARE_WE_ON_RHEL" ]; then
        # We're on RHEL:
        yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER devtoolset-2-toolchain || return 1
    else
        # On Cent OS:
        install_wget || return 1
        ( cd /etc/yum.repos.d && wget http://people.centos.org/tru/devtools-2/devtools-2.repo ) \
            || return 1
        yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER devtoolset-2-toolchain || return 1
    fi
}


# Builds an RPM and installs Google Test with a devtoolset toolchain.
#   We need the compiler from the devtoolset-2 toolchain on RHEL 6 and
# Cent OS 6, because the distribution GCC is too old to compile Open GEE.
#   We need to build our own gtest with the devtooset-2-toolchain so it works
# with the GCC compiler from that toolchain.
install_gtest_devtoolset_rpm()
{
    #   The 'cmake', gettext' and 'rpm-build' packages are needed for building the
    # Google Test RPM (for `cmake`, `envsubst` and `rpmbuild`).
    yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER cmake gettext rpm-build || return 1
    make_package_build_dir || return 1
    ( cd "$PACKAGE_BUILD_DIR" && \
        git clone https://github.com/thermopylae/gtest-devtoolset-rpm.git && \
        cd gtest-devtoolset-rpm && \
        ./bin/build.sh && \
        yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER \
            "build/RPMS/$(uname -m)/gtest-devtoolset2-1.8.0-1.$(uname -m).rpm"
    )
}

if [ "$PACKAGE_MANAGER" == "yum" ]; then
    install_git_lfs()
    {
        curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh \
            | bash || return 1
        yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER install git-lfs
    }
elif [ "$PACKAGE_MANAGER" == "apt-get" ]; then
    install_git_lfs()
    {
        curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh \
            | bash || return 1
        apt-get $ASSUME_YES_PACKAGE_MANAGER_PARAMETER install git-lfs
    }
fi


if [ "$PACKAGE_MANAGER" == "yum" ]; then
    # RHEL or Cent OS:
    install_packages()
    {
        yum update $ASSUME_YES_PACKAGE_MANAGER_PARAMETER || return 1
        install_epel || return 1
        yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER git || return 1

        # Enable optional Yum repositories on RHEL 6 before installing compilers:
        if [ -n "$ARE_WE_ON_RHEL" ]; then
            subscription-manager repos \
                "--enable=rhel-${RED_HAT_VERSION_MAJOR}-server-optional-rpms" \
                || return 1
            subscription-manager repos \
                "--enable=rhel-${RED_HAT_VERSION_MAJOR}-server-optional-source-rpms" \
                || return 1
        fi

        # Install development / build tools:
        yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER \
            ant bzip2 doxygen gcc-c++ patch python-argparse \
            python-setuptools rsync swig tar || return 1

        # We need to use an alternative toolchain on RHEL 6 and Cent OS 6 to
        # get GCC 4.8+.  We need to make sure libgtest works with the
        # toolchain we're using.
        if is_version_ge "$(gcc -dumpversion)" "$MIN_GCC_VERSION"; then
            yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER gtest-devel
        else
            install_devtoolset_toolchain || return 1
            install_gtest_devtoolset_rpm || return 1
        fi

        #   The "X Window System" is not needed for building, but for running
        # the Fusion UI.
        #   The 'pexpect' and 'python-tornado' packages are not needed for 
        # Earth Server, or Fusion, but they are for Portable Server.
        yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER \
            bison-devel freeglut-devel gdbm-devel geos-devel giflib-devel \
            libcap-devel libmng-devel libX11-devel libXcursor-devel \
            libXft-devel libXinerama-devel libXmu-devel libXrandr-devel \
            libxml2-devel \
            ogdi-devel openjpeg-devel openjpeg2-devel openssl-devel \
            proj-devel python-devel scons \
            xerces-c xerces-c-devel xorg-x11-server-devel \
            zlib-devel || return 1
        yum groupinstall $ASSUME_YES_PACKAGE_MANAGER_PARAMETER \
            "X Window System" || return 1
        yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER \
            pexpect python-tornado
        
        # libpng 1.2 has different names on RHEL / Cent OS 6 and 7:
        if yum list libpng12-devel; then
            yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER libpng12-devel \
                || return 1
        else
            yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER libpng-devel \
                || return 1
        fi

        # RHEL / Cent OS 6 don't have some packages, but Open GEE builds without them:
        if yum list perl-Alien-Packages; then
            yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER \
                perl-Alien-Packages || return 1
        fi

        if yum list perl-Perl4-CoreLibs; then
            yum install $ASSUME_YES_PACKAGE_MANAGER_PARAMETER \
                perl-Perl4-CoreLibs || return 1
        fi

        install_git_lfs
    }
elif [ "$PACKAGE_MANAGER" == "apt-get" ]; then
    install_packages()
    {
        # The 'pexpect' package is not needed for Earth Server, or Fusion, 
        # but it is for Portable Server.
        apt-get $ASSUME_YES_PACKAGE_MANAGER_PARAMETER update || return 1
        apt-get $ASSUME_YES_PACKAGE_MANAGER_PARAMETER install \
            alien autoconf automake bison++ bisonc++ curl \
            doxygen flex freeglut3-dev g++ gettext gcc \
            libc6 libc6-dev libcap-dev libfreetype6 libfreetype6-dev \
            libgdbm-dev libgeos-dev libgeos++-dev libgif-dev libgtest-dev \
            libjpeg-dev libjpeg8-dev libmng-dev libogdi3.2-dev \
            libperl4-corelibs-perl libpng12-0 libpng12-dev libpq-dev \
            libproj-dev libstdc++6 libtool libx11-dev libxcursor-dev \
            libxerces-c-dev libxft-dev libxinerama-dev libxml2-dev \
            libxml2-utils libxrandr-dev openssl \
            python-dev python2.7 python2.7-dev python-imaging \
            python-setuptools rsync scons software-properties-common swig \
            wget xorg-dev zlib1g-dev || return 1
        apt-get $ASSUME_YES_PACKAGE_MANAGER_PARAMETER install python-pexpect \
            || return 1

        # Install Git:
        # add-apt-repository ppa:git-core/ppa || return 1
        # apt-get $ASSUME_YES_PACKAGE_MANAGER_PARAMETER update || return 1
        apt-get $ASSUME_YES_PACKAGE_MANAGER_PARAMETER install git || return 1

        install_git_lfs
    }
fi

clone_repository()
{
    if [ -d earthenterprise -a -d earthenterprise/.git ]; then
        echo "The GEE Open Source repository has already been cloned. Pulling latest updates"
        cd earthenterprise
        if [ -n "$CLONE_BRANCH" ]; then
            git checkout "$CLONE_BRANCH"
        fi
        git pull
        git lfs pull
    else
        if [ -z "$CLONE_BRANCH" ]; then
            GIT_LFS_SKIP_SMUDGE=1 git clone --depth 1 "$CLONE_URL"
        else
            GIT_LFS_SKIP_SMUDGE=1 git clone -b "$CLONE_BRANCH" --depth 1 "$CLONE_URL"
        fi
        cd earthenterprise
        git lfs pull
    fi
}


print_command_line_help()
{
    cat <<MSG
${SELF_NAME} [-r] [-b <branch_name>] [-p <dir>] [-d <dir>] [-c] [-l] [-y] [-h]

    -r|--clone-repository
        Specify '--clone-repository=no' to skip cloning the Open GEE source
        code repository.

    -b|--branch <branch_name>
        Repository branch to clone.

    -p|--package-build-dir <dir>
        Directory under which to build packages (.rpm, .deb) that are not
        provided by the distribution we're running on.
    
    -d|--package-download-dir <dir>
        Directory under which to download packages (.rpm, .deb) which are not
        in package repositories.
    
    -c|--clean-package-build-dir
        Specify '--clean-package-build-dir=no' to not delete the package build
        directory before exiting.
    
    -l|--clean-package-download-dir
        Specify '--clean-package-download-dir=no' to not delete the package
        download directory before exiting.
    
    -y|--yes|--assume-yes
        Specify '--assume-yes' to answer yes to all prompts. (Used when
        installing packages.)  Use this option to run without user
        interaction.
    
    -h|--help
        Show this help message and exit.
MSG
}


# Parse command-line arguments:
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -r|--clone-repository)
            CLONE_REPOSITORY="yes"
            ;;
        -r=*|--clone-repository=*)
            if is_string_value_false "${1#*=}"; then
                CLONE_REPOSITORY=""
            else
                CLONE_REPOSITORY="yes"
            fi
            ;;
        -b|--branch)
            CLONE_BRANCH="$2"
            shift
            ;;
        -b=*|--branch=*)
            CLONE_BRANCH="${1#*=}"
            ;;
        -p|--package-build-dir)
            PACKAGE_BUILD_DIR="$2"
            shift
            ;;
        -p=*|--package-build-dir=*)
            PACKAGE_BUILD_DIR="${1#*=}"
            ;;
        -d|--package-download-dir)
            PACKAGE_DOWNLOAD_DIR="$2"
            shift
            ;;
        -d=*|--package-download-dir=*)
            PACKAGE_DOWNLOAD_DIR="${1#*=}"
            ;;
        -c|--clean-package-build-dir)
            CLEAN_PACKAGE_BUILD_DIR="yes"
            ;;
        -c=*|--clean_package_build_dir=*)
            if is_string_value_false "${1#*=}"; then
                CLEAN_PACKAGE_BUILD_DIR=""
            else
                CLEAN_PACKAGE_BUILD_DIR="yes"
            fi
            ;;
        -l|--clean-package-download-dir)
            CLEAN_PACKAGE_DOWNLOAD_DIR="yes"
            ;;
        -l=*|--clean-package-download-dir=*)
            if is_string_value_false "${1#*=}"; then
                CLEAN_PACKAGE_DOWNLOAD_DIR=""
            else
                CLEAN_PACKAGE_DOWNLOAD_DIR="yes"
            fi
            ;;
        -y|--yes|--assume-yes)
            ASSUME_YES="yes"
            ;;
        -y=*|--yes=*|--assume-yes=*)
            if is_string_value_false "${1#*=}"; then
                ASSUME_YES=""
            else
                ASSUME_YES="yes"
            fi
            ;;
        -h|--help)
            print_command_line_help
            exit 0
            ;;
        *)
            echoh "Unrecognized command-line argument: $1" >&2
            exit 1
            ;;
    esac
    shift
done


if [ -n "$ASSUME_YES" ]; then
    ASSUME_YES_PACKAGE_MANAGER_PARAMETER="-y"
else
    ASSUME_YES_PACKAGE_MANAGER_PARAMETER=""
fi


install_packages || exit 1

if [ -n "$CLEAN_PACKAGE_BUILD_DIR" ]; then
    clean_package_build_dir || exit 1
fi

if [ -n "$CLEAN_PACKAGE_DOWNLOAD_DIR" ]; then
    clean_package_download_dir || exit 1
fi

if [ -n "$CLONE_REPOSITORY" ]; then
    clone_repository || exit 1
fi
