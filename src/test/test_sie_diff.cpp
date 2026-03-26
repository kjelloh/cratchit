#include "fiscal/BASFramework.hpp" // BAS::anonymous::JournalEntry,...
#include "test/data/sie_test_sz_data.hpp"
#include "sie/SIEEnvironmentFramework.hpp" // sie_from_utf8_sv,...
#include "sie/sie_diff.hpp"
#include <gtest/gtest.h>

namespace tests {
  namespace sie {

    namespace sie_diff_test_suite {

      class FinancialEventCompareFixture : public ::testing::Test {
        public:
        std::vector<BAS::MDJournalEntry> m_lhs;
        std::vector<BAS::MDJournalEntry> m_rhs;
        void SetUp() override {
          using namespace std::chrono; // operator""y,d...

          BAS::MDJournalEntry base_lhs{
            BAS::WeakJournalEntryMeta{
              .series = 'A'
              ,.verno = 1
            }
            ,{
                .caption = "Event 1"
                ,.date = 2025y / 01 / 01d
                ,.account_transactions = {}
            }};

          auto base_rhs = base_lhs;

          m_lhs.push_back(base_lhs);
          m_rhs.push_back(base_rhs);

        }

      }; // FinancialEventFixture

      TEST_F(FinancialEventCompareFixture,FullyEqualOK) {
        {
          size_t lhs_ix = 0;
          size_t rhs_ix = 0;

          auto const& lhs = m_lhs[lhs_ix];
          auto const& rhs = m_rhs[rhs_ix];

          ASSERT_TRUE(hash_of_id_and_all_content(lhs) == hash_of_id_and_all_content(rhs)) << std::format(
            "Expected base \n\trhs:{} \n\tlhs:{} \n\tto be fully equal"
            ,to_string(lhs)
            ,to_string(rhs)
          );
        }
      }

      TEST_F(FinancialEventCompareFixture,AnyDiffDetected) {
        size_t lhs_ix = 0;
        size_t rhs_ix = 0;

        auto const& lhs = m_lhs[lhs_ix];
        {
          auto rhs = m_rhs[rhs_ix];
          auto& [meta,defacto] = rhs;

          meta.series = meta.series + 1;

          ASSERT_TRUE(hash_of_id_and_all_content(lhs) != hash_of_id_and_all_content(rhs)) << std::format(
            "Expected base \n\trhs:{} \n\tlhs:{} \n\tto be treated as different"
            ,to_string(lhs)
            ,to_string(rhs)
          );
        }
      }

    } // sie_diff_test_suite

    namespace parse_sie_file_suite {
        // SIE file parsing test suite

        class SIEFileParseFixture : public ::testing::Test {
        protected:

            SIEEnvironment fixture_three_entries_env{FiscalYear::to_current_fiscal_year(std::chrono::month{})};

            void SetUp() override {

              logger::scope_logger log_raii{logger::development_trace,"TEST SIEFileParseFixture::SetUp"};

              auto maybe_sie = sie_from_utf8_sv(sz_sie_three_transactions_text);

              try {
                fixture_three_entries_env = maybe_sie.value();
              }
              catch (std::exception const& e) {
                std::print("SIEFileParseFixture::SetUp: Failed - Exception:{}",e.what());
              }
              catch (...) {
                std::print("SIEFileParseFixture::SetUp: Failed - Exception(...)");
              }

            }
        };

        TEST(SIEFileParseTests,ParseEmpty) {
          logger::scope_logger log_raii{logger::development_trace,"TEST(SIEFileParseTests,ParseEmpty)"};

          auto maybe_sie = sie_from_utf8_sv("");

          ASSERT_FALSE(maybe_sie.has_value());
        }

        TEST(SIEFileParseTests,ParseMinimal) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(SIEFileParseFixture,ParseBasic)"};

          auto maybe_sie = sie_from_utf8_sv(sz_minimal_sie_text);

          ASSERT_TRUE(maybe_sie.has_value());
        }

        TEST(SIEFileParseTests,ParseTransactions) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(SIEFileParseFixture,ParseBasic)"};

          auto maybe_sie = sie_from_utf8_sv(sz_sie_three_transactions_text);
          
