# Contributing to rxtxcpu

If you're reading this, it seems likely you're interested in contributing to rxtxcpu. We think that's awesome! Below are some guidelines to help you get started along with some expectations and tips to make the process easier.

Bug reports and pull requests (PRs) are welcome.

Before coding any new features or substantial changes, please [open an issue](https://github.com/stackpath/rxtxcpu/issues/new) to discuss the proposed changes and benefits provided to the project.

PRs comprised of whitespace fixes, code formatting changes, or other purely cosmetic changes are unlikely to be merged.

## Bugs

**Do not open a GitHub issue if the bug is a security vulnerability;** instead, email security@stackpath.com.

Before opening an issue, ensure the bug hasn't [already been reported](https://github.com/stackpath/rxtxcpu/issues). If not, [open a new issue](https://github.com/stackpath/rxtxcpu/issues/new) with details demonstrating the unexpected behavior.

PRs are welcome if you've written a patch for a bug.

## Pull request guidelines

Please ensure the following when submitting a pull request against the project.

* A single commit should implement a single logical change.
* Code should match the surrounding style conventions.
* There should be no trailing or excessive whitespace.
* There should be no commented out code or unneeded files.
* Tests should be included for bug fixes and feature additions.
* Documentation should be updated or extended for behavior changes or feature additions.
* The commit message should be meaningful.

For example, here is a meaningful commit message.
```
Call pthread_attr_destory() to fix memory leak.
```
And now for an example of what not to do; the same change could have a commit message like this, but the message is of little value.
```
Update manager.c.
```

## Opening a pull request

The recommended process for opening a PR against this project is as follows.

* Fork the project.
* Clone your fork.
* Create a feature branch within your fork to be used for this PR.
* Make your code updates following the guidelines above.
* Ensure all tests are passing.
* Commit your changes.
* Push your feature branch to your fork.
* Open a pull request for the feature branch in your fork against the main project.

For further assistance in opening a PR, see the [GitHub PR documentation](https://help.github.com/articles/about-pull-requests/).
