#include "tests.hpp"
#include <gtest/gtest.h>
#include <numeric> // std::accumulate,
#include <vector>

#include "fiscal/amount/functional.hpp"

namespace test {

    namespace functional_suite {

        TEST(MapCombinatorTest, BasicTransformation) {
            std::vector<int> input = {1, 2, 3, 4, 5};

            // Apply a map: multiply each value by 10
            auto mapped = input 
                        | cratchit::functional::map([](int x) { return x * 10; });

            std::vector<int> expected = {10, 20, 30, 40, 50};

            auto it_mapped = mapped.begin();
            auto it_expected = expected.begin();

            for (; it_mapped != mapped.end() && it_expected != expected.end(); ++it_mapped, ++it_expected) {
                EXPECT_EQ(*it_mapped, *it_expected);
            }

            // Ensure both ranges are the same length
            EXPECT_EQ(it_mapped, mapped.end());
            EXPECT_EQ(it_expected, expected.end());
        }

        TEST(FoldCombinatorTest, WorksWithStdSum) {
            std::vector<int> values = {2, 3, 4};

            auto sum = cratchit::functional::fold(0, std::plus<>{});

            int result = sum(values);

            EXPECT_EQ(result, 9);
        }

        TEST(FoldCombinatorTest, WorksWithStdMultiplies) {
            std::vector<int> values = {2, 3, 4};

            auto product = cratchit::functional::fold(1, std::multiplies<>{});

            int result = product(values);

            EXPECT_EQ(result, 24);
        }

        TEST(ScanCombinatorTest, RunningSum) {
            std::vector<int> input = {1, 2, 3, 4};

            auto running_sum = cratchit::functional::scan(0, std::plus<>{});

            auto result = running_sum(input);

            std::vector<int> expected = {0, 1, 3, 6, 10};

            EXPECT_EQ(result, expected);
        }

        TEST(ScanCombinatorTest, RunningProduct) {
            std::vector<int> input = {2, 3, 4};

            auto running_product = cratchit::functional::scan(1, std::multiplies<>{});

            auto result = running_product(input);

            std::vector<int> expected = {1, 2, 6, 24};

            EXPECT_EQ(result, expected);
        }

        TEST(KeyCombinatorTest, GroupsByTag) {
            struct Amount {
                std::string tag;
                int cents;

                bool operator==(const Amount& other) const {
                    return tag == other.tag && cents == other.cents;
                }
            };
            std::vector<Amount> amounts = {
                {"food", 1000},
                {"rent", 50000},
                {"food", 500},
                {"utilities", 6000},
                {"rent", 45000}
            };

            auto grouped = cratchit::functional::key(amounts, [](const Amount& a) {
                return a.tag;
            });

            // Check keys (std::map orders keys)
            std::vector<std::string> expected_keys = {"food", "rent", "utilities"};
            std::vector<std::string> actual_keys;
            for (const auto& [key, _] : grouped) {
                actual_keys.push_back(key);
            }
            EXPECT_EQ(actual_keys, expected_keys);

            // Check grouped contents for "food"
            ASSERT_TRUE(grouped.contains("food"));
            EXPECT_EQ(grouped.at("food").size(), 2);
            EXPECT_EQ(grouped.at("food")[0], (Amount{"food", 1000}));
            EXPECT_EQ(grouped.at("food")[1], (Amount{"food", 500}));

            // Check grouped contents for "rent"
            ASSERT_TRUE(grouped.contains("rent"));
            EXPECT_EQ(grouped.at("rent").size(), 2);
            EXPECT_EQ(grouped.at("rent")[0], (Amount{"rent", 50000}));
            EXPECT_EQ(grouped.at("rent")[1], (Amount{"rent", 45000}));

            // Check grouped contents for "utilities"
            ASSERT_TRUE(grouped.contains("utilities"));
            EXPECT_EQ(grouped.at("utilities").size(), 1);
            EXPECT_EQ(grouped.at("utilities")[0], (Amount{"utilities", 6000}));
        }

