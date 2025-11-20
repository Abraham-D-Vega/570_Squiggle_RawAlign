// chain_seeds.hpp
// This file takes seed hits (anchors) for a read and produces chains using the same chaining logic as rmap.cpp
// To be used by simulate_seeder.cpp

#ifndef CHAINSEEDS_HPP
#define CHAINSEEDS_HPP

#include "utils.hpp"
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>

// Helper: compare for sorting chains
bool compare_chain_score(const ri_chain_t &a, const ri_chain_t &b) {
    return a.score > b.score;
}

// Helper: compare for sorting chaining scores
bool compare_score_idx(const std::pair<float, size_t> &left, const std::pair<float, size_t> &right) {
    if (left.first > right.first) return true;
    else if (left.first == right.first) return (left.second > right.second);
    else return false;
}

// Compute MAPQ for chains
void comp_mapq(std::vector<ri_chain_t> &chains) {
    if (chains.size() == 1) {
        chains[0].mapq = 60;
        return;
    } else {
        int mapq = 40 * (1 - chains[1].score / chains[0].score);
        if (mapq > 60) mapq = 60;
        if (mapq < 0) mapq = 0;
        chains[0].mapq = (uint8_t)mapq;
    }
}

// Generate primary chains (non-overlapping, high scoring)
void gen_primary_chains(std::vector<ri_chain_t> &chains) {
    // Output all chains, do not merge or filter
}

// Traceback to build a chain from DP
void traceback_chains(
    int min_num_anchors, int strand, size_t chain_end_anchor_index,
    uint32_t chain_target_signal_index,
    const std::vector<float> &chaining_scores,
    const std::vector<size_t> &chaining_predecessors,
    const std::vector<std::vector<ri_anchor_t> > &anchors_fr,
    std::vector<bool> &anchor_is_used, std::vector<ri_chain_t> &chains) {

    if (!anchor_is_used[chain_end_anchor_index]) {
        std::vector<ri_anchor_t> anchors;
        anchors.reserve(100);
        bool stop_at_an_used_anchor = false;
        size_t chain_start_anchor_index = chain_end_anchor_index;
        anchors.push_back(anchors_fr[chain_target_signal_index][chain_start_anchor_index]);
        if (chaining_predecessors[chain_start_anchor_index] != chain_start_anchor_index && anchor_is_used[chaining_predecessors[chain_start_anchor_index]]) {
            stop_at_an_used_anchor = true;
        }
        anchor_is_used[chain_start_anchor_index] = true;
        uint32_t chain_num_anchors = 1;
        while (chaining_predecessors[chain_start_anchor_index] != chain_start_anchor_index && !anchor_is_used[chaining_predecessors[chain_start_anchor_index]]) {
            chain_start_anchor_index = chaining_predecessors[chain_start_anchor_index];
            anchors.push_back(anchors_fr[chain_target_signal_index][chain_start_anchor_index]);
            if (chaining_predecessors[chain_start_anchor_index] != chain_start_anchor_index && anchor_is_used[chaining_predecessors[chain_start_anchor_index]]) {
                stop_at_an_used_anchor = true;
            }
            anchor_is_used[chain_start_anchor_index] = true;
            ++chain_num_anchors;
        }
        if (chain_num_anchors >= (uint32_t)min_num_anchors) {
            float adjusted_chaining_score = chaining_scores[chain_end_anchor_index];
            if (stop_at_an_used_anchor) {
                adjusted_chaining_score -= chaining_scores[chaining_predecessors[chain_start_anchor_index]];
            }
            // Use std::vector for anchors
            std::vector<ri_anchor_t> chain_anchors = anchors;
            chains.emplace_back(ri_chain_t{adjusted_chaining_score, chain_target_signal_index,
                anchors_fr[chain_target_signal_index][chain_start_anchor_index].target_position,
                anchors_fr[chain_target_signal_index][chain_end_anchor_index].target_position,
                chain_num_anchors, 0, strand, chain_anchors});
        }
    }
}

