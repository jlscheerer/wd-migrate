#ifndef HANDLER_ENTITY_COUNT_HANDLER_
#define HANDLER_ENTITY_COUNT_HANDLER_

#include <array>
#include <cstdint>
#include <vector>

#include <type_traits>
#include <unordered_map>

#include "wikidata_handler.h"

namespace wd_migrate {
struct entity_count_handler : public skip_novalue_handler {
public:
  auto summary() -> void {
    std::cout << "# entities: " << entity_counts_.size() << std::endl;
    static const std::array target_counts{1, 2, 3, 4, 5, 10, 100, 1000};
    std::vector<int> counts(std::size(target_counts));
    for (const auto &[_, cnt] : entity_counts_) {
      for (int index = 0; index < std::size(target_counts); ++index) {
        const int limit = target_counts[index];
        if (cnt <= limit) {
          ++counts[index];
        }
      }
    }
    for (int index = 0; index < std::size(target_counts); ++index) {
      std::cout << "  edge_count(" << target_counts[index]
                << "): " << counts[index] << std::endl;
    }
  }

public:
  template <typename columns_type, typename result_type>
  auto handle(const columns_type &columns, const result_type &value) {
    ++count_;
    const std::string &entity_id =
        columns.template get_field<detail::kEntityId>();
    ++entity_counts_[entity_id];
    if constexpr (std::is_same_v<result_type, wd_entity_id_t>) {
      ++entity_counts_[value.value];
    }
  }

  using skip_novalue_handler::handle;

private:
  std::uint64_t count_;
  std::unordered_map<std::string, std::uint64_t> entity_counts_;
};
} // namespace wd_migrate

#endif // !HANDLER_ENTITY_COUNT_HANDLER_