        TEST(StencilCombinatorTest, MovingSumWithWindowSize3) {
            std::vector<int> input = {1, 2, 3, 4, 5};

            // auto moving_sum = cratchit::functional::stencil(3, [](auto window) {
            //     return std::accumulate(window.begin(), window.end(), 0);
            // });
            auto moving_sum = cratchit::functional::stencil(
                3,
                [](auto window) {
                    return cratchit::functional::fold(0, std::plus<>{})(window);
                }
            );

            auto result = moving_sum(input);

            std::vector<int> expected = {
                1 + 2 + 3,
                2 + 3 + 4,
                3 + 4 + 5
            };

            EXPECT_EQ(result, expected);
        }

        TEST(StencilCombinatorTest, EmptyOnTooShortRange) {
            std::vector<int> input = {1, 2};

            // auto moving_sum = cratchit::functional::stencil(3, [](auto window) {
            //     return std::accumulate(window.begin(), window.end(), 0);
            // });
            auto moving_sum = cratchit::functional::stencil(
                3,
                [](auto window) {
                    return cratchit::functional::fold(0, std::plus<>{})(window);
                }
            );

            auto result = moving_sum(input);

            EXPECT_TRUE(result.empty());
        }

        TEST(StencilCombinatorTest, SingleElementWindows) {
            std::vector<int> input = {5, 6, 7};

            auto identity = cratchit::functional::stencil(1, [](auto window) {
                return *window.begin();  // single element
            });

            auto result = identity(input);
            EXPECT_EQ(result, input);
        }

        TEST(FilterCombinatorTest, FiltersValuesGreaterThanThree) {
            std::vector<int> input = {1, 2, 3, 4, 5};

            auto only_greater_than_3 = cratchit::functional::filter(
                [](int x) { return std::ranges::greater{}(x, 3); }
            );

            std::vector<int> result;
            for (int x : input | only_greater_than_3) {
                result.push_back(x);
            }

            std::vector<int> expected = {4, 5};
            EXPECT_EQ(result, expected);
        }

        TEST(PartitionTest, SeparatesEvenAndOdd) {
            std::vector<int> input = {1, 2, 3, 4, 5, 6};

            auto even_odd = cratchit::functional::partition([](int x) {
                return x % 2 == 0;
            });

            auto [evens, odds] = even_odd(input);

            EXPECT_EQ(evens, std::vector<int>({2, 4, 6}));
            EXPECT_EQ(odds, std::vector<int>({1, 3, 5}));
        }

        TEST(GroupByTest, GroupsItemsByCategory) {
            struct Item {
                std::string category;
                int amount;

                bool operator==(const Item&) const = default;
            };

            std::vector<Item> items = {
                {"food", 10}, {"transport", 20}, {"food", 15}, {"books", 5}
            };

            auto grouped = cratchit::functional::groupBy([](const Item& item) {
                return item.category;
            })(items);

            EXPECT_EQ(grouped["food"], (std::vector<Item>{{"food", 10}, {"food", 15}}));
            EXPECT_EQ(grouped["transport"], (std::vector<Item>{{"transport", 20}}));
            EXPECT_EQ(grouped["books"], (std::vector<Item>{{"books", 5}}));
        }

        TEST(SortByTest, SortsTransactionsByAmount) {
            struct Transaction {
                std::string date;
                int amount;

                bool operator==(const Transaction&) const = default;
            };

            std::vector<Transaction> transactions = {
                {"2023-01-02", 100},
                {"2023-01-01", 50},
                {"2023-01-03", 75}
            };

            auto sorted = cratchit::functional::sortBy([](const Transaction& t) {
                return t.amount;
            })(transactions);

            ASSERT_EQ(sorted.size(), 3);
            EXPECT_EQ(sorted[0].amount, 50);
            EXPECT_EQ(sorted[1].amount, 75);
            EXPECT_EQ(sorted[2].amount, 100);
        }

