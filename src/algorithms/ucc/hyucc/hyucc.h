#pragma once

#include <memory>

#include "hycommon/types.h"
#include "model/column_layout_relation_data.h"
#include "model/idataset_stream.h"
#include "thread_number/option.h"
#include "thread_number/type.h"
#include "ucc/ucc_algorithm.h"

namespace algos {

class HyUCC : public UCCAlgorithm {
private:
    std::unique_ptr<ColumnLayoutRelationData> relation_;
    config::ThreadNumType threads_num_ = 1;

    void FitInternal(model::IDatasetStream& data_stream) override;
    unsigned long long ExecuteInternal() override;
    void ResetUCCAlgorithmState() override {}
    void RegisterUCCs(std::vector<boost::dynamic_bitset<>>&& uccs,
                      const std::vector<hy::ClusterId>& og_mapping);
    void MakeExecuteOptsAvailable() final {
        MakeOptionsAvailable({config::ThreadNumberOpt.GetName()});
    }

public:
    HyUCC() : UCCAlgorithm({}) {
        RegisterOption(config::ThreadNumberOpt(&threads_num_));
    }
};

}  // namespace algos
