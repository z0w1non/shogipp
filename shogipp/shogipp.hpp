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
    using hash_t = std::int32_t;

    inline bool is_promoted(koma_t koma)
    {
        constexpr static bool map[]{
            false,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true,
            false, false, false, false, false, false, false, false, true, true, true, true, true, true
        };
        return map[koma];
    }

    inline bool is_promotable(koma_t koma)
    {
        constexpr static bool map[]{
            false,
            true, true, true, true, false, true, true, false, false, false, false, false, false, false,
            true, true, true, true, false, true, true, false, false, false, false, false, false, false,
        };
        return map[koma];
    }

    inline bool is_sente(koma_t koma)
    {
        constexpr static bool map[]{
            false,
            true, true, true, true, false, true, true, true, true, true, true, true, true, true,
            false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        };
        return map[koma];
    }

    inline bool is_gote(koma_t koma)
    {
        constexpr static bool map[]{
            false,
            false, false, false, false, false, false, false, false, false, false, false, false, false, false,
            true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        };
        return map[koma];
    }

    inline bool is_hasirigoma(koma_t koma)
    {
        constexpr static bool map[]{
            false,
            false, true, false, false, false, true, true, false, false, false, false, false, true, true,
            false, true, false, false, false, true, true, false, false, false, false, false, true, true,
        };
        return map[koma];
    }

    inline koma_t to_unpromoted(koma_t koma)
    {
        constexpr static koma_t map[]{
            FU, KYO, KEI, GIN, KIN, KAKU, HI, OU, FU, KYO, KEI, GIN, KAKU, HI,
            FU, KYO, KEI, GIN, KIN, KAKU, HI, OU, FU, KYO, KEI, GIN, KAKU, HI,
        };
        return map[koma];
    }

    inline koma_t to_promoted(koma_t koma)
    {
        SHOGIPP_ASSERT(is_promotable(koma));

        constexpr static koma_t map[]{
            EMPTY,
            NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, 0, UMA, RYU, 0, 0, 0, 0, 0, 0, 0,
            NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, 0, UMA, RYU, 0, 0, 0, 0, 0, 0, 0,
        };
        return map[koma];
    }

    inline koma_t trim_sengo(koma_t koma)
    {
        constexpr static koma_t map[]{
            EMPTY,
            FU, KYO, KEI, GIN, KIN, KAKU, HI, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU,
            FU, KYO, KEI, GIN, KIN, KAKU, HI, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU,
        };
        return map[koma];
    }

    inline koma_t to_gote(koma_t koma)
    {
        constexpr static koma_t map[]{
            EMPTY,
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

    inline const char * tebanstr(koma_t koma)
    {
        const char * map[]{ "先手", "後手" };
        return map[koma];
    }

    inline const char * numberstr(koma_t koma) {
        const char * map[]{ "０", "１", "２", "３", "４", "５", "６", "７", "８", "９" };
        return map[koma];
    }
    
    static const int offsets[][9]{
        { -W },
    {},
    { -2 * W - 1, -2 * W + 1 },
    { -W - 1, -W, -W + 1, W - 1, W + 1 },
    { -W - 1, -W, -W + 1, -1, 1, W },
    {},
    {},
    { -W - 1, -W, -W + 1, -1, 1, W - 1, W, W + 1 },
    };
    static const bool kin_nari[]{ false, true, true, true, true, false, false, false, false };
    static const int kak_offsets[]{ -W - 1, -W + 1, W - 1, W + 1 };
    static const int hi_offsets[]{ -W, -1, 1, W };

    inline int pos_to_dan(int pos)
    {
        return pos / W - PADDING_H;
    }

    inline int pos_to_suji(int pos)
    {
        return pos % W - PADDING_W;
    }

    inline const char * to_string(koma_t koma)
    {
        static const char * map[]{
            "・",
            "歩", "香", "桂", "銀", "金", "角", "飛", "王", "と", "杏", "圭", "全", "馬", "竜",
            "歩", "香", "桂", "銀", "金", "角", "飛", "王", "と", "杏", "圭", "全", "馬", "竜",
        };
        SHOGIPP_ASSERT(koma < KOMA_ENUM_NUMBER);
        return map[koma];
    }

    inline const char * danstr(int pos)
    {
        static const char * map[]{ "一", "二", "三", "四", "五", "六", "七", "八", "九" };
        return map[pos];
    }

    inline const char * sujistr(int pos)
    {
        static const char * map[]{ "９", "８", "７", "６", "５", "４", "３", "２", "１" };
        return map[pos];
    }

    inline int suji_dan_to_pos(int suji, int dan)
    {
        return W * (dan + PADDING_H) + suji + PADDING_W;
    }

    void print_pos(int pos)
    {
        std::cout << sujistr(pos_to_suji(pos)) << danstr(pos_to_dan(pos));
        std::cout.flush();
    }

    struct te_t
    {
        int src, dst;
        koma_t srckoma, dstkoma;
        bool promote;
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
            int kind = 0;
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
            constexpr static std::size_t map[]{
                0,
                FU - FU, KYO - FU, KEI - FU, GIN - FU, KIN - FU, KAKU - FU, HI - FU, OU - FU,
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
        inline static bool out(int pos)
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
            for (int dan = 0; dan < 9; ++dan)
            {
                std::cout << "|";
                for (int suji = 0; suji < 9; ++suji)
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
        int offset;     // 利きの相対座標
        int pos;        // 利いている駒の座標
        bool aigoma;    // 合駒が可能か
    };

    using aigoma_info_t = std::unordered_map<int, std::vector<int>>;

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
        }

        /**
         * @breif 駒komaが座標dstに移動する場合に成りが可能か判定する。
         * @param koma 駒
         * @param dst 移動先の座標
         * @return 成りが可能の場合(komaが既に成っている場合、常にfalse)
         */
        inline static bool can_promote(koma_t koma, int dst)
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
        inline static bool must_promote(koma_t koma, int dst)
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
         * @breif 座標srcから移動可能の移動先を反復的に検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
         * @param offset 移動先の相対座標
         */
        template<typename OutputIterator>
        void find_dst_far(OutputIterator result, int src, int offset) const
        {
            int cur = src + offset;
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
        void find_dst_near(OutputIterator result, int src, int offset) const
        {
            int cur = src + offset;
            if (!ban_t::out(cur) && (ban[cur] == EMPTY || is_gote(ban[cur]) != is_gote(ban[src])))
                *result++ = cur;
        }

        /**
         * @breif 座標srcから移動可能の移動先を検索する。
         * @param result 移動先の座標の出力イテレータ
         * @param src 移動元の座標
         */
        template<typename OutputIterator>
        void find_dst(OutputIterator result, int src) const
        {
            koma_t koma = ban[src];
            int cur, reverse = (is_gote(koma)) ? -1 : 1;
            const int * p;

            if (is_promoted(koma))
            {
                if (kin_nari[trim_sengo(koma)])
                {
                    for (p = offsets[KIN - FU]; *p && (cur = src + *p * reverse, !ban_t::out(cur)); ++p)
                    {
                        if (ban[cur] == EMPTY)
                            *result++ = cur;
                        else
                        {
                            if (is_gote(ban[cur]) == is_gote(koma)) break;
                            *result++ = cur;
                            if (is_gote(ban[cur]) != is_gote(koma)) break;
                        }
                    }
                }
                else if (trim_sengo(koma) == KAKU)
                {
                    for (int offset : kak_offsets)
                        find_dst_far(result, src, offset * reverse);
                    for (int offset : hi_offsets)
                        find_dst_near(result, src, offset * reverse);
                }
                else if (trim_sengo(koma) == HI)
                {
                    for (int offset : hi_offsets)
                        find_dst_far(result, src, offset * reverse);
                    for (int offset : kak_offsets)
                        find_dst_near(result, src, offset * reverse);
                }
                return;
            }

            if (trim_sengo(koma) == KYO)
            {
                find_dst_far(result, src, -W * reverse);
            }
            else if (trim_sengo(koma) == KAKU)
            {
                for (int offset : kak_offsets)
                    find_dst_far(result, src, offset * reverse);
            }
            else if (trim_sengo(koma) == HI)
            {
                for (int offset : hi_offsets)
                    find_dst_far(result, src, offset * reverse);
            }
            else
            {
                for (p = offsets[trim_sengo(koma) - FU]; *p && (cur = src + *p * reverse, !ban_t::out(cur)); ++p)
                    if (ban[cur] == EMPTY || is_gote(ban[cur]) != (is_gote(koma)))
                        *result++ = cur;
            }
        }

        /**
         * @breif 持ち駒komaを座標dstに置くことができるか判定する。歩、香、桂に限りfalseを返す可能性がある。
         * @param koma 持ち駒
         * @param dst 移動先の座標
         * @return 置くことができる場合 true
         */
        inline bool can_put(koma_t koma, int dst)
        {
            if (ban[dst] != EMPTY)
                return false;
            if (tesu % 2)
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
                for (int i = 0; i < H; ++i)
                {
                    koma_t cur = ban[dst % W + W * i];
                    if (trim_sengo(cur) == FU && bool(tesu % 2) == is_gote(cur))
                        return false;
                }

                // 打ち歩詰め
                int pos = dst - W * (tesu % 2 ? -1 : 1);
                if (!ban_t::out(pos) && (trim_sengo(ban[pos])) == OU && is_gote(ban[pos]) != bool(tesu % 2))
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
         * @breif 移動元の座標を検索する。
         * @param result 出力イテレータ
         */
        template<typename OutputIterator>
        void find_src(OutputIterator result) const
        {
            for (int i = 0; i < W * H; ++i)
                if (ban[i] != EMPTY && ban[i] != X && is_gote(ban[i]) == bool(tesu % 2))
                    *result++ = i;
        }

        /**
         * @breif 座標posから相対座標offset方向に走査し最初に駒が現れる座標を返す。
         * @param pos 走査を開始する座標
         * @param offset 走査する相対座標
         * @return 最初に駒が現れる座標(駒が見つからない場合-1)
         */
        inline int search(int pos, int offset) const
        {
            int cur;
            for (cur = pos + offset; !ban_t::out(cur) && ban[cur] == EMPTY; cur += offset);
            if (ban_t::out(cur))
                return -1;
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
        void search_kiki_near(OutputIterator result, int pos, int offset, bool gote, InputIterator first, InputIterator last)
        {
            if (int cur = pos + offset; !ban_t::out(cur) && gote != is_gote(ban[cur]))
            {
                if (std::find(first, last, trim_sengo(ban[cur])) != last)
                    *result++ = { offset, cur, false };
            }
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
        void search_kiki_far(OutputIterator result, int pos, int offset, bool gote, InputIterator first, InputIterator last)
        {
            if (int found = search(pos, offset); found != -1 && gote != is_gote(ban[found]))
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
         * @breif 座標posに対する利きを検索する。
         * @param result 利きの出力イテレータ
         * @param pos 座標
         * @param gote 後手視点か
         */
        template<typename OutputIterator>
        void search_kiki(OutputIterator result, int pos, bool gote)
        {
            using pair = std::pair<int, std::vector<koma_t>>;
            static const pair near[]{
                { -2 * W - 1, { KEI } },
            { -2 * W + 1, { KEI } },
            { -W - 1, { GIN, KIN, KAKU, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
            { -W, { FU, KYO, GIN, KIN, HI, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
            { -W + 1, { GIN, KIN, KAKU, OU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
            { -1, { KIN, OU, HI, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
            { +1, { KIN, OU, HI, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
            { W - 1, { GIN, OU, KAKU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
            { W, { KIN, OU, HI, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
            { W + 1, { GIN, OU, KAKU, NARI_FU, NARI_KYO, NARI_KEI, NARI_GIN, UMA, RYU } },
            };
            static const pair far[]{
                { -W - 1, { KAKU, UMA } },
            { -W, { KYO, HI, RYU } },
            { -W + 1, { KAKU, UMA } },
            { -1, { HI, RYU } },
            { 1, { HI, RYU } },
            { W - 1, { KAKU, UMA } },
            { W, { HI, RYU } },
            { W + 1, { KAKU, UMA } }
            };

            int reverse = gote ? -1 : 1;
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
                search_kiki(std::back_inserter(oute_list[i]), ou_pos[i], bool(i % 2));
            }
        }

        inline void search_aigoma(aigoma_info_t & aigoma_info, bool gote)
        {
            using pair = std::pair<int, std::vector<koma_t>>;
            static const std::vector<pair> table{
                { -W, { KYO, HI, RYU } },
            { -1, { HI, RYU } },
            { +1, { HI, RYU } },
            { +W, { HI, RYU } },
            { -W - 1, { KAKU, UMA } },
            { -W + 1, { KAKU, UMA } },
            { +W - 1, { KAKU, UMA } },
            { +W + 1, { KAKU, UMA } },
            };

            int ou_pos = this->ou_pos[gote ? 1 : 0];
            int reverse = gote ? -1 : 1;
            for (const auto & [o, hashirigoma_list] : table)
            {
                int offset = o * reverse;
                int first = search(ou_pos, offset);
                if (first != -1 && is_gote(ban[first]) == gote)
                {
                    int second = search(first, offset);
                    if (second != -1 && is_gote(ban[second]) != gote)
                    {
                        koma_t kind = trim_sengo(ban[second]);
                        bool match = std::find(hashirigoma_list.begin(), hashirigoma_list.end(), kind) != hashirigoma_list.end();
                        if (match)
                        {
                            std::vector<int> candidates;
                            for (int candidate = second; candidate != ou_pos; candidate -= offset)
                                candidates.push_back(candidate);
                            aigoma_info[first] = std::move(candidates);
                        }
                    }
                }
            }
        }

        /**
         * @breif 合法着手を生成する。
         * @param result 合法着手の出力イテレータ
         */
        template<typename OutputIterator>
        void search_te(OutputIterator result)
        {
            if (oute_list[tesu % 2].empty())
            {
                aigoma_info_t aigoma_info;
                search_aigoma(aigoma_info, bool(tesu % 2));

#ifndef NDEBUG
                for (auto & [pos, candidates] : aigoma_info)
                {
                    std::cout << "合駒：";
                    print_pos(pos);
                    std::cout << to_string(trim_sengo(ban[pos])) << std::endl;
                }
#endif

                std::vector<int> found_src;
                find_src(std::back_inserter(found_src));
                for (int src : found_src)
                {
                    auto aigoma_iter = aigoma_info.find(src);
                    bool is_aigoma = aigoma_iter != aigoma_info.end();

                    std::vector<int> found_dst;
                    find_dst(std::back_inserter(found_dst), src);
                    for (int dst : found_dst)
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
                            const std::vector<int> & candidates = aigoma_iter->second;
                            if (std::find(candidates.begin(), candidates.end(), dst) == candidates.end())
                                continue;
                        }

                        // 利いている場所に王を移動させてはならない
                        if (trim_sengo(ban[src]) == OU)
                        {
                            std::vector<kiki_t> kiki_list;
                            search_kiki(std::back_inserter(kiki_list), dst, bool(tesu % 2));
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
                        for (int dst = 0; dst < W * H; ++dst)
                            if (can_put(koma, dst))
                                *result++ = { -1, dst, koma, ban[dst] };
                }
            }
            else // 王手されている場合
            {
                int reverse = (tesu % 2) ? -1 : 1;
                int src = ou_pos[tesu % 2];
                int dst;

                // 王を移動して王手を解除できる手を検索する。
                for (const int * p = offsets[OU - FU]; *p; ++p)
                {
                    dst = src + *p * reverse;
                    if (!ban_t::out(dst)
                        && (ban[dst] == EMPTY || is_gote(ban[dst]) != is_gote(ban[src])))
                    {
                        te_t te{ src, dst, ban[src], ban[dst], false };
                        do_te(te);
                        std::vector<kiki_t> kiki;
                        search_kiki(std::back_inserter(kiki), dst, !bool(tesu % 2));
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
                        int offset = oute.front().offset;
                        for (int dst = ou_pos[tesu % 2] + offset; !ban_t::out(dst) && ban[dst] == EMPTY; dst += offset)
                        {
                            // 駒を移動させる合駒
                            std::vector<kiki_t> kiki;
                            search_kiki(std::back_inserter(kiki), dst, tesu % 2 != 0);
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
                                        *result++ = { -1, dst, koma, ban[dst] };
                        }
                    }
                    else
                    {
                        // 王手している駒を取る手を検索する。
                        int dst = oute.front().pos;
                        std::vector<kiki_t> kiki;
                        search_kiki(std::back_inserter(kiki), dst, !bool(tesu % 2));
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

        inline hash_t make_hash() const
        {
            hash_t temp = 0;
            for (int suji = 0; suji < 9; ++suji)
                for (int dan = 0; dan < 9; ++dan)
                    temp ^= hash_table.rand[ban[XY_TO_POS(suji, dan)] * (9 * 9) + XY_TO_POS(suji, dan)];
            return temp;
        }

        inline hash_t make_hash(hash_t hash, const te_t & te) const
        {
            if (te.src == -1)
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
         * @breif 合法着手を出力する。
         * @param te 合法着手
         */
        inline void print_te(const te_t & te) const
        {
            std::cout << (tesu % 2 == 0 ? "▲" : "△");
            if (te.src != -1)
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
         * @breif 合法着手を出力する。
         * @param first 合法着手の入力イテレータのbegin
         * @param last 合法着手の入力イテレータのend
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
         * @breif 合法着手を実行する。
         * @param te 合法着手
         */
        inline void do_te(const te_t & te)
        {
            if (te.src == -1)
            {
                SHOGIPP_ASSERT(mochigoma_list[tesu % 2][te.srckoma] > 0);
                ban[te.dst] = tesu % 2 ? to_gote(te.srckoma) : te.srckoma;
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
         * @breif 合法着手を実行する前に戻す。
         * @param te 合法着手
         */
        inline void undo_te(const te_t & te)
        {
            SHOGIPP_ASSERT(tesu > 0);
            --tesu;
            if (te.src == -1)
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

        ban_t ban;                      // 盤
        mochigoma_t mochigoma_list[2];       // 持ち駒 { 先手, 後手 }
        int tesu;                       // 手数
        int ou_pos[2];                  // 王の座標 { 先手, 後手 }
        std::vector<kiki_t> oute_list[2];    // 王に対する利き { 先手, 後手 }
    };

    /**
     * @breif 評価関数オブジェクトのインターフェース
     */
    struct abstract_evaluator_t
    {
        virtual ~abstract_evaluator_t() {};

        /**
         * @breif 局面に対して合法着手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法着手
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
                    std::cout << tebanstr(static_cast<koma_t>(i)) << "：" << evaluators[i]->name() << std::endl;
                std::cout << std::endl;
            }

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));
            if (te_list.empty())
            {
                std::cout << kyokumen.tesu << "手詰み" << std::endl;
                kyokumen.print();
                std::cout << tebanstr((kyokumen.tesu + 1) % 2) << "勝利 (" << evaluator->name() << ")";
                std::cout.flush();
                return false;
            }
            else
            {
                std::cout << (kyokumen.tesu + 1) << "手目" << tebanstr(kyokumen.tesu % 2) << "番" << std::endl;
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
         * @breif 局面に対して評価値が最も高くなる合法着手を選択する。
         * @param kyokumen 局面
         * @return 選択された合法着手
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
                int reverse = bool(is_gote(koma)) == bool(kyokumen.tesu % 2) ? 1 : -1;
                if (koma != EMPTY)
                {
                    if (is_promoted(koma))
                        score += pkomascore[trim_sengo(koma) - FU] * reverse;
                    else
                        score += komascore[trim_sengo(koma) - FU] * reverse;
                }
            }
            for (int i = 0; i <= HI - FU; ++i)
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