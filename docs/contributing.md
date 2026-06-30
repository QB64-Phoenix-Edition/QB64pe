Contributing to QB64-PE
=======================

Everybody is invited to contribute to QB64-PE via GitHub pull requests. However, as QB64-PE is maintained by an international Team and also everybody seems to prefer different source layout and formatting we've some simple rules which every contributor should adhere to.

1. Generally we use the english language (us-en) in this repository and in the QB64-PE sources. It must not be perfect in grammar and even a translation from Google or any AI is in most cases better than spicing the sources with comments in your native language.
2. When you make changes to the BASIC code using the IDE of QB64-PE, then please use the menu Options > Code Layout to activate all (Line Indent, SUB/FUNC Indent, Single spacing), the Indent spacing should be 4 and UPPER for keywords.
3. In general you should follow the style you see in the source files so that the diffs become clean and readable and not getting cluttered by needless whitespace and/or keyword case changes.

Requirements
------------

Formerly people simply forked the QB64pe repository, made their changes and then did a pull request right from their fork back into the parent repository.

**This practice does no longer work reliably!**

Since our PR/CI workflows do fetch data from our Website `qb64phoenix.com`, and this is behind **Cloudflare** now, we've to do some extra steps to configure CF properly. This requires repository secrets to be used which are not accessible when making a pull request from a fork.

The only possible way to access these secrets and guarantee proper workflow behavior is to run the pull request in the context of the original repository.

Hence, instead of making yourself a new fork, rather clone the original repository using `GitHub Desktop` or the `git` commandline tool, then create a new branch and do your changes there, finally publish the new branch with your changes and make a pull request from that new branch.
