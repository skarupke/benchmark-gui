/* The Computer Language Benchmarks Game
   http://benchmarksgame.alioth.debian.org/

   Contributed by Branimir Maksimovic
*/

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cstdio>
#include <string>
#include <cstring>
#include <algorithm>
#include <map>
#include "container/flat_hash_map.hpp"
#include "container/bytell_hash_map.hpp"
#include "container/google_flat16_hash_map.hpp"
#include "container/flat_hash_map16.hpp"
#include "container/ptr_hash_map.hpp"
#include "container/unordered_map.hpp"
#include "google/dense_hash_map"
#include <future>
#include <unistd.h>
#include <ext/pb_ds/assoc_container.hpp>

#include "benchmark/benchmark.h"
#include <fstream>

static constexpr const unsigned char tochar[4] =
{
    'A', 'C', 'T', 'G'
};
static constexpr const unsigned char tonum[256] =
{
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 0  - 15
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 16 - 31
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 32 - 47
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 48
    255, 0,   255, 1,   255, 255, 255, 3,   255, 255, 255, 255, 255, 255, 255, 255, // 64 - 79
    255, 255, 255, 255, 2,   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 80
    255, 0,   255, 1,   255, 255, 255, 3,   255, 255, 255, 255, 255, 255, 255, 255, // 96
    255, 255, 255, 255, 2,   255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 112
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 128
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 144
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 160
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 176
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 192
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 208
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 224
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 240 - 255
};
static_assert(tonum[int('A')] == 0, "Checking tonum array");
static_assert(tonum[int('a')] == 0, "Checking tonum array");
static_assert(tonum[int('C')] == 1, "Checking tonum array");
static_assert(tonum[int('c')] == 1, "Checking tonum array");
static_assert(tonum[int('T')] == 2, "Checking tonum array");
static_assert(tonum[int('t')] == 2, "Checking tonum array");
static_assert(tonum[int('G')] == 3, "Checking tonum array");
static_assert(tonum[int('g')] == 3, "Checking tonum array");

struct KNucleotideHash
{
    KNucleotideHash()
        : KNucleotideHash(0)
    {
    }

    explicit KNucleotideHash(size_t size)
        : data(0)
        , size(size)
    {
    }
    explicit KNucleotideHash(const std::string & str)
        : KNucleotideHash(str.size())
    {
        uint64_t mask = this->mask();
        for (char c : str)
            append(c, mask);
    }

    uint64_t mask() const
    {
        uint64_t full_mask = -1;
        full_mask <<= 2 * size;
        return ~full_mask;
    }
    void append(char to_append, uint64_t mask)
    {
        data <<= 2;
        data |= tonum[int(to_append)];
        data &= mask;
    }

   bool operator<(const KNucleotideHash & in) const
   {
      return data < in.data;
   }
   bool operator==(const KNucleotideHash & in) const
   {
      return data == in.data;
   }
   operator std::string() const
   {
      std::string result(size, '\0');
      uint64_t tmp1 = data;
      for(auto it = result.rbegin(), end = result.rend(); it != end; ++it)
      {
         *it += tochar[tmp1 & 0b11];
         tmp1 >>= 2;
      }
      return result;
   }
   struct hash
   {
       uint64_t operator()(const KNucleotideHash & t) const
       {
           return t.data;
       }
       typedef ska::power_of_two_hash_policy hash_policy;
   };
   struct other_extra_bits_hash
   {
       uint64_t operator()(const KNucleotideHash & t) const
       {
           return t.data;
       }
       typedef ska::power_of_two_hash_policy_other_bits hash_policy;
   };
   struct prime_hash
   {
       uint64_t operator()(const KNucleotideHash & t) const
       {
           return t.data;
       }
       typedef ska::prime_number_hash_policy hash_policy;
   };
   struct switch_prime_hash
   {
       uint64_t operator()(const KNucleotideHash & t) const
       {
           return t.data;
       }
       typedef ska::switch_prime_number_hash_policy hash_policy;
   };
   struct libdivide_hash
   {
       uint64_t operator()(const KNucleotideHash & t) const
       {
           return t.data;
       }
       typedef ska::libdivide_prime_hash_policy hash_policy;
   };
   uint64_t data;
   size_t size;
};

