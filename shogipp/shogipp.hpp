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

//#define NONDETERMINISM
#ifdef NONDETERMINISM
#define SHOGIPP_SEED std::random_device{}()
#else
#define SHOGIPP_SEED
#endif

#ifdef NDEBUG
#define SHOGIPP_ASSERT(expr)
#else
#define SHOGIPP_ASSERT(expr) do { shogipp::assert_impl((expr), #expr, __FILE__, __func__, __LINE__); } while (false)
#endif

//#define VALIDATE_MOVEMENT_CACHE
#define DISABLE_MOVEMENT_CACHE

#ifdef NDEBUG
#define VALIDATE_KYOKUMEN_ROLLBACK(kyokumen)
#else
#define VALIDATE_KYOKUMEN_ROLLBACK(kyokumen) kyokumen_rollback_validator_t kyokumen_rollback_validator{ kyokumen }
#endif

namespace shogipp
{
    inline unsigned long long total_search_count = 0;

    /**
     * @breif SHOGIPP_ASSERT �}�N���̎���
     * @param assertion ����]������ bool �l
     * @param expr ����\�����镶����
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

    using koma_t = unsigned char;
    enum : koma_t
    {
        empty,
        fu, kyo, kei, gin, kin, kaku, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu,
        sente_fu = fu, sente_kyo, sente_kei, sente_gin, sente_kin, sente_kaku, sente_hi, sente_ou, sente_tokin, sente_nari_kyo, sente_nari_kei, sente_nari_gin, sente_uma, sente_ryu,
        gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, gote_uma, gote_ryu,
        koma_enum_number,
        out_of_range = 0xff
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
     * @breif �t�̎�Ԃ��擾����B
     * @param sengo ��肩��肩
     * @retval sente sengo == gote �̏ꍇ
     * @retval gote sengo == sente �̏ꍇ
     */
    inline sengo_t operator !(sengo_t sengo)
    {
        SHOGIPP_ASSERT(sengo >= sente);
        SHOGIPP_ASSERT(sengo <= gote);
        return static_cast<sengo_t>((sengo + 1) % sengo_size);
    }

    using pos_t = signed char;
    constexpr pos_t npos = -1; // �����ȍ��W��\������萔
    constexpr pos_t width = 11;
    constexpr pos_t height = 11;
    constexpr pos_t pos_size = width * height;
    constexpr pos_t padding_width = 1;
    constexpr pos_t padding_height = 1;
    constexpr pos_t suji_size = 9;
    constexpr pos_t dan_size = 9;

    /**
     * @breif ���W����i�𒊏o����B
     * @param pos ���W
     * @return �i
     */
    inline constexpr pos_t pos_to_dan(pos_t pos)
    {
        return pos / width - padding_height;
    }

    /**
     * @breif ���W����؂𒊏o����B
     * @param pos ���W
     * @return ��
     */
    inline constexpr pos_t pos_to_suji(pos_t pos)
    {
        return pos % width - padding_width;
    }

    /**
     * @breif 2�̍��W�Ԃ̃}���n�b�^���������v�Z����B
     * @param a ���WA
     * @param b ���WB
     * @return 2�̍��W�Ԃ̃}���n�b�^������
     */
    inline pos_t distance(pos_t a, pos_t b)
    {
        pos_t suji_a = pos_to_suji(a);
        pos_t dan_a = pos_to_dan(a);
        pos_t suji_b = pos_to_suji(b);
        pos_t  dan_b = pos_to_dan(b);
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
        { "���" , sente },
        { "���" , gote }
    };
    static constexpr std::size_t sengo_string_size = 4;

    static const std::map<std::string, unsigned char> digit_string_map
    {
        { "�O", 0 },
        { "�P", 1 },
        { "�Q", 2 },
        { "�R", 3 },
        { "�S", 4 },
        { "�T", 5 },
        { "�U", 6 },
        { "�V", 7 },
        { "�W", 8 },
        { "�X", 9 }
    };
    static constexpr std::size_t digit_string_size = 2;

