#ifndef TrainTicketSystem_System_HPP
#define TrainTicketSystem_System_HPP

#include "../lib/map.hpp"
#include "../lib/vector.hpp"
#include "../lib/linked_hashmap.hpp"
#include "../lib/tools.hpp"
#include "../db/BPlusTree.h"
#include "../db/CacheMap.h"
#include "Commander.hpp"

#define STORAGE_DIR ""
#define puts(x) std::cout<<x<<std::endl

namespace hnyls2002 {

    class System {

#define ret_value(x) ret_type(std::to_string(x))

        typedef sjtu::vector<std::string> ret_type;

        static const int UserNameMax = 21, PasswdMax = 31, NameMax = 16, mailAddMax = 31, privilegeMax = 3;
        static const int TrainIDMax = 21, StNameMax = 31, StNumMax = 101;
        static std::hash<std::string> Hash;

    public:

        System() : UserDb(STORAGE_DIR "./data/index1", STORAGE_DIR "./data/record1"),
                   BasicTrainDb(STORAGE_DIR "./data/index2", STORAGE_DIR "./data/record2"),
                   TrainDb(STORAGE_DIR "./data/index3", STORAGE_DIR "./data/record3"),
                   StDb(STORAGE_DIR "./data/index4", STORAGE_DIR "./data/record4"),
                   DayTrainDb(STORAGE_DIR "./data/index5", STORAGE_DIR "./data/record5"),
                   OrderDb(STORAGE_DIR "./data/index6", STORAGE_DIR "./data/record6"),
                   PendDb(STORAGE_DIR "./data/index7", STORAGE_DIR "./data/record7") {
        }

        void GetSize() const {
            UserDb.tree->GetSizeInfo();
            BasicTrainDb.tree->GetSizeInfo();
            TrainDb.tree->GetSizeInfo();
            StDb.tree->GetSizeInfo();
            DayTrainDb.tree->GetSizeInfo();
            OrderDb.tree->GetSizeInfo();
            PendDb.tree->GetSizeInfo();
        }

        static void GetCachedSize() {
            std::cerr << "UserDb : " << sizeof(UserDb) / 1e6 << std::endl;
            std::cerr << "BasicTrainDb : " << sizeof(BasicTrainDb) / 1e6 << std::endl;
            std::cerr << "TrainDb : " << sizeof(TrainDb) / 1e6 << std::endl;
            std::cerr << "StDb : " << sizeof(StDb) / 1e6 << std::endl;
            std::cerr << "DayTrainDb : " << sizeof(DayTrainDb) / 1e6 << std::endl;
            std::cerr << "OrderDb : " << sizeof(OrderDb) / 1e6 << std::endl;
            std::cerr << "PendDb : " << sizeof(PendDb) / 1e6 << std::endl;
        }

    private:

        struct UserInfo {
            fstr<UserNameMax> UserName;
            fstr<PasswdMax> Passwd;
            fstr<NameMax> Name;
            fstr<mailAddMax> mailAdd;
            fstr<privilegeMax> privilege;
            int OrderNum{};

            std::string to_string() const {
                std::string ret;
                ret += UserName.to_string() + " " + Name.to_string() + " ";
                ret += mailAdd.to_string() + " " + privilege.to_string();
                return ret;
            }
        };

        // UserName
        ds::CacheMap<size_t, UserInfo, 5, 339, 29> UserDb;

        // UserName
        sjtu::map<size_t, bool> Logged;

        struct BasicTrainInfo { // ??????????????????????????????????????????System????????????????????????????????????exit?????????????????????????????????
            bool is_released{};
            int StNum{}, SeatNum{};
            std::pair<Date, Date> SaleDate;
            char Type{};
        };

        // UserName
        ds::CacheMap<size_t, BasicTrainInfo, 5, 339, 127> BasicTrainDb;// ????????????

        struct TrainInfo { // ??????????????????????????????????????????query_train?????????????????????
            fstr<StNameMax> StName[StNumMax];
            int Prices[StNumMax]{};// ???????????????????????? i ??????i????????????
            std::pair<Time, Time> TimeTable[StNumMax];
        };

        //TrainID
        ds::CacheMap<size_t, TrainInfo, 5, 339, 2> TrainDb;

        struct StInfo {// ????????????????????????????????????????????????????????????????????????????????????????????????
            int Rank{}, Price{};// Rank???????????????????????????????????????????????????,Price?????????Prices[]
            fstr<TrainIDMax> TrainID;
            Time Arriving, Leaving;// ?????????TimeTable[]
        };

