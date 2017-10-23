#!/usr/bin/env bash

# Copyright 2017 OpenGee Contributors

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
USER_NAME=""
DIR_NAME="earthenterprise"
IS_SSH=1
IS_HTTPS=2
IS_MAIN=3
IS_FORK=4
SSH_DIR=".ssh"
SSH_KEY_PRIV="id_rsa"
SSH_KEY_PUB="$SSH_KEY_PRIV.pub"

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
    read clone_type
    if ! [ "$clone_type" = "m" ]
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
    WD="$(pwd)"
    cd
    if [ ! -f "$SSH_DIR/$SSH_KEY_PRIV" ] || [ ! -f "$SSH_DIR/$SSH_KEY_PUB" ]
    then
        echo "SSH Key(s) missing!"
        exit 1
    fi
    cd $WD
    echo "Attempting to clone fork $USER_NAME..."
    echo "git clone git@github.com:$USER_NAME/earthenterprise.git"
    git clone git@github.com:$USER_NAME/earthenterprise.git
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
        echo "git clone https://github.com/google/earthenterprise.git"
        git clone https://github.com/google/earthenterprise.git
    else
        printf "what is your password? "
        # -s suppresses output but still reads data
        read -s password

        len=${#password}
        if ! [ -z "$password" ]
        then
            password=":$password"
        fi
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

        echo "git clone https://$USER_NAME$hidden@github.com/$USER_NAME/$DIR_NAME.git" 
        git clone https://$USER_NAME$password@github.com/$USER_NAME/$DIR_NAME.git 
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
    if ! [ "$clonetype" = "h" ]
    then
        return $IS_SSH
    fi   
    return $IS_HTTPS
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


#================================================================================
#	MAIN 
#================================================================================
check_root
if is_cloned
then
    echo "$DIR_NAME is already cloned!"
    exit 1 
fi
get_type
CLONE_TYPE=$?

# if cloning main, just use standard https
if [ $CLONE_TYPE -eq $IS_MAIN ]
then
    clone_https $IS_MAIN
    exit 0
fi

#cloning fork

#get username and whether ssh or https
get_username
ssh_or_https
clonetype=$?

if [ $clonetype = $IS_SSH ]
then
    clone_ssh
else
    clone_https $IS_FORK
fi