    static const std::map<std::string, koma_t> koma_string_map
    {
        { "�E", empty },
        { "��", fu },
        { "��", kyo },
        { "�j", kei },
        { "��", gin },
        { "��", kin },
        { "�p", kaku },
        { "��", hi },
        { "��", ou },
        { "��", tokin },
        { "��", nari_kyo },
        { "�\", nari_kei },
        { "�S", nari_gin },
        { "�n", uma },
        { "��", hi }
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
        { "��", sente },
        { "��", gote }
    };
    static constexpr std::size_t sengo_mark_size = 2;

    static const std::map<std::string, unsigned char> suji_string_map
    {
        { "�P", 0 },
        { "�Q", 1 },
        { "�R", 2 },
        { "�S", 3 },
        { "�T", 4 },
        { "�U", 5 },
        { "�V", 6 },
        { "�W", 7 },
        { "�X", 8 }
    };
    static constexpr std::size_t suji_string_size = 2;

    static const std::map<std::string, unsigned char> dan_string_map
    {
        { "��", 0 },
        { "��", 1 },
        { "�O", 2 },
        { "�l", 3 },
        { "��", 4 },
        { "�Z", 5 },
        { "��", 6 },
        { "��", 7 },
        { "��", 8 }
    };
    static constexpr std::size_t dan_string_size = 2;

    template<typename Map>
    inline typename std::decay_t<Map>::mapped_type parse(std::string_view & rest, Map && map, std::size_t size)
    {
        if (rest.size() < size)
            throw parse_error{ "parse 1-1" };
        auto iter = map.find(std::string{ rest.substr(0, size) });
        if (iter == map.end())
            throw parse_error{ "parse 1-2" };
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

    inline constexpr sengo_t tesu_to_sengo(tesu_t tesu)
    {
        return static_cast<sengo_t>(tesu % sengo_size);
    }

    /**
     * @breif ���̏ꍇ�� -1 ���A���̏ꍇ�� 1 ��Ԃ��B
     * @param ��肩��肩
     * @return �������]�p�̐��l
     */
    inline constexpr pos_t reverse(sengo_t sengo)
    {
        return sengo ? -1 : 1;
    }

    using hash_t = std::size_t;

    /*
     * @breif �������肷��B
     * @param koma ��
     * @retval true ����ł���
     * @retval false ����łȂ�
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
     * @breif �����邩���肷��B
     * @param koma ��
     * @retval true �����
     * @retval false ����Ȃ�
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
     * @breif ����̋���肷��B
     * @param koma ��
     * @return ���̋�ł���ꍇ sente
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
     * @breif ������(���E�p�E��E�n�E��)�����肷��B
     * @param koma ��
     * @return �����ł���ꍇ true
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
     * @breif ���������Ƃ��ēK�i�̋�ɕϊ�����B
     * @param koma ��
     * @return ������Ƃ��ēK�i�̋�
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
     * @breif ��𐬂�O�̋�ɕϊ�����B
     * @param koma ��
     * @return ����O�̋�
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
     * @breif ��𐬂��ɕϊ�����B
     * @param koma ��
     * @return �����
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
     * @breif �������̏�����菜���B
     * @param koma ��
     * @return �����̏�����菜���ꂽ��
     * @details SENTE_FU -> FU, GOTE_FU -> FU
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
     * @breif ������̋�ɕϊ�����B
     * @param koma ��
     * @return ���̋�
     * @details FU -> GOTE_FU
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
     * @breif koma �� target_koma �ɍ��v���邩���肷��B
     * @param koma ��
     * @retval true ���v����
     * @retval false ���v���Ȃ�
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
         * @param koma ��
         * @param pos ��̍��W
         * @return �n�b�V���l
         */
        inline hash_t koma_hash(koma_t koma, pos_t pos) const;

        /**
         * @breif ������̃n�b�V���l���v�Z����B
         * @param koma ��
         * @param count ��̐�
         * @param is_gote ���̎����
         * @return �n�b�V���l
         */
        inline hash_t mochigoma_hash(koma_t koma, std::size_t count, sengo_t sengo) const;

    private:
        hash_t ban_table[koma_enum_number * suji_size * dan_size];
        hash_t mochigoma_table[(18 + 4 + 4 + 4 + 4 + 2 + 2) * 2 * 2];
    };

    static const hash_table_t hash_table;

    inline hash_table_t::hash_table_t()
    {
        std::minstd_rand rand{ SHOGIPP_SEED };
        std::uniform_int_distribution<hash_t> uid{ std::numeric_limits<hash_t>::min(), std::numeric_limits<hash_t>::max() };
        for (std::size_t i = 0; i < std::size(ban_table); ++i)
            ban_table[i] = uid(rand);
        for (std::size_t i = 0; i < std::size(mochigoma_table); ++i)
            mochigoma_table[i] = uid(rand);
    }

    inline hash_t hash_table_t::koma_hash(koma_t koma, pos_t pos) const
    {
        std::size_t index = koma;
        index *= suji_size;
        index += pos_to_suji(pos);
        index *= dan_size;
        index += pos_to_dan(pos);
        return ban_table[index];
    }

    inline hash_t hash_table_t::mochigoma_hash(koma_t koma, std::size_t count, sengo_t sengo) const
    {
        enum
        {
            fu_offset   = 0                     , fu_max   = 18,
            kyo_offset  = fu_offset   + fu_max  , kyo_max  =  4,
            kei_offset  = kyo_offset  + kyo_max , kei_max  =  4,
            gin_offset  = kei_offset  + kei_max , gin_max  =  4,
            kin_offset  = gin_offset  + gin_max , kin_max  =  4,
            kaku_offset = kin_offset  + kin_max , kaku_max =  2,
            hi_offset   = kaku_offset + kaku_max, hi_max   =  2,
            size        = hi_offset   + hi_max
        };

        SHOGIPP_ASSERT(koma != empty);
        SHOGIPP_ASSERT(koma >= fu);
        SHOGIPP_ASSERT(koma <= hi);
        SHOGIPP_ASSERT(!(koma == fu && count > fu_max));
        SHOGIPP_ASSERT(!(koma == kyo && count > kyo_max));
        SHOGIPP_ASSERT(!(koma == kei && count > kei_max));
        SHOGIPP_ASSERT(!(koma == gin && count > gin_max));
        SHOGIPP_ASSERT(!(koma == kin && count > kin_max));
        SHOGIPP_ASSERT(!(koma == kaku && count > kaku_max));
        SHOGIPP_ASSERT(!(koma == hi && count > hi_max));

        static const std::size_t map[]
        {
            0,
            fu_offset,
            kyo_offset,
            kei_offset,
            gin_offset,
            kin_offset,
            kaku_offset,
            hi_offset,
        };

        std::size_t index = map[koma];
        index += count;
        index *= sengo_size;
        if (sengo)
            ++index;
        return ban_table[index];
    }

    /**
     * @breif ����\�����镶������擾����B
     * @param sengo ���
     * @return ����\�����镶����
     */
    inline const char * sengo_to_string(sengo_t sengo)
    {
        const char * map[]{ "���", "���" };
        return map[sengo];
    }

    /**
     * @breif ���l��S�p������ɕϊ�����B
     * @param value ���l
     * @return �S�p������
     * @details ������̍ő喇��18�𒴂���l���w�肵�Ă��̊֐����Ăяo���Ă͂Ȃ�Ȃ��B
     */
    inline const char * to_zenkaku_digit(unsigned int value)
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
     * @param koma ��
     * @return ��̈ړ���̑��΍��W�̔z��̐擪���w���|�C���^
     * @details ���̊֐����Ԃ��|�C���^�̎w�����W�� 0 �ŏI�[������Ă���B
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
     * @breif ��̈ړ���̑��΍��W�̔z��̐擪���w���|�C���^���擾����B
     * @param koma ��
     * @return ��̈ړ���̑��΍��W�̔z��̐擪���w���|�C���^
     * @details ���̊֐����Ԃ��|�C���^�̎w�����W�� 0 �ŏI�[������Ă���B
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
     * @breif ��𕶎���ɕϊ�����B
     * @param koma ��
     * @return ������
     */
    inline const char * koma_to_string(koma_t koma)
    {
        SHOGIPP_ASSERT(koma < koma_enum_number);
        static const char * map[]{
            "�E",
            "��", "��", "�j", "��", "��", "�p", "��", "��", "��", "��", "�\", "�S", "�n", "��",
            "��", "��", "�j", "��", "��", "�p", "��", "��", "��", "��", "�\", "�S", "�n", "��",
        };
        return map[koma];
    }

    /**
     * @breif �i�𕶎���ɕϊ�����B
     * @param dan �i
     * @return ������
     */
    inline const char * dan_to_string(pos_t dan)
    {
        static const char * map[]{ "��", "��", "�O", "�l", "��", "�Z", "��", "��", "��" };
        SHOGIPP_ASSERT(dan >= 0);
        SHOGIPP_ASSERT(dan < static_cast<pos_t>(std::size(map)));
        return map[dan];
    }

    /**
     * @breif �؂𕶎���ɕϊ�����B
     * @param dan ��
     * @return ������
     */
    inline const char * suji_to_string(pos_t suji)
    {
        static const char * map[]{ "�X", "�W", "�V", "�U", "�T", "�S", "�R", "�Q", "�P" };
        SHOGIPP_ASSERT(suji >= 0);
        SHOGIPP_ASSERT(suji < static_cast<pos_t>(std::size(map)));
        return map[suji];
    }

    /**
     * @breif ���W�𕶎���ɕϊ�����B
     * @param pos ���W
     * @return ������
     */
    inline std::string pos_to_string(pos_t pos)
    {
        return std::string{}.append(suji_to_string(pos_to_suji(pos))).append(dan_to_string(pos_to_dan(pos)));
    }

    /**
     * @breif �؂ƒi������W���擾����B
     * @param suji ��
     * @param dan �i
     * @return ���W
     */
    inline pos_t suji_dan_to_pos(pos_t suji, pos_t dan)
    {
        return width * (dan + padding_height) + suji + padding_width;
    }

    static const pos_t default_ou_pos_list[]
    {
        suji_dan_to_pos(4, 8),
        suji_dan_to_pos(4, 0)
    };

    /**
     * @breif ���W��W���o�͂ɏo�͂���B
     * @param pos ���W
     */
    inline void print_pos(pos_t pos)
    {
        std::cout << suji_to_string(pos_to_suji(pos)) << dan_to_string(pos_to_dan(pos));
        std::cout.flush();
    }

    /**
     * @breif ���@��
     */
    class te_t
    {
    public:
        /**
         * @breif �ł�����\�z����B
         * @param destination �ł��W
         * @param source_koma �ł�
         */
        inline constexpr te_t(pos_t destination, koma_t source_koma) noexcept;

        /**
         * @breif �ړ��������\�z����B
         * @param source �ړ����̍��W
         * @param destination �ړ���̍��W
         * @param source_koma �ړ����̋�
         * @param captured_koma �ړ���̋�
         * @param promote �����s����
         */
        inline constexpr te_t(pos_t source, pos_t destination, koma_t source_koma, koma_t captured_koma, bool promote) noexcept;

        /**
         * @breif �ł��肩���肷��B
         * @retval true �ł���ł���
         * @retval false �ړ������ł���
         */
        inline constexpr bool is_uchite() const noexcept;

        /**
         * @breif �ړ����̍��W���擾����B
         * @return �ړ����̍��W
         * @details is_uchite �� true ��Ԃ��ꍇ�ɂ��̊֐����Ăяo�����ꍇ�A�����Ȓl���Ԃ�B
         */
        inline constexpr pos_t source() const noexcept;

        /**
         * @breif �ړ���̍��W���擾����B
         * @return �ړ���̍��W
         * @details is_uchite �� true ��Ԃ��ꍇ�A���̊֐��͑ł�̍��W��Ԃ��B
         */
        inline constexpr pos_t destination() const noexcept;

        /**
         * @breif �ړ����̋���擾����B
         * @return �ړ����̋�
         */
        inline constexpr koma_t source_koma() const noexcept;

        /**
         * @breif �ړ���̋���擾����B
         * @return �ړ���̋�
         * @detalis is_uchite �� true ��Ԃ��ꍇ�ɂ��̊֐����Ăяo�����ꍇ�A�����Ȓl���Ԃ�B
         */
        inline constexpr koma_t captured_koma() const noexcept;

        /**
         * @breif ���邩�ۂ����擾����B
         * @retval true ����
         * @retval false ����Ȃ�
         * @detalis is_uchite �� true ��Ԃ��ꍇ�ɂ��̊֐����Ăяo�����ꍇ�A�����Ȓl���Ԃ�B
         */
        inline constexpr bool promote() const noexcept;

    private:
        pos_t   m_source;           // �ړ����̍��W(src == npos �̏ꍇ�A�������ł�)
        pos_t   m_destination;      // �ړ���̍��W(src == npos �̏ꍇ�A dst �͑ł��W)
        koma_t  m_source_koma;      // �ړ����̋�(src == npos �̏ꍇ�A source_koma() �͑ł�����)
        koma_t  m_captured_koma;    // �ړ���̋�(src == npos �̏ꍇ�A dstkoma �͖���`)
        bool    m_promote;          // ����ꍇ true
    };

    inline constexpr te_t::te_t(pos_t destination, koma_t source_koma) noexcept
        : m_source{ npos }
        , m_destination{ destination }
        , m_source_koma{ source_koma }
        , m_captured_koma{ empty }
        , m_promote{ false }
    {
    }

    inline constexpr te_t::te_t(pos_t source, pos_t destination, koma_t source_koma, koma_t captured_koma, bool promote) noexcept
        : m_source{ source }
        , m_destination{ destination }
        , m_source_koma{ source_koma }
        , m_captured_koma{ captured_koma }
        , m_promote{ promote }
    {
    }

    inline constexpr bool te_t::is_uchite() const noexcept
    {
        return m_source == npos;
    }

    inline constexpr pos_t te_t::source() const noexcept
    {
        SHOGIPP_ASSERT(!is_uchite());
        return m_source;
    }

    inline constexpr pos_t te_t::destination() const noexcept
    {
        return m_destination;
    }

    inline constexpr koma_t te_t::source_koma() const noexcept
    {
        return m_source_koma;
    }

    inline constexpr koma_t te_t::captured_koma() const noexcept
    {
        SHOGIPP_ASSERT(!is_uchite());
        return m_captured_koma;
    }

    inline constexpr bool te_t::promote() const noexcept
    {
        SHOGIPP_ASSERT(!is_uchite());
        return m_promote;
    }

    /**
     * @breif ������
     */
    class mochigoma_t
    {
    public:
        /**
         * @breif �����������������B
         */
        inline void init();

        /**
         * @breif �������W���o�͂ɏo�͂���B
         */
        inline void print() const;

        /**
         * @breif ��ƑΉ����鎝����̐��̎Q�Ƃ�Ԃ��B
         * @param ��
         * @return ��ƑΉ����鎝����̐��̎Q��
         */
        inline unsigned char & operator [](koma_t koma);

        /**
         * @breif ��ƑΉ����鎝����̐��̎Q�Ƃ�Ԃ��B
         * @param ��
         * @return ��ƑΉ����鎝����̐��̎Q��
         */
        inline const unsigned char & operator [](koma_t koma) const;

    private:
        unsigned char count[hi - fu + 1];
    };

    inline void mochigoma_t::init()
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
            std::cout << "�Ȃ�";
        std::cout << std::endl;
    }

    inline unsigned char & mochigoma_t::operator [](koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        SHOGIPP_ASSERT(trim_sengo(koma) != ou);
        static const std::size_t map[]{
            0,
            fu - fu, kyo - fu, kei - fu, gin - fu, kin - fu, kaku - fu, hi - fu, 0,
            fu - fu, kyo - fu, kei - fu, gin - fu, kaku - fu, hi - fu
        };
        SHOGIPP_ASSERT(trim_sengo(koma) < std::size(map));
        return count[map[trim_sengo(koma)]];
    }

    inline const unsigned char & mochigoma_t::operator [](koma_t koma) const
    {
        return (*const_cast<mochigoma_t *>(this))[koma];
    }

    class kyokumen_rollback_validator_t;

    /**
     * @breif ��
     */
    class ban_t
    {
    public:
        /**
         * @breif �Ղ�����������B
         */
        inline void init();

        inline koma_t & operator [](size_t i) { return data[i]; }
        inline const koma_t & operator [](size_t i) const { return data[i]; }

        /**
         * @breif ���Wpos���ՊO�����肷��B
         * @param pos ���W
         * @return �ՊO�̏ꍇtrue
         */
        inline static bool out(pos_t pos);

        /**
         * @breif �Ղ�W���o�͂ɏo�͂���B
         */
        inline void print() const;

    private:
        friend class kyokumen_rollback_validator_t;
        koma_t data[pos_size];
    };

#define _ empty
#define x out_of_range
    inline void ban_t::init()
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
        std::cout << "  �X �W �V �U �T �S �R �Q �P" << std::endl;
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

    /**
     * @breif ����
     */
    class kiki_t
    {
    public:
        pos_t pos;      // �����Ă����̍��W
        pos_t offset;   // �����̑��΍��W
        bool aigoma;    // ����\��
    };

    using aigoma_info_t = std::unordered_map<pos_t, std::vector<pos_t>>;

    /**
     * @breif �ǖ�
     */
    class kyokumen_t
    {
    public:
        using move_table_t = std::map<pos_t, std::vector<pos_t>>;

        /**
         * @breif ��̈ړ����ƈړ���̏�������������B
         */
        inline void init_move_table_list();

        /**
         * @breif ��̈ړ����ƈړ���̏����X�V����B
         * @param move_table  ��̈ړ����ƈړ���̏��
         * @param source �ړ����̍��W
         * @param destination_list �ړ���̍��W�� vector
         * @details destination_list.empty() == true �̏ꍇ�A move_table ���� source ���폜����B
         */
        inline void update_move_table(move_table_t & move_table, pos_t source, std::vector<pos_t> && destination_list);

        /**
         * @breif ���W pos �ɗ����Ă��邩�R��t���Ă������������A���̍��W���ړ����Ƃ����̈ړ����ƈړ���̏����X�V����B
         * @param pos ���W
         * @details destination_list.empty() == true �̏ꍇ�A move_table ���� source ���폜����B
         */
        inline void update_move_table_relative_to(pos_t pos);

        /**
         * @breif �Ō�ɉ������ꂽ���@������ɁA��̈ړ����ƈړ���̏����X�V����B
         * @param te �Ō�Ɏ��{���ꂽ���@��
         * @details do_te �̓����ō��@�肪�������ꂽ��ŌĂ΂��z��Ŏ��������B
         */
        inline void do_updating_move_table_list(const te_t & te);

        /**
         * @breif �Ō�ɉ������ꂽ���@������ɁA��̈ړ����ƈړ���̏����X�V����B
         * @param te �Ō�Ɏ��{���ꂽ���@��
         * @details undo_te �̓����ō��@�肪�������ꂽ��ŌĂ΂��z��Ŏ��������B
         */
        inline void undo_updating_move_table_list(const te_t & te);

        /**
         * @breif �ǖʂ�����������B
         */
        inline void init();

        /**
         * @breif ��koma�����Wdst�Ɉړ�����ꍇ�ɐ��肪�\�����肷��B
         * @param koma ��
         * @param dst �ړ���̍��W
         * @return ���肪�\�̏ꍇ(koma�����ɐ����Ă���ꍇ�A���false)
         */
        inline static bool can_promote(koma_t koma, pos_t dst);

        /**
         * @breif ��koma�����Wdst�Ɉړ�����ꍇ�ɐ��肪�K�{�����肷��B
         * @param koma ��
         * @param dst �ړ���̍��W
         * @return ���肪�K�{�̏ꍇ(koma�����ɐ����Ă���ꍇ�A���false)
         */
        inline static bool must_promote(koma_t koma, pos_t dst);

        /**
         * @breif ���Wsrc����ړ��\�̈ړ���𔽕��I�Ɍ�������B
         * @param result �ړ���̍��W�̏o�̓C�e���[�^
         * @param src �ړ����̍��W
         * @param offset �ړ���̑��΍��W
         */
        template<typename OutputIterator>
        inline void search_far_destination(OutputIterator result, pos_t src, pos_t offset) const;

        /**
         * @breif ���Wsrc����ړ��\�̈ړ����񔽕��I�Ɍ�������B
         * @param result �ړ���̍��W�̏o�̓C�e���[�^
         * @param src �ړ����̍��W
         * @param offset �ړ���̑��΍��W
         */
        template<typename OutputIterator>
        inline void search_near_destination(OutputIterator result, pos_t src, pos_t offset) const;

        /**
         * @breif ���Wsrc����ړ��\�̈ړ������������B
         * @param result �ړ���̍��W�̏o�̓C�e���[�^
         * @param src �ړ����̍��W
         * @param sengo ���E���ǂ���̈ړ���
         */
        template<typename OutputIterator>
        inline void search_destination(OutputIterator result, pos_t src, sengo_t sengo) const;

        /**
         * @breif ������koma�����Wdst�ɒu�����Ƃ��ł��邩���肷��B���A���A�j�Ɍ���false��Ԃ��\��������B
         * @param koma ������
         * @param dst �ړ���̍��W
         * @return �u�����Ƃ��ł���ꍇ true
         */
        inline bool can_put(koma_t koma, pos_t dst);

        /**
         * @breif �ړ����̍��W����������B
         * @param result �o�̓C�e���[�^
         * @param sengo ���E���ǂ���̈ړ���
         */
        template<typename OutputIterator>
        inline void search_source(OutputIterator result, sengo_t sengo) const;

        /**
         * @breif ���Wpos���瑊�΍��Woffset�����ɑ������ŏ��ɋ�������W��Ԃ��B
         * @param pos �������J�n������W
         * @param offset �������鑊�΍��W
         * @return �ŏ��ɋ�������W(�������Ȃ��ꍇ npos )
         */
        inline pos_t search(pos_t pos, pos_t offset) const;

        /**
         * @breif ���Wpos�𗘂��Ă����邢�͕R��t���Ă�������������B
         * @param result �����̏o�̓C�e���[�^
         * @param pos ���W
         * @param offset �����̑��΍��W
         * @param first ������̓��̓C�e���[�^(begin)
         * @param last ������̓��̓C�e���[�^(end)
         * @param is_collected ����������̎�Ԃɑ΂��ďo�̓C�e���[�^�ɏo�͂��邩���肷�鏖�q�֐�(bool(bool))
         * @param transform (pos, offset, aigoma) ���o�̓C�e���[�^�ɏo�͂���ϐ��ɕϊ�����֐�
         */
        template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
        inline void search_koma_near(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform);

        /**
         * @breif ���Wpos�𗘂��Ă����邢�͕R��t���Ă�������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param pos ���W
         * @param offset �����̑��΍��W
         * @param first ������̓��̓C�e���[�^(begin)
         * @param last ������̓��̓C�e���[�^(end)
         * @param is_collected ����������̎�Ԃɑ΂��ďo�̓C�e���[�^�ɏo�͂��邩���肷�鏖�q�֐�(bool(bool))
         * @param transform (pos, offset, aigoma) ���o�̓C�e���[�^�ɏo�͂���ϐ��ɕϊ�����֐�
         * @sa search_kiki_far
         */
        template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
        inline void search_koma_far(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform);

        /**
         * @breif ���Wpos�ɗ����Ă����邢�͕R��t���Ă�������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param pos ���W
         * @param sengo ��ア����̎��_��
         * @param is_collected ����������̎�Ԃɑ΂��ďo�̓C�e���[�^�ɏo�͂��邩���肷�鏖�q�֐�(bool(bool))
         * @param transform (pos, offset, aigoma) ���o�̓C�e���[�^�ɏo�͂���ϐ��ɕϊ�����֐�
         */
        template<typename OutputIterator, typename IsCollected, typename Transform>
        inline void search_koma(OutputIterator result, pos_t pos, sengo_t sengo, IsCollected is_collected, Transform transform);

        /**
         * @breif ���Wpos�ɕR��t���Ă�������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param pos ���W
         * @param sengo ��ア����̎��_��
         */
        template<typename OutputIterator>
        inline void search_himo(OutputIterator result, pos_t pos, sengo_t sengo);

        /**
         * @breif ���Wpos�ɗ����Ă�������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param pos ���W
         * @param sengo ��ア����̎��_��
         */
        template<typename OutputIterator>
        inline void search_kiki(OutputIterator result, pos_t pos, sengo_t sengo);

        /**
         * @breif ���Wpos�ɗ����Ă����邢�͕R��t���Ă�������������B
         * @param result ���W�̏o�̓C�e���[�^
         * @param pos ���W
         * @param sengo ��ア����̎��_��
         */
        template<typename OutputIterator>
        inline void search_kiki_or_himo(OutputIterator result, pos_t pos, sengo_t sengo);

        /**
         * @breif ���ɑ΂��闘�����X�V����B
         */
        inline void update_oute();

        /**
         * @breif �������������B
         * @param aigoma_info ����̏o�͐�
         * @param sengo ��ア����̎��_��
         */
        inline void search_aigoma(aigoma_info_t & aigoma_info, sengo_t sengo);

        /**
         * @breif �ړ����ƈړ���̍��W���獇�@��𐶐�����B
         * @param result ���@��̏o�̓C�e���[�^
         * @param source �ړ����̍��W
         * @param destination �ړ���̍��W
         */
        template<typename OutputIterator>
        inline void search_te_from_positions(OutputIterator result, pos_t source, pos_t destination);

        /**
         * @breif ���@��𐶐�����B
         * @param result ���@��̏o�̓C�e���[�^
         */
        template<typename OutputIterator>
        inline void search_te(OutputIterator result);

        /**
         * @breif �ǖʂ̃n�b�V���l���v�Z����B
         * @return �ǖʂ̃n�b�V���l
         */
        inline hash_t make_hash() const;

        /**
         * @breif �ǖʂ̃n�b�V���l�ƍ��@�肩��A���@������{������̋ǖʂ̃n�b�V���l���v�Z����B
         * @param hash ���@������{����O�̋ǖʂ̃n�b�V���l
         * @param te ���{���鍇�@��
         * @return ���@������{������̋ǖʂ̃n�b�V���l
         * @details ���@��ɂ�蔭�����鍷���Ɋ�Â��v�Z���邽�� make_hash() ����r�I�����ɏ��������B
         *          ���̊֐��͍��@������{������O�ɌĂяo�����K�v������B
         */
        inline hash_t make_hash(hash_t hash, const te_t & te) const;

        /**
         * @breif ���@���W���o�͂ɏo�͂���B
         * @param te ���@��
         * @param is_gote ���̍��@�肩
         */
        inline void print_te(const te_t & te, sengo_t sengo) const;

        /**
         * @breif ���@���W���o�͂ɏo�͂���B
         * @param first ���@��̓��̓C�e���[�^��begin
         * @param last ���@��̓��̓C�e���[�^��end
         */
        template<typename InputIterator>
        inline void print_te(InputIterator first, InputIterator last) const;

        /**
         * @breif ���@���W���o�͂ɏo�͂���B
         */
        inline void print_te();

        /**
         * @breif �����W���o�͂ɏo�͂���B
         */
        inline void print_oute();

        /**
         * @breif �ǖʂ�W���o�͂ɏo�͂���B
         */
        inline void print();

        inline void print_kifu();

        /**
         * @breif �ǖʂ̃n�b�V���l��Ԃ��B
         * @return �ǖʂ̃n�b�V���l
         */
        inline hash_t hash() const;

#ifdef VALIDATE_MOVEMENT_CACHE
        /**
         * @breif search_srouce �� search_destination �ɂ��擾�����ړ����ƈړ��悪 move_table �ƈ�v���Ă��邩���؂���B
         */
        inline void validate_move_table()
        {
            for (sengo_t sengo : sengo_list)
            {
                auto & move_table = move_table_list[sengo];

                std::vector<pos_t> source_list;
                search_source(std::back_inserter(source_list), sengo);
                for (pos_t source : source_list)
                {
                    std::vector<pos_t> destination_list;
                    search_destination(std::back_inserter(destination_list), source, sengo);
                    
                    auto iter = move_table.find(source);
                    if (iter != move_table.end())
                    {
                        auto & [source2, destination_list2] = *iter;
                        if (!std::equal(destination_list.begin(), destination_list.end(), destination_list2.begin(), destination_list2.end()))
                        {
                            std::cerr << sengo_to_string(sengo) << " source " << pos_to_string(source) << " destination list { ";
                            for (pos_t destination : destination_list)
                                std::cerr << pos_to_string(destination) << " ";
                            std::cerr << "} != { ";
                            for (pos_t destination : destination_list2)
                                std::cerr << pos_to_string(destination) << " ";
                            std::cerr << "}" << std::endl;
                            SHOGIPP_ASSERT(false);
                        }
                    }
                    else if (destination_list.size())
                    {
                        std::cerr << sengo_to_string(sengo) << " source " << pos_to_string(source) << " is not found ({ ";
                        for (pos_t destination : destination_list)
                            std::cerr << pos_to_string(destination) << " ";
                        std::cerr << "})"<< std::endl;
                        SHOGIPP_ASSERT(false);
                    }
                }
            }
        }
#endif

        inline void validate_ban_out();

        /**
         * @breif ���@������s����B
         * @param te ���@��
         */
        inline void do_te(const te_t & te);

        /**
         * @breif ���@������s����O�ɖ߂��B
         * @param te ���@��
         */
        inline void undo_te(const te_t & te);

        /**
         * @breif ��Ԃ��擾����B
         * @return ���
         */
        inline sengo_t sengo() const;

        /**
         * @breif �ǖʃt�@�C������ǖʂ�ǂݍ��ށB
         * @param kyokumen_file �ǖʃt�@�C��
         */
        inline void read_kyokumen_file(std::filesystem::path kyokumen_file);

        /**
         * @breif �����t�@�C�����������ǂݍ��ށB
         * @param kifu_file �����t�@�C��
         */
        inline void read_kifu_file(std::filesystem::path kifu_file);

        ban_t ban;                                      // ��
        mochigoma_t mochigoma_list[sengo_size];         // ������
        tesu_t tesu;                                    // �萔
        pos_t ou_pos[sengo_size];                       // ���̍��W
        std::vector<kiki_t> oute_list[sengo_size];      // ���ɑ΂��闘��
        std::stack<hash_t> hash_stack;                  // ����܂ł̊e��Ԃɂ�����n�b�V���l���i�[����X�^�b�N
        std::vector<te_t> kifu;                         // ����
        move_table_t move_table_list[sengo_size];       // ���@��̕\
    };

    /**
     * @breif �R�s�[�R���X�g���N�g����Ă���f�X�g���N�g�����܂łɋǖʂ��ύX����Ă��Ȃ����Ƃ����؂���B
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

    inline void kyokumen_t::init_move_table_list()
    {
        for (sengo_t sengo : sengo_list)
        {
            auto & move_table = move_table_list[sengo];
            move_table.clear();
            std::vector<pos_t> source_list;
            search_source(std::back_inserter(source_list), tesu_to_sengo(sengo));
            for (pos_t src : source_list)
            {
                std::vector<pos_t> destination_list;
                search_destination(std::back_inserter(destination_list), src, tesu_to_sengo(sengo));
                move_table[src] = std::move(destination_list);
            }
        }
    }

    inline kyokumen_rollback_validator_t::~kyokumen_rollback_validator_t()
    {
        for (std::size_t i = 0; i < std::size(data); ++i)
            SHOGIPP_ASSERT(data[i] == kyokumen.ban.data[i]);
        for (sengo_t sengo : sengo_list)
            for (koma_t koma = fu; koma <= hi; ++koma)
                SHOGIPP_ASSERT(mochigoma_list[sengo][koma] == kyokumen.mochigoma_list[sengo][koma]);
    }

    inline void kyokumen_t::init()
    {
        ban.init();
        for (mochigoma_t & m : mochigoma_list)
            m.init();
        tesu = 0;
        for (sengo_t sengo : sengo_list)
            ou_pos[sengo] = default_ou_pos_list[sengo];
        for (auto & k : oute_list)
            k.clear();
        while (hash_stack.size())
            hash_stack.pop();
        hash_stack.push(make_hash());
        kifu.clear();
        init_move_table_list();
    }

    inline bool kyokumen_t::can_promote(koma_t koma, pos_t dst)
    {
        if ((is_promoted(koma)) || trim_sengo(koma) == kin || trim_sengo(koma) == ou)
            return false;
        if (to_sengo(koma))
            return dst >= width * (6 + padding_height);
        return dst < width * (3 + padding_height);
    }

    inline bool kyokumen_t::must_promote(koma_t koma, pos_t dst)
    {
        if (trim_sengo(koma) == fu || trim_sengo(koma) == kyo)
        {
            if (to_sengo(koma))
                return dst >= width * (8 + padding_height);
            return dst < (width * (1 + padding_height));
        }
        else if (trim_sengo(koma) == kei)
        {
            if (to_sengo(koma))
                return dst >= width * (7 + padding_height);
            return dst < width * (2 + padding_height);
        }
        return false;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_far_destination(OutputIterator result, pos_t src, pos_t offset) const
    {
        for (pos_t cur = src + offset; !ban_t::out(cur); cur += offset)
        {
            if (ban[cur] == empty)
                *result++ = cur;
            else
            {
                if (to_sengo(ban[src]) == to_sengo(ban[cur])) break;
                *result++ = cur;
                if (to_sengo(ban[src]) != to_sengo(ban[cur])) break;
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_near_destination(OutputIterator result, pos_t src, pos_t offset) const
    {
        pos_t cur = src + offset;
        if (!ban_t::out(cur) && (ban[cur] == empty || to_sengo(ban[cur]) != to_sengo(ban[src])))
            *result++ = cur;
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_destination(OutputIterator result, pos_t src, sengo_t sengo) const
    {
        koma_t koma = trim_sengo(ban[src]);
        pos_t r = reverse(sengo);
        for (const pos_t * offset = far_move_offsets(koma); *offset; ++offset)
            search_far_destination(result, src, *offset * r);
        for (const pos_t * offset = near_move_offsets(koma); *offset; ++offset)
            search_near_destination(result, src, *offset * r);
    }

    inline bool kyokumen_t::can_put(koma_t koma, pos_t dst)
    {
        if (ban[dst] != empty)
            return false;
        if (sengo())
        {
            if ((koma == fu || koma == kyo) && dst >= width * 8)
                return false;
            if (koma == kei && dst >= width * 7)
                return false;
        }
        if ((koma == fu || koma == kyo) && dst < width)
            return false;
        if (koma == kei && dst < width * 2)
            return false;
        if (koma == fu)
        {
            pos_t suji = pos_to_suji(dst);
            for (pos_t dan = 0; dan < height; ++dan)
            {
                koma_t cur = ban[suji_dan_to_pos(suji, dan)];
                if (cur != empty && trim_sengo(cur) == fu && sengo() == to_sengo(cur))
                    return false;
            }

            // �ł����l��
            pos_t pos = dst + front * (reverse(sengo()));
            if (!ban_t::out(pos) && ban[pos] != empty && trim_sengo(ban[pos]) == ou && to_sengo(ban[pos]) != sengo())
            {
                te_t te{ dst, koma };
                std::vector<te_t> te_list;
                {
                    VALIDATE_KYOKUMEN_ROLLBACK(*this);
                    do_te(te);
                    search_te(std::back_inserter(te_list));
                    undo_te(te);
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
        pos_t cur;
        for (cur = pos + offset; !ban_t::out(cur) && ban[cur] == empty; cur += offset);
        if (ban_t::out(cur))
            return npos;
        return cur;
    }
    
    template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_koma_near(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform)
    {
        if (pos_t cur = pos + offset; !ban_t::out(cur) && ban[cur] != empty)
            if (is_collected(to_sengo(ban[cur])) && std::find(first, last, trim_sengo(ban[cur])) != last)
                *result++ = transform(cur, offset, false);
    }

    template<typename OutputIterator, typename InputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_koma_far(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform)
    {
        if (pos_t found = search(pos + offset, offset); found != npos && ban[found] != empty)
            if (is_collected(to_sengo(ban[found])) && std::find(first, last, trim_sengo(ban[found])) != last)
                *result++ = transform(found, offset, true);
    }

    template<typename OutputIterator, typename IsCollected, typename Transform>
    inline void kyokumen_t::search_koma(OutputIterator result, pos_t pos, sengo_t sengo, IsCollected is_collected, Transform transform)
    {
        pos_t r = reverse(sengo);
        for (auto & [offset, koma_list] : near_kiki_list)
            search_koma_near(result, pos, offset * r, koma_list.begin(), koma_list.end(), is_collected, transform);
        for (auto & [offset, koma_list] : far_kiki_list_synmmetric)
            search_koma_far(result, pos, offset, koma_list.begin(), koma_list.end(), is_collected, transform);
        for (auto & [offset, koma_list] : far_kiki_list_asynmmetric)
            search_koma_far(result, pos, offset * r, koma_list.begin(), koma_list.end(), is_collected, transform);
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_himo(OutputIterator result, pos_t pos, sengo_t sengo)
    {
        search_koma(result, pos, sengo,
            [sengo](sengo_t g) { return g == sengo; },
            [](pos_t pos, pos_t offset, bool aigoma) -> pos_t { return pos; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_kiki(OutputIterator result, pos_t pos, sengo_t sengo)
    {
        search_koma(result, pos, sengo,
            [sengo](sengo_t g) { return g != sengo; },
            [](pos_t pos, pos_t offset, bool aigoma) -> kiki_t { return { pos, offset, aigoma }; });
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_kiki_or_himo(OutputIterator result, pos_t pos, sengo_t sengo)
    {
        search_koma(result, pos, sengo,
            [](sengo_t) { return true; },
            [](pos_t pos, pos_t offset, bool aigoma) -> pos_t { return pos; });
    }

    inline void kyokumen_t::update_oute()
    {
        for (sengo_t sengo : sengo_list)
        {
            oute_list[sengo].clear();
            search_kiki(std::back_inserter(oute_list[sengo]), ou_pos[sengo], static_cast<sengo_t>(sengo));
        }
    }

    inline void kyokumen_t::search_aigoma(aigoma_info_t & aigoma_info, sengo_t sengo)
    {
        using pair = std::pair<pos_t, std::vector<koma_t>>;
        static const std::vector<pair> table
        {
            { front, { kyo, hi, ryu } },
            { left, { hi, ryu } },
            { right, { hi, ryu } },
            { back, { hi, ryu } },
            { front_left, { kaku, uma } },
            { front_right, { kaku, uma } },
            { back_left, { kaku, uma } },
            { back_right, { kaku, uma } },
        };

        pos_t ou_pos = this->ou_pos[sengo ? 1 : 0];
        pos_t r = reverse(sengo);
        for (const auto & [o, hashirigoma_list] : table)
        {
            pos_t offset = o * r;
            pos_t first = search(ou_pos, offset);
            if (first != npos && to_sengo(ban[first]) == sengo)
            {
                pos_t second = search(first, offset);
                if (second != npos && to_sengo(ban[second]) != sengo)
                {
                    koma_t kind = trim_sengo(ban[second]);
                    bool match = std::find(hashirigoma_list.begin(), hashirigoma_list.end(), kind) != hashirigoma_list.end();
                    if (match)
                    {
                        std::vector<pos_t> candidates;
                        for (pos_t candidate = second; candidate != ou_pos; candidate -= offset)
                            candidates.push_back(candidate);
                        aigoma_info[first] = std::move(candidates);
                    }
                }
            }
        }
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_te_from_positions(OutputIterator result, pos_t source, pos_t destination)
    {
        if (can_promote(ban[source], destination))
            *result++ = { source, destination, ban[source], ban[destination], true };
        if (!must_promote(ban[source], destination))
            *result++ = { source, destination, ban[source], ban[destination], false };
    }

    template<typename OutputIterator>
    inline void kyokumen_t::search_te(OutputIterator result)
    {
        aigoma_info_t aigoma_info;
        search_aigoma(aigoma_info, sengo());

        if (oute_list[sengo()].empty())
        {
#ifdef DISABLE_MOVEMENT_CACHE
            std::vector<pos_t> source_list;
            search_source(std::back_inserter(source_list), sengo());
            for (auto source : source_list)
            {
                std::vector<pos_t> destination_list;
                search_destination(std::back_inserter(destination_list), source, sengo());
#else
            for (auto & [source, destination_list] : move_table_list[sengo()])
            {
#endif
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
                        print_te(kifu.begin(), kifu.end());
                        SHOGIPP_ASSERT(false);
                    }
#endif

                    // ����͗����͈̔͂ɂ����ړ��ł��Ȃ��B
                    if (is_aigoma)
                    {
                        const std::vector<pos_t> & candidates = aigoma_iter->second;
                        if (std::find(candidates.begin(), candidates.end(), destination) == candidates.end())
                            continue;
                    }

                    // �����Ă���ꏊ�ɉ����ړ������Ă͂Ȃ�Ȃ�
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

            for (koma_t koma = fu; koma <= hi; ++koma)
            {
                if (mochigoma_list[sengo()][koma])
                    for (pos_t dst = 0; dst < pos_size; ++dst)
                        if (can_put(koma, dst))
                            *result++ = { dst, koma };
            }
            }
        else // ���肳��Ă���ꍇ
        {
            pos_t r = reverse(sengo());
            pos_t src = ou_pos[sengo()];

            // �����ړ����ĉ���������ł�������������B
            for (const pos_t * p = near_move_offsets(ou); *p; ++p)
            {
                pos_t dst = src + *p * r;
                if (!ban_t::out(dst)
                    && (ban[dst] == empty || to_sengo(ban[dst]) != to_sengo(ban[src])))
                {
                    te_t te{ src, dst, ban[src], ban[dst], false };
                    std::vector<kiki_t> kiki;
                    {
                        VALIDATE_KYOKUMEN_ROLLBACK(*this);
                        do_te(te);
                        search_kiki(std::back_inserter(kiki), dst, !sengo());
                        undo_te(te);
                    }
                    if (kiki.empty())
                        *result++ = te;
                }
            }

            auto & oute = oute_list[sengo()];
            if (oute.size() == 1)
            {
                // ����̎����������B
                if (oute.front().aigoma)
                {
                    pos_t offset = oute.front().offset;
                    for (pos_t dst = ou_pos[sengo()] + offset; !ban_t::out(dst) && ban[dst] == empty; dst += offset)
                    {
                        // ����ړ������鍇��
                        std::vector<kiki_t> kiki_list;
                        search_kiki(std::back_inserter(kiki_list), dst, !sengo());
                        for (kiki_t & kiki : kiki_list)
                        {
                            // ���ō���͂ł��Ȃ��B
                            if (trim_sengo(ban[kiki.pos]) != ou)
                            {
                                // ���ɍ���Ƃ��Ďg���Ă����͈ړ��ł��Ȃ��B
                                auto aigoma_iter = aigoma_info.find(kiki.pos);
                                bool is_aigoma = aigoma_iter != aigoma_info.end();
                                if (is_aigoma)
                                    continue;
                                search_te_from_positions(result, kiki.pos, dst);
                            }
                        }

                        // ���ł���
                        for (koma_t koma = fu; koma <= hi; ++koma)
                            if (mochigoma_list[sengo()][koma])
                                if (can_put(koma, dst))
                                    *result++ = { dst, koma };
                    }

                    // ���肵�Ă������������������B
                    pos_t dst = oute.front().pos;
                    std::vector<kiki_t> kiki;
                    search_kiki(std::back_inserter(kiki), dst, !sengo());
                    for (auto & k : kiki)
                    {
                        // ���𓮂�����͊��Ɍ����ς�
                        if (trim_sengo(ban[k.pos]) != ou)
                        {
                            // ���ɍ���Ƃ��Ďg���Ă����͈ړ��ł��Ȃ��B
                            auto aigoma_iter = aigoma_info.find(k.pos);
                            bool is_aigoma = aigoma_iter != aigoma_info.end();
                            if (is_aigoma)
                                continue;
                            search_te_from_positions(result, k.pos, dst);
                        }
                    }
                }
            }
        }
    }

    inline hash_t kyokumen_t::make_hash() const
    {
        hash_t hash = 0;

        // �Տ�̋�̃n�b�V���l��XOR���Z
        for (pos_t pos = 0; pos < pos_size; ++pos)
            if (!ban_t::out(pos))
                if (koma_t koma = ban[pos]; koma != empty)
                    hash ^= hash_table.koma_hash(koma, pos);

        // ������̃n�b�V���l��XOR���Z
        for (std::size_t i = 0; i < std::size(mochigoma_list); ++i)
            for (koma_t koma = fu; koma <= hi; ++koma)
                hash ^= hash_table.mochigoma_hash(koma, mochigoma_list[i][koma], tesu_to_sengo(i));

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
        return hash;
    }

    inline void kyokumen_t::print_te(const te_t & te, sengo_t sengo) const
    {
        std::cout << (sengo == sente ? "��" : "��");
        if (te.is_uchite())
        {
            std::cout << suji_to_string(pos_to_suji(te.destination())) << dan_to_string(pos_to_dan(te.destination())) << koma_to_string(trim_sengo(te.source_koma())) << "��";
        }
        else
        {
            const char * naristr;
            if (can_promote(te.source_koma(), te.destination()))
                naristr = te.promote() ? "��" : "�s��";
            else
                naristr = "";
            std::cout << suji_to_string(pos_to_suji(te.destination())) << dan_to_string(pos_to_dan(te.destination())) << koma_to_string(trim_sengo(te.source_koma())) << naristr
                << "�i" << suji_to_string(pos_to_suji(te.source())) << dan_to_string(pos_to_dan(te.source())) << "�j";
        }
    }

    template<typename InputIterator>
    inline void kyokumen_t::print_te(InputIterator first, InputIterator last) const
    {
        for (std::size_t i = 0; first != last; ++i)
        {
            std::printf("#%3d ", i + 1);
            print_te(*first++, sengo());
            std::cout << std::endl;
        }
    }

    inline void kyokumen_t::print_te()
    {
        std::vector<te_t> te;
        search_te(std::back_inserter(te));
        print_te(te.begin(), te.end());
    }

    inline void kyokumen_t::print_oute()
    {
        for (sengo_t sengo : sengo_list)
        {
            auto & oute = oute_list[sengo];
            if (!oute.empty())
            {
                std::cout << "����F";
                for (std::size_t i = 0; i < oute_list[sengo].size(); ++i)
                {
                    kiki_t & kiki = oute[i];
                    if (i > 0)
                        std::cout << "�@";
                    print_pos(kiki.pos);
                    std::cout << koma_to_string(trim_sengo(ban[kiki.pos])) << std::endl;
                }
            }
        }
    }

    inline void kyokumen_t::print()
    {
        std::cout << "��莝����F";
        mochigoma_list[1].print();
        ban.print();
        std::cout << "��莝����F";
        mochigoma_list[0].print();
    }

    inline void kyokumen_t::print_kifu()
    {
        for (tesu_t tesu = 0; tesu < static_cast<tesu_t>(kifu.size()); ++tesu)
        {
            print_te(kifu[tesu], tesu_to_sengo(tesu));
            std::cout << std::endl;
        }
    }

    inline hash_t kyokumen_t::hash() const
    {
        return hash_stack.top();
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
        hash_t hash = make_hash(hash_stack.top(), te);
        if (te.is_uchite())
        {
            SHOGIPP_ASSERT(mochigoma_list[sengo()][te.source_koma()] > 0);
            ban[te.destination()] = sengo() ? to_gote(te.source_koma()) : te.source_koma();
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
                ou_pos[sengo()] = te.destination();
        }
        ++tesu;
        hash_stack.push(hash);
        update_oute();

#ifndef DISABLE_MOVEMENT_CACHE
        do_updating_move_table_list(te);
#endif

#ifdef VALIDATE_MOVEMENT_CACHE
        validate_move_table();
#endif
        kifu.push_back(te);
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
                ou_pos[sengo()] = te.source();
            ban[te.source()] = te.source_koma();
            ban[te.destination()] = te.captured_koma();
            if (te.captured_koma() != empty)
                --mochigoma_list[sengo()][te.captured_koma()];
        }
        hash_stack.pop();
        update_oute();

#ifndef DISABLE_MOVEMENT_CACHE
        undo_updating_move_table_list(te);
#endif

#ifdef VALIDATE_MOVEMENT_CACHE
        validate_move_table();
#endif

        kifu.pop_back();
    }

    inline sengo_t kyokumen_t::sengo() const
    {
        return tesu_to_sengo(tesu);
    }

    inline void kyokumen_t::read_kyokumen_file(std::filesystem::path kyokumen_file)
    {
        static const std::string mochigoma_string = "������F";
        static const std::string nothing_string = "�Ȃ�";
        static const std::string tesu_suffix = "���";
        static const std::string sengo_suffix = "��";

        std::ifstream stream{ kyokumen_file };
        std::string line;
        pos_t dan = 0;

        kyokumen_t temp_kyokumen;
        temp_kyokumen.init();

        try
        {
            while (std::getline(stream, line))
            {
                if (line.empty() || (!line.empty() && line.front() == '#'))
                    continue;

                std::string_view rest = line;
                if (line.size() >= 1 && std::isdigit(line[0]))
                {
                    tesu_t temp_tesu = 0;
                    do
                    {
                        temp_tesu *= 10;
                        temp_tesu += rest[0] - '0';
                        rest.remove_prefix(1);
                    } while (rest.size() >= 1 && std::isdigit(rest[0]));
                    if (temp_tesu == 0)
                        throw file_format_error{ "read_kyokumen_file 1-1" };
                    --temp_tesu;

                    parse(rest, tesu_suffix);

                    sengo_t sengo = parse(rest, sengo_string_map, sengo_string_size);

                    if (tesu_to_sengo(temp_tesu) != sengo)
                        throw file_format_error{ "read_kyokumen_file 1-6" };

                    parse(rest, sengo_suffix);

                    tesu = temp_tesu;
                }
                else if (line.size() >= 1 && line[0] == '|')
                {
                    rest.remove_prefix(1);
                    for (pos_t suji = 0; suji < suji_size; ++suji)
                    {
                        sengo_t sengo = parse(rest, sengo_string_map, sengo_string_size);
                        koma_t koma = parse(rest, koma_string_map, koma_string_size);
                        if (sengo == gote)
                            koma = to_gote(koma);
                        ban[suji_dan_to_pos(suji, dan)] = koma;
                    }
                    ++dan;
                }
                else if (line.size() >= sengo_string_size)
                {
                    sengo_t sengo = parse(rest, sengo_string_map, sengo_string_size);
                    parse(rest, mochigoma_string);

                    if (rest.size() >= nothing_string.size() && rest == nothing_string)
                        continue;

                    while (true)
                    {
                        unsigned char count = 1;
                        koma_t koma = parse(rest, koma_string_map, koma_string_size);
                        if (koma < fu)
                            throw file_format_error{ "read_kyokumen_file 2-1" };
                        if (koma > hi)
                            throw file_format_error{ "read_kyokumen_file 2-2" };

                        if (rest.size() >= digit_string_size)
                        {
                            unsigned char digit = 0;
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
                        if (count > 18)
                            throw file_format_error{ "read_kyokumen_file 2-3" };

                        mochigoma_list[sengo][koma] = count;
                    }
                }
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

    inline void kyokumen_t::read_kifu_file(std::filesystem::path kifu_file)
    {
        static constexpr std::string_view source_position_prefix = "�i";
        static constexpr std::string_view source_position_suffix = "�j";
        static constexpr std::string_view uchite_string = "��";
        static constexpr std::string_view promote_string = "��";
        static constexpr std::string_view nonpromote_string = "�s��";

        std::ifstream stream{ kifu_file };
        std::string line;
        
        kyokumen_t temp_kyokumen;
        temp_kyokumen.init();

        try
        {
            while (std::getline(stream, line))
            {
                if (line.empty() || (!line.empty() && line.front() == '#'))
                    continue;

                std::string_view rest = line;
                sengo_t sengo = parse(rest, sengo_mark_map, sengo_mark_size);
                pos_t destination_suji = parse(rest, suji_string_map, suji_string_size);
                pos_t destination_dan = parse(rest, dan_string_map, dan_string_size);
                pos_t destination = suji_dan_to_pos(destination_suji, destination_dan);
                koma_t koma = parse(rest, koma_string_map, koma_string_size);

                if (sengo != temp_kyokumen.sengo())
                    throw file_format_error{ "read_kifu_file 1" };

                if (temp_kyokumen.sengo() == gote)
                    koma = to_gote(koma);

                bool promote = false;
                if (rest.size() >= uchite_string.size()
                    && rest.substr(0, uchite_string.size()) == uchite_string)
                {
                    rest.remove_prefix(uchite_string.size());
                    if (koma < fu || koma >hi)
                        throw file_format_error{ "read_kifu_file 2" };
                    if (!rest.empty())
                        throw file_format_error{ "read_kifu_file 3" };
                    te_t te{ destination, koma };
                    do_te(te);
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
                    pos_t source_suji = parse(rest, suji_string_map, suji_string_size);
                    pos_t source_dan = parse(rest, dan_string_map, dan_string_size);
                    pos_t source = suji_dan_to_pos(source_suji, source_dan);
                    if (rest.size() >= source_position_suffix.size()
                        && rest.substr(0, source_position_suffix.size()) == source_position_suffix)
                    {
                        rest.remove_prefix(source_position_suffix.size());
                        if (!rest.empty())
                            throw file_format_error{ "read_kifu_file 4" };
                        te_t te{ source, destination, temp_kyokumen.ban[source], temp_kyokumen.ban[destination], promote };
                        do_te(te);
                        continue;
                    }
                }

                throw file_format_error{ "read_kifu_file 5" };
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

    inline void kyokumen_t::update_move_table(move_table_t & move_table, pos_t source, std::vector<pos_t> && destination_list)
    {
        SHOGIPP_ASSERT(!ban_t::out(source));
        SHOGIPP_ASSERT(ban[source] != empty);
        if (destination_list.empty())
            move_table.erase(source);
        else
            move_table[source] = std::move(destination_list);
    }

    inline void kyokumen_t::update_move_table_relative_to(pos_t source)
    {
        std::vector<pos_t> kiki_or_himo_list;
        search_kiki_or_himo(std::back_inserter(kiki_or_himo_list), source, sengo());
        if (kiki_or_himo_list.size())
        {
            for (pos_t pos : kiki_or_himo_list)
            {
                sengo_t sengo = to_sengo(ban[pos]);
                std::vector<pos_t> new_destination_list;
                search_destination(std::back_inserter(new_destination_list), pos, sengo);
                auto & move_table = move_table_list[sengo];
                update_move_table(move_table, pos, std::move(new_destination_list));
            }
        }
    }

    inline void kyokumen_t::do_updating_move_table_list(const te_t & te)
    {
        SHOGIPP_ASSERT(tesu > 0);
        sengo_t sengo = tesu_to_sengo(tesu - 1);
        auto & self_move_table = move_table_list[sengo];
        auto & nonself_move_table = move_table_list[sengo];

        if (te.is_uchite())
        {
            // ��̈ړ�����ړ����Ƃ��鎩���̎���X�V����B
            std::vector<pos_t> new_destination_list;
            search_destination(std::back_inserter(new_destination_list), te.destination(), sengo);
            update_move_table(self_move_table, te.destination(), std::move(new_destination_list));

            // ��̈ړ���ɗ����Ă��鑖���̈ړ�����X�V����B
            update_move_table_relative_to(te.destination());
        }
        else
        {
            // ��̈ړ�����ړ����Ƃ��鎩���̎���X�V����B
            std::vector<pos_t> new_destination_list;
            search_destination(std::back_inserter(new_destination_list), te.destination(), sengo);
            update_move_table(self_move_table, te.destination(), std::move(new_destination_list));

            // ��̈ړ���ɋ�������ꍇ�A��̈ړ�����ړ����Ƃ��鑊��̎���폜����B
            if (te.captured_koma() != empty)
                nonself_move_table.erase(te.destination());

            // ��̈ړ������ړ����Ƃ��鎩���̎���폜����B
            self_move_table.erase(te.source());

            // ��̈ړ������邢�͈ړ���ɗ����Ă��邠�邢�͕R��t���Ă����̈ړ�����X�V����B
            update_move_table_relative_to(te.source());
            update_move_table_relative_to(te.destination());
        }
    }

    inline void kyokumen_t::undo_updating_move_table_list(const te_t & te)
    {
        auto & self_move_table = move_table_list[sengo()];
        auto & nonself_move_table = move_table_list[!sengo()];

        if (te.is_uchite())
        {
            // ��̈ړ�����ړ����Ƃ��鎩���̎���폜����B
            self_move_table.erase(te.destination());

            // ��̈ړ���ɗ����Ă��鑖���̈ړ�����X�V����B
            update_move_table_relative_to(te.destination());
        }
        else
        {
            // ��̈ړ�����ړ����Ƃ��鎩���̎���폜����B
            self_move_table.erase(te.destination());

            // ��̈ړ���ɋ�������ꍇ�A��̈ړ�����ړ����Ƃ��鑊��̎���X�V����B
            if (te.captured_koma() != empty)
            {
                std::vector<pos_t> new_destination_list;
                search_destination(std::back_inserter(new_destination_list), te.destination(), !sengo());
                update_move_table(nonself_move_table, te.destination(), std::move(new_destination_list));
            }

            // ��̈ړ������ړ����Ƃ��鎩���̎���X�V����B
            std::vector<pos_t> new_destination_list;
            search_destination(std::back_inserter(new_destination_list), te.source(), sengo());
            update_move_table(self_move_table, te.source(), std::move(new_destination_list));

            // ��̈ړ������邢�͈ړ���ɗ����Ă��邠�邢�͕R��t���Ă����̈ړ�����X�V����B
            update_move_table_relative_to(te.source());
            update_move_table_relative_to(te.destination());
        }
    }

    /**
     * @breif �]���֐��I�u�W�F�N�g�̃C���^�[�t�F�[�X
     */
    class abstract_evaluator_t
    {
    public:
        virtual ~abstract_evaluator_t() {};

        /**
         * @breif �ǖʂɑ΂��č��@���I������B
         * @param kyokumen �ǖ�
         * @return �I�����ꂽ���@��
         */
        virtual te_t select_te(kyokumen_t & kyokumen) = 0;

        /**
         * @breif �]���֐��I�u�W�F�N�g�̖��O��Ԃ��B
         * @return �]���֐��I�u�W�F�N�g�̖��O
         */
        virtual const char * name() = 0;
    };

    /**
     * @breif �΋�
     */
    class taikyoku_t
    {
    public:
        /**
         * @breif �΋ǂ�����������B
         */
        inline void init();

        /**
         * @breif �΋ǂ����s����B
         * @retval true �΋ǂ��I�����Ă��Ȃ�
         * @retval false �΋ǂ��I������
         * @details ���̊֐��� true ��Ԃ����ꍇ�A�ēx���̊֐����Ăяo���B
         */
        inline bool procedure();

        std::shared_ptr<abstract_evaluator_t> evaluators[sengo_size];
        kyokumen_t kyokumen;
        bool sente_win;
    };

    inline void taikyoku_t::init()
    {
        kyokumen.init();
    }

    inline bool taikyoku_t::procedure()
    {
        auto & evaluator = evaluators[kyokumen.sengo()];

        if (kyokumen.tesu == 0)
        {
            for (sengo_t sengo : sengo_list)
                std::cout << sengo_to_string(static_cast<sengo_t>(sengo)) << "�F" << evaluators[sengo]->name() << std::endl;
            std::cout << std::endl;
        }

        std::vector<te_t> te_list;
        kyokumen.search_te(std::back_inserter(te_list));
        if (te_list.empty())
        {
            auto & winner_evaluator = evaluators[!kyokumen.sengo()];
            std::cout << kyokumen.tesu << "��l��" << std::endl;
            kyokumen.print();
            std::cout << sengo_to_string(!kyokumen.sengo()) << "�����i" << winner_evaluator->name() << "�j";
            std::cout.flush();
            return false;
        }
        else
        {
            std::cout << (kyokumen.tesu + 1) << "���" << sengo_to_string(kyokumen.sengo()) << "��" << std::endl;
            kyokumen.print();
            kyokumen.print_te();
            kyokumen.print_oute();
        }

        kyokumen_t temp_kyokumen = kyokumen;
        te_t selected_te = evaluator->select_te(temp_kyokumen);

        kyokumen.print_te(selected_te, kyokumen.sengo());
        std::cout << std::endl << std::endl;

        kyokumen.do_te(selected_te);

        return true;
    }

    /**
     * @breif �΋ǂ���B
     * @tparam Evaluator1 abstract_evaluator_t ���p�������N���X
     * @tparam Evaluator2 abstract_evaluator_t ���p�������N���X
     */
    template<typename Evaluator1, typename Evaluator2>
    inline void do_taikyoku()
    {
        std::chrono::system_clock::time_point begin, end;
        begin = std::chrono::system_clock::now();

        taikyoku_t taikyoku{ { std::make_shared<Evaluator1>(), std::make_shared<Evaluator2>() } };
        taikyoku.init();
        while (taikyoku.procedure());

        end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        unsigned long long sps = (unsigned long long)total_search_count * 1000 / duration;

        std::cout
            << std::endl << std::endl
            << "���v�ǂݎ萔: " << total_search_count << std::endl
            << "���s����[ms]: " << duration << std::endl
            << "�ǂݎ葬�x[��/s]: " << sps;
        std::cout.flush();
    }

    void print_help()
    {
        std::cout
            << ""
            ;
    }

    template<typename Callback>
    void parse_program_options(int argc, const char ** argv, Callback && callback)
    {
        std::map<char, std::vector<std::string>> params;
        constexpr char quotations[] = { '\'', '"' };

        char current_option{};
        for (int i = 0; i < argc; ++i)
        {
            unsigned int count = 0;
            if (argv[i][0] == '-')
            {
                const char * p = argv[i] + 1;
                char option = *p;
                while (std::isalpha(*p))
                {
                    params[*p];
                    ++p;
                    ++count;
                }
                if (count == 1)
                    current_option = option;
                else
                    current_option = char{};
            }
            else if (current_option)
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
                        params[current_option].emplace_back(begin, p);
                    }
                    else
                    {
                        const char * begin = p;
                        while (*p && !std::isspace(*p))
                            ++p;
                        params[current_option].emplace_back(begin, p);
                    }
                }
            }
        }

