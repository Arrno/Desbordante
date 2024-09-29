#include "config/column_index/option.h"

#include "config/column_index/validate_index.h"

namespace config {

ColumnIndexOption::ColumnIndexOption(
        std::string_view name, std::string_view description,
        typename Option<config::IndexType>::DefaultFunc get_default_value)
    : common_option_(name, description, std::move(get_default_value)) {}

std::string_view ColumnIndexOption::GetName() const {
    return common_option_.GetName();
}

Option<config::IndexType> ColumnIndexOption::operator()(
        config::IndexType* value_ptr, std::function<config::IndexType()> get_col_count,
        typename Option<config::IndexType>::ValueCheckFunc value_check_func) const {
    Option<config::IndexType> option = common_option_(value_ptr);
    option.SetValueCheck(
            [get_col_count = std::move(get_col_count),
             value_check_func = std::move(value_check_func)](config::IndexType const index) {
                config::ValidateIndex(index, get_col_count());
                if (value_check_func) {
                    value_check_func(index);
                }
            });
    return option;
}

}  // namespace config