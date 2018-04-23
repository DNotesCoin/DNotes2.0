#!/bin/bash

set -e

date
ps axjf

#################################################################
# Update Ubuntu and install prerequisites for running DNotes   #
#################################################################
sudo apt-get update
#################################################################
# Build DNotes from source                                     #
#################################################################
NPROC=$(nproc)
echo "nproc: $NPROC"
#################################################################
# Install all necessary packages for building DNotes           #
#################################################################
sudo apt-get install -y qt4-qmake libqt4-dev libminiupnpc-dev libdb++-dev libdb-dev libcrypto++-dev libqrencode-dev libboost-all-dev build-essential libboost-system-dev libboost-filesystem-dev libboost-program-options-dev libboost-thread-dev libboost-filesystem-dev libboost-program-options-dev libboost-thread-dev libssl-dev libdb++-dev libssl-dev ufw git
sudo add-apt-repository -y ppa:bitcoin/bitcoin
sudo apt-get update
sudo apt-get install -y libdb4.8-dev libdb4.8++-dev

cd /usr/local
file=/usr/local/dnotes
if [ ! -e "$file" ]
then
        sudo git clone https://github.com/dnotescoin/dnotes2.0.git
fi

cd /usr/local/dnotes/src
file=/usr/local/dnotes/src/dnotesd
if [ ! -e "$file" ]
then
        sudo make -j$NPROC -f makefile.unix
fi

sudo cp /usr/local/dnotes/src/dnotesd /usr/bin/dnotesd

################################################################
# Configure to auto start at boot                                      #
################################################################
file=$HOME/.dnotes
if [ ! -e "$file" ]
then
        sudo mkdir $HOME/.dnotes
fi
printf '%s\n%s\n%s\n%s\n' 'daemon=1' 'server=1' 'rpcuser=u' 'rpcpassword=p' | sudo tee $HOME/.dnotes/dnotes.conf
file=/etc/init.d/dnotes
if [ ! -e "$file" ]
then
        printf '%s\n%s\n' '#!/bin/sh' 'sudo dnotesd' | sudo tee /etc/init.d/dnotes
        sudo chmod +x /etc/init.d/dnotes
        sudo update-rc.d dnotes defaults
fi

/usr/bin/dnotesd
echo "DNotes has been setup successfully and is running..."
exit 0

