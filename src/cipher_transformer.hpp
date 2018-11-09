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

    const key_t& DecriptionKey() const;
    const key_t& EncryptionKey() const;
    const key_t& Alphabet() const;

    scorer_t MakeScorer() const;
    preprocessor_t MakePreprocessor() const;

    void Fit(const text_t& text, uint32_t seed = 0,
             uint32_t num_trials = 20, uint32_t num_swaps = 2000);
    text_t Transform(const text_t& text);
    text_t InverseTransform(const text_t& text);

private:
    using result_t = std::pair<score_t, key_t>;
    using future_result_t = std::future<result_t>;

    text_t Transform(const text_t& text, const key_t& key);

    template<class Generator>
    text_t Swap(Generator* gen, const key_t& key);

    template<class Generator>
    text_t Shuffle(Generator* gen, const key_t& key);

private:
    preprocessor_t preprocessor_;
    scorer_t scorer_;
    key_t key_;
    const key_t alphabet_;
    size_t nthread_;
};

template<class Preprocessor, class Scorer>
CipherTransformer<Preprocessor, Scorer>::CipherTransformer(Preprocessor preprocessor,
                                                           Scorer scorer,
                                                           const std::string& alphabet,
                                                           int nthread)
    : preprocessor_(preprocessor),
      scorer_(scorer),
      key_(alphabet),
      alphabet_(alphabet)
{
    size_t maxthread = std::thread::hardware_concurrency();
    nthread_ = maxthread ? nthread == -1 : static_cast<size_t>(nthread);
    nthread_ = std::min(static_cast<size_t>(nthread), maxthread);
}

template<class Preprocessor, class Scorer>
const typename CipherTransformer<Preprocessor, Scorer>::key_t&
CipherTransformer<Preprocessor, Scorer>::DecriptionKey() const {
    return key_;
}

template<class Preprocessor, class Scorer>
const typename CipherTransformer<Preprocessor, Scorer>::key_t&
CipherTransformer<Preprocessor, Scorer>::EncryptionKey() const {
    return key_;
}

template<class Preprocessor, class Scorer>
typename CipherTransformer<Preprocessor, Scorer>::text_t
CipherTransformer<Preprocessor, Scorer>::Transform(const text_t& text) {
    return Transform(text, DecriptionKey());
}

template<class Preprocessor, class Scorer>
typename CipherTransformer<Preprocessor, Scorer>::text_t
CipherTransformer<Preprocessor, Scorer>::InverseTransform(const text_t& text) {
    return Transform(text, EncryptionKey());
}

template<class Preprocessor, class Scorer>
typename CipherTransformer<Preprocessor, Scorer>::text_t
CipherTransformer<Preprocessor, Scorer>::Transform(const text_t& text, const key_t& key) {
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
void
CipherTransformer<Preprocessor, Scorer>::Fit(const text_t& text, uint32_t seed, uint32_t num_trials, uint32_t num_swaps) {
    auto preprocessed_text = preprocessor_(text);

    std::vector<future_result_t> futures;
    const size_t thread_num_trials = static_cast<size_t>(num_trials / nthread_) + 1;

    for (size_t ithread = 0; ithread < nthread_; ++ithread) {
        futures.emplace_back(
            std::async(
                std::launch::async, [=]() -> result_t {
                    std::mt19937_64 gen(seed + ithread);

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
    std::transform(futures.begin(), futures.end(),
                   std::back_inserter(results), [] (auto& future) { return future.get(); });
    auto best_iter = std::max_element(results.begin(), results.end(),
                                      [] (auto lhs, auto rhs) { return lhs.first < rhs.first; });
    key_ = best_iter->second;
}

template<class Preprocessor, class Scorer>
template<class Generator>
typename CipherTransformer<Preprocessor, Scorer>::text_t
CipherTransformer<Preprocessor, Scorer>::Swap(Generator *gen, const key_t& key) {
    std::uniform_int_distribution<size_t> uid(0, alphabet_.size() - 1);
    size_t lhs = uid(*gen);
    size_t rhs = uid(*gen);
    while (lhs == rhs) {
        rhs = uid(*gen);
    }
    key_t swapped{key};
    std::swap(swapped[lhs], swapped[rhs]);
    return swapped;
}

template<class Preprocessor, class Scorer>
template<class Generator>
typename CipherTransformer<Preprocessor, Scorer>::text_t
CipherTransformer<Preprocessor, Scorer>::Shuffle(Generator* gen, const key_t &key){
    text_t shuffled{key};
    std::shuffle(shuffled.begin(), shuffled.end(), *gen);
    return shuffled;
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
