#!/bin/bash

# constants
SCRIPTNAME=$0
CENTOS="CentOS"
RHEL="Red Hat"
UBUNTU="Ubuntu"
USERNAME="google"
GIT="/usr/bin/git"
RH_REL="/etc/redhat-release"
LSB_REL="/usr/bin/lsb_release"
UBUNTU_SCRIPT="./BUILD_Ubuntu.sh"
RH_COS_SCRIPT="./BUILD_RHEL_CentOS.sh"

UNKNOWN_DISTRIBUTION=0
IS_RHEL_OR_CENTOS=1
IS_UBUNTU=2
ALREADYCLONED=3
GITFAIL=4
GITSUCCESS=5
RHEL_CENTOS_FAIL=6
UBUNTU_FAIL=7

# check if the user has git
# if not: will install
# if so: return
git_check()
{
	if [ ! -f $GIT ] # /usr/bin/git ]
	then
		echo "$SCRIPTNAME: git not present, installing..."
		if [ $1 -eq $IS_RHEL_OR_CENTOS ] 
		then
			 echo "yum install -y git"
		elif [ $1 -eq $IS_UBUNTU ]
		then
			echo "apt-get install git"
		fi
	else
		echo "$SCRIPTNAME: git present...."
	fi	
}

get_distribution()
{
	# first, check to see if /etc/redhat-release is present
	# this means that the distribution is either RHEL or CentOS
	if [ -a $RH_REL ] # /etc/redhat-release ]
	then
		# dump the contents of /etc/redhat-release into an array
		contents=(`cat $RH_REL`) # /etc/redhat-release`)
		dist=${contents[0]}
		if [ "$dist" == "$CENTOS" ] || [ "$dist" == "$RHEL" ]
		then
			# retval=`./BUILD_RHEL_CentOS.sh`
			#do_compile
			return $IS_RHEL_OR_CENTOS
		fi	
	fi

	# next, check for /usr/bin/release to get other versions
	if [ -a $LSB_REL ] # /usr/bin/lsb_release ] 
	then
		#lsb_release exists, get distribution
		dist=`lsb_release -si`
		if [ "$dist" == "$UBUNTU" ]
		then
			# echo `./BUILD_Ubuntu.sh`
			return $IS_UBUNTU 
		fi
	fi
	return $UNKNOWN_DISTRIBUTION
}

clone_Ubuntu()
{
	USERNAME=$1
	echo "****************************************"
	echo "*  Make sure that a fork has been created" 
	echo "*  and that a public SSH key has been "
	echo "*  uploaded to your github profile. Also," 
	echo "*  make certain that both the private and"
	echo "*  public keys (called id_rsa and "
	echo "*  id_rsa.pub) which correspond to the "
	echo "*  github public key reside in ~/.ssh"
	echo "**************************************"
	echo
	echo -n "$SCRIPTNAME: press enter to continue"
	read wait_for_input
	#echo "$SCRIPTNAME: git clone https://$USERNAME@github.com/earthenterprise.git"
	#git clone https://$USERNAME@github.com/$USERNAME/earthenterprise.git
	echo "$SCRIPTNAME: git clone git@github.com:$USERNAME/earthenterprise.git"
	git clone git@github.com:$USERNAME/earthenterprise.git
}

clone_rhel_centos()
{
	USERNAME=$1
	echo "****************************************"
	echo "*  Make sure that a fork has been created" 
	echo "****************************************"
	echo
	echo -n "$SCRIPTNAME: press enter to continue"
	read wait_for_input
	echo "$SCRIPTNAME: git clone https://$USERNAME@github.com/$USERNAME/earthenterprise.git"
	git clone https://$USERNAME@github.com/$USERNAME/earthenterprise.git	
}

clone_code()
{
	# check to see if already cloned
	if [ -d "./earthenterprise" ]
	then
		echo "$SCRIPTNAME Error: earthenterprise already cloned!"
		return $ALREADYCLONED	
	fi

	echo -n "$SCRIPTNAME: clone from master or fork [ m/f ]: "
	read choice
	while :
	do
		if [ "$choice" == "m" ] || [ "$choice" == "f" ]
		then
			break;
		fi
		echo "$SCRIPTNAME: only valid input is m or f"
		echo -n "$SCRIPTNAME: clone from master or fork [ m/f ]: "
		read choice
	done
	if [ "$choice" == "m" ]
	then
		echo "$SCRIPTNAME: cloning from master"
		echo "$SCRIPTNAME: git clone https://github.com/google/earthenterprise.git"
		git clone https://github.com/google/earthenterprise.git
		exit 0
	else

		echo "$SCRIPTNAME: cloning from fork"
		echo -n "$SCRIPTNAME: enter username: "
		read USERNAME
		if [ "$dist" == "$UBUNTU" ]
		then
			clone_Ubuntu $USERNAME
		else
			clone_rhel_centos $USERNAME
		fi
		
	fi

	retval=$?
	if [ $retval -eq 0 ]
	then
		return $GITFAIL
	fi
	return $GITSUCCESS
}

die()
{
	# if we reach here, then we know that the distribution isnt' supported
	echo "$SCRIPTNAME Error: The only supported distributions are:"		
	echo "	- Red Hat Enterprise Linux 6/7"
	echo "	- CentOS 6/7"
	echo "	- Ubuntu 14.04/16.04"
	exit 1
}

get_distribution
dist=$?
retval=0
if [ $dist -eq $IS_RHEL_OR_CENTOS ]
then
	git_check $IS_RHEL_OR_CENTOS
	#./BUILD_RHEL_CentOS.sh
	${RH_COS_SCRIPT}	
	retval=$?
	if [ $retval -eq $RHEL_CENTOS_FAIL ]
	then
		die
	fi

elif [ $dist -eq $IS_UBUNTU ]
then
	git_check $IS_UBUNTU
	#./BUILD_Ubuntu.sh
	${UBUNTU_SCRIPT}
	retval=$?
else
	die
fi

if [ $retval -eq 0 ]
then
	die
fi

echo -n "$SCRIPTNAME: clone code [ y/n ]? "
read clone
while :
do
	if [ "$clone" == "y" ] || [ "$clone" == "n" ] 
	then
		break
	fi

	echo "$SCRIPTNAME: only valid input is y or n"
	echo -n "$SCRIPTNAME: clone code [ y/n ]? "
	read clone
done

if [ "$clone" == "y" ]
then
	clone_code
fi 