        TEST(SortByTest, SortsByDateThenAmount) {
            struct Transaction {
                std::string date;
                int amount;

                bool operator==(const Transaction&) const = default;
            };

            std::vector<Transaction> transactions = {
                {"2023-01-02", 200},
                {"2023-01-01", 100},
                {"2023-01-01", 50},
                {"2023-01-03", 75}
            };

            auto sorted = cratchit::functional::sortBy([](const Transaction& t) {
                return std::make_tuple(t.date, t.amount); // Lexicographic sort: date then amount
            })(transactions);

            ASSERT_EQ(sorted.size(), 4);
            EXPECT_EQ(sorted[0], (Transaction{"2023-01-01", 50}));
            EXPECT_EQ(sorted[1], (Transaction{"2023-01-01", 100}));
            EXPECT_EQ(sorted[2], (Transaction{"2023-01-02", 200}));
            EXPECT_EQ(sorted[3], (Transaction{"2023-01-03", 75}));
        }

    } // namespace functional_suite

    namespace tafw_suite {

        // Helper to create some example TaggedAmount objects for testing
        std::vector<TaggedAmount> createSampleEntries() {
            using namespace std::chrono;
            std::vector<TaggedAmount> entries;

            // You’ll need to replace these constructor calls with actual ones matching your TaggedAmount API.
            // Here's an example stub:
            // TaggedAmount(year_month_day{2025y / March / 19}, CentsAmount(6000), {{"Account", "NORDEA"}, {"Text", "Sample Text"}, {"yyyymmdd_date", "20250319"}});
            // Please adjust as needed.

            // Example entries:
            entries.emplace_back(year_month_day{2025y / March / 19}, CentsAmount(6000), TaggedAmount::Tags{{"Account", "NORDEA"}, {"Text", "Payment 1"}});
            entries.emplace_back(year_month_day{2025y / March / 20}, CentsAmount(12000), TaggedAmount::Tags{{"Account", "OTHER"}, {"Text", "Payment 2"}});
            entries.emplace_back(year_month_day{2025y / March / 19}, CentsAmount(3000), TaggedAmount::Tags{{"Account", "NORDEA"}, {"Text", "Payment 3"}});
            return entries;
        }

        // Test fixture (optional if you want setup/teardown)
        class TaggedAmountTest : public ::testing::Test {
        protected:
            std::vector<TaggedAmount> entries;

            void SetUp() override {
                entries = createSampleEntries();
            }
        };

        // Test filtering by Account and Date
        TEST_F(TaggedAmountTest, FilterByAccountAndDate) {
            using namespace std::chrono;

            auto isNordea = tafw::keyEquals("Account", "NORDEA");
            auto isMarch19 = tafw::dateEquals(year_month_day{2025y / March / 19});

            auto filtered = entries | std::views::filter(tafw::and_(isNordea, isMarch19));

            std::vector<std::string> texts;
            for (auto const& ta : filtered) {
                texts.push_back(ta.tag_value("Text").value_or("[no text]"));
            }

            EXPECT_EQ(texts.size(), 2);
            EXPECT_NE(std::find(texts.begin(), texts.end(), "Payment 1"), texts.end());
            EXPECT_NE(std::find(texts.begin(), texts.end(), "Payment 3"), texts.end());
        }

        // Test sorting by date
        TEST_F(TaggedAmountTest, SortByDate) {
            tafw::sortByDate(entries);

            // Verify entries sorted by date ascending
            for (size_t i = 1; i < entries.size(); ++i) {
                EXPECT_LE(entries[i-1].date(), entries[i].date());
            }
        }

    } 

    int run_all() {
        ::testing::InitGoogleTest();
        return RUN_ALL_TESTS();
    }
} // namespace test