        class PairHash {
        public:
            size_t operator()(const std::pair<size_t, size_t> &x) {
                return x.first + x.second;
            }
        };

        //StName
        ds::CacheMap<std::pair<size_t, size_t>, StInfo, 5, 208, 68, PairHash> StDb;

        struct DayTrainInfo {
            int RemainSeats[StNumMax]{};// ???1??????SeatNum???????????????
            int Get_Remain(int l, int r) { // ??????l????????????r???
                int ret = 0x3f3f3f3f;
                for (int i = l + 1; i <= r; ++i)
                    ret = std::min(ret, RemainSeats[i]);
                return ret;
            }

            void Modify(int l, int r, int x) {
                for (int i = l + 1; i <= r; ++i)
                    RemainSeats[i] -= x;
            }
        };

        //TrainID
        class DayTrainHash {
        public:
            size_t operator()(const std::pair<size_t, Date> &x) {
                return x.first + x.second.num_d;
            }
        };

        ds::CacheMap<std::pair<size_t, Date>, DayTrainInfo, 5, 208, 9, DayTrainHash> DayTrainDb;

        //reference to stackoverflow
        //https://stackoverflow.com/questions/26331628/reference-to-non-static-member-function-must-be-called
        void (System::* func[17])(const CmdType &) ={&System::add_user, &System::login, &System::logout,
                                                     &System::query_profile, &System::modify_profile,
                                                     &System::add_train, &System::delete_train,
                                                     &System::release_train, &System::query_train,
                                                     &System::query_ticket, &System::query_transfer,
                                                     &System::buy_ticket, &System::query_order,
                                                     &System::refund_ticket, &System::rollback,
                                                     &System::clean, &System::exit};

    public:

        bool Opt(const std::string &str) {
            CmdType a = Parser(str);
            (this->*func[a.FuncID])(a);
            if (a.FuncID == 16)return true;
            return false;
        }

    private:

        void add_user(const CmdType &arg) {
            size_t u_h = Hash(arg['u']), c_h = Hash(arg['c']);
            if (UserDb.GetSize()) { // ??????????????????????????????
                // ???????????? || ????????????????????? || ??????????????????u
                if (Logged.find(c_h) == Logged.end() || UserDb[c_h].privilege.to_int() <= std::stoi(arg['g']) ||
                    UserDb.Find(u_h).first) {
                    std::cout << -1 << std::endl;
                    if (Logged.find(c_h) == Logged.end())puts("current-User NOT logg in!");
                    if (UserDb[c_h].privilege.to_int() <= std::stoi(arg['g']))puts("illegal privilege!");
                    if (UserDb.Find(u_h).first)puts("new-User already exist!");
                    return;
                }//
            }
            UserInfo User;
            User.UserName = arg['u'], User.Passwd = arg['p'], User.Name = arg['n'];
            User.mailAdd = arg['m'], User.privilege = arg['g'];
            User.OrderNum = 0;
            UserDb.Insert(u_h, User);
            std::cout << 0 << std::endl;
        }

        void login(const CmdType &arg) {
            size_t u_h = Hash(arg['u']);
            // ??????????????? || ?????????????????? || ????????????
            if (!UserDb.Find(u_h).first /*|| Logged.find(u_h) != Logged.end()*/ || UserDb[u_h].Passwd != arg['p']) {
                std::cout << -1 << std::endl;
                if (!UserDb.Find(u_h).first)puts("User NOT exist!");
                //if (Logged.find(u_h) != Logged.end())puts("User already log in!");
                if (UserDb[u_h].Passwd != arg['p'])puts("Password NOT correct!");
                return;
            }
            Logged[u_h] = true;
            std::cout << 0 << std::endl;
        }

        void logout(const CmdType &arg) {
            size_t u_h = Hash(arg['u']);
            if (Logged.find(u_h) == Logged.end()) {
                std::cout << -1 << std::endl;
                puts("You haven't logged in before!");
                return;
            }
            Logged.erase(Logged.find(u_h));
            std::cout << 0 << std::endl;
        }