template<typename T>
void set_empty(T &)
{
}
template<typename T>
void set_empty(google::dense_hash_map<KNucleotideHash, T, KNucleotideHash::hash> & map)
{
    static const KNucleotideHash empty("tttttttttttttttttttttttttttttttt");
    map.set_empty_key(empty);
}

template<typename HashMap>
HashMap calculate(const std::string& input, int size, size_t begin = 0, size_t end = -1)
{
    end = std::min(end, input.size());
    HashMap frequencies;
    set_empty(frequencies);
    KNucleotideHash running_hash(size);
    uint64_t mask = running_hash.mask();
    const char * it = input.data() + begin;
    for (int i = 1; i < size; ++i)
    {
        running_hash.append(*it++, mask);
    }
    for (const char * it_end = input.data() + end; it != it_end; ++it)
    {
        running_hash.append(*it, mask);
        ++frequencies[running_hash];
    }
    return frequencies;
}

template<typename HashMap>
HashMap threaded_calculate(const std::string& input, unsigned size)
{
    size_t num_threads = std::thread::hardware_concurrency();

    size_t chars_per_thread = input.size() / num_threads;
    std::vector<HashMap> futures(num_threads - 1);
    std::vector<std::thread> threads;
    threads.reserve(num_threads - 1);
    size_t start = 0;
    for (size_t i = 0; i < num_threads - 1; ++i)
    {
        size_t end = start + chars_per_thread;
        threads.emplace_back([&, i, start, end]
        {
            futures[i] = calculate<HashMap>(input, size, start, end);
        });
        start = end - (size - 1);
    }

    HashMap frequencies = calculate<HashMap>(input, size, start, input.size());

    for (std::thread & thread : threads)
        thread.join();
    for (const HashMap & map : futures)
    {
        for(auto & j : map)
        {
            frequencies[j.first] += j.second;
        }
    }
    return frequencies;
}

template<typename HashMap>
void write_frequencies(const std::string & input, unsigned size)
{
    unsigned sum = input.size() + 1 - size;
    auto frequencies = calculate<HashMap>(input, size);
    std::map<int64_t, std::string, std::greater<unsigned>> freq;
    for(auto & i: frequencies)
    {
        freq.emplace(i.second, i.first);
    }
    for(auto & i : freq)
        std::cout << i.second << ' ' << (sum ? double(100 * i.first) / sum : 0.0) << '\n';
    std::cout << '\n';
}

template<typename HashMap>
void write_count(const std::string & input, const std::string & string)
{
   auto frequencies = calculate<HashMap>(input, string.size());

   std::cout << frequencies[KNucleotideHash(string)] << '\t' << string << '\n';
}

class ScopedRedirect
{
public:
    ScopedRedirect(std::ostream & inOriginal, std::ostream & inRedirect)
        : mOriginal(inOriginal)
        , mRedirect(inRedirect)
    {
        mOriginal.rdbuf(mRedirect.rdbuf(mOriginal.rdbuf()));
    }

    ~ScopedRedirect()
    {
        mOriginal.rdbuf(mRedirect.rdbuf(mOriginal.rdbuf()));
    }

private:
    ScopedRedirect(const ScopedRedirect&);
    ScopedRedirect& operator=(const ScopedRedirect&);

    std::ostream & mOriginal;
    std::ostream & mRedirect;
};

