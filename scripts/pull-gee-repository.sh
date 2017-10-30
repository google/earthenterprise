#!/usr/bin/env bash

#
# Copyright 2017 Open GEE Contributors
#
# Licensed under the Apache license, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the license.
#

#===============================================================================
#
#          FILE:  pull-gee-repository.sh
# 
#         USAGE:  ./pull-gee-repository.sh 
# 
#   DESCRIPTION:  Acts as a frontend to allow for easier cloning of codebase 
# 
#  REQUIREMENTS:  Must not be root 
#       VERSION:  1.0
#===============================================================================


#===============================================================================
#       Constants  
#===============================================================================
THIS=$(basename "$0")
USER_NAME=""
PASSWORD=""
DIR_NAME="earthenterprise"
IS_SSH=1
IS_HTTPS=2
IS_MAIN=3
IS_FORK=4

CLONE_REPO=$IS_MAIN
CLONE_TYPE=""

#===  FUNCTION  ================================================================
#          NAME:  is_cloned
#   DESCRIPTION:  Determines if already cloned
#    PARAMETERS:  none
#       RETURNS:  1, if cloned; 0, if not cloned
#===============================================================================
function is_cloned ()
{
    [ -d "$DIR_NAME" ];
}


#===  FUNCTION  ================================================================
#          NAME:  get_type 
#   DESCRIPTION:  returns the repo to clone, main or fork 
#    PARAMETERS:  n/a
#       RETURNS:  IS_FORK, if fork; IS_MAIN, if main
#===============================================================================
function get_type ()
{
    printf "Do you want to clone the (m)ain repo or a (f)ork (default is main)? "
    read CLONE_REPO
    if ! [ "$CLONE_REPO" = "m" ]
    then
        return $IS_FORK  
    fi
    return $IS_MAIN
}

#===  FUNCTION  ================================================================
#          NAME:  clone_ssh
#   DESCRIPTION:  will cone the repo over ssh
#    PARAMETERS:  n/a
#       RETURNS:  n/a
#===============================================================================
function clone_ssh ()
{
    if ! [ -n "$USER_NAME" ]
    then
        echo "Error: no USER_NAME given!"
        exit 1
    fi

    echo "Attempting to clone fork $USER_NAME..."
    echo "git clone git@github.com:$USER_NAME/earthenterprise.git $DIR_NAME"
    git clone git@github.com:$USER_NAME/earthenterprise.git $DIR_NAME
    echo $?
}

