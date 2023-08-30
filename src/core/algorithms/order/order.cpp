#include "order.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <utility>

#include "config/names_and_descriptions.h"
#include "config/tabular_data/input_table/option.h"
#include "model/types/types.h"

namespace algos::order {

Order::Order() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::TableOpt.GetName()});
}

void Order::RegisterOptions() {
    using namespace config::names;
    using namespace config::descriptions;
    using config::Option;

    RegisterOption(config::TableOpt(&input_table_));
}

void Order::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, false);
}

void Order::ResetState() {}

void Order::CreateSortedPartitions() {
    std::vector<model::TypedColumnData> const& data = typed_relation_->GetColumnData();
    std::unordered_set<unsigned long> null_rows;
    for (unsigned int i = 0; i < data.size(); ++i) {
        if (!model::Type::IsOrdered(data[i].GetTypeId())) {
            continue;
        }
        for (size_t k = 0; k < data[i].GetNumRows(); ++k) {
            if (data[i].IsNullOrEmpty(k)) {
                null_rows.insert(k);
            }
        }
    }
    for (unsigned int i = 0; i < data.size(); ++i) {
        if (!model::Type::IsOrdered(data[i].GetTypeId())) {
            continue;
        }
        single_attributes_.push_back({i});
        std::vector<std::pair<unsigned long, std::byte const*>> indexed_byte_data;
        indexed_byte_data.reserve(data[i].GetNumRows());
        std::vector<std::byte const*> const& byte_data = data[i].GetData();
        for (size_t k = 0; k < byte_data.size(); ++k) {
            if (null_rows.find(k) != null_rows.end()) {
                continue;
            }
            indexed_byte_data.emplace_back(k, byte_data[k]);
        }
        std::unique_ptr<model::Type> type = model::CreateType(data[i].GetTypeId(), true);
        std::unique_ptr<model::MixedType> mixed_type =
                model::CreateSpecificType<model::MixedType>(model::TypeId::kMixed, true);
        auto less = [&type, &mixed_type](std::pair<unsigned long, std::byte const*> l,
                                         std::pair<unsigned long, std::byte const*> r) {
            if (type->GetTypeId() == +(model::TypeId::kMixed)) {
                return mixed_type->CompareAsStrings(l.second, r.second) ==
                       model::CompareResult::kLess;
            }
            return type->Compare(l.second, r.second) == model::CompareResult::kLess;
        };
        auto equal = [&type, &mixed_type](std::pair<unsigned long, std::byte const*> l,
                                          std::pair<unsigned long, std::byte const*> r) {
            if (type->GetTypeId() == +(model::TypeId::kMixed)) {
                return mixed_type->CompareAsStrings(l.second, r.second) ==
                       model::CompareResult::kEqual;
            }
            return type->Compare(l.second, r.second) == model::CompareResult::kEqual;
        };
        std::sort(indexed_byte_data.begin(), indexed_byte_data.end(), less);
        std::vector<std::unordered_set<unsigned long>> equivalence_classes;
        equivalence_classes.push_back({indexed_byte_data.front().first});
        for (size_t k = 1; k < indexed_byte_data.size(); ++k) {
            if (equal(indexed_byte_data[k - 1], indexed_byte_data[k])) {
                equivalence_classes.back().insert(indexed_byte_data[k].first);
            } else {
                equivalence_classes.push_back({indexed_byte_data[k].first});
            }
        }
        sorted_partitions_[{i}] = SortedPartition(std::move(equivalence_classes));
    }
}

void Order::CreateSortedPartitionsFromSingletons(AttributeList const& attr_list) {
    if (sorted_partitions_.find(attr_list) != sorted_partitions_.end()) {
        return;
    }
    SortedPartition res = sorted_partitions_[{attr_list[0]}];
    for (size_t i = 1; i < attr_list.size(); ++i) {
        res = res * sorted_partitions_[{attr_list[i]}];
    }
    sorted_partitions_[attr_list] = res;
}

bool SubsetSetDifference(std::unordered_set<unsigned long>& a,
                         std::unordered_set<unsigned long>& b) {
    auto const not_found = b.end();
    for (auto const& element : a)
        if (b.find(element) == not_found) {
            return false;
        } else {
            b.erase(element);
        }
    return true;
}

