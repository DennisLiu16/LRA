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
# g++10 or 11
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt install -y g++-10

# boost
sudo apt-get install libboost-all-dev
```

## Installation

```sh
git clone --recurse-submodules -j8 https://github.com/DennisLiu16/LRA.git
```

## USB_Test

1. v1.1.1: LRA trigger
