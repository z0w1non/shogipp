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

#ifndef DEFAULT_SENTE_KISHI
#define DEFAULT_SENTE_KISHI "stdin"
#endif

#ifndef DEFAULT_GOTE_KISHI
#define DEFAULT_GOTE_KISHI "stdin"
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
    namespace details
    {
        SHOGIPP_STRING_LITERAL(split_tokens_literal, R"(\s+)")

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
            inline unsigned long long & search_count();

            /**
             * @breif 読み手数の参照を返す。
             * @return 読み手数の参照
             */
            inline const unsigned long long & search_count() const;

        private:
            std::chrono::system_clock::time_point m_begin;
            unsigned long long m_search_count;
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
            const unsigned long long sps = (unsigned long long)m_search_count * 1000 / duration;

            std::cout
                << std::endl
                << "総読み手数: " << m_search_count << std::endl
                << "実行時間[ms]: " << duration << std::endl
                << "読み手速度[手/s]: " << sps << std::endl << std::endl;
        }

        inline unsigned long long & timer_t::search_count()
        {
            return m_search_count;
        }

        inline const unsigned long long & timer_t::search_count() const
        {
            return m_search_count;
        }

        thread_local timer_t timer;
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

    using koma_t = unsigned char;
    enum : koma_t
    {
        empty,
        fu, kyo, kei, gin, kin, kaku, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu,
        sente_fu = fu, sente_kyo, sente_kei, sente_gin, sente_kin, sente_kaku, sente_hi, sente_ou, sente_tokin, sente_nari_kyo, sente_nari_kei, sente_nari_gin, sente_uma, sente_ryu,
        gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, gote_uma, gote_ryu,
        koma_enum_number,
        out_of_range = std::numeric_limits<unsigned char>::max()
    };

    const std::map<char, koma_t>char_to_koma
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

    enum sengo_t : unsigned char
    {
        sente = 0,
        gote = 1,
        sengo_size = 2
    };

    static constexpr sengo_t sengo_list[]
    {
        sente,
        gote
    };

    /**
     * @breif 逆の手番を取得する。
     * @param sengo 先手か後手か
     * @retval sente sengo == gote の場合
     * @retval gote sengo == sente の場合
     */
    inline sengo_t operator !(sengo_t sengo)
    {
        SHOGIPP_ASSERT(sengo >= sente);
        SHOGIPP_ASSERT(sengo <= gote);
        return static_cast<sengo_t>((sengo + 1) % sengo_size);
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

    using pos_to_koma_pair = std::pair<pos_t, std::vector<koma_t>>;

    static const pos_to_koma_pair near_kiki_list[]
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

    static const pos_to_koma_pair far_kiki_list[]
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

    static const pos_to_koma_pair far_kiki_list_asynmmetric[]
    {
        { front      , { kyo } },
    };

    static const pos_to_koma_pair far_kiki_list_synmmetric[]
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

    static const std::map<std::string, sengo_t> sengo_string_map
    {
        { "先手" , sente },
        { "後手" , gote }
    };
    static constexpr std::size_t sengo_string_size = 4;

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

    static const std::map<std::string, koma_t> koma_string_map
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
    static constexpr std::size_t koma_string_size = 2;

    static const std::map<std::string, sengo_t> sengo_prefix_string_map
    {
        { " ", sente },
        { "v", gote }
    };
    static constexpr std::size_t sengo_prefix_string_size = 1;

    static const std::map<std::string, sengo_t> sengo_mark_map
    {
        { "▲", sente },
        { "△", gote }
    };
    static constexpr std::size_t sengo_mark_size = 2;

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

    using tesu_t = unsigned int;
    using depth_t = signed int;

    inline constexpr sengo_t tesu_to_sengo(tesu_t tesu)
    {
        return static_cast<sengo_t>(tesu % sengo_size);
    }

    /**
     * @breif 後手の場合に -1 を、先手の場合に 1 を返す。
     * @param sengo 先手か後手か
     * @return 符号反転用の数値
     */
    inline constexpr pos_t reverse(sengo_t sengo)
    {
        return sengo ? -1 : 1;
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
            static std::minstd_rand rand{ SHOGIPP_SEED };
            static std::uniform_int_distribution<std::size_t> uid{ std::numeric_limits<std::size_t>::min(), std::numeric_limits<std::size_t>::max() };
            static_assert(hash_size % sizeof(std::size_t) == 0);
            auto random = [&]() -> std::size_t { return uid(rand); };
            for (std::size_t i = 0; i < hash_size / sizeof(std::size_t); ++i)
                *reinterpret_cast<std::size_t *>(data + i * sizeof(std::size_t)) = rand();
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
     * @param koma 駒
     * @retval true 成駒である
     * @retval false 成駒でない
     */
    inline bool is_promoted(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr bool map[]
        {
            false,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true
        };
        return map[koma];
    }

    /*
     * @breif 駒が成れるか判定する。
     * @param koma 駒
     * @retval true 成れる
     * @retval false 成れない
     */
    inline bool is_promotable(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static bool map[]
        {
            false,
            true, true, true, true, false, true, true, false, false, false, false, false, false, false,
            true, true, true, true, false, true, true, false, false, false, false, false, false, false,
        };
        return map[koma];
    }

    /*
     * @breif 駒が後手の駒か判定する。
     * @param koma 駒
     * @return 先手の駒である場合 sente
     */
    inline sengo_t to_sengo(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static sengo_t map[]
        {
            sente, /* dummy */
            sente, sente, sente, sente, sente, sente, sente, sente, sente, sente, sente, sente, sente, sente,
            gote, gote, gote, gote, gote, gote, gote, gote, gote, gote, gote, gote, gote, gote,
        };
        return map[koma];
    }

    /*
     * @breif 駒が走り駒(香・角・飛・馬・竜)か判定する。
     * @param koma 駒
     * @return 走り駒である場合 true
     */
    inline bool is_hashirigoma(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static bool map[]
        {
            false,
            false, true, false, false, false, true, true, false, false, false, false, false, true, true,
            false, true, false, false, false, true, true, false, false, false, false, false, true, true,
        };
        return map[koma];
    }

    /**
     * @breif 駒を持ち駒として適格の駒に変換する。
     * @param koma 駒
     * @return 持ち駒として適格の駒
     */
    inline koma_t to_mochigoma(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static koma_t map[]
        {
            0,
            fu, kyo, kei, gin, kin, kaku, hi, ou, fu, kyo, kei, gin, kaku, hi,
            fu, kyo, kei, gin, kin, kaku, hi, ou, fu, kyo, kei, gin, kaku, hi,
        };
        return map[koma];
    }

    /*
     * @breif 駒を成る前の駒に変換する。
     * @param koma 駒
     * @return 成る前の駒
     */
    inline koma_t to_unpromoted(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static koma_t map[]
        {
            0,
            sente_fu, sente_kyo, sente_kei, sente_gin, sente_kin, sente_kaku, sente_hi, sente_ou, sente_fu, sente_kyo, sente_kei, sente_gin, sente_kaku, sente_hi,
            gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_fu, gote_kyo, gote_kei, gote_gin, gote_kaku, gote_hi,
        };
        return map[koma];
    }

    /*
     * @breif 駒を成り駒に変換する。
     * @param koma 駒
     * @return 成り駒
     */
    inline koma_t to_promoted(koma_t koma)
    {
        SHOGIPP_ASSERT(is_promotable(koma));
        constexpr static koma_t map[]
        {
            0,
            sente_tokin, sente_nari_kyo, sente_nari_kei, sente_nari_gin, 0, sente_uma, sente_ryu, 0, 0, 0, 0, 0, 0, 0,
            gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, 0, gote_uma, gote_ryu, 0, 0, 0, 0, 0, 0, 0,
        };
        return map[koma];
    }

    /*
     * @breif 駒から先手後手の情報を取り除く。
     * @param koma 駒
     * @return 先手後手の情報を取り除かれた駒
     * @details sente_fu -> fu, gote_fu -> fu
     */
    inline koma_t trim_sengo(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static koma_t map[]
        {
            0,
            fu, kyo, kei, gin, kin, kaku, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu,
            fu, kyo, kei, gin, kin, kaku, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu,
        };
        return map[koma];
    }

    /**
     * @breif 駒を後手の駒に変換する。
     * @param koma 駒
     * @return 後手の駒
     * @details fu -> gote_fu
     */
    inline koma_t to_gote(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static koma_t map[]
        {
            0,
            gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, gote_uma, gote_ryu,
            gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, gote_uma, gote_ryu,
        };
        return map[koma];
    }

    /**
     * @breif koma が target_koma に合致するか判定する。
     * @param koma 駒
     * @retval true 合致する
     * @retval false 合致しない
     */
    template<koma_t target_koma>
    inline bool match(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        static const struct impl_t
        {
            impl_t()
            {
                for (koma_t koma = fu; koma < koma_enum_number; ++koma)
                    map[koma] = trim_sengo(koma) == target_koma;
            }
            bool map[koma_enum_number]{};
        } impl;
        return impl.map[koma];
    }
    

    class te_t;

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
         * @param koma 駒
         * @param pos 駒の座標
         * @return ハッシュ値
         */
        inline hash_t koma_hash(koma_t koma, pos_t pos) const;

        /**
         * @breif 持ち駒のハッシュ値を計算する。
         * @param koma 駒
         * @param count 駒の数
         * @param is_gote 後手の持ち駒か
         * @return ハッシュ値
         */
        inline hash_t mochigoma_hash(koma_t koma, std::size_t count, sengo_t sengo) const;

        /**
         * @breif 手番のハッシュ値を計算する。
         * @param sengo 先手か後手か
         * @return ハッシュ値
         */
        inline hash_t sengo_hash(sengo_t sengo) const;

        /**
         * @breif 合法手のハッシュ値を計算する。
         * @param te 合法手
         * @param sengo 先手か後手か
         * @return ハッシュ値
         */
        inline hash_t te_hash(const te_t & te, sengo_t sengo) const;

    private:
        hash_t ban_table[koma_enum_number * suji_size * dan_size];      // 盤のハッシュテーブル
        hash_t mochigoma_table[(18 + 4 + 4 + 4 + 4 + 2 + 2) * 2 * 2];   // 持ち駒のハッシュテーブル
        hash_t sengo_table[sengo_size];                                 // 手番のハッシュテーブル
        hash_t move_table[(pos_size + 1) * pos_size * sengo_size];      // 移動する手のハッシュテーブル
        hash_t put_table[pos_size * (hi - fu + 1) * sengo_size];        // 打つ手のハッシュテーブル
    };

    static const hash_table_t hash_table;

    inline hash_table_t::hash_table_t()
    {
#ifndef SIZE_OF_HASH
        std::minstd_rand rand{ SHOGIPP_SEED };
        std::uniform_int_distribution<hash_t> uid{ std::numeric_limits<hash_t>::min(), std::numeric_limits<hash_t>::max() };
        auto random = [&rand, &uid]() -> hash_t { return uid(rand); };
        std::generate(std::begin(ban_table      ), std::end(ban_table      ), random);
        std::generate(std::begin(mochigoma_table), std::end(mochigoma_table), random);
        std::generate(std::begin(sengo_table    ), std::end(sengo_table    ), random);
        std::generate(std::begin(move_table     ), std::end(move_table     ), random);
        std::generate(std::begin(put_table      ), std::end(put_table      ), random);
#endif
    }

    inline hash_t hash_table_t::koma_hash(koma_t koma, pos_t pos) const
    {
        std::size_t index = static_cast<std::size_t>(koma);
        index *= suji_size;
        index += pos_to_suji(pos);
        index *= dan_size;
        index += pos_to_dan(pos);
        SHOGIPP_ASSERT(index < std::size(ban_table));
        return ban_table[index];
    }

    inline hash_t hash_table_t::mochigoma_hash(koma_t koma, std::size_t count, sengo_t sengo) const
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

        SHOGIPP_ASSERT(koma != empty);
        SHOGIPP_ASSERT(koma >= fu);
        SHOGIPP_ASSERT(koma <= hi);
        SHOGIPP_ASSERT(!(koma == fu   && count >= mochigoma_fu_size ));
        SHOGIPP_ASSERT(!(koma == kyo  && count >= mochigoma_kyo_size ));
        SHOGIPP_ASSERT(!(koma == kei  && count >= mochigoma_kei_size ));
        SHOGIPP_ASSERT(!(koma == gin  && count >= mochigoma_gin_size ));
        SHOGIPP_ASSERT(!(koma == kin  && count >= mochigoma_kin_size ));
        SHOGIPP_ASSERT(!(koma == kaku && count >= mochigoma_kaku_size ));
        SHOGIPP_ASSERT(!(koma == hi   && count >= mochigoma_hi_size ));

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

        std::size_t index = map[koma];
        index += count;
        index *= sengo_size;
        index += static_cast<std::size_t>(sengo);
        SHOGIPP_ASSERT(index < std::size(mochigoma_table));
        return mochigoma_table[index];
    }

    inline hash_t hash_table_t::sengo_hash(sengo_t sengo) const
    {
        SHOGIPP_ASSERT(sengo >= sente);
        SHOGIPP_ASSERT(sengo <= gote);
        return sengo_table[sengo];
    }

    /**
     * @breif 先後を表現する文字列を取得する。
     * @param sengo 先後
     * @return 先後を表現する文字列
     */
    inline const char * sengo_to_string(sengo_t sengo)
    {
        const char * map[]{ "先手", "後手" };
        return map[sengo];
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
     * @param koma 駒
     * @return 駒の移動先の相対座標の配列の先頭を指すポインタ
     * @details この関数が返すポインタの指す座標は 0 で終端化されている。
     */
    inline const pos_t * near_move_offsets(koma_t koma)
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
        SHOGIPP_ASSERT(koma != empty);
        SHOGIPP_ASSERT(koma <= std::size(map));
        return map[koma].data();
    }

    /**
     * @breif 駒の移動先の相対座標の配列の先頭を指すポインタを取得する。
     * @param koma 駒
     * @return 駒の移動先の相対座標の配列の先頭を指すポインタ
     * @details この関数が返すポインタの指す座標は 0 で終端化されている。
     */
    inline const pos_t * far_move_offsets(koma_t koma)
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
        SHOGIPP_ASSERT(koma != empty);
        SHOGIPP_ASSERT(koma <= std::size(map));
        return map[koma].data();
    }

    /**
     * @breif 駒を文字列に変換する。
     * @param koma 駒
     * @return 文字列
     */
    inline const char * koma_to_string(koma_t koma)
    {
        SHOGIPP_ASSERT(koma < koma_enum_number);
        static const char * map[]{
            "・",
            "歩", "香", "桂", "銀", "金", "角", "飛", "王", "と", "杏", "圭", "全", "馬", "竜",
            "歩", "香", "桂", "銀", "金", "角", "飛", "王", "と", "杏", "圭", "全", "馬", "竜",
        };
        return map[koma];
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
        if (sfen_pos[0] < '0')
            throw invalid_usi_input{ "sfen_pos[0] < '0'" };
        if (sfen_pos[0] > '9')
            throw invalid_usi_input{ "sfen_pos[0] > '9'" };
        if (sfen_pos[1] < 'a')
            throw invalid_usi_input{ "sfen_pos[1] < 'a'" };
        if (sfen_pos[1] > 'i')
            throw invalid_usi_input{ "sfen_pos[1] > 'i'" };
        const pos_t suji = static_cast<pos_t>(sfen_pos[0] - '0');
        const pos_t dan = dan_size - static_cast<pos_t>(sfen_pos[1] - 'a');
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

    class ban_t;

    /**
     * @breif 合法手
     */
    class te_t
    {
    public:
        /**
         * @breif 打ち手を構築する。
         * @param destination 打つ座標
         * @param source_koma 打つ駒
         */
        inline te_t(pos_t destination, koma_t source_koma) noexcept;

        /**
         * @breif 移動する手を構築する。
         * @param source 移動元の座標
         * @param destination 移動先の座標
         * @param source_koma 移動元の駒
         * @param captured_koma 移動先の駒
         * @param promote 成か不成か
         */
        inline te_t(pos_t source, pos_t destination, koma_t source_koma, koma_t captured_koma, bool promote) noexcept;

        /**
         * @breif SFEN表記法に準拠した moves の後に続く文字列から手を構築する。
         * @param sfen SFEN表記法に準拠した moves の後に続く文字列
         * @param ban 盤
         */
        inline te_t(std::string_view sfen_move, const ban_t & ban);

        /**
         * @breif 打ち手か判定する。
         * @retval true 打ち手である
         * @retval false 移動する手である
         */
        inline bool is_uchite() const noexcept;

        /**
         * @breif 移動元の座標を取得する。
         * @return 移動元の座標
         * @details is_uchite が true を返す場合にこの関数を呼び出した場合、無効な値が返る。
         */
        inline pos_t source() const noexcept;

        /**
         * @breif 移動先の座標を取得する。
         * @return 移動先の座標
         * @details is_uchite が true を返す場合、この関数は打つ先の座標を返す。
         */
        inline pos_t destination() const noexcept;

        /**
         * @breif 移動元の駒を取得する。
         * @return 移動元の駒
         */
        inline koma_t source_koma() const noexcept;

        /**
         * @breif 移動先の駒を取得する。
         * @return 移動先の駒
         * @detalis is_uchite が true を返す場合にこの関数を呼び出した場合、無効な値が返る。
         */
        inline koma_t captured_koma() const noexcept;

        /**
         * @breif 成るか否かを取得する。
         * @retval true 成る
         * @retval false 成らない
         * @detalis is_uchite が true を返す場合にこの関数を呼び出した場合、無効な値が返る。
         */
        inline bool promote() const noexcept;

    private:
        pos_t   m_source;           // 移動元の座標(source == npos の場合、持ち駒を打つ)
        pos_t   m_destination;      // 移動先の座標(source == npos の場合、 destination は打つ座標)
        koma_t  m_source_koma;      // 移動元の駒(source == npos の場合、 source_koma() は打つ持ち駒)
        koma_t  m_captured_koma;    // 移動先の駒(source == npos の場合、 captured_koma は未定義)
        bool    m_promote;          // 成る場合 true
    };

    inline te_t::te_t(pos_t destination, koma_t source_koma) noexcept
        : m_source{ npos }
        , m_destination{ destination }
        , m_source_koma{ source_koma }
        , m_captured_koma{ empty }
        , m_promote{ false }
    {
        SHOGIPP_ASSERT(source_koma >= fu);
        SHOGIPP_ASSERT(source_koma <= hi);
    }

    inline te_t::te_t(pos_t source, pos_t destination, koma_t source_koma, koma_t captured_koma, bool promote) noexcept
        : m_source{ source }
        , m_destination{ destination }
        , m_source_koma{ source_koma }
        , m_captured_koma{ captured_koma }
        , m_promote{ promote }
    {
    }

    inline bool te_t::is_uchite() const noexcept
    {
        return m_source == npos;
    }

    inline pos_t te_t::source() const noexcept
    {
        SHOGIPP_ASSERT(!is_uchite());
        return m_source;
    }

    inline pos_t te_t::destination() const noexcept
    {
        return m_destination;
    }

    inline koma_t te_t::source_koma() const noexcept
    {
        return m_source_koma;
    }

    inline koma_t te_t::captured_koma() const noexcept
    {
        SHOGIPP_ASSERT(!is_uchite());
        return m_captured_koma;
    }

    inline bool te_t::promote() const noexcept
    {
        SHOGIPP_ASSERT(!is_uchite());
        return m_promote;
    }

    /**
     * @breif 合法手を格納する std::vector を表現する。
     */
    class te_list_t
        : public std::vector<te_t>
    {
    public:
        using std::vector<te_t>::vector;

        inline te_list_t()
        {
            constexpr std::size_t max_size = 593;
            reserve(max_size);
        }
    };

    inline int to_category(const te_t & te)
    {
        if (te.is_uchite())
            return 0;
        if (te.captured_koma() == empty)
            return 1;
        return 2;
    };

    inline hash_t hash_table_t::te_hash(const te_t & te, sengo_t sengo) const
    {
        std::size_t index;
        if (te.is_uchite())
        {
            index = static_cast<std::size_t>(te.destination());
            index *= koma_enum_number;
            index += te.source_koma();
            index *= sengo_size;
            index += sengo;
            SHOGIPP_ASSERT(index < std::size(put_table));
            return put_table[index];
        }
        index = static_cast<std::size_t>(te.source() - npos);
        index *= pos_size;
        index += static_cast<std::size_t>(te.destination());
        index *= sengo_size;
        index += sengo;
        SHOGIPP_ASSERT(index < std::size(move_table));
        return move_table[index];
    }

    /**
     * @breif 持ち駒
     */
    class mochigoma_t
    {
    public:
        using size_type = unsigned char;

        /**
         * @breif 持ち駒を構築する。
         */
        inline mochigoma_t();

        /**
         * @breif 持ち駒を標準出力に出力する。
         */
        inline void print() const;

        /**
         * @breif 駒と対応する持ち駒の数の参照を返す。
         * @param 駒
         * @return 駒と対応する持ち駒の数の参照
         */
        inline size_type & operator [](koma_t koma);

        /**
         * @breif 駒と対応する持ち駒の数の参照を返す。
         * @param 駒
         * @return 駒と対応する持ち駒の数の参照
         */
        inline const size_type & operator [](koma_t koma) const;

    private:
        size_type count[hi - fu + 1];
    };

    inline mochigoma_t::mochigoma_t()
    {
        std::fill(std::begin(count), std::end(count), 0);
    }

    inline void mochigoma_t::print() const
    {
        unsigned int kind = 0;
        for (koma_t koma = hi; koma >= fu; --koma)
        {
            if ((*this)[koma] > 0)
            {
                std::cout << koma_to_string(koma);
                if ((*this)[koma] > 1)
                    std::cout << to_zenkaku_digit((*this)[koma]);
                ++kind;
            }
        }
        if (kind == 0)
            std::cout << "なし";
        std::cout << std::endl;
    }

    inline mochigoma_t::size_type & mochigoma_t::operator [](koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        SHOGIPP_ASSERT(to_mochigoma(koma) != ou);
        return count[to_mochigoma(koma) - fu];
    }

    inline const mochigoma_t::size_type & mochigoma_t::operator [](koma_t koma) const
    {
        return (*const_cast<mochigoma_t *>(this))[koma];
    }

    class kyokumen_rollback_validator_t;

    /**
     * @breif 盤
     */
    class ban_t
    {
    public:
        /**
         * @breif 盤を構築する。
         */
        inline ban_t();

        inline koma_t & operator [](size_t i) { return data[i]; }
        inline const koma_t & operator [](size_t i) const { return data[i]; }

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

    private:
        friend class kyokumen_rollback_validator_t;
        koma_t data[pos_size];
    };

#define _ empty
#define x out_of_range
    inline ban_t::ban_t()
    {
        static const koma_t temp[]
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
        std::copy(std::begin(temp), std::end(temp), std::begin(data));
    }

    inline bool ban_t::out(pos_t pos)
    {
        static const koma_t table[]
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
        return pos < 0 || pos >= pos_size || table[pos] == out_of_range;
    }
#undef _
#undef x

    inline void ban_t::print() const
    {
        std::cout << "  ９ ８ ７ ６ ５ ４ ３ ２ １" << std::endl;
        std::cout << "+---------------------------+" << std::endl;
        for (pos_t dan = 0; dan < dan_size; ++dan)
        {
            std::cout << "|";
            for (pos_t suji = 0; suji < suji_size; ++suji)
            {
                koma_t koma = data[suji_dan_to_pos(suji, dan)];
                std::cout << ((koma != empty && to_sengo(koma)) ? "v" : " ") << koma_to_string(koma);
            }
            std::cout << "| " << dan_to_string(dan) << std::endl;
        }
        std::cout << "+---------------------------+" << std::endl;
    }

    inline void ban_t::clear()
    {
        std::fill(std::begin(data), std::end(data), empty);
    }

    inline te_t::te_t(std::string_view sfen_move, const ban_t & ban)
    {
        if (sfen_move.size() < 4)
            throw invalid_usi_input{ "invalid sfen move" };

        if (sfen_move[1] == '*')
        {
            if (sfen_move.size() > 4)
                throw invalid_usi_input{ "invalid sfen move" };
            auto iter = char_to_koma.find(sfen_move[0]);
            if (iter == char_to_koma.end())
                throw invalid_usi_input{ "invalid sfen move" };
            const koma_t koma = iter->second;
            if (to_sengo(koma) == gote)
                throw invalid_usi_input{ "invalid sfen move" };
            const pos_t destination = sfen_pos_to_pos(sfen_move.substr(2, 2));

            m_source = npos;
            m_destination = destination;
            m_source_koma = koma;
            m_captured_koma = empty;
            m_promote = false;
        }
        else
        {
            const pos_t source = sfen_pos_to_pos(sfen_move.substr(0, 2));
            const pos_t destination = sfen_pos_to_pos(sfen_move.substr(2, 2));
            bool promote;
            if (sfen_move.size() == 5 && sfen_move[4] == '+')
                promote = true;
            else if (sfen_move.size() > 5)
                throw invalid_usi_input{ "invalid sfen move" };
            else
                promote = false;

            m_source = source;
            m_destination = destination;
            m_source_koma = ban[source];
            m_captured_koma = ban[destination];
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
            for (auto & [pos, candidates] : *this)
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
        std::vector<std::vector<kiki_t>> check_list_stack;   // 手番にかかっている王手
        std::vector<hash_t> hash_stack;                     // 局面のハッシュ値
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
         * @breif SFEN表記法に準拠した文字列から局面を構築する。
         */
        inline kyokumen_t(std::string_view sfen);

        /**
         * @breif 駒が移動する場合に成りが可能か判定する。
         * @param koma 駒
         * @param source 移動元の座標
         * @param destination 移動先の座標
         * @return 成りが可能の場合(komaが既に成っている場合、常にfalse)
         */
        inline static bool promotable(koma_t koma, pos_t source, pos_t destination);

        /**
         * @breif 駒が移動する場合に成りが必須か判定する。
         * @param koma 駒
         * @param destination 移動先の座標
         * @return 成りが必須の場合(komaが既に成っている場合、常にfalse)
         */
        inline static bool must_promote(koma_t koma, pos_t destination);

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
         * @param sengo 先手・後手どちらの移動か
         */
        template<typename OutputIterator>
        inline void search_destination(OutputIterator result, pos_t source, sengo_t sengo) const;

        /**
         * @breif 持ち駒を移動先の座標に打つことができるか判定する。歩、香、桂に限りfalseを返す可能性がある。
         * @param koma 持ち駒
         * @param destination 移動先の座標
         * @return 置くことができる場合 true
         */
        inline bool puttable(koma_t koma, pos_t destination) const;

        /**
         * @breif 移動元の座標を検索する。
         * @param result 出力イテレータ
         * @param sengo 先手・後手どちらの移動か
         */
        template<typename OutputIterator>
        inline void search_source(OutputIterator result, sengo_t sengo) const;

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
        inline void search_koma_near(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const;

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
        inline void search_koma_far(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const;

        /**
         * @breif 座標posに利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param sengo 先後いずれの視点か
         * @param is_collected 見つかった駒の手番に対して出力イテレータに出力するか判定する叙述関数(bool(bool))
         * @param transform (pos, offset, aigoma) を出力イテレータに出力する変数に変換する関数
         */
        template<typename OutputIterator, typename IsCollected, typename Transform>
        inline void search_koma(OutputIterator result, pos_t pos, sengo_t sengo, IsCollected is_collected, Transform transform) const;

        /**
         * @breif 座標posに紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param sengo 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_himo(OutputIterator result, pos_t pos, sengo_t sengo) const;

        /**
         * @breif 座標posに利いている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param sengo 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_kiki(OutputIterator result, pos_t pos, sengo_t sengo) const;

        /**
         * @breif 座標posに利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param sengo 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_kiki_or_himo(OutputIterator result, pos_t pos, sengo_t sengo) const;

        /**
         * @breif 王手を検索する。
         * @param result 座標の出力イテレータ
         * @param sengo 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_check(OutputIterator result, sengo_t sengo) const;

        /**
         * @breif 王手を検索する。
         * @param sengo 先後いずれの視点か
         * @return 王手
         */
        std::vector<kiki_t> search_check(sengo_t sengo) const;

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
         * @param sengo 先後いずれの視点か
         */
        inline void search_aigoma(aigoma_info_t & aigoma_info, sengo_t sengo) const;

        /**
         * @breif 合駒を検索する。
         * @param sengo 先後いずれの視点か
         * @return 合駒の情報
         */
        inline aigoma_info_t search_aigoma(sengo_t sengo) const;

        /**
         * @breif 移動元と移動先の座標から合法手を検索する。
         * @param result 合法手の出力イテレータ
         * @param source 移動元の座標
         * @param destination 移動先の座標
         */
        template<typename OutputIterator>
        inline void search_te_from_positions(OutputIterator result, pos_t source, pos_t destination) const;

        /**
         * @breif 合法手のうち王手を外さない手を検索する。
         * @param result 合法手の出力イテレータ
         * @details 王手されていない場合、この関数により生成される手の集合は合法手全体と完全に一致する。
         */
        template<typename OutputIterator>
        inline void search_te_nonevasions(OutputIterator result) const;

        /**
         * @breif 王手を外す手のうち王を移動する手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_te_evasions_ou_move(OutputIterator result) const;

        /**
         * @breif 王手を外す手のうち合駒する手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_te_evasions_aigoma(OutputIterator result) const;

        /**
         * @breif 合法手のうち王手を外す手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_te_evasions(OutputIterator result) const;

        /**
         * @breif 王手を外さない手のうち駒を動かすte する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_te_moves(OutputIterator result) const;

        /**
         * @breif 王手を外さない手のうち持ち駒を打つ手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_te_puts(OutputIterator result) const;

        /**
         * @breif 合法手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_te(OutputIterator result) const;

        /**
         * @breif 合法手を検索する。
         * @param result 合法手の出力イテレータ
         */
        inline te_list_t search_te() const;

        /**
         * @breif 局面のハッシュ値を計算する。
         * @return 局面のハッシュ値
         */
        inline hash_t make_hash() const;

        /**
         * @breif 局面のハッシュ値と合法手から、合法手を実施した後の局面のハッシュ値を計算する。
         * @param hash 合法手を実施する前の局面のハッシュ値
         * @param te 実施する合法手
         * @return 合法手を実施した後の局面のハッシュ値
         * @details 合法手により発生する差分に基づき計算するため make_hash() より比較的高速に処理される。
         *          この関数は合法手を実施するより前に呼び出される必要がある。
         */
        inline hash_t make_hash(hash_t hash, const te_t & te) const;

        /**
         * @breif 合法手を標準出力に出力する。
         * @param te 合法手
         * @param is_gote 後手の合法手か
         */
        inline void print_te(const te_t & te, sengo_t sengo) const;

        /**
         * @breif 合法手を標準出力に出力する。
         * @param first 合法手の入力イテレータのbegin
         * @param last 合法手の入力イテレータのend
         */
        template<typename InputIterator>
        inline void print_te(InputIterator first, InputIterator last) const;

        /**
         * @breif 合法手を標準出力に出力する。
         */
        inline void print_te() const;

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

        inline void validate_ban_out();

        /**
         * @breif 合法手を実行する。
         * @param te 合法手
         */
        inline void do_te(const te_t & te);

        /**
         * @breif 合法手を実行する前に戻す。
         * @param te 合法手
         */
        inline void undo_te(const te_t & te);

        /**
         * @breif 手番を取得する。
         * @return 手番
         */
        inline sengo_t sengo() const;

        /**
         * @breif 指定された手数で分岐する局面の数を数える。
         * @param depth 手数
         * @return 局面の数
         */
        inline unsigned long long count_node(tesu_t depth) const;

        /**
         * @breif 局面ファイルから局面を読み込む。
         * @param kyokumen_file 局面ファイル
         */
        inline void read_kyokumen_file(std::filesystem::path kyokumen_file);

        /**
         * @breif 棋譜ファイルから棋譜を読み込む。
         * @param kifu_file 棋譜ファイル
         */
        inline void read_kifu_file(std::filesystem::path kifu_file);

        ban_t ban;                                  // 盤
        mochigoma_t mochigoma_list[sengo_size];     // 持ち駒
        tesu_t tesu;                                // 手数
        pos_t ou_pos_list[sengo_size];                   // 王の座標
        std::vector<te_t> kifu;                     // 棋譜
        additional_info_t additional_info;          // 追加情報
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
        koma_t data[pos_size];
        mochigoma_t mochigoma_list[sengo_size];
    };

    inline kyokumen_rollback_validator_t::kyokumen_rollback_validator_t(const kyokumen_t & kyokumen)
        : kyokumen{ kyokumen }
    {
        std::copy(std::begin(kyokumen.ban.data), std::end(kyokumen.ban.data), std::begin(data));
        for (sengo_t sengo : sengo_list)
            std::copy(std::begin(kyokumen.mochigoma_list), std::end(kyokumen.mochigoma_list), std::begin(mochigoma_list));
    }

    inline kyokumen_rollback_validator_t::~kyokumen_rollback_validator_t()
    {
        for (std::size_t i = 0; i < std::size(data); ++i)
            SHOGIPP_ASSERT(data[i] == kyokumen.ban.data[i]);
        for (sengo_t sengo : sengo_list)
            for (koma_t koma = fu; koma <= hi; ++koma)
                SHOGIPP_ASSERT(mochigoma_list[sengo][koma] == kyokumen.mochigoma_list[sengo][koma]);
    }

    inline kyokumen_t::kyokumen_t()
    {
        tesu = 0;
        for (sengo_t sengo : sengo_list)
            ou_pos_list[sengo] = default_ou_pos_list[sengo];
        push_additional_info();
    }

    inline kyokumen_t::kyokumen_t(std::string_view sfen)
    {
        kyokumen_t temp;
        temp.ban.clear();

        bool promoted = false;
        pos_t dan = 0, suji = 0;

        std::size_t i = 0;
        for (; i < sfen.size(); ++i)
        {
            if (sfen[i] == ' ')
                break;
            else if (sfen[i] == '+')
            {
                promoted = true;
            }
            else if (sfen[i] == '/')
            {
                if (suji != suji_size)
                    throw invalid_usi_input{ "unexpected '/'" };
                ++dan;
            }
            else if (sfen[i] >= '1' && sfen[i] <= '9')
            {
                suji += static_cast<pos_t>(sfen[i] - '0');
            }
            else
            {
                auto iter = char_to_koma.find(sfen[i]);
                if (iter == char_to_koma.end())
                    throw invalid_usi_input{ "unexpected character" };
                koma_t koma = iter->second;
                if (promoted)
                    koma = to_promoted(koma);
                temp.ban[suji_dan_to_pos(suji, dan)] = koma;
                ++suji;
            }
        }

        while (i < sfen.size() && sfen[i] == ' ')
            ++i;

        if (i >= sfen.size())
            throw invalid_usi_input{ "unexpected sfen end" };

        sengo_t sengo;
        if (sfen[i] == 'b')
            sengo = sente;
        if (sfen[i] == 'w')
            sengo = gote;
        else
            throw invalid_usi_input{ "invalid color" };
        ++i;

        while (i < sfen.size() && sfen[i] == ' ')
            ++i;

        if (i >= sfen.size())
            throw invalid_usi_input{ "unexpected sfen end" };

        if (sfen[i] == '-')
        {
            ++i;
        }
        else
        {
            mochigoma_t::size_type count = 1;
            while (i < sfen.size() && sfen[i] != ' ')
            {
                if (sfen[i] >= '0' && sfen[i] <= '9')
                {
                    count = static_cast<mochigoma_t::size_type>(sfen[i] - '0');
                    while (++i < sfen.size() && sfen[i] >= '0' && sfen[i] <= '9')
                        count = static_cast<mochigoma_t::size_type>(count * 10 + sfen[i] - '0');
                }
                else
                {
                    auto iter = char_to_koma.find(sfen[i]);
                    if (iter == char_to_koma.end())
                        throw invalid_usi_input{ "unexpected character" };
                    koma_t koma = iter->second;
                    temp.mochigoma_list[to_sengo(koma)][trim_sengo(koma)] = count;
                    count = 1;
                }
                ++i;
            }
        }

        while (i < sfen.size() && sfen[i] == ' ')
            ++i;

        if (sfen[i] >= '0' && sfen[i] <= '9')
        {
            tesu_t tesu = static_cast<tesu_t>(sfen[i] - '0');
            while (++i < sfen.size() && sfen[i] >= '0' && sfen[i] <= '9')
                tesu = static_cast<tesu_t>(tesu * 10 + sfen[i] - '0');
            --tesu;
            /*temp.tesu = tesu;*/ /* unused */
        }

        while (i < sfen.size() && sfen[i] == ' ')
            ++i;

        constexpr std::string_view moves = "moves";
        if (i + moves.size() > sfen.size() && sfen.substr(i, moves.size()) == moves)
        {
            i += moves.size();

            while (i < sfen.size() && sfen[i] == ' ')
                ++i;

            while (i < sfen.size() && sfen[i] == ' ')
            {
                while (i < sfen.size() && sfen[i] == ' ')
                    ++i;

                if (i < sfen.size())
                {
                    const std::size_t begin = i;
                    while (i < sfen.size() && sfen[i] != ' ')
                        ++i;

                    const std::string_view token = sfen.substr(begin, i - begin + 1);
                    const te_t te{ token, temp.ban };
                    temp.do_te(te);
                }
            }
        }

        if (temp.sengo() != sengo)
            throw invalid_usi_input{ "invalid color" };

        *this = std::move(temp);
    }

    inline bool kyokumen_t::promotable(koma_t koma, pos_t source, pos_t destination)
    {
        if (!is_promotable(koma))
            return false;
        if (to_sengo(koma) == sente)
            return source < width * (3 + padding_height) || destination < width * (3 + padding_height);
        return source >= width * (6 + padding_height) || destination >= width * (6 + padding_height);
    }

    inline bool kyokumen_t::must_promote(koma_t koma, pos_t destination)
    {
        if (trim_sengo(koma) == fu || trim_sengo(koma) == kyo)
        {
            if (to_sengo(koma) == sente)
                return destination < (width * (1 + padding_height));
            return destination >= width * (8 + padding_height);
        }
        else if (trim_sengo(koma) == kei)
        {
            if (to_sengo(koma) == sente)
                return destination < width * (2 + padding_height);
            return destination >= width * (7 + padding_height);
        }
        return false;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_far_destination(OutputIterator result, pos_t source, pos_t offset) const
    {
        for (pos_t current = source + offset; !ban_t::out(current); current += offset)
        {
            if (ban[current] == empty)
                *result++ = current;
            else
            {
                if (to_sengo(ban[source]) == to_sengo(ban[current])) break;
                *result++ = current;
                if (to_sengo(ban[source]) != to_sengo(ban[current])) break;
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_near_destination(OutputIterator result, pos_t source, pos_t offset) const
    {
        pos_t current = source + offset;
        if (!ban_t::out(current) && (ban[current] == empty || to_sengo(ban[current]) != to_sengo(ban[source])))
            *result++ = current;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_destination(OutputIterator result, pos_t source, sengo_t sengo) const
    {
        koma_t koma = trim_sengo(ban[source]);
        for (const pos_t * offset = far_move_offsets(koma); *offset; ++offset)
            search_far_destination(result, source, *offset * reverse(sengo));
        for (const pos_t * offset = near_move_offsets(koma); *offset; ++offset)
            search_near_destination(result, source, *offset * reverse(sengo));
    }

    inline bool kyokumen_t::puttable(koma_t koma, pos_t destination) const
    {
        if (ban[destination] != empty)
            return false;
        if (sengo() == sente)
        {
            if ((koma == fu || koma == kyo) && destination < width * (padding_height + 1))
                return false;
            if (koma == kei && destination < width * (padding_height + 2))
                return false;
        }
        else
        {
            if ((koma == fu || koma == kyo) && destination >= width * (padding_height + 8))
                return false;
            if (koma == kei && destination >= width * (padding_height + 7))
                return false;
        }
        if (koma == fu)
        {
            pos_t suji = pos_to_suji(destination);

            // 二歩
            for (pos_t dan = 0; dan < dan_size; ++dan)
            {
                koma_t current = ban[suji_dan_to_pos(suji, dan)];
                if (current != empty && trim_sengo(current) == fu && sengo() == to_sengo(current))
                    return false;
            }

            // 打ち歩詰め
            pos_t pos = destination + front * (reverse(sengo()));
            if (!ban_t::out(pos) && ban[pos] != empty && trim_sengo(ban[pos]) == ou && to_sengo(ban[pos]) != sengo())
            {
                te_t te{ destination, koma };
                te_list_t te_list;
                {
                    VALIDATE_KYOKUMEN_ROLLBACK(*this);
                    const_cast<kyokumen_t &>(*this).do_te(te);
                    search_te(std::back_inserter(te_list));
                    const_cast<kyokumen_t &>(*this).undo_te(te);
                }
                if (te_list.empty())
                    return false;
            }
        }
        return true;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_source(OutputIterator result, sengo_t sengo) const
    {
        for (pos_t pos = 0; pos < pos_size; ++pos)
            if (!ban_t::out(pos) && ban[pos] != empty && to_sengo(ban[pos]) == sengo)
                *result++ = pos;
    }

    inline pos_t kyokumen_t::search(pos_t pos, pos_t offset) const
    {
        pos_t current;
        for (current = pos + offset; !ban_t::out(current) && ban[current] == empty; current += offset);
        if (ban_t::out(current))
            return npos;
        return current;
    }
    
    template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_koma_near(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const
    {
        if (pos_t current = pos + offset; !ban_t::out(current) && ban[current] != empty)
            if (is_collected(to_sengo(ban[current])) && std::find(first, last, trim_sengo(ban[current])) != last)
                *result++ = transform(current, offset, false);
    }

    template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_koma_far(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const
    {
        if (pos_t found = search(pos, offset); found != npos && found != pos + offset && ban[found] != empty)
            if (is_collected(to_sengo(ban[found])) && std::find(first, last, trim_sengo(ban[found])) != last)
                *result++ = transform(found, offset, true);
    }

    template<typename OutputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_koma(OutputIterator result, pos_t pos, sengo_t sengo, IsCollected is_collected, Transform transform) const
    {
        for (auto & [offset, koma_list] : near_kiki_list)
            search_koma_near(result, pos, offset * reverse(sengo), koma_list.begin(), koma_list.end(), is_collected, transform);
        for (auto & [offset, koma_list] : far_kiki_list_synmmetric)
            search_koma_far(result, pos, offset, koma_list.begin(), koma_list.end(), is_collected, transform);
        for (auto & [offset, koma_list] : far_kiki_list_asynmmetric)
            search_koma_far(result, pos, offset * reverse(sengo), koma_list.begin(), koma_list.end(), is_collected, transform);
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_himo(OutputIterator result, pos_t pos, sengo_t sengo) const
    {
        search_koma(result, pos, sengo,
            [sengo](sengo_t g) { return g == sengo; },
            [](pos_t pos, pos_t offset, bool aigoma) -> pos_t { return pos; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_kiki(OutputIterator result, pos_t pos, sengo_t sengo) const
    {
        search_koma(result, pos, sengo,
            [sengo](sengo_t g) { return g != sengo; },
            [](pos_t pos, pos_t offset, bool aigoma) -> kiki_t { return { pos, offset, aigoma }; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_kiki_or_himo(OutputIterator result, pos_t pos, sengo_t sengo) const
    {
        search_koma(result, pos, sengo,
            [](sengo_t) { return true; },
            [](pos_t pos, pos_t offset, bool aigoma) -> pos_t { return pos; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_check(OutputIterator result, sengo_t sengo) const
    {
        search_kiki(result, ou_pos_list[sengo], sengo);
    }

    std::vector<kiki_t> kyokumen_t::search_check(sengo_t sengo) const
    {
        std::vector<kiki_t> check_list;
        search_check(std::back_inserter(check_list), sengo);
        return check_list;
    }

    inline void kyokumen_t::push_additional_info()
    {
        push_additional_info(make_hash());
    }

    inline void kyokumen_t::push_additional_info(hash_t hash)
    {
        additional_info.check_list_stack.push_back(search_check(sengo()));
        additional_info.hash_stack.push_back(hash);
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

    inline void kyokumen_t::search_aigoma(aigoma_info_t & aigoma_info, sengo_t sengo) const
    {
        using pair = std::pair<pos_t, std::vector<koma_t>>;
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

        pos_t ou_pos = this->ou_pos_list[sengo];
        for (const auto & [offset, hashirigoma_list] : table)
        {
            pos_t reversed_offset = offset * reverse(sengo);
            pos_t first = search(ou_pos, reversed_offset);
            if (first != npos && to_sengo(ban[first]) == sengo)
            {
                pos_t second = search(first, reversed_offset);
                if (second != npos && to_sengo(ban[second]) != sengo)
                {
                    koma_t kind = trim_sengo(ban[second]);
                    bool match = std::find(hashirigoma_list.begin(), hashirigoma_list.end(), kind) != hashirigoma_list.end();
                    if (match)
                    {
                        std::vector<pos_t> candidates;
                        for (pos_t candidate = second; candidate != ou_pos; candidate -= reversed_offset)
                            candidates.push_back(candidate);
                        aigoma_info[first] = std::move(candidates);
                    }
                }
            }
        }
    }

    inline aigoma_info_t kyokumen_t::search_aigoma(sengo_t sengo) const
    {
        aigoma_info_t aigoma_info;
        search_aigoma(aigoma_info, sengo);
        return aigoma_info;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_te_from_positions(OutputIterator result, pos_t source, pos_t destination) const
    {
        if (promotable(ban[source], source, destination))
            *result++ = { source, destination, ban[source], ban[destination], true };
        if (!must_promote(ban[source], destination))
            *result++ = { source, destination, ban[source], ban[destination], false };
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_te_nonevasions(OutputIterator result) const
    {
        search_te_moves(result);
        search_te_puts(result);
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_te_evasions_ou_move(OutputIterator result) const
    {
        const pos_t source = ou_pos_list[sengo()];
        for (const pos_t * p = near_move_offsets(ou); *p; ++p)
        {
            const pos_t destination = source + *p * reverse(sengo());
            if (!ban_t::out(destination)
                && (ban[destination] == empty || to_sengo(ban[destination]) != sengo()))
            {
                const te_t te{ source, destination, ban[source], ban[destination], false };
                std::vector<kiki_t> kiki;
                {
                    VALIDATE_KYOKUMEN_ROLLBACK(*this);
                    kyokumen_t & nonconst_this = const_cast<kyokumen_t &>(*this);
                    koma_t captured = ban[destination];
                    nonconst_this.ban[destination] = ban[source];
                    nonconst_this.ban[source] = empty;
                    search_kiki(std::back_inserter(kiki), destination, sengo());
                    nonconst_this.ban[source] = ban[destination];
                    nonconst_this.ban[destination] = captured;
                }
                if (kiki.empty())
                    *result++ = te;
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_te_evasions_aigoma(OutputIterator result) const
    {
        const aigoma_info_t aigoma_info = search_aigoma(sengo());
        const pos_t ou_pos = ou_pos_list[sengo()];
        
        SHOGIPP_ASSERT(tesu < additional_info.check_list_stack.size());
        auto & check_list = additional_info.check_list_stack[tesu];
        if (check_list.size() == 1)
        {
            if (check_list.front().aigoma)
            {
                const pos_t offset = check_list.front().offset;
                for (pos_t destination = ou_pos + offset; !ban_t::out(destination) && ban[destination] == empty; destination += offset)
                {
                    // 駒を移動させる合駒
                    std::vector<kiki_t> kiki_list;
                    search_kiki(std::back_inserter(kiki_list), destination, !sengo());
                    for (kiki_t & kiki : kiki_list)
                    {
                        // 王で合駒はできない。
                        if (trim_sengo(ban[kiki.pos]) != ou)
                        {
                            // 既に合駒として使っている駒は移動できない。
                            auto aigoma_iter = aigoma_info.find(kiki.pos);
                            bool is_aigoma = aigoma_iter != aigoma_info.end();
                            if (!is_aigoma)
                                search_te_from_positions(result, kiki.pos, destination);
                        }
                    }

                    // 駒を打つ合駒
                    for (koma_t koma = fu; koma <= hi; ++koma)
                        if (mochigoma_list[sengo()][koma])
                            if (puttable(koma, destination))
                                *result++ = { destination, koma };
                }
            }

            // 王手している駒を取る手を検索する。
            const pos_t destination = check_list.front().pos;
            std::vector<kiki_t> kiki_list;
            search_kiki(std::back_inserter(kiki_list), destination, !sengo());
            for (const kiki_t & kiki : kiki_list)
            {
                // 王を動かす手は既に検索済み
                if (trim_sengo(ban[kiki.pos]) != ou)
                {
                    // 既に合駒として使っている駒は移動できない。
                    auto aigoma_iter = aigoma_info.find(kiki.pos);
                    bool is_aigoma = aigoma_iter != aigoma_info.end();
                    if (!is_aigoma)
                        search_te_from_positions(result, kiki.pos, destination);
                }
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_te_evasions(OutputIterator result) const
    {
        search_te_evasions_ou_move(result);
        search_te_evasions_aigoma(result);
    }

    /**
     * @breif 王手を外さない手のうち駒を動かす手を検索する。
     * @param result 合法手の出力イテレータ
     */
    template<typename OutputIterator>
    inline void kyokumen_t::search_te_moves(OutputIterator result) const
    {
        const aigoma_info_t aigoma_info = search_aigoma(sengo());
        std::vector<pos_t> source_list;
        source_list.reserve(pos_size);
        search_source(std::back_inserter(source_list), sengo());
        for (pos_t source : source_list)
        {
            std::vector<pos_t> destination_list;
            destination_list.reserve(pos_size);
            search_destination(std::back_inserter(destination_list), source, sengo());
            auto aigoma_iter = aigoma_info.find(source);
            bool is_aigoma = aigoma_iter != aigoma_info.end();

            for (pos_t destination : destination_list)
            {
#ifndef NDEBUG
                if (ban[destination] != empty && trim_sengo(ban[destination]) == ou)
                {
                    ban.print();
                    std::cout << pos_to_string(source) << std::endl;
                    te_t te{ source, destination, ban[source], ban[destination], false };
                    print_te(te, sengo());
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
                if (trim_sengo(ban[source]) == ou)
                {
                    std::vector<kiki_t> kiki_list;
                    search_kiki(std::back_inserter(kiki_list), destination, sengo());
                    if (kiki_list.size() > 0)
                        continue;
                }

                search_te_from_positions(result, source, destination);
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_te_puts(OutputIterator result) const
    {
        for (koma_t koma = fu; koma <= hi; ++koma)
        {
            if (mochigoma_list[sengo()][koma])
                for (pos_t destination = 0; destination < pos_size; ++destination)
                    if (puttable(koma, destination))
                        *result++ = { destination, koma };
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_te(OutputIterator result) const
    {
        SHOGIPP_ASSERT(tesu < additional_info.check_list_stack.size());
        auto & check_list = additional_info.check_list_stack[tesu];
        if (check_list.empty())
            search_te_nonevasions(result);
        else
            search_te_evasions(result);
    }

    inline te_list_t kyokumen_t::search_te() const
    {
        te_list_t te_list;
        search_te(std::back_inserter(te_list));
        return te_list;
    }

    inline hash_t kyokumen_t::make_hash() const
    {
        hash_t hash{};

        // 盤上の駒のハッシュ値をXOR演算
        for (pos_t pos = 0; pos < pos_size; ++pos)
            if (!ban_t::out(pos))
                if (koma_t koma = ban[pos]; koma != empty)
                    hash ^= hash_table.koma_hash(koma, pos);

        // 持ち駒のハッシュ値をXOR演算
        for (sengo_t sengo : sengo_list)
            for (koma_t koma = fu; koma <= hi; ++koma)
                hash ^= hash_table.mochigoma_hash(koma, mochigoma_list[sengo][koma], sengo);

        // 手番のハッシュ値をXOR演算
        hash ^= hash_table.sengo_hash(sengo());
        hash ^= hash_table.sengo_hash(!sengo());

        return hash;
    }

    inline hash_t kyokumen_t::make_hash(hash_t hash, const te_t & te) const
    {
        if (te.is_uchite())
        {
            std::size_t mochigoma_count = mochigoma_list[sengo()][te.source_koma()];
            SHOGIPP_ASSERT(mochigoma_count > 0);
            hash ^= hash_table.koma_hash(te.source_koma(), te.destination());
            hash ^= hash_table.mochigoma_hash(te.source_koma(), mochigoma_count, sengo());
            hash ^= hash_table.mochigoma_hash(te.source_koma(), mochigoma_count - 1, sengo());
        }
        else
        {
            SHOGIPP_ASSERT(!(!is_promotable(te.source_koma()) && te.promote()));
            hash ^= hash_table.koma_hash(te.source_koma(), te.source());
            if (te.captured_koma() != empty)
            {
                std::size_t mochigoma_count = mochigoma_list[sengo()][te.captured_koma()];
                hash ^= hash_table.mochigoma_hash(to_mochigoma(te.captured_koma()), mochigoma_count, sengo());
                hash ^= hash_table.mochigoma_hash(to_mochigoma(te.captured_koma()), mochigoma_count + 1, sengo());
                hash ^= hash_table.koma_hash(te.captured_koma(), te.destination());
            }
            hash ^= hash_table.koma_hash(te.promote() ? to_unpromoted(te.source_koma()) : te.source_koma(), te.destination());
        }
        hash ^= hash_table.sengo_hash(!sengo());
        hash ^= hash_table.sengo_hash(sengo());
        return hash;
    }

    inline void kyokumen_t::print_te(const te_t & te, sengo_t sengo) const
    {
        std::cout << (sengo == sente ? "▲" : "△");
        if (te.is_uchite())
        {
            std::cout << suji_to_string(pos_to_suji(te.destination())) << dan_to_string(pos_to_dan(te.destination())) << koma_to_string(trim_sengo(te.source_koma())) << "打";
        }
        else
        {
            const char * promotion_string;
            if (promotable(te.source_koma(), te.source(), te.destination()))
            {
                if (te.promote())
                    promotion_string = "成";
                else
                    promotion_string = "不成";
            }
            else
                promotion_string = "";
            std::cout
                << pos_to_string(te.destination()) << koma_to_string(trim_sengo(te.source_koma())) << promotion_string
                << "（" << pos_to_string(te.source()) << "）";
        }
    }

    template<typename InputIterator>
    inline void kyokumen_t::print_te(InputIterator first, InputIterator last) const
    {
        for (std::size_t i = 0; first != last; ++i)
        {
            std::printf("#%3zd ", i + 1);
            print_te(*first++, sengo());
            std::cout << std::endl;
        }
    }

    inline void kyokumen_t::print_te() const
    {
        te_list_t te;
        kyokumen_t temp = *this;
        temp.search_te(std::back_inserter(te));
        print_te(te.begin(), te.end());
    }

    inline void kyokumen_t::print_check() const
    {
        SHOGIPP_ASSERT(tesu < additional_info.check_list_stack.size());
        auto & check_list = additional_info.check_list_stack[tesu];
        if (!check_list.empty())
        {
            std::cout << "王手：";
            for (std::size_t i = 0; i < check_list.size(); ++i)
            {
                const kiki_t & kiki = check_list[i];
                if (i > 0)
                    std::cout << "　";
                print_pos(kiki.pos);
                std::cout << koma_to_string(trim_sengo(ban[kiki.pos])) << std::endl;
            }
        }
    }

    inline void kyokumen_t::print() const
    {
        std::cout << "後手持ち駒：";
        mochigoma_list[gote].print();
        ban.print();
        std::cout << "先手持ち駒：";
        mochigoma_list[sente].print();
    }

    inline void kyokumen_t::print_kifu() const
    {
        for (tesu_t tesu = 0; tesu < static_cast<tesu_t>(kifu.size()); ++tesu)
        {
            print_te(kifu[tesu], tesu_to_sengo(tesu));
            std::cout << std::endl;
        }
    }

    inline hash_t kyokumen_t::hash() const
    {
        SHOGIPP_ASSERT(tesu < additional_info.hash_stack.size());
        return additional_info.hash_stack[tesu];
    }

    inline void kyokumen_t::validate_ban_out()
    {
        for (pos_t pos = 0; pos < pos_size; ++pos)
        {
            if (ban_t::out(pos))
                SHOGIPP_ASSERT(ban[pos] == out_of_range);
        }
    }

    inline void kyokumen_t::do_te(const te_t & te)
    {
        hash_t hash = make_hash(this->hash(), te);
        if (te.is_uchite())
        {
            SHOGIPP_ASSERT(mochigoma_list[sengo()][te.source_koma()] > 0);
            ban[te.destination()] = sengo() == gote ? to_gote(te.source_koma()) : te.source_koma();
            --mochigoma_list[sengo()][te.source_koma()];
        }
        else
        {
            SHOGIPP_ASSERT(!(!is_promotable(te.source_koma()) && te.promote()));
            if (ban[te.destination()] != empty)
                ++mochigoma_list[sengo()][ban[te.destination()]];
            ban[te.destination()] = te.promote() ? to_promoted(ban[te.source()]) : ban[te.source()];
            ban[te.source()] = empty;
            if (trim_sengo(te.source_koma()) == ou)
                ou_pos_list[sengo()] = te.destination();
        }
        ++tesu;
        kifu.push_back(te);
        push_additional_info(hash);
        validate_ban_out();
    }

    inline void kyokumen_t::undo_te(const te_t & te)
    {
        SHOGIPP_ASSERT(tesu > 0);
        --tesu;
        if (te.is_uchite())
        {
            ++mochigoma_list[sengo()][te.source_koma()];
            ban[te.destination()] = empty;
        }
        else
        {
            if (trim_sengo(te.source_koma()) == ou)
                ou_pos_list[sengo()] = te.source();
            ban[te.source()] = te.source_koma();
            ban[te.destination()] = te.captured_koma();
            if (te.captured_koma() != empty)
                --mochigoma_list[sengo()][te.captured_koma()];
        }
        kifu.pop_back();
        pop_additional_info();
    }

    inline sengo_t kyokumen_t::sengo() const
    {
        return tesu_to_sengo(tesu);
    }

    inline unsigned long long kyokumen_t::count_node(tesu_t depth) const
    {
        if (depth == 0)
            return 1;

        unsigned long long count = 0;
        for (const te_t & te : search_te())
        {
            VALIDATE_KYOKUMEN_ROLLBACK(*this);
            const_cast<kyokumen_t &>(*this).do_te(te);
            count += count_node(depth - 1);
            const_cast<kyokumen_t &>(*this).undo_te(te);
        }
        return count;
    }

    inline void kyokumen_t::read_kyokumen_file(std::filesystem::path kyokumen_file)
    {
        static const std::string mochigoma_string = "持ち駒：";
        static const std::string nothing_string = "なし";
        static const std::string tesu_suffix = "手目";
        static const std::string sengo_suffix = "番";

        std::ifstream stream{ kyokumen_file };
        std::string line;
        pos_t dan = 0;

        kyokumen_t temp_kyokumen;

        try
        {
            while (std::getline(stream, line))
            {
                if (line.empty() || (!line.empty() && line.front() == '#'))
                    continue;

                std::string_view rest = line;
                if (rest.size() >= 1 && rest[0] >= '0' && rest[0] <= '9')
                {
                    tesu_t temp_tesu = 0;
                    do
                    {
                        temp_tesu *= 10;
                        temp_tesu += rest[0] - '0';
                        rest.remove_prefix(1);
                    } while (rest.size() >= 1 && rest[0] >= '0' && rest[0] <= '9');
                    if (temp_tesu == 0)
                        throw file_format_error{ "read_kyokumen_file 1-1" };
                    --temp_tesu;

                    parse(rest, tesu_suffix);

                    std::optional<sengo_t> sengo = parse(rest, sengo_string_map, sengo_string_size);

                    if (tesu_to_sengo(temp_tesu) != *sengo)
                        throw file_format_error{ "read_kyokumen_file 1-6" };

                    parse(rest, sengo_suffix);

                    temp_kyokumen.tesu = temp_tesu;
                    continue;
                }
                
                if (rest.size() >= 1 && rest[0] == '|')
                {
                    rest.remove_prefix(1);
                    for (pos_t suji = 0; suji < suji_size; ++suji)
                    {
                        std::optional<sengo_t> sengo = parse(rest, sengo_prefix_string_map, sengo_prefix_string_size);
                        std::optional<koma_t> koma = parse(rest, koma_string_map, koma_string_size);
                        if (*koma != empty && *sengo == gote)
                            koma = to_gote(*koma);
                        temp_kyokumen.ban[suji_dan_to_pos(suji, dan)] = *koma;
                    }
                    ++dan;
                    continue;
                }
                
                if (rest.size() >= sengo_string_size)
                {
                    std::optional<sengo_t> sengo = parse(rest, sengo_string_map, sengo_string_size);
                    if (!sengo)
                        continue;

                    parse(rest, mochigoma_string);

                    if (rest.size() >= nothing_string.size() && rest == nothing_string)
                        continue;

                    while (true)
                    {
                        mochigoma_t::size_type count = 1;
                        std::optional<koma_t> koma = parse(rest, koma_string_map, koma_string_size);
                        if (!koma)
                            break;
                        if (koma < fu)
                            throw file_format_error{ "read_kyokumen_file 2-1" };
                        if (koma > hi)
                            throw file_format_error{ "read_kyokumen_file 2-2" };

                        if (rest.size() >= digit_string_size)
                        {
                            if (digit_string_map.find(std::string{ rest.substr(0, digit_string_size) }) != digit_string_map.end())
                            {
                                mochigoma_t::size_type digit = 0;
                                do
                                {
                                    auto digit_iterator = digit_string_map.find(std::string{ rest.substr(0, digit_string_size) });
                                    if (digit_iterator == digit_string_map.end())
                                        break;
                                    digit *= 10;
                                    digit += digit_iterator->second;
                                    rest.remove_prefix(digit_string_size);
                                } while (rest.size() >= digit_string_size);
                                count = digit;
                            }
                        }
                        if (count > 18)
                            throw file_format_error{ "read_kyokumen_file 2-3" };

                        temp_kyokumen.mochigoma_list[*sengo][*koma] = count;
                    }
                }
            }
            
            temp_kyokumen.clear_additional_info();
            temp_kyokumen.push_additional_info();
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
                
                std::optional<sengo_t> sengo = parse(rest, sengo_mark_map, sengo_mark_size);
                if (!sengo)
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

                std::optional<koma_t> koma = parse(rest, koma_string_map, koma_string_size);
                if (!koma)
                    throw file_format_error{ "read_kifu_file 5" };

                if (*sengo != temp_kyokumen.sengo())
                    throw file_format_error{ "read_kifu_file 6" };

                bool promote = false;
                if (rest.size() >= uchite_string.size()
                    && rest.substr(0, uchite_string.size()) == uchite_string)
                {
                    rest.remove_prefix(uchite_string.size());
                    if (*koma < fu || *koma > hi)
                        throw file_format_error{ "read_kifu_file 7" };
                    if (!rest.empty())
                        throw file_format_error{ "read_kifu_file 8" };
                    if (temp_kyokumen.mochigoma_list[temp_kyokumen.sengo()][*koma] == 0)
                    {
                        temp_kyokumen.mochigoma_list[sente].print();
                        throw file_format_error{ "read_kifu_file 9" };
                    }
                    te_t te{ *destination, *koma };
                    temp_kyokumen.do_te(te);
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
                        if (temp_kyokumen.ban[source] == empty)
                            throw file_format_error{ "read_kifu_file 11" };
                        if (to_sengo(temp_kyokumen.ban[source]) != temp_kyokumen.sengo())
                            throw file_format_error{ "read_kifu_file 12" };
                        te_t te{ source, *destination, temp_kyokumen.ban[source], temp_kyokumen.ban[*destination], promote };
                        temp_kyokumen.do_te(te);
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
    using evaluation_value_cache_t = lru_cache_t<hash_t, evaluation_value_t, basic_hash_hash_t<SIZE_OF_HASH>>;
#else
    using evaluation_value_cache_t = lru_cache_t<hash_t, evaluation_value_t>;
#endif

    /**
     * @breif 評価関数オブジェクトのインターフェース
     */
    class abstract_evaluator_t
    {
    public:
        virtual ~abstract_evaluator_t() {};

        /**
         * @breif 局面に対して合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
         */
        virtual te_t select_te(kyokumen_t & kyokumen) = 0;

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
        te_t select_te(kyokumen_t & kyokumen)
        {
            bool selected = false;

            unsigned int id;
            te_list_t te_list;
            kyokumen.search_te(std::back_inserter(te_list));
            
            while (!selected)
            {
                try
                {
                    std::cout << "#";
                    std::cout.flush();
                    std::cin >> id;
                    if (id == 0)
                        throw invalid_command_line_input{ "invalid command line input" };
                    if (id > te_list.size())
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
            return te_list[id];
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

        for (auto & [option, params] : params_map)
            callback(option, params);
    }

    /**
     * @breif 駒と価値の連想配列から局面の点数を計算する。
     * @param kyokumen 局面
     * @param map []演算子により駒から価値を連想するオブジェクト
     * @return 局面の点数
     */
    template<typename MapKomaInt>
    inline evaluation_value_t kyokumen_map_evaluation_value(kyokumen_t & kyokumen, MapKomaInt & map)
    {
        evaluation_value_t evaluation_value = 0;

        for (pos_t pos = 0; pos < pos_size; ++pos)
        {
            koma_t koma = kyokumen.ban[pos];
            if (!ban_t::out(pos) && koma != empty)
                evaluation_value += map[trim_sengo(koma)] * reverse(to_sengo(koma));
        }

        for (sengo_t sengo : sengo_list)
            for (koma_t koma = fu; koma <= hi; ++koma)
                evaluation_value += map[koma] * kyokumen.mochigoma_list[sengo][koma] * reverse(tesu_to_sengo(sengo));

        return evaluation_value;
    }

    using evaluated_te = std::pair<te_t *, evaluation_value_t>;

    /**
     * @breif 合法手を得点により並び替える。
     * @param first scored_te の先頭を指すランダムアクセスイテレータ
     * @param last scored_te の末尾を指すランダムアクセスイテレータ
     * @param sengo 先手か後手か
     * @details 評価値の符号を手番により変更するようにしたため、現在この関数を使用する予定はない。
     */
    template<typename RandomAccessIterator>
    void sort_te_by_evaluation_value(RandomAccessIterator first, RandomAccessIterator last, sengo_t sengo)
    {
        using comparator = bool (const evaluated_te & a, const evaluated_te & b);
        comparator * const map[]{
            [](const evaluated_te & a, const evaluated_te & b) -> bool { return a.second > b.second; },
            [](const evaluated_te & a, const evaluated_te & b) -> bool { return a.second < b.second; }
        };
        std::sort(first, last, map[sengo]);
    }

    /**
     * @breif 合法手を区分により並び替える。
     * @param first scored_te の先頭を指すランダムアクセスイテレータ
     * @param last scored_te の末尾を指すランダムアクセスイテレータ
     * @details 駒取りが発生する手、駒取りが発生しない手、打つ手の順に並び替える。
     */
    template<typename RandomAccessIterator>
    void sort_te_by_category(RandomAccessIterator first, RandomAccessIterator last)
    {
        std::sort(first, last, [](const te_t & a, const te_t & b) -> bool { return to_category(a) >= to_category(b); });
    }

    /**
     * @breif 合法手を得点により並び替える。
     * @param first scored_te の先頭を指すランダムアクセスイテレータ
     * @param last scored_te の末尾を指すランダムアクセスイテレータ
     */
    template<typename RandomAccessIterator>
    void sort_te_by_evaluation_value(RandomAccessIterator first, RandomAccessIterator last)
    {
        std::sort(first, last, [](const evaluated_te & a, const evaluated_te & b) -> bool { return a.second > b.second; });
    }

    /**
     * @breif negamax で合法手を選択する評価関数オブジェクトの抽象クラス
     */
    class negamax_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        evaluation_value_t negamax(
            kyokumen_t & kyokumen,
            depth_t depth,
            unsigned int & search_count,
            std::optional<te_t> & selected_te
        )
        {
            if (depth <= 0)
            {
                ++search_count;
                std::optional<evaluation_value_t> cached_evaluation_value = evaluation_value_cache.get(kyokumen.hash());
                if (cached_evaluation_value)
                {
                    ++cache_hit_count;
                    return *cached_evaluation_value;
                }
                evaluation_value_t evaluation_value = eval(kyokumen) * reverse(kyokumen.sengo());
                evaluation_value_cache.push(kyokumen.hash(), evaluation_value);
                return evaluation_value;
            }

            te_list_t te_list;
            kyokumen.search_te(std::back_inserter(te_list));

            if (te_list.empty())
                return -std::numeric_limits<evaluation_value_t>::max();

            std::vector<evaluated_te> evaluated_te_list;
            auto inserter = std::back_inserter(evaluated_te_list);

            for (te_t & te : te_list)
            {
                std::optional<te_t> selected_te_;
                evaluation_value_t evaluation_value;
                {
                    VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                    kyokumen.do_te(te);
                    evaluation_value = -negamax(kyokumen, depth - 1, search_count, selected_te_);
                    kyokumen.undo_te(te);
                }
                *inserter++ = { &te, evaluation_value };
            }

            SHOGIPP_ASSERT(!te_list.empty());
            sort_te_by_evaluation_value(evaluated_te_list.begin(), evaluated_te_list.end());
            selected_te = *evaluated_te_list.front().first;
            return evaluated_te_list.front().second;
        }

        /**
         * @breif 局面に対して minimax で合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            cache_hit_count = 0;
            evaluation_value_cache.clear();
            unsigned int search_count = 0;
            depth_t default_max_depth = 3;
            std::optional<te_t> selected_te;
            evaluation_value_t evaluation_value = negamax(kyokumen, default_max_depth, search_count, selected_te);
            details::timer.search_count() += search_count;
            std::cout << "読み手数：" << search_count << std::endl;
            std::cout << "読み手数（キャッシュ）：" << cache_hit_count << std::endl;
            std::cout << "評価値：" << evaluation_value << std::endl;
            SHOGIPP_ASSERT(selected_te.has_value());
            return *selected_te;
        }

        /**
         * @breif 局面に対して評価値を返す。
         * @param kyokumen 局面
         * @return 局面の評価値
         */
        virtual evaluation_value_t eval(kyokumen_t & kyokumen) = 0;

        unsigned long long cache_hit_count = 0;
        evaluation_value_cache_t evaluation_value_cache{ std::numeric_limits<std::size_t>::max() };
    };

    /**
     * @breif alphabeta で合法手を選択する評価関数オブジェクトの抽象クラス
     */
    class alphabeta_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        evaluation_value_t alphabeta(
            kyokumen_t & kyokumen,
            depth_t depth,
            evaluation_value_t alpha,
            evaluation_value_t beta,
            unsigned int & search_count,
            std::optional<te_t> & selected_te)
        {
            if (depth <= 0)
            {
                ++search_count;
                std::optional<evaluation_value_t> cached_evaluation_value = evaluation_value_cache.get(kyokumen.hash());
                if (cached_evaluation_value)
                {
                    ++cache_hit_count;
                    return *cached_evaluation_value;
                }
                evaluation_value_t evaluation_value = eval(kyokumen) * reverse(kyokumen.sengo());
                evaluation_value_cache.push(kyokumen.hash(), evaluation_value);
                return evaluation_value;
            }

            te_list_t te_list;
            kyokumen.search_te(std::back_inserter(te_list));
            sort_te_by_category(te_list.begin(), te_list.end());

            if (te_list.empty())
                return -std::numeric_limits<evaluation_value_t>::max();

            std::vector<evaluated_te> evaluated_te_list;
            auto inserter = std::back_inserter(evaluated_te_list);

            for (te_t & te : te_list)
            {
                std::optional<te_t> selected_te_;
                evaluation_value_t evaluation_value;
                {
                    VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                    kyokumen.do_te(te);
                    evaluation_value = -alphabeta(kyokumen, depth - 1, -beta, -alpha, search_count, selected_te_);
                    kyokumen.undo_te(te);
                }
                *inserter++ = { &te, evaluation_value };
                alpha = std::max(alpha, evaluation_value);
                if (alpha >= beta)
                    break;
            }

            SHOGIPP_ASSERT(!te_list.empty());
            sort_te_by_evaluation_value(evaluated_te_list.begin(), evaluated_te_list.end());
            selected_te = *evaluated_te_list.front().first;
            return evaluated_te_list.front().second;
        }

        /**
         * @breif 局面に対して minimax で合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            cache_hit_count = 0;
            evaluation_value_cache.clear();
            unsigned int search_count = 0;
            depth_t default_max_depth = 3;
            std::optional<te_t> selected_te;
            evaluation_value_t evaluation_value = alphabeta(kyokumen, default_max_depth, -std::numeric_limits<evaluation_value_t>::max(), std::numeric_limits<evaluation_value_t>::max(), search_count, selected_te);
            details::timer.search_count() += search_count;
            std::cout << "読み手数：" << search_count << std::endl;
            std::cout << "読み手数（キャッシュ）：" << cache_hit_count << std::endl;
            std::cout << "評価値：" << evaluation_value << std::endl;
            SHOGIPP_ASSERT(selected_te.has_value());
            return *selected_te;
        }

        /**
         * @breif 局面に対して評価値を返す。
         * @param kyokumen 局面
         * @return 局面の評価値
         */
        virtual evaluation_value_t eval(kyokumen_t & kyokumen) = 0;
        
        unsigned long long cache_hit_count = 0;
        evaluation_value_cache_t evaluation_value_cache{ std::numeric_limits<std::size_t>::max() };
    };

    /**
     * @breif alphabeta で合法手を選択する評価関数オブジェクトの抽象クラス
     * @details 前回駒取りが発生していた場合、探索を延長する。
     */
    class extendable_alphabeta_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        evaluation_value_t extendable_alphabeta(
            kyokumen_t & kyokumen,
            depth_t depth,
            evaluation_value_t alpha,
            evaluation_value_t beta,
            unsigned int & search_count,
            std::optional<te_t> & selected_te,
            pos_t previous_destination)
        {
            if (depth <= 0)
            {
                // 前回駒取りが発生していた場合、探索を延長する。
                if (previous_destination != npos)
                {
                    std::vector<evaluated_te> evaluated_te_list;
                    auto inserter = std::back_inserter(evaluated_te_list);
                    te_list_t te_list;
                    kyokumen.search_te(std::back_inserter(te_list));
                    for (te_t & te : te_list)
                    {
                        if (!te.is_uchite() && te.destination() == previous_destination)
                        {
                            std::optional<te_t> selected_te_;
                            evaluation_value_t evaluation_value;
                            {
                                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                                kyokumen.do_te(te);
                                evaluation_value = -extendable_alphabeta(kyokumen, depth - 1, -beta, -alpha, search_count, selected_te_, previous_destination);
                                kyokumen.undo_te(te);
                            }
                            *inserter++ = { &te, evaluation_value };
                            alpha = std::max(alpha, evaluation_value);
                            if (alpha >= beta)
                                break;
                        }
                    }
                    if (!evaluated_te_list.empty())
                    {
                        sort_te_by_evaluation_value(evaluated_te_list.begin(), evaluated_te_list.end());
                        selected_te = *evaluated_te_list.front().first;
                        return evaluated_te_list.front().second;
                    }
                }
                ++search_count;
                return eval(kyokumen) * reverse(kyokumen.sengo());
            }

            te_list_t te_list;
            kyokumen.search_te(std::back_inserter(te_list));
            sort_te_by_category(te_list.begin(), te_list.end());

            if (te_list.empty())
                return -std::numeric_limits<evaluation_value_t>::max();

            std::vector<evaluated_te> evaluated_te_list;
            auto inserter = std::back_inserter(evaluated_te_list);

            for (te_t & te : te_list)
            {
                std::optional<te_t> selected_te_;
                pos_t destination = (!te.is_uchite() && te.captured_koma() != empty) ? te.destination() : npos;
                evaluation_value_t evaluation_value;
                {
                    VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                    kyokumen.do_te(te);
                    evaluation_value = -extendable_alphabeta(kyokumen, depth - 1, -beta, -alpha, search_count, selected_te_, destination);
                    kyokumen.undo_te(te);
                }
                *inserter++ = { &te, evaluation_value };
                alpha = std::max(alpha, evaluation_value);
                if (alpha >= beta)
                    break;
            }

            SHOGIPP_ASSERT(!te_list.empty());
            sort_te_by_evaluation_value(evaluated_te_list.begin(), evaluated_te_list.end());
            selected_te = *evaluated_te_list.front().first;
            return evaluated_te_list.front().second;
        }

        /**
         * @breif 局面に対して minimax で合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            unsigned int search_count = 0;
            depth_t default_max_depth = 3;
            std::optional<te_t> selected_te;
            evaluation_value_t evaluation_value = extendable_alphabeta(kyokumen, default_max_depth, -std::numeric_limits<evaluation_value_t>::max(), std::numeric_limits<evaluation_value_t>::max(), search_count, selected_te, npos);
            details::timer.search_count() += search_count;
            std::cout << "読み手数：" << search_count << std::endl;
            std::cout << "評価値：" << evaluation_value << std::endl;
            SHOGIPP_ASSERT(selected_te.has_value());
            return *selected_te;
        }

        /**
         * @breif 局面に対して評価値を返す。
         * @param kyokumen 局面
         * @return 局面の評価値
         */
        virtual evaluation_value_t eval(kyokumen_t & kyokumen) = 0;
    };

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
        te_t select_te(kyokumen_t & kyokumen) override
        {
            te_list_t te_list;
            kyokumen.search_te(std::back_inserter(te_list));

            std::vector<evaluated_te> scores;
            auto back_inserter = std::back_inserter(scores);
            for (te_t & t : te_list)
            {
                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                kyokumen.do_te(t);
                *back_inserter++ = { &t, eval(kyokumen) };
                kyokumen.undo_te(t);
            }

            std::sort(scores.begin(), scores.end(), [](auto & a, auto & b) { return a.second > b.second; });
            return *scores.front().first;
        }

        /**
         * @breif 局面に対して評価値を返す。
         * @param kyokumen 局面
         * @return 局面の評価値
         */
        virtual evaluation_value_t eval(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif 評価関数オブジェクトの実装例
     */
    class sample_evaluator_t
        : public alphabeta_evaluator_t
    {
    public:
        evaluation_value_t eval(kyokumen_t & kyokumen) override
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
        evaluation_value_t eval(kyokumen_t & kyokumen) override
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
        evaluation_value_t eval(kyokumen_t & kyokumen) override
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
        evaluation_value_t eval(kyokumen_t & kyokumen) override
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
                if (!ban_t::out(pos) && kyokumen.ban[pos] != empty)
                {
                    std::vector<kiki_t> kiki_list;
                    sengo_t sengo = to_sengo(kyokumen.ban[pos]);
                    kyokumen.search_kiki(std::back_inserter(kiki_list), pos, sengo);
                    evaluation_value += kiki_point * static_cast<evaluation_value_t>(kiki_list.size()) * reverse(sengo);
                    std::vector<pos_t> himo_list;
                    kyokumen.search_himo(std::back_inserter(himo_list), pos, sengo);
                    evaluation_value += himo_point * static_cast<evaluation_value_t>(himo_list.size()) * reverse(sengo);

                    std::vector<pos_t> destination_list;
                    kyokumen.search_destination(std::back_inserter(destination_list), pos, sengo);
                    evaluation_value += destination_point * static_cast<evaluation_value_t>(destination_list.size()) * reverse(sengo);
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
        inline evaluation_value_t eval(kyokumen_t & kyokumen) override
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
        };

        id_t id{ id_t::error };
        std::optional<te_t> opt_te;
        std::optional<tesu_t> opt_depth;
    };

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
     * @breif 標準入力からコマンドを読み込む。
     * @param te_list 合法手
     * @return 読み込まれたコマンド
     */
    inline command_t read_command_line_input(const te_list_t & te_list)
    {
        std::string command_line;
        while (true)
        {
            try
            {
                std::getline(std::cin, command_line);

                std::vector<std::string> tokens;
                split_tokens(std::back_inserter(tokens), std::string_view{ command_line });

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
                        tesu_t depth;
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
                    if (move_index > te_list.size())
                        throw invalid_command_line_input{ "move_index > te_list.size()" };
                    const std::size_t raw_move_index = move_index - 1;
                    command_t command;
                    command.id = command_t::id_t::move;
                    command.opt_te = te_list[raw_move_index];
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
        inline const te_list_t & get_te_list() const;

        /**
         * @breif 手番の合法手を更新する。
         */
        inline void update_te_list() const;

        std::shared_ptr<abstract_kishi_t> kishi_list[sengo_size];
        mutable te_list_t te_list;
        kyokumen_t kyokumen;
        bool sente_win;
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
        return read_command_line_input(taikyoku.get_te_list());
    }

    const char * stdin_kishi_t::name() const
    {
        return "stdin";
    }

    command_t computer_kishi_t::get_command(taikyoku_t & taikyoku)
    {
        return command_t{ command_t::id_t::move, ptr->select_te(taikyoku.kyokumen) };
    }

    const char * computer_kishi_t::name() const
    {
        return ptr->name();
    }

    inline taikyoku_t::taikyoku_t(const std::shared_ptr<abstract_kishi_t> & a, const std::shared_ptr<abstract_kishi_t> & b)
        : kishi_list{ a, b }
        , sente_win{ false }
    {
        update_te_list();
    }

    inline bool taikyoku_t::procedure()
    {
        auto & kishi = kishi_list[kyokumen.sengo()];

        kyokumen_t temp_kyokumen = kyokumen;

        while (true)
        {
            command_t cmd = kishi->get_command(*this);
            switch (cmd.id)
            {
            case command_t::id_t::error:
                break;
            case command_t::id_t::move:
                kyokumen.print_te(*cmd.opt_te, kyokumen.sengo());
                std::cout << std::endl << std::endl;
                kyokumen.do_te(*cmd.opt_te);
                update_te_list();
                return !te_list.empty();
            case command_t::id_t::undo:
                if (kyokumen.tesu >= 2)
                {
                    for (int i = 0; i < 2; ++i)
                        kyokumen.undo_te(kyokumen.kifu.back());
                    update_te_list();
                    return !te_list.empty();
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
            }
        }
    }

    inline void taikyoku_t::print() const
    {
        if (kyokumen.tesu == 0)
        {
            for (sengo_t sengo : sengo_list)
                std::cout << sengo_to_string(static_cast<sengo_t>(sengo)) << "：" << kishi_list[sengo]->name() << std::endl;
            std::cout << std::endl;
        }

        if (te_list.empty())
        {
            auto & winner_evaluator = kishi_list[!kyokumen.sengo()];
            std::cout << kyokumen.tesu << "手詰み" << std::endl;
            kyokumen.print();
            std::cout << sengo_to_string(!kyokumen.sengo()) << "勝利（" << winner_evaluator->name() << "）";
            std::cout.flush();
        }
        else
        {
            std::cout << (kyokumen.tesu + 1) << "手目" << sengo_to_string(kyokumen.sengo()) << "番" << std::endl;
            kyokumen.print();
            std::cout << hash_to_string(kyokumen.hash()) << std::endl;
            kyokumen.print_te();
            kyokumen.print_check();
        }
    }

    inline const te_list_t & taikyoku_t::get_te_list() const
    {
        return te_list;
    }

    inline void taikyoku_t::update_te_list() const
    {
        te_list.clear();
        kyokumen.search_te(std::back_inserter(te_list));
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

    static const std::map<std::string, std::shared_ptr<abstract_kishi_t>> kishi_map
    {
        {"stdin"   , std::make_shared<stdin_kishi_t>()},
        {"random"  , std::make_shared<computer_kishi_t>(std::make_shared<random_evaluator_t  >())},
        {"sample"  , std::make_shared<computer_kishi_t>(std::make_shared<sample_evaluator_t  >())},
        {"hiyoko"  , std::make_shared<computer_kishi_t>(std::make_shared<hiyoko_evaluator_t  >())},
        {"niwatori", std::make_shared<computer_kishi_t>(std::make_shared<niwatori_evaluator_t>())},
        {"fukayomi", std::make_shared<computer_kishi_t>(std::make_shared<fukayomi_evaluator_t>())},
    };

    inline int parse_command_line(int argc, const char ** argv)
    {
        try
        {
            std::string kifu_path;
            std::string kyokumen_path;
            std::string sente_name = DEFAULT_SENTE_KISHI;
            std::string gote_name = DEFAULT_GOTE_KISHI;

            std::shared_ptr<abstract_evaluator_t> a, b;

            auto callback = [&](const std::string & option, const std::vector<std::string> & params)
            {
                if (option == "kifu" && !params.empty())
                {
                    kifu_path = params[0];
                }
                else if (option == "kyokumen" && !params.empty())
                {
                    kyokumen_path = params[0];
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

            auto sente_iter = kishi_map.find(sente_name);
            if (sente_iter == kishi_map.end())
            {
                throw invalid_command_line_input{"invalid sente name"};
            }
            const std::shared_ptr<abstract_kishi_t> & sente_kishi = sente_iter->second;

            auto gote_iter = kishi_map.find(gote_name);
            if (gote_iter == kishi_map.end())
            {
                throw invalid_command_line_input{ "invalid gote name" };
            }
            const std::shared_ptr<abstract_kishi_t> & gote_kishi = gote_iter->second;

            if (!kifu_path.empty() && !kyokumen_path.empty())
            {
                throw invalid_command_line_input{ "!kifu.empty() && !kyokumen.empty()" };
            }
            else if (!kifu_path.empty())
            {
                kyokumen_t kyokumen;
                kyokumen.read_kifu_file(kifu_path);
                do_taikyoku(kyokumen, sente_kishi, gote_kishi);
            }
            else if (!kyokumen_path.empty())
            {
                kyokumen_t kyokumen;
                kyokumen.read_kyokumen_file(kyokumen_path);
                do_taikyoku(kyokumen, sente_kishi, gote_kishi);
            }
            else
            {
                do_taikyoku(sente_kishi, gote_kishi);
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