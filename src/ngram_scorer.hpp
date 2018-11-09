#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>


struct NgramScorer {
    using score_t = double;
    using text_t = std::string;
    using freq_map_t = std::unordered_map<text_t, score_t>;

    NgramScorer(const freq_map_t& map, uint32_t ngram)
        : map_(map),
          ngram_(ngram) { }

    score_t operator()(const std::string& text) {
        std::vector<text_t> tokens;
        for (size_t i = 0; i + ngram_ - 1 < text.size(); i++) {
            tokens.push_back(text.substr(i, ngram_));
        }

        freq_map_t ngram_map;
        for (const auto& s : tokens) {
            ++ngram_map[s];
        }

        score_t score = 0;
        for (auto it = ngram_map.begin(); it != ngram_map.end(); ++it) {
            score_t value = 0;
            if (map_.find(it->first) != map_.end()) {
                value = map_[it->first];
            }
            score += it->second * value;
        }
        return score;
    }

private:
    freq_map_t map_;
    uint32_t ngram_;
};
