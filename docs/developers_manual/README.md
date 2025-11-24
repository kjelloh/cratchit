# cratchit developers manual

* Also see AI assistance [ai/README.md](ai/README.md)

## git command reference (good to remember)

To get a 'proper' visual feed back how branches relates to master the 'git log' with '--graph' seems to do the job and show branches 'indented' in paralell with 'matsre' as I like to have it visualised?

```sh
git log --oneline --graph --decorate --all
```

## macOS BUILD issues

At times it seems the tool-chain builds the binary with an unacceptable 'code sign'?

Youy can check for this pn macOS in zsh-shell by running cratchit through the lldb debugger.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % lldb workspace/cratchit
(lldb) target create "workspace/cratchit"
Current executable set to '/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/cratchit' (arm64).
(lldb) run
Process 90231 launched: '/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/workspace/cratchit' (arm64)
Process 90231 exited with status = 9 (0x00000009) Terminated due to code signing error
(lldb) quit
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit %
```

In case the error is 'Terminated due to code signing error' you can generate an acceptable one with the macOS code signing tool.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit % codesign -s - --force --deep workspace/cratchit

workspace/cratchit: replacing existing signature
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/cratchit %
```

Now macOS should allow the cratchit binary to execute again (fingers crossed).

Note: As of 20251103 I do not know exactly when and why this happens.

## SIE file handling

An SIE file contains registered financial events using Swedish BAS framework.

The cratchit app models and SIE file content with the SIEEnvironment class.

```cpp 
class SIEEnvironment {...}
```

As of this writing cratchit variant 'zeroth' handling of financial year is rudimentary and lacking in many ways.

* The envrionment stores one instance of the tag/type sie_file as an array of paths to imported sie-files

```sh
sie_file:0 "-1=sie_in/TheITfiedAB20250211_145723.se;current=sie_in/TheITfiedAB20250510_190029.se"
```

* The 'current' (also index 0) financial year is NOT checked nor 'shifted' when a new year starts.
* The cratchit app does NOT check if a registered financial event has a date withing the current fiscal year.
* The cratchit app keeps a single 'cratchit.se' SIE-file with staged entries. These are entries that are not in the corresponding imported SIE-file for the same 'year key'.
* The 'model->sie[year_key]' contains the SIE file contents for year key 'current',-1','-2',...
* The cratchit app reads sie-files into the model with the code:

```cpp
if (environment.contains("sie_file")) {
  auto const id_ev_pairs = environment.at("sie_file");
  if (id_ev_pairs.size() > 1) {
    std::cout << "\nDESIGN_INSUFFICIENCY: Expected at most one but found " << id_ev_pairs.size() << " 'sie_file' entries in environment file";
  }
  for (auto const& id_ev_pair : id_ev_pairs) {
    auto const& [id,ev] = id_ev_pair;
    for (auto const& [year_key,sie_file_name] : ev) {
      std::filesystem::path sie_file_path{sie_file_name};
      if (auto sie_environment = from_sie_file(sie_file_path)) {
        model->sie[year_key] = std::move(*sie_environment);
        model->sie_file_path[year_key] = {sie_file_name};
        prompt << "\nsie[" << year_key << "] from " << sie_file_path;
      }
      else {
        prompt << "\nsie[" << year_key << "] from " << sie_file_path << " - *FAILED*";
      }
    }
  }
}
```

* There is only one 'cratchit.se' to store new financial entries that are not in the imported sie-file for 'current'.

### SIE files indexed by financial year?

Maybe it would be better to organise SIE files indexed by an actual financial year?

20250806 - Started refactoring from SIEEnvironments_X -> SIEEnvironments_R. Where 'X' maps from int 0,-1,-2... relative index to SIEEnvironment and 'R maps Date -> SIEEnvironment (Date being forst day of financial year)
