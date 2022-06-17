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

    inline pos_t pos_to_dan(pos_t pos)
    {
        return pos / W - PADDING_H;
    }

    inline pos_t pos_to_suji(pos_t pos)
    {
        return pos % W - PADDING_W;
    }

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

    using hash_t = std::size_t;

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
     * @breif 駒が先手の駒か判定する。
     * @param koma 駒
     * @return 先手の駒である場合 true
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
     * @breif 駒が後手の駒か判定する。
     * @param koma 駒
     * @return 後手の駒である場合 true
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
     * @breif 駒が走り駒(香・角・飛・馬・竜)か判定する。
     * @param koma 駒
     * @return 走り駒である場合 true
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
     * @breif 駒を成る前の駒に変換する。
     * @param koma 駒
     * @return 成る前の駒
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
     * @breif 駒を成り駒に変換する。
     * @param koma 駒
     * @return 成り駒
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
     * @breif 駒から先手後手の情報を取り除く。
     * @param koma 駒
     * @return 先手後手の情報を取り除かれた駒
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
     * @breif 駒を後手の駒に変換する。
     * @param koma 駒
     * @return 後手の駒
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
            std::minstd_rand rand;
            std::uniform_int_distribution<hash_t> uid{ std::numeric_limits<hash_t>::min(), std::numeric_limits<hash_t>::max() };
            for (std::size_t i = 0; i < std::size(ban_table); ++i)
                ban_table[i] = uid(rand);
            for (std::size_t i = 0; i < std::size(mochigoma_table); ++i)
                mochigoma_table[i] = uid(rand);
        }
        hash_t ban_table[KOMA_ENUM_NUMBER * 9 * 9];
        hash_t mochigoma_table[(18 + 4 + 4 + 4 + 4 + 2 + 2) * 2 * 2];

        /**
         * @breif 盤上の駒のハッシュ値を計算する。
         * @param koma 駒
         * @param pos 駒の座標
         * @return ハッシュ値
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
         * @breif 持ち駒のハッシュ値を計算する。
         * @param koma 駒
         * @param count 駒の数
         * @param is_gote 後手の持ち駒か
         * @return ハッシュ値
         */
        inline hash_t mochigoma_hash(koma_t koma, std::size_t count, bool is_gote) const
        {
            enum
            {
                FU_OFFSET   = 0                     , FU_MAX   = 18,
                KYO_OFFSET  = FU_OFFSET   + FU_MAX  , KYO_MAX  =  4,
                KEI_OFFSET  = KYO_OFFSET  + KYO_MAX , KEI_MAX  =  4,
                GIN_OFFSET  = KEI_OFFSET  + KEI_MAX , GIN_MAX  =  4,
                KIN_OFFSET  = GIN_OFFSET  + GIN_MAX , KIN_MAX  =  4,
                KAKU_OFFSET = KIN_OFFSET  + KIN_MAX , KAKU_MAX =  2,
                HI_OFFSET   = KAKU_OFFSET + KAKU_MAX, HI_MAX   =  2,
                SIZE        = HI_OFFSET   + HI_MAX
            };

            SHOGIPP_ASSERT(!(koma == FU && count > FU_MAX));
            SHOGIPP_ASSERT(!(koma == KYO && count > KYO_MAX));
            SHOGIPP_ASSERT(!(koma == KEI && count > KEI_MAX));
            SHOGIPP_ASSERT(!(koma == GIN && count > GIN_MAX));
            SHOGIPP_ASSERT(!(koma == KIN && count > KIN_MAX));
            SHOGIPP_ASSERT(!(koma == KAKU && count > KAKU_MAX));
            SHOGIPP_ASSERT(!(koma == HI && count > HI_MAX));

            static const std::size_t map[]
            {
                FU_OFFSET  ,
                KYO_OFFSET ,
                KEI_OFFSET ,
                GIN_OFFSET ,
                KIN_OFFSET ,
                KAKU_OFFSET,
                HI_OFFSET  ,
            };

            std::size_t index = map[koma];
            index *= SIZE;
            index += count;
            index *= 2;
            if (is_gote)
                ++index;
            return ban_table[index];
        }
    } hash_table;

    inline const char * tebanstr(tesu_t tesu)
    {
        const char * map[]{ "先手", "後手" };
        return map[tesu % 2];
    }

    inline const char * numberstr(koma_t koma) {
        const char * map[]{ "０", "１", "２", "３", "４", "５", "６", "７", "８", "９" };
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

    inline const char * to_string(koma_t koma)
    {
        SHOGIPP_ASSERT(koma < KOMA_ENUM_NUMBER);
        static const char * map[]{
            "・",
            "歩", "香", "桂", "銀", "金", "角", "飛", "王", "と", "杏", "圭", "全", "馬", "竜",
            "歩", "香", "桂", "銀", "金", "角", "飛", "王", "と", "杏", "圭", "全", "馬", "竜",
        };
        return map[koma];
    }

    inline const char * danstr(pos_t pos)
    {
        static const char * map[]{ "一", "二", "三", "四", "五", "六", "七", "八", "九" };
        return map[pos];
    }

    inline const char * sujistr(pos_t pos)
    {
        static const char * map[]{ "９", "８", "７", "６", "５", "４", "３", "２", "１" };
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

    /**
     * @breif 合法手
     */
    struct te_t
    {
        pos_t src;      // 移動元の座標(src == npos の場合、持ち駒を打つ)
        pos_t dst;      // 移動先の座標(src == npos の場合、 dst は打つ座標)
        koma_t srckoma; // 移動元の駒(src == npos の場合、 srckoma は打つ持ち駒)
        koma_t dstkoma; // 移動先の駒(src == npos の場合、 dstkoma は未定義)
        bool promote;   // 成る場合 true
    };

    /**
     * @breif 持ち駒
     */
    struct mochigoma_t
    {
        unsigned char count[HI - FU + 1];

        /**
         * @breif 持ち駒を初期化する。
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
                std::cout << "なし";
            std::cout << std::endl;
        }

        /**
         * @breif 駒と対応する持ち駒の数の参照を返す。
         * @param 駒
         * @return 駒と対応する持ち駒の数の参照
         */
        inline unsigned char & operator [](koma_t koma)
        {
            SHOGIPP_ASSERT(trim_sengo(koma) != EMPTY);
            SHOGIPP_ASSERT(trim_sengo(koma) != OU);
            static const std::size_t map[]{
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
     * @breif 盤
     */
    struct ban_t
    {
        /**
         * @breif 盤を初期化する。
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
         * @breif 座標posが盤外か判定する。
         * @param pos 座標
         * @return 盤外の場合true
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
         * @breif 盤を標準出力に出力する。
         */
        inline void print() const
        {
            std::cout << "  ９ ８ ７ ６ ５ ４ ３ ２ １" << std::endl;
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
     * @breif 利き
     */
    struct kiki_t
    {
        pos_t offset;     // 利きの相対座標
        pos_t pos;        // 利いている駒の座標
        bool aigoma;    // 合駒が可能か
    };

    using aigoma_info_t = std::unordered_map<pos_t, std::vector<pos_t>>;

    /**
     * @breif 局面
     */
    struct kyokumen_t
    {
        /**
         * @breif 局面を初期化する。
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
         * @breif 駒komaが座標dstに移動する場合に成りが可能か判定する。
         * @param koma 駒
         * @param dst 移動先の座標
         * @return 成りが可能の場合(komaが既に成っている場合、常にfalse)
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
         * @breif 駒komaが座標dstに移動する場合に成りが必須か判定する。
         * @param koma 駒
         * @param dst 移動先の座標
         * @return 成りが必須の場合(komaが既に成っている場合、常にfalse)
         */
        inline static bool must_promote(koma_t koma, pos_t dst)
        {
            if (trim_sengo(koma) == FU || trim_sengo(koma) == KYO)
            {
                if (is_gote(koma))
                    return dst >= W * (8 + PADDING_H);
                return dst < (W * (1 + PADDING_H));
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
         * @breif 座標srcから移動可能の移動先を反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
         * @param offset 移動先の相対座標
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
         * @breif 座標srcから移動可能の移動先を非反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
         * @param offset 移動先の相対座標
         */
        template<typename OutputIterator>
        void find_near_dst(OutputIterator result, pos_t src, pos_t offset) const
        {
            pos_t cur = src + offset;
            if (!ban_t::out(cur) && (ban[cur] == EMPTY || is_gote(ban[cur]) != is_gote(ban[src])))
                *result++ = cur;
        }

        /**
         * @breif 座標srcから移動可能の移動先を検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
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
         * @breif 持ち駒komaを座標dstに置くことができるか判定する。歩、香、桂に限りfalseを返す可能性がある。
         * @param koma 持ち駒
         * @param dst 移動先の座標
         * @return 置くことができる場合 true
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

                // 打ち歩詰め
                pos_t pos = dst + FRONT * (is_goteban(tesu) ? -1 : 1);
                if (!ban_t::out(pos) && trim_sengo(ban[pos]) == OU && is_gote(ban[pos]) != is_goteban(tesu))
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
         * @breif 移動元の座標を検索する。
         * @param result 出力イテレータ
         */
        template<typename OutputIterator>
        void find_src(OutputIterator result) const
        {
            for (pos_t i = 0; i < W * H; ++i)
                if (ban[i] != EMPTY && ban[i] != X && is_gote(ban[i]) == is_goteban(tesu))
                    *result++ = i;
        }

        /**
         * @breif 座標posから相対座標offset方向に走査し最初に駒が現れる座標を返す。
         * @param pos 走査を開始する座標
         * @param offset 走査する相対座標
         * @return 最初に駒が現れる座標(駒が見つからない場合 npos )
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
         * @breif 座標posの駒に対する隣接した駒の利きを検索する。
         * @param result 利きの出力イテレータ
         * @param pos 座標
         * @param offset 利きの相対座標
         * @param gote 後手視点か
         * @param first 利く駒の入力イテレータ(begin)
         * @param last 利く駒の入力イテレータ(end)
         */
        template<typename OutputIterator, typename InputIterator>
        void search_kiki_near(OutputIterator result, pos_t pos, pos_t offset, bool gote, InputIterator first, InputIterator last)
        {
            if (pos_t cur = pos + offset; !ban_t::out(cur) && gote != is_gote(ban[cur]))
                if (std::find(first, last, trim_sengo(ban[cur])) != last)
                    *result++ = { offset, cur, false };
        }

        /**
         * @breif 座標posの駒に対する隣接しない駒の利きを検索する。
         * @param result 利きの出力イテレータ
         * @param pos 座標
         * @param offset 利きの相対座標
         * @param gote 後手視点か
         * @param first 利く駒の入力イテレータ(begin)
         * @param last 利く駒の入力イテレータ(end)
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
         * @breif 座標posに対する利きを検索する。
         * @param result 利きの出力イテレータ
         * @param pos 座標
         * @param gote 後手視点か
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
         * @breif 王に対する利きを更新する。
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
         * @breif 合法手を生成する。
         * @param result 合法手の出力イテレータ
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
                    std::cout << "合駒：";
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

                        // 合駒は利きの範囲にしか移動できない。
                        if (is_aigoma)
                        {
                            const std::vector<pos_t> & candidates = aigoma_iter->second;
                            if (std::find(candidates.begin(), candidates.end(), dst) == candidates.end())
                                continue;
                        }

                        // 利いている場所に王を移動させてはならない
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
                                *result++ = { npos, dst, koma };
                }
            }
            else // 王手されている場合
            {
                pos_t reverse = is_goteban(tesu) ? -1 : 1;
                pos_t src = ou_pos[tesu % 2];

                // 王を移動して王手を解除できる手を検索する。
                for (const pos_t * p = near_move_offsets(OU); *p; ++p)
                {
                    pos_t dst = src + *p * reverse;
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
                    // 合駒の手を検索する。
                    if (oute.front().aigoma)
                    {
                        pos_t offset = oute.front().offset;
                        for (pos_t dst = ou_pos[tesu % 2] + offset; !ban_t::out(dst) && ban[dst] == EMPTY; dst += offset)
                        {
                            // 駒を移動させる合駒
                            std::vector<kiki_t> kiki;
                            search_kiki(std::back_inserter(kiki), dst, is_goteban(tesu));
                            for (auto & k : kiki)
                            {
                                if (can_promote(ban[k.pos], dst))
                                    *result++ = { k.pos, dst, ban[k.pos], ban[dst], true };
                                if (!must_promote(ban[k.pos], dst))
                                    *result++ = { k.pos, dst, ban[k.pos], ban[dst], false };
                            }

                            // 駒を打つ合駒
                            for (koma_t koma = FU; koma <= HI; ++koma)
                                if (mochigoma_list[tesu % 2][koma])
                                    if (can_put(koma, dst))
                                        *result++ = { npos, dst, koma };
                        }
                    }
                    else
                    {
                        // 王手している駒を取る手を検索する。
                        pos_t dst = oute.front().pos;
                        std::vector<kiki_t> kiki;
                        search_kiki(std::back_inserter(kiki), dst, !is_goteban(tesu));
                        for (auto & k : kiki)
                        {
                            // 王を動かす手は既に検索済み
                            if (trim_sengo(ban[k.pos]) != OU)
                            {
                                te_t te{ k.pos, dst, ban[k.pos], ban[dst], false };
                                std::vector<kiki_t> kiki;
                                do_te(te);
                                bool sucide = oute.size() > 0;
                                undo_te(te);

                                // 駒を取った後王手されていてはいけない
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
         * @breif 局面のハッシュ値を計算する。
         * @return 局面のハッシュ値
         */
        inline hash_t make_hash() const
        {
            hash_t hash = 0;

            // 盤上の駒のハッシュ値をXOR演算
            for (pos_t pos = 0; pos < W * H; ++pos)
                if (!ban_t::out(pos))
                    if (koma_t koma = ban[pos]; koma != EMPTY)
                        hash ^= hash_table.koma_hash(koma, pos);

            // 持ち駒のハッシュ値をXOR演算
            for (std::size_t i = 0; i < std::size(mochigoma_list); ++i)
                for (koma_t koma = FU; koma <= HI; ++koma)
                    hash ^= hash_table.mochigoma_hash(koma, mochigoma_list[i][koma], is_goteban(i));

            return hash;
        }

        /**
         * @breif 局面のハッシュ値と合法手から、合法手を実施した後の局面のハッシュ値を計算する。
         * @param hash 合法手を実施する前の局面のハッシュ値
         * @param te 実施する合法手
         * @return 合法手を実施した後の局面のハッシュ値
         * @details 合法手により発生する差分に基づき計算するため make_hash() より比較的高速に処理される。
         *          この関数は合法手を実施するより前に呼び出される必要がある。
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
                if (te.dstkoma != EMPTY)
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
         * @breif 合法手を出力する。
         * @param te 合法手
         */
        inline void print_te(const te_t & te) const
        {
            std::cout << (is_goteban(tesu) ? "△" : "▲");
            if (te.src != npos)
            {
                const char * naristr;
                if (can_promote(te.srckoma, te.dst))
                    naristr = te.promote ? "成" : "不成";
                else
                    naristr = "";
                std::cout << sujistr(pos_to_suji(te.dst)) << danstr(pos_to_dan(te.dst)) << to_string(trim_sengo(te.srckoma)) << naristr
                    << " (" << sujistr(pos_to_suji(te.src)) << danstr(pos_to_dan(te.src)) << ")" << std::endl;
            }
            else
            {
                std::cout << sujistr(pos_to_suji(te.dst)) << danstr(pos_to_dan(te.dst)) << to_string(trim_sengo(te.srckoma)) << "打" << std::endl;
            }
        }

        /**
         * @breif 合法手を出力する。
         * @param first 合法手の入力イテレータのbegin
         * @param last 合法手の入力イテレータのend
         */
        template<typename InputIterator>
        void print_te(InputIterator first, InputIterator last) const
        {
            for (std::size_t i = 0; first != last; ++i)
            {
                std::cout << "#" << (i + 1) << " ";
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
                    std::cout << "王手：";
                    for (std::size_t j = 0; j < oute_list[i].size(); ++j)
                    {
                        kiki_t & kiki = oute[j];
                        if (j > 0)
                            std::cout << "　";
                        print_pos(kiki.pos);
                        std::cout << to_string(trim_sengo(ban[kiki.pos])) << std::endl;
                    }
                }
            }
        }

        /**
         * @breif 局面を出力する。
         */
        inline void print()
        {
            std::cout << "後手持ち駒：";
            mochigoma_list[1].print();
            ban.print();
            std::cout << "先手持ち駒：";
            mochigoma_list[0].print();
        }

        /**
         * @breif 局面のハッシュ値を返す。
         * @return 局面のハッシュ値
         */
        hash_t hash() const
        {
            return hash_stack.top();
        }

        /**
         * @breif 合法手を実行する。
         * @param te 合法手
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
                if (ban[te.dst] != EMPTY)
                    ++mochigoma_list[tesu % 2][ban[te.dst]];
                ban[te.dst] = te.promote ? to_promoted(ban[te.src]) : ban[te.src];
                ban[te.src] = EMPTY;
                if (trim_sengo(te.srckoma) == OU)
                    ou_pos[tesu % 2] = te.dst;
            }
            ++tesu;
            hash_stack.push(hash);
            update_oute();
        }

        /**
         * @breif 合法手を実行する前に戻す。
         * @param te 合法手
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
            hash_stack.pop();
            update_oute();
        }

        ban_t ban;                          // 盤
        mochigoma_t mochigoma_list[2];      // 持ち駒 { 先手, 後手 }
        tesu_t tesu;                        // 手数
        pos_t ou_pos[2];                    // 王の座標 { 先手, 後手 }
        std::vector<kiki_t> oute_list[2];   // 王に対する利き { 先手, 後手 }
        std::stack<hash_t> hash_stack;      // それまでの各手番におけるハッシュ値を格納するスタック
    };

    /**
     * @breif 評価関数オブジェクトのインターフェース
     */
    struct abstract_evaluator_t
    {
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
                    std::cout << tebanstr(static_cast<tesu_t>(i)) << "：" << evaluators[i]->name() << std::endl;
                std::cout << std::endl;
            }

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));
            if (te_list.empty())
            {
                std::cout << kyokumen.tesu << "手詰み" << std::endl;
                kyokumen.print();
                std::cout << tebanstr(kyokumen.tesu + 1) << "勝利 (" << evaluator->name() << ")";
                std::cout.flush();
                return false;
            }
            else
            {
                std::cout << (kyokumen.tesu + 1) << "手目" << tebanstr(kyokumen.tesu) << "番" << std::endl;
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
     * @breif 単純に評価値が最も高い手を返す評価関数オブジェクトの抽象クラス
     */
    struct basic_evaluator_t
        : public abstract_evaluator_t
    {
        /**
         * @breif 局面に対して評価値が最も高くなる合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
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
         * @breif 局面に対して評価値を返す。
         * @param kyokumen 局面
         * @return 局面の評価値
         */
        virtual int eval(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif 評価関数オブジェクトの実装例
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
     * @breif 評価関数オブジェクトの実装例
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