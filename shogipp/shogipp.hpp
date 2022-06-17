#include <iostream>

#include <cstdlib>
#include <ctime>
#include <vector>
#include <utility>
#include <algorithm>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <random>
#include <limits>
#include <stack>

#define NDEBUG

#ifdef NDEBUG
#define SHOGIPP_ASSERT(expr)
#else
#define SHOGIPP_ASSERT(expr) do { shogipp::assert_impl((expr), #expr, __FILE__, __func__, __LINE__); } while (false)
#endif

#define XY_TO_ROW_POS(x, y) ((y + padding_height) * width + x + padding_width)
#define XY_TO_POS(x, y) (y * 9 + x)

namespace shogipp
{
    inline void assert_impl(bool assertion, const char * expr, const char * file, const char * func, unsigned int line)
    {
        if (!assertion)
        {
            std::ostringstream what;
            what << "SHOGIPP_ASSERTion failed: " << expr << ", file " << file << ", line " << line;
            std::cerr << what.str() << std::endl;
            std::terminate();
        }
    }

    using koma_t = unsigned char;
    enum : koma_t
    {
        empty,
        fu, kyo, kei, gin, kin, kaku, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu,
        sente_fu = fu, sente_kyo, sente_kei, sente_gin, sente_kin, sente_kaku, sente_hi, sente_ou, sente_tokin, sente_nari_kyo, sente_nari_kei, sente_nari_gin, sente_uma, sente_ryu,
        gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, gote_uma, gote_ryu,
        koma_enum_number,
        x = 0xff
    };

    using pos_t = int;
    constexpr pos_t npos = -1;

    enum : pos_t
    {
        width = 11,
        height = 13,
        padding_width = 1,
        padding_height = 2
    };


    inline pos_t pos_to_dan(pos_t pos)
    {
        return pos / width - padding_height;
    }

