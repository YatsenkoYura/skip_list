#include "../include/skip_list.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <numeric>

template<typename SL>
std::vector<typename SL::value_type> to_vector(const SL& sl) {
    std::vector<typename SL::value_type> v;
    for (auto& x : sl) v.push_back(x);
    return v;
}

class SkipListInt : public ::testing::Test {
protected:
    skip_list<int> sl;
};

TEST_F(SkipListInt, EmptyOnCreate) {
    EXPECT_TRUE(sl.empty());
    EXPECT_EQ(sl.size(), 0u);
    EXPECT_EQ(sl.begin(), sl.end());
}

TEST_F(SkipListInt, InsertAndFind) {
    auto p1 = sl.insert(10);
    EXPECT_TRUE(p1.second);
    EXPECT_EQ(*p1.first, 10);
    EXPECT_FALSE(sl.empty());
    EXPECT_EQ(sl.size(), 1u);

    auto p2 = sl.insert(10);
    EXPECT_FALSE(p2.second);
    EXPECT_EQ(sl.size(), 1u);

    EXPECT_NE(sl.find(10), sl.end());
    EXPECT_EQ(*sl.find(10), 10);
    EXPECT_EQ(sl.count(10), 1u);
    EXPECT_TRUE(sl.contains(10));

    EXPECT_EQ(sl.find(5), sl.end());
    EXPECT_EQ(sl.count(5), 0u);
}

TEST_F(SkipListInt, Erase) {
    sl.insert(1);
    sl.insert(2);
    sl.insert(3);
    EXPECT_EQ(sl.size(), 3u);

    EXPECT_EQ(sl.erase(5), 0u);
    EXPECT_EQ(sl.size(), 3u);

    EXPECT_EQ(sl.erase(2), 1u);
    EXPECT_EQ(sl.size(), 2u);
    EXPECT_EQ(sl.find(2), sl.end());

    EXPECT_EQ(sl.erase(1), 1u);
    EXPECT_EQ(sl.erase(3), 1u);
    EXPECT_TRUE(sl.empty());
}

TEST_F(SkipListInt, Clear) {
    for (int i = 0; i < 100; ++i) sl.insert(i);
    EXPECT_EQ(sl.size(), 100u);
    sl.clear();
    EXPECT_TRUE(sl.empty());
    EXPECT_EQ(sl.size(), 0u);
    EXPECT_EQ(sl.begin(), sl.end());
}

TEST_F(SkipListInt, IteratorOrder) {
    std::vector<int> data = {5, 3, 9, 1, 7, 4};
    for (int x : data) sl.insert(x);
    std::sort(data.begin(), data.end());

    auto got = to_vector(sl);
    EXPECT_EQ(got, data);
}

TEST_F(SkipListInt, LowerUpperBounds) {
    for (int i = 0; i < 10; ++i) sl.insert(i*2);

    auto lb = sl.lower_bound(7);
    EXPECT_EQ(*lb, 8);

    auto ub = sl.upper_bound(8);
    EXPECT_EQ(*ub, 10);

    EXPECT_EQ(sl.lower_bound(0), sl.begin());
    EXPECT_EQ(sl.upper_bound(18), sl.end());
}

TEST_F(SkipListInt, CopyConstructorAndEquality) {
    for (int i = 0; i < 20; ++i) sl.insert(i);
    skip_list<int> copy(sl);
    EXPECT_EQ(copy, sl);
    sl.erase(5);
    EXPECT_NE(copy, sl);
}

TEST_F(SkipListInt, CopyAssignment) {
    skip_list<int> a, b;
    for (int i = 0; i < 5; ++i) a.insert(i);
    b = a;
    EXPECT_EQ(a, b);

    a = a;
    EXPECT_EQ(a, b);
}

TEST_F(SkipListInt, MoveConstructorAndAssignment) {
    for (int i = 0; i < 10; ++i) sl.insert(i);
    skip_list<int> moved(std::move(sl));
    EXPECT_TRUE(sl.empty());
    EXPECT_EQ(moved.size(), 10u);

    skip_list<int> sl2;
    sl2 = std::move(moved);
    EXPECT_TRUE(moved.empty());
    EXPECT_EQ(sl2.size(), 10u);
}

TEST(SkipListCustomCompare, DescendingOrder) {
    skip_list<int, std::greater<int>> slg;
    std::vector<int> data = {1,4,2,8,5,3};
    for (int x : data) slg.insert(x);
    std::sort(data.begin(), data.end(), std::greater<int>());

    std::vector<int> got;
    for (auto x : slg) got.push_back(x);
    EXPECT_EQ(got, data);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}