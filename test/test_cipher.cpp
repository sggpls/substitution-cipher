#include <gtest/gtest.h>

#include <cipher_transformer.hpp>
#include <english_preprocessor.hpp>
#include <ngram_scorer.hpp>

static const text_t english = "abcdefghijklmnopqrstuvwxyz";
using EnglishDecoder = CipherTransformer<EnglishPreprocessor, NgramScorer>;


freq_map_t ReadNgrams(const text_t& path_to_file) {
    freq_map_t map;

    std::ifstream file;
    file.open(path_to_file);

    EnglishPreprocessor ep;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::vector<text_t> tokens{std::istream_iterator<text_t>{iss},
                              std::istream_iterator<text_t>{}};
        map[ep(tokens[0])] = std::stod(tokens[1]);
    }
    return map;
}

text_t ReadText(const text_t& filename) {
    char current;
    text_t text;
    std::ifstream fin(filename);
    while (fin.get(current)) {
        text += current;
    }
    return text;
}

TEST(EnglishPreprocessor, SimpleTest) {
    EnglishPreprocessor ep;

    EXPECT_EQ(ep("Hello, World!"), "helloworld");
    EXPECT_EQ(ep("Winter is CoMinG!"), "winteriscoming");
    EXPECT_EQ(ep("ABCD&7%4#$?,<>.\"\n~*^QWerTy"), "abcdqwerty");
    EXPECT_EQ(ep("ЙЦУКЕНQWERTY"), "qwerty");
}

TEST(EnglishDecoder, JohnBrzenk) {
    EnglishPreprocessor preprocessor;

    const auto freq_map = ReadNgrams("../text/ngram/english_quadgrams.txt");
    NgramScorer quad_scorer(freq_map, 4);

    EnglishDecoder decoder(preprocessor, quad_scorer, english, -1);
    const text_t plain_text = ReadText("../text/plain_john.txt");
    const text_t cipher_text = ReadText("../text/cipher_john.txt");

    decoder.Fit(cipher_text, 890, 20, 2000);
    EXPECT_EQ(decoder.Transform(cipher_text), plain_text);
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