        bool JudgeUserQM(const CmdType &arg) {// ??????c???????????????????????????u?????????
            size_t u_h = Hash(arg['u']), c_h = Hash(arg['c']);
            if (Logged.find(c_h) == Logged.end()) {
                std::cout << -1 << std::endl;
                puts("current-User NOT log in!");
                return false;// ????????????
            }
            if (!UserDb.Find(u_h).first) {
                std::cout << -1 << std::endl;
                puts("target-User NOT exist!");
                return false;// ????????????u
            }
            if (arg['u'] == arg['c'])return true;// ?????????????????????????????????????????????????????????
            if (UserDb[c_h].privilege.to_int() <= UserDb[u_h].privilege.to_int()) {
                std::cout << -1 << std::endl;
                puts("Privilege not satisfy!");
                return false;// ?????????????????????
            }
            return true;
        }

        void query_profile(const CmdType &arg) {
            size_t u_h = Hash(arg['u']);
            if (!JudgeUserQM(arg)) return;
            UserInfo User = UserDb[u_h];
            std::cout << User.to_string() << std::endl;
        }

        void modify_profile(const CmdType &arg) {
            size_t u_h = Hash(arg['u']), c_h = Hash(arg['c']);
            if (!JudgeUserQM(arg)) return;
            if (!arg['g'].empty() && UserDb[c_h].privilege.to_int() <= std::stoi(arg['g'])) {
                std::cout << -1 << std::endl;
                puts("Privilege not satisfy!");
                return;
            }
            UserInfo User = UserDb[u_h];// ???????????????????????????????????????????????????????????????
            if (!arg['p'].empty())User.Passwd = arg['p'];
            if (!arg['n'].empty())User.Name = arg['n'];
            if (!arg['m'].empty())User.mailAdd = arg['m'];
            if (!arg['g'].empty())User.privilege = arg['g'];
            UserDb.Modify(u_h, User);
            std::cout << User.to_string() << std::endl;
        }

        void add_train(const CmdType &arg) {
            size_t i_h = Hash(arg['i']);
            if (BasicTrainDb.Find(i_h).first) {// ???????????????????????????
                std::cout << -1 << std::endl;
                puts("Train already exist!");
                return;
            }
            BasicTrainInfo BasicTrain;
            TrainInfo Train;
            BasicTrain.SeatNum = std::stoi(arg['m']);
            BasicTrain.StNum = std::stoi(arg['n']);
            BasicTrain.Type = arg['y'][0];
            BasicTrain.is_released = false;// ?????????????????????

            ret_type tmp = split_cmd(arg['s'], '|');// StationName
            for (int i = 0; i < BasicTrain.StNum; ++i)
                Train.StName[i + 1] = tmp[i];

            tmp = split_cmd(arg['p'], '|'); // Prices n-1 ????????? ???1??????0
            Train.Prices[0] = 0;
            for (int i = 0; i < BasicTrain.StNum - 1; ++i)
                Train.Prices[i + 2] = Train.Prices[i + 1] + std::stoi(tmp[i]);

            int TravelTimes[StNumMax], StopOverTimes[StNumMax];
            tmp = split_cmd(arg['t'], '|');
            for (int i = 0; i < BasicTrain.StNum - 1; ++i)// TravelTimes n-1????????? 1 ??????
                TravelTimes[i + 1] = std::stoi(tmp[i]);
            tmp = split_cmd(arg['o'], '|');
            for (int i = 0; i < BasicTrain.StNum - 2; ++i)// StopOverTimes n-2?????????1??????
                StopOverTimes[i + 1] = std::stoi(tmp[i]);

            tmp = split_cmd(arg['d'], '|');
            BasicTrain.SaleDate.first = tmp[0];
            BasicTrain.SaleDate.second = tmp[1];

            Time BasicTime(tmp[0], arg['x']);// ???????????????????????????????????????TimeTable
            for (int i = 1; i <= BasicTrain.StNum; ++i) {
                // first-Arriving second-Leaving
                if (i > 1)Train.TimeTable[i].first = (BasicTime += TravelTimes[i - 1]);
                if (i > 1 && i < BasicTrain.StNum)Train.TimeTable[i].second = (BasicTime += StopOverTimes[i - 1]);
                if (i == 1)Train.TimeTable[i].second = BasicTime;
            }

            BasicTrainDb.Insert(i_h, BasicTrain);
            TrainDb.Insert(i_h, Train);

            std::cout << 0 << std::endl;
        }

