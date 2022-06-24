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
#define DISABLE_MOVEMENT_CACHE

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

    enum sengo_t : unsigned char
    {
        sente = 0,
        gote = 1,
        sengo_size = 2
    };

    sengo_t sengo_next(sengo_t sengo)
    {
        SHOGIPP_ASSERT(sengo >= sente);
        SHOGIPP_ASSERT(sengo <= gote);
        return static_cast<sengo_t>((sengo + 1) % 2);
    }

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

    inline pos_t distance(pos_t a, pos_t b)
    {
        int asuji = pos_to_suji(a);
        int adan = pos_to_dan(a);
        int bsuji = pos_to_suji(b);
        int bdan = pos_to_dan(b);
        return std::abs(asuji - bsuji) + std::abs(adan - bdan);
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

    using tesu_t = unsigned int;
    
    inline sengo_t tesu_to_sengo(tesu_t tesu)
    {
        return static_cast<sengo_t>(tesu % 2);
    }

    /**
     * @breif 後手の場合に -1 を、先手の場合に 1 を返す。
     * @param 先手か後手か
     * @return 符号反転用の数値
     */
    inline pos_t reverse(sengo_t sengo)
    {
        return sengo ? -1 : 1;
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
        inline hash_t mochigoma_hash(koma_t koma, std::size_t count, sengo_t sengo) const
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
            index *= sengo_size;
            if (sengo)
                ++index;
            return ban_table[index];
        }
    } hash_table;

    inline const char * tebanstr(tesu_t tesu)
    {
        const char * map[]{ "先手", "後手" };
        return map[tesu_to_sengo(tesu)];
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

    static const pos_t default_ou_pos_list[]
    {
        suji_dan_to_pos(4, 8),
        suji_dan_to_pos(4, 0)
    };

    inline void print_pos(pos_t pos)
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
                    if ((*this)[koma] > 1)
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
                    std::cout << ((koma != empty && to_sengo(koma)) ? "v" : " ") << to_string(koma);
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
        pos_t pos;      // 利いている駒の座標
        pos_t offset;   // 利きの相対座標
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
            for (unsigned char sengo = sente; sengo < sengo_size; ++sengo)
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
            if (to_sengo(koma))
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

        /**
         * @breif 座標srcから移動可能の移動先を反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
         * @param offset 移動先の相対座標
         */
        template<typename OutputIterator>
        inline void search_far_destination(OutputIterator result, pos_t src, pos_t offset) const
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

        /**
         * @breif 座標srcから移動可能の移動先を非反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
         * @param offset 移動先の相対座標
         */
        template<typename OutputIterator>
        inline void search_near_destination(OutputIterator result, pos_t src, pos_t offset) const
        {
            pos_t cur = src + offset;
            if (!ban_t::out(cur) && (ban[cur] == empty || to_sengo(ban[cur]) != to_sengo(ban[src])))
                *result++ = cur;
        }

        /**
         * @breif 座標srcから移動可能の移動先を検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
         * @param sengo 先手・後手どちらの移動か
         */
        template<typename OutputIterator>
        inline void search_destination(OutputIterator result, pos_t src, sengo_t sengo) const
        {
            koma_t koma = trim_sengo(ban[src]);
            pos_t r = reverse(sengo);
            for (const pos_t * offset = far_move_offsets(koma); *offset; ++offset)
                search_far_destination(result, src, *offset * r);
            for (const pos_t * offset = near_move_offsets(koma); *offset; ++offset)
                search_near_destination(result, src, *offset * r);
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
            if (tesu_to_sengo(tesu))
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
                    if (cur != empty && trim_sengo(cur) == fu && tesu_to_sengo(tesu) == to_sengo(cur))
                        return false;
                }

                // 打ち歩詰め
                pos_t pos = dst + front * (reverse(tesu_to_sengo(tesu)));
                if (!ban_t::out(pos) && ban[pos] != empty && trim_sengo(ban[pos]) == ou && to_sengo(ban[pos]) != tesu_to_sengo(tesu))
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
         * @param sengo 先手・後手どちらの移動か
         */
        template<typename OutputIterator>
        inline void search_source(OutputIterator result, sengo_t sengo) const
        {
            for (pos_t i = 0; i < width * height; ++i)
                if (!ban_t::out(i) && ban[i] != empty && to_sengo(ban[i]) == sengo)
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
        inline void search_koma_near_internal(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform)
        {
            if (pos_t cur = pos + offset; !ban_t::out(cur) && ban[cur] != empty)
                if (is_collected(to_sengo(ban[cur])) && std::find(first, last, trim_sengo(ban[cur])) != last)
                    *result++ = transform(cur, offset, false);
        }

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
        inline void search_koma_far_internal(OutputIterator result, pos_t pos, pos_t offset, InputIterator first, InputIterator last, IsCollected is_collected, Transform transform)
        {
            if (pos_t found = search(pos, offset); found != npos && ban[found] != empty)
                if (is_collected(to_sengo(ban[found])) && std::find(first, last, trim_sengo(ban[found])) != last)
                    *result++ = transform(found, offset, found != pos + offset);
        }

        /**
         * @breif 座標posに利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param sengo 先手か後手か
         * @param is_collected 見つかった駒の手番に対して出力イテレータに出力するか判定する叙述関数(bool(bool))
         * @param transform (pos, offset, aigoma) を出力イテレータに出力する変数に変換する関数
         */
        template<typename OutputIterator, typename IsCollected, typename Transform>
        inline void search_koma(OutputIterator result, pos_t pos, sengo_t sengo, IsCollected is_collected, Transform transform)
        {
            pos_t r = reverse(sengo);
            for (auto & [offset, koma_list] : near_kiki_list)
                search_koma_near_internal(result, pos, offset * r, koma_list.begin(), koma_list.end(), is_collected, transform);
            for (auto & [offset, koma_list] : far_kiki_list_synmmetric)
                search_koma_far_internal(result, pos, offset, koma_list.begin(), koma_list.end(), is_collected, transform);
            for (auto & [offset, koma_list] : far_kiki_list_asynmmetric)
                search_koma_far_internal(result, pos, offset * r, koma_list.begin(), koma_list.end(), is_collected, transform);
        }

        /**
         * @breif 座標posに紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param sengo 先手か後手か
         */
        template<typename OutputIterator>
        inline void search_himo(OutputIterator result, pos_t pos, sengo_t sengo)
        {
            search_koma(result, pos, sengo,
                [sengo](sengo_t g) { return g == sengo; },
                [](pos_t pos, pos_t offset, bool aigoma) -> pos_t { return pos; });
        }

        /**
         * @breif 座標posに利いている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param sengo 先手か後手か
         */
        template<typename OutputIterator>
        inline void search_kiki(OutputIterator result, pos_t pos, sengo_t sengo)
        {
            search_koma(result, pos, sengo,
                [sengo](sengo_t g) { return g != sengo; },
                [](pos_t pos, pos_t offset, bool aigoma) -> kiki_t { return { pos, offset, aigoma }; });
        }

        /**
         * @breif 座標posに利いている駒あるいは紐を付けている駒を検索する。
         * @param result 座標の出力イテレータ
         * @param pos 座標
         * @param sengo 先手か後手か
         */
        template<typename OutputIterator>
        inline void search_kiki_or_himo(OutputIterator result, pos_t pos, sengo_t sengo)
        {
            search_koma(result, pos, sengo,
                [](sengo_t) { return true; },
                [](pos_t pos, pos_t offset, bool aigoma) -> pos_t { return pos; });
        }

        /**
         * @breif 王に対する利きを更新する。
         */
        inline void update_oute()
        {
            for (std::size_t i = 0; i < 2; ++i)
            {
                oute_list[i].clear();
                search_kiki(std::back_inserter(oute_list[i]), ou_pos[i], tesu_to_sengo(i));
            }
        }

        inline void search_aigoma(aigoma_info_t & aigoma_info, sengo_t sengo)
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

        /**
         * @breif 合法手を生成する。
         * @param result 合法手の出力イテレータ
         */
        template<typename OutputIterator>
        inline void search_te(OutputIterator result)
        {
            aigoma_info_t aigoma_info;
            search_aigoma(aigoma_info, tesu_to_sengo(tesu));

            if (oute_list[tesu_to_sengo(tesu)].empty())
            {
#ifdef DISABLE_MOVEMENT_CACHE
                std::vector<pos_t> source_list;
                search_source(std::back_inserter(source_list), tesu_to_sengo(tesu));
                for (auto source : source_list)
                {
                    std::vector<pos_t> destination_list;
                    search_destination(std::back_inserter(destination_list), source, tesu_to_sengo(tesu));
#else
                for (auto & [source, destination_list] : move_table_list[tesu_to_sengo(tesu)])
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
                            print_te(te, tesu_to_sengo(tesu));
                            print_te(kifu.begin(), kifu.end());
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
                            search_kiki(std::back_inserter(kiki_list), destination, tesu_to_sengo(tesu));
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
                    if (mochigoma_list[tesu_to_sengo(tesu)][koma])
                        for (pos_t dst = 0; dst < width * height; ++dst)
                            if (can_put(koma, dst))
                                *result++ = { npos, dst, koma };
                }
            }
            else // 王手されている場合
            {
                pos_t r = reverse(tesu_to_sengo(tesu));
                pos_t src = ou_pos[tesu_to_sengo(tesu)];

                // 王を移動して王手を解除できる手を検索する。
                for (const pos_t * p = near_move_offsets(ou); *p; ++p)
                {
                    pos_t dst = src + *p * r;
                    if (!ban_t::out(dst)
                        && (ban[dst] == empty || to_sengo(ban[dst]) != to_sengo(ban[src])))
                    {
                        te_t te{ src, dst, ban[src], ban[dst], false };
                        do_te(te);
                        std::vector<kiki_t> kiki;
                        search_kiki(std::back_inserter(kiki), dst, sengo_next(tesu_to_sengo(tesu)));
                        undo_te(te);
                        if (kiki.empty())
                            *result++ = te;
                    }
                }

                auto & oute = oute_list[tesu_to_sengo(tesu)];
                if (oute.size() == 1)
                {
                    // 合駒の手を検索する。
                    if (oute.front().aigoma)
                    {
                        pos_t offset = oute.front().offset;
                        for (pos_t dst = ou_pos[tesu_to_sengo(tesu)] + offset; !ban_t::out(dst) && ban[dst] == empty; dst += offset)
                        {
                            // 駒を移動させる合駒
                            std::vector<kiki_t> kiki_list;
                            search_kiki(std::back_inserter(kiki_list), dst, sengo_next(tesu_to_sengo(tesu)));
                            for (kiki_t & kiki : kiki_list)
                            {
                                // 王で合駒はできない。
                                if (trim_sengo(ban[kiki.pos]) != ou)
                                {
                                    // 既に合駒として使っている駒は移動できない。
                                    auto aigoma_iter = aigoma_info.find(kiki.pos);
                                    bool is_aigoma = aigoma_iter != aigoma_info.end();
                                    if (is_aigoma)
                                        continue;

                                    if (can_promote(ban[kiki.pos], dst))
                                        *result++ = { kiki.pos, dst, ban[kiki.pos], ban[dst], true };
                                    if (!must_promote(ban[kiki.pos], dst))
                                        *result++ = { kiki.pos, dst, ban[kiki.pos], ban[dst], false };
                                }
                            }

                            // 駒を打つ合駒
                            for (koma_t koma = fu; koma <= hi; ++koma)
                                if (mochigoma_list[tesu_to_sengo(tesu)][koma])
                                    if (can_put(koma, dst))
                                        *result++ = { npos, dst, koma };
                        }

                        // 王手している駒を取る手を検索する。
                        pos_t dst = oute.front().pos;
                        std::vector<kiki_t> kiki;
                        search_kiki(std::back_inserter(kiki), dst, sengo_next(tesu_to_sengo(tesu)));
                        for (auto & k : kiki)
                        {
                            // 王を動かす手は既に検索済み
                            if (trim_sengo(ban[k.pos]) != ou)
                            {
                                // 既に合駒として使っている駒は移動できない。
                                auto aigoma_iter = aigoma_info.find(k.pos);
                                bool is_aigoma = aigoma_iter != aigoma_info.end();
                                if (is_aigoma)
                                    continue;

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
                    hash ^= hash_table.mochigoma_hash(koma, mochigoma_list[i][koma], tesu_to_sengo(i));

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
                std::size_t mochigoma_count = mochigoma_list[tesu_to_sengo(tesu)][te.srckoma];
                SHOGIPP_ASSERT(mochigoma_count > 0);
                hash ^= hash_table.koma_hash(te.srckoma, te.dst);
                hash ^= hash_table.mochigoma_hash(te.srckoma, mochigoma_count, tesu_to_sengo(tesu));
                hash ^= hash_table.mochigoma_hash(te.srckoma, mochigoma_count - 1, tesu_to_sengo(tesu));
            }
            else
            {
                SHOGIPP_ASSERT(!(!is_promotable(te.srckoma) && te.promote));
                hash ^= hash_table.koma_hash(te.srckoma, te.src);
                if (te.dstkoma != empty)
                {
                    std::size_t mochigoma_count = mochigoma_list[tesu_to_sengo(tesu)][te.dstkoma];
                    hash ^= hash_table.mochigoma_hash(to_mochigoma(te.dstkoma), mochigoma_count, tesu_to_sengo(tesu));
                    hash ^= hash_table.mochigoma_hash(to_mochigoma(te.dstkoma), mochigoma_count + 1, tesu_to_sengo(tesu));
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
        inline void print_te(const te_t & te, sengo_t sengo) const
        {
            std::cout << (sengo == sente ? "▲" : "△");
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
        inline void print_te(InputIterator first, InputIterator last) const
        {
            for (std::size_t i = 0; first != last; ++i)
            {
                std::printf("#%3d ", i + 1);
                print_te(*first++, tesu_to_sengo(tesu));
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
            for (unsigned char sengo = sente; sengo < sengo_size; ++sengo)
            {
                auto & oute = oute_list[sengo];
                if (oute_list[sengo].size() > 0)
                {
                    std::cout << "王手：";
                    for (std::size_t j = 0; j < oute_list[sengo].size(); ++j)
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
                print_te(kifu[i], tesu_to_sengo(i));
                std::cout << std::endl;
            }
        }

        /**
         * @breif 局面のハッシュ値を返す。
         * @return 局面のハッシュ値
         */
        inline hash_t hash() const
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

        inline void validate_ban_out()
        {
            for (pos_t pos = 0; pos < width * height; ++pos)
            {
                if (ban_t::out(pos))
                    SHOGIPP_ASSERT(ban[pos] == x);
            }
        }

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
                SHOGIPP_ASSERT(mochigoma_list[tesu_to_sengo(tesu)][te.srckoma] > 0);
                ban[te.dst] = tesu_to_sengo(tesu) ? to_gote(te.srckoma) : te.srckoma;
                --mochigoma_list[tesu_to_sengo(tesu)][te.srckoma];
            }
            else
            {
                SHOGIPP_ASSERT(!(!is_promotable(te.srckoma) && te.promote));
                if (ban[te.dst] != empty)
                    ++mochigoma_list[tesu_to_sengo(tesu)][ban[te.dst]];
                ban[te.dst] = te.promote ? to_promoted(ban[te.src]) : ban[te.src];
                ban[te.src] = empty;
                if (trim_sengo(te.srckoma) == ou)
                    ou_pos[tesu_to_sengo(tesu)] = te.dst;
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
                ++mochigoma_list[tesu_to_sengo(tesu)][te.srckoma];
                ban[te.dst] = empty;
            }
            else
            {
                if (trim_sengo(te.srckoma) == ou)
                    ou_pos[tesu_to_sengo(tesu)] = te.src;
                ban[te.src] = te.srckoma;
                ban[te.dst] = te.dstkoma;
                if (ban[te.dst] != empty)
                    --mochigoma_list[tesu_to_sengo(tesu)][ban[te.dst]];
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

        ban_t ban;                                      // 盤
        mochigoma_t mochigoma_list[sengo_size];         // 持ち駒 { 先手, 後手 }
        tesu_t tesu;                                    // 手数
        pos_t ou_pos[sengo_size];                       // 王の座標 { 先手, 後手 }
        std::vector<kiki_t> oute_list[sengo_size];      // 王に対する利き { 先手, 後手 }
        std::stack<hash_t> hash_stack;                  // それまでの各手番におけるハッシュ値を格納するスタック
        std::vector<te_t> kifu;                         // 棋譜
        move_table_t move_table_list[sengo_size];       // 合法手の表
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
        sengo_t sengo = tesu_to_sengo(tesu);
        std::vector<pos_t> kiki_or_himo_list;
        search_kiki_or_himo(std::back_inserter(kiki_or_himo_list), source, sengo);
        if (kiki_or_himo_list.size())
        {
            for (pos_t pos : kiki_or_himo_list)
            {
                ///*DEBUG*/std::cout << pos_to_string(source) << " " << pos_to_string(pos) << std::endl;
                sengo_t sengo = to_sengo(ban[pos]);
                std::vector<pos_t> new_destination_list;
                search_destination(std::back_inserter(new_destination_list), pos, sengo);
                auto & move_table = move_table_list[sengo];
                update_move_table(move_table, pos, std::move(new_destination_list));
            }
        }
    }

    void kyokumen_t::do_updating_move_table_list(const te_t & te)
    {
        SHOGIPP_ASSERT(tesu > 0);
        sengo_t sengo = tesu_to_sengo(tesu - 1);
        auto & self_move_table = move_table_list[sengo];
        auto & nonself_move_table = move_table_list[sengo];

        if (te.src == npos)
        {
            // 手の移動先を移動元とする自分の手を更新する。
            std::vector<pos_t> new_destination_list;
            search_destination(std::back_inserter(new_destination_list), te.dst, sengo);
            update_move_table(self_move_table, te.dst, std::move(new_destination_list));

            // 手の移動先に利いている走り駒の移動先を更新する。
            update_move_table_relative_to(te.dst);
        }
        else
        {
            // 手の移動先を移動元とする自分の手を更新する。
            std::vector<pos_t> new_destination_list;
            search_destination(std::back_inserter(new_destination_list), te.dst, sengo);
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
        sengo_t sengo = tesu_to_sengo(tesu);
        auto & self_move_table = move_table_list[sengo];
        auto & nonself_move_table = move_table_list[sengo_next(sengo)];

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
                search_destination(std::back_inserter(new_destination_list), te.dst, sengo_next(sengo));
                update_move_table(nonself_move_table, te.dst, std::move(new_destination_list));
            }

            // 手の移動元を移動元とする自分の手を更新する。
            std::vector<pos_t> new_destination_list;
            search_destination(std::back_inserter(new_destination_list), te.src, sengo);
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
        std::shared_ptr<abstract_evaluator_t> evaluators[sengo_size];
        kyokumen_t kyokumen;
        bool sente_win;

        inline void init()
        {
            kyokumen.init();
        }

        inline bool procedure()
        {
            auto & evaluator = evaluators[tesu_to_sengo(kyokumen.tesu)];

            if (kyokumen.tesu == 0)
            {
                for (unsigned char sengo = sente; sengo < sengo_size; ++sengo)
                    std::cout << tebanstr(static_cast<tesu_t>(sengo)) << "：" << evaluators[sengo]->name() << std::endl;
                std::cout << std::endl;
            }

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));
            if (te_list.empty())
            {
                auto & winner_evaluator = evaluators[sengo_next(tesu_to_sengo(kyokumen.tesu))];
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

            kyokumen.print_te(selected_te, tesu_to_sengo(kyokumen.tesu));
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
        unsigned long long sps = (unsigned long long)total_search_count * 1000 / duration;

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
            if (!ban_t::out(pos) && koma != empty)
                score += map[trim_sengo(koma)] * reverse(to_sengo(koma));
        }

        for (unsigned char sengo = 0; sengo < sengo_size; ++sengo)
            for (koma_t koma = fu; koma <= hi; ++koma)
                score += map[koma] * kyokumen.mochigoma_list[sengo][koma] * reverse(tesu_to_sengo(sengo));

        return score;
    }

    using scored_te = std::pair<te_t *, int>;

    /**
     * @breif 合法手を得点により並び替える。
     * @param first scored_te の先頭を指すランダムアクセスイテレータ
     * @param last scored_te の末尾を指すランダムアクセスイテレータ
     * @param sengo 先手か後手か
     */
    template<typename RandomAccessIterator>
    void sort_te_by_score(RandomAccessIterator first, RandomAccessIterator last)
    {
        std::sort(first, last, [](const scored_te & a, const scored_te & b) -> bool { return a.second > b.second; });
    }

    /**
     * @breif negamax で合法手を選択する評価関数オブジェクトの抽象クラス
     */
    struct negamax_evaluator_t
        : public abstract_evaluator_t
    {
        std::tuple<
            std::optional<te_t>,
            int
        > negamax(
            kyokumen_t & kyokumen,
            int depth,
            unsigned int & search_count)
        {
            if (depth <= 0)
                return { std::nullopt, eval(kyokumen) * reverse(tesu_to_sengo(kyokumen.tesu)) };

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));

            if (te_list.empty())
                return { std::nullopt, -std::numeric_limits<int>::max() };

            std::vector<scored_te> scored_te_list;
            auto inserter = std::back_inserter(scored_te_list);

            for (te_t & te : te_list)
            {
                ++search_count;
                kyokumen.do_te(te);
                auto [te_, score_] = negamax(kyokumen, depth - 1, search_count);
                kyokumen.undo_te(te);
                score_ *= -1;
                *inserter++ = { &te, score_ };
            }

            SHOGIPP_ASSERT(!te_list.empty());
            sort_te_by_score(scored_te_list.begin(), scored_te_list.end());
            return { *scored_te_list.front().first, scored_te_list.front().second };
        }

        /**
         * @breif 局面に対して minimax で合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            unsigned int search_count = 0;
            int default_max_depth = 2;
            auto [selected_te, score] = negamax(kyokumen, default_max_depth, search_count);
            std::cout << "読み手数：" << search_count << std::endl;
            std::cout << "評価値：" << score << std::endl;
            SHOGIPP_ASSERT(selected_te.has_value());
            return *selected_te;
        }

        /**
         * @breif 局面に対して評価値を返す。
         * @param kyokumen 局面
         * @return 局面の評価値
         */
        virtual int eval(kyokumen_t & kyokumen) = 0;
    };

    /**
     * @breif alphabeta で合法手を選択する評価関数オブジェクトの抽象クラス
     */
    struct alphabeta_evaluator_t
        : public abstract_evaluator_t
    {
        std::tuple<
            std::optional<te_t>,
            int
        > alphabeta(
            kyokumen_t & kyokumen,
            int depth,
            int alpha,
            int beta,
            unsigned int & search_count)
        {
            if (depth <= 0)
                return { std::nullopt, eval(kyokumen) * reverse(tesu_to_sengo(kyokumen.tesu)) };

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));

            if (te_list.empty())
                return { std::nullopt, -std::numeric_limits<int>::max() };

            std::vector<scored_te> scored_te_list;
            auto inserter = std::back_inserter(scored_te_list);

            for (te_t & te : te_list)
            {
                ++search_count;
                kyokumen.do_te(te);
                auto [te_, score_] = alphabeta(kyokumen, depth - 1, -beta, -alpha, search_count);
                kyokumen.undo_te(te);
                score_ *= -1;
                alpha = std::max(alpha, -score_);
                if (alpha >= beta)
                    break;
                *inserter++ = { &te, score_ };
            }

            SHOGIPP_ASSERT(!te_list.empty());
            sort_te_by_score(scored_te_list.begin(), scored_te_list.end());
            return { *scored_te_list.front().first, scored_te_list.front().second };
        }

        /**
         * @breif 局面に対して minimax で合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            unsigned int search_count = 0;
            int default_max_depth = 2;
            auto [selected_te, score] = alphabeta(kyokumen, default_max_depth, -std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), search_count);
            std::cout << "読み手数：" << search_count << std::endl;
            std::cout << "評価値：" << score << std::endl;
            SHOGIPP_ASSERT(selected_te.has_value());
            return *selected_te;
        }

        /**
         * @breif 局面に対して評価値を返す。
         * @param kyokumen 局面
         * @return 局面の評価値
         */
        virtual int eval(kyokumen_t & kyokumen) = 0;
    };

    /**
      * @breif alphabeta で合法手を選択する評価関数オブジェクトの抽象クラス
      */
    struct alphabeta_fukayomi_evaluator_t
        : public abstract_evaluator_t
    {
        std::tuple<
            std::optional<te_t>,
            int
        > alphabeta(
            kyokumen_t & kyokumen,
            int depth,
            int alpha,
            int beta,
            pos_t previous_destination,
            unsigned int & search_count)
        {
            if (depth <= 0)
            {
                // 前回駒取りが発生していた場合、取り合いを深読みする。
                if (previous_destination != npos)
                {
                    std::vector<pos_t> himo_list;
                    kyokumen.search_himo(std::back_inserter(himo_list), previous_destination, tesu_to_sengo(kyokumen.tesu));
                    if (himo_list.empty())
                        return { std::nullopt, eval(kyokumen) * reverse(tesu_to_sengo(kyokumen.tesu)) };
                    else
                    {
                        for (pos_t source : himo_list)
                        {
                            // 駒の移動元と移動先から成不成の手を生成する関数がほしい
                        }
                    }
                }
                else
                    return { std::nullopt, eval(kyokumen) * reverse(tesu_to_sengo(kyokumen.tesu)) };
            }

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));

            if (te_list.empty())
                return { std::nullopt, -std::numeric_limits<int>::max() };

            std::vector<scored_te> scored_te_list;
            auto inserter = std::back_inserter(scored_te_list);

            for (te_t & te : te_list)
            {
                ++search_count;
                pos_t destination = te.src != npos && te.dstkoma != empty ? te.dst : npos;
                kyokumen.do_te(te);
                auto [te_, score_] = alphabeta(kyokumen, depth - 1, -beta, -alpha, destination, search_count);
                kyokumen.undo_te(te);
                score_ *= -1;
                alpha = std::max(alpha, -score_);
                if (alpha >= beta)
                    break;
                *inserter++ = { &te, score_ };
            }

            SHOGIPP_ASSERT(!te_list.empty());
            sort_te_by_score(scored_te_list.begin(), scored_te_list.end());
            return { *scored_te_list.front().first, scored_te_list.front().second };
        }

        /**
         * @breif 局面に対して minimax で合法手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法手
         */
        te_t select_te(kyokumen_t & kyokumen) override
        {
            unsigned int search_count = 0;
            int default_max_depth = 2;
            auto [selected_te, score] = alphabeta(kyokumen, default_max_depth, -std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), search_count);
            std::cout << "読み手数：" << search_count << std::endl;
            std::cout << "評価値：" << score << std::endl;
            SHOGIPP_ASSERT(selected_te.has_value());
            return *selected_te;
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

            std::vector<scored_te> scores;
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
        : public alphabeta_evaluator_t
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

    struct hiyoko_evaluator_t
        : public negamax_evaluator_t
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
            constexpr int himo_score = 2;
            constexpr int kiki_score = 1;
            constexpr int ou_destination_score = 1;
            constexpr int anti_igyoku_score = 3;

            int score = 0;
            score += kyokumen_map_score(kyokumen, map);
            score *= 100;

            // 紐が付いている数だけ加点する。
            //for (pos_t pos = 0; pos < width * height; ++pos)
            //{
            //    if (!ban_t::out(pos) && kyokumen.ban[pos] != empty)
            //    {
            //        sengo_t sengo = to_sengo(kyokumen.ban[pos]);
            //        pos_t r = reverse(sengo);

            //        std::vector<pos_t> himo_list;
            //        kyokumen.search_himo(std::back_inserter(himo_list), pos, sengo);
            //        score += himo_list.size() * r * himo_score;

            //        std::vector<kiki_t> kiki_list;
            //        kyokumen.search_kiki(std::back_inserter(kiki_list), pos, sengo);
            //        score += kiki_list.size() * r * -1 * kiki_score;
            //    }
            //}

            //for (unsigned char sengo = 0; sengo < sengo_size; ++sengo)
            //{
            //    std::vector<pos_t> ou_destination;
            //    kyokumen.search_destination(std::back_inserter(ou_destination), kyokumen.ou_pos[sengo], tesu_to_sengo(sengo));
            //    score += ou_destination.size() * ou_destination_score;

            //    pos_t default_ou_pos = default_ou_pos_list[sengo];
            //    pos_t ou_pos = kyokumen.ou_pos[sengo];
            //    score += distance(ou_pos, default_ou_pos) * reverse(tesu_to_sengo(sengo)) * anti_igyoku_score;
            //}

            return score;
        }

        const char * name() override
        {
            return "ひよこ10級";
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