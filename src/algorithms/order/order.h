#pragma once

#include <memory>

#include "algorithms/algorithm.h"
#include "model/column_layout_typed_relation_data.h"
#include "util/config/tabular_data/input_table_type.h"

namespace algos::order {

class Order : public Algorithm {
private:
    using TypedRelation = model::ColumnLayoutTypedRelationData;

    util::config::InputTable input_table_;
    std::unique_ptr<TypedRelation> typed_relation_;

    void RegisterOptions();
    void LoadDataInternal() override;
    void ResetState() override;
    unsigned long long ExecuteInternal() final;

public:
    Order();
};

}  // namespace algos::order