        void delete_train(const CmdType &arg) {
            size_t i_h = Hash(arg['i']);
            if (!BasicTrainDb.Find(i_h).first || BasicTrainDb[i_h].is_released) {// ?????????????????? || ???????????????
                std::cout << -1 << std::endl;
                if (!BasicTrainDb.Find(i_h).first)puts("Train NOT exist");
                if (BasicTrainDb[i_h].is_released)puts("already published");
                return;
            }
            // ????????????BasicTrain?????????????????????Mp???
            BasicTrainDb.Remove(i_h);
            TrainDb.Remove(i_h);
            std::cout << 0 << std::endl;
        }

        void release_train(const CmdType &arg) {// ?????????DayTrain???????????????????????????????????????
            size_t i_h = Hash(arg['i']);
            if (!BasicTrainDb.Find(i_h).first) {// ?????????????????????
                std::cout << -1 << std::endl;
                puts("Train NOT exist");
                return;
            }
            auto BasicTrain = BasicTrainDb[i_h];
            if (BasicTrain.is_released) {// ???????????????
                std::cout << -1 << std::endl;
                puts("already published");
                return;
            }
            BasicTrain.is_released = true;
            BasicTrainDb.Modify(i_h, BasicTrain);

            auto Train = TrainDb[i_h];
            // ????????????????????????????????????StInfo????????????
            for (int i = 1; i <= BasicTrain.StNum; ++i) {
                StInfo St;
                St.Rank = i;
                St.Leaving = Train.TimeTable[i].second;
                St.Arriving = Train.TimeTable[i].first;
                St.Price = Train.Prices[i];
                St.TrainID = arg['i'];
                StDb.Insert({Hash(Train.StName[i].to_string()), Hash(arg['i'])}, St);
            }
            std::cout << 0 << std::endl;
        }

        void query_train(const CmdType &arg) {
            size_t i_h = Hash(arg['i']);
            if (!BasicTrainDb.Find(i_h).first) {// ?????????????????????
                std::cout << -1 << std::endl;
                puts("Train NOT exist");
                return;
            }
            auto BasicTrain = BasicTrainDb[i_h];
            auto Train = TrainDb[i_h];

            Date Day = arg['d'];
            if (Day < BasicTrain.SaleDate.first || BasicTrain.SaleDate.second < Day) {// ?????????????????????
                std::cout << -1 << std::endl;
                puts("Train not launch during this period!");
                return;
            }
            // ????????????
            bool is_in = true;
            DayTrainInfo DayTrain;
            if (!DayTrainDb.Find({i_h, arg['d']}).first)is_in = false;
            if (is_in) DayTrain = DayTrainDb[{i_h, arg['d']}];

            std::string ret;
            ret += arg['i'] + ' ' + BasicTrain.Type + '\n';
            //int IntervalDays = GetDate(Train, 1, arg['d']);
            int IntervalDays = Day - Date(Train.TimeTable[1].second);
            for (int i = 1; i <= BasicTrain.StNum; ++i) {
                ret += Train.StName[i].to_string() + ' ';
                ret += (i == 1 ? "xx-xx xx:xx" : Train.TimeTable[i].first.DayStep(IntervalDays).to_string()) + " -> ";
                ret += i == BasicTrain.StNum ? "xx-xx xx:xx" : Train.TimeTable[i].second.DayStep(
                        IntervalDays).to_string();
                ret += ' ' + std::to_string(Train.Prices[i]) + ' ';
                // ???????????????????????????????????????????????????
                if (i == BasicTrain.StNum)ret += 'x';
                else if (is_in) ret += std::to_string(DayTrain.RemainSeats[i + 1]);
                else ret += std::to_string(BasicTrain.SeatNum);
                ret += '\n';
            }
            std::cout << ret;
        }

        struct TicketType {
            fstr<TrainIDMax> TrainID;
            int TravelTime{}, Cost{}, RemainSeat{};
            Time Leaving, Arriving;

            std::string to_string(const std::string &s, const std::string &t) const {
                std::string ret;
                ret += TrainID.to_string() + " " + s + " " + Leaving.to_string() + " -> ";
                ret += t + " " + Arriving.to_string() + " ";
                ret += std::to_string(Cost) + " " + std::to_string(RemainSeat);
                return ret;
            }
        };

        static bool cmp_ticket_time(const TicketType &t1, const TicketType &t2) {
            if (t1.TravelTime != t2.TravelTime) return t1.TravelTime < t2.TravelTime;
            return t1.TrainID < t2.TrainID;
        }

