#include <iostream>

#include <cstdlib>
#include <ctime>
#include <vector>
#include <utility>
#include <algorithm>
#include <memory>
#include <sstream>
#include <map>
#include <unordered_map>
#include <random>
#include <limits>
#include <stack>
#include <optional>
#include <array>
#include <chrono>

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

namespace shogipp
{
    inline unsigned long long total_search_count = 0;

    inline void assert_impl(bool assertion, const char * expr, const char * file, const char * func, unsigned int line)
    {
        if (!assertion)
        {
            std::ostringstream what;
            what << "Assertion failed: " << expr << ", file " << file << ", line " << line;
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
        { back_left  , { gin, ou, kaku, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
        { back       , { kin, ou, hi, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
        { back_right , { gin, ou, kaku, tokin, nari_kyo, nari_kei, nari_gin, uma, ryu } },
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

    using tesu_t = unsigned int;
    
    inline bool is_goteban(tesu_t tesu)
    {
        return tesu % 2 != 0;
    }

    using hash_t = std::size_t;

    inline bool is_promoted(koma_t koma)
    {
        SHOGIPP_ASSERT(koma != empty);
        constexpr static bool map[]
        {
            false,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true
        };
        return map[koma];
    }

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
     * @breif 駒が先手の駒か判定する。
     * @param koma 駒
     * @return 先手の駒である場合 true
     */
    inline bool is_sente(koma_t koma)
    {
        constexpr static bool map[]
        {
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
        constexpr static bool map[]
        {
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

    /*
     * @breif 駒を後手の駒に変換する。
     * @param koma 駒
     * @return 後手の駒
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
    
    static const struct hash_table_t
    {
        hash_table_t()
        {
            std::minstd_rand rand{ SHOGIPP_SEED };
            std::uniform_int_distribution<hash_t> uid{ std::numeric_limits<hash_t>::min(), std::numeric_limits<hash_t>::max() };
            for (std::size_t i = 0; i < std::size(ban_table); ++i)
                ban_table[i] = uid(rand);
            for (std::size_t i = 0; i < std::size(mochigoma_table); ++i)
                mochigoma_table[i] = uid(rand);
        }
        hash_t ban_table[koma_enum_number * 9 * 9];
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
                fu_offset  ,
                kyo_offset ,
                kei_offset ,
                gin_offset ,
                kin_offset ,
                kaku_offset,
                hi_offset  ,
            };

            std::size_t index = map[koma];
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
            /* nari_kyo */ { front_left, front, front_right, left, right, back },
            /* nari_kei */ { front_left, front, front_right, left, right, back },
            /* nari_gin */ { front_left, front, front_right, left, right, back },
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
            /* nari_kyo */ { },
            /* nari_kei */ { },
            /* nari_gin */ { },
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

    inline std::string pos_to_string(pos_t pos)
    {
        return std::string{} +sujistr(pos_to_suji(pos)) + danstr(pos_to_dan(pos));
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
        unsigned char count[hi - fu + 1];

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
            SHOGIPP_ASSERT(koma != empty);
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
         * @breif 座標posが盤外か判定する。
         * @param pos 座標
         * @return 盤外の場合true
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

        koma_t data[width * height];
    };

    /**
     * @breif 利き
     */
    struct kiki_t
    {
        pos_t offset;   // 利きの相対座標
        pos_t pos;      // 利いている駒の座標
        bool aigoma;    // 合駒が可能か
    };

    using aigoma_info_t = std::unordered_map<pos_t, std::vector<pos_t>>;

    /**
     * @breif 局面
     */
    struct kyokumen_t
    {
        using move_table_t = std::map<pos_t, std::vector<pos_t>>;

        /**
         * @breif 駒の移動元と移動先の情報を初期化する。
         */
        inline void init_move_table_list()
        {
            for (std::size_t i = 0; i < move_table_list.size(); ++i)
            {
                auto & move_table = move_table_list[i];
                move_table.clear();
                std::vector<pos_t> source_list;
                search_source(std::back_inserter(source_list), is_goteban(i));
                for (pos_t src : source_list)
                {
                    std::vector<pos_t> destination_list;
                    search_destination(std::back_inserter(destination_list), src, is_goteban(i));
                    move_table[src] = std::move(destination_list);
                }
            }
        }

        /**
         * @breif 駒の移動元と移動先の情報を更新する。
         * @param move_table  駒の移動元と移動先の情報
         * @param source 移動元の座標
         * @param destination_list 移動先の座標の vector
         * @details destination_list.empty() == true の場合、 move_table から source を削除する。
         */
        inline void update_move_table(move_table_t & move_table, pos_t source, std::vector<pos_t> && destination_list);

        /**
         * @breif 座標 pos に利いているか紐を付けている駒を検索し、その座標を移動元とする駒の移動元と移動先の情報を更新する。
         * @param pos 座標
         * @details destination_list.empty() == true の場合、 move_table から source を削除する。
         */
        inline void update_move_table_relative_to(pos_t pos);

        /**
         * @breif 最後に解決された合法手を元に、駒の移動元と移動先の情報を更新する。
         * @param te 最後に実施された合法手
         * @details do_te の内部で合法手が解決された後で呼ばれる想定で実装される。
         */
        inline void do_updating_move_table_list(const te_t & te);

        /**
         * @breif 最後に解決された合法手を元に、駒の移動元と移動先の情報を更新する。
         * @param te 最後に実施された合法手
         * @details undo_te の内部で合法手が解決された後で呼ばれる想定で実装される。
         */
        inline void undo_updating_move_table_list(const te_t & te);

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
            kifu.clear();
            init_move_table_list();
        }

        /**
         * @breif 駒komaが座標dstに移動する場合に成りが可能か判定する。
         * @param koma 駒
         * @param dst 移動先の座標
         * @return 成りが可能の場合(komaが既に成っている場合、常にfalse)
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
         * @breif 駒komaが座標dstに移動する場合に成りが必須か判定する。
         * @param koma 駒
         * @param dst 移動先の座標
         * @return 成りが必須の場合(komaが既に成っている場合、常にfalse)
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
         * @breif 座標srcから移動可能の移動先を反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
         * @param offset 移動先の相対座標
         */
        template<typename OutputIterator>
        void search_far_destination(OutputIterator result, pos_t src, pos_t offset) const
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
         * @breif 座標srcから移動可能の移動先を非反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
         * @param offset 移動先の相対座標
         */
        template<typename OutputIterator>
        void search_near_destination(OutputIterator result, pos_t src, pos_t offset) const
        {
            pos_t cur = src + offset;
            if (!ban_t::out(cur) && (ban[cur] == empty || is_gote(ban[cur]) != is_gote(ban[src])))
                *result++ = cur;
        }

        /**
         * @breif 座標srcから移動可能の移動先を検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
         * @param is_goteban 後手の移動か
         */
        template<typename OutputIterator>
        void search_destination(OutputIterator result, pos_t src, bool is_goteban) const
        {
            koma_t koma = trim_sengo(ban[src]);
            pos_t reverse = is_goteban ? -1 : 1;
            for (const pos_t * offset = far_move_offsets(koma); *offset; ++offset)
                search_far_destination(result, src, *offset * reverse);
            for (const pos_t * offset = near_move_offsets(koma); *offset; ++offset)
                search_near_destination(result, src, *offset * reverse);
        }

        /**
         * @breif 持ち駒komaを座標dstに置くことができるか判定する。歩、香、桂に限りfalseを返す可能性がある。
         * @param koma 持ち駒
         * @param dst 移動先の座標
         * @return 置くことができる場合 true
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
                pos_t suji = pos_to_suji(dst);
                for (pos_t dan = 0; dan < height; ++dan)
                {
                    koma_t cur = ban[suji_dan_to_pos(suji, dan)];
                    if (cur != empty && trim_sengo(cur) == fu && is_goteban(tesu) == is_gote(cur))
                        return false;
                }

                // 打ち歩詰め
                pos_t pos = dst + front * (is_goteban(tesu) ? -1 : 1);
                if (!ban_t::out(pos) && ban[pos] != empty && trim_sengo(ban[pos]) == ou && is_gote(ban[pos]) != is_goteban(tesu))
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
         * @param is_goteban 後手の移動か
         */
        template<typename OutputIterator>
        void search_source(OutputIterator result, bool is_goteban) const
        {
            for (pos_t i = 0; i < width * height; ++i)
                if (ban[i] != empty && ban[i] != x && is_gote(ban[i]) == is_goteban)
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
            for (cur = pos + offset; !ban_t::out(cur) && ban[cur] == empty; cur += offset);
            if (ban_t::out(cur))
                return npos;
            return cur;
        }

        /**
         * @breif 座標posの駒に対する隣接する駒の利きを検索する。
         * @param result 利きの出力イテレータ
         * @param pos 座標
         * @param offset 利きの相対座標
         * @param gote 後手視点か
         * @param first 利く駒の入力イテレータ(begin)
         * @param last 利く駒の入力イテレータ(end)
         */
        template<typename OutputIterator, typename InputIterator>
        void search_kiki_near_internal(OutputIterator result, pos_t pos, pos_t offset, bool gote, InputIterator first, InputIterator last)
        {
            if (pos_t cur = pos + offset; !ban_t::out(cur) && ban[cur] != empty && gote != is_gote(ban[cur]))
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
        void search_kiki_far_internal(OutputIterator result, pos_t pos, pos_t offset, bool gote, InputIterator first, InputIterator last)
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
         * @breif 座標posに対する隣接する駒の利きを検索する。
         * @param result 利きの出力イテレータ
         * @param pos 座標
         * @param gote 後手視点か
         */
        template<typename OutputIterator>
        void search_kiki_near(OutputIterator result, pos_t pos, bool gote)
        {
            pos_t reverse = gote ? -1 : 1;
            for (auto & [offset, koma_list] : near_kiki_list)
                search_kiki_near_internal(result, pos, offset * reverse, gote, koma_list.begin(), koma_list.end());
        }

        /**
         * @breif 座標posに対する隣接しない駒の利きを検索する。
         * @param result 利きの出力イテレータ
         * @param pos 座標
         * @param gote 後手視点か
         */
        template<typename OutputIterator>
        void search_kiki_far(OutputIterator result, pos_t pos, bool gote)
        {
            pos_t reverse = gote ? -1 : 1;
            for (auto & [offset, koma_list] : far_kiki_list)
                search_kiki_far_internal(result, pos, offset * reverse, gote, koma_list.begin(), koma_list.end());
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
            search_kiki_near(result, pos, gote);
            search_kiki_far(result, pos, gote);
        }

        /**
         * @breif 座標posを利いている駒あるいは紐を付けている駒を検索する。
         * @param result 利きの出力イテレータ
         * @param pos 座標
         * @param offset 利きの相対座標
         * @param gote 後手視点か
         * @param first 利く駒の入力イテレータ(begin)
         * @param last 利く駒の入力イテレータ(end)
         */
        template<typename OutputIterator, typename InputIterator>
        void search_kiki_or_himo_near_internal(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last)
        {
            if (pos_t cur = pos + offset; !ban_t::out(cur) && ban[cur] != empty)
                if (std::find(first, last, trim_sengo(ban[cur])) != last)
                    *result++ = cur;
        }

        /**
         * @breif 座標posを利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param offset 利きの相対座標
         * @param first 利く駒の入力イテレータ(begin)
         * @param last 利く駒の入力イテレータ(end)
         * @sa search_kiki_far
         */
        template<typename OutputIterator, typename InputIterator>
        void search_kiki_or_himo_far_internal(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last)
        {
            if (pos_t found = search(pos, offset); found != npos)
                if (std::find(first, last, trim_sengo(ban[found])) != last)
                    *result++ = found;
        }

        /**
         * @breif 座標posに利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         */
        template<typename OutputIterator>
        void search_kiki_or_himo(OutputIterator result, pos_t pos)
        {
            for (auto & [offset, koma_list] : far_kiki_list_synmmetric)
                search_kiki_or_himo_far_internal(result, pos, offset, koma_list.begin(), koma_list.end());
            for (int teban = 0; teban < 2; ++teban)
            {
                pos_t reverse = is_goteban(teban) ? -1 : 1;
                for (auto & [offset, koma_list] : near_kiki_list)
                    search_kiki_or_himo_near_internal(result, pos, offset * reverse, koma_list.begin(), koma_list.end());
                for (auto & [offset, koma_list] : far_kiki_list_asynmmetric)
                    search_kiki_or_himo_far_internal(result, pos, offset * reverse, koma_list.begin(), koma_list.end());
            }
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

                for (auto & [source, destination_list] : move_table_list[tesu % 2])
                {
                    auto aigoma_iter = aigoma_info.find(source);
                    bool is_aigoma = aigoma_iter != aigoma_info.end();

                    for (pos_t destination : destination_list)
                    {
#ifndef NDEBUG
                        if (ban[destination] != empty && trim_sengo(ban[destination]) == ou)
                        {
                            std::cout << pos_to_string(source) << std::endl;
                            te_t te{ source, destination, ban[source], ban[destination], false };
                            print_te(te, is_goteban(tesu));
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
                            search_kiki(std::back_inserter(kiki_list), destination, is_goteban(tesu));
                            if (kiki_list.size() > 0)
                                continue;
                        }

                        if (can_promote(ban[source], destination))
                            *result++ = { source, destination, ban[source], ban[destination], true };
                        if (!must_promote(ban[source], destination))
                            *result++ = { source, destination, ban[source], ban[destination], false };
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
            else // 王手されている場合
            {
                pos_t reverse = is_goteban(tesu) ? -1 : 1;
                pos_t src = ou_pos[tesu % 2];

                // 王を移動して王手を解除できる手を検索する。
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
                    // 合駒の手を検索する。
                    if (oute.front().aigoma)
                    {
                        pos_t offset = oute.front().offset;
                        for (pos_t dst = ou_pos[tesu % 2] + offset; !ban_t::out(dst) && ban[dst] == empty; dst += offset)
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
                            for (koma_t koma = fu; koma <= hi; ++koma)
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
                            if (trim_sengo(ban[k.pos]) != ou)
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
            for (pos_t pos = 0; pos < width * height; ++pos)
                if (!ban_t::out(pos))
                    if (koma_t koma = ban[pos]; koma != empty)
                        hash ^= hash_table.koma_hash(koma, pos);

            // 持ち駒のハッシュ値をXOR演算
            for (std::size_t i = 0; i < std::size(mochigoma_list); ++i)
                for (koma_t koma = fu; koma <= hi; ++koma)
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
                if (te.dstkoma != empty)
                {
                    std::size_t mochigoma_count = mochigoma_list[tesu % 2][te.dstkoma];
                    hash ^= hash_table.mochigoma_hash(to_mochigoma(te.dstkoma), mochigoma_count, is_goteban(tesu));
                    hash ^= hash_table.mochigoma_hash(to_mochigoma(te.dstkoma), mochigoma_count + 1, is_goteban(tesu));
                    hash ^= hash_table.koma_hash(te.dstkoma, te.dst);
                }
                hash ^= hash_table.koma_hash(te.promote ? to_unpromoted(te.srckoma) : te.srckoma, te.dst);
            }
            return hash;
        }

        /**
         * @breif 合法手を出力する。
         * @param te 合法手
         * @param is_gote 後手の合法手か
         */
        inline void print_te(const te_t & te, bool is_gote) const
        {
            std::cout << (is_gote ? "△" : "▲");
            if (te.src != npos)
            {
                const char * naristr;
                if (can_promote(te.srckoma, te.dst))
                    naristr = te.promote ? "成" : "不成";
                else
                    naristr = "";
                std::cout << sujistr(pos_to_suji(te.dst)) << danstr(pos_to_dan(te.dst)) << to_string(trim_sengo(te.srckoma)) << naristr
                    << " (" << sujistr(pos_to_suji(te.src)) << danstr(pos_to_dan(te.src)) << ")";
            }
            else
            {
                std::cout << sujistr(pos_to_suji(te.dst)) << danstr(pos_to_dan(te.dst)) << to_string(trim_sengo(te.srckoma)) << "打";
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
                std::printf("#%3d ", i + 1);
                //std::cout << "#" << (i + 1) << " ";
                print_te(*first++, is_goteban(tesu));
                std::cout << std::endl;
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

        inline void print_kifu()
        {
            for (std::size_t i = 0; i < kifu.size(); ++i)
            {
                print_te(kifu[i], is_goteban(i));
                std::cout << std::endl;
            }
        }

        /**
         * @breif 局面のハッシュ値を返す。
         * @return 局面のハッシュ値
         */
        hash_t hash() const
        {
            return hash_stack.top();
        }

#ifdef VALIDATE_MOVEMENT_CACHE
        /**
         * @breif search_srouce と search_destination により取得される移動元と移動先が move_table と一致しているか検証する。
         */
        inline void validate_move_table()
        {
            for (std::size_t i = 0; i < 2; ++i)
            {
                auto & move_table = move_table_list[i];

                std::vector<pos_t> source_list;
                search_source(std::back_inserter(source_list), is_goteban(i));
                for (pos_t source : source_list)
                {
                    std::vector<pos_t> destination_list;
                    search_destination(std::back_inserter(destination_list), source, is_goteban(i));
                    
                    auto iter = move_table.find(source);
                    if (iter != move_table.end())
                    {
                        auto & [source2, destination_list2] = *iter;
                        if (!std::equal(destination_list.begin(), destination_list.end(), destination_list2.begin(), destination_list2.end()))
                        {
                            std::cerr << tebanstr(i) << " source " << pos_to_string(source) << " destination list { ";
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
                        std::cerr << tebanstr(i) << " source " << pos_to_string(source) << " is not found ({ ";
                        for (pos_t destination : destination_list)
                            std::cerr << pos_to_string(destination) << " ";
                        std::cerr << "})"<< std::endl;
                        SHOGIPP_ASSERT(false);
                    }
                }
            }
        }
#endif

        /**
         * @breif 合法手を実行する。
         * @param te 合法手
         */
        inline void do_te(const te_t & te)
        {
            ++total_search_count;
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
            do_updating_move_table_list(te);

#ifdef VALIDATE_MOVEMENT_CACHE
            validate_move_table();
#endif
            kifu.push_back(te);
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
            undo_updating_move_table_list(te);

#ifdef VALIDATE_MOVEMENT_CACHE
            validate_move_table();
#endif

            kifu.pop_back();
        }

        ban_t ban;                                      // 盤
        mochigoma_t mochigoma_list[2];                  // 持ち駒 { 先手, 後手 }
        tesu_t tesu;                                    // 手数
        pos_t ou_pos[2];                                // 王の座標 { 先手, 後手 }
        std::vector<kiki_t> oute_list[2];               // 王に対する利き { 先手, 後手 }
        std::stack<hash_t> hash_stack;                  // それまでの各手番におけるハッシュ値を格納するスタック
        std::vector<te_t> kifu;                         // 棋譜
        std::array<move_table_t, 2> move_table_list;    // 合法手の表
    };

    void kyokumen_t::update_move_table(move_table_t & move_table, pos_t source, std::vector<pos_t> && destination_list)
    {
        SHOGIPP_ASSERT(!ban_t::out(source));
        SHOGIPP_ASSERT(ban[source] != empty);
        if (destination_list.empty())
            move_table.erase(source);
        else
            move_table[source] = std::move(destination_list);
    }

    void kyokumen_t::update_move_table_relative_to(pos_t source)
    {
        bool gote = is_goteban(tesu);
        std::vector<pos_t> kiki_or_himo_list;
        search_kiki_or_himo(std::back_inserter(kiki_or_himo_list), source);
        if (kiki_or_himo_list.size())
        {
            for (pos_t pos : kiki_or_himo_list)
            {
                ///*DEBUG*/std::cout << pos_to_string(source) << " " << pos_to_string(pos) << std::endl;
                bool gotegoma = is_gote(ban[pos]);
                std::vector<pos_t> new_destination_list;
                search_destination(std::back_inserter(new_destination_list), pos, gotegoma);
                auto & move_table = move_table_list[gotegoma ? 1 : 0];
                update_move_table(move_table, pos, std::move(new_destination_list));
            }
        }
    }

    void kyokumen_t::do_updating_move_table_list(const te_t & te)
    {
        SHOGIPP_ASSERT(tesu > 0);
        bool gote = is_goteban(tesu - 1);
        auto & self_move_table = move_table_list[gote ? 1 : 0];
        auto & nonself_move_table = move_table_list[gote ? 0 : 1];

        if (te.src == npos)
        {
            // 手の移動先を移動元とする自分の手を更新する。
            std::vector<pos_t> new_destination_list;
            search_destination(std::back_inserter(new_destination_list), te.dst, gote);
            update_move_table(self_move_table, te.dst, std::move(new_destination_list));

            // 手の移動先に利いている走り駒の移動先を更新する。
            update_move_table_relative_to(te.dst);
        }
        else
        {
            // 手の移動先を移動元とする自分の手を更新する。
            std::vector<pos_t> new_destination_list;
            search_destination(std::back_inserter(new_destination_list), te.dst, gote);
            update_move_table(self_move_table, te.dst, std::move(new_destination_list));

            // 手の移動先に駒があった場合、手の移動先を移動元とする相手の手を削除する。
            if (te.dstkoma != empty)
                nonself_move_table.erase(te.dst);

            // 手の移動元を移動元とする自分の手を削除する。
            self_move_table.erase(te.src);

            // 手の移動元あるいは移動先に利いているあるいは紐を付けている駒の移動先を更新する。
            update_move_table_relative_to(te.src);
            update_move_table_relative_to(te.dst);
        }
    }

    void kyokumen_t::undo_updating_move_table_list(const te_t & te)
    {
        bool gote = is_goteban(tesu);
        auto & self_move_table = move_table_list[gote ? 1 : 0];
        auto & nonself_move_table = move_table_list[gote ? 0 : 1];

        if (te.src == npos)
        {
            // 手の移動先を移動元とする自分の手を削除する。
            self_move_table.erase(te.dst);

            // 手の移動先に利いている走り駒の移動先を更新する。
            update_move_table_relative_to(te.dst);
        }
        else
        {
            // 手の移動先を移動元とする自分の手を削除する。
            self_move_table.erase(te.dst);

            // 手の移動先に駒があった場合、手の移動先を移動元とする相手の手を更新する。
            if (te.dstkoma != empty)
            {
                std::vector<pos_t> new_destination_list;
                search_destination(std::back_inserter(new_destination_list), te.dst, !gote);
                update_move_table(nonself_move_table, te.dst, std::move(new_destination_list));
            }

            // 手の移動元を移動元とする自分の手を更新する。
            std::vector<pos_t> new_destination_list;
            search_destination(std::back_inserter(new_destination_list), te.src, gote);
            update_move_table(self_move_table, te.src, std::move(new_destination_list));

            // 手の移動元あるいは移動先に利いているあるいは紐を付けている駒の移動先を更新する。
            update_move_table_relative_to(te.src);
            update_move_table_relative_to(te.dst);
        }
    }

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
                auto & winner_evaluator = evaluators[(kyokumen.tesu + 1) % 2];
                std::cout << kyokumen.tesu << "手詰み" << std::endl;
                kyokumen.print();
                std::cout << tebanstr(kyokumen.tesu + 1) << "勝利 (" << winner_evaluator->name() << ")";
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

            kyokumen.print_te(selected_te, is_goteban(kyokumen.tesu));
            std::cout << std::endl << std::endl;

            kyokumen.do_te(selected_te);

            return true;
        }
    };

    template<typename Evaluator1, typename Evaluator2>
    inline void do_game(bool dump_details)
    {
        std::chrono::system_clock::time_point begin, end;
        begin = std::chrono::system_clock::now();

        game_t game{ { std::make_shared<Evaluator1>(), std::make_shared<Evaluator2>() } };
        game.init();
        while (game.procedure());

        end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        double sps = (double)total_search_count / duration * 1000;

        if (dump_details)
        {
            std::cout
                << std::endl << std::endl
                << "total search count: " << total_search_count << std::endl
                << "duration[ms]: " << duration << std::endl
                << "sps: " << sps;
            std::cout.flush();
        }
    }

    /**
     * @breif 駒と価値の連想配列から局面の点数を計算する。
     * @param kyokumen 局面
     * @param map []演算子により駒から価値を連想するオブジェクト
     * @return 局面の点数
     */
    template<typename MapKomaInt>
    inline int kyokumen_map_score(kyokumen_t & kyokumen, MapKomaInt & map)
    {
        int score = 0;
        for (pos_t pos = 0; pos < width * height; ++pos)
        {
            koma_t koma = kyokumen.ban[pos];
            pos_t reverse = is_gote(koma) ? -1 : 1;
            if (!ban_t::out(pos) && koma != empty)
                score += map[trim_sengo(koma)] * reverse;
        }

        for (std::size_t sengo = 0; sengo < std::size(kyokumen.mochigoma_list); ++sengo)
            for (koma_t koma = fu; koma <= hi; ++koma)
                score += map[koma] * kyokumen.mochigoma_list[sengo][koma] * is_goteban(sengo) ? -1 : 1;

        return score;
    }

    /**
     * @breif minimax で合法手を選択する評価関数オブジェクトの抽象クラス
     */
    struct minimax_evaluator_t
        : public abstract_evaluator_t
    {
        using cache_t = std::unordered_map<hash_t, int>;
        
        struct search_info_t
        {
            kyokumen_t * kyokumen;
            unsigned int * search_count;
            cache_t * cache;
            unsigned int depth;
            te_t selected_te;
            int selected_score;
            int te_number;
        };

        void min_max(search_info_t & search_info)
        {
            std::vector<te_t> te_list;
            search_info.kyokumen->search_te(std::back_inserter(te_list));
            search_info.te_number = te_list.size();

            using pair = std::pair<te_t *, int>;
            std::vector<pair> scores;
            auto back_inserter = std::back_inserter(scores);

            for (te_t & te : te_list)
            {
                ++*search_info.search_count;

                int score;
                hash_t hash;

                if (search_info.depth >= 2)
                {
                    search_info.kyokumen->do_te(te);
                    hash = search_info.kyokumen->hash();
                    score = eval(*search_info.kyokumen);
                    search_info.kyokumen->undo_te(te);
                }
                else
                {
                    search_info_t temp;
                    temp.kyokumen = search_info.kyokumen;
                    temp.search_count = search_info.search_count;
                    temp.cache = search_info.cache;
                    temp.depth = search_info.depth + 1;
                    temp.selected_score = 0;

                    search_info.kyokumen->do_te(te);
                    hash = search_info.kyokumen->hash();

                    min_max(temp);
                    if (temp.te_number)
                        score = temp.selected_score;
                    else // 詰み
                        score = (search_info.kyokumen->tesu % 2 == 0) ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();

                    search_info.kyokumen->undo_te(te);
                }

                *back_inserter++ = { &te, score };
            }

            using comparator_t = bool(const pair &, const pair &);
            comparator_t * comparator = (search_info.kyokumen->tesu % 2 == 0) ?
                  comparator = [](const pair & a, const pair & b) -> bool { return a.second > b.second; }
                : comparator = [](const pair & a, const pair & b) -> bool { return a.second < b.second; };
            std::sort(scores.begin(), scores.end(), comparator);

            if (!te_list.empty())
            {
                search_info.selected_te = *scores.front().first;
                search_info.selected_score = scores.front().second;
            }

            search_info.te_number = te_list.size();
        }

        /**
         * @breif 局面に対して minimax で合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            unsigned int search_count = 0;
            cache_t cache;

            search_info_t search_info;
            search_info.kyokumen = &kyokumen;
            search_info.cache = &cache;
            search_info.depth = 0;
            search_info.search_count = &search_count;
            search_info.selected_score = 0;

            min_max(search_info);

            std::cout << "読み手数：" << search_count << std::endl;

            //std::cout << "hash begin" << std::endl;
            //for (auto & [hash, score] : score_cache)
            //    std::printf("%08x: %d\n", hash, score);
            //std::cout << "hash end" << std::endl;

            return search_info.selected_te;
        }

        /**
         * @breif 局面に対して評価値を返す。
         * @param kyokumen 局面
         * @return 局面の評価値
         */
        virtual int eval(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif 単純に評価値が最も高い手を返す評価関数オブジェクトの抽象クラス
     */
    struct max_evaluator_t
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
        : public minimax_evaluator_t
    {
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

            int score = kyokumen_map_score(kyokumen, map);
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
        : public max_evaluator_t
    {
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