ValidityType Order::CheckForSwap(SortedPartition const& l, SortedPartition const& r) {
    ValidityType res = ValidityType::valid;
    size_t l_i = 0, r_i = 0;
    bool next_l = true, next_r = true;
    std::unordered_set<unsigned long> l_eq_class;
    std::unordered_set<unsigned long> r_eq_class;
    while (l_i < l.sorted_partition.size() && r_i < r.sorted_partition.size()) {
        if (next_l) {
            l_eq_class = l.sorted_partition[l_i];
        }
        if (next_r) {
            r_eq_class = r.sorted_partition[r_i];
        }
        if (l_eq_class.size() < r_eq_class.size()) {
            if (!SubsetSetDifference(l_eq_class, r_eq_class)) {
                return ValidityType::swap;
            } else {
                res = ValidityType::merge;
                ++l_i;
                next_l = true;
                next_r = false;
            }
        } else {
            if (!SubsetSetDifference(r_eq_class, l_eq_class)) {
                return ValidityType::swap;
            } else {
                ++r_i;
                next_r = true;
                if (l_eq_class.empty()) {
                    ++l_i;
                    next_l = true;
                } else {
                    next_l = false;
                }
            }
        }
    }
    return res;
}

std::vector<Order::AttributeList> GetPrefixes(Order::Node const& node) {
    std::vector<Order::AttributeList> res;
    res.reserve(node.size() - 1);
    for (size_t i = 1; i < node.size(); ++i) {
        res.emplace_back(node.begin(), node.begin() + i);
    }
    return res;
}

std::vector<unsigned int> MaxPrefix(std::vector<unsigned int> const& attribute_list) {
    return std::vector<unsigned int>(attribute_list.begin(), attribute_list.end() - 1);
}

using CandidatePairs = std::vector<std::pair<Order::AttributeList, Order::AttributeList>>;

CandidatePairs ObtainCandidates(Order::Node const& node) {
    CandidatePairs res;
    res.reserve(node.size() - 1);
    for (size_t i = 1; i < node.size(); ++i) {
        Order::AttributeList lhs(node.begin(), node.begin() + i);
        Order::AttributeList rhs(node.begin() + i, node.end());
        res.emplace_back(lhs, rhs);
    }
    return res;
}

bool InUnorderedMap(Order::OrderDependencies const& map, Order::AttributeList const& lhs,
                    Order::AttributeList const& rhs) {
    if (map.find(lhs) == map.end()) {
        return false;
    }
    if (map.at(lhs).find(rhs) == map.at(lhs).end()) {
        return false;
    }
    return true;
}

void PrintOD(Order::AttributeList const& lhs, Order::AttributeList const& rhs) {
    for (auto const& attr : lhs) {
        std::cout << attr << " ";
    }
    std::cout << "-> ";
    for (auto const& attr : rhs) {
        std::cout << attr << " ";
    }
}

void Order::ComputeDependencies(LatticeLevel const& lattice_level) {
    if (level_ < 2) {
        return;
    }
    UpdateCandidateSets();
    PrintValidOD();
    for (Node const& node : lattice_level) {
        CandidatePairs candidate_pairs = ObtainCandidates(node);
        for (auto const& [lhs, rhs] : candidate_pairs) {
            if (!InUnorderedMap(candidate_sets_, lhs, rhs)) {
                continue;
            }
            bool prefix_valid = false;
            for (AttributeList const& rhs_prefix : GetPrefixes(rhs)) {
                if (InUnorderedMap(valid_, lhs, rhs_prefix)) {
                    prefix_valid = true;
                    break;
                }
            }
            if (prefix_valid) {
                continue;
            }
            CreateSortedPartitionsFromSingletons(lhs);
            CreateSortedPartitionsFromSingletons(rhs);
            ValidityType candidate_validity =
                    CheckForSwap(sorted_partitions_[lhs], sorted_partitions_[rhs]);
            if (candidate_validity == +ValidityType::valid) {
                bool non_minimal_by_merge = false;
                for (AttributeList const& merge_lhs : GetPrefixes(lhs)) {
                    if (InUnorderedMap(merge_invalidated_, merge_lhs, rhs)) {
                        non_minimal_by_merge = true;
                        break;
                    }
                }
                if (non_minimal_by_merge) {
                    continue;
                }
                if (valid_.find(lhs) == valid_.end()) {
                    valid_[lhs] = {};
                }
                valid_[lhs].insert(rhs);
                bool lhs_unique = typed_relation_->GetNumRows() ==
                                  sorted_partitions_[lhs].sorted_partition.size();
                if (lhs_unique) {
                    candidate_sets_[lhs].erase(rhs);
                }
            } else if (candidate_validity == +ValidityType::swap) {
                candidate_sets_[lhs].erase(rhs);
                std::cout << "SWAP: ";
                PrintOD(lhs, rhs);
                std::cout << '\n';
            } else if (candidate_validity == +ValidityType::merge) {
                if (merge_invalidated_.find(lhs) == merge_invalidated_.end()) {
                    merge_invalidated_[lhs] = {};
                }
                merge_invalidated_[lhs].insert(rhs);
                std::cout << "MERGE: ";
                PrintOD(lhs, rhs);
                std::cout << '\n';
            }
        }
    }
    MergePrune();
}

