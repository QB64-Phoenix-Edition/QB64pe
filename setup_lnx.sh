#!/bin/bash
# QB64-PE Installer
# Argument 1: If not blank, qb64pe will not be started after compilation

dont_run="$1"

#This checks the currently installed packages for the one's QB64-PE needs
#And runs the package manager to install them if that is the case
pkg_install() {
  #Search
  packages_to_install=
  for pkg in $pkg_list; do
    if [ -z "$(echo "$installed_packages" | grep $pkg)" ]; then
      packages_to_install="$packages_to_install $pkg"
    fi
  done
  if [ -n "$packages_to_install" ]; then
    echo "Installing required packages. If prompted to, please enter your password."
    $installer_command $packages_to_install
  fi

}



#Make sure we're not running as root
if [ $EUID == "0" ]; then
  echo "You are trying to run this script as root. This is highly unrecommended."
  echo "This script will prompt you for your sudo password if needed to install packages."
  exit 1
fi

GET_WGET=
#Path to Icon
#Relative Path to icon -- Don't include beginning or trailing '/'
QB64_ICON_PATH="source"

#Name of the Icon picture
QB64_ICON_NAME="qb64pe.png"

DISTRO=

lsb_command=`which lsb_release 2> /dev/null`
if [ -z "$lsb_command" ]; then
  lsb_command=`which lsb_release 2> /dev/null`
fi

#Outputs from lsb_command:

#Arch Linux  = arch
#Debian      = debian
#Fedora      = Fedora
#KUbuntu     = ubuntu
#LUbuntu     = ubuntu
#Linux Mint  = linuxmint
#Ubuntu      = ubuntu
#Slackware   = slackware
#VoidLinux   = voidlinux
#XUbuntu     = ubuntu
#Zorin       = Zorin
if [ -n "$lsb_command" ]; then
  DISTRO=`$lsb_command -si | tr '[:upper:]' '[:lower:]'`
elif [ -e /etc/arch-release ]; then
  DISTRO=arch
elif [ -e /etc/debian_version ] || [ -e /etc/debian_release ]; then
  DISTRO=debian
elif [ -e /etc/fedora-release ]; then
  DISTRO=fedora
elif [ -e /etc/redhat-release ]; then
  DISTRO=redhat
elif [ -e /etc/centos-release ]; then
  DISTRO=centos
fi

#Find and install packages
if [ "$DISTRO" == "arch" ]; then
  echo "ArchLinux detected."
  pkg_list="gcc make zlib curl $GET_WGET"
  installed_packages=`pacman -Q`
  installer_command="sudo pacman -S "
  pkg_install
elif [ "$DISTRO" == "linuxmint" ] || [ "$DISTRO" == "ubuntu" ] || [ "$DISTRO" == "debian" ] || [ "$DISTRO" == "zorin" ]; then
  echo "Debian based distro detected."
  pkg_list="build-essential x11-utils mesa-common-dev libglu1-mesa-dev libasound2-dev libpng-dev libcurl4-openssl-dev $GET_WGET"
  installed_packages=`dpkg -l`
  installer_command="sudo apt-get -y install "
  pkg_install
elif [ "$DISTRO" == "fedora" ] || [ "$DISTRO" == "redhat" ] || [ "$DISTRO" == "centos" ]; then
  echo "Fedora/Redhat based distro detected."
  pkg_list="gcc-c++ make mesa-libGLU-devel alsa-lib-devel libpng-devel libcurl-devel $GET_WGET"
  installed_packages=`yum list installed`
  installer_command="sudo yum install "
  pkg_install
elif [ "$DISTRO" == "voidlinux" ]; then
   echo "VoidLinux detected."
   pkg_list="gcc make glu-devel libpng-devel alsa-lib-devel libcurl-devel $GET_WGET"
   installed_packages=`xbps-query -l |grep -v libgcc`
   installer_command="sudo xbps-install -Sy "
   pkg_install

elif [ -z "$DISTRO" ]; then
  echo "Unable to detect distro, skipping package installation"
  echo "Please be aware that for QB64-PE to compile, you will need the following installed:"
  echo "  OpenGL development libraries"
  echo "  ALSA development libraries"
  echo "  GNU C++ Compiler (g++)"
  echo "  libpng"
fi

echo "Compiling and installing QB64-PE..."
make clean OS=lnx
make OS=lnx BUILD_QB64=y -j3

if [ -e "./qb64pe" ]; then
  echo "Done compiling!!"

  echo "Creating ./run_qb64pe.sh script..."
  _pwd=`pwd`
  echo "#!/bin/sh" > ./run_qb64pe.sh
  echo "cd $_pwd" >> ./run_qb64pe.sh
  echo "./qb64pe &" >> ./run_qb64pe.sh
  
  chmod +x ./run_qb64pe.sh
  #chmod -R 777 ./
  echo "Adding QB64-PE menu entry..."
  cat > ~/.local/share/applications/qb64pe.desktop <<EOF
[Desktop Entry]
Name=QB64-PE Programming IDE
GenericName=QB64-PE Programming IDE
Exec=$_pwd/run_qb64pe.sh
Icon=$_pwd/$QB64_ICON_PATH/$QB64_ICON_NAME
Terminal=false
Type=Application
Categories=Development;IDE;
Path=$_pwd
StartupNotify=false
EOF

  if [ -z "$dont_run" ]; then
    echo "Running QB64-PE..."
    ./qb64pe &
  fi

  echo "QB64-PE is located in this folder:"
  echo "`pwd`"
  echo "There is a ./run_qb64pe.sh script in this folder that should let you run qb64pe if using the executable directly isn't working."
  echo 
  echo "You should also find a QB64-PE option in the Programming/Development section of your menu you can use."
else
  ### QB64-PE didn't compile
  echo "It appears that the qb64pe executable file was not created, this is usually an indication of a compile failure (You probably saw lots of error messages pop up on the screen)"
  echo "Usually these are due to missing packages needed for compilation. If you're not running a distro supported by this compiler, please note you will need to install the packages listed above."
  echo "If you need help, please feel free to post on the QB64 Phoenix Edition Forums detailing what happened and what distro you are using."
  echo "Also, please tell them the exact contents of this next line:"
  echo "DISTRO: $DISTRO"
fi
echo
echo "Thank you for using the QB64-PE installer."
