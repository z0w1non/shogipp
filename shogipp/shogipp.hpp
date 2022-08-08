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

#undef NDEBUG

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
#define SHOGIPP_ASSERT(expr) (void)0
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

# define    SHOGIPP_STRING_LITERAL_IMPL(name, s, CharT, prefix)         \
/*define*/      template<>                                              \
/*define*/      struct name ## _impl<CharT>                             \
/*define*/      {                                                       \
/*define*/          inline const CharT * operator()() const             \
/*define*/          {                                                   \
/*define*/              return prefix ## s;                             \
/*define*/          }                                                   \
/*define*/      };                                                      \

# define    SHOGIPP_STRING_LITERAL(name, s)                                      \
/*define*/      template<typename CharT>                                         \
/*define*/      struct name ## _impl;                                            \
/*define*/      SHOGIPP_STRING_LITERAL_IMPL(name, s, char, )                     \
/*define*/      SHOGIPP_STRING_LITERAL_IMPL_U8                                   \
/*define*/      SHOGIPP_STRING_LITERAL_IMPL(name, s, char16_t, u)                \
/*define*/      SHOGIPP_STRING_LITERAL_IMPL(name, s, char32_t, U)                \
/*define*/      SHOGIPP_STRING_LITERAL_IMPL(name, s, wchar_t, L)                 \
/*define*/      template<typename CharT>                                         \
/*define*/      inline const CharT * name() { return name ## _impl<CharT>{}(); } \

namespace shogipp
{
    using search_count_t = unsigned long long;
    using move_count_t = unsigned int;
    using depth_t = unsigned int;
    using evaluation_value_t = int;
    using pruning_threshold_t = unsigned int;
    using iddfs_iteration_t = unsigned int;

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
        * @param func __pawnnc__
        * @param line __LINE__
        */
        inline constexpr void assert_impl(bool assertion, const char * expr, const char * file, const char * func, unsigned int line) noexcept
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
            inline timer_t() noexcept;

            /**
             * @breif 時間計測を再開始する。
             */
            inline void clear() noexcept;

            /**
             * @breif 経過時間を標準出力に出力する。
             */
            inline void print_elapsed_time(std::ostream & ostream = std::cout) noexcept;

            /**
             * @breif 読み手数の参照を返す。
             * @return 読み手数の参照
             */
            inline search_count_t & search_count() noexcept;

            /**
             * @breif 読み手数の参照を返す。
             * @return 読み手数の参照
             */
            inline const search_count_t & search_count() const noexcept;

        private:
            std::chrono::system_clock::time_point m_begin;
            search_count_t m_search_count;
        };

        inline timer_t::timer_t() noexcept
        {
            clear();
        }

        inline void timer_t::clear() noexcept
        {
            m_begin = std::chrono::system_clock::now();
            m_search_count = 0;
        }

        inline void timer_t::print_elapsed_time(std::ostream & ostream) noexcept
        {
            const std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            const std::chrono::milliseconds::rep duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_begin).count();
            const search_count_t nps = duration != 0 ? m_search_count * 1000 / duration : 0;

            ostream
                << std::endl
                << "総読み手数     : " << m_search_count << std::endl
                << "実行時間[ms]   : " << duration << std::endl
                << "読み手速度[n/s]: " << nps << std::endl << std::endl;
        }

        inline search_count_t & timer_t::search_count() noexcept
        {
            return m_search_count;
        }

        inline const search_count_t & timer_t::search_count() const noexcept
        {
            return m_search_count;
        }

        thread_local timer_t timer;

        /**
         * @breif std::string_view の先頭から続く空白を削除する。
         */
        inline void trim_front_space(std::string_view & sv) noexcept
        {
            while (!sv.empty() && sv.front() == ' ')
                sv.remove_prefix(1);
        }

        /**
         * @breif std::string_view の先頭から予測された文字列を削除することを試みる。
         * @param sv 文字列
         * @param token 予測された文字列
         * @retval true 予測された文字列の削除に成功した場合
         * @retval false 予測された文字列の削除に成功しなかった場合
         */
        inline bool try_parse(std::string_view & sv, std::string_view token) noexcept
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

        template<std::size_t N>
        constexpr inline std::uint32_t bool_array_to_32bitmask(const bool(&bool_array)[N]) noexcept
        {
            static_assert(N < 32);
            std::uint32_t bitmask = 0;
            for (std::size_t i = 0; i < N; ++i)
                if (bool_array[i])
                    bitmask |= (1 << i);
            return bitmask;
        }

        constexpr inline bool bitmask_has(std::int32_t bitmask, unsigned int index) noexcept
        {
            return ((bitmask >> index) & 1) != 0;
        }

        template<typename Function>
        inline std::chrono::milliseconds test_time_performance(Function && func, std::size_t n)
        {
            const std::chrono::system_clock::time_point begin = std::chrono::system_clock::now();
            for (std::size_t i = 0; i < n; ++i)
                func();
            const std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
        }

        /**
         * @breif 乱数を返す。
         * @return 乱数
         */
        template<typename T>
        inline T random(int min = std::numeric_limits<T>::min(), int max = std::numeric_limits<T>::max())
        {
            static std::mt19937 random_impl{ SHOGIPP_SEED };
            std::uniform_int_distribution<> uid{ min, max };
            return uid(random_impl);
        }

        /**
         * @breif バイト値を一様交叉する。
         * @param a 値1
         * @param b 値2
         * @return 一様交叉された値
         */
        inline unsigned char uniform_croossover(unsigned char a, unsigned char b)
        {
            unsigned char result = 0;
            const unsigned char random_byte = random<unsigned char>();
            result |= (a & random_byte);
            result |= (b & (~random_byte));
            return result;
        }

        /**
         * @breif 文字列を bool 値に変換する。
         * @param s 文字列
         * @return bool 値
         */
        inline std::optional<bool> to_bool(std::string_view s) noexcept
        {
            if (s == "true" || s == "enable" || s == "1")
                return true;
            if (s == "false" || s == "disable" || s == "0")
                return false;
            return std::nullopt;
        }

        template<typename T>
        inline std::size_t count_bit(T value) noexcept
        {
            std::size_t count = 0;
            for (std::size_t i = 0; i < sizeof(value) * CHAR_BIT; ++i)
                if (value & (1 << i))
                    ++count;
            return count;
        }

        template<typename Integer>
        std::optional<Integer> cast_to(const std::string & s) noexcept
        {
            try
            {
                Integer integer;
                std::istringstream stream{ s };
                stream >> integer;
                return integer;
            }
            catch (...)
            {
                ;
            }
            return std::nullopt;
        }

        constexpr double power(double x, unsigned int y) noexcept
        {
            if (y == 0)
                return 1.0;
            else if (y == 1)
                return x;
            return x * power(x, y - 1);
        }

        namespace program_options
        {
            constexpr bool default_print_move = false;
            bool print_moves = default_print_move;

            constexpr bool default_print_check = true;
            bool print_check = default_print_check;

            constexpr bool default_print_board = true;
            bool print_board = default_print_board;

            constexpr std::chrono::milliseconds default_limit_time{ 10 * 1000 };
            std::chrono::milliseconds limit_time = default_limit_time;

            constexpr depth_t default_max_iddfs_iteration = 1;
            depth_t max_iddfs_iteration = default_max_iddfs_iteration;

            const std::string default_ga_logs_directory = "logs";
            std::string ga_logs_directory = default_ga_logs_directory;

            const std::string default_ga_chromosomes_directory = "chromosomes";
            std::string ga_chromosomes_directory = default_ga_chromosomes_directory;

            std::optional<std::string> black_name;
            std::optional<std::string> white_name;
            std::optional<unsigned long long> ga_iteration;
            std::optional<unsigned int> ga_create_chromosome;
            std::optional<std::string> ga_create_mode;
            std::optional<unsigned int> ga_mutation_rate;
            std::optional<unsigned int> ga_crossover_rate;
            std::optional<unsigned int> ga_selection_rate;
            std::optional<unsigned int> ga_max_move_count;
            std::optional<unsigned int> ga_min_chromosome_distance;
            std::optional<unsigned int> ga_elite_number;
            std::optional<std::string> ga_dump_chromosome;
            std::optional<unsigned int> ga_mutation_number;

            constexpr unsigned int default_ga_thread_number = 1;
            unsigned int ga_thread_number = default_ga_thread_number;

            std::optional<std::string> sfen;

            const std::string default_piece_pair_statistics = "piece_pair_statistics.bin";
            std::string piece_pair_statistics = default_piece_pair_statistics;
        } // namespace program_options

        namespace evaluation_value_template
        {
            constexpr evaluation_value_t board_pawn            =  100;
            constexpr evaluation_value_t board_lance           =  300;
            constexpr evaluation_value_t board_knight          =  400;
            constexpr evaluation_value_t board_silver          =  500;
            constexpr evaluation_value_t board_gold            =  600;
            constexpr evaluation_value_t board_bishop          =  800;
            constexpr evaluation_value_t board_rook            = 1000;
            constexpr evaluation_value_t board_promoted_pawn   =  700;
            constexpr evaluation_value_t board_promoted_lance  =  600;
            constexpr evaluation_value_t board_promoted_knight =  600;
            constexpr evaluation_value_t board_promoted_silver =  600;
            constexpr evaluation_value_t board_promoted_bishop = 1000;
            constexpr evaluation_value_t board_promoted_rook   = 1200;

            constexpr double captured_bonus = 1.125;

            constexpr evaluation_value_t captured_pawn   = static_cast<evaluation_value_t>(board_pawn   * captured_bonus);
            constexpr evaluation_value_t captured_lance  = static_cast<evaluation_value_t>(board_lance  * captured_bonus);
            constexpr evaluation_value_t captured_knight = static_cast<evaluation_value_t>(board_knight * captured_bonus);
            constexpr evaluation_value_t captured_silver = static_cast<evaluation_value_t>(board_silver * captured_bonus);
            constexpr evaluation_value_t captured_gold   = static_cast<evaluation_value_t>(board_gold   * captured_bonus);
            constexpr evaluation_value_t captured_bishop = static_cast<evaluation_value_t>(board_bishop * captured_bonus);
            constexpr evaluation_value_t captured_rook   = static_cast<evaluation_value_t>(board_rook   * captured_bonus);

            constexpr evaluation_value_t map[]
            {
                /* empty           */ 0, /* dummy */
                /* pawn            */ board_pawn,
                /* lance           */ board_lance,
                /* knight          */ board_knight,
                /* silver          */ board_silver,
                /* gold            */ board_gold,
                /* bishop          */ board_bishop,
                /* rook            */ board_rook,
                /* king            */ 0, /* dummy */
                /* promoted_pawn   */ board_promoted_pawn,
                /* promoted_lance  */ board_promoted_lance,
                /* promoted_knight */ board_promoted_knight,
                /* promoted_silver */ board_promoted_silver,
                /* promoted_bishop */ board_promoted_bishop,
                /* promoted_rook   */ board_promoted_rook,
            };
        }
    } // namespace details

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

    class timeout_exception
        : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    class color_t
    {
    public:
        using value_type = unsigned char;
        
        constexpr inline color_t(value_type value) noexcept;
        constexpr inline color_t operator !() const noexcept { return m_value != 0 ? 0 : 1; }
        constexpr inline bool operator ==(const color_t & color) const noexcept { return m_value == color.m_value; }
        constexpr inline bool operator !=(const color_t & color) const noexcept { return m_value != color.m_value; }
        constexpr inline value_type value() const noexcept { return m_value; }
        constexpr inline static std::size_t min() noexcept { return 0; }
        constexpr inline static std::size_t max() noexcept { return 1; }
        constexpr inline static std::size_t size() noexcept { return 2; }

    private:
        value_type m_value;
    };

    constexpr inline color_t::color_t(value_type value) noexcept
        : m_value{ value }
    {
    }

    constexpr color_t black{ 0 };
    constexpr color_t white{ 1 };

    constexpr color_t colors[]
    {
        black,
        white
    };

    using piece_value_t = unsigned char;
    constexpr piece_value_t color_distance       { 0x0E };
    constexpr piece_value_t piece_size           { 0x1D };
    constexpr piece_value_t empty_value          { 0x00 };
    constexpr piece_value_t pawn_value           { 0x01 };
    constexpr piece_value_t lance_value          { 0x02 };
    constexpr piece_value_t knight_value         { 0x03 };
    constexpr piece_value_t silver_value         { 0x04 };
    constexpr piece_value_t gold_value           { 0x05 };
    constexpr piece_value_t bishop_value         { 0x06 };
    constexpr piece_value_t rook_value           { 0x07 };
    constexpr piece_value_t king_value           { 0x08 };
    constexpr piece_value_t promoted_pawn_value  { 0x09 };
    constexpr piece_value_t promoted_lance_value { 0x0A };
    constexpr piece_value_t promoted_knight_value{ 0x0B };
    constexpr piece_value_t promoted_silver_value{ 0x0C };
    constexpr piece_value_t promoted_bishop_value{ 0x0D };
    constexpr piece_value_t promoted_rook_value  { 0x0E };
    constexpr piece_value_t out_of_range_value   { 0xFF };

    /**
     * @breif 駒を文字列に変換する。
     * @param piece 駒
     * @return 文字列
     */
    inline const char * to_string_impl(piece_value_t piece) noexcept
    {
        static const char * map[]{
            "・",
            "歩", "香", "桂", "銀", "金", "角", "飛", "王", "と", "杏", "圭", "全", "馬", "竜",
            "歩", "香", "桂", "銀", "金", "角", "飛", "王", "と", "杏", "圭", "全", "馬", "竜",
        };
        SHOGIPP_ASSERT(piece < std::size(map));
        return map[piece];
    }

    class basic_piece_t
    {
    public:
        constexpr inline basic_piece_t() noexcept;
        constexpr inline basic_piece_t(piece_value_t value) noexcept;
        constexpr inline piece_value_t value() const noexcept;

        /**
         * @breif 駒が空であるか判定する。
         * @retval true 駒が空である
         * @retval false 駒が空でない
         */
        constexpr inline bool empty() const noexcept;

        /**
         * @breif 駒を文字列に変換する。
         * @param piece 駒
         * @return 文字列
         */
        inline const char * to_string() const noexcept;
    private:
        piece_value_t m_value;
    };

    constexpr inline basic_piece_t::basic_piece_t() noexcept
        : m_value{ empty_value }
    {
    }

    constexpr inline basic_piece_t::basic_piece_t(piece_value_t value) noexcept
        : m_value{ value }
    {
    }

    constexpr inline piece_value_t basic_piece_t::value() const noexcept
    {
        return m_value;
    }

    constexpr inline bool basic_piece_t::empty() const noexcept
    {
        return m_value == empty_value;
    }

    inline const char * basic_piece_t::to_string() const noexcept
    {
        return to_string_impl(value());
    }

    class captured_piece_t;
    class noncolored_piece_t;
    class colored_piece_t;

    class captured_piece_t
        : public basic_piece_t
    {
    public:
        using basic_piece_t::basic_piece_t;
        constexpr inline captured_piece_t(const colored_piece_t & piece) noexcept;
        constexpr inline bool operator ==(const captured_piece_t & piece) const noexcept { return value() == piece.value(); }
        constexpr inline bool operator !=(const captured_piece_t & piece) const noexcept { return value() != piece.value(); }
        constexpr inline bool operator <(const captured_piece_t & piece) const noexcept { return value() < piece.value(); }
    };

    class noncolored_piece_t
        : public basic_piece_t
    {
    public:
        using basic_piece_t::basic_piece_t;
        constexpr inline noncolored_piece_t(const colored_piece_t & piece) noexcept;
        constexpr inline bool operator ==(const noncolored_piece_t & piece) const noexcept { return value() == piece.value(); }
        constexpr inline bool operator !=(const noncolored_piece_t & piece) const noexcept { return value() != piece.value(); }
        constexpr inline bool operator <(const noncolored_piece_t & piece) const noexcept { return value() < piece.value(); }
    };

    class colored_piece_t
        : public basic_piece_t
    {
    public:
        using basic_piece_t::basic_piece_t;
        constexpr inline colored_piece_t(const captured_piece_t & piece, color_t color) noexcept;
        constexpr inline colored_piece_t(const noncolored_piece_t & piece, color_t color) noexcept;
        constexpr inline bool operator ==(const colored_piece_t & piece) const noexcept { return value() == piece.value(); }
        constexpr inline bool operator !=(const colored_piece_t & piece) const noexcept { return value() != piece.value(); }
        constexpr inline bool operator <(const colored_piece_t & piece) const noexcept { return value() < piece.value(); }

        /*
         * @breif 駒が成れるか判定する。
         * @retval true 成れる
         * @retval false 成れない
         */
        inline bool is_promotable() const noexcept;

        /*
         * @breif 所有者の手番を取得する。
         * @return 手番
         */
        inline color_t to_color() const noexcept;

        /**
         * @breif 駒が持ち駒として適格であるか判定する。
         * @retval true 持ち駒として適格である
         * @retval false 持ち駒として適格でない
         */
        inline bool is_captured() const noexcept;

        /*
         * @breif 駒を成る前の駒に変換する。
         * @return 成る前の駒
         */
        inline colored_piece_t to_unpromoted() const noexcept;

        /*
         * @breif 駒を成り駒に変換する。
         * @return 成り駒
         */
        inline colored_piece_t to_promoted() const noexcept;

        /**
         * @breif piece が target_piece に合致するか判定する。
         * @param piece 駒
         * @retval true 合致する
         * @retval false 合致しない
         */
        template<piece_value_t target_piece>
        inline bool match(colored_piece_t piece) const noexcept
        {
            SHOGIPP_ASSERT(!empty());
            static const struct impl_t
            {
                impl_t()
                {
                    for (piece_value_t piece = empty_value; piece < piece_size; ++piece)
                        map[piece] = piece == target_piece;
                }
                bool map[piece_size]{};
            } impl;
            return impl.map[piece.value()];
        }
    };

    constexpr noncolored_piece_t empty          { empty_value           };
    constexpr noncolored_piece_t pawn           { pawn_value            };
    constexpr noncolored_piece_t lance          { lance_value           };
    constexpr noncolored_piece_t knight         { knight_value          };
    constexpr noncolored_piece_t silver         { silver_value          };
    constexpr noncolored_piece_t gold           { gold_value            };
    constexpr noncolored_piece_t bishop         { bishop_value          };
    constexpr noncolored_piece_t rook           { rook_value            };
    constexpr noncolored_piece_t king           { king_value            };
    constexpr noncolored_piece_t promoted_pawn  { promoted_pawn_value   };
    constexpr noncolored_piece_t promoted_lance { promoted_lance_value  };
    constexpr noncolored_piece_t promoted_knight{ promoted_knight_value };
    constexpr noncolored_piece_t promoted_silver{ promoted_silver_value };
    constexpr noncolored_piece_t promoted_bishop{ promoted_bishop_value };
    constexpr noncolored_piece_t promoted_rook  { promoted_rook_value   };
    constexpr noncolored_piece_t out_of_range   { out_of_range_value    };

    constexpr captured_piece_t captured_pawn  { pawn_value   };
    constexpr captured_piece_t captured_lance { lance_value  };
    constexpr captured_piece_t captured_knight{ knight_value };
    constexpr captured_piece_t captured_silver{ silver_value };
    constexpr captured_piece_t captured_gold  { gold_value   };
    constexpr captured_piece_t captured_bishop{ bishop_value };
    constexpr captured_piece_t captured_rook  { rook_value   };

    constexpr colored_piece_t black_pawn           { pawn_value            };
    constexpr colored_piece_t black_lance          { lance_value           };
    constexpr colored_piece_t black_knight         { knight_value          };
    constexpr colored_piece_t black_silver         { silver_value          };
    constexpr colored_piece_t black_gold           { gold_value            };
    constexpr colored_piece_t black_bishop         { bishop_value          };
    constexpr colored_piece_t black_rook           { rook_value            };
    constexpr colored_piece_t black_king           { king_value            };
    constexpr colored_piece_t black_promoted_pawn  { promoted_pawn_value   };
    constexpr colored_piece_t black_promoted_lance { promoted_lance_value  };
    constexpr colored_piece_t black_promoted_knight{ promoted_knight_value };
    constexpr colored_piece_t black_promoted_silver{ promoted_silver_value };
    constexpr colored_piece_t black_promoted_bishop{ promoted_bishop_value };
    constexpr colored_piece_t black_promoted_rook  { promoted_rook_value   };

    constexpr colored_piece_t white_pawn           { pawn_value            + color_distance };
    constexpr colored_piece_t white_lance          { lance_value           + color_distance };
    constexpr colored_piece_t white_knight         { knight_value          + color_distance };
    constexpr colored_piece_t white_silver         { silver_value          + color_distance };
    constexpr colored_piece_t white_gold           { gold_value            + color_distance };
    constexpr colored_piece_t white_bishop         { bishop_value          + color_distance };
    constexpr colored_piece_t white_rook           { rook_value            + color_distance };
    constexpr colored_piece_t white_king           { king_value            + color_distance };
    constexpr colored_piece_t white_promoted_pawn  { promoted_pawn_value   + color_distance };
    constexpr colored_piece_t white_promoted_lance { promoted_lance_value  + color_distance };
    constexpr colored_piece_t white_promoted_knight{ promoted_knight_value + color_distance };
    constexpr colored_piece_t white_promoted_silver{ promoted_silver_value + color_distance };
    constexpr colored_piece_t white_promoted_bishop{ promoted_bishop_value + color_distance };
    constexpr colored_piece_t white_promoted_rook  { promoted_rook_value   + color_distance };

    constexpr piece_value_t captured_piece_size = rook.value() - pawn.value() + 1;

    inline bool colored_piece_t::is_promotable() const noexcept
    {
        SHOGIPP_ASSERT(!empty());
        constexpr bool map[]
        {
            false,
            true, true, true, true, false, true, true, false, false, false, false, false, false, false,
            true, true, true, true, false, true, true, false, false, false, false, false, false, false,
        };
        constexpr std::uint32_t bitmask = details::bool_array_to_32bitmask(map);
        return details::bitmask_has(bitmask, value());
    }

    inline color_t colored_piece_t::to_color() const noexcept
    {
        SHOGIPP_ASSERT(!empty());
        if (value() <= color_distance)
            return black;
        return white;
    }

    inline bool colored_piece_t::is_captured() const noexcept
    {
        SHOGIPP_ASSERT(!empty());
        constexpr bool map[]
        {
            false,
            true, true, true, true, true, true, true, false, false, false, false, false, true, false,
            true, true, true, true, true, true, true, false, false, false, false, false, true, false,
        };
        constexpr std::uint32_t bitmask = details::bool_array_to_32bitmask(map);
        return details::bitmask_has(bitmask, value());
    }

    inline colored_piece_t colored_piece_t::to_unpromoted() const noexcept
    {
        SHOGIPP_ASSERT(!empty());
        constexpr static colored_piece_t map[]
        {
            0,
            black_pawn, black_lance, black_knight, black_silver, black_gold, black_bishop, black_rook, black_king, black_pawn, black_lance, black_knight, black_silver, black_bishop, black_rook,
            white_pawn, white_lance, white_knight, white_silver, white_gold, white_bishop, white_rook, white_king, white_pawn, white_lance, white_knight, white_silver, white_bishop, white_rook,
        };
        return map[value()];
    }

    inline colored_piece_t colored_piece_t::to_promoted() const noexcept
    {
        SHOGIPP_ASSERT(is_promotable());
        constexpr static colored_piece_t map[]
        {
            0,
            black_promoted_pawn, black_promoted_lance, black_promoted_knight, black_promoted_silver, 0, black_promoted_bishop, black_promoted_rook, 0, 0, 0, 0, 0, 0, 0,
            white_promoted_pawn, white_promoted_lance, white_promoted_knight, white_promoted_silver, 0, white_promoted_bishop, white_promoted_rook, 0, 0, 0, 0, 0, 0, 0,
        };
        return map[value()];
    }

    inline piece_value_t to_noncolored_impl(piece_value_t piece) noexcept
    {
        SHOGIPP_ASSERT(piece != empty_value);
        return (piece > color_distance) ? piece - color_distance : piece;
    }

    /**
     * @breif 駒を特定の手番の駒に変換する。
     * @param piece 駒
     * @param color 手番
     * @return 特定の手番の駒
     * @details pawn -> black_pawn or white_pawn
     */
    inline piece_value_t to_colored_impl(piece_value_t piece, color_t color) noexcept
    {
        SHOGIPP_ASSERT(piece != empty_value);
        return (color == black) ? piece : piece + color_distance;
    }

    inline piece_value_t to_captured_impl(piece_value_t piece) noexcept
    {
        SHOGIPP_ASSERT(piece != empty_value);
        SHOGIPP_ASSERT(piece != king_value);
        SHOGIPP_ASSERT(piece != king_value + color_distance);
        constexpr static piece_value_t map[]
        {
            0,
            pawn_value, lance_value, knight_value, silver_value, gold_value, bishop_value, rook_value, 0, pawn_value, lance_value, knight_value, silver_value, bishop_value, rook_value,
            pawn_value, lance_value, knight_value, silver_value, gold_value, bishop_value, rook_value, 0, pawn_value, lance_value, knight_value, silver_value, bishop_value, rook_value,
        };
        return map[piece];
    }

    constexpr inline captured_piece_t::captured_piece_t(const colored_piece_t & piece) noexcept
        : basic_piece_t{ to_captured_impl(piece.value()) }
    {
    }

    constexpr inline noncolored_piece_t::noncolored_piece_t(const colored_piece_t & piece) noexcept
        : basic_piece_t{ to_noncolored_impl(piece.value()) }
    {
    }

    constexpr inline colored_piece_t::colored_piece_t(const captured_piece_t & piece, color_t color) noexcept
        : basic_piece_t{ to_colored_impl(piece.value(), color) }
    {
    }

    constexpr inline colored_piece_t::colored_piece_t(const noncolored_piece_t & piece, color_t color) noexcept
        : basic_piece_t{ to_colored_impl(piece.value(), color) }
    {
    }

    inline std::optional<colored_piece_t> char_to_piece(char c) noexcept
    {
        static const std::map<char, colored_piece_t> map
        {
            { 'P', black_pawn   },
            { 'p', white_pawn   },
            { 'L', black_lance  },
            { 'l', white_lance  },
            { 'N', black_knight },
            { 'n', white_knight },
            { 'S', black_silver },
            { 's', white_silver },
            { 'G', black_gold   },
            { 'g', white_gold   },
            { 'B', black_bishop },
            { 'b', white_bishop },
            { 'R', black_rook   },
            { 'r', white_rook   },
            { 'K', black_king   },
            { 'k', white_king   },
        };
        const auto iter = map.find(c);
        if (iter == map.end())
            return std::nullopt;
        return iter->second;
    }

    inline std::optional<std::string> piece_to_sfen_string(colored_piece_t piece) noexcept
    {
        static const std::map<colored_piece_t, std::string> map
        {
            { black_pawn            , "P"  },
            { white_pawn            , "p"  },
            { black_lance           , "L"  },
            { white_lance           , "l"  },
            { black_knight          , "N"  },
            { white_knight          , "n"  },
            { black_silver          , "S"  },
            { white_silver          , "s"  },
            { black_gold            , "G"  },
            { white_gold            , "g"  },
            { black_bishop          , "B"  },
            { white_bishop          , "b"  },
            { black_rook            , "R"  },
            { white_rook            , "r"  },
            { black_king            , "K"  },
            { white_king            , "k"  },
            { black_pawn            , "P+" },
            { white_pawn            , "p+" },
            { black_promoted_lance  , "L+" },
            { white_promoted_lance  , "l+" },
            { black_promoted_knight , "N+" },
            { white_promoted_knight , "n+" },
            { black_promoted_silver , "S+" },
            { white_promoted_silver , "s+" },
            { black_promoted_bishop , "B+" },
            { white_promoted_bishop , "b+" },
            { black_promoted_rook   , "R+" },
            { white_promoted_rook   , "r+" },
        };
        const auto iter = map.find(piece);
        if (iter == map.end())
            return std::nullopt;
        return iter->second;
    }

    inline char color_to_color_char(color_t color) noexcept
    {
        static constexpr char map[] { 'b', 'w' };
        SHOGIPP_ASSERT(color.value() >= black.value());
        SHOGIPP_ASSERT(color.value() <= white.value());
        return map[color.value()];
    }

    using position_t = signed char;
    constexpr position_t npos = -1; // 無効な座標を表現する定数
    constexpr position_t width = 11;
    constexpr position_t height = 11;
    constexpr position_t padding_width = 1;
    constexpr position_t padding_height = 1;
    constexpr position_t position_size = width * height;
    constexpr position_t position_begin = width + padding_width;
    constexpr position_t position_end = position_size - position_begin;
    constexpr position_t file_size = 9;
    constexpr position_t rank_size = 9;

    constexpr position_t front        = -width;
    constexpr position_t left         = -1;
    constexpr position_t right        = +1;
    constexpr position_t back         = +width;
    constexpr position_t knight_left  = front * 2 + left;
    constexpr position_t knight_right = front * 2 + right;
    constexpr position_t front_left   = front + left;
    constexpr position_t front_right  = front + right;
    constexpr position_t back_left    = back + left;
    constexpr position_t back_right   = back + right;

    enum position_alias
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
     * @breif 段を文字列に変換する。
     * @param rank 段
     * @return 文字列
     */
    inline const char * rank_to_string(position_t rank) noexcept
    {
        static const char * map[]{ "一", "二", "三", "四", "五", "六", "七", "八", "九" };
        SHOGIPP_ASSERT(rank >= 0);
        SHOGIPP_ASSERT(rank < static_cast<position_t>(std::size(map)));
        return map[rank];
    }

    /**
     * @breif 筋を文字列に変換する。
     * @param rank 筋
     * @return 文字列
     */
    inline const char * file_to_string(position_t file) noexcept
    {
        static const char * map[]{ "９", "８", "７", "６", "５", "４", "３", "２", "１" };
        SHOGIPP_ASSERT(file >= 0);
        SHOGIPP_ASSERT(file < static_cast<position_t>(std::size(map)));
        return map[file];
    }

    /**
     * @breif 座標から段を抽出する。
     * @param position 座標
     * @return 段
     */
    inline constexpr position_t position_to_rank(position_t position) noexcept
    {
        return position / width - padding_height;
    }

    /**
     * @breif 座標から筋を抽出する。
     * @param position 座標
     * @return 筋
     */
    inline constexpr position_t position_to_file(position_t position) noexcept
    {
        return position % width - padding_width;
    }

    /**
     * @breif 座標を文字列に変換する。
     * @param position 座標
     * @return 文字列
     */
    inline std::string position_to_string(position_t position)
    {
        return std::string{}.append(file_to_string(position_to_file(position))).append(rank_to_string(position_to_rank(position)));
    }

    /**
     * @breif 座標をSFEN表記法に準拠した文字列に変換する。
     * @param position 座標
     * @return SFEN表記法に準拠した文字列
     */
    inline std::string position_to_sfen_string(position_t position)
    {
        std::string sfen_string;
        const char file = static_cast<char>(file_size - 1 - position_to_file(position) + '1');
        const char rank = static_cast<char>(position_to_rank(position) + 'a');
        sfen_string += file;
        sfen_string += rank;
        return sfen_string;
    }

    /**
     * @breif 筋と段から座標を取得する。
     * @param file 筋
     * @param rank 段
     * @return 座標
     */
    inline constexpr position_t file_rank_to_position(position_t file, position_t rank) noexcept
    {
        return width * (rank + padding_height) + padding_width + file;
    }

    /**
    * @breif 生の座標を 0 以上 81 未満の座標に変換する。
    * @param position 座標
    * @return 0 以上 81 未満の座標
    */
    inline constexpr position_t position_to_position9x9(position_t position) noexcept
    {
        class impl_t
        {
        public:
            inline constexpr impl_t() noexcept
            {
                for (position_t rank = 0; rank < rank_size; ++rank)
                    for (position_t file = 0; file < file_size; ++file)
                        map[file_rank_to_position(file, rank)] = rank * file_size + file;
            }
            position_t map[position_size]{};
        };
        constexpr impl_t impl;
        return impl.map[position];
    }

    /**
     * @breif 0 以上 81 未満の座標を生の座標に変換する。
     * @param position 0 以上 81 未満の座標
     * @return 生の座標
     */
    inline constexpr position_t position9x9_to_position(position_t position) noexcept
    {
        return file_rank_to_position(static_cast<position_t>(position % file_size), static_cast<position_t>(position / file_size));
    }


    static const position_t default_king_pos_list[]
    {
        file_rank_to_position(4, 8),
        file_rank_to_position(4, 0)
    };

    /**
     * @breif 2つの座標間の距離を計算する。
     * @param a 座標A
     * @param b 座標B
     * @return 2つの座標間の距離
     */
    inline position_t distance(position_t a, position_t b) noexcept
    {
        const position_t file_a = position_to_file(a);
        const position_t rank_a = position_to_rank(a);
        const position_t file_b = position_to_file(b);
        const position_t rank_b = position_to_rank(b);
        return static_cast<position_t>(std::max((file_a - file_b), (rank_a - rank_b)));
    }

    /**
     * @breif 2つの座標間のマンハッタン距離を計算する。
     * @param a 座標A
     * @param b 座標B
     * @return 2つの座標間のマンハッタン距離
     */
    inline position_t manhattan_distance(position_t a, position_t b) noexcept
    {
        const position_t file_a = position_to_file(a);
        const position_t rank_a = position_to_rank(a);
        const position_t file_b = position_to_file(b);
        const position_t rank_b = position_to_rank(b);
        return static_cast<position_t>(std::abs(file_a - file_b) + std::abs(rank_a - rank_b));
    }

    /**
     * @breif 指定された座標の8近傍のうち中央に近い3近傍の相対座標を取得する。
     * @param a 座標
     * @return 2つの座標間の距離
     */
    inline const position_t * nearest_center_side_3(position_t position) noexcept
    {
        constexpr position_t map[9][4]
        {
            { right, back, back_right, 0 },
            { back_left, back, back_right, 0 },
            { left, back_left, back, 0 },
            { front_right, right, back_right, 0 },
            { 0 },
            { front_left, left, back_left, 0 },
            { front, front_right, right, 0 },
            { front_left, front, front_right, 0 },
            { front_left, front, left, 0 },
        };
        const std::size_t index = position_to_rank(position) / 3 * 3 + position_to_file(position) / 3;
        SHOGIPP_ASSERT(index < std::size(map));
        return map[index];
    }

    /**
     * @breif 入玉の進捗の最大値
     * @sa nyugyoku_progress
     */
    constexpr unsigned int max_nyugyoku_progress = 12;

    /**
     * @breif 入玉の進捗を返す。
     * @param king_position 王の座標
     * @param color 王を所有する手番
     * @return 入玉の進捗
     */
    inline unsigned int nyugyoku_progress(position_t king_position, color_t color) noexcept
    {
        return manhattan_distance(king_position, default_king_pos_list[color.value()]);
    }

    using position_to_noncolored_piece_pair = std::pair<position_t, std::vector<noncolored_piece_t>>;

    const position_to_noncolored_piece_pair near_kiki_list[]
    {
        { knight_left   , { knight } },
        { knight_right  , { knight } },
        { front_left    , { silver, gold, bishop, king, promoted_pawn, promoted_lance, promoted_knight, promoted_silver, promoted_bishop, promoted_rook } },
        { front         , { pawn, lance, silver, gold, rook, king, promoted_pawn, promoted_lance, promoted_knight, promoted_silver, promoted_bishop, promoted_rook } },
        { front_right   , { silver, gold, bishop, king, promoted_pawn, promoted_lance, promoted_knight, promoted_silver, promoted_bishop, promoted_rook } },
        { left          , { gold, king, rook, promoted_pawn, promoted_lance, promoted_knight, promoted_silver, promoted_bishop, promoted_rook } },
        { right         , { gold, king, rook, promoted_pawn, promoted_lance, promoted_knight, promoted_silver, promoted_bishop, promoted_rook } },
        { back_left     , { silver, king, bishop, promoted_bishop, promoted_rook } },
        { back          , { gold, king, rook, promoted_pawn, promoted_lance, promoted_knight, promoted_silver, promoted_bishop, promoted_rook } },
        { back_right    , { silver, king, bishop, promoted_bishop, promoted_rook } },
    };

    const position_to_noncolored_piece_pair far_kiki_list[]
    {
        { front_left , { bishop, promoted_bishop } },
        { front      , { lance, rook, promoted_rook } },
        { front_right, { bishop, promoted_bishop } },
        { left       , { rook, promoted_rook } },
        { right      , { rook, promoted_rook } },
        { back_left  , { bishop, promoted_bishop } },
        { back       , { rook, promoted_rook } },
        { back_right , { bishop, promoted_bishop } }
    };

    const position_to_noncolored_piece_pair far_kiki_list_asynmmetric[]
    {
        { front      , { lance } },
    };

    const position_to_noncolored_piece_pair far_kiki_list_synmmetric[]
    {
        { front_left , { bishop, promoted_bishop } },
        { front      , { rook, promoted_rook } },
        { front_right, { bishop, promoted_bishop } },
        { left       , { rook, promoted_rook } },
        { right      , { rook, promoted_rook } },
        { back_left  , { bishop, promoted_bishop } },
        { back       , { rook, promoted_rook } },
        { back_right , { bishop, promoted_bishop } }
    };

    template<typename Map>
    inline std::optional<typename std::decay_t<Map>::mapped_type> parse(std::string_view & rest, Map && map, std::size_t size) noexcept
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

    /**
     * @breif 後手の場合に -1 を、先手の場合に 1 を返す。
     * @param color 手番
     * @return 符号反転用の数値
     */
    inline constexpr position_t reverse(color_t color) noexcept
    {
        return color == white ? -1 : 1;
    }

    /**
     * @breif 重複しないハッシュ値を返す。
     * @return 重複しないハッシュ値
     */
    inline std::size_t unique_size_t_hash()
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

        inline basic_hash_t() noexcept
            : data{}
        {
        }

        inline basic_hash_t(const basic_hash_t & hash) noexcept
        {
            std::copy(std::begin(hash.data), std::end(hash.data), std::begin(data));
        }

        inline basic_hash_t & operator =(const basic_hash_t & hash) noexcept
        {
            std::copy(std::begin(hash.data), std::end(hash.data), std::begin(data));
            return *this;
        }

        inline basic_hash_t & operator ^=(const basic_hash_t & hash) noexcept
        {
            std::size_t * first = reinterpret_cast<std::size_t *>(data);
            std::size_t * last = reinterpret_cast<std::size_t *>(data + hash_size);
            const std::size_t * input = reinterpret_cast<const std::size_t *>(hash.data);
            while (first != last)
                *first++ ^= *input++;
            return *this;
        }

        inline basic_hash_t operator ^(const basic_hash_t & hash) const noexcept
        {
            basic_hash_t temp{ *this };
            temp ^= hash;
            return temp;
        }

        inline bool operator ==(const basic_hash_t & hash) const noexcept
        {
            return std::equal(std::begin(data), std::end(data), std::begin(hash.data));
        }

        inline bool operator !=(const basic_hash_t & hash) const noexcept
        {
            return !std::equal(std::begin(data), std::end(data), std::begin(hash.data));
        }

        inline explicit operator std::size_t() const noexcept
        {
            std::size_t hash = 0;
            for (std::size_t i = 0; i < hash_size / sizeof(std::size_t); ++i)
                hash ^= *reinterpret_cast<const std::size_t *>(data + i * sizeof(std::size_t));
            return hash;
        }

        inline explicit operator std::string() const noexcept
        {
            std::ostringstream stream;
            stream << "0x";
            for (std::size_t i = 0; i < hash_size / sizeof(std::size_t); ++i)
                stream << std::hex << std::setfill('0') << std::setw(sizeof(size_t) * 2) << *reinterpret_cast<const std::size_t *>(data + i * sizeof(std::size_t));
            stream << std::flush;
            return stream.str();
        }

        inline static basic_hash_t make_unique()
        {
            static_assert(hash_size % sizeof(std::size_t) == 0);
            basic_hash_t unique;
            for (std::size_t i = 0; i < hash_size / sizeof(std::size_t); ++i)
                *reinterpret_cast<std::size_t *>(unique.data + i * sizeof(std::size_t)) = unique_size_t_hash();
            return unique;
        }

    private:
        using byte_type = unsigned char;
        byte_type data[hash_size]{};
    };

    template<typename CharT, typename Traits, std::size_t HashSize>
    std::basic_ostream<CharT, Traits> & operator<<(std::basic_ostream<CharT, Traits> & o, const basic_hash_t<HashSize> & hash)
    {
        o << static_cast<std::string>(hash);
        return o;
    }

    template<std::size_t HashSize>
    class basic_hash_hasher_t
    {
    public:
        inline std::size_t operator()(basic_hash_t<HashSize> key) const noexcept
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

    constexpr std::size_t captured_pawn_size   = 9 * 2 + 1;
    constexpr std::size_t captured_lance_size  = 2 * 2 + 1;
    constexpr std::size_t captured_knight_size = 2 * 2 + 1;
    constexpr std::size_t captured_silver_size = 2 * 2 + 1;
    constexpr std::size_t captured_gold_size   = 2 * 2 + 1;
    constexpr std::size_t captured_bishop_size = 1 * 2 + 1;
    constexpr std::size_t captured_rook_size   = 1 * 2 + 1;

    constexpr std::size_t captured_pawn_offset   = 0;
    constexpr std::size_t captured_lance_offset  = captured_pawn_offset   + captured_pawn_size;
    constexpr std::size_t captured_knight_offset = captured_lance_offset  + captured_lance_size;
    constexpr std::size_t captured_silver_offset = captured_knight_offset + captured_knight_size;
    constexpr std::size_t captured_gold_offset   = captured_silver_offset + captured_silver_size;
    constexpr std::size_t captured_bishop_offset = captured_gold_offset   + captured_gold_size;
    constexpr std::size_t captured_rook_offset   = captured_bishop_offset + captured_bishop_size;
    constexpr std::size_t captured_size          = captured_rook_offset   + captured_rook_size;

    /**
     * @breif 持ち駒と所持数の組の配列におけるオフセットとサイズを表現する。
     */
    class captured_piece_range_t
    {
    public:
        std::size_t offset;
        std::size_t size;
    };

    /**
     * @breif 持ち駒と所持数の組の配列におけるオフセットとサイズを取得する。
     * @param captured_piece 持ち駒
     * @return 持ち駒と所持数の組の配列におけるオフセットとサイズ
     */
    captured_piece_range_t captured_piece_range(captured_piece_t captured_piece)
    {
        constexpr captured_piece_range_t map[]
        {
            { captured_pawn_offset  , captured_pawn_size   },
            { captured_lance_offset , captured_lance_size  },
            { captured_knight_offset, captured_knight_size },
            { captured_silver_offset, captured_silver_size },
            { captured_gold_offset  , captured_gold_size   },
            { captured_bishop_offset, captured_bishop_size },
            { captured_rook_offset  , captured_rook_size   },
        };
        const std::size_t index = captured_piece.value() - pawn_value;
        SHOGIPP_ASSERT(index < std::size(map));
        return map[index];
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
         * @param position 駒の座標
         * @return ハッシュ値
         */
        inline hash_t piece_hash(colored_piece_t piece, position_t position) const noexcept;

        /**
         * @breif 持ち駒のハッシュ値を計算する。
         * @param piece 駒
         * @param count 駒の数
         * @param color 手番
         * @return ハッシュ値
         */
        inline hash_t captured_piece_hash(captured_piece_t piece, std::size_t count, color_t color) const noexcept;

        /**
         * @breif 手番のハッシュ値を計算する。
         * @param color 手番
         * @return ハッシュ値
         */
        inline hash_t color_hash(color_t color) const noexcept;

        /**
         * @breif 合法手のハッシュ値を計算する。
         * @param move 合法手
         * @param color 手番
         * @return ハッシュ値
         */
        inline hash_t move_hash(const move_t & move, color_t color) const noexcept;

        hash_t board_table[piece_size * file_size * rank_size];              // 盤のハッシュテーブル
        hash_t captured_piece_table[captured_size * color_t::size()];            // 持ち駒のハッシュテーブル
        hash_t color_table[color_t::size()];                                     // 手番のハッシュテーブル
        hash_t move_table[(position_size + 1) * position_size * color_t::size()];          // 移動する手のハッシュテーブル
        hash_t put_table[position_size * captured_piece_size * color_t::size()];      // 打つ手のハッシュテーブル
    };

    static const hash_table_t hash_table;

    inline hash_table_t::hash_table_t()
    {
#ifdef SIZE_OF_HASH
        auto generator = hash_t::make_unique;
#else
        auto generator = unique_size_t_hash;
#endif

        std::generate(std::begin(board_table         ), std::end(board_table         ), generator);
        std::generate(std::begin(captured_piece_table), std::end(captured_piece_table), generator);
        std::generate(std::begin(color_table         ), std::end(color_table         ), generator);
        std::generate(std::begin(move_table          ), std::end(move_table          ), generator);
        std::generate(std::begin(put_table           ), std::end(put_table           ), generator);
    }

    inline hash_t hash_table_t::piece_hash(colored_piece_t piece, position_t position) const noexcept
    {
        SHOGIPP_ASSERT(!piece.empty());
        std::size_t index = static_cast<std::size_t>(piece.value());
        index *= file_size;
        index += position_to_file(position);
        index *= rank_size;
        index += position_to_rank(position);
        SHOGIPP_ASSERT(index < std::size(board_table));
        return board_table[index];
    }

    inline hash_t hash_table_t::captured_piece_hash(captured_piece_t piece, std::size_t count, color_t color) const noexcept
    {
        SHOGIPP_ASSERT(piece.value() >= pawn.value());
        SHOGIPP_ASSERT(piece.value() <= rook.value());
        SHOGIPP_ASSERT(!(piece == captured_pawn   && count >= captured_pawn_size  ));
        SHOGIPP_ASSERT(!(piece == captured_lance  && count >= captured_lance_size ));
        SHOGIPP_ASSERT(!(piece == captured_knight && count >= captured_knight_size));
        SHOGIPP_ASSERT(!(piece == captured_silver && count >= captured_silver_size));
        SHOGIPP_ASSERT(!(piece == captured_gold   && count >= captured_gold_size  ));
        SHOGIPP_ASSERT(!(piece == captured_bishop && count >= captured_bishop_size));
        SHOGIPP_ASSERT(!(piece == captured_rook   && count >= captured_rook_size  ));

        std::size_t index = captured_piece_range(piece).offset;
        index += count;
        index *= color_t::size();
        index += color.value();
        SHOGIPP_ASSERT(index < std::size(captured_piece_table));
        return captured_piece_table[index];
    }

    inline hash_t hash_table_t::color_hash(color_t color) const noexcept
    {
        return color_table[color.value()];
    }

    /**
     * @breif 先後を表現する文字列を取得する。
     * @param color 先後
     * @return 先後を表現する文字列
     */
    inline const char * color_to_string(color_t color) noexcept
    {
        const char * map[]{ "先手", "後手" };
        return map[color.value()];
    }

    /**
     * @breif 数値を全角文字列に変換する。
     * @param value 数値
     * @return 全角文字列
     */
    inline std::string to_zenkaku_digit(unsigned int value) noexcept
    {
        const char * map[]
        {
            "０", "１", "２", "３", "４", "５", "６", "７", "８", "９",
        };
        std::string temp = std::to_string(value);
        std::string result;
        for (const char c : temp)
            result += map[c - '0'];
        return result;
    }

    /**
     * @breif 駒の移動先の相対座標の配列の先頭を指すポインタを取得する。
     * @param piece 駒
     * @return 駒の移動先の相対座標の配列の先頭を指すポインタ
     * @details この関数が返すポインタの指す座標は 0 で終端化されている。
     */
    inline const position_t * near_move_offsets(noncolored_piece_t piece) noexcept
    {
        static const std::vector<position_t> map[]
        {
            /* empty           */ { 0 },
            /* pawn            */ { front, 0 },
            /* lance           */ { 0 },
            /* knight          */ { knight_left, knight_right, 0 },
            /* silver          */ { front_left, front, front_right, back_left, back_right, 0 },
            /* gold            */ { front_left, front, front_right, left, right, back, 0 },
            /* bishop          */ { 0 },
            /* rook            */ { 0 },
            /* king            */ { front_left, front, front_right, left, right, back_left, back, back_right, 0 },
            /* promoted_pawn   */ { front_left, front, front_right, left, right, back, 0 },
            /* promoted_lance  */ { front_left, front, front_right, left, right, back, 0 },
            /* promoted_knight */ { front_left, front, front_right, left, right, back, 0 },
            /* promoted_silver */ { front_left, front, front_right, left, right, back, 0 },
            /* promoted_bishop */ { front, left, right, back, 0 },
            /* promoted_rook   */ { front_left, front_right, back_left, back_right, 0 },
        };
        SHOGIPP_ASSERT(!piece.empty());
        SHOGIPP_ASSERT(piece.value() <= std::size(map));
        return map[piece.value()].data();
    }

    /**
     * @breif 駒の移動先の相対座標の配列の先頭を指すポインタを取得する。
     * @param piece 駒
     * @return 駒の移動先の相対座標の配列の先頭を指すポインタ
     * @details この関数が返すポインタの指す座標は 0 で終端化されている。
     */
    inline const position_t * far_move_offsets(noncolored_piece_t piece) noexcept
    {
        static const std::vector<position_t> map[]
        {
            /* empty           */ { 0 },
            /* pawn            */ { 0 },
            /* lance           */ { front, 0 },
            /* knight          */ { 0 },
            /* silver          */ { 0 },
            /* gold            */ { 0 },
            /* bishop          */ { front_left, front_right, back_left, back_right, 0 },
            /* rook            */ { front, left, right, back, 0 },
            /* king            */ { 0 },
            /* promoted_pawn   */ { 0 },
            /* promoted_lance  */ { 0 },
            /* promoted_knight */ { 0 },
            /* promoted_silver */ { 0 },
            /* promoted_bishop */ { front_left, front_right, back_left, back_right, 0 },
            /* promoted_rook   */ { front, left, right, back, 0 },
        };
        SHOGIPP_ASSERT(!piece.empty());
        SHOGIPP_ASSERT(piece.value() <= std::size(map));
        return map[piece.value()].data();
    }

    /**
     * @breif SFEN表記法に準拠した座標の文字列から座標を取得する。
     * @param sfen_position SFEN表記法に準拠した座標の文字列
     * @return 座標
     */
    inline position_t sfen_position_to_position(std::string_view sfen_position)
    {
        if (sfen_position.size() != 2)
            throw invalid_usi_input{ "sfen_pos.size() != 2" };
        if (sfen_position[0] < '1')
            throw invalid_usi_input{ "sfen_pos[0] < '1'" };
        if (sfen_position[0] > '9')
            throw invalid_usi_input{ "sfen_pos[0] > '9'" };
        if (sfen_position[1] < 'a')
            throw invalid_usi_input{ "sfen_pos[1] < 'a'" };
        if (sfen_position[1] > 'i')
            throw invalid_usi_input{ "sfen_pos[1] > 'i'" };
        const position_t file = static_cast<position_t>(file_size - 1 - (sfen_position[0] - '1'));
        const position_t rank = static_cast<position_t>(sfen_position[1] - 'a');
        return file_rank_to_position(file, rank);
    }

    class board_t;

    /**
     * @breif 合法手
     */
    class move_t
    {
    public:
        using tag_t = unsigned char;
        enum : tag_t
        {
            none_tag    = 0x00,
            check_tag   = 0x01,
            escape_tag  = 0x02,
            aigoma_tag  = 0x04,
            promote_tag = 0x08,
            capture_tag = 0x10,
            put_tag     = 0x20,
        };

        /**
         * @breif 打ち手を構築する。
         * @param destination 打つ座標
         * @param source_piece 打つ駒
         */
        inline move_t(position_t destination, captured_piece_t captured_piece, tag_t tag = put_tag) noexcept;

        /**
         * @breif 移動する手を構築する。
         * @param source 移動元の座標
         * @param destination 移動先の座標
         * @param source_piece 移動元の駒
         * @param captured_piece 移動先の駒
         * @param promote 成か不成か
         */
        inline move_t(position_t source, position_t destination, colored_piece_t source_piece, colored_piece_t captured_piece, bool promote, tag_t tag = none_tag) noexcept;

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
         * @details put() が true を返す場合にこの関数を呼び出した場合、無効な値が返る。
         */
        inline position_t source() const noexcept;

        /**
         * @breif 移動先の座標を取得する。
         * @return 移動先の座標
         * @details put() が true を返す場合、この関数は打つ先の座標を返す。
         */
        inline position_t destination() const noexcept;

        /**
         * @breif 移動元の駒を取得する。
         * @return 移動元の駒
         * @details put() が true を返す場合にこの関数を呼び出した場合、無効な値が返る。
         */
        inline colored_piece_t source_piece() const noexcept;

        /**
         * @breif 打つ駒を取得する。
         * @return 打つ駒
         * @details put() が false を返す場合にこの関数を呼び出した場合、無効な値が返る。
         */
        inline captured_piece_t captured_piece() const noexcept;

        /**
         * @breif 移動先の駒を取得する。
         * @return 移動先の駒
         * @detalis put が true を返す場合にこの関数を呼び出した場合、無効な値が返る。
         */
        inline colored_piece_t destination_piece() const noexcept;

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

        /**
         * @breif 合法手のタグを取得する。
         * @return 合法手のタグ
         */
        inline tag_t tag() const noexcept;

    private:
        position_t      m_source;           // 移動元の座標(source == npos の場合、持ち駒を打つ)
        position_t      m_destination;      // 移動先の座標(source == npos の場合、 destination は打つ座標)
        colored_piece_t m_source_piece;     // 移動元の駒(source == npos の場合、 source_piece() は打つ持ち駒)
        colored_piece_t m_destination_piece;// 移動先の駒(source == npos の場合、 captured_piece は未定義)
        bool            m_promote;          // 成る場合 true
        tag_t           m_tag;              // タグ
    };

    inline move_t::move_t(position_t destination, captured_piece_t captured_piece, tag_t tag) noexcept
        : m_source{ npos }
        , m_destination{ destination }
        , m_source_piece{ captured_piece.value() }
        , m_destination_piece{ empty.value() }
        , m_promote{ false }
        , m_tag{ tag }
    {
        SHOGIPP_ASSERT(captured_piece.value() >= captured_pawn.value());
        SHOGIPP_ASSERT(captured_piece.value() <= captured_rook.value());
    }

    inline move_t::move_t(position_t source, position_t destination, colored_piece_t source_piece, colored_piece_t captured_piece, bool promote, tag_t tag) noexcept
        : m_source{ source }
        , m_destination{ destination }
        , m_source_piece{ source_piece }
        , m_destination_piece{ captured_piece }
        , m_promote{ promote }
        , m_tag{ tag }
    {
    }

    inline bool move_t::put() const noexcept
    {
        return m_source == npos;
    }

    inline position_t move_t::source() const noexcept
    {
        SHOGIPP_ASSERT(!put());
        return m_source;
    }

    inline position_t move_t::destination() const noexcept
    {
        return m_destination;
    }

    inline colored_piece_t move_t::source_piece() const noexcept
    {
        SHOGIPP_ASSERT(!put());
        return m_source_piece;
    }

    inline captured_piece_t move_t::captured_piece() const noexcept
    {
        SHOGIPP_ASSERT(put());
        return captured_piece_t{ m_source_piece.value() };
    }

    inline colored_piece_t move_t::destination_piece() const noexcept
    {
        SHOGIPP_ASSERT(!put());
        return m_destination_piece;
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
            SHOGIPP_ASSERT(source_piece().to_color() == black);
            const auto optional_piece = piece_to_sfen_string(source_piece());
            SHOGIPP_ASSERT(optional_piece.has_value());
            SHOGIPP_ASSERT(optional_piece->size() == 1);
            result += *optional_piece;
            result += '*';
            result += position_to_sfen_string(destination());
        }
        else
        {
            result += position_to_sfen_string(source());
            result += position_to_sfen_string(destination());
            if (promote())
                result += '+';
        }
        return result;
    }

    inline move_t::tag_t move_t::tag() const noexcept
    {
        return m_tag;
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

    inline int to_category(const move_t & move) noexcept
    {
        if (move.tag() & move_t::check_tag)
            return 6;
        if (move.tag() & move_t::escape_tag)
            return 5;
        if (move.tag() & move_t::aigoma_tag)
            return 4;
        if (move.tag() & move_t::promote_tag)
            return 3;
        if (move.tag() & move_t::capture_tag)
            return 2;
        if (move.tag() & move_t::put_tag)
            return 1;
        return 0;
    };

    inline hash_t hash_table_t::move_hash(const move_t & move, color_t color) const noexcept
    {
        std::size_t index;
        if (move.put())
        {
            index = static_cast<std::size_t>(move.destination());
            index *= captured_piece_size;
            index += move.captured_piece().value();
            index *= color_t::size();
            index += color.value();
            SHOGIPP_ASSERT(index < std::size(put_table));
            return put_table[index];
        }
        index = static_cast<std::size_t>(move.source() - npos);
        index *= position_size;
        index += static_cast<std::size_t>(move.destination());
        index *= color_t::size();
        index += color.value();
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
        inline captured_pieces_t() noexcept;

        constexpr inline captured_pieces_t(const captured_pieces_t &) noexcept = default;
        constexpr inline captured_pieces_t & operator =(const captured_pieces_t &) noexcept = default;

        /**
         * @breif 持ち駒を出力ストリームに出力する。
         * @param ostream 出力ストリーム
         */
        inline void print(std::ostream & ostream = std::cout) const;

        /**
         * @breif 駒と対応する持ち駒の数の参照を返す。
         * @param 駒
         * @return 駒と対応する持ち駒の数の参照
         */
        inline size_type & operator [](captured_piece_t piece) noexcept;

        /**
         * @breif 駒と対応する持ち駒の数の参照を返す。
         * @param 駒
         * @return 駒と対応する持ち駒の数の参照
         */
        inline const size_type & operator [](captured_piece_t piece) const noexcept;

    private:
        size_type count[captured_piece_size];
    };

    inline captured_pieces_t::captured_pieces_t() noexcept
        : count{}
    {
    }

    inline void captured_pieces_t::print(std::ostream & ostream) const
    {
        unsigned int kind = 0;
        for (piece_value_t piece = captured_rook.value(); piece >= captured_pawn.value(); --piece)
        {
            if ((*this)[captured_piece_t{ piece }] > 0)
            {
                ostream << captured_piece_t{ piece }.to_string();
                if ((*this)[captured_piece_t{ piece }] > 1)
                    ostream << to_zenkaku_digit((*this)[captured_piece_t{ piece }]);
                ++kind;
            }
        }
        if (kind == 0)
            ostream << "なし";
        ostream << std::endl;
    }

    inline captured_pieces_t::size_type & captured_pieces_t::operator [](captured_piece_t piece) noexcept
    {
        SHOGIPP_ASSERT(!piece.empty());
        return count[piece.value() - pawn_value];
    }

    inline const captured_pieces_t::size_type & captured_pieces_t::operator [](captured_piece_t piece) const noexcept
    {
        SHOGIPP_ASSERT(!piece.empty());
        return count[piece.value() - pawn_value];
    }

    class kyokumen_rollback_validator_t;

#define _ { empty        .value() }
#define P { black_pawn   .value() }
#define p { white_pawn   .value() }
#define L { black_lance  .value() }
#define l { white_lance  .value() }
#define N { black_knight .value() }
#define n { white_knight .value() }
#define S { black_silver .value() }
#define s { white_silver .value() }
#define G { black_gold   .value() }
#define g { white_gold   .value() }
#define B { black_bishop .value() }
#define b { white_bishop .value() }
#define R { black_rook   .value() }
#define r { white_rook   .value() }
#define K { black_king   .value() }
#define k { white_king   .value() }
#define x { out_of_range .value() }
    constexpr colored_piece_t clear_board[]
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

    constexpr colored_piece_t initial_board[]
    {
        x, x, x, x, x, x, x, x, x, x, x,
        x, l, n, s, g, k, g, s, n, l, x,
        x, _, r, _, _, _, _, _, b, _, x,
        x, p, p, p, p, p, p, p, p, p, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, P, P, P, P, P, P, P, P, P, x,
        x, _, B, _, _, _, _, _, R, _, x,
        x, L, N, S, G, K, G, S, N, L, x,
        x, x, x, x, x, x, x, x, x, x, x,
    };
#undef _
#undef P
#undef p
#undef L
#undef l
#undef N
#undef n
#undef S
#undef s
#undef G
#undef g
#undef B
#undef b
#undef R
#undef r
#undef K
#undef k
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

        inline colored_piece_t & operator [](std::size_t i) noexcept;
        inline const colored_piece_t & operator [](std::size_t i) const noexcept;

        /**
         * @breif 座標が盤外か判定する。
         * @param position 座標
         * @return 盤外の場合true
         */
        inline static bool out(position_t position) noexcept;

        /**
         * @breif 盤を出力ストリームに出力する。
         * @param ostream 出力ストリーム
         */
        inline void print(std::ostream & ostream = std::cout) const;

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
        colored_piece_t data[position_size];
    };

    inline board_t::board_t()
    {
        std::copy(std::begin(initial_board), std::end(initial_board), std::begin(data));
    }

    inline colored_piece_t & board_t::operator [](std::size_t i) noexcept
    {
        return data[i];
    }

    inline const colored_piece_t & board_t::operator [](std::size_t i) const noexcept
    {
        return data[i];
    }

    inline bool board_t::out(position_t position) noexcept
    {
        return position < position_begin || position >= position_end || clear_board[position].value() == out_of_range.value();
    }

    inline void board_t::print(std::ostream & ostream) const
    {
        ostream << "  ９ ８ ７ ６ ５ ４ ３ ２ １" << std::endl;
        ostream << "+---------------------------+" << std::endl;
        for (position_t rank = 0; rank < rank_size; ++rank)
        {
            ostream << "|";
            for (position_t file = 0; file < file_size; ++file)
            {
                const colored_piece_t piece = data[file_rank_to_position(file, rank)];
                ostream << ((!piece.empty() && piece.to_color() == white) ? "v" : " ") << piece.to_string();
            }
            ostream << "| " << rank_to_string(rank) << std::endl;
        }
        ostream << "+---------------------------+" << std::endl;
    }

    inline void board_t::clear()
    {
        std::copy(std::begin(clear_board), std::end(clear_board), std::begin(data));
    }

    inline std::string board_t::sfen_string() const
    {
        std::string result;
        for (position_t rank = 0; rank < rank_size; ++rank)
        {
            position_t empty_count = 0;
            for (position_t file = 0; file < file_size; ++file)
            {
                const colored_piece_t piece = data[file_rank_to_position(file, rank)];
                if (piece.empty())
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
            if (rank + 1 < rank_size)
                result += '/';
        }
        return result;
    }

    inline move_t::move_t(std::string_view sfen_move, const board_t & board)
        : m_tag{ none_tag }
    {
        if (sfen_move.size() < 4)
            throw invalid_usi_input{ "invalid sfen move" };

        if (sfen_move[1] == '*')
        {
            if (sfen_move.size() > 4)
                throw invalid_usi_input{ "invalid sfen move" };
            const std::optional<colored_piece_t> optional_piece = char_to_piece(sfen_move[0]);
            if (!optional_piece)
                throw invalid_usi_input{ "invalid sfen move" };
            if (optional_piece->to_color() == white)
                throw invalid_usi_input{ "invalid sfen move" };
            const position_t destination = sfen_position_to_position(sfen_move.substr(2, 2));

            m_source = npos;
            m_destination = destination;
            m_source_piece = *optional_piece;
            m_destination_piece = colored_piece_t{};
            m_promote = false;
        }
        else
        {
            const position_t source = sfen_position_to_position(sfen_move.substr(0, 2));
            if (board[source].empty())
                throw invalid_usi_input{ "invalid sfen move 1" };
            if (board_t::out(source))
                throw invalid_usi_input{ "invalid sfen move 2" };
            const position_t destination = sfen_position_to_position(sfen_move.substr(2, 2));
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
            m_destination_piece = board[destination];
            m_promote = promote;
        }
    }

    /**
     * @breif 利き
     */
    class kiki_t
    {
    public:
        position_t position;    // 利いている駒の座標
        position_t offset;      // 利かされている駒の座標を基準とする利きの相対座標
        bool aigoma;            // 合駒が可能か
    };

    class aigoma_info_t
        : public std::unordered_map<position_t, std::vector<position_t>>
    {
    public:
        using std::unordered_map<position_t, std::vector<position_t>>::unordered_map;

        inline void print() const
        {
            for (const auto & [position, candidates] : *this)
                std::cout << "合駒：" << position_to_string(position) << std::endl;
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

    template<typename Value, typename Hash = std::hash<Value>>
    class stack_set_t
    {
    public:
        using value_type = Value;
        using stack_type = std::deque<value_type>;
        using hash_type = Hash;
        using unordered_set_type = std::unordered_set<value_type, hash_type>;

        /**
         * @breif スタックで管理されるキャッシュを構築する。
         */
        inline stack_set_t()
        {
        }

        inline stack_set_t(const stack_set_t &) = default;
        inline stack_set_t(stack_set_t &&) = default;
        inline stack_set_t & operator =(const stack_set_t &) = default;
        inline stack_set_t & operator =(stack_set_t &&) = default;

        /**
         * @breif キャッシュを破棄する。
         */
        inline void clear()
        {
            stack.clear();
            uset.clear();
        }

        /**
         * @breif 値が登録されているか判定する。
         * @param 値
         * @retval true 値が登録されている
         * @retval false 値が登録されていない
         */
        inline bool contains(value_type value) const
        {
            return uset.find(value) != uset.end();
        }

        /**
         * @breif 値を登録する。
         * @param value 値
         */
        inline void push(value_type value)
        {
            stack.emplace_back(value);
            uset.insert(value);
        }

        inline void pop()
        {
            uset.erase(stack.back());
            stack.pop_back();
        }

    private:
        stack_type stack;
        unordered_set_type uset;
    };

#ifdef SIZE_OF_HASH
    using stack_cache_t = stack_set_t<hash_t, basic_hash_hasher_t<SIZE_OF_HASH>>;
#else
    using stack_cache_t = stack_set_t<hash_t>;
#endif

    /**
     * @breif 局面の追加情報
     * @details 手番の合法手を検索する過程で手番にかかっている王手が必要になるため、個別にスタック構造を保持する。
     */
    class additional_info_t
    {
    public:
        std::vector<std::vector<kiki_t>> check_list_stack;  // 手番にかかっている王手
        std::vector<hash_t> hash_stack;                     // 局面のハッシュ値
        position_t king_position_list[color_t::size()]{};   // 王の座標
        stack_cache_t previously_done_moves;                // 既出の合法手
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
        inline static bool promotable(colored_piece_t piece, position_t source, position_t destination);

        /**
         * @breif 駒が移動する場合に成りが必須か判定する。
         * @param piece 駒
         * @param destination 移動先の座標
         * @return 成りが必須の場合(駒が既に成っている場合、常にfalse)
         */
        inline static bool must_promote(colored_piece_t piece, position_t destination);

        /**
         * @breif 移動元の座標から移動可能の移動先を反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param source 移動元の座標
         * @param offset 移動先の相対座標
         */
        template<typename OutputIterator>
        inline void search_far_destination(OutputIterator result, position_t source, position_t offset) const;

        /**
         * @breif 移動元の座標から移動可能の移動先を非反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param source 移動元の座標
         * @param offset 移動先の相対座標
         */
        template<typename OutputIterator>
        inline void search_near_destination(OutputIterator result, position_t source, position_t offset) const;

        /**
         * @breif 移動元の座標から移動可能の移動先を検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param source 移動元の座標
         * @param color どちらの手番の移動か
         */
        template<typename OutputIterator>
        inline void search_destination(OutputIterator result, position_t source, color_t color) const;

        /**
         * @breif 持ち駒を移動先の座標に打つことができるか判定する。歩、香、桂に限りfalseを返す可能性がある。
         * @param piece 持ち駒
         * @param destination 移動先の座標
         * @return 置くことができる場合 true
         */
        inline bool puttable(captured_piece_t piece, position_t destination) const;

        /**
         * @breif 移動元の座標を検索する。
         * @param result 出力イテレータ
         * @param color どちらの手番の移動か
         */
        template<typename OutputIterator>
        inline void search_source(OutputIterator result, color_t color) const;

        /**
         * @breif 座標positionから相対座標offset方向に走査し最初に駒が現れる座標を返す。
         * @param position 走査を開始する座標
         * @param offset 走査する相対座標
         * @return 最初に駒が現れる座標(駒が見つからない場合 npos )
         */
        inline position_t search(position_t position, position_t offset) const;

        /**
         * @breif 座標positionを利いている駒あるいは紐を付けている駒を検索する。
         * @param result 利きの出力イテレータ
         * @param position 座標
         * @param offset 利きの相対座標
         * @param first 利く駒の入力イテレータ(begin)
         * @param last 利く駒の入力イテレータ(end)
         * @param is_collected 見つかった駒の手番に対して出力イテレータに出力するか判定する叙述関数(bool(bool))
         * @param transform (position, offset, aigoma) を出力イテレータに出力する変数に変換する関数
         */
        template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
        inline void search_piece_near(OutputIterator result, position_t position, position_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const;

        /**
         * @breif 座標positionを利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param position 座標
         * @param offset 利きの相対座標
         * @param first 利く駒の入力イテレータ(begin)
         * @param last 利く駒の入力イテレータ(end)
         * @param is_collected 見つかった駒の手番に対して出力イテレータに出力するか判定する叙述関数(bool(bool))
         * @param transform (position, offset, aigoma) を出力イテレータに出力する変数に変換する関数
         * @sa search_kiki_far
         */
        template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
        inline void search_piece_far(OutputIterator result, position_t position, position_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const;

        /**
         * @breif 座標positionに利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param position 座標
         * @param color 先後いずれの視点か
         * @param is_collected 見つかった駒の手番に対して出力イテレータに出力するか判定する叙述関数(bool(bool))
         * @param transform (position, offset, aigoma) を出力イテレータに出力する変数に変換する関数
         */
        template<typename OutputIterator, typename IsCollected, typename Transform>
        inline void search_piece(OutputIterator result, position_t position, color_t color, IsCollected is_collected, Transform transform) const;

        /**
         * @breif 座標positionに紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param position 座標
         * @param color 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_himo(OutputIterator result, position_t position, color_t color) const;

        /**
         * @breif 座標positionに利いている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param position 座標
         * @param color 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_kiki(OutputIterator result, position_t position, color_t color) const;

        /**
         * @breif 座標positionに利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param position 座標
         * @param color 先後いずれの視点か
         */
        template<typename OutputIterator>
        inline void search_kiki_or_himo(OutputIterator result, position_t position, color_t color) const;

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
         * @breif 王の座標を更新する。
         */
        inline void update_king_position_list();

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
         * @param tag タグ
         */
        template<typename OutputIterator>
        inline void search_moves_from_positions(OutputIterator result, position_t source, position_t destination, move_t::tag_t tag) const;

        /**
         * @breif 合法手のうち王手を外さない手を検索する。
         * @param result 合法手の出力イテレータ
         * @details 王手されていない場合、この関数により生成される手の集合は合法手全体と完全に一致する。
         */
        template<typename OutputIterator>
        inline void search_moves_nonescapes(OutputIterator result) const;

        /**
         * @breif 王手を外す手のうち王を移動する手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_moves_escapes_king_move(OutputIterator result) const;

        /**
         * @breif 王手を外す手のうち合駒する手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_moves_escapes_aigoma(OutputIterator result) const;

        /**
         * @breif 合法手のうち王手を外す手を検索する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_moves_escapes(OutputIterator result) const;

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
         * @details anti_repetition_of_moves == true の場合この関数は strict_search_moves にリダイレクトされる。
         */
        inline moves_t search_moves() const;

        /**
         * @breif 千日手を含む厳密でない合法手を検索する。
         * @param result 合法手の出力イテレータ
         */
        inline moves_t nonstrict_search_moves() const;

        /**
         * @breif 千日手を含まない厳密な合法手を検索する。
         * @param result 合法手の出力イテレータ
         */
        inline moves_t strict_search_moves() const;

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
         * @breif 合法手を出力ストリームに出力する。
         * @param move 合法手
         * @param color 後手の合法手か
         * @param ostream 出力ストリーム
         */
        inline void print_move(const move_t & move, color_t color, std::ostream & ostream = std::cout) const;

        /**
         * @breif 合法手を出力ストリームに出力する。
         * @param first 合法手の入力イテレータのbegin
         * @param last 合法手の入力イテレータのend
         * @param ostream 出力ストリーム
         */
        template<typename InputIterator>
        inline void print_moves(InputIterator first, InputIterator last, std::ostream & ostream = std::cout) const;

        /**
         * @breif 合法手を出力ストリームに出力する。
         * @param ostream 出力ストリーム
         */
        inline void print_moves(std::ostream & ostream = std::cout) const;

        /**
         * @breif 王手を出力ストリームに出力する。
         * @param ostream 出力ストリーム
         */
        inline void print_check(std::ostream & ostream = std::cout) const;

        /**
         * @breif 局面を出力ストリームに出力する。
         * @param ostream 出力ストリーム
         */
        inline void print(std::ostream & ostream = std::cout) const;

        /**
         * @breif 棋譜を出力ストリームに出力する。
         * @param ostream 出力ストリーム
         */
        inline void print_kifu(std::ostream & ostream = std::cout) const;

        /**
         * @breif 末尾から指定された手数分の棋譜を出力ストリームに出力する。
         * @param ostream 出力ストリーム
         */
        inline void print_recent_kifu(std::size_t size, std::ostream & ostream = std::cout) const;

        /**
         * @breif 局面のハッシュ値を返す。
         * @return 局面のハッシュ値
         */
        inline hash_t hash() const;

        inline void validate_board_out();

        /**
         * @breif 合法手を実行する。
         * @param move 合法手
         * @details この関数を呼び出すときに指定した move を指定して undo_move を呼び出すことにより
         *          この関数を呼び出す前の状態に復元することができる。
         *          この関数は additional_info のメンバに再割当てを行う可能性があるため、
         *          この関数を呼び出す前後で additional_info に関連した参照やポインタを継続して使用しないよう注意する。
         * @sa undo_move
         */
        inline void do_move(const move_t & move);

        /**
         * @breif 合法手を実行する前に戻す。
         * @param move 合法手
         * @sa do_move
         */
        inline void undo_move(const move_t & move);

        /**
         * @breif 最後に指された合法手を返す。
         * @return 最後に指された合法手
         */
        inline std::optional<move_t> last_move() const noexcept;

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
         * @breif 局面をSFEN表記法に準拠した文字列に変換する。
         * @return SFEN表記法に準拠した文字列
         */
        inline std::string sfen_string() const;

        /**
         * @breif 合法手の集合から千日手を削除する。
         */
        inline void remove_repetition_of_moves(moves_t & moves) const noexcept;

        /**
         * @breif 王の座標を取得する。
         * @param color 手番
         * @return 座標
         */
        inline position_t king_position(color_t color) const noexcept;

        /**
         * @breif 手番にかかっている王手を取得する。
         * @return 手番にかかっている王手
         */
        inline const std::vector<kiki_t> & check_list() const noexcept;

        board_t board;                                              // 盤
        captured_pieces_t captured_pieces_list[color_t::size()];    // 持ち駒
        move_count_t move_count = 0;                                // 手数
        std::vector<move_t> kifu;                                   // 棋譜
        additional_info_t additional_info;                          // 追加情報
        bool anti_repetition_of_moves = true;                       // 耐千日手
        std::string initial_sfen_string;                            // 初期状態
    };

#ifndef NDEBUG
    /**
     * @breif コピーコンストラクトされてからデストラクトされるまでに局面が変更されていないことを検証する。
     */
    class kyokumen_rollback_validator_t
    {
    public:
        inline kyokumen_rollback_validator_t(const kyokumen_t & kyokumen) noexcept;
        inline ~kyokumen_rollback_validator_t() noexcept;

    private:
        const kyokumen_t & kyokumen;
        colored_piece_t data[position_size];
        captured_pieces_t captured_pieces_list[color_t::size()];
    };

    inline kyokumen_rollback_validator_t::kyokumen_rollback_validator_t(const kyokumen_t & kyokumen) noexcept
        : kyokumen{ kyokumen }
    {
        std::copy(std::begin(kyokumen.board.data), std::end(kyokumen.board.data), std::begin(data));
        for (const color_t color : colors)
            captured_pieces_list[color.value()] = kyokumen.captured_pieces_list[color.value()];
    }

    inline kyokumen_rollback_validator_t::~kyokumen_rollback_validator_t() noexcept
    {
        // 最善手を探索している間に例外が送出された場合、 do_move と対応する undo_move が呼び出されない。
        // 捕捉されていない例外が存在しない場合のみ、不整合がないか検証する。
        if (std::uncaught_exceptions() == 0)
        {
            for (std::size_t i = 0; i < std::size(data); ++i)
                SHOGIPP_ASSERT(data[i] == kyokumen.board.data[i]);
            for (const color_t color : colors)
                for (piece_value_t piece = pawn_value; piece <= rook_value; ++piece)
                    SHOGIPP_ASSERT(captured_pieces_list[color.value()][captured_piece_t{ piece }] == kyokumen.captured_pieces_list[color.value()][captured_piece_t{ piece }]);
        }
    }
#endif

    inline kyokumen_t::kyokumen_t()
    {
        update_king_position_list();
        push_additional_info();
        initial_sfen_string = "startpos";
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

        const bool is_startpos = *current_token == startpos;
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
            position_t rank = 0, file = 0;

            const std::string_view sfen_string = *current_token;
            for (const char c : *current_token)
            {
                if (c == '+')
                {
                    promoted = true;
                }
                else if (c == '/')
                {
                    if (file != file_size)
                        throw invalid_usi_input{ "unexpected '/'" };
                    ++rank;
                    file = 0;
                }
                else if (c >= '1' && c <= '9')
                {
                    file += static_cast<position_t>(c - '0');
                }
                else
                {
                    const std::optional<colored_piece_t> optional_piece = char_to_piece(c);
                    if (!optional_piece)
                        throw invalid_usi_input{ "unexpected character 1" };
                    colored_piece_t piece = *optional_piece;
                    if (promoted)
                        piece = piece.to_promoted();
                    temp.board[file_rank_to_position(file, rank)] = piece;
                    ++file;
                }
            }
            temp.clear_additional_info();
            temp.push_additional_info();

            ++current_token;
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
                    if (!move.put() && move.source_piece().to_color() != temp.color())
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
                            std::optional<colored_piece_t> optional_piece = char_to_piece(*iter);
                            if (!optional_piece)
                                throw invalid_usi_input{ "unexpected character 2" };
                            if (!optional_piece->is_captured())
                                throw invalid_usi_input{ "unexpected character 3" };
                            temp.captured_pieces_list[optional_piece->to_color().value()][captured_piece_t{ *optional_piece }] = count;
                            count = 1;
                        }
                    }
                    ++current_token;
                }
            }
        }

        *this = std::move(temp);
        initial_sfen_string = position;
    }

    inline bool kyokumen_t::promotable(colored_piece_t piece, position_t source, position_t destination)
    {
        if (!piece.is_promotable())
            return false;
        if (piece.to_color() == black)
            return source < width * (3 + padding_height) || destination < width * (3 + padding_height);
        return source >= width * (6 + padding_height) || destination >= width * (6 + padding_height);
    }

    inline bool kyokumen_t::must_promote(colored_piece_t piece, position_t destination)
    {
        if (noncolored_piece_t{ piece } == pawn || noncolored_piece_t{ piece } == lance)
        {
            if (piece.to_color() == black)
                return destination < (width * (1 + padding_height));
            return destination >= width * (8 + padding_height);
        }
        else if (noncolored_piece_t{ piece } == knight)
        {
            if (piece.to_color() == black)
                return destination < width * (2 + padding_height);
            return destination >= width * (7 + padding_height);
        }
        return false;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_far_destination(OutputIterator result, position_t source, position_t offset) const
    {
        for (position_t current = source + offset; !board_t::out(current); current += offset)
        {
            if (board[current].empty())
                *result++ = current;
            else
            {
                if (board[source].to_color() == board[current].to_color()) break;
                *result++ = current;
                if (board[source].to_color() != board[current].to_color()) break;
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_near_destination(OutputIterator result, position_t source, position_t offset) const
    {
        const position_t current = source + offset;
        if (!board_t::out(current) && (board[current].empty() || board[current].to_color() != board[source].to_color()))
            *result++ = current;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_destination(OutputIterator result, position_t source, color_t color) const
    {
        const noncolored_piece_t piece{ board[source] };
        for (const position_t * offset = far_move_offsets(piece); *offset; ++offset)
            search_far_destination(result, source, *offset * reverse(color));
        for (const position_t * offset = near_move_offsets(piece); *offset; ++offset)
            search_near_destination(result, source, *offset * reverse(color));
    }

    inline bool kyokumen_t::puttable(captured_piece_t piece, position_t destination) const
    {
        if (!board[destination].empty())
            return false;
        if (color() == black)
        {
            if ((piece == captured_pawn || piece == captured_lance) && destination < width * (padding_height + 1))
                return false;
            if (piece == captured_knight && destination < width * (padding_height + 2))
                return false;
        }
        else
        {
            if ((piece == captured_pawn || piece == captured_lance) && destination >= width * (padding_height + 8))
                return false;
            if (piece == captured_knight && destination >= width * (padding_height + 7))
                return false;
        }
        if (piece == captured_pawn)
        {
            const position_t file = position_to_file(destination);

            // 二歩
            for (position_t rank = 0; rank < rank_size; ++rank)
            {
                const colored_piece_t current = board[file_rank_to_position(file, rank)];
                if (!current.empty() && noncolored_piece_t{ current } == pawn && color() == current.to_color())
                    return false;
            }

            // 打ち歩詰め
            const position_t position = destination + front * (reverse(color()));
            if (!board_t::out(position) && !board[position].empty() && noncolored_piece_t{ board[position] } == king && board[position].to_color() != color())
            {
                const move_t move{ destination, piece };
                moves_t moves;
                {
                    VALIDATE_KYOKUMEN_ROLLBACK(*this);
                    const_cast<kyokumen_t &>(*this).do_move(move);
                    moves = search_moves();
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
        for (position_t position = position_begin; position < position_end; ++position)
            if (!board_t::out(position) && !board[position].empty() && board[position].to_color() == color)
                *result++ = position;
    }

    inline position_t kyokumen_t::search(position_t position, position_t offset) const
    {
        position_t current;
        for (current = position + offset; !board_t::out(current) && board[current].empty(); current += offset);
        if (board_t::out(current))
            return npos;
        return current;
    }
    
    template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_piece_near(OutputIterator result, position_t position, position_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const
    {
        if (position_t current = position + offset; !board_t::out(current) && !board[current].empty())
            if (is_collected(board[current].to_color()) && std::find(first, last, noncolored_piece_t{ board[current] }) != last)
                *result++ = transform(current, offset, false);
    }

    template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_piece_far(OutputIterator result, position_t position, position_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const
    {
        if (position_t found = search(position, offset); found != npos && found != position + offset && !board[found].empty())
            if (is_collected(board[found].to_color()) && std::find(first, last, noncolored_piece_t{ board[found] }) != last)
                *result++ = transform(found, offset, true);
    }

    template<typename OutputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_piece(OutputIterator result, position_t position, color_t color, IsCollected is_collected, Transform transform) const
    {
        for (const auto & [offset, candidates] : near_kiki_list)
            search_piece_near(result, position, offset * reverse(color), candidates.begin(), candidates.end(), is_collected, transform);
        for (const auto & [offset, candidates] : far_kiki_list_synmmetric)
            search_piece_far(result, position, offset, candidates.begin(), candidates.end(), is_collected, transform);
        for (const auto & [offset, candidates] : far_kiki_list_asynmmetric)
            search_piece_far(result, position, offset * reverse(color), candidates.begin(), candidates.end(), is_collected, transform);
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_himo(OutputIterator result, position_t position, color_t color) const
    {
        search_piece(result, position, color,
            [color](color_t g) { return g == color; },
            [](position_t position, position_t offset, bool aigoma) -> position_t { return position; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_kiki(OutputIterator result, position_t position, color_t color) const
    {
        search_piece(result, position, color,
            [color](color_t g) { return g != color; },
            [](position_t position, position_t offset, bool aigoma) -> kiki_t { return { position, offset, aigoma }; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_kiki_or_himo(OutputIterator result, position_t position, color_t color) const
    {
        search_piece(result, position, color,
            [](color_t) { return true; },
            [](position_t position, position_t offset, bool aigoma) -> position_t { return position; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_check(OutputIterator result, color_t color) const
    {
        search_kiki(result, additional_info.king_position_list[color.value()], color);
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
        if (!kifu.empty())
        {
            const hash_t hash = this->hash() ^ hash_table.move_hash(kifu.back(), !color());
            additional_info.previously_done_moves.push(hash);
        }
    }

    inline void kyokumen_t::pop_additional_info()
    {
        SHOGIPP_ASSERT(!additional_info.check_list_stack.empty());
        SHOGIPP_ASSERT(!additional_info.hash_stack.empty());
        additional_info.check_list_stack.pop_back();
        additional_info.hash_stack.pop_back();
        additional_info.previously_done_moves.pop();
    }

    inline void kyokumen_t::clear_additional_info()
    {
        additional_info.check_list_stack.clear();
        additional_info.hash_stack.clear();
        update_king_position_list();
        additional_info.previously_done_moves.clear();
    }

    inline void kyokumen_t::update_king_position_list()
    {
        for (position_t position = position_begin; position < position_end; ++position)
            if (!board_t::out(position) && !board[position].empty() && noncolored_piece_t{ board[position] } == king)
                additional_info.king_position_list[board[position].to_color().value()] = position;
    }

    inline void kyokumen_t::search_aigoma(aigoma_info_t & aigoma_info, color_t color) const
    {
        using pair = std::pair<position_t, std::vector<noncolored_piece_t>>;
        static const std::vector<pair> table
        {
            { front      , { lance, rook, promoted_rook } },
            { left       , { rook, promoted_rook } },
            { right      , { rook, promoted_rook } },
            { back       , { rook, promoted_rook } },
            { front_left , { bishop, promoted_bishop } },
            { front_right, { bishop, promoted_bishop } },
            { back_left  , { bishop, promoted_bishop } },
            { back_right , { bishop, promoted_bishop } },
        };

        const position_t king_position = additional_info.king_position_list[color.value()];
        for (const auto & [offset, hashirigoma_list] : table)
        {
            const position_t reversed_offset = offset * reverse(color);
            const position_t first = search(king_position, reversed_offset);
            if (first != npos && board[first].to_color() == color)
            {
                const position_t second = search(first, reversed_offset);
                if (second != npos && board[second].to_color() != color)
                {
                    const noncolored_piece_t kind = noncolored_piece_t{ board[second] };
                    bool match = std::find(hashirigoma_list.begin(), hashirigoma_list.end(), kind) != hashirigoma_list.end();
                    if (match)
                    {
                        std::vector<position_t> candidates;
                        for (position_t candidate = second; candidate != king_position; candidate -= reversed_offset)
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
    inline void kyokumen_t::search_moves_from_positions(OutputIterator result, position_t source, position_t destination, move_t::tag_t tag) const
    {
        if (promotable(board[source], source, destination))
            *result++ = { source, destination, board[source], board[destination], true, static_cast<move_t::tag_t>(tag | move_t::promote_tag) };
        if (!must_promote(board[source], destination))
            *result++ = { source, destination, board[source], board[destination], false, tag };
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_nonescapes(OutputIterator result) const
    {
        search_moves_moves(result);
        search_moves_puts(result);
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_escapes_king_move(OutputIterator result) const
    {
        const position_t source = additional_info.king_position_list[color().value()];
        for (const position_t * ptr = near_move_offsets(king); *ptr; ++ptr)
        {
            const position_t destination = source + *ptr * reverse(color());
            if (!board_t::out(destination)
                && (board[destination].empty() || board[destination].to_color() != color()))
            {
                const move_t move{ source, destination, board[source], board[destination], false, move_t::escape_tag };
                std::vector<kiki_t> kiki;
                {
                    VALIDATE_KYOKUMEN_ROLLBACK(*this);
                    kyokumen_t & nonconst_this = const_cast<kyokumen_t &>(*this);
                    const colored_piece_t captured = board[destination];
                    nonconst_this.board[destination] = board[source];
                    nonconst_this.board[source] = colored_piece_t{};
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
    inline void kyokumen_t::search_moves_escapes_aigoma(OutputIterator result) const
    {
        const aigoma_info_t aigoma_info = search_aigoma(color());
        const position_t king_pos = additional_info.king_position_list[color().value()];
        
        SHOGIPP_ASSERT(move_count < additional_info.check_list_stack.size());
        const std::size_t saved_count = check_list().size();

        // 合駒を打つ際に打ち歩詰めを判定するため puttable は do_move / undo_move を呼び出す。
        // ここで additional_info.check_list_stack の要素の参照をローカル変数として保持してはならない。
        if (check_list().size() == 1)
        {
            if (check_list().front().aigoma)
            {
                const position_t offset = check_list().front().offset;
                for (position_t destination = king_pos + offset; !board_t::out(destination) && board[destination].empty(); destination += offset)
                {
                    // 駒を移動させる合駒
                    std::vector<kiki_t> kiki_list;
                    search_kiki(std::back_inserter(kiki_list), destination, !color());
                    for (const kiki_t & kiki : kiki_list)
                    {
                        // 王で合駒はできない。
                        if (noncolored_piece_t{ board[kiki.position] } != king)
                        {
                            // 既に合駒として使っている駒は移動できない。
                            const auto aigoma_iter = aigoma_info.find(kiki.position);
                            const bool is_aigoma = aigoma_iter != aigoma_info.end();
                            if (!is_aigoma)
                                search_moves_from_positions(result, kiki.position, destination, move_t::escape_tag | move_t::capture_tag);
                        }
                    }

                    // 駒を打つ合駒
                    for (piece_value_t piece = pawn_value; piece <= rook_value; ++piece)
                        if (captured_pieces_list[color().value()][piece])
                            if (puttable(piece, destination))
                                *result++ = { destination, piece, move_t::escape_tag | move_t::aigoma_tag | move_t::put_tag };
                }
            }

            if (check_list().size() != saved_count)
                std::cerr << sfen_string() << std::endl;
            // 王手している駒を取る手を検索する。
            const position_t destination = check_list().front().position;
            std::vector<kiki_t> kiki_list;
            search_kiki(std::back_inserter(kiki_list), destination, !color());
            for (const kiki_t & kiki : kiki_list)
            {
                // 王を動かす手は既に検索済み
                if (noncolored_piece_t{ board[kiki.position] } != king)
                {
                    // 既に合駒として使っている駒は移動できない。
                    const auto aigoma_iter = aigoma_info.find(kiki.position);
                    const bool is_aigoma = aigoma_iter != aigoma_info.end();
                    if (!is_aigoma)
                        search_moves_from_positions(result, kiki.position, destination, move_t::escape_tag | move_t::capture_tag);
                }
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_escapes(OutputIterator result) const
    {
        search_moves_escapes_king_move(result);
        search_moves_escapes_aigoma(result);
    }

    /**
     * @breif 王手を外さない手のうち駒を動かす手を検索する。
     * @param result 合法手の出力イテレータ
     */
    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_moves(OutputIterator result) const
    {
        const aigoma_info_t aigoma_info = search_aigoma(color());
        std::vector<position_t> source_list;
        source_list.reserve(position_size);
        search_source(std::back_inserter(source_list), color());
        for (const position_t source : source_list)
        {
            std::vector<position_t> destination_list;
            destination_list.reserve(position_size);
            search_destination(std::back_inserter(destination_list), source, color());
            const auto aigoma_iter = aigoma_info.find(source);
            const bool is_aigoma = aigoma_iter != aigoma_info.end();

            for (const position_t destination : destination_list)
            {
#ifndef NDEBUG
                if (!board[destination].empty() && noncolored_piece_t { board[destination] } == king)
                {
                    board.print();
                    std::cout << position_to_string(source) << std::endl;
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
                    const std::vector<position_t> & candidates = aigoma_iter->second;
                    if (std::find(candidates.begin(), candidates.end(), destination) == candidates.end())
                        continue;
                }

                // 利いている場所に王を移動させてはならない
                if (noncolored_piece_t{ board[source] } == king)
                {
                    std::vector<kiki_t> kiki_list;
                    search_kiki(std::back_inserter(kiki_list), destination, color());
                    if (kiki_list.size() > 0)
                        continue;
                }

                search_moves_from_positions(result, source, destination, move_t::none_tag);
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_puts(OutputIterator result) const
    {
        for (piece_value_t piece = pawn_value; piece <= rook_value; ++piece)
            if (captured_pieces_list[color().value()][piece])
                for (position_t destination = position_begin; destination < position_end; ++destination)
                    if (puttable(piece, destination))
                        *result++ = { destination, piece };
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves(OutputIterator result) const
    {
        SHOGIPP_ASSERT(move_count < additional_info.check_list_stack.size());
        auto & check_list = additional_info.check_list_stack[move_count];
        if (check_list.empty())
            search_moves_nonescapes(result);
        else
            search_moves_escapes(result);
    }

    inline moves_t kyokumen_t::search_moves() const
    {
        if (anti_repetition_of_moves)
            return strict_search_moves();
        return nonstrict_search_moves();
    }

    inline moves_t kyokumen_t::nonstrict_search_moves() const
    {
        moves_t moves;
        search_moves(std::back_inserter(moves));
        return moves;
    }

    inline moves_t kyokumen_t::strict_search_moves() const
    {
        moves_t moves;
        search_moves(std::back_inserter(moves));
        remove_repetition_of_moves(moves);
        return moves;
    }

    inline hash_t kyokumen_t::make_hash() const
    {
        hash_t hash{};

        // 盤上の駒のハッシュ値をXOR演算
        for (position_t position = position_begin; position < position_end; ++position)
            if (!board_t::out(position))
                if (const colored_piece_t piece = board[position]; !piece.empty())
                    hash ^= hash_table.piece_hash(piece, position);

        // 持ち駒のハッシュ値をXOR演算
        for (const color_t color : colors)
            for (piece_value_t piece = pawn.value(); piece <= rook.value(); ++piece)
                hash ^= hash_table.captured_piece_hash(captured_piece_t{ piece }, captured_pieces_list[color.value()][captured_piece_t{ piece }], color);

        // 手番のハッシュ値をXOR演算
        hash ^= hash_table.color_hash(color());
        hash ^= hash_table.color_hash(!color());

        return hash;
    }

    inline hash_t kyokumen_t::make_hash(hash_t hash, const move_t & move) const
    {
        if (move.put())
        {
            const captured_pieces_t::size_type count = captured_pieces_list[color().value()][move.captured_piece()];
            SHOGIPP_ASSERT(count > 0);
            const colored_piece_t piece{ move.captured_piece(), color() };
            hash ^= hash_table.piece_hash(piece, move.destination());
            hash ^= hash_table.captured_piece_hash(move.captured_piece(), count, color());
            hash ^= hash_table.captured_piece_hash(move.captured_piece(), count - 1, color());
        }
        else
        {
            SHOGIPP_ASSERT(!(!move.source_piece().is_promotable() && move.promote()));
            hash ^= hash_table.piece_hash(move.source_piece(), move.source());
            if (!move.destination_piece().empty())
            {
                const captured_piece_t new_captured_piece{ move.destination_piece() };
                const captured_pieces_t::size_type count = captured_pieces_list[color().value()][new_captured_piece];
                hash ^= hash_table.captured_piece_hash(new_captured_piece, count, color());
                hash ^= hash_table.captured_piece_hash(new_captured_piece, count + 1, color());
                hash ^= hash_table.piece_hash(move.destination_piece(), move.destination());
            }
            const colored_piece_t new_destination_piece = move.promote() ? move.source_piece().to_promoted() : move.source_piece();
            hash ^= hash_table.piece_hash(new_destination_piece, move.destination());
        }
        hash ^= hash_table.color_hash(!color());
        hash ^= hash_table.color_hash(color());
        return hash;
    }

    inline void kyokumen_t::print_move(const move_t & move, color_t color, std::ostream & ostream) const
    {
        ostream << (color == black ? "▲" : "△");
        if (move.put())
        {
            ostream << position_to_string(move.destination()) << move.captured_piece().to_string() << "打" << std::flush;
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
            ostream
                << position_to_string(move.destination()) << noncolored_piece_t{ move.source_piece() }.to_string() << promotion_string
                << "（" << position_to_string(move.source()) << "）" << std::flush;
        }
    }

    template<typename InputIterator>
    inline void kyokumen_t::print_moves(InputIterator first, InputIterator last, std::ostream & ostream) const
    {
        for (std::size_t i = 0; first != last; ++i)
        {
            const std::ios::fmtflags flags = ostream.flags();
            ostream << "#" << std::setw(3) << (i + 1);
            ostream.flags(flags);
            print_move(*first++, color(), ostream);
            ostream << std::endl;
        }
    }

    inline void kyokumen_t::print_moves(std::ostream & ostream) const
    {
        const kyokumen_t temp = *this;
        const moves_t moves = temp.strict_search_moves();
        print_moves(moves.begin(), moves.end(), ostream);
    }

    inline void kyokumen_t::print_check(std::ostream & ostream) const
    {
        if (details::program_options::print_check)
        {
            SHOGIPP_ASSERT(move_count < additional_info.check_list_stack.size());
            auto & check_list = additional_info.check_list_stack[move_count];
            if (!check_list.empty())
            {
                ostream << "王手：";
                for (std::size_t i = 0; i < check_list.size(); ++i)
                {
                    const kiki_t & kiki = check_list[i];
                    if (i > 0)
                        ostream << "　";
                    ostream << position_to_string(kiki.position) << noncolored_piece_t{ board[kiki.position] }.to_string() << std::endl;
                }
            }
        }
    }

    inline void kyokumen_t::print(std::ostream & ostream) const
    {
        if (details::program_options::print_board)
        {
            ostream << "後手持ち駒：";
            captured_pieces_list[white.value()].print(ostream);
            board.print(ostream);
            ostream << "先手持ち駒：";
            captured_pieces_list[black.value()].print(ostream);
        }
    }

    inline void kyokumen_t::print_kifu(std::ostream & ostream) const
    {
        for (move_count_t i = 0; i < static_cast<move_count_t>(kifu.size()); ++i)
        {
            const move_count_t diff = static_cast<move_count_t>(kifu.size() - i);
            print_move(kifu[i], diff % color_t::size() == 0 ? color() : !color(), ostream);
            ostream << std::endl;
        }
    }

    inline void kyokumen_t::print_recent_kifu(std::size_t size, std::ostream & ostream) const
    {
        size = std::min(size, kifu.size());
        for (move_count_t i = 0; i < size; ++i)
        {
            if (i > 0)
                ostream << "　";
            std::size_t index = kifu.size() - size + i;
            const move_count_t diff = static_cast<move_count_t>(kifu.size() - index);
            print_move(kifu[index], diff % color_t::size() == 0 ? color() : !color(), ostream);
        }
        ostream << std::flush;
    }

    inline hash_t kyokumen_t::hash() const
    {
        SHOGIPP_ASSERT(move_count < additional_info.hash_stack.size());
        return additional_info.hash_stack[move_count];
    }

    inline void kyokumen_t::validate_board_out()
    {
        for (position_t position = 0; position < position_size; ++position)
            if (board_t::out(position))
                SHOGIPP_ASSERT(board[position].value() == out_of_range.value());
    }

    inline void kyokumen_t::do_move(const move_t & move)
    {
        hash_t hash = make_hash(this->hash(), move);
        if (move.put())
        {
            SHOGIPP_ASSERT(captured_pieces_list[color().value()][move.captured_piece()] > 0);
            board[move.destination()] = colored_piece_t{ move.captured_piece(), color() };
            --captured_pieces_list[color().value()][move.captured_piece()];
        }
        else
        {
            SHOGIPP_ASSERT(!(!move.source_piece().is_promotable() && move.promote()));
            if (!board[move.destination()].empty())
                ++captured_pieces_list[color().value()][captured_piece_t{ board[move.destination()] }];
            board[move.destination()] = move.promote() ? board[move.source()].to_promoted() : board[move.source()];
            board[move.source()] = colored_piece_t{};
            if (noncolored_piece_t{ move.source_piece() } == king)
                additional_info.king_position_list[color().value()] = move.destination();
        }
        ++move_count;
        kifu.push_back(move);
        push_additional_info(hash);
        validate_board_out();
    }

    inline void kyokumen_t::undo_move(const move_t & move)
    {
        SHOGIPP_ASSERT(move_count > 0);
        --move_count;
        if (move.put())
        {
            ++captured_pieces_list[color().value()][move.captured_piece()];
            board[move.destination()] = colored_piece_t{};
        }
        else
        {
            if (noncolored_piece_t{ move.source_piece() } == king)
                additional_info.king_position_list[color().value()] = move.source();
            board[move.source()] = move.source_piece();
            board[move.destination()] = move.destination_piece();
            if (!move.destination_piece().empty())
                --captured_pieces_list[color().value()][captured_piece_t{ move.destination_piece() }];
        }
        kifu.pop_back();
        pop_additional_info();
    }

    inline std::optional<move_t> kyokumen_t::last_move() const noexcept
    {
        if (kifu.empty())
            return std::nullopt;
        return kifu.back();
    }

    inline color_t kyokumen_t::color() const
    {
        return static_cast<color_t>(move_count % 2);
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

    inline std::string kyokumen_t::sfen_string() const
    {
        std::string result;
        result += initial_sfen_string;
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

    inline void kyokumen_t::remove_repetition_of_moves(moves_t & moves) const noexcept
    {
        const hash_t hash = this->hash();
        const color_t color = this->color();
        moves.erase(std::remove_if(moves.begin(), moves.end(), [&](const auto & i) -> bool
            {
                return additional_info.previously_done_moves.contains(
                    hash_table.move_hash(i, color) ^ hash
                );
            }
        ), moves.end());
    }

    inline position_t kyokumen_t::king_position(color_t color) const noexcept
    {
        return additional_info.king_position_list[color.value()];
    }

    inline const std::vector<kiki_t> & kyokumen_t::check_list() const noexcept
    {
        SHOGIPP_ASSERT(move_count < additional_info.check_list_stack.size());
        return additional_info.check_list_stack[move_count];
    }

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
    using cache_t = lru_cache_t<hash_t, evaluation_value_t, basic_hash_hasher_t<SIZE_OF_HASH>>;
#else
    using cache_t = lru_cache_t<hash_t, evaluation_value_t>;
#endif

    /**
     * @breif 盤の2駒の組と対応する評価値の統計
     */
    class piece_pair_statistics_t
    {
    public:
        using value_type = unsigned long long;
        constexpr static std::size_t data_size = (file_size * rank_size - 1) * (piece_size / 2) * piece_size;

        /**
         * @breif 盤の2駒の組と対応する評価値の統計を構築する。
         * @details 全ての評価値は 0 で初期化される。
         */
        constexpr inline piece_pair_statistics_t() noexcept = default;
        constexpr inline piece_pair_statistics_t(const piece_pair_statistics_t &) noexcept = default;
        constexpr inline piece_pair_statistics_t(piece_pair_statistics_t &&) noexcept = default;
        constexpr inline piece_pair_statistics_t & operator =(const piece_pair_statistics_t &) noexcept = default;
        constexpr inline piece_pair_statistics_t & operator =(piece_pair_statistics_t &&) noexcept = default;

        /**
         * @breif 駒と座標の組を正規化する。
         *        座標2 >= 座標1 を満たすように組を入れ替える。
         *        相対的な筋は常に正の値に変換される。換言すれば、この関数は位置関係を左右対称に扱う。
         *        また、駒1と駒2の関係(同手番である、あるいは医)を維持したまま駒1を先手の駒に変換する。
         * @param piece1 駒1
         * @param position1 座標1
         * @param piece2 駒2
         * @param position2 座標2
         * @return 並び替えた後の座標1を基準とした1-81の相対座標
         */
        inline static position_t canonicalize(colored_piece_t & piece1, position_t & position1, colored_piece_t & piece2, position_t & position2)
        {
            SHOGIPP_ASSERT(position1 != position2);
            
            if (position1 > position2)
            {
                std::swap(piece1, piece2);
                std::swap(position1, position2);
            }
            position_t relative_file = position_to_file(position2) - position_to_file(position1);
            if (relative_file < 0) // abs
                relative_file = -relative_file;
            const position_t relative_rank = position_to_rank(position2) - position_to_rank(position1);
            
            if (piece1.to_color() == white)
            {
                piece1 = colored_piece_t{ noncolored_piece_t{ piece1 }, black };
                piece2 = colored_piece_t{ noncolored_piece_t{ piece2 }, !piece2.to_color() };
            }
            
            return relative_rank * file_size + relative_file;
        }

        /**
         * @breif 盤の2駒の組と対応する評価値のオフセットを取得する。
         * @param piece1 駒1
         * @param position1 座標1
         * @param piece2 駒2
         * @param position2 座標2
         * @return 盤の2駒の組と対応する評価値のオフセット
         */
        inline std::size_t offset(colored_piece_t & piece1, position_t & position1, colored_piece_t & piece2, position_t & position2) const
        {
            std::size_t offset = 0;
            const position_t relative_position = canonicalize(piece1, position1, piece2, position2);
            offset = relative_position - 1;
            offset *= piece_size / 2;
            offset += piece1.value();
            offset *= piece_size;
            offset += piece2.value();
            return offset;
        }

        /**
         * @breif 盤の2駒の組と対応する評価値を取得する。
         * @param piece1 駒1
         * @param position1 座標1
         * @param piece2 駒2
         * @param position2 座標2
         * @return 盤の2駒の組と対応する評価値
         * @details この関数は先手から見て有利な状況を正とする評価値を返す。
         */
        inline value_type get(colored_piece_t piece1, position_t position1, colored_piece_t piece2, position_t position2) const
        {
            const std::size_t offset = this->offset(piece1, position1, piece2, position2);
            SHOGIPP_ASSERT(offset < std::size(m_data));
            return m_data[offset] * reverse(piece1.to_color());
        }

        /**
         * @breif 盤の全ての2駒の組み合わせを引数に callback を呼び出す。
         * @param board 盤
         * @param callback コールバック関数 void (colored_piece_t piece1, position_t position1, colored_piece_t piece2, position_t position2)
         */
        template<typename Callback>
        inline static void for_each(const board_t & board, Callback callback)
        {
            for (position_t position1 = position_begin; position1 < position_end; ++position1)
                for (position_t position2 = position1 + 1; position2 < position_end; ++position2)
                    if (position1 != position2
                        && !board_t::out(position1)
                        && !board_t::out(position2)
                        && !board[position1].empty()
                        && !board[position2].empty())
                        callback(board[position1], position1, board[position2], position2);
        }

        /**
         * @breif 指定された駒を一方とする盤の全ての2駒の組み合わせを引数に callback を呼び出す。
         * @param board 盤
         * @param piece1 駒1
         * @param position1 座標1
         * @param callback コールバック関数 void (colored_piece_t piece2, position_t position2)
         */
        template<typename Callback>
        inline static void for_each(const board_t & board, colored_piece_t piece1, position_t position1, Callback callback)
        {
            for (position_t position2 = position_begin; position2 < position_end; ++position2)
                if (position1 != position2
                    && !board_t::out(position1)
                    && !board_t::out(position2)
                    && !piece1.empty()
                    && !board[position2].empty())
                    callback(board[position2], position2);
        }

        /**
         * @breif 盤の2駒の組と対応する評価値を 1 加算する。
         * @param piece1 駒1
         * @param position1 座標1
         * @param piece2 駒2
         * @param position2 座標2
         * @details 加算により評価値がオーバーフローする場合、この関数は加算の前に全ての評価値をそれらの比率を維持したまま減らす。
         */
        inline void increase(colored_piece_t piece1, position_t position1, colored_piece_t piece2, position_t position2)
        {
            const std::size_t offset = this->offset(piece1, position1, piece2, position2);
            SHOGIPP_ASSERT(offset < std::size(m_data));
            if (m_max == std::numeric_limits<value_type>::max())
                normalize();
            ++m_data[offset];
            if (m_max < m_data[offset])
                m_max = m_data[offset];
        }

        /**
         * @breif 盤の全ての2駒の組と対応する評価値を 1 加算する。
         * @param board 盤
         * @details 加算により評価値がオーバーフローする場合、この関数は加算の前に全ての評価値をそれらの比率を維持したまま減らす。
         */
        inline void increase(const board_t & board)
        {
            const auto callback = [&](colored_piece_t piece1, position_t position1, colored_piece_t piece2, position_t position2)
            {
                increase(piece1, position1, piece2, position2);
            };
            for_each(board, callback);
        }

        /**
         * @breif 盤の評価値を取得する。
         * @param board 盤
         * @return 盤の評価値
         */
        inline value_type accumulate(const board_t & board) const
        {
            value_type accumulated_value = 0;
            const auto callback = [&](colored_piece_t piece1, position_t position1, colored_piece_t piece2, position_t position2)
            {
                accumulated_value += get(piece1, position1, piece2, position2);
            };
            for_each(board, callback);
            return accumulated_value;
        }

        /**
         * @breif 盤の評価値の最大値を取得する。
         * @return 盤の評価値の最大値
         */
        inline value_type max() const noexcept
        {
            return m_max;
        }

        /**
         * @breif 合法手を実行した際の評価値の差分を取得する。
         * @param board 合法手を実行した後の盤
         * @param move 合法手
         * @return 合法手を実行した際の評価値の差分
         */
        inline value_type accumulate_diff(const board_t & board, const move_t & move) const
        {
            value_type accumulated_value = 0;

            // 移動先にある駒の評価値を加算する。
            const colored_piece_t piece1 = board[move.destination()];
            const position_t position1 = move.destination();
            {
                const auto callback = [&](colored_piece_t piece2, position_t position2)
                {
                    accumulated_value += get(piece1, position1, board[position2], position2);
                };
                for_each(board, piece1, position1, callback);
            }

            if (!move.put())
            {
                // 移動先にあった駒の評価値を減算する。
                if (!move.destination_piece().empty())
                {
                    const colored_piece_t piece1 = move.destination_piece();
                    const position_t position1 = move.destination();
                    const auto callback = [&](colored_piece_t piece2, position_t position2)
                    {
                        accumulated_value -= get(piece1, position1, board[position2], position2);
                    };
                    for_each(board, piece1, position1, callback);
                }

                // 移動元にあった駒の評価値を減算する。
                const colored_piece_t piece1 = move.source_piece();
                const position_t position1 = move.source();
                const auto callback = [&](colored_piece_t piece2, position_t position2)
                {
                    accumulated_value -= get(piece1, position1, board[position2], position2);
                };
                for_each(board, piece1, position1, callback);
            }

            return accumulated_value;
        }

        /**
         * @breif 出現回数の多い駒関係のうち上位 n 件を出力する。
         * @param n 表示する件数
         * @param ostream 出力ストリーム
         */
        inline void print_most_frequent(std::size_t n, std::ostream & ostream = std::cout) const
        {
            class element_t
            {
            public:
                colored_piece_t piece1{};
                colored_piece_t piece2{};
                position_t relative_position{};
                value_type value{};
            };

            std::vector<element_t> elements;
            for (std::size_t i = 0; i < std::size(m_data); ++i)
            {
                std::size_t temp = i;
                element_t element;
                element.piece2 = colored_piece_t{ temp % piece_size };
                temp /= piece_size;
                element.piece2 = colored_piece_t{ temp % (piece_size / 2) };
                temp /= piece_size / 2;
                element.relative_position = static_cast<position_t>(temp);
                element.value = m_data[i];
                elements.push_back(element);
            }

            const auto comparator = [](const element_t & a, const element_t & b) -> bool
            {
                return a.value > b.value;
            };

            std::sort(elements.begin(), elements.end(), comparator);

            for (std::size_t i = 0; i < n; ++i)
            {
                const element_t & element = elements[i];
                board_t board;
                board.clear();
                const position_t relative_file = element.relative_position % file_size;
                const position_t relative_rank = element.relative_position / file_size;
                constexpr position_t position1 = file_rank_to_position(0, rank_size - 1);
                const position_t position2 = file_rank_to_position(0 + relative_file, rank_size - 1 - relative_rank);
                board[position1] = element.piece1;
                board[position2] = element.piece2;
                board.print(ostream);
                ostream << "count: " << element.value << std::endl << std::endl;
            }
        }

        /**
         * @breif ファイルを読み込む。
         * @param path ファイルのパス
         */
        inline void read_file(const std::filesystem::path & path)
        {
            m_max = std::numeric_limits<value_type>::min();
            std::ifstream out(path, std::ios_base::in | std::ios::binary);
            out.read(reinterpret_cast<char *>(m_data), sizeof(m_data));
            for (value_type value : m_data)
                if (m_max < value)
                    m_max = value;
        }

        /**
         * @breif ファイルに書き出す。
         * @param path ファイルのパス
         */
        inline void write_file(const std::filesystem::path & path) const
        {
            std::ofstream out(path, std::ios_base::out | std::ios::binary);
            out.write(reinterpret_cast<const char *>(m_data), sizeof(m_data));
        }

    private:
        value_type m_max{};
        value_type m_data[data_size]{};

        /**
         * @breif 評価値を加算してもオーバーフローが発生しないよう、全ての評価値をそれらの比率を維持したまま減らす。
         */
        inline void normalize() noexcept
        {
            constexpr value_type d = 2;
            for (value_type & value : m_data)
                value /= d;
            m_max /= d;
        }
    };

    namespace details
    {
        piece_pair_statistics_t piece_pair_statistics;
    }

    /**
     * @breif 評価関数オブジェクトが呼び出された文脈を表現する。
     */
    class iddfs_context_t
    {
    public:
        constexpr inline iddfs_context_t() noexcept = default;

        constexpr inline iddfs_context_t(
            iddfs_iteration_t max_iddfs_iteration,
            std::chrono::milliseconds limit_time
        ) noexcept
            : m_max_iddfs_iteration{ max_iddfs_iteration }
            , m_limit_time{ limit_time }
        {
        }

        constexpr inline iddfs_context_t(iddfs_context_t & context) noexcept = default;
        constexpr inline iddfs_context_t(iddfs_context_t && context) noexcept = default;
        constexpr inline iddfs_context_t & operator =(iddfs_context_t & context) noexcept = default;
        constexpr inline iddfs_context_t & operator =(iddfs_context_t && context) noexcept = default;

        inline iddfs_iteration_t max_iddfs_iteration() const noexcept
        {
            return m_max_iddfs_iteration;
        }

        inline void start() noexcept
        {
            m_begin = std::chrono::system_clock::now();
        }

        inline bool timeout() const noexcept
        {
            const std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(end - m_begin) >= std::chrono::milliseconds{ m_limit_time };
        }

    private:
        iddfs_iteration_t m_max_iddfs_iteration{};
        std::chrono::milliseconds m_limit_time{};
        std::chrono::system_clock::time_point m_begin;
    };

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
         * @param context 評価関数オブジェクトが呼び出された文脈
         * @param iddfs_iteration IDDFSの反復回数
         * @return 選択された合法手
         * @details 制限時間内に探索が終了しなかった場合、 timeout_exception を送出する。
         * @sa best_move
         * @sa best_move_iddfs
         */
        virtual move_t query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration) = 0;

        /**
         * @breif 局面に対して合法手を選択する。
         * @param kyokumen 局面
         * @param context 評価関数オブジェクトが呼び出された文脈
         * @return 選択された合法手
         * @details この関数は timeout_exception を送出しない。
         */
        virtual move_t best_move(kyokumen_t & kyokumen, iddfs_context_t & context);

        /**
         * @breif 反復深化深さ優先探索で局面に対して合法手を選択する。
         * @param kyokumen 局面
         * @param context 評価関数オブジェクトが呼び出された文脈
         * @return 選択された合法手
         * @details この関数は timeout_exception を送出しない。
         */
        virtual move_t best_move_iddfs(kyokumen_t & kyokumen, iddfs_context_t & context);

        /**
         * @breif 評価関数オブジェクトの名前を返す。
         * @return 評価関数オブジェクトの名前
         */
        virtual std::string name() const = 0;
    };

    move_t abstract_evaluator_t::best_move(kyokumen_t & kyokumen, iddfs_context_t & context)
    {
        std::optional<move_t> opt_best_move;

        try
        {
            kyokumen_t duplicated{ kyokumen };
            opt_best_move = query_best_move(duplicated, context, 0);
        }
        catch (const timeout_exception &)
        {
            ;
        }
        catch (...)
        {
            std::cerr << "best_move failed" << std::endl;
        }

        // 最善手を取得できなかった場合適当な合法手を選択する。
        if (!opt_best_move)
        {
            const moves_t moves = kyokumen.strict_search_moves();
            return moves.front();
        }

        return *opt_best_move;
    }

    move_t abstract_evaluator_t::best_move_iddfs(kyokumen_t & kyokumen, iddfs_context_t & context)
    {
        std::optional<move_t> opt_best_move;
        iddfs_iteration_t last_iddfs_iteration = 0;
        try
        {
            for (iddfs_iteration_t iddf_iteration = 0; iddf_iteration <= context.max_iddfs_iteration(); ++iddf_iteration)
            {
                kyokumen_t duplicated{ kyokumen };
                opt_best_move = query_best_move(duplicated, context, iddf_iteration);
                last_iddfs_iteration = iddf_iteration;
            }
        }
        catch (const timeout_exception &)
        {
            ;
        }
        catch (...)
        {
            std::cerr << "best_move_iddfs failed" << std::endl;
        }

        // 最善手を取得できなかった場合適当な合法手を選択する。
        if (!opt_best_move)
        {
            const moves_t moves = kyokumen.strict_search_moves();
            return moves.front();
        }

        return *opt_best_move;
    }

    /**
     * @breif 標準入力により合法手を選択する評価関数オブジェクト
     */
    class command_line_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        move_t query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration) override
        {
            bool selected = false;

            unsigned int id;
            moves_t moves = kyokumen.search_moves();
            
            while (!selected)
            {
                try
                {
                    std::cout << "#" << std::flush;
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

        std::string name() const override
        {
            return "command_line_evaluator";
        }
    };

    void print_help()
    {
        std::cout
            << "shogipp.exe" << std::endl
            << "shogipp.exe --black <evaluator> --white <evaluator> [--max-depth <max-depth>] [--max-selective-depth <max-selective-depth>]" << std::endl
            << "shogipp.exe --ga-chromosome <chromosome-directory>" << std::endl
            << "    [--ga-create-chromosome <chromosome-number>]" << std::endl
            << "    [--ga-mutation-rate <ga-mutation-rate>]" << std::endl
            << "    [--ga-crossover-rate <ga-crossover-rate>]" << std::endl
            << "    [--ga-selection-rate <ga-selection-rate>]" << std::endl
            << "    [--ga-iteration <iteration-number>]" << std::endl
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
                    const char * ptr = argv[i] + 2;
                    const char * begin = ptr;
                    while (*ptr && !std::isspace(*ptr))
                        ++ptr;
                    params_map[std::string{ begin, ptr }];
                    current_option = std::string(begin, ptr);
                }
                else
                {
                    const char * ptr = argv[i] + 1;
                    const char option = *ptr;
                    while (std::isalpha(*ptr))
                    {
                        params_map[std::string{ *ptr }];
                        ++ptr;
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
                const char * arg = argv[i];
                while (std::isspace(*arg))
                    ++arg;
                while (*arg)
                {
                    while (std::isspace(*arg))
                        ++arg;
                    if (!*arg)
                        break;
                    auto qiter = std::find(std::begin(quotations), std::end(quotations), *arg);
                    if (qiter != std::end(quotations))
                    {
                        const char * begin = arg;
                        const char quotation = *qiter;
                        while (*arg && *arg != quotation)
                            ++arg;
                        if (*arg)
                            ++arg;
                        params_map[current_option].emplace_back(begin, arg);
                    }
                    else
                    {
                        const char * begin = arg;
                        while (*arg && !std::isspace(*arg))
                            ++arg;
                        params_map[current_option].emplace_back(begin, arg);
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

        for (position_t position = position_begin; position < position_end; ++position)
        {
            const colored_piece_t piece = kyokumen.board[position];
            if (!board_t::out(position) && !piece.empty())
                evaluation_value += map[noncolored_piece_t{ piece }.value()] * reverse(piece.to_color());
        }

        for (const color_t color : colors)
            for (piece_value_t piece = pawn_value; piece <= rook_value; ++piece)
                evaluation_value += map[piece] * kyokumen.captured_pieces_list[color.value()][piece] * reverse(color);

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
     * @breif 合法手をその区分により並び替える。
     * @param first evaluated_moves の先頭を指すランダムアクセスイテレータ
     * @param last evaluated_moves の末尾を指すランダムアクセスイテレータ
     * @details alpha-beta 法で探索する前に合法手を最善手が期待される順に並び替えることで探索を効率化する。
     */
    template<typename RandomAccessIterator>
    void sort_moves_by_category(RandomAccessIterator first, RandomAccessIterator last)
    {
        std::sort(first, last, [](const move_t & a, const move_t & b) -> bool { return to_category(a) > to_category(b); });
    }

    /**
     * @breif 合法手をその評価値により並び替える。
     * @param first first の先頭を指すランダムアクセスイテレータ
     * @param last last の末尾を指すランダムアクセスイテレータ
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
        std::chrono::milliseconds limit_time{};
        std::map<std::string, std::string> options;
        bool ponder = false;

        mutable std::recursive_mutex mutex;

        /**
         * @breif begin と現在時間の差分をミリ秒単位で返す。
         * @return begin と現在時間の差分
         */
        inline std::chrono::milliseconds time() const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            const std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            return static_cast<std::chrono::milliseconds>(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
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
            const std::chrono::milliseconds milli_second_time = time();
            if (milli_second_time.count() == 0)
                return 0;
            return nodes * 1000 / milli_second_time.count();
        }

        /**
         * @breif 即時的な出力を行う。
         */
        inline void ondemand_print() const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            std::cout << "info"
                //<< " pv " << pv
                //<< " multipv " << multipv
                << " score cp " << cp
                //<< " score mate " << mate
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
                << " time " << time().count()
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

        /**
         * @breif depth と nodes を更新する。
         * @param depth 現在の探索深度
         * @details この関数は USI サーバーから stop コマンドが通知されている場合、 timeout_exception を送出する。
         *          この関数は再帰的に呼び出される探索関数の先頭で呼び出される必要がある。
         */
        inline void resolve_request_to_stop(depth_t depth)
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            if (state == usi_info_t::state_t::requested_to_stop)
                throw timeout_exception{ "requested to stop" };
            this->depth = depth;
            this->nodes += 1;
        }

        /**
         * @breif cache_hit_count を1増加させる。
         */
        inline void increase_cache_hit_count()
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            ++cache_hit_count;
        }

        /**
         * @breif 現在探索している合法手を通知する。
         * @param currmove 現在探索している合法手
         * @details この関数は深度0の再帰で呼び出される。
         */
        inline void notify_currmove(const move_t & currmove)
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            this->currmove = currmove;
            ondemand_print();
        }

        /**
         * @breif 最善手を通知する。
         * @param best_move 最善手
         * @param cp 歩1枚を100とした場合の評価値
         */
        inline void notify_best_move(const move_t & best_move, evaluation_value_t cp)
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            this->best_move = best_move;
            this->cp = cp;
        }

        inline void notify_search_begin()
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            begin = std::chrono::system_clock::now();
            state = usi_info_t::state_t::searching;
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
        move_t query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration) override;
        std::shared_ptr<usi_info_t> usi_info;

    private:
        class arguments_t
        {
        public:
            cache_t & cache;
            iddfs_context_t & context;
            const depth_t max_depth{};
        };

        evaluation_value_t negamax(
            kyokumen_t & kyokumen,
            depth_t depth,
            std::optional<move_t> & candidate_move,
            arguments_t & arguments
        );
    };

    evaluation_value_t negamax_evaluator_t::negamax(
        kyokumen_t & kyokumen,
        depth_t depth,
        std::optional<move_t> & candidate_move,
        arguments_t & arguments
    )
    {
        if (usi_info)
            usi_info->resolve_request_to_stop(depth);

        if (depth >= arguments.max_depth)
        {
            if (arguments.context.timeout())
                throw timeout_exception{ "context.timeout() == true" };

            ++details::timer.search_count();
            const std::optional<evaluation_value_t> cached_evaluation_value = arguments.cache.get(kyokumen.hash());
            if (cached_evaluation_value)
            {
                if (usi_info)
                    usi_info->increase_cache_hit_count();
                return *cached_evaluation_value;
            }
            const evaluation_value_t evaluation_value = evaluate(kyokumen) * reverse(kyokumen.color());
            arguments.cache.push(kyokumen.hash(), evaluation_value);
            return evaluation_value;
        }

        moves_t moves = kyokumen.search_moves();

        if (moves.empty())
            return -std::numeric_limits<evaluation_value_t>::max();

        std::vector<evaluated_moves> evaluated_moves;
        auto inserter = std::back_inserter(evaluated_moves);
        evaluation_value_t max_evaluation_value = -std::numeric_limits<evaluation_value_t>::max();

        for (const move_t & move : moves)
        {
            if (usi_info && depth == 0)
                usi_info->notify_currmove(move);

            std::optional<move_t> nested_candidate_move;
            evaluation_value_t evaluation_value;
            {
                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                evaluation_value = -negamax(kyokumen, depth + 1, nested_candidate_move, arguments);
                kyokumen.undo_move(move);
            }
            *inserter++ = { &move, evaluation_value };

            if (usi_info && depth == 0 && evaluation_value > max_evaluation_value)
                usi_info->notify_best_move(move, evaluation_value);

            max_evaluation_value = evaluation_value;
        }

        SHOGIPP_ASSERT(!moves.empty());
        sort_moves_by_evaluation_value(evaluated_moves.begin(), evaluated_moves.end());
        candidate_move = *evaluated_moves.front().first;
        return evaluated_moves.front().second;
    }

    move_t negamax_evaluator_t::query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration)
    {
        if (usi_info)
            usi_info->notify_search_begin();

        cache_t cache = usi_info_t::make_cache(usi_info.get());
        std::optional<move_t> candidate_move;
        evaluation_value_t evaluation_value;
        const depth_t max_depth = iddfs_iteration * 2 + 1;
        arguments_t arguments{ cache, context, max_depth };

        try
        {
            evaluation_value = negamax(kyokumen, 0, candidate_move, arguments);
        }
        catch (const timeout_exception &)
        {
            ;
        }

        if (usi_info)
            usi_info->terminate();

        if (!candidate_move)
            throw timeout_exception{ "query_best_move failed" };

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
        move_t query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration) override;
        std::shared_ptr<usi_info_t> usi_info;

    private:
        class arguments_t
        {
        public:
            cache_t & cache;
            iddfs_context_t & context;
            const depth_t max_depth{};
        };

        evaluation_value_t alphabeta(
            kyokumen_t & kyokumen,
            depth_t depth,
            evaluation_value_t alpha,
            evaluation_value_t beta,
            std::optional<move_t> & candidate_move,
            arguments_t & arguments
        );
    };

    evaluation_value_t alphabeta_evaluator_t::alphabeta(
        kyokumen_t & kyokumen,
        depth_t depth,
        evaluation_value_t alpha,
        evaluation_value_t beta,
        std::optional<move_t> & candidate_move,
        arguments_t & arguments
    )
    {
        if (usi_info)
            usi_info->resolve_request_to_stop(depth);

        if (depth >= arguments.max_depth)
        {
            if (arguments.context.timeout())
                throw timeout_exception{ "context.timeout() == true" };

            ++details::timer.search_count();
            const std::optional<evaluation_value_t> cached_evaluation_value = arguments.cache.get(kyokumen.hash());
            if (cached_evaluation_value)
            {
                if (usi_info)
                    usi_info->increase_cache_hit_count();
                return *cached_evaluation_value;
            }
            const evaluation_value_t evaluation_value = evaluate(kyokumen) * reverse(kyokumen.color());
            arguments.cache.push(kyokumen.hash(), evaluation_value);
            return evaluation_value;
        }

        moves_t moves = kyokumen.search_moves();
        sort_moves_by_category(moves.begin(), moves.end());

        if (moves.empty())
            return -std::numeric_limits<evaluation_value_t>::max();

        std::vector<evaluated_moves> evaluated_moves;
        auto inserter = std::back_inserter(evaluated_moves);
        evaluation_value_t max_evaluation_value = -std::numeric_limits<evaluation_value_t>::max();

        for (const move_t & move : moves)
        {
            if (usi_info && depth == 0)
                usi_info->notify_currmove(move);

            std::optional<move_t> nested_candidate_move;
            evaluation_value_t evaluation_value;
            {
                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                evaluation_value = -alphabeta(kyokumen, depth + 1, -beta, -alpha, nested_candidate_move, arguments);
                kyokumen.undo_move(move);
            }
            *inserter++ = { &move, evaluation_value };

            if (usi_info && depth == 0 && evaluation_value > max_evaluation_value)
                usi_info->notify_best_move(move, evaluation_value);

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

    move_t alphabeta_evaluator_t::query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration)
    {
        if (usi_info)
            usi_info->notify_search_begin();

        cache_t cache = usi_info_t::make_cache(usi_info.get());
        std::optional<move_t> candidate_move;
        evaluation_value_t evaluation_value;
        const depth_t max_depth = iddfs_iteration * 2 + 1;
        arguments_t arguments{ cache, context, max_depth };

        try
        {
            evaluation_value = alphabeta(kyokumen, 0, -std::numeric_limits<evaluation_value_t>::max(), std::numeric_limits<evaluation_value_t>::max(), candidate_move, arguments);
        }
        catch (const timeout_exception &)
        {
            ;
        }

        if (usi_info)
            usi_info->terminate();

        if (!candidate_move)
            throw timeout_exception{ "query_best_move failed" };

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
        move_t query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration) override;
        std::shared_ptr<usi_info_t> usi_info;

    private:
        class arguments_t
        {
        public:
            cache_t & cache;
            iddfs_context_t & context;
            const depth_t max_depth{};
            const depth_t max_selective_depth{};
        };

        evaluation_value_t extendable_alphabeta(
            kyokumen_t & kyokumen,
            depth_t depth,
            evaluation_value_t alpha,
            evaluation_value_t beta,
            std::optional<move_t> & candidate_move,
            position_t previous_destination,
            arguments_t arguments
        );
    };

    evaluation_value_t extendable_alphabeta_evaluator_t::extendable_alphabeta(
        kyokumen_t & kyokumen,
        depth_t depth,
        evaluation_value_t alpha,
        evaluation_value_t beta,
        std::optional<move_t> & candidate_move,
        position_t previous_destination,
        arguments_t arguments
    )
    {
        if (usi_info)
            usi_info->resolve_request_to_stop(depth);
        
        if (depth >= arguments.max_depth)
        {
            if (arguments.context.timeout())
                throw timeout_exception{ "context.timeout() == true" };
            
            // 前回駒取りが発生していた場合、探索を延長する。
            if (depth >= arguments.max_selective_depth && previous_destination != npos)
            {
                std::vector<evaluated_moves> evaluated_moves;
                auto inserter = std::back_inserter(evaluated_moves);
                const moves_t moves = kyokumen.search_moves();
                for (const move_t & move : moves)
                {
                    if (!move.put() && move.destination() == previous_destination)
                    {
                        std::optional<move_t> nested_candidate_move;
                        evaluation_value_t evaluation_value;
                        {
                            VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                            kyokumen.do_move(move);
                            evaluation_value = -extendable_alphabeta(kyokumen, depth + 1, -beta, -alpha, nested_candidate_move, previous_destination, arguments);
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

            ++details::timer.search_count();
            const std::optional<evaluation_value_t> cached_evaluation_value = arguments.cache.get(kyokumen.hash());
            if (cached_evaluation_value)
            {
                if (usi_info)
                    usi_info->increase_cache_hit_count();
                return *cached_evaluation_value;
            }
            const evaluation_value_t evaluation_value = evaluate(kyokumen) * reverse(kyokumen.color());
            arguments.cache.push(kyokumen.hash(), evaluation_value);
            return evaluation_value;
        }

        moves_t moves = kyokumen.search_moves();
        sort_moves_by_category(moves.begin(), moves.end());

        if (moves.empty())
            return -std::numeric_limits<evaluation_value_t>::max();

        std::vector<evaluated_moves> evaluated_moves;
        auto inserter = std::back_inserter(evaluated_moves);
        evaluation_value_t max_evaluation_value = -std::numeric_limits<evaluation_value_t>::max();

        for (const move_t & move : moves)
        {
            if (usi_info && depth == 0)
                usi_info->notify_currmove(move);

            std::optional<move_t> nested_candidate_move;
            position_t destination = (!move.put() && !move.destination_piece().empty()) ? move.destination() : npos;
            evaluation_value_t evaluation_value;
            {
                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                evaluation_value = -extendable_alphabeta(kyokumen, depth + 1, -beta, -alpha, nested_candidate_move, destination, arguments);
                kyokumen.undo_move(move);
            }
            *inserter++ = { &move, evaluation_value };

            if (usi_info && depth == 0 && evaluation_value > max_evaluation_value)
                usi_info->notify_best_move(move, evaluation_value);

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

    move_t extendable_alphabeta_evaluator_t::query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration)
    {
        if (usi_info)
            usi_info->notify_search_begin();

        cache_t cache = usi_info_t::make_cache(usi_info.get());
        std::optional<move_t> candidate_move;
        evaluation_value_t evaluation_value;
        const depth_t max_depth = iddfs_iteration * 2 + 1;
        const depth_t max_selective_depth = std::numeric_limits<depth_t>::max();
        arguments_t arguments{ cache, context, max_depth, max_selective_depth };
        
        try
        {
            evaluation_value = extendable_alphabeta(kyokumen, 0, -std::numeric_limits<evaluation_value_t>::max(), std::numeric_limits<evaluation_value_t>::max(), candidate_move, npos, arguments);
        }
        catch (const timeout_exception &)
        {
            ;
        }

        if (usi_info)
            usi_info->terminate();

        if (!candidate_move)
            throw timeout_exception{ "query_best_move failed" };

        return *candidate_move;
    }

    /**
     * @breif 枝刈りを含む alphabeta で合法手を選択する評価関数オブジェクトの抽象クラス
     */
    class pruning_alphabeta_evaluator_t
        : public abstract_evaluator_t
        , public evaluatable_t
    {
    public:
        move_t query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration) override;
        std::shared_ptr<usi_info_t> usi_info;

    private:
        class arguments_t
        {
        public:
            cache_t & cache;
            iddfs_context_t & context;
            const pruning_threshold_t pruning_threshold{};
        };

        evaluation_value_t pruning_alphabeta(
            kyokumen_t & kyokumen,
            depth_t depth,
            evaluation_value_t alpha,
            evaluation_value_t beta,
            std::optional<move_t> & candidate_move,
            position_t previous_destination,
            pruning_threshold_t pruning_parameter,
            arguments_t & arguments
        );

        virtual pruning_threshold_t get_pruning_parameter(kyokumen_t & kyokumen, const move_t & move) const = 0;
        virtual pruning_threshold_t get_pruning_threshold() const = 0;
    };

    evaluation_value_t pruning_alphabeta_evaluator_t::pruning_alphabeta(
        kyokumen_t & kyokumen,
        depth_t depth,
        evaluation_value_t alpha,
        evaluation_value_t beta,
        std::optional<move_t> & candidate_move,
        position_t previous_destination,
        pruning_threshold_t pruning_parameter,
        arguments_t & arguments
    )
    {
        if (usi_info)
            usi_info->resolve_request_to_stop(depth);

        // 深度が奇数であり、枝刈りパラメータが閾値以上である場合、局面の評価値を返す。
        if (depth % 2 == 1 && pruning_parameter >= arguments.pruning_threshold)
        {
            if (arguments.context.timeout())
                throw timeout_exception{ "context.timeout() == true" };

            ++details::timer.search_count();
            const std::optional<evaluation_value_t> cached_evaluation_value = arguments.cache.get(kyokumen.hash());
            if (cached_evaluation_value)
            {
                if (usi_info)
                    usi_info->increase_cache_hit_count();
                return *cached_evaluation_value;
            }
            const evaluation_value_t evaluation_value = evaluate(kyokumen) * reverse(kyokumen.color());
            arguments.cache.push(kyokumen.hash(), evaluation_value);
            return evaluation_value;
        }

        moves_t moves = kyokumen.search_moves();
        sort_moves_by_category(moves.begin(), moves.end());

        if (moves.empty())
            return -std::numeric_limits<evaluation_value_t>::max();

        std::vector<evaluated_moves> evaluated_moves;
        auto inserter = std::back_inserter(evaluated_moves);
        evaluation_value_t max_evaluation_value = -std::numeric_limits<evaluation_value_t>::max();

        for (const move_t & move : moves)
        {
            if (usi_info && depth == 0)
                usi_info->notify_currmove(move);

            const pruning_threshold_t increased_pruning_parameter = pruning_parameter + get_pruning_parameter(kyokumen, move);
            std::optional<move_t> nested_candidate_move;
            position_t destination = (!move.put() && !move.destination_piece().empty()) ? move.destination() : npos;
            evaluation_value_t evaluation_value;
            {
                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                evaluation_value = -pruning_alphabeta(kyokumen, depth + 1, -beta, -alpha, nested_candidate_move, destination, increased_pruning_parameter, arguments);
                kyokumen.undo_move(move);
            }
            *inserter++ = { &move, evaluation_value };

            if (usi_info && depth == 0 && evaluation_value > max_evaluation_value)
                usi_info->notify_best_move(move, evaluation_value);

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

    move_t pruning_alphabeta_evaluator_t::query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration)
    {
        if (usi_info)
            usi_info->notify_search_begin();

        cache_t cache = usi_info_t::make_cache(usi_info.get());
        std::optional<move_t> candidate_move;
        evaluation_value_t evaluation_value;
        arguments_t arguments{ cache, context, get_pruning_threshold() * iddfs_iteration };

        try
        {
            evaluation_value = pruning_alphabeta(kyokumen, 0, -std::numeric_limits<evaluation_value_t>::max(), std::numeric_limits<evaluation_value_t>::max(), candidate_move, npos, 0, arguments);
        }
        catch (const timeout_exception &)
        {
            ;
        }

        if (usi_info)
            usi_info->terminate();

        if (!candidate_move)
            throw timeout_exception{ "query_best_move failed" };

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
        move_t query_best_move(kyokumen_t & kyokumen, iddfs_context_t & context, iddfs_iteration_t iddfs_iteration) override
        {
            moves_t moves = kyokumen.search_moves();

            std::vector<evaluated_moves> scores;
            auto back_inserter = std::back_inserter(scores);
            for (const move_t & move : moves)
            {
                VALIDATE_KYOKUMEN_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                *back_inserter++ = { &move, evaluate(kyokumen) };
                kyokumen.undo_move(move);
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
            evaluation_value_t evaluation_value = kyokumen_map_evaluation_value(kyokumen, details::evaluation_value_template::map);
            return evaluation_value;
        }

        std::string name() const override
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
            evaluation_value_t evaluation_value = 0;
            evaluation_value += kyokumen_map_evaluation_value(kyokumen, details::evaluation_value_template::map);
            return evaluation_value;
        }

        std::string name() const override
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
            evaluation_value_t evaluation_value = 0;
            evaluation_value += kyokumen_map_evaluation_value(kyokumen, details::evaluation_value_template::map);

            return evaluation_value;
        }

        std::string name() const override
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
            constexpr evaluation_value_t destination_point = 1;
            constexpr evaluation_value_t kiki_point = -10;
            constexpr evaluation_value_t himo_point = 10;

            evaluation_value_t evaluation_value = 0;
            evaluation_value += kyokumen_map_evaluation_value(kyokumen, details::evaluation_value_template::map);

            for (position_t position = position_begin; position < position_end; ++position)
            {
                if (!board_t::out(position) && !kyokumen.board[position].empty())
                {
                    std::vector<kiki_t> kiki_list;
                    const color_t color = kyokumen.board[position].to_color();
                    kyokumen.search_kiki(std::back_inserter(kiki_list), position, color);
                    evaluation_value += kiki_point * static_cast<evaluation_value_t>(kiki_list.size()) * reverse(color);

                    std::vector<position_t> himo_list;
                    kyokumen.search_himo(std::back_inserter(himo_list), position, color);
                    evaluation_value += himo_point * static_cast<evaluation_value_t>(himo_list.size()) * reverse(color);

                    std::vector<position_t> destination_list;
                    kyokumen.search_destination(std::back_inserter(destination_list), position, color);
                    evaluation_value += destination_point * static_cast<evaluation_value_t>(destination_list.size()) * reverse(color);
                }
            }

            return evaluation_value;
        }

        std::string name() const override
        {
            return "深読み";
        }
    };

    class edagari_evaluator_t
        : public extendable_alphabeta_evaluator_t
    {
    public:
        evaluation_value_t evaluate(kyokumen_t & kyokumen) override
        {
            constexpr evaluation_value_t destination_point = 1;
            constexpr evaluation_value_t kiki_point = -10;
            constexpr evaluation_value_t himo_point = 10;

            evaluation_value_t evaluation_value = 0;
            evaluation_value += kyokumen_map_evaluation_value(kyokumen, details::evaluation_value_template::map);

            for (position_t position = position_begin; position < position_end; ++position)
            {
                if (!board_t::out(position) && !kyokumen.board[position].empty())
                {
                    std::vector<kiki_t> kiki_list;
                    const color_t color = kyokumen.board[position].to_color();
                    kyokumen.search_kiki(std::back_inserter(kiki_list), position, color);
                    evaluation_value += kiki_point * static_cast<evaluation_value_t>(kiki_list.size()) * reverse(color);
                    std::vector<position_t> himo_list;
                    kyokumen.search_himo(std::back_inserter(himo_list), position, color);
                    evaluation_value += himo_point * static_cast<evaluation_value_t>(himo_list.size()) * reverse(color);

                    std::vector<position_t> destination_list;
                    kyokumen.search_destination(std::back_inserter(destination_list), position, color);
                    evaluation_value += destination_point * static_cast<evaluation_value_t>(destination_list.size()) * reverse(color);
                }
            }

            return evaluation_value;
        }

        std::string name() const override
        {
            return "枝刈り";
        }

        //pruning_threshold_t get_pruning_parameter(kyokumen_t & kyokumen, const move_t & move) const override
        //{
        //    if (move.tag() & move_t::check_tag)
        //        return 3;
        //    if (move.tag() & move_t::escape_tag)
        //        return 3;
        //    if (move.tag() & move_t::aigoma_tag)
        //        return 3;
        //    if (move.tag() & move_t::promote_tag)
        //        return 3;
        //    if (move.tag() & move_t::capture_tag)
        //        return 4;
        //    if (move.tag() & move_t::put_tag)
        //        return 4;
        //    return 5;
        //}

        //pruning_threshold_t get_pruning_threshold() const override
        //{
        //    return 12;
        //}
    };

    /**
     * @breif 染色体
     */
    class chromosome_t
    {
    private:
        enum : std::size_t
        {
            check_offset,
            escape_offset,
            aigoma_offset,
            promote_offset,
            capture_offset,
            put_offset,
            none_offset,
            pruning_coefficient_size
        };

        enum : std::size_t
        {
            front_left_offset,
            front_offset,
            front_right_offset,
            left_offset,
            right_offset,
            back_left_offset,
            back_offset,
            back_right_offset,
            nearest_8_size
        };

    public:
        unsigned short board_piece_points[promoted_rook_value - pawn_value - 1]{};
        unsigned short captured_piece_points[captured_size]{};
        short kiki_coefficient[4]{};
        short himo_coefficient[4]{};
        unsigned char destination_points[16]{};
        unsigned short nyugyoku_coefficient[max_nyugyoku_progress + 1]{};
        short nearest_8_coefficient[nearest_8_size]{};
        //unsigned short pruning_parameters[pruning_coefficient_size]{};
        //unsigned short pruning_threshold{};

        //inline pruning_threshold_t get_pruning_parameter(move_t::tag_t tag) const noexcept
        //{
        //    if (tag & move_t::check_tag)
        //        return pruning_parameters[check_offset];
        //    if (tag & move_t::escape_tag)
        //        return pruning_parameters[escape_offset];
        //    if (tag & move_t::aigoma_tag)
        //        return pruning_parameters[aigoma_offset];
        //    if (tag & move_t::promote_tag)
        //        return pruning_parameters[promote_offset];
        //    if (tag & move_t::capture_tag)
        //        return pruning_parameters[capture_offset];
        //    if (tag & move_t::put_tag)
        //        return pruning_parameters[put_offset];
        //    return pruning_parameters[none_offset];
        //}

        //inline bool get_pruning_threshold() const noexcept
        //{
        //    return pruning_threshold;
        //}

        inline void generate_template() noexcept
        {
            // 歩の点数は100固定として、染色体の要素として保持しない。
            constexpr unsigned short board_piece_points_template[]
            {
                details::evaluation_value_template::board_lance,
                details::evaluation_value_template::board_knight,
                details::evaluation_value_template::board_silver,
                details::evaluation_value_template::board_gold,
                details::evaluation_value_template::board_bishop,
                details::evaluation_value_template::board_rook,
                details::evaluation_value_template::board_promoted_pawn,
                details::evaluation_value_template::board_promoted_lance,
                details::evaluation_value_template::board_promoted_knight,
                details::evaluation_value_template::board_promoted_silver,
                details::evaluation_value_template::board_promoted_bishop,
                details::evaluation_value_template::board_promoted_rook,
            };
            std::copy(std::begin(board_piece_points_template), std::end(board_piece_points_template), std::begin(board_piece_points));

            {
                constexpr double coefficient = 3.0 / 4;
                for (piece_value_t piece = pawn_value; piece <= rook_value; ++piece)
                {
                    const captured_piece_range_t range = captured_piece_range(piece);
                    for (std::size_t i = 0; i < range.size; ++i)
                    {
                        if (i == 0)
                            captured_piece_points[range.offset + i] = 0;
                        else
                            captured_piece_points[range.offset + i]
                                = static_cast<unsigned short>(captured_piece_points[range.offset + i - 1]
                                + details::evaluation_value_template::map[piece] * details::power(coefficient, i - 1));
                    }
                }
            }

            constexpr std::decay_t<decltype(*kiki_coefficient)> kiki_coefficient_template[]
            {
                std::numeric_limits<unsigned char>::max() * -1 / 8,
                std::numeric_limits<unsigned char>::max() * -2 / 8,
                std::numeric_limits<unsigned char>::max() * -3 / 8,
                std::numeric_limits<unsigned char>::max() * -4 / 8
            };
            std::copy(std::begin(kiki_coefficient_template), std::end(kiki_coefficient_template), std::begin(kiki_coefficient));

            constexpr std::decay_t<decltype(*himo_coefficient)> himo_coefficient_template[]
            {
                std::numeric_limits<unsigned char>::max() * -1 / 8,
                std::numeric_limits<unsigned char>::max() *  0 / 8,
                std::numeric_limits<unsigned char>::max() *  1 / 8,
                std::numeric_limits<unsigned char>::max() *  2 / 8
            };
            std::copy(std::begin(himo_coefficient_template), std::end(himo_coefficient_template), std::begin(himo_coefficient));

            std::fill(std::begin(destination_points), std::end(destination_points), std::numeric_limits<std::decay_t<decltype(*destination_points)>>::max());

            for (unsigned short i = 0; i < static_cast<unsigned short>(std::size(nyugyoku_coefficient)); ++i)
                nyugyoku_coefficient[i] = i;

            //pruning_parameters[check_offset]   = std::numeric_limits<unsigned char>::max() / 2;
            //pruning_parameters[escape_offset]  = std::numeric_limits<unsigned char>::max() / 2;
            //pruning_parameters[aigoma_offset]  = std::numeric_limits<unsigned char>::max() / 2;
            //pruning_parameters[promote_offset] = std::numeric_limits<unsigned char>::max() / 2;
            //pruning_parameters[capture_offset] = std::numeric_limits<unsigned char>::max() / 2;
            //pruning_parameters[put_offset]     = std::numeric_limits<unsigned char>::max() / 2;
            //pruning_parameters[none_offset]    = std::numeric_limits<unsigned char>::max() / 2;

            nearest_8_coefficient[front_left_offset ] = std::numeric_limits<unsigned char>::max() * 1 / 64;
            nearest_8_coefficient[front_offset      ] = std::numeric_limits<unsigned char>::max() * 1 / 64;
            nearest_8_coefficient[front_right_offset] = std::numeric_limits<unsigned char>::max() * 1 / 64;
            nearest_8_coefficient[left_offset       ] = std::numeric_limits<unsigned char>::max() * 1 / 64;
            nearest_8_coefficient[right_offset      ] = std::numeric_limits<unsigned char>::max() * 1 / 64;
            nearest_8_coefficient[back_left_offset  ] = std::numeric_limits<unsigned char>::max() * 1 / 64;
            nearest_8_coefficient[back_offset       ] = std::numeric_limits<unsigned char>::max() * 1 / 64;
            nearest_8_coefficient[back_right_offset ] = std::numeric_limits<unsigned char>::max() * 1 / 64;

            //pruning_threshold = pruning_parameters[none_offset] * 2;
        }

        inline void generate_random() noexcept
        {
            unsigned char * begin = reinterpret_cast<unsigned char *>(this);
            unsigned char * end = reinterpret_cast<unsigned char *>(this) + sizeof(*this);
            std::generate(begin, end, []() -> unsigned char { return details::random<unsigned char>(); });
        }

        inline void mutate() noexcept
        {
            unsigned char * begin = reinterpret_cast<unsigned char *>(this);
            const unsigned int shift = details::random<unsigned int>(0, CHAR_BIT - 1);
            const std::size_t offset = details::random<std::size_t>(0, sizeof(*this) - 1);
            unsigned char * data = reinterpret_cast<unsigned char *>(this);
            data[offset] ^= (1 << shift);
        }

        inline void print(std::ostream & ostream = std::cout) const noexcept
        {
            ostream << "board-piece-points-lance          : " << static_cast<unsigned int>(board_piece_points[lance_value           - pawn_value - 1]) << std::endl;
            ostream << "board-piece-points-knight         : " << static_cast<unsigned int>(board_piece_points[knight_value          - pawn_value - 1]) << std::endl;
            ostream << "board-piece-points-silver         : " << static_cast<unsigned int>(board_piece_points[silver_value          - pawn_value - 1]) << std::endl;
            ostream << "board-piece-points-gold           : " << static_cast<unsigned int>(board_piece_points[gold_value            - pawn_value - 1]) << std::endl;
            ostream << "board-piece-points-bishop         : " << static_cast<unsigned int>(board_piece_points[bishop_value          - pawn_value - 1]) << std::endl;
            ostream << "board-piece-points-rook           : " << static_cast<unsigned int>(board_piece_points[rook_value            - pawn_value - 1]) << std::endl;
            ostream << "board-piece-points-promoted-pawn  : " << static_cast<unsigned int>(board_piece_points[promoted_pawn_value   - pawn_value - 2]) << std::endl;
            ostream << "board-piece-points-promoted-lance : " << static_cast<unsigned int>(board_piece_points[promoted_lance_value  - pawn_value - 2]) << std::endl;
            ostream << "board-piece-points-promoted-knight: " << static_cast<unsigned int>(board_piece_points[promoted_knight_value - pawn_value - 2]) << std::endl;
            ostream << "board-piece-points-promoted-silver: " << static_cast<unsigned int>(board_piece_points[promoted_silver_value - pawn_value - 2]) << std::endl;
            ostream << "board-piece-points-promoted-bishop: " << static_cast<unsigned int>(board_piece_points[promoted_bishop_value - pawn_value - 2]) << std::endl;
            ostream << "board-piece-points-promoted-rook  : " << static_cast<unsigned int>(board_piece_points[promoted_rook_value   - pawn_value - 2]) << std::endl;
            for (std::size_t i = 0; i < captured_pawn_size; ++i)
                ostream << "captured-piece-points-pawn-" << i << ": " << static_cast<unsigned int>(captured_piece_points[captured_pawn_offset + i]) << std::endl;
            for (std::size_t i = 0; i < captured_lance_size; ++i)
                ostream << "captured-piece-points-lance-" << i << ": " << static_cast<unsigned int>(captured_piece_points[captured_lance_offset + i]) << std::endl;
            for (std::size_t i = 0; i < captured_knight_size; ++i)
                ostream << "captured-piece-points-knight-" << i << ": " << static_cast<unsigned int>(captured_piece_points[captured_knight_offset + i]) << std::endl;
            for (std::size_t i = 0; i < captured_silver_size; ++i)
                ostream << "captured-piece-points-silver-" << i << ": " << static_cast<unsigned int>(captured_piece_points[captured_silver_offset + i]) << std::endl;
            for (std::size_t i = 0; i < captured_gold_size; ++i)
                ostream << "captured-piece-points-gold-" << i << ": " << static_cast<unsigned int>(captured_piece_points[captured_gold_offset + i]) << std::endl;
            for (std::size_t i = 0; i < captured_bishop_size; ++i)
                ostream << "captured-piece-points-bishop-" << i << ": " << static_cast<unsigned int>(captured_piece_points[captured_bishop_offset + i]) << std::endl;
            for (std::size_t i = 0; i < captured_rook_size; ++i)
                ostream << "captured-piece-points-rook-" << i << ": " << static_cast<unsigned int>(captured_piece_points[captured_rook_offset + i]) << std::endl;
            for (std::size_t i = 0; i < std::size(kiki_coefficient); ++i)
                ostream << "kiki-coefficient-" << i << ": " << static_cast<int>(kiki_coefficient[i]) << std::endl;
            for (std::size_t i = 0; i < std::size(himo_coefficient); ++i)
                ostream << "himo-coefficient-" << i << ": " << static_cast<int>(himo_coefficient[i]) << std::endl;
            for (std::size_t i = 0; i < std::size(destination_points); ++i)
                ostream << "destination-points-" << i << ": " << static_cast<unsigned int>(destination_points[i]) << std::endl;
            for (std::size_t i = 0; i < std::size(nyugyoku_coefficient); ++i)
                ostream << "nyugyoku-coefficient-" << i << ": " << static_cast<unsigned int>(nyugyoku_coefficient[i]) << std::endl;
            //for (std::size_t i = 0; i < std::size(pruning_parameters); ++i)
            //    ostream << "pruning-parameter-" << i << ": " << static_cast<unsigned int>(pruning_parameters[i]) << std::endl;
            //ostream << "pruning-threshold: " << static_cast<unsigned int>(pruning_threshold) << std::endl;
        }

        inline void clossover(const chromosome_t & chromosome) noexcept
        {
            unsigned char * first = reinterpret_cast<unsigned char *>(this);
            unsigned char * last = reinterpret_cast<unsigned char *>(this) + sizeof(*this);
            const unsigned char * input = reinterpret_cast<const unsigned char *>(&chromosome);
            while (first != last)
            {
                *first = details::uniform_croossover(*first, *input);
                ++first;
                ++input;
            }
        }

        inline void read_file(const std::filesystem::path & path)
        {
            std::ifstream in(path, std::ios_base::in | std::ios::binary);
            char * data = reinterpret_cast<char *>(this);
            in.read(data, sizeof(*this));
        }

        inline void write_file(const std::filesystem::path & path) const
        {
            std::ofstream out(path, std::ios_base::out | std::ios::binary);
            const char * data = reinterpret_cast<const char *>(this);
            out.write(data, sizeof(*this));
        }

        inline std::size_t distance(const chromosome_t chromosome) const noexcept
        {
            std::size_t result = 0;
            const unsigned char * first = reinterpret_cast<const unsigned char *>(this);
            const unsigned char * last = reinterpret_cast<const unsigned char *>(this) + sizeof(*this);
            const unsigned char * input = reinterpret_cast<const unsigned char *>(&chromosome);
            while (first != last)
            {
                result += details::count_bit(static_cast<unsigned char>(*first ^ *input));
                ++first;
                ++input;
            }
            return result;
        }

        inline evaluation_value_t evaluate_board_piece(noncolored_piece_t piece) const noexcept
        {
            if (piece == pawn)
                return 100;
            else if (piece == king)
                return 0;
            std::size_t index = piece.value();
            if (index >= promoted_pawn_value)
                index -= pawn_value + 2;
            else
                index -= pawn_value + 1;
            SHOGIPP_ASSERT(index < std::size(board_piece_points));
            return board_piece_points[index];
        }

        inline evaluation_value_t evaluate_captured_piece(captured_piece_t piece, captured_pieces_t::size_type count) const noexcept
        {
            const std::size_t index = captured_piece_range(piece).offset + count;
            SHOGIPP_ASSERT(index < std::size(captured_piece_points));
            return captured_piece_points[index];
        }

        inline evaluation_value_t evaluate(kyokumen_t & kyokumen) const
        {
            evaluation_value_t evaluation_value = 0;

            for (position_t position = position_begin; position < position_end; ++position)
            {
                const colored_piece_t piece = kyokumen.board[position];
                if (!board_t::out(position) && !piece.empty())
                    evaluation_value += evaluate_board_piece(noncolored_piece_t{ piece }) * reverse(piece.to_color());
            }

            for (const color_t color : colors)
            {
                for (piece_value_t piece = pawn_value; piece <= rook_value; ++piece)
                    evaluation_value += evaluate_captured_piece(captured_piece_t{ piece }, kyokumen.captured_pieces_list[color.value()][piece]) * reverse(color);

                const position_t king_position = kyokumen.king_position(color);
                constexpr position_t nearest_8[]
                {
                    front_left,
                    front,
                    front_right,
                    left,
                    right,
                    back_left,
                    back,
                    back_right,
                };

                for (std::size_t i = 0; i < nearest_8_size; ++i)
                {
                    const position_t offset = nearest_8[i];
                    const position_t nearest = king_position + offset;
                    if (!board_t::out(nearest))
                    {
                        const colored_piece_t piece = kyokumen.board[nearest];
                        if (!piece.empty())
                        {
                            evaluation_value += (evaluate_board_piece(noncolored_piece_t{ piece }) * reverse(piece.to_color())
                                * nearest_8_coefficient[i]) >> CHAR_BIT;
                        }
                    }
                }

                {
                    const unsigned int offset = nyugyoku_progress(kyokumen.additional_info.king_position_list[color.value()], color);
                    SHOGIPP_ASSERT(offset < std::size(nyugyoku_coefficient));
                    evaluation_value += nyugyoku_coefficient[offset];
                }
            }

            for (position_t position = position_begin; position < position_end; ++position)
            {
                if (!board_t::out(position))
                {
                    const colored_piece_t piece = kyokumen.board[position];
                    if (!piece.empty())
                    {
                        const color_t color = piece.to_color();

                        {
                            std::vector<kiki_t> kiki_list;
                            kyokumen.search_kiki(std::back_inserter(kiki_list), position, color);
                            const std::size_t offset = std::min(kiki_list.size(), std::size(kiki_coefficient) - 1);
                            evaluation_value += (evaluate_board_piece(piece) * reverse(color)
                                * kiki_coefficient[offset]) >> CHAR_BIT;
                        }

                        {
                            std::vector<position_t> himo_list;
                            kyokumen.search_himo(std::back_inserter(himo_list), position, color);
                            const std::size_t offset = std::min(himo_list.size(), std::size(himo_coefficient) - 1);
                            evaluation_value += (evaluate_board_piece(piece) * reverse(color)
                                * himo_coefficient[offset]) >> CHAR_BIT;
                        }

                        {
                            std::vector<position_t> destination_list;
                            kyokumen.search_destination(std::back_inserter(destination_list), position, color);
                            const std::size_t offset = std::min(destination_list.size(), std::size(destination_points) - 1);
                            evaluation_value += (static_cast<evaluation_value_t>(destination_list.size()) * reverse(color)
                                * destination_points[offset]) >> (sizeof(*destination_points) * CHAR_BIT);
                        }
                    }
                }
            }

            std::optional<move_t> last_move = kyokumen.last_move();
            if (last_move)
            {
                ;
            }

            return evaluation_value;
        }
    };

    class chromosome_evaluator_t
        : public extendable_alphabeta_evaluator_t
    {
    public:
        using id_type = unsigned long long;

        inline chromosome_evaluator_t(const std::filesystem::path & path, const std::string & name, id_type id)
            : m_chromosome{ std::make_shared<chromosome_t>() }
            , m_name{ name }
            , m_id{ id }
        {
            m_chromosome->read_file(path);
        }

        inline chromosome_evaluator_t(const std::shared_ptr<chromosome_t> & chromosome, const std::string & name, id_type id)
            : m_chromosome{ chromosome }
            , m_name{ name }
            , m_id{ id }
        {
        }

        inline chromosome_evaluator_t(const chromosome_evaluator_t &) = default;
        inline chromosome_evaluator_t(chromosome_evaluator_t &&) = default;
        inline chromosome_evaluator_t & operator =(const chromosome_evaluator_t &) = default;
        inline chromosome_evaluator_t & operator =(chromosome_evaluator_t &&) = default;

        evaluation_value_t evaluate(kyokumen_t & kyokumen) override
        {
            return m_chromosome->evaluate(kyokumen);
        }

        std::string name() const override
        {
            return m_name;
        }

        //pruning_threshold_t get_pruning_parameter(kyokumen_t & kyokumen, const move_t & move) const override
        //{
        //    return m_chromosome->get_pruning_parameter(move.tag());
        //}

        //pruning_threshold_t get_pruning_threshold() const override
        //{
        //    return m_chromosome->get_pruning_threshold();
        //}

        std::string file_name() const
        {
            return m_name + "_" + std::to_string(m_id);
        }

        inline void write_file(const std::filesystem::path & directory) const
        {
            m_chromosome->write_file(directory / file_name());
        }

        inline const std::shared_ptr<chromosome_t> & chromosome() const noexcept
        {
            return m_chromosome;
        }

        inline std::shared_ptr<chromosome_t> & chromosome() noexcept
        {
            return m_chromosome;
        }

        inline id_type id() const noexcept
        {
            return m_id;
        }

    private:
        std::shared_ptr<chromosome_t> m_chromosome;
        std::string m_name;
        id_type m_id;
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

        std::string name() const override
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
            eval,
        };

        id_t id{ id_t::error };
        std::optional<move_t> opt_move;
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
                        return command_t{ command_t::id_t::undo };

                    if (tokens[0] == "giveup")
                        return command_t{ command_t::id_t::giveup };

                    if (tokens[0] == "dump")
                        return command_t{ command_t::id_t::dump };

                    if (tokens[0] == "perft")
                    {
                        if (tokens.size() != 2)
                            throw invalid_command_line_input{ "unknown command line input" };
                        std::optional<move_count_t> depth = details::cast_to<move_count_t>(tokens[1]);
                        if (!depth)
                            throw invalid_command_line_input{ "unknown command line input" };
                        command_t command;
                        command.id = command_t::id_t::perft;
                        command.opt_depth = *depth;
                        return command;
                    }

                    if (tokens[0] == "hash")
                        return command_t{ command_t::id_t::hash };

                    if (tokens[0] == "sfen")
                        return command_t{ command_t::id_t::sfen };

                    if (tokens[0] == "eval")
                        return command_t{ command_t::id_t::eval };

                    std::optional<std::size_t> move_index = details::cast_to<std::size_t>(tokens[0]);
                    if (!move_index)
                        throw invalid_command_line_input{ "unknown command line input" };
                    if (*move_index == 0)
                        throw invalid_command_line_input{ "move_index == 0" };
                    if (*move_index > moves.size())
                        throw invalid_command_line_input{ "move_index > moves.size()" };
                    const std::size_t raw_move_index = *move_index - 1;
                    command_t command;
                    command.id = command_t::id_t::move;
                    command.opt_move = moves[raw_move_index];
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
         * @param 出力ストリーム
         * @details この関数が true を返した場合、再度この関数を呼び出す。
         */
        inline bool procedure(std::ostream & ostream = std::cout);

        /**
         * @breif 対局を出力ストリームに出力する。
         * @param 出力ストリーム
         */
        inline void print(std::ostream & ostream = std::cout) const;

        /**
         * @breif 手番の合法手を返す。
         * @return 合法手
         */
        inline const moves_t & get_moves() const;

        /**
         * @breif 手番の合法手を更新する。
         */
        inline void update_moves() const;

        std::shared_ptr<abstract_kishi_t> kishi_list[color_t::size()];
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
        virtual std::string name() const = 0;

        /**
         * @breif 棋士がコンピュータであるか判定する。
         * @retval true 棋士がコンピュータである場合
         * @retval false 棋士が人間である場合
         */
        virtual bool is_computer() const = 0;
    };

    /**
     * @breif 標準入力により制御される棋士
     */
    class stdin_kishi_t
        : public abstract_kishi_t
    {
    public:
        command_t get_command(taikyoku_t & taikyoku) override;
        std::string name() const override;
        bool is_computer() const override;
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
        std::string name() const override;
        bool is_computer() const override;

    private:
        std::shared_ptr<abstract_evaluator_t> ptr;
    };

    command_t stdin_kishi_t::get_command(taikyoku_t & taikyoku)
    {
        return read_command_line_input(taikyoku.get_moves());
    }

    std::string stdin_kishi_t::name() const
    {
        return "stdin";
    }

    bool stdin_kishi_t::is_computer() const
    {
        return false;
    }

    command_t computer_kishi_t::get_command(taikyoku_t & taikyoku)
    {
        iddfs_context_t context{ details::program_options::max_iddfs_iteration ,details::program_options::limit_time };
        context.start();
        return command_t{ command_t::id_t::move, ptr->best_move_iddfs(taikyoku.kyokumen, context) };
    }

    std::string computer_kishi_t::name() const
    {
        return ptr->name();
    }

    bool computer_kishi_t::is_computer() const
    {
        return true;
    }

    inline taikyoku_t::taikyoku_t(const std::shared_ptr<abstract_kishi_t> & a, const std::shared_ptr<abstract_kishi_t> & b)
        : kishi_list{ a, b }
        , black_win{ false }
    {
        update_moves();
    }

    inline bool taikyoku_t::procedure(std::ostream & ostream)
    {
        const std::shared_ptr<abstract_kishi_t> & kishi = kishi_list[kyokumen.color().value()];

        kyokumen_t temp_kyokumen = kyokumen;

        while (true)
        {
            command_t cmd = kishi->get_command(*this);
            switch (cmd.id)
            {
            case command_t::id_t::error:
                break;
            case command_t::id_t::move:
                kyokumen.print_move(*cmd.opt_move, kyokumen.color(), ostream);
                ostream << std::endl << std::endl;
                kyokumen.do_move(*cmd.opt_move);
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
                kyokumen.print_kifu(ostream);
                break;
            case command_t::id_t::perft:
            {
                search_count_t node;
                const std::chrono::milliseconds time = details::test_time_performance([&] { node = kyokumen.count_node(*cmd.opt_depth); }, 1);
                const search_count_t nps = time.count() != 0 ? node * 1000 / time.count() : 0;
                ostream << "node: " << node << std::endl;
                ostream << "time[ms]: " << time.count() << std::endl;
                ostream << "nps[n/s]: " << nps << std::endl;
                break;
            }
            case command_t::id_t::hash:
                ostream << hash_to_string(kyokumen.hash()) << std::endl;
                break;
            case command_t::id_t::sfen:
                ostream << kyokumen.sfen_string() << std::endl;
                break;
            case command_t::id_t::eval:
                break;
            }
        }
    }

    inline void taikyoku_t::print(std::ostream & ostream) const
    {
        if (kyokumen.move_count == 0)
        {
            for (const color_t color : colors)
                ostream << color_to_string(static_cast<color_t>(color)) << "：" << kishi_list[color.value()]->name() << std::endl;
            ostream << std::endl;
        }

        if (moves.empty())
        {
            const std::shared_ptr<abstract_kishi_t> & winner_evaluator = kishi_list[!kyokumen.color().value()];
            ostream << kyokumen.move_count << "手詰み" << std::endl;
            kyokumen.print(ostream);
            ostream << color_to_string(!kyokumen.color()) << "勝利（" << winner_evaluator->name() << "）" << std::flush;
        }
        else
        {
            const bool is_computer = kishi_list[kyokumen.color().value()]->is_computer();
            ostream << (kyokumen.move_count + 1) << "手目" << color_to_string(kyokumen.color()) << "番" << std::endl;
            kyokumen.print(ostream);
            if (!is_computer || details::program_options::print_moves)
                kyokumen.print_moves(ostream);
            kyokumen.print_check(ostream);
        }
    }

    inline const moves_t & taikyoku_t::get_moves() const
    {
        return moves;
    }

    inline void taikyoku_t::update_moves() const
    {
        moves = kyokumen.search_moves();
    }

    /**
    * @breif 対局する。
    * @param black_kishi 先手の棋士
    * @param white_kishi 後手の棋士
    */
    inline void do_taikyoku(kyokumen_t & kyokumen, const std::shared_ptr<abstract_kishi_t> & black_kishi, const std::shared_ptr<abstract_kishi_t> & white_kishi)
    {
        details::timer.clear();

        taikyoku_t taikyoku{ black_kishi, white_kishi };
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
        std::cout << std::flush;
    }

    /**
     * @breif 対局する。
     * @param black_kishi 先手の棋士
     * @param white_kishi 後手の棋士
     */
    inline void do_taikyoku(const std::shared_ptr<abstract_kishi_t> & black_kishi, const std::shared_ptr<abstract_kishi_t> & white_kishi)
    {
        kyokumen_t kyokumen;
        do_taikyoku(kyokumen, black_kishi, white_kishi);
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
                                throw invalid_usi_input{ "unexpected end of command" };
                            name = tokens[current];
                            ++current;
                        }
                        else if (tokens[current] == "value")
                        {
                            ++current;
                            if (current >= tokens.size())
                                throw invalid_usi_input{ "unexpected end of command" };
                            value = tokens[current];
                            ++current;
                        }
                    }

                    if (!name)
                        throw invalid_usi_input{ "name parameter not specified" };
                    else if (!value)
                        throw invalid_usi_input{ "value parameter not specified" };

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
                 
                    std::optional<std::chrono::milliseconds> opt_time[color_t::size()];
                    std::optional<std::chrono::milliseconds> opt_byoyomi;
                    std::optional<std::chrono::milliseconds> opt_inc[color_t::size()];
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
                                throw invalid_usi_input{ "btime value not found" };
                            const std::optional<std::chrono::milliseconds::rep> opt_rep = details::cast_to<std::chrono::milliseconds::rep>(tokens[current]);
                            if (opt_rep)
                                opt_time[black.value()] = std::chrono::milliseconds{ *opt_rep };
                            ++current;
                        }
                        else if (tokens[current] == "wtime")
                        {
                            ++current;
                            if (current >= tokens.size())
                                throw invalid_usi_input{ "wtime value not found" };
                            const std::optional<std::chrono::milliseconds::rep> opt_rep = details::cast_to<std::chrono::milliseconds::rep>(tokens[current]);
                            if (opt_rep)
                                opt_time[white.value()] = std::chrono::milliseconds{ *opt_rep };
                            ++current;
                        }
                        else if (tokens[current] == "byoyomi")
                        {
                            ++current;
                            if (current >= tokens.size())
                                throw invalid_usi_input{ "byoyomi value not found" };
                            const std::optional<std::chrono::milliseconds::rep> opt_rep = details::cast_to<std::chrono::milliseconds::rep>(tokens[current]);
                            if (opt_rep)
                                opt_byoyomi = std::chrono::milliseconds{ *opt_rep };
                            ++current;
                        }
                        else if (tokens[current] == "binc")
                        {
                            ++current;
                            if (current >= tokens.size())
                                throw invalid_usi_input{ "binc value not found" };
                            const std::optional<std::chrono::milliseconds::rep> opt_rep = details::cast_to<std::chrono::milliseconds::rep>(tokens[current]);
                            if (opt_rep)
                                opt_inc[black.value()] = std::chrono::milliseconds{ *opt_rep };
                            ++current;
                        }
                        else if (tokens[current] == "winc")
                        {
                            ++current;
                            if (current >= tokens.size())
                                throw invalid_usi_input{ "winc value not found" };
                            const std::optional<std::chrono::milliseconds::rep> opt_rep = details::cast_to<std::chrono::milliseconds::rep>(tokens[current]);
                            if (opt_rep)
                                opt_inc[white.value()] = std::chrono::milliseconds{ *opt_rep };
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
                                if (!opt_time[kyokumen.color().value()])
                                    throw invalid_usi_input{ "limit time not specified" };
                                std::chrono::milliseconds limit_time = *opt_time[kyokumen.color().value()];
                                if (opt_byoyomi)
                                    limit_time += *opt_byoyomi;
                                usi_info->limit_time = limit_time;
                            }
                            usi_info->ponder = ponder;
                            usi_info->options = setoptions;
                        }

                        evaluator->usi_info = usi_info;

                        auto search_thread_impl = [evaluator, kyokumen, usi_info]() mutable
                        {
                            try
                            {
                                iddfs_context_t context(details::program_options::max_iddfs_iteration, usi_info->limit_time);
                                context.start();
                                evaluator->best_move_iddfs(kyokumen, context);
                            }
                            catch (...)
                            {
                                ;
                            }
                        };
                        std::thread search_thread{ search_thread_impl };
                        search_thread.detach();

                        auto notify_thread_impl = [usi_info]()
                        {
                            try
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
                            }
                            catch (...)
                            {
                                ;
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
                        throw invalid_usi_input{ "unexpected stop command" };
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
                    std::terminate();
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
        { "edagari" , std::make_shared<computer_kishi_t>(std::make_shared<edagari_evaluator_t >()) },
    };

    static const std::map<std::string, std::shared_ptr<abstract_evaluator_t>> evaluator_map
    {
        { "random"  , std::make_shared<random_evaluator_t  >() },
        { "sample"  , std::make_shared<sample_evaluator_t  >() },
        { "hiyoko"  , std::make_shared<hiyoko_evaluator_t  >() },
        { "niwatori", std::make_shared<niwatori_evaluator_t>() },
        { "fukayomi", std::make_shared<fukayomi_evaluator_t>() },
        { "edagari" , std::make_shared<edagari_evaluator_t>() },
    };

    class genetic_algorithm_t
    {
    public:
        enum class action_t
        {
            mutation,
            crossover,
            selection
        };

        using fitness_type = unsigned int;
        using parameter_type = unsigned int;

        inline action_t random_action() noexcept
        {
            parameter_type div = m_mutation_rate + m_crossover_rate + m_selection_rate;
            parameter_type value = details::random<parameter_type>(0, div - 1);
            if (value < m_mutation_rate)
                return action_t::mutation;
            value -= m_mutation_rate;
            if (value < m_crossover_rate)
                return action_t::crossover;
            return action_t::selection;
        }

        inline const std::shared_ptr<chromosome_evaluator_t> & select_individual(const std::vector<fitness_type> & fitness_table) const
        {
            fitness_type div = 0;
            for (const fitness_type fitness : fitness_table)
                div += fitness;
            fitness_type value = details::random<fitness_type>(0, div - 1);
            for (std::size_t i = 0; i < fitness_table.size(); ++i)
            {
                const fitness_type fitness = fitness_table[i];
                if (value < fitness)
                    return individuals[i];
                value -= fitness;
            }
            return individuals.back();
        }

        inline std::vector<move_t> make_kifu
        (
            const std::shared_ptr<chromosome_evaluator_t> & black,
            const std::shared_ptr<chromosome_evaluator_t> & white,
            std::ostream & ostream
        ) const
        {
            const std::shared_ptr<abstract_kishi_t> black_kishi{ std::make_shared<computer_kishi_t>(black) };
            const std::shared_ptr<abstract_kishi_t> white_kishi{ std::make_shared<computer_kishi_t>(white) };

            taikyoku_t taikyoku{ black_kishi, white_kishi };
            while (true)
            {
                taikyoku.print(ostream);
                if (!taikyoku.procedure(ostream) || taikyoku.kyokumen.move_count >= m_max_move_count)
                    break;
            }

            taikyoku.print(ostream); // 詰んだ局面を標準出力に出力する。

            ostream << std::endl;
            details::timer.print_elapsed_time(ostream);

            taikyoku.kyokumen.print_kifu(ostream);
            ostream << std::flush;

            return taikyoku.kyokumen.kifu;
        }

        inline static void update_piece_pair_statistics(const std::vector<move_t> & kifu)
        {
            kyokumen_t kyokumen;
            const move_count_t winner_mod = (kifu.size() + 1) % 2;
            details::piece_pair_statistics.increase(kyokumen.board);
            for (move_count_t i = 0; i < kifu.size(); ++i)
            {
                kyokumen.do_move(kifu[i]);
                if (i % 2 == winner_mod)
                    details::piece_pair_statistics.increase(kyokumen.board);
            }
        }

        inline void run(const std::filesystem::path & log_directory, unsigned long long & uid, unsigned int thread_number)
        {
            std::vector<fitness_type> fitness_table;
            fitness_table.resize(individuals.size(), 0);

            std::mutex mutex;

            // 総当りの対戦表を作成する。
            class thread_arguments_t
            {
            public:
                std::size_t i{};
                std::size_t j{};
            };
            std::deque<thread_arguments_t> thread_arguments_queue;
            for (std::size_t i = 0; i < individuals.size(); ++i)
            {
                for (std::size_t j = 0; j < individuals.size(); ++j)
                {
                    if (i != j)
                    {
                        thread_arguments_t thread_arguments;
                        thread_arguments.i = i;
                        thread_arguments.j = j;
                        thread_arguments_queue.push_back(thread_arguments);
                    }
                }
            }

            // スレッドを作成する。
            {
                std::vector<std::future<void>> futures;
                for (unsigned int thread_id = 0; thread_id < thread_number; ++thread_id)
                {
                    futures.push_back(std::async(std::launch::async, [this, thread_id, &thread_arguments_queue, &fitness_table, &log_directory, &mutex]
                        {
                            while (true)
                            {
                                try
                                {
                                    thread_arguments_t arguments;

                                    {
                                        std::lock_guard<decltype(mutex)> lock{ mutex };
                                        if (thread_arguments_queue.empty())
                                            return;
                                        arguments = thread_arguments_queue.front();
                                        thread_arguments_queue.pop_front();
                                    }

                                    std::ostringstream temp_stream;
                                    const std::shared_ptr<chromosome_evaluator_t> black = individuals[arguments.i];
                                    const std::shared_ptr<chromosome_evaluator_t> white = individuals[arguments.j];
                                    const std::vector<move_t> kifu = make_kifu(black, white, temp_stream);

                                    {
                                        std::lock_guard<decltype(mutex)> lock{ mutex };

                                        const std::string log_file_name = std::to_string(arguments.i) + "_" + std::to_string(arguments.j) + ".txt";
                                        const std::filesystem::path log_path = log_directory / log_file_name;
                                        std::ofstream log_stream{ log_path };
                                        log_stream << temp_stream.str() << std::flush;

                                        if (kifu.size() < m_max_move_count)
                                        {
                                            if (kifu.size() % 2 == 1)
                                                fitness_table[arguments.i] += 1;
                                            else
                                                fitness_table[arguments.j] += 1;
                                        }

                                        update_piece_pair_statistics(kifu);
                                    }
                                }
                                catch (const std::exception & e)
                                {
                                    std::cerr << e.what() << std::endl;
                                }
                                catch (...)
                                {
                                    std::cerr << "unkown error at thread (" << thread_id << ")" << std::endl;
                                }
                            }
                        }
                    ));
                }
            }

            try
            {
                details::piece_pair_statistics.write_file(details::program_options::piece_pair_statistics);
            }
            catch (const std::filesystem::filesystem_error &)
            {
                std::cerr << "piece_pair_statistics.write_file() failed" << std::endl;
            }

            // 勝率により降順に個体を並び替える。
            using evaluated_individual_t = std::pair<chromosome_evaluator_t *, evaluation_value_t>;
            std::vector<evaluated_individual_t> evaluated_individuals;
            for (std::size_t i = 0; i < individuals.size(); ++i)
                evaluated_individuals.emplace_back(individuals[i].get(), fitness_table[i]);
            std::sort(evaluated_individuals.begin(), evaluated_individuals.end(), [](const evaluated_individual_t & a, const evaluated_individual_t & b) -> bool { return a.second > b.second; });

            // 対局の結果を出力する。
            {
                const std::filesystem::path log_path = log_directory / "summary.txt";
                std::ofstream log_stream{ log_path };
                
                std::size_t div = (individuals.size() - 1) * 2;
                for (std::size_t i = 0; i < individuals.size(); ++i)
                {
                    const std::string name = individuals[i]->name() + "_" + std::to_string(individuals[i]->id());
                    double wp = 100.0 * fitness_table[i] / div;
                    log_stream << name << ": " << wp << "% (" << fitness_table[i] << "/" << div << ")" << std::endl;
                }
                log_stream << std::endl;
                
                log_stream << "best chromosome:" << std::endl;
                evaluated_individuals.front().first->chromosome()->print(log_stream);
                std::cout << std::endl;

                details::piece_pair_statistics.print_most_frequent(10, log_stream);
            }

            std::vector<std::shared_ptr<chromosome_evaluator_t>> next_individuals;

            // 次世代に現世代のエリートを複製する。
            if (m_elite_number > 0)
            {
                for (std::size_t i = 0; i < m_elite_number; ++i)
                {
                    if (next_individuals.size() < individuals.size())
                    {
                        const std::shared_ptr<chromosome_t> chromosome = std::make_shared<chromosome_t>(*evaluated_individuals[i].first->chromosome());
                        const std::shared_ptr<chromosome_evaluator_t> next_individual = std::make_shared<chromosome_evaluator_t>(chromosome, evaluated_individuals[i].first->name(), uid++);
                        next_individuals.push_back(next_individual);
                    }
                }
            }

            // 次世代が現世代と同数になるまで変異・交叉・複製を繰り返す。
            while (next_individuals.size() < individuals.size())
            {
                const action_t action = random_action();
                if (action == action_t::mutation)
                {
                    const std::shared_ptr<chromosome_evaluator_t> & individual = select_individual(fitness_table);
                    const std::shared_ptr<chromosome_t> chromosome = std::make_shared<chromosome_t>(*individual->chromosome());
                    chromosome->mutate();
                    const std::shared_ptr<chromosome_evaluator_t> next_individual = std::make_shared<chromosome_evaluator_t>(chromosome, individual->name(), uid++);
                    next_individuals.push_back(next_individual);
                }
                else if (action == action_t::crossover)
                {
                    const std::shared_ptr<chromosome_evaluator_t> & base = select_individual(fitness_table);
                    const std::shared_ptr<chromosome_evaluator_t> & sub = select_individual(fitness_table);
                    const std::size_t chromosome_distance = base->chromosome()->distance(*sub->chromosome());
                    const std::shared_ptr<chromosome_t> chromosome = std::make_shared<chromosome_t>(*base->chromosome());
                    chromosome->clossover(*sub->chromosome());
                    
                    if (chromosome_distance < m_min_chromosome_distance)
                    {
                        const std::size_t chromosome_distance_diff = m_min_chromosome_distance - chromosome_distance;
                        for (std::size_t i = 0; i < chromosome_distance_diff; ++i)
                            chromosome->mutate();
                    }
                    
                    const std::shared_ptr<chromosome_evaluator_t> next_individual = std::make_shared<chromosome_evaluator_t>(chromosome, base->name(), uid++);
                    next_individuals.push_back(next_individual);
                }
                else if (action == action_t::selection)
                {
                    const std::shared_ptr<chromosome_evaluator_t> & individual = select_individual(fitness_table);
                    const std::shared_ptr<chromosome_t> chromosome = std::make_shared<chromosome_t>(*individual->chromosome());
                    const std::shared_ptr<chromosome_evaluator_t> next_individual = std::make_shared<chromosome_evaluator_t>(chromosome, individual->name(), uid++);
                    next_individuals.push_back(next_individual);
                }
            }

            // 世代を更新する。
            individuals = std::move(next_individuals);
        }

        inline genetic_algorithm_t(const std::vector<std::filesystem::path> & paths)
        {
            for (const std::filesystem::path & path : paths)
            {
                try
                {
                    const std::string filename = path.filename().string();
                    std::smatch results;
                    std::string name;
                    chromosome_evaluator_t::id_type id{};
                    if (!std::regex_match(filename, results, std::regex(R"((.*)_(\d+))")))
                        throw std::exception();
                    name = results[1].str();
                    id = std::stoull(results[2].str());
                    individuals.push_back(std::make_shared<chromosome_evaluator_t>(path, name, id));
                }
                catch (...)
                {
                    std::cerr << "invalid format path" << std::endl;
                }
            }
        }

        inline void write_file(const std::string & directory) const
        {
            std::filesystem::create_directories(directory);
            for (const auto & individual : individuals)
                individual->write_file(directory);
        }

        inline void set_mutation_rate (parameter_type mutation_rate ) noexcept { m_mutation_rate  = mutation_rate ; }
        inline void set_crossover_rate(parameter_type crossover_rate) noexcept { m_crossover_rate = crossover_rate; }
        inline void set_selection_rate(parameter_type selection_rate) noexcept { m_selection_rate = selection_rate; }
        inline void set_max_move_count(move_count_t max_move_count) noexcept { m_max_move_count = max_move_count; }
        inline void set_min_chromosome_distance(parameter_type min_chromosome_distance) noexcept { m_min_chromosome_distance = min_chromosome_distance; }
        inline void set_elite_number(parameter_type elite_number) noexcept { m_elite_number = elite_number; }

    private:
        std::vector<std::shared_ptr<chromosome_evaluator_t>> individuals;
        parameter_type m_mutation_rate  = 10;
        parameter_type m_crossover_rate = 800;
        parameter_type m_selection_rate = 190;
        move_count_t m_max_move_count = 300;
        parameter_type m_min_chromosome_distance = 1;
        parameter_type m_elite_number = 1;
    };

    inline int parse_command_line(int argc, const char ** argv) noexcept
    {
        try
        {
            auto callback = [&](const std::string & option, const std::vector<std::string> & params)
            {
                if (option == "help")
                    print_help();
                else if (option == "print-board" && !params.empty())
                {
                    const std::optional<bool> value = details::to_bool(params[0]);
                    if (value)
                        details::program_options::print_board = *value;
                    else
                        std::cerr << "invalid print-board parameter" << std::endl;
                }
                else if (option == "print-moves" && !params.empty())
                {
                    const std::optional<bool> value = details::to_bool(params[0]);
                    if (value)
                        details::program_options::print_moves = *value;
                    else
                        std::cerr << "invalid print-moves parameter" << std::endl;
                }
                else if (option == "print-check" && !params.empty())
                {
                    const std::optional<bool> value = details::to_bool(params[0]);
                    if (value)
                        details::program_options::print_check = *value;
                    else
                        std::cerr << "invalid print-check parameter" << std::endl;
                }
                else if (option == "black" && !params.empty())
                {
                    details::program_options::black_name = params[0];
                }
                else if (option == "white" && !params.empty())
                {
                    details::program_options::white_name = params[0];
                }
                else if (option == "sfen" && !params.empty())
                {
                    std::string sfen_string;
                    for (std::size_t i = 0; i < params.size(); ++i)
                    {
                        if (i > 0)
                            sfen_string += ' ';
                        sfen_string += params[i];
                    }
                    details::program_options::sfen = sfen_string;
                }
                else if (option == "max-iddfs-iteration" && !params.empty())
                {
                    const std::optional<depth_t> opt_max_iddfs_iteration = details::cast_to<depth_t>(params[0]);
                    if (opt_max_iddfs_iteration)
                        details::program_options::max_iddfs_iteration = *opt_max_iddfs_iteration;
                    else
                        std::cerr << "invalid max-depth parameter" << std::endl;
                }
                else if (option == "limit-time" && !params.empty())
                {
                    const std::optional<std::chrono::milliseconds::rep> opt_rep = details::cast_to< std::chrono::milliseconds::rep>(params[0]);
                    if (opt_rep)
                        details::program_options::limit_time = std::chrono::milliseconds{ *opt_rep };
                    else
                        std::cerr << "invalid limit-time parameter" << std::endl;
                }
                else if (option == "ga-iteration" && !params.empty())
                {
                    details::program_options::ga_iteration = details::cast_to<unsigned long long>(params[0]);
                    if (!details::program_options::ga_iteration)
                        std::cerr << "invalid ga-iteration parameter" << std::endl;
                }
                else if (option == "ga-chromosomes-directory" && !params.empty())
                {
                    details::program_options::ga_chromosomes_directory = params[0];
                }
                else if (option == "ga-logs-directory" && !params.empty())
                {
                    details::program_options::ga_logs_directory = params[0];
                }
                else if (option == "ga-create-chromosome" && !params.empty())
                {
                    details::program_options::ga_create_chromosome = details::cast_to<unsigned int>(params[0]);
                    if (!details::program_options::ga_create_chromosome)
                        std::cerr << "invalid ga-create-chromosome parameter" << std::endl;
                }
                else if (option == "ga-create-mode" && !params.empty())
                {
                    if (params[0] == "random" || params[0] == "template")
                        details::program_options::ga_create_mode = params[0];
                    else
                        std::cerr << "invalid ga-create-mode parameter" << std::endl;
                }
                else if (option == "ga-mutation-rate" && !params.empty())
                {
                    details::program_options::ga_mutation_rate = details::cast_to<unsigned int>(params[0]);
                    if  (!details::program_options::ga_mutation_rate)
                        std::cerr << "invalid ga-mutation-rate parameter" << std::endl;
                }
                else if (option == "ga-crossover-rate" && !params.empty())
                {
                    details::program_options::ga_crossover_rate = details::cast_to<unsigned int>(params[0]);
                    if (!details::program_options::ga_crossover_rate)
                        std::cerr << "invalid ga-crossover-rate parameter" << std::endl;
                }
                else if (option == "ga-selection-rate" && !params.empty())
                {
                    details::program_options::ga_selection_rate = details::cast_to<unsigned int>(params[0]);
                    if (!details::program_options::ga_selection_rate)
                        std::cerr << "invalid ga-selection-rate parameter" << std::endl;
                }
                else if (option == "ga-max-move-count" && !params.empty())
                {
                    details::program_options::ga_max_move_count = details::cast_to<unsigned int>(params[0]);
                    if (!details::program_options::ga_max_move_count)
                        std::cerr << "invalid ga-selection-rate parameter" << std::endl;
                }
                else if (option == "ga-min-chromosome-distance" && !params.empty())
                {
                    details::program_options::ga_min_chromosome_distance = details::cast_to<unsigned int>(params[0]);
                    if  (!details::program_options::ga_min_chromosome_distance)
                        std::cerr << "invalid ga-min-chromosome-distance parameter" << std::endl;
                }
                else if (option == "ga-elite-number" && !params.empty())
                {
                    details::program_options::ga_elite_number = details::cast_to<unsigned int>(params[0]);
                    if (!details::program_options::ga_elite_number)
                        std::cerr << "invalid ga-elite-number parameter" << std::endl;
                }
                else if (option == "ga-dump-chromosome" && !params.empty())
                {
                    details::program_options::ga_dump_chromosome = params[0];
                }
                else if (option == "ga-mutation-number" && !params.empty())
                {
                    details::program_options::ga_mutation_number = details::cast_to<unsigned int>(params[0]);
                    if (!details::program_options::ga_mutation_number)
                        std::cerr << "invalid ga-mutation-number parameter" << std::endl;
                }
                else if (option == "ga-thread-number" && !params.empty())
                {
                    const std::optional<unsigned int> ga_thread_number = details::cast_to<unsigned int>(params[0]);
                    if (ga_thread_number && *ga_thread_number > 0)
                        details::program_options::ga_thread_number = *ga_thread_number;
                    else
                        std::cerr << "invalid ga-thread-number parameter" << std::endl;
                }
                else if (option == "piece-pair-statistics" && !params.empty())
                {
                    details::program_options::piece_pair_statistics = params[0];
                }
            };
            parse_program_options(argc, argv, callback);

            try
            {
                if (std::filesystem::exists(details::program_options::piece_pair_statistics))
                    details::piece_pair_statistics.read_file(details::program_options::piece_pair_statistics);
            }
            catch (const std::filesystem::filesystem_error &)
            {
                ;
            }

            if (details::program_options::ga_create_chromosome && details::program_options::ga_create_mode)
            {
                std::filesystem::create_directories(details::program_options::ga_chromosomes_directory);
                const unsigned int mutation_number = details::program_options::ga_mutation_number ? *details::program_options::ga_mutation_number : 0;
                for (unsigned int i = 0; i < *details::program_options::ga_create_chromosome; ++i)
                {
                    const std::filesystem::path path = std::filesystem::path{ details::program_options::ga_chromosomes_directory } / (std::to_string(i) + "_0");
                    const std::shared_ptr<chromosome_t> chromosome = std::make_shared<chromosome_t>();
                    if (*details::program_options::ga_create_mode == "random")
                        chromosome->generate_random();
                    else if (*details::program_options::ga_create_mode == "template")
                        chromosome->generate_template();
                    for (unsigned int mutation_count = 0; mutation_count < mutation_number; ++mutation_count)
                        chromosome->mutate();
                    chromosome->write_file(path);
                }
            }
            else if (details::program_options::ga_iteration)
            {
                try
                {
                    std::filesystem::remove_all(details::program_options::ga_logs_directory);
                }
                catch (const std::filesystem::filesystem_error &)
                {
                    ;
                }
                std::vector<std::filesystem::path> chromosome_paths;
                for (const std::filesystem::directory_entry & entry : std::filesystem::directory_iterator{ details::program_options::ga_chromosomes_directory })
                    if (entry.is_regular_file())
                        chromosome_paths.push_back(entry.path());
                const std::shared_ptr<genetic_algorithm_t> ga = std::make_shared<genetic_algorithm_t>(chromosome_paths);

                if (details::program_options::ga_mutation_rate)
                    ga->set_mutation_rate(*details::program_options::ga_mutation_rate);
                if (details::program_options::ga_crossover_rate)
                    ga->set_crossover_rate(*details::program_options::ga_crossover_rate);
                if (details::program_options::ga_selection_rate)
                    ga->set_selection_rate(*details::program_options::ga_selection_rate);
                if (details::program_options::ga_max_move_count)
                    ga->set_max_move_count(*details::program_options::ga_max_move_count);
                if (details::program_options::ga_min_chromosome_distance)
                    ga->set_min_chromosome_distance(*details::program_options::ga_min_chromosome_distance);
                if (details::program_options::ga_elite_number)
                    ga->set_elite_number(*details::program_options::ga_elite_number);

                unsigned long long uid = 0;
                for (unsigned long long iteration_count = 0; iteration_count < *details::program_options::ga_iteration; ++iteration_count)
                {
                    const std::filesystem::path log_directory = std::filesystem::path{ details::program_options::ga_logs_directory } / std::to_string(iteration_count);
                    std::filesystem::create_directories(log_directory);
                    ga->run(log_directory, uid, details::program_options::ga_thread_number);
                    ga->write_file(details::program_options::ga_chromosomes_directory);
                }
            }
            else if (details::program_options::ga_dump_chromosome)
            {
                const std::shared_ptr<chromosome_t> chromosome = std::make_shared<chromosome_t>();
                chromosome->read_file(*details::program_options::ga_dump_chromosome);
                chromosome->print();
            }
            else if (details::program_options::black_name && details::program_options::white_name)
            {
                auto black_iter = kishi_map.find(*details::program_options::black_name);
                if (black_iter == kishi_map.end())
                    throw invalid_command_line_input{ "invalid black name" };
                const std::shared_ptr<abstract_kishi_t> & black_kishi = black_iter->second;

                auto white_iter = kishi_map.find(*details::program_options::white_name);
                if (white_iter == kishi_map.end())
                    throw invalid_command_line_input{ "invalid white name" };
                const std::shared_ptr<abstract_kishi_t> & white_kishi = white_iter->second;

                std::optional<kyokumen_t> opt_kyokumen;
                if (details::program_options::sfen)
                    opt_kyokumen = kyokumen_t{ *details::program_options::sfen };
                else
                    opt_kyokumen = kyokumen_t{};
                do_taikyoku(*opt_kyokumen, black_kishi, white_kishi);
            }
            else
            {
                usi_engine_t usi_engine;
                usi_engine.run();
            }

        }
        catch (const std::exception & e)
        {
            std::cerr << e.what() << std::endl;
            return 1;
        }
        catch (...)
        {
            std::cerr << "unknown exception" << std::endl;
            return 1;
        }

        return 0;
    }

} // namespace shogipp

#endif // SHOGIPP_DEFINED