bool AreDisjoint(Order::AttributeList const& a, Order::AttributeList const& b) {
    for (auto const& a_atr : a) {
        for (auto const& b_atr : b) {
            if (a_atr == b_atr) {
                return false;
            }
        }
    }
    return true;
}

std::vector<Order::AttributeList> Order::Extend(AttributeList const& lhs,
                                                AttributeList const& rhs) {
    std::vector<AttributeList> extended_rhss;
    for (auto const& single_attribute : single_attributes_) {
        if (AreDisjoint(single_attribute, lhs) && AreDisjoint(single_attribute, rhs)) {
            AttributeList extended(rhs);
            extended.push_back(single_attribute[0]);
            extended_rhss.push_back(extended);
        }
    }
    return extended_rhss;
}

bool Order::IsMinimal(AttributeList const& a) {
    for (auto const& [lhs, rhs_list] : valid_) {
        for (AttributeList const& rhs : rhs_list) {
            auto it_rhs = std::search(a.begin(), a.end(), rhs.begin(), rhs.end());
            if (it_rhs == a.end()) {
                continue;
            }
            if (std::search(it_rhs + rhs.size(), a.end(), lhs.begin(), lhs.end()) != a.end()) {
                return false;
            }
            auto it_lhs = std::search(a.begin(), it_rhs, lhs.begin(), lhs.end());
            if (it_lhs + lhs.size() == it_rhs) {
                return false;
            }
        }
    }
    return true;
}

void Order::UpdateCandidateSets() {
    if (level_ < 3) {
        return;
    }
    CandidateSets next_candidates;
    for (auto const& [lhs, rhs_list] : candidate_sets_) {
        next_candidates[lhs] = {};
        if (lhs.size() != level_ - 1) {
            for (AttributeList const& rhs : rhs_list) {
                if (InUnorderedMap(valid_, lhs, rhs)) {
                    continue;
                }
                std::vector<AttributeList> extended_rhss = Extend(lhs, rhs);
                for (AttributeList const& extended : extended_rhss) {
                    if (lhs.size() > 1) {
                        AttributeList lhs_max_prefix = MaxPrefix(lhs);
                        std::vector<AttributeList> extended_prefixes = GetPrefixes(extended);
                        auto prefix_is_valid = [&](AttributeList const& extended_prefix) {
                            return InUnorderedMap(valid_, lhs_max_prefix, extended_prefix);
                        };
                        if ((std::find_if(extended_prefixes.begin(), extended_prefixes.end(),
                                          prefix_is_valid) == extended_prefixes.end()) &&
                            !InUnorderedMap(candidate_sets_, lhs_max_prefix, extended)) {
                            continue;
                        }
                    }
                    if (!IsMinimal(extended)) {
                        continue;
                    }
                    next_candidates[lhs].insert(extended);
                }
            }
        } else if (IsMinimal(lhs)) {
            AttributeList lhs_max_prefix = MaxPrefix(lhs);
            for (AttributeList const& rhs : candidate_sets_[lhs_max_prefix]) {
                if (AreDisjoint(lhs, rhs)) {
                    next_candidates[lhs].insert(rhs);
                }
            }
        }
        if (next_candidates[lhs].empty()) {
            next_candidates.erase(lhs);
        }
    }
    previous_candidate_sets_ = std::move(candidate_sets_);
    candidate_sets_ = std::move(next_candidates);
}

bool StartsWith(Order::AttributeList const& rhs_candidate, Order::AttributeList const& rhs) {
    for (size_t i = 0; i < rhs.size(); ++i) {
        if (rhs[i] != rhs_candidate[i]) {
            return false;
        }
    }
    return true;
}

