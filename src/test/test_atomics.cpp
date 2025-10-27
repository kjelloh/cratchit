#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp" // logger::
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
        std::vector<TaggedAmount> createUnorderedSampleEntries() {
            using namespace std::chrono;
            std::vector<TaggedAmount> result;

            // You’ll need to replace these constructor calls with actual ones matching your TaggedAmount API.
            // Here's an example stub:
            // TaggedAmount(year_month_day{2025y / March / 19}, CentsAmount(6000), {{"Account", "NORDEA"}, {"Text", "Sample Text"}, {"yyyymmdd_date", "20250319"}});
            // Please adjust as needed.

            // Example result: (unordered)
            result.emplace_back(year_month_day{2025y / March / 19}, CentsAmount(6000), TaggedAmount::Tags{{"Account", "NORDEA"}, {"Text", "Payment 1"}});
            result.emplace_back(year_month_day{2025y / March / 20}, CentsAmount(12000), TaggedAmount::Tags{{"Account", "OTHER"}, {"Text", "Payment 2"}});
            result.emplace_back(year_month_day{2025y / March / 19}, CentsAmount(3000), TaggedAmount::Tags{{"Account", "NORDEA"}, {"Text", "Payment 3"}});
            return result;
        }

        // Test fixture
        class TaggedAmountsFixture : public ::testing::Test {
        protected:
            std::vector<TaggedAmount> fixture_unordered_entries;

            void SetUp() override {
                fixture_unordered_entries = createUnorderedSampleEntries();
            }
        };

        // Test filtering by Account and Date
        TEST_F(TaggedAmountsFixture, FilterByAccountAndDate) {
            using namespace std::chrono;

            auto isNordea = tafw::keyEquals("Account", "NORDEA");
            auto isMarch19 = tafw::dateEquals(year_month_day{2025y / March / 19});

            auto filtered = fixture_unordered_entries | std::views::filter(tafw::and_(isNordea, isMarch19));

            std::vector<std::string> texts;
            for (auto const& ta : filtered) {
                texts.push_back(ta.tag_value("Text").value_or("[no text]"));
            }

            EXPECT_EQ(texts.size(), 2);
            EXPECT_NE(std::find(texts.begin(), texts.end(), "Payment 1"), texts.end());
            EXPECT_NE(std::find(texts.begin(), texts.end(), "Payment 3"), texts.end());
        }

        // Test sorting by date
        TEST_F(TaggedAmountsFixture, SortByDate) {
            auto entries = fixture_unordered_entries;
            tafw::sortByDate(entries);

            // Verify entries sorted by date ascending
            for (size_t i = 1; i < entries.size(); ++i) {
                EXPECT_LE(entries[i-1].date(), entries[i].date());
            }
        }

        TEST_F(TaggedAmountsFixture, TestValueCompare) {
          ASSERT_TRUE(fixture_unordered_entries[0] == fixture_unordered_entries[0]);
          auto cloned_ta = fixture_unordered_entries[0];
          ASSERT_TRUE(fixture_unordered_entries[0] == cloned_ta);
          auto original_ta = fixture_unordered_entries[0];
          auto different_tag_ta = fixture_unordered_entries[0];
          different_tag_ta.tags()["new_tag"] = "*new*";
          ASSERT_FALSE(different_tag_ta == original_ta);
          auto different_meta_tag_ta = fixture_unordered_entries[0];
          different_meta_tag_ta.tags()["_new_meta_tag"] = "*new meta*";
          ASSERT_FALSE(different_meta_tag_ta == original_ta);
          auto later_date = Date{std::chrono::sys_days{original_ta.date()} + std::chrono::days{1}};
          TaggedAmount later_ta{later_date,original_ta.cents_amount()};
          later_ta.tags() = original_ta.tags();
          ASSERT_TRUE(original_ta < later_ta);
        }

        TEST_F(TaggedAmountsFixture, TestCASCompare) {
          auto original_ta = fixture_unordered_entries[0];
          auto later_date = Date{std::chrono::sys_days{original_ta.date()} + std::chrono::days{1}};
          auto later_ta = TaggedAmount{later_date,original_ta.cents_amount(),original_ta.tags()};
          ASSERT_FALSE(to_value_id(original_ta) == to_value_id(later_ta));
          auto different_amount_ta = TaggedAmount{
             original_ta.date()
            ,original_ta.cents_amount() + CentsAmount{1}
            ,original_ta.tags()};
          ASSERT_FALSE(to_value_id(original_ta) == to_value_id(different_amount_ta));

          auto different_tag_ta = original_ta;
          different_tag_ta.tags()["new_tag"] = "*new*";
          ASSERT_FALSE(to_value_id(original_ta) == to_value_id(different_tag_ta));
          auto different_meta_tag_ta = fixture_unordered_entries[0];
          different_meta_tag_ta.tags()["_new_meta_tag"] = "*new meta*";
          ASSERT_FALSE(to_value_id(original_ta) == to_value_id(different_meta_tag_ta));
        }


    } // tafw_suite

    namespace dotasfw_suite {

        // Date Ordered Tagges Amounts Framework Test Suite

        TaggedAmount create_tagged_amount(Date date,CentsAmount cents_amount,TaggedAmount::Tags&& tags) {
          return TaggedAmount{date,cents_amount,std::move(tags)};
        }

        // Helper to create some example TaggedAmount objects for testing
        DateOrderedTaggedAmountsContainer createSample() {
            DateOrderedTaggedAmountsContainer result{};
            auto tas = tests::atomics::tafw_suite::createUnorderedSampleEntries();
            result.dotas_insert_auto_ordered_sequence(tas);
            return result;
        }

        // Test fixture
        class DateOrderedTaggedAmountsContainerFixture : public ::testing::Test {
        protected:
            DateOrderedTaggedAmountsContainer fixture_dotas;

            void SetUp() override {
                logger::scope_logger log_raii{logger::development_trace,"TEST_F DateOrderedTaggedAmountsContainerFixture::Setup"};

                fixture_dotas = createSample();
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
          for (auto const& [lhs,rhs] : adjacent_pairs(dotas.ordered_tagged_amounts())) {
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
        TEST_F(DateOrderedTaggedAmountsContainerFixture, OrderedTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, OrderedTest)"};

          auto tas = fixture_dotas.ordered_tagged_amounts();

          auto is_invalid_order = [](TaggedAmount const& lhs,TaggedAmount const& rhs) {
              bool result = lhs.date() > rhs.date();
              if (result) {
                std::print("\nInvalid Order\n{}\n{}",to_string(lhs),to_string(rhs));
              }
              return result;
            };


          auto iter = std::ranges::adjacent_find(
             tas
            ,is_invalid_order);

          auto is_all_date_ordered = (iter == tas.end()); // no violations found

          if (!is_all_date_ordered) {
            log_order(fixture_dotas);
          }

          EXPECT_EQ(is_all_date_ordered,true);          
        }

        // Test to insert value at end
        TEST_F(DateOrderedTaggedAmountsContainerFixture, InsertLastTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, InsertLastTest)"};

          auto original_tas = fixture_dotas.ordered_tagged_amounts();

          auto last_date = original_tas.back().date(); // trust non-empty
          auto later_date = Date{std::chrono::sys_days{last_date} + std::chrono::days{1}};
          auto new_ta = create_tagged_amount(later_date,CentsAmount{7000},TaggedAmount::Tags{{"Account", "NORDEA"}, {"Text", "*NEW*"}});
          auto [value_id,is_new_value] = fixture_dotas.dotas_insert_auto_ordered_value(new_ta);

          auto new_tas = fixture_dotas.ordered_tagged_amounts();
          auto result = (new_tas.back().date() == new_ta.date());
          EXPECT_TRUE(result);
        }

        // Test to insert value at begining
        TEST_F(DateOrderedTaggedAmountsContainerFixture, InsertFirstTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, InsertFirstTest)"};

          // Insert as first
          auto original_tas = fixture_dotas.ordered_tagged_amounts();
          auto first_date = original_tas.front().date(); // trust non-empty
          auto earlier_date = Date{std::chrono::sys_days{first_date} - std::chrono::days{1}};
          auto new_ta = create_tagged_amount(earlier_date,CentsAmount{7000},TaggedAmount::Tags{{"Account", "NORDEA"}, {"Text", "*NEW*"}});
          auto [value_id,is_new_value] = fixture_dotas.dotas_insert_auto_ordered_value(new_ta);

          auto new_tas = fixture_dotas.ordered_tagged_amounts();
          auto result = (new_tas.front() == new_ta);

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
                std::print("\nNEW:{}",to_string(ta));
              }
            );
            std::print("\n");
          }

          EXPECT_TRUE(result);
        }

        // Test that insert extends lengt by one
        TEST_F(DateOrderedTaggedAmountsContainerFixture, InsertlengthTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, InsertlengthTest)"};

          // Insert as first
          auto original_tas = fixture_dotas.ordered_tagged_amounts();
          auto first_date = original_tas.front().date(); // trust non-empty
          auto earlier_date = Date{std::chrono::sys_days{first_date} - std::chrono::days{1}};
          auto new_ta = create_tagged_amount(earlier_date,CentsAmount{7000},TaggedAmount::Tags{{"Account", "NORDEA"}, {"Text", "*NEW*"}});
          auto [value_id,is_new_value] = fixture_dotas.dotas_insert_auto_ordered_value(new_ta);

          auto new_tas = fixture_dotas.ordered_tagged_amounts();

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
                std::print("\nNEW:{}",to_string(ta));
              }
            );
            std::print("\n");
          }

          EXPECT_TRUE(result);
        }

        // Test append value
        TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendOrphanValueTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendOrphanValueTest)"};

          DateOrderedTaggedAmountsContainer dotas{};
          auto first_ta = create_tagged_amount(
             Date{std::chrono::year{2025} / std::chrono::October / 25}
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*First*"}});

          auto [value_id,was_inserted] = dotas.dotas_append_value(std::nullopt,first_ta);
          EXPECT_TRUE(was_inserted);
        }

        // Test append value
        TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendSecondValueTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendSecondValueTest)"};
          DateOrderedTaggedAmountsContainer dotas{};
          auto first_ta = create_tagged_amount(
             Date{std::chrono::year{2025} / std::chrono::October / 25}
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*First*"}});
          auto second_ta = create_tagged_amount(
             Date{std::chrono::year{2025} / std::chrono::October / 25}
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*Second*"}});

          auto [first_value_id,first_was_inserted] = dotas.dotas_append_value(std::nullopt,first_ta);
          auto [second_value_id,second_was_inserted] = dotas.dotas_append_value(first_value_id,second_ta);

          ASSERT_TRUE(first_was_inserted) << std::format("First insert failed");
          ASSERT_TRUE(second_was_inserted) << std::format("second insert failed");
          ASSERT_TRUE(dotas.ordered_ids_view().size() == 2) << std::format("Final size was not 2");
        }

        // Test append value
        TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendSecondValuePrevNullFailTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendSecondValuePrevNullFailTest)"};

          DateOrderedTaggedAmountsContainer dotas{};
          auto first_ta = create_tagged_amount(
             Date{std::chrono::year{2025} / std::chrono::October / 25}
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*First*"}});

          auto second_ta = create_tagged_amount(
             Date{std::chrono::year{2025} / std::chrono::October / 25}
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*Second*"}});

          auto [first_value_id,first_was_inserted] = dotas.dotas_append_value(std::nullopt,first_ta);
          auto [second_value_id,second_was_inserted] = dotas.dotas_append_value(std::nullopt,second_ta);

          ASSERT_TRUE(first_was_inserted) << std::format("First insert failed");
          ASSERT_FALSE(second_was_inserted) << std::format("second insert should have failed");
          ASSERT_TRUE(dotas.ordered_ids_view().size() == 1) << std::format("Final size was not 1");
        }

        // Test append value
        TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendSecondValueOlderOKTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendSecondValueOlderOKTest)"};

          DateOrderedTaggedAmountsContainer dotas{};
          auto first_ta = create_tagged_amount(
             Date{std::chrono::year{2025} / std::chrono::October / 25}
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*First*"}});

          auto first_date = first_ta.date();
          auto earlier_date = Date{std::chrono::sys_days{first_date} - std::chrono::days{1}};

          auto second_ta = create_tagged_amount(
             earlier_date
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*Second*"}});

          auto [first_value_id,first_was_inserted] = dotas.dotas_append_value(std::nullopt,first_ta);
          auto [second_value_id,second_was_inserted] = dotas.dotas_append_value(first_value_id,second_ta);

          ASSERT_TRUE(first_was_inserted) << std::format("First insert failed");
          ASSERT_TRUE(second_was_inserted) << std::format("second insert failed (expected forced append)");
          ASSERT_TRUE(dotas.ordered_ids_view().size() == 2) << std::format("Final size was not 2");
        }

        // Test append value
        TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendSecondValueCompabilityTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendSecondValueCompabilityTest)"};

          DateOrderedTaggedAmountsContainer dotas{};
          auto first_ta = create_tagged_amount(
             Date{std::chrono::year{2025} / std::chrono::October / 25}
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*First*"}});

          auto second_ta = create_tagged_amount(
             Date{std::chrono::year{2025} / std::chrono::October / 25}
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*Second*"}});

          auto [first_value_id,first_was_inserted] = dotas.dotas_append_value(std::nullopt,first_ta);          
          auto [second_value_id,second_was_inserted] = dotas.dotas_append_value(first_value_id,second_ta,true);

          ASSERT_TRUE(first_was_inserted) << std::format("First insert failed");
          ASSERT_TRUE(second_was_inserted) << std::format("second insert failed (should have succeeded in compability mode)");
          ASSERT_TRUE(dotas.ordered_ids_view().size() == 2) << std::format("Final size was not 1");
        }

        class IsInvalidPrevLink {
        public:

          IsInvalidPrevLink(DateOrderedTaggedAmountsContainer const& dotas_ref)
            : m_dotas_ref{dotas_ref} {}

          bool operator()(TaggedAmount::ValueId lhs, TaggedAmount::ValueId rhs) {
              // lhs == rhs[_prev]

              auto maybe_rhs_ta = m_dotas_ref.cas().cas_repository_get(rhs);
              if (!maybe_rhs_ta) return true;                       // missing RHS entry → treat as invalid

              auto maybe_prev_id = to_maybe_value_id(
                  maybe_rhs_ta->tag_value("_prev").value_or(std::string{"null"})); // empty string -> no tag
              if (!maybe_prev_id) return true;                      // RHS missing _prev tag -> invalid

              auto result = lhs != *maybe_prev_id;                         // invalid if prev != lhs

              return result;
          }
        private:
          DateOrderedTaggedAmountsContainer const& m_dotas_ref;
        };

        TEST_F(DateOrderedTaggedAmountsContainerFixture, PrevOrderingTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, PrevOrderingTest)"};

          auto ordered_ids_view = fixture_dotas.ordered_ids_view();

          auto iter = std::ranges::adjacent_find(
             ordered_ids_view
            ,IsInvalidPrevLink{fixture_dotas});

          auto is_all_prev_ordered = (iter == ordered_ids_view.end()); // no violations found

          if (!is_all_prev_ordered) {
            std::print("ordered_ids_view: null");
            std::ranges::for_each(ordered_ids_view,[](auto value_id){
              std::print(" -> {:x}",value_id);
            });
            auto lhs = *iter;
            auto rhs = *(iter+1);
            if (auto maybe_ta = fixture_dotas.at(rhs)) {
              std::println("\nFailed at lhs:{:x} rhs:{:x} ta:{}",lhs,rhs,to_string(maybe_ta.value()));
            }
          }

          ASSERT_TRUE(is_all_prev_ordered);

        }

        // Test append value
        TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendSecondValueOlderCompabilityFailTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, AppendSecondValueOlderCompabilityFailTest)"};

          DateOrderedTaggedAmountsContainer dotas{};
          auto first_ta = create_tagged_amount(
             Date{std::chrono::year{2025} / std::chrono::October / 25}
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*First*"}});

          auto first_date = first_ta.date();
          auto earlier_date = Date{std::chrono::sys_days{first_date} - std::chrono::days{1}};

          auto second_ta = create_tagged_amount(
             earlier_date
            ,CentsAmount{7000}
            ,TaggedAmount::Tags{{"Text", "*Second*"}});

          auto [first_value_id,first_was_inserted] = dotas.dotas_append_value(std::nullopt,first_ta);
          auto [second_value_id,second_was_inserted] = dotas.dotas_append_value(first_value_id,second_ta,true);

          ASSERT_TRUE(first_was_inserted) << std::format("First insert failed");
          ASSERT_TRUE(second_was_inserted) << std::format("second insert should have falied (compability mode)");
          ASSERT_TRUE(dotas.ordered_ids_view().size() == 2) << std::format("Final size was not 1");
        }

        TEST_F(DateOrderedTaggedAmountsContainerFixture, InsertTas) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(DateOrderedTaggedAmountsContainerFixture, InsertTas)"};

          auto original_tas = fixture_dotas.ordered_tagged_amounts();
          auto first_date = original_tas[0].date();
          auto lhs_tas = 
              std::views::iota(0)
            | std::views::take(3)
            | std::views::transform([first_date,&original_tas](auto i){
                auto const& ta = original_tas[i];
                auto date = Date{std::chrono::sys_days{first_date} + std::chrono::days{2*i}};
                return TaggedAmount{date,ta.cents_amount(),ta.tags()};
              })
            | std::ranges::to<TaggedAmounts>();

          auto rhs_tas = 
              std::views::iota(0)
            | std::views::take(3)
            | std::views::transform([first_date,&original_tas](auto i){
                auto const& ta = original_tas[i];
                auto date = Date{std::chrono::sys_days{first_date} + std::chrono::days{2*i+1}}; // Interleaved dates
                return TaggedAmount{date,ta.cents_amount(),ta.tags()};
              })
            | std::ranges::to<TaggedAmounts>();

          auto dotas = DateOrderedTaggedAmountsContainer{};
          dotas.dotas_insert_auto_ordered_sequence(lhs_tas);
          dotas.dotas_insert_auto_ordered_sequence(rhs_tas);

          ASSERT_TRUE(dotas.ordered_ids_view().size() == 6);
          bool is_sorted = std::ranges::is_sorted(
             dotas.ordered_tagged_amounts()
            ,std::less{}
            ,[](auto const& ta){return ta.date();});
          ASSERT_TRUE(is_sorted);

          auto ordered_ids_view = dotas.ordered_ids_view();

          auto iter = std::ranges::adjacent_find(
             ordered_ids_view
            ,IsInvalidPrevLink{dotas});

          auto is_all_prev_ordered = (iter == ordered_ids_view.end()); // no violations found

          if (!is_all_prev_ordered) {
            std::print("ordered_ids_view: null");
            std::ranges::for_each(ordered_ids_view,[](auto value_id){
              std::print(" -> {:x}",value_id);
            });
            auto lhs = *iter;
            auto rhs = *(iter+1);
            if (auto maybe_ta = fixture_dotas.at(rhs)) {
              std::println("\nFailed at lhs:{:x} rhs:{:x} ta:{}",lhs,rhs,to_string(maybe_ta.value()));
            }
            else {
              std::println("\nFailed at lhs:{:x} rhs:{:x} ta:null",lhs,rhs);
            }
          }

          ASSERT_TRUE(is_all_prev_ordered);

        }

    } // dotasfw_suite

    namespace env2dotas_suite {
      // Environment to Date Ordered Tagged Amounts suite

      Environment createSample() {
        auto dotas = dotasfw_suite::createSample();
        auto id_ev_pairs = 
           dotas.ordered_tagged_amounts()
          | std::views::transform([](auto const& ta) -> Environment::MutableIdValuePair {
              return {to_value_id(ta),to_environment_value(ta)};
            });
        Environment result{};
        std::ranges::for_each(id_ev_pairs,[&result](auto const& pair){
          result["TaggedAmount"].push_back(pair);
        });
        return result;
      }

      // Test fixture
      class Env2DotasTestFixture : public ::testing::Test {
      protected:
          Environment fixture_env;

          void SetUp() override {
              logger::scope_logger log_raii{logger::development_trace,"TEST_F Env2DotasTestFixture::Setup"};
              fixture_env = createSample();
          }
      };

      TEST_F(Env2DotasTestFixture,Env2DotasHappyPath) {
        logger::scope_logger log_raii{logger::development_trace,"TEST_F(Env2DotasTestFixture,Env2DotasHappyPath)"};

        auto dotas = dotas_from_environment(fixture_env);

        ASSERT_TRUE(dotas.ordered_ids_view().size() == fixture_env["TaggedAmount"].size()) 
          << std::format(
                 "Should be same environment:{} -> dotas:{}"
                ,fixture_env["TaggedAmount"].size()
                ,dotas.ordered_ids_view().size());
      }

    } // env2dotasfw_suite

    
    // bool run_all() {
    //     std::cout << "Running atomic tests..." << std::endl;
        
    //     // Run gtest with filter for atomic tests only
    //     ::testing::GTEST_FLAG(filter) = "AtomicTests.*";
    //     int result = RUN_ALL_TESTS();
        
    //     return result == 0;
    // }
}

