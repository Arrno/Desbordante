#pragma once

#include <cassert>
#include <utility>
#include <vector>

#include <boost/asio/thread_pool.hpp>

#include "hycommon/primitive_validations.h"
#include "hycommon/types.h"
#include "model/raw_ucc.h"
#include "position_list_index.h"
#include "structures/ucc_tree.h"
#include "thread_number/type.h"

namespace algos {

class Validator {
private:
    using UCCValidations = hy::PrimitiveValidations<model::RawUCC>;

    UCCTree* tree_;
    hy::PLIsPtr plis_;
    hy::RowsPtr compressed_records_;
    unsigned current_level_number_ = 1;
    config::ThreadNumType threads_num_ = 1;

    bool IsUnique(util::PLI const& pivot_pli, model::RawUCC const& ucc,
                  hy::IdPairs& comparison_suggestions) const;
    UCCValidations GetValidations(LhsPair const& vertex_and_ucc) const;
    UCCValidations ValidateAndExtendSeq(std::vector<LhsPair> const& current_level);
    UCCValidations ValidateAndExtendParallel(std::vector<LhsPair> const& current_level);
    UCCValidations ValidateAndExtend(std::vector<LhsPair> const& current_level);

public:
    Validator(UCCTree* tree, hy::PLIsPtr plis, hy::RowsPtr compressed_records,
              config::ThreadNumType threads_num) noexcept
        : tree_(tree),
          plis_(std::move(plis)),
          compressed_records_(std::move(compressed_records)),
          threads_num_(threads_num) {}

    hy::IdPairs ValidateAndExtendCandidates();
};

}  // namespace algos