void Order::MergePrune() {
    if (level_ < 3) {
        return;
    }
    for (auto const& [lhs, rhs_list] : candidate_sets_) {
        if (lhs.size() <= 1) {
            continue;
        }
        for (auto rhs_it = rhs_list.begin(); rhs_it != rhs_list.end();) {
            AttributeList lhs_max_prefix = MaxPrefix(lhs);
            if (InUnorderedMap(merge_invalidated_, lhs_max_prefix, *rhs_it)) {
                bool prunable = true;
                for (auto const& other_rhs : candidate_sets_[lhs_max_prefix]) {
                    if (MaxPrefix(other_rhs) == *rhs_it) {
                        prunable = false;
                        break;
                    }
                }
                if (prunable) {
                    rhs_it = candidate_sets_[lhs].erase(rhs_it);
                } else {
                    ++rhs_it;
                }
            } else {
                ++rhs_it;
            }
        }
    }
}

void Order::Prune(LatticeLevel& lattice_level) {
    if (level_ < 2) {
        return;
    }
    for (auto node_it = lattice_level.begin(); node_it != lattice_level.end();) {
        bool all_candidates_empty = false;
        std::vector<AttributeList> prefixes = GetPrefixes(*node_it);
        for (AttributeList const& lhs : prefixes) {
            if (!candidate_sets_[lhs].empty()) {
                all_candidates_empty = false;
                break;
            } else {
                all_candidates_empty = true;
            }
        }
        if (all_candidates_empty) {
            node_it = lattice_level.erase(node_it);
        } else {
            ++node_it;
        }
    }
    /* TODO: Make iteration from metanome */
    for (auto candidate_it = candidate_sets_.begin(); candidate_it != candidate_sets_.end();) {
        if (candidate_it->second.empty()) {
            candidate_it = candidate_sets_.erase(candidate_it);
        } else {
            ++candidate_it;
        }
    }
}

using PrefixBlocks = std::unordered_map<Order::AttributeList, std::vector<Order::Node>,
                                        boost::hash<std::vector<unsigned int>>>;

PrefixBlocks GetPrefixBlocks(Order::LatticeLevel const& l) {
    PrefixBlocks res;
    for (Order::Node const& node : l) {
        std::vector<unsigned int> node_prefix = MaxPrefix(node);
        if (res.find(node_prefix) == res.end()) {
            res[node_prefix] = {};
        }
        res[node_prefix].push_back(node);
    }
    return res;
}

Order::Node JoinNodes(Order::Node const& l, Order::Node const& r) {
    Order::Node res(l);
    res.push_back(r.back());
    return res;
}

Order::LatticeLevel Order::GenerateNextLevel(LatticeLevel const& l) {
    LatticeLevel next;
    PrefixBlocks prefix_blocks = GetPrefixBlocks(l);
    for (auto const& [prefix, prefix_block] : prefix_blocks) {
        for (Node const& node : prefix_block) {
            for (Node const& join_node : prefix_block) {
                if (node == join_node) {
                    continue;
                }
                Node joined = JoinNodes(node, join_node);
                next.insert(joined);
            }
        }
    }
    if (level_ > 1 && !candidate_sets_.empty()) {
        for (Node const& node : l) {
            candidate_sets_[node] = {};
        }
    }
    return next;
}

void Order::PrintValidOD() {
    std::cout << "***VALID ORDER DEPENDENCIES***" << '\n';
    unsigned int cnt = 0;
    for (auto const& [lhs, rhs_list] : valid_) {
        for (AttributeList const& rhs : rhs_list) {
            ++cnt;
            for (auto const& attr : lhs) {
                std::cout << attr + 1 << ",";
            }
            std::cout << "->";
            for (auto const& attr : rhs) {
                std::cout << attr + 1 << ",";
            }
            std::cout << '\n';
        }
    }
    std::cout << "OD amount: " << cnt;
    std::cout << '\n' << '\n';
}

unsigned long long Order::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();
    CreateSortedPartitions();
    LatticeLevel lattice_level = {};
    for (AttributeList const& single_attribute : single_attributes_) {
        lattice_level.insert(single_attribute);
        candidate_sets_[single_attribute] = {};
        for (AttributeList const& rhs_single_attribute : single_attributes_) {
            if (single_attribute != rhs_single_attribute) {
                candidate_sets_[single_attribute].insert(rhs_single_attribute);
            }
        }
    }
    level_ = 1;
    while (!lattice_level.empty()) {
        ComputeDependencies(lattice_level);
        Prune(lattice_level);
        lattice_level = GenerateNextLevel(lattice_level);
        ++level_;
    }
    PrintValidOD();
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    std::cout << "ms: " << elapsed_milliseconds.count() << '\n';
    return elapsed_milliseconds.count();
}

}  // namespace algos::order
