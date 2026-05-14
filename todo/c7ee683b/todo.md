# Consider to clean upp the confusing test fixture for SIE file handling at startup and shutdown?

There is still a lot of confusion and unclearity regarding how the SIE mechanism for posted (shared,base) vs staged (mutated, current) SIE files should be design and work.

I have a hard time getting to grips with what I want to aim for, what the current mechanism actually use and why?

* Should I remove the virtual file system and always process files from disk?
* Or should I commit to the virtual file system apporach (but then how do I make it understandable and actually usefull)?

For now the following confusions can be identified:

* We have two indirections
  + Environment: sie year-id to sie file name (posted,base,shared sie file)
  + Virtual file system: sie file name to istream
* We have ModelTestsFixture actually writing a 'posted' SIE file to disk
  - But no test actually reads this file
  - And ModelTestsFixture never creates any corresponding 'staged' (edited by cratchit) sie file

In more detail we have:

* The mechanism uses a 'virtual file system' to map SIE file names to istream access
  - This is used to enable tests to redirect model_from_environment_and_filesystem_ifc to test files instead of IRL files
* The ModelTestsFixture creates m_fixture_temp_dir / "sie_in" / "TheITfiedAB20250812_145743.se" from sz_TheITfiedAB20250812_145743_se
  - But that file is never read not used by any test?
  - All tests seems to still use 'in memory' SIE files cerated from sz-strings?
* The TestCratchitFileSystemDefacto creates a virtual (in memory) file system with SIE files from sz-strings
* The to_test_filesystem_defacto_ifc takes two sz-strings and returns an TestCratchitFileSystemDefacto instance (unique ptr)
  - This instance hard codes 'TheITfiedAB20250812_145743.se' and 'cratchit_2025-05-01_2026-04-30.se' file names
  - But reads SIE content from memory (never from actual file)
* The Environment2ModelTest maps sied ID "current" to ModelTestsFixture::m_fixture_sie_file_path
  - But it then uses a virtual (in memory) file system as ifc to the file names

```c++
        MDCratchitFileSystemIfc md_filesystem_ifc{
          .meta = {.m_root_path = "*test dummy path*"}
          ,.defacto = to_test_filesystem_defacto_ifc(
             szThreePostedSIE
            ,szOneNewStagedSIE)
        };

        auto model = ::zeroth::model_from_environment_and_filesystem_ifc(environment,md_filesystem_ifc);

```

* The model_from_environment_and_filesystem_ifc takes an envornment and a 'file system interface' to populate a model with SIE data
  - The Environment maps sie-year-id to SIE file name (the posted, base,shared)
  - And then uses file system interface to get an istream to the file with provided name
  - This is just overkill and confusing?
  - Better to always read as ordinary file?
  - But then, how do we redirect testing to another root folder to have somewhere to generate test SIE files? 