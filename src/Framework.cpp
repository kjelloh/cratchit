#include "Framework.hpp"
#include "logger/log.hpp"
#include "persistent/in/raw_text_read.hpp" // path_to_istream_ptr_step,istream_ptr_to_byte_buffer_step
#include "text/encoding_pipeline.hpp" // to_with_threshold_step_f, to_with_inferred_encoding, to_platform_encoded_string_step
#include "Key.hpp" // Key::Path as vector of csv fields in a line
#include "KeyValueMap.hpp"
#include <exception>
#include <sstream> // std::istringstream

// public:

Framework::Framework(std::filesystem::path persistent_file_path)
  : m_persistent_file_path{persistent_file_path} 
    ,m_valid{digest_persistent_file()} {}

bool Framework::is_valid() {
  return this->m_valid;
}


// private:

bool Framework::digest_persistent_file() {

  logger::scope_logger log_raii(logger::development_trace,"digest_persistent_file",logger::LogToConsole::OFF);

  try {

    auto maybe_string = persistent::in::text::monadic::path_to_istream_ptr_step(m_persistent_file_path)
      .and_then(persistent::in::text::monadic::istream_ptr_to_byte_buffer_step)
      .and_then(text::encoding::monadic::to_with_threshold_step_f(100))
      .and_then(text::encoding::monadic::to_with_inferred_encoding)
      .and_then(text::encoding::monadic::to_platform_encoded_string_step);

    if (!maybe_string) {
      std::string messages{};
      for (auto const& message : maybe_string.m_messages) {
        messages += std::string(" : ")  + message;
      }
      logger::business(
        " framework digest failed: {}"
        ,messages
      );
    }

    // auto maybe_key_values = persistent::in::text::maybe::path_to_istream_ptr_step(m_persistent_file_path)
    //   .and_then(persistent::in::text::maybe::istream_ptr_to_byte_buffer_step)
    //   .and_then(text::encoding::maybe::to_with_threshold_step_f(100))
    //   .and_then(text::encoding::maybe::to_with_inferred_encoding)
    //   .and_then(text::encoding::maybe::to_platform_encoded_string_step)

    auto maybe_key_values  = maybe_string.m_value
      .and_then([this](std::string const& csv_string) -> std::optional<KeyValueMap> {
        logger::development_trace(
           "Parsing file {}"
          ,this->m_persistent_file_path.string()
        );
        KeyValueMap key_values{};
        std::istringstream iss{csv_string};
        std::string line{};
        while (std::getline(iss,line)) {
          logger::development_trace(
            "Parsing line '{}'"
            ,line
          );

          Key::Sequence fields{line,','}; // Expect framework.cvs to use ',' as delimiters
          if (fields.size() == 2) {
            key_values.insert(fields[0],fields[1]);            
          }
          else {
            logger::business(
              "key value file {} entry '{}' does not define a valid key-value-pair"
              ,this->m_persistent_file_path.string()
              ,line);
          }
        }

        if (key_values.size() > 0) return key_values;

        return std::nullopt;
      });

      if (maybe_key_values) {
      }
      else {
        logger::business("digest_persistent_file failed to digest any valid key-value-pairs");
      }

  }
  catch (std::exception& e) {
    logger::design_insufficiency("Framework::digest_persistent_file Failed. Exception:{}",e.what());
  }
  catch (...) {
    logger::design_insufficiency("Framework::digest_persistent_file Failed. General Exception");
  }
  return false;
}