          ASSERT_TRUE(maybe_sie.has_value());
          int trans_count = maybe_sie.transform([](auto const& sie_env){
            return sie_env.journals_entry_count();
          }).value_or(-1);
          ASSERT_TRUE(trans_count == 3) << std::format("Parsed {} transactions from sz_sie_three_transactions_text",trans_count);
        }
    } // parse_sie_file_suite

    namespace sie_envs_merge_suite {
      // SIE Environments merge test suite

        std::vector<BAS::MDJournalEntry> to_sample_md_entries() {
          std::vector<BAS::MDJournalEntry> result{};
          {
            using namespace std::chrono; // operator""y,d...

            result.push_back(BAS::MDJournalEntry{
              BAS::WeakJournalEntryMeta{
                .series = 'A'
                ,.verno = 1
              }
              ,{
                  .caption = "Event 1"
                  ,.date = 2025y / 01 / 01d
                  ,.account_transactions = {}
              }});
            result.push_back(BAS::MDJournalEntry{
              BAS::WeakJournalEntryMeta{
                .series = 'A'
                ,.verno = 2
              }
              ,{
			            .caption = "Event 2"
			            ,.date = 2025y / 01 / 01d
			            ,.account_transactions = {}
              }});
          }
          return result;
        }

        using SIEEnvironmentTestFixture = parse_sie_file_suite::SIEFileParseFixture;

        TEST(SIEEnvironmentTests,EntryAddTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(SIEEnvironmentTests,EntryAddTest)"};

          auto entries = to_sample_md_entries();
          SIEEnvironment sie_env{FiscalYear::to_current_fiscal_year(std::chrono::month{1})};

          {
            auto add_result = sie_env.add_(entries[0]);
            ASSERT_TRUE(add_result) << "Expected add to empty env to succeed";
          }
          {
            auto add_result = sie_env.add_(entries[1]);
            ASSERT_TRUE(add_result) << "Expected add new valid entry to to succeed";
          }
          {
            auto add_result = sie_env.add_(entries[0]);
            ASSERT_FALSE(add_result) << "Expected re-add same value previous entry to fail";
          }
          {
            auto no_series_entry_0 = entries[0];
            no_series_entry_0.meta.series = ' ';
            no_series_entry_0.meta.verno = std::nullopt;
            auto add_result = sie_env.add_(no_series_entry_0);
            ASSERT_TRUE(add_result) << "Expected add of anonymous series,verno entry to succeed";
            ASSERT_TRUE(add_result.md_entry().meta.series == 'A') << "Expected add of anonymous series,verno entry to be addes in series 'A'";
          }
          {
            auto mutated_entry_0 = entries[0];
            mutated_entry_0.defacto.caption = mutated_entry_0.defacto.caption + " *mutated caption*";
            auto add_result = sie_env.add_(mutated_entry_0);
            ASSERT_FALSE(add_result) << "Expected re-add mutated previous entry to fail";
          }
        }

        TEST(SIEEnvironmentTests,EntryUpdateTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(SIEEnvironmentTests,EntryUpdateTest)"};

          auto entries = to_sample_md_entries();
          SIEEnvironment sie_env{FiscalYear::to_current_fiscal_year(std::chrono::month{1})};

          {
            auto update_result = sie_env.update_(entries[0]);
            ASSERT_FALSE(update_result) << "Expected update to empty env to fail (no entry to update)";
          }
          if (auto add_result = sie_env.add_(entries[0])) {
            auto update_result = sie_env.update_(entries[0]);
            ASSERT_TRUE(update_result) << "Expected update to existing same value to succeed";

            {
              auto mutated_entry_0 = entries[0];
              mutated_entry_0.defacto.caption = mutated_entry_0.defacto.caption + " *mutated caption*";
              
              auto update_result = sie_env.update_(mutated_entry_0);
              ASSERT_FALSE(update_result) << "Expected update to existing entry with different caption to fail";
            }
            {
              auto mutated_entry_0 = entries[0];
              mutated_entry_0.defacto.account_transactions.push_back(BAS::anonymous::AccountTransaction{
                 .account_no = 1920
                ,.transtext = std::string{"*New transaction*"}
                ,.amount = to_amount("12,00").value_or(0)
              });
              
              auto update_result = sie_env.update_(mutated_entry_0);
              ASSERT_FALSE(update_result) << "Expected update to existing entry with different trasnactions to fail";
            }
          }
        }

        TEST(SIEEnvironmentTests,EntryPostTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(SIEEnvironmentTests,EntryPostTest)"};

          auto entries = to_sample_md_entries();
          SIEEnvironment sie_env{FiscalYear::to_current_fiscal_year(std::chrono::month{1})};

          {
            auto post_result = sie_env.post_(entries[0]);
            ASSERT_TRUE(post_result) << "Expected post to empty env to succeed";
          }
          {
            auto post_result = sie_env.post_(entries[1]);
            ASSERT_TRUE(post_result) << "Expected post valid entry to to succeed";
          }
          {
            auto post_result = sie_env.post_(entries[0]);
            ASSERT_TRUE(post_result) << "Expected re-post previous entry to to succeed";
          }
          {
            auto mutated_entry_0 = entries[0];
            mutated_entry_0.defacto.caption = mutated_entry_0.defacto.caption + " *mutated caption*";
            auto post_result = sie_env.post_(mutated_entry_0);
            ASSERT_TRUE(!post_result) << "Expected re-post mutated previous entry to fail";
          }
        }

        TEST(SIEEnvironmentTests,EmptyStageEmptyTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST(SIEEnvironmentTests,EmptyStageEmptyTest)"};
          SIEEnvironment lhs{FiscalYear::to_current_fiscal_year(std::chrono::month{})};
          SIEEnvironment rhs{FiscalYear::to_current_fiscal_year(std::chrono::month{})};
          auto merged = lhs;
          auto stage_result = merged.stage_sie_(rhs);
          ASSERT_TRUE(stage_result.size() == 0);
          ASSERT_TRUE(merged.journals_entry_count() == 0);
        }

        TEST_F(SIEEnvironmentTestFixture,EmptyPostThreeTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(SIEEnvironmentTestFixture,EmptyPostThreeTest)"};
          ASSERT_TRUE(fixture_three_entries_env.journals_entry_count() == 3);
          SIEEnvironment merged{fixture_three_entries_env.fiscal_year()};

          for (auto const& [series,journal] : fixture_three_entries_env.journals()) {
            for (auto const& [verno,aje] : journal) {
              merged.post_({{.series=series,.verno=verno},aje});
            }
          }

          ASSERT_TRUE(merged.journals_entry_count() == 3);
        }

        TEST_F(SIEEnvironmentTestFixture,EmptyStageThreeTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(SIEEnvironmentTestFixture,EmptyStageThreeTest)"};
          SIEEnvironment merged{fixture_three_entries_env.fiscal_year()};
          auto stage_result = merged.stage_sie_(fixture_three_entries_env);
          ASSERT_TRUE(merged.journals_entry_count() == 3)
            << std::format("Expected 3 journal entries but found  :{}",merged.journals_entry_count());
          ASSERT_TRUE(merged.unposted().size() == 3)
            << std::format("Expected 3 staged entries but found unposted:{}",merged.unposted().size());
          ASSERT_TRUE(stage_result.size() == 3) << std::format("Expected stage_result.size() to be 3 (all staged accounted for) but found:{}",stage_result.size());
          ASSERT_TRUE(std::ranges::all_of(
             stage_result
            ,[](auto const& e){return static_cast<bool>(e);})) << "Expetced all entries to be staged ok";
        }

        TEST(SIEEnvironmentTests,StageToPostedOkTest) {
          logger::scope_logger log_raii{logger::development_trace,"TEST_F(SIEEnvironmentTestFixture,StageToPostedOkTest)"};

          auto entries = to_sample_md_entries();
          auto entry_0 = entries[0];
          auto entry_date = entry_0.defacto.date;
          SIEEnvironment posted{FiscalYear(entry_date.year(),std::chrono::month{1})};
          posted.post_(entry_0);
          auto stage_result = posted.stage_entry_(entry_0);

          ASSERT_TRUE(stage_result.now_posted()) 
            << std::format("Expected 'stage' to detect 'now posted'");
        }

    } // sie_envs_merge_suite
  } // sie
} // tests


