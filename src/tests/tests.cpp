#include "tests.hpp"
#include <gtest/gtest.h>
#include <numeric> // std::accumulate,

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