static std::string get_debug_input()
{
    return  "aacacttcaccaggtatcgtgaaggctcaagattacccagagaacctttgcaatataagaatat"
            "gtatgcagcattaccctaagtaattatattctttttctgactcaaagtgacaagccctagtgta"
            "tattaaatcggtatatttgggaaattcctcaaactatcctaatcaggtagccatgaaagtgatc"
            "aaaaaagttcgtacttataccatacatgaattctggccaagtaaaaaatagattgcgcaaaatt"
            "cgtaccttaagtctctcgccaagatattaggatcctattactcatatcgtgtttttctttattg"
            "ccgccatccccggagtatctcacccatccttctcttaaaggcctaatattacctatgcaaataa"
            "acatatattgttgaaaattgagaacctgatcgtgattcttatgtgtaccatatgtatagtaatc"
            "acgcgactatatagtgctttagtatcgcccgtgggtgagtgaatattctgggctagcgtgagat"
            "agtttcttgtcctaatatttttcagatcgaatagcttctatttttgtgtttattgacatatgtc"
            "gaaactccttactcagtgaaagtcatgaccagatccacgaacaatcttcggaatcagtctcgtt"
            "ttacggcggaatcttgagtctaacttatatcccgtcgcttactttctaacaccccttatgtatt"
            "tttaaaattacgtttattcgaacgtacttggcggaagcgttattttttgaagtaagttacattg"
            "ggcagactcttgacattttcgatacgactttctttcatccatcacaggactcgttcgtattgat"
            "atcagaagctcgtgatgattagttgtcttctttaccaatactttgaggcctattctgcgaaatt"
            "tttgttgccctgcgaacttcacataccaaggaacacctcgcaacatgccttcatatccatcgtt"
            "cattgtaattcttacacaatgaatcctaagtaattacatccctgcgtaaaagatggtaggggca"
            "ctgaggatatattaccaagcatttagttatgagtaatcagcaatgtttcttgtattaagttctc"
            "taaaatagttacatcgtaatgttatctcgggttccgcgaataaacgagatagattcattatata"
            "tggccctaagcaaaaacctcctcgtattctgttggtaattagaatcacacaatacgggttgaga"
            "tattaattatttgtagtacgaagagatataaaaagatgaacaattactcaagtcaagatgtata"
            "cgggatttataataaaaatcgggtagagatctgctttgcaattcagacgtgccactaaatcgta"
            "atatgtcgcgttacatcagaaagggtaactattattaattaataaagggcttaatcactacata"
            "ttagatcttatccgatagtcttatctattcgttgtatttttaagcggttctaattcagtcatta"
            "tatcagtgctccgagttctttattattgttttaaggatgacaaaatgcctcttgttataacgct"
            "gggagaagcagactaagagtcggagcagttggtagaatgaggctgcaaaagacggtctcgacga"
            "atggacagactttactaaaccaatgaaagacagaagtagagcaaagtctgaagtggtatcagct"
            "taattatgacaacccttaatacttccctttcgccgaatactggcgtggaaaggttttaaaagtc"
            "gaagtagttagaggcatctctcgctcataaataggtagactactcgcaatccaatgtgactatg"
            "taatactgggaacatcagtccgcgatgcagcgtgtttatcaaccgtccccactcgcctggggag"
            "acatgagaccacccccgtggggattattagtccgcagtaatcgactcttgacaatccttttcga"
            "ttatgtcatagcaatttacgacagttcagcgaagtgactactcggcgaaatggtattactaaag"
            "cattcgaacccacatgaatgtgattcttggcaatttctaatccactaaagcttttccgttgaat"
            "ctggttgtagatatttatataagttcactaattaagatcacggtagtatattgatagtgatgtc"
            "tttgcaagaggttggccgaggaatttacggattctctattgatacaatttgtctggcttataac"
            "tcttaaggctgaaccaggcgtttttagacgacttgatcagctgttagaatggtttggactccct"
            "ctttcatgtcagtaacatttcagccgttattgttacgatatgcttgaacaatattgatctacca"
            "cacacccatagtatattttataggtcatgctgttacctacgagcatggtattccacttcccatt"
            "caatgagtattcaacatcactagcctcagagatgatgacccacctctaataacgtcacgttgcg"
            "gccatgtgaaacctgaacttgagtagacgatatcaagcgctttaaattgcatataacatttgag"
            "ggtaaagctaagcggatgctttatataatcaatactcaataataagatttgattgcattttaga"
            "gttatgacacgacatagttcactaacgagttactattcccagatctagactgaagtactgatcg"
            "agacgatccttacgtcgatgatcgttagttatcgacttaggtcgggtctctagcggtattggta"
            "cttaaccggacactatactaataacccatgatcaaagcataacagaatacagacgataatttcg"
            "ccaacatatatgtacagaccccaagcatgagaagctcattgaaagctatcattgaagtcccgct"
            "cacaatgtgtcttttccagacggtttaactggttcccgggagtcctggagtttcgacttacata"
            "aatggaaacaatgtattttgctaatttatctatagcgtcatttggaccaatacagaatattatg"
            "ttgcctagtaatccactataacccgcaagtgctgatagaaaatttttagacgatttataaatgc"
            "cccaagtatccctcccgtgaatcctccgttatactaattagtattcgttcatacgtataccgcg"
            "catatatgaacatttggcgataaggcgcgtgaattgttacgtgacagagatagcagtttcttgt"
            "gatatggttaacagacgtacatgaagggaaactttatatctatagtgatgcttccgtagaaata"
            "ccgccactggtctgccaatgatgaagtatgtagctttaggtttgtactatgaggctttcgtttg"
            "tttgcagagtataacagttgcgagtgaaaaaccgacgaatttatactaatacgctttcactatt"
            "ggctacaaaatagggaagagtttcaatcatgagagggagtatatggatgctttgtagctaaagg"
            "tagaacgtatgtatatgctgccgttcattcttgaaagatacataagcgataagttacgacaatt"
            "ataagcaacatccctaccttcgtaacgatttcactgttactgcgcttgaaatacactatggggc"
            "tattggcggagagaagcagatcgcgccgagcatatacgagacctataatgttgatgatagagaa"
            "ggcgtctgaattgatacatcgaagtacactttctttcgtagtatctctcgtcctctttctatct"
            "ccggacacaagaattaagttatatatatagagtcttaccaatcatgttgaatcctgattctcag"
            "agttctttggcgggccttgtgatgactgagaaacaatgcaatattgctccaaatttcctaagca"
            "aattctcggttatgttatgttatcagcaaagcgttacgttatgttatttaaatctggaatgacg"
            "gagcgaagttcttatgtcggtgtgggaataattcttttgaagacagcactccttaaataatatc"
            "gctccgtgtttgtatttatcgaatgggtctgtaaccttgcacaagcaaatcggtggtgtatata"
            "tcggataacaattaatacgatgttcatagtgacagtatactgatcgagtcctctaaagtcaatt"
            "acctcacttaacaatctcattgatgttgtgtcattcccggtatcgcccgtagtatgtgctctga"
            "ttgaccgagtgtgaaccaaggaacatctactaatgcctttgttaggtaagatctctctgaattc"
            "cttcgtgccaacttaaaacattatcaaaatttcttctacttggattaactacttttacgagcat"
            "ggcaaattcccctgtggaagacggttcattattatcggaaaccttatagaaattgcgtgttgac"
            "tgaaattagatttttattgtaagagttgcatctttgcgattcctctggtctagcttccaatgaa"
            "cagtcctcccttctattcgacatcgggtccttcgtacatgtctttgcgatgtaataattaggtt"
            "cggagtgtggccttaatgggtgcaactaggaatacaacgcaaatttgctgacatgatagcaaat"
            "cggtatgccggcaccaaaacgtgctccttgcttagcttgtgaatgagactcagtagttaaataa"
            "atccatatctgcaatcgattccacaggtattgtccactatctttgaactactctaagagataca"
            "agcttagctgagaccgaggtgtatatgactacgctgatatctgtaaggtaccaatgcaggcaaa"
            "gtatgcgagaagctaataccggctgtttccagctttataagattaaaatttggctgtcctggcg"
            "gcctcagaattgttctatcgtaatcagttggttcattaattagctaagtacgaggtacaactta"
            "tctgtcccagaacagctccacaagtttttttacagccgaaacccctgtgtgaatcttaatatcc"
            "aagcgcgttatctgattagagtttacaactcagtattttatcagtacgttttgtttccaacatt"
            "acccggtatgacaaaatgacgccacgtgtcgaataatggtctgaccaatgtaggaagtgaaaag"
            "ataaatat";
}
#if 1
static const std::string & get_input()
{
    static std::string result = []
    {
        std::string input;
        char buffer[256];
        FILE * file = fopen("/home/malte/workspace/knucleotide/input25000000.txt", "r");
        while (fgets(buffer, 100, file) && memcmp(">THREE", buffer, 6) != 0);
        while (fgets(buffer, 100, file) && buffer[0] != '>')
        {
           if (buffer[0] != ';')
           {
              input.append(buffer,strlen(buffer)-1);
           }
        }
        fclose(file);
        return input;
    }();
    return result;
}
#else
static std::string get_input()
{
    return get_debug_input();
}
#endif