        static bool cmp_ticket_cost(const TicketType &t1, const TicketType &t2) {
            if (t1.Cost != t2.Cost) return t1.Cost < t2.Cost;
            return t1.TrainID < t2.TrainID;
        }

        std::pair<int, TicketType>
        Get_Ticket(const fstr<TrainIDMax> &TrainID, const BasicTrainInfo &BasicTrain, const StInfo &from,
                   const StInfo &to, const Date &Day) {// ???????????????????????????????????????????????????????????????????????????
            TicketType ret;
//            int IntervalDays = GetDate(Train, pl, Day);
            int IntervalDays = Day - Date(from.Leaving);
            Date BuyDate = BasicTrain.SaleDate.first + IntervalDays;
            if (BuyDate < BasicTrain.SaleDate.first)return {-1, ret};// ???????????????
            if (BasicTrain.SaleDate.second < BuyDate)return {-2, ret};// ???????????????
            auto res = DayTrainDb.Find({Hash(TrainID.to_string()), BuyDate});
            if (res.first)ret.RemainSeat = res.second.second.Get_Remain(from.Rank, to.Rank);
            else ret.RemainSeat = BasicTrain.SeatNum;// ???????????????

            ret.Leaving = from.Leaving.DayStep(IntervalDays);
            ret.Arriving = to.Arriving.DayStep(IntervalDays);
            ret.TravelTime = ret.Arriving - ret.Leaving;
            ret.Cost = to.Price - from.Price;
            ret.TrainID = TrainID;
            return {0, ret};
        }

        void query_ticket(const CmdType &arg) {
            size_t s_h = Hash(arg['s']), t_h = Hash(arg['t']);
            auto it_s = StDb.tree->FindBigger({s_h, 0});
            auto it_t = StDb.tree->FindBigger({t_h, 0});
            sjtu::vector<StInfo> lis_s, lis_t;
            sjtu::vector<size_t> hash_s, hash_t;
            for (; !it_s.AtEnd() && (*it_s).first.first == s_h; ++it_s) {
                lis_s.push_back((*it_s).second);
                hash_s.push_back((*it_s).first.second);
            }
            for (; !it_t.AtEnd() && (*it_t).first.first == t_h; ++it_t) {
                lis_t.push_back((*it_t).second);
                hash_t.push_back((*it_t).first.second);
            }

            sjtu::vector<TicketType> tickets;
            for (int i = 0, j = 0; i < lis_s.size(); ++i) {
                while (j < lis_t.size() && hash_t[j] < hash_s[i])++j;
                if (j >= lis_t.size())break;
                if (hash_s[i] != hash_t[j])continue;
                if (lis_s[i].Rank >= lis_t[j].Rank)continue;
                auto TrainID = lis_t[j].TrainID.to_string();
                auto BasicTrain = BasicTrainDb[hash_s[i]];
                auto tik = Get_Ticket(TrainID, BasicTrain, lis_s[i], lis_t[j], arg['d']);
                if (tik.first == 0)tickets.push_back(tik.second);
            }

            auto cmp = arg['p'] == "cost" ? &System::cmp_ticket_cost : &System::cmp_ticket_time;
            sort(tickets.begin(), tickets.end(), cmp);
            std::string ret;
            ret += std::to_string(tickets.size()) + '\n';
            for (auto tik: tickets)
                ret += tik.to_string(arg['s'], arg['t']) + '\n';
            std::cout << ret;
        }

        struct TransType {
            TicketType tik1, tik2;
            std::string trans;

            int get_time() const { return tik2.Arriving - tik1.Leaving; }

            int get_cost() const { return tik1.Cost + tik2.Cost; }
        };

        static bool cmp_trans_time(const TransType &t1, const TransType &t2) {
            if (t1.get_time() != t2.get_time())return t1.get_time() < t2.get_time();
            else if (t1.get_cost() != t2.get_cost())return t1.get_cost() < t2.get_cost();
            else if (t1.tik1.TrainID != t2.tik1.TrainID)return t1.tik1.TrainID < t2.tik1.TrainID;
            return t1.tik2.TrainID < t2.tik2.TrainID;
        }

        static bool cmp_trans_cost(const TransType &t1, const TransType &t2) {
            if (t1.get_cost() != t2.get_cost())return t1.get_cost() < t2.get_cost();
            else if (t1.get_time() != t2.get_time())return t1.get_time() < t2.get_time();
            else if (t1.tik1.TrainID != t2.tik1.TrainID)return t1.tik1.TrainID < t2.tik1.TrainID;
            return t1.tik2.TrainID < t2.tik2.TrainID;
        }

