#include <ios>
#include <iostream>

#include "fast-cpp-csv-parser/csv.h"
#include "handler/wikidata_handler.h"
#include "parser/qualifiers_parser.h"
#include "utils/progress_indicator.h"

auto main(int argc, char **argv) -> int {
  using namespace wd_migrate;
  if (argc <= 1) {
    std::cout << "usage: " << argv[0] << " <filename>" << std::endl;
    return -1;
  }
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  auto handler = stacked_handler(stats_handler());
  qualifiers_parser<decltype(handler)> parser;
  parser.parse(argv[1], &handler);

  handler.get<stats_handler>().summary();

  return 0;
}