template<typename HashMap>
void knucleotide(benchmark::State & state)
{
    const std::string & input = get_input();

    std::ofstream dev_null("/dev/null");
    ScopedRedirect redirect_cout(std::cout, dev_null);
    std::cout << std::setprecision(3) << std::setiosflags(std::ios::fixed);
    while (state.KeepRunning())
    {
        write_frequencies<HashMap>(input, 1);
        write_frequencies<HashMap>(input, 2);
        write_count<HashMap>(input, "GGT");
        write_count<HashMap>(input, "GGTA");
        write_count<HashMap>(input, "GGTATT");
        write_count<HashMap>(input, "GGTATTTTAATT");
        write_count<HashMap>(input, "GGTATTTTAATTTATAGT");
    }
}

BENCHMARK_TEMPLATE(knucleotide, ska::flat_hash_map<KNucleotideHash, int64_t, KNucleotideHash::hash>);
BENCHMARK_TEMPLATE(knucleotide, ska::bytell_hash_map<KNucleotideHash, int64_t, KNucleotideHash::libdivide_hash>);
BENCHMARK_TEMPLATE(knucleotide, ska::bytell_hash_map<KNucleotideHash, int64_t, KNucleotideHash::hash>);
BENCHMARK_TEMPLATE(knucleotide, ska::bytell_hash_map<KNucleotideHash, int64_t, KNucleotideHash::prime_hash>);
BENCHMARK_TEMPLATE(knucleotide, ska::bytell_hash_map<KNucleotideHash, int64_t, KNucleotideHash::switch_prime_hash>);
BENCHMARK_TEMPLATE(knucleotide, google::dense_hash_map<KNucleotideHash, int64_t, KNucleotideHash::hash>);
BENCHMARK_TEMPLATE(knucleotide, ska::flat16_hash_map<KNucleotideHash, int64_t, KNucleotideHash::hash>);
BENCHMARK_TEMPLATE(knucleotide, ska::ptr_hash_map<KNucleotideHash, int64_t, KNucleotideHash::hash>);
BENCHMARK_TEMPLATE(knucleotide, ska::unordered_map<KNucleotideHash, int64_t, KNucleotideHash::hash>);
BENCHMARK_TEMPLATE(knucleotide, ska::google_flat16_hash_map<KNucleotideHash, int64_t, KNucleotideHash::hash>);
BENCHMARK_TEMPLATE(knucleotide, ska::google_flat16_hash_map<KNucleotideHash, int64_t, KNucleotideHash::other_extra_bits_hash>);