#===  FUNCTION  ================================================================
#          NAME:  clone_https
#   DESCRIPTION:  Will clone the repo over https
#    PARAMETERS:  $1 = type: can be either IS_MAIN or IS_FORK
#       RETURNS:  n/a
#===============================================================================
function clone_https ()
{
    if [ "$1" -eq "$IS_MAIN" ]
    then
        echo "Attempting to clone main..." 
        echo "git clone https://github.com/google/earthenterprise.git $DIR_NAME"
        git clone https://github.com/google/earthenterprise.git $DIR_NAME
    else
        if ! [ -n "$USER_NAME" ]
        then
            echo "Error: no USER_NAME given!"
            exit 1
        fi
	if ! [ -n "$PASSWORD" ]
        then
            echo "Error: no password given!"
            exit 1
        fi
        password=":$PASSWORD"
        len=${#PASSWORD}
        echo
        echo "Attempting to clone fork $USER_NAME/earthenterprise.git..."

        # hide the password from display through *
        hidden=""
        for ((i=0;i<$len;i+=1))
        do
            if [[ $i -eq 0 ]]
            then
                hidden=":"
            fi
            hidden="$hidden*"
        done

        echo "git clone https://$USER_NAME$hidden@github.com/$USER_NAME/earthenterprise.git $DIR_NAME" 
        git clone https://$USER_NAME$password@github.com/$USER_NAME/earthenterprise.git $DIR_NAME 
    fi
}   


#===  FUNCTION  ================================================================
#          NAME:  check_root
#   DESCRIPTION:  Checks to see if the user is root. root will cause some
#                 permissions issues with the code, so do not allow
#    PARAMETERS:  n/a
#       RETURNS:  n/a, will die if root detected
#===============================================================================
function check_root ()
{
    if [ $(id -u) = 0 ]
    then
        echo "Error: do not run as root"
        exit 1
    fi
} 


#===  FUNCTION  ================================================================
#          NAME:  ssh_or_https
#   DESCRIPTION:  allows the user to decide between ssh or https cloning
#    PARAMETERS:  n/a
#       RETURNS:  IS_SSH for ssh, IS_HTTPS for https
#===============================================================================
function ssh_or_https ()
{
    printf "Do you want to clone via (h)ttps or (s)sh (default is https)? " 
    read clonetype
    if [ "$clonetype" = "s" ]
    then
        return $IS_SSH
    fi   
    return $IS_HTTPS
}

#===  FUNCTION  ================================================================
#          NAME:  get_password
#   DESCRIPTION:  prompts user to enter password and sets global
#    PARAMETERS:  n/a
#       RETURNS:  n/a
#===============================================================================
function get_password ()
{
    printf "What is your password? "
    read -s PASSWORD 
}

#===  FUNCTION  ================================================================
#          NAME:  get_username
#   DESCRIPTION:  prompts user to enter username and sets global
#    PARAMETERS:  n/a
#       RETURNS:  n/a
#===============================================================================
function get_username ()
{
    printf "What is your username? "
    read USER_NAME 
}

#===  FUNCTION  ================================================================
#          NAME:  get_dir
#   DESCRIPTION:  allows the user to set the directory name to clone into
#    PARAMETERS:  n/a
#       RETURNS:  n/a
#===============================================================================

function get_dir ()
{
    printf "Which directory do you wish to clone into (default is ./earthenterprise) ? "
    read DIR
    if [ -n "$DIR" ]
    then
        DIR_NAME=$DIR
    fi
}

#===  FUNCTION  ================================================================
#          NAME:  print_help
#   DESCRIPTION:  prints help info
#    PARAMETERS:  n/a
#       RETURNS:  n/a
#===============================================================================
function print_help ()
{
    cat <<MSG

${THIS} [-d <dir>] [-f <fork>] [-h] [-i] [-p <password>]

    
    -d|--dir <dir>
        Directory under which to clone into (default is "earthenterprise")
    
    -f|--fork <fork>
        Clone from forked repository. Default is SSH

    -h|--help
        Show this help message and exit.

    -i|--interactive
        Allows for user interaction

    -p|--password <password>
        User password. Only forked repositories cloned in HTTPS use this

MSG
}

#===  FUNCTION  ================================================================
#          NAME:  interactive_mode
#   DESCRIPTION:  allows for user interaction
#    PARAMETERS:  n/a
#       RETURNS:  n/a
#===============================================================================

function interactive_mode ()
{
    get_dir
    get_type
    CLONE_REPO=$?

    if is_cloned
    then
        echo "$DIR_NAME is already cloned!"
        exit 1 
    fi
    
    # if cloning main, just use standard https
    if [ $CLONE_REPO -eq $IS_MAIN ]
    then
        clone_https $IS_MAIN
        exit 0
    fi

    #cloning fork

    #get USER_NAME and whether ssh or https
    get_username
    ssh_or_https
    clonetype=$?
    if [ $clonetype = $IS_SSH ]
    then
        clone_ssh
    else
        get_password
        clone_https $IS_FORK
    fi
}

#===  FUNCTION  ================================================================
#          NAME:  parse_cmd_line_args
#   DESCRIPTION:  parses any command line arguments
#    PARAMETERS:  n/a
#       RETURNS:  n/a
#===============================================================================
function parse_cmd_line_args ()
{
    while [[ "$#" -gt 0 ]]
    do
        case "$1" in
            -d|--dir)
                DIR_NAME="$2"
                if ! [ -n "$DIR_NAME" ]
                then
                    echo "Error: no directory specified!"
                    exit 1
                fi
                shift
                ;;
            -f|--fork)
                CLONE_REPO=$IS_FORK
                USER_NAME="$2"
                if ! [ -n "$CLONE_TYPE" ]
                then
                    CLONE_TYPE=$IS_SSH
                fi
                shift
                ;;
            -h|--help)
                print_help
                exit 0     
                ;;
            -i|--interactive)
                interactive_mode
                exit 0
                ;;
            -p|--password)
                CLONE_REPO=$IS_FORK
                CLONE_TYPE=$IS_HTTPS
                PASSWORD="$2"
                shift
                ;;
            *)
                echo "Unrecognized command-line argument: $1" >&2
                exit 1
                ;;
        esac
        shift
    done
}

#================================================================================
#	MAIN 
#================================================================================
check_root
parse_cmd_line_args "$@"

if [ $CLONE_REPO -eq $IS_MAIN ]
then
    clone_https $IS_MAIN
    exit 0
fi

#cloning fork
if [ $CLONE_TYPE = $IS_SSH ]
then
    clone_ssh
else
    clone_https $IS_FORK
fi

exit 0
