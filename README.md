Please refer to directory 

*GridBallastTest/controller_example/* 

for specific examples of how to add controller to the water heaters using .glm files, and how to use the binary gridlabd.bin in MAC OS.

If the binary version is not working, you can always compile the gridlabd from the source by following the commands below.


```{bash}
cd ~
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
./configure prefix='/tmp/temp'
make
make install
```
