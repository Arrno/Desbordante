#pragma once

#include <string_view>

#include "config/column_index/type.h"
#include "config/common_option.h"

namespace config {

// Simplifies creating of options that represent a single index of the attribute in the table
class ColumnIndexOption {
public:
    ColumnIndexOption(std::string_view name, std::string_view description,
                      typename Option<config::IndexType>::DefaultFunc get_default_value = nullptr);

    [[nodiscard]] std::string_view GetName() const;
    // Creates an option which performs a check that index is not greater than column count
    [[nodiscard]] Option<config::IndexType> operator()(
            config::IndexType* value_ptr, std::function<config::IndexType()> get_col_count,
            typename Option<config::IndexType>::ValueCheckFunc value_check_func = nullptr) const;

private:
    CommonOption<config::IndexType> const common_option_;
};

}  // namespace config
