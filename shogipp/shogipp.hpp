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
 * @breif �n�b�V���l�̃o�C�g�����`����B
 * @details SIZE_OF_HASH % sizeof(std::size_t) == 0 �łȂ���΂Ȃ�Ȃ��B
 *          ���̃}�N������`����Ă��Ȃ��ꍇ�A�n�b�V���l�Ƃ��� std::size_t ���g�p����B
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
#define VALIDATE_kyokumen_ROLLBACK(kyokumen)
#else
#define VALIDATE_kyokumen_ROLLBACK(kyokumen) kyokumen_rollback_validator_t kyokumen_rollback_validator{ kyokumen }
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
         * @breif ��������󔒂ŋ�؂�B
         * @tparam OutputIterator ��؂�ꂽ������̏o�̓C�e���[�^�^
         * @tparam CharT �����^
         * @param result ��؂�ꂽ������̏o�̓C�e���[�^
         * @param s ��؂��镶����
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
        * @breif SHOGIPP_ASSERT �}�N���̎���
        * @param assertion ����]������ bool �l
        * @param expr ����\�����镶����
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
         * @breif ���ǂݎ萔�A���s���ԁA�ǂݎ葬�x�𑪒肷��@�\��񋟂���B
         */
        class timer_t
        {
        public:
            /**
             * @breif ���Ԍv�����J�n����B
             */
            inline timer_t() noexcept;

            /**
             * @breif ���Ԍv�����ĊJ�n����B
             */
            inline void clear() noexcept;

            /**
             * @breif �o�ߎ��Ԃ�W���o�͂ɏo�͂���B
             */
            inline void print_elapsed_time() noexcept;

            /**
             * @breif �ǂݎ萔�̎Q�Ƃ�Ԃ��B
             * @return �ǂݎ萔�̎Q��
             */
            inline search_count_t & search_count() noexcept;

            /**
             * @breif �ǂݎ萔�̎Q�Ƃ�Ԃ��B
             * @return �ǂݎ萔�̎Q��
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

        inline void timer_t::print_elapsed_time() noexcept
        {
            const std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_begin).count();
            const search_count_t nps = m_search_count * 1000 / duration;

            std::cout
                << std::endl
                << "���ǂݎ萔: " << m_search_count << std::endl
                << "���s����[ms]: " << duration << std::endl
                << "�ǂݎ葬�x[n/s]: " << nps << std::endl << std::endl;
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
         * @breif std::string_view �̐擪���瑱���󔒂��폜����B
         */
        inline void trim_front_space(std::string_view & sv) noexcept
        {
            while (!sv.empty() && sv.front() == ' ')
                sv.remove_prefix(1);
        }

        /**
         * @breif std::string_view �̐擪����\�����ꂽ��������폜���邱�Ƃ����݂�B
         * @param sv ������
         * @param token �\�����ꂽ������
         * @retval true �\�����ꂽ������̍폜�ɐ��������ꍇ
         * @retval false �\�����ꂽ������̍폜�ɐ������Ȃ������ꍇ
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
        constexpr inline std::uint32_t bool_array_to_32bitmask(const bool (&bool_array)[N]) noexcept
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
        inline milli_second_time_t test_time_performance(Function && func, std::size_t n)
        {
            const std::chrono::system_clock::time_point begin = std::chrono::system_clock::now();
            for (std::size_t i = 0; i < n; ++i)
                func();
            const std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        }

        /**
         * @breif ������Ԃ��B
         * @return ����
         */
        template<typename T>
        inline T random(int min = std::numeric_limits<T>::min(), int max = std::numeric_limits<T>::max())
        {
            static std::minstd_rand random_impl{ SHOGIPP_SEED };
            std::uniform_int_distribution<> uid{ min, max };
            return uid(random_impl);
        }

        /**
         * @breif �o�C�g�l����l��������B
         * @param a �l1
         * @param b �l2
         * @return ��l�������ꂽ�l
         */
        inline unsigned char uniform_croossover(unsigned char a, unsigned char b)
        {
            unsigned char result = 0;
            unsigned char random_byte = random<unsigned char>();
            result |= (a & random_byte);
            result |= (b & (~random_byte));
            return result;
        }
    } // namespace details

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
     * @breif ��𕶎���ɕϊ�����B
     * @param piece ��
     * @return ������
     */
    inline const char * to_string_impl(piece_value_t piece) noexcept
    {
        static const char * map[]{
            "�E",
            "��", "��", "�j", "��", "��", "�p", "��", "��", "��", "��", "�\", "�S", "�n", "��",
            "��", "��", "�j", "��", "��", "�p", "��", "��", "��", "��", "�\", "�S", "�n", "��",
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
        constexpr inline bool empty() const noexcept;

        /**
         * @breif ��𕶎���ɕϊ�����B
         * @param piece ��
         * @return ������
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

        /**
         * @breif ������̋�ɕϊ�����B
         * @return ���̋�
         * @details pawn -> black_pawn or white_pawn
         */
        inline colored_piece_t to_colored(color_t color) const noexcept;
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
         * @breif �����邩���肷��B
         * @retval true �����
         * @retval false ����Ȃ�
         */
        inline bool is_promotable() const noexcept;

        /*
         * @breif ���L�҂̎�Ԃ��擾����B
         * @return ���
         */
        inline color_t to_color() const noexcept;

        /*
         * @breif ������(���E�p�E��E�n�E��)�����肷��B
         * @return �����ł���ꍇ true
         */
        inline bool is_hashirigoma() const noexcept;

        /**
         * @breif �������Ƃ��ēK�i�ł��邩���肷��B
         * @retval true ������Ƃ��ēK�i�ł���
         * @retval false ������Ƃ��ēK�i�łȂ�
         */
        inline bool is_captured() const noexcept;

        /*
         * @breif ��𐬂�O�̋�ɕϊ�����B
         * @return ����O�̋�
         */
        inline colored_piece_t to_unpromoted() const noexcept;

        /*
         * @breif ��𐬂��ɕϊ�����B
         * @return �����
         */
        inline colored_piece_t to_promoted() const noexcept;

        /**
         * @breif piece �� target_piece �ɍ��v���邩���肷��B
         * @param piece ��
         * @retval true ���v����
         * @retval false ���v���Ȃ�
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

    inline bool colored_piece_t::is_hashirigoma() const noexcept
    {
        SHOGIPP_ASSERT(!empty());
        constexpr bool map[]
        {
            false,
            false, true, false, false, false, true, true, false, false, false, false, false, true, true,
            false, true, false, false, false, true, true, false, false, false, false, false, true, true,
        };
        constexpr std::uint32_t bitmask = details::bool_array_to_32bitmask(map);
        return details::bitmask_has(bitmask, value());
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
            black_pawn, black_promoted_lance, black_promoted_knight, black_promoted_silver, 0, black_promoted_bishop, black_promoted_rook, 0, 0, 0, 0, 0, 0, 0,
            white_pawn, white_promoted_lance, white_promoted_knight, white_promoted_silver, 0, white_promoted_bishop, white_promoted_rook, 0, 0, 0, 0, 0, 0, 0,
        };
        return map[value()];
    }

    inline piece_value_t to_noncolored_impl(piece_value_t piece) noexcept
    {
        SHOGIPP_ASSERT(piece != empty_value);
        return (piece > color_distance) ? piece - color_distance : piece;
    }

    /**
     * @breif ������̎�Ԃ̋�ɕϊ�����B
     * @param piece ��
     * @param color ���
     * @return ����̎�Ԃ̋�
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
    constexpr position_t npos = -1; // �����ȍ��W��\������萔
    constexpr position_t width = 11;
    constexpr position_t height = 11;
    constexpr position_t padding_width = 1;
    constexpr position_t padding_height = 1;
    constexpr position_t position_size = width * height;
    constexpr position_t position_begin = width + padding_width;
    constexpr position_t position_end = position_size - position_begin;
    constexpr position_t file_size = 9;
    constexpr position_t rank_size = 9;

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
     * @breif ���W����i�𒊏o����B
     * @param position ���W
     * @return �i
     */
    inline constexpr position_t position_to_rank(position_t position) noexcept
    {
        return position / width - padding_height;
    }

    /**
     * @breif ���W����؂𒊏o����B
     * @param position ���W
     * @return ��
     */
    inline constexpr position_t position_to_file(position_t position) noexcept
    {
        return position % width - padding_width;
    }

    /**
     * @breif 2�̍��W�Ԃ̃}���n�b�^���������v�Z����B
     * @param a ���WA
     * @param b ���WB
     * @return 2�̍��W�Ԃ̃}���n�b�^������
     */
    inline position_t distance(position_t a, position_t b) noexcept
    {
        const position_t file_a = position_to_file(a);
        const position_t rank_a = position_to_rank(a);
        const position_t file_b = position_to_file(b);
        const position_t  rank_b = position_to_rank(b);
        return static_cast<position_t>(std::abs(file_a - file_b) + std::abs(rank_a - rank_b));
    }

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

    using move_count_t = unsigned int;
    using depth_t = unsigned int;

    constexpr depth_t default_max_depth = 3;
    constexpr depth_t default_max_selective_depth = std::numeric_limits<depth_t>::max();

    depth_t program_option_max_depth = default_max_depth;
    depth_t program_option_max_selective_depth = default_max_selective_depth;

    /**
     * @breif ���̏ꍇ�� -1 ���A���̏ꍇ�� 1 ��Ԃ��B
     * @param color ���
     * @return �������]�p�̐��l
     */
    inline constexpr position_t reverse(color_t color) noexcept
    {
        return color == white ? -1 : 1;
    }

    /**
     * @breif �d�����Ȃ��n�b�V���l��Ԃ��B
     * @return �d�����Ȃ��n�b�V���l
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
     * @breif �C�ӂ̃o�C�g���̃n�b�V���l��񋟂���B
     * @tparam HashSize �n�b�V���l�̃o�C�g��
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
     * @breif �n�b�V���l��16�i���\���̕�����ɕϊ�����B
     * @param hash �n�b�V���l
     * @return 16�i���\���̕�����
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

    enum : std::size_t
    {
        captured_pawn_offset    = 0                                            , captured_pawn_size   = 18 + 1,
        captured_lance_offset   = captured_pawn_offset   + captured_pawn_size  , captured_lance_size  =  4 + 1,
        captured_knight_offset  = captured_lance_offset  + captured_lance_size , captured_knight_size =  4 + 1,
        captured_silver_offset  = captured_knight_offset + captured_knight_size, captured_silver_size =  4 + 1,
        captured_gold_offset    = captured_silver_offset + captured_silver_size, captured_gold_size   =  4 + 1,
        captured_bishop_offset  = captured_gold_offset   + captured_gold_size  , captured_bishop_size =  2 + 1,
        captured_rook_offset    = captured_bishop_offset + captured_bishop_size, captured_rook_size   =  2 + 1,
        captured_size           = captured_rook_offset   + captured_rook_size
    };

    constexpr std::size_t captured_offsets[]
    {
        0, /* dummy */
        captured_pawn_offset,
        captured_lance_offset,
        captured_knight_offset,
        captured_silver_offset,
        captured_gold_offset,
        captured_bishop_offset,
        captured_rook_offset,
    };

    class move_t;

    /**
     * @breif �n�b�V���e�[�u��
     */
    class hash_table_t
    {
    public:
        /**
         * �n�b�V���e�[�u�����\�z����B
         */
        inline hash_table_t();

        /**
         * @breif �Տ�̋�̃n�b�V���l���v�Z����B
         * @param piece ��
         * @param position ��̍��W
         * @return �n�b�V���l
         */
        inline hash_t piece_hash(colored_piece_t piece, position_t position) const noexcept;

        /**
         * @breif ������̃n�b�V���l���v�Z����B
         * @param piece ��
         * @param count ��̐�
         * @param color ���
         * @return �n�b�V���l
         */
        inline hash_t captured_piece_hash(captured_piece_t piece, std::size_t count, color_t color) const noexcept;

        /**
         * @breif ��Ԃ̃n�b�V���l���v�Z����B
         * @param color ���
         * @return �n�b�V���l
         */
        inline hash_t color_hash(color_t color) const noexcept;

        /**
         * @breif ���@��̃n�b�V���l���v�Z����B
         * @param move ���@��
         * @param color ���
         * @return �n�b�V���l
         */
        inline hash_t move_hash(const move_t & move, color_t color) const noexcept;

        hash_t board_table[piece_size * file_size * rank_size];              // �Ղ̃n�b�V���e�[�u��
        hash_t captured_piece_table[captured_size * color_t::size()];            // ������̃n�b�V���e�[�u��
        hash_t color_table[color_t::size()];                                     // ��Ԃ̃n�b�V���e�[�u��
        hash_t move_table[(position_size + 1) * position_size * color_t::size()];          // �ړ������̃n�b�V���e�[�u��
        hash_t put_table[position_size * captured_piece_size * color_t::size()];      // �ł�̃n�b�V���e�[�u��
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
        SHOGIPP_ASSERT(!(piece == captured_knight && count >= captured_knight_size ));
        SHOGIPP_ASSERT(!(piece == captured_silver && count >= captured_silver_size ));
        SHOGIPP_ASSERT(!(piece == captured_gold   && count >= captured_gold_size ));
        SHOGIPP_ASSERT(!(piece == captured_bishop && count >= captured_bishop_size));
        SHOGIPP_ASSERT(!(piece == captured_rook   && count >= captured_rook_size  ));

        std::size_t index = captured_offsets[piece.value()];
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
     * @breif ����\�����镶������擾����B
     * @param color ���
     * @return ����\�����镶����
     */
    inline const char * color_to_string(color_t color) noexcept
    {
        const char * map[]{ "���", "���" };
        return map[color.value()];
    }

    /**
     * @breif ���l��S�p������ɕϊ�����B
     * @param value ���l
     * @return �S�p������
     * @details ������̍ő喇��18�𒴂���l���w�肵�Ă��̊֐����Ăяo���Ă͂Ȃ�Ȃ��B
     */
    inline const char * to_zenkaku_digit(unsigned int value) noexcept
    {
        const char * map[]
        {
            "�O", "�P", "�Q", "�R", "�S", "�T", "�U", "�V", "�W", "�X",
            "�P�O", "�P�P", "�P�Q", "�P�R", "�P�S", "�P�T", "�P�U", "�P�V", "�P�W"
        };
        SHOGIPP_ASSERT(value <= std::size(map));
        return map[value];
    }

    /**
     * @breif ��̈ړ���̑��΍��W�̔z��̐擪���w���|�C���^���擾����B
     * @param piece ��
     * @return ��̈ړ���̑��΍��W�̔z��̐擪���w���|�C���^
     * @details ���̊֐����Ԃ��|�C���^�̎w�����W�� 0 �ŏI�[������Ă���B
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
     * @breif ��̈ړ���̑��΍��W�̔z��̐擪���w���|�C���^���擾����B
     * @param piece ��
     * @return ��̈ړ���̑��΍��W�̔z��̐擪���w���|�C���^
     * @details ���̊֐����Ԃ��|�C���^�̎w�����W�� 0 �ŏI�[������Ă���B
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
     * @breif �i�𕶎���ɕϊ�����B
     * @param rank �i
     * @return ������
     */
    inline const char * rank_to_string(position_t rank) noexcept
    {
        static const char * map[]{ "��", "��", "�O", "�l", "��", "�Z", "��", "��", "��" };
        SHOGIPP_ASSERT(rank >= 0);
        SHOGIPP_ASSERT(rank < static_cast<position_t>(std::size(map)));
        return map[rank];
    }

    /**
     * @breif �؂𕶎���ɕϊ�����B
     * @param rank ��
     * @return ������
     */
    inline const char * file_to_string(position_t file) noexcept
    {
        static const char * map[]{ "�X", "�W", "�V", "�U", "�T", "�S", "�R", "�Q", "�P" };
        SHOGIPP_ASSERT(file >= 0);
        SHOGIPP_ASSERT(file < static_cast<position_t>(std::size(map)));
        return map[file];
    }

    /**
     * @breif ���W�𕶎���ɕϊ�����B
     * @param position ���W
     * @return ������
     */
    inline std::string position_to_string(position_t position)
    {
        return std::string{}.append(file_to_string(position_to_file(position))).append(rank_to_string(position_to_rank(position)));
    }

    /**
     * @breif ���W��SFEN�\�L�@�ɏ�������������ɕϊ�����B
     * @param position ���W
     * @return SFEN�\�L�@�ɏ�������������
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
     * @breif �؂ƒi������W���擾����B
     * @param file ��
     * @param rank �i
     * @return ���W
     */
    inline position_t file_rank_to_position(position_t file, position_t rank) noexcept
    {
        return width * (rank + padding_height) + padding_width + file;
    }

    static const position_t default_king_pos_list[]
    {
        file_rank_to_position(4, 8),
        file_rank_to_position(4, 0)
    };

    /**
     * @breif SFEN�\�L�@�ɏ����������W�̕����񂩂���W���擾����B
     * @param sfen_position SFEN�\�L�@�ɏ����������W�̕�����
     * @return ���W
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
     * @breif ���@��
     */
    class move_t
    {
    public:
        /**
         * @breif �ł�����\�z����B
         * @param destination �ł��W
         * @param source_piece �ł�
         */
        inline move_t(position_t destination, captured_piece_t captured_piece) noexcept;

        /**
         * @breif �ړ��������\�z����B
         * @param source �ړ����̍��W
         * @param destination �ړ���̍��W
         * @param source_piece �ړ����̋�
         * @param captured_piece �ړ���̋�
         * @param promote �����s����
         */
        inline move_t(position_t source, position_t destination, colored_piece_t source_piece, colored_piece_t captured_piece, bool promote) noexcept;

        /**
         * @breif SFEN�\�L�@�ɏ������� moves �̌�ɑ��������񂩂����\�z����B
         * @param sfen SFEN�\�L�@�ɏ������� moves �̌�ɑ���������
         * @param board ��
         */
        inline move_t(std::string_view sfen_move, const board_t & board);

        /**
         * @breif �ł��肩���肷��B
         * @retval true �ł���ł���
         * @retval false �ړ������ł���
         */
        inline bool put() const noexcept;

        /**
         * @breif �ړ����̍��W���擾����B
         * @return �ړ����̍��W
         * @details put() �� true ��Ԃ��ꍇ�ɂ��̊֐����Ăяo�����ꍇ�A�����Ȓl���Ԃ�B
         */
        inline position_t source() const noexcept;

        /**
         * @breif �ړ���̍��W���擾����B
         * @return �ړ���̍��W
         * @details put() �� true ��Ԃ��ꍇ�A���̊֐��͑ł�̍��W��Ԃ��B
         */
        inline position_t destination() const noexcept;

        /**
         * @breif �ړ����̋���擾����B
         * @return �ړ����̋�
         * @details put() �� true ��Ԃ��ꍇ�ɂ��̊֐����Ăяo�����ꍇ�A�����Ȓl���Ԃ�B
         */
        inline colored_piece_t source_piece() const noexcept;

        /**
         * @breif �ł���擾����B
         * @return �ł�
         * @details put() �� false ��Ԃ��ꍇ�ɂ��̊֐����Ăяo�����ꍇ�A�����Ȓl���Ԃ�B
         */
        inline captured_piece_t captured_piece() const noexcept;

        /**
         * @breif �ړ���̋���擾����B
         * @return �ړ���̋�
         * @detalis put �� true ��Ԃ��ꍇ�ɂ��̊֐����Ăяo�����ꍇ�A�����Ȓl���Ԃ�B
         */
        inline colored_piece_t destination_piece() const noexcept;

        /**
         * @breif ���邩�ۂ����擾����B
         * @retval true ����
         * @retval false ����Ȃ�
         * @detalis put �� true ��Ԃ��ꍇ�ɂ��̊֐����Ăяo�����ꍇ�A�����Ȓl���Ԃ�B
         */
        inline bool promote() const noexcept;

        /**
         * @breif ���@���SFEN�\�L�@�ɏ�������������ɕϊ�����B
         * @return SFEN�\�L�@�ɏ�������������
         */
        inline std::string sfen_string() const;

    private:
        position_t      m_source;           // �ړ����̍��W(source == npos �̏ꍇ�A�������ł�)
        position_t      m_destination;      // �ړ���̍��W(source == npos �̏ꍇ�A destination �͑ł��W)
        colored_piece_t m_source_piece;     // �ړ����̋�(source == npos �̏ꍇ�A source_piece() �͑ł�����)
        colored_piece_t m_destination_piece;   // �ړ���̋�(source == npos �̏ꍇ�A captured_piece �͖���`)
        bool            m_promote;          // ����ꍇ true
    };

    inline move_t::move_t(position_t destination, captured_piece_t captured_piece) noexcept
        : m_source{ npos }
        , m_destination{ destination }
        , m_source_piece{ captured_piece.value() }
        , m_destination_piece{ empty.value() }
        , m_promote{ false }
    {
        SHOGIPP_ASSERT(captured_piece.value() >= captured_pawn.value());
        SHOGIPP_ASSERT(captured_piece.value() <= captured_rook.value());
    }

    inline move_t::move_t(position_t source, position_t destination, colored_piece_t source_piece, colored_piece_t captured_piece, bool promote) noexcept
        : m_source{ source }
        , m_destination{ destination }
        , m_source_piece{ source_piece }
        , m_destination_piece{ captured_piece }
        , m_promote{ promote }
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

    /**
     * @breif ���@����i�[���� std::vector ��\������B
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
        if (move.put())
            return 0;
        if (move.destination_piece().empty())
            return 1;
        return 2;
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
     * @breif ������
     */
    class captured_pieces_t
    {
    public:
        using size_type = unsigned char;

        /**
         * @breif ��������\�z����B
         */
        inline captured_pieces_t() noexcept;

        constexpr inline captured_pieces_t(const captured_pieces_t &) noexcept = default;
        constexpr inline captured_pieces_t & operator =(const captured_pieces_t &) noexcept = default;

        /**
         * @breif �������W���o�͂ɏo�͂���B
         */
        inline void print() const;

        /**
         * @breif ��ƑΉ����鎝����̐��̎Q�Ƃ�Ԃ��B
         * @param ��
         * @return ��ƑΉ����鎝����̐��̎Q��
         */
        inline size_type & operator [](captured_piece_t piece) noexcept;

        /**
         * @breif ��ƑΉ����鎝����̐��̎Q�Ƃ�Ԃ��B
         * @param ��
         * @return ��ƑΉ����鎝����̐��̎Q��
         */
        inline const size_type & operator [](captured_piece_t piece) const noexcept;

    private:
        size_type count[captured_piece_size];
    };

    inline captured_pieces_t::captured_pieces_t() noexcept
    {
        std::fill(std::begin(count), std::end(count), 0);
    }

    inline void captured_pieces_t::print() const
    {
        unsigned int kind = 0;
        for (piece_value_t piece = captured_rook.value(); piece >= captured_pawn.value(); --piece)
        {
            if ((*this)[captured_piece_t{ piece }] > 0)
            {
                std::cout << captured_piece_t{ piece }.to_string();
                if ((*this)[captured_piece_t{ piece }] > 1)
                    std::cout << to_zenkaku_digit((*this)[captured_piece_t{ piece }]);
                ++kind;
            }
        }
        if (kind == 0)
            std::cout << "�Ȃ�";
        std::cout << std::endl;
    }

    inline captured_pieces_t::size_type & captured_pieces_t::operator [](captured_piece_t piece) noexcept
    {
        SHOGIPP_ASSERT(!piece.empty());
        return count[piece.value() - pawn_value];
    }

    inline const captured_pieces_t::size_type & captured_pieces_t::operator [](captured_piece_t piece) const noexcept
    {
        return (*const_cast<captured_pieces_t *>(this))[piece];
    }

    class kyokumen_rollback_validator_t;

#define _ { empty.value() }
#define x { out_of_range.value() }
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
        x, white_lance, white_knight, white_silver, white_gold, white_king, white_gold, white_silver, white_knight, white_lance, x,
        x, _, white_rook, _, _, _, _, _, white_bishop, _, x,
        x, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, _, _, _, _, _, _, _, _, _, x,
        x, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, x,
        x, _, black_bishop, _, _, _, _, _, black_rook, _, x,
        x, black_lance, black_knight, black_silver, black_gold, black_king, black_gold, black_silver, black_knight, black_lance, x,
        x, x, x, x, x, x, x, x, x, x, x,
    };
#undef _
#undef x

    /**
     * @breif ��
     */
    class board_t
    {
    public:
        /**
         * @breif �Ղ��\�z����B
         */
        inline board_t();

        inline colored_piece_t & operator [](size_t i) noexcept;
        inline const colored_piece_t & operator [](size_t i) const noexcept;

        /**
         * @breif ���W���ՊO�����肷��B
         * @param position ���W
         * @return �ՊO�̏ꍇtrue
         */
        inline static bool out(position_t position) noexcept;

        /**
         * @breif �Ղ�W���o�͂ɏo�͂���B
         */
        inline void print() const;

        /**
         * @breif �Ղ���S�Ă̋����菜���B
         */
        inline void clear();

        /**
         * @breif �Ղ�SFEN�\�L�@�ɏ�������������ɕϊ�����B
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

    inline colored_piece_t & board_t::operator [](size_t i) noexcept
    {
        return data[i];
    }

    inline const colored_piece_t & board_t::operator [](size_t i) const noexcept
    {
        return data[i];
    }

    inline bool board_t::out(position_t position) noexcept
    {
        return position < position_begin || position >= position_end || clear_board[position].value() == out_of_range.value();
    }

    inline void board_t::print() const
    {
        std::cout << "  �X �W �V �U �T �S �R �Q �P" << std::endl;
        std::cout << "+---------------------------+" << std::endl;
        for (position_t rank = 0; rank < rank_size; ++rank)
        {
            std::cout << "|";
            for (position_t file = 0; file < file_size; ++file)
            {
                const colored_piece_t piece = data[file_rank_to_position(file, rank)];
                std::cout << ((!piece.empty() && piece.to_color() == white) ? "v" : " ") << piece.to_string();
            }
            std::cout << "| " << rank_to_string(rank) << std::endl;
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
     * @breif ����
     */
    class kiki_t
    {
    public:
        position_t position;      // �����Ă����̍��W
        position_t offset;   // ��������Ă����̍��W����Ƃ��闘���̑��΍��W
        bool aigoma;    // ����\��
    };

    class aigoma_info_t
        : public std::unordered_map<position_t, std::vector<position_t>>
    {
    public:
        using std::unordered_map<position_t, std::vector<position_t>>::unordered_map;

        inline void print() const
        {
            for (const auto & [position, candidates] : *this)
                std::cout << "����F" << position_to_string(position) << std::endl;
        }
    };

    /**
     * @breif �Q�Ƃ���钼�O�ɒx���]������@�\��񋟂���B
     */
    template<typename T>
    class lazy_evaluated_t
    {
    public:
        using value_type = T;
        using function_type = std::function<void(value_type &)>;

        /**
         * @breif �Q�Ƃ���钼�O�ɒx���]������@�\��񋟂���B
         * @param evaluator �x���]������֐�
         */
        inline lazy_evaluated_t(const function_type & evaluator)
            : m_value{}
            , m_valid{ false }
            , m_evaluator{ evaluator }
        {
        }

        /**
         * @breif �Q�Ƃ���钼�O�ɒx���]������@�\��񋟂���B
         * @return �x���]�����ꂽ�l
         */
        inline value_type & operator *()
        {
            evaluate();
            return m_value;
        }

        /**
         * @breif �Q�Ƃ���钼�O�ɒx���]������@�\��񋟂���B
         * @return �x���]�����ꂽ�l
         */
        inline const value_type & operator *() const
        {
            evaluate();
            return m_value;
        }

        /**
         * @breif �Q�Ƃ���钼�O�ɒx���]������@�\��񋟂���B
         * @return �x���]�����ꂽ�l
         */
        inline value_type * operator ->()
        {
            evaluate();
            return &m_value;
        }

        /**
         * @breif �Q�Ƃ���钼�O�ɒx���]������@�\��񋟂���B
         * @return �x���]�����ꂽ�l
         */
        inline const value_type * operator ->() const
        {
            evaluate();
            return &m_value;
        }

        /**
         * @breif �ĕ]����v������B
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
         * @breif �X�^�b�N�ŊǗ������L���b�V�����\�z����B
         */
        inline stack_set_t()
        {
        }

        inline stack_set_t(const stack_set_t &) = default;
        inline stack_set_t(stack_set_t &&) = default;
        inline stack_set_t & operator =(const stack_set_t &) = default;
        inline stack_set_t & operator =(stack_set_t &&) = default;

        /**
         * @breif �L���b�V����j������B
         */
        inline void clear()
        {
            stack.clear();
            uset.clear();
        }

        /**
         * @breif �l���o�^����Ă��邩���肷��B
         * @param �l
         * @retval true �l���o�^����Ă���
         * @retval false �l���o�^����Ă��Ȃ�
         */
        inline bool contains(value_type value) const
        {
            return uset.find(value) != uset.end();
        }

        /**
         * @breif �l��o�^����B
         * @param value �l
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
     * @breif �ǖʂ̒ǉ����
     * @details ��Ԃ̍��@�����������ߒ��Ŏ�Ԃɂ������Ă��鉤�肪�K�v�ɂȂ邽�߁A�ʂɃX�^�b�N�\����ێ�����B
     */
    class additional_info_t
    {
    public:
        std::vector<std::vector<kiki_t>> check_list_stack;  // ��Ԃɂ������Ă��鉤��
        std::vector<hash_t> hash_stack;                     // �ǖʂ̃n�b�V���l
        position_t king_position_list[color_t::size()]{};   // ���̍��W
        stack_cache_t previously_done_moves;                // ���o�̍��@��
    };

    /**
     * @breif �ǖ�
     */
    class kyokumen_t
    {
    public:
        /**
         * @breif �ǖʂ��\�z����B
         */
        inline kyokumen_t();

        /**
         * @breif position �R�}���h�Ŏw�肳�ꂽ�����񂩂�ǖʂ��\�z����B
         */
        inline kyokumen_t(std::string_view position);

        /**
         * @breif ��ړ�����ꍇ�ɐ��肪�\�����肷��B
         * @param piece ��
         * @param source �ړ����̍��W
         * @param destination �ړ���̍��W
         * @return ���肪�\�̏ꍇ(����ɐ����Ă���ꍇ�A���false)
         */
        inline static bool promotable(colored_piece_t piece, position_t source, position_t destination);

        /**
         * @breif ��ړ�����ꍇ�ɐ��肪�K�{�����肷��B
         * @param piece ��
         * @param destination �ړ���̍��W
         * @return ���肪�K�{�̏ꍇ(����ɐ����Ă���ꍇ�A���false)
         */
        inline static bool must_promote(colored_piece_t piece, position_t destination);

        /**
         * @breif �ړ����̍��W����ړ��\�̈ړ���𔽕��I�Ɍ�������B
         * @param result �ړ���̍��W�̏o�̓C�e���[�^
         * @param source �ړ����̍��W
         * @param offset �ړ���̑��΍��W
         */
        template<typename OutputIterator>
        inline void search_far_destination(OutputIterator result, position_t source, position_t offset) const;

        /**
         * @breif �ړ����̍��W����ړ��\�̈ړ����񔽕��I�Ɍ�������B
         * @param result �ړ���̍��W�̏o�̓C�e���[�^
         * @param source �ړ����̍��W
         * @param offset �ړ���̑��΍��W
         */
        template<typename OutputIterator>
        inline void search_near_destination(OutputIterator result, position_t source, position_t offset) const;

        /**
         * @breif �ړ����̍��W����ړ��\�̈ړ������������B
         * @param result �ړ���̍��W�̏o�̓C�e���[�^
         * @param source �ړ����̍��W
         * @param color �ǂ���̎�Ԃ̈ړ���
         */
        template<typename OutputIterator>
        inline void search_destination(OutputIterator result, position_t source, color_t color) const;

        /**
         * @breif ��������ړ���̍��W�ɑł��Ƃ��ł��邩���肷��B���A���A�j�Ɍ���false��Ԃ��\��������B
         * @param piece ������
         * @param destination �ړ���̍��W
         * @return �u�����Ƃ��ł���ꍇ true
         */
        inline bool puttable(captured_piece_t piece, position_t destination) const;

        /**
         * @breif �ړ����̍��W����������B
         * @param result �o�̓C�e���[�^
         * @param color �ǂ���̎�Ԃ̈ړ���
         */
        template<typename OutputIterator>
        inline void search_source(OutputIterator result, color_t color) const;

        /**
         * @breif ���Wposition���瑊�΍��Woffset�����ɑ������ŏ��ɋ�������W��Ԃ��B
         * @param position �������J�n������W
         * @param offset �������鑊�΍��W
         * @return �ŏ��ɋ�������W(�������Ȃ��ꍇ npos )
         */
        inline position_t search(position_t position, position_t offset) const;

        /**
         * @breif ���Wposition�𗘂��Ă����邢�͕R��t���Ă�������������B
         * @param result �����̏o�̓C�e���[�^
         * @param position ���W
         * @param offset �����̑��΍��W
         * @param first ������̓��̓C�e���[�^(begin)
         * @param last ������̓��̓C�e���[�^(end)
         * @param is_collected ����������̎�Ԃɑ΂��ďo�̓C�e���[�^�ɏo�͂��邩���肷�鏖�q�֐�(bool(bool))
         * @param transform (position, offset, aigoma) ���o�̓C�e���[�^�ɏo�͂���ϐ��ɕϊ�����֐�
         */
        template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
        inline void search_piece_near(OutputIterator result, position_t position, position_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const;

        /**
         * @breif ���Wposition�𗘂��Ă����邢�͕R��t���Ă�������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param position ���W
         * @param offset �����̑��΍��W
         * @param first ������̓��̓C�e���[�^(begin)
         * @param last ������̓��̓C�e���[�^(end)
         * @param is_collected ����������̎�Ԃɑ΂��ďo�̓C�e���[�^�ɏo�͂��邩���肷�鏖�q�֐�(bool(bool))
         * @param transform (position, offset, aigoma) ���o�̓C�e���[�^�ɏo�͂���ϐ��ɕϊ�����֐�
         * @sa search_kiki_far
         */
        template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
        inline void search_piece_far(OutputIterator result, position_t position, position_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform) const;

        /**
         * @breif ���Wposition�ɗ����Ă����邢�͕R��t���Ă�������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param position ���W
         * @param color ��ア����̎��_��
         * @param is_collected ����������̎�Ԃɑ΂��ďo�̓C�e���[�^�ɏo�͂��邩���肷�鏖�q�֐�(bool(bool))
         * @param transform (position, offset, aigoma) ���o�̓C�e���[�^�ɏo�͂���ϐ��ɕϊ�����֐�
         */
        template<typename OutputIterator, typename IsCollected, typename Transform>
        inline void search_piece(OutputIterator result, position_t position, color_t color, IsCollected is_collected, Transform transform) const;

        /**
         * @breif ���Wposition�ɕR��t���Ă�������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param position ���W
         * @param color ��ア����̎��_��
         */
        template<typename OutputIterator>
        inline void search_himo(OutputIterator result, position_t position, color_t color) const;

        /**
         * @breif ���Wposition�ɗ����Ă�������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param position ���W
         * @param color ��ア����̎��_��
         */
        template<typename OutputIterator>
        inline void search_kiki(OutputIterator result, position_t position, color_t color) const;

        /**
         * @breif ���Wposition�ɗ����Ă����邢�͕R��t���Ă�������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param position ���W
         * @param color ��ア����̎��_��
         */
        template<typename OutputIterator>
        inline void search_kiki_or_himo(OutputIterator result, position_t position, color_t color) const;

        /**
         * @breif �������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param color ��ア����̎��_��
         */
        template<typename OutputIterator>
        inline void search_check(OutputIterator result, color_t color) const;

        /**
         * @breif �������������B
         * @param color ��ア����̎��_��
         * @return ����
         */
        std::vector<kiki_t> search_check(color_t color) const;

        /**
         * @breif �ǉ�����push����B
         */
        inline void push_additional_info();

        /**
         * @breif �ǉ�����push����B
         * @param hash �n�b�V���l
         */
        inline void push_additional_info(hash_t hash);

        /**
         * @breif �ǉ�����pop����B
         */
        inline void pop_additional_info();

        /**
         * @breif �ǉ�����clear����B
         */
        inline void clear_additional_info();

        /**
         * @breif ���̍��W���X�V����B
         */
        inline void update_king_position_list();

        /**
         * @breif �������������B
         * @param aigoma_info ����̏o�͐�
         * @param color ��ア����̎��_��
         */
        inline void search_aigoma(aigoma_info_t & aigoma_info, color_t color) const;

        /**
         * @breif �������������B
         * @param color ��ア����̎��_��
         * @return ����̏��
         */
        inline aigoma_info_t search_aigoma(color_t color) const;

        /**
         * @breif �ړ����ƈړ���̍��W���獇�@�����������B
         * @param result ���@��̏o�̓C�e���[�^
         * @param source �ړ����̍��W
         * @param destination �ړ���̍��W
         */
        template<typename OutputIterator>
        inline void search_moves_from_positions(OutputIterator result, position_t source, position_t destination) const;

        /**
         * @breif ���@��̂���������O���Ȃ������������B
         * @param result ���@��̏o�̓C�e���[�^
         * @details ���肳��Ă��Ȃ��ꍇ�A���̊֐��ɂ�萶��������̏W���͍��@��S�̂Ɗ��S�Ɉ�v����B
         */
        template<typename OutputIterator>
        inline void search_moves_nonevasions(OutputIterator result) const;

        /**
         * @breif ������O����̂��������ړ���������������B
         * @param result ���@��̏o�̓C�e���[�^
         */
        template<typename OutputIterator>
        inline void search_moves_evasions_king_move(OutputIterator result) const;

        /**
         * @breif ������O����̂������������������B
         * @param result ���@��̏o�̓C�e���[�^
         */
        template<typename OutputIterator>
        inline void search_moves_evasions_aigoma(OutputIterator result) const;

        /**
         * @breif ���@��̂���������O�������������B
         * @param result ���@��̏o�̓C�e���[�^
         */
        template<typename OutputIterator>
        inline void search_moves_evasions(OutputIterator result) const;

        /**
         * @breif ������O���Ȃ���̂�����𓮂��������������B
         * @param result ���@��̏o�̓C�e���[�^
         */
        template<typename OutputIterator>
        inline void search_moves_moves(OutputIterator result) const;

        /**
         * @breif ������O���Ȃ���̂����������ł����������B
         * @param result ���@��̏o�̓C�e���[�^
         */
        template<typename OutputIterator>
        inline void search_moves_puts(OutputIterator result) const;

        /**
         * @breif ���@�����������B
         * @param result ���@��̏o�̓C�e���[�^
         */
        template<typename OutputIterator>
        inline void search_moves(OutputIterator result) const;

        /**
         * @breif ���@�����������B
         * @param result ���@��̏o�̓C�e���[�^
         * @details anti_repetition_of_moves == true �̏ꍇ���̊֐��� strict_search_moves �Ƀ��_�C���N�g�����B
         */
        inline moves_t search_moves() const;

        /**
         * @breif �������܂ތ����łȂ����@�����������B
         * @param result ���@��̏o�̓C�e���[�^
         */
        inline moves_t nonstrict_search_moves() const;

        /**
         * @breif �������܂܂Ȃ������ȍ��@�����������B
         * @param result ���@��̏o�̓C�e���[�^
         */
        inline moves_t strict_search_moves() const;

        /**
         * @breif �ǖʂ̃n�b�V���l���v�Z����B
         * @return �ǖʂ̃n�b�V���l
         */
        inline hash_t make_hash() const;

        /**
         * @breif �ǖʂ̃n�b�V���l�ƍ��@�肩��A���@������{������̋ǖʂ̃n�b�V���l���v�Z����B
         * @param hash ���@������{����O�̋ǖʂ̃n�b�V���l
         * @param move ���{���鍇�@��
         * @return ���@������{������̋ǖʂ̃n�b�V���l
         * @details ���@��ɂ�蔭�����鍷���Ɋ�Â��v�Z���邽�� make_hash() ����r�I�����ɏ��������B
         *          ���̊֐��͍��@������{������O�ɌĂяo�����K�v������B
         */
        inline hash_t make_hash(hash_t hash, const move_t & move) const;

        /**
         * @breif ���@���W���o�͂ɏo�͂���B
         * @param move ���@��
         * @param color ���̍��@�肩
         */
        inline void print_move(const move_t & move, color_t color) const;

        /**
         * @breif ���@���W���o�͂ɏo�͂���B
         * @param first ���@��̓��̓C�e���[�^��begin
         * @param last ���@��̓��̓C�e���[�^��end
         */
        template<typename InputIterator>
        inline void print_move(InputIterator first, InputIterator last) const;

        /**
         * @breif ���@���W���o�͂ɏo�͂���B
         */
        inline void print_move() const;

        /**
         * @breif �����W���o�͂ɏo�͂���B
         */
        inline void print_check() const;

        /**
         * @breif �ǖʂ�W���o�͂ɏo�͂���B
         */
        inline void print() const;

        inline void print_kifu() const;

        inline void print_recent_kifu(std::size_t size) const;

        /**
         * @breif �ǖʂ̃n�b�V���l��Ԃ��B
         * @return �ǖʂ̃n�b�V���l
         */
        inline hash_t hash() const;

        inline void validate_board_out();

        /**
         * @breif ���@������s����B
         * @param move ���@��
         */
        inline void do_move(const move_t & move);

        /**
         * @breif ���@������s����O�ɖ߂��B
         * @param move ���@��
         */
        inline void undo_move(const move_t & move);

        /**
         * @breif ��Ԃ��擾����B
         * @return ���
         */
        inline color_t color() const;

        /**
         * @breif �w�肳�ꂽ�萔�ŕ��򂷂�ǖʂ̐��𐔂���B
         * @param depth �萔
         * @return �ǖʂ̐�
         */
        inline search_count_t count_node(move_count_t depth) const;

        /**
         * @breif �ǖʂ�SFEN�\�L�@�ɏ�������������ɕϊ�����B
         * @return SFEN�\�L�@�ɏ�������������
         */
        inline std::string sfen_string() const;

        /**
         * @breif ���@��̏W������������폜����B
         */
        inline void remove_repetition_of_moves(moves_t & moves) const noexcept;

        board_t board;                                              // ��
        captured_pieces_t captured_pieces_list[color_t::size()];    // ������
        move_count_t move_count = 0;                                // �萔
        std::vector<move_t> kifu;                                   // ����
        additional_info_t additional_info;                          // �ǉ����
        bool anti_repetition_of_moves = true;                       // �ϐ����
    };

    /**
     * @breif �R�s�[�R���X�g���N�g����Ă���f�X�g���N�g�����܂łɋǖʂ��ύX����Ă��Ȃ����Ƃ����؂���B
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
        for (std::size_t i = 0; i < std::size(data); ++i)
            SHOGIPP_ASSERT(data[i] == kyokumen.board.data[i]);
        for (const color_t color : colors)
            for (piece_value_t piece = pawn_value; piece <= rook_value; ++piece)
                SHOGIPP_ASSERT(captured_pieces_list[color.value()][captured_piece_t{ piece }] == kyokumen.captured_pieces_list[color.value()][captured_piece_t{ piece }]);
    }

    inline kyokumen_t::kyokumen_t()
    {
        update_king_position_list();
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

            // ���
            for (position_t rank = 0; rank < rank_size; ++rank)
            {
                const colored_piece_t current = board[file_rank_to_position(file, rank)];
                if (!current.empty() && noncolored_piece_t{ current } == pawn && color() == current.to_color())
                    return false;
            }

            // �ł����l��
            const position_t position = destination + front * (reverse(color()));
            if (!board_t::out(position) && !board[position].empty() && noncolored_piece_t{ board[position] } == king && board[position].to_color() != color())
            {
                const move_t move{ destination, piece };
                moves_t moves;
                {
                    VALIDATE_kyokumen_ROLLBACK(*this);
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
    inline void kyokumen_t::search_moves_from_positions(OutputIterator result, position_t source, position_t destination) const
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
    inline void kyokumen_t::search_moves_evasions_king_move(OutputIterator result) const
    {
        const position_t source = additional_info.king_position_list[color().value()];
        for (const position_t * ptr = near_move_offsets(king); *ptr; ++ptr)
        {
            const position_t destination = source + *ptr * reverse(color());
            if (!board_t::out(destination)
                && (board[destination].empty() || board[destination].to_color() != color()))
            {
                const move_t move{ source, destination, board[source], board[destination], false };
                std::vector<kiki_t> kiki;
                {
                    VALIDATE_kyokumen_ROLLBACK(*this);
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
    inline void kyokumen_t::search_moves_evasions_aigoma(OutputIterator result) const
    {
        const aigoma_info_t aigoma_info = search_aigoma(color());
        const position_t king_pos = additional_info.king_position_list[color().value()];
        
        SHOGIPP_ASSERT(move_count < additional_info.check_list_stack.size());
        const auto & check_list = additional_info.check_list_stack[move_count];
        if (check_list.size() == 1)
        {
            if (check_list.front().aigoma)
            {
                const position_t offset = check_list.front().offset;
                for (position_t destination = king_pos + offset; !board_t::out(destination) && board[destination].empty(); destination += offset)
                {
                    // ����ړ������鍇��
                    std::vector<kiki_t> kiki_list;
                    search_kiki(std::back_inserter(kiki_list), destination, !color());
                    for (const kiki_t & kiki : kiki_list)
                    {
                        // ���ō���͂ł��Ȃ��B
                        if (noncolored_piece_t{ board[kiki.position] } != king)
                        {
                            // ���ɍ���Ƃ��Ďg���Ă����͈ړ��ł��Ȃ��B
                            const auto aigoma_iter = aigoma_info.find(kiki.position);
                            const bool is_aigoma = aigoma_iter != aigoma_info.end();
                            if (!is_aigoma)
                                search_moves_from_positions(result, kiki.position, destination);
                        }
                    }

                    // ���ł���
                    for (piece_value_t piece = pawn_value; piece <= rook_value; ++piece)
                        if (captured_pieces_list[color().value()][piece])
                            if (puttable(piece, destination))
                                *result++ = { destination, piece };
                }
            }

            // ���肵�Ă������������������B
            const position_t destination = check_list.front().position;
            std::vector<kiki_t> kiki_list;
            search_kiki(std::back_inserter(kiki_list), destination, !color());
            for (const kiki_t & kiki : kiki_list)
            {
                // ���𓮂�����͊��Ɍ����ς�
                if (noncolored_piece_t{ board[kiki.position] } != king)
                {
                    // ���ɍ���Ƃ��Ďg���Ă����͈ړ��ł��Ȃ��B
                    const auto aigoma_iter = aigoma_info.find(kiki.position);
                    const bool is_aigoma = aigoma_iter != aigoma_info.end();
                    if (!is_aigoma)
                        search_moves_from_positions(result, kiki.position, destination);
                }
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_moves_evasions(OutputIterator result) const
    {
        search_moves_evasions_king_move(result);
        search_moves_evasions_aigoma(result);
    }

    /**
     * @breif ������O���Ȃ���̂�����𓮂��������������B
     * @param result ���@��̏o�̓C�e���[�^
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

                // ����͗����͈̔͂ɂ����ړ��ł��Ȃ��B
                if (is_aigoma)
                {
                    const std::vector<position_t> & candidates = aigoma_iter->second;
                    if (std::find(candidates.begin(), candidates.end(), destination) == candidates.end())
                        continue;
                }

                // �����Ă���ꏊ�ɉ����ړ������Ă͂Ȃ�Ȃ�
                if (noncolored_piece_t{ board[source] } == king)
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
            search_moves_nonevasions(result);
        else
            search_moves_evasions(result);
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

        // �Տ�̋�̃n�b�V���l��XOR���Z
        for (position_t position = position_begin; position < position_end; ++position)
            if (!board_t::out(position))
                if (const colored_piece_t piece = board[position]; !piece.empty())
                    hash ^= hash_table.piece_hash(piece, position);

        // ������̃n�b�V���l��XOR���Z
        for (const color_t color : colors)
            for (piece_value_t piece = pawn.value(); piece <= rook.value(); ++piece)
                hash ^= hash_table.captured_piece_hash(captured_piece_t{ piece }, captured_pieces_list[color.value()][captured_piece_t{ piece }], color);

        // ��Ԃ̃n�b�V���l��XOR���Z
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

    inline void kyokumen_t::print_move(const move_t & move, color_t color) const
    {
        std::cout << (color == black ? "��" : "��");
        if (move.put())
        {
            std::cout << position_to_string(move.destination()) << move.captured_piece().to_string() << "��";
        }
        else
        {
            const char * promotion_string;
            if (promotable(move.source_piece(), move.source(), move.destination()))
            {
                if (move.promote())
                    promotion_string = "��";
                else
                    promotion_string = "�s��";
            }
            else
                promotion_string = "";
            std::cout
                << position_to_string(move.destination()) << noncolored_piece_t{ move.source_piece() }.to_string() << promotion_string
                << "�i" << position_to_string(move.source()) << "�j";
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
        kyokumen_t temp = *this;
        const moves_t moves = temp.strict_search_moves();
        print_move(moves.begin(), moves.end());
    }

    inline void kyokumen_t::print_check() const
    {
        SHOGIPP_ASSERT(move_count < additional_info.check_list_stack.size());
        auto & check_list = additional_info.check_list_stack[move_count];
        if (!check_list.empty())
        {
            std::cout << "����F";
            for (std::size_t i = 0; i < check_list.size(); ++i)
            {
                const kiki_t & kiki = check_list[i];
                if (i > 0)
                    std::cout << "�@";
                std::cout << position_to_string(kiki.position) << noncolored_piece_t{ board[kiki.position] }.to_string() << std::endl;
            }
        }
    }

    inline void kyokumen_t::print() const
    {
        std::cout << "��莝����F";
        captured_pieces_list[white.value()].print();
        board.print();
        std::cout << "��莝����F";
        captured_pieces_list[black.value()].print();
    }

    inline void kyokumen_t::print_kifu() const
    {
        for (move_count_t i = 0; i < static_cast<move_count_t>(kifu.size()); ++i)
        {
            const move_count_t diff = static_cast<move_count_t>(kifu.size() - i);
            print_move(kifu[i], diff % color_t::size() == 0 ? color() : !color());
            std::cout << std::endl;
        }
    }

    inline void kyokumen_t::print_recent_kifu(std::size_t size) const
    {
        size = std::min(size, kifu.size());
        for (move_count_t i = 0; i < size; ++i)
        {
            if (i > 0)
                std::cout << "�@";
            std::size_t index = kifu.size() - size + i;
            const move_count_t diff = static_cast<move_count_t>(kifu.size() - index);
            print_move(kifu[index], diff % color_t::size() == 0 ? color() : !color());
        }
        std::cout << std::flush;
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
            VALIDATE_kyokumen_ROLLBACK(*this);
            const_cast<kyokumen_t &>(*this).do_move(move);
            count += count_node(depth - 1);
            const_cast<kyokumen_t &>(*this).undo_move(move);
        }
        return count;
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
         * @breif LRU�ŊǗ������L���b�V�����\�z����B
         * @param capacity �ő�v�f��
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
         * @breif �L���b�V����j������B
         */
        inline void clear()
        {
            list.clear();
            umap.clear();
        }

        /**
         * @breif �L�[�ƑΉ�����l���擾����B
         * @param key �L�[
         * @return �L�[�ƑΉ�����l
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
         * @breif �L�[�ƒl��o�^����B
         * @param key �L�[
         * @param value �l
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
     * @breif �]���֐��I�u�W�F�N�g���Ăяo���ꂽ������\������B
     */
    class context_t
    {
    public:
        constexpr inline context_t() noexcept = default;

        constexpr inline context_t(depth_t max_depth, depth_t max_selective_depth) noexcept
            : m_max_depth{ max_depth }
            , m_max_selective_depth{ max_selective_depth }
        {
        }

        constexpr inline context_t(const context_t & context) noexcept = default;
        constexpr inline context_t(context_t && context) noexcept = default;
        constexpr inline context_t & operator =(const context_t & context) noexcept = default;
        constexpr inline context_t & operator =(context_t && context) noexcept = default;

        inline depth_t max_depth() const noexcept
        {
            return m_max_depth;
        }

        inline depth_t max_selective_depth() const noexcept
        {
            return m_max_selective_depth;
        }

    private:
        depth_t m_max_depth{};
        depth_t m_max_selective_depth{};
    };

    /**
     * @breif �]���֐��I�u�W�F�N�g�̃C���^�[�t�F�[�X
     */
    class abstract_evaluator_t
    {
    public:
        virtual ~abstract_evaluator_t() {}

        /**
         * @breif �ǖʂɑ΂��č��@���I������B
         * @param kyokumen �ǖ�
         * @return �I�����ꂽ���@��
         */
        virtual move_t best_move(kyokumen_t & kyokumen, const context_t & context) = 0;

        /**
         * @breif �]���֐��I�u�W�F�N�g�̖��O��Ԃ��B
         * @return �]���֐��I�u�W�F�N�g�̖��O
         */
        virtual std::string name() const = 0;
    };

    /**
     * @breif �W�����͂ɂ�荇�@���I������]���֐��I�u�W�F�N�g
     */
    class command_line_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        move_t best_move(kyokumen_t & kyokumen, const context_t & context) override
        {
            bool selected = false;

            unsigned int id;
            moves_t moves = kyokumen.search_moves();
            
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
                    char option = *ptr;
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
                        char quotation = *qiter;
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
     * @breif ��Ɖ��l�̘A�z�z�񂩂�ǖʂ̓_�����v�Z����B
     * @param kyokumen �ǖ�
     * @param map []���Z�q�ɂ���牿�l��A�z����I�u�W�F�N�g
     * @return �ǖʂ̓_��
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
     * @breif ���@��𓾓_�ɂ����ёւ���B
     * @param first evaluated_moves �̐擪���w�������_���A�N�Z�X�C�e���[�^
     * @param last evaluated_moves �̖������w�������_���A�N�Z�X�C�e���[�^
     * @param color ��肩��肩
     * @details �]���l�̕�������Ԃɂ��ύX����悤�ɂ������߁A���݂��̊֐����g�p����\��͂Ȃ��B
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
     * @breif ���@����敪�ɂ����ёւ���B
     * @param first evaluated_moves �̐擪���w�������_���A�N�Z�X�C�e���[�^
     * @param last evaluated_moves �̖������w�������_���A�N�Z�X�C�e���[�^
     * @details ���肪���������A���肪�������Ȃ���A�ł�̏��ɕ��ёւ���B
     */
    template<typename RandomAccessIterator>
    void sort_moves_by_category(RandomAccessIterator first, RandomAccessIterator last)
    {
        std::sort(first, last, [](const move_t & a, const move_t & b) -> bool { return to_category(a) >= to_category(b); });
    }

    /**
     * @breif ���@��𓾓_�ɂ����ёւ���B
     * @param first scored_te �̐擪���w�������_���A�N�Z�X�C�e���[�^
     * @param last scored_te �̖������w�������_���A�N�Z�X�C�e���[�^
     */
    template<typename RandomAccessIterator>
    void sort_moves_by_evaluation_value(RandomAccessIterator first, RandomAccessIterator last)
    {
        std::sort(first, last, [](const evaluated_moves & a, const evaluated_moves & b) -> bool { return a.second > b.second; });
    }

    /**
     * @breif USI�v���g�R���ŃN���C�A���g����T�[�o�[�ɑ��M��������\������B
     * @details ���̃N���X�̃����o�֐��̓X���b�h�Z�[�t�Ɏ��s�����B
     *          ���̃N���X�̃����o�֐����Q�Ƃ���ꍇ�A mutex �����o�ϐ��𗘗p���Ĕr�����䂷�邱�ƁB
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
        search_count_t cache_rookt_count{};
        state_t state = state_t::not_ready;
        milli_second_time_t limit_time{};
        std::map<std::string, std::string> options;
        bool ponder = false;

        mutable std::recursive_mutex mutex;

        /**
         * @breif begin �ƌ��ݎ��Ԃ̍������~���b�P�ʂŕԂ��B
         * @return begin �ƌ��ݎ��Ԃ̍���
         */
        inline milli_second_time_t time() const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            const std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            return static_cast<milli_second_time_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
        }

        /**
         * @breif �T���ǖʐ��S�̂̂����L���b�V����K�p�����T���ǖʐ��̐�߂銄����番���ŕԂ��B
         * @return �T���ǖʐ��S�̂̂����L���b�V����K�p�����T���ǖʐ��̐�߂銄��
         */
        inline search_count_t hashfull() const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            if (nodes == 0)
                return 0;
            return cache_rookt_count * 1000 / nodes;
        }

        /**
         * @breif 1�b������̒T���ǖʐ���Ԃ��B
         * @return 1�b������̒T���ǖʐ�
         */
        inline search_count_t nps() const
        {
            std::lock_guard<decltype(mutex)> lock{ mutex };
            const milli_second_time_t milli_second_time = time();
            if (milli_second_time == 0)
                return 0;
            return nodes * 1000 / milli_second_time;
        }

        /**
         * @breif �����I�ȏo�͂��s���B
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
         * @breif ����I�ȏo�͂��s���B
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
         * @breif ���O�Ɋ֘A�t�����I�v�V�����̒l�𕶎���Ƃ��Ď擾����B
         * @param name �I�v�V�����̖��O
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
         * @breif ���O�Ɋ֘A�t�����I�v�V�����̒l�� Result �^�ɕϊ����Ď擾����B
         * @tparam Result �ϊ���̌^
         * @param name �I�v�V�����̖��O
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
         * @breif USI_Hash �I�v�V�����̒l�ɏ��������ő�T�C�Y������ cache_t ���\�z����B
         * @param usi_info usi_info_t �̃|�C���^
         * @return USI_Hash �I�v�V�����̒l�ɏ��������ő�T�C�Y������ cache_t
         * @details usi_info �� nullptr �ł���ꍇ�A���邢�� USI_Hash �I�v�V���� ���w�肳��Ă��Ȃ��ꍇ�A
         *          ���̊֐��͖������̍ő�T�C�Y������ cache_t ���\�z����B
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
         * @breif state �� state_t::terminated ��ݒ肵�Aponder == false �ł���� bestmove �R�}���h���o�͂���B
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
     * @breif �ǖʂɑ΂��ĕ]���l��Ԃ��@�\��񋟂���B
     */
    class evaluatable_t
    {
    public:
        virtual ~evaluatable_t() {}

        /**
         * @breif �ǖʂɑ΂��ĕ]���l��Ԃ��B
         * @param kyokumen �ǖ�
         * @return �ǖʂ̕]���l
         */
        virtual evaluation_value_t evaluate(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif negamax �ō��@���I������]���֐��I�u�W�F�N�g�̒��ۃN���X
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
            std::optional<move_t> & candidate_move,
            const context_t & context
        );

        move_t best_move(kyokumen_t & kyokumen, const context_t & context) override;

        std::shared_ptr<usi_info_t> usi_info;
    };

    evaluation_value_t negamax_evaluator_t::negamax(
        kyokumen_t & kyokumen,
        depth_t depth,
        cache_t & cache,
        std::optional<move_t> & candidate_move,
        const context_t & context
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

        if (depth >= context.max_depth())
        {
            ++details::timer.search_count();
            const std::optional<evaluation_value_t> cached_evaluation_value = cache.get(kyokumen.hash());
            if (cached_evaluation_value)
            {
                if (usi_info)
                {
                    std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                    ++usi_info->cache_rookt_count;
                }
                return *cached_evaluation_value;
            }
            const evaluation_value_t evaluation_value = evaluate(kyokumen) * reverse(kyokumen.color());
            cache.push(kyokumen.hash(), evaluation_value);
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
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->currmove = move;
                usi_info->ondemand_print();
            }

            std::optional<move_t> nested_candidate_move;
            evaluation_value_t evaluation_value;
            {
                VALIDATE_kyokumen_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                evaluation_value = -negamax(kyokumen, depth + 1, cache, nested_candidate_move, context);
                kyokumen.undo_move(move);
            }
            *inserter++ = { &move, evaluation_value };

            if (usi_info && depth == 0 && evaluation_value > max_evaluation_value)
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->best_move = move;
                usi_info->cp = evaluation_value;
            }
            max_evaluation_value = evaluation_value;
        }

        SHOGIPP_ASSERT(!moves.empty());
        sort_moves_by_evaluation_value(evaluated_moves.begin(), evaluated_moves.end());
        candidate_move = *evaluated_moves.front().first;
        return evaluated_moves.front().second;
    }

    move_t negamax_evaluator_t::best_move(kyokumen_t & kyokumen, const context_t & context)
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
            evaluation_value = negamax(kyokumen, 0, cache, candidate_move, context);
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
     * @breif alphabeta �ō��@���I������]���֐��I�u�W�F�N�g�̒��ۃN���X
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
            std::optional<move_t> & candidate_move,
            const context_t & context
        );

        move_t best_move(kyokumen_t & kyokumen, const context_t & context) override;

        std::shared_ptr<usi_info_t> usi_info;
    };

    evaluation_value_t alphabeta_evaluator_t::alphabeta(
        kyokumen_t & kyokumen,
        depth_t depth,
        evaluation_value_t alpha,
        evaluation_value_t beta,
        cache_t & cache,
        std::optional<move_t> & candidate_move,
        const context_t & context
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
        
        if (depth >= context.max_depth())
        {
            ++details::timer.search_count();
            const std::optional<evaluation_value_t> cached_evaluation_value = cache.get(kyokumen.hash());
            if (cached_evaluation_value)
            {
                if (usi_info)
                {
                    std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                    ++usi_info->cache_rookt_count;
                }
                return *cached_evaluation_value;
            }
            const evaluation_value_t evaluation_value = evaluate(kyokumen) * reverse(kyokumen.color());
            cache.push(kyokumen.hash(), evaluation_value);
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
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->currmove = move;
                usi_info->ondemand_print();
            }

            std::optional<move_t> nested_candidate_move;
            evaluation_value_t evaluation_value;
            {
                VALIDATE_kyokumen_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                evaluation_value = -alphabeta(kyokumen, depth + 1, -beta, -alpha, cache, nested_candidate_move, context);
                kyokumen.undo_move(move);
            }
            *inserter++ = { &move, evaluation_value };

            if (usi_info && depth == 0 && evaluation_value > max_evaluation_value)
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->best_move = move;
                usi_info->cp = evaluation_value;
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

    move_t alphabeta_evaluator_t::best_move(kyokumen_t & kyokumen, const context_t & context)
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
            evaluation_value = alphabeta(kyokumen, 0, -std::numeric_limits<evaluation_value_t>::max(), std::numeric_limits<evaluation_value_t>::max(), cache, candidate_move, context);
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
     * @breif alphabeta �ō��@���I������]���֐��I�u�W�F�N�g�̒��ۃN���X
     * @details �O����肪�������Ă����ꍇ�A�T������������B
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
            position_t previous_destination,
            const context_t & context
        );

        move_t best_move(kyokumen_t & kyokumen, const context_t & context) override;

        std::shared_ptr<usi_info_t> usi_info;
    };

    evaluation_value_t extendable_alphabeta_evaluator_t::extendable_alphabeta(
        kyokumen_t & kyokumen,
        depth_t depth,
        evaluation_value_t alpha,
        evaluation_value_t beta,
        cache_t & cache,
        std::optional<move_t> & candidate_move,
        position_t previous_destination,
        const context_t & context
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
        
        if (depth >= context.max_depth())
        {
            // �O����肪�������Ă����ꍇ�A�T������������B
            if (depth >= context.max_selective_depth() && previous_destination != npos)
            {
                std::vector<evaluated_moves> evaluated_moves;
                auto inserter = std::back_inserter(evaluated_moves);
                moves_t moves = kyokumen.search_moves();
                for (const move_t & move : moves)
                {
                    if (!move.put() && move.destination() == previous_destination)
                    {
                        std::optional<move_t> nested_candidate_move;
                        evaluation_value_t evaluation_value;
                        {
                            VALIDATE_kyokumen_ROLLBACK(kyokumen);
                            kyokumen.do_move(move);
                            evaluation_value = -extendable_alphabeta(kyokumen, depth - 1, -beta, -alpha, cache, nested_candidate_move, previous_destination, context);
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
            const std::optional<evaluation_value_t> cached_evaluation_value = cache.get(kyokumen.hash());
            if (cached_evaluation_value)
            {
                if (usi_info)
                {
                    std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                    ++usi_info->cache_rookt_count;
                }
                return *cached_evaluation_value;
            }
            const evaluation_value_t evaluation_value = evaluate(kyokumen) * reverse(kyokumen.color());
            cache.push(kyokumen.hash(), evaluation_value);
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
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->currmove = move;
                usi_info->ondemand_print();
            }

            std::optional<move_t> nested_candidate_move;
            position_t destination = (!move.put() && !move.destination_piece().empty()) ? move.destination() : npos;
            evaluation_value_t evaluation_value;
            {
                VALIDATE_kyokumen_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                evaluation_value = -extendable_alphabeta(kyokumen, depth + 1, -beta, -alpha, cache, nested_candidate_move, destination, context);
                kyokumen.undo_move(move);
            }
            *inserter++ = { &move, evaluation_value };

            if (usi_info && depth == 0 && evaluation_value > max_evaluation_value)
            {
                std::lock_guard<decltype(usi_info->mutex)> lock{ usi_info->mutex };
                usi_info->best_move = move;
                usi_info->cp = evaluation_value;
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

    move_t extendable_alphabeta_evaluator_t::best_move(kyokumen_t & kyokumen, const context_t & context)
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
            evaluation_value = extendable_alphabeta(kyokumen, 0, -std::numeric_limits<evaluation_value_t>::max(), std::numeric_limits<evaluation_value_t>::max(), cache, candidate_move, npos, context);
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
     * @breif �P���ɕ]���l���ł��������Ԃ��]���֐��I�u�W�F�N�g�̒��ۃN���X
     */
    class max_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        /**
         * @breif �ǖʂɑ΂��ĕ]���l���ł������Ȃ鍇�@���I������B
         * @param kyokumen �ǖ�
         * @return �I�����ꂽ���@��
         */
        move_t best_move(kyokumen_t & kyokumen, const context_t & context) override
        {
            moves_t moves = kyokumen.search_moves();

            std::vector<evaluated_moves> scores;
            auto back_inserter = std::back_inserter(scores);
            for (const move_t & move : moves)
            {
                VALIDATE_kyokumen_ROLLBACK(kyokumen);
                kyokumen.do_move(move);
                *back_inserter++ = { &move, evaluate(kyokumen) };
                kyokumen.undo_move(move);
            }

            std::sort(scores.begin(), scores.end(), [](auto & a, auto & b) { return a.second > b.second; });
            return *scores.front().first;
        }

        /**
         * @breif �ǖʂɑ΂��ĕ]���l��Ԃ��B
         * @param kyokumen �ǖ�
         * @return �ǖʂ̕]���l
         */
        virtual evaluation_value_t evaluate(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif �]���֐��I�u�W�F�N�g�̎�����
     */
    class sample_evaluator_t
        : public alphabeta_evaluator_t
    {
    public:
        evaluation_value_t evaluate(kyokumen_t & kyokumen) override
        {
            static const evaluation_value_t map[]
            {
                /* empty           */  0,
                /* pawn            */  1,
                /* lance           */  3,
                /* knight          */  4,
                /* silver          */  5,
                /* gold            */  6,
                /* bishop          */  8,
                /* rook            */ 10,
                /* king            */  0,
                /* promoted_pawn   */  7,
                /* promoted_lance  */  6,
                /* promoted_knight */  6,
                /* promoted_silver */  6,
                /* promoted_bishop */ 10,
                /* promoted_rook   */ 12
            };

            evaluation_value_t evaluation_value = kyokumen_map_evaluation_value(kyokumen, map);
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
            static const evaluation_value_t map[]
            {
                /* empty           */  0,
                /* pawn            */  1,
                /* lance           */  3,
                /* knight          */  4,
                /* silver          */  5,
                /* gold            */  6,
                /* bishop          */  8,
                /* rook            */ 10,
                /* king            */  0,
                /* promoted_pawn   */  7,
                /* promoted_lance  */  6,
                /* promoted_knight */  6,
                /* promoted_silver */  6,
                /* promoted_bishop */ 10,
                /* promoted_rook   */ 12
            };

            evaluation_value_t evaluation_value = 0;
            evaluation_value += kyokumen_map_evaluation_value(kyokumen, map);
            evaluation_value *= 100;
            return evaluation_value;
        }

        std::string name() const override
        {
            return "�Ђ悱";
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
                /* empty           */  0,
                /* pawn            */  1,
                /* lance           */  3,
                /* knight          */  4,
                /* silver          */  5,
                /* gold            */  6,
                /* bishop          */  8,
                /* rook            */ 10,
                /* king            */  0,
                /* promoted_pawn   */  7,
                /* promoted_lance  */  6,
                /* promoted_knight */  6,
                /* promoted_silver */  6,
                /* promoted_bishop */ 10,
                /* promoted_rook   */ 12
            };

            evaluation_value_t evaluation_value = 0;
            evaluation_value += kyokumen_map_evaluation_value(kyokumen, map);
            evaluation_value *= 100;

            return evaluation_value;
        }

        std::string name() const override
        {
            return "�ɂ�Ƃ�";
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
                /* nari_lance */  6,
                /* nari_knight */  6,
                /* nari_silver */  6,
                /* uma      */ 10,
                /* ryu      */ 12
            };

            constexpr evaluation_value_t destination_point = 1;
            constexpr evaluation_value_t kiki_point = -10;
            constexpr evaluation_value_t himo_point = 10;

            evaluation_value_t evaluation_value = 0;
            evaluation_value += kyokumen_map_evaluation_value(kyokumen, map);
            evaluation_value *= 100;

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
            return "�[�ǂ�";
        }
    };

    class chromosome_t
    {
    public:
        unsigned short board_piece[promoted_rook_value - pawn_value]{};
        unsigned short captured_piece_point[captured_size]{};
        short kiki_point{};
        short himo_point{};
        short destination_point{};

        inline void generate() noexcept
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
            std::ifstream in(path, std::ios::in | std::ios::binary);
            char * data = reinterpret_cast<char *>(this);
            in.read(data, sizeof(*this));
        }

        inline void write_file(const std::filesystem::path & path) const
        {
            std::ofstream out(path, std::ios::out | std::ios::binary);
            const char * data = reinterpret_cast<const char *>(this);
            out.write(data, sizeof(*this));
        }

        inline evaluation_value_t evaluate(kyokumen_t & kyokumen) const
        {
            evaluation_value_t evaluation_value = 0;

            for (position_t position = position_begin; position < position_end; ++position)
            {
                const colored_piece_t piece = kyokumen.board[position];
                if (!board_t::out(position) && !piece.empty())
                {
                    const noncolored_piece_t noncolored_piece{ piece };
                    if (noncolored_piece == pawn)
                        evaluation_value += 100 * reverse(piece.to_color());
                    else
                    {
                        const std::size_t index = noncolored_piece.value() - pawn_value - 1;
                        SHOGIPP_ASSERT(index < std::size(board_piece));
                        evaluation_value += board_piece[index] * reverse(piece.to_color());
                    }
                }
            }

            for (const color_t color : colors)
            {
                for (piece_value_t piece = pawn_value; piece <= rook_value; ++piece)
                {
                    const std::size_t index = captured_offsets[piece] + kyokumen.captured_pieces_list[color.value()][piece];
                    SHOGIPP_ASSERT(index < std::size(captured_piece_point));
                    evaluation_value += captured_piece_point[index] * reverse(color);
                }
            }

            for (position_t position = position_begin; position < position_end; ++position)
            {
                if (!board_t::out(position) && !kyokumen.board[position].empty())
                {
                    const color_t color = kyokumen.board[position].to_color();

                    std::vector<kiki_t> kiki_list;
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

        inline chromosome_evaluator_t()
            : m_chromosome{ std::make_shared<chromosome_t>() }
            , m_name{ std::to_string(unique_size_t_hash()) }
            , m_id{}
        {
            m_chromosome->generate();
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

        inline void write_file(const std::filesystem::path & path) const
        {
            m_chromosome->write_file(path);
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
     * @breif �]���֐��I�u�W�F�N�g�̎�����
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
        std::optional<move_t> opt_te;
        std::optional<move_count_t> opt_depth;
    };

    /**
     * @breif �W�����͂���R�}���h��ǂݍ��ށB
     * @param moves ���@��
     * @return �ǂݍ��܂ꂽ�R�}���h
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

                    if (tokens[0] == "eval")
                    {
                        return command_t{ command_t::id_t::eval };
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
     * @breif �΋�
     */
    class taikyoku_t
    {
    public:
        /**
         * @breif �΋ǂ��\�z����B
         * @param a ���̊��m
         * @param b ���̊��m
         */
        inline taikyoku_t(const std::shared_ptr<abstract_kishi_t> & a, const std::shared_ptr<abstract_kishi_t> & b);

        /**
         * @breif �΋ǂ����s����B
         * @retval true �΋ǂ��I�����Ă��Ȃ�
         * @retval false �΋ǂ��I������
         * @details ���̊֐��� true ��Ԃ����ꍇ�A�ēx���̊֐����Ăяo���B
         */
        inline bool procedure();

        /**
         * @breif �΋ǂ�W���o�͂ɏo�͂���B
         */
        inline void print() const;

        /**
         * @breif ��Ԃ̍��@���Ԃ��B
         * @return ���@��
         */
        inline const moves_t & get_moves() const;

        /**
         * @breif ��Ԃ̍��@����X�V����B
         */
        inline void update_moves() const;

        std::shared_ptr<abstract_kishi_t> kishi_list[color_t::size()];
        mutable moves_t moves;
        kyokumen_t kyokumen;
        bool black_win;
    };

    /**
     * @breif ���m
     */
    class abstract_kishi_t
    {
    public:
        virtual ~abstract_kishi_t() {}

        /**
         * @breif �R�}���h��Ԃ��B
         * @param taikyoku �΋�
         * @return �R�}���h
         */
        virtual command_t get_command(taikyoku_t & taikyoku) = 0;

        /**
         * @breif ���m�̖��O��Ԃ��B
         * @return ���m�̖��O
         */
        virtual std::string name() const = 0;
    };

    /**
     * @breif �W�����͂ɂ�萧�䂳�����m
     */
    class stdin_kishi_t
        : public abstract_kishi_t
    {
    public:
        command_t get_command(taikyoku_t & taikyoku) override;
        std::string name() const override;
    };

    /**
     * @breif �]���֐��I�u�W�F�N�g�ɂ�萧�䂳�����m
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

    command_t computer_kishi_t::get_command(taikyoku_t & taikyoku)
    {
        const context_t context{ program_option_max_depth, program_option_max_selective_depth };
        return command_t{ command_t::id_t::move, ptr->best_move(taikyoku.kyokumen, context) };
    }

    std::string computer_kishi_t::name() const
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
        auto & kishi = kishi_list[kyokumen.color().value()];

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
            {
                search_count_t node;
                const milli_second_time_t time = details::test_time_performance([&] { node = kyokumen.count_node(*cmd.opt_depth); }, 1);
                std::cout << "node: " << node << std::endl;
                std::cout << "time[ms]: " << time << std::endl;
                std::cout << "nps[n/s]: " << (node * 1000 / time) << std::endl;
                break;
            }
            case command_t::id_t::hash:
                std::cout << hash_to_string(kyokumen.hash()) << std::endl;
                break;
            case command_t::id_t::sfen:
                std::cout << kyokumen.sfen_string() << std::endl;
                break;
            case command_t::id_t::eval:
                break;
            }
        }
    }

    inline void taikyoku_t::print() const
    {
        if (kyokumen.move_count == 0)
        {
            for (const color_t color : colors)
                std::cout << color_to_string(static_cast<color_t>(color)) << "�F" << kishi_list[color.value()]->name() << std::endl;
            std::cout << std::endl;
        }

        if (moves.empty())
        {
            auto & winner_evaluator = kishi_list[!kyokumen.color().value()];
            std::cout << kyokumen.move_count << "��l��" << std::endl;
            kyokumen.print();
            std::cout << color_to_string(!kyokumen.color()) << "�����i" << winner_evaluator->name() << "�j" << std::flush;
        }
        else
        {
            std::cout << (kyokumen.move_count + 1) << "���" << color_to_string(kyokumen.color()) << "��" << std::endl;
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
        moves = kyokumen.search_moves();
    }

    /**
    * @breif �΋ǂ���B
    * @param black_kishi ���̊��m
    * @param white_kishi ���̊��m
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

        taikyoku.print(); // �l�񂾋ǖʂ�W���o�͂ɏo�͂���B

        std::cout << std::endl;
        details::timer.print_elapsed_time();

        taikyoku.kyokumen.print_kifu();
        std::cout.flush();
    }

    /**
     * @breif �΋ǂ���B
     * @param black_kishi ���̊��m
     * @param white_kishi ���̊��m
     */
    inline void do_taikyoku(const std::shared_ptr<abstract_kishi_t> & black_kishi, const std::shared_ptr<abstract_kishi_t> & white_kishi)
    {
        kyokumen_t kyokumen;
        do_taikyoku(kyokumen, black_kishi, white_kishi);
    }

    /**
     * @breif USI�v���g�R���ŒʐM����@�\��񋟂���B
     */
    class abstract_usi_engine_t
    {
    public:
        /**
         * @breif USI�v���g�R���ŒʐM���J�n����B
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
                 
                    std::optional<unsigned long> opt_time[color_t::size()];
                    std::optional<unsigned long> opt_byoyomi;
                    std::optional<unsigned long> opt_inc[color_t::size()];
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
                            opt_time[black.value()] = std::stoul(tokens[current]);
                            ++current;
                        }
                        else if (tokens[current] == "wtime")
                        {
                            ++current;
                            if (current >= tokens.size())
                            {
                                throw invalid_usi_input{ "wtime value not found" };
                            }
                            opt_time[white.value()] = std::stoul(tokens[current]);
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
                            opt_inc[black.value()] = std::stoul(tokens[current]);
                            ++current;
                        }
                        else if (tokens[current] == "winc")
                        {
                            ++current;
                            if (current >= tokens.size())
                            {
                                throw invalid_usi_input{ "winc value not found" };
                            }
                            opt_inc[white.value()] = std::stoul(tokens[current]);
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
                                milli_second_time_t limit_time = *opt_time[kyokumen.color().value()];
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
                            try
                            {
                                const context_t context(program_option_max_depth, program_option_max_selective_depth);
                                evaluator->best_move(kyokumen, context);
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
         * @breif �v���O�����̖��O��Ԃ��B
         * @return �v���O�����̖��O
         */
        virtual std::string name() = 0;

        /**
         * @breif �v���O�����̊J���Җ���Ԃ��B
         * @return �v���O�����̊J���Җ�
         */
        virtual std::string author() = 0;

        /**
         * @breif �v���O�����̌ŗL�̐ݒ�l��Ԃ��B
         * @return �v���O�����̌ŗL�̐ݒ�l
         * @details ���̊֐����Ԃ���������́A�T�[�o�[���� usi �R�}���h�����M���ꂽ��A usiok �𑗐M����O�ɑ��M�����B
         */
        virtual std::string options() = 0;

        /**
         * @breif isready �R�}���h����M�����ꍇ�ɌĂяo�����B
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

        inline action_t random_action() noexcept
        {
            unsigned int div = m_mutation_rate + m_crossover_rate + m_selection_rate;
            unsigned int value = details::random<unsigned int>(0, div - 1);
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
            for (std::size_t i = 0; i < fitness_table.size(); ++i)
            {
                const fitness_type fitness = fitness_table[i];
                if (div < fitness)
                    return individuals[i];
                div -= fitness;
            }
            return individuals.back();
        }

        inline static std::vector<move_t> make_kifu(const std::shared_ptr<chromosome_evaluator_t> & black, const std::shared_ptr<chromosome_evaluator_t> & white)
        {
            const std::shared_ptr<abstract_kishi_t> black_kishi{ std::make_shared<computer_kishi_t>(black) };
            const std::shared_ptr<abstract_kishi_t> white_kishi{ std::make_shared<computer_kishi_t>(white) };

            taikyoku_t taikyoku{ black_kishi, white_kishi };
            while (true)
            {
                taikyoku.print();
                if (!taikyoku.procedure())
                    break;
            }

            taikyoku.print(); // �l�񂾋ǖʂ�W���o�͂ɏo�͂���B

            std::cout << std::endl;
            details::timer.print_elapsed_time();

            taikyoku.kyokumen.print_kifu();
            std::cout.flush();

            return taikyoku.kyokumen.kifu;
        }

        inline void run()
        {
            std::vector<fitness_type> fitness_table;
            fitness_table.resize(individuals.size(), 0);

            // ������ő΋ǂ�����B
            for (std::size_t i = 0; i < individuals.size(); ++i)
            {
                for (std::size_t j = 0; j < individuals.size(); ++j)
                {
                    if (i != j)
                    {
                        std::vector<move_t> kifu = make_kifu(individuals[i], individuals[j]);
                        if (kifu.size() % 2 == 1) // �����̒�������̏ꍇ�A���̏���
                            fitness_table[i] += 1;
                    }
                }
            }

            // ��������쐬����B
            std::vector<std::shared_ptr<chromosome_evaluator_t>> next_individuals;
            while (next_individuals.size() < individuals.size())
            {
                const action_t action = random_action();
                if (action == action_t::mutation)
                {
                    const std::shared_ptr<chromosome_evaluator_t> & individual = select_individual(fitness_table);
                    const std::shared_ptr<chromosome_t> chromosome = std::make_shared<chromosome_t>(*individual->chromosome());
                    chromosome->mutate();
                    const std::shared_ptr<chromosome_evaluator_t> next_individual = std::make_shared<chromosome_evaluator_t>(chromosome, individual->name(), individual->id() + 1);
                    next_individuals.push_back(next_individual);
                }
                else if (action == action_t::crossover)
                {
                    const std::shared_ptr<chromosome_evaluator_t> & base = select_individual(fitness_table);
                    const std::shared_ptr<chromosome_evaluator_t> & sub = select_individual(fitness_table);
                    const std::shared_ptr<chromosome_t> chromosome = std::make_shared<chromosome_t>(*base->chromosome());
                    chromosome->clossover(*sub->chromosome());
                    const std::shared_ptr<chromosome_evaluator_t> next_individual = std::make_shared<chromosome_evaluator_t>(chromosome, base->name(), base->id() + 1);
                    next_individuals.push_back(next_individual);
                }
                else if (action == action_t::selection)
                {
                    const std::shared_ptr<chromosome_evaluator_t> & individual = select_individual(fitness_table);
                    const std::shared_ptr<chromosome_t> chromosome = std::make_shared<chromosome_t>(*individual->chromosome());
                    const std::shared_ptr<chromosome_evaluator_t> next_individual = std::make_shared<chromosome_evaluator_t>(chromosome, individual->name(), individual->id() + 1);
                    next_individuals.push_back(next_individual);
                }
            }

            // ������X�V����B
            individuals = std::move(next_individuals);
        }

        inline genetic_algorithm_t(const std::vector<std::string> & paths)
        {
            for (const std::string & path : paths)
            {
                try
                {
                    std::smatch results;
                    std::string name;
                    chromosome_evaluator_t::id_type id{};

                    if (!std::regex_match(path, results, std::regex(R"((.*)_(\d+))")))
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
            for (const auto & individual : individuals)
                individual->write_file(directory + "/" + individual->name() + "_" + std::to_string(individual->id()));
        }

        inline void set_mutation_rate (unsigned int mutation_rate ) noexcept { m_mutation_rate  = mutation_rate ; }
        inline void set_crossover_rate(unsigned int crossover_rate) noexcept { m_crossover_rate = crossover_rate; }
        inline void set_selection_rate(unsigned int selection_rate) noexcept { m_selection_rate = selection_rate; }

    private:
        std::vector<std::shared_ptr<chromosome_evaluator_t>> individuals;
        unsigned int m_mutation_rate  = 10;
        unsigned int m_crossover_rate = 800;
        unsigned int m_selection_rate = 190;
    };

    inline int parse_command_line(int argc, const char ** argv) noexcept
    {
        try
        {
            std::optional<std::string> black_name;
            std::optional<std::string> white_name;
            std::optional<unsigned long long> ga_iteration;
            std::optional<std::string> ga_chromosome;
            std::optional<unsigned int> ga_create_chromosome;
            std::optional<unsigned int> ga_mutation_rate;
            std::optional<unsigned int> ga_crossover_rate;
            std::optional<unsigned int> ga_selection_rate;

            auto callback = [&](const std::string & option, const std::vector<std::string> & params)
            {
                if (option == "help")
                {
                    print_help();
                }
                else if (option == "black" && !params.empty())
                {
                    black_name = params[0];
                }
                else if (option == "white" && !params.empty())
                {
                    white_name = params[0];
                }
                else if (option == "max-depth" && !params.empty())
                {
                    try
                    {
                        program_option_max_depth = std::stoi(params[0]);
                    }
                    catch (...)
                    {
                        std::cerr << "invalid max-depth parameter" << std::endl;
                    }
                }
                else if (option == "max-selective-depth" && !params.empty())
                {
                    try
                    {
                        program_option_max_selective_depth = std::stoi(params[0]);
                    }
                    catch (...)
                    {
                        std::cerr << "invalid max-selective-depth parameter" << std::endl;
                    }
                }
                else if (option == "ga-iteration" && !params.empty())
                {
                    try
                    {
                        ga_iteration = std::stoull(params[0]);
                    }
                    catch (...)
                    {
                        std::cerr << "invalid ga-iteration parameter" << std::endl;
                    }
                }
                else if (option == "ga-chromosome" && !params.empty())
                {
                    ga_chromosome = params[0];
                }
                else if (option == "ga-create-chromosome" && !params.empty())
                {
                    try
                    {
                        ga_create_chromosome = std::stoi(params[0]);
                    }
                    catch (...)
                    {
                        std::cerr << "invalid ga-create-chromosome parameter" << std::endl;
                    }
                }
                else if (option == "ga-mutation-rate" && !params.empty())
                {
                    try
                    {
                        ga_mutation_rate = std::stoi(params[0]);
                    }
                    catch (...)
                    {
                        std::cerr << "invalid ga-mutation-rate parameter" << std::endl;
                    }
                }
                else if (option == "ga-crossover-rate" && !params.empty())
                {
                    try
                    {
                        ga_crossover_rate = std::stoi(params[0]);
                    }
                    catch (...)
                    {
                        std::cerr << "invalid ga-crossover-rate parameter" << std::endl;
                    }
                }
                else if (option == "ga-selection-rate" && !params.empty())
                {
                    try
                    {
                        ga_selection_rate = std::stoi(params[0]);
                    }
                    catch (...)
                    {
                        std::cerr << "invalid ga-selection-rate parameter" << std::endl;
                    }
                }
            };
            parse_program_options(argc, argv, callback);

            if (ga_chromosome)
            {
                if (ga_create_chromosome)
                {
                    for (unsigned int i = 0; i < *ga_create_chromosome; ++i)
                    {
                        const std::filesystem::path path = *ga_chromosome + "/" + std::to_string(i) + "_0";
                        const std::shared_ptr<chromosome_t> chromosome = std::make_shared<chromosome_t>();
                        chromosome->generate();
                        chromosome->write_file(path);
                    }
                }
                else if (ga_iteration)
                {
                    std::vector<std::string> chromosome_paths;
                    for (const std::filesystem::directory_entry & entry : std::filesystem::directory_iterator{ *ga_chromosome })
                        chromosome_paths.push_back(entry.path().string());
                    const std::shared_ptr<genetic_algorithm_t> ga = std::make_shared<genetic_algorithm_t>(chromosome_paths);

                    if (ga_mutation_rate)
                        ga->set_mutation_rate(*ga_mutation_rate);
                    if (ga_crossover_rate)
                        ga->set_crossover_rate(*ga_crossover_rate);
                    if (ga_selection_rate)
                        ga->set_selection_rate(*ga_selection_rate);

                    for (unsigned long long iteration_count = 0; iteration_count < *ga_iteration; ++iteration_count)
                        ga->run();
                    ga->write_file(*ga_chromosome);
                }
            }
            else if (black_name && white_name)
            {
                auto black_iter = kishi_map.find(*black_name);
                if (black_iter == kishi_map.end())
                {
                    throw invalid_command_line_input{ "invalid black name" };
                }
                const std::shared_ptr<abstract_kishi_t> & black_kishi = black_iter->second;

                auto white_iter = kishi_map.find(*white_name);
                if (white_iter == kishi_map.end())
                {
                    throw invalid_command_line_input{ "invalid white name" };
                }
                const std::shared_ptr<abstract_kishi_t> & white_kishi = white_iter->second;

                do_taikyoku(black_kishi, white_kishi);
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
            return 1;
        }

        return 0;
    }

} // namespace shogipp

#endif // SHOGIPP_DEFINED