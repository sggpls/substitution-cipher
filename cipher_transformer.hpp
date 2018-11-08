#pragma once

#include <string>
#include <thread>
#include <future>
#include <vector>
#include <algorithm>
#include <map>
#include <random>


template<class Preprocessor, class Scorer>
class CipherTransformer {
public:
    CipherTransformer(
            Preprocessor preprocessor, Scorer scorer,
            const std::string& alphabet, int nthread = -1);

    using scorer_t = Scorer;
    using score_t = typename Scorer::score_t;
    using preprocessor_t = Preprocessor;
    using text_t = std::string;
    using key_t = std::string;
    using const_key_t = const std::string;

    const_key_t& DecriptionKey() const;
    const_key_t& EncryptionKey() const;
    const_key_t& Alphabet() const;

    scorer_t MakeScorer() const;
    preprocessor_t MakePreprocessor() const;

    CipherTransformer& Fit(const text_t& text, uint32_t seed = 0, uint32_t num_trials = 20, uint32_t num_swaps = 2000);
    text_t Transform(const text_t& text);
    text_t InverseTransform(const text_t& text);

private:
    using result_t = std::pair<score_t, key_t>;
    using future_t = std::future<result_t>;

    text_t Transform(const text_t& text, const_key_t& key);

    template<class Generator>
    text_t Swap(Generator* gen, const_key_t& key);

    template<class Generator>
    text_t Shuffle(Generator* gen, const_key_t& key);

    preprocessor_t preprocessor_;
    scorer_t scorer_;
    key_t decryption_;
    key_t encryption_;
    const_key_t alphabet_;
    int nthread_;
};

template<class Preprocessor, class Scorer>
CipherTransformer<Preprocessor, Scorer>::CipherTransformer(Preprocessor preprocessor, Scorer scorer, const std::string& alphabet, int nthread)
    : preprocessor_(preprocessor),
      scorer_(scorer),
      decryption_(alphabet),
      encryption_(alphabet),
      alphabet_(alphabet) {
    int maxthread = static_cast<int>(std::thread::hardware_concurrency());
    nthread_ = maxthread ? nthread == -1 : nthread;
    nthread_ = std::min(nthread, maxthread);
}

template<class Preprocessor, class Scorer>
typename CipherTransformer<Preprocessor, Scorer>::text_t
CipherTransformer<Preprocessor, Scorer>::Transform(const CipherTransformer::text_t &text) {
    return Transform(text, decryption_);
}

template<class Preprocessor, class Scorer>
typename CipherTransformer<Preprocessor, Scorer>::text_t
CipherTransformer<Preprocessor, Scorer>::Transform(const text_t& text, const_key_t& key) {
    std::map<char, char> key_map;
    for (size_t i = 0; i < key.size(); i++) {
        key_map[alphabet_[i]] = key[i];
    }

    std::string key_text{text};
    for (char& c : key_text) {
        if (std::isalpha(c)) {
            if (std::islower(c)) {
                c = key_map[c];
            } else {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(
                    key_map[static_cast<char>(std::tolower(static_cast<unsigned char>(c)))])));
            }
        }
    }
    return key_text;
}

template<class Preprocessor, class Scorer>
CipherTransformer<Preprocessor, Scorer>&
CipherTransformer<Preprocessor, Scorer>::Fit(const text_t& text, uint32_t seed, uint32_t num_trials, uint32_t num_swaps) {
    auto preprocessed_text = preprocessor_(text);

    std::vector<future_t> futures;
    const size_t thread_num_trials = static_cast<size_t>(num_trials / nthread_);

    for (int ithread = 0; ithread < nthread_; ++ithread) {
        futures.emplace_back(
            std::async(
                std::launch::async, [=]() -> result_t {
                    std::mt19937_64 gen(seed + static_cast<size_t>(ithread));

                    key_t best_key = alphabet_;
                    score_t best_score = std::numeric_limits<score_t>::min();

                    for (size_t inner = 0; inner < thread_num_trials; ++inner) {
                        score_t best_trial_score = std::numeric_limits<score_t>::min();
                        key_t key = Shuffle(&gen, alphabet_);
                        for (size_t outer = 0; outer < num_swaps; ++outer) {
                            text_t new_key = Swap(&gen, key);
                            score_t score = scorer_(Transform(preprocessed_text, new_key));
                            if (score > best_trial_score) {
                                key = new_key;
                                best_trial_score = score;
                            }
                        }
                        if (best_trial_score > best_score) {
                            best_key = key;
                            best_score = best_trial_score;
                        }
                    }
                    return {best_score, best_key};
                }
            )
        );
    }
    std::vector<result_t> results;
    for (auto& future: futures) {
        results.push_back(future.get());
    }
    auto iter = std::max_element(results.begin(), results.end(), [] (const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        }
    );
    decryption_ = iter->second;
    encryption_ = iter->second; // TODO: Make function. It's wrong key!
    return *this;
}


template<class Preprocessor, class Scorer>
template<class Generator>
typename CipherTransformer<Preprocessor, Scorer>::text_t
CipherTransformer<Preprocessor, Scorer>::Swap(Generator *gen, const_key_t &key) {
    std::uniform_int_distribution<size_t> uid(0, alphabet_.size() - 1);
    size_t lhs = uid(*gen);
    size_t rhs = uid(*gen);
    while (lhs == rhs) {
        rhs = uid(*gen);
    }
    std::string swap_key{key};
    std::swap(swap_key[lhs], swap_key[rhs]);
    return swap_key;
}

template<class Preprocessor, class Scorer>
template<class Generator>
typename CipherTransformer<Preprocessor, Scorer>::text_t
CipherTransformer<Preprocessor, Scorer>::Shuffle(Generator* gen, const_key_t& key){
    text_t shuffled_key(key);
    std::shuffle(shuffled_key.begin(), shuffled_key.end(), *gen);
    return shuffled_key;
}

struct RowTextProcessor {
    std::string operator ()(const std::string& text) const {
        std::string row_text;
        for (char c : text) {
            if (std::isalpha(c)) {
                row_text += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
        return row_text;
    }
};