// Main chaining function: takes anchors and produces chains
void chain_seeds(
    const std::vector<std::vector<std::vector<ri_anchor_t> > > &anchors_fr, // [strand][ref][anchors]
    const ri_mapopt_t *opt,
    std::vector<ri_chain_t> &out_chains
) {
    int max_gap_length = opt->max_gap_length;
    int max_target_gap_length = opt->max_target_gap_length;
    int chaining_band_length = opt->chaining_band_length;
    int max_num_skips = opt->max_num_skips;
    int min_num_anchors = opt->min_num_anchors;
    int num_best_chains = opt->num_best_chains;
    float min_chaining_score = opt->min_chaining_score;
    size_t n_seq = anchors_fr[0].size();

    float max_chaining_score = 0;
    std::vector<ri_chain_t> chains;
    for (size_t target_signal_index = 0; target_signal_index < n_seq; ++target_signal_index) {
        for (int strand_i = 0; strand_i < 2; ++strand_i) {
            const auto &anchors = anchors_fr[strand_i][target_signal_index];
            std::vector<float> chaining_scores;
            chaining_scores.reserve(anchors.size());
            std::vector<size_t> chaining_predecessors;
            chaining_predecessors.reserve(anchors.size());
            std::vector<bool> anchor_is_used(anchors.size(), false);
            std::vector<std::pair<float, size_t> > end_anchor_index_chaining_scores;
            end_anchor_index_chaining_scores.reserve(10);
            for (size_t anchor_index = 0; anchor_index < anchors.size(); ++anchor_index) {
                float distance_coefficient = 1;
                chaining_scores.emplace_back(distance_coefficient);
                chaining_predecessors.emplace_back(anchor_index);
                int32_t current_anchor_target_position = anchors[anchor_index].target_position;
                int32_t current_anchor_query_position = anchors[anchor_index].query_position;
                int32_t start_anchor_index = 0;
                if (anchor_index > (size_t)chaining_band_length) start_anchor_index = anchor_index - chaining_band_length;

                int32_t previous_anchor_index = anchor_index - 1;
                int32_t num_skips = 0;
                for (; previous_anchor_index >= start_anchor_index; --previous_anchor_index) {
                    int32_t previous_anchor_target_position = anchors[previous_anchor_index].target_position;
                    int32_t previous_anchor_query_position = anchors[previous_anchor_index].query_position;

                    if (previous_anchor_query_position == current_anchor_query_position) continue;
                    if (previous_anchor_target_position == current_anchor_target_position) continue;
                    if (previous_anchor_target_position + max_target_gap_length < current_anchor_target_position) break;

                    int32_t target_position_diff = current_anchor_target_position - previous_anchor_target_position;
                    int32_t query_position_diff = current_anchor_query_position - previous_anchor_query_position;
                    float current_chaining_score = 0;

                    if (query_position_diff < 0) continue;
                    float matching_dimensions = std::min(std::min(target_position_diff, query_position_diff), 1) * distance_coefficient;
                    int gap_length = std::abs(target_position_diff - query_position_diff);
                    float gap_scale = target_position_diff > 0 ? (float)query_position_diff / target_position_diff : 1;
                    if (gap_length < max_gap_length && gap_scale < 1000000 && gap_scale > 0.0) {
                        current_chaining_score = chaining_scores[previous_anchor_index] + matching_dimensions;
                    }
                    if (current_chaining_score > chaining_scores[anchor_index]) {
                        chaining_scores[anchor_index] = current_chaining_score;
                        chaining_predecessors[anchor_index] = previous_anchor_index;
                        --num_skips;
                    } else {
                        ++num_skips;
                        if (num_skips > max_num_skips) break;
                    }
                }
                if (chaining_scores[anchor_index] > max_chaining_score) {
                    max_chaining_score = chaining_scores[anchor_index];
                }
                if (chaining_scores.back() >= min_chaining_score && chaining_scores.back() > max_chaining_score / 2) {
                    end_anchor_index_chaining_scores.emplace_back(chaining_scores.back(), anchor_index);
                }
            }
            std::sort(end_anchor_index_chaining_scores.begin(), end_anchor_index_chaining_scores.end(), compare_score_idx);
            for (size_t anchor_index = 0; anchor_index < end_anchor_index_chaining_scores.size() && anchor_index < (size_t)num_best_chains; ++anchor_index) {
                traceback_chains(min_num_anchors, strand_i, end_anchor_index_chaining_scores[anchor_index].second, target_signal_index, chaining_scores,
                    chaining_predecessors, anchors_fr[strand_i], anchor_is_used, chains);
                if (chaining_scores[end_anchor_index_chaining_scores[anchor_index].second] < max_chaining_score / 2) break;
            }
        }
    }
    if (chains.size() > 0) {
        gen_primary_chains(chains);
        comp_mapq(chains);
    }
    out_chains = chains;
}

#endif // CHAINSEEDS_HPP