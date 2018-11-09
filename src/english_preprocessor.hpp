#pragma once

#include <string>
#include <algorithm>


struct EnglishPreprocessor {
    using text_t = std::string;
    using char_t = typename text_t::value_type;

    text_t operator ()(const text_t& text) const {
        text_t removed(text);
        auto begin = std::remove_if(removed.begin(), removed.end(),
                                    [=] (auto c) {return !IsValidChar(c); });
        removed.erase(begin, removed.end());

        text_t preprocessed;
        std::transform(removed.begin(), removed.end(), std::back_inserter(preprocessed),
                       [](auto c) { return std::tolower(c); });
        return preprocessed;
    }
    bool IsValidChar(char_t c) const {
        return std::isalpha(c);
    }
};
