# Attend to Github Pages build error 20260417

I get Github Build error mail eah time I push to the repo.

* 'Run failed: pages build and deployment - master (3488e34)'
* 'pages build and deployment: Some jobs were not successful'

The mail contains a button 'View workflow run'. In the web Ux I can see:

```text
Annotations
1 error and 1 warning
build
 Logging at level: debug Configuration file: /github/workspace/./_config.yml Theme: jekyll-theme-cayman Theme source: /usr/local/bundle/gems/jekyll-theme-cayman-0.2.0 GitHub Pages: github-pages v232 GitHub Pages: jekyll v3.10.0 Theme: jekyll-theme-cayman Theme source: /usr/local/bundle/gems/jekyll-theme-cayman-0.2.0 Requiring: jekyll-seo-tag Requiring: jekyll-coffeescript Requiring: jekyll-commonmark-ghpages Requiring: jekyll-gist Requiring: jekyll-github-metadata Requiring: jekyll-paginate Requiring: jekyll-relative-links Requiring: jekyll-optional-front-matter Requiring: jekyll-readme-index Requiring: jekyll-default-layout Requiring: jekyll-titles-from-headings GitHub Metadata: Initializing... Source: /github/workspace/. Destination: /github/workspace/./_site Incremental build: disabled. Enable with --incremental Generating... Generating: JekyllOptionalFrontMatter::Generator finished in 0.007346235 seconds. github-pages 232 | Error: Invalid scheme format: 'What is missing to close branch origin' 
build
Node.js 20 actions are deprecated. The following actions are running on Node.js 20 and may not work as expected: actions/checkout@v4. Actions will be forced to run with Node.js 24 by default starting June 2nd, 2026. Node.js 20 will be removed from the runner on September 16th, 2026. Please check if updated versions of these actions are available that support Node.js 24. To opt into Node.js 24 now, set the FORCE_JAVASCRIPT_ACTIONS_TO_NODE24=true environment variable on the runner or in your workflow file. Once Node.js 24 becomes the default, you can temporarily opt out by setting ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION=true. For more information see: https://github.blog/changelog/2025-09-19-deprecation-of-node-20-on-github-actions-runners/
```

* What is 'Invalid scheme format'?

I asked Github Copilot and was told to edit _config.yml:

```yml
theme: jekyll-theme-cayman
title: Cratchit
description: A Swedish C++ accounting SIE file based application
url: https://kjelloh.github.io
baseurl: /cratchit
repository: kjelloh/cratchit
```

Lets try!