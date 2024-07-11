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
sudo apt install libserial-dev
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

## Trouble Shooting

- Encounter `Bad file descriptor` when opening RCWS

    If you encounter `Bad file descriptor` when opening ACM devices, please check following instructions. The issue is caused by tty permissions. Please check if the current user is in the `dialout` group. If not, execute:

    ```bash
    sudo usermod -aG dialout <username>
    ```
    
    and then log out and log back in or restart the system. This will permanently resolve the issue. As a temporary workaround, you can run the command with `sudo` to elevate permissions.

## Which bin you should run
./build/bin/lra_usb_util_test_v1.1
