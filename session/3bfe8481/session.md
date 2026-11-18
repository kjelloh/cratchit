# Consider to merge cratchit from plaiground session/029b12e4 into this repo and possibly in the same codebase and tool chain?

```#3bfe8481```

## 20261118

I now have a vibe coded cratchit in my github plaiground repo session/029b12e4.

I will call plaiground repo session/029b12e4 cratchit for playground cratchit. 

I will call current cratchit here for legacy cratchit.

I want playground cratchit to now move here.

* In the best case as part of the same code base and build system.
* In the very least in its own self-contained folder.

Now, to have it live as part of our code base and tool chain there are some issues to attend to.

* Github workflows issues
  * playground cratchit builds cross platform on Github with workflows.
  * Legacy cratchit builds local on macOS only.

* Cross platform issues
  * Plaiground session/029b12e4 is cross platform
  * Current cratchit here builds locally on macOS only.

* CLI argument issues
  * Plaiground cratchit has a CLI-mode triggered by a 'mode' argument (see --pdf-walk, --sie-to-ar etc.,)
  * Legacy cratchit has cli modes 'zeroth','first' and 'second'

* cmake install issues
  * Plaiground cratchit can be installed with cmake install mechanism
  * Legacy cratchit runs locally suing custom run-script.

But maybe Clade can help me merge these two code bases into this same code base and repo?