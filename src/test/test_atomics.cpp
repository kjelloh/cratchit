#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <numeric> // std::accumulate,
#include <ranges> // std::ranges::is_sorted,
#include <print>

#include "fiscal/amount/functional.hpp"

namespace tests::atomics {

    // Dummy integration test using gtest - always passes
    TEST(AtomicTests, DummyAlwaysPass) {
        EXPECT_TRUE(true);
        ASSERT_STREQ("hello", "hello");
    }

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

        TEST(ZipTest, ZipsTwoVectorsCorrectly) {
            using cratchit::functional::zip;

            std::vector<int> a = {1, 2, 3};
            std::vector<char> b = {'a', 'b'};

            auto zipped = zip(a, b);

            ASSERT_EQ(zipped.size(), 2);
            EXPECT_EQ(zipped[0], std::make_pair(1, 'a'));
            EXPECT_EQ(zipped[1], std::make_pair(2, 'b'));
        }

        TEST(FunctionalZipTest, ZipThreeVectors) {
            std::vector<int> v1{1, 2, 3, 4};
            std::vector<char> v2{'a', 'b', 'c'};
            std::vector<std::string> v3{"x", "y", "z", "w"};

            auto zipped = cratchit::functional::nzip(v1, v2, v3);

            // Should truncate to shortest vector size = 3
            ASSERT_EQ(zipped.size(), 3);

            EXPECT_EQ(zipped[0], std::make_tuple(1, 'a', std::string("x")));
            EXPECT_EQ(zipped[1], std::make_tuple(2, 'b', std::string("y")));
            EXPECT_EQ(zipped[2], std::make_tuple(3, 'c', std::string("z")));
        }

        TEST(FunctionalZipTest, ZipEmpty) {
            std::vector<int> v1{};
            std::vector<int> v2{1, 2, 3};

            auto zipped = cratchit::functional::nzip(v1, v2);

            EXPECT_TRUE(zipped.empty());
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

    } // tafw_suite

    namespace dotasfw_suite {

        // Date Ordrered Tagges Amounts Framework Test Suite

        TaggedAmount create_tagged_amount(Date date,CentsAmount cents_amount,TaggedAmount::Tags&& tags) {
          return TaggedAmount{date,cents_amount,std::move(tags)};
        }

        // Helper to create some example TaggedAmount objects for testing
        DateOrderedTaggedAmountsContainer createSample() {
            DateOrderedTaggedAmountsContainer result{};
            auto tas = tests::atomics::tafw_suite::createSampleEntries();
            result.date_ordered_tagged_amounts_put_sequence(tas);
            return result;
        }

        // Test fixture (optional if you want setup/teardown)
        class DateOrderedTaggedAmountsContainerTest : public ::testing::Test {
        protected:
            DateOrderedTaggedAmountsContainer dotas;

            void SetUp() override {
                dotas = createSample();
            }
        };

        template <std::ranges::range R>
        auto adjacent_pairs(R&& r) {
            return std::views::zip(
                std::forward<R>(r) | std::views::take(r.size() - 1),
                std::forward<R>(r) | std::views::drop(1)
            );
        }        

        void log_order(DateOrderedTaggedAmountsContainer const& dotas) {
          std::print("\nDateOrderedTaggedAmountsContainer ordering listing");
          for (auto const& [lhs,rhs] : adjacent_pairs(dotas.ordered_tas_view())) {
            auto is_correct_order = (lhs.date() <= rhs.date());
            std::print(
               "\ncorrect order={}\n{}\n{}"
              ,is_correct_order
              ,to_string(lhs)
              ,to_string(rhs));
          }
          std::print("\n\n"); // Work with google test asuming post-newline logging
        }

