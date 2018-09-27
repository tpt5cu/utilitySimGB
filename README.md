## Overview

This repository holds the modified version of GridLAB-D for the utility-scale GridBallast simulations.

In short, this is a version of GridLAB-D with a new object, "gridballast", which can act as a load controller for waterheater and zipload objects.

## Build and Installation

Tested and passing on MacOS as of 2017.

```sh
git clone -b feature/730 https://github.com/jingkungao/gridlab-d.git
cd gridlab-d 
cd third_party 
chmod +x install_xercesc 
. install_xercesc 
tar -xvf cppunit-1.12.0.tar
cd cppunit-1.12.0
./configure LDFLAGS="-ldl"
make 
sudo make install
cd ../..
autoreconf -isf 
./configure 
make 
sudo make install
gridlabd --version
```

## Configuring the GridBallast Controller

See project team.
