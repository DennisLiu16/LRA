# LRA

## Reference

- [Google Cpp Guide](https://google.github.io/styleguide/cppguide.html)

## Goal

- Refactor the src tree
- Implement abstract based class system
- Unify coding style

## Third-Party Libraries

- [BS thread-pool](https://github.com/bshoshany/thread-pool)
- [spdlog](https://github.com/gabime/spdlog)
- [asio](https://github.com/chriskohlhoff/asio.git)
- [jsnocpp](https://github.com/open-source-parsers/jsoncpp.git)
- [websocketpp](https://github.com/zaphoyd/websocketpp.git)

## Todo

- Impl Interface isolation (template specialization ...)

## Dependency

```sh
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt install -y g++-10
sudo apt-get install libboost-all-dev
sudo apt-get install libfftw3-dev
sudo apt install libserialport-dev
sudo apt-get install libudev-dev
sudo apt-get install libusb-1.0-0-dev
```

## Installation

```sh
git clone --recurse-submodules -j8 https://github.com/DennisLiu16/LRA.git
cd LRA
git checkout usb_test_wsl
```

## Build
```sh
mkdir build
cd build
cmake .. && make
```

## Which bin you should run
./build/bin/lra_usb_util_test_v1.1
