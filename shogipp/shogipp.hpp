#include <iostream>

#include <cstdlib>
#include <ctime>
#include <vector>
#include <utility>
#include <algorithm>
#include <memory>
#include <sstream>
#include <unordered_map>

namespace shogipp
{

#ifdef NDEBUG
#define assert(expr)
#else
#define assert(expr) do { assert_impl((expr), #expr, __FILE__, __func__, __LINE__); } while (false)
#endif

    void assert_impl(bool assertion, const char * expr, const char * file, const char * func, unsigned int line)
    {
        if (!assertion)
        {
            std::ostringstream what;
            what << "Assertion failed: " << expr << ", file " << file << ", line " << line;
            std::cerr << what.str() << std::endl;
            std::terminate();
        }
    }

#define XY_TO_ROW_POS(x, y) ((y + PADDING_H) * W + x + PADDING_W)
#define XY_TO_POS(x, y) (y * 9 + x)

    using koma_t = unsigned char;
    enum : koma_t { EMPTY, FU, KYO, KEI, GIN, KIN, KAK, HI, OU, KIND = 15, NARI = 16, GOTE = 32, X = 0xFF };
    enum { W = 11, H = 13, PADDING_W = 1, PADDING_H = 2 };
    using hash_t = std::int32_t;

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

    const char * tebanstr[]{ "先手", "後手" };
    const char * numberstr[]{ "０", "１", "２", "３", "４", "５", "６", "７", "８", "９" };

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

    const char * to_string(koma_t koma)
    {
        static const char * str[][9]{ { "・", "歩", "香", "桂", "銀", "金", "角", "飛", "王" },
        { "・", "と", "杏", "圭", "全", "金", "馬", "竜", "王" } };
        assert((koma & KIND) < 9);
        return str[(koma & NARI) ? 1 : 0][koma & KIND];
    }

    static const char * danstr[]{ "一", "二", "三", "四", "五", "六", "七", "八", "九" };
    static const char * sujistr[]{ "９", "８", "７", "６", "５", "４", "３", "２", "１" };

    const char * pos_to_suji(int pos)
    {
        return sujistr[(pos % W) - 1];
    }

    const char * pos_to_dan(int pos)
    {
        return danstr[(pos / W) - 2];
    }

    int suji_dan_to_pos(int suji, int dan)
    {
        return W * (dan + PADDING_H) + suji + PADDING_W;
    }

    void print_pos(int pos)
    {
        std::cout << pos_to_suji(pos) << pos_to_dan(pos);
        std::cout.flush();
    }

    struct te_t
    {
        int src, dst;
        koma_t srckoma, dstkoma;
        bool nari;
    };

    /**
     * @breif 持ち駒
     */
    struct mochigoma_t
    {
        unsigned char cnt[HI - FU + 1];

        /**
         * @breif 持ち駒を初期化する。
         */
        void init()
        {
            std::fill(std::begin(cnt), std::end(cnt), 0);
        }

        void print() const
        {
            int kind = 0;
            for (std::size_t i = 0; i < std::size(cnt); ++i)
            {
                int j = std::size(cnt) - i - 1;
                if (cnt[j] > 0)
                {
                    std::cout << to_string(static_cast<koma_t>(j) + FU);
                    if (cnt[j] >= 2)
                        std::cout << numberstr[cnt[j]];
                    ++kind;
                }
            }
            if (kind == 0)
                std::cout << "なし";
            std::cout << std::endl;
        }

        unsigned char & operator [](size_t i)
        {
            assert(i <= HI - FU);
            return cnt[i];
        }