        // Test correct order by default
        TEST_F(DateOrderedTaggedAmountsContainerTest, OrderedTest) {

          auto tas_view = dotas.ordered_tas_view();

          auto is_invalid_order = [](TaggedAmount const& lhs,TaggedAmount const& rhs) {
              bool result = lhs.date() > rhs.date();
              if (result) {
                std::print("\nInvalid Order\n{}\n{}",to_string(lhs),to_string(rhs));
              }
              return result;
            };


          auto iter = std::ranges::adjacent_find(
             tas_view
            ,is_invalid_order);

          auto is_all_date_ordered = (iter == tas_view.end()); // no violations found

          if (!is_all_date_ordered) {
            log_order(dotas);
          }

          EXPECT_EQ(is_all_date_ordered,true);          
        }

        // Test to insert value at end
        TEST_F(DateOrderedTaggedAmountsContainerTest, InsertLastTest) {
          auto original_tas = std::ranges::to<TaggedAmounts>(dotas.ordered_tas_view());

          auto last_date = original_tas.back().date(); // trust non-empty
          auto later_date = Date{std::chrono::sys_days{last_date} + std::chrono::days{1}};
          auto new_ta = create_tagged_amount(later_date,CentsAmount{7000},TaggedAmount::Tags{{"Account", "NORDEA"}, {"Text", "Payment 5"}});
          auto [value_id,is_new_value] = dotas.date_ordered_tagged_amounts_insert_value(new_ta);

          auto new_tas = std::ranges::to<TaggedAmounts>(dotas.ordered_tas_view());
          auto result = (new_tas.back() == new_ta);
          EXPECT_TRUE(result);
        }

        // Test to insert value at begining
        TEST_F(DateOrderedTaggedAmountsContainerTest, InsertFirstTest) {
          // Insert as first
          auto original_tas = std::ranges::to<TaggedAmounts>(dotas.ordered_tas_view());
          auto first_date = original_tas.front().date(); // trust non-empty
          auto earlier_date = Date{std::chrono::sys_days{first_date} - std::chrono::days{1}};
          auto new_ta = create_tagged_amount(earlier_date,CentsAmount{7000},TaggedAmount::Tags{{"Account", "NORDEA"}, {"Text", "Payment 5"}});
          auto [value_id,is_new_value] = dotas.date_ordered_tagged_amounts_insert_value(new_ta);

          auto new_tas = std::ranges::to<TaggedAmounts>(dotas.ordered_tas_view());
          auto result = (new_tas.front() == new_ta);
          EXPECT_TRUE(result);
        }

        // Test that insert extends lengt by one
        TEST_F(DateOrderedTaggedAmountsContainerTest, InsertlengthTest) {
          // Insert as first
          auto original_tas = std::ranges::to<TaggedAmounts>(dotas.ordered_tas_view());
          auto first_date = original_tas.front().date(); // trust non-empty
          auto earlier_date = Date{std::chrono::sys_days{first_date} - std::chrono::days{1}};
          auto new_ta = create_tagged_amount(earlier_date,CentsAmount{7000},TaggedAmount::Tags{{"Account", "NORDEA"}, {"Text", "Payment 5"}});
          auto [value_id,is_new_value] = dotas.date_ordered_tagged_amounts_insert_value(new_ta);

          auto new_tas = std::ranges::to<TaggedAmounts>(dotas.ordered_tas_view());

          auto length_diff = (new_tas.size() - original_tas.size());
          auto result = (length_diff == 1);

          // Log
          if (!result) {
            std::ranges::for_each(
               original_tas
              ,[](auto const& ta) {
                std::print("\nORIGINAL:{}",to_string(ta));
              }
            );
            std::print("\n");
            std::ranges::for_each(
               new_tas
              ,[](auto const& ta) {
                std::print("\nEXTENDED:{}",to_string(ta));
              }
            );
            std::print("\n");
          }

          EXPECT_TRUE(result);
        }


    } // dotasfw_suite
    
    // bool run_all() {
    //     std::cout << "Running atomic tests..." << std::endl;
        
    //     // Run gtest with filter for atomic tests only
    //     ::testing::GTEST_FLAG(filter) = "AtomicTests.*";
    //     int result = RUN_ALL_TESTS();
        
    //     return result == 0;
    // }
}