        for (auto & [option, param_list] : params)
            callback(option, param_list);
    }

    void parse_command_line(int argc, const char ** argv)
    {
        auto callback = [](char option, auto && param_list)
        {
            std::cout << option << ":" << std::endl;
            for (auto && p : param_list)
                std::cout << "    " << p << std::endl;
        };
        parse_program_options(argc, argv, callback);
    }

    /**
     * @breif ��Ɖ��l�̘A�z�z�񂩂�ǖʂ̓_�����v�Z����B
     * @param kyokumen �ǖ�
     * @param map []���Z�q�ɂ���牿�l��A�z����I�u�W�F�N�g
     * @return �ǖʂ̓_��
     */
    template<typename MapKomaInt>
    inline int kyokumen_map_evaluation_value(kyokumen_t & kyokumen, MapKomaInt & map)
    {
        int score = 0;

        for (pos_t pos = 0; pos < pos_size; ++pos)
        {
            koma_t koma = kyokumen.ban[pos];
            if (!ban_t::out(pos) && koma != empty)
                score += map[trim_sengo(koma)] * reverse(to_sengo(koma));
        }

        for (sengo_t sengo : sengo_list)
            for (koma_t koma = fu; koma <= hi; ++koma)
                score += map[koma] * kyokumen.mochigoma_list[sengo][koma] * reverse(tesu_to_sengo(sengo));

        return score;
    }

    using evaluated_te = std::pair<te_t *, int>;

    /**
     * @breif ���@��𓾓_�ɂ����ёւ���B
     * @param first scored_te �̐擪���w�������_���A�N�Z�X�C�e���[�^
     * @param last scored_te �̖������w�������_���A�N�Z�X�C�e���[�^
     * @param sengo ��肩��肩
     * @details �]���l�̕�������Ԃɂ��ύX����悤�ɂ������߁A���݂��̊֐����g�p����\��͂Ȃ��B
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
     * @breif ���@��𓾓_�ɂ����ёւ���B
     * @param first scored_te �̐擪���w�������_���A�N�Z�X�C�e���[�^
     * @param last scored_te �̖������w�������_���A�N�Z�X�C�e���[�^
     * @param sengo ��肩��肩
     */
    template<typename RandomAccessIterator>
    void sort_te_by_evaluation_value(RandomAccessIterator first, RandomAccessIterator last)
    {
        std::sort(first, last, [](const evaluated_te & a, const evaluated_te & b) -> bool { return a.second > b.second; });
    }

    /**
     * @breif negamax �ō��@���I������]���֐��I�u�W�F�N�g�̒��ۃN���X
     */
    class negamax_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        int negamax(
            kyokumen_t & kyokumen,
            int depth,
            unsigned int & search_count,
            std::optional<te_t> & selected_te
        )
        {
            if (depth <= 0)
            {
                ++search_count;
                return eval(kyokumen) * reverse(kyokumen.sengo());
            }

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));

            if (te_list.empty())
                return -std::numeric_limits<int>::max();

            std::vector<evaluated_te> evaluated_te_list;
            auto inserter = std::back_inserter(evaluated_te_list);

            for (te_t & te : te_list)
            {
                std::optional<te_t> selected_te_;
                int evaluation_value;
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
         * @breif �ǖʂɑ΂��� minimax �ō��@���I������B
         * @param kyokumen �ǖ�
         * @return �I�����ꂽ���@��
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            unsigned int search_count = 0;
            int default_max_depth = 3;
            std::optional<te_t> selected_te;
            int evaluation_value = negamax(kyokumen, default_max_depth, search_count, selected_te);
            total_search_count += search_count;
            std::cout << "�ǂݎ萔�F" << search_count << std::endl;
            std::cout << "�]���l�F" << evaluation_value << std::endl;
            SHOGIPP_ASSERT(selected_te.has_value());
            return *selected_te;
        }

        /**
         * @breif �ǖʂɑ΂��ĕ]���l��Ԃ��B
         * @param kyokumen �ǖ�
         * @return �ǖʂ̕]���l
         */
        virtual int eval(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif alphabeta �ō��@���I������]���֐��I�u�W�F�N�g�̒��ۃN���X
     */
    class alphabeta_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        int alphabeta(
            kyokumen_t & kyokumen,
            int depth,
            int alpha,
            int beta,
            unsigned int & search_count,
            std::optional<te_t> & selected_te)
        {
            if (depth <= 0)
            {
                ++search_count;
                return eval(kyokumen) * reverse(kyokumen.sengo());
            }

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));

            if (te_list.empty())
                return -std::numeric_limits<int>::max();

            std::vector<evaluated_te> evaluated_te_list;
            auto inserter = std::back_inserter(evaluated_te_list);

            for (te_t & te : te_list)
            {
                std::optional<te_t> selected_te_;
                int evaluation_value;
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
         * @breif �ǖʂɑ΂��� minimax �ō��@���I������B
         * @param kyokumen �ǖ�
         * @return �I�����ꂽ���@��
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            unsigned int search_count = 0;
            int default_max_depth = 3;
            std::optional<te_t> selected_te;
            int score = alphabeta(kyokumen, default_max_depth, -std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), search_count, selected_te);
            total_search_count += search_count;
            std::cout << "�ǂݎ萔�F" << search_count << std::endl;
            std::cout << "�]���l�F" << score << std::endl;
            SHOGIPP_ASSERT(selected_te.has_value());
            return *selected_te;
        }

        /**
         * @breif �ǖʂɑ΂��ĕ]���l��Ԃ��B
         * @param kyokumen �ǖ�
         * @return �ǖʂ̕]���l
         */
        virtual int eval(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif alphabeta �ō��@���I������]���֐��I�u�W�F�N�g�̒��ۃN���X
     * @details �O����肪�������Ă����ꍇ�A�T������������B
     */
    class extendable_alphabeta_evaluator_t
        : public abstract_evaluator_t
    {
    public:
        int extendable_alphabeta(
            kyokumen_t & kyokumen,
            int depth,
            int alpha,
            int beta,
            unsigned int & search_count,
            std::optional<te_t> & selected_te,
            pos_t previous_destination)
        {
            if (depth <= 0)
            {
                // �O����肪�������Ă����ꍇ�A�T������������B
                if (previous_destination != npos)
                {
                    std::vector<evaluated_te> evaluated_te_list;
                    auto inserter = std::back_inserter(evaluated_te_list);
                    std::vector<te_t> te_list;
                    kyokumen.search_te(std::back_inserter(te_list));
                    for (te_t & te : te_list)
                    {
                        if (!te.is_uchite() && te.destination() == previous_destination)
                        {
                            std::optional<te_t> selected_te_;
                            int evaluation_value;
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

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));

            if (te_list.empty())
                return -std::numeric_limits<int>::max();

            std::vector<evaluated_te> evaluated_te_list;
            auto inserter = std::back_inserter(evaluated_te_list);

            for (te_t & te : te_list)
            {
                std::optional<te_t> selected_te_;
                pos_t destination = (!te.is_uchite() && te.captured_koma() != empty) ? te.destination() : npos;
                int evaluation_value;
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
         * @breif �ǖʂɑ΂��� minimax �ō��@���I������B
         * @param kyokumen �ǖ�
         * @return �I�����ꂽ���@��
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            unsigned int search_count = 0;
            int default_max_depth = 3;
            std::optional<te_t> selected_te;
            int evaluation_value = extendable_alphabeta(kyokumen, default_max_depth, -std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), search_count, selected_te, npos);
            total_search_count += search_count;
            std::cout << "�ǂݎ萔�F" << search_count << std::endl;
            std::cout << "�]���l�F" << evaluation_value << std::endl;
            SHOGIPP_ASSERT(selected_te.has_value());
            return *selected_te;
        }

        /**
         * @breif �ǖʂɑ΂��ĕ]���l��Ԃ��B
         * @param kyokumen �ǖ�
         * @return �ǖʂ̕]���l
         */
        virtual int eval(kyokumen_t & kyokumen) = 0;
    };

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
        te_t select_te(kyokumen_t & kyokumen) override
        {
            std::vector<te_t> te;
            kyokumen.search_te(std::back_inserter(te));

            std::vector<evaluated_te> scores;
            auto back_inserter = std::back_inserter(scores);
            for (te_t & t : te)
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
         * @breif �ǖʂɑ΂��ĕ]���l��Ԃ��B
         * @param kyokumen �ǖ�
         * @return �ǖʂ̕]���l
         */
        virtual int eval(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif �]���֐��I�u�W�F�N�g�̎�����
     */
    class sample_evaluator_t
        : public alphabeta_evaluator_t
    {
    public:
        int eval(kyokumen_t & kyokumen) override
        {
            static const int map[]
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

            int score = kyokumen_map_evaluation_value(kyokumen, map);
            return score;
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
        int eval(kyokumen_t & kyokumen) override
        {
            static const int map[]
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

            int score = 0;
            score += kyokumen_map_evaluation_value(kyokumen, map);
            return score;
        }

        const char * name() override
        {
            return "�Ђ悱10��";
        }
    };

    class niwatori_evaluator_t
        : public alphabeta_evaluator_t
    {
    public:
        int eval(kyokumen_t & kyokumen) override
        {
            static const int map[]
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

            int score = 0;
            score += kyokumen_map_evaluation_value(kyokumen, map);

            return score;
        }

        const char * name() override
        {
            return "�ɂ�Ƃ�9��";
        }
    };

    class niwatori_dou_evaluator_t
        : public extendable_alphabeta_evaluator_t
    {
    public:
        int eval(kyokumen_t & kyokumen) override
        {
            static const int map[]
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

            int score = 0;
            score += kyokumen_map_evaluation_value(kyokumen, map);

            return score;
        }

        const char * name() override
        {
            return "�ɂ�Ƃ蓺8��";
        }
    };

    /**
     * @breif �]���֐��I�u�W�F�N�g�̎�����
     */
    class random_evaluator_t
        : public max_evaluator_t
    {
    public:
        inline int eval(kyokumen_t & kyokumen) override
        {
            return uid(rand);
        }

        const char * name() override
        {
            return "random evaluator";
        }

        std::minstd_rand rand{ SHOGIPP_SEED };
        std::uniform_int_distribution<int> uid{ std::numeric_limits<int>::min(), std::numeric_limits<int>::max() };
    };

} // namespace shogipp

#endif // SHOGIPP_DEFINED