        const unsigned char & operator [](size_t i) const
        {
            assert(i <= HI - FU);
            return cnt[i];
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
        void init()
        {
            koma_t temp[]{
                X, X, X, X, X, X, X, X, X, X, X,
                X, X, X, X, X, X, X, X, X, X, X,
                X, GOTE | KYO, GOTE | KEI, GOTE | GIN, GOTE | KIN, GOTE | OU, GOTE | KIN, GOTE | GIN, GOTE | KEI, GOTE | KYO, X,
                X, EMPTY, GOTE | HI, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, GOTE | KAK, EMPTY, X,
                X, GOTE | FU, GOTE | FU, GOTE | FU, GOTE | FU, GOTE | FU, GOTE | FU, GOTE | FU, GOTE | FU, GOTE | FU, X,
                X, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, X,
                X, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, X,
                X, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, X,
                X, FU, FU, FU, FU, FU, FU, FU, FU, FU, X,
                X, EMPTY, KAK, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, HI, EMPTY, X,
                X, KYO, KEI, GIN, KIN, OU, KIN, GIN, KEI, KYO, X,
                X, X, X, X, X, X, X, X, X, X, X,
                X, X, X, X, X, X, X, X, X, X, X,
            };
            std::copy(std::begin(temp), std::end(temp), std::begin(data));
        }

        koma_t & operator [](size_t i) { return data[i]; }
        const koma_t & operator [](size_t i) const { return data[i]; }

        /**
         * @breif 座標posが盤外か判定する。
         * @param pos 座標
         * @return 盤外の場合true
         */
        static bool out(int pos)
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
        void print() const
        {
            std::cout << "  ９ ８ ７ ６ ５ ４ ３ ２ １" << std::endl;
            std::cout << "+---------------------------+" << std::endl;
            for (int y = 0; y < 9; ++y)
            {
                std::cout << "|";
                for (int x = 0; x < 9; ++x)
                {
                    koma_t k = data[(y + 2) * W + x + 1];
                    std::cout << ((k != EMPTY && (k & GOTE)) ? "v" : " ") << to_string(k);
                }
                std::cout << "| " << danstr[y] << std::endl;
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
        void init()
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
        static bool can_promote(koma_t koma, int dst)
        {
            if ((koma & NARI) || (koma & KIND) == KIN || (koma & KIND) == OU)
                return false;
            if (koma & GOTE)
                return dst >= W * (6 + PADDING_H);
            return dst < W * (3 + PADDING_H);
        }

        /**
         * @breif 駒komaが座標dstに移動する場合に成りが必須か判定する。
         * @param koma 駒
         * @param dst 移動先の座標
         * @return 成りが必須の場合(komaが既に成っている場合、常にfalse)
         */
        static bool must_promote(koma_t koma, int dst)
        {
            if ((koma & KIND) == FU || (koma & KIND) == KYO)
            {
                if (koma & GOTE)
                    return dst >= W * (8 + PADDING_H);
                return dst < (W + PADDING_H);
            }
            else if ((koma & KIND) == KEI)
            {
                if (koma & GOTE)
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
                    if ((ban[src] & GOTE) == (ban[cur] & GOTE)) break;
                    *result++ = cur;
                    if ((ban[src] & GOTE) != (ban[cur] & GOTE)) break;
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
            if (!ban_t::out(cur) && (ban[cur] == EMPTY || bool(ban[cur] & GOTE) != bool(ban[src] & GOTE)))
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
            int cur, reverse = (koma & GOTE) ? -1 : 1;
            const int * p;

            if (koma & NARI)
            {
                if (kin_nari[koma & KIND])
                {
                    for (p = offsets[KIN - FU]; *p && (cur = src + *p * reverse, !ban_t::out(cur)); ++p)
                    {
                        if (ban[cur] == EMPTY)
                            *result++ = cur;
                        else
                        {
                            if ((ban[cur] & GOTE) == (koma & GOTE)) break;
                            *result++ = cur;
                            if ((ban[cur] & GOTE) != (koma & GOTE)) break;
                        }
                    }
                }
                else if ((koma & KIND) == KAK)
                {
                    for (int offset : kak_offsets)
                        find_dst_far(result, src, offset * reverse);
                    for (int offset : hi_offsets)
                        find_dst_near(result, src, offset * reverse);
                }
                else if ((koma & KIND) == HI)
                {
                    for (int offset : hi_offsets)
                        find_dst_far(result, src, offset * reverse);
                    for (int offset : kak_offsets)
                        find_dst_near(result, src, offset * reverse);
                }
                return;
            }

            if ((koma & KIND) == KYO)
            {
                find_dst_far(result, src, -W * reverse);
            }
            else if ((koma & KIND) == KAK)
            {
                for (int offset : kak_offsets)
                    find_dst_far(result, src, offset * reverse);
            }
            else if ((koma & KIND) == HI)
            {
                for (int offset : hi_offsets)
                    find_dst_far(result, src, offset * reverse);
            }
            else
            {
                for (p = offsets[(koma & KIND) - FU]; *p && (cur = src + *p * reverse, !ban_t::out(cur)); ++p)
                    if (ban[cur] == EMPTY || (ban[cur] & GOTE) != (koma & GOTE))
                        *result++ = cur;
            }
        }

        /**
         * @breif 持ち駒komaを座標dstに置くことができるか判定する。歩、香、桂に限りfalseを返す可能性がある。
         * @param koma 持ち駒
         * @param dst 移動先の座標
         * @return 置くことができる場合 true
         */
        bool can_put(koma_t koma, int dst)
        {
            if (ban[dst] != EMPTY)
                return false;
            if (tesu % 2)
            {
                if (((koma & KIND) == FU || (koma & KIND) == KYO) && dst >= W * 8)
                    return false;
                if ((koma & KIND) == KEI && dst >= W * 7)
                    return false;
            }
            if (((koma & KIND) == FU || (koma & KIND) == KYO) && dst < W)
                return false;
            if ((koma & KIND) == KEI && dst < W * 2)
                return false;
            if ((koma & KIND) == FU)
            {
                for (int i = 0; i < H; ++i)
                {
                    koma_t cur = ban[dst % W + W * i];
                    if ((cur & KIND) == FU && !(cur & NARI) && bool(tesu % 2) == bool(cur & GOTE))
                        return false;
                }

                // 打ち歩詰め
                int pos = dst - W * (tesu % 2 ? -1 : 1);
                if (!ban_t::out(pos) && (ban[pos] & KIND) == OU && bool(ban[pos] & GOTE) != bool(tesu % 2))
                {
                    te_t te{ -1, dst, static_cast<koma_t>(koma + FU), ban[dst] };
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
                if (ban[i] != EMPTY && ban[i] != X && bool(ban[i] & GOTE) == bool(tesu % 2))
                    *result++ = i;
        }

        /**
         * @breif 座標posから相対座標offset方向に走査し最初に駒が現れる座標を返す。
         * @param pos 走査を開始する座標
         * @param offset 走査する相対座標
         * @return 最初に駒が現れる座標(駒が見つからない場合-1)
         */
        int search(int pos, int offset) const
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
            if (int cur = pos + offset; !ban_t::out(cur) && gote != bool(ban[cur] & GOTE))
            {
                koma_t nkind = ban[cur] & ~GOTE;
                if (std::find(first, last, nkind) != last)
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
            if (int found = search(pos, offset); found != -1 && gote != bool(ban[found] & GOTE))
            {
                bool adjacency = found == pos + offset;
                if (!adjacency)
                {
                    koma_t nkind = ban[found] & ~GOTE;
                    if (std::find(first, last, nkind) != last)
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
            { -W - 1, { GIN, KIN, KAK, OU, NARI | FU, NARI | KYO, NARI | KEI, NARI | GIN, NARI | KAK, NARI | HI } },
            { -W, { FU, KYO, GIN, KIN, HI, OU, NARI | FU, NARI | KYO, NARI | KEI, NARI | GIN, NARI | KAK, NARI | HI } },
            { -W + 1, { GIN, KIN, KAK, OU, NARI | FU, NARI | KYO, NARI | KEI, NARI | GIN, NARI | KAK, NARI | HI } },
            { -1, { KIN, OU, HI, NARI | FU, NARI | KYO, NARI | KEI, NARI | GIN, NARI | KAK, NARI | HI } },
            { +1, { KIN, OU, HI, NARI | FU, NARI | KYO, NARI | KEI, NARI | GIN, NARI | KAK, NARI | HI } },
            { W - 1, { GIN, OU, KAK, NARI | FU, NARI | KYO, NARI | KEI, NARI | GIN, NARI | KAK, NARI | HI } },
            { W, { KIN, OU, HI, NARI | FU, NARI | KYO, NARI | KEI, NARI | GIN, NARI | KAK, NARI | HI } },
            { W + 1, { GIN, OU, KAK, NARI | FU, NARI | KYO, NARI | KEI, NARI | GIN, NARI | KAK, NARI | HI } },
            };
            static const pair far[]{
                { -W - 1, { KAK, NARI | KAK } },
            { -W, { KYO, HI, NARI | HI } },
            { -W + 1, { KAK, NARI | KAK } },
            { -1, { HI, NARI | HI } },
            { 1, { HI, NARI | HI } },
            { W - 1, { KAK, NARI | KAK } },
            { W, { HI, NARI | HI } },
            { W + 1, { KAK, NARI | KAK } }
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
        void update_oute()
        {
            for (std::size_t i = 0; i < 2; ++i)
            {
                oute_list[i].clear();
                search_kiki(std::back_inserter(oute_list[i]), ou_pos[i], bool(i % 2));
            }
        }

        void search_aigoma(aigoma_info_t & aigoma_info, bool gote)
        {
            using pair = std::pair<int, std::vector<koma_t>>;
            static const std::vector<pair> table{
                { -W, { KYO, HI, NARI | HI } },
            { -1, { HI, NARI | HI } },
            { +1, { HI, NARI | HI } },
            { +W, { HI, NARI | HI } },
            { -W - 1, { KAK, NARI | KAK } },
            { -W + 1, { KAK, NARI | KAK } },
            { +W - 1, { KAK, NARI | KAK } },
            { +W + 1, { KAK, NARI | KAK } },
            };

            int ou_pos = this->ou_pos[gote ? 1 : 0];
            int reverse = gote ? -1 : 1;
            for (const auto & [o, hashirigoma_list] : table)
            {
                int offset = o * reverse;
                int first = search(ou_pos, offset);
                if (first != -1 && bool(ban[first] & GOTE) == gote)
                {
                    int second = search(first, offset);
                    if (second != -1 && bool(ban[second] & GOTE) != gote)
                    {
                        koma_t kind = ban[second] & ~GOTE;
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
                    std::cout << to_string(ban[pos] & ~GOTE) << std::endl;
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
                        if ((ban[dst] & KIND) == OU)
                        {
                            te_t te{ src, dst, ban[src], ban[dst], false };
                            print_te(te);
                        }
#endif
                        assert((ban[dst] & KIND) != OU);

                        // 合駒は利きの範囲にしか移動できない。
                        if (is_aigoma)
                        {
                            const std::vector<int> & candidates = aigoma_iter->second;
                            if (std::find(candidates.begin(), candidates.end(), dst) == candidates.end())
                                continue;
                        }

                        // 利いている場所に王を移動させてはならない
                        if ((ban[src] & KIND) == OU)
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

                for (koma_t koma = 0; koma < HI - FU; ++koma)
                {
                    if (mochigoma_list[tesu % 2][koma])
                        for (int dst = 0; dst < W * H; ++dst)
                            if (can_put(koma + FU, dst))
                                *result++ = { -1, dst, static_cast<koma_t>(koma + FU), ban[dst] };
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
                        && (ban[dst] == EMPTY || bool(ban[dst] & GOTE) != bool(ban[src] & GOTE)))
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
                            for (koma_t koma = 0; koma < HI - FU; ++koma)
                                if (mochigoma_list[tesu % 2][koma])
                                    if (can_put(koma + FU, dst))
                                        *result++ = { -1, dst, static_cast<koma_t>(koma + FU), ban[dst] };
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
                            if ((ban[k.pos] & KIND) != OU)
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

        hash_t make_hash() const
        {
            hash_t temp = 0;
            for (int suji = 0; suji < 9; ++suji)
                for (int dan = 0; dan < 9; ++dan)
                    temp ^= hash_table.rand[ban[XY_TO_POS(suji, dan)] * (9 * 9) + XY_TO_POS(suji, dan)];
            return temp;
        }

        hash_t make_hash(hash_t hash, const te_t & te) const
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
                    ^ hash_table.rand[(te.srckoma | (te.nari ? NARI : 0)) * (9 * 9) + te.dst];
            }
        }

        /**
         * @breif 合法着手を出力する。
         * @param te 合法着手
         */
        void print_te(const te_t & te) const
        {
            std::cout << (tesu % 2 == 0 ? "▲" : "△");
            if (te.src != -1)
            {
                const char * naristr;
                if (can_promote(te.srckoma, te.dst))
                    naristr = te.nari ? "成" : "不成";
                else
                    naristr = "";
                std::cout << pos_to_suji(te.dst) << pos_to_dan(te.dst) << to_string(te.srckoma & (~GOTE)) << naristr << "(" << te.src << ")" << std::endl;
            }
            else
            {
                std::cout << pos_to_suji(te.dst) << pos_to_dan(te.dst) << to_string(te.srckoma & (~GOTE)) << "打" << std::endl;
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

        void print_te()
        {
            std::vector<te_t> te;
            search_te(std::back_inserter(te));
            print_te(te.begin(), te.end());
        }

        void print_oute()
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
                        std::cout << to_string(ban[kiki.pos] & ~GOTE) << std::endl;
                    }
                }
            }
        }

        /**
         * @breif 局面を出力する。
         */
        void print()
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
        void do_te(const te_t & te)
        {
            if (te.src == -1)
            {
                ban[te.dst] = (te.srckoma | (tesu % 2 ? GOTE : 0));
                --mochigoma_list[tesu % 2][(te.srckoma & KIND) - FU];
            }
            else
            {
                assert(!((te.srckoma & KIND) == KIN && te.nari));
                assert(!((te.srckoma & KIND) == OU && te.nari));
                assert(!((te.srckoma & NARI) && te.nari));
                if (ban[te.dst] != EMPTY)
                    ++mochigoma_list[tesu % 2][(ban[te.dst] & KIND) - FU];
                ban[te.dst] = ban[te.src] | static_cast<koma_t>(te.nari ? NARI : 0);
                ban[te.src] = EMPTY;
                if ((te.srckoma & KIND) == OU)
                    ou_pos[tesu % 2] = te.dst;
            }
            ++tesu;
            update_oute();
        }

        /**
         * @breif 合法着手を実行する前に戻す。
         * @param te 合法着手
         */
        void undo_te(const te_t & te)
        {
            --tesu;
            if (te.src == -1)
            {
                ++mochigoma_list[tesu % 2][(te.srckoma) - FU];
                ban[te.dst] = EMPTY;
            }
            else
            {
                if ((te.srckoma & KIND) == OU)
                    ou_pos[tesu % 2] = te.src;
                ban[te.src] = te.srckoma;
                ban[te.dst] = te.dstkoma;
                if (ban[te.dst] != EMPTY)
                    --mochigoma_list[tesu % 2][(ban[te.dst] & KIND) - FU];
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

        void init()
        {
            kyokumen.init();
        }

        bool procedure()
        {
            auto & evaluator = evaluators[kyokumen.tesu % 2];

            if (kyokumen.tesu == 0)
            {
                for (std::size_t i = 0; i < 2; ++i)
                    std::cout << tebanstr[i] << "：" << evaluators[i]->name() << std::endl;
                std::cout << std::endl;
            }

            std::vector<te_t> te_list;
            kyokumen.search_te(std::back_inserter(te_list));
            if (te_list.empty())
            {
                std::cout << kyokumen.tesu << "手詰み" << std::endl;
                kyokumen.print();
                std::cout << tebanstr[(kyokumen.tesu + 1) % 2] << "勝利 (" << evaluator->name() << ")";
                std::cout.flush();
                return false;
            }
            else
            {
                std::cout << (kyokumen.tesu + 1) << "手目" << tebanstr[kyokumen.tesu % 2] << "番" << std::endl;
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
                int reverse = bool(koma & GOTE) == bool(kyokumen.tesu % 2) ? 1 : -1;
                if (koma != EMPTY)
                {
                    if (koma & NARI)
                        score += pkomascore[(koma & KIND) - FU] * reverse;
                    else
                        score += komascore[(koma & KIND) - FU] * reverse;
                }
            }
            for (int i = 0; i <= HI - FU; ++i)
            {
                mochigoma_t * m0 = kyokumen.mochigoma_list + kyokumen.tesu % 2;
                mochigoma_t * m1 = kyokumen.mochigoma_list + (kyokumen.tesu + 1) % 2;
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
        int eval(kyokumen_t & kyokumen) override
        {
            return std::rand();
        }

        const char * name() override
        {
            return "random evaluator";
        }
    };

} // namespace shogipp