    inline pos_t pos_to_suji(pos_t pos)
    {
        return pos % width - padding_width;
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

    using tesu_t = unsigned int;
    
    inline bool is_goteban(tesu_t tesu)
    {
        return tesu % 2 != 0;
    }

    using hash_t = std::size_t;

    inline bool is_promoted(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static bool map[]{
            false,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true
        };
        return map[koma];
    }

    inline bool is_promotable(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static bool map[]{
            false,
            true, true, true, true, false, true, true, false, false, false, false, false, false, false,
            true, true, true, true, false, true, true, false, false, false, false, false, false, false,
        };
        return map[koma];
    }

    /*
     * @breif ����̋���肷��B
     * @param koma ��
     * @return ���̋�ł���ꍇ true
     */
    inline bool is_sente(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static bool map[]{
            false,
            true, true, true, true, false, true, true, true, true, true, true, true, true, true,
            false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        };
        return map[koma];
    }

    /*
     * @breif ����̋���肷��B
     * @param koma ��
     * @return ���̋�ł���ꍇ true
     */
    inline bool is_gote(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static bool map[]{
            false,
            false, false, false, false, false, false, false, false, false, false, false, false, false, false,
            true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        };
        return map[koma];
    }

    /*
     * @breif ������(���E�p�E��E�n�E��)�����肷��B
     * @param koma ��
     * @return �����ł���ꍇ true
     */
    inline bool is_hasirigoma(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static bool map[]{
            false,
            false, true, false, false, false, true, true, false, false, false, false, false, true, true,
            false, true, false, false, false, true, true, false, false, false, false, false, true, true,
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
        constexpr static koma_t map[]{
            0,
            fu, kyo, kei, gin, kin, kaku, hi, ou, fu, kyo, kei, gin, kaku, hi,
            fu, kyo, kei, gin, kin, kaku, hi, ou, fu, kyo, kei, gin, kaku, hi,
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
        constexpr static koma_t map[]{
            0,
            tokin, nari_kyo, nari_kei, nari_gin, 0, uma, ryu, 0, 0, 0, 0, 0, 0, 0,
            tokin, nari_kyo, nari_kei, nari_gin, 0, uma, ryu, 0, 0, 0, 0, 0, 0, 0,
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
        constexpr static koma_t map[]{
            0,
            fu, kyo, kei, gin, kin, kaku, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu,
            fu, kyo, kei, gin, kin, kaku, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu,
        };
        return map[koma];
    }

    /*
     * @breif ������̋�ɕϊ�����B
     * @param koma ��
     * @return ���̋�
     * @details FU -> GOTE_FU
     */
    inline koma_t to_gote(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static koma_t map[]{
            0,
            gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, gote_uma, gote_ryu,
            gote_fu, gote_kyo, gote_kei, gote_gin, gote_kin, gote_kaku, gote_hi, gote_ou, gote_tokin, gote_nari_kyo, gote_nari_kei, gote_nari_gin, gote_uma, gote_ryu,
        };
        return map[koma];
    }
    
    static const struct hash_table_t
    {
        hash_table_t()
        {
            std::minstd_rand rand;
            std::uniform_int_distribution<hash_t> uid{ std::numeric_limits<hash_t>::min(), std::numeric_limits<hash_t>::max() };
            for (std::size_t i = 0; i < std::size(ban_table); ++i)
                ban_table[i] = uid(rand);
            for (std::size_t i = 0; i < std::size(mochigoma_table); ++i)
                mochigoma_table[i] = uid(rand);
        }
        hash_t ban_table[koma_enum_number * 9 * 9];
        hash_t mochigoma_table[(18 + 4 + 4 + 4 + 4 + 2 + 2) * 2 * 2];

        /**
         * @breif �Տ�̋�̃n�b�V���l���v�Z����B
         * @param koma ��
         * @param pos ��̍��W
         * @return �n�b�V���l
         */
        inline hash_t koma_hash(koma_t koma, pos_t pos) const
        {
            std::size_t index = koma;
            index *= 9;
            index += pos_to_suji(pos);
            index *= 9;
            index += pos_to_dan(pos);
            return ban_table[index];
        }

        /**
         * @breif ������̃n�b�V���l���v�Z����B
         * @param koma ��
         * @param count ��̐�
         * @param is_gote ���̎����
         * @return �n�b�V���l
         */
        inline hash_t mochigoma_hash(koma_t koma, std::size_t count, bool is_gote) const
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

            SHOGIPP_ASSERT(!(koma == fu && count > fu_max));
            SHOGIPP_ASSERT(!(koma == kyo && count > kyo_max));
            SHOGIPP_ASSERT(!(koma == kei && count > kei_max));
            SHOGIPP_ASSERT(!(koma == gin && count > gin_max));
            SHOGIPP_ASSERT(!(koma == kin && count > kin_max));
            SHOGIPP_ASSERT(!(koma == kaku && count > kaku_max));
            SHOGIPP_ASSERT(!(koma == hi && count > hi_max));

            static const std::size_t map[]
            {
                fu_offset  ,
                kyo_offset ,
                kei_offset ,
                gin_offset ,
                kin_offset ,
                kaku_offset,
                hi_offset  ,
            };

            std::size_t index = map[koma];
            index *= size;
            index += count;
            index *= 2;
            if (is_gote)
                ++index;
            return ban_table[index];
        }
    } hash_table;

    inline const char * tebanstr(tesu_t tesu)
    {
        const char * map[]{ "���", "���" };
        return map[tesu % 2];
    }

    inline const char * numberstr(koma_t koma) {
        const char * map[]{ "�O", "�P", "�Q", "�R", "�S", "�T", "�U", "�V", "�W", "�X" };
        return map[koma];
    }
    
    inline const pos_t * near_move_offsets(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        static const pos_t map[][10]
        {
            /* empty    */ { },
            /* fu       */ { front },
            /* kyo      */ { },
            /* kei      */ { kei_left, kei_right},
            /* gin      */ { front_left, front, front_right, back_left, back_right },
            /* kin      */ { front_left, front, front_right, left, right, back },
            /* kaku     */ { },
            /* hi       */ { },
            /* ou       */ { front_left, front, front_right, left, right, back_left, back, back_right },
            /* tokin    */ { front_left, front, front_right, left, right, back },
            /* kaku_kyo */ { front_left, front, front_right, left, right, back },
            /* kaku_kei */ { front_left, front, front_right, left, right, back },
            /* kaku_gin */ { front_left, front, front_right, left, right, back },
            /* uma      */ { front, left, right, back },
            /* ryu      */ { front_left, front_right, back_left, back_right },
        };
        return map[koma];
    }

    inline const pos_t * far_move_offsets(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        static const pos_t map[][10]
        {
            /* empty    */ { },
            /* fu       */ { },
            /* kyo      */ { front },
            /* kei      */ { },
            /* gin      */ { },
            /* kin      */ { },
            /* kaku     */ { front_left, front_right, back_left, back_right },
            /* hi       */ { front, left, right, back },
            /* ou       */ { },
            /* tokin    */ { },
            /* kaku_kyo */ { },
            /* kaku_kei */ { },
            /* kaku_gin */ { },
            /* uma      */ { front_left, front_right, back_left, back_right },
            /* ryu      */ { front, left, right, back },
        };
        return map[koma];
    }

    static const bool kin_nari[]{ false, true, true, true, true, false, false, false, false };

    inline const char * to_string(koma_t koma)
    {
        SHOGIPP_ASSERT(koma < koma_enum_number);
        static const char * map[]{
            "�E",
            "��", "��", "�j", "��", "��", "�p", "��", "��", "��", "��", "�\", "�S", "�n", "��",
            "��", "��", "�j", "��", "��", "�p", "��", "��", "��", "��", "�\", "�S", "�n", "��",
        };
        return map[koma];
    }

    inline const char * danstr(pos_t pos)
    {
        static const char * map[]{ "��", "��", "�O", "�l", "��", "�Z", "��", "��", "��" };
        return map[pos];
    }

    inline const char * sujistr(pos_t pos)
    {
        static const char * map[]{ "�X", "�W", "�V", "�U", "�T", "�S", "�R", "�Q", "�P" };
        return map[pos];
    }

    inline pos_t suji_dan_to_pos(pos_t suji, pos_t dan)
    {
        return width * (dan + padding_height) + suji + padding_width;
    }

    void print_pos(pos_t pos)
    {
        std::cout << sujistr(pos_to_suji(pos)) << danstr(pos_to_dan(pos));
        std::cout.flush();
    }

    /**
     * @breif ���@��
     */
    struct te_t
    {
        pos_t src;      // �ړ����̍��W(src == npos �̏ꍇ�A�������ł�)
        pos_t dst;      // �ړ���̍��W(src == npos �̏ꍇ�A dst �͑ł��W)
        koma_t srckoma; // �ړ����̋�(src == npos �̏ꍇ�A srckoma �͑ł�����)
        koma_t dstkoma; // �ړ���̋�(src == npos �̏ꍇ�A dstkoma �͖���`)
        bool promote;   // ����ꍇ true
    };

    /**
     * @breif ������
     */
    struct mochigoma_t
    {
        unsigned char count[hi - fu + 1];

        /**
         * @breif �����������������B
         */
        inline void init()
        {
            std::fill(std::begin(count), std::end(count), 0);
        }

        inline void print() const
        {
            unsigned int kind = 0;
            for (koma_t koma = hi; koma >= fu; --koma)
            {
                if ((*this)[koma] > 0)
                {
                    std::cout << to_string(koma);
                    if ((*this)[koma] >= 2)
                        std::cout << numberstr((*this)[koma]);
                    ++kind;
                }
            }
            if (kind == 0)
                std::cout << "�Ȃ�";
            std::cout << std::endl;
        }

        /**
         * @breif ��ƑΉ����鎝����̐��̎Q�Ƃ�Ԃ��B
         * @param ��
         * @return ��ƑΉ����鎝����̐��̎Q��
         */
        inline unsigned char & operator [](koma_t koma)
        {
            SHOGIPP_ASSERT(trim_sengo(koma) != empty);
            SHOGIPP_ASSERT(trim_sengo(koma) != ou);
            static const std::size_t map[]{
                0,
                fu - fu, kyo - fu, kei - fu, gin - fu, kin - fu, kaku - fu, hi - fu, 0,
                fu - fu, kyo - fu, kei - fu, gin - fu, kaku - fu, hi - fu
            };
            return count[map[trim_sengo(koma)]];
        }

        inline const unsigned char & operator [](koma_t koma) const
        {
            return (*const_cast<mochigoma_t*>(this))[koma];
        }
    };

    /**
     * @breif ��
     */
    struct ban_t
    {
        /**
         * @breif �Ղ�����������B
         */
        inline void init()
        {
            static const koma_t temp[]{
                x, x, x, x, x, x, x, x, x, x, x,
                x, x, x, x, x, x, x, x, x, x, x,
                x, gote_kyo, gote_kei, gote_gin, gote_kin, gote_ou, gote_kin, gote_gin, gote_kei, gote_kyo, x,
                x, empty, gote_hi, empty, empty, empty, empty, empty, gote_kaku, empty, x,
                x, gote_fu, gote_fu, gote_fu, gote_fu, gote_fu, gote_fu, gote_fu, gote_fu, gote_fu, x,
                x, empty, empty, empty, empty, empty, empty, empty, empty, empty, x,
                x, empty, empty, empty, empty, empty, empty, empty, empty, empty, x,
                x, empty, empty, empty, empty, empty, empty, empty, empty, empty, x,
                x, sente_fu, sente_fu, sente_fu, sente_fu, sente_fu, sente_fu, sente_fu, sente_fu, sente_fu, x,
                x, empty, sente_kaku, empty, empty, empty, empty, empty, sente_hi, empty, x,
                x, sente_kyo, sente_kei, sente_gin, sente_kin, sente_ou, sente_kin, sente_gin, sente_kei, sente_kyo, x,
                x, x, x, x, x, x, x, x, x, x, x,
                x, x, x, x, x, x, x, x, x, x, x,
            };
            std::copy(std::begin(temp), std::end(temp), std::begin(data));
        }

        inline koma_t & operator [](size_t i) { return data[i]; }
        inline const koma_t & operator [](size_t i) const { return data[i]; }

        /**
         * @breif ���Wpos���ՊO�����肷��B
         * @param pos ���W
         * @return �ՊO�̏ꍇtrue
         */
        inline static bool out(pos_t pos)
        {
#define _ empty
            static const koma_t table[]{
                x, x, x, x, x, x, x, x, x, x, x,
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
                x, x, x, x, x, x, x, x, x, x, x,
            };
#undef _
            return pos < 0 || pos >= width * height || table[pos] == x;
        }

        /**
         * @breif �Ղ�W���o�͂ɏo�͂���B
         */
        inline void print() const
        {
            std::cout << "  �X �W �V �U �T �S �R �Q �P" << std::endl;
            std::cout << "+---------------------------+" << std::endl;
            for (pos_t dan = 0; dan < 9; ++dan)
            {
                std::cout << "|";
                for (pos_t suji = 0; suji < 9; ++suji)
                {
                    koma_t koma = data[suji_dan_to_pos(suji, dan)];
                    std::cout << (is_gote(koma) ? "v" : " ") << to_string(koma);
                }
                std::cout << "| " << danstr(dan) << std::endl;
            }
            std::cout << "+---------------------------+" << std::endl;
        }

        koma_t data[width * height];
    };

    /**
     * @breif ����
     */
    struct kiki_t
    {
        pos_t offset;     // �����̑��΍��W
        pos_t pos;        // �����Ă����̍��W
        bool aigoma;    // ����\��
    };

    using aigoma_info_t = std::unordered_map<pos_t, std::vector<pos_t>>;

    /**
     * @breif �ǖ�
     */
    struct kyokumen_t
    {
        /**
         * @breif �ǖʂ�����������B
         */
        inline void init()
        {
            ban.init();
            for (mochigoma_t & m : mochigoma_list)
                m.init();
            tesu = 0;
            ou_pos[0] = suji_dan_to_pos(4, 8);
            ou_pos[1] = suji_dan_to_pos(4, 0);
            for (auto & k : oute_list)
                k.clear();
            while (hash_stack.size())
                hash_stack.pop();
            hash_stack.push(make_hash());
        }

        /**
         * @breif ��koma�����Wdst�Ɉړ�����ꍇ�ɐ��肪�\�����肷��B
         * @param koma ��
         * @param dst �ړ���̍��W
         * @return ���肪�\�̏ꍇ(koma�����ɐ����Ă���ꍇ�A���false)
         */
        inline static bool can_promote(koma_t koma, pos_t dst)
        {
            if ((is_promoted(koma)) || trim_sengo(koma) == kin || trim_sengo(koma) == ou)
                return false;
            if (is_gote(koma))
                return dst >= width * (6 + padding_height);
            return dst < width * (3 + padding_height);
        }

        /**
         * @breif ��koma�����Wdst�Ɉړ�����ꍇ�ɐ��肪�K�{�����肷��B
         * @param koma ��
         * @param dst �ړ���̍��W
         * @return ���肪�K�{�̏ꍇ(koma�����ɐ����Ă���ꍇ�A���false)
         */
        inline static bool must_promote(koma_t koma, pos_t dst)
        {
            if (trim_sengo(koma) == fu || trim_sengo(koma) == kyo)
            {
                if (is_gote(koma))
                    return dst >= width * (8 + padding_height);
                return dst < (width * (1 + padding_height));
            }
            else if (trim_sengo(koma) == kei)
            {
                if (is_gote(koma))
                    return dst >= width * (7 + padding_height);
                return dst < width * (2 + padding_height);
            }
            return false;
        }

        /**
         * @breif ���Wsrc����ړ��\�̈ړ���𔽕��I�Ɍ�������B
         * @param result �ړ���̍��W�̏o�̓C�e���[�^
         * @param src �ړ����̍��W
         * @param offset �ړ���̑��΍��W
         */
        template<typename OutputIterator>
        void find_far_dst(OutputIterator result, pos_t src, pos_t offset) const
        {
            pos_t cur = src + offset;
            for (; !ban_t::out(cur); cur += offset)
            {
                if (ban[cur] == empty)
                    *result++ = cur;
                else
                {
                    if (is_gote(ban[src]) == is_gote(ban[cur])) break;
                    *result++ = cur;
                    if (is_gote(ban[src]) != is_gote(ban[cur])) break;
                }
            }
        }

        /**
         * @breif ���Wsrc����ړ��\�̈ړ����񔽕��I�Ɍ�������B
         * @param result �ړ���̍��W�̏o�̓C�e���[�^
         * @param src �ړ����̍��W
         * @param offset �ړ���̑��΍��W
         */
        template<typename OutputIterator>
        void find_near_dst(OutputIterator result, pos_t src, pos_t offset) const
        {
            pos_t cur = src + offset;
            if (!ban_t::out(cur) && (ban[cur] == empty || is_gote(ban[cur]) != is_gote(ban[src])))
                *result++ = cur;
        }

        /**
         * @breif ���Wsrc����ړ��\�̈ړ������������B
         * @param result �ړ���̍��W�̏o�̓C�e���[�^
         * @param src �ړ����̍��W
         */
        template<typename OutputIterator>
        void find_dst(OutputIterator result, pos_t src) const
        {
            koma_t koma = trim_sengo(ban[src]);
            pos_t reverse = is_gote(ban[src]) ? -1 : 1;
            for (const pos_t * offset = far_move_offsets(koma); *offset; ++offset)
                find_far_dst(result, src, *offset * reverse);
            for (const pos_t * offset = near_move_offsets(koma); *offset; ++offset)
                find_near_dst(result, src, *offset * reverse);
        }

        /**
         * @breif ������koma�����Wdst�ɒu�����Ƃ��ł��邩���肷��B���A���A�j�Ɍ���false��Ԃ��\��������B
         * @param koma ������
         * @param dst �ړ���̍��W
         * @return �u�����Ƃ��ł���ꍇ true
         */
        inline bool can_put(koma_t koma, pos_t dst)
        {
            if (ban[dst] != empty)
                return false;
            if (is_goteban(tesu))
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
                for (pos_t dan = 0; dan < height; ++dan)
                {
                    koma_t cur = ban[dst % width + width * dan];
                    if (trim_sengo(cur) == fu && is_goteban(tesu) == is_gote(cur))
                        return false;
                }

                // �ł����l��
                pos_t pos = dst + front * (is_goteban(tesu) ? -1 : 1);
                if (!ban_t::out(pos) && trim_sengo(ban[pos]) == ou && is_gote(ban[pos]) != is_goteban(tesu))
                {
                    te_t te{ npos, dst, koma };
                    std::vector<te_t> te_list;
                    do_te(te);
                    search_te(std::back_inserter(te_list));
                    undo_te(te);
                    if (te_list.empty())
                        return false;
                }
            }
            return true;
        }

        /**
         * @breif �ړ����̍��W����������B
         * @param result �o�̓C�e���[�^
         */
        template<typename OutputIterator>
        void find_src(OutputIterator result) const
        {
            for (pos_t i = 0; i < width * height; ++i)
                if (ban[i] != empty && ban[i] != x && is_gote(ban[i]) == is_goteban(tesu))
                    *result++ = i;
        }

        /**
         * @breif ���Wpos���瑊�΍��Woffset�����ɑ������ŏ��ɋ�������W��Ԃ��B
         * @param pos �������J�n������W
         * @param offset �������鑊�΍��W
         * @return �ŏ��ɋ�������W(�������Ȃ��ꍇ npos )
         */
        inline pos_t search(pos_t pos, pos_t offset) const
        {
            pos_t cur;
            for (cur = pos + offset; !ban_t::out(cur) && ban[cur] == empty; cur += offset);
            if (ban_t::out(cur))
                return npos;
            return cur;
        }


        /**
         * @breif ���Wpos�̋�ɑ΂���אڂ�����̗�������������B
         * @param result �����̏o�̓C�e���[�^
         * @param pos ���W
         * @param offset �����̑��΍��W
         * @param gote ��莋�_��
         * @param first ������̓��̓C�e���[�^(begin)
         * @param last ������̓��̓C�e���[�^(end)
         */
        template<typename OutputIterator, typename InputIterator>
        void search_kiki_near(OutputIterator result, pos_t pos, pos_t offset, bool gote, InputIterator first, InputIterator last)
        {
            if (pos_t cur = pos + offset; !ban_t::out(cur) && gote != is_gote(ban[cur]))
                if (std::find(first, last, trim_sengo(ban[cur])) != last)
                    *result++ = { offset, cur, false };
        }

        /**
         * @breif ���Wpos�̋�ɑ΂���אڂ��Ȃ���̗�������������B
         * @param result �����̏o�̓C�e���[�^
         * @param pos ���W
         * @param offset �����̑��΍��W
         * @param gote ��莋�_��
         * @param first ������̓��̓C�e���[�^(begin)
         * @param last ������̓��̓C�e���[�^(end)
         */
        template<typename OutputIterator, typename InputIterator>
        void search_kiki_far(OutputIterator result, pos_t pos, pos_t offset, bool gote, InputIterator first, InputIterator last)
        {
            if (pos_t found = search(pos, offset); found != npos && gote != is_gote(ban[found]))
            {
                bool adjacency = found == pos + offset;
                if (!adjacency)
                    if (std::find(first, last, trim_sengo(ban[found])) != last)
                        *result++ = { offset, found, adjacency };
            }
        }

        /**
         * @breif ���Wpos�ɑ΂��闘������������B
         * @param result �����̏o�̓C�e���[�^
         * @param pos ���W
         * @param gote ��莋�_��
         */
        template<typename OutputIterator>
        void search_kiki(OutputIterator result, pos_t pos, bool gote)
        {
            using pair = std::pair<pos_t, std::vector<koma_t>>;
            static const pair near[]
            {
                { kei_left   , { kei } },
                { kei_right  , { kei } },
                { front_left , { gin, kin, kaku, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
                { front      , { fu, kyo, gin, kin, hi, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
                { front_right, { gin, kin, kaku, ou, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
                { left       , { kin, ou, hi, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
                { right      , { kin, ou, hi, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
                { back_left  , { gin, ou, kaku, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
                { back       , { kin, ou, hi, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
                { back_right , { gin, ou, kaku, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
            };
            static const pair far[]
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

            pos_t reverse = gote ? -1 : 1;
            for (auto & [offset, koma_list] : near)
                search_kiki_near(result, pos, offset * reverse, gote, koma_list.begin(), koma_list.end());
            for (auto & [offset, koma_list] : far)
                search_kiki_far(result, pos, offset * reverse, gote, koma_list.begin(), koma_list.end());
        }

        /**
         * @breif ���ɑ΂��闘�����X�V����B
         */
        inline void update_oute()
        {
            for (std::size_t i = 0; i < 2; ++i)
            {
                oute_list[i].clear();
                search_kiki(std::back_inserter(oute_list[i]), ou_pos[i], is_goteban(i));
            }
        }

        inline void search_aigoma(aigoma_info_t & aigoma_info, bool gote)
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

            pos_t ou_pos = this->ou_pos[gote ? 1 : 0];
            pos_t reverse = gote ? -1 : 1;
            for (const auto & [o, hashirigoma_list] : table)
            {
                pos_t offset = o * reverse;
                pos_t first = search(ou_pos, offset);
                if (first != npos && is_gote(ban[first]) == gote)
                {
                    pos_t second = search(first, offset);
                    if (second != npos && is_gote(ban[second]) != gote)
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

        /**
         * @breif ���@��𐶐�����B
         * @param result ���@��̏o�̓C�e���[�^
         */
        template<typename OutputIterator>
        void search_te(OutputIterator result)
        {
            if (oute_list[tesu % 2].empty())
            {
                aigoma_info_t aigoma_info;
                search_aigoma(aigoma_info, is_goteban(tesu));

#ifndef NDEBUG
                for (auto & [pos, candidates] : aigoma_info)
                {
                    std::cout << "����F";
                    print_pos(pos);
                    std::cout << to_string(trim_sengo(ban[pos])) << std::endl;
                }
#endif

                std::vector<pos_t> found_src;
                find_src(std::back_inserter(found_src));
                for (pos_t src : found_src)
                {
                    auto aigoma_iter = aigoma_info.find(src);
                    bool is_aigoma = aigoma_iter != aigoma_info.end();

                    std::vector<pos_t> found_dst;
                    find_dst(std::back_inserter(found_dst), src);
                    for (pos_t dst : found_dst)
                    {
#ifndef NDEBUG
                        if (trim_sengo(ban[dst]) == OU)
                        {
                            te_t te{ src, dst, ban[src], ban[dst], false };
                            print_te(te);
                        }
#endif
                        SHOGIPP_ASSERT(trim_sengo(ban[dst]) != ou);

                        // ����͗����͈̔͂ɂ����ړ��ł��Ȃ��B
                        if (is_aigoma)
                        {
                            const std::vector<pos_t> & candidates = aigoma_iter->second;
                            if (std::find(candidates.begin(), candidates.end(), dst) == candidates.end())
                                continue;
                        }

                        // �����Ă���ꏊ�ɉ����ړ������Ă͂Ȃ�Ȃ�
                        if (trim_sengo(ban[src]) == ou)
                        {
                            std::vector<kiki_t> kiki_list;
                            search_kiki(std::back_inserter(kiki_list), dst, is_goteban(tesu));
                            if (kiki_list.size() > 0)
                                continue;
                        }

                        if (can_promote(ban[src], dst))
                            *result++ = { src, dst, ban[src], ban[dst], true };
                        if (!must_promote(ban[dst], dst))
                            *result++ = { src, dst, ban[src], ban[dst], false };
                    }
                }

                for (koma_t koma = fu; koma <= hi; ++koma)
                {
                    if (mochigoma_list[tesu % 2][koma])
                        for (pos_t dst = 0; dst < width * height; ++dst)
                            if (can_put(koma, dst))
                                *result++ = { npos, dst, koma };
                }
            }
            else // ���肳��Ă���ꍇ
            {
                pos_t reverse = is_goteban(tesu) ? -1 : 1;
                pos_t src = ou_pos[tesu % 2];

                // �����ړ����ĉ���������ł�������������B
                for (const pos_t * p = near_move_offsets(ou); *p; ++p)
                {
                    pos_t dst = src + *p * reverse;
                    if (!ban_t::out(dst)
                        && (ban[dst] == empty || is_gote(ban[dst]) != is_gote(ban[src])))
                    {
                        te_t te{ src, dst, ban[src], ban[dst], false };
                        do_te(te);
                        std::vector<kiki_t> kiki;
                        search_kiki(std::back_inserter(kiki), dst, !is_goteban(tesu));
                        undo_te(te);
                        if (kiki.empty())
                            *result++ = te;
                    }
                }

                auto & oute = this->oute_list[tesu % 2];
                if (oute.size() == 1)
                {
                    // ����̎����������B
                    if (oute.front().aigoma)
                    {
                        pos_t offset = oute.front().offset;
                        for (pos_t dst = ou_pos[tesu % 2] + offset; !ban_t::out(dst) && ban[dst] == empty; dst += offset)
                        {
                            // ����ړ������鍇��
                            std::vector<kiki_t> kiki;
                            search_kiki(std::back_inserter(kiki), dst, is_goteban(tesu));
                            for (auto & k : kiki)
                            {
                                if (can_promote(ban[k.pos], dst))
                                    *result++ = { k.pos, dst, ban[k.pos], ban[dst], true };
                                if (!must_promote(ban[k.pos], dst))
                                    *result++ = { k.pos, dst, ban[k.pos], ban[dst], false };
                            }

                            // ���ł���
                            for (koma_t koma = fu; koma <= hi; ++koma)
                                if (mochigoma_list[tesu % 2][koma])
                                    if (can_put(koma, dst))
                                        *result++ = { npos, dst, koma };
                        }
                    }
                    else
                    {
                        // ���肵�Ă������������������B
                        pos_t dst = oute.front().pos;
                        std::vector<kiki_t> kiki;
                        search_kiki(std::back_inserter(kiki), dst, !is_goteban(tesu));
                        for (auto & k : kiki)
                        {
                            // ���𓮂�����͊��Ɍ����ς�
                            if (trim_sengo(ban[k.pos]) != ou)
                            {
                                te_t te{ k.pos, dst, ban[k.pos], ban[dst], false };
                                std::vector<kiki_t> kiki;
                                do_te(te);
                                bool sucide = oute.size() > 0;
                                undo_te(te);

                                // ���������㉤�肳��Ă��Ă͂����Ȃ�
                                if (!sucide)
                                {
                                    if (can_promote(ban[k.pos], dst))
                                        *result++ = { k.pos, dst, ban[k.pos], ban[dst], true };
                                    if (!must_promote(ban[k.pos], dst))
                                        *result++ = { k.pos, dst, ban[k.pos], ban[dst], false };
                                }
                            }
                        }
                    }
                }
            }
        }

        /**
         * @breif �ǖʂ̃n�b�V���l���v�Z����B
         * @return �ǖʂ̃n�b�V���l
         */
        inline hash_t make_hash() const
        {
            hash_t hash = 0;

            // �Տ�̋�̃n�b�V���l��XOR���Z
            for (pos_t pos = 0; pos < width * height; ++pos)
                if (!ban_t::out(pos))
                    if (koma_t koma = ban[pos]; koma != empty)
                        hash ^= hash_table.koma_hash(koma, pos);

            // ������̃n�b�V���l��XOR���Z
            for (std::size_t i = 0; i < std::size(mochigoma_list); ++i)
                for (koma_t koma = fu; koma <= hi; ++koma)
                    hash ^= hash_table.mochigoma_hash(koma, mochigoma_list[i][koma], is_goteban(i));

            return hash;
        }

        /**
         * @breif �ǖʂ̃n�b�V���l�ƍ��@�肩��A���@������{������̋ǖʂ̃n�b�V���l���v�Z����B
         * @param hash ���@������{����O�̋ǖʂ̃n�b�V���l
         * @param te ���{���鍇�@��
         * @return ���@������{������̋ǖʂ̃n�b�V���l
         * @details ���@��ɂ�蔭�����鍷���Ɋ�Â��v�Z���邽�� make_hash() ����r�I�����ɏ��������B
         *          ���̊֐��͍��@������{������O�ɌĂяo�����K�v������B
         */
        inline hash_t make_hash(hash_t hash, const te_t & te) const
        {
            if (te.src == npos)
            {
                std::size_t mochigoma_count = mochigoma_list[tesu % 2][te.srckoma];
                SHOGIPP_ASSERT(mochigoma_count > 0);
                hash ^= hash_table.koma_hash(te.srckoma, te.dst);
                hash ^= hash_table.mochigoma_hash(te.srckoma, mochigoma_count, is_goteban(tesu));
                hash ^= hash_table.mochigoma_hash(te.srckoma, mochigoma_count - 1, is_goteban(tesu));
            }
            else
            {
                SHOGIPP_ASSERT(!(!is_promotable(te.srckoma) && te.promote));
                hash ^= hash_table.koma_hash(te.srckoma, te.src);
                if (te.dstkoma != empty)
                {
                    std::size_t mochigoma_count = mochigoma_list[tesu % 2][te.srckoma];
                    hash ^= hash_table.mochigoma_hash(te.srckoma, mochigoma_count, is_goteban(tesu));
                    hash ^= hash_table.mochigoma_hash(te.srckoma, mochigoma_count + 1, is_goteban(tesu));
                    hash ^= hash_table.koma_hash(te.dstkoma, te.dst);
                }
                hash ^= hash_table.koma_hash(te.promote ? to_unpromoted(te.srckoma) : te.srckoma, te.dst);
            }
            return hash;
        }

        /**
         * @breif ���@����o�͂���B
         * @param te ���@��
         */
        inline void print_te(const te_t & te) const
        {
            std::cout << (is_goteban(tesu) ? "��" : "��");
            if (te.src != npos)
            {
                const char * naristr;
                if (can_promote(te.srckoma, te.dst))
                    naristr = te.promote ? "��" : "�s��";
                else
                    naristr = "";
                std::cout << sujistr(pos_to_suji(te.dst)) << danstr(pos_to_dan(te.dst)) << to_string(trim_sengo(te.srckoma)) << naristr
                    << " (" << sujistr(pos_to_suji(te.src)) << danstr(pos_to_dan(te.src)) << ")" << std::endl;
            }
            else
            {
                std::cout << sujistr(pos_to_suji(te.dst)) << danstr(pos_to_dan(te.dst)) << to_string(trim_sengo(te.srckoma)) << "��" << std::endl;
            }
        }

        /**
         * @breif ���@����o�͂���B
         * @param first ���@��̓��̓C�e���[�^��begin
         * @param last ���@��̓��̓C�e���[�^��end
         */
        template<typename InputIterator>
        void print_te(InputIterator first, InputIterator last) const
        {
            for (std::size_t i = 0; first != last; ++i)
            {
                std::printf("#%3d ", i + 1);
                //std::cout << "#" << (i + 1) << " ";
                print_te(*first++);
            }
        }

        inline void print_te()
        {
            std::vector<te_t> te;
            search_te(std::back_inserter(te));
            print_te(te.begin(), te.end());
        }

        inline void print_oute()
        {
            for (std::size_t i = 0; i < std::size(oute_list); ++i)
            {
                auto & oute = oute_list[i];
                if (oute_list[i].size() > 0)
                {
                    std::cout << "����F";
                    for (std::size_t j = 0; j < oute_list[i].size(); ++j)
                    {
                        kiki_t & kiki = oute[j];
                        if (j > 0)
                            std::cout << "�@";
                        print_pos(kiki.pos);
                        std::cout << to_string(trim_sengo(ban[kiki.pos])) << std::endl;
                    }
                }
            }
        }

        /**
         * @breif �ǖʂ��o�͂���B
         */
        inline void print()
        {
            std::cout << "��莝����F";
            mochigoma_list[1].print();
            ban.print();
            std::cout << "��莝����F";
            mochigoma_list[0].print();
        }

        /**
         * @breif �ǖʂ̃n�b�V���l��Ԃ��B
         * @return �ǖʂ̃n�b�V���l
         */
        hash_t hash() const
        {
            return hash_stack.top();
        }

        /**
         * @breif ���@������s����B
         * @param te ���@��
         */
        inline void do_te(const te_t & te)
        {
            hash_t hash = make_hash(hash_stack.top(), te);
            if (te.src == npos)
            {
                SHOGIPP_ASSERT(mochigoma_list[tesu % 2][te.srckoma] > 0);
                ban[te.dst] = is_goteban(tesu) ? to_gote(te.srckoma) : te.srckoma;
                --mochigoma_list[tesu % 2][te.srckoma];
            }
            else
            {
                SHOGIPP_ASSERT(!(!is_promotable(te.srckoma) && te.promote));
                if (ban[te.dst] != empty)
                    ++mochigoma_list[tesu % 2][ban[te.dst]];
                ban[te.dst] = te.promote ? to_promoted(ban[te.src]) : ban[te.src];
                ban[te.src] = empty;
                if (trim_sengo(te.srckoma) == ou)
                    ou_pos[tesu % 2] = te.dst;
            }
            ++tesu;
            hash_stack.push(hash);
            update_oute();
        }

        /**
         * @breif ���@������s����O�ɖ߂��B
         * @param te ���@��
         */
        inline void undo_te(const te_t & te)
        {
            SHOGIPP_ASSERT(tesu > 0);
            --tesu;
            if (te.src == npos)
            {
                ++mochigoma_list[tesu % 2][te.srckoma];
                ban[te.dst] = empty;
            }
            else
            {
                if (trim_sengo(te.srckoma) == ou)
                    ou_pos[tesu % 2] = te.src;
                ban[te.src] = te.srckoma;
                ban[te.dst] = te.dstkoma;
                if (ban[te.dst] != empty)
                    --mochigoma_list[tesu % 2][ban[te.dst]];
            }
            hash_stack.pop();
            update_oute();
        }

        ban_t ban;                          // ��
        mochigoma_t mochigoma_list[2];      // ������ { ���, ��� }
        tesu_t tesu;                        // �萔
        pos_t ou_pos[2];                    // ���̍��W { ���, ��� }
        std::vector<kiki_t> oute_list[2];   // ���ɑ΂��闘�� { ���, ��� }
        std::stack<hash_t> hash_stack;      // ����܂ł̊e��Ԃɂ�����n�b�V���l���i�[����X�^�b�N
    };

    /**
     * @breif �]���֐��I�u�W�F�N�g�̃C���^�[�t�F�[�X
     */
    struct abstract_evaluator_t
    {
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

    struct game_t
    {
        std::shared_ptr<abstract_evaluator_t> evaluators[2];
        kyokumen_t kyokumen;
        bool sente_win;

        inline void init()
        {
            kyokumen.init();
        }

        inline bool procedure()
        {
            auto & evaluator = evaluators[kyokumen.tesu % 2];

            if (kyokumen.tesu == 0)
            {
                for (std::size_t i = 0; i < 2; ++i)
                    std::cout << tebanstr(static_cast<tesu_t>(i)) << "�F" << evaluators[i]->name() << std::endl;
                std::cout << std::endl;
            }

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));
            if (te_list.empty())
            {
                std::cout << kyokumen.tesu << "��l��" << std::endl;
                kyokumen.print();
                std::cout << tebanstr(kyokumen.tesu + 1) << "���� (" << evaluator->name() << ")";
                std::cout.flush();
                return false;
            }
            else
            {
                std::cout << (kyokumen.tesu + 1) << "���" << tebanstr(kyokumen.tesu) << "��" << std::endl;
                kyokumen.print();
                kyokumen.print_te();
                kyokumen.print_oute();
            }

            kyokumen_t temp_kyokumen = kyokumen;
            te_t selected_te = evaluator->select_te(temp_kyokumen);

            kyokumen.print_te(selected_te);
            std::cout << std::endl;

            kyokumen.do_te(selected_te);

            return true;
        }
    };

    /**
     * @breif �P���ɕ]���l���ł��������Ԃ��]���֐��I�u�W�F�N�g�̒��ۃN���X
     */
    struct basic_evaluator_t
        : public abstract_evaluator_t
    {
        /**
         * @breif �ǖʂɑ΂��ĕ]���l���ł������Ȃ鍇�@���I������B
         * @param kyokumen �ǖ�
         * @return �I�����ꂽ���@��
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            std::vector<te_t> te;
            kyokumen.search_te(std::back_inserter(te));

            using pair = std::pair<te_t *, int>;
            std::vector<pair> scores;
            auto back_inserter = std::back_inserter(scores);
            for (te_t & t : te)
            {
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
    struct sample_evaluator_t
        : public basic_evaluator_t
    {
        int eval(kyokumen_t & kyokumen) override
        {
            static const int komascore[]{ 5, 30, 35, 55, 60, 95, 100, 0xFFFF };
            static const int pkomascore[]{ 60, 60, 60, 60, 0, 115, 120, 0 };
            int score = 0;
            for (int i = 0; i < width * height; ++i)
            {
                koma_t koma = kyokumen.ban[i];
                pos_t reverse = is_gote(koma) == is_goteban(kyokumen.tesu) ? 1 : -1;
                if (koma != empty)
                {
                    if (is_promoted(koma))
                        score += pkomascore[trim_sengo(koma) - fu] * reverse;
                    else
                        score += komascore[trim_sengo(koma) - fu] * reverse;
                }
            }
            for (int i = fu; i <= hi; ++i)
            {
                mochigoma_t * m0 = &kyokumen.mochigoma_list[kyokumen.tesu % 2];
                mochigoma_t * m1 = &kyokumen.mochigoma_list[(kyokumen.tesu + 1) % 2];
                score += komascore[i] * ((*m0)[i] - (*m1)[i]);
            }
            return score;
        }

        const char * name() override
        {
            return "sample evaluator";
        }
    };

    /**
     * @breif �]���֐��I�u�W�F�N�g�̎�����
     */
    struct random_evaluator_t
        : public basic_evaluator_t
    {
        inline int eval(kyokumen_t & kyokumen) override
        {
            return std::rand();
        }

        const char * name() override
        {
            return "random evaluator";
        }
    };

} // namespace shogipp