        void query_transfer(const CmdType &arg) {
            size_t s_h = Hash(arg['s']), t_h = Hash(arg['t']);
            auto it_s = StDb.tree->FindBigger({s_h, 0});
            auto it_t = StDb.tree->FindBigger({t_h, 0});

            sjtu::vector<StInfo> lis_s, lis_t;

            for (; !it_s.AtEnd() && (*it_s).first.first == s_h; ++it_s)
                lis_s.push_back((*it_s).second);
            for (; !it_t.AtEnd() && (*it_t).first.first == t_h; ++it_t)
                lis_t.push_back((*it_t).second);

            // ???????????????list
            TransType tik;
            bool flag = false;
            auto cmp = arg['p'] == "cost" ? &System::cmp_trans_cost : &System::cmp_trans_time;
            for (auto &S: lis_s) {// ???????????? S
                sjtu::linked_hashmap<std::string, int> mp;
                auto TrainIDS = S.TrainID.to_string();
                auto TrainS = TrainDb[Hash(TrainIDS)];
                auto BasicTrainS = BasicTrainDb[Hash(TrainIDS)];
                for (int i = S.Rank + 1; i <= BasicTrainS.StNum; ++i) // map ?????? -s ??????????????????????????? (-s) -> (-trans)
                    mp[TrainS.StName[i].to_string()] = i;
                for (auto &T: lis_t) {// ???????????? T
                    auto TrainIDT = T.TrainID.to_string();
                    if (TrainIDS == TrainIDT)continue;// ?????????????????????
                    auto TrainT = TrainDb[Hash(TrainIDT)];
                    auto BasicTrainT = BasicTrainDb[Hash(TrainIDT)];
                    for (int i = 1; i < T.Rank; ++i) {// ?????????????????? -t ??????????????? (-trans) -> (-t)
                        fstr<StNameMax> trans = TrainT.StName[i];
                        if (mp.find(trans.to_string()) == mp.end())continue;// ???????????????
                        int pl = S.Rank, pr = mp[trans.to_string()], ql = i, qr = T.Rank;
                        StInfo TransS, TransT;
                        TransS.Rank = pr, TransT.Rank = ql;
                        TransS.Price = TrainS.Prices[pr], TransT.Price = TrainT.Prices[ql];
                        TransS.Arriving = TrainS.TimeTable[pr].first;
                        TransT.Leaving = TrainT.TimeTable[ql].second;
                        auto tik1 = Get_Ticket(TrainIDS, BasicTrainS, S, TransS, arg['d']);
                        if (tik1.first < 0) continue;// ?????????????????????
                        Time Arrival = std::max(tik1.second.Arriving, TransT.Leaving);
                        for (auto d = (Date) Arrival;; d += 1) {
                            auto tik2 = Get_Ticket(TrainIDT, BasicTrainT, TransT, T, d);
                            if (tik2.first == -2)break;// ?????????????????????????????????
                            if (tik2.first < 0)continue;
                            if (tik2.second.Leaving < Arrival)continue;// ??????????????????????????????????????????????????????Arriving Time!!!
                            if (!flag) tik = {tik1.second, tik2.second, trans.to_string()}, flag = true;
                            else tik = std::min(tik, {tik1.second, tik2.second, trans.to_string()}, cmp);
                            break;
                        }
                    }
                }
            }
            if (!flag) {
                std::cout << 0 << std::endl;
                return;
            }
            std::cout << tik.tik1.to_string(arg['s'], tik.trans) << std::endl;
            std::cout << tik.tik2.to_string(tik.trans, arg['t']) << std::endl;
        }

        enum StatusType {
            success = 0, pending = 1, refunded = 2
        };

        const std::string StatusToString[3] = {"[success]", "[pending]", "[refunded]"};

        struct Order {
            StatusType Status{};
            TicketType tik;
            fstr<StNameMax> From, To;
            Date Day;
            int pl{}, pr{}, TimeStamp{};
        };

        class PIHash {
        public:
            size_t operator()(const std::pair<size_t, int> &x) {
                return x.first + x.second;
            }
        };

