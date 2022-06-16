#include <iostream>

#include <cstdlib>
#include <ctime>
#include <vector>
#include <utility>
#include <algorithm>
#include <memory>
#include <sstream>
#include <unordered_map>

#define NDEBUG

#ifdef NDEBUG
#define SHOGIPP_ASSERT(expr)
#else
#define SHOGIPP_ASSERT(expr) do { shogipp::assert_impl((expr), #expr, __FILE__, __func__, __LINE__); } while (false)
#endif

#define XY_TO_ROW_POS(x, y) ((y + PADDING_H) * W + x + PADDING_W)
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
    enum : koma_t {
        EMPTY,
        FU, KYO, KEI, GIN, KIN, KAKU, HI, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU,
        SENTE_FU = FU, SENTE_KYO, SENTE_KEI, SENTE_GIN, SENTE_KIN, SENTE_KAKU, SENTE_HI, SENTE_OU, SENTE_NARI_FU, SENTE_NARI_KYO, SENTE_NARI_KEI, SENTE_NARI_GIN, SENTE_UMA, SENTE_RYU,
        GOTE_FU, GOTE_KYO, GOTE_KEI, GOTE_GIN, GOTE_KIN, GOTE_KAKU, GOTE_HI, GOTE_OU, GOTE_NARI_FU, GOTE_NARI_KYO, GOTE_NARI_KEI, GOTE_NARI_GIN, GOTE_UMA, GOTE_RYU,
        KOMA_ENUM_NUMBER,
        X = 0xFF
    };
    enum { W = 11, H = 13, PADDING_W = 1, PADDING_H = 2 };

    using pos_t = int;
    constexpr pos_t npos = -1;

    constexpr pos_t FRONT = -W;
    constexpr pos_t LEFT = -1;
    constexpr pos_t RIGHT = 1;
    constexpr pos_t BACK = W;
    constexpr pos_t KEI_LEFT = FRONT * 2 + LEFT;
    constexpr pos_t KEI_RIGHT = FRONT * 2 + RIGHT;
    constexpr pos_t FRONT_LEFT = FRONT + LEFT;
    constexpr pos_t FRONT_RIGHT = FRONT + RIGHT;
    constexpr pos_t BACK_LEFT = BACK + LEFT;
    constexpr pos_t BACK_RIGHT = BACK + RIGHT;

    using tesu_t = unsigned int;
    
    inline bool is_goteban(tesu_t tesu)
    {
        return tesu % 2 != 0;
    }

    using hash_t = std::int32_t;

    inline bool is_promoted(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != EMPTY);
        constexpr static bool map[]{
            false,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true
        };
        return map[koma];
    }

    inline bool is_promotable(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != EMPTY);
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
        SHOGIPP_ASSERT(koma != EMPTY);
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
        SHOGIPP_ASSERT(koma != EMPTY);
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
        SHOGIPP_ASSERT(koma != EMPTY);
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
        SHOGIPP_ASSERT(koma != EMPTY);
        constexpr static koma_t map[]{
            0,
            FU, KYO, KEI, GIN, KIN, KAKU, HI, OU, FU, KYO, KEI, GIN, KAKU, HI,
            FU, KYO, KEI, GIN, KIN, KAKU, HI, OU, FU, KYO, KEI, GIN, KAKU, HI,
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
            NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, 0, UMA, RYU, 0, 0, 0, 0, 0, 0, 0,
            NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, 0, UMA, RYU, 0, 0, 0, 0, 0, 0, 0,
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
        SHOGIPP_ASSERT(koma != EMPTY);
        constexpr static koma_t map[]{
            0,
            FU, KYO, KEI, GIN, KIN, KAKU, HI, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU,
            FU, KYO, KEI, GIN, KIN, KAKU, HI, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU,
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
        SHOGIPP_ASSERT(koma != EMPTY);
        constexpr static koma_t map[]{
            0,
            GOTE_FU, GOTE_KYO, GOTE_KEI, GOTE_GIN, GOTE_KIN, GOTE_KAKU, GOTE_HI, GOTE_OU, GOTE_NARI_FU, GOTE_NARI_KYO, GOTE_NARI_KEI, GOTE_NARI_GIN, GOTE_UMA, GOTE_RYU,
            GOTE_FU, GOTE_KYO, GOTE_KEI, GOTE_GIN, GOTE_KIN, GOTE_KAKU, GOTE_HI, GOTE_OU, GOTE_NARI_FU, GOTE_NARI_KYO, GOTE_NARI_KEI, GOTE_NARI_GIN, GOTE_UMA, GOTE_RYU,
        };
        return map[koma];
    }
    
    static const struct hash_table_t
    {
        hash_table_t()
        {
            std::srand((unsigned int)std::time(nullptr));
            for (std::size_t i = 0; i < std::size(rand); ++i)
                rand[i] = std::rand();
        }
        hash_t rand[64 * 9 * 9];
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
        SHOGIPP_ASSERT(koma != EMPTY);
        static const pos_t map[][10]
        {
            /* EMPTY    */ { },
            /* FU       */ { FRONT },
            /* KYO      */ { },
            /* KEI      */ { KEI_LEFT, KEI_RIGHT},
            /* GIN      */ { FRONT_LEFT, FRONT, FRONT_RIGHT, BACK_LEFT, BACK_RIGHT },
            /* KIN      */ { FRONT_LEFT, FRONT, FRONT_RIGHT, LEFT, RIGHT, BACK },
            /* KAKU     */ { },
            /* HI       */ { },
            /* OU       */ { FRONT_LEFT, FRONT, FRONT_RIGHT, LEFT, RIGHT, BACK_LEFT, BACK, BACK_RIGHT },
            /* NARI_FU  */ { FRONT_LEFT, FRONT, FRONT_RIGHT, LEFT, RIGHT, BACK },
            /* KAKU_KYO */ { FRONT_LEFT, FRONT, FRONT_RIGHT, LEFT, RIGHT, BACK },
            /* KAKU_KEI */ { FRONT_LEFT, FRONT, FRONT_RIGHT, LEFT, RIGHT, BACK },
            /* KAKU_GIN */ { FRONT_LEFT, FRONT, FRONT_RIGHT, LEFT, RIGHT, BACK },
            /* UMA      */ { FRONT, LEFT, RIGHT, BACK },
            /* RYU      */ { FRONT_LEFT, FRONT_RIGHT, BACK_LEFT, BACK_RIGHT },
        };
        return map[koma];
    }

    inline const pos_t * far_move_offsets(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != EMPTY);
        static const pos_t map[][10]
        {
            /* EMPTY    */ { },
            /* FU       */ { },
            /* KYO      */ { FRONT },
            /* KEI      */ { },
            /* GIN      */ { },
            /* KIN      */ { },
            /* KAKU     */ { FRONT_LEFT, FRONT_RIGHT, BACK_LEFT, BACK_RIGHT },
            /* HI       */ { FRONT, LEFT, RIGHT, BACK },
            /* OU       */ { },
            /* NARI_FU  */ { },
            /* KAKU_KYO */ { },
            /* KAKU_KEI */ { },
            /* KAKU_GIN */ { },
            /* UMA      */ { FRONT_LEFT, FRONT_RIGHT, BACK_LEFT, BACK_RIGHT },
            /* RYU      */ { FRONT, LEFT, RIGHT, BACK },
        };
        return map[koma];
    }

    static const bool kin_nari[]{ false, true, true, true, true, false, false, false, false };

    inline pos_t pos_to_dan(pos_t pos)
    {
        return pos / W - PADDING_H;
    }

    inline pos_t pos_to_suji(pos_t pos)
    {
        return pos % W - PADDING_W;
    }

    inline const char * to_string(koma_t koma)
    {
        SHOGIPP_ASSERT(koma < KOMA_ENUM_NUMBER);
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
        return W * (dan + PADDING_H) + suji + PADDING_W;
    }

    void print_pos(pos_t pos)
    {
        std::cout << sujistr(pos_to_suji(pos)) << danstr(pos_to_dan(pos));
        std::cout.flush();
    }

    struct te_t
    {
        pos_t src, dst;
        koma_t srckoma, dstkoma;
        bool promote;
    };

    /**
     * @breif ������
     */
    struct mochigoma_t
    {
        unsigned char count[HI - FU + 1];

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
            for (koma_t koma = FU; koma <= HI; ++koma)
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
            SHOGIPP_ASSERT(trim_sengo(koma) != EMPTY);
            SHOGIPP_ASSERT(trim_sengo(koma) != OU);
            constexpr static std::size_t map[]{
                0,
                FU - FU, KYO - FU, KEI - FU, GIN - FU, KIN - FU, KAKU - FU, HI - FU, 0,
                FU - FU, KYO - FU, KEI - FU, GIN - FU, KAKU - FU, HI - FU
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
                X, X, X, X, X, X, X, X, X, X, X,
                X, X, X, X, X, X, X, X, X, X, X,
                X, GOTE_KYO, GOTE_KEI, GOTE_GIN, GOTE_KIN, GOTE_OU, GOTE_KIN, GOTE_GIN, GOTE_KEI, GOTE_KYO, X,
                X, EMPTY, GOTE_HI, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, GOTE_KAKU, EMPTY, X,
                X, GOTE_FU, GOTE_FU, GOTE_FU, GOTE_FU, GOTE_FU, GOTE_FU, GOTE_FU, GOTE_FU, GOTE_FU, X,
                X, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, X,
                X, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, X,
                X, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, X,
                X, SENTE_FU, SENTE_FU, SENTE_FU, SENTE_FU, SENTE_FU, SENTE_FU, SENTE_FU, SENTE_FU, SENTE_FU, X,
                X, EMPTY, SENTE_KAKU, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, SENTE_HI, EMPTY, X,
                X, SENTE_KYO, SENTE_KEI, SENTE_GIN, SENTE_KIN, SENTE_OU, SENTE_KIN, SENTE_GIN, SENTE_KEI, SENTE_KYO, X,
                X, X, X, X, X, X, X, X, X, X, X,
                X, X, X, X, X, X, X, X, X, X, X,
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
#define _ EMPTY
            static const koma_t table[]{
                X, X, X, X, X, X, X, X, X, X, X,
                X, X, X, X, X, X, X, X, X, X, X,
                X, _, _, _, _, _, _, _, _, _, X,
                X, _, _, _, _, _, _, _, _, _, X,
                X, _, _, _, _, _, _, _, _, _, X,
                X, _, _, _, _, _, _, _, _, _, X,
                X, _, _, _, _, _, _, _, _, _, X,
                X, _, _, _, _, _, _, _, _, _, X,
                X, _, _, _, _, _, _, _, _, _, X,
                X, _, _, _, _, _, _, _, _, _, X,
                X, _, _, _, _, _, _, _, _, _, X,
                X, X, X, X, X, X, X, X, X, X, X,
                X, X, X, X, X, X, X, X, X, X, X,
            };
#undef _
            return pos < 0 || pos >= W * H || table[pos] == X;
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

        koma_t data[W * H];
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
        }

        /**
         * @breif ��koma�����Wdst�Ɉړ�����ꍇ�ɐ��肪�\�����肷��B
         * @param koma ��
         * @param dst �ړ���̍��W
         * @return ���肪�\�̏ꍇ(koma�����ɐ����Ă���ꍇ�A���false)
         */
        inline static bool can_promote(koma_t koma, pos_t dst)
        {
            if ((is_promoted(koma)) || trim_sengo(koma) == KIN || trim_sengo(koma) == OU)
                return false;
            if (is_gote(koma))
                return dst >= W * (6 + PADDING_H);
            return dst < W * (3 + PADDING_H);
        }

        /**
         * @breif ��koma�����Wdst�Ɉړ�����ꍇ�ɐ��肪�K�{�����肷��B
         * @param koma ��
         * @param dst �ړ���̍��W
         * @return ���肪�K�{�̏ꍇ(koma�����ɐ����Ă���ꍇ�A���false)
         */
        inline static bool must_promote(koma_t koma, pos_t dst)
        {
            if (trim_sengo(koma) == FU || trim_sengo(koma) == KYO)
            {
                if (is_gote(koma))
                    return dst >= W * (8 + PADDING_H);
                return dst < (W + PADDING_H);
            }
            else if (trim_sengo(koma) == KEI)
            {
                if (is_gote(koma))
                    return dst >= W * (7 + PADDING_H);
                return dst < W * (2 + PADDING_H);
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
                if (ban[cur] == EMPTY)
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
            if (!ban_t::out(cur) && (ban[cur] == EMPTY || is_gote(ban[cur]) != is_gote(ban[src])))
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
            if (ban[dst] != EMPTY)
                return false;
            if (is_goteban(tesu))
            {
                if ((koma == FU || koma == KYO) && dst >= W * 8)
                    return false;
                if (koma == KEI && dst >= W * 7)
                    return false;
            }
            if ((koma == FU || koma == KYO) && dst < W)
                return false;
            if (koma == KEI && dst < W * 2)
                return false;
            if (koma == FU)
            {
                for (pos_t dan = 0; dan < H; ++dan)
                {
                    koma_t cur = ban[dst % W + W * dan];
                    if (trim_sengo(cur) == FU && is_goteban(tesu) == is_gote(cur))
                        return false;
                }

                // �ł����l��
                pos_t pos = dst + FRONT * (is_goteban(tesu) ? -1 : 1);
                if (!ban_t::out(pos) && (trim_sengo(ban[pos])) == OU && is_gote(ban[pos]) != is_goteban(tesu))
                {
                    te_t te{ -1, dst, koma, ban[dst] };
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
            for (pos_t i = 0; i < W * H; ++i)
                if (ban[i] != EMPTY && ban[i] != X && is_gote(ban[i]) == is_goteban(tesu))
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
            for (cur = pos + offset; !ban_t::out(cur) && ban[cur] == EMPTY; cur += offset);
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
            {
                if (std::find(first, last, trim_sengo(ban[cur])) != last)
                    *result++ = { offset, cur, false };
            }
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
                {
                    if (std::find(first, last, trim_sengo(ban[found])) != last)
                        *result++ = { offset, found, adjacency };
                }
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
                { KEI_LEFT   , { KEI } },
                { KEI_RIGHT  , { KEI } },
                { FRONT_LEFT , { GIN, KIN, KAKU, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
                { FRONT      , { FU, KYO, GIN, KIN, HI, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
                { FRONT_RIGHT, { GIN, KIN, KAKU, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
                { LEFT       , { KIN, OU, HI, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
                { RIGHT      , { KIN, OU, HI, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
                { BACK_LEFT  , { GIN, OU, KAKU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
                { BACK       , { KIN, OU, HI, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
                { BACK_RIGHT , { GIN, OU, KAKU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
            };
            static const pair far[]
            {
                { FRONT_LEFT , { KAKU, UMA } },
                { FRONT      , { KYO, HI, RYU } },
                { FRONT_RIGHT, { KAKU, UMA } },
                { LEFT       , { HI, RYU } },
                { RIGHT      , { HI, RYU } },
                { BACK_LEFT  , { KAKU, UMA } },
                { BACK       , { HI, RYU } },
                { BACK_RIGHT , { KAKU, UMA } }
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
                { FRONT, { KYO, HI, RYU } },
                { LEFT, { HI, RYU } },
                { RIGHT, { HI, RYU } },
                { BACK, { HI, RYU } },
                { FRONT_LEFT, { KAKU, UMA } },
                { FRONT_RIGHT, { KAKU, UMA } },
                { BACK_LEFT, { KAKU, UMA } },
                { BACK_RIGHT, { KAKU, UMA } },
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
         * @breif ���@����𐶐�����B
         * @param result ���@����̏o�̓C�e���[�^
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
                        SHOGIPP_ASSERT(trim_sengo(ban[dst]) != OU);

                        // ����͗����͈̔͂ɂ����ړ��ł��Ȃ��B
                        if (is_aigoma)
                        {
                            const std::vector<pos_t> & candidates = aigoma_iter->second;
                            if (std::find(candidates.begin(), candidates.end(), dst) == candidates.end())
                                continue;
                        }

                        // �����Ă���ꏊ�ɉ����ړ������Ă͂Ȃ�Ȃ�
                        if (trim_sengo(ban[src]) == OU)
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

                for (koma_t koma = FU; koma <= HI; ++koma)
                {
                    if (mochigoma_list[tesu % 2][koma])
                        for (pos_t dst = 0; dst < W * H; ++dst)
                            if (can_put(koma, dst))
                                *result++ = { npos, dst, koma, ban[dst] };
                }
            }
            else // ���肳��Ă���ꍇ
            {
                pos_t reverse = is_goteban(tesu) ? -1 : 1;
                pos_t src = ou_pos[tesu % 2];
                pos_t dst;

                // �����ړ����ĉ���������ł�������������B
                for (const pos_t * p = near_move_offsets(OU); *p; ++p)
                {
                    dst = src + *p * reverse;
                    if (!ban_t::out(dst)
                        && (ban[dst] == EMPTY || is_gote(ban[dst]) != is_gote(ban[src])))
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
                        for (pos_t dst = ou_pos[tesu % 2] + offset; !ban_t::out(dst) && ban[dst] == EMPTY; dst += offset)
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
                            for (koma_t koma = FU; koma <= HI; ++koma)
                                if (mochigoma_list[tesu % 2][koma])
                                    if (can_put(koma, dst))
                                        *result++ = { npos, dst, koma, ban[dst] };
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
                            if (trim_sengo(ban[k.pos]) != OU)
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

        inline hash_t make_hash() const
        {
            hash_t temp = 0;
            for (pos_t suji = 0; suji < 9; ++suji)
                for (pos_t dan = 0; dan < 9; ++dan)
                    temp ^= hash_table.rand[ban[XY_TO_POS(suji, dan)] * (9 * 9) + XY_TO_POS(suji, dan)];
            return temp;
        }

        inline hash_t make_hash(hash_t hash, const te_t & te) const
        {
            if (te.src == npos)
            {
                return hash;
            }
            else
            {
                return hash
                    ^ hash_table.rand[te.srckoma * (9 * 9) + te.src]
                    ^ hash_table.rand[te.dstkoma * (9 * 9) + te.dst]
                    ^ hash_table.rand[(te.promote ? to_unpromoted(te.srckoma) : te.srckoma) * (9 * 9) + te.dst];
            }
        }

        /**
         * @breif ���@������o�͂���B
         * @param te ���@����
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
         * @breif ���@������o�͂���B
         * @param first ���@����̓��̓C�e���[�^��begin
         * @param last ���@����̓��̓C�e���[�^��end
         */
        template<typename InputIterator>
        void print_te(InputIterator first, InputIterator last) const
        {
            std::size_t i = 0;
            while (first != last)
            {
                std::cout << "#" << (i + 1) << " ";
                print_te(*first++);
                ++i;
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
         * @breif ���@��������s����B
         * @param te ���@����
         */
        inline void do_te(const te_t & te)
        {
            if (te.src == npos)
            {
                SHOGIPP_ASSERT(mochigoma_list[tesu % 2][te.srckoma] > 0);
                ban[te.dst] = is_goteban(tesu) ? to_gote(te.srckoma) : te.srckoma;
                --mochigoma_list[tesu % 2][te.srckoma];
            }
            else
            {
                SHOGIPP_ASSERT(!(trim_sengo(te.srckoma) == KIN && te.promote));
                SHOGIPP_ASSERT(!(trim_sengo(te.srckoma) == OU && te.promote));
                SHOGIPP_ASSERT(!(is_promoted(te.srckoma) && te.promote));
                if (ban[te.dst] != EMPTY)
                    ++mochigoma_list[tesu % 2][ban[te.dst]];
                ban[te.dst] = te.promote ? to_promoted(ban[te.src]) : ban[te.src];
                ban[te.src] = EMPTY;
                if (trim_sengo(te.srckoma) == OU)
                    ou_pos[tesu % 2] = te.dst;
            }
            ++tesu;
            update_oute();
        }

        /**
         * @breif ���@��������s����O�ɖ߂��B
         * @param te ���@����
         */
        inline void undo_te(const te_t & te)
        {
            SHOGIPP_ASSERT(tesu > 0);
            --tesu;
            if (te.src == npos)
            {
                ++mochigoma_list[tesu % 2][te.srckoma];
                ban[te.dst] = EMPTY;
            }
            else
            {
                if (trim_sengo(te.srckoma) == OU)
                    ou_pos[tesu % 2] = te.src;
                ban[te.src] = te.srckoma;
                ban[te.dst] = te.dstkoma;
                if (ban[te.dst] != EMPTY)
                    --mochigoma_list[tesu % 2][ban[te.dst]];
            }
            update_oute();
        }

        ban_t ban;                          // ��
        mochigoma_t mochigoma_list[2];      // ������ { ���, ��� }
        tesu_t tesu;                        // �萔
        pos_t ou_pos[2];                      // ���̍��W { ���, ��� }
        std::vector<kiki_t> oute_list[2];   // ���ɑ΂��闘�� { ���, ��� }
    };

    /**
     * @breif �]���֐��I�u�W�F�N�g�̃C���^�[�t�F�[�X
     */
    struct abstract_evaluator_t
    {
        virtual ~abstract_evaluator_t() {};

        /**
         * @breif �ǖʂɑ΂��č��@�����I������B
         * @param kyokumen �ǖ�
         * @return �I�����ꂽ���@����
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
         * @breif �ǖʂɑ΂��ĕ]���l���ł������Ȃ鍇�@�����I������B
         * @param kyokumen �ǖ�
         * @return �I�����ꂽ���@����
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
            for (int i = 0; i < W * H; ++i)
            {
                koma_t koma = kyokumen.ban[i];
                pos_t reverse = is_gote(koma) == is_goteban(kyokumen.tesu) ? 1 : -1;
                if (koma != EMPTY)
                {
                    if (is_promoted(koma))
                        score += pkomascore[trim_sengo(koma) - FU] * reverse;
                    else
                        score += komascore[trim_sengo(koma) - FU] * reverse;
                }
            }
            for (int i = FU; i <= HI; ++i)
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