#ifndef SHOGIPP_DEFINED
#define SHOGIPP_DEFINED

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <utility>
#include <algorithm>
#include <memory>
#include <sstream>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <limits>
#include <stack>
#include <optional>
#include <array>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <regex>
#include <functional>
#include <mutex>
#include <future>
#include <thread>

/**
 * @breif ハッシュ値のバイト数を定義する。
 * @details SIZE_OF_HASH % sizeof(std::size_t) == 0 でなければならない。
 *          このマクロが定義されていない場合、ハッシュ値として std::size_t を使用する。
 */
#define SIZE_OF_HASH 16

//#define NONDETERMINISM
#ifdef NONDETERMINISM
#define SHOGIPP_SEED std::random_device{}()
#else
#define SHOGIPP_SEED
#endif

#ifdef NDEBUG
#define SHOGIPP_ASSERT(expr)
#else
#define SHOGIPP_ASSERT(expr)                                                        \
    do                                                                              \
    {                                                                               \
        shogipp::details::assert_impl((expr), #expr, __FILE__, __func__, __LINE__); \
    } while (false)                                                                 \

#endif

#ifdef NDEBUG
#define VALIDATE_KYOKUMEN_ROLLBACK(kyokumen)
#else
#define VALIDATE_KYOKUMEN_ROLLBACK(kyokumen) kyokumen_rollback_validator_t kyokumen_rollback_validator{ kyokumen }
#endif

#if __cplusplus >= 202002L
    #define SHOGIPP_STRING_LITERAL_IMPL_U8 SHOGIPP_STRING_LITERAL_IMPL(name, s, char8_t, u8)
#else
    #define SHOGIPP_STRING_LITERAL_IMPL_U8
#endif

#define SHOGIPP_STRING_LITERAL_IMPL(name, s, CharT, prefix) \
    template<>                                              \
    struct name ## _impl<CharT>                             \
    {                                                       \
        inline const CharT * operator()() const             \
        {                                                   \
            return prefix ## s;                             \
        }                                                   \
    };                                                      \

#define SHOGIPP_STRING_LITERAL(name, s)                              \
    template<typename CharT>                                         \
    struct name ## _impl;                                            \
    SHOGIPP_STRING_LITERAL_IMPL(name, s, char, )                     \
    SHOGIPP_STRING_LITERAL_IMPL_U8                                   \
    SHOGIPP_STRING_LITERAL_IMPL(name, s, char16_t, u)                \
    SHOGIPP_STRING_LITERAL_IMPL(name, s, char32_t, U)                \
    SHOGIPP_STRING_LITERAL_IMPL(name, s, wchar_t, L)                 \
    template<typename CharT>                                         \
    inline const CharT * name() { return name ## _impl<CharT>{}(); } \

namespace shogipp
{
    using search_count_t = unsigned long long;
    using milli_second_time_t = unsigned long long;

    namespace details
    {
        SHOGIPP_STRING_LITERAL(split_tokens_literal, R"(\s+)");

        /**
         * @breif 文字列を空白で区切る。
         * @tparam OutputIterator 区切られた文字列の出力イテレータ型
         * @tparam CharT 文字型
         * @param result 区切られた文字列の出力イテレータ
         * @param s 区切られる文字列
         */
        template<typename OutputIterator, typename CharT>
        inline void split_tokens(OutputIterator result, std::basic_string_view<CharT> s)
        {
            std::basic_regex<CharT> separator{ details::split_tokens_literal<CharT>() };
            using regex_token_iterator = std::regex_token_iterator<typename std::basic_string_view<CharT>::const_iterator>;
            auto iter = regex_token_iterator{ s.begin(), s.end(), separator, -1 };
            auto end = regex_token_iterator{};
            while (iter != end)
                *result++ = *iter++;
        }

        /**
        * @breif SHOGIPP_ASSERT マクロの実装
        * @param assertion 式を評価した bool 値
        * @param expr 式を表現する文字列
        * @param file __FILE__
        * @param func __func__
        * @param line __LINE__
        */
        inline constexpr void assert_impl(bool assertion, const char * expr, const char * file, const char * func, unsigned int line)
        {
            if (!assertion)
            {
                std::cerr << "Assertion failed: " << expr << ", file " << file << ", line " << line << std::endl;
                std::terminate();
            }
        }

        /**
         * @breif 総読み手数、実行時間、読み手速度を測定する機能を提供する。
         */
        class timer_t
        {
        public:
            /**
             * @breif 時間計測を開始する。
             */
            inline timer_t();

            /**
             * @breif 時間計測を再開始する。
             */
            inline void clear();

            /**
             * @breif 経過時間を標準出力に出力する。
             */
            inline void print_elapsed_time();

            /**
             * @breif 読み手数の参照を返す。
             * @return 読み手数の参照
             */
            inline search_count_t & search_count();

            /**
             * @breif 読み手数の参照を返す。
             * @return 読み手数の参照
             */
            inline const search_count_t & search_count() const;

        private:
            std::chrono::system_clock::time_point m_begin;
            search_count_t m_search_count;
        };

        inline timer_t::timer_t()
        {
            clear();
        }

        inline void timer_t::clear()
        {
            m_begin = std::chrono::system_clock::now();
            m_search_count = 0;
        }

        inline void timer_t::print_elapsed_time()
        {
            const std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_begin).count();
            const search_count_t sps = m_search_count * 1000 / duration;

            std::cout
                << std::endl
                << "総読み手数: " << m_search_count << std::endl
                << "実行時間[ms]: " << duration << std::endl
                << "読み手速度[手/s]: " << sps << std::endl << std::endl;
        }

        inline search_count_t & timer_t::search_count()
        {
            return m_search_count;
        }

        inline const search_count_t & timer_t::search_count() const
        {
            return m_search_count;
        }

        thread_local timer_t timer;

        inline void trim_front_space(std::string_view & sv)
        {
            while (!sv.empty() && sv.front() == ' ')
                sv.remove_prefix(1);
        }

        inline bool try_parse(std::string_view & sv, std::string_view token)
        {
            if (sv.size() >= token.size())
            {
                if (sv.substr(0, token.size()) == token)
                {
                    sv.remove_prefix(token.size());
                    return true;
                }
            }
            return false;
        }
    }

    class file_format_error
        : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    class parse_error
        : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    class invalid_command_line_input
        : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    class invalid_usi_input
        : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    class usi_stop_exception
        : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    using piece_t = unsigned char;
    enum : piece_t
    {
        empty,
        fu, kyo, kei, gin, kin, kaku, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu,
        sente_fu = fu, sente_kyo, sente_kei, sente_gin, sente_kin, sente_kaku, sente_hi, sente_ou, sente_tokin, sente_nari_kyo, sente_nari_kei, sente_nari_gin, sente_uma, sente_ryu,
        gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, gote_uma, gote_ryu,
        piece_enum_number,
        out_of_range = std::numeric_limits<unsigned char>::max()
    };

    inline std::optional<piece_t> char_to_piece(char c)
    {
        static const std::map<char, piece_t> map
        {
            { 'P', sente_fu },
            { 'p', gote_fu },
            { 'L', sente_kyo },
            { 'l', gote_kyo },
            { 'N', sente_kei },
            { 'n', gote_kei },
            { 'S', sente_gin },
            { 's', gote_gin },
            { 'G', sente_kin },
            { 'g', gote_kin },
            { 'B', sente_kaku },
            { 'b', gote_kaku },
            { 'R', sente_hi },
            { 'r', gote_hi },
            { 'K', sente_ou },
            { 'k', gote_ou },
        };
        const auto iter = map.find(c);
        if (iter == map.end())
            return std::nullopt;
        return iter->second;
    }

    inline std::optional<std::string> piece_to_sfen_string(piece_t piece)
    {
        static const std::map<piece_t, std::string> map
        {
            { sente_fu       , "P" },
            { gote_fu        , "p" },
            { sente_kyo      , "L" },
            { gote_kyo       , "l" },
            { sente_kei      , "N" },
            { gote_kei       , "n" },
            { sente_gin      , "S" },
            { gote_gin       , "s" },
            { sente_kin      , "G" },
            { gote_kin       , "g" },
            { sente_kaku     , "B" },
            { gote_kaku      , "b" },
            { sente_hi       , "R" },
            { gote_hi        , "r" },
            { sente_ou       , "K" },
            { gote_ou        , "k" },
            { sente_tokin    , "P+" },
            { gote_tokin     , "p+" },
            { sente_nari_kyo , "L+" },
            { gote_nari_kyo  , "l+" },
            { sente_nari_kei , "N+" },
            { gote_nari_kei  , "n+" },
            { sente_nari_gin , "S+" },
            { gote_nari_gin  , "s+" },
            { sente_uma      , "B+" },
            { gote_uma       , "b+" },
            { sente_ryu      , "R+" },
            { gote_ryu       , "r+" },
        };
        const auto iter = map.find(piece);
        if (iter == map.end())
            return std::nullopt;
        return iter->second;
    }

    enum color_t : unsigned char
    {
        black = 0,
        white = 1,
        color_size = 2
    };

    static constexpr color_t colors[]
    {
        black,
        white
    };

    inline char color_to_color_char(color_t color)
    {
        static constexpr char map[] { 'b', 'w' };
        SHOGIPP_ASSERT(color >= black);
        SHOGIPP_ASSERT(color <= white);
        return map[color];
    }

    /**
     * @breif 逆の手番を取得する。
     * @param color 先手か後手か
     * @retval sente color == gote の場合
     * @retval gote color == sente の場合
     */
    inline color_t operator !(color_t color)
    {
        SHOGIPP_ASSERT(color >= black);
        SHOGIPP_ASSERT(color <= white);
        return static_cast<color_t>((color + 1) % color_size);
    }

    using pos_t = signed char;
    constexpr pos_t npos = -1; // 無効な座標を表現する定数
    constexpr pos_t width = 11;
    constexpr pos_t height = 11;
    constexpr pos_t pos_size = width * height;
    constexpr pos_t padding_width = 1;
    constexpr pos_t padding_height = 1;
    constexpr pos_t suji_size = 9;
    constexpr pos_t dan_size = 9;

    enum pos_alias
    {
        X00, P10, P20, P30, P40, P50, P60, P70, P80, P90, XA0,
        X01, P11, P21, P31, P41, P51, P61, P71, P81, P91, XA1,
        X02, P12, P22, P32, P42, P52, P62, P72, P82, P92, XA2,
        X03, P13, P23, P33, P43, P53, P63, P73, P83, P93, XA3,
        X04, P14, P24, P34, P44, P54, P64, P74, P84, P94, XA4,
        X05, P15, P25, P35, P45, P55, P65, P75, P85, P95, XA5,
        X06, P16, P26, P36, P46, P56, P66, P76, P86, P96, XA6,
        X07, P17, P27, P37, P47, P57, P67, P77, P87, P97, XA7,
        X08, P18, P28, P38, P48, P58, P68, P78, P88, P98, XA8,
        X09, P19, P29, P39, P49, P59, P69, P79, P89, P99, XA9,
        X0A, X1A, X2A, X3A, X4A, X5A, X6A, X7A, X8A, X9A, XAA,
    };

    /**
     * @breif 座標から段を抽出する。
     * @param pos 座標
     * @return 段
     */
    inline constexpr pos_t pos_to_dan(pos_t pos)
    {
        return pos / width - padding_height;
    }

    /**
     * @breif 座標から筋を抽出する。
     * @param pos 座標
     * @return 筋
     */
    inline constexpr pos_t pos_to_suji(pos_t pos)
    {
        return pos % width - padding_width;
    }

    /**
     * @breif 2つの座標間のマンハッタン距離を計算する。
     * @param a 座標A
     * @param b 座標B
     * @return 2つの座標間のマンハッタン距離
     */
    inline pos_t distance(pos_t a, pos_t b)
    {
        const pos_t suji_a = pos_to_suji(a);
        const pos_t dan_a = pos_to_dan(a);
        const pos_t suji_b = pos_to_suji(b);
        const pos_t  dan_b = pos_to_dan(b);
        return static_cast<pos_t>(std::abs(suji_a - suji_b) + std::abs(dan_a - dan_b));
    }

    constexpr pos_t front = -width;
    constexpr pos_t left = -1;
    constexpr pos_t right = 1;
    constexpr pos_t back = width;
    constexpr pos_t kei_left = front * 2 + left;
    constexpr pos_t kei_right = front * 2 + right;
    constexpr pos_t front_left = front + left;
    constexpr pos_t front_right = front + right;
    constexpr pos_t back_left = back + left;
    constexpr pos_t back_right = back + right;

    using pos_to_piece_pair = std::pair<pos_t, std::vector<piece_t>>;

    static const pos_to_piece_pair near_kiki_list[]
    {
        { kei_left   , { kei } },
        { kei_right  , { kei } },
        { front_left , { gin, kin, kaku, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
        { front      , { fu, kyo, gin, kin, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
        { front_right, { gin, kin, kaku, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
        { left       , { kin, ou, hi, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
        { right      , { kin, ou, hi, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
        { back_left  , { gin, ou, kaku, uma, ryu } },
        { back       , { kin, ou, hi, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
        { back_right , { gin, ou, kaku, uma, ryu } },
    };

    static const pos_to_piece_pair far_kiki_list[]
    {
        { front_left , { kaku, uma } },
        { front      , { kyo, hi, ryu } },
        { front_right, { kaku, uma } },
        { left       , { hi, ryu } },
        { right      , { hi, ryu } },
        { back_left  , { kaku, uma } },
        { back       , { hi, ryu } },
        { back_right , { kaku, uma } }
    };

    static const pos_to_piece_pair far_kiki_list_asynmmetric[]
    {
        { front      , { kyo } },
    };

    static const pos_to_piece_pair far_kiki_list_synmmetric[]
    {
        { front_left , { kaku, uma } },
        { front      , { hi, ryu } },
        { front_right, { kaku, uma } },
        { left       , { hi, ryu } },
        { right      , { hi, ryu } },
        { back_left  , { kaku, uma } },
        { back       , { hi, ryu } },
        { back_right , { kaku, uma } }
    };

    static const std::map<std::string, color_t> color_string_map
    {
        { "先手" , black },
        { "後手" , white }
    };
    static constexpr std::size_t color_string_size = 4;

    static const std::map<std::string, unsigned char> digit_string_map
    {
        { "０", 0 },
        { "１", 1 },
        { "２", 2 },
        { "３", 3 },
        { "４", 4 },
        { "５", 5 },
        { "６", 6 },
        { "７", 7 },
        { "８", 8 },
        { "９", 9 }
    };
    static constexpr std::size_t digit_string_size = 2;

    static const std::map<std::string, piece_t> piece_string_map
    {
        { "・", empty },
        { "歩", fu },
        { "香", kyo },
        { "桂", kei },
        { "銀", gin },
        { "金", kin },
        { "角", kaku },
        { "飛", hi },
        { "王", ou },
        { "と", tokin },
        { "杏", nari_kyo },
        { "圭", nari_kei },
        { "全", nari_gin },
        { "馬", uma },
        { "竜", ryu }
    };
    static constexpr std::size_t piece_string_size = 2;

    static const std::map<std::string, color_t> color_prefix_string_map
    {
        { " ", black },
        { "v", white }
    };
    static constexpr std::size_t color_prefix_string_size = 1;

    static const std::map<std::string, color_t> color_mark_map
    {
        { "▲", black },
        { "△", white }
    };
    static constexpr std::size_t color_mark_size = 2;

    static const std::map<std::string, unsigned char> suji_string_map
    {
        { "１", 8 },
        { "２", 7 },
        { "３", 6 },
        { "４", 5 },
        { "５", 4 },
        { "６", 3 },
        { "７", 2 },
        { "８", 1 },
        { "９", 0 }
    };
    static constexpr std::size_t suji_string_size = 2;

    static const std::map<std::string, unsigned char> dan_string_map
    {
        { "一", 0 },
        { "二", 1 },
        { "三", 2 },
        { "四", 3 },
        { "五", 4 },
        { "六", 5 },
        { "七", 6 },
        { "八", 7 },
        { "九", 8 }
    };
    static constexpr std::size_t dan_string_size = 2;

    template<typename Map>
    inline std::optional<typename std::decay_t<Map>::mapped_type> parse(std::string_view & rest, Map && map, std::size_t size)
    {
        if (rest.size() < size)
            return std::nullopt;
        const auto iter = map.find(std::string{ rest.substr(0, size) });
        if (iter == map.end())
            return std::nullopt;
        rest.remove_prefix(size);
        return iter->second;
    }

    inline void parse(std::string_view & rest, std::string_view s)
    {
        if (rest.size() < s.size())
            throw parse_error{ "parse 1-1" };
        if (rest.substr(0, s.size()) != s)
            throw parse_error{ "parse 1-2" };
        rest.remove_prefix(s.size());
    }

    using move_count_t = unsigned int;
    using depth_t = unsigned int;

    /**
     * @breif 後手の場合に -1 を、先手の場合に 1 を返す。
     * @param color 先手か後手か
     * @return 符号反転用の数値
     */
    inline constexpr pos_t reverse(color_t color)
    {
        return color ? -1 : 1;
    }

    /**
     * @breif 重複しないハッシュ値を返す。
     * @return 重複しないハッシュ値
     */
    inline std::size_t unique_hash()
    {
        static std::minstd_rand rand{ SHOGIPP_SEED };
        static std::uniform_int_distribution<std::size_t> uid{ std::numeric_limits<std::size_t>::min(), std::numeric_limits<std::size_t>::max() };
        static std::unordered_set<std::size_t> returned;

        std::size_t hash;
        while (hash = uid(rand), returned.find(hash) != returned.end());
        returned.insert(hash);
        return hash;
    }

#ifdef SIZE_OF_HASH

    /**
     * @breif 任意のバイト数のハッシュ値を提供する。
     * @tparam HashSize ハッシュ値のバイト数
     */
    template<std::size_t HashSize>
    class basic_hash_t
    {
    public:
        template<typename CharT, typename Traits, std::size_t HashSize>
        friend std::basic_ostream<CharT, Traits> & operator<<(std::basic_ostream<CharT, Traits> & o, const basic_hash_t<HashSize> & hash);

        constexpr static std::size_t hash_size = HashSize;

        inline basic_hash_t()
        {
            static_assert(hash_size % sizeof(std::size_t) == 0);
            for (std::size_t i = 0; i < hash_size / sizeof(std::size_t); ++i)
                *reinterpret_cast<std::size_t *>(data + i * sizeof(std::size_t)) = unique_hash();
        }

        inline basic_hash_t(const basic_hash_t & hash)
        {
            std::copy(std::begin(hash.data), std::end(hash.data), std::begin(data));
        }

        inline basic_hash_t & operator =(const basic_hash_t & hash)
        {
            std::copy(std::begin(hash.data), std::end(hash.data), std::begin(data));
            return *this;
        }

        inline basic_hash_t & operator ^=(const basic_hash_t & hash)
        {
            std::size_t * first = reinterpret_cast<std::size_t *>(data);
            std::size_t * end = reinterpret_cast<std::size_t *>(data + hash_size);
            const std::size_t * input = reinterpret_cast<const std::size_t *>(hash.data);
            while (first != end)
                *first++ ^= *input++;
            return *this;
        }

        inline operator std::size_t() const
        {
            std::size_t hash = 0;
            for (std::size_t i = 0; i < hash_size / sizeof(std::size_t); ++i)
                hash ^= *reinterpret_cast<const std::size_t *>(data + i * sizeof(std::size_t));
            return hash;
        }

        inline operator std::string() const
        {
            std::ostringstream stream;
            stream << "0x";
            for (std::size_t i = 0; i < hash_size / sizeof(std::size_t); ++i)
                stream << std::hex << std::setfill('0') << *reinterpret_cast<const std::size_t *>(data + i * sizeof(std::size_t));
            stream << std::flush;
            return stream.str();
        }

    private:
        using byte_type = unsigned char;
        byte_type data[hash_size];
    };

    template<typename CharT, typename Traits, std::size_t HashSize>
    std::basic_ostream<CharT, Traits> & operator<<(std::basic_ostream<CharT, Traits> & o, const basic_hash_t<HashSize> & hash)
    {
        o << static_cast<std::string>(hash);
        return o;
    }

    template<std::size_t HashSize>
    struct basic_hash_hash_t
    {
        inline std::size_t operator()(basic_hash_t<HashSize> key) const
        {
            return static_cast<std::size_t>(key);
        }
    };

    using hash_t = basic_hash_t<SIZE_OF_HASH>;
#else
    using hash_t = std::size_t;
#endif

    /**
     * @breif ハッシュ値を16進数表現の文字列に変換する。
     * @param hash ハッシュ値
     * @return 16進数表現の文字列
     */
    inline std::string hash_to_string(hash_t hash)
    {
#ifdef SIZE_OF_HASH
        return static_cast<std::string>(hash);
#else
        std::ostringstream stream;
        stream << "0x" << std::hex << std::setfill('0') << hash << std::flush;
        return stream.str();
#endif
    }

    /*
     * @breif 駒が成駒か判定する。
     * @param piece 駒
     * @retval true 成駒である
     * @retval false 成駒でない
     */
    inline bool is_promoted(piece_t piece)
    {
        SHOGIPP_ASSERT(piece != empty);
        constexpr bool map[]
        {
            false,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true
        };
        return map[piece];
    }

    /*
     * @breif 駒が成れるか判定する。
     * @param piece 駒
     * @retval true 成れる
     * @retval false 成れない
     */
    inline bool is_promotable(piece_t piece)
    {
        SHOGIPP_ASSERT(piece != empty);
        constexpr static bool map[]
        {
            false,
            true, true, true, true, false, true, true, false, false, false, false, false, false, false,
            true, true, true, true, false, true, true, false, false, false, false, false, false, false,
        };
        return map[piece];
    }

    /*
     * @breif 駒が後手の駒か判定する。
     * @param piece 駒
     * @return 先手の駒である場合 sente
     */
    inline color_t to_color(piece_t piece)
    {
        SHOGIPP_ASSERT(piece != empty);
        constexpr static color_t map[]
        {
            black, /* dummy */
            black, black, black, black, black, black, black, black, black, black, black, black, black, black,
            white, white, white, white, white, white, white, white, white, white, white, white, white, white,
        };
        return map[piece];
    }

    /*
     * @breif 駒が走り駒(香・角・飛・馬・竜)か判定する。
     * @param piece 駒
     * @return 走り駒である場合 true
     */
    inline bool is_hashirigoma(piece_t piece)
    {
        SHOGIPP_ASSERT(piece != empty);
        constexpr static bool map[]
        {
            false,
            false, true, false, false, false, true, true, false, false, false, false, false, true, true,
            false, true, false, false, false, true, true, false, false, false, false, false, true, true,
        };
        return map[piece];
    }

    /**
     * @breif 駒を持ち駒として適格の駒に変換する。
     * @param piece 駒
     * @return 持ち駒として適格の駒
     */
    inline piece_t to_mochigoma(piece_t piece)
    {
        SHOGIPP_ASSERT(piece != empty);
        constexpr static piece_t map[]
        {
            0,
            fu, kyo, kei, gin, kin, kaku, hi, ou, fu, kyo, kei, gin, kaku, hi,
            fu, kyo, kei, gin, kin, kaku, hi, ou, fu, kyo, kei, gin, kaku, hi,
        };
        return map[piece];
    }

    /*
     * @breif 駒を成る前の駒に変換する。
     * @param piece 駒
     * @return 成る前の駒
     */
    inline piece_t to_unpromoted(piece_t piece)
    {
        SHOGIPP_ASSERT(piece != empty);
        constexpr static piece_t map[]
        {
            0,
            sente_fu, sente_kyo, sente_kei, sente_gin, sente_kin, sente_kaku, sente_hi, sente_ou, sente_fu, sente_kyo, sente_kei, sente_gin, sente_kaku, sente_hi,
            gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_fu, gote_kyo, gote_kei, gote_gin, gote_kaku, gote_hi,
        };
        return map[piece];
    }

    /*
     * @breif 駒を成り駒に変換する。
     * @param piece 駒
     * @return 成り駒
     */
    inline piece_t to_promoted(piece_t piece)
    {
        SHOGIPP_ASSERT(is_promotable(piece));
        constexpr static piece_t map[]
        {
            0,
            sente_tokin, sente_nari_kyo, sente_nari_kei, sente_nari_gin, 0, sente_uma, sente_ryu, 0, 0, 0, 0, 0, 0, 0,
            gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, 0, gote_uma, gote_ryu, 0, 0, 0, 0, 0, 0, 0,
        };
        return map[piece];
    }

    /*
     * @breif 駒から先手後手の情報を取り除く。
     * @param piece 駒
     * @return 先手後手の情報を取り除かれた駒
     * @details sente_fu -> fu, gote_fu -> fu
     */
    inline piece_t trim_color(piece_t piece)
    {
        SHOGIPP_ASSERT(piece != empty);
        constexpr static piece_t map[]
        {
            0,
            fu, kyo, kei, gin, kin, kaku, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu,
            fu, kyo, kei, gin, kin, kaku, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu,
        };
        return map[piece];
    }

    /**
     * @breif 駒を後手の駒に変換する。
     * @param piece 駒
     * @return 後手の駒
     * @details fu -> gote_fu
     */
    inline piece_t to_gote(piece_t piece)
    {
        SHOGIPP_ASSERT(piece != empty);
        constexpr static piece_t map[]
        {
            0,
            gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, gote_uma, gote_ryu,
            gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, gote_uma, gote_ryu,
        };
        return map[piece];
    }

    /**
     * @breif piece が target_piece に合致するか判定する。
     * @param piece 駒
     * @retval true 合致する
     * @retval false 合致しない
     */
    template<piece_t target_piece>
    inline bool match(piece_t piece)
    {
        SHOGIPP_ASSERT(piece != empty);
        static const struct impl_t
        {
            impl_t()
            {
                for (piece_t piece = fu; piece < piece_enum_number; ++piece)
                    map[piece] = trim_color(piece) == target_piece;
            }
            bool map[piece_enum_number]{};
        } impl;
        return impl.map[piece];
    }
    

    class move_t;

    /**
     * @breif ハッシュテーブル
     */
    class hash_table_t
    {
    public:
        /**
         * ハッシュテーブルを構築する。
         */
        inline hash_table_t();

        /**
         * @breif 盤上の駒のハッシュ値を計算する。
         * @param piece 駒
         * @param pos 駒の座標
         * @return ハッシュ値
         */
        inline hash_t piece_hash(piece_t piece, pos_t pos) const;

        /**
         * @breif 持ち駒のハッシュ値を計算する。
         * @param piece 駒
         * @param count 駒の数
         * @param is_gote 後手の持ち駒か
         * @return ハッシュ値
         */
        inline hash_t captured_piece_hash(piece_t piece, std::size_t count, color_t color) const;

        /**
         * @breif 手番のハッシュ値を計算する。
         * @param color 先手か後手か
         * @return ハッシュ値
         */
        inline hash_t color_hash(color_t color) const;

        /**
         * @breif 合法手のハッシュ値を計算する。
         * @param move 合法手
         * @param color 先手か後手か
         * @return ハッシュ値
         */
        inline hash_t move_hash(const move_t & move, color_t color) const;

    private:
        hash_t board_table[piece_enum_number * suji_size * dan_size];        // 盤のハッシュテーブル
        hash_t captured_piece_table[(18 + 4 + 4 + 4 + 4 + 2 + 2) * 2 * 2];  // 持ち駒のハッシュテーブル
        hash_t color_table[color_size];                                     // 手番のハッシュテーブル
        hash_t move_table[(pos_size + 1) * pos_size * color_size];          // 移動する手のハッシュテーブル
        hash_t put_table[pos_size * (hi - fu + 1) * color_size];            // 打つ手のハッシュテーブル
    };

    static const hash_table_t hash_table;

    inline hash_table_t::hash_table_t()
    {
#ifndef SIZE_OF_HASH
        std::generate(std::begin(board_table         ), std::end(board_table         ), unique_hash);
        std::generate(std::begin(captured_piece_table), std::end(captured_piece_table), unique_hash);
        std::generate(std::begin(color_table         ), std::end(color_table         ), unique_hash);
        std::generate(std::begin(move_table          ), std::end(move_table          ), unique_hash);
        std::generate(std::begin(put_table           ), std::end(put_table           ), unique_hash);
#endif
    }

    inline hash_t hash_table_t::piece_hash(piece_t piece, pos_t pos) const
    {
        std::size_t index = static_cast<std::size_t>(piece);
        index *= suji_size;
        index += pos_to_suji(pos);
        index *= dan_size;
        index += pos_to_dan(pos);
        SHOGIPP_ASSERT(index < std::size(board_table));
        return board_table[index];
    }

    inline hash_t hash_table_t::captured_piece_hash(piece_t piece, std::size_t count, color_t color) const
    {
        enum
        {
            mochigoma_fu_offset   = 0                                           , mochigoma_fu_size    = 18 + 1,
            mochigoma_kyo_offset  = mochigoma_fu_offset   + mochigoma_fu_size   , mochigoma_kyo_size   =  4 + 1,
            mochigoma_kei_offset  = mochigoma_kyo_offset  + mochigoma_kyo_size  , mochigoma_kei_size   =  4 + 1,
            mochigoma_gin_offset  = mochigoma_kei_offset  + mochigoma_kei_size  , mochigoma_gin_size   =  4 + 1,
            mochigoma_kin_offset  = mochigoma_gin_offset  + mochigoma_gin_size  , mochigoma_kin_size   =  4 + 1,
            mochigoma_kaku_offset = mochigoma_kin_offset  + mochigoma_kin_size  , mochigoma_kaku_size  =  2 + 1,
            mochigoma_hi_offset   = mochigoma_kaku_offset + mochigoma_kaku_size , mochigoma_hi_size    =  2 + 1,
            mochigoma_size        = mochigoma_hi_offset   + mochigoma_hi_size 
        };

        SHOGIPP_ASSERT(piece != empty);
        SHOGIPP_ASSERT(piece >= fu);
        SHOGIPP_ASSERT(piece <= hi);
        SHOGIPP_ASSERT(!(piece == fu   && count >= mochigoma_fu_size ));
        SHOGIPP_ASSERT(!(piece == kyo  && count >= mochigoma_kyo_size ));
        SHOGIPP_ASSERT(!(piece == kei  && count >= mochigoma_kei_size ));
        SHOGIPP_ASSERT(!(piece == gin  && count >= mochigoma_gin_size ));
        SHOGIPP_ASSERT(!(piece == kin  && count >= mochigoma_kin_size ));
        SHOGIPP_ASSERT(!(piece == kaku && count >= mochigoma_kaku_size ));
        SHOGIPP_ASSERT(!(piece == hi   && count >= mochigoma_hi_size ));

        static const std::size_t map[]
        {
            0,
            mochigoma_fu_offset,
            mochigoma_kyo_offset,
            mochigoma_kei_offset,
            mochigoma_gin_offset,
            mochigoma_kin_offset,
            mochigoma_kaku_offset,
            mochigoma_hi_offset,
        };

        std::size_t index = map[piece];
        index += count;
        index *= color_size;
        index += static_cast<std::size_t>(color);
        SHOGIPP_ASSERT(index < std::size(captured_piece_table));
        return captured_piece_table[index];
    }

    inline hash_t hash_table_t::color_hash(color_t color) const
    {
        SHOGIPP_ASSERT(color >= black);
        SHOGIPP_ASSERT(color <= white);
        return color_table[color];
    }

    /**
     * @breif 先後を表現する文字列を取得する。
     * @param color 先後
     * @return 先後を表現する文字列
     */
    inline const char * color_to_string(color_t color)
    {
        const char * map[]{ "先手", "後手" };
        return map[color];
    }

    /**
     * @breif 数値を全角文字列に変換する。
     * @param value 数値
     * @return 全角文字列
     * @details 持ち駒の最大枚数18を超える値を指定してこの関数を呼び出してはならない。
     */
    inline const char * to_zenkaku_digit(unsigned int value)
    {
        const char * map[]
        {
            "０", "１", "２", "３", "４", "５", "６", "７", "８", "９",
            "１０", "１１", "１２", "１３", "１４", "１５", "１６", "１７", "１８"
        };
        SHOGIPP_ASSERT(value <= std::size(map));
        return map[value];
    }

    /**
     * @breif 駒の移動先の相対座標の配列の先頭を指すポインタを取得する。
     * @param piece 駒
     * @return 駒の移動先の相対座標の配列の先頭を指すポインタ
     * @details この関数が返すポインタの指す座標は 0 で終端化されている。
     */
    inline const pos_t * near_move_offsets(piece_t piece)
    {
        static const std::vector<pos_t> map[]
        {
            /* empty    */ { 0 },
            /* fu       */ { front, 0 },
            /* kyo      */ { 0 },
            /* kei      */ { kei_left, kei_right, 0},
            /* gin      */ { front_left, front, front_right, back_left, back_right, 0 },
            /* kin      */ { front_left, front, front_right, left, right, back, 0 },
            /* kaku     */ { 0 },
            /* hi       */ { 0 },
            /* ou       */ { front_left, front, front_right, left, right, back_left, back, back_right, 0 },
            /* tokin    */ { front_left, front, front_right, left, right, back, 0 },
            /* nari_kyo */ { front_left, front, front_right, left, right, back, 0 },
            /* nari_kei */ { front_left, front, front_right, left, right, back, 0 },
            /* nari_gin */ { front_left, front, front_right, left, right, back, 0 },
            /* uma      */ { front, left, right, back, 0 },
            /* ryu      */ { front_left, front_right, back_left, back_right, 0 },
        };
        SHOGIPP_ASSERT(piece != empty);
        SHOGIPP_ASSERT(piece <= std::size(map));
        return map[piece].data();
    }

    /**
     * @breif 駒の移動先の相対座標の配列の先頭を指すポインタを取得する。
     * @param piece 駒
     * @return 駒の移動先の相対座標の配列の先頭を指すポインタ
     * @details この関数が返すポインタの指す座標は 0 で終端化されている。
     */
    inline const pos_t * far_move_offsets(piece_t piece)
    {
        static const std::vector<pos_t> map[]
        {
            /* empty    */ { 0 },
            /* fu       */ { 0 },
            /* kyo      */ { front, 0 },
            /* kei      */ { 0 },
            /* gin      */ { 0 },
            /* kin      */ { 0 },
            /* kaku     */ { front_left, front_right, back_left, back_right, 0 },
            /* hi       */ { front, left, right, back, 0 },
            /* ou       */ { 0 },
            /* tokin    */ { 0 },
            /* nari_kyo */ { 0 },
            /* nari_kei */ { 0 },
            /* nari_gin */ { 0 },
            /* uma      */ { front_left, front_right, back_left, back_right, 0 },
            /* ryu      */ { front, left, right, back, 0 },
        };
        SHOGIPP_ASSERT(piece != empty);
        SHOGIPP_ASSERT(piece <= std::size(map));
        return map[piece].data();
    }

    /**
     * @breif 駒を文字列に変換する。
     * @param piece 駒
     * @return 文字列
     */
    inline const char * piece_to_string(piece_t piece)
    {
        SHOGIPP_ASSERT(piece < piece_enum_number);
        static const char * map[]{
            "・",
            "歩", "香", "桂", "銀", "金", "角", "飛", "王", "と", "杏", "圭", "全", "馬", "竜",
            "歩", "香", "桂", "銀", "金", "角", "飛", "王", "と", "杏", "圭", "全", "馬", "竜",
        };
        return map[piece];
    }

    /**
     * @breif 段を文字列に変換する。
     * @param dan 段
     * @return 文字列
     */
    inline const char * dan_to_string(pos_t dan)
    {
        static const char * map[]{ "一", "二", "三", "四", "五", "六", "七", "八", "九" };
        SHOGIPP_ASSERT(dan >= 0);
        SHOGIPP_ASSERT(dan < static_cast<pos_t>(std::size(map)));
        return map[dan];
    }

    /**
     * @breif 筋を文字列に変換する。
     * @param dan 筋
     * @return 文字列
     */
    inline const char * suji_to_string(pos_t suji)
    {
        static const char * map[]{ "９", "８", "７", "６", "５", "４", "３", "２", "１" };
        SHOGIPP_ASSERT(suji >= 0);
        SHOGIPP_ASSERT(suji < static_cast<pos_t>(std::size(map)));
        return map[suji];
    }

    /**
     * @breif 座標を文字列に変換する。
     * @param pos 座標
     * @return 文字列
     */
    inline std::string pos_to_string(pos_t pos)
    {
        return std::string{}.append(suji_to_string(pos_to_suji(pos))).append(dan_to_string(pos_to_dan(pos)));
    }

    /**
     * @breif 座標をSFEN表記法に準拠した文字列に変換する。
     * @param pos 座標
     * @return SFEN表記法に準拠した文字列
     */
    inline std::string pos_to_sfen_string(pos_t pos)
    {
        std::string sfen_string;
        const char suji = static_cast<char>(suji_size - 1 - pos_to_suji(pos) + '1');
        const char dan = static_cast<char>(pos_to_dan(pos) + 'a');
        sfen_string += suji;
        sfen_string += dan;
        return sfen_string;
    }

    /**
     * @breif 筋と段から座標を取得する。
     * @param suji 筋
     * @param dan 段
     * @return 座標
     */
    inline pos_t suji_dan_to_pos(pos_t suji, pos_t dan)
    {
        return width * (dan + padding_height) + padding_width + suji;
    }

    static const pos_t default_ou_pos_list[]
    {
        suji_dan_to_pos(4, 8),
        suji_dan_to_pos(4, 0)
    };

    /**
     * @breif SFEN表記法に準拠した座標の文字列から座標を取得する。
     * @param sfen_pos SFEN表記法に準拠した座標の文字列
     * @return 座標
     */
    inline pos_t sfen_pos_to_pos(std::string_view sfen_pos)
    {
        if (sfen_pos.size() != 2)
            throw invalid_usi_input{ "sfen_pos.size() != 2" };
        if (sfen_pos[0] < '1')
            throw invalid_usi_input{ "sfen_pos[0] < '1'" };
        if (sfen_pos[0] > '9')
            throw invalid_usi_input{ "sfen_pos[0] > '9'" };
        if (sfen_pos[1] < 'a')
            throw invalid_usi_input{ "sfen_pos[1] < 'a'" };
        if (sfen_pos[1] > 'i')
            throw invalid_usi_input{ "sfen_pos[1] > 'i'" };
        const pos_t suji = static_cast<pos_t>(suji_size - 1 - (sfen_pos[0] - '1'));
        const pos_t dan = static_cast<pos_t>(sfen_pos[1] - 'a');
        return suji_dan_to_pos(suji, dan);
    }

    /**
     * @breif 座標を標準出力に出力する。
     * @param pos 座標
     */
    inline void print_pos(pos_t pos)
    {
        std::cout << suji_to_string(pos_to_suji(pos)) << dan_to_string(pos_to_dan(pos));
        std::cout.flush();
    }

    class board_t;

    /**
     * @breif 合法手
     */
    class move_t
    {
    public:
        /**
         * @breif 打ち手を構築する。
         * @param destination 打つ座標
         * @param source_piece 打つ駒
         */
        inline move_t(pos_t destination, piece_t source_piece) noexcept;

        /**
         * @breif 移動する手を構築する。
         * @param source 移動元の座標
         * @param destination 移動先の座標
         * @param source_piece 移動元の駒
         * @param captured_piece 移動先の駒
         * @param promote 成か不成か
         */
        inline move_t(pos_t source, pos_t destination, piece_t source_piece, piece_t captured_piece, bool promote) noexcept;

        /**
         * @breif SFEN表記法に準拠した moves の後に続く文字列から手を構築する。
         * @param sfen SFEN表記法に準拠した moves の後に続く文字列
         * @param board 盤
         */
        inline move_t(std::string_view sfen_move, const board_t & board);

        /**
         * @breif 打ち手か判定する。
         * @retval true 打ち手である
         * @retval false 移動する手である
         */
        inline bool put() const noexcept;

        /**
         * @breif 移動元の座標を取得する。
         * @return 移動元の座標
         * @details put が true を返す場合にこの関数を呼び出した場合、無効な値が返る。
         */
        inline pos_t source() const noexcept;

        /**
         * @breif 移動先の座標を取得する。
         * @return 移動先の座標
         * @details put が true を返す場合、この関数は打つ先の座標を返す。
         */
        inline pos_t destination() const noexcept;

        /**
         * @breif 移動元の駒を取得する。
         * @return 移動元の駒
         */
        inline piece_t source_piece() const noexcept;

        /**
         * @breif 移動先の駒を取得する。
         * @return 移動先の駒
         * @detalis put が true を返す場合にこの関数を呼び出した場合、無効な値が返る。
         */
        inline piece_t captured_piece() const noexcept;

        /**
         * @breif 成るか否かを取得する。
         * @retval true 成る
         * @retval false 成らない
         * @detalis put が true を返す場合にこの関数を呼び出した場合、無効な値が返る。
         */
        inline bool promote() const noexcept;

        /**
         * @breif 合法手をSFEN表記法に準拠した文字列に変換する。
         * @return SFEN表記法に準拠した文字列
         */
        inline std::string sfen_string() const;

    private:
        pos_t   m_source;           // 移動元の座標(source == npos の場合、持ち駒を打つ)
        pos_t   m_destination;      // 移動先の座標(source == npos の場合、 destination は打つ座標)
        piece_t m_source_piece;     // 移動元の駒(source == npos の場合、 source_piece() は打つ持ち駒)
        piece_t m_captured_piece;   // 移動先の駒(source == npos の場合、 captured_piece は未定義)
        bool    m_promote;          // 成る場合 true
    };

    inline move_t::move_t(pos_t destination, piece_t source_piece) noexcept
        : m_source{ npos }
        , m_destination{ destination }
        , m_source_piece{ source_piece }
        , m_captured_piece{ empty }
        , m_promote{ false }
    {
        SHOGIPP_ASSERT(source_piece >= fu);
        SHOGIPP_ASSERT(source_piece <= hi);
    }

    inline move_t::move_t(pos_t source, pos_t destination, piece_t source_piece, piece_t captured_piece, bool promote) noexcept
        : m_source{ source }
        , m_destination{ destination }
        , m_source_piece{ source_piece }
        , m_captured_piece{ captured_piece }
        , m_promote{ promote }
    {
    }

    inline bool move_t::put() const noexcept
    {
        return m_source == npos;
    }

    inline pos_t move_t::source() const noexcept
    {
        SHOGIPP_ASSERT(!put());
        return m_source;
    }

    inline pos_t move_t::destination() const noexcept
    {
        return m_destination;
    }

    inline piece_t move_t::source_piece() const noexcept
    {
        return m_source_piece;
    }

    inline piece_t move_t::captured_piece() const noexcept
    {
        SHOGIPP_ASSERT(!put());
        return m_captured_piece;
    }

    inline bool move_t::promote() const noexcept
    {
        SHOGIPP_ASSERT(!put());
        return m_promote;
    }
    
    inline std::string move_t::sfen_string() const
    {
        std::string result;
        if (put())
        {
            SHOGIPP_ASSERT(to_color(source_piece()) == black);
            const auto optional_piece = piece_to_sfen_string(source_piece());
            SHOGIPP_ASSERT(optional_piece.has_value());
            SHOGIPP_ASSERT(optional_piece->size() == 1);
            result += *optional_piece;
            result += '*';
            result += pos_to_sfen_string(destination());
        }
        else
        {
            result += pos_to_sfen_string(source());
            result += pos_to_sfen_string(destination());
            if (promote())
                result += '+';
        }
        return result;
    }

    /**
     * @breif 合法手を格納する std::vector を表現する。
     */
    class moves_t
        : public std::vector<move_t>
    {
    public:
        using std::vector<move_t>::vector;

        inline moves_t()
        {
            constexpr std::size_t max_size = 593;
            reserve(max_size);
        }
    };

    inline int to_category(const move_t & move)
    {
        if (move.put())
            return 0;
        if (move.captured_piece() == empty)
            return 1;
        return 2;
    };

    class context_t
    {
    public:
        inline context_t(piece_t captured);
        inline piece_t captured() const;

    private:
        piece_t m_captured;
    };

    inline context_t::context_t(piece_t captured)
        : m_captured{ captured }
    {
    }

    inline piece_t context_t::captured() const
    {
        return m_captured;
    }

    inline hash_t hash_table_t::move_hash(const move_t & move, color_t color) const
    {
        std::size_t index;
        if (move.put())
        {
            index = static_cast<std::size_t>(move.destination());
            index *= piece_enum_number;
            index += move.source_piece();
            index *= color_size;
            index += color;
            SHOGIPP_ASSERT(index < std::size(put_table));
            return put_table[index];
        }
        index = static_cast<std::size_t>(move.source() - npos);
        index *= pos_size;
        index += static_cast<std::size_t>(move.destination());
        index *= color_size;
        index += color;
        SHOGIPP_ASSERT(index < std::size(move_table));
        return move_table[index];
    }

    /**
     * @breif 持ち駒
     */
    class captured_pieces_t
    {
    public:
        using size_type = unsigned char;

        /**
         * @breif 持ち駒を構築する。
         */
        inline captured_pieces_t();

        /**
         * @breif 持ち駒を標準出力に出力する。
         */
        inline void print() const;

        /**
         * @breif 駒と対応する持ち駒の数の参照を返す。
         * @param 駒
         * @return 駒と対応する持ち駒の数の参照
         */
        inline size_type & operator [](piece_t piece);

        /**
         * @breif 駒と対応する持ち駒の数の参照を返す。
         * @param 駒
         * @return 駒と対応する持ち駒の数の参照
         */
        inline const size_type & operator [](piece_t piece) const;

    private:
        size_type count[hi - fu + 1];
    };

    inline captured_pieces_t::captured_pieces_t()
    {
        std::fill(std::begin(count), std::end(count), 0);
    }

    inline void captured_pieces_t::print() const
    {
        unsigned int kind = 0;
        for (piece_t piece = hi; piece >= fu; --piece)
        {
            if ((*this)[piece] > 0)
            {
                std::cout << piece_to_string(piece);
                if ((*this)[piece] > 1)
                    std::cout << to_zenkaku_digit((*this)[piece]);
                ++kind;
            }
        }
        if (kind == 0)
            std::cout << "なし";
        std::cout << std::endl;
    }

    inline captured_pieces_t::size_type & captured_pieces_t::operator [](piece_t piece)
    {
        SHOGIPP_ASSERT(piece != empty);
        SHOGIPP_ASSERT(to_mochigoma(piece) != ou);
        return count[to_mochigoma(piece) - fu];
    }

    inline const captured_pieces_t::size_type & captured_pieces_t::operator [](piece_t piece) const
    {
        return (*const_cast<captured_pieces_t *>(this))[piece];
    }

    class kyokumen_rollback_validator_t;

#define _ empty
#define x out_of_range
    constexpr piece_t clear_board[]
    {
        x, x, x, x, x, x, x, x, x, x, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, x, x, x, x, x, x, x, x, x, x,
    };

    constexpr piece_t initial_board[]
    {
        x, x, x, x, x, x, x, x, x, x, x,
        x, gote_kyo, gote_kei, gote_gin, gote_kin, gote_ou, gote_kin, gote_gin, gote_kei, gote_kyo, x,
        x, _, gote_hi, _, _, _, _, _, gote_kaku, _, x,
        x, gote_fu, gote_fu, gote_fu, gote_fu, gote_fu, gote_fu, gote_fu, gote_fu, gote_fu, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, sente_fu, sente_fu, sente_fu, sente_fu, sente_fu, sente_fu, sente_fu, sente_fu, sente_fu, x,
        x, _, sente_kaku, _, _, _, _, _, sente_hi, _, x,
        x, sente_kyo, sente_kei, sente_gin, sente_kin, sente_ou, sente_kin, sente_gin, sente_kei, sente_kyo, x,
        x, x, x, x, x, x, x, x, x, x, x,
    };
#undef _
#undef x

    /**
     * @breif 盤
     */
    class board_t
    {
    public:
        /**
         * @breif 盤を構築する。
         */
        inline board_t();

        inline piece_t & operator [](size_t i) { return data[i]; }
        inline const piece_t & operator [](size_t i) const { return data[i]; }

        /**
         * @breif 座標posが盤外か判定する。
         * @param pos 座標
         * @return 盤外の場合true
         */
        inline static bool out(pos_t pos);

        /**
         * @breif 盤を標準出力に出力する。
         */
        inline void print() const;

        /**
         * @breif 盤から全ての駒を取り除く。
         */
        inline void clear();

        /**
         * @breif 盤をSFEN表記法に準拠した文字列に変換する。
         */
        inline std::string sfen_string() const;

    private:
        friend class kyokumen_rollback_validator_t;
        piece_t data[pos_size];
    };

    inline board_t::board_t()
    {

        std::copy(std::begin(initial_board), std::end(initial_board), std::begin(data));
    }

    inline bool board_t::out(pos_t pos)
    {
        return pos < 0 || pos >= pos_size || clear_board[pos] == out_of_range;
    }

    inline void board_t::print() const
    {
        std::cout << "  ９ ８ ７ ６ ５ ４ ３ ２ １" << std::endl;
        std::cout << "+---------------------------+" << std::endl;
        for (pos_t dan = 0; dan < dan_size; ++dan)
        {
            std::cout << "|";
            for (pos_t suji = 0; suji < suji_size; ++suji)
            {
                piece_t piece = data[suji_dan_to_pos(suji, dan)];
                std::cout << ((piece != empty && to_color(piece)) ? "v" : " ") << piece_to_string(piece);
            }
            std::cout << "| " << dan_to_string(dan) << std::endl;
        }
        std::cout << "+---------------------------+" << std::endl;
    }

    inline void board_t::clear()
    {
        std::copy(std::begin(clear_board), std::end(clear_board), std::begin(data));
    }

    inline std::string board_t::sfen_string() const
    {
        std::string result;
        for (pos_t dan = 0; dan < dan_size; ++dan)
        {
            pos_t empty_count = 0;
            for (pos_t suji = 0; suji < suji_size; ++suji)
            {
                const piece_t piece = data[suji_dan_to_pos(suji, dan)];
                if (piece == empty)
                    empty_count += 1;
                else
                {
                    if (empty_count > 0)
                    {
                        result += static_cast<char>('0' + empty_count);
                        empty_count = 0;
                    }
                    const std::optional<std::string> optional_sfen_string = piece_to_sfen_string(piece);
                    SHOGIPP_ASSERT(optional_sfen_string.has_value());
                    result += *optional_sfen_string;
                }
            }
            if (empty_count > 0)
                result += static_cast<char>('0' + empty_count);
            if (dan + 1 < dan_size)
                result += '/';
        }
        return result;
    }

    inline move_t::move_t(std::string_view sfen_move, const board_t & board)
    {
        if (sfen_move.size() < 4)
            throw invalid_usi_input{ "invalid sfen move" };

        if (sfen_move[1] == '*')
        {
            if (sfen_move.size() > 4)
                throw invalid_usi_input{ "invalid sfen move" };
            const std::optional<piece_t> optional_piece = char_to_piece(sfen_move[0]);
            if (!optional_piece)
                throw invalid_usi_input{ "invalid sfen move" };
            if (to_color(*optional_piece) == white)
                throw invalid_usi_input{ "invalid sfen move" };
            const pos_t destination = sfen_pos_to_pos(sfen_move.substr(2, 2));

            m_source = npos;
            m_destination = destination;
            m_source_piece = *optional_piece;
            m_captured_piece = empty;
            m_promote = false;
        }
        else
        {
            const pos_t source = sfen_pos_to_pos(sfen_move.substr(0, 2));
            if (board[source] == empty)
                throw invalid_usi_input{ "invalid sfen move 1" };
            if (board_t::out(source))
                throw invalid_usi_input{ "invalid sfen move 2" };
            const pos_t destination = sfen_pos_to_pos(sfen_move.substr(2, 2));
            if (board_t::out(destination))
                throw invalid_usi_input{ "invalid sfen move 3" };
            bool promote;
            if (sfen_move.size() == 5 && sfen_move[4] == '+')
                promote = true;
            else if (sfen_move.size() == 4)
                promote = false;
            else
                throw invalid_usi_input{ "invalid sfen move" };

            m_source = source;
            m_destination = destination;
            m_source_piece = board[source];
            m_captured_piece = board[destination];
            m_promote = promote;
        }
    }

    /**
     * @breif 利き
     */
    class kiki_t
    {
    public:
        pos_t pos;      // 利いている駒の座標
        pos_t offset;   // 利かされている駒の座標を基準とする利きの相対座標
        bool aigoma;    // 合駒が可能か
    };

    class aigoma_info_t : public std::unordered_map<pos_t, std::vector<pos_t>>
    {
    public:
        using std::unordered_map<pos_t, std::vector<pos_t>>::unordered_map;

        inline void print() const
        {
            for (const auto & [pos, candidates] : *this)
                std::cout << "合駒：" << pos_to_string(pos) << std::endl;
        }
    };

    /**
     * @breif 参照される直前に遅延評価する機能を提供する。
     */
    template<typename T>
    class lazy_evaluated_t
    {
    public:
        using value_type = T;
        using function_type = std::function<void(value_type &)>;

        /**
         * @breif 参照される直前に遅延評価する機能を提供する。
         * @param evaluator 遅延評価する関数
         */
        inline lazy_evaluated_t(const function_type & evaluator)
            : m_value{}
            , m_valid{ false }
            , m_evaluator{ evaluator }
        {
        }

        /**
         * @breif 参照される直前に遅延評価する機能を提供する。
         * @return 遅延評価された値
         */
        inline value_type & operator *()
        {
            evaluate();
            return m_value;
        }

        /**
         * @breif 参照される直前に遅延評価する機能を提供する。
         * @return 遅延評価された値
         */
        inline const value_type & operator *() const
        {
            evaluate();
            return m_value;
        }

        /**
         * @breif 参照される直前に遅延評価する機能を提供する。
         * @return 遅延評価された値
         */
        inline value_type * operator ->()
        {
            evaluate();
            return &m_value;
        }

        /**
         * @breif 参照される直前に遅延評価する機能を提供する。
         * @return 遅延評価された値
         */
        inline const value_type * operator ->() const
        {
            evaluate();
            return &m_value;
        }

        /**
         * @breif 再評価を要求する。
         */
        inline void request_reevaluation()
        {
            m_valid = false;
        }
        
    //private:
        mutable value_type m_value;
        mutable bool m_valid;
        function_type m_evaluator;

        inline void evaluate() const
        {
            if (!m_valid)
            {
                m_evaluator(m_value);
                m_valid = true;
            }
        }
    };

    /**
     * @breif 局面の追加情報
     * @details 手番の合法手を検索する過程で手番にかかっている王手が必要になるため、個別にスタック構造を保持する。
     */
    class additional_info_t
    {
    public:
        std::vector<std::vector<kiki_t>> check_list_stack;  // 手番にかかっている王手
        std::vector<hash_t> hash_stack;                     // 局面のハッシュ値
        pos_t king_pos_list[color_size];                    // 王の座標
    };

    /**
     * @breif 局面
     */
    class kyokumen_t
    {
    public:
        /**
         * @breif 局面を構築する。
         */
        inline kyokumen_t();

        /**
         * @breif position コマンドで指定された文字列から局面を構築する。
         */
        inline kyokumen_t(std::string_view position);

        /**
         * @breif 駒が移動する場合に成りが可能か判定する。
         * @param piece 駒
         * @param source 移動元の座標
         * @param destination 移動先の座標
         * @return 成りが可能の場合(駒が既に成っている場合、常にfalse)
         */
        inline static bool promotable(piece_t piece, pos_t source, pos_t destination);

        /**
         * @breif 駒が移動する場合に成りが必須か判定する。
         * @param piece 駒
         * @param destination 移動先の座標
         * @return 成りが必須の場合(駒が既に成っている場合、常にfalse)
         */
        inline static bool must_promote(piece_t piece, pos_t destination);

        /**
         * @breif 移動元の座標から移動可能の移動先を反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param source 移動元の座標
         * @param offset 移動先の相対座標
         */
        template<typename OutputIterator>
        inline void search_far_destination(OutputIterator result, pos_t source, pos_t offset) const;

        /**
         * @breif 移動元の座標から移動可能の移動先を非反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param source 移動元の座標
         * @param offset 移動先の相対座標
         */
        template<typename OutputIterator>
        inline void search_near_destination(OutputIterator result, pos_t source, pos_t offset) const;

        /**
         * @breif 移動元の座標から移動可能の移動先を検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param source 移動元の座標
         * @param color 先手・後手どちらの移動か
         */
        template<typename OutputIterator>
        inline void search_destination(OutputIterator result, pos_t source, color_t color) const;

        /**
         * @breif 持ち駒を移動先の座標に打つことができるか判定する。歩、香、桂に限りfalseを返す可能性がある。
         * @param piece 持ち駒
         * @param destination 移動先の座標
         * @return 置くことができる場合 true
         */
        inline bool puttable(piece_t piece, pos_t destination) const;

        /**
         * @breif 移動元の座標を検索する。
         * @param result 出力イテレータ
         * @param color 先手・後手どちらの移動か
         */
        template<typename OutputIterator>
        inline void search_source(OutputIterator result, color_t color) const;

        /**
         * @breif 座標posから相対座標offset方向に走査し最初に駒が現れる座標を返す。
         * @param pos 走査を開始する座標
         * @param offset 走査する相対座標
         * @return 最初に駒が現れる座標(駒が見つからない場合 npos )
         */
        inline pos_t search(pos_t pos, pos_t offset) const;

        /**
         * @breif 座標posを利いている駒あるいは紐を付けている駒を検索する。
         * @param result 利きの出力イテレータ
         * @param pos 座標
         * @param offset 利きの相対座標
         * @param first 利く駒の入力イテレータ(begin)
         * @param last 利く駒の入力イテレータ(end)
         * @param is_collected 見つかった駒の手番に対して出力イテレータに出力するか判定する叙述関数(bool(bool))
         * @param transform (pos, offset, aigoma) を出力イテレータに出力する変数に変換する関数
         */
        template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
        inline void search_piece_near(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const;

        /**
         * @breif 座標posを利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param offset 利きの相対座標
         * @param first 利く駒の入力イテレータ(begin)
         * @param last 利く駒の入力イテレータ(end)
         * @param is_collected 見つかった駒の手番に対して出力イテレータに出力するか判定する叙述関数(bool(bool))
         * @param transform (pos, offset, aigoma) を出力イテレータに出力する変数に変換する関数
         * @sa search_kiki_far
         */
        template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
        inline void search_piece_far(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const;

        /**
         * @breif 座標posに利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param color 先後いずれの視点か
         * @param is_collected 見つかった駒の手番に対して出力イテレータに出力するか判定する叙述関数(bool(bool))
         * @param transform (pos, offset, aigoma) を出力イテレータに出力する変数に変換する関数
         */
        template<typename OutputIterator, typename IsCollected, typename Transform>
        inline void search_piece(OutputIterator result, pos_t pos, color_t color, IsCollected is_collected, Transform transform) const;

        /**
         * @breif 座標posに紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param color 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_himo(OutputIterator result, pos_t pos, color_t color) const;

        /**
         * @breif 座標posに利いている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param color 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_kiki(OutputIterator result, pos_t pos, color_t color) const;

        /**
         * @breif 座標posに利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param color 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_kiki_or_himo(OutputIterator result, pos_t pos, color_t color) const;

        /**
         * @breif 王手を検索する。
         * @param result 座標の出力イテレータ
         * @param color 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_check(OutputIterator result, color_t color) const;

        /**
         * @breif 王手を検索する。
         * @param color 先後いずれの視点か
         * @return 王手
         */
        std::vector<kiki_t> search_check(color_t color) const;

        /**
         * @breif 追加情報をpushする。
         */
        inline void push_additional_info();

        /**
         * @breif 追加情報をpushする。
         * @param hash ハッシュ値
         */
        inline void push_additional_info(hash_t hash);

        /**
         * @breif 追加情報をpopする。
         */
        inline void pop_additional_info();

        /**
         * @breif 追加情報をclearする。
         */
        inline void clear_additional_info();

        /**
         * @breif 合駒を検索する。
         * @param aigoma_info 合駒の出力先
         * @param color 先後いずれの視点か
         */
        inline void search_aigoma(aigoma_info_t & aigoma_info, color_t color) const;

        /**
         * @breif 合駒を検索する。
         * @param color 先後いずれの視点か
         * @return 合駒の情報
         */
        inline aigoma_info_t search_aigoma(color_t color) const;

        /**
         * @breif 移動元と移動先の座標から合法手を検索する。
         * @param result 合法手の出力イテレータ
         * @param source 移動元の座標
         * @param destination 移動先の座標
         */
        template<typename OutputIterator>
        inline void search_moves_from_positions(OutputIterator result, pos_t source, pos_t destination) const;

        /**
         * @breif 合法手のうち王手を外さない手を検索する。
         * @param result 合法手の出力イテレータ
         * @details 王手されていない場合、この関数により生成される手の集合は合法手全体と完全に一致する。
         */
        template<typename OutputIterator>
        inline void search_moves_nonevasions(OutputIterator result) const;

        /**
         * @breif 王手を外す手のうち王を移動する手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_moves_evasions_ou_move(OutputIterator result) const;

        /**
         * @breif 王手を外す手のうち合駒する手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_moves_evasions_aigoma(OutputIterator result) const;

        /**
         * @breif 合法手のうち王手を外す手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_moves_evasions(OutputIterator result) const;

        /**
         * @breif 王手を外さない手のうち駒を動かす手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_moves_moves(OutputIterator result) const;

        /**
         * @breif 王手を外さない手のうち持ち駒を打つ手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_moves_puts(OutputIterator result) const;

        /**
         * @breif 合法手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_moves(OutputIterator result) const;

        /**
         * @breif 合法手を検索する。
         * @param result 合法手の出力イテレータ
         */
        inline moves_t search_moves() const;

        /**
         * @breif 局面のハッシュ値を計算する。
         * @return 局面のハッシュ値
         */
        inline hash_t make_hash() const;

        /**
         * @breif 局面のハッシュ値と合法手から、合法手を実施した後の局面のハッシュ値を計算する。
         * @param hash 合法手を実施する前の局面のハッシュ値
         * @param move 実施する合法手
         * @return 合法手を実施した後の局面のハッシュ値
         * @details 合法手により発生する差分に基づき計算するため make_hash() より比較的高速に処理される。
         *          この関数は合法手を実施するより前に呼び出される必要がある。
         */
        inline hash_t make_hash(hash_t hash, const move_t & move) const;

        /**
         * @breif 合法手を標準出力に出力する。
         * @param move 合法手
         * @param is_gote 後手の合法手か
         */
        inline void print_move(const move_t & move, color_t color) const;

        /**
         * @breif 合法手を標準出力に出力する。
         * @param first 合法手の入力イテレータのbegin
         * @param last 合法手の入力イテレータのend
         */
        template<typename InputIterator>
        inline void print_move(InputIterator first, InputIterator last) const;

        /**
         * @breif 合法手を標準出力に出力する。
         */
        inline void print_move() const;

        /**
         * @breif 王手を標準出力に出力する。
         */
        inline void print_check() const;

        /**
         * @breif 局面を標準出力に出力する。
         */
        inline void print() const;

        inline void print_kifu() const;

        /**
         * @breif 局面のハッシュ値を返す。
         * @return 局面のハッシュ値
         */
        inline hash_t hash() const;

        inline void validate_board_out();

        /**
         * @breif 合法手を実行する。
         * @param move 合法手
         */
        inline void do_move(const move_t & move);

        /**
         * @breif 合法手を実行する前に戻す。
         * @param move 合法手
         */
        inline void undo_move(const move_t & move);

        /**
         * @breif 手番を取得する。
         * @return 手番
         */
        inline color_t color() const;

        /**
         * @breif 指定された手数で分岐する局面の数を数える。
         * @param depth 手数
         * @return 局面の数
         */
        inline search_count_t count_node(move_count_t depth) const;

        /**
         * @breif 棋譜ファイルから棋譜を読み込む。
         * @param kifu_file 棋譜ファイル
         */
        inline void read_kifu_file(std::filesystem::path kifu_file);

        /**
         * @breif 局面をSFEN表記法に準拠した文字列に変換する。
         * @return SFEN表記法に準拠した文字列
         */
        inline std::string sfen_string() const;

        board_t board;                                          // 盤
        captured_pieces_t captured_pieces_list[color_size];     // 持ち駒
        move_count_t move_count = 0;                            // 手数
        color_t m_color = black;                                // 手番
        std::vector<move_t> kifu;                               // 棋譜
        additional_info_t additional_info;                      // 追加情報
    };

    /**
     * @breif コピーコンストラクトされてからデストラクトされるまでに局面が変更されていないことを検証する。
     */
    class kyokumen_rollback_validator_t
    {
    public:
        inline kyokumen_rollback_validator_t(const kyokumen_t & kyokumen);
        inline ~kyokumen_rollback_validator_t();

    private:
        const kyokumen_t & kyokumen;
        piece_t data[pos_size];
        captured_pieces_t mochigoma_list[color_size];
    };

    inline kyokumen_rollback_validator_t::kyokumen_rollback_validator_t(const kyokumen_t & kyokumen)
        : kyokumen{ kyokumen }
    {
        std::copy(std::begin(kyokumen.board.data), std::end(kyokumen.board.data), std::begin(data));
        for (const color_t color : colors)
            std::copy(std::begin(kyokumen.captured_pieces_list), std::end(kyokumen.captured_pieces_list), std::begin(mochigoma_list));
    }

    inline kyokumen_rollback_validator_t::~kyokumen_rollback_validator_t()
    {
        for (std::size_t i = 0; i < std::size(data); ++i)
            SHOGIPP_ASSERT(data[i] == kyokumen.board.data[i]);
        for (const color_t color : colors)
            for (piece_t piece = fu; piece <= hi; ++piece)
                SHOGIPP_ASSERT(mochigoma_list[color][piece] == kyokumen.captured_pieces_list[color][piece]);
    }

    inline kyokumen_t::kyokumen_t()
    {
        push_additional_info();
    }

    inline kyokumen_t::kyokumen_t(std::string_view position)
    {
        kyokumen_t temp;
        bool promoted = false;

        std::vector<std::string> tokens;
        details::split_tokens(std::back_inserter(tokens), position);

        auto current_token = tokens.begin();

        constexpr std::string_view startpos = "startpos";
        if (current_token == tokens.end())
            throw invalid_usi_input{ "unexpected sfen end 1" };

        bool is_startpos = *current_token == startpos;
        if (is_startpos)
        {
            ++current_token;
        }
        else
        {
            constexpr std::string_view sfen = "sfen";
            if (*current_token != sfen)
                throw invalid_usi_input{ "sfen not found" };

            ++current_token;
            if (current_token == tokens.end())
                throw invalid_usi_input{ "unexpected sfen end 2" };

            temp.board.clear();
            pos_t dan = 0, suji = 0;

            const std::string_view sfen_string = *current_token;
            for (const char c : *current_token)
            {
                if (c == '+')
                {
                    promoted = true;
                }
                else if (c == '/')
                {
                    if (suji != suji_size)
                        throw invalid_usi_input{ "unexpected '/'" };
                    ++dan;
                    suji = 0;
                }
                else if (c >= '1' && c <= '9')
                {
                    suji += static_cast<pos_t>(c - '0');
                }
                else
                {
                    const std::optional<piece_t> optional_piece = char_to_piece(c);
                    if (!optional_piece)
                        throw invalid_usi_input{ "unexpected character 1" };
                    piece_t piece = *optional_piece;
                    if (promoted)
                        piece = to_promoted(piece);
                    temp.board[suji_dan_to_pos(suji, dan)] = piece;
                    ++suji;
                }
            }
            temp.clear_additional_info();
            temp.push_additional_info();

            ++current_token;
        }

        if (current_token != tokens.end())
        {

        }

        while (current_token != tokens.end())
        {
            if (*current_token == "w")
            {
                ++current_token;
                /* unused */;
            }
            else if (*current_token == "b")
            {
                ++current_token;
                /* unused */;
            }
            else if (*current_token == "moves")
            {
                ++current_token;
                while (current_token != tokens.end())
                {
                    const move_t move{ *current_token, temp.board };
                    if (!move.put() && to_color(move.source_piece()) != temp.color())
                        throw invalid_usi_input{ "invalid source color" };
                    temp.do_move(move);
                    ++current_token;
                }
            }
            else if (std::all_of(current_token->begin(), current_token->end(), [](char c) -> bool { return std::isdigit(c); }))
            {
                /* unused */;
                ++current_token;
            }
            else
            {
                if (*current_token == "-")
                {
                    ++current_token;
                    /* unused */;
                }
                else
                {
                    captured_pieces_t::size_type count = 1;
                    for (auto iter = current_token->begin(); iter != current_token->end(); ++iter)
                    {
                        if (*iter >= '0' && *iter <= '9')
                        {
                            count = 0;
                            for (; iter != current_token->end() && *iter >= '0' && *iter <= '9'; ++iter)
                                count = static_cast<captured_pieces_t::size_type>(count * 10 + *iter - '0');
                        }
                        else
                        {
                            std::optional<piece_t> optional_piece = char_to_piece(*iter);
                            if (!optional_piece)
                                throw invalid_usi_input{ "unexpected character 2" };
                            temp.captured_pieces_list[to_color(*optional_piece)][trim_color(*optional_piece)] = count;
                            count = 1;
                        }
                    }
                    ++current_token;
                }
            }
        }

        *this = std::move(temp);
    }

    inline bool kyokumen_t::promotable(piece_t piece, pos_t source, pos_t destination)
    {
        if (!is_promotable(piece))
            return false;
        if (to_color(piece) == black)
            return source < width * (3 + padding_height) || destination < width * (3 + padding_height);
        return source >= width * (6 + padding_height) || destination >= width * (6 + padding_height);
    }

    inline bool kyokumen_t::must_promote(piece_t piece, pos_t destination)
    {
        if (trim_color(piece) == fu || trim_color(piece) == kyo)
        {
            if (to_color(piece) == black)
                return destination < (width * (1 + padding_height));
            return destination >= width * (8 + padding_height);
        }
        else if (trim_color(piece) == kei)
        {
            if (to_color(piece) == black)
                return destination < width * (2 + padding_height);
            return destination >= width * (7 + padding_height);
        }
        return false;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_far_destination(OutputIterator result, pos_t source, pos_t offset) const
    {
        for (pos_t current = source + offset; !board_t::out(current); current += offset)
        {
            if (board[current] == empty)
                *result++ = current;
            else
            {
                if (to_color(board[source]) == to_color(board[current])) break;
                *result++ = current;
                if (to_color(board[source]) != to_color(board[current])) break;
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_near_destination(OutputIterator result, pos_t source, pos_t offset) const
    {
        const pos_t current = source + offset;
        if (!board_t::out(current) && (board[current] == empty || to_color(board[current]) != to_color(board[source])))
            *result++ = current;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_destination(OutputIterator result, pos_t source, color_t color) const
    {
        const piece_t piece = trim_color(board[source]);
        for (const pos_t * offset = far_move_offsets(piece); *offset; ++offset)
            search_far_destination(result, source, *offset * reverse(color));
        for (const pos_t * offset = near_move_offsets(piece); *offset; ++offset)
            search_near_destination(result, source, *offset * reverse(color));
    }

    inline bool kyokumen_t::puttable(piece_t piece, pos_t destination) const
    {
        if (board[destination] != empty)
            return false;
        if (color() == black)
        {
            if ((piece == fu || piece == kyo) && destination < width * (padding_height + 1))
                return false;
            if (piece == kei && destination < width * (padding_height + 2))
                return false;
        }
        else
        {
            if ((piece == fu || piece == kyo) && destination >= width * (padding_height + 8))
                return false;
            if (piece == kei && destination >= width * (padding_height + 7))
                return false;
        }
        if (piece == fu)
        {
            const pos_t suji = pos_to_suji(destination);

            // 二歩
            for (pos_t dan = 0; dan < dan_size; ++dan)
            {
                piece_t current = board[suji_dan_to_pos(suji, dan)];
                if (current != empty && trim_color(current) == fu && color() == to_color(current))
                    return false;
            }

            // 打ち歩詰め
            const pos_t pos = destination + front * (reverse(color()));
            if (!board_t::out(pos) && board[pos] != empty && trim_color(board[pos]) == ou && to_color(board[pos]) != color())
            {
                move_t move{ destination, piece };
                moves_t moves;
                {
                    VALIDATE_KYOKUMEN_ROLLBACK(*this);
                    const_cast<kyokumen_t &>(*this).do_move(move);
                    search_moves(std::back_inserter(moves));
                    const_cast<kyokumen_t &>(*this).undo_move(move);
                }
                if (moves.empty())
                    return false;
            }
        }
        return true;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_source(OutputIterator result, color_t color) const
    {
        for (pos_t pos = 0; pos < pos_size; ++pos)
            if (!board_t::out(pos) && board[pos] != empty && to_color(board[pos]) == color)
                *result++ = pos;
    }

    inline pos_t kyokumen_t::search(pos_t pos, pos_t offset) const
    {
        pos_t current;
        for (current = pos + offset; !board_t::out(current) && board[current] == empty; current += offset);
        if (board_t::out(current))
            return npos;
        return current;
    }
    
    template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_piece_near(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const
    {
        if (pos_t current = pos + offset; !board_t::out(current) && board[current] != empty)
            if (is_collected(to_color(board[current])) && std::find(first, last, trim_color(board[current])) != last)
                *result++ = transform(current, offset, false);
    }

    template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_piece_far(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const
    {
        if (pos_t found = search(pos, offset); found != npos && found != pos + offset && board[found] != empty)
            if (is_collected(to_color(board[found])) && std::find(first, last, trim_color(board[found])) != last)
                *result++ = transform(found, offset, true);
    }

    template<typename OutputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_piece(OutputIterator result, pos_t pos, color_t color, IsCollected is_collected, Transform transform) const
    {
        for (const auto & [offset, candidates] : near_kiki_list)
            search_piece_near(result, pos, offset * reverse(color), candidates.begin(), candidates.end(), is_collected, transform);
        for (const auto & [offset, candidates] : far_kiki_list_synmmetric)
            search_piece_far(result, pos, offset, candidates.begin(), candidates.end(), is_collected, transform);
        for (const auto & [offset, candidates] : far_kiki_list_asynmmetric)
            search_piece_far(result, pos, offset * reverse(color), candidates.begin(), candidates.end(), is_collected, transform);
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_himo(OutputIterator result, pos_t pos, color_t color) const
    {
        search_piece(result, pos, color,
            [color](color_t g) { return g == color; },
            [](pos_t pos, pos_t offset, bool aigoma) -> pos_t { return pos; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_kiki(OutputIterator result, pos_t pos, color_t color) const
    {
        search_piece(result, pos, color,
            [color](color_t g) { return g != color; },
            [](pos_t pos, pos_t offset, bool aigoma) -> kiki_t { return { pos, offset, aigoma }; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_kiki_or_himo(OutputIterator result, pos_t pos, color_t color) const
    {
        search_piece(result, pos, color,
            [](color_t) { return true; },
            [](pos_t pos, pos_t offset, bool aigoma) -> pos_t { return pos; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_check(OutputIterator result, color_t color) const
    {
        search_kiki(result, additional_info.king_pos_list[color], color);
    }

    std::vector<kiki_t> kyokumen_t::search_check(color_t color) const
    {
        std::vector<kiki_t> check_list;
        search_check(std::back_inserter(check_list), color);
        return check_list;
    }

    inline void kyokumen_t::push_additional_info()
    {
        push_additional_info(make_hash());
    }

    inline void kyokumen_t::push_additional_info(hash_t hash)
    {
        additional_info.check_list_stack.push_back(search_check(color()));
        additional_info.hash_stack.push_back(hash);
        if (kifu.empty())
        {
            for (pos_t pos = 0; pos < pos_size; ++pos)
                if (!board_t::out(board[pos]) && board[pos] != empty && trim_color(board[pos]) == ou)
                    additional_info.king_pos_list[to_color(board[pos])] = pos;
        }
    }

    inline void kyokumen_t::pop_additional_info()
    {
        SHOGIPP_ASSERT(!additional_info.check_list_stack.empty());
        SHOGIPP_ASSERT(!additional_info.hash_stack.empty());
        additional_info.check_list_stack.pop_back();
        additional_info.hash_stack.pop_back();
    }

    inline void kyokumen_t::clear_additional_info()
    {
        additional_info.check_list_stack.clear();
        additional_info.hash_stack.clear();
    }

    inline void kyokumen_t::search_aigoma(aigoma_info_t & aigoma_info, color_t color) const
    {
        using pair = std::pair<pos_t, std::vector<piece_t>>;
        static const std::vector<pair> table
        {
            { front      , { kyo, hi, ryu } },
            { left       , { hi, ryu } },
            { right      , { hi, ryu } },
            { back       , { hi, ryu } },
            { front_left , { kaku, uma } },
            { front_right, { kaku, uma } },
            { back_left  , { kaku, uma } },
            { back_right , { kaku, uma } },
        };

        const pos_t king_pos = additional_info.king_pos_list[color];
        for (const auto & [offset, hashirigoma_list] : table)
        {
            const pos_t reversed_offset = offset * reverse(color);
            const pos_t first = search(king_pos, reversed_offset);
            if (first != npos && to_color(board[first]) == color)
            {
                const pos_t second = search(first, reversed_offset);
                if (second != npos && to_color(board[second]) != color)
                {
                    const piece_t kind = trim_color(board[second]);
                    bool match = std::find(hashirigoma_list.begin(), hashirigoma_list.end(), kind) != hashirigoma_list.end();
                    if (match)
                    {
                        std::vector<pos_t> candidates;
                        for (pos_t candidate = second; candidate != king_pos; candidate -= reversed_offset)
                            candidates.push_back(candidate);
                        aigoma_info[first] = std::move(candidates);
                    }
                }
            }
        }
    }

    inline aigoma_info_t kyokumen_t::search_aigoma(color_t color) const
    {
        aigoma_info_t aigoma_info;
        search_aigoma(aigoma_info, color);
        return aigoma_info;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_from_positions(OutputIterator result, pos_t source, pos_t destination) const
    {
        if (promotable(board[source], source, destination))
            *result++ = { source, destination, board[source], board[destination], true };
        if (!must_promote(board[source], destination))
            *result++ = { source, destination, board[source], board[destination], false };
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_nonevasions(OutputIterator result) const
    {
        search_moves_moves(result);
        search_moves_puts(result);
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_evasions_ou_move(OutputIterator result) const
    {
        const pos_t source = additional_info.king_pos_list[color()];
        for (const pos_t * p = near_move_offsets(ou); *p; ++p)
        {
            const pos_t destination = source + *p * reverse(color());
            if (!board_t::out(destination)
                && (board[destination] == empty || to_color(board[destination]) != color()))
            {
                const move_t move{ source, destination, board[source], board[destination], false };
                std::vector<kiki_t> kiki;
                {
                    VALIDATE_KYOKUMEN_ROLLBACK(*this);
                    kyokumen_t & nonconst_this = const_cast<kyokumen_t &>(*this);
                    piece_t captured = board[destination];
                    nonconst_this.board[destination] = board[source];
                    nonconst_this.board[source] = empty;
                    search_kiki(std::back_inserter(kiki), destination, color());
                    nonconst_this.board[source] = board[destination];
                    nonconst_this.board[destination] = captured;
                }
                if (kiki.empty())
                    *result++ = move;
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_evasions_aigoma(OutputIterator result) const
    {
        const aigoma_info_t aigoma_info = search_aigoma(color());
        const pos_t ou_pos = additional_info.king_pos_list[color()];
        
        SHOGIPP_ASSERT(move_count < additional_info.check_list_stack.size());
        const auto & check_list = additional_info.check_list_stack[move_count];
        if (check_list.size() == 1)
        {
            if (check_list.front().aigoma)
            {
                const pos_t offset = check_list.front().offset;
                for (pos_t destination = ou_pos + offset; !board_t::out(destination) && board[destination] == empty; destination += offset)
                {
                    // 駒を移動させる合駒
                    std::vector<kiki_t> kiki_list;
                    search_kiki(std::back_inserter(kiki_list), destination, !color());
                    for (const kiki_t & kiki : kiki_list)
                    {
                        // 王で合駒はできない。
                        if (trim_color(board[kiki.pos]) != ou)
                        {
                            // 既に合駒として使っている駒は移動できない。
                            const auto aigoma_iter = aigoma_info.find(kiki.pos);
                            const bool is_aigoma = aigoma_iter != aigoma_info.end();
                            if (!is_aigoma)
                                search_moves_from_positions(result, kiki.pos, destination);
                        }
                    }

                    // 駒を打つ合駒
                    for (piece_t piece = fu; piece <= hi; ++piece)
                        if (captured_pieces_list[color()][piece])
                            if (puttable(piece, destination))
                                *result++ = { destination, piece };
                }
            }

            // 王手している駒を取る手を検索する。
            const pos_t destination = check_list.front().pos;
            std::vector<kiki_t> kiki_list;
            search_kiki(std::back_inserter(kiki_list), destination, !color());
            for (const kiki_t & kiki : kiki_list)
            {
                // 王を動かす手は既に検索済み
                if (trim_color(board[kiki.pos]) != ou)
                {
                    // 既に合駒として使っている駒は移動できない。
                    const auto aigoma_iter = aigoma_info.find(kiki.pos);
                    const bool is_aigoma = aigoma_iter != aigoma_info.end();
                    if (!is_aigoma)
                        search_moves_from_positions(result, kiki.pos, destination);
                }
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_evasions(OutputIterator result) const
    {
        search_moves_evasions_ou_move(result);
        search_moves_evasions_aigoma(result);
    }

    /**
     * @breif 王手を外さない手のうち駒を動かす手を検索する。
     * @param result 合法手の出力イテレータ
     */
    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_moves(OutputIterator result) const
    {
        const aigoma_info_t aigoma_info = search_aigoma(color());
        std::vector<pos_t> source_list;
        source_list.reserve(pos_size);
        search_source(std::back_inserter(source_list), color());
        for (const pos_t source : source_list)
        {
            std::vector<pos_t> destination_list;
            destination_list.reserve(pos_size);
            search_destination(std::back_inserter(destination_list), source, color());
            const auto aigoma_iter = aigoma_info.find(source);
            const bool is_aigoma = aigoma_iter != aigoma_info.end();

            for (const pos_t destination : destination_list)
            {
#ifndef NDEBUG
                if (board[destination] != empty && trim_color(board[destination]) == ou)
                {
                    board.print();
                    std::cout << pos_to_string(source) << std::endl;
                    const move_t move{ source, destination, board[source], board[destination], false };
                    print_move(move, color());
                    std::cout << std::endl;
                    print_kifu();
                    SHOGIPP_ASSERT(false);
                }
#endif

                // 合駒は利きの範囲にしか移動できない。
                if (is_aigoma)
                {
                    const std::vector<pos_t> & candidates = aigoma_iter->second;
                    if (std::find(candidates.begin(), candidates.end(), destination) == candidates.end())
                        continue;
                }

                // 利いている場所に王を移動させてはならない
                if (trim_color(board[source]) == ou)
                {
                    std::vector<kiki_t> kiki_list;
                    search_kiki(std::back_inserter(kiki_list), destination, color());
                    if (kiki_list.size() > 0)
                        continue;
                }

                search_moves_from_positions(result, source, destination);
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_puts(OutputIterator result) const
    {
        for (piece_t piece = fu; piece <= hi; ++piece)
            if (captured_pieces_list[color()][piece])
                for (pos_t destination = 0; destination < pos_size; ++destination)
                    if (puttable(piece, destination))
                        *result++ = { destination, piece };
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves(OutputIterator result) const
    {
        SHOGIPP_ASSERT(move_count < additional_info.check_list_stack.size());
        auto & check_list = additional_info.check_list_stack[move_count];
        if (check_list.empty())
            search_moves_nonevasions(result);
        else
            search_moves_evasions(result);
    }

    inline moves_t kyokumen_t::search_moves() const
    {
        moves_t moves;
        search_moves(std::back_inserter(moves));
        return moves;
    }

    inline hash_t kyokumen_t::make_hash() const
    {
        hash_t hash{};

        // 盤上の駒のハッシュ値をXOR演算
        for (pos_t pos = 0; pos < pos_size; ++pos)
            if (!board_t::out(pos))
                if (piece_t piece = board[pos]; piece != empty)
                    hash ^= hash_table.piece_hash(piece, pos);

        // 持ち駒のハッシュ値をXOR演算
        for (const color_t color : colors)
            for (piece_t piece = fu; piece <= hi; ++piece)
                hash ^= hash_table.captured_piece_hash(piece, captured_pieces_list[color][piece], color);

        // 手番のハッシュ値をXOR演算
        hash ^= hash_table.color_hash(color());
        hash ^= hash_table.color_hash(!color());

        return hash;
    }

    inline hash_t kyokumen_t::make_hash(hash_t hash, const move_t & move) const
    {
        if (move.put())
        {
            std::size_t mochigoma_count = captured_pieces_list[color()][move.source_piece()];
            SHOGIPP_ASSERT(mochigoma_count > 0);
            hash ^= hash_table.piece_hash(move.source_piece(), move.destination());
            hash ^= hash_table.captured_piece_hash(move.source_piece(), mochigoma_count, color());
            hash ^= hash_table.captured_piece_hash(move.source_piece(), mochigoma_count - 1, color());
        }
        else
        {
            SHOGIPP_ASSERT(!(!is_promotable(move.source_piece()) && move.promote()));
            hash ^= hash_table.piece_hash(move.source_piece(), move.source());
            if (move.captured_piece() != empty)
            {
                std::size_t mochigoma_count = captured_pieces_list[color()][move.captured_piece()];
                hash ^= hash_table.captured_piece_hash(to_mochigoma(move.captured_piece()), mochigoma_count, color());
                hash ^= hash_table.captured_piece_hash(to_mochigoma(move.captured_piece()), mochigoma_count + 1, color());
                hash ^= hash_table.piece_hash(move.captured_piece(), move.destination());
            }
            hash ^= hash_table.piece_hash(move.promote() ? to_unpromoted(move.source_piece()) : move.source_piece(), move.destination());
        }
        hash ^= hash_table.color_hash(!color());
        hash ^= hash_table.color_hash(color());
        return hash;
    }

    inline void kyokumen_t::print_move(const move_t & move, color_t color) const
    {
        std::cout << (color == black ? "▲" : "△");
        if (move.put())
        {
            std::cout << suji_to_string(pos_to_suji(move.destination())) << dan_to_string(pos_to_dan(move.destination())) << piece_to_string(trim_color(move.source_piece())) << "打";
        }
        else
        {
            const char * promotion_string;
            if (promotable(move.source_piece(), move.source(), move.destination()))
            {
                if (move.promote())
                    promotion_string = "成";
                else
                    promotion_string = "不成";
            }
            else
                promotion_string = "";
            std::cout
                << pos_to_string(move.destination()) << piece_to_string(trim_color(move.source_piece())) << promotion_string
                << "（" << pos_to_string(move.source()) << "）";
        }
    }

    template<typename InputIterator>
    inline void kyokumen_t::print_move(InputIterator first, InputIterator last) const
    {
        for (std::size_t i = 0; first != last; ++i)
        {
            std::printf("#%3zd ", i + 1);
            print_move(*first++, color());
            std::cout << std::endl;
        }
    }

    inline void kyokumen_t::print_move() const
    {
        moves_t move;
        kyokumen_t temp = *this;
        temp.search_moves(std::back_inserter(move));
        print_move(move.begin(), move.end());
    }

    inline void kyokumen_t::print_check() const
    {
        SHOGIPP_ASSERT(move_count < additional_info.check_list_stack.size());
        auto & check_list = additional_info.check_list_stack[move_count];
        if (!check_list.empty())
        {
            std::cout << "王手：";
            for (std::size_t i = 0; i < check_list.size(); ++i)
            {
                const kiki_t & kiki = check_list[i];
                if (i > 0)
                    std::cout << "　";
                print_pos(kiki.pos);
                std::cout << piece_to_string(trim_color(board[kiki.pos])) << std::endl;
            }
        }
    }

    inline void kyokumen_t::print() const
    {
        std::cout << "後手持ち駒：";
        captured_pieces_list[white].print();
        board.print();
        std::cout << "先手持ち駒：";
        captured_pieces_list[black].print();
    }

    inline void kyokumen_t::print_kifu() const
    {
        for (move_count_t i = 0; i < static_cast<move_count_t>(kifu.size()); ++i)
        {
            const move_count_t diff = static_cast<move_count_t>(kifu.size() - i);
            print_move(kifu[i], diff % color_size == 0 ? color() : !color());
            std::cout << std::endl;
        }
    }

    inline hash_t kyokumen_t::hash() const
    {
        SHOGIPP_ASSERT(move_count < additional_info.hash_stack.size());
        return additional_info.hash_stack[move_count];
    }

    inline void kyokumen_t::validate_board_out()
    {
        for (pos_t pos = 0; pos < pos_size; ++pos)
            if (board_t::out(pos))
                SHOGIPP_ASSERT(board[pos] == out_of_range);
    }

    inline void kyokumen_t::do_move(const move_t & move)
    {
        hash_t hash = make_hash(this->hash(), move);
        if (move.put())
        {
            SHOGIPP_ASSERT(captured_pieces_list[color()][move.source_piece()] > 0);
            board[move.destination()] = color() == white ? to_gote(move.source_piece()) : move.source_piece();
            --captured_pieces_list[color()][move.source_piece()];
        }
        else
        {
            SHOGIPP_ASSERT(!(!is_promotable(move.source_piece()) && move.promote()));
            if (board[move.destination()] != empty)
                ++captured_pieces_list[color()][board[move.destination()]];
            board[move.destination()] = move.promote() ? to_promoted(board[move.source()]) : board[move.source()];
            board[move.source()] = empty;
            if (trim_color(move.source_piece()) == ou)
                additional_info.king_pos_list[color()] = move.destination();
        }
        ++move_count;
        m_color = !m_color;
        kifu.push_back(move);
        push_additional_info(hash);
        validate_board_out();
    }

    inline void kyokumen_t::undo_move(const move_t & move)
    {
        SHOGIPP_ASSERT(move_count > 0);
        --move_count;
        m_color = !m_color;
        if (move.put())
        {
            ++captured_pieces_list[color()][move.source_piece()];
            board[move.destination()] = empty;
        }
        else
        {
            if (trim_color(move.source_piece()) == ou)
                additional_info.king_pos_list[color()] = move.source();
            board[move.source()] = move.source_piece();
            board[move.destination()] = move.captured_piece();
            if (move.captured_piece() != empty)
                --captured_pieces_list[color()][move.captured_piece()];
        }
        kifu.pop_back();
        pop_additional_info();
    }

    inline color_t kyokumen_t::color() const
    {
        return m_color;
    }

    inline search_count_t kyokumen_t::count_node(move_count_t depth) const
    {
        if (depth == 0)
            return 1;

        search_count_t count = 0;
        for (const move_t & move : search_moves())
        {
            VALIDATE_KYOKUMEN_ROLLBACK(*this);
            const_cast<kyokumen_t &>(*this).do_move(move);
            count += count_node(depth - 1);
            const_cast<kyokumen_t &>(*this).undo_move(move);
        }
        return count;
    }

    inline void kyokumen_t::read_kifu_file(std::filesystem::path kifu_file)
    {
        static constexpr std::string_view source_position_prefix = "（";
        static constexpr std::string_view source_position_suffix = "）";
        static constexpr std::string_view uchite_string = "打";
        static constexpr std::string_view promote_string = "成";
        static constexpr std::string_view nonpromote_string = "不成";

        std::ifstream stream{ kifu_file };
        std::string line;
        
        kyokumen_t temp_kyokumen;

        try
        {
            while (std::getline(stream, line))
            {
                if (line.empty() || (!line.empty() && line.front() == '#'))
                    continue;

                std::string_view rest = line;
                
                std::optional<color_t> color = parse(rest, color_mark_map, color_mark_size);
                if (!color)
                    throw file_format_error{ "read_kifu_file 1" };
                
                std::optional<pos_t> destination_suji = parse(rest, suji_string_map, suji_string_size);
                if (!destination_suji)
                    throw file_format_error{ "read_kifu_file 2" };

                std::optional<pos_t> destination_dan = parse(rest, dan_string_map, dan_string_size);
                if (!destination_dan)
                    throw file_format_error{ "read_kifu_file 3" };

                std::optional<pos_t> destination = suji_dan_to_pos(*destination_suji, *destination_dan);
                if (!destination)
                    throw file_format_error{ "read_kifu_file 4" };

                std::optional<piece_t> piece = parse(rest, piece_string_map, piece_string_size);
                if (!piece)
                    throw file_format_error{ "read_kifu_file 5" };

                if (*color != temp_kyokumen.color())
                    throw file_format_error{ "read_kifu_file 6" };

                bool promote = false;
                if (rest.size() >= uchite_string.size()
                    && rest.substr(0, uchite_string.size()) == uchite_string)
                {
                    rest.remove_prefix(uchite_string.size());
                    if (*piece < fu || *piece > hi)
                        throw file_format_error{ "read_kifu_file 7" };
                    if (!rest.empty())
                        throw file_format_error{ "read_kifu_file 8" };
                    if (temp_kyokumen.captured_pieces_list[temp_kyokumen.color()][*piece] == 0)
                    {
                        temp_kyokumen.captured_pieces_list[black].print();
                        throw file_format_error{ "read_kifu_file 9" };
                    }
                    move_t move{ *destination, *piece };
                    temp_kyokumen.do_move(move);
                    continue;
                }
                else if (rest.size() >= promote_string.size()
                    && rest.substr(0, promote_string.size()) == promote_string)
                {
                    promote = true;
                    rest.remove_prefix(promote_string.size());
                }
                else if (rest.size() >= nonpromote_string.size()
                    && rest.substr(0, nonpromote_string.size()) == nonpromote_string)
                {
                    rest.remove_prefix(nonpromote_string.size());
                }

                if (rest.size() >= source_position_prefix.size()
                    && rest.substr(0, source_position_prefix.size()) == source_position_prefix)
                {
                    rest.remove_prefix(source_position_prefix.size());
                    std::optional<pos_t> source_suji = parse(rest, suji_string_map, suji_string_size);
                    std::optional<pos_t> source_dan = parse(rest, dan_string_map, dan_string_size);
                    pos_t source = suji_dan_to_pos(*source_suji, *source_dan);
                    if (rest.size() >= source_position_suffix.size()
                        && rest.substr(0, source_position_suffix.size()) == source_position_suffix)
                    {
                        rest.remove_prefix(source_position_suffix.size());
                        if (!rest.empty())
                            throw file_format_error{ "read_kifu_file 10" };
                        if (temp_kyokumen.board[source] == empty)
                            throw file_format_error{ "read_kifu_file 11" };
                        if (to_color(temp_kyokumen.board[source]) != temp_kyokumen.color())
                            throw file_format_error{ "read_kifu_file 12" };
                        move_t move{ source, *destination, temp_kyokumen.board[source], temp_kyokumen.board[*destination], promote };
                        temp_kyokumen.do_move(move);
                        continue;
                    }
                }

                throw file_format_error{ "read_kifu_file 10" };
            }

            std::swap(*this, temp_kyokumen);
        }
        catch (const parse_error & e)
        {
            throw file_format_error{ e.what() };
        }
        catch (const file_format_error &)
        {
            throw;
        }
    }

    inline std::string kyokumen_t::sfen_string() const
    {
        std::string result;
        result += "sfen ";
        result += board.sfen_string();
        result += ' ';
        result += color_to_color_char(color());
        result += ' ';
        result += std::to_string(move_count + 1);
        if (!kifu.empty())
        {
            result += " moves";
            for (const move_t & move : kifu)
            {
                result += ' ';
                result += move.sfen_string();
            }
        }
        return result;
    }

    using evaluation_value_t = int;

    template<typename Key, typename Value, typename Hash = std::hash<Key>>
    class lru_cache_t
    {
    public:
        using key_type = Key;
        using value_type = Value;
        using pair_type = std::pair<key_type, value_type>;
        using list_type = std::list<pair_type>;
        using hash_type = Hash;
        using unordered_map_type = std::unordered_map<key_type, typename list_type::iterator, hash_type>;

        /**
         * @breif LRUで管理されるキャッシュを構築する。
         * @param capacity 最大要素数
         */
        inline lru_cache_t(std::size_t capacity)
            : capacity{ capacity }
        {
        }

        inline lru_cache_t(const lru_cache_t &) = default;
        inline lru_cache_t(lru_cache_t &&) = default;
        inline lru_cache_t & operator =(const lru_cache_t &) = default;
        inline lru_cache_t & operator =(lru_cache_t &&) = default;

        /**
         * @breif キャッシュを破棄する。
         */
        inline void clear()
        {
            list.clear();
            umap.clear();
        }

        /**
         * @breif キーと対応する値を取得する。
         * @param key キー
         * @return キーと対応する値
         */
        inline std::optional<Value> get(key_type key)
        {
            typename unordered_map_type::iterator iter = umap.find(key);
            if (iter == umap.end())
                return std::nullopt;
            value_type value = iter->second->second;
            list.erase(iter->second);
            list.emplace_front(key, value);
            umap[key] = list.begin();
            return value;
        }

        /**
         * @breif キーと値を登録する。
         * @param key キー
         * @param value 値
         */
        inline void push(key_type key, value_type value)
        {
            typename unordered_map_type::iterator iter = umap.find(key);
            if (iter != umap.end())
                list.erase(iter->second);
            list.emplace_front(key, value);
            umap[key] = list.begin();
            if (list.size() > capacity)
            {
                umap.erase(list.rbegin()->first);
                list.pop_back();
            }
        }

    private:
        list_type list;
        unordered_map_type umap;
        std::size_t capacity;
    };

#ifdef SIZE_OF_HASH
    using cache_t = lru_cache_t<hash_t, evaluation_value_t, basic_hash_hash_t<SIZE_OF_HASH>>;
#else
    using evaluation_value_cache_t = lru_cache_t<hash_t, evaluation_value_t>;
#endif

    /**
     * @breif 評価関数オブジェクトのインターフェース
     */
    class abstract_evaluator_t
    {
    public:
        virtual ~abstract_evaluator_t() {}

        /**
         * @breif 局面に対して合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
         */
        virtual move_t best_move(kyokumen_t & kyokumen) = 0;

        /**
         * @breif 評価関数オブジェクトの名前を返す。
         * @return 評価関数オブジェクトの名前
         */
        virtual const char * name() = 0;
    };

    /**
     * @breif 標準入力により合法手を選択する評価関数オブジェクト
     */
    class command_line_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        move_t best_move(kyokumen_t & kyokumen)
        {
            bool selected = false;

            unsigned int id;
            moves_t moves;
            kyokumen.search_moves(std::back_inserter(moves));
            
            while (!selected)
            {
                try
                {
                    std::cout << "#";
                    std::cout.flush();
                    std::cin >> id;
                    if (id == 0)
                        throw invalid_command_line_input{ "invalid command line input" };
                    if (id > moves.size())
                        throw invalid_command_line_input{ "invalid command line input" };
                    --id;
                    selected = true;
                }
                catch (const std::exception & e)
                {
                    std::cerr << e.what() << std::endl;
                    std::cin.clear();
                    std::cin.ignore();
                }
            }
            return moves[id];
        }

        const char * name()
        {
            return "command_line_evaluator";
        }
    };

    void print_help()
    {
        std::cout
            << ""
            ;
    }

    template<typename Callback>
    void parse_program_options(int argc, const char ** argv, Callback && callback)
    {
        std::map<std::string, std::vector<std::string>> params_map;
        constexpr char quotations[] = { '\'', '"' };

        std::string current_option;
        for (int i = 0; i < argc; ++i)
        {
            unsigned int count = 0;
            if (argv[i][0] == '-')
            {
                if (argv[i][1] == '-')
                {
                    const char * p = argv[i] + 2;
                    const char * begin = p;
                    while (*p && !std::isspace(*p))
                        ++p;
                    params_map[std::string{ begin, p }];
                    current_option = std::string(begin, p);
                }
                else
                {
                    const char * p = argv[i] + 1;
                    char option = *p;
                    while (std::isalpha(*p))
                    {
                        params_map[std::string{ *p }];
                        ++p;
                        ++count;
                    }
                    if (count == 1)
                        current_option = option;
                    else
                        current_option.clear();
                }
            }
            else if (!current_option.empty())
            {
                const char * p = argv[i];
                while (std::isspace(*p))
                    ++p;
                while (*p)
                {
                    while (std::isspace(*p))
                        ++p;
                    if (!*p)
                        break;
                    auto qiter = std::find(std::begin(quotations), std::end(quotations), *p);
                    if (qiter != std::end(quotations))
                    {
                        const char * begin = p;
                        char quotation = *qiter;
                        while (*p && *p != quotation)
                            ++p;
                        if (*p)
                            ++p;
                        params_map[current_option].emplace_back(begin, p);
                    }
                    else
                    {
                        const char * begin = p;
                        while (*p && !std::isspace(*p))
                            ++p;
                        params_map[current_option].emplace_back(begin, p);
                    }
                }
            }
        }

        for (const auto & [option, params] : params_map)
            callback(option, params);
    }

    /**
     * @breif 駒と価値の連想配列から局面の点数を計算する。
     * @param kyokumen 局面
     * @param map []演算子により駒から価値を連想するオブジェクト
     * @return 局面の点数
     */
    template<typename MapPieceInt>
    inline evaluation_value_t kyokumen_map_evaluation_value(kyokumen_t & kyokumen, MapPieceInt & map)
    {
        evaluation_value_t evaluation_value = 0;

        for (pos_t pos = 0; pos < pos_size; ++pos)
        {
            piece_t piece = kyokumen.board[pos];
            if (!board_t::out(pos) && piece != empty)
                evaluation_value += map[trim_color(piece)] * reverse(to_color(piece));
        }

        for (const color_t color : colors)
            for (piece_t piece = fu; piece <= hi; ++piece)
                evaluation_value += map[piece] * kyokumen.captured_pieces_list[color][piece] * reverse(color);

        return evaluation_value;
    }

    using evaluated_moves = std::pair<const move_t *, evaluation_value_t>;

    /**
     * @breif 合法手を得点により並び替える。
     * @param first evaluated_moves の先頭を指すランダムアクセスイテレータ
     * @param last evaluated_moves の末尾を指すランダムアクセスイテレータ
     * @param color 先手か後手か
     * @details 評価値の符号を手番により変更するようにしたため、現在この関数を使用する予定はない。
     */
    template<typename RandomAccessIterator>
    void sort_moves_by_evaluation_value(RandomAccessIterator first, RandomAccessIterator last, color_t color)
    {
        using comparator = bool (const evaluated_moves & a, const evaluated_moves & b);
        comparator * const map[]{
            [](const evaluated_moves & a, const evaluated_moves & b) -> bool { return a.second > b.second; },
            [](const evaluated_moves & a, const evaluated_moves & b) -> bool { return a.second < b.second; }
        };
        std::sort(first, last, map[color]);
    }

    /**
     * @breif 合法手を区分により並び替える。
     * @param first evaluated_moves の先頭を指すランダムアクセスイテレータ
     * @param last evaluated_moves の末尾を指すランダムアクセスイテレータ
     * @details 駒取りが発生する手、駒取りが発生しない手、打つ手の順に並び替える。
     */
    template<typename RandomAccessIterator>
    void sort_moves_by_category(RandomAccessIterator first, RandomAccessIterator last)
    {
        std::sort(first, last, [](const move_t & a, const move_t & b) -> bool { return to_category(a) >= to_category(b); });
    }

    /**
     * @breif 合法手を得点により並び替える。
     * @param first scored_te の先頭を指すランダムアクセスイテレータ
     * @param last scored_te の末尾を指すランダムアクセスイテレータ
     */
    template<typename RandomAccessIterator>
    void sort_moves_by_evaluation_value(RandomAccessIterator first, RandomAccessIterator last)
    {
        std::sort(first, last, [](const evaluated_moves & a, const evaluated_moves & b) -> bool { return a.second > b.second; });
    }

    /**
     * @breif USIプロトコルでクライアントからサーバーに送信される情報を表現する。
     * @details このクラスのメンバ関数はスレッドセーフに実行される。
     *          このクラスのメンバ関数を参照する場合、 mutex メンバ変数を利用して排他制御すること。
     */
    class usi_info_t
    {
    public:
        enum class state_t : unsigned int
        {
            not_ready,
            searching,
            terminated,
            requested_to_stop
        };

        depth_t depth{};
        depth_t seldepth{};
        std::chrono::system_clock::time_point begin;
        search_count_t nodes{};
        std::vector<move_t> pv;
        // multipv
        evaluation_value_t cp{};
        move_count_t mate{};
        std::optional<move_t> currmove;
        std::optional<move_t> best_move;
        search_count_t cache_hit_count{};
        state_t state = state_t::not_ready;
        milli_second_time_t limit_time;
        std::map<std::string, std::string> options;
        bool ponder = false;

        mutable std::recursive_mutex mutex;

        /**
         * @breif begin と現在時間の差分をミリ秒単位で返す。
         * @return begin と現在時間の差分
         */
        inline milli_second_time_t time() const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            return static_cast<milli_second_time_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
        }

        /**
         * @breif 探索局面数全体のうちキャッシュを適用した探索局面数の占める割合を千分率で返す。
         * @return 探索局面数全体のうちキャッシュを適用した探索局面数の占める割合
         */
        inline search_count_t hashfull() const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            if (nodes == 0)
                return 0;
            return cache_hit_count * 1000 / nodes;
        }

        /**
         * @breif 1秒あたりの探索局面数を返す。
         * @return 1秒あたりの探索局面数
         */
        inline search_count_t nps() const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            milli_second_time_t milli_second_time = time();
            if (milli_second_time == 0)
                return 0;
            return nodes * 1000 / milli_second_time;
        }

        /**
         * @breif 即時的な出力を行う。
         */
        inline void ondemand_print() const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            std::cout << "info"
                //<< "pv " << pv
                //<< "multipv " << multipv
                //<< "score cp " << cp
                //<< "score mate " << mate
                ;

            if (currmove)
                std::cout << " currmove " << currmove->sfen_string();

            std::cout << std::endl;
        }

        /**
         * @breif 定期的な出力を行う。
         */
        inline void periodic_print() const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            std::cout << "info"
                << " depth " << depth
                << " seldepth " << seldepth
                << " time " << time()
                << " nodes " << nodes
                << " hashfull " << hashfull()
                << " nps " << nps()
                << std::endl;
        }

        /**
         * @breif 名前に関連付けられオプションの値を文字列として取得する。
         * @param name オプションの名前
         * @return std::optional<std::string>
         */
        inline std::optional<std::string> get_option(const std::string & name) const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            auto iter = options.find(name);
            if (iter != options.end())
                return iter->second;
            return std::nullopt;
        }

        /**
         * @breif 名前に関連付けられオプションの値を Result 型に変換して取得する。
         * @tparam Result 変換先の型
         * @param name オプションの名前
         * @return std::optional<Result>
         */
        template<typename Result>
        inline std::optional<Result> get_option_as(const std::string & name) const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            std::optional<std::string> opt_value = get_option(name);
            if (opt_value)
            {
                try
                {
                    std::istringstream stream{ *opt_value };
                    Result result;
                    stream >> std::boolalpha >> result;
                    return result;
                }
                catch (const std::exception &)
                {
                    ;
                }
            }
            return std::nullopt;
        }

        /**
         * @breif USI_Hash オプションの値に準拠した最大サイズを持つ cache_t を構築する。
         * @param usi_info usi_info_t のポインタ
         * @return USI_Hash オプションの値に準拠した最大サイズを持つ cache_t
         * @details usi_info が nullptr である場合、あるいは USI_Hash オプション が指定されていない場合、
         *          この関数は無制限の最大サイズを持つ cache_t を構築する。
         */
        inline static cache_t make_cache(usi_info_t * usi_info)
        {
            std::size_t cache_size = std::numeric_limits<std::size_t>::max();
            if (usi_info)
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                const std::optional<std::size_t> cache_mb_size = usi_info->get_option_as<std::size_t>("USI_Hash");
                if (cache_mb_size)
                    cache_size = *cache_mb_size * 1000 * 1000 / sizeof(hash_t);
            }
            return cache_t{ cache_size };
        }

        /**
         * @breif state に state_t::terminated を設定し、ponder == false であれば bestmove コマンドを出力する。
         */
        inline void terminate()
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            if (best_move && !ponder)
                std::cout << "bestmove " << best_move->sfen_string() << std::endl;
            state = state_t::terminated;
        }
    };

    /**
     * @breif 局面に対して評価値を返す機能を提供する。
     */
    class evaluatable_t
    {
    public:
        virtual ~evaluatable_t() {}

        /**
         * @breif 局面に対して評価値を返す。
         * @param kyokumen 局面
         * @return 局面の評価値
         */
        virtual evaluation_value_t evaluate(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif negamax で合法手を選択する評価関数オブジェクトの抽象クラス
     */
    class negamax_evaluator_t
        : public abstract_evaluator_t
        , public evaluatable_t
    {
    public:
        evaluation_value_t negamax(
            kyokumen_t & kyokumen,
            depth_t depth,
            cache_t & cache,
            std::optional<move_t> & candidate_move
        );

        move_t best_move(kyokumen_t & kyokumen) override;

        std::shared_ptr<usi_info_t> usi_info;

    private:
        depth_t max_depth = 3;
    };

    evaluation_value_t negamax_evaluator_t::negamax(
        kyokumen_t & kyokumen,
        depth_t depth,
        cache_t & cache,
        std::optional<move_t> & candidate_move
    )
    {
        if (usi_info)
        {
            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
            if (usi_info->state == usi_info_t::state_t::requested_to_stop)
                throw usi_stop_exception{ "requested to stop" };
            usi_info->depth = depth;
            usi_info->nodes += 1;
        }

        if (depth >= max_depth)
        {
            const std::optional<evaluation_value_t> cached_evaluation_value = cache.get(kyokumen.hash());
            if (cached_evaluation_value)
            {
                if (usi_info)
                {
                    std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                    ++usi_info->cache_hit_count;
                }
                return *cached_evaluation_value;
            }
            const evaluation_value_t evaluation_value = evaluate(kyokumen) * reverse(kyokumen.color());
            cache.push(kyokumen.hash(), evaluation_value);
            return evaluation_value;
        }

        moves_t moves;
        kyokumen.search_moves(std::back_inserter(moves));

        if (moves.empty())
            return -std::numeric_limits<evaluation_value_t>::max();

        std::vector<evaluated_moves> evaluated_moves;
        auto inserter = std::back_inserter(evaluated_moves);
        evaluation_value_t max_evaluation_value = -std::numeric_limits<evaluation_value_t>::max();

        for (const move_t & move : moves)
        {
            if (usi_info && depth == 0)
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->currmove = move;
                usi_info->ondemand_print();
            }

            std::optional<move_t> nested_candidate_move;
            evaluation_value_t evaluation_value;
            {
                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                evaluation_value = -negamax(kyokumen, depth + 1, cache, nested_candidate_move);
                kyokumen.undo_move(move);
            }
            *inserter++ = { &move, evaluation_value };

            if (usi_info && depth == 0 && evaluation_value > max_evaluation_value)
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->best_move = move;
            }
            max_evaluation_value = evaluation_value;
        }

        SHOGIPP_ASSERT(!moves.empty());
        sort_moves_by_evaluation_value(evaluated_moves.begin(), evaluated_moves.end());
        candidate_move = *evaluated_moves.front().first;
        return evaluated_moves.front().second;
    }

    move_t negamax_evaluator_t::best_move(kyokumen_t & kyokumen)
    {
        if (usi_info)
        {
            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
            usi_info->begin = std::chrono::system_clock::now();
            usi_info->state = usi_info_t::state_t::searching;
        }

        cache_t cache = usi_info_t::make_cache(usi_info.get());
        std::optional<move_t> candidate_move;
        evaluation_value_t evaluation_value;
        try
        {
            evaluation_value = negamax(kyokumen, 0, cache, candidate_move);
        }
        catch (const usi_stop_exception &)
        {
            ;
        }

        if (usi_info)
        {
            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
            details::timer.search_count() += usi_info->nodes;
            usi_info->terminate();
            SHOGIPP_ASSERT(candidate_move.has_value());
        }

        return *candidate_move;
    }

    /**
     * @breif alphabeta で合法手を選択する評価関数オブジェクトの抽象クラス
     */
    class alphabeta_evaluator_t
        : public abstract_evaluator_t
        , public evaluatable_t
    {
    public:
        evaluation_value_t alphabeta(
            kyokumen_t & kyokumen,
            depth_t depth,
            evaluation_value_t alpha,
            evaluation_value_t beta,
            cache_t & cache,
            std::optional<move_t> & candidate_move);

        move_t best_move(kyokumen_t & kyokumen) override;

        std::shared_ptr<usi_info_t> usi_info;

    private:
        depth_t max_depth = 3;
    };

    evaluation_value_t alphabeta_evaluator_t::alphabeta(
        kyokumen_t & kyokumen,
        depth_t depth,
        evaluation_value_t alpha,
        evaluation_value_t beta,
        cache_t & cache,
        std::optional<move_t> & candidate_move)
    {
        if (usi_info)
        {
            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
            if (usi_info->state == usi_info_t::state_t::requested_to_stop)
                throw usi_stop_exception{ "requested to stop" };
            usi_info->depth = depth;
            usi_info->nodes += 1;
        }
        
        if (depth >= max_depth)
        {
            const std::optional<evaluation_value_t> cached_evaluation_value = cache.get(kyokumen.hash());
            if (cached_evaluation_value)
            {
                if (usi_info)
                {
                    std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                    ++usi_info->cache_hit_count;
                }
                return *cached_evaluation_value;
            }
            const evaluation_value_t evaluation_value = evaluate(kyokumen) * reverse(kyokumen.color());
            cache.push(kyokumen.hash(), evaluation_value);
            return evaluation_value;
        }

        moves_t moves;
        kyokumen.search_moves(std::back_inserter(moves));
        sort_moves_by_category(moves.begin(), moves.end());

        if (moves.empty())
            return -std::numeric_limits<evaluation_value_t>::max();

        std::vector<evaluated_moves> evaluated_moves;
        auto inserter = std::back_inserter(evaluated_moves);
        evaluation_value_t max_evaluation_value = -std::numeric_limits<evaluation_value_t>::max();

        for (const move_t & move : moves)
        {
            if (usi_info && depth == 0)
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->currmove = move;
                usi_info->ondemand_print();
            }

            std::optional<move_t> nested_candidate_move;
            evaluation_value_t evaluation_value;
            {
                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                evaluation_value = -alphabeta(kyokumen, depth + 1, -beta, -alpha, cache, nested_candidate_move);
                kyokumen.undo_move(move);
            }
            *inserter++ = { &move, evaluation_value };

            if (usi_info && depth == 0 && evaluation_value > max_evaluation_value)
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->best_move = move;
            }
            max_evaluation_value = evaluation_value;

            alpha = std::max(alpha, evaluation_value);
            if (alpha >= beta)
                break;
        }

        SHOGIPP_ASSERT(!moves.empty());
        sort_moves_by_evaluation_value(evaluated_moves.begin(), evaluated_moves.end());
        candidate_move = *evaluated_moves.front().first;
        return evaluated_moves.front().second;
    }

    move_t alphabeta_evaluator_t::best_move(kyokumen_t & kyokumen)
    {
        if (usi_info)
        {
            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
            usi_info->begin = std::chrono::system_clock::now();
            usi_info->state = usi_info_t::state_t::searching;
        }

        cache_t cache = usi_info_t::make_cache(usi_info.get());
        std::optional<move_t> candidate_move;
        evaluation_value_t evaluation_value;
        try
        {
            evaluation_value = alphabeta(kyokumen, 0, -std::numeric_limits<evaluation_value_t>::max(), std::numeric_limits<evaluation_value_t>::max(), cache, candidate_move);
        }
        catch (const usi_stop_exception &)
        {
            ;
        }

        if (usi_info)
        {
            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
            details::timer.search_count() += usi_info->nodes;
            usi_info->terminate();
            SHOGIPP_ASSERT(candidate_move.has_value());
        }

        return *candidate_move;
    }

    /**
     * @breif alphabeta で合法手を選択する評価関数オブジェクトの抽象クラス
     * @details 前回駒取りが発生していた場合、探索を延長する。
     */
    class extendable_alphabeta_evaluator_t
        : public abstract_evaluator_t
        , public evaluatable_t
    {
    public:
        evaluation_value_t extendable_alphabeta(
            kyokumen_t & kyokumen,
            depth_t depth,
            evaluation_value_t alpha,
            evaluation_value_t beta,
            cache_t & cache,
            std::optional<move_t> & candidate_move,
            pos_t previous_destination);

        move_t best_move(kyokumen_t & kyokumen) override;

        std::shared_ptr<usi_info_t> usi_info;

    private:
        depth_t default_max_depth = 3;
    };

    evaluation_value_t extendable_alphabeta_evaluator_t::extendable_alphabeta(
        kyokumen_t & kyokumen,
        depth_t depth,
        evaluation_value_t alpha,
        evaluation_value_t beta,
        cache_t & cache,
        std::optional<move_t> & candidate_move,
        pos_t previous_destination)
    {
        if (usi_info)
        {
            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
            if (usi_info->state == usi_info_t::state_t::requested_to_stop)
                throw usi_stop_exception{ "requested to stop" };
            usi_info->depth = depth;
            usi_info->nodes += 1;
        }
        
        if (depth >= default_max_depth)
        {
            // 前回駒取りが発生していた場合、探索を延長する。
            if (previous_destination != npos)
            {
                std::vector<evaluated_moves> evaluated_moves;
                auto inserter = std::back_inserter(evaluated_moves);
                moves_t moves;
                kyokumen.search_moves(std::back_inserter(moves));
                for (const move_t & move : moves)
                {
                    if (!move.put() && move.destination() == previous_destination)
                    {
                        std::optional<move_t> nested_candidate_move;
                        evaluation_value_t evaluation_value;
                        {
                            VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                            kyokumen.do_move(move);
                            evaluation_value = -extendable_alphabeta(kyokumen, depth - 1, -beta, -alpha, cache, nested_candidate_move, previous_destination);
                            kyokumen.undo_move(move);
                        }
                        *inserter++ = { &move, evaluation_value };
                        alpha = std::max(alpha, evaluation_value);
                        if (alpha >= beta)
                            break;
                    }
                }
                if (!evaluated_moves.empty())
                {
                    sort_moves_by_evaluation_value(evaluated_moves.begin(), evaluated_moves.end());
                    candidate_move = *evaluated_moves.front().first;
                    return evaluated_moves.front().second;
                }
            }

            const std::optional<evaluation_value_t> cached_evaluation_value = cache.get(kyokumen.hash());
            if (cached_evaluation_value)
            {
                if (usi_info)
                {
                    std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                    ++usi_info->cache_hit_count;
                }
                return *cached_evaluation_value;
            }
            const evaluation_value_t evaluation_value = evaluate(kyokumen) * reverse(kyokumen.color());
            cache.push(kyokumen.hash(), evaluation_value);
            return evaluation_value;
        }

        moves_t moves;
        kyokumen.search_moves(std::back_inserter(moves));
        sort_moves_by_category(moves.begin(), moves.end());

        if (moves.empty())
            return -std::numeric_limits<evaluation_value_t>::max();

        std::vector<evaluated_moves> evaluated_moves;
        auto inserter = std::back_inserter(evaluated_moves);
        evaluation_value_t max_evaluation_value = -std::numeric_limits<evaluation_value_t>::max();

        for (const move_t & move : moves)
        {
            if (usi_info && depth == 0)
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->currmove = move;
                usi_info->ondemand_print();
            }

            std::optional<move_t> nested_candidate_move;
            pos_t destination = (!move.put() && move.captured_piece() != empty) ? move.destination() : npos;
            evaluation_value_t evaluation_value;
            {
                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                evaluation_value = -extendable_alphabeta(kyokumen, depth + 1, -beta, -alpha, cache, nested_candidate_move, destination);
                kyokumen.undo_move(move);
            }
            *inserter++ = { &move, evaluation_value };

            if (usi_info && depth == 0 && evaluation_value > max_evaluation_value)
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->best_move = move;
            }
            max_evaluation_value = evaluation_value;

            alpha = std::max(alpha, evaluation_value);
            if (alpha >= beta)
                break;
        }

        SHOGIPP_ASSERT(!moves.empty());
        sort_moves_by_evaluation_value(evaluated_moves.begin(), evaluated_moves.end());
        candidate_move = *evaluated_moves.front().first;
        return evaluated_moves.front().second;
    }

    move_t extendable_alphabeta_evaluator_t::best_move(kyokumen_t & kyokumen)
    {
        if (usi_info)
        {
            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
            usi_info->begin = std::chrono::system_clock::now();
            usi_info->state = usi_info_t::state_t::searching;
        }

        cache_t cache = usi_info_t::make_cache(usi_info.get());
        std::optional<move_t> candidate_move;
        evaluation_value_t evaluation_value;
        try
        {
            evaluation_value = extendable_alphabeta(kyokumen, 0, -std::numeric_limits<evaluation_value_t>::max(), std::numeric_limits<evaluation_value_t>::max(), cache, candidate_move, npos);
        }
        catch (const usi_stop_exception &)
        {
            ;
        }

        if (usi_info)
        {
            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
            details::timer.search_count() += usi_info->nodes;
            usi_info->terminate();
            SHOGIPP_ASSERT(candidate_move.has_value());
        }

        return *candidate_move;
    }

    /**
     * @breif 単純に評価値が最も高い手を返す評価関数オブジェクトの抽象クラス
     */
    class max_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        /**
         * @breif 局面に対して評価値が最も高くなる合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
         */
        move_t best_move(kyokumen_t & kyokumen) override
        {
            moves_t moves;
            kyokumen.search_moves(std::back_inserter(moves));

            std::vector<evaluated_moves> scores;
            auto back_inserter = std::back_inserter(scores);
            for (const move_t & t : moves)
            {
                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                kyokumen.do_move(t);
                *back_inserter++ = { &t, evaluate(kyokumen) };
                kyokumen.undo_move(t);
            }

            std::sort(scores.begin(), scores.end(), [](auto & a, auto & b) { return a.second > b.second; });
            return *scores.front().first;
        }

        /**
         * @breif 局面に対して評価値を返す。
         * @param kyokumen 局面
         * @return 局面の評価値
         */
        virtual evaluation_value_t evaluate(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif 評価関数オブジェクトの実装例
     */
    class sample_evaluator_t
        : public alphabeta_evaluator_t
    {
    public:
        evaluation_value_t evaluate(kyokumen_t & kyokumen) override
        {
            static const evaluation_value_t map[]
            {
                /* empty    */  0,
                /* fu       */  1,
                /* kyo      */  3,
                /* kei      */  4,
                /* gin      */  5,
                /* kin      */  6,
                /* kaku     */  8,
                /* hi       */ 10,
                /* ou       */  0,
                /* tokin    */  7,
                /* nari_kyo */  6,
                /* nari_kei */  6,
                /* nari_gin */  6,
                /* uma      */ 10,
                /* ryu      */ 12
            };

            evaluation_value_t evaluation_value = kyokumen_map_evaluation_value(kyokumen, map);
            return evaluation_value;
        }

        const char * name() override
        {
            return "sample evaluator";
        }
    };

    class hiyoko_evaluator_t
        : public negamax_evaluator_t
    {
    public:
        evaluation_value_t evaluate(kyokumen_t & kyokumen) override
        {
            static const evaluation_value_t map[]
            {
                /* empty    */  0,
                /* fu       */  1,
                /* kyo      */  3,
                /* kei      */  4,
                /* gin      */  5,
                /* kin      */  6,
                /* kaku     */  8,
                /* hi       */ 10,
                /* ou       */  0,
                /* tokin    */  7,
                /* nari_kyo */  6,
                /* nari_kei */  6,
                /* nari_gin */  6,
                /* uma      */ 10,
                /* ryu      */ 12
            };

            evaluation_value_t evaluation_value = 0;
            evaluation_value += kyokumen_map_evaluation_value(kyokumen, map);
            return evaluation_value;
        }

        const char * name() override
        {
            return "ひよこ";
        }
    };

    class niwatori_evaluator_t
        : public alphabeta_evaluator_t
    {
    public:
        evaluation_value_t evaluate(kyokumen_t & kyokumen) override
        {
            static const evaluation_value_t map[]
            {
                /* empty    */  0,
                /* fu       */  1,
                /* kyo      */  3,
                /* kei      */  4,
                /* gin      */  5,
                /* kin      */  6,
                /* kaku     */  8,
                /* hi       */ 10,
                /* ou       */  0,
                /* tokin    */  7,
                /* nari_kyo */  6,
                /* nari_kei */  6,
                /* nari_gin */  6,
                /* uma      */ 10,
                /* ryu      */ 12
            };

            evaluation_value_t evaluation_value = 0;
            evaluation_value += kyokumen_map_evaluation_value(kyokumen, map);

            return evaluation_value;
        }

        const char * name() override
        {
            return "にわとり";
        }
    };

    class fukayomi_evaluator_t
        : public extendable_alphabeta_evaluator_t
    {
    public:
        evaluation_value_t evaluate(kyokumen_t & kyokumen) override
        {
            static const evaluation_value_t map[]
            {
                /* empty    */  0,
                /* fu       */  1,
                /* kyo      */  3,
                /* kei      */  4,
                /* gin      */  5,
                /* kin      */  6,
                /* kaku     */  8,
                /* hi       */ 10,
                /* ou       */  0,
                /* tokin    */  7,
                /* nari_kyo */  6,
                /* nari_kei */  6,
                /* nari_gin */  6,
                /* uma      */ 10,
                /* ryu      */ 12
            };

            constexpr evaluation_value_t destination_point = 1;
            constexpr evaluation_value_t kiki_point = -10;
            constexpr evaluation_value_t himo_point = 10;

            evaluation_value_t evaluation_value = 0;
            evaluation_value += kyokumen_map_evaluation_value(kyokumen, map);

            evaluation_value *= 100;

            for (pos_t pos = 0; pos < pos_size; ++pos)
            {
                if (!board_t::out(pos) && kyokumen.board[pos] != empty)
                {
                    std::vector<kiki_t> kiki_list;
                    color_t color = to_color(kyokumen.board[pos]);
                    kyokumen.search_kiki(std::back_inserter(kiki_list), pos, color);
                    evaluation_value += kiki_point * static_cast<evaluation_value_t>(kiki_list.size()) * reverse(color);
                    std::vector<pos_t> himo_list;
                    kyokumen.search_himo(std::back_inserter(himo_list), pos, color);
                    evaluation_value += himo_point * static_cast<evaluation_value_t>(himo_list.size()) * reverse(color);

                    std::vector<pos_t> destination_list;
                    kyokumen.search_destination(std::back_inserter(destination_list), pos, color);
                    evaluation_value += destination_point * static_cast<evaluation_value_t>(destination_list.size()) * reverse(color);
                }
            }

            return evaluation_value;
        }

        const char * name() override
        {
            return "深読み";
        }
    };

    /**
     * @breif 評価関数オブジェクトの実装例
     */
    class random_evaluator_t
        : public max_evaluator_t
    {
    public:
        inline evaluation_value_t evaluate(kyokumen_t & kyokumen) override
        {
            return uid(rand);
        }

        const char * name() override
        {
            return "random evaluator";
        }

        std::minstd_rand rand{ SHOGIPP_SEED };
        std::uniform_int_distribution<evaluation_value_t> uid{ -std::numeric_limits<evaluation_value_t>::max(), std::numeric_limits<evaluation_value_t>::max() };
    };

    class command_t
    {
    public:
        enum class id_t
        {
            error,
            move,
            undo,
            giveup,
            dump,
            perft,
            hash,
            sfen,
        };

        id_t id{ id_t::error };
        std::optional<move_t> opt_te;
        std::optional<move_count_t> opt_depth;
    };

    /**
     * @breif 標準入力からコマンドを読み込む。
     * @param moves 合法手
     * @return 読み込まれたコマンド
     */
    inline command_t read_command_line_input(const moves_t & moves)
    {
        std::string command_line;
        while (true)
        {
            try
            {
                std::getline(std::cin, command_line);

                std::vector<std::string> tokens;
                details::split_tokens(std::back_inserter(tokens), std::string_view{ command_line });

                if (!tokens.empty())
                {
                    if (tokens[0] == "undo")
                    {
                        return command_t{ command_t::id_t::undo };
                    }

                    if (tokens[0] == "giveup")
                    {
                        return command_t{ command_t::id_t::giveup };
                    }

                    if (tokens[0] == "dump")
                    {
                        return command_t{ command_t::id_t::dump };
                    }

                    if (tokens[0] == "perft")
                    {
                        if (tokens.size() != 2)
                            throw invalid_command_line_input{ "unknown command line input" };
                        move_count_t depth;
                        try
                        {
                            depth = std::stol(tokens[1]);
                        }
                        catch (...)
                        {
                            throw invalid_command_line_input{ "unknown command line input" };
                        }
                        command_t command;
                        command.id = command_t::id_t::perft;
                        command.opt_depth = depth;
                        return command;
                    }

                    if (tokens[0] == "hash")
                    {
                        return command_t{ command_t::id_t::hash };
                    }

                    if (tokens[0] == "sfen")
                    {
                        return command_t{ command_t::id_t::sfen };
                    }

                    std::size_t move_index;
                    try
                    {
                        move_index = std::stol(tokens[0]);
                    }
                    catch (...)
                    {
                        throw invalid_command_line_input{ "unknown command line input" };
                    }
                    if (move_index == 0)
                        throw invalid_command_line_input{ "move_index == 0" };
                    if (move_index > moves.size())
                        throw invalid_command_line_input{ "move_index > moves.size()" };
                    const std::size_t raw_move_index = move_index - 1;
                    command_t command;
                    command.id = command_t::id_t::move;
                    command.opt_te = moves[raw_move_index];
                    return command;
                }
            }
            catch (const std::exception & e)
            {
                std::cerr << e.what() << std::endl;
            }
            std::cin.clear();
        }

        SHOGIPP_ASSERT(false);
        return command_t{ command_t::id_t::error };
    }

    class abstract_kishi_t;

    /**
     * @breif 対局
     */
    class taikyoku_t
    {
    public:
        /**
         * @breif 対局を構築する。
         * @param a 先手の棋士
         * @param b 後手の棋士
         */
        inline taikyoku_t(const std::shared_ptr<abstract_kishi_t> & a, const std::shared_ptr<abstract_kishi_t> & b);

        /**
         * @breif 対局を実行する。
         * @retval true 対局が終了していない
         * @retval false 対局が終了した
         * @details この関数が true を返した場合、再度この関数を呼び出す。
         */
        inline bool procedure();

        /**
         * @breif 対局を標準出力に出力する。
         */
        inline void print() const;

        /**
         * @breif 手番の合法手を返す。
         * @return 合法手
         */
        inline const moves_t & get_moves() const;

        /**
         * @breif 手番の合法手を更新する。
         */
        inline void update_moves() const;

        std::shared_ptr<abstract_kishi_t> kishi_list[color_size];
        mutable moves_t moves;
        kyokumen_t kyokumen;
        bool black_win;
    };

    /**
     * @breif 棋士
     */
    class abstract_kishi_t
    {
    public:
        virtual ~abstract_kishi_t() {}

        /**
         * @breif コマンドを返す。
         * @param taikyoku 対局
         * @return コマンド
         */
        virtual command_t get_command(taikyoku_t & taikyoku) = 0;

        /**
         * @breif 棋士の名前を返す。
         * @return 棋士の名前
         */
        virtual const char * name() const = 0;
    };

    /**
     * @breif 標準入力により制御される棋士
     */
    class stdin_kishi_t
        : public abstract_kishi_t
    {
    public:
        command_t get_command(taikyoku_t & taikyoku) override;
        const char * name() const override;
    };

    /**
     * @breif 評価関数オブジェクトにより制御される棋士
     */
    class computer_kishi_t
        : public abstract_kishi_t
    {
    public:
        inline computer_kishi_t(const std::shared_ptr<abstract_evaluator_t> & ptr)
            : ptr{ ptr }
        {}

        command_t get_command(taikyoku_t & taikyoku) override;
        const char * name() const override;

    private:
        std::shared_ptr<abstract_evaluator_t> ptr;
    };

    command_t stdin_kishi_t::get_command(taikyoku_t & taikyoku)
    {
        return read_command_line_input(taikyoku.get_moves());
    }

    const char * stdin_kishi_t::name() const
    {
        return "stdin";
    }

    command_t computer_kishi_t::get_command(taikyoku_t & taikyoku)
    {
        return command_t{ command_t::id_t::move, ptr->best_move(taikyoku.kyokumen) };
    }

    const char * computer_kishi_t::name() const
    {
        return ptr->name();
    }

    inline taikyoku_t::taikyoku_t(const std::shared_ptr<abstract_kishi_t> & a, const std::shared_ptr<abstract_kishi_t> & b)
        : kishi_list{ a, b }
        , black_win{ false }
    {
        update_moves();
    }

    inline bool taikyoku_t::procedure()
    {
        auto & kishi = kishi_list[kyokumen.color()];

        kyokumen_t temp_kyokumen = kyokumen;

        while (true)
        {
            command_t cmd = kishi->get_command(*this);
            switch (cmd.id)
            {
            case command_t::id_t::error:
                break;
            case command_t::id_t::move:
                kyokumen.print_move(*cmd.opt_te, kyokumen.color());
                std::cout << std::endl << std::endl;
                kyokumen.do_move(*cmd.opt_te);
                update_moves();
                return !moves.empty();
            case command_t::id_t::undo:
                if (kyokumen.move_count >= 2)
                {
                    for (int i = 0; i < 2; ++i)
                        kyokumen.undo_move(kyokumen.kifu.back());
                    update_moves();
                    return !moves.empty();
                }
                break;
            case command_t::id_t::giveup:
                break;
            case command_t::id_t::dump:
                kyokumen.print_kifu();
                break;
            case command_t::id_t::perft:
                std::cout << kyokumen.count_node(*cmd.opt_depth) << std::endl;
                break;
            case command_t::id_t::hash:
                std::cout << hash_to_string(kyokumen.hash()) << std::endl;
                break;
            case command_t::id_t::sfen:
                std::cout << kyokumen.sfen_string() << std::endl;
                break;
            }
        }
    }

    inline void taikyoku_t::print() const
    {
        if (kyokumen.move_count == 0)
        {
            for (const color_t color : colors)
                std::cout << color_to_string(static_cast<color_t>(color)) << "：" << kishi_list[color]->name() << std::endl;
            std::cout << std::endl;
        }

        if (moves.empty())
        {
            auto & winner_evaluator = kishi_list[!kyokumen.color()];
            std::cout << kyokumen.move_count << "手詰み" << std::endl;
            kyokumen.print();
            std::cout << color_to_string(!kyokumen.color()) << "勝利（" << winner_evaluator->name() << "）" << std::flush;
        }
        else
        {
            std::cout << (kyokumen.move_count + 1) << "手目" << color_to_string(kyokumen.color()) << "番" << std::endl;
            kyokumen.print();
            std::cout << hash_to_string(kyokumen.hash()) << std::endl;
            kyokumen.print_move();
            kyokumen.print_check();
        }
    }

    inline const moves_t & taikyoku_t::get_moves() const
    {
        return moves;
    }

    inline void taikyoku_t::update_moves() const
    {
        moves.clear();
        kyokumen.search_moves(std::back_inserter(moves));
    }

    /**
    * @breif 対局する。
    * @param sente_kishi 先手の棋士
    * @param gote_kishi 後手の棋士
    */
    inline void do_taikyoku(kyokumen_t & kyokumen, const std::shared_ptr<abstract_kishi_t> & sente_kishi, const std::shared_ptr<abstract_kishi_t> & gote_kishi)
    {
        details::timer.clear();

        taikyoku_t taikyoku{ sente_kishi, gote_kishi };
        taikyoku.kyokumen = kyokumen;
        while (true)
        {
            taikyoku.print();
            if (!taikyoku.procedure())
                break;
        }

        taikyoku.print(); // 詰んだ局面を標準出力に出力する。

        std::cout << std::endl;
        details::timer.print_elapsed_time();

        taikyoku.kyokumen.print_kifu();
        std::cout.flush();
    }

    /**
     * @breif 対局する。
     * @param sente_kishi 先手の棋士
     * @param gote_kishi 後手の棋士
     */
    inline void do_taikyoku(const std::shared_ptr<abstract_kishi_t> & sente_kishi, const std::shared_ptr<abstract_kishi_t> & gote_kishi)
    {
        kyokumen_t kyokumen;
        do_taikyoku(kyokumen, sente_kishi, gote_kishi);
    }

    /**
     * @breif USIプロトコルで通信する機能を提供する。
     */
    class abstract_usi_engine_t
    {
    public:
        /**
         * @breif USIプロトコルで通信を開始する。
         */
        inline void run()
        {
            std::string line;
            std::string position;
            std::vector<std::string> moves;
            std::shared_ptr<usi_info_t> usi_info;
            std::map<std::string, std::string> setoptions;

            while (std::getline(std::cin, line))
            {
                std::vector<std::string> tokens;
                details::split_tokens(std::back_inserter(tokens), std::string_view{ line });

                std::size_t current = 0;

                if (tokens.empty())
                {
                    continue;
                }
                if (tokens[current] == "usi")
                {
                    std::cout << "id name " << name() << std::endl;
                    std::cout << "id author " << author() << std::endl;
                    std::cout << options() << std::flush;
                    std::cout << "usiok" << std::endl;
                }
                else if (tokens[current] == "setoption")
                {
                    std::optional<std::string> name;
                    std::optional<std::string> value;

                    ++current;
                    while (current < tokens.size())
                    {
                        if (tokens[current] == "name")
                        {
                            ++current;
                            if (current >= tokens.size())
                            {
                                throw invalid_usi_input{ "unexpected end of command" };
                            }
                            name = tokens[current];
                            ++current;
                        }
                        else if (tokens[current] == "value")
                        {
                            ++current;
                            if (current >= tokens.size())
                            {
                                throw invalid_usi_input{ "unexpected end of command" };
                            }
                            value = tokens[current];
                            ++current;
                        }
                    }

                    if (!name)
                    {
                        throw invalid_usi_input{ "name parameter not specified" };
                    }
                    else if (!value)
                    {
                        throw invalid_usi_input{ "value parameter not specified" };
                    }

                    setoptions[*name] = *value;
                }
                else if (tokens[current] == "isready")
                {
                    ready();
                    std::cout << "readyok" << std::endl;
                }
                else if (tokens[current] == "usinewgame")
                {
                    ;
                }
                else if (tokens[current] == "position")
                {
                    ++current;
                    position.clear();
                    for (std::size_t i = current; i < tokens.size(); ++i)
                    {
                        if (i > current)
                            position += ' ';
                        position += tokens[i];
                    }
                }
                else if (tokens[current] == "go")
                {
                    ++current;
                 
                    std::optional<unsigned long> opt_time[color_size];
                    std::optional<unsigned long> opt_byoyomi;
                    std::optional<unsigned long> opt_inc[color_size];
                    bool ponder = false;
                    bool infinite = false;
                    bool mate = false;

                    while (current < tokens.size())
                    {
                        if (tokens[current] == "ponder")
                        {
                            ++current;
                            ponder = true;
                        }
                        else if (tokens[current] == "btime")
                        {
                            ++current;
                            if (current >= tokens.size())
                            {
                                throw invalid_usi_input{ "btime value not found" };
                            }
                            opt_time[black] = std::stoul(tokens[current]);
                            ++current;
                        }
                        else if (tokens[current] == "wtime")
                        {
                            ++current;
                            if (current >= tokens.size())
                            {
                                throw invalid_usi_input{ "wtime value not found" };
                            }
                            opt_time[white] = std::stoul(tokens[current]);
                            ++current;
                        }
                        else if (tokens[current] == "byoyomi")
                        {
                            ++current;
                            if (current >= tokens.size())
                            {
                                throw invalid_usi_input{ "byoyomi value not found" };
                            }
                            opt_byoyomi = std::stoul(tokens[current]);
                            ++current;
                        }
                        else if (tokens[current] == "binc")
                        {
                            ++current;
                            if (current >= tokens.size())
                            {
                                throw invalid_usi_input{ "binc value not found" };
                            }
                            opt_inc[black] = std::stoul(tokens[current]);
                            ++current;
                        }
                        else if (tokens[current] == "winc")
                        {
                            ++current;
                            if (current >= tokens.size())
                            {
                                throw invalid_usi_input{ "winc value not found" };
                            }
                            opt_inc[white] = std::stoul(tokens[current]);
                            ++current;
                        }
                        else if (tokens[current] == "infinite")
                        {
                            ++current;
                            infinite = true;
                        }
                        else if (tokens[current] == "mate")
                        {
                            ++current;
                            mate = true;
                        }
                    }

                    if (mate)
                    {
                        ;
                    }
                    else // ponder or none
                    {
                        kyokumen_t kyokumen{ position };
                        auto evaluator = std::make_shared<hiyoko_evaluator_t>();
                        usi_info = std::make_shared<usi_info_t>();

                        {
                            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                            if (infinite)
                                usi_info->limit_time = std::numeric_limits<decltype(usi_info->limit_time)>::max();
                            else
                            {
                                if (!opt_time[kyokumen.color()])
                                    throw invalid_usi_input{ "limit time not specified" };
                                milli_second_time_t limit_time = *opt_time[kyokumen.color()];
                                if (opt_byoyomi)
                                    limit_time += *opt_byoyomi;
                                usi_info->limit_time = limit_time;
                            }
                            usi_info->ponder = ponder;
                            usi_info->options = setoptions;
                        }

                        evaluator->usi_info = usi_info;

                        auto search_thread_impl = [evaluator, kyokumen]() mutable
                        {
                            evaluator->best_move(kyokumen);
                        };
                        std::thread search_thread{ search_thread_impl };
                        search_thread.detach();

                        auto notify_thread_impl = [usi_info]()
                        {
                            while (true)
                            {
                                {
                                    std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                                    if (usi_info->state == usi_info_t::state_t::terminated)
                                        break;
                                    usi_info->periodic_print();
                                }
                                std::this_thread::sleep_for(std::chrono::milliseconds{ 1000 });
                            }
                        };
                        std::thread notify_thread{ notify_thread_impl };
                        notify_thread.detach();
                    }
                }
                else if (tokens[current] == "stop")
                {
                    if (usi_info)
                    {
                        std::optional<move_t> best_move;

                        {
                            std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                            best_move = usi_info->best_move;
                            usi_info->state = usi_info_t::state_t::requested_to_stop;
                        }
                        usi_info = std::make_shared<usi_info_t>();

                        if (!best_move)
                            throw invalid_usi_input{ "unexpected stop command" };
                    }
                    else
                    {
                        throw invalid_usi_input{ "unexpected stop command" };
                    }
                }
                else if (tokens[0] == "ponderhit")
                {
                    std::optional<move_t> best_move;
                    {
                        std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                        if (usi_info->state == usi_info_t::state_t::terminated)
                            best_move = usi_info->best_move;
                    }
                    if (best_move)
                        std::cout << "bestmove " << best_move->sfen_string() << std::endl;
                }
                else if (tokens[0] == "quit")
                {
                    std::terminate();
                }
                else if (tokens[0] == "gameover")
                {
                    ;
                }
            }
        }

        /**
         * @breif プログラムの名前を返す。
         * @return プログラムの名前
         */
        virtual std::string name() = 0;

        /**
         * @breif プログラムの開発者名を返す。
         * @return プログラムの開発者名
         */
        virtual std::string author() = 0;

        /**
         * @breif プログラムの固有の設定値を返す。
         * @return プログラムの固有の設定値
         * @details この関数が返した文字列は、サーバーから usi コマンドが送信された後、 usiok を送信する前に送信される。
         */
        virtual std::string options() = 0;

        /**
         * @breif isready コマンドを受信した場合に呼び出される。
         */
        virtual void ready() = 0;

    private:
        usi_info_t m_usi_info;
    };

    class usi_engine_t
        : public abstract_usi_engine_t
    {
    public:


        std::string name() override
        {
            return "shogipp";
        }

        std::string author() override
        {
            return "Noz";
        }

        std::string options() override
        {
            return "option name routine type combo default hiyoko var hiyoko var niwatori var fukayomi\n";
        }

        void ready() override
        {
            ;
        }
    };

    static const std::map<std::string, std::shared_ptr<abstract_kishi_t>> kishi_map
    {
        { "stdin"   , std::make_shared<stdin_kishi_t>()                                            },
        { "random"  , std::make_shared<computer_kishi_t>(std::make_shared<random_evaluator_t  >()) },
        { "sample"  , std::make_shared<computer_kishi_t>(std::make_shared<sample_evaluator_t  >()) },
        { "hiyoko"  , std::make_shared<computer_kishi_t>(std::make_shared<hiyoko_evaluator_t  >()) },
        { "niwatori", std::make_shared<computer_kishi_t>(std::make_shared<niwatori_evaluator_t>()) },
        { "fukayomi", std::make_shared<computer_kishi_t>(std::make_shared<fukayomi_evaluator_t>()) },
    };

    static const std::map<std::string, std::shared_ptr<abstract_evaluator_t>> evaluator_map
    {
        { "random"  , std::make_shared<random_evaluator_t  >() },
        { "sample"  , std::make_shared<sample_evaluator_t  >() },
        { "hiyoko"  , std::make_shared<hiyoko_evaluator_t  >() },
        { "niwatori", std::make_shared<niwatori_evaluator_t>() },
        { "fukayomi", std::make_shared<fukayomi_evaluator_t>() },
    };

    inline int parse_command_line(int argc, const char ** argv)
    {
        try
        {
            std::string kifu_path;
            std::optional<std::string> sente_name;
            std::optional<std::string> gote_name;

            std::shared_ptr<abstract_evaluator_t> a, b;

            auto callback = [&](const std::string & option, const std::vector<std::string> & params)
            {
                if (option == "kifu" && !params.empty())
                {
                    kifu_path = params[0];
                }
                else if (option == "sente" && !params.empty())
                {
                    sente_name = params[0];
                }
                else if (option == "gote" && !params.empty())
                {
                    gote_name = params[0];
                }
            };
            parse_program_options(argc, argv, callback);

            if (!sente_name || !gote_name)
            {
                usi_engine_t usi_engine;
                usi_engine.run();
            }
            else
            {
                auto sente_iter = kishi_map.find(*sente_name);
                if (sente_iter == kishi_map.end())
                {
                    throw invalid_command_line_input{ "invalid sente name" };
                }
                const std::shared_ptr<abstract_kishi_t> & sente_kishi = sente_iter->second;

                auto gote_iter = kishi_map.find(*gote_name);
                if (gote_iter == kishi_map.end())
                {
                    throw invalid_command_line_input{ "invalid gote name" };
                }
                const std::shared_ptr<abstract_kishi_t> & gote_kishi = gote_iter->second;

                if (!kifu_path.empty())
                {
                    kyokumen_t kyokumen;
                    kyokumen.read_kifu_file(kifu_path);
                    do_taikyoku(kyokumen, sente_kishi, gote_kishi);
                }
                else
                {
                    do_taikyoku(sente_kishi, gote_kishi);
                }
            }
        }
        catch (const std::exception & e)
        {
            std::cerr << e.what() << std::endl;
            return 1;
        }
        catch (...)
        {
            return 1;
        }

        return 0;
    }

    inline void usi_connect()
    {
        std::string line;
        while (std::getline(std::cin, line))
        {
        }
    }

} // namespace shogipp

#endif // SHOGIPP_DEFINED