#ifndef DISABLE_GTEST
#include <test/include_test.hpp>

TEST(knucleotide, to_string)
{
    for (std::string str : { "", "AAA", "ACT", "GATTACA" })
    {
        KNucleotideHash hash(str);
        std::string as_string = hash;
        ASSERT_EQ(str, as_string);
    }
}
TEST(knucleotide, calculate)
{
    std::string input = get_debug_input();
    using hash_map_type = ska::bytell_hash_map<KNucleotideHash, int64_t, KNucleotideHash::hash>;
    hash_map_type size_1 = calculate<hash_map_type>(input, 1);
    hash_map_type size_2 = calculate<hash_map_type>(input, 2);
    hash_map_type size_3 = calculate<hash_map_type>(input, 3);
    ASSERT_EQ(1480, size_1[KNucleotideHash("A")]);
    ASSERT_EQ(974, size_1[KNucleotideHash("C")]);
    ASSERT_EQ(1576, size_1[KNucleotideHash("T")]);
    ASSERT_EQ(970, size_1[KNucleotideHash("G")]);

    ASSERT_EQ(420, size_2[KNucleotideHash("AA")]);
    ASSERT_EQ(272, size_2[KNucleotideHash("AC")]);
    ASSERT_EQ(496, size_2[KNucleotideHash("AT")]);
    ASSERT_EQ(292, size_2[KNucleotideHash("AG")]);
    ASSERT_EQ(273, size_2[KNucleotideHash("CA")]);
    ASSERT_EQ(202, size_2[KNucleotideHash("CC")]);
    ASSERT_EQ(298, size_2[KNucleotideHash("CT")]);
    ASSERT_EQ(201, size_2[KNucleotideHash("CG")]);
    ASSERT_EQ(470, size_2[KNucleotideHash("TA")]);
    ASSERT_EQ(315, size_2[KNucleotideHash("TC")]);
    ASSERT_EQ(480, size_2[KNucleotideHash("TT")]);
    ASSERT_EQ(310, size_2[KNucleotideHash("TG")]);
    ASSERT_EQ(316, size_2[KNucleotideHash("GA")]);
    ASSERT_EQ(185, size_2[KNucleotideHash("GC")]);
    ASSERT_EQ(302, size_2[KNucleotideHash("GT")]);
    ASSERT_EQ(167, size_2[KNucleotideHash("GG")]);

    ASSERT_EQ(107, size_3[KNucleotideHash("AAA")]);
    ASSERT_EQ(145, size_3[KNucleotideHash("ATT")]);
    ASSERT_EQ(57,  size_3[KNucleotideHash("CCA")]);
    ASSERT_EQ(62,  size_3[KNucleotideHash("CGT")]);
    ASSERT_EQ(146, size_3[KNucleotideHash("TTA")]);
    ASSERT_EQ(87,  size_3[KNucleotideHash("GAT")]);
    ASSERT_EQ(54,  size_3[KNucleotideHash("GGT")]);
}
TEST(knucleotide, threaded_calculate)
{
    std::string input = get_debug_input();
    using hash_map_type = ska::bytell_hash_map<KNucleotideHash, int64_t, KNucleotideHash::hash>;
    hash_map_type size_1 = threaded_calculate<hash_map_type>(input, 1);
    hash_map_type size_2 = threaded_calculate<hash_map_type>(input, 2);
    hash_map_type size_3 = threaded_calculate<hash_map_type>(input, 3);
    ASSERT_EQ(1480, size_1[KNucleotideHash("A")]);
    ASSERT_EQ(974, size_1[KNucleotideHash("C")]);
    ASSERT_EQ(1576, size_1[KNucleotideHash("T")]);
    ASSERT_EQ(970, size_1[KNucleotideHash("G")]);

    ASSERT_EQ(420, size_2[KNucleotideHash("AA")]);
    ASSERT_EQ(272, size_2[KNucleotideHash("AC")]);
    ASSERT_EQ(496, size_2[KNucleotideHash("AT")]);
    ASSERT_EQ(292, size_2[KNucleotideHash("AG")]);
    ASSERT_EQ(273, size_2[KNucleotideHash("CA")]);
    ASSERT_EQ(202, size_2[KNucleotideHash("CC")]);
    ASSERT_EQ(298, size_2[KNucleotideHash("CT")]);
    ASSERT_EQ(201, size_2[KNucleotideHash("CG")]);
    ASSERT_EQ(470, size_2[KNucleotideHash("TA")]);
    ASSERT_EQ(315, size_2[KNucleotideHash("TC")]);
    ASSERT_EQ(480, size_2[KNucleotideHash("TT")]);
    ASSERT_EQ(310, size_2[KNucleotideHash("TG")]);
    ASSERT_EQ(316, size_2[KNucleotideHash("GA")]);
    ASSERT_EQ(185, size_2[KNucleotideHash("GC")]);
    ASSERT_EQ(302, size_2[KNucleotideHash("GT")]);
    ASSERT_EQ(167, size_2[KNucleotideHash("GG")]);

    ASSERT_EQ(107, size_3[KNucleotideHash("AAA")]);
    ASSERT_EQ(145, size_3[KNucleotideHash("ATT")]);
    ASSERT_EQ(57,  size_3[KNucleotideHash("CCA")]);
    ASSERT_EQ(62,  size_3[KNucleotideHash("CGT")]);
    ASSERT_EQ(146, size_3[KNucleotideHash("TTA")]);
    ASSERT_EQ(87,  size_3[KNucleotideHash("GAT")]);
    ASSERT_EQ(54,  size_3[KNucleotideHash("GGT")]);
}

#endif
