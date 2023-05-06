/*
 * File: cnc_file_loader.hpp
 * Created Date: 2023-04-15
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Wednesday April 19th 2023 11:57:44 am
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include <fft_lib/third_party/csv.h>

#include <cstdint>
#include <host_usb_lib/logger/logger.hpp>
#include <string>
#include <unordered_map>
#include <variant>

namespace lra::fft_lib {
using lra::usb_lib::Log;

struct Position {
  std::size_t row;
  std::size_t column;
};

template <typename DataType>
struct CsvDataPoint_t {
  using type = DataType;
  std::string name{};
  DataType value{};
  Position pos{.row = 0, .column = 0};
};

template <typename DataType>
using CsvDataPoint = CsvDataPoint_t<DataType>;

template <typename... Ts>
using CsvDataPointVariantTemplate = std::variant<CsvDataPoint<Ts>...>;

using CsvDataPointVariant =
    CsvDataPointVariantTemplate<int, float, double, std::string, uint32_t>;

template <typename T>
CsvDataPoint<T> make_info(std::string name, uint32_t row, uint32_t column) {
  CsvDataPoint<T> dp{.name = name, .pos = {.row = row, .column = column}};
  return dp;
}

/* TODO: https://godbolt.org/z/q3bzexzf6, 但是依賴模板輸入 */
/* TODO: https://godbolt.org/z/GG9Ev7n18, 想不出來... */

namespace detail {
auto get_dp_name = [](const auto& dp) { return dp.name; };

template <typename T, bool Match>
struct TupleBuilder {
  static auto build(T& data) { return std::tuple<std::string>{""}; }
};

template <typename T>
struct TupleBuilder<T, true> {
  static auto build(T& data) { return std::tuple<T&>{data}; }
};

template <unsigned TargetColNum, typename Reader, typename T, std::size_t... Is>
auto read_row_with_col_helper(Reader& reader, T& data,
                              std::index_sequence<Is...>) {
  auto tu =
      std::tuple_cat(TupleBuilder<T, (Is == TargetColNum)>::build(data)...);

  bool result = std::apply(
      [&reader](auto&... cols) { return reader.read_row(cols...); }, tu);
  return result;
}

template <unsigned TargetColNum, unsigned N, typename T>
auto read_row_with_col(io::CSVReader<N>& reader, T& data) {
  static_assert(TargetColNum < N);

  return read_row_with_col_helper<TargetColNum>(reader, data,
                                                std::make_index_sequence<N>{});
}

}  // namespace detail

/* Load a CNC three axes force data */
template <unsigned ColNum_>
class CncFileLoader {
 public:
  static constexpr unsigned ColNum = ColNum_;

  explicit CncFileLoader(std::string csv_path) : csv_path_(csv_path) {
    addCsvInfo(make_info<float>("sampling_rate", 11, 2));
    getCsvFileInfo();
  }

  /*  */
  void addCsvInfo(const CsvDataPointVariant info) {
    std::string dp_name = std::visit(detail::get_dp_name, info);
    file_info_[dp_name] = info;
  }

  auto getDPValue(CsvDataPointVariant dp) const {
    return std::visit([](auto& _dp) { return _dp.value; }, dp);
  }

  auto getFileInfoValue(std::string name) {
    return getDPValue(file_info_[name]);
  }

  /* updateDPvalue from csv */
  void updateDPValue(auto& dp) {
    std::visit(
        [&](auto& dp) {
          // get something like int
          // using T = typename std::decay_t<decltype(dp)>::type;

          io::CSVReader<ColNum> in(csv_path_);
          skipLines(in, dp.pos.row - 1);

          detail::read_row_with_col(in, dp.value, dp.pos.column - 1);

          /* log info */
          Log("{}: {}\n", dp.name, dp.value);
        },
        dp);
  }

  /* getFileInfo */
  void getCsvFileInfo() {
    Log("Start to get CNC CSV information: {}\n",
        fmt::format(fg(fmt::terminal_color::bright_blue), csv_path_));
    try {
      for (auto& info : file_info_) {
        updateDPValue(info.second);
      }

    } catch (std::exception& e) {
      Log(fg(fmt::terminal_color::red), "getCsvFileInfo exception: {}\n",
          e.what());
    }
  }

  void readHeader() {}

  void getCsvFileData() {}

 private:
  /* getNthLine */
  void skipLines(auto& csv_reader, uint32_t lines) {
    for (uint32_t i = 0; i < lines; i++) csv_reader.next_line();
  }

  std::string csv_path_;
  std::unordered_map<std::string, CsvDataPointVariant> file_info_;
};
}  // namespace lra::fft_lib