        ds::CacheMap<std::pair<size_t, int>, Order, 5, 203, 26, PIHash> OrderDb;
        //bptree<std::pair<fstr<UserNameMax>, int>, Order> OrderDb;// ?????????????????????????????????

        struct PendType {
            fstr<UserNameMax> UserName;
            int TicketNum, pl, pr, id;// ????????????????????????????????????
        };

        class PDHash {
        public:
            size_t operator()(const std::pair<std::pair<size_t, Date>, int> &x) {
                return size_t(x.first.first + x.first.second.num_d) * x.second;
            }
        };

        ds::CacheMap<std::pair<std::pair<size_t, Date>, int>, PendType, 5, 145, 60, PDHash> PendDb;
        //bptree<std::pair<std::pair<fstr<TrainIDMax>, Date>, int>, PendType> PendDb;// ????????????[-?????????] ??????????????????

        void buy_ticket(const CmdType &arg) {
            size_t f_h = Hash(arg['f']), t_h = Hash(arg['t']), i_h = Hash(arg['i']), u_h = Hash(arg['u']);
            if (Logged.find(u_h) == Logged.end()) {// ????????????
                std::cout << -1 << std::endl;
                puts("User NOT log in!");
                return;
            }
            if (!BasicTrainDb.Find(i_h).first) {// ???????????????
                std::cout << -1 << std::endl;
                puts("Train NOT exist!");
                return;
            }
            auto BasicTrain = BasicTrainDb[i_h];
            if (!BasicTrain.is_released) {// ?????????release
                std::cout << -1 << std::endl;
                puts("Train NOT released!");
                return;
            }
            if (std::stoi(arg['n']) > BasicTrain.SeatNum) {// ?????????????????????????????????????????????????????????-1
                std::cout << -1 << std::endl;
                puts("So many tickets, can you afford ?");
                return;
            }
            // ?????????????????????
            auto St_res_f = StDb.Find({f_h, i_h});
            auto St_res_t = StDb.Find({t_h, i_h});
            if (!St_res_f.first || !St_res_t.first) {
                std::cout << -1 << std::endl;
                puts("It's a ghost station!!!");
                return;
            }

            auto St1 = St_res_f.second.second, St2 = St_res_t.second.second;
            if (St1.Rank >= St2.Rank) {
                std::cout << -1 << std::endl;
                puts("WRONG Direction!");
                return;
            }
            int IntervalDays = arg['d'] - Date(St1.Leaving);
            Date Day = BasicTrain.SaleDate.first + IntervalDays;
            if (Day < BasicTrain.SaleDate.first || BasicTrain.SaleDate.second < Day) {// ???????????????
                std::cout << -1 << std::endl;
                puts("The tickets NOT on sale during this period!");
                return;
            }
//            if (!DayTrainDb.Find({Train.TrainID, Day}).first) {// ?????????????????????????????????
            DayTrainInfo tmp;
            for (int i = 1; i <= BasicTrain.StNum; ++i)
                tmp.RemainSeats[i] = BasicTrain.SeatNum;
            //DayTrainDb[{Train.TrainID, Day}] = tmp;
            DayTrainDb.Insert({i_h, Day}, tmp);
//            }
            auto DayTrain = DayTrainDb[{i_h, Day}];
            auto User = UserDb[u_h];
            int RemainSeat = DayTrain.Get_Remain(St1.Rank, St2.Rank);
            int TicketNum = std::stoi(arg['n']);
            if (RemainSeat < TicketNum && (arg['q'].empty() || arg['q'] == "false")) {
                std::cout << -1 << std::endl;
                puts("No remain Seats!");
                return;
            }
            Order order;
            if (RemainSeat >= TicketNum) {// ????????????
                DayTrain.Modify(St1.Rank, St2.Rank, TicketNum);
                //DayTrainDb[{Train.TrainID, Day}] = DayTrain;
                DayTrainDb.Modify({i_h, Day}, DayTrain);
                order.Status = success;
            } else {
                //PendDb[{{Train.TrainID, Day}, arg.TimeStamp}] = PendType{arg['u'], TicketNum, pl, pr, User.OrderNum + 1};
                PendDb.Insert({{i_h, Day}, arg.TimeStamp},
                              PendType{arg['u'], TicketNum, St1.Rank, St2.Rank, User.OrderNum + 1});
                order.Status = pending;
            }
            TicketType tik;
            tik.Leaving = St1.Leaving.DayStep(IntervalDays);
            tik.Arriving = St2.Arriving.DayStep(IntervalDays);
            tik.RemainSeat = TicketNum;
            // ??????Cost????????????!!!
            tik.Cost = St2.Price - St1.Price;
            tik.TrainID = arg['i'];
            order.tik = tik, order.From = arg['f'], order.To = arg['t'], order.Day = Day;
            order.pl = St1.Rank, order.pr = St2.Rank, order.TimeStamp = arg.TimeStamp;
            //OrderDb[{User.UserName, -(++User.OrderNum)}] = order;
            OrderDb.Insert({Hash(User.UserName.to_string()), -(++User.OrderNum)}, order);
            UserDb.Modify(u_h, User);
            if (order.Status == success)std::cout << tik.Cost * TicketNum << std::endl;
            else std::cout << "queue" << std::endl;
        }

        void query_order(const CmdType &arg) {
            size_t u_h = Hash(arg['u']);
            if (Logged.find(u_h) == Logged.end()) {// ????????????
                std::cout << -1 << std::endl;
                puts("User NOT log in");
                return;
            }
            OrderDb.Flush();
            auto it = OrderDb.tree->FindBigger({u_h, -0x3f3f3f3f});
            std::string ret;
            ret += std::to_string(UserDb[u_h].OrderNum) + '\n';
            for (; !it.AtEnd() && (*it).first.first == u_h; ++it) {
                ret += StatusToString[(*it).second.Status] + ' ';
                ret += (*it).second.tik.to_string((*it).second.From.to_string(), (*it).second.To.to_string()) + '\n';
            }
            std::cout << ret;
        }

        void refund_ticket(const CmdType &arg) {
            size_t u_h = Hash(arg['u']);
            if (Logged.find(u_h) == Logged.end()) {// ????????????
                std::cout << -1 << std::endl;
                puts("User NOT login!");
                return;
            }
            int tot_order = UserDb[u_h].OrderNum;
            int id = arg['n'].empty() ? tot_order : tot_order - std::stoi(arg['n']) + 1;
            if (id <= 0) {// ?????????????????????
                std::cout << -1 << std::endl;
                puts("Oh guy, you don't have so much orders!");
                return;
            }
            auto order = OrderDb[{u_h, -id}];// order ??????????????? order2 ??????????????????
            if (order.Status == refunded) {
                std::cout << -1 << std::endl;
                puts("???The order already REFUNDED!!!");
                return;
            }
            bool flag = true;
            if (order.Status == pending)flag = false;
            order.Status = refunded;
            //OrderDb[{arg['u'], -id}] = order;
            auto res = OrderDb.Modify({u_h, -id}, order);

            std::pair<size_t, Date> info = {Hash(order.tik.TrainID.to_string()), order.Day};
            if (!flag) {// ??????????????????????????????????????????PendDb?????????
                PendDb.Remove({info, order.TimeStamp});
            } else {// ????????????????????????????????????
                auto DayTrain = DayTrainDb[info];
                DayTrain.Modify(order.pl, order.pr, -order.tik.RemainSeat);

                // ?????????????????????
                auto it = PendDb.tree->FindBigger({info, 0});
                for (; !it.AtEnd() && (*it).first.first == info;) {
                    int RemainTickets = DayTrain.Get_Remain((*it).second.pl, (*it).second.pr);
                    if (RemainTickets >= (*it).second.TicketNum) {// ????????????
                        auto order2 = OrderDb[{Hash((*it).second.UserName.to_string()), -(*it).second.id}];
                        order2.Status = success;
                        //OrderDb[{(*it).second.UserName, -(*it).second.id}] = order2;
                        OrderDb.Modify({Hash((*it).second.UserName.to_string()), -(*it).second.id}, order2);
                        DayTrain.Modify((*it).second.pl, (*it).second.pr, (*it).second.TicketNum);
                        PendDb.Remove((*it).first);// ???????????????????????????
                        ++it;
                    } else ++it;
                }
                //DayTrainDb[info] = DayTrain;
                DayTrainDb.Modify(info, DayTrain);
            }
            std::cout << 0 << std::endl;
        }

        void rollback(const CmdType &arg) {
            std::cout << 0 << std::endl;
        }

        void clean(const CmdType &arg) {
            std::cout << 0 << std::endl;
        }

        void exit(const CmdType &arg) {
            Logged.clear();
            std::cout << "bye" << std::endl;
        }

#undef ret_value

    };
}

#endif
