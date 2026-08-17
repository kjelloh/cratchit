# Consider a git-only based C++ package manager to replace conan?

## 20260817

From todays thinking.

4. Do something about the now convoluted MESS of a build system for cratchit?
  1. Back to basics as direct calls to compiler (remove cmake layer)?
  2. Remove conan package manager and replace with some automated git-clone any dependancies?
  3. Make a compiler-wrapper to control code-to-binary and also format compiler error outputs?

For 4.2 I imagine we can get quite far to base a package manager on git-clone-available stuff only?

* For one the git-clone paradihm is very well spread.
* So we can expect most C++ depenadncies can bo 'solved' by git-cloning' a repo from somewhere?
* Many such repos also comes wioth cmake build-ability?
  * So we can consider to make this package manager build-from-source able?

So how ca we imagine the API and/or user interaction to look like?

* Imagine we have identified a dependancy.
* What is it we then know and have to resolve?
  * It can be that we recognise an '#include' in our C++ source?
    * And we 'know' how to map this path to a git-repo + a relative path into that repo?
    * So we can clone this repo and 'refer' the path to a path into the cloned repo? 
  * It can be that if we resolve an '#include', we suspect wer also need to link the ascosiated binary?
    * Now if the cloned repo comes with cmake-support we can in fact build it?
    * And the populate the link-phase with the apropriate build binary?
    * Now problem is then that we may need to interrogate the built binaries fo the one we need?
    * That is, there either be many compiled compilation untis as a-files?
    * And we need to link the one with the code ascosiated with the #include and actual call in the code.
    * And then we may also need to link all the a-files on which the linked a-file depends?

Ok, this can grow quite rapidly.

* But it seems each step in the process is in fact known for a C++ compile and build process?
* And all tool chains on all platfporms provide tooling to process source code files and interrogate built binaris?
* So we should be able to piece together 'combinators' to figure everything out in a predictable way?

So what do we have on the top-level?

* We can imagine one function is to provide a git-repo-path and have the package-manager clone it to some 'cache'?

  * Maybe a clean and easy to understand way is to create the 'cahche' in a '.xxx' local folder?
  * I imagine if we automatically place the '.xxx' folder in the root of the current git-repo we will have a clean behaviour?
  * Then we require the git-based package manager to always operate in a git-repo?

* I imagine this use-git-repo-as-package operation shlud be usable manually by the user?

  * So we need a name for this console app?
  * Then we can provide the user to do: '>xxx_use_as_package('some_git_repo_url')'?

Maybe at this stage I need a name for this 'git-clone-based-package-manager'.

* I kind-of like 'C++ Habilis' as (the skillfull C++)
  * This is a riff on 'Homo Habilis' as 'the skillfull man'
* But it could be worth exploring other latin constructs?

  * Homo instrumentis utens — “man using tools/instruments”, according to chatGPT (needs verification)

```text
Homo instrumentis utens — “man using tools/instruments”
  homo = man/human
  instrumentis = with tools/instruments
  utens = using
```

  * Homo instrumentorum usor — “man who uses tools” (also chatGPT = needs verification)

```text
Homo instrumentorum usor — “man who uses tools”
  instrumentorum = of tools
  usor = user
```

Hm, I